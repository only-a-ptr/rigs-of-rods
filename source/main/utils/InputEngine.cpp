/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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

#include "InputEngine.h"

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

#include "Application.h"
#include "ErrorUtils.h"
#include "OgreSubsystem.h"
#include "RoRWindowEventUtilities.h"
#include "Settings.h"

#include <Ogre.h>
#include <OgreStringConverter.h>
#include <OgreException.h>

#include "GUIManager.h"
#include "Language.h"

const char* OIS_DEVICE_TYPE[6] = {"Unknown Device", "Keyboard", "Mouse", "JoyStick", "Tablet", "Other Device"};

// LOOOONG list of possible m_events. see the struct type for the structure ;)
eventInfo_t eventInfo[] = {
    {
        "AIRPLANE_STEER_RIGHT",
        EV_AIRPLANE_STEER_RIGHT,
        "Keyboard RIGHT",
        _L("steer right")
    },
    {
        "AIRPLANE_BRAKE",
        EV_AIRPLANE_BRAKE,
        "Keyboard B",
        _L("normal brake for an aircraft")
    },
    {
        "AIRPLANE_ELEVATOR_DOWN",
        EV_AIRPLANE_ELEVATOR_DOWN,
        "Keyboard DOWN",
        _L("pull the elevator down in an aircraft.")
    },
    {
        "AIRPLANE_ELEVATOR_UP",
        EV_AIRPLANE_ELEVATOR_UP,
        "Keyboard UP",
        _L("pull the elevator up in an aircraft.")
    },
    {
        "AIRPLANE_FLAPS_FULL",
        EV_AIRPLANE_FLAPS_FULL,
        "Keyboard CTRL+2",
        _L("full flaps in an aircraft.")
    },
    {
        "AIRPLANE_FLAPS_LESS",
        EV_AIRPLANE_FLAPS_LESS,
        "Keyboard EXPL+1",
        _L("one step less flaps.")
    },
    {
        "AIRPLANE_FLAPS_MORE",
        EV_AIRPLANE_FLAPS_MORE,
        "Keyboard EXPL+2",
        _L("one step more flaps.")
    },
    {
        "AIRPLANE_FLAPS_NONE",
        EV_AIRPLANE_FLAPS_NONE,
        "Keyboard CTRL+1",
        _L("no flaps.")
    },
    {
        "AIRPLANE_PARKING_BRAKE",
        EV_AIRPLANE_PARKING_BRAKE,
        "Keyboard P",
        _L("airplane parking brake.")

    },
    {
        "AIRPLANE_REVERSE",
        EV_AIRPLANE_REVERSE,
        "Keyboard R",
        _L("reverse the turboprops")
    },
    {
        "AIRPLANE_RUDDER_LEFT",
        EV_AIRPLANE_RUDDER_LEFT,
        "Keyboard Z",
        _L("rudder left")
    },
    {
        "AIRPLANE_RUDDER_RIGHT",
        EV_AIRPLANE_RUDDER_RIGHT,
        "Keyboard X",
        _L("rudder right")
    },
    {
        "AIRPLANE_STEER_LEFT",
        EV_AIRPLANE_STEER_LEFT,
        "Keyboard LEFT",
        _L("steer left")
    },
    {
        "AIRPLANE_STEER_RIGHT",
        EV_AIRPLANE_STEER_RIGHT,
        "Keyboard RIGHT",
        _L("steer right")
    },
    {
        "AIRPLANE_THROTTLE_AXIS",
        EV_AIRPLANE_THROTTLE_AXIS,
        "None",
        _L("throttle axis. Only use this if you have fitting hardware :) (i.e. a Slider)")
    },
    {
        "AIRPLANE_THROTTLE_DOWN",
        EV_AIRPLANE_THROTTLE_DOWN,
        "Keyboard EXPL+PGDOWN",
        _L("decreases the airplane thrust")
    },
    {
        "AIRPLANE_THROTTLE_FULL",
        EV_AIRPLANE_THROTTLE_FULL,
        "Keyboard CTRL+PGUP",
        _L("full thrust")
    },
    {
        "AIRPLANE_THROTTLE_NO",
        EV_AIRPLANE_THROTTLE_NO,
        "Keyboard CTRL+PGDOWN",
        _L("no thrust")
    },
    {
        "AIRPLANE_THROTTLE_UP",
        EV_AIRPLANE_THROTTLE_UP,
        "Keyboard EXPL+PGUP",
        _L("increase the airplane thrust")
    },
    {
        "AIRPLANE_TOGGLE_ENGINES",
        EV_AIRPLANE_TOGGLE_ENGINES,
        "Keyboard CTRL+HOME",
        _L("switch all engines on / off")
    },
    {
        "BOAT_CENTER_RUDDER",
        EV_BOAT_CENTER_RUDDER,
        "Keyboard PGDOWN",
        _L("center the rudder")
    },
    {
        "BOAT_REVERSE",
        EV_BOAT_REVERSE,
        "Keyboard PGUP",
        _L("no thrust")
    },
    {
        "BOAT_STEER_LEFT",
        EV_BOAT_STEER_LEFT,
        "Keyboard LEFT",
        _L("steer left a step")
    },
    {
        "BOAT_STEER_LEFT_AXIS",
        EV_BOAT_STEER_LEFT_AXIS,
        "None",
        _L("steer left (analog value!)")
    },
    {
        "BOAT_STEER_RIGHT",
        EV_BOAT_STEER_RIGHT,
        "Keyboard RIGHT",
        _L("steer right a step")
    },
    {
        "BOAT_STEER_RIGHT_AXIS",
        EV_BOAT_STEER_RIGHT_AXIS,
        "None",
        _L("steer right (analog value!)")
    },
    {
        "BOAT_THROTTLE_AXIS",
        EV_BOAT_THROTTLE_AXIS,
        "None",
        _L("throttle axis. Only use this if you have fitting hardware :) (i.e. a Slider)")
    },
    {
        "BOAT_THROTTLE_DOWN",
        EV_BOAT_THROTTLE_DOWN,
        "Keyboard DOWN",
        _L("decrease throttle")
    },
    {
        "BOAT_THROTTLE_UP",
        EV_BOAT_THROTTLE_UP,
        "Keyboard UP",
        _L("increase throttle")
    },
    {
        "SKY_DECREASE_TIME",
        EV_SKY_DECREASE_TIME,
        "Keyboard EXPL+SUBTRACT",
        _L("decrease day-time")
    },
    {
        "SKY_DECREASE_TIME_FAST",
        EV_SKY_DECREASE_TIME_FAST,
        "Keyboard SHIFT+SUBTRACT",
        _L("decrease day-time a lot faster")
    },
    {
        "SKY_INCREASE_TIME",
        EV_SKY_INCREASE_TIME,
        "Keyboard EXPL+ADD",
        _L("increase day-time")
    },
    {
        "SKY_INCREASE_TIME_FAST",
        EV_SKY_INCREASE_TIME_FAST,
        "Keyboard SHIFT+ADD",
        _L("increase day-time a lot faster")
    },
    {
        "CAMERA_CHANGE",
        EV_CAMERA_CHANGE,
        "Keyboard EXPL+C",
        _L("change camera mode")
    },
    {
        "CAMERA_LOOKBACK",
        EV_CAMERA_LOOKBACK,
        "Keyboard NUMPAD1",
        _L("look back (toggles between normal and lookback)")
    },
    {
        "CAMERA_RESET",
        EV_CAMERA_RESET,
        "Keyboard NUMPAD5",
        _L("reset the camera position")
    },
    {
        "CAMERA_ROTATE_DOWN",
        EV_CAMERA_ROTATE_DOWN,
        "Keyboard NUMPAD2",
        _L("rotate camera down")
    },
    {
        "CAMERA_ROTATE_LEFT",
        EV_CAMERA_ROTATE_LEFT,
        "Keyboard NUMPAD4",
        _L("rotate camera left")
    },
    {
        "CAMERA_ROTATE_RIGHT",
        EV_CAMERA_ROTATE_RIGHT,
        "Keyboard NUMPAD6",
        _L("rotate camera right")
    },
    {
        "CAMERA_ROTATE_UP",
        EV_CAMERA_ROTATE_UP,
        "Keyboard NUMPAD8",
        _L("rotate camera up")
    },
    {
        "CAMERA_SWIVEL_DOWN",
        EV_CAMERA_SWIVEL_DOWN,
        "Keyboard CTRL+NUMPAD2",
        _L("swivel camera down")
    },
    {
        "CAMERA_SWIVEL_LEFT",
        EV_CAMERA_SWIVEL_LEFT,
        "Keyboard CTRL+NUMPAD4",
        _L("swivel camera left")
    },
    {
        "CAMERA_SWIVEL_RIGHT",
        EV_CAMERA_SWIVEL_RIGHT,
        "Keyboard CTRL+NUMPAD6",
        _L("swivel camera right")
    },
    {
        "CAMERA_SWIVEL_UP",
        EV_CAMERA_SWIVEL_UP,
        "Keyboard CTRL+NUMPAD8",
        _L("swivel camera up")
    },
    {
        "CAMERA_ZOOM_IN",
        EV_CAMERA_ZOOM_IN,
        "Keyboard EXPL+NUMPAD9",
        _L("zoom camera in")
    },
    {
        "CAMERA_ZOOM_IN_FAST",
        EV_CAMERA_ZOOM_IN_FAST,
        "Keyboard SHIFT+NUMPAD9",
        _L("zoom camera in faster")
    },
    {
        "CAMERA_ZOOM_OUT",
        EV_CAMERA_ZOOM_OUT,
        "Keyboard EXPL+NUMPAD3",
        _L("zoom camera out")
    },
    {
        "CAMERA_ZOOM_OUT_FAST",
        EV_CAMERA_ZOOM_OUT_FAST,
        "Keyboard SHIFT+NUMPAD3",
        _L("zoom camera out faster")
    },
    {
        "CHARACTER_BACKWARDS",
        EV_CHARACTER_BACKWARDS,
        "Keyboard S",
        _L("step backwards with the character")
    },
    {
        "CHARACTER_FORWARD",
        EV_CHARACTER_FORWARD,
        "Keyboard W",
        _L("step forward with the character")
    },
    {
        "CHARACTER_JUMP",
        EV_CHARACTER_JUMP,
        "Keyboard SPACE",
        _L("let the character jump")
    },
    {
        "CHARACTER_LEFT",
        EV_CHARACTER_LEFT,
        "Keyboard LEFT",
        _L("rotate character left")
    },
    {
        "CHARACTER_RIGHT",
        EV_CHARACTER_RIGHT,
        "Keyboard RIGHT",
        _L("rotate character right")
    },
    {
        "CHARACTER_RUN",
        EV_CHARACTER_RUN,
        "Keyboard SHIFT+W",
        _L("let the character run")
    },
    {
        "CHARACTER_SIDESTEP_LEFT",
        EV_CHARACTER_SIDESTEP_LEFT,
        "Keyboard A",
        _L("sidestep to the left")
    },
    {
        "CHARACTER_SIDESTEP_RIGHT",
        EV_CHARACTER_SIDESTEP_RIGHT,
        "Keyboard D",
        _L("sidestep to the right")
    },
    {
        "COMMANDS_01",
        EV_COMMANDS_01,
        "Keyboard EXPL+F1",
        _L("Command 1")
    },
    {
        "COMMANDS_02",
        EV_COMMANDS_02,
        "Keyboard EXPL+F2",
        _L("Command 2")
    },
    {
        "COMMANDS_03",
        EV_COMMANDS_03,
        "Keyboard EXPL+F3",
        _L("Command 3")
    },
    {
        "COMMANDS_04",
        EV_COMMANDS_04,
        "Keyboard EXPL+F4",
        _L("Command 4")
    },
    {
        "COMMANDS_05",
        EV_COMMANDS_05,
        "Keyboard EXPL+F5",
        _L("Command 5")
    },
    {
        "COMMANDS_06",
        EV_COMMANDS_06,
        "Keyboard EXPL+F6",
        _L("Command 6")
    },
    {
        "COMMANDS_07",
        EV_COMMANDS_07,
        "Keyboard EXPL+F7",
        _L("Command 7")
    },
    {
        "COMMANDS_08",
        EV_COMMANDS_08,
        "Keyboard EXPL+F8",
        _L("Command 8")
    },
    {
        "COMMANDS_09",
        EV_COMMANDS_09,
        "Keyboard EXPL+F9",
        _L("Command 9")
    },
    {
        "COMMANDS_10",
        EV_COMMANDS_10,
        "Keyboard EXPL+F10",
        _L("Command 10")
    },
    {
        "COMMANDS_11",
        EV_COMMANDS_11,
        "Keyboard EXPL+F11",
        _L("Command 11")
    },
    {
        "COMMANDS_12",
        EV_COMMANDS_12,
        "Keyboard EXPL+F12",
        _L("Command 12")
    },
    {
        "COMMANDS_13",
        EV_COMMANDS_13,
        "Keyboard EXPL+CTRL+F1",
        _L("Command 13")
    },
    {
        "COMMANDS_14",
        EV_COMMANDS_14,
        "Keyboard EXPL+CTRL+F2",
        _L("Command 14")
    },
    {
        "COMMANDS_15",
        EV_COMMANDS_15,
        "Keyboard EXPL+CTRL+F3",
        _L("Command 15")
    },
    {
        "COMMANDS_16",
        EV_COMMANDS_16,
        "Keyboard EXPL+CTRL+F4",
        _L("Command 16")
    },
    {
        "COMMANDS_17",
        EV_COMMANDS_17,
        "Keyboard EXPL+CTRL+F5",
        _L("Command 17")
    },
    {
        "COMMANDS_18",
        EV_COMMANDS_18,
        "Keyboard EXPL+CTRL+F6",
        _L("Command 18")
    },
    {
        "COMMANDS_19",
        EV_COMMANDS_19,
        "Keyboard EXPL+CTRL+F7",
        _L("Command 19")
    },
    {
        "COMMANDS_20",
        EV_COMMANDS_20,
        "Keyboard EXPL+CTRL+F8",
        _L("Command 20")
    },
    {
        "COMMANDS_21",
        EV_COMMANDS_21,
        "Keyboard EXPL+CTRL+F9",
        _L("Command 21")
    },
    {
        "COMMANDS_22",
        EV_COMMANDS_22,
        "Keyboard EXPL+CTRL+F10",
        _L("Command 22")
    },
    {
        "COMMANDS_23",
        EV_COMMANDS_23,
        "Keyboard EXPL+CTRL+F11",
        _L("Command 23")
    },
    {
        "COMMANDS_24",
        EV_COMMANDS_24,
        "Keyboard EXPL+CTRL+F12",
        _L("Command 24")
    },
    {
        "COMMANDS_25",
        EV_COMMANDS_25,
        "Keyboard EXPL+SHIFT+F1",
        _L("Command 25")
    },
    {
        "COMMANDS_26",
        EV_COMMANDS_26,
        "Keyboard EXPL+SHIFT+F2",
        _L("Command 26")
    },
    {
        "COMMANDS_27",
        EV_COMMANDS_27,
        "Keyboard EXPL+SHIFT+F3",
        _L("Command 27")
    },
    {
        "COMMANDS_28",
        EV_COMMANDS_28,
        "Keyboard EXPL+SHIFT+F4",
        _L("Command 28")
    },
    {
        "COMMANDS_29",
        EV_COMMANDS_29,
        "Keyboard EXPL+SHIFT+F5",
        _L("Command 29")
    },
    {
        "COMMANDS_30",
        EV_COMMANDS_30,
        "Keyboard EXPL+SHIFT+F6",
        _L("Command 30")
    },
    {
        "COMMANDS_31",
        EV_COMMANDS_31,
        "Keyboard EXPL+SHIFT+F7",
        _L("Command 31")
    },
    {
        "COMMANDS_32",
        EV_COMMANDS_32,
        "Keyboard EXPL+SHIFT+F8",
        _L("Command 32")
    },
    {
        "COMMANDS_33",
        EV_COMMANDS_33,
        "Keyboard EXPL+SHIFT+F9",
        _L("Command 33")
    },
    {
        "COMMANDS_34",
        EV_COMMANDS_34,
        "Keyboard EXPL+SHIFT+F10",
        _L("Command 34")
    },
    {
        "COMMANDS_35",
        EV_COMMANDS_35,
        "Keyboard EXPL+SHIFT+F11",
        _L("Command 35")
    },
    {
        "COMMANDS_36",
        EV_COMMANDS_36,
        "Keyboard EXPL+SHIFT+F12",
        _L("Command 36")
    },
    {
        "COMMANDS_37",
        EV_COMMANDS_37,
        "Keyboard EXPL+ALT+F1",
        _L("Command 37")
    },
    {
        "COMMANDS_38",
        EV_COMMANDS_38,
        "Keyboard EXPL+ALT+F2",
        _L("Command 38")
    },
    {
        "COMMANDS_39",
        EV_COMMANDS_39,
        "Keyboard EXPL+ALT+F3",
        _L("Command 39")
    },
    {
        "COMMANDS_40",
        EV_COMMANDS_40,
        "Keyboard EXPL+ALT+F4",
        _L("Command 40")
    },
    {
        "COMMANDS_41",
        EV_COMMANDS_41,
        "Keyboard EXPL+ALT+F5",
        _L("Command 41")
    },
    {
        "COMMANDS_42",
        EV_COMMANDS_42,
        "Keyboard EXPL+ALT+F6",
        _L("Command 42")
    },
    {
        "COMMANDS_43",
        EV_COMMANDS_43,
        "Keyboard EXPL+ALT+F7",
        _L("Command 43")
    },
    {
        "COMMANDS_44",
        EV_COMMANDS_44,
        "Keyboard EXPL+ALT+F8",
        _L("Command 44")
    },
    {
        "COMMANDS_45",
        EV_COMMANDS_45,
        "Keyboard EXPL+ALT+F9",
        _L("Command 45")
    },
    {
        "COMMANDS_46",
        EV_COMMANDS_46,
        "Keyboard EXPL+ALT+F10",
        _L("Command 46")
    },
    {
        "COMMANDS_47",
        EV_COMMANDS_47,
        "Keyboard EXPL+ALT+F11",
        _L("Command 47")
    },
    {
        "COMMANDS_48",
        EV_COMMANDS_48,
        "Keyboard EXPL+ALT+F12",
        _L("Command 48")
    },
    {
        "COMMANDS_49",
        EV_COMMANDS_49,
        "Keyboard EXPL+CTRL+SHIFT+F1",
        _L("Command 49")
    },
    {
        "COMMANDS_50",
        EV_COMMANDS_50,
        "Keyboard EXPL+CTRL+SHIFT+F2",
        _L("Command 50")
    },
    {
        "COMMANDS_51",
        EV_COMMANDS_51,
        "Keyboard EXPL+CTRL+SHIFT+F3",
        _L("Command 51")
    },
    {
        "COMMANDS_52",
        EV_COMMANDS_52,
        "Keyboard EXPL+CTRL+SHIFT+F4",
        _L("Command 52")
    },
    {
        "COMMANDS_53",
        EV_COMMANDS_53,
        "Keyboard EXPL+CTRL+SHIFT+F5",
        _L("Command 53")
    },
    {
        "COMMANDS_54",
        EV_COMMANDS_54,
        "Keyboard EXPL+CTRL+SHIFT+F6",
        _L("Command 54")
    },
    {
        "COMMANDS_55",
        EV_COMMANDS_55,
        "Keyboard EXPL+CTRL+SHIFT+F7",
        _L("Command 55")
    },
    {
        "COMMANDS_56",
        EV_COMMANDS_56,
        "Keyboard EXPL+CTRL+SHIFT+F8",
        _L("Command 56")
    },
    {
        "COMMANDS_57",
        EV_COMMANDS_57,
        "Keyboard EXPL+CTRL+SHIFT+F9",
        _L("Command 57")
    },
    {
        "COMMANDS_58",
        EV_COMMANDS_58,
        "Keyboard EXPL+CTRL+SHIFT+F10",
        _L("Command 58")
    },
    {
        "COMMANDS_59",
        EV_COMMANDS_59,
        "Keyboard EXPL+CTRL+SHIFT+F11",
        _L("Command 59")
    },
    {
        "COMMANDS_60",
        EV_COMMANDS_60,
        "Keyboard EXPL+CTRL+SHIFT+F12",
        _L("Command 60")
    },
    {
        "COMMANDS_61",
        EV_COMMANDS_61,
        "Keyboard EXPL+CTRL+ALT+F1",
        _L("Command 61")
    },
    {
        "COMMANDS_62",
        EV_COMMANDS_62,
        "Keyboard EXPL+CTRL+ALT+F2",
        _L("Command 62")
    },
    {
        "COMMANDS_63",
        EV_COMMANDS_63,
        "Keyboard EXPL+CTRL+ALT+F3",
        _L("Command 63")
    },
    {
        "COMMANDS_64",
        EV_COMMANDS_64,
        "Keyboard EXPL+CTRL+ALT+F4",
        _L("Command 64")
    },
    {
        "COMMANDS_65",
        EV_COMMANDS_65,
        "Keyboard EXPL+CTRL+ALT+F5",
        _L("Command 65")
    },
    {
        "COMMANDS_66",
        EV_COMMANDS_66,
        "Keyboard EXPL+CTRL+ALT+F6",
        _L("Command 66")
    },
    {
        "COMMANDS_67",
        EV_COMMANDS_67,
        "Keyboard EXPL+CTRL+ALT+F7",
        _L("Command 67")
    },
    {
        "COMMANDS_68",
        EV_COMMANDS_68,
        "Keyboard EXPL+CTRL+ALT+F8",
        _L("Command 68")
    },
    {
        "COMMANDS_69",
        EV_COMMANDS_69,
        "Keyboard EXPL+CTRL+ALT+F9",
        _L("Command 69")
    },
    {
        "COMMANDS_70",
        EV_COMMANDS_70,
        "Keyboard EXPL+CTRL+ALT+F10",
        _L("Command 70")
    },
    {
        "COMMANDS_71",
        EV_COMMANDS_71,
        "Keyboard EXPL+CTRL+ALT+F11",
        _L("Command 71")
    },
    {
        "COMMANDS_72",
        EV_COMMANDS_72,
        "Keyboard EXPL+CTRL+ALT+F12",
        _L("Command 72")
    },
    {
        "COMMANDS_73",
        EV_COMMANDS_73,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F1",
        _L("Command 73")
    },
    {
        "COMMANDS_74",
        EV_COMMANDS_74,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F2",
        _L("Command 74")
    },
    {
        "COMMANDS_75",
        EV_COMMANDS_75,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F3",
        _L("Command 75")
    },
    {
        "COMMANDS_76",
        EV_COMMANDS_76,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F4",
        _L("Command 76")
    },
    {
        "COMMANDS_77",
        EV_COMMANDS_77,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F5",
        _L("Command 77")
    },
    {
        "COMMANDS_78",
        EV_COMMANDS_78,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F6",
        _L("Command 78")
    },
    {
        "COMMANDS_79",
        EV_COMMANDS_79,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F7",
        _L("Command 79")
    },
    {
        "COMMANDS_80",
        EV_COMMANDS_80,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F8",
        _L("Command 80")
    },
    {
        "COMMANDS_81",
        EV_COMMANDS_81,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F9",
        _L("Command 81")
    },
    {
        "COMMANDS_82",
        EV_COMMANDS_82,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F10",
        _L("Command 82")
    },
    {
        "COMMANDS_83",
        EV_COMMANDS_83,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F11",
        _L("Command 83")
    },
    {
        "COMMANDS_84",
        EV_COMMANDS_84,
        "Keyboard EXPL+CTRL+SHIFT+ALT+F12",
        _L("Command 84")
    },
    {
        "COMMON_ACCELERATE_SIMULATION",
        EV_COMMON_ACCELERATE_SIMULATION,
        "Keyboard CTRL+EQUALS",
        _L("accelerate the simulation")
    },
    {
        "COMMON_DECELERATE_SIMULATION",
        EV_COMMON_DECELERATE_SIMULATION,
        "Keyboard SHIFT+EQUALS",
        _L("decelerate the simulation")
    },
    {
        "COMMON_RESET_SIMULATION_PACE",
        EV_COMMON_RESET_SIMULATION_PACE,
        "Keyboard BACKSLASH",
        _L("reset the simulation pace")
    },
    {
        "COMMON_CONSOLE_TOGGLE",
        EV_COMMON_CONSOLE_TOGGLE,
        "Keyboard EXPL+GRAVE",
        _L("show / hide the console")
    },
    {
        "COMMON_ENTER_CHATMODE",
        EV_COMMON_ENTER_CHATMODE,
        "Keyboard Y",
        _L("enter the chat")
    },
    {
        "COMMON_SEND_CHAT",
        EV_COMMON_SEND_CHAT,
        "Keyboard RETURN",
        _L("sends the entered text")
    },
    {
        "COMMON_ENTER_OR_EXIT_TRUCK",
        EV_COMMON_ENTER_OR_EXIT_TRUCK,
        "Keyboard RETURN",
        _L("enter or exit a truck")
    },
    {
        "COMMON_ENTER_NEXT_TRUCK",
        EV_COMMON_ENTER_NEXT_TRUCK,
        "Keyboard EXPL+CTRL+RBRACKET",
        _L("enter next truck")
    },
    {
        "COMMON_ENTER_PREVIOUS_TRUCK",
        EV_COMMON_ENTER_PREVIOUS_TRUCK,
        "Keyboard EXPL+CTRL+LBRACKET",
        _L("enter previous truck")
    },
    {
        "COMMON_REMOVE_CURRENT_TRUCK",
        EV_COMMON_REMOVE_CURRENT_TRUCK,
        "Keyboard EXPL+CTRL+DELETE",
        _L("remove current truck")
    },
    {
        "COMMON_RESPAWN_LAST_TRUCK",
        EV_COMMON_RESPAWN_LAST_TRUCK,
        "Keyboard EXPL+CTRL+PERIOD",
        _L("respawn last truck")
    },
    {
        "COMMON_HIDE_GUI",
        EV_COMMON_HIDE_GUI,
        "Keyboard EXPL+U",
        _L("hide all GUI elements")
    },
    {
        "COMMON_LOCK",
        EV_COMMON_LOCK,
        "Keyboard EXPL+L",
        _L("connect hook node to a node in close proximity")
    },
    {
        "COMMON_AUTOLOCK",
        EV_COMMON_AUTOLOCK,
        "Keyboard EXPL+ALT+L",
        _L("unlock autolock hook node")
    },
    {
        "COMMON_ROPELOCK",
        EV_COMMON_ROPELOCK,
        "Keyboard EXPL+CTRL+L",
        _L("connect a rope to a node in close proximity")
    },
    {
        "COMMON_OUTPUT_POSITION",
        EV_COMMON_OUTPUT_POSITION,
        "Keyboard H",
        _L("write current position to log (you can open the logfile and reuse the position)")
    },
    {
        "COMMON_GET_NEW_VEHICLE",
        EV_COMMON_GET_NEW_VEHICLE,
        "Keyboard EXPL+CTRL+G",
        _L("get new vehicle")
    },
    {
        "COMMON_PRESSURE_LESS",
        EV_COMMON_PRESSURE_LESS,
        "Keyboard LBRACKET",
        _L("decrease tire pressure (note: only very few trucks support this)")
    },
    {
        "COMMON_PRESSURE_MORE",
        EV_COMMON_PRESSURE_MORE,
        "Keyboard RBRACKET",
        _L("increase tire pressure (note: only very few trucks support this)")
    },
    {
        "COMMON_QUIT_GAME",
        EV_COMMON_QUIT_GAME,
        "Keyboard EXPL+ESCAPE",
        _L("exit the game")
    },
    {
        "COMMON_REPAIR_TRUCK",
        EV_COMMON_REPAIR_TRUCK,
        "Keyboard BACK",
        _L("repair truck")
    },
    {
        "COMMON_RESCUE_TRUCK",
        EV_COMMON_RESCUE_TRUCK,
        "Keyboard EXPL+R",
        _L("teleport to rescue truck")
    },
    {
        "COMMON_RESET_TRUCK",
        EV_COMMON_RESET_TRUCK,
        "Keyboard I",
        _L("reset truck to original starting position")
    },
    {
        "COMMON_SCREENSHOT",
        EV_COMMON_SCREENSHOT,
        "Keyboard EXPL+SYSRQ",
        _L("take a screenshot")
    },
    {
        "COMMON_SCREENSHOT_BIG",
        EV_COMMON_SCREENSHOT_BIG,
        "Keyboard EXPL+CTRL+SYSRQ",
        _L("take a big screenshot (3 times the screen size)")
    },
    {
        "COMMON_SAVE_TERRAIN",
        EV_COMMON_SAVE_TERRAIN,
        "Keyboard EXPL+ALT+SHIF+CTRL+M",
        _L("save the currently loaded terrain to a mesh file")
    },
    {
        "COMMON_SECURE_LOAD",
        EV_COMMON_SECURE_LOAD,
        "Keyboard O",
        _L("tie a load to the truck")
    },
    {
        "COMMON_SHOW_SKELETON",
        EV_COMMON_SHOW_SKELETON,
        "Keyboard K",
        _L("toggle skeleton display mode")
    },
    {
        "COMMON_TOGGLE_TERRAIN_EDITOR",
        EV_COMMON_TOGGLE_TERRAIN_EDITOR,
        "Keyboard EXPL+SHIFT+Y",
        _L("toggle terrain editor")
    },
    {
        "COMMON_TOGGLE_CUSTOM_PARTICLES",
        EV_COMMON_TOGGLE_CUSTOM_PARTICLES,
        "Keyboard G",
        _L("toggle particle cannon")
    },
    {
        "COMMON_TOGGLE_MAT_DEBUG",
        EV_COMMON_TOGGLE_MAT_DEBUG,
        "",
        _L("debug purpose - dont use")
    },
    {
        "COMMON_TOGGLE_RENDER_MODE",
        EV_COMMON_TOGGLE_RENDER_MODE,
        "Keyboard E",
        _L("toggle render mode (solid, wireframe and points)")
    },
    {
        "COMMON_TOGGLE_REPLAY_MODE",
        EV_COMMON_TOGGLE_REPLAY_MODE,
        "Keyboard J",
        _L("enable or disable replay mode")
    },
    {
        "COMMON_TOGGLE_STATS",
        EV_COMMON_TOGGLE_STATS,
        "Keyboard EXPL+F",
        _L("toggle Ogre statistics (FPS etc.)")
    },
    {
        "COMMON_TOGGLE_TRUCK_BEACONS",
        EV_COMMON_TOGGLE_TRUCK_BEACONS,
        "Keyboard M",
        _L("toggle truck beacons")
    },
    {
        "COMMON_TOGGLE_TRUCK_LIGHTS",
        EV_COMMON_TOGGLE_TRUCK_LIGHTS,
        "Keyboard N",
        _L("toggle truck front lights")
    },
    {
        "COMMON_TRUCK_INFO",
        EV_COMMON_TRUCK_INFO,
        "Keyboard EXPL+T",
        _L("toggle truck HUD")
    },
    {
        "COMMON_TRUCK_DESCRIPTION",
        EV_COMMON_TRUCK_DESCRIPTION,
        "Keyboard EXPL+CTRL+T",
        _L("toggle truck description")
    },
    {
        "COMMON_FOV_LESS",
        EV_COMMON_FOV_LESS,
        "Keyboard EXPL+NUMPAD7",
        _L("decreases the current FOV value")
    },
    {
        "COMMON_FOV_MORE",
        EV_COMMON_FOV_MORE,
        "Keyboard EXPL+CTRL+NUMPAD7",
        _L("increase the current FOV value")
    },
    {
        "GRASS_LESS",
        EV_GRASS_LESS,
        "",
        _L("EXPERIMENTAL: remove some grass")
    },
    {
        "GRASS_MORE",
        EV_GRASS_MORE,
        "",
        _L("EXPERIMENTAL: add some grass")
    },
    {
        "GRASS_MOST",
        EV_GRASS_MOST,
        "",
        _L("EXPERIMENTAL: set maximum amount of grass")
    },
    {
        "GRASS_NONE",
        EV_GRASS_NONE,
        "",
        _L("EXPERIMENTAL: remove grass completely")
    },
    {
        "GRASS_SAVE",
        EV_GRASS_SAVE,
        "",
        _L("EXPERIMENTAL: save changes to the grass density image")
    },
    {
        "MENU_DOWN",
        EV_MENU_DOWN,
        "Keyboard DOWN",
        _L("select next element in current category")
    },
    {
        "MENU_LEFT",
        EV_MENU_LEFT,
        "Keyboard LEFT",
        _L("select previous category")
    },
    {
        "MENU_RIGHT",
        EV_MENU_RIGHT,
        "Keyboard RIGHT",
        _L("select next category")
    },
    {
        "MENU_SELECT",
        EV_MENU_SELECT,
        "Keyboard EXPL+RETURN",
        _L("select focussed item and close menu")
    },
    {
        "MENU_UP",
        EV_MENU_UP,
        "Keyboard UP",
        _L("select previous element in current category")
    },
    {
        "SURVEY_MAP_TOGGLE_ICONS",
        EV_SURVEY_MAP_TOGGLE_ICONS,
        "Keyboard EXPL+CTRL+SHIFT+ALT+TAB",
        _L("toggle map icons")
    },
    {
        "SURVEY_MAP_TOGGLE_VIEW",
        EV_SURVEY_MAP_TOGGLE_VIEW,
        "Keyboard EXPL+TAB",
        _L("toggle map modes")
    },
    {
        "SURVEY_MAP_TOGGLE_ALPHA",
        EV_SURVEY_MAP_ALPHA,
        "Keyboard EXPL+CTRL+SHIFT+TAB",
        _L("toggle translucency of overview-map")
    },
    {
        "SURVEY_MAP_ZOOM_IN",
        EV_SURVEY_MAP_ZOOM_IN,
        "Keyboard EXPL+CTRL+TAB",
        _L("zoom in")
    },
    {
        "SURVEY_MAP_ZOOM_OUT",
        EV_SURVEY_MAP_ZOOM_OUT,
        "Keyboard EXPL+SHIFT+TAB",
        _L("zoom out")
    },
    {
        "TRUCK_ACCELERATE",
        EV_TRUCK_ACCELERATE,
        "Keyboard UP",
        _L("accelerate the truck")
    },
    {
        "TRUCK_ACCELERATE_MODIFIER_25",
        EV_TRUCK_ACCELERATE_MODIFIER_25,
        "Keyboard ALT+UP",
        _L("accelerate with 25 percent pedal input")
    },
    {
        "TRUCK_ACCELERATE_MODIFIER_50",
        EV_TRUCK_ACCELERATE_MODIFIER_50,
        "Keyboard CTRL+UP",
        _L("accelerate with 50 percent pedal input")
    },
    {
        "TRUCK_AUTOSHIFT_DOWN",
        EV_TRUCK_AUTOSHIFT_DOWN,
        "Keyboard PGDOWN",
        _L("shift automatic transmission one gear down")
    },
    {
        "TRUCK_AUTOSHIFT_UP",
        EV_TRUCK_AUTOSHIFT_UP,
        "Keyboard PGUP",
        _L("shift automatic transmission one gear up")
    },
    {
        "TRUCK_BLINK_LEFT",
        EV_TRUCK_BLINK_LEFT,
        "Keyboard COMMA",
        _L("toggle left direction indicator (blinker)")
    },
    {
        "TRUCK_BLINK_RIGHT",
        EV_TRUCK_BLINK_RIGHT,
        "Keyboard PERIOD",
        _L("toggle right direction indicator (blinker)")
    },
    {
        "TRUCK_BLINK_WARN",
        EV_TRUCK_BLINK_WARN,
        "Keyboard MINUS",
        _L("toggle all direction indicators")
    },
    {
        "TRUCK_BRAKE",
        EV_TRUCK_BRAKE,
        "Keyboard DOWN",
        _L("brake")
    },
    {
        "TRUCK_BRAKE_MODIFIER_25",
        EV_TRUCK_BRAKE_MODIFIER_25,
        "Keyboard ALT+DOWN",
        _L("brake with 25 percent pedal input")
    },
    {
        "TRUCK_BRAKE_MODIFIER_50",
        EV_TRUCK_BRAKE_MODIFIER_50,
        "Keyboard CTRL+DOWN",
        _L("brake with 50 percent pedal input")
    },
    {
        "TRUCK_HORN",
        EV_TRUCK_HORN,
        "Keyboard H",
        _L("truck horn")
    },
    {
        "TRUCK_LIGHTTOGGLE1",
        EV_TRUCK_LIGHTTOGGLE01,
        "Keyboard EXPL+CTRL+1",
        _L("toggle custom light 1")
    },
    {
        "TRUCK_LIGHTTOGGLE2",
        EV_TRUCK_LIGHTTOGGLE02,
        "Keyboard EXPL+CTRL+2",
        _L("toggle custom light 2")
    },
    {
        "TRUCK_LIGHTTOGGLE3",
        EV_TRUCK_LIGHTTOGGLE03,
        "Keyboard EXPL+CTRL+3",
        _L("toggle custom light 3")
    },
    {
        "TRUCK_LIGHTTOGGLE4",
        EV_TRUCK_LIGHTTOGGLE04,
        "Keyboard EXPL+CTRL+4",
        _L("toggle custom light 4")
    },
    {
        "TRUCK_LIGHTTOGGLE5",
        EV_TRUCK_LIGHTTOGGLE05,
        "Keyboard EXPL+CTRL+5",
        _L("toggle custom light 5")
    },
    {
        "TRUCK_LIGHTTOGGLE6",
        EV_TRUCK_LIGHTTOGGLE06,
        "Keyboard EXPL+CTRL+6",
        _L("toggle custom light 6")
    },
    {
        "TRUCK_LIGHTTOGGLE7",
        EV_TRUCK_LIGHTTOGGLE07,
        "Keyboard EXPL+CTRL+7",
        _L("toggle custom light 7")
    },
    {
        "TRUCK_LIGHTTOGGLE8",
        EV_TRUCK_LIGHTTOGGLE08,
        "Keyboard EXPL+CTRL+8",
        _L("toggle custom light 8")
    },
    {
        "TRUCK_LIGHTTOGGLE9",
        EV_TRUCK_LIGHTTOGGLE09,
        "Keyboard EXPL+CTRL+9",
        _L("toggle custom light 9")
    },
    {
        "TRUCK_LIGHTTOGGLE10",
        EV_TRUCK_LIGHTTOGGLE10,
        "Keyboard EXPL+CTRL+0",
        _L("toggle custom light 10")
    },
    {
        "TRUCK_MANUAL_CLUTCH",
        EV_TRUCK_MANUAL_CLUTCH,
        "Keyboard LSHIFT",
        _L("manual clutch (for manual transmission)")
    },
    {
        "TRUCK_PARKING_BRAKE",
        EV_TRUCK_PARKING_BRAKE,
        "Keyboard P",
        _L("toggle parking brake")
    },
    {
        "TRUCK_ANTILOCK_BRAKE",
        EV_TRUCK_ANTILOCK_BRAKE,
        "Keyboard EXPL+SHIFT+B",
        _L("toggle antilock brake")
    },
    {
        "TRUCK_TOGGLE_VIDEOCAMERA",
        EV_TRUCK_TOGGLE_VIDEOCAMERA,
        "Keyboard EXPL+CTRL+V",
        _L("toggle videocamera")
    },
    {
        "TRUCK_TRACTION_CONTROL",
        EV_TRUCK_TRACTION_CONTROL,
        "Keyboard EXPL+SHIFT+T",
        _L("toggle traction control")
    },
    {
        "TRUCK_CRUISE_CONTROL",
        EV_TRUCK_CRUISE_CONTROL,
        "Keyboard EXPL+SPACE",
        _L("toggle cruise control")
    },
    {
        "TRUCK_CRUISE_CONTROL_READJUST",
        EV_TRUCK_CRUISE_CONTROL_READJUST,
        "Keyboard EXPL+CTRL+SPACE",
        _L("match target speed / rpm with current truck speed / rpm")
    },
    {
        "TRUCK_CRUISE_CONTROL_ACCL",
        EV_TRUCK_CRUISE_CONTROL_ACCL,
        "Keyboard EXPL+CTRL+R",
        _L("increase target speed / rpm")
    },
    {
        "TRUCK_CRUISE_CONTROL_DECL",
        EV_TRUCK_CRUISE_CONTROL_DECL,
        "Keyboard EXPL+CTRL+F",
        _L("decrease target speed / rpm")
    },
    {
        "TRUCK_SHIFT_DOWN",
        EV_TRUCK_SHIFT_DOWN,
        "Keyboard Z",
        _L("shift one gear down in manual transmission mode")
    },
    {
        "TRUCK_SHIFT_NEUTRAL",
        EV_TRUCK_SHIFT_NEUTRAL,
        "Keyboard D",
        _L("shift to neutral gear in manual transmission mode")
    },
    {
        "TRUCK_SHIFT_UP",
        EV_TRUCK_SHIFT_UP,
        "Keyboard A",
        _L("shift one gear up in manual transmission mode")
    },
    {
        "TRUCK_SHIFT_GEAR_REVERSE",
        EV_TRUCK_SHIFT_GEAR_REVERSE,
        "",
        _L("shift directly to reverse gear")
    },
    {
        "TRUCK_SHIFT_GEAR1",
        EV_TRUCK_SHIFT_GEAR01,
        "",
        _L("shift directly to first gear")
    },
    {
        "TRUCK_SHIFT_GEAR2",
        EV_TRUCK_SHIFT_GEAR02,
        "",
        _L("shift directly to second gear")
    },
    {
        "TRUCK_SHIFT_GEAR3",
        EV_TRUCK_SHIFT_GEAR03,
        "",
        _L("shift directly to third gear")
    },
    {
        "TRUCK_SHIFT_GEAR4",
        EV_TRUCK_SHIFT_GEAR04,
        "",
        _L("shift directly to fourth gear")
    },
    {
        "TRUCK_SHIFT_GEAR5",
        EV_TRUCK_SHIFT_GEAR05,
        "",
        _L("shift directly to 5th gear")
    },
    {
        "TRUCK_SHIFT_GEAR6",
        EV_TRUCK_SHIFT_GEAR06,
        "",
        _L("shift directly to 6th gear")
    },
    {
        "TRUCK_SHIFT_GEAR7",
        EV_TRUCK_SHIFT_GEAR07,
        "",
        _L("shift directly to 7th gear")
    },
    {
        "TRUCK_SHIFT_GEAR8",
        EV_TRUCK_SHIFT_GEAR08,
        "",
        _L("shift directly to 8th gear")
    },
    {
        "TRUCK_SHIFT_GEAR9",
        EV_TRUCK_SHIFT_GEAR09,
        "",
        _L("shift directly to 9th gear")
    },
    {
        "TRUCK_SHIFT_GEAR10",
        EV_TRUCK_SHIFT_GEAR10,
        "",
        _L("shift directly to 10th gear")
    },
    {
        "TRUCK_SHIFT_GEAR11",
        EV_TRUCK_SHIFT_GEAR11,
        "",
        _L("shift directly to 11th gear")
    },
    {
        "TRUCK_SHIFT_GEAR12",
        EV_TRUCK_SHIFT_GEAR12,
        "",
        _L("shift directly to 12th gear")
    },

    {
        "TRUCK_SHIFT_GEAR13",
        EV_TRUCK_SHIFT_GEAR13,
        "",
        _L("shift directly to 13th gear")
    },
    {
        "TRUCK_SHIFT_GEAR14",
        EV_TRUCK_SHIFT_GEAR14,
        "",
        _L("shift directly to 14th gear")
    },
    {
        "TRUCK_SHIFT_GEAR15",
        EV_TRUCK_SHIFT_GEAR15,
        "",
        _L("shift directly to 15th gear")
    },
    {
        "TRUCK_SHIFT_GEAR16",
        EV_TRUCK_SHIFT_GEAR16,
        "",
        _L("shift directly to 16th gear")
    },
    {
        "TRUCK_SHIFT_GEAR17",
        EV_TRUCK_SHIFT_GEAR17,
        "",
        _L("shift directly to 17th gear")
    },
    {
        "TRUCK_SHIFT_GEAR18",
        EV_TRUCK_SHIFT_GEAR18,
        "",
        _L("shift directly to 18th gear")
    },
    {
        "TRUCK_SHIFT_LOWRANGE",
        EV_TRUCK_SHIFT_LOWRANGE,
        "",
        _L("sets low range (1-6) for H-shaft")
    },
    {
        "TRUCK_SHIFT_MIDRANGE",
        EV_TRUCK_SHIFT_MIDRANGE,
        "",
        _L("sets middle range (7-12) for H-shaft")
    },
    {
        "TRUCK_SHIFT_HIGHRANGE",
        EV_TRUCK_SHIFT_HIGHRANGE,
        "",
        _L("sets high range (13-18) for H-shaft")
    },
    {
        "TRUCK_STARTER",
        EV_TRUCK_STARTER,
        "Keyboard S",
        _L("hold to start the engine")
    },
    {
        "TRUCK_STEER_LEFT",
        EV_TRUCK_STEER_LEFT,
        "Keyboard LEFT",
        _L("steer left")
    },
    {
        "TRUCK_STEER_RIGHT",
        EV_TRUCK_STEER_RIGHT,
        "Keyboard RIGHT",
        _L("steer right")
    },
    {
        "TRUCK_SWITCH_SHIFT_MODES",
        EV_TRUCK_SWITCH_SHIFT_MODES,
        "Keyboard Q",
        _L("toggle between transmission modes")
    },
    {
        "TRUCK_TOGGLE_AXLE_LOCK",
        EV_TRUCK_TOGGLE_AXLE_LOCK,
        "Keyboard W",
        _L("Cycle between available differental models")
    },
    {
        "TRUCK_TOGGLE_CONTACT",
        EV_TRUCK_TOGGLE_CONTACT,
        "Keyboard X",
        _L("toggle ignition")
    },
    {
        "TRUCK_TOGGLE_FORWARDCOMMANDS",
        EV_TRUCK_TOGGLE_FORWARDCOMMANDS,
        "Keyboard EXPL+CTRL+SHIFT+F",
        _L("toggle forwardcommands")
    },
    {
        "TRUCK_TOGGLE_IMPORTCOMMANDS",
        EV_TRUCK_TOGGLE_IMPORTCOMMANDS,
        "Keyboard EXPL+CTRL+SHIFT+I",
        _L("toggle importcommands")
    },

    // "new" commands
    {
        "COMMON_FULLSCREEN_TOGGLE",
        EV_COMMON_FULLSCREEN_TOGGLE,
        "Keyboard EXPL+ALT+RETURN",
        _L("toggle between windowed and fullscreen mode")
    },
    {
        "CAMERA_FREE_MODE_FIX",
        EV_CAMERA_FREE_MODE_FIX,
        "Keyboard EXPL+ALT+C",
        _L("fix the camera to a position")
    },
    {
        "CAMERA_FREE_MODE",
        EV_CAMERA_FREE_MODE,
        "Keyboard EXPL+SHIFT+C",
        _L("enable / disable free camera mode")
    },
    {
        "TRUCK_LEFT_MIRROR_LEFT",
        EV_TRUCK_LEFT_MIRROR_LEFT,
        "Keyboard EXPL+SEMICOLON",
        _L("move left mirror to the left")
    },
    {
        "TRUCK_LEFT_MIRROR_RIGHT",
        EV_TRUCK_LEFT_MIRROR_RIGHT,
        "Keyboard EXPL+CTRL+SEMICOLON",
        _L("move left mirror to the right")
    },
    {
        "TRUCK_RIGHT_MIRROR_LEFT",
        EV_TRUCK_RIGHT_MIRROR_LEFT,
        "Keyboard EXPL+COLON",
        _L("more right mirror to the left")
    },
    {
        "TRUCK_RIGHT_MIRROR_RIGHT",
        EV_TRUCK_RIGHT_MIRROR_RIGHT,
        "Keyboard EXPL+CTRL+COLON",
        _L("move right mirror to the right")
    },
    {
        "COMMON_REPLAY_FORWARD",
        EV_COMMON_REPLAY_FORWARD,
        "Keyboard EXPL+RIGHT",
        _L("more replay forward")
    },
    {
        "COMMON_REPLAY_BACKWARD",
        EV_COMMON_REPLAY_BACKWARD,
        "Keyboard EXPL+LEFT",
        _L("more replay backward")
    },
    {
        "COMMON_REPLAY_FAST_FORWARD",
        EV_COMMON_REPLAY_FAST_FORWARD,
        "Keyboard EXPL+SHIFT+RIGHT",
        _L("move replay fast forward")
    },
    {
        "COMMON_REPLAY_FAST_BACKWARD",
        EV_COMMON_REPLAY_FAST_BACKWARD,
        "Keyboard EXPL+SHIFT+LEFT",
        _L("move replay fast backward")
    },
    {
        "AIRPLANE_AIRBRAKES_NONE",
        EV_AIRPLANE_AIRBRAKES_NONE,
        "Keyboard CTRL+3",
        _L("no airbrakes")
    },
    {
        "AIRPLANE_AIRBRAKES_FULL",
        EV_AIRPLANE_AIRBRAKES_FULL,
        "Keyboard CTRL+4",
        _L("full airbrakes")
    },
    {
        "AIRPLANE_AIRBRAKES_LESS",
        EV_AIRPLANE_AIRBRAKES_LESS,
        "Keyboard EXPL+3",
        _L("less airbrakes")
    },
    {
        "AIRPLANE_AIRBRAKES_MORE",
        EV_AIRPLANE_AIRBRAKES_MORE,
        "Keyboard EXPL+4",
        _L("more airbrakes")
    },
    {
        "AIRPLANE_THROTTLE",
        EV_AIRPLANE_THROTTLE,
        "",
        _L("airplane throttle")
    },
    {
        "COMMON_TRUCK_REMOVE",
        EV_COMMON_TRUCK_REMOVE,
        "Keyboard EXPL+CTRL+SHIFT+DELETE",
        _L("delete current truck")
    },
    {
        "COMMON_NETCHATDISPLAY",
        EV_COMMON_NETCHATDISPLAY,
        "Keyboard EXPL+SHIFT+U",
        _L("display or hide net chat")
    },
    {
        "COMMON_NETCHATMODE",
        EV_COMMON_NETCHATMODE,
        "Keyboard EXPL+CTRL+U",
        _L("toggle between net chat display modes")
    },
    {
        "CHARACTER_ROT_UP",
        EV_CHARACTER_ROT_UP,
        "Keyboard UP",
        _L("rotate view up")
    },
    {
        "CHARACTER_ROT_DOWN",
        EV_CHARACTER_ROT_DOWN,
        "Keyboard DOWN",
        _L("rotate view down")
    },
    {
        "CAMERA_UP",
        EV_CAMERA_UP,
        "Keyboard Q",
        _L("move camera up")
    },
    {
        "CAMERA_DOWN",
        EV_CAMERA_DOWN,
        "Keyboard Z",
        _L("move camera down")
    },

    // now position storage
    {"TRUCK_SAVE_POS1", EV_TRUCK_SAVE_POS01, "Keyboard EXPL+ALT+CTRL+1", _L("save position as slot 1")},
    {"TRUCK_SAVE_POS2", EV_TRUCK_SAVE_POS02, "Keyboard EXPL+ALT+CTRL+2", _L("save position as slot 2")},
    {"TRUCK_SAVE_POS3", EV_TRUCK_SAVE_POS03, "Keyboard EXPL+ALT+CTRL+3", _L("save position as slot 3")},
    {"TRUCK_SAVE_POS4", EV_TRUCK_SAVE_POS04, "Keyboard EXPL+ALT+CTRL+4", _L("save position as slot 4")},
    {"TRUCK_SAVE_POS5", EV_TRUCK_SAVE_POS05, "Keyboard EXPL+ALT+CTRL+5", _L("save position as slot 5")},
    {"TRUCK_SAVE_POS6", EV_TRUCK_SAVE_POS06, "Keyboard EXPL+ALT+CTRL+6", _L("save position as slot 6")},
    {"TRUCK_SAVE_POS7", EV_TRUCK_SAVE_POS07, "Keyboard EXPL+ALT+CTRL+7", _L("save position as slot 7")},
    {"TRUCK_SAVE_POS8", EV_TRUCK_SAVE_POS08, "Keyboard EXPL+ALT+CTRL+8", _L("save position as slot 8")},
    {"TRUCK_SAVE_POS9", EV_TRUCK_SAVE_POS09, "Keyboard EXPL+ALT+CTRL+9", _L("save position as slot 9")},
    {"TRUCK_SAVE_POS10", EV_TRUCK_SAVE_POS10, "Keyboard EXPL+ALT+CTRL+0", _L("save position as slot 10")},

    {"TRUCK_LOAD_POS1", EV_TRUCK_LOAD_POS01, "Keyboard EXPL+ALT+1", _L("load position under slot 1")},
    {"TRUCK_LOAD_POS2", EV_TRUCK_LOAD_POS02, "Keyboard EXPL+ALT+2", _L("load position under slot 2")},
    {"TRUCK_LOAD_POS3", EV_TRUCK_LOAD_POS03, "Keyboard EXPL+ALT+3", _L("load position under slot 3")},
    {"TRUCK_LOAD_POS4", EV_TRUCK_LOAD_POS04, "Keyboard EXPL+ALT+4", _L("load position under slot 4")},
    {"TRUCK_LOAD_POS5", EV_TRUCK_LOAD_POS05, "Keyboard EXPL+ALT+5", _L("load position under slot 5")},
    {"TRUCK_LOAD_POS6", EV_TRUCK_LOAD_POS06, "Keyboard EXPL+ALT+6", _L("load position under slot 6")},
    {"TRUCK_LOAD_POS7", EV_TRUCK_LOAD_POS07, "Keyboard EXPL+ALT+7", _L("load position under slot 7")},
    {"TRUCK_LOAD_POS8", EV_TRUCK_LOAD_POS08, "Keyboard EXPL+ALT+8", _L("load position under slot 8")},
    {"TRUCK_LOAD_POS9", EV_TRUCK_LOAD_POS09, "Keyboard EXPL+ALT+9", _L("load position under slot 9")},
    {"TRUCK_LOAD_POS10", EV_TRUCK_LOAD_POS10, "Keyboard EXPL+ALT+0", _L("load position under slot 10")},

    {"DOF_TOGGLE", EV_DOF_TOGGLE, "Keyboard EXPL+CTRL+D", _L("turn on Depth of Field on or off")},
    {"DOF_DEBUG", EV_DOF_DEBUG, "Keyboard EXPL+ALT+D", _L("turn on the Depth of field debug view")},
    {"DOF_DEBUG_TOGGLE_FOCUS_MODE", EV_DOF_DEBUG_TOGGLE_FOCUS_MODE, "Keyboard EXPL+SPACE", _L("toggle the DOF focus mode")},
    {"DOF_DEBUG_ZOOM_IN", EV_DOF_DEBUG_ZOOM_IN, "Keyboard EXPL+Q", _L("zoom in")},
    {"DOF_DEBUG_ZOOM_OUT", EV_DOF_DEBUG_ZOOM_OUT, "Keyboard EXPL+Z", _L("zoom in")},
    {"DOF_DEBUG_APERTURE_MORE", EV_DOF_DEBUG_APERTURE_MORE, "Keyboard EXPL+1", _L("more aperture")},
    {"DOF_DEBUG_APERTURE_LESS", EV_DOF_DEBUG_APERTURE_LESS, "Keyboard EXPL+2", _L("less aperture")},
    {"DOF_DEBUG_FOCUS_IN", EV_DOF_DEBUG_FOCUS_IN, "Keyboard EXPL+3", _L("move focus in")},
    {"DOF_DEBUG_FOCUS_OUT", EV_DOF_DEBUG_FOCUS_OUT, "Keyboard EXPL+4", _L("move focus out")},

    {"TRUCKEDIT_RELOAD", EV_TRUCKEDIT_RELOAD, "Keyboard EXPL+SHIFT+CTRL+R", _L("reload truck")},

    {"TOGGLESHADERS", EV_TOGGLESHADERS, "Keyboard EXPL+SHIFT+CTRL+S", _L("toggle shader usage")},

    // the end, DO NOT MODIFY
    {"", -1, "", ""},
};

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OIS.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <X11/Xlib.h>
//#include <linux/LinuxMouse.h>
#endif

