/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#ifdef USE_OPENAL

#pragma once

#include "RoRPrerequisites.h"

#include "Singleton.h"

#include <OgreScriptLoader.h>

enum {
    MAX_SOUNDS_PER_SCRIPT = 16
};

enum SoundTriggers {        // RESEARCH FOR DEFERRED SOUND PROCESSING ~ only_a_ptr, 03/2017
    SS_TRIG_NONE = -1,
    SS_TRIG_ENGINE = 0,     // DONE
    SS_TRIG_AEROENGINE1,    // DONE
    SS_TRIG_AEROENGINE2,    // ^
    SS_TRIG_AEROENGINE3,    // ^
    SS_TRIG_AEROENGINE4,    // ^
    SS_TRIG_AEROENGINE5,    // ^
    SS_TRIG_AEROENGINE6,    // ^
    SS_TRIG_AEROENGINE7,    // ^
    SS_TRIG_AEROENGINE8,    // ^
    SS_TRIG_HORN,           // DONE
    SS_TRIG_BRAKE,          // DONE
    SS_TRIG_PUMP,           // DONE
    SS_TRIG_STARTER,        // DONE
    SS_TRIG_TURBOBOV,       // DONE
    SS_TRIG_TURBOWASTEGATE, // DONE
    SS_TRIG_TURBOBACKFIRE,  // DONE
    SS_TRIG_ALWAYSON,       // DUMMY, never used in simulation.
    SS_TRIG_REPAIR,         // DONE
    SS_TRIG_AIR,
    SS_TRIG_GPWS_APDISCONNECT, // trigOnce: hint needed
    SS_TRIG_GPWS_10,        // DONE
    SS_TRIG_GPWS_20,        // DONE
    SS_TRIG_GPWS_30,        // DONE
    SS_TRIG_GPWS_40,        // DONE
    SS_TRIG_GPWS_50,        // DONE
    SS_TRIG_GPWS_100,       // DONE
    SS_TRIG_GPWS_PULLUP,    // DONE
    SS_TRIG_GPWS_MINIMUMS,  // DONE
    SS_TRIG_AIR_PURGE,      // DONE
    SS_TRIG_SHIFT,          // DONE
    SS_TRIG_GEARSLIDE,      // DONE
    SS_TRIG_CREAK,          // *** DEAD ***
    SS_TRIG_BREAK,          // DONE
    SS_TRIG_SCREETCH,       // DONE
    SS_TRIG_PARK,           // DONE
    SS_TRIG_AFTERBURNER1,   // DONE
    SS_TRIG_AFTERBURNER2,   // ^
    SS_TRIG_AFTERBURNER3,   // ^
    SS_TRIG_AFTERBURNER4,   // ^
    SS_TRIG_AFTERBURNER5,   // ^
    SS_TRIG_AFTERBURNER6,   // ^
    SS_TRIG_AFTERBURNER7,   // ^
    SS_TRIG_AFTERBURNER8,   // ^
    SS_TRIG_AOA,            // DONE
    SS_TRIG_IGNITION,       // DONE
    SS_TRIG_REVERSE_GEAR,   // DONE
    SS_TRIG_TURN_SIGNAL,    // DONE
    SS_TRIG_TURN_SIGNAL_TICK, // DONE
    SS_TRIG_TURN_SIGNAL_WARN_TICK, // DONE
    SS_TRIG_ALB_ACTIVE,     // DONE
    SS_TRIG_TC_ACTIVE,      // DONE
    SS_TRIG_AVICHAT01, //DONE
    SS_TRIG_AVICHAT02, //DONE
    SS_TRIG_AVICHAT03, //DONE
    SS_TRIG_AVICHAT04, //DONE
    SS_TRIG_AVICHAT05, //DONE
    SS_TRIG_AVICHAT06, //DONE
    SS_TRIG_AVICHAT07, //DONE
    SS_TRIG_AVICHAT08, //DONE
    SS_TRIG_AVICHAT09, //DONE
    SS_TRIG_AVICHAT10, //DONE
    SS_TRIG_AVICHAT11, //DONE
    SS_TRIG_AVICHAT12, //DONE
    SS_TRIG_AVICHAT13, //DONE
    SS_TRIG_LINKED_COMMAND,
    SS_TRIG_MAIN_MENU,
    SS_MAX_TRIG
};

