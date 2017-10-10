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
#include "Euler.h"
#include "NodeGraphTool.h"

#include <math.h>

using namespace RoR;

// RoR's physics frequency is 2000hz (0.0005 sec)

const size_t SEND_INTERVAL_MICROSEC = 1000000/MPLATFORM_SEND_RATE;
const float  UPDATES_PER_SEC        = static_cast<float>(MPLATFORM_SEND_RATE);

bool G_motionsim_connected = false;


MotionPlatform::MotionPlatform():
    m_socket(ENET_SOCKET_NULL),
    m_elapsed_time(0),
    m_last_update_time(0),
    m_last_send_result(0),
    m_connected(false)
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
    //?? const char IP[] = {char(192),char(168),char(223),char(101)};
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

    m_connected = true;
    return true;
}

void MotionPlatform::MPlatformDisconnect()
{
    enet_socket_shutdown(m_socket, ENET_SOCKET_SHUTDOWN_READ_WRITE);
    this->DeleteSocket();
    LOG("[RoR|MotionPlatform] Disconnected");
    m_connected = false;
}

void MotionPlatform::MPlatformUpdate(Beam* actor) // Called per physics tick (2000hz)
{
    m_elapsed_time += 500;

    NodeGraphTool* feeder = App::GetGuiManager()->GetMotionFeeder();
    feeder->PhysicsTick(actor);

    if ((m_elapsed_time - m_last_update_time) < SEND_INTERVAL_MICROSEC)
    {
        return;
    }

    RoR::NodeGraphTool::RefImplDisplayNode* demo_node = feeder->GetDemoNode();
    if (demo_node != nullptr)
    {
        demo_node->CalcUdpPacket(
            m_elapsed_time,
            actor->GetCamcoordCenterPos(0),  // Any vehicle has camera[0] + cinecam[0]
            actor->GetCamcoordRearPos(0),
            actor->GetCamcoordLeftPos(0),
            actor->GetCinecamPos(0)); // Proof of concept: hardcoded to "cinecam" method

        if (demo_node->IsUdpEnabled())
        {
            // Send data
            ENetBuffer buf;
            buf.data       = static_cast<void*>(&demo_node->GetDatagram());
            buf.dataLength = sizeof(DatagramDboxRorx);

            // `enet_socket_send()` (Win32 implmentation) returns number of bytes sent on success, 0 on WSAEWOULDBLOCK and -1 on error.
            m_last_send_result = enet_socket_send(m_socket, &m_addr_remote, &buf, 1);

            return;
        }
    }

    DatagramDboxRorx datagram;
    datagram._unused    = Ogre::Vector3::ZERO;
    datagram.game       = reinterpret_cast<int32_t>(MPLATFORM_GAME_ID);
    datagram.time_milis = m_elapsed_time/1000; // microsec -> milisec

    // ## OGRE engine coords: Right-handed; X is right, Y is up (screen-like), Z is back
    // ## Motion plat. coords: Z is up, Y/X is mixed.

    // NOTE: The output must be in (meters*10000*60) --- mistake from proof-of-concept
    datagram.position_x = static_cast<int32_t>((feeder->udp_position_node.Capture(0)));
    datagram.position_y = static_cast<int32_t>((feeder->udp_position_node.Capture(1)));
    datagram.position_z = static_cast<int32_t>((feeder->udp_position_node.Capture(2)));

    // Orientation
    datagram.orient.x = feeder->udp_orient_node.Capture(0); // Roll
    datagram.orient.y = feeder->udp_orient_node.Capture(1); // Pitch (range 0-1.0: mistake from proof-of-concept)
    datagram.orient.z = feeder->udp_orient_node.Capture(2); // Yaw.

    // Velocity
    datagram.velocity.x = feeder->udp_velocity_node.Capture(0); // Must be transformed to (m/s) by MotionFeeder
    datagram.velocity.y = feeder->udp_velocity_node.Capture(1); // Must be transformed to (m/s) by MotionFeeder
    datagram.velocity.z = feeder->udp_velocity_node.Capture(2); // Must be transformed to (m/s) by MotionFeeder

    // Acceleration
    datagram.accel.x = feeder->udp_accel_node.Capture(0); // Must be transformed to (m/s^2) by MotionFeeder
    datagram.accel.y = feeder->udp_accel_node.Capture(1); // Must be transformed to (m/s^2) by MotionFeeder
    datagram.accel.z = feeder->udp_accel_node.Capture(2); // Must be transformed to (m/s^2) by MotionFeeder

    // Send data
    ENetBuffer buf;
    buf.data       = static_cast<void*>(&datagram);
    buf.dataLength = sizeof(DatagramDboxRorx);

    // `enet_socket_send()` (Win32 implmentation) returns number of bytes sent on success, 0 on WSAEWOULDBLOCK and -1 on error.
    m_last_send_result = enet_socket_send(m_socket, &m_addr_remote, &buf, 1);
}

#endif // USE_MPLATFORM