using namespace std;
using namespace Ogre;

// Constructor takes a RenderWindow because it uses that to determine input context
InputEngine::InputEngine() :
      m_num_joysticks(0)
    , m_inputs_changed(true)
    , m_forcefeedback(nullptr)
    , m_input_manager(nullptr)
    , m_keyboard(nullptr)
    , m_mouse(nullptr)
    , m_unique_counter(0)
{
    memset(m_joy, 0, sizeof(m_joy));
    LOG("*** Loading OIS ***");
    initAllKeys();
}

InputEngine::~InputEngine()
{
    LOG("[RoR|Input] *** Terminating destructor ***");
    destroy();
}

void InputEngine::destroy()
{
    if (m_input_manager)
    {
        LOG("[RoR|Input] *** Terminating OIS ***");
        if (m_mouse)
        {
            m_input_manager->destroyInputObject(m_mouse);
            m_mouse = 0;
        }
        if (m_keyboard)
        {
            m_input_manager->destroyInputObject(m_keyboard);
            m_keyboard = 0;
        }
        if (m_joy)
        {
            for (int i = 0; i < MAX_JOYSTICKS; i++)
            {
                if (!m_joy[i])
                    continue;
                m_input_manager->destroyInputObject(m_joy[i]);
                m_joy[i] = 0;
            }
        }

        OIS::InputManager::destroyInputSystem(m_input_manager);
        m_input_manager = 0;
    }
}

