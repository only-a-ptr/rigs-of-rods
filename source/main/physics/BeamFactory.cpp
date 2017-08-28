/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   24th of August 2009

#include "BeamFactory.h"

#include "Application.h"
#include "BeamEngine.h"
#include "BeamStats.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "DynamicCollisions.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "MainMenu.h"

#include "Network.h"
#include "PointColDetector.h"
#include "RigLoadingProfiler.h"
#include "RigLoadingProfilerControl.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "ThreadPool.h"
#include "Utils.h"
#include "VehicleAI.h"

#ifdef _GNU_SOURCE
#include <sys/sysinfo.h>
#endif

#if defined(__APPLE__) || defined (__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif // __APPLE__ || __FREEBSD__

#include "DashBoardManager.h"

#include <algorithm>
#include <cstring>
#include <unordered_set>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
  #include <intrin.h>
#endif

using namespace Ogre;

void cpuID(unsigned i, unsigned regs[4])
{
#ifdef _WIN32
    __cpuid((int *)regs, (int)i);
#elif defined(__x86_64__) || defined(__i386)
    asm volatile
        ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
         : "a" (i), "c" (0));
#endif
}

static unsigned hardware_concurrency()
{
#if defined(_GNU_SOURCE)
        return get_nprocs();
#elif defined(__APPLE__) || defined(__FreeBSD__)
        int count;
        size_t size = sizeof(count);
        return sysctlbyname("hw.ncpu", &count, &size, NULL, 0) ? 0 : count;
#elif defined(BOOST_HAS_UNISTD_H) && defined(_SC_NPROCESSORS_ONLN)
        int const count = sysconf(_SC_NPROCESSORS_ONLN);
        return (count > 0) ? count : 0;
#else
    return std::thread::hardware_concurrency();
#endif
}

unsigned int getNumberOfCPUCores()
{
#if defined(_WIN32) || defined(__x86_64__) || defined(__i386)
    unsigned regs[4];

    // Get CPU vendor
    char vendor[12];
    cpuID(0, regs);
    memcpy((void *)vendor,     (void *)&regs[1], 4); // EBX
    memcpy((void *)&vendor[4], (void *)&regs[3], 4); // EDX
    memcpy((void *)&vendor[8], (void *)&regs[2], 4); // ECX
    std::string cpuVendor = std::string(vendor, 12);

    // Get CPU features
    cpuID(1, regs);
    unsigned cpuFeatures = regs[3]; // EDX

    // Logical core count per CPU
    cpuID(1, regs);
    unsigned logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
    unsigned cores = logical;

    if (cpuVendor == "GenuineIntel")
    {
        // Get DCP cache info
        cpuID(4, regs);
        cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
    }
    else if (cpuVendor == "AuthenticAMD")
    {
        // Get NC: Number of CPU cores - 1
        cpuID(0x80000008, regs);
        cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
    }

    // Detect hyper-threads
    bool hyperThreads = cpuFeatures & (1 << 28) && cores < logical;

    LOG("BEAMFACTORY: " + TOSTRING(logical) + " logical CPU cores" + " found");
    LOG("BEAMFACTORY: " + TOSTRING(cores) + " CPU cores" + " found");
    LOG("BEAMFACTORY: Hyper-Threading " + TOSTRING(hyperThreads));
#else
    unsigned cores = hardware_concurrency();
    LOG("BEAMFACTORY: " + TOSTRING(cores) + " CPU cores" + " found");
#endif
    return cores;
}

RoR::BeamFactory::BeamFactory(RoRFrameListener* sim_controller)
    : m_current_truck(-1)
    , m_dt_remainder(0.0f)
    , m_forced_active(false)
    , m_free_truck(0)
    , m_num_cpu_cores(0)
    , m_physics_frames(0)
    , m_physics_steps(2000)
    , m_previous_truck(-1)
    , m_simulated_truck(0)
    , m_simulation_speed(1.0f)
    , m_sim_controller(sim_controller)
{
    memset(m_trucks, 0, MAX_TRUCKS * sizeof(void*));

    if (RoR::App::GetAppMultithread())
    {
        // Create thread pool
        int numThreadsInPool = ISETTING("NumThreadsInThreadPool", 0);

        if (numThreadsInPool > 1)
        {
            m_num_cpu_cores = numThreadsInPool;
        }
        else
        {
            int logical_cpus = hardware_concurrency();
            int physical_cpus = getNumberOfCPUCores();

            if (physical_cpus < 6 && logical_cpus > physical_cpus)
                m_num_cpu_cores = logical_cpus - 1;
            else
                m_num_cpu_cores = physical_cpus - 1;
        }

        bool disableThreadPool = BSETTING("DisableThreadPool", false);

        if (m_num_cpu_cores < 2)
        {
            disableThreadPool = true;
            LOG("BEAMFACTORY: Not enough CPU cores to enable the thread pool");
        }
        else if (!disableThreadPool)
        {
            gEnv->threadPool = new ThreadPool(m_num_cpu_cores);
            LOG("BEAMFACTORY: Creating " + TOSTRING(m_num_cpu_cores) + " threads");
        }

        // Create worker thread (used for physics calculations)
        m_sim_thread_pool = std::unique_ptr<ThreadPool>(new ThreadPool(1));
    }
}

RoR::BeamFactory::~BeamFactory()
{
    this->SyncWithSimThread(); // Wait for sim task to finish
    delete gEnv->threadPool;
    m_particle_manager.DustManDiscard(gEnv->sceneManager); // TODO: de-globalize SceneManager
}

