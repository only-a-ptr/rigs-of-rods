/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#ifdef USE_OPENAL

#include "SoundScriptManager.h"

#include "Application.h"
#include "AutoPilot.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "Settings.h"
#include "Sound.h"
#include "SoundManager.h"
#include "TurboJet.h"
#include "Utils.h"

#include <OgreResourceGroupManager.h>

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

using namespace Ogre;
using namespace RoR;

const float SoundScriptInstance::PITCHDOWN_FADE_FACTOR = 3.0f;
const float SoundScriptInstance::PITCHDOWN_CUTOFF_FACTOR = 5.0f;

SoundScriptManager::SoundScriptManager() :
    disabled(true)
    , loading_base(false)
    , instance_counter(0)
    , max_distance(500.0f)
    , rolloff_factor(1.0f)
    , reference_distance(7.5f)
    , sound_manager(0)
    , m_music(nullptr)
{

    sound_manager = new SoundManager();

    if (!sound_manager)
    {
        LOG("SoundScriptManager: Failed to create the Sound Manager");
        return;
    }

    disabled = sound_manager->isDisabled() || BSETTING("regen-cache-only", false);

    if (disabled)
    {
        LOG("SoundScriptManager: Sound Manager is disabled");
        return;
    }

    LOG("SoundScriptManager: Sound Manager started with " + TOSTRING(sound_manager->getNumHardwareSources())+" sources");
    script_patterns.push_back("*.soundscript");
    ResourceGroupManager::getSingleton()._registerScriptLoader(this);
}

void SoundScriptManager::setCamera(Vector3 position, Vector3 direction, Vector3 up, Vector3 velocity)
{
    if (disabled)
        return;
    sound_manager->setCamera(position, direction, up, velocity);
}

const StringVector& SoundScriptManager::getScriptPatterns(void) const
{
    return script_patterns;
}

Real SoundScriptManager::getLoadingOrder(void) const
{
    // load late
    return 1000.0f;
}

void SoundScriptManager::clearNonBaseTemplates()
{
    int counter = 0;

    auto itor = m_defs.begin();
    while (itor != m_defs.end())
    {
        if (!itor->second.is_base)
        {
            itor = m_defs.erase(itor);
        }
        else
        {
            ++itor;
        }
    }

    if (counter > 0)
    {
        LOG("[RoR|Audio] Removed " + TOSTRING(counter) + " non-base soundscripts");
    }
}

