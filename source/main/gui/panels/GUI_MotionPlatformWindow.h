/*
    This source file is part of Rigs of Rods
    Copyright 2017 Petr Ohlidal & contributors

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
/// @date   02/2017

#pragma once

#include "ForwardDeclarations.h"
#include "GUI_MotionPlatformWindowLayout.h"
#include "MotionPlatform.h"

namespace RoR {
namespace GUI {

class MotionPlatformWindow: public MotionPlatformWindowLayout
{
public:
    MotionPlatformWindow();

    void SetVisible(bool v);
    bool IsVisible();

    void SetDisplayText(const char* text);

    void UpdateMPlatformGui();
    void SetMPlatformController(RoR::MotionPlatform* obj) { m_motion_platform = obj; }

private:
    void OnWindowButtonClick(MyGUI::WidgetPtr _sender, const std::string& _name);

    MyGUI::RotatingSkin* m_rot_yaw;
    MyGUI::RotatingSkin* m_rot_pitch;
    MyGUI::RotatingSkin* m_rot_roll;

    RoR::MotionPlatform* m_motion_platform;
};

} // namespace GUI
} // namespace RoR