#define LOADRIG_PROFILER_CHECKPOINT(ENTRY_ID) rig_loading_profiler.Checkpoint(RoR::RigLoadingProfiler::ENTRY_ID);

Beam* RoR::BeamFactory::CreateLocalRigInstance(
    Ogre::Vector3 pos,
    Ogre::Quaternion rot,
    Ogre::String fname,
    int cache_entry_number, // = -1,
    collision_box_t* spawnbox /* = nullptr */,
    bool ismachine /* = false */,
    const std::vector<Ogre::String>* truckconfig /* = nullptr */,
    RoR::SkinDef* skin /* = nullptr */,
    bool freePosition, /* = false */
    bool preloaded_with_terrain /* = false */
)
{
    RoR::RigLoadingProfiler rig_loading_profiler;
#ifdef ROR_PROFILE_RIG_LOADING
    ::Profiler::reset();
#endif

    int truck_num = this->GetFreeTruckSlot();
    if (truck_num == -1)
    {
        LOG("ERROR: Could not add beam to main list");
        return 0;
    }

    Beam* b = new Beam(
        m_sim_controller,
        truck_num,
        pos,
        rot,
        fname.c_str(),
        &rig_loading_profiler,
        false, // networked
        (RoR::App::GetActiveMpState() == RoR::App::MP_STATE_CONNECTED), // networking
        spawnbox,
        ismachine,
        truckconfig,
        skin,
        freePosition,
        preloaded_with_terrain,
        cache_entry_number
    );

    if (b->state == INVALID)
    {
        this->DeleteTruck(b);
        return nullptr;
    }

    m_trucks[truck_num] = b;

    // lock slide nodes after spawning the truck?
    if (b->getSlideNodesLockInstant())
    {
        b->toggleSlideNodeLock();
    }

    RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();

    // add own username to truck
    if (RoR::App::GetActiveMpState() == RoR::App::MP_STATE_CONNECTED)
    {
        b->updateNetworkInfo();
    }
    LOADRIG_PROFILER_CHECKPOINT(ENTRY_BEAMFACTORY_CREATELOCAL_POSTPROCESS);

    LOG(rig_loading_profiler.Report());

#ifdef ROR_PROFILE_RIG_LOADING
    std::string out_path = RoR::App::GetSysUserDir() + PATH_SLASH + "profiler" + PATH_SLASH + ROR_PROFILE_RIG_LOADING_OUTFILE;
    ::Profiler::DumpHtml(out_path.c_str());
#endif
    return b;
}

#undef LOADRIG_PROFILER_CHECKPOINT

int RoR::BeamFactory::CreateRemoteInstance(RoRnet::TruckStreamRegister* reg)
{
    LOG(" new beam truck for " + TOSTRING(reg->origin_sourceid) + ":" + TOSTRING(reg->origin_streamid));

#ifdef USE_SOCKETW
    RoRnet::UserInfo info;
    RoR::Networking::GetUserInfo(reg->origin_sourceid, info);

    UTFString message = RoR::ChatSystem::GetColouredName(info.username, info.colournum) + RoR::Color::CommandColour + _L(" spawned a new vehicle: ") + RoR::Color::NormalColour + reg->name;
    RoR::App::GetGuiManager()->pushMessageChatBox(message);
#endif // USE_SOCKETW

    // check if we got this truck installed
    String filename = String(reg->name);
    String group = "";
    if (!RoR::App::GetCacheSystem()->checkResourceLoaded(filename, group))
    {
        LOG("wont add remote stream (truck not existing): '"+filename+"'");
        return -1;
    }

    // fill truckconfig
    std::vector<String> truckconfig;
    for (int t = 0; t < 10; t++)
    {
        if (!strnlen(reg->truckconfig[t], 60))
            break;
        truckconfig.push_back(String(reg->truckconfig[t]));
    }

    // DO NOT spawn the truck far off anywhere
    // the truck parsing will break flexbodies initialization when using huge numbers here
    Vector3 pos = Vector3::ZERO;

    int truck_num = this->GetFreeTruckSlot();
    if (truck_num == -1)
    {
        LOG("ERROR: could not add beam to main list");
        return -1;
    }
    RoR::RigLoadingProfiler p; // TODO: Placeholder. Use it
    Beam* b = new Beam(
        m_sim_controller,
        truck_num,
        pos,
        Quaternion::ZERO,
        reg->name,
        &p,
        true, // networked
        (RoR::App::GetActiveMpState() == RoR::App::MP_STATE_CONNECTED), // networking
        nullptr, // spawnbox
        false, // ismachine
        &truckconfig,
        nullptr // skin
    );

    if (b->state == INVALID)
    {
        this->DeleteTruck(b);
        return -1;
    }
    m_trucks[truck_num] = b;

    b->m_source_id = reg->origin_sourceid;
    b->m_stream_id = reg->origin_streamid;
    b->updateNetworkInfo();


    RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();


    return 1;
}

void RoR::BeamFactory::RemoveStreamSource(int sourceid)
{
    m_stream_mismatches.erase(sourceid);

    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        if (m_trucks[t]->state != NETWORKED)
            continue;

        if (m_trucks[t]->m_source_id == sourceid)
        {
            this->DeleteTruck(m_trucks[t]);
        }
    }
}