void InputEngine::SetupInputDevices()
{
    LOG("[RoR|Input] *** Initializing OIS ***");

    size_t hwnd_raw;
    RoR::App::GetOgreSubsystem()->GetRenderWindow()->getCustomAttribute("WINDOW", &hwnd_raw);
    std::string hwnd = TOSTRING(hwnd_raw);
    OIS::ParamList pl;

    pl.insert(OIS::ParamList::value_type("WINDOW", hwnd));
    if (RoR::App::GetIoInputGrabMode() != RoR::App::INPUT_GRAB_ALL)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        pl.insert(OIS::ParamList::value_type("x11_mouse_hide", "false"));
        pl.insert(OIS::ParamList::value_type("XAutoRepeatOn", "false"));
        pl.insert(OIS::ParamList::value_type("x11_mouse_grab", "false"));
        pl.insert(OIS::ParamList::value_type("x11_keyboard_grab", "false"));
#else
        pl.insert(OIS::ParamList::value_type("w32_mouse", "DISCL_FOREGROUND"));
        pl.insert(OIS::ParamList::value_type("w32_mouse", "DISCL_NONEXCLUSIVE"));
        pl.insert(OIS::ParamList::value_type("w32_keyboard", "DISCL_FOREGROUND"));
        pl.insert(OIS::ParamList::value_type("w32_keyboard", "DISCL_NONEXCLUSIVE"));
#endif // LINUX
    }

    LOG("[RoR|Input] *** OIS window handle: " + hwnd);

    m_input_manager = OIS::InputManager::createInputSystem(pl);

    //Print debugging information
    unsigned int v = m_input_manager->getVersionNumber();
    LOG("[RoR|Input] OIS Version: " + TOSTRING(v>>16) + String(".") + TOSTRING((v>>8) & 0x000000FF) + String(".") + TOSTRING(v & 0x000000FF));
    LOG("[RoR|Input] >  Release Name: "    + m_input_manager->getVersionName());
    LOG("[RoR|Input] >  Total Keyboards: " + TOSTRING(m_input_manager->getNumberOfDevices(OIS::OISKeyboard)));
    LOG("[RoR|Input] >  Total Mice: "      + TOSTRING(m_input_manager->getNumberOfDevices(OIS::OISMouse)));
    LOG("[RoR|Input] >  Total JoySticks: " + TOSTRING(m_input_manager->getNumberOfDevices(OIS::OISJoyStick)));

    LOG("[RoR|Input] Listing all devices...");
    OIS::DeviceList deviceList = m_input_manager->listFreeDevices();
    for (OIS::DeviceList::iterator i = deviceList.begin(); i != deviceList.end(); ++i)
    {
        LOG("[RoR|Input] >  Device: " + String(OIS_DEVICE_TYPE[i->first]) + String(" Vendor: ") + i->second);
    }

    //Create all devices (We only catch joystick exceptions here, as, most people have Key/Mouse)
    try
    {
        m_keyboard = static_cast<OIS::Keyboard*>(m_input_manager->createInputObject(OIS::OISKeyboard, true));
        m_keyboard->setTextTranslation(OIS::Keyboard::Unicode);
        m_keyboard->setEventCallback(this);
    }
    catch (...)
    {
        this->HandleException("creating keyboard");
    }

    //This demo uses at most 10 joysticks - use old way to create (i.e. disregard vendor)
    int numSticks = std::min(m_input_manager->getNumberOfDevices(OIS::OISJoyStick), 10);
    m_num_joysticks = 0;
    for (int i = 0; i < numSticks; ++i)
    {
        try
        {
            m_joy[i] = (OIS::JoyStick*)m_input_manager->createInputObject(OIS::OISJoyStick, true);
            m_joy[i]->setEventCallback(this);
            m_joy_state[i] = m_joy[i]->getJoyStickState();
            m_num_joysticks++;
            //create force feedback too
            //here, we take the first device we can get, but we could take a device index
            if (!m_forcefeedback)
            {
                m_forcefeedback = (OIS::ForceFeedback*)m_joy[i]->queryInterface(OIS::Interface::ForceFeedback);
            }

            LOG("[RoR|Input] Created device [" + TOSTRING(i + 1) + "] \"" + m_joy[i]->vendor() + "\"");
            LOG("[RoR|Input] >  Axes: "     + TOSTRING(m_joy[i]->getNumberOfComponents(OIS::OIS_Axis)));
            LOG("[RoR|Input] >  Sliders: "  + TOSTRING(m_joy[i]->getNumberOfComponents(OIS::OIS_Slider)));
            LOG("[RoR|Input] >  POV/HATs: " + TOSTRING(m_joy[i]->getNumberOfComponents(OIS::OIS_POV)));
            LOG("[RoR|Input] >  Buttons: "  + TOSTRING(m_joy[i]->getNumberOfComponents(OIS::OIS_Button)));
            LOG("[RoR|Input] >  Vector3: "  + TOSTRING(m_joy[i]->getNumberOfComponents(OIS::OIS_Vector3)));
        }
        catch (...)
        {
            this->HandleException("creating device ["+TOSTRING(i)+"]");
        }
    }

    try
    {
        m_mouse = static_cast<OIS::Mouse*>(m_input_manager->createInputObject(OIS::OISMouse, true));
        m_mouse->setEventCallback(this);
        m_mouse_state = m_mouse->getMouseState(); // init states (not required for keyboard)
    }
    catch (...)
    {
        this->HandleException("creating mouse");
    }

    // set the mouse to the middle of the screen, hackish!
