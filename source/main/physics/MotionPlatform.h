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
/// In both physics world and 3d engine, distance of 1.0 means "1 meter".
/// RoR's vehicles are entirely soft-body structures; there is no definite
/// 'front' or 'up', everything can and will deform. See also
/// http://docs.rigsofrods.org/vehicle-creation/vehicle-concepts/
/// To acquire orientation in space, each vehicle has 1 node marked "center"
/// and 2 other marked "left" and "back". This mechanism is known as "camera":
/// http://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#cameras
///
///     RETRIEVING MOTION DATA
/// There are several possibilities:
/// 1. Cinecam - RoR's 'cockpit camera' - a single node tied to 8 other; see
///    http://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#cinecam
///    It's position is paired with data from "camera" mechanism, see above.
///    Advantage: single source of image+motion data.
///    Caveat: The camera may slightly swing against the vehicle.
/// 2. Cinecam base nodes - same as above except the position is interpolated
///    from binding nodes rather than the camera node.
///    Caveat: camera swings -> slight mismatch between motion and image.
/// 3. Cinecam + DriverSeat-prop - Read motion data from a "prop" (static 3d
///    mesh attached to the softbody), specifically the "driver seat" prop
///    (http://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#props)

#pragma once

#ifdef USE_MPLATFORM

#include "ForwardDeclarations.h"
#include <OgreVector3.h> // { float x,y,z; }
#include <enet\enet.h> // UDP networking
#include <stdint.h>

namespace RoR {

struct DBox
{
    int32_t       time_sec;     // time in seconds since game start
    int32_t       position_x;   // position (meters/10000)
    int32_t       position_y;
    int32_t       position_z;
    Ogre::Vector3 velocity;    // velocity m.s-1
    Ogre::Vector3 angular_vel; // angular velocity, rad/s
    Ogre::Vector3 accel;       // acceleration, m.s-2
    Ogre::Vector3 orient;      // orientation vector, radians
    int32_t       game;        // 4 letter game identification, e.g. "GAME" would be 0x71657769
};

const char    MPLATFORM_GAME_ID[] = {'R','o','R',' '};
const size_t  MPLATFORM_SEND_RATE = 60; ///< Datagrams per second.

class MotionPlatform
{
public:
    MotionPlatform();

    bool  MPlatformConnect     ();
    void  MPlatformDisconnect  ();
    void  MPlatformSetActive   (bool active);
    void  MPlatformUpdate      (Beam* vehicle);

private:
    void  DeleteSocket  ();

    ENetAddress   m_addr_remote;
    ENetAddress   m_addr_local;
    ENetSocket    m_socket;
    size_t        m_elapsed_time;
    size_t        m_last_update_time;
    Ogre::Vector3 m_last_cinecam_pos;
    Ogre::Vector3 m_last_velocity;
    Ogre::Vector3 m_last_orient_euler;
};

} // namespace RoR

#endif // USE_MPLATFORM