#ifdef USE_SOCKETW
void RoR::BeamFactory::handleStreamData(std::vector<RoR::Networking::recv_packet_t> packet_buffer)
{
    for (auto packet : packet_buffer)
    {
        if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            if (reg->type == 0)
            {
                reg->status = this->CreateRemoteInstance((RoRnet::TruckStreamRegister *)packet.buffer);
                RoR::Networking::AddPacket(0, RoRnet::MSG2_STREAM_REGISTER_RESULT, sizeof(RoRnet::StreamRegister), (char *)reg);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER_RESULT)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            for (int t = 0; t < m_free_truck; t++)
            {
                if (!m_trucks[t])
                    continue;
                if (m_trucks[t]->state == NETWORKED)
                    continue;
                if (m_trucks[t]->m_stream_id == reg->origin_streamid)
                {
                    int sourceid = packet.header.source;
                    m_trucks[t]->m_stream_results[sourceid] = reg->status;

                    if (reg->status == 1)
                    LOG("Client " + TOSTRING(sourceid) + " successfully loaded stream " + TOSTRING(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));
                    else
                    LOG("Client " + TOSTRING(sourceid) + " could not load stream " + TOSTRING(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));

                    break;
                }
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_UNREGISTER)
        {
            Beam* b = this->getBeam(packet.header.source, packet.header.streamid);
            if (b && b->state == NETWORKED)
            {
                this->DeleteTruck(b);
            }
            auto search = m_stream_mismatches.find(packet.header.source);
            if (search != m_stream_mismatches.end())
            {
                auto& mismatches = search->second;
                auto it = std::find(mismatches.begin(), mismatches.end(), packet.header.streamid);
                if (it != mismatches.end())
                    mismatches.erase(it);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_USER_LEAVE)
        {
            this->RemoveStreamSource(packet.header.source);
        }
        else
        {
            for (int t = 0; t < m_free_truck; t++)
            {
                if (!m_trucks[t])
                    continue;
                if (m_trucks[t]->state != NETWORKED)
                    continue;

                m_trucks[t]->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, packet.buffer, packet.header.size);
            }
        }
    }
}
#endif // USE_SOCKETW

int RoR::BeamFactory::checkStreamsOK(int sourceid)
{
    if (m_stream_mismatches[sourceid].size() > 0)
        return 0;

    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        if (m_trucks[t]->state != NETWORKED)
            continue;

        if (m_trucks[t]->m_source_id == sourceid)
        {
            return 1;
        }
    }

    return 2;
}

int RoR::BeamFactory::checkStreamsRemoteOK(int sourceid)
{
    int result = 2;

    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        if (m_trucks[t]->state == NETWORKED)
            continue;

        int stream_result = m_trucks[t]->m_stream_results[sourceid];
        if (stream_result == -1)
            return 0;
        if (stream_result == 1)
            result = 1;
    }

    return result;
}

Beam* RoR::BeamFactory::getBeam(int source_id, int stream_id)
{
    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        if (m_trucks[t]->state != NETWORKED)
            continue;

        if (m_trucks[t]->m_source_id == source_id && m_trucks[t]->m_stream_id == stream_id)
        {
            return m_trucks[t];
        }
    }

    return nullptr;
}

bool RoR::BeamFactory::intersectionAABB(Ogre::AxisAlignedBox a, Ogre::AxisAlignedBox b, float scale)
{
    if (scale != 1.0f)
    {
        Vector3 a_center = a.getCenter();
        Vector3 a_half_size = a.getHalfSize();
        a.setMaximum(a_center + a_half_size * scale);
        a.setMinimum(a_center - a_half_size * scale);

        Vector3 b_center = b.getCenter();
        Vector3 b_half_size = b.getHalfSize();
        b.setMaximum(b_center + b_half_size * scale);
        b.setMinimum(b_center - b_half_size * scale);
    }

    return a.intersects(b);
}

bool RoR::BeamFactory::truckIntersectionAABB(int a, int b, float scale)
{
    return intersectionAABB(m_trucks[a]->boundingBox, m_trucks[b]->boundingBox, scale);
}

bool RoR::BeamFactory::predictTruckIntersectionAABB(int a, int b, float scale)
{
    return intersectionAABB(m_trucks[a]->predictedBoundingBox, m_trucks[b]->predictedBoundingBox, scale);
}

bool RoR::BeamFactory::truckIntersectionCollAABB(int a, int b, float scale)
{
    if (m_trucks[a]->collisionBoundingBoxes.empty() && m_trucks[b]->collisionBoundingBoxes.empty())
    {
        return truckIntersectionAABB(a, b, scale);
    }
    else if (m_trucks[a]->collisionBoundingBoxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_trucks[b]->collisionBoundingBoxes.begin(); it != m_trucks[b]->collisionBoundingBoxes.end(); ++it)
            if (intersectionAABB(*it, m_trucks[a]->boundingBox, scale))
                return true;
    }
    else if (m_trucks[b]->collisionBoundingBoxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_trucks[a]->collisionBoundingBoxes.begin(); it != m_trucks[a]->collisionBoundingBoxes.end(); ++it)
            if (intersectionAABB(*it, m_trucks[b]->boundingBox, scale))
                return true;
    }
    else
    {
        for (std::vector<AxisAlignedBox>::iterator it_a = m_trucks[a]->collisionBoundingBoxes.begin(); it_a != m_trucks[a]->collisionBoundingBoxes.end(); ++it_a)
            for (std::vector<AxisAlignedBox>::iterator it_b = m_trucks[b]->collisionBoundingBoxes.begin(); it_b != m_trucks[b]->collisionBoundingBoxes.end(); ++it_b)
                if (intersectionAABB(*it_a, *it_b, scale))
                    return true;
    }

    return false;
}