enum ModulationSources {
    SS_MOD_NONE,
    SS_MOD_ENGINE,      // DONE
    SS_MOD_TURBO,       // DONE
    SS_MOD_AEROENGINE1, // DONE
    SS_MOD_AEROENGINE2, // ^
    SS_MOD_AEROENGINE3, // ^
    SS_MOD_AEROENGINE4, // ^
    SS_MOD_AEROENGINE5, // ^
    SS_MOD_AEROENGINE6, // ^
    SS_MOD_AEROENGINE7, // ^
    SS_MOD_AEROENGINE8, // ^
    SS_MOD_WHEELSPEED,  // DONE
    SS_MOD_INJECTOR,    // DONE
    SS_MOD_TORQUE,      // DONE
    SS_MOD_GEARBOX,     // DONE
    SS_MOD_CREAK,       // *** DEAD ***
    SS_MOD_BREAK,       // DONE
    SS_MOD_SCREETCH,    // DONE
    SS_MOD_PUMP,        // DONE
    SS_MOD_THROTTLE1,   // DONE
    SS_MOD_THROTTLE2,   // ^
    SS_MOD_THROTTLE3,   // ^
    SS_MOD_THROTTLE4,   // ^
    SS_MOD_THROTTLE5,   // ^
    SS_MOD_THROTTLE6,   // ^
    SS_MOD_THROTTLE7,   // ^
    SS_MOD_THROTTLE8,   // ^
    SS_MOD_AIRSPEED,    // DONE
    SS_MOD_AOA,         // DONE
    SS_MOD_LINKED_COMMANDRATE,
    SS_MOD_MUSIC_VOLUME,
    SS_MAX_MOD
};

class Sound;
class SoundManager;

class SoundScriptTemplate : public ZeroedMemoryAllocator
{
    friend class SoundScriptManager;
    friend class SoundScriptInstance;

public:

    SoundScriptTemplate(Ogre::String name, Ogre::String groupname, Ogre::String filename, bool baseTemplate);
    
private:

    int parseModulation(Ogre::String str);
    bool setParameter(Ogre::StringVector vec);

    Ogre::String name;
    Ogre::String file_name;

    bool         base_template;
    bool         has_start_sound;
    bool         has_stop_sound;
    bool         unpitchable;

    float        gain_multiplier;
    float        gain_offset;
    float        gain_square;
    int          gain_source;

    float        pitch_multiplier;
    float        pitch_offset;
    float        pitch_square;
    int          pitch_source;

    Ogre::String sound_names[MAX_SOUNDS_PER_SCRIPT];
    float        sound_pitches[MAX_SOUNDS_PER_SCRIPT];
    Ogre::String start_sound_name;
    float        start_sound_pitch;
    Ogre::String stop_sound_name;
    float        stop_sound_pitch;

    int          trigger_source;
    int          free_sound;
};

class SoundScriptInstance : public ZeroedMemoryAllocator
{
    friend class SoundScriptManager;
    friend class RigInspector;

public:

    SoundScriptInstance(SoundScriptTemplate* templ, SoundManager* sm, int node_id, int type);
    void runOnce();
    void setEnabled(bool e);
    void setPosition(Ogre::Vector3 pos, Ogre::Vector3 velocity);
    void start();
    void stop();
    void kill();

    inline void transition(const bool cur_state, const bool prev_state)
    {
        if (cur_state && !prev_state)
            this->start();
        else if (!cur_state && prev_state)
            this->stop();
    }

    inline void SetActive(bool active)
    {
        if(active)
            this->start();
        else
            this->stop();
    }

    inline SoundTriggers      GetTrigger()     const { return SoundTriggers(templ->trigger_source); }
    inline ModulationSources  GetPitchSource() const { return ModulationSources(templ->pitch_source); }
    inline ModulationSources  GetGainSource()  const { return ModulationSources(templ->gain_source); }
    inline int                GetNodeId()      const { return m_node_id; }
    inline int                GetType()        const { return m_type; }

    void ModulateGain(float value);
    void ModulatePitch(float value);

    static const float PITCHDOWN_FADE_FACTOR;
    static const float PITCHDOWN_CUTOFF_FACTOR;

private:

    void setGain(float value);
    void setPitch(float value);

    float pitchgain_cutoff(float sourcepitch, float targetpitch);