// Internal helper
// NOTE: This is a temporary solution to integrate new parser <-> old SoundScriptManager ~ only_a_ptr, 03/2017
SoundTriggers ResolveSoundTrigger(RoR::SoundTriggerDef def)
{
    switch (def.source)
    {
    case RoR::SoundTriggerDef::SND_TRIG_NONE             : return SS_TRIG_NONE;
    case RoR::SoundTriggerDef::SND_TRIG_ENGINE           : return SS_TRIG_ENGINE;
    case RoR::SoundTriggerDef::SND_TRIG_AEROENGINE_RPM   : return SoundTriggers(SS_TRIG_AEROENGINE1 + def.source_param);
    case RoR::SoundTriggerDef::SND_TRIG_HORN             : return SS_TRIG_HORN;
    case RoR::SoundTriggerDef::SND_TRIG_BRAKE            : return SS_TRIG_BRAKE;
    case RoR::SoundTriggerDef::SND_TRIG_PUMP             : return SS_TRIG_PUMP;
    case RoR::SoundTriggerDef::SND_TRIG_STARTER          : return SS_TRIG_STARTER;
    case RoR::SoundTriggerDef::SND_TRIG_TURBO_BOV        : return SS_TRIG_TURBOBOV;
    case RoR::SoundTriggerDef::SND_TRIG_TURBO_WASTEGATE  : return SS_TRIG_TURBOWASTEGATE;
    case RoR::SoundTriggerDef::SND_TRIG_TURBO_BACKFIRE   : return SS_TRIG_TURBOBACKFIRE;
    case RoR::SoundTriggerDef::SND_TRIG_ALWAYS_ON        : return SS_TRIG_ALWAYSON;
    case RoR::SoundTriggerDef::SND_TRIG_REPAIR           : return SS_TRIG_REPAIR;
    case RoR::SoundTriggerDef::SND_TRIG_AIR              : return SS_TRIG_AIR;
    case RoR::SoundTriggerDef::SND_TRIG_GPWS_APDISCONNECT: return SS_TRIG_GPWS_APDISCONNECT;
    case RoR::SoundTriggerDef::SND_TRIG_GPWS_VALUE:  ///< Param = value; replaces "GPWS[10|20|30|40|50|100]"
        switch (def.source_param)
        {
        case  10: return SS_TRIG_GPWS_10;
        case  20: return SS_TRIG_GPWS_20;
        case  30: return SS_TRIG_GPWS_30;
        case  40: return SS_TRIG_GPWS_40;
        case  50: return SS_TRIG_GPWS_50;
        case 100: return SS_TRIG_GPWS_100;
        default: return SS_TRIG_NONE;
        }
    case RoR::SoundTriggerDef::SND_TRIG_GPWS_PULLUP      : return SS_TRIG_GPWS_PULLUP;
    case RoR::SoundTriggerDef::SND_TRIG_GPWS_MINIMUMS    : return SS_TRIG_GPWS_MINIMUMS;
    case RoR::SoundTriggerDef::SND_TRIG_AIR_PURGE        : return SS_TRIG_AIR_PURGE;
    case RoR::SoundTriggerDef::SND_TRIG_SHIFT            : return SS_TRIG_SHIFT;
    case RoR::SoundTriggerDef::SND_TRIG_GEARSLIDE        : return SS_TRIG_GEARSLIDE;
    case RoR::SoundTriggerDef::SND_TRIG_BREAK            : return SS_TRIG_BREAK;
    case RoR::SoundTriggerDef::SND_TRIG_SCREETCH         : return SS_TRIG_SCREETCH;
    case RoR::SoundTriggerDef::SND_TRIG_PARK             : return SS_TRIG_PARK;
    case RoR::SoundTriggerDef::SND_TRIG_AFTERBURNER      : return SoundTriggers(SS_TRIG_AFTERBURNER1 + def.source_param);
    case RoR::SoundTriggerDef::SND_TRIG_AOA              : return SS_TRIG_AOA;
    case RoR::SoundTriggerDef::SND_TRIG_IGNITION         : return SS_TRIG_IGNITION;
    case RoR::SoundTriggerDef::SND_TRIG_REVERSE_GEAR     : return SS_TRIG_REVERSE_GEAR;
    case RoR::SoundTriggerDef::SND_TRIG_TURN_SIGNAL      : return SS_TRIG_TURN_SIGNAL;
    case RoR::SoundTriggerDef::SND_TRIG_TURN_SIGNAL_TICK : return SS_TRIG_TURN_SIGNAL_TICK;
    case RoR::SoundTriggerDef::SND_TRIG_TURN_SIGNAL_WARN_TICK:return SS_TRIG_TURN_SIGNAL_WARN_TICK;
    case RoR::SoundTriggerDef::SND_TRIG_ALB_ACTIVE       : return SS_TRIG_ALB_ACTIVE;
    case RoR::SoundTriggerDef::SND_TRIG_TC_ACTIVE        : return SS_TRIG_TC_ACTIVE;
    case RoR::SoundTriggerDef::SND_TRIG_AVIONIC_CHAT     : return SoundTriggers(SS_TRIG_AVICHAT01 + def.source_param);
    case RoR::SoundTriggerDef::SND_TRIG_MAIN_MENU        : return SS_TRIG_MAIN_MENU;
    default: return SS_TRIG_NONE;
    }
}
// Internal helper
// NOTE: This is a temporary solution to integrate new parser <-> old SoundScriptManager ~ only_a_ptr, 03/2017
ModulationSources ResolveModSource(RoR::SoundModulationDef def)
{
    switch (def.source)
    {
    case RoR::SoundModulationDef::SND_MOD_NONE               : return SS_MOD_NONE;
    case RoR::SoundModulationDef::SND_MOD_ENGINE             : return SS_MOD_ENGINE;
    case RoR::SoundModulationDef::SND_MOD_TURBO              : return SS_MOD_TURBO;
    case RoR::SoundModulationDef::SND_MOD_AEROENGINE_RPM     : return ModulationSources(SS_MOD_AEROENGINE1 + def.source_param);
    case RoR::SoundModulationDef::SND_MOD_WHEELSPEED         : return SS_MOD_WHEELSPEED;
    case RoR::SoundModulationDef::SND_MOD_INJECTOR           : return SS_MOD_INJECTOR;
    case RoR::SoundModulationDef::SND_MOD_TORQUE             : return SS_MOD_TORQUE;
    case RoR::SoundModulationDef::SND_MOD_GEARBOX            : return SS_MOD_GEARBOX;
    case RoR::SoundModulationDef::SND_MOD_BREAK              : return SS_MOD_BREAK;
    case RoR::SoundModulationDef::SND_MOD_SCREETCH           : return SS_MOD_SCREETCH;
    case RoR::SoundModulationDef::SND_MOD_PUMP               : return SS_MOD_PUMP;
    case RoR::SoundModulationDef::SND_MOD_AEROENGINE_THROTTLE: return ModulationSources(SS_MOD_THROTTLE1 + def.source_param);
    case RoR::SoundModulationDef::SND_MOD_AIRSPEED           : return SS_MOD_AIRSPEED;
    case RoR::SoundModulationDef::SND_MOD_AOA                : return SS_MOD_AOA;
    case RoR::SoundModulationDef::SND_MOD_LINKED_COMMANDRATE : return SS_MOD_LINKED_COMMANDRATE;
    case RoR::SoundModulationDef::SND_MOD_MUSIC_VOLUME       : return SS_MOD_MUSIC_VOLUME;
    default: return SS_MOD_NONE;
    }
}