bool RoR::BeamFactory::predictTruckIntersectionCollAABB(int a, int b, float scale)
{
    if (m_trucks[a]->predictedCollisionBoundingBoxes.empty() && m_trucks[b]->predictedCollisionBoundingBoxes.empty())
    {
        return predictTruckIntersectionAABB(a, b, scale);
    }
    else if (m_trucks[a]->predictedCollisionBoundingBoxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_trucks[b]->predictedCollisionBoundingBoxes.begin(); it != m_trucks[b]->predictedCollisionBoundingBoxes.end(); ++it)
            if (intersectionAABB(*it, m_trucks[a]->predictedBoundingBox, scale))
                return true;
    }
    else if (m_trucks[b]->predictedCollisionBoundingBoxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_trucks[a]->predictedCollisionBoundingBoxes.begin(); it != m_trucks[a]->predictedCollisionBoundingBoxes.end(); ++it)
            if (intersectionAABB(*it, m_trucks[b]->predictedBoundingBox, scale))
                return true;
    }
    else
    {
        for (std::vector<AxisAlignedBox>::iterator it_a = m_trucks[a]->predictedCollisionBoundingBoxes.begin(); it_a != m_trucks[a]->predictedCollisionBoundingBoxes.end(); ++it_a)
            for (std::vector<AxisAlignedBox>::iterator it_b = m_trucks[b]->predictedCollisionBoundingBoxes.begin(); it_b != m_trucks[b]->predictedCollisionBoundingBoxes.end(); ++it_b)
                if (intersectionAABB(*it_a, *it_b, scale))
                    return true;
    }

    return false;
}

void RoR::BeamFactory::RecursiveActivation(int j, std::bitset<MAX_TRUCKS>& visited)
{
    if (visited[j] || !m_trucks[j] || m_trucks[j]->state != SIMULATED)
        return;

    visited.set(j, true);

    for (int t = 0; t < m_free_truck; t++)
    {
        if (t == j || !m_trucks[t] || visited[t])
            continue;
        if (m_trucks[t]->state == SIMULATED && truckIntersectionCollAABB(t, j, 1.2f))
        {
            m_trucks[t]->sleeptime = 0.0f;
            this->RecursiveActivation(t, visited);
        }
        if (m_trucks[t]->state == SLEEPING && predictTruckIntersectionCollAABB(t, j))
        {
            m_trucks[t]->sleeptime = 0.0f;
            m_trucks[t]->state = SIMULATED;
            this->RecursiveActivation(t, visited);
        }
    }
}

void RoR::BeamFactory::UpdateSleepingState(float dt)
{
    if (!m_forced_active)
    {
        for (int t = 0; t < m_free_truck; t++)
        {
            if (!m_trucks[t])
                continue;
            if (m_trucks[t]->state != SIMULATED)
                continue;
            if (m_trucks[t]->getVelocity().squaredLength() > 0.01f)
                continue;

            m_trucks[t]->sleeptime += dt;

            if (m_trucks[t]->sleeptime >= 10.0f)
            {
                m_trucks[t]->state = SLEEPING;
            }
        }
    }

    Beam* current_truck = getCurrentTruck();
    if (current_truck && current_truck->state == SLEEPING)
    {
        current_truck->state = SIMULATED;
    }

    std::bitset<MAX_TRUCKS> visited;
    // Recursivly activate all trucks which can be reached from current_truck
    if (current_truck && current_truck->state == SIMULATED)
    {
        current_truck->sleeptime = 0.0f;
        this->RecursiveActivation(m_current_truck, visited);
    }
    // Snowball effect (activate all trucks which might soon get hit by a moving truck)
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->state == SIMULATED && m_trucks[t]->sleeptime == 0.0f)
            this->RecursiveActivation(t, visited);
    }
}

int RoR::BeamFactory::GetFreeTruckSlot()
{
    // find a free slot for the truck
    for (int t = 0; t < MAX_TRUCKS; t++)
    {
        if (!m_trucks[t] && t >= m_free_truck) // XXX: TODO: remove this hack
        {
            // reuse slots
            if (t >= m_free_truck)
                m_free_truck = t + 1;
            return t;
        }
    }
    return -1;
}

void RoR::BeamFactory::activateAllTrucks()
{
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->state == SLEEPING)
        {
            m_trucks[t]->state = SIMULATED;
            m_trucks[t]->sleeptime = 0.0f;

            if (this->getTruck(m_simulated_truck))
            {
                m_trucks[t]->disableDrag = this->getTruck(m_simulated_truck)->driveable == AIRPLANE;
            }
        }
    }
}

void RoR::BeamFactory::sendAllTrucksSleeping()
{
    m_forced_active = false;
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->state == SIMULATED)
        {
            m_trucks[t]->state = SLEEPING;
        }
    }
}

void RoR::BeamFactory::recalcGravityMasses()
{
    // update the mass of all trucks
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t])
        {
            m_trucks[t]->recalc_masses();
        }
    }
}

int RoR::BeamFactory::FindTruckInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box)
{
    // try to find the desired truck (the one in the box)
    int id = -1;
    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        if (collisions->isInside(m_trucks[t]->nodes[0].AbsPosition, inst, box))
        {
            if (id == -1)
            // first truck found
                id = t;
            else
            // second truck found -> unclear which vehicle was meant
                return -1;
        }
    }
    return id;
}

void RoR::BeamFactory::repairTruck(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition)
{
    int rtruck = this->FindTruckInsideBox(collisions, inst, box);
    if (rtruck >= 0)
    {
        // take a position reference
#ifdef USE_OPENAL
        SoundScriptManager::getSingleton().trigOnce(rtruck, SS_TRIG_REPAIR);
#endif // USE_OPENAL
        Vector3 ipos = m_trucks[rtruck]->nodes[0].AbsPosition;
        m_trucks[rtruck]->reset();
        m_trucks[rtruck]->resetPosition(ipos.x, ipos.z, false, 0);
        m_trucks[rtruck]->updateVisual();
    }
}