#if _WIN32
    // under linux, this will not work and the cursor will never reach (0,0)
    if (m_mouse && RoR::App::GetOgreSubsystem()->GetRenderWindow())
    {
        OIS::MouseState& mutableMouseState = const_cast<OIS::MouseState &>(m_mouse->getMouseState());
        mutableMouseState.X.abs = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getWidth() * 0.5f;
        mutableMouseState.Y.abs = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getHeight() * 0.5f;
    }
#endif // _WIN32
}

void InputEngine::LoadInputMappings()
{
    // load default one
    this->loadMapping(CONFIGFILENAME);

    // then load device specific ones
    for (int i = 0; i < m_num_joysticks; ++i)
    {
        String deviceStr = m_joy[i]->vendor();

        // care about unsuitable chars
        String repl = "\\/ #@?!$%^&*()+=-><.:'|\";";
        for (unsigned int c = 0; c < repl.size(); c++)
        {
            deviceStr = StringUtil::replaceAll(deviceStr, repl.substr(c, 1), "_");
        }
        deviceStr += ".map";

        this->loadMapping(deviceStr, true, i);
    }
    this->completeMissingEvents();
}

void InputEngine::grabMouse(bool enable)
{
    static int lastmode = -1;
    if (!m_mouse)
        return;
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    if ((enable && lastmode == 0) || (!enable && lastmode == 1) || (lastmode == -1))
    {
        LOG("[RoR|Input] *** mouse grab: " + TOSTRING(enable));
        lastmode = enable?1:0;
    }
#endif
}