void SoundScriptManager::parseScript(DataStreamPtr& stream, const String& group_name)
{
    RoR::SoundScriptParser parser(m_defs);
    parser.ProcessOgreDataStream(stream, group_name, loading_base);

    if (parser.GetMessages().empty())
    {
        LOG("[RoR|Audio] Loaded soundscript: " + stream->getName());
        return;
    }

    std::stringstream log_buf;
    log_buf << "[RoR|Audio] Loaded soundscript '" << stream->getName()
            << "' (" << parser.GetMessages().size() << " messages)" << std::endl;
    for (auto& msg: parser.GetMessages())
    {
        log_buf << "\t" << msg << std::endl;
    }
    LOG(log_buf.str());
}

void SoundScriptManager::setEnabled(bool state)
{
    if (state)
        sound_manager->resumeAllSounds();
    else
        sound_manager->pauseAllSounds();
}

void SoundScriptManager::PlayMusic(SoundTriggers trig)
{
    if (m_music != nullptr)
    {
        m_music->kill();
        delete m_music;
        m_music = nullptr;
    }

    switch (trig)
    {
    case SS_TRIG_MAIN_MENU:
        m_music = new SoundScriptInstance(&(m_defs.find("tracks/main_menu_tune")->second), sound_manager, -1, -2);
        m_music->start();
        break;
    default:;
    }
}

RoR::SoundScriptDef* SoundScriptManager::GetSoundScriptDef(std::string name)
{
    auto res = m_defs.find(name);
    if (res != m_defs.end())
        return &res->second;
    else
        return nullptr;
}

//====================================================================

