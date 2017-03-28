/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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
#include "SoundscriptFileformat.h"

enum {
    MAX_SOUNDS_PER_SCRIPT = 30, // TODO: make unlimited; temporarily just setting very high. ~ only_a_ptr 03/2017
};

enum SoundTriggers {
    SS_TRIG_NONE = -1,
    SS_TRIG_ENGINE = 0,
    SS_TRIG_AEROENGINE1,
    SS_TRIG_AEROENGINE2,
    SS_TRIG_AEROENGINE3,
    SS_TRIG_AEROENGINE4,
    SS_TRIG_AEROENGINE5,
    SS_TRIG_AEROENGINE6,
    SS_TRIG_AEROENGINE7,
    SS_TRIG_AEROENGINE8,
    SS_TRIG_HORN,
    SS_TRIG_BRAKE,
    SS_TRIG_PUMP,
    SS_TRIG_STARTER,
    SS_TRIG_TURBOBOV,
    SS_TRIG_TURBOWASTEGATE,
    SS_TRIG_TURBOBACKFIRE,
    SS_TRIG_ALWAYSON,
    SS_TRIG_REPAIR,
    SS_TRIG_AIR,
    SS_TRIG_GPWS_APDISCONNECT,
    SS_TRIG_GPWS_10,
    SS_TRIG_GPWS_20,
    SS_TRIG_GPWS_30,
    SS_TRIG_GPWS_40,
    SS_TRIG_GPWS_50,
    SS_TRIG_GPWS_100,
    SS_TRIG_GPWS_PULLUP,
    SS_TRIG_GPWS_MINIMUMS,
    SS_TRIG_AIR_PURGE,
    SS_TRIG_SHIFT,
    SS_TRIG_GEARSLIDE,
    SS_TRIG_BREAK,
    SS_TRIG_SCREETCH,
    SS_TRIG_PARK,
    SS_TRIG_AFTERBURNER1,
    SS_TRIG_AFTERBURNER2,
    SS_TRIG_AFTERBURNER3,
    SS_TRIG_AFTERBURNER4,
    SS_TRIG_AFTERBURNER5,
    SS_TRIG_AFTERBURNER6,
    SS_TRIG_AFTERBURNER7,
    SS_TRIG_AFTERBURNER8,
    SS_TRIG_AOA,
    SS_TRIG_IGNITION,
    SS_TRIG_REVERSE_GEAR,
    SS_TRIG_TURN_SIGNAL,
    SS_TRIG_TURN_SIGNAL_TICK,
    SS_TRIG_TURN_SIGNAL_WARN_TICK,
    SS_TRIG_ALB_ACTIVE,
    SS_TRIG_TC_ACTIVE,
    SS_TRIG_AVICHAT01,
    SS_TRIG_AVICHAT02,
    SS_TRIG_AVICHAT03,
    SS_TRIG_AVICHAT04,
    SS_TRIG_AVICHAT05,
    SS_TRIG_AVICHAT06,
    SS_TRIG_AVICHAT07,
    SS_TRIG_AVICHAT08,
    SS_TRIG_AVICHAT09,
    SS_TRIG_AVICHAT10,
    SS_TRIG_AVICHAT11,
    SS_TRIG_AVICHAT12,
    SS_TRIG_AVICHAT13,
    SS_TRIG_LINKED_COMMAND,
    SS_TRIG_MAIN_MENU,
    SS_MAX_TRIG
};

enum ModulationSources {
    SS_MOD_NONE,
    SS_MOD_ENGINE,
    SS_MOD_TURBO,
    SS_MOD_AEROENGINE1,
    SS_MOD_AEROENGINE2,
    SS_MOD_AEROENGINE3,
    SS_MOD_AEROENGINE4,
    SS_MOD_AEROENGINE5,
    SS_MOD_AEROENGINE6,
    SS_MOD_AEROENGINE7,
    SS_MOD_AEROENGINE8,
    SS_MOD_WHEELSPEED,
    SS_MOD_INJECTOR,
    SS_MOD_TORQUE,
    SS_MOD_GEARBOX,
    SS_MOD_BREAK,
    SS_MOD_SCREETCH,
    SS_MOD_PUMP,
    SS_MOD_THROTTLE1,
    SS_MOD_THROTTLE2,
    SS_MOD_THROTTLE3,
    SS_MOD_THROTTLE4,
    SS_MOD_THROTTLE5,
    SS_MOD_THROTTLE6,
    SS_MOD_THROTTLE7,
    SS_MOD_THROTTLE8,
    SS_MOD_AIRSPEED,
    SS_MOD_AOA,
    SS_MOD_LINKED_COMMANDRATE,
    SS_MOD_MUSIC_VOLUME,
    SS_MAX_MOD
};

class Sound;
class SoundManager;

class SoundScriptInstance : public ZeroedMemoryAllocator
{
    friend class SoundScriptManager;

public:

    SoundScriptInstance(RoR::SoundScriptDef* def, SoundManager* sm, int node_id, int type);
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

    inline SoundTriggers      GetTrigger()     const { return m_trigger; }
    inline ModulationSources  GetPitchSource() const { return m_pitch_source; }
    inline ModulationSources  GetGainSource()  const { return m_gain_source; }
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

    struct Def
    {
        float start_sound_pitch;
        float stop_sound_pitch;
        float pitches[MAX_SOUNDS_PER_SCRIPT];
        size_t num_sounds;
        float gain_square;
        float gain_mul;
        float gain_offset;
        float pitch_square;
        float pitch_mul;
        float pitch_offset;
    } m_def;

    SoundTriggers      m_trigger;
    ModulationSources  m_gain_source;
    ModulationSources  m_pitch_source;
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
    RoR::SoundScriptDef* GetSoundScriptDef(std::string name);
    void PlayMusic(SoundTriggers trig); ///< Use SS_TRIG_NONE to stop music!
    inline SoundManager* GetSoundMgr() { return sound_manager; }

private:

    bool disabled;
    bool loading_base;
    float max_distance;
    float reference_distance;
    float rolloff_factor;
    int instance_counter;
    Ogre::StringVector script_patterns;
    SoundScriptInstance* m_music;

    std::unordered_map <std::string, RoR::SoundScriptDef> m_defs;

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
