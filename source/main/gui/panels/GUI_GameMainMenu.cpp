/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @date   06/2017

#include "GUI_GameMainMenu.h"
#include <fmt/format.h>

#include "Application.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "Language.h"

#include "PlatformUtils.h"
#include "RoRVersion.h"
#include "RoRnet.h" // for version string

using namespace RoR;
using namespace GUI;

GameMainMenu::GameMainMenu():
    m_is_visible(false), m_num_buttons(5), m_kb_focus_index(-1), m_kb_enter_index(-1)
{
    if (FileExists(PathCombine(App::sys_savegames_dir->GetStr(), "autosave.sav")))
    {
        m_num_buttons++;
    }
}

void GameMainMenu::Draw()
{
    this->DrawMenuPanel();
    this->DrawVersionBox();
}

void GameMainMenu::DrawMenuPanel()
{
    // Keyboard updates - move up/down and wrap on top/bottom. Initial index is '-1' which means "no focus"
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
    {
        m_kb_focus_index = (m_kb_focus_index <= 0) ? (m_num_buttons - 1) : (m_kb_focus_index - 1);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
    {
        m_kb_focus_index = (m_kb_focus_index < (m_num_buttons - 1)) ? (m_kb_focus_index + 1) : 0;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
    {
        m_kb_enter_index = m_kb_focus_index;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BUTTON_PADDING);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, WINDOW_BG_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Button, BUTTON_BG_COLOR);

    // Display in bottom left corner for single-screen setups and centered for multi-screen (typically 3-screen) setups.
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    if ((display_size.x > 2200.f) && (display_size.y < 1100.f)) // Silly approximate values
    {
        ImGui::SetNextWindowPosCenter();
    }
    else
    {
        const float btn_height = ImGui::GetTextLineHeight() + (BUTTON_PADDING.y * 2);
        const float window_height = (6*btn_height) + (5*ImGui::GetStyle().ItemSpacing.y) + (2*ImGui::GetStyle().WindowPadding.y); // 5 buttons + titlebar; 2x spacing around separator
        const float margin = display_size.y / 15.f;
        const float top = display_size.y - window_height - margin;
        ImGui::SetNextWindowPos(ImVec2(margin, top));
    }
    ImGui::SetNextWindowContentWidth(WINDOW_WIDTH);
    int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin(_LC("MainMenu", "Main menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
    {
        int button_index = 0;
        ImVec2 btn_size(WINDOW_WIDTH, 0.f);

        if (HighlightButton(_LC("MainMenu", "Single player"), btn_size, button_index++))
        {
            this->SetVisible(false);
            if (App::diag_preset_terrain->GetStr().empty())
            {
                RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                m.payload = reinterpret_cast<void*>(new LoaderType(LT_Terrain));
                App::GetGameContext()->PushMessage(m);
            }
            else
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_TERRN_REQUESTED, App::diag_preset_terrain->GetStr()));
            }
        }

        if (FileExists(PathCombine(App::sys_savegames_dir->GetStr(), "autosave.sav")))
        {
            if ( HighlightButton(_LC("MainMenu", "Resume game"), btn_size, button_index++))
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, "autosave.sav"));
                this->SetVisible(false);
            }
        }

        if (HighlightButton(_LC("MainMenu", "Multiplayer"), btn_size, button_index++))
        {
            App::GetGuiManager()->SetVisible_MultiplayerSelector(true);
            this->SetVisible(false);
        }

        if (HighlightButton(_LC("MainMenu", "Settings"), btn_size, button_index++))
        {
            App::GetGuiManager()->SetVisible_GameSettings(true);
            this->SetVisible(false);
        }

        if (HighlightButton(_LC("MainMenu", "About"), btn_size, button_index++))
        {
            App::GetGuiManager()->SetVisible_GameAbout(true);
            this->SetVisible(false);
        }

        if (HighlightButton(_LC("MainMenu", "Exit game"), btn_size, button_index))
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
            this->SetVisible(false);
        }
    }

    if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
    {
        this->SetVisible(false);
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    m_kb_enter_index = -1;
}

void GameMainMenu::DrawVersionBox()
{
    const float margin = ImGui::GetIO().DisplaySize.y / 30.f;
    std::string game_ver   = fmt::format("{}: {}", _LC("MainMenu", "Game version"), ROR_VERSION_STRING);
    std::string rornet_ver = fmt::format("{}: {}", _LC("MainMenu", "Net. protocol"), RORNET_VERSION);
    float text_w = std::max(
        ImGui::CalcTextSize(game_ver.c_str()).x, ImGui::CalcTextSize(rornet_ver.c_str()).x);
    ImVec2 box_size(
        (2 * ImGui::GetStyle().WindowPadding.y) + text_w,
        (2 * ImGui::GetStyle().WindowPadding.y) + (2 * ImGui::GetTextLineHeight()));
    ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize - (box_size + ImVec2(margin, margin)));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, WINDOW_BG_COLOR);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoInputs;
    if (ImGui::Begin(_LC("MainMenu", "Version box"), nullptr, flags))
    {
        ImGui::Text("%s", game_ver.c_str());
        ImGui::Text("%s", rornet_ver.c_str());
        ImGui::End();
    }
    ImGui::PopStyleColor(1);
}

bool GameMainMenu::HighlightButton(const std::string& txt,ImVec2 btn_size, int index) const{
    std::string button_txt = (m_kb_focus_index == index) ? fmt::format("--> {} <--", txt) : txt;
    return ImGui::Button(button_txt.c_str(), btn_size) || (m_kb_enter_index == index);
}