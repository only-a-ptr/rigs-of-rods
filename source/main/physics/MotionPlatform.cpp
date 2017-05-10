/*
    This source file is part of Rigs of Rods
    Copyright 2017 Petr Ohlidal

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

#ifdef USE_MPLATFORM

#include "MotionPlatform.h"

#include "Application.h"
#include "Beam.h"
#include "GUIManager.h"
#include "GUI_MotionPlatformWindow.h"
#include "Euler.h"

#include <math.h>

using namespace RoR;

// RoR's physics frequency is 2000hz (0.0005 sec)

const size_t SEND_INTERVAL_MICROSEC = 1000000/MPLATFORM_SEND_RATE;
const float  UPDATES_PER_SEC        = static_cast<float>(MPLATFORM_SEND_RATE);


MotionPlatform::MotionPlatform():
    m_socket(ENET_SOCKET_NULL),
    m_elapsed_time(0),
    m_last_update_time(0)
{
    memset(&m_addr_remote, 0, sizeof(ENetAddress));
    memset(&m_addr_local , 0, sizeof(ENetAddress));
}

// ## TODO: Error reporting!

void MotionPlatform::DeleteSocket()
{
    if (m_socket != ENET_SOCKET_NULL)
    {
        enet_socket_destroy(m_socket);
    }

    m_socket = ENET_SOCKET_NULL;
}

bool MotionPlatform::MPlatformConnect()
{
    if (enet_initialize() != 0)
    {
        LOG("[RoR|MotionPlatform] Failed to connect");
        return false;
    }

    m_socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);

    // Proof-of-concept mode: hardcode everything!
    m_addr_local.host = ENET_HOST_ANY;
    m_addr_local.port = 43000;
    const char IP[] = {char(192),char(168),char(223),char(101)};
    // 
    m_addr_remote.host = ENET_HOST_TO_NET_32(0x7F000001); // = 127.0.0.1 localhost
        //ENET_HOST_TO_NET_32 (0xC0A8DF65); //(reinterpret_cast<int>(IP));//
    m_addr_remote.port = 4123;

    if (enet_socket_bind(m_socket, &m_addr_local) != 0)
    {
        LOG("[RoR|MotionPlatform] Failed to connect");
        this->DeleteSocket();
        return false;
    }

    m_elapsed_time = SEND_INTERVAL_MICROSEC; // Force update
    m_last_update_time = 0;

    LOG("[RoR|MotionPlatform] Connected");
    return true;
}

void MotionPlatform::MPlatformDisconnect()
{
    enet_socket_shutdown(m_socket, ENET_SOCKET_SHUTDOWN_READ_WRITE);
    this->DeleteSocket();
    LOG("[RoR|MotionPlatform] Disconnected");
}

void MotionPlatform::MPlatformUpdate(Beam* vehicle) // Called per physics tick (2000hz)
{

    m_elapsed_time += 500;
    if ((m_elapsed_time - m_last_update_time) < SEND_INTERVAL_MICROSEC)
    {
        return;
    }

    UdpElsaco1 datagram;
    datagram._unused    = Ogre::Vector3::ZERO;
    datagram.game       = reinterpret_cast<int32_t>(MPLATFORM_GAME_ID);
    datagram.time_milis = m_elapsed_time/1000; // microsec -> milisec

    // ## OGRE engine coords: Right-handed; X is right, Y is up (screen-like), Z is back
    // ## Motion plat. coords: Z is up, Y/X is mixed.

    // Readings
    Ogre::Vector3 cinecam_pos  = vehicle->GetCinecamPos(0); // Proof of concept: hardcoded to "cinecam" method
    Ogre::Vector3 coord_center = vehicle->GetCamcoordCenterPos(0); // Any vehicle has camera[0] + cinecam[0]
    Ogre::Vector3 roll_axis    = -(vehicle->GetCamcoordRearPos(0) - coord_center);
    Ogre::Vector3 pitch_axis   = -(vehicle->GetCamcoordLeftPos(0) - coord_center);
    Ogre::Vector3 yaw_axis     = pitch_axis.crossProduct(roll_axis);

    datagram.position_x = static_cast<int32_t>((cinecam_pos.x  * 10000.f) * UPDATES_PER_SEC);
    datagram.position_y = static_cast<int32_t>((-cinecam_pos.z * 10000.f) * UPDATES_PER_SEC);
    datagram.position_z = static_cast<int32_t>((cinecam_pos.y  * 10000.f) * UPDATES_PER_SEC);

    // Orientation
    Ogre::Matrix3 orient_mtx;
    orient_mtx.FromAxes(pitch_axis, yaw_axis, roll_axis);
    Ogre::Radian yaw, pitch, roll;
    orient_mtx.ToEulerAnglesYXZ(yaw, roll, pitch); // NOTE: This is probably swapped... Function args are(Y, P, R)
    datagram.orient.x = pitch.valueRadians();
    datagram.orient.y = roll.valueRadians();
    datagram.orient.z = yaw.valueRadians();

    // Velocity
    Ogre::Vector3 ogre_velocity = (cinecam_pos - m_last_cinecam_pos) * UPDATES_PER_SEC;
    datagram.velocity.x = ogre_velocity.x;
    datagram.velocity.y = -ogre_velocity.z;
    datagram.velocity.z = ogre_velocity.y;

    // Acceleration
    Ogre::Vector3 ogre_accel = (ogre_velocity - m_last_velocity) * UPDATES_PER_SEC;
    datagram.accel.x = ogre_accel.x;
    datagram.accel.y = -ogre_accel.z;
    datagram.accel.z = ogre_accel.y;

    // Send data
    ENetBuffer buf;
    buf.data       = static_cast<void*>(&datagram);
    buf.dataLength = sizeof(UdpElsaco1);
    if (enet_socket_send(m_socket, &m_addr_remote, &buf, 1) != 0)
    {
        LOG("[RoR|MotionPlatform] Failed to send data!");
    }

    // Remember values
    m_last_update_time   = m_elapsed_time;
    m_last_cinecam_pos   = cinecam_pos;
    m_last_orient_euler  = datagram.orient;
    m_last_velocity      = ogre_velocity;
    m_last_orient_matrix = orient_mtx;

    // Update stats
    m_velo_jitter.AddReading(ogre_velocity);
    m_pos_jitter.AddReading(cinecam_pos);
    m_acc_jitter.AddReading(ogre_accel);

    Ogre::Euler euler(orient_mtx);
    m_euler_jitter.AddReading(
        Ogre::Vector3(
            euler.GetYaw().valueRadians(),
            euler.GetPitch().valueRadians(),
            euler.GetRoll().valueRadians())
        );
}

float CalcJitter(float start, float mid, float end)
{
    float low, high;
    
    if (start > end)
    {
        high = start;
        low = end;
    }
    else
    {
        high = end;
        low = start;
    }

    if (high == low)
    {
        return 0.f; // cannot compute percentage - div by zero.
    }

    if (low < 0.f)
    {
        float drop = fabsf(low);
        high += drop;
        low += drop;
    }

    float diff = (high - low);
    float optim_mid = (diff/2.f) + low;

    return (fabsf(mid - optim_mid) / (diff/2)) * 100.f; // percentual error
}

void JitterCacheV3::AddReading(Ogre::Vector3 val)
{
    // Keep the last value
    last_udp_value = val;

    // Compute jitter
    results2[results_caret].x = CalcJitter(val.x, readings[0].x, readings[1].x); // Sequence of readings: START, MID, END
    results2[results_caret].y = CalcJitter(val.y, readings[0].y, readings[1].y);
    results2[results_caret].z = CalcJitter(val.z, readings[0].z, readings[1].z);

    results4[results_caret].x = CalcJitter(val.x, readings[1].x, readings[3].x); // START, n, MID, n, END
    results4[results_caret].y = CalcJitter(val.y, readings[1].y, readings[3].y);
    results4[results_caret].z = CalcJitter(val.z, readings[1].z, readings[3].z);

    results8[results_caret].x = CalcJitter(val.x, readings[3].x, readings[7].x); // START, n, n, n, MID, n, n, n, END
    results8[results_caret].y = CalcJitter(val.y, readings[3].y, readings[7].y);
    results8[results_caret].z = CalcJitter(val.z, readings[3].z, readings[7].z);

    // Move caret
    results_caret = ((results_caret + 1) % NUM_RESULTS);

    // Shove the old readings and insert new
    memmove((readings + 1), readings, 7 * sizeof(Ogre::Vector3));
    readings[0] = val;
}

JitterStatV3 JitterCacheV3::ProduceStats()
{
    Ogre::Vector3 total2, total4, total8;
    for (size_t i = 0; i < NUM_RESULTS; ++i)
    {
        total2 += results2[i];
        total4 += results4[i];
        total8 += results8[i];
    }

    JitterStatV3 stat;
    stat.stat2 = total2 / static_cast<float>(NUM_RESULTS);
    stat.stat4 = total4 / static_cast<float>(NUM_RESULTS);
    stat.stat8 = total8 / static_cast<float>(NUM_RESULTS);
    stat.last_udp = last_udp_value;
    return stat;
}

#endif // USE_MPLATFORM