void InputEngine::hideMouse(bool visible)
{
    static int mode = -1;
    if (!m_mouse)
        return;
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    if ((visible && mode == 0) || (!visible && mode == 1) || mode == -1)
    {
        mode = visible?1:0;
    }
#endif
}

OIS::MouseState InputEngine::getMouseState()
{
    static float mX = 999999, mY = 999999;
    static int mode = -1;
    if (mode == -1)
    {
        // try to find the correct location!
        mX = FSETTING("MouseSensX", 1);
        mY = FSETTING("MouseSensY", 1);
        LOG("[RoR|Input] Mouse X sens: " + TOSTRING((Real)mX));
        LOG("[RoR|Input] Mouse Y sens: " + TOSTRING((Real)mY));
        mode = 1;
        if (mX == 0 || mY == 0)
            mode = 2;
    }

    OIS::MouseState m;
    if (m_mouse)
    {
        m = m_mouse->getMouseState();
        if (mode == 1)
        {
            m.X.rel = (int)((float)m.X.rel * mX);
            m.Y.rel = (int)((float)m.X.rel * mY);
        }
    }
    return m;
}

String InputEngine::getKeyNameForKeyCode(OIS::KeyCode keycode)
{
    if (keycode == OIS::KC_LSHIFT || keycode == OIS::KC_RSHIFT)
        return "SHIFT";
    if (keycode == OIS::KC_LCONTROL || keycode == OIS::KC_RCONTROL)
        return "CTRL";
    if (keycode == OIS::KC_LMENU || keycode == OIS::KC_RMENU)
        return "ALT";
    for (auto allit = m_all_keys.begin(); allit != m_all_keys.end(); allit++)
    {
        if (allit->second == keycode)
            return allit->first;
    }
    return "unknown";
}

void InputEngine::Capture()
{
    if (m_keyboard)
    {
        m_keyboard->capture();
    }

    if (m_mouse)
    {
        m_mouse->capture();
    }

    for (int i = 0; i < m_num_joysticks; i++)
    {
        if (m_joy[i])
        {
            m_joy[i]->capture();
        }
    }
}

void InputEngine::windowResized(Ogre::RenderWindow* rw)
{
    if (!m_mouse)
        return;
    //update mouse area
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);
    const OIS::MouseState& ms = m_mouse->getMouseState();
    ms.width = width;
    ms.height = height;
    RoR::App::GetGuiManager()->windowResized(rw);
}

void InputEngine::SetKeyboardListener(OIS::KeyListener* keyboard_listener)
{
    assert (m_keyboard != nullptr);
    m_keyboard->setEventCallback(keyboard_listener);
}

OIS::MouseState InputEngine::SetMouseListener(OIS::MouseListener* mouse_listener)
{
    assert (m_mouse != nullptr);
    m_mouse->setEventCallback(mouse_listener);
    return m_mouse->getMouseState();
}

void InputEngine::RestoreMouseListener()
{
    if (m_mouse)
    {
        m_mouse->setEventCallback(this);

        // init states (not required for keyboard)
        m_mouse_state = m_mouse->getMouseState();
    }
}

void InputEngine::RestoreKeyboardListener()
{
    SetKeyboardListener(this);
}

/* --- Joystik Events ------------------------------------------ */
bool InputEngine::buttonPressed(const OIS::JoyStickEvent& arg, int button)
{
    m_inputs_changed = true;
    int i = arg.device->getID();
    if (i < 0 || i >= MAX_JOYSTICKS)
        i = 0;
    m_joy_state[i] = arg.state;
    return true;
}

bool InputEngine::buttonReleased(const OIS::JoyStickEvent& arg, int button)
{
    m_inputs_changed = true;
    int i = arg.device->getID();
    if (i < 0 || i >= MAX_JOYSTICKS)
        i = 0;
    m_joy_state[i] = arg.state;
    return true;
}

bool InputEngine::axisMoved(const OIS::JoyStickEvent& arg, int axis)
{
    m_inputs_changed = true;
    int i = arg.device->getID();
    if (i < 0 || i >= MAX_JOYSTICKS)
        i = 0;
    m_joy_state[i] = arg.state;
    return true;
}

bool InputEngine::sliderMoved(const OIS::JoyStickEvent& arg, int)
{
    m_inputs_changed = true;
    int i = arg.device->getID();
    if (i < 0 || i >= MAX_JOYSTICKS)
        i = 0;
    m_joy_state[i] = arg.state;
    return true;
}

bool InputEngine::povMoved(const OIS::JoyStickEvent& arg, int)
{
    m_inputs_changed = true;
    int i = arg.device->getID();
    if (i < 0 || i >= MAX_JOYSTICKS)
        i = 0;
    m_joy_state[i] = arg.state;
    return true;
}

/* --- Key Events ------------------------------------------ */
bool InputEngine::keyPressed(const OIS::KeyEvent& arg)
{
    if (RoR::App::GetGuiManager()->keyPressed(arg))
        return true;

    if (m_key_state[arg.key] != 1)
        m_inputs_changed = true;
    m_key_state[arg.key] = 1;

    return true;
}

bool InputEngine::keyReleased(const OIS::KeyEvent& arg)
{
    if (RoR::App::GetGuiManager()->keyReleased(arg))
        return true;
    if (m_key_state[arg.key] != 0)
        m_inputs_changed = true;
    m_key_state[arg.key] = 0;
    return true;
}

/* --- Mouse Events ------------------------------------------ */
bool InputEngine::mouseMoved(const OIS::MouseEvent& arg)
{
    if (RoR::App::GetGuiManager()->mouseMoved(arg))
        return true;
    m_inputs_changed = true;
    m_mouse_state = arg.state;
    return true;
}

bool InputEngine::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    if (RoR::App::GetGuiManager()->mousePressed(arg, id))
        return true;
    m_inputs_changed = true;
    m_mouse_state = arg.state;
    return true;
}

bool InputEngine::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    if (RoR::App::GetGuiManager()->mouseReleased(arg, id))
        return true;
    m_inputs_changed = true;
    m_mouse_state = arg.state;
    return true;
}

/* --- Custom Methods ------------------------------------------ */
void InputEngine::prepareShutdown()
{
    LOG("[RoR|Input] *** Inputsystem prepare for shutdown ***");
    destroy();
}

void InputEngine::resetKeys()
{
    for (std::map<int, bool>::iterator iter = m_key_state.begin(); iter != m_key_state.end(); ++iter)
    {
        iter->second = false;
    }
}

bool InputEngine::getEventBoolValue(int eventID)
{
    return (getEventValue(eventID) > 0.5f);
}

bool InputEngine::getEventBoolValueBounce(int eventID, float time)
{
    if (m_event_times[eventID] > 0)
        return false;
    else
    {
        bool res = getEventBoolValue(eventID);
        if (res)
            m_event_times[eventID] = time;
        return res;
    }
}

float InputEngine::getEventBounceTime(int eventID)
{
    return m_event_times[eventID];
}

void InputEngine::updateKeyBounces(float dt)
{
    for (std::map<int, float>::iterator it = m_event_times.begin(); it != m_event_times.end(); it++)
    {
        if (it->second > 0)
            it->second -= dt;
    }
}

float InputEngine::deadZone(float axisValue, float dz)
{
    // no deadzone?
    if (dz < 0.0001f)
        return axisValue;

    // check for deadzone
    if (fabs(axisValue) < dz)
    // dead zone case
        return 0.0f;
    else
    // non-deadzone, remap the remaining part
        return (axisValue - dz) * (1.0f / (1.0f - dz));
}

float InputEngine::axisLinearity(float axisValue, float linearity)
{
    return (axisValue * linearity);
}

void InputEngine::smoothValue(float& ref, float value, float rate)
{
    if (value < -1)
        value = -1;
    if (value > 1)
        value = 1;
    // smooth!
    if (ref > value)
    {
        ref -= rate;
        if (ref < value)
            ref = value;
    }
    else if (ref < value)
        ref += rate;
}

String InputEngine::getEventCommand(int eventID)
{
    std::vector<event_trigger_t> t_vec = m_events[eventID];
    if (t_vec.size() > 0)
        return String(t_vec[0].configline);
    return "";
}

event_trigger_t* InputEngine::getEventBySUID(int suid)
{
    std::map<int, std::vector<event_trigger_t>>::iterator a;
    std::vector<event_trigger_t>::iterator b;
    for (a = m_events.begin(); a != m_events.end(); a++)
    {
        for (b = a->second.begin(); b != a->second.end(); b++)
        {
            if (b->suid == suid)
                return &(*b);
        }
    }
    return 0;
}

bool InputEngine::deleteEventBySUID(int suid)
{
    std::map<int, std::vector<event_trigger_t>>::iterator a;
    std::vector<event_trigger_t>::iterator b;
    for (a = m_events.begin(); a != m_events.end(); a++)
    {
        for (b = a->second.begin(); b != a->second.end(); b++)
        {
            if (b->suid == suid)
            {
                a->second.erase(b);
                return true;
            }
        }
    }
    return false;
}

bool InputEngine::isEventDefined(int eventID)
{
    std::vector<event_trigger_t> t_vec = m_events[eventID];
    if (t_vec.size() > 0)
    {
        if (t_vec[0].eventtype != ET_NONE)
            return true;
    }
    return false;
}

int InputEngine::getKeboardKeyForCommand(int eventID)
{
    std::vector<event_trigger_t> t_vec = m_events[eventID];
    for (std::vector<event_trigger_t>::iterator i = t_vec.begin(); i != t_vec.end(); i++)
    {
        event_trigger_t t = *i;
        if (t.eventtype == ET_Keyboard)
            return t.keyCode;
    }
    return -1;
}

bool InputEngine::isEventAnalog(int eventID)
{
    std::vector<event_trigger_t> t_vec = m_events[eventID];
    if (t_vec.size() > 0)
    {
        //loop through all eventtypes, because we want to find a analog device wether it is the first device or not
        //this means a analog device is always preferred over a digital one
        for (unsigned int i = 0; i < t_vec.size(); i++)
        {
            if ((t_vec[i].eventtype == ET_MouseAxisX
                    || t_vec[i].eventtype == ET_MouseAxisY
                    || t_vec[i].eventtype == ET_MouseAxisZ
                    || t_vec[i].eventtype == ET_JoystickAxisAbs
                    || t_vec[i].eventtype == ET_JoystickAxisRel
                    || t_vec[i].eventtype == ET_JoystickSliderX
                    || t_vec[i].eventtype == ET_JoystickSliderY)
                //check if value comes from analog device
                //this way, only valid m_events (e.g. joystick mapped, but unplugged) are recognized as analog m_events
                && getEventValue(eventID, true, 2) != 0.0)
            {
                return true;
            }
        }
    }
    return false;
}

