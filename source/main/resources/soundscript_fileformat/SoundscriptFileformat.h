/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2017 Petr Ohlidal & contributors

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
/// @author Petr Ohlidal, 03/2017
/// @brief 'Soundscript' fileformat handling.

#include <OgreDataStream.h>

#include <string>
#include <list>
#include <unordered_map>

namespace RoR {

struct SoundTriggerDef
{
    enum TriggerSource
    {
        SND_TRIG_NONE = 0,
        SND_TRIG_ENGINE,
        SND_TRIG_AEROENGINE_RPM,     ///< Param = aeroengine index; replaces "AEROENGINE[1-8]"
        SND_TRIG_HORN,
        SND_TRIG_BRAKE,
        SND_TRIG_PUMP,
        SND_TRIG_STARTER,
        SND_TRIG_TURBO_BOV,
        SND_TRIG_TURBO_WASTEGATE,
        SND_TRIG_TURBO_BACKFIRE,
        SND_TRIG_ALWAYS_ON,
        SND_TRIG_REPAIR,
        SND_TRIG_AIR,
        SND_TRIG_GPWS_APDISCONNECT,
        SND_TRIG_GPWS_VALUE,         ///< Param = value; replaces "GPWS[10|20|30|40|50|100]"
        SND_TRIG_GPWS_PULLUP,
        SND_TRIG_GPWS_MINIMUMS,
        SND_TRIG_AIR_PURGE,
        SND_TRIG_SHIFT,
        SND_TRIG_GEARSLIDE,
        SND_TRIG_BREAK,
        SND_TRIG_SCREETCH,
        SND_TRIG_PARK,
        SND_TRIG_AFTERBURNER,        ///< Param = aeroengine index; replaces "AFTERBURNER[1-8]"
        SND_TRIG_AOA,
        SND_TRIG_IGNITION,
        SND_TRIG_REVERSE_GEAR,
        SND_TRIG_TURN_SIGNAL,
        SND_TRIG_TURN_SIGNAL_TICK,
        SND_TRIG_TURN_SIGNAL_WARN_TICK,
        SND_TRIG_ALB_ACTIVE,
        SND_TRIG_TC_ACTIVE,
        SND_TRIG_AVIONIC_CHAT,       ///< Param = index; replaces "AVICHAT[00-13]"
        SND_TRIG_MAIN_MENU,
        SND_MAX_TRIG
    };

    SoundTriggerDef():
        source(SND_TRIG_NONE),
        source_param(-1)
    {}

    TriggerSource source;
    int           source_param;
};

/// Generalized 'dynamic attribute' of sound - used for "gain" and "pitch"
struct SoundModulationDef
{
    enum ModulationSource
    {
        SND_MOD_NONE = 0,
        SND_MOD_ENGINE,
        SND_MOD_TURBO,
        SND_MOD_AEROENGINE_RPM,      ///< Param = aeroengine index; replaces "AEROENGINE[1-8]"
        SND_MOD_WHEELSPEED,
        SND_MOD_INJECTOR,
        SND_MOD_TORQUE,
        SND_MOD_GEARBOX,
        SND_MOD_BREAK,
        SND_MOD_SCREETCH,
        SND_MOD_PUMP,
        SND_MOD_AEROENGINE_THROTTLE, ///< Param = aeroengine index; replaces "THROTTLE[1-8]"
        SND_MOD_AIRSPEED,
        SND_MOD_AOA,
        SND_MOD_LINKED_COMMANDRATE,
        SND_MOD_MUSIC_VOLUME,
        SND_MAX_MOD
    };

    SoundModulationDef():
        multiplier(0.f),
        offset(0.f),
        square(0.f),
        source(SND_MOD_NONE),
        source_param(-1)
    {}

    float            multiplier;
    float            offset;
    float            square;
    ModulationSource source;
    int              source_param;
};

struct SoundFileDef
{
    SoundFileDef(): pitch(0.f) {}

    std::string filename;
    float       pitch;
};

struct SoundScriptDef
{
    std::string             name;
    std::string             resource_group;
    bool                    is_base; // TODO: remove (legacy logic)
    SoundFileDef            file_start;
    SoundFileDef            file_stop;
    std::list<SoundFileDef> files;
    SoundModulationDef      pitch;
    SoundModulationDef      gain;
    SoundTriggerDef         trigger;
};

class SoundScriptParser
{
public:
    SoundScriptParser(std::unordered_map<std::string, SoundScriptDef>& defs):
        m_defs(defs)
    {}

    void ProcessOgreDataStream(Ogre::DataStreamPtr stream, std::string const & resource_group, bool base_flag);

    inline std::list<std::string>& GetMessages() { return m_messages; }

private:

    enum ExpectedElement ///< Parser state; Blank lines and comments are always ignored
    {
        SSP_NEXT_INVALID,
        SSP_NEXT_NAME,          ///< Parse next line as soundscript name
        SSP_NEXT_OPENBRACE,     ///< Go to next '{'; ignore all else
        SSP_NEXT_OPENBRACE_BAD, ///< Go to next '{'; ignore all else; the def. ahead will be discarded
        SSP_NEXT_DEF,           ///< Parse next line(s) as parameter until '}'
        SSP_NEXT_DEF_BAD        ///< Ignore next line(s) until '}'
    };

    void  ProcessLine         (char* line); ///< Independent of data source
    bool  TrimAndCheckComment (char** line);
    void  AddMessage          (const char* text);
    void  ParseTriggerDef     (char* value);
    void  ParseModulation     (SoundModulationDef* def, char* line);
    void  ParseParamLine      (char* line);

    std::list<std::string>    m_messages;
    size_t                    m_line_number;
    ExpectedElement           m_expected;
    std::string               m_resource_group;
    bool                      m_base_flag;

    std::unordered_map<std::string, SoundScriptDef>&          m_defs;
    std::unordered_map<std::string, SoundScriptDef>::iterator m_last_def;
};

} // namespace RoR