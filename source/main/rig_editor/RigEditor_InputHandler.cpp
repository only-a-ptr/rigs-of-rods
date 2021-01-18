/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
    @file   RigEditor_InputHandler.cpp
    @date   06/2014
    @author Petr Ohlidal
*/

#include "RigEditor_InputHandler.h"

#include "Application.h"
#include "GUIManager.h"
#include "InputEngine.h"

#include <MyGUI_InputManager.h>

using namespace RoR;
using namespace RoR::RigEditor;

// ================================================================================
// Static variables
// ================================================================================

#define DECLARE_EVENT(_FIELD_, _INDEX_, _NAME_) const InputHandler::Event InputHandler::Event::_FIELD_(_INDEX_, _NAME_);

DECLARE_EVENT (                           INVALID,   0,                             "INVALID" )
DECLARE_EVENT (                            ESCAPE,   5,                              "ESCAPE" )
DECLARE_EVENT (      NODES_DESELECT_OR_SELECT_ALL,   6,        "NODES_DESELECT_OR_SELECT_ALL" )
DECLARE_EVENT (              GUI_SHOW_DELETE_MENU,   7,                "GUI_SHOW_DELETE_MENU" )
DECLARE_EVENT (            NODES_EXTRUDE_SELECTED,   8,              "NODES_EXTRUDE_SELECTED" )

#define DECLARE_MODE(_FIELD_, _INDEX_, _NAME_, _KEY_ENTERS_, _KEY_EXITS_) const InputHandler::Mode InputHandler::Mode::_FIELD_(_INDEX_, _NAME_, _KEY_ENTERS_, _KEY_EXITS_);

DECLARE_MODE (                            INVALID,   0,                             "INVALID",   true,   true  )
DECLARE_MODE (                    CREATE_NEW_NODE,   1,                     "CREATE_NEW_NODE",   true,   true  )
DECLARE_MODE (                         GRAB_NODES,   2,                          "GRAB_NODES",   true,  false  )

// ================================================================================
// Functions
// ================================================================================

Vector2int InputHandler::MouseMotionEvent::GetPreviousAbsolutePosition() const
{
    return Vector2int( abs_x - rel_x, abs_y - rel_y );
}

InputHandler::MouseMotionEvent::MouseMotionEvent():
    rel_x(0),
    rel_y(0),
    rel_wheel(0),
    abs_x(0),
    abs_y(0),
    abs_wheel(0)
{
}

InputHandler::InputHandler()
{
    std::memset(m_event_key_mappings, 0, KEY_MAPPING_ARRAY_SIZE * sizeof(Event *)); 
    std::memset(m_mode_key_mappings, 0, KEY_MAPPING_ARRAY_SIZE * sizeof(Mode *)); 

    SetupDefaultKeyMappings();
}

void InputHandler::SetupDefaultKeyMappings()
{
    m_event_key_mappings[OIS::KC_A]       = & Event::NODES_DESELECT_OR_SELECT_ALL;
    m_event_key_mappings[OIS::KC_DELETE]  = & Event::GUI_SHOW_DELETE_MENU;
    m_event_key_mappings[OIS::KC_E]       = & Event::NODES_EXTRUDE_SELECTED;
    m_event_key_mappings[OIS::KC_ESCAPE]  = & Event::ESCAPE;

    m_mode_key_mappings[OIS::KC_N]        = & Mode::CREATE_NEW_NODE;
    m_mode_key_mappings[OIS::KC_G]        = & Mode::GRAB_NODES;
}

bool InputHandler::WasEventFired(Event const & event)
{
    return m_events_fired.test(event.index);
}

void InputHandler::ResetEvents()
{
    m_events_fired.reset();

    m_mouse_motion_event.ResetRelativeMove();
    m_mouse_motion_event.ResetRelativeScroll();

    m_mouse_button_event.ResetEvents();

    m_modes_entered.reset();
    m_modes_exited.reset();
}

InputHandler::MouseMotionEvent const & InputHandler::GetMouseMotionEvent()
{
    return m_mouse_motion_event;
}

InputHandler::MouseButtonEvent const & InputHandler::GetMouseButtonEvent()
{
    return m_mouse_button_event;
}

bool InputHandler::IsModeActive(Mode const & mode)
{
    return m_active_modes[mode.index];
}

bool InputHandler::WasModeEntered(Mode const & mode)
{
    return m_modes_entered[mode.index];
}
    
bool InputHandler::WasModeExited(Mode const & mode)
{
    return m_modes_exited[mode.index];
}

void InputHandler::EnterMode(Mode const & mode)
{
    m_active_modes[mode.index] = true;
    m_modes_entered[mode.index] = true;
}

void InputHandler::ExitMode(Mode const & mode)
{
    m_active_modes[mode.index] = false;
    m_modes_exited[mode.index] = true;
}

// ================================================================================
// OIS Keyboard listener
// ================================================================================

bool InputHandler::keyPressed( const OIS::KeyEvent &arg )
{
    // HANDLE EVENTS \\

    const Event* event_ptr = m_event_key_mappings[arg.key];
    if (event_ptr != nullptr) 
    {
        m_events_fired[event_ptr->index] = true;
    }
    
    // HANDLE MODES \\

    const Mode* mode_ptr = m_mode_key_mappings[arg.key];
    if (mode_ptr != nullptr && mode_ptr->key_enters) 
    {
        m_active_modes[mode_ptr->index] = true;
        m_modes_entered[mode_ptr->index] = true;
    }

    return true;
}

bool InputHandler::keyReleased( const OIS::KeyEvent &arg )
{
    // HANDLE MODES \\

    const Mode* mode_ptr = m_mode_key_mappings[arg.key];
    if (mode_ptr != nullptr && mode_ptr->key_exits) 
    {
        m_active_modes[mode_ptr->index] = false;
        m_modes_exited[mode_ptr->index] = true;
    }

    return true;
}

