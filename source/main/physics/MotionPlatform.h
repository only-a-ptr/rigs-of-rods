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

/// @file
/// @author Petr Ohlidal
/// @date 02/2017
/// @brief Motion platform integration
///
/// This implementation is based on motion simulator hardware & software
/// by Elsaco s.r.o, Czech Republic (http://motion-sim.cz/new)
///
///     THEORY
/// RoR's vehicles are entirely soft-body structures; there is no definite
/// 'front' or 'up', everything can and will deform. See also
/// http://docs.rigsofrods.org/vehicle-creation/vehicle-concepts/
/// To acquire orientation in space, each vehicle has 1 node marked "center"
/// and 2 other marked "left" and "back". This mechanism is known as "camera":
/// http://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#cameras
///
///     CONVENTIONS
/// In both physics world and 3d engine, distance of 1.0 means "1 meter".
/// OGRE engine uses OpenGL-like RHS coordinates: Y is up, X is right, -Z is forward
/// All readings and GUI visualizations are in RHS coordinates: Z is up, X is right, Y is forward
/// Therefore orientation vector (separate Yaw/Pitch/Roll) will be:
///    Yaw: Z, Pitch: X, Roll: Y
/// RoR's vehicles are defined in the following coordinates (used by Editorizer tool):
///    Forward: -X, Right: +Z, Up: +Y
///
///     RETRIEVING MOTION DATA
///  Method in place: use the "MotionFeeder" node-graph visual editor. User specifies node numbers to read from
/// and designs the process of calculating the outputs.

#pragma once

#ifdef USE_MPLATFORM

#include "ForwardDeclarations.h"
#include <OgreVector3.h> // { float x,y,z; }
#include <enet\enet.h> // UDP networking
#include <stdint.h>

namespace RoR {

struct UdpElsaco1
{
    int32_t       time_milis;   ///< Time since game start (miliseconds)
    int32_t       position_x;   ///< World position (meters/10000)
    int32_t       position_y;   ///< World position (meters/10000)
    int32_t       position_z;   ///< World position (meters/10000)
    Ogre::Vector3 velocity;     ///< World velocity m.s-1
    Ogre::Vector3 _unused;
    Ogre::Vector3 accel;        ///< World acceleration, m.s-2
    Ogre::Vector3 orient;       ///< orientation vector (x=pitch, y=roll, z=yaw), radians
    int32_t       game;         ///< 4 letter game identification, e.g. "GAME" would be 0x71657769
};

const char    MPLATFORM_GAME_ID[] = {'R','o','R',' '};
const size_t  MPLATFORM_SEND_RATE = 60; ///< Number of datagrams per second.

class MotionPlatform
{
public:
    MotionPlatform();

    bool MPlatformConnect     ();
    void MPlatformDisconnect  ();
    void MPlatformSetActive   (bool active);
    void MPlatformUpdate      (Beam* vehicle);

private:
    void  DeleteSocket  ();

    ENetAddress   m_addr_remote;
    ENetAddress   m_addr_local;
    ENetSocket    m_socket;
    size_t        m_elapsed_time; ///< Microseconds
    size_t        m_last_update_time; ///< Microseconds
    Ogre::Vector3 m_last_cinecam_pos;
    Ogre::Vector3 m_last_velocity;
    Ogre::Vector3 m_last_orient_euler;
    Ogre::Matrix3 m_last_orient_matrix;
};

} // namespace RoR

#endif // USE_MPLATFORM

