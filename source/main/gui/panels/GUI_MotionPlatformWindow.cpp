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

void MotionPlatformWindow::UpdateMPlatformGui()
{

    Ogre::Euler euler(m_motion_platform->MPlatformGetLastOrientMatrix());
    m_rot_yaw  ->setAngle(euler.GetYaw().valueRadians());
    m_rot_pitch->setAngle(euler.GetPitch().valueRadians());
    m_rot_roll ->setAngle(euler.GetRoll().valueRadians() * -1);

    JitterStatV3 cine_stat = m_motion_platform->MPlatformGetJitterPos().ProduceStats();
    JitterStatV3 velo_stat = m_motion_platform->MPlatformGetJitterVelo().ProduceStats();
    JitterStatV3 accl_stat = m_motion_platform->MPlatformGetJitterAcc().ProduceStats();
    JitterStatV3 eurl_stat = m_motion_platform->MPlatformGetJitterAcc().ProduceStats();

    const size_t BUF_LEN = 4000;
    char buf[BUF_LEN];
    snprintf(buf, BUF_LEN,
        "Pos(cinecam):\n"
        " Cur | %10.5f | %10.5f | %10.5f\n"
        " j~3 | %10.5f | %10.5f | %10.5f\n"
        " j~5 | %10.5f | %10.5f | %10.5f\n"
        " j~9 | %10.5f | %10.5f | %10.5f\n"
        "\nVelo:\n"
        " Cur | %10.5f | %10.5f | %10.5f\n"
        " j~3 | %10.5f | %10.5f | %10.5f\n"
        " j~5 | %10.5f | %10.5f | %10.5f\n"
        " j~9 | %10.5f | %10.5f | %10.5f\n"
        "\nAcc:\n"
        " Cur | %10.5f | %10.5f | %10.5f\n"
        " j~3 | %10.5f | %10.5f | %10.5f\n"
        " j~5 | %10.5f | %10.5f | %10.5f\n"
        " j~9 | %10.5f | %10.5f | %10.5f\n"
        "\nEuler:\n"
        " YPR | %10.5f | %10.5f | %10.5f\n"
        " j~3 | %10.5f | %10.5f | %10.5f\n"
        " j~5 | %10.5f | %10.5f | %10.5f\n"
        " j~9 | %10.5f | %10.5f | %10.5f\n",
        // Pos
        cine_stat.last_udp.x, cine_stat.last_udp.y, cine_stat.last_udp.z,
           cine_stat.stat2.x,    cine_stat.stat2.y,    cine_stat.stat2.z,
           cine_stat.stat4.x,    cine_stat.stat4.y,    cine_stat.stat4.z,
           cine_stat.stat8.x,    cine_stat.stat8.y,    cine_stat.stat8.z,
        // velo
        velo_stat.last_udp.x, velo_stat.last_udp.y, velo_stat.last_udp.z,
           velo_stat.stat2.x,    velo_stat.stat2.y,    velo_stat.stat2.z,
           velo_stat.stat4.x,    velo_stat.stat4.y,    velo_stat.stat4.z,
           velo_stat.stat8.x,    velo_stat.stat8.y,    velo_stat.stat8.z,
        // Acc
        accl_stat.last_udp.x, accl_stat.last_udp.y, accl_stat.last_udp.z,
           accl_stat.stat2.x,    accl_stat.stat2.y,    accl_stat.stat2.z,
           accl_stat.stat4.x,    accl_stat.stat4.y,    accl_stat.stat4.z,
           accl_stat.stat8.x,    accl_stat.stat8.y,    accl_stat.stat8.z,
        // Euler
        eurl_stat.last_udp.x, eurl_stat.last_udp.y, eurl_stat.last_udp.z,
           eurl_stat.stat2.x,    eurl_stat.stat2.y,    eurl_stat.stat2.z,
           eurl_stat.stat4.x,    eurl_stat.stat4.y,    eurl_stat.stat4.z,
           eurl_stat.stat8.x,    eurl_stat.stat8.y,    eurl_stat.stat8.z
        );

    m_text_panel->setCaption(buf);
}