SoundScriptInstance::SoundScriptInstance(RoR::SoundScriptDef* def, SoundManager* sound_manager, int node_id, int type)
    : m_trigger(ResolveSoundTrigger(def->trigger))
    , m_gain_source(ResolveModSource(def->gain))
    , m_pitch_source(ResolveModSource(def->pitch))
    , start_sound(nullptr)
    , start_sound_pitchgain(0.0f)
    , stop_sound(nullptr)
    , stop_sound_pitchgain(0.0f)
    , lastgain(1.0f)
    , m_node_id(node_id)
    , m_type(type)
{
    memset(&m_def, 0, sizeof(Def));

    // create sounds
    if (!def->file_start.filename.empty())
    {
        start_sound = sound_manager->createSound(def->file_start.filename);
        m_def.start_sound_pitch = def->file_start.pitch;
    }

    if (!def->file_stop.filename.empty())
    {
        stop_sound = sound_manager->createSound(def->file_stop.filename);
        m_def.stop_sound_pitch = def->file_stop.pitch;
    }

    int index = 0;
    for (SoundFileDef& file_def: def->files)
    {
        sounds[index] = sound_manager->createSound(file_def.filename);
        m_def.pitches[index] = file_def.pitch;
        ++index;
    }
    m_def.num_sounds = def->files.size();

    m_def.pitch_mul    = def->pitch.multiplier;
    m_def.pitch_offset = def->pitch.offset;
    m_def.pitch_square = def->pitch.square;
    this->setPitch(0.0f);
    m_def.gain_mul    = def->gain.multiplier;
    m_def.gain_offset = def->gain.offset;
    m_def.gain_square = def->gain.square;
    this->setGain(1.0f);

    LOG("[RoR|Audio] Created instance of soundscript: " + def->name);
}

void SoundScriptInstance::setPitch(float value)
{
    if (start_sound)
    {
        start_sound_pitchgain = pitchgain_cutoff(m_def.start_sound_pitch, value);

        if (start_sound_pitchgain != 0.0f && m_def.start_sound_pitch != 0.0f)
        {
            start_sound->setPitch(value / m_def.start_sound_pitch);
        }
    }

    if (m_def.num_sounds)
    {
        // searching the interval
        size_t up = 0;

        for (up = 0; up < m_def.num_sounds; up++)
        {
            if (m_def.pitches[up] > value)
            {
                break;
            }
        }

        if (up == 0)
        {
            // low sound case
            sounds_pitchgain[0] = pitchgain_cutoff(m_def.pitches[0], value);

            if (sounds_pitchgain[0] != 0.0f && m_def.pitches[0] != 0.0f && sounds[0])
            {
                sounds[0]->setPitch(value / m_def.pitches[0]);
            }

            for (size_t i = 1; i < m_def.num_sounds; i++)
            {
                if (m_def.pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }
        }
        else if (up == m_def.num_sounds)
        {
            // high sound case
            for (size_t i = 0; i < m_def.num_sounds - 1; i++)
            {
                if (m_def.pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }

            sounds_pitchgain[m_def.num_sounds - 1] = 1.0f;

            if (m_def.pitches[m_def.num_sounds - 1] != 0.0f && sounds[m_def.num_sounds - 1])
            {
                sounds[m_def.num_sounds - 1]->setPitch(value / m_def.pitches[m_def.num_sounds - 1]);
            }
        }
        else
        {
            // middle sound case
            int low = up - 1;

            for (int i = 0; i < low; i++)
            {
                if (m_def.pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }

            if (m_def.pitches[low] != 0.0f && sounds[low])
            {
                sounds_pitchgain[low] = (m_def.pitches[up] - value) / (m_def.pitches[up] - m_def.pitches[low]);
                sounds[low]->setPitch(value / m_def.pitches[low]);
            }
            else
            {
                sounds_pitchgain[low] = 1.0f; // unpitched
            }

            if (m_def.pitches[up] != 0.0f && sounds[up])
            {
                sounds_pitchgain[up] = (value - m_def.pitches[low]) / (m_def.pitches[up] - m_def.pitches[low]);
                sounds[up]->setPitch(value / m_def.pitches[up]);
            }
            else
            {
                sounds_pitchgain[up] = 1.0f; // unpitched
            }

            for (size_t i = up + 1; i < m_def.num_sounds; i++)
            {
                if (m_def.pitches[i] != 0.0f)
                {
                    sounds_pitchgain[i] = 0.0f;
                    // pause?
                }
                else
                {
                    sounds_pitchgain[i] = 1.0f; // unpitched
                }
            }
        }
    }

    if (stop_sound)
    {
        stop_sound_pitchgain = pitchgain_cutoff(m_def.stop_sound_pitch, value);

        if (stop_sound_pitchgain != 0.0f && m_def.stop_sound_pitch != 0.0f)
        {
            stop_sound->setPitch(value / m_def.stop_sound_pitch);
        }
    }

    // propagate new gains
    setGain(lastgain);
}

float SoundScriptInstance::pitchgain_cutoff(float sourcepitch, float targetpitch)
{
    if (sourcepitch == 0.0f)
    {
        return 1.0f; // unpitchable
    }

    if (targetpitch > sourcepitch / PITCHDOWN_FADE_FACTOR)
    {
        return 1.0f; // pass
    }

    if (targetpitch < sourcepitch / PITCHDOWN_CUTOFF_FACTOR)
    {
        return 0.0f; // cutoff
    }

    // linear fading
    return (targetpitch - sourcepitch / PITCHDOWN_CUTOFF_FACTOR) / (sourcepitch / PITCHDOWN_FADE_FACTOR - sourcepitch / PITCHDOWN_CUTOFF_FACTOR);
}

void SoundScriptInstance::setGain(float value)
{
    if (start_sound)
    {
        start_sound->setGain(value * start_sound_pitchgain);
    }

    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setGain(value * sounds_pitchgain[i]);
        }
    }

    if (stop_sound)
    {
        stop_sound->setGain(value * stop_sound_pitchgain);
    }

    lastgain = value;
}

void SoundScriptInstance::ModulateGain(float value)
{
    float gain = value * value * m_def.gain_square + value * m_def.gain_mul + m_def.gain_offset;
    gain = std::max(0.0f, gain);
    gain = std::min(gain, 1.0f);
    this->setGain(gain);
}

void SoundScriptInstance::ModulatePitch(float value)
{
    float pitch = value * value * m_def.pitch_square + value * m_def.pitch_mul + m_def.pitch_offset;
    pitch = std::max(0.0f, pitch);
    this->setPitch(pitch);
}

void SoundScriptInstance::setPosition(Vector3 pos, Vector3 velocity)
{
    if (start_sound)
    {
        start_sound->setPosition(pos);
        start_sound->setVelocity(velocity);
    }

    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setPosition(pos);
            sounds[i]->setVelocity(velocity);
        }
    }

    if (stop_sound)
    {
        stop_sound->setPosition(pos);
        stop_sound->setVelocity(velocity);
    }
}