void RoR::BeamFactory::MuteAllTrucks()
{
    for (int i = 0; i < m_free_truck; i++)
    {
        if (m_trucks[i])
        {
            m_trucks[i]->StopAllSounds();
        }
    }
}

void RoR::BeamFactory::UnmuteAllTrucks()
{
    for (int i = 0; i < m_free_truck; i++)
    {
        if (m_trucks[i])
        {
            m_trucks[i]->UnmuteAllSounds();
        }
    }
}

void RoR::BeamFactory::removeTruck(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box)
{
    removeTruck(this->FindTruckInsideBox(collisions, inst, box));
}

void RoR::BeamFactory::removeTruck(int truck)
{
    if (truck < 0 || truck > m_free_truck)
        return;

    if (!m_trucks[truck])
        return;

    if (m_trucks[truck]->state == NETWORKED)
        return;

    this->DeleteTruck(m_trucks[truck]);
}

void RoR::BeamFactory::CleanUpAllTrucks() // Called after simulation finishes
{
    for (int i = 0; i < m_free_truck; i++)
    {
        if (m_trucks[i] == nullptr)
            continue; // This is how things currently work (but not for long...) ~ only_a_ptr, 01/2017

        delete m_trucks[i];
        m_trucks[i] = nullptr;
    }

    // Reset to empty value. Do NOT call `setCurrentTruck(-1)` - performs updates which are invalid at this point
    m_current_truck = -1;

    // TEMPORARY: DO !NOT! attempt to reuse slots
    // Yields bad behavior when player disconnects from game where other players had vehicles spawned
    // Upon reconnect, vehicles with flexbodies (tested on Gavril MZR) show up badly deformed and twitching.
    // Vehicles with cabs (tested on Agora L) show up without the cab.
    // ~only_a_ptr, 01/2017
    //m_free_truck = 0;
}

void RoR::BeamFactory::DeleteTruck(Beam* b)
{
    if (b == 0)
        return;

    this->SyncWithSimThread();

#ifdef USE_SOCKETW
    if (b->networking && b->state != NETWORKED && b->state != INVALID)
    {
        RoR::Networking::AddPacket(b->m_stream_id, RoRnet::MSG2_STREAM_UNREGISTER, 0, 0);
    }
#endif // USE_SOCKETW

    if (m_current_truck == b->trucknum)
        setCurrentTruck(-1);

    m_trucks[b->trucknum] = 0;
    delete b;


    RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();

}

void RoR::BeamFactory::removeCurrentTruck()
{
    removeTruck(m_current_truck);
}

int RoR::BeamFactory::GetMostRecentTruckSlot()
{
    if (getTruck(m_current_truck))
    {
        return m_current_truck;
    }
    else if (getTruck(m_previous_truck))
    {
        return m_previous_truck;
    }

    return -1;
}

void RoR::BeamFactory::enterNextTruck()
{
    int pivot_index = this->GetMostRecentTruckSlot();

    for (int i = pivot_index + 1; i < m_free_truck; i++)
    {
        if (m_trucks[i] && m_trucks[i]->state != NETWORKED && !m_trucks[i]->isPreloadedWithTerrain())
        {
            setCurrentTruck(i);
            return;
        }
    }

    for (int i = 0; i < pivot_index; i++)
    {
        if (m_trucks[i] && m_trucks[i]->state != NETWORKED && !m_trucks[i]->isPreloadedWithTerrain())
        {
            setCurrentTruck(i);
            return;
        }
    }

    if (pivot_index >= 0 && m_trucks[pivot_index] && m_trucks[pivot_index]->state != NETWORKED && !m_trucks[pivot_index]->isPreloadedWithTerrain())
    {
        setCurrentTruck(pivot_index);
        return;
    }
}

void RoR::BeamFactory::enterPreviousTruck()
{
    int pivot_index = this->GetMostRecentTruckSlot();

    for (int i = pivot_index - 1; i >= 0; i--)
    {
        if (m_trucks[i] && m_trucks[i]->state != NETWORKED && !m_trucks[i]->isPreloadedWithTerrain())
        {
            setCurrentTruck(i);
            return;
        }
    }

    for (int i = m_free_truck - 1; i > pivot_index; i--)
    {
        if (m_trucks[i] && m_trucks[i]->state != NETWORKED && !m_trucks[i]->isPreloadedWithTerrain())
        {
            setCurrentTruck(i);
            return;
        }
    }

    if (pivot_index >= 0 && m_trucks[pivot_index] && m_trucks[pivot_index]->state != NETWORKED && !m_trucks[pivot_index]->isPreloadedWithTerrain())
    {
        setCurrentTruck(pivot_index);
        return;
    }
}

void RoR::BeamFactory::setCurrentTruck(int new_truck)
{
    m_previous_truck = m_current_truck;
    m_current_truck = new_truck;

    if (m_previous_truck >= 0 && m_current_truck >= 0)
    {
        m_sim_controller->ChangedCurrentVehicle(m_trucks[m_previous_truck], m_trucks[m_current_truck]);
    }
    else if (m_previous_truck >= 0)
    {
        m_sim_controller->ChangedCurrentVehicle(m_trucks[m_previous_truck], nullptr);
    }
    else if (m_current_truck >= 0)
    {
        m_sim_controller->ChangedCurrentVehicle(nullptr, m_trucks[m_current_truck]);
    }
    else
    {
        m_sim_controller->ChangedCurrentVehicle(nullptr, nullptr);
    }

    this->UpdateSleepingState(0.0f);
}

