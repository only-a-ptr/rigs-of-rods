/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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
/// @date   12/2014

#pragma once

#include "ForwardDeclarations.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_Config.h"

#include <MyGUI_Common.h>
#include <MyGUI_Prerequest.h> // Forward declarations
#include <MyGUI_Window.h>

namespace RoR
{

class GuiPanelBase
{
    friend class RoR::GUIManager;

public:
    GuiPanelBase(MyGUI::Window* main_widget):
        m_panel_widget(main_widget),
        m_is_temporarily_hidden(false)
    {}

    inline void Show()
    {
        m_panel_widget->setVisible(true); 
        m_is_temporarily_hidden = false;
    }
    
    inline void HideTemporarily()
    { 
        if (m_panel_widget->getVisible())
        {
            m_panel_widget->setVisible(false); 
            m_is_temporarily_hidden = true; 
        }
    }

    inline void ShowIfHiddenTemporarily()
    { 
        if (m_is_temporarily_hidden) 
        { 
            Show();
        } 
    }

    void AlignToScreen(RigEditor::GuiPanelPositionData* position_data)
    {
	    MyGUI::IntSize screenSize = m_panel_widget->getParentSize();
	    int x = position_data->margin_left_px; // Anchor: left
	    int y = position_data->margin_top_px; // Anchor: top
	    if (position_data->anchor_right)
	    {
		    x = screenSize.width - GetWidthPixels() - position_data->margin_right_px;
	    }
	    if (position_data->anchor_bottom)
	    {
		    y = screenSize.height - GetHeightPixels() - position_data->margin_bottom_px;
	    }
	    SetPosition(x, y);
    }

    void StretchToScreen(RigEditor::GuiPanelPositionData* position_data, bool stretch_horizontal, bool stretch_vertical)
    {
	    this->AlignToScreen(position_data);
        MyGUI::IntSize screen_size = m_panel_widget->getParentSize();
        MyGUI::IntSize panel_size = m_panel_widget->getSize();
        if (stretch_horizontal)
        {
            panel_size.width -= (position_data->margin_left_px + position_data->margin_right_px);
        }
        if (stretch_vertical)
        {
            panel_size.height -= (position_data->margin_top_px + position_data->margin_bottom_px);
        }
        m_panel_widget->setSize(panel_size);
    }

    inline void Hide()                    { m_panel_widget->setVisible(false); }
    inline bool IsVisible() const         { return m_panel_widget->getVisible(); }
    inline int  GetWidthPixels() const    { return GetSizePixels().width; }
    inline int  GetHeightPixels() const   { return GetSizePixels().height; }

    inline void SetPosition(int x_pixels, int y_pixels)  { m_panel_widget->setPosition(x_pixels, y_pixels); }
    inline void SetWidth(int width_pixels)               { m_panel_widget->setSize(width_pixels, GetHeightPixels()); }
    inline void SetHeight(int height_pixels)             { m_panel_widget->setSize(GetWidthPixels(), height_pixels); }

    inline MyGUI::IntSize GetSizePixels() const          { return m_panel_widget->getSize(); }
    
    inline void CenterToScreen()
    {
        MyGUI::IntSize parentSize = m_panel_widget->getParentSize();
        SetPosition((parentSize.width - GetWidthPixels()) / 2, (parentSize.height - GetHeightPixels()) / 2);
    }

protected:
    MyGUI::Window* m_panel_widget;
    bool           m_is_temporarily_hidden;
};

} // namespace RoR