void SoundScriptInstance::runOnce()
{
    if (start_sound)
    {
        if (start_sound->isPlaying())
        {
            return;
        }
        start_sound->play();
    }

    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
        {
            if (sounds[i]->isPlaying())
            {
                continue;
            }
            sounds[i]->setLoop(false);
            sounds[i]->play();
        }
    }

    if (stop_sound)
    {
        if (stop_sound->isPlaying())
        {
            return;
        }
        stop_sound->play();
    }
}

void SoundScriptInstance::start()
{
    if (start_sound)
    {
        start_sound->stop();
        //start_sound->setLoop(true);
        start_sound->play();
    }

    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setLoop(true);
            sounds[i]->play();
        }
    }
}

void SoundScriptInstance::stop()
{
    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
            sounds[i]->stop();
    }

    if (stop_sound)
    {
        stop_sound->stop();
        stop_sound->play();
    }
}

void SoundScriptInstance::kill()
{
    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
            sounds[i]->stop();
    }

    if (start_sound)
        start_sound->stop();

    if (stop_sound)
    {
        stop_sound->stop();
        stop_sound->play();
    }
}

void SoundScriptInstance::setEnabled(bool e)
{
    if (start_sound)
    {
        start_sound->setEnabled(e);
    }

    if (stop_sound)
    {
        stop_sound->setEnabled(e);
    }

    for (size_t i = 0; i < m_def.num_sounds; i++)
    {
        if (sounds[i])
        {
            sounds[i]->setEnabled(e);
        }
    }
}

// ============================================================================

AudioActor::AudioActor(Beam* actor):
    m_actor(actor),
    m_aoa_active(false),
    m_avi_chatter_trigger(SS_TRIG_NONE),
    m_play_turn_signal_tick(false),
    m_play_warn_signal_tick(false),
    m_repair_play_once(false),
    m_screetch_play_once(false),
    m_break_play_once(false),
    m_tirepressure_active(false),
    m_stabilizers_active(false)
{}