bool RoR::BeamFactory::enterRescueTruck()
{
    // rescue!
    // search a rescue truck
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->rescuer)
        {
            // go to person mode first
            setCurrentTruck(-1);
            // then to the rescue truck, this fixes overlapping interfaces
            setCurrentTruck(t);
            return true;
        }
    }
    return false;
}

void RoR::BeamFactory::updateFlexbodiesPrepare()
{
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->state < SLEEPING)
        {
            m_trucks[t]->updateFlexbodiesPrepare();
        }
    }
}

void RoR::BeamFactory::joinFlexbodyTasks()
{
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->state < SLEEPING)
        {
            m_trucks[t]->joinFlexbodyTasks();
        }
    }
}

void RoR::BeamFactory::updateFlexbodiesFinal()
{
    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t] && m_trucks[t]->state < SLEEPING)
        {
            m_trucks[t]->updateFlexbodiesFinal();
        }
    }
}

void RoR::BeamFactory::updateVisual(float dt)
{
    dt *= m_simulation_speed;

    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;

        // always update the labels
        m_trucks[t]->updateLabels(dt);

        if (m_trucks[t]->state < SLEEPING)
        {
            m_trucks[t]->updateVisual(dt);
            m_trucks[t]->updateSkidmarks();
            m_trucks[t]->updateFlares(dt, (t == m_current_truck));
        }
    }
}

void RoR::BeamFactory::update(float dt)
{
    m_physics_frames++;

    // do not allow dt > 1/20
    dt = std::min(dt, 1.0f / 20.0f);

    dt *= m_simulation_speed;

    dt += m_dt_remainder;
    m_physics_steps = dt / PHYSICS_DT;
    m_dt_remainder = dt - (m_physics_steps * PHYSICS_DT);
    dt = PHYSICS_DT * m_physics_steps;

    gEnv->mrTime += dt;

    this->SyncWithSimThread();

    this->UpdateSleepingState(dt);

    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;

        m_trucks[t]->handleResetRequests(dt);
        m_trucks[t]->updateAngelScriptEvents(dt);

#ifdef USE_ANGELSCRIPT
        if (m_trucks[t]->vehicle_ai && (m_trucks[t]->vehicle_ai->IsActive()))
            m_trucks[t]->vehicle_ai->update(dt, 0);
#endif // USE_ANGELSCRIPT

        switch (m_trucks[t]->state)
        {
        case NETWORKED:
            m_trucks[t]->calcNetwork();
            break;

        case INVALID:
            break;

        default:
            if (m_trucks[t]->state != SIMULATED && m_trucks[t]->engine)
                m_trucks[t]->engine->update(dt, 1);
            if (m_trucks[t]->state < SLEEPING)
                m_trucks[t]->UpdatePropAnimations(dt);
            if (m_trucks[t]->networking)
            {
                if (m_trucks[t]->state == SIMULATED)
                    m_trucks[t]->sendStreamData();
                else if (m_trucks[t]->state == SLEEPING && m_trucks[t]->netTimer.getMilliseconds() < 10000)
                // Also send update messages for 'SLEEPING' trucks during the first 10 seconds of lifetime
                    m_trucks[t]->sendStreamData();
                else if (m_trucks[t]->state == SLEEPING && m_trucks[t]->netTimer.getMilliseconds() - m_trucks[t]->lastNetUpdateTime > 5000)
                // Also send update messages for 'SLEEPING' trucks periodically every 5 seconds
                    m_trucks[t]->sendStreamData();
            }
            break;
        }
    }

    m_simulated_truck = m_current_truck;

    if (m_simulated_truck == -1)
    {
        for (int t = 0; t < m_free_truck; t++)
        {
            if (m_trucks[t] && m_trucks[t]->state == SIMULATED)
            {
                m_simulated_truck = t;
                break;
            }
        }
    }

    if (m_simulated_truck >= 0 && m_simulated_truck < m_free_truck)
    {
        if (m_simulated_truck == m_current_truck)
        {

            m_trucks[m_simulated_truck]->updateDashBoards(dt);

#ifdef FEAT_TIMING
            if (m_trucks[m_simulated_truck]->statistics)     m_trucks[m_simulated_truck]->statistics->frameStep(dt);
            if (m_trucks[m_simulated_truck]->statistics_gfx) m_trucks[m_simulated_truck]->statistics_gfx->frameStep(dt);
#endif // FEAT_TIMING
        }
        if (!m_trucks[m_simulated_truck]->replayStep())
        {
            m_trucks[m_simulated_truck]->ForceFeedbackStep(m_physics_steps);
            if (m_sim_thread_pool)
            {
                auto func = std::function<void()>([this]()
                    {
                        this->UpdatePhysicsSimulation();
                    });
                m_sim_task = m_sim_thread_pool->RunTask(func);
            }
            else
            {
                this->UpdatePhysicsSimulation();
            }
        }
    }
}

void RoR::BeamFactory::windowResized()
{

    for (int t = 0; t < m_free_truck; t++)
    {
        if (m_trucks[t])
        {
            m_trucks[t]->dash->windowResized();
        }
    }

}

void RoR::BeamFactory::prepareShutdown()
{
    this->SyncWithSimThread();
}

Beam* RoR::BeamFactory::getCurrentTruck()
{
    return this->getTruck(m_current_truck);
}

Beam* RoR::BeamFactory::getTruck(int number)
{
    if (number >= 0 && number < m_free_truck)
    {
        return m_trucks[number];
    }
    return 0;
}