float InputEngine::getEventValue(int eventID, bool pure, int valueSource)
{
    float returnValue = 0;
    std::vector<event_trigger_t> t_vec = m_events[eventID];
    float value = 0;
    for (std::vector<event_trigger_t>::iterator i = t_vec.begin(); i != t_vec.end(); i++)
    {
        event_trigger_t t = *i;

        if (valueSource == 0 || valueSource == 1)
        {
            switch (t.eventtype)
            {
            case ET_NONE:
                break;
            case ET_Keyboard:
                if (!m_key_state[t.keyCode])
                    break;

                // only use explicite mapping, if two keys with different modifiers exist, i.e. F1 and SHIFT+F1.
                // check for modificators
                if (t.explicite)
                {
                    if (t.ctrl != (m_key_state[OIS::KC_LCONTROL] || m_key_state[OIS::KC_RCONTROL]))
                        break;
                    if (t.shift != (m_key_state[OIS::KC_LSHIFT] || m_key_state[OIS::KC_RSHIFT]))
                        break;
                    if (t.alt != (m_key_state[OIS::KC_LMENU] || m_key_state[OIS::KC_RMENU]))
                        break;
                }
                else
                {
                    if (t.ctrl && !(m_key_state[OIS::KC_LCONTROL] || m_key_state[OIS::KC_RCONTROL]))
                        break;
                    if (t.shift && !(m_key_state[OIS::KC_LSHIFT] || m_key_state[OIS::KC_RSHIFT]))
                        break;
                    if (t.alt && !(m_key_state[OIS::KC_LMENU] || m_key_state[OIS::KC_RMENU]))
                        break;
                }
                value = 1;
                break;
            case ET_MouseButton:
                value = m_mouse_state.buttonDown(OIS::MB_Left);
                break;
            case ET_JoystickButton:
                {
                    if (t.joystickNumber > m_num_joysticks || !m_joy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.joystickButtonNumber >= (int)m_joy[t.joystickNumber]->getNumberOfComponents(OIS::OIS_Button))
                    {
                        LOG("*** Joystick has not enough buttons for mapping: need button "+TOSTRING(t.joystickButtonNumber) 
                            + ", availabe buttons: "+TOSTRING(m_joy[t.joystickNumber]->getNumberOfComponents(OIS::OIS_Button)));
                        value = 0;
                        continue;
                    }
                    value = m_joy_state[t.joystickNumber].mButtons[t.joystickButtonNumber];
                }
                break;
            case ET_JoystickPov:
                {
                    if (t.joystickNumber > m_num_joysticks || !m_joy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.joystickPovNumber >= (int)m_joy[t.joystickNumber]->getNumberOfComponents(OIS::OIS_POV))
                    {
                        LOG("*** Joystick has not enough POVs for mapping: need POV "+TOSTRING(t.joystickPovNumber) 
                            + ", availabe POVs: "+TOSTRING(m_joy[t.joystickNumber]->getNumberOfComponents(OIS::OIS_POV)));
                        value = 0;
                        continue;
                    }
                    if (m_joy_state[t.joystickNumber].mPOV[t.joystickPovNumber].direction & t.joystickPovDirection)
                        value = 1;
                    else
                        value = 0;
                }
                break;
            }
        }
        if (valueSource == 0 || valueSource == 2)
        {
            switch (t.eventtype)
            {
            case ET_MouseAxisX:
                value = m_mouse_state.X.abs / 32767;
                break;
            case ET_MouseAxisY:
                value = m_mouse_state.Y.abs / 32767;
                break;
            case ET_MouseAxisZ:
                value = m_mouse_state.Z.abs / 32767;
                break;

            case ET_JoystickAxisRel:
            case ET_JoystickAxisAbs:
                {
                    if (t.joystickNumber > m_num_joysticks || !m_joy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.joystickAxisNumber >= (int)m_joy_state[t.joystickNumber].mAxes.size())
                    {
                        LOG("*** Joystick has not enough axis for mapping: need axe "
                            +TOSTRING(t.joystickAxisNumber) + ", availabe axis: "+TOSTRING(m_joy_state[t.joystickNumber].mAxes.size()));
                        value = 0;
                        continue;
                    }
                    OIS::Axis axe = m_joy_state[t.joystickNumber].mAxes[t.joystickAxisNumber];

                    if (t.eventtype == ET_JoystickAxisRel)
                    {
                        value = (float)axe.rel / (float)m_joy[t.joystickNumber]->MAX_AXIS;
                    }
                    else
                    {
                        value = (float)axe.abs / (float)m_joy[t.joystickNumber]->MAX_AXIS;
                        switch (t.joystickAxisRegion)
                        {
                        case 0:
                            // normal case, full axis used
                            value = (value + 1) / 2;
                            break;
                        case -1:
                            // lower range used
                            if (value > 0)
                                value = 0;
                            else
                                value = -value;
                            break;
                        case 1:
                            // upper range used
                            if (value < 0)
                                value = 0;
                            break;
                        }

                        if (t.joystickAxisHalf)
                        {
                            //no dead zone in half axis
                            value = (1.0 + value) / 2.0;
                            if (t.joystickAxisReverse)
                                value = 1.0 - value;
                            if (!pure)
                                value = axisLinearity(value, t.joystickAxisLinearity);
                        }
                        else
                        {
                            if (t.joystickAxisReverse)
                                value = 1 - value;
                            if (!pure)
                            // no deadzone when using oure value
                                value = deadZone(value, t.joystickAxisDeadzone);
                            if (!pure)
                                value = axisLinearity(value, t.joystickAxisLinearity);
                        }
                        // digital mapping of analog axis
                        if (t.joystickAxisUseDigital)
                            if (value >= 0.5)
                                value = 1;
                            else
                                value = 0;
                    }
                }
                break;
            case ET_JoystickSliderX:
            case ET_JoystickSliderY:
                {
                    if (t.joystickNumber > m_num_joysticks || !m_joy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.eventtype == ET_JoystickSliderX)
                        value = (float)m_joy_state[t.joystickNumber].mSliders[t.joystickSliderNumber].abX / (float)m_joy[t.joystickNumber]->MAX_AXIS;
                    else if (t.eventtype == ET_JoystickSliderY)
                        value = (float)m_joy_state[t.joystickNumber].mSliders[t.joystickSliderNumber].abY / (float)m_joy[t.joystickNumber]->MAX_AXIS;
                    value = (value + 1) / 2; // full axis
                    if (t.joystickSliderReverse)
                        value = 1.0 - value; // reversed
                }
                break;
            }
        }
        // only return if grater zero, otherwise check all other bombinations
        if (value > returnValue)
            returnValue = value;
    }
    return returnValue;
}

bool InputEngine::isKeyDown(OIS::KeyCode key)
{
    if (!m_keyboard)
        return false;
    return this->m_keyboard->isKeyDown(key);
}

bool InputEngine::isKeyDownValueBounce(OIS::KeyCode mod, float time)
{
    if (m_event_times[-mod] > 0)
        return false;
    else
    {
        bool res = isKeyDown(mod);
        if (res)
            m_event_times[-mod] = time;
        return res;
    }
}

String InputEngine::getDeviceName(event_trigger_t evt)
{
    switch (evt.eventtype)
    {
    case ET_NONE:
        return "None";
    case ET_Keyboard:
        return "Keyboard";
    case ET_MouseButton:
    case ET_MouseAxisX:
    case ET_MouseAxisY:
    case ET_MouseAxisZ:
        return "Mouse";
    case ET_JoystickButton:
    case ET_JoystickAxisAbs:
    case ET_JoystickAxisRel:
    case ET_JoystickPov:
    case ET_JoystickSliderX:
    case ET_JoystickSliderY:
        return "Joystick: " + getJoyVendor(evt.joystickNumber);
    }
    return "unknown";
}

String InputEngine::getEventTypeName(int type)
{
    switch (type)
    {
    case ET_NONE: return "None";
    case ET_Keyboard: return "Keyboard";
    case ET_MouseButton: return "MouseButton";
    case ET_MouseAxisX: return "MouseAxisX";
    case ET_MouseAxisY: return "MouseAxisY";
    case ET_MouseAxisZ: return "MouseAxisZ";
    case ET_JoystickButton: return "JoystickButton";
    case ET_JoystickAxisAbs: return "JoystickAxis";
    case ET_JoystickAxisRel: return "JoystickAxis";
    case ET_JoystickPov: return "JoystickPov";
    case ET_JoystickSliderX: return "JoystickSliderX";
    case ET_JoystickSliderY: return "JoystickSliderY";
    }
    return "unknown";
}

void InputEngine::addEvent(int eventID, event_trigger_t t)
{
    m_unique_counter++;
    t.suid = m_unique_counter;

    if (eventID == -1)
    //unknown event, discard
        return;
    if (m_events.find(eventID) == m_events.end())
    {
        m_events[eventID] = std::vector<event_trigger_t>();
        m_events[eventID].clear();
    }
    m_events[eventID].push_back(t);
}

void InputEngine::updateEvent(int eventID, event_trigger_t t)
{
    if (eventID == -1)
    //unknown event, discard
        return;
    if (m_events.find(eventID) == m_events.end())
    {
        m_events[eventID] = std::vector<event_trigger_t>();
        m_events[eventID].clear();
    }
    m_events[eventID].push_back(t);
}

