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

#include "GUI_MotionPlatformWindow.h"

#include "Euler.h"

#define MAIN_WIDGET ((MyGUI::Window*)m_window)

using namespace RoR::GUI;

MotionPlatformWindow::MotionPlatformWindow()
{
    MAIN_WIDGET->eventWindowButtonPressed += MyGUI::newDelegate(this, &MotionPlatformWindow::OnWindowButtonClick); //The window's [X] button

    this->SetVisible(false);
    MAIN_WIDGET->setPosition(50, 50);

    // Setup icons

    m_yaw_icon  ->setImageTexture("motion-yaw.png");
    m_pitch_icon->setImageTexture("motion-pitch.png");
    m_roll_icon ->setImageTexture("motion-roll.png");

    m_rot_yaw   = m_yaw_icon  ->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
    m_rot_pitch = m_pitch_icon->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
    m_rot_roll  = m_roll_icon ->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();

    m_rot_yaw  ->setCenter(MyGUI::IntPoint(40, 40));
    m_rot_pitch->setCenter(MyGUI::IntPoint(40, 40));
    m_rot_roll ->setCenter(MyGUI::IntPoint(40, 40));
}

void MotionPlatformWindow::SetVisible(bool v)
{
    MAIN_WIDGET->setVisible(v);
}

void MotionPlatformWindow::SetDisplayText(const char* text)
{
    m_text_panel->setCaption(text);
}

void MotionPlatformWindow::OnWindowButtonClick(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    this->SetVisible(false);
}

bool MotionPlatformWindow::IsVisible() { return MAIN_WIDGET->getVisible(); }

void MotionPlatformWindow::UpdateOrientationDisplay(Ogre::Matrix3 const & orient_matrix)
{
    // Input:   (x=pitch, y=yaw, z=roll)
    // Display: (x=pitch, y=roll, z=yaw)

    Ogre::Euler euler(orient_matrix);
    m_rot_yaw  ->setAngle(euler.GetYaw().valueRadians());
    m_rot_pitch->setAngle(euler.GetPitch().valueRadians());
    m_rot_roll ->setAngle(euler.GetRoll().valueRadians() * -1);

    // Just output font test...
    std::stringstream msg;
    msg << "yaw  : " << euler.GetYaw().valueRadians() << std::endl;
    msg << "pitch: " << euler.GetPitch().valueRadians() << std::endl;
    msg << "roll : " << euler.GetRoll().valueRadians() << std::endl;
    m_text_panel->setCaption(msg.str());
}