void RoR::BeamFactory::UpdatePhysicsSimulation()
{
    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        m_trucks[t]->preUpdatePhysics(m_physics_steps * PHYSICS_DT);
    }
    if (gEnv->threadPool)
    {
        for (int i = 0; i < m_physics_steps; i++)
        {
            int num_simulated_trucks = 0;
            {
                std::vector<std::function<void()>> tasks;
                for (int t = 0; t < m_free_truck; t++)
                {
                    if (m_trucks[t] && (m_trucks[t]->simulated = m_trucks[t]->calcForcesEulerPrepare(i == 0, PHYSICS_DT, i, m_physics_steps)))
                    {
                        num_simulated_trucks++;
                        auto func = std::function<void()>([this, i, t]()
                            {
                                m_trucks[t]->calcForcesEulerCompute(i == 0, PHYSICS_DT, i, m_physics_steps);
                                if (!m_trucks[t]->disableTruckTruckSelfCollisions)
                                {
                                    m_trucks[t]->IntraPointCD()->update(m_trucks[t]);
                                    intraTruckCollisions(PHYSICS_DT,
                                        *(m_trucks[t]->IntraPointCD()),
                                        m_trucks[t]->free_collcab,
                                        m_trucks[t]->collcabs,
                                        m_trucks[t]->cabs,
                                        m_trucks[t]->intra_collcabrate,
                                        m_trucks[t]->nodes,
                                        m_trucks[t]->collrange,
                                        *(m_trucks[t]->submesh_ground_model));
                                }
                            });
                        tasks.push_back(func);
                    }
                }
                gEnv->threadPool->Parallelize(tasks);
            }

            for (int t = 0; t < m_free_truck; t++)
            {
                if (m_trucks[t] && m_trucks[t]->simulated)
                {
                    this->CalcHooks(m_trucks[t]);
                    this->CalcRopes(m_trucks[t]);
                }
            }

            if (num_simulated_trucks > 1)
            {
                std::vector<std::function<void()>> tasks;
                for (int t = 0; t < m_free_truck; t++)
                {
                    if (m_trucks[t] && m_trucks[t]->simulated && !m_trucks[t]->disableTruckTruckCollisions)
                    {
                        auto func = std::function<void()>([this, t]()
                            {
                                m_trucks[t]->InterPointCD()->update(m_trucks[t], m_trucks, m_free_truck);
                                if (m_trucks[t]->collisionRelevant)
                                {
                                    interTruckCollisions(PHYSICS_DT,
                                        *(m_trucks[t]->InterPointCD()),
                                        m_trucks[t]->free_collcab,
                                        m_trucks[t]->collcabs,
                                        m_trucks[t]->cabs,
                                        m_trucks[t]->inter_collcabrate,
                                        m_trucks[t]->nodes,
                                        m_trucks[t]->collrange,
                                        m_trucks, m_free_truck,
                                        *(m_trucks[t]->submesh_ground_model));
                                }
                            });
                        tasks.push_back(func);
                    }
                }
                gEnv->threadPool->Parallelize(tasks);
            }
        }
    }
    else
    {
        for (int i = 0; i < m_physics_steps; i++)
        {
            int num_simulated_trucks = 0;

            for (int t = 0; t < m_free_truck; t++)
            {
                if (m_trucks[t] && (m_trucks[t]->simulated = m_trucks[t]->calcForcesEulerPrepare(i == 0, PHYSICS_DT, i, m_physics_steps)))
                {
                    num_simulated_trucks++;
                    m_trucks[t]->calcForcesEulerCompute(i == 0, PHYSICS_DT, i, m_physics_steps);
                    this->CalcHooks(m_trucks[t]);
                    this->CalcRopes(m_trucks[t]);
                    if (!m_trucks[t]->disableTruckTruckSelfCollisions)
                    {
                        m_trucks[t]->IntraPointCD()->update(m_trucks[t]);
                        intraTruckCollisions(PHYSICS_DT,
                            *(m_trucks[t]->IntraPointCD()),
                            m_trucks[t]->free_collcab,
                            m_trucks[t]->collcabs,
                            m_trucks[t]->cabs,
                            m_trucks[t]->intra_collcabrate,
                            m_trucks[t]->nodes,
                            m_trucks[t]->collrange,
                            *(m_trucks[t]->submesh_ground_model));
                    }
                }
            }

            if (num_simulated_trucks > 1)
            {
                BES_START(BES_CORE_Contacters);
                for (int t = 0; t < m_free_truck; t++)
                {
                    if (m_trucks[t] && m_trucks[t]->simulated && !m_trucks[t]->disableTruckTruckCollisions)
                    {
                        m_trucks[t]->InterPointCD()->update(m_trucks[t], m_trucks, m_free_truck);
                        if (m_trucks[t]->collisionRelevant)
                        {
                            interTruckCollisions(
                                PHYSICS_DT,
                                *(m_trucks[t]->InterPointCD()),
                                m_trucks[t]->free_collcab,
                                m_trucks[t]->collcabs,
                                m_trucks[t]->cabs,
                                m_trucks[t]->inter_collcabrate,
                                m_trucks[t]->nodes,
                                m_trucks[t]->collrange,
                                m_trucks, m_free_truck,
                                *(m_trucks[t]->submesh_ground_model));
                        }
                    }
                }
                BES_STOP(BES_CORE_Contacters);
            }
        }
    }
    for (int t = 0; t < m_free_truck; t++)
    {
        if (!m_trucks[t])
            continue;
        if (!m_trucks[t]->simulated)
            continue;
        m_trucks[t]->postUpdatePhysics(m_physics_steps * PHYSICS_DT);
    }
}