bool InputEngine::processLine(char* line, int deviceID)
{
    static String cur_comment = "";

    char eventName[256] = "", evtype[256] = "";
    const char delimiters[] = "+";
    size_t linelen = strnlen(line, 1024);
    enum eventtypes eventtype = ET_NONE;

    int joyNo = 0;
    float defaultDeadzone = 0.1f;
    float defaultLinearity = 1.0f;
    if (line[0] == ';' || linelen < 5)
    {
        cur_comment += line;;
        return false;
    }
    sscanf(line, "%s %s", eventName, evtype);
    if (strnlen(eventName, 255) == 0 || strnlen(evtype, 255) == 0)
        return false;

    if (!strncmp(evtype, "Keyboard", 8))
        eventtype = ET_Keyboard;
    else if (!strncmp(evtype, "MouseButton", 10))
        eventtype = ET_MouseButton;
    else if (!strncmp(evtype, "MouseAxisX", 9))
        eventtype = ET_MouseAxisX;
    else if (!strncmp(evtype, "MouseAxisY", 9))
        eventtype = ET_MouseAxisY;
    else if (!strncmp(evtype, "MouseAxisZ", 9))
        eventtype = ET_MouseAxisZ;
    else if (!strncmp(evtype, "JoystickButton", 14))
        eventtype = ET_JoystickButton;
    else if (!strncmp(evtype, "JoystickAxis", 12))
        eventtype = ET_JoystickAxisAbs;
    //else if (!strncmp(evtype, "JoystickAxis", 250)) eventtype = ET_JoystickAxisRel;
    else if (!strncmp(evtype, "JoystickPov", 11))
        eventtype = ET_JoystickPov;
    else if (!strncmp(evtype, "JoystickSlider", 14))
        eventtype = ET_JoystickSliderX;
    else if (!strncmp(evtype, "JoystickSliderX", 15))
        eventtype = ET_JoystickSliderX;
    else if (!strncmp(evtype, "JoystickSliderY", 15))
        eventtype = ET_JoystickSliderY;
    else if (!strncmp(evtype, "None", 4))
        eventtype = ET_NONE;

    switch (eventtype)
    {
    case ET_Keyboard:
        {
            bool alt = false;
            bool shift = false;
            bool ctrl = false;
            bool expl = false;

            char* keycode = 0;
            char keycodes[256] = {};
            char keycodes_work[256] = {};

            OIS::KeyCode key = OIS::KC_UNASSIGNED;

            sscanf(line, "%s %s %s", eventName, evtype, keycodes);
            // separate all keys and construct the key combination
            strncpy(keycodes_work, keycodes, 255);
            keycodes_work[255] = '\0';
            char* token = strtok(keycodes_work, delimiters);

            while (token != NULL)
            {
                if (strncmp(token, "SHIFT", 5) == 0)
                    shift = true;
                else if (strncmp(token, "CTRL", 4) == 0)
                    ctrl = true;
                else if (strncmp(token, "ALT", 3) == 0)
                    alt = true;
                else if (strncmp(token, "EXPL", 4) == 0)
                    expl = true;
                keycode = token;
                token = strtok(NULL, delimiters);
            }

            auto allit = m_all_keys.find(keycode);
            if (allit == m_all_keys.end())
            {
                LOG("[RoR|Input] unknown key: " + string(keycodes));
                key = OIS::KC_UNASSIGNED;
            }
            else
            {
                //LOG("found key: " + string(keycode) + " = " + TOSTRING((int)key));
                key = allit->second;
            }
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
            {
                LOG("[RoR|Input] Error while processing input config: Unknown Event: "+String(eventName));
                return false;
            }
            event_trigger_t t_key = newEvent();
            //memset(&t_key, 0, sizeof(event_trigger_t));
            t_key.eventtype = ET_Keyboard;
            t_key.shift = shift;
            t_key.ctrl = ctrl;
            t_key.alt = alt;
            t_key.keyCode = key;
            t_key.explicite = expl;

            strncpy(t_key.configline, keycodes, 128);
            strncpy(t_key.group, getEventGroup(eventName).c_str(), 128);
            strncpy(t_key.tmp_eventname, eventName, 128);

            strncpy(t_key.comments, cur_comment.c_str(), 1024);
            addEvent(eventID, t_key);

            return true;
        }
    case ET_JoystickButton:
        {
            int buttonNo = 0;
            char tmp2[256] = {};
            sscanf(line, "%s %s %d %d %s", eventName, evtype, &joyNo, &buttonNo, tmp2);
            event_trigger_t t_joy = newEvent();
            //memset(&t_joy, 0, sizeof(event_trigger_t));
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;
            t_joy.eventtype = ET_JoystickButton;
            t_joy.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            t_joy.joystickButtonNumber = buttonNo;
            if (!strcmp(tmp2, "!NEW!"))
            {
                strncpy(t_joy.configline, tmp2, 128);
            }
            else
            {
                char tmp[256] = {};
                sprintf(tmp, "%d", buttonNo);
                strncpy(t_joy.configline, tmp, 128);
            }
            strncpy(t_joy.group, getEventGroup(eventName).c_str(), 128);
            strncpy(t_joy.tmp_eventname, eventName, 128);
            strncpy(t_joy.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_joy);
            return true;
        }
    case ET_JoystickAxisRel:
    case ET_JoystickAxisAbs:
        {
            int axisNo = 0;
            char options[256] = {};
            sscanf(line, "%s %s %d %d %s", eventName, evtype, &joyNo, &axisNo, options);
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;

            bool half = false;
            bool reverse = false;
            bool linear = false;
            bool relative = false;
            bool usedigital = false;
            float deadzone = defaultDeadzone;
            float linearity = defaultLinearity;
            int jAxisRegion = 0;
            //  0 = all
            // -1 = lower
            //  1 = upper
            char tmp[256] = {};
            strcpy(tmp, options);
            tmp[255] = '\0';
            char* token = strtok(tmp, delimiters);
            while (token != NULL)
            {
                if (strncmp(token, "HALF", 4) == 0)
                    half = true;
                else if (strncmp(token, "REVERSE", 7) == 0)
                    reverse = true;
                else if (strncmp(token, "LINEAR", 6) == 0)
                    linear = true;
                else if (strncmp(token, "UPPER", 5) == 0)
                    jAxisRegion = 1;
                else if (strncmp(token, "LOWER", 5) == 0)
                    jAxisRegion = -1;
                else if (strncmp(token, "RELATIVE", 8) == 0)
                    relative = true;
                else if (strncmp(token, "DIGITAL", 7) == 0)
                    usedigital = true;
                else if (strncmp(token, "DEADZONE", 8) == 0 && strnlen(token, 250) > 9)
                {
                    char tmp2[256] = {};
                    strcpy(tmp2, token + 9);
                    deadzone = atof(tmp2);
                }
                else if (strncmp(token, "LINEARITY", 9) == 0 && strnlen(token, 250) > 10)
                {
                    char tmp2[256] = {};
                    strcpy(tmp2, token + 10);
                    linearity = atof(tmp2);
                }
                token = strtok(NULL, delimiters);
            }

            if (relative)
                eventtype = ET_JoystickAxisRel;

            event_trigger_t t_joy = newEvent();
            t_joy.eventtype = eventtype;
            t_joy.joystickAxisRegion = jAxisRegion;
            t_joy.joystickAxisUseDigital = usedigital;
            t_joy.joystickAxisDeadzone = deadzone;
            t_joy.joystickAxisHalf = half;
            t_joy.joystickAxisLinearity = linearity;
            t_joy.joystickAxisReverse = reverse;
            t_joy.joystickAxisNumber = axisNo;
            t_joy.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            strncpy(t_joy.configline, options, 128);
            strncpy(t_joy.group, getEventGroup(eventName).c_str(), 128);
            strncpy(t_joy.tmp_eventname, eventName, 128);
            strncpy(t_joy.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_joy);
            return true;
        }
    case ET_NONE:
        {
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;
            event_trigger_t t_none = newEvent();
            t_none.eventtype = eventtype;
            strncpy(t_none.group, getEventGroup(eventName).c_str(), 128);
            strncpy(t_none.tmp_eventname, eventName, 128);
            strncpy(t_none.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_none);
            return true;
        }
    case ET_MouseButton:
    case ET_MouseAxisX:
    case ET_MouseAxisY:
    case ET_MouseAxisZ:
        // no mouse support D:
        return false;
    case ET_JoystickPov:
        {
            int povNumber = 0;
            char dir[256] = {};
            sscanf(line, "%s %s %d %d %s", eventName, evtype, &joyNo, &povNumber, dir);
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;

            int direction = OIS::Pov::Centered;
            if (!strcmp(dir, "North"))
                direction = OIS::Pov::North;
            if (!strcmp(dir, "South"))
                direction = OIS::Pov::South;
            if (!strcmp(dir, "East"))
                direction = OIS::Pov::East;
            if (!strcmp(dir, "West"))
                direction = OIS::Pov::West;
            if (!strcmp(dir, "NorthEast"))
                direction = OIS::Pov::NorthEast;
            if (!strcmp(dir, "SouthEast"))
                direction = OIS::Pov::SouthEast;
            if (!strcmp(dir, "NorthWest"))
                direction = OIS::Pov::NorthWest;
            if (!strcmp(dir, "SouthWest"))
                direction = OIS::Pov::SouthWest;

            event_trigger_t t_pov = newEvent();
            t_pov.eventtype = eventtype;
            t_pov.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            t_pov.joystickPovNumber = povNumber;
            t_pov.joystickPovDirection = direction;

            strncpy(t_pov.group, getEventGroup(eventName).c_str(), 128);
            strncpy(t_pov.tmp_eventname, eventName, 128);
            strncpy(t_pov.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_pov);
            return true;
        }
    case ET_JoystickSliderX:
    case ET_JoystickSliderY:
        {
            int sliderNumber = 0;
            char options[256] = {};
            char type;
            sscanf(line, "%s %s %d %c %d %s", eventName, evtype, &joyNo, &type, &sliderNumber, options);
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;

            bool reverse = false;
            char tmp[256] = {};
            strncpy(tmp, options, 255);
            tmp[255] = '\0';
            char* token = strtok(tmp, delimiters);
            while (token != NULL)
            {
                if (strncmp(token, "REVERSE", 7) == 0)
                    reverse = true;

                token = strtok(NULL, delimiters);
            }

            event_trigger_t t_slider = newEvent();

            if (type == 'Y' || type == 'y')
                eventtype = ET_JoystickSliderY;
            else if (type == 'X' || type == 'x')
                eventtype = ET_JoystickSliderX;

            t_slider.eventtype = eventtype;
            t_slider.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            t_slider.joystickSliderNumber = sliderNumber;
            t_slider.joystickSliderReverse = reverse;
            // TODO: add region support to sliders!
            t_slider.joystickSliderRegion = 0;
            strncpy(t_slider.configline, options, 128);
            strncpy(t_slider.group, getEventGroup(eventName).c_str(), 128);
            strncpy(t_slider.tmp_eventname, eventName, 128);
            strncpy(t_slider.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_slider);
            return true;
        }
    default:
        return false;
    }
}

int InputEngine::getCurrentJoyButton(int& joystickNumber, int& button)
{
    for (int j = 0; j < m_num_joysticks; j++)
    {
        for (int i = 0; i < (int)m_joy_state[j].mButtons.size(); i++)
        {
            if (m_joy_state[j].mButtons[i])
            {
                joystickNumber = j;
                button = i;
                return 1;
            }
        }
    }
    return 0;
}

int InputEngine::getCurrentPovValue(int& joystickNumber, int& pov, int& povdir)
{
    for (int j = 0; j < m_num_joysticks; j++)
    {
        for (int i = 0; i < MAX_JOYSTICK_POVS; i++)
        {
            if (m_joy_state[j].mPOV[i].direction != OIS::Pov::Centered)
            {
                joystickNumber = j;
                pov = i;
                povdir = m_joy_state[j].mPOV[i].direction;
                return 1;
            }
        }
    }
    return 0;
}

event_trigger_t InputEngine::newEvent()
{
    event_trigger_t res;
    memset(&res, 0, sizeof(event_trigger_t));
    return res;
}

int InputEngine::getJoyComponentCount(OIS::ComponentType type, int joystickNumber)
{
    if (joystickNumber > m_num_joysticks || !m_joy[joystickNumber])
        return 0;
    return m_joy[joystickNumber]->getNumberOfComponents(type);
}

std::string InputEngine::getJoyVendor(int joystickNumber)
{
    if (joystickNumber > m_num_joysticks || !m_joy[joystickNumber])
        return "unknown";
    return m_joy[joystickNumber]->vendor();
}

OIS::JoyStickState* InputEngine::getCurrentJoyState(int joystickNumber)
{
    if (joystickNumber > m_num_joysticks)
        return 0;
    return &m_joy_state[joystickNumber];
}

int InputEngine::getCurrentKeyCombo(String* combo)
{
    std::map<int, bool>::iterator i;
    int keyCounter = 0;
    int modCounter = 0;

    // list all modificators first
    for (i = m_key_state.begin(); i != m_key_state.end(); i++)
    {
        if (i->second)
        {
            if (i->first != OIS::KC_LSHIFT && i->first != OIS::KC_RSHIFT && i->first != OIS::KC_LCONTROL && i->first != OIS::KC_RCONTROL && i->first != OIS::KC_LMENU && i->first != OIS::KC_RMENU)
                continue;
            modCounter++;
            String keyName = getKeyNameForKeyCode((OIS::KeyCode)i->first);
            if (*combo == "")
                *combo = keyName;
            else
                *combo = *combo + "+" + keyName;
        }
    }

    // now list all keys
    for (i = m_key_state.begin(); i != m_key_state.end(); i++)
    {
        if (i->second)
        {
            if (i->first == OIS::KC_LSHIFT || i->first == OIS::KC_RSHIFT || i->first == OIS::KC_LCONTROL || i->first == OIS::KC_RCONTROL || i->first == OIS::KC_LMENU || i->first == OIS::KC_RMENU)
                continue;
            String keyName = getKeyNameForKeyCode((OIS::KeyCode)i->first);
            if (*combo == "")
                *combo = keyName;
            else
                *combo = *combo + "+" + keyName;
            keyCounter++;
        }
    }

    //
    if (modCounter > 0 && keyCounter == 0)
    {
        return -modCounter;
    }
    else if (keyCounter == 0 && modCounter == 0)
    {
        *combo = "(Please press a key)";
        return 0;
    }
    return keyCounter;
}

String InputEngine::getEventGroup(String eventName)
{
    const char delimiters[] = "_";
    char tmp[256] = {};
    strncpy(tmp, eventName.c_str(), 255);
    tmp[255] = '\0';
    char* token = strtok(tmp, delimiters);
    while (token != NULL)
    {
        return String(token);
    }
    return "";
}

bool InputEngine::appendLineToConfig(std::string line, std::string outfile)
{
    FILE* f = fopen(const_cast<char *>(outfile.c_str()), "a");
    if (!f)
        return false;
    fprintf(f, "%s\n", line.c_str());
    fclose(f);
    return true;
}

bool InputEngine::reloadConfig(std::string outfile)
{
    m_events.clear();
    loadMapping(outfile);
    return true;
}

bool InputEngine::updateConfigline(event_trigger_t* t)
{
    if (t->eventtype != ET_JoystickAxisAbs && t->eventtype != ET_JoystickSliderX && t->eventtype != ET_JoystickSliderY)
        return false;
    bool isSlider = (t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY);
    strcpy(t->configline, "");

    if (isSlider)
    {
        if (t->joystickSliderReverse && !strlen(t->configline))
            strcat(t->configline, "REVERSE");
        else if (t->joystickSliderReverse && strlen(t->configline))
            strcat(t->configline, "+REVERSE");

        if (t->joystickSliderRegion == 1 && !strlen(t->configline))
            strcat(t->configline, "UPPER");
        else if (t->joystickSliderRegion == 1 && strlen(t->configline))
            strcat(t->configline, "+UPPER");
        else if (t->joystickSliderRegion == -1 && !strlen(t->configline))
            strcat(t->configline, "LOWER");
        else if (t->joystickSliderRegion == -1 && strlen(t->configline))
            strcat(t->configline, "+LOWER");

        // is this is a slider, ignore the rest
        return true;
    }

    if (t->joystickAxisReverse && !strlen(t->configline))
        strcat(t->configline, "REVERSE");
    else if (t->joystickAxisReverse && strlen(t->configline))
        strcat(t->configline, "+REVERSE");

    if (t->joystickAxisRegion == 1 && !strlen(t->configline))
        strcat(t->configline, "UPPER");
    else if (t->joystickAxisRegion == 1 && strlen(t->configline))
        strcat(t->configline, "+UPPER");
    else if (t->joystickAxisRegion == -1 && !strlen(t->configline))
        strcat(t->configline, "LOWER");
    else if (t->joystickAxisRegion == -1 && strlen(t->configline))
        strcat(t->configline, "+LOWER");

    if (fabs(t->joystickAxisDeadzone - 0.1) > 0.0001f)
    {
        char tmp[256] = {};
        sprintf(tmp, "DEADZONE=%0.2f", t->joystickAxisDeadzone);
        if (strlen(t->configline))
        {
            strcat(t->configline, "+");
            strcat(t->configline, tmp);
        }
        else
            strcat(t->configline, tmp);
    }
    if (fabs(1.0f - t->joystickAxisLinearity) > 0.01f)
    {
        char tmp[256] = {};
        sprintf(tmp, "LINEARITY=%0.2f", t->joystickAxisLinearity);
        if (strlen(t->configline))
        {
            strcat(t->configline, "+");
            strcat(t->configline, tmp);
        }
        else
            strcat(t->configline, tmp);
    }
    return true;
}

bool InputEngine::saveMapping(String outfile, String hwnd, int joyNum)
{
    // -10 = all
    // -2  = keyboard
    // -3  = mouse
    // >0 joystick
    FILE* f = fopen(const_cast<char *>(outfile.c_str()), "w");
    if (!f)
        return false;

    bool created = false;

    int counter = 0;
    char curGroup[128] = "";
    std::map<int, std::vector<event_trigger_t>> controls = getEvents();
    std::map<int, std::vector<event_trigger_t>>::iterator mapIt;
    std::vector<event_trigger_t>::iterator vecIt;
    for (mapIt = controls.begin(); mapIt != controls.end(); mapIt++)
    {
        std::vector<event_trigger_t> vec = mapIt->second;

        for (vecIt = vec.begin(); vecIt != vec.end(); vecIt++ , counter++)
        {
            // filters
            if (vecIt->eventtype == ET_Keyboard && joyNum != -10 && joyNum != -2)
                continue;
            if ((vecIt->eventtype == ET_MouseAxisX || vecIt->eventtype == ET_MouseAxisY || vecIt->eventtype == ET_MouseAxisZ) && joyNum != -10 && joyNum != -3)
                continue;
            if ((vecIt->eventtype == ET_JoystickAxisAbs || vecIt->eventtype == ET_JoystickAxisRel || vecIt->eventtype == ET_JoystickButton || vecIt->eventtype == ET_JoystickPov || vecIt->eventtype == ET_JoystickSliderX || vecIt->eventtype == ET_JoystickSliderY) && joyNum >= 0 && vecIt->joystickNumber != joyNum)
                continue;

            if (strcmp(vecIt->group, curGroup))
            {
                strncpy(curGroup, vecIt->group, 128);
                // group title:
                fprintf(f, "\n; %s\n", curGroup);
            }

            // no user comments for now!
            //if (vecIt->comments!="")
            //	fprintf(f, "%s", vecIt->comments.c_str());

            // print event name
            fprintf(f, "%-30s ", eventIDToName(mapIt->first).c_str());
            // print event type
            fprintf(f, "%-20s ", getEventTypeName(vecIt->eventtype).c_str());

            if (vecIt->eventtype == ET_Keyboard)
            {
                fprintf(f, "%s ", vecIt->configline);
            }
            else if (vecIt->eventtype == ET_JoystickAxisAbs || vecIt->eventtype == ET_JoystickAxisRel)
            {
                fprintf(f, "%d ", vecIt->joystickNumber);
                fprintf(f, "%d ", vecIt->joystickAxisNumber);
                fprintf(f, "%s ", vecIt->configline);
            }
            else if (vecIt->eventtype == ET_JoystickSliderX || vecIt->eventtype == ET_JoystickSliderY)
            {
                fprintf(f, "%d ", vecIt->joystickNumber);
                char type = 'X';
                if (vecIt->eventtype == ET_JoystickSliderY)
                    type = 'Y';
                fprintf(f, "%c ", type);
                fprintf(f, "%d ", vecIt->joystickSliderNumber);
                fprintf(f, "%s ", vecIt->configline);
            }
            else if (vecIt->eventtype == ET_JoystickButton)
            {
                fprintf(f, "%d ", vecIt->joystickNumber);
                fprintf(f, "%d ", vecIt->joystickButtonNumber);
            }
            else if (vecIt->eventtype == ET_JoystickPov)
            {
                const char* dirStr = "North";
                if (vecIt->joystickPovDirection == OIS::Pov::North)
                    dirStr = "North";
                if (vecIt->joystickPovDirection == OIS::Pov::South)
                    dirStr = "South";
                if (vecIt->joystickPovDirection == OIS::Pov::East)
                    dirStr = "East";
                if (vecIt->joystickPovDirection == OIS::Pov::West)
                    dirStr = "West";
                if (vecIt->joystickPovDirection == OIS::Pov::NorthEast)
                    dirStr = "NorthEast";
                if (vecIt->joystickPovDirection == OIS::Pov::SouthEast)
                    dirStr = "SouthEast";
                if (vecIt->joystickPovDirection == OIS::Pov::NorthWest)
                    dirStr = "NorthWest";
                if (vecIt->joystickPovDirection == OIS::Pov::SouthWest)
                    dirStr = "SouthWest";

                fprintf(f, "%d %d %s", vecIt->joystickNumber, vecIt->joystickPovNumber, dirStr);
            }
            // end this line
            fprintf(f, "\n");
        }
    }
    fclose(f);
    return true;
}

