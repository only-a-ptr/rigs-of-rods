#pragma once

// @file
// Macroscripts (or just 'macros') are recorded user actions which can be played back
// They run on physics engine update frequency (constant PHYSICS_DT, 2khz)

#include "InputEngine.h" // Event types

#include <vector>
#include <memory> // std::unique_ptr

namespace RoR {

struct MacroAction
{
    enum class ActionType
    {
        INVALID,
        SLEEP,             //!< Wait for 'value_i' physics ticks (2khz) before next Action
        SET_EVENT_VALUE,   //!< Set value of game event 'event_id'. Initial value of all events is 0.
        TELEPORT_NAME,     //!< Teleport player avatar to telepoint 'name'
        TELEPORT_POS,      //!< Teleport player avatar to position 'location'
        SPAWN_ACTOR        //!< Spawn truckfile 'name'
    };

    explicit MacroAction(events ev, float val):             ma_type(ActionType::SET_EVENT_VALUE), ma_event_id(ev), ma_value_f(val) {}
    explicit MacroAction(ActionType a, int ival):           ma_type(a), ma_value_i(ival) {}
    explicit MacroAction(ActionType a, Ogre::Vector3 loc):  ma_type(a), ma_location(loc) {}
    explicit MacroAction(ActionType a, const char* name):   ma_type(a), ma_name(name) {}

    ActionType    ma_type;
    float         ma_value_f;
    int           ma_value_i;
    events        ma_event_id;
    Ogre::Vector3 ma_location;
    std::string   ma_name;
};

struct MacroScript
{
    std::vector<MacroAction> ms_actions;
    std::string              ms_name;
};

/// Macro processing state machine; controlled from the game console
/// Console command syntax: macro [command] [argument]
///   -no command- -- Prints help
///   RECORD       -- Start recording (arg = new name) or resume recording (no arguments)
///   PLAY         -- Start playback (arg = name) or resume playback (no args)
///   PAUSE        -- Pause recording/playback (no arg)
///   RESUME       -- Pause recording/playback (no arg)
///   LIST         -- List existing macroscripts (no arg)
///   DELETE       -- Delete macroscript (arg = name)
class MacroManager
{
public:
    enum class State { IDLE, RECORDING, REC_PAUSED, PLAYBACK, PLAY_PAUSED };

    MacroManager(): m_state(State::IDLE), m_active_macro(nullptr) {}

    void ProcessConsole(const char* cmd, const char* arg);
    bool IsRecording() const { return m_state == State::RECORDING; }

    void RecordEventChanged(events ev, float val)
    {
        this->RecordSleep();
        m_active_macro->ms_actions.push_back(MacroAction(ev, val));
    }

    void RecordTeleportToPosition(Ogre::Vector3 pos)
    {
        this->RecordSleep();
        m_active_macro->ms_actions.push_back(MacroAction(MacroAction::ActionType::TELEPORT_POS, pos));
    }

    void RecordTeleportToTelepoint(const char* name)
    {
        this->RecordSleep();
        m_active_macro->ms_actions.push_back(MacroAction(MacroAction::ActionType::TELEPORT_NAME, name));
    }

    void AdvanceTime(size_t num_steps)
    {
        m_step_counter += num_steps;
    }

private:

    void ReplyConsole(const char* format, ...); //!< Prints to console as 'CONSOLE_SYSTEM_REPLY'

    void RecordSleep()
    {
        if (m_step_counter != 0)
        {
            m_active_macro->ms_actions.push_back(MacroAction(MacroAction::ActionType::SLEEP, static_cast<int>(m_step_counter)));
            m_step_counter = 0;
        }
    }

    State          m_state;
    MacroScript*   m_active_macro;  //!< Playback or recording
    size_t         m_step_counter;  //!< Playback = position; Recording = elapsed steps since last recorded action.
    std::vector<std::unique_ptr<MacroScript>> m_all_macros;
};

} // namespace RoR