void RoR::BeamFactory::SyncWithSimThread()
{
    if (m_sim_task)
        m_sim_task->join();
}

#define LOGSTREAM Ogre::LogManager::getSingleton().stream() << "[RoR|Physics] "

void RoR::BeamFactory::AddNewInterBeam(inter_beam_t & new_ib)
{
    m_inter_beams.push_back(new_ib);
}

inter_beam_t* RoR::BeamFactory::FindInterBeam(const Beam* actor, const InterBeamType type, const size_t entry_index)
{
    for (inter_beam_t& ib: m_inter_beams)
    {
        if ((ib.ib_actor_master == actor) && (ib.ib_type == type) && (ib.ib_entry_index == entry_index))
        {
            return &ib;
        }
    }
    return nullptr;
}

void RoR::BeamFactory::RemoveAllInterBeams(Beam* actor)
{
    auto itor = m_inter_beams.begin();
    auto endi = m_inter_beams.end();
    while (itor != endi)
    {
        if (itor->ib_actor_master == actor)
        {
            itor = m_inter_beams.erase(itor);
        }
        else if (itor->ib_actor_slave == actor)
        {
            itor->ib_actor_slave = nullptr;
            itor->ib_beam.disabled = true;
            itor->ib_beam.p2 = itor->ib_rest_node2;

            if (itor->ib_type == InterBeamType::IB_HOOK)
            {
                const hook_t& hook = itor->ib_actor_master->hooks.at(itor->ib_entry_index);
                const node_t& node0 = itor->ib_actor_master->nodes[0];
                itor->ib_beam.L = (node0.AbsPosition - hook.hookNode->AbsPosition).length();
            }
        }
        ++itor;
    }
}

void RoR::BeamFactory::FindLinkedActors(Beam* actor, std::unordered_set<Beam*>& slave_actors)
{
    for (auto& ib: m_inter_beams)
    {
        if (ib.ib_actor_master == actor)
            slave_actors.insert(ib.ib_actor_slave);
    }
}

size_t RoR::BeamFactory::GetNumLinkedActors(Beam* actor)
{
    std::unordered_set<Beam*> slave_actors;
    this->FindLinkedActors(actor, slave_actors);
    return slave_actors.size();
}

float RoR::BeamFactory::GetTotalLinkedMass(Beam* actor)
{
    std::unordered_set<Beam*> slave_actors;
    this->FindLinkedActors(actor, slave_actors);

    float mass = 0.f;
    for (Beam* slave: slave_actors)
    {
        mass += slave->getTotalMass(false);
    }

    return mass;
}

void RoR::BeamFactory::SetSkeletonViewActive (Beam* actor, bool active)
{
    std::unordered_set<Beam*> slave_actors;
    for (auto& ib: m_inter_beams)
    {
        if (ib.ib_actor_master == actor)
            slave_actors.insert(ib.ib_actor_slave);
    }

    if (active)
    {
        actor->showSkeleton();
        for (Beam* slave: slave_actors)
        {
            slave->showSkeleton(true);
        }
    }
    else
    {
        actor->hideSkeleton();
        for (Beam* slave: slave_actors)
        {
            slave->hideSkeleton();
        }
    }
}

void RoR::BeamFactory::DetachTieInterBeam(inter_beam_t* tie_ib)
{
    tie_ib->ib_actor_slave = nullptr;
    tie_ib->ib_beam.disabled = true;
    tie_ib->ib_beam.p2 = tie_ib->ib_rest_node2;
    if (tie_ib->ib_actor_slave != nullptr)
    {
        this->SetSkeletonViewActive(tie_ib->ib_actor_slave, false);
    }
}

void RoR::BeamFactory::AttachTieInterBeam(inter_beam_t* tie_ib, Beam* slave, node_t* node)
{
    tie_ib->ib_actor_slave = slave;
    tie_ib->ib_beam.p2 = node;
    tie_ib->ib_beam.stress = 0.f;
    tie_ib->ib_beam.L = tie_ib->ib_beam.refL;
    this->SetSkeletonViewActive(slave, tie_ib->ib_actor_master->m_skeletonview_is_active);
}

void RoR::BeamFactory::UnlockRopeInterBeam(inter_beam_t* interbeam)
{
    interbeam->ib_actor_slave = nullptr;
    interbeam->ib_beam.disabled = true;
}

void RoR::BeamFactory::LockRopeInterBeam(inter_beam_t* interbeam, Beam* slave)
{
    interbeam->ib_actor_slave = slave;
}

void RoR::BeamFactory::UnlockHookInterBeam(inter_beam_t* hook_ib)
{
    hook_ib->ib_beam.p2 = hook_ib->ib_rest_node2;
    hook_ib->ib_beam.disabled = true;
    hook_ib->ib_beam.L = (hook_ib->ib_rest_node2->AbsPosition - hook_ib->ib_beam.p1->AbsPosition).length();
    hook_ib->ib_actor_slave = nullptr;
}

void RoR::BeamFactory::PreLockHookInterBeam(inter_beam_t* hook_ib, Beam* slave)
{
    hook_ib->ib_actor_slave = slave;
}

void RoR::BeamFactory::LockHookInterBeamCalc(inter_beam_t* interbeam, node_t* node)
{
    interbeam->ib_beam.disabled = false;
    interbeam->ib_beam.p2 = node;
    interbeam->ib_beam.L = (interbeam->ib_beam.p1->AbsPosition - node->AbsPosition).length();
}

