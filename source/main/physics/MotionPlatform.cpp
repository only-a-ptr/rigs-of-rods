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

#include <math.h>

using namespace RoR;

// RoR's physics frequency is 2000hz (0.0005 sec)

const size_t SEND_INTERVAL_MICROSEC = 1000000/MPLATFORM_SEND_RATE;
const float  SEND_INTERVAL_SEC = 1.f/static_cast<float>(MPLATFORM_SEND_RATE);

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
    m_addr_remote.host = ENET_HOST_TO_NET_32(0x7F000001); // = 127.0.0.1 localhost
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

    DBox datagram;
    datagram.game       = reinterpret_cast<int32_t>(MPLATFORM_GAME_ID);
    datagram.time_sec   = m_elapsed_time/1000000; // Microsec->sec
    datagram.position_x = 0; // Direct positioning - not used;
    datagram.position_y = 0;
    datagram.position_z = 0;

    // ## OGRE engine coords: Right-handed; X is right, Y is up (screen-like), Z is back
    // ## Motion plat. coords: Z is up, X is ??? 

    // Readings
    Ogre::Vector3 cinecam_pos  = vehicle->GetCinecamPos(0); // Proof of concept: hardcoded to "cinecam" method
    Ogre::Vector3 coord_center = vehicle->GetCamcoordCenterPos(0); // Any vehicle has camera[0] + cinecam[0]
    Ogre::Vector3 roll_axis    = -(vehicle->GetCamcoordRearPos(0) - coord_center);
    Ogre::Vector3 pitch_axis   = -(vehicle->GetCamcoordLeftPos(0) - coord_center);
    Ogre::Vector3 yaw_axis     = pitch_axis.crossProduct(roll_axis);

    // Orientation
    Ogre::Matrix3 orient_mtx;
    orient_mtx.FromAxes(pitch_axis, yaw_axis, roll_axis);
    Ogre::Radian yaw, pitch, roll;
    orient_mtx.ToEulerAnglesXYZ(yaw, pitch, roll);
    datagram.orient.x = roll.valueRadians();
    datagram.orient.y = pitch.valueRadians();
    datagram.orient.z = yaw.valueRadians();

    // Velocity
    datagram.velocity = (cinecam_pos - m_last_cinecam_pos) * SEND_INTERVAL_SEC;

    // Acceleration
    datagram.accel = (datagram.velocity - m_last_velocity) * SEND_INTERVAL_SEC;

    // Angular velocity
    datagram.angular_vel = (datagram.orient - m_last_orient_euler) * SEND_INTERVAL_SEC;

    // Send data
    ENetBuffer buf;
    buf.data       = static_cast<void*>(&datagram);
    buf.dataLength = sizeof(DBox);
    if (enet_socket_send(m_socket, &m_addr_remote, &buf, 1) != 0)
    {
        LOG("[RoR|MotionPlatform] Failed to send data!");
    }

    // Debug display
    // !!TODO!!: Don't do this inside physics update!
    char text_buf[1000];
    snprintf(text_buf, 1000, "Time: %03d \n\n"
        " oriX: %8.4f\n oriY: %8.4f\n oriZ: %8.4f\n\n velX: %8.4f\n velY: %8.4f\n velZ: %8.4f\n"
        " accX: %8.4f\n accY: %6.f\n accZ: %8.4f\n\n angVX: %8.4f\n angVY: %8.4f\n angVZ: %8.4f",
        datagram.time_sec,
        datagram.orient.x,      datagram.orient.y,      datagram.orient.z,
        datagram.velocity.x,    datagram.velocity.y,    datagram.velocity.z,
        datagram.accel.x,       datagram.accel.y,       datagram.accel.z,
        datagram.angular_vel.x, datagram.angular_vel.y, datagram.angular_vel.z);
    App::GetGuiManager()->GetMotionPlatform()->SetDisplayText(text_buf);

    // Remember values
    m_last_update_time  = m_elapsed_time;
    m_last_cinecam_pos  = cinecam_pos;
    m_last_orient_euler = datagram.orient;
    m_last_velocity     = datagram.velocity;
}

#endif // USE_MPLATFORM