AudioActor::~AudioActor()
{
    for (auto& sound : m_sounds)
    {
        sound->stop();
        sound->setEnabled(false);
    }
}

void AudioActor::AddSound(int node_id, int mode, std::string soundscript_name)
{
    SoundScriptDef* def = SoundScriptManager::getSingleton().GetSoundScriptDef(soundscript_name);
    auto* instance = new SoundScriptInstance(def, SoundScriptManager::getSingleton().GetSoundMgr(), node_id, mode);
    m_sounds.push_back(instance);
}

/// Deduce aeroengine slot (0-7) from trigger/modulation
#define AFTERBURNER_TRIG_SLOT(_TRIG_) (_TRIG_ - SS_TRIG_AFTERBURNER1)
#define AEROENGINE_TRIG_SLOT(_TRIG_) (_TRIG_ - SS_TRIG_AEROENGINE1)
#define AEROENGINE_MOD_SLOT(_MOD_) (_MOD_ - SS_MOD_AEROENGINE1)
#define THROTTLE_MOD_SLOT(_MOD_) (_MOD_ - SS_MOD_THROTTLE1)
#define GPWS_TRIG_VALUE(_TRIG_) (((_TRIG_ - SS_TRIG_GPWS_10) + 1) * 10)

void AudioActor::UpdateSounds()
{
    for (SoundScriptInstance* sound : m_sounds)
    {
        node_t& sound_source = m_actor->nodes[sound->GetNodeId()];
        sound->setPosition(sound_source.AbsPosition, sound_source.Velocity);

        float gain;
        if (this->ResolveModulation(gain, sound->GetGainSource()))
            sound->ModulateGain(gain);

        float pitch;
        if (this->ResolveModulation(pitch, sound->GetPitchSource()))
            sound->ModulatePitch(gain);

        Turbojet* turbojet = nullptr;
        AeroEngine* aeroengine = nullptr;

        const SoundTriggers trigger = sound->GetTrigger();
        switch (trigger)
        {
        case SS_TRIG_ENGINE:
            if (m_engine_forced_state != 0)
                sound->SetActive(m_engine_forced_state == 1);
            else
                sound->SetActive(m_actor->engine->isRunning());
            break;

        case SS_TRIG_BRAKE:
            sound->SetActive(m_actor->brake > (m_actor->brakeforce / 6.0f));
            break;

        case SS_TRIG_PUMP: // Hydraulic pump
            sound->SetActive(m_hydropump_active);
            break;

        case SS_TRIG_STARTER:
            sound->SetActive(m_actor->engine->IsStarterActive());
            break;

        case SS_TRIG_IGNITION:
            sound->SetActive(m_actor->engine->hasContact());
            break;

        case SS_TRIG_TURBOBOV:
            sound->SetActive(m_actor->engine->IsBovSoundActive());
            break;

        case SS_TRIG_TURBOWASTEGATE:
            sound->SetActive(m_actor->engine->IsTurboFluttering());
            break;

        case SS_TRIG_TURBOBACKFIRE:
            sound->SetActive(m_actor->engine->IsBackfireSoundActive());
            break;

        case SS_TRIG_AIR_PURGE:
            if (m_actor->engine->AudioWasAirPurged())
                sound->runOnce();
            break;

        case SS_TRIG_SHIFT:
            sound->SetActive(m_actor->engine->AudioIsShifting());
            if (m_actor->engine->AudioWasGearShifted())
            {
                sound->runOnce();
                m_actor->engine->AudioResetGearShifted();
            }
            break;

        case SS_TRIG_GEARSLIDE:
            if (m_actor->engine->AudioDidGearsSlide())
            {
                sound->runOnce();
                m_actor->engine->AudioResetGearSlide();
            }
            break;

        case SS_TRIG_REVERSE_GEAR: // Reverse gear beeping
            if (m_actor->state == NETWORKED)
                sound->SetActive(m_actor->getReverseLightVisible());
            else
                sound->SetActive(m_actor->IsReverseBeepAudioActive());
            break;

        case SS_TRIG_ALB_ACTIVE:
            sound->SetActive(m_actor->IsAntiLockBrakeActive());
            break;

        case SS_TRIG_TC_ACTIVE:
            sound->SetActive(m_actor->IsTractionControlActive());
            break;

        case SS_TRIG_PARK:
            sound->SetActive(m_actor->parkingbrake != 0);
            break;

        case SS_TRIG_TURN_SIGNAL_WARN_TICK:
            if (m_play_warn_signal_tick)
            {
                sound->runOnce();
                m_play_warn_signal_tick = false;
            }
            break;

        case SS_TRIG_TURN_SIGNAL_TICK:
            if (m_play_turn_signal_tick)
            {
                sound->runOnce();
                m_play_turn_signal_tick = false;
            }
            break;

        case SS_TRIG_TURN_SIGNAL:
            sound->SetActive(m_actor->getBlinkType() != BLINK_NONE);
            break;

        case SS_TRIG_HORN:
            sound->SetActive(m_actor->IsPoliceSirenActive() || m_actor->IsCarHornActive());
            break;

        case SS_TRIG_GPWS_10:
        case SS_TRIG_GPWS_20:
        case SS_TRIG_GPWS_30:
        case SS_TRIG_GPWS_40:
        case SS_TRIG_GPWS_50:
            if (m_actor->autopilot->AudioGetGpwsTrigger() == GPWS_TRIG_VALUE(trigger))
                sound->runOnce();
            break;

        case SS_TRIG_GPWS_100:
            if (m_actor->autopilot->AudioGetGpwsTrigger() == 100)
                sound->runOnce();
            break;

        case SS_TRIG_GPWS_PULLUP:
            if (m_actor->autopilot->AudioShouldPlayPullup())
                sound->runOnce();
            break;

        case SS_TRIG_GPWS_MINIMUMS:
            if (m_actor->autopilot->AudioShouldPlayMinimums())
                sound->runOnce();
            break;

        case SS_TRIG_GPWS_APDISCONNECT:
            if (m_gpws_ap_disconnected)
            {
                m_gpws_ap_disconnected = false;
                sound->runOnce();
            }
            break;

        case SS_TRIG_AOA:
            sound->SetActive(m_aoa_active);
            break;

        case SS_TRIG_SCREETCH:
            if (m_screetch_play_once)
            {
                sound->runOnce();
                m_screetch_play_once = false;
            }
            break;

        case SS_TRIG_BREAK:
            if (m_break_play_once)
            {
                sound->runOnce();
                m_break_play_once = false;
            }
            break;

        case SS_TRIG_REPAIR:
            if (m_repair_play_once)
            {
                sound->runOnce();
                m_repair_play_once = false;
            }
            break;

        case SS_TRIG_AIR:
            sound->SetActive(m_tirepressure_active || m_stabilizers_active);
            break;

        case SS_TRIG_AFTERBURNER1:
        case SS_TRIG_AFTERBURNER2:
        case SS_TRIG_AFTERBURNER3:
        case SS_TRIG_AFTERBURNER4:
        case SS_TRIG_AFTERBURNER5:
        case SS_TRIG_AFTERBURNER6:
        case SS_TRIG_AFTERBURNER7:
        case SS_TRIG_AFTERBURNER8:
            turbojet = reinterpret_cast<Turbojet*>(m_actor->aeroengines[AFTERBURNER_TRIG_SLOT(trigger)]);
            sound->SetActive(turbojet->IsAfterburnerActive());
            break;

        case SS_TRIG_AEROENGINE1:
        case SS_TRIG_AEROENGINE2:
        case SS_TRIG_AEROENGINE3:
        case SS_TRIG_AEROENGINE4:
        case SS_TRIG_AEROENGINE5:
        case SS_TRIG_AEROENGINE6:
        case SS_TRIG_AEROENGINE7:
        case SS_TRIG_AEROENGINE8:
            aeroengine = m_actor->aeroengines[AEROENGINE_TRIG_SLOT(trigger)];
            sound->SetActive(aeroengine->getIgnition() && !aeroengine->isFailed());
            break;

        case SS_TRIG_AVICHAT01:
        case SS_TRIG_AVICHAT02:
        case SS_TRIG_AVICHAT03:
        case SS_TRIG_AVICHAT04:
        case SS_TRIG_AVICHAT05:
        case SS_TRIG_AVICHAT06:
        case SS_TRIG_AVICHAT07:
        case SS_TRIG_AVICHAT08:
        case SS_TRIG_AVICHAT09:
        case SS_TRIG_AVICHAT10:
        case SS_TRIG_AVICHAT11:
        case SS_TRIG_AVICHAT12:
        case SS_TRIG_AVICHAT13:
            if (m_avi_chatter_trigger == trigger)
            {
                sound->runOnce();
                m_avi_chatter_trigger = SS_TRIG_NONE;
            }
            break;

        default:;
        }
    }
}