void InputEngine::completeMissingEvents()
{
    for (int i = 0; i < EV_MODE_LAST; i++)
    {
        if (m_events.find(eventInfo[i].eventID) == m_events.end())
        {
            if (eventInfo[i].defaultKey.empty())
                continue;
            if (eventInfo[i].defaultKey == "None")
                continue;

            // not existing, insert default
            char tmp[256] = "";
            sprintf(tmp, "%s %s", eventInfo[i].name.c_str(), eventInfo[i].defaultKey.c_str());
            processLine(tmp);
        }
    }
}

bool InputEngine::loadMapping(std::string filename, bool append, int deviceID)
{
    char line[1025] = "";
    int oldState = m_unique_counter;

    if (!append)
    {
        // clear everything
        resetKeys();
        m_events.clear();
    }

    LOG("[RoR|Input]  * Loading input mapping: " + filename);
    {
        DataStreamPtr ds;
        try
        {
            ds = ResourceGroupManager::getSingleton().openResource(filename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        }
        catch (...)
        {
            return false;
        }
        while (!ds->eof())
        {
            size_t size = 1024;
            if (ds->tell() + size >= ds->size())
                size = ds->size() - ds->tell();
            if (ds->tell() >= ds->size())
                break;
            size_t readnum = ds->readLine(line, size);
            if (readnum > 5)
                processLine(line, deviceID);
        }
    }

    int newEvents = m_unique_counter - oldState;
    LOG("[RoR|Input]  * Input map successfully loaded: " + TOSTRING(newEvents) + " entries");
    return true;
}

int InputEngine::resolveEventName(String eventName)
{
    int i = 0;
    while (i != EV_MODE_LAST)
    {
        if (eventInfo[i].name == eventName)
            return eventInfo[i].eventID;
        i++;
    }
    LOG("[RoR|Input] unknown event (ignored): " + eventName);
    return -1;
}

String InputEngine::eventIDToName(int eventID)
{
    int i = 0;
    while (i != EV_MODE_LAST)
    {
        if (eventInfo[i].eventID == eventID)
            return eventInfo[i].name;
        i++;
    }
    return "unknown";
}

void InputEngine::initAllKeys()
{
    m_all_keys["0"]            = OIS::KC_0;
    m_all_keys["1"]            = OIS::KC_1;
    m_all_keys["2"]            = OIS::KC_2;
    m_all_keys["3"]            = OIS::KC_3;
    m_all_keys["4"]            = OIS::KC_4;
    m_all_keys["5"]            = OIS::KC_5;
    m_all_keys["6"]            = OIS::KC_6;
    m_all_keys["7"]            = OIS::KC_7;
    m_all_keys["8"]            = OIS::KC_8;
    m_all_keys["9"]            = OIS::KC_9;
    m_all_keys["A"]            = OIS::KC_A;
    m_all_keys["ABNT_C1"]      = OIS::KC_ABNT_C1;
    m_all_keys["ABNT_C2"]      = OIS::KC_ABNT_C2;
    m_all_keys["ADD"]          = OIS::KC_ADD;
    m_all_keys["APOSTROPHE"]   = OIS::KC_APOSTROPHE;
    m_all_keys["APPS"]         = OIS::KC_APPS;
    m_all_keys["AT"]           = OIS::KC_AT;
    m_all_keys["AX"]           = OIS::KC_AX;
    m_all_keys["B"]            = OIS::KC_B;
    m_all_keys["BACK"]         = OIS::KC_BACK;
    m_all_keys["BACKSLASH"]    = OIS::KC_BACKSLASH;
    m_all_keys["C"]            = OIS::KC_C;
    m_all_keys["CALCULATOR"]   = OIS::KC_CALCULATOR;
    m_all_keys["CAPITAL"]      = OIS::KC_CAPITAL;
    m_all_keys["COLON"]        = OIS::KC_COLON;
    m_all_keys["COMMA"]        = OIS::KC_COMMA;
    m_all_keys["CONVERT"]      = OIS::KC_CONVERT;
    m_all_keys["D"]            = OIS::KC_D;
    m_all_keys["DECIMAL"]      = OIS::KC_DECIMAL;
    m_all_keys["DELETE"]       = OIS::KC_DELETE;
    m_all_keys["DIVIDE"]       = OIS::KC_DIVIDE;
    m_all_keys["DOWN"]         = OIS::KC_DOWN;
    m_all_keys["E"]            = OIS::KC_E;
    m_all_keys["END"]          = OIS::KC_END;
    m_all_keys["EQUALS"]       = OIS::KC_EQUALS;
    m_all_keys["ESCAPE"]       = OIS::KC_ESCAPE;
    m_all_keys["F"]            = OIS::KC_F;
    m_all_keys["F1"]           = OIS::KC_F1;
    m_all_keys["F10"]          = OIS::KC_F10;
    m_all_keys["F11"]          = OIS::KC_F11;
    m_all_keys["F12"]          = OIS::KC_F12;
    m_all_keys["F13"]          = OIS::KC_F13;
    m_all_keys["F14"]          = OIS::KC_F14;
    m_all_keys["F15"]          = OIS::KC_F15;
    m_all_keys["F2"]           = OIS::KC_F2;
    m_all_keys["F3"]           = OIS::KC_F3;
    m_all_keys["F4"]           = OIS::KC_F4;
    m_all_keys["F5"]           = OIS::KC_F5;
    m_all_keys["F6"]           = OIS::KC_F6;
    m_all_keys["F7"]           = OIS::KC_F7;
    m_all_keys["F8"]           = OIS::KC_F8;
    m_all_keys["F9"]           = OIS::KC_F9;
    m_all_keys["G"]            = OIS::KC_G;
    m_all_keys["GRAVE"]        = OIS::KC_GRAVE;
    m_all_keys["H"]            = OIS::KC_H;
    m_all_keys["HOME"]         = OIS::KC_HOME;
    m_all_keys["I"]            = OIS::KC_I;
    m_all_keys["INSERT"]       = OIS::KC_INSERT;
    m_all_keys["J"]            = OIS::KC_J;
    m_all_keys["K"]            = OIS::KC_K;
    m_all_keys["KANA"]         = OIS::KC_KANA;
    m_all_keys["KANJI"]        = OIS::KC_KANJI;
    m_all_keys["L"]            = OIS::KC_L;
    m_all_keys["LBRACKET"]     = OIS::KC_LBRACKET;
    m_all_keys["LCONTROL"]     = OIS::KC_LCONTROL;
    m_all_keys["LEFT"]         = OIS::KC_LEFT;
    m_all_keys["LMENU"]        = OIS::KC_LMENU;
    m_all_keys["LSHIFT"]       = OIS::KC_LSHIFT;
    m_all_keys["LWIN"]         = OIS::KC_LWIN;
    m_all_keys["M"]            = OIS::KC_M;
    m_all_keys["MAIL"]         = OIS::KC_MAIL;
    m_all_keys["MEDIASELECT"]  = OIS::KC_MEDIASELECT;
    m_all_keys["MEDIASTOP"]    = OIS::KC_MEDIASTOP;
    m_all_keys["MINUS"]        = OIS::KC_MINUS;
    m_all_keys["MULTIPLY"]     = OIS::KC_MULTIPLY;
    m_all_keys["MUTE"]         = OIS::KC_MUTE;
    m_all_keys["MYCOMPUTER"]   = OIS::KC_MYCOMPUTER;
    m_all_keys["N"]            = OIS::KC_N;
    m_all_keys["NEXTTRACK"]    = OIS::KC_NEXTTRACK;
    m_all_keys["NOCONVERT"]    = OIS::KC_NOCONVERT;
    m_all_keys["NUMLOCK"]      = OIS::KC_NUMLOCK;
    m_all_keys["NUMPAD0"]      = OIS::KC_NUMPAD0;
    m_all_keys["NUMPAD1"]      = OIS::KC_NUMPAD1;
    m_all_keys["NUMPAD2"]      = OIS::KC_NUMPAD2;
    m_all_keys["NUMPAD3"]      = OIS::KC_NUMPAD3;
    m_all_keys["NUMPAD4"]      = OIS::KC_NUMPAD4;
    m_all_keys["NUMPAD5"]      = OIS::KC_NUMPAD5;
    m_all_keys["NUMPAD6"]      = OIS::KC_NUMPAD6;
    m_all_keys["NUMPAD7"]      = OIS::KC_NUMPAD7;
    m_all_keys["NUMPAD8"]      = OIS::KC_NUMPAD8;
    m_all_keys["NUMPAD9"]      = OIS::KC_NUMPAD9;
    m_all_keys["NUMPADCOMMA"]  = OIS::KC_NUMPADCOMMA;
    m_all_keys["NUMPADENTER"]  = OIS::KC_NUMPADENTER;
    m_all_keys["NUMPADEQUALS"] = OIS::KC_NUMPADEQUALS;
    m_all_keys["O"]            = OIS::KC_O;
    m_all_keys["OEM_102"]      = OIS::KC_OEM_102;
    m_all_keys["P"]            = OIS::KC_P;
    m_all_keys["PAUSE"]        = OIS::KC_PAUSE;
    m_all_keys["PERIOD"]       = OIS::KC_PERIOD;
    m_all_keys["PGDOWN"]       = OIS::KC_PGDOWN;
    m_all_keys["PGUP"]         = OIS::KC_PGUP;
    m_all_keys["PLAYPAUSE"]    = OIS::KC_PLAYPAUSE;
    m_all_keys["POWER"]        = OIS::KC_POWER;
    m_all_keys["PREVTRACK"]    = OIS::KC_PREVTRACK;
    m_all_keys["Q"]            = OIS::KC_Q;
    m_all_keys["R"]            = OIS::KC_R;
    m_all_keys["RBRACKET"]     = OIS::KC_RBRACKET;
    m_all_keys["RCONTROL"]     = OIS::KC_RCONTROL;
    m_all_keys["RETURN"]       = OIS::KC_RETURN;
    m_all_keys["RIGHT"]        = OIS::KC_RIGHT;
    m_all_keys["RMENU"]        = OIS::KC_RMENU;
    m_all_keys["RSHIFT"]       = OIS::KC_RSHIFT;
    m_all_keys["RWIN"]         = OIS::KC_RWIN;
    m_all_keys["S"]            = OIS::KC_S;
    m_all_keys["SCROLL"]       = OIS::KC_SCROLL;
    m_all_keys["SEMICOLON"]    = OIS::KC_SEMICOLON;
    m_all_keys["SLASH"]        = OIS::KC_SLASH;
    m_all_keys["SLEEP"]        = OIS::KC_SLEEP;
    m_all_keys["SPACE"]        = OIS::KC_SPACE;
    m_all_keys["STOP"]         = OIS::KC_STOP;
    m_all_keys["SUBTRACT"]     = OIS::KC_SUBTRACT;
    m_all_keys["SYSRQ"]        = OIS::KC_SYSRQ;
    m_all_keys["T"]            = OIS::KC_T;
    m_all_keys["TAB"]          = OIS::KC_TAB;
    m_all_keys["U"]            = OIS::KC_U;
    m_all_keys["UNDERLINE"]    = OIS::KC_UNDERLINE;
    m_all_keys["UNLABELED"]    = OIS::KC_UNLABELED;
    m_all_keys["UP"]           = OIS::KC_UP;
    m_all_keys["V"]            = OIS::KC_V;
    m_all_keys["VOLUMEDOWN"]   = OIS::KC_VOLUMEDOWN;
    m_all_keys["VOLUMEUP"]     = OIS::KC_VOLUMEUP;
    m_all_keys["W"]            = OIS::KC_W;
    m_all_keys["WAKE"]         = OIS::KC_WAKE;
    m_all_keys["WEBBACK"]      = OIS::KC_WEBBACK;
    m_all_keys["WEBFAVORITES"] = OIS::KC_WEBFAVORITES;
    m_all_keys["WEBFORWARD"]   = OIS::KC_WEBFORWARD;
    m_all_keys["WEBHOME"]      = OIS::KC_WEBHOME;
    m_all_keys["WEBREFRESH"]   = OIS::KC_WEBREFRESH;
    m_all_keys["WEBSEARCH"]    = OIS::KC_WEBSEARCH;
    m_all_keys["WEBSTOP"]      = OIS::KC_WEBSTOP;
    m_all_keys["X"]            = OIS::KC_X;
    m_all_keys["Y"]            = OIS::KC_Y;
    m_all_keys["YEN"]          = OIS::KC_YEN;
    m_all_keys["Z"]            = OIS::KC_Z;
}

String InputEngine::getKeyForCommand(int eventID)
{
    std::map<int, std::vector<event_trigger_t>>::iterator it = m_events.find(eventID);

    if (it == m_events.end())
        return String();
    if (it->second.empty())
        return String();

    std::vector<event_trigger_t>::iterator it2 = it->second.begin();
    return getKeyNameForKeyCode((OIS::KeyCode)it2->keyCode);
}

void InputEngine::HandleException(std::string action)
{
    try
    {
        throw; // rethrow
    }
    catch (OIS::Exception& ex)
    {
        LOG("[RoR|Input] Error during "+action+", type: 'OIS::Exception', message: " + ex.eText);
    }
    catch (Ogre::Exception& oex)
    {
        LOG("[RoR|Input] Error during "+action+", type: 'OGRE::Exception', message: " + oex.getFullDescription());
    }
    catch (std::exception& stdex)
    {
        LOG("[RoR|Input] Error during "+action+", type: 'std::exception', message: " + stdex.what());
    }
}