    SoundScriptTemplate* templ;
    Sound *start_sound;
    Sound *stop_sound;
    Sound *sounds[MAX_SOUNDS_PER_SCRIPT];
    float start_sound_pitchgain;
    float stop_sound_pitchgain;
    float sounds_pitchgain[MAX_SOUNDS_PER_SCRIPT];
    float lastgain;
    int   m_node_id;
    int   m_type; ///< -2 = global, -1: external cam. only, 0+: cinecam index
};

class SoundScriptManager : public Ogre::ScriptLoader, public RoRSingleton<SoundScriptManager>, public ZeroedMemoryAllocator
{
public:

    SoundScriptManager();

    // ScriptLoader interface
    const Ogre::StringVector& getScriptPatterns(void) const;
    void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName);
    Ogre::Real getLoadingOrder(void) const;

    void clearNonBaseTemplates();

    void setEnabled(bool state);

    void setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity);
    void setLoadingBaseSounds(bool value) { loading_base = value; };

    bool isDisabled() { return disabled; }

    SoundScriptTemplate* GetSoundScriptTemplate(std::string name);
    void PlayMusic(SoundTriggers trig); ///< Use SS_TRIG_NONE to stop music!
    inline SoundManager* GetSoundMgr() { return sound_manager; }

private:

    SoundScriptTemplate* createTemplate(Ogre::String name, Ogre::String groupname, Ogre::String filename);
    void skipToNextCloseBrace(Ogre::DataStreamPtr& chunk);
    void skipToNextOpenBrace(Ogre::DataStreamPtr& chunk);

    bool disabled;
    bool loading_base;
    float max_distance;
    float reference_distance;
    float rolloff_factor;
    int instance_counter;
    Ogre::StringVector script_patterns;
    SoundScriptInstance* m_music;

    std::map <Ogre::String, SoundScriptTemplate*> templates;

    SoundManager* sound_manager;
};

namespace RoR {

/// Manages sounds for a single actor (instance of Beam)
class AudioActor
{
public:
    AudioActor(Beam* actor);
    ~AudioActor();

    void          AddSound               (int node_id, int mode, std::string soundscript_name);
    void          UpdateSounds           ();
    void          SetHydropumpState      (bool active, float modulation);
    void          SetAoaState            (bool active, float modulation);
    void          PlayScreetchOnce       (float modulation);
    void          PlayBreakOnce          (float modulation);
    void          SetMuteAllSounds       (bool mute);
    void          NotifyChangedCamera    (int cam_type);
    inline void   TriggerAviChatter      (SoundTriggers trig) { m_avi_chatter_trigger = trig; }
    inline void   PlayWarnTickOnce       ()                   { m_play_warn_signal_tick = true; }
    inline void   PlayTurnTickOnce       ()                   { m_play_turn_signal_tick = true; }
    inline void   PlayRepairSoundOnce    ()                   { m_repair_play_once = true; }
    inline void   SetEngineModulation    (float m)            { m_engine_modulation = m; }
    inline void   SetEngineForcedState   (int s)              { m_engine_forced_state = s; }
    inline void   SetTirePressureActive  (bool s)             { m_tirepressure_active = s; }
    inline void   SetStabilizersActive   (bool s)             { m_stabilizers_active = s; }
    inline void   NotifyGpwsApDisconnect ()                   { m_gpws_ap_disconnected = true; }

private:
    bool ResolveModulation(float& result, const ModulationSources src);

    Beam* m_actor;
    std::vector<SoundScriptInstance*> m_sounds;

    // States - values
    float m_hydropump_modulation;
    float m_aoa_modulation;
    SoundTriggers m_avi_chatter_trigger;
    float m_screetch_modulation;
    float m_break_modulation;
    float m_engine_modulation;
    int   m_engine_forced_state; // 0=none, -1 forced off, 1 = forced on
    // States - bit flags
    bool  m_aoa_active:1;
    bool  m_hydropump_active:1;
    bool  m_play_turn_signal_tick:1;
    bool  m_play_warn_signal_tick:1;
    bool  m_repair_play_once:1;
    bool  m_screetch_play_once:1;
    bool  m_break_play_once:1;
    bool  m_tirepressure_active:1;
    bool  m_stabilizers_active:1;
    bool  m_gpws_ap_disconnected:1;
};

} // namespace RoR

#endif // USE_OPENAL