bool AudioActor::ResolveModulation(float& out_value, const ModulationSources source)
{
    switch (source)
    {
    case SS_MOD_THROTTLE1:
    case SS_MOD_THROTTLE2:
    case SS_MOD_THROTTLE3:
    case SS_MOD_THROTTLE4:
    case SS_MOD_THROTTLE5:
    case SS_MOD_THROTTLE6:
    case SS_MOD_THROTTLE7:
    case SS_MOD_THROTTLE8:
        out_value = m_actor->aeroengines[THROTTLE_MOD_SLOT(source)]->getThrottle();
        return true;

    case SS_MOD_AEROENGINE1:
    case SS_MOD_AEROENGINE2:
    case SS_MOD_AEROENGINE3:
    case SS_MOD_AEROENGINE4:
    case SS_MOD_AEROENGINE5:
    case SS_MOD_AEROENGINE6:
    case SS_MOD_AEROENGINE7:
    case SS_MOD_AEROENGINE8:
        out_value = m_actor->aeroengines[AEROENGINE_MOD_SLOT(source)]->getRPM();
        return true;

    case SS_MOD_PUMP: // Hydraulic pump
        out_value = m_hydropump_modulation;
        return true;

    case SS_MOD_AOA:
        out_value = m_aoa_modulation;
        return true;

    case SS_MOD_INJECTOR:
        out_value = m_actor->engine->GetInjectorAudioLevel();
        return true;

    case SS_MOD_SCREETCH:
        out_value = m_screetch_modulation;
        return true;

    case SS_MOD_BREAK:
        out_value = m_break_modulation;
        return true;

    case SS_MOD_ENGINE:
        out_value = m_engine_modulation;
        return true;

    case SS_MOD_TURBO:
        out_value = m_actor->engine->GetTurboRPM();
        return true;

    case SS_MOD_WHEELSPEED:
        out_value = m_actor->WheelSpeed * 3.6; // TODO: Magic!
        return true;

    case SS_MOD_AIRSPEED:
        out_value = m_actor->nodes[0].Velocity.length() * 1.9438; // TODO: Magic!
        return true;

    case SS_MOD_TORQUE:
        out_value = m_actor->engine->GetClutchTorque();
        return true;

    case SS_MOD_GEARBOX:
        out_value = m_actor->engine->GetWheelRevolutions();
        return true;

    default:
        return false;
    }
}

void AudioActor::SetHydropumpState(bool active, float modulation)
{
    m_hydropump_active = active;
    m_hydropump_modulation = modulation;
}

void AudioActor::SetAoaState(bool active, float modulation)
{
    m_aoa_active = active;
    m_aoa_modulation = modulation;
}

void AudioActor::PlayScreetchOnce(float modulation)
{
    m_screetch_play_once = true;
    m_screetch_modulation = modulation;
}

void AudioActor::PlayBreakOnce(float modulation)
{
    m_break_play_once = true;
    m_break_modulation = modulation;
}

void AudioActor::SetMuteAllSounds(bool mute)
{
    for (auto& sound: m_sounds)
        sound->setEnabled(!mute);
}

void AudioActor::NotifyChangedCamera(int cam_type)
{
    for (auto& sound: m_sounds)
    {
        sound->setEnabled((sound->GetType() == -2) || (sound->GetType() == cam_type));
    }
}

#endif // USE_OPENAL
