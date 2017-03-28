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

#include "SoundscriptFileformat.h"

#include "RoRPrerequisites.h"

using namespace RoR;

static const size_t LINE_BUF_SIZE = 4000;

void SoundScriptParser::ProcessOgreDataStream(Ogre::DataStreamPtr stream, std::string const & resource_group, bool base_flag)
{
    // Reset the parser
    m_line_number = 0;
    m_messages.clear();
    m_expected = SSP_NEXT_NAME;
    m_resource_group = resource_group;
    m_base_flag = base_flag;

    char line_buf[LINE_BUF_SIZE];
    while (!stream->eof())
    {
        stream->readLine(line_buf, LINE_BUF_SIZE);
        ++m_line_number;
        this->ProcessLine(line_buf);
    }
}

bool SoundScriptParser::TrimAndCheckComment(char** line_ptr)
{
    // Trim leading blanks
    char* line = *line_ptr;
    while (*line == ' ' || *line == '\t')
        ++line;

    // Detect blankline and comment
    if ((*line == '\0') || (*line == '/' && *(line+1) == '/'))
        return true;

    // Trim trailing blanks
    size_t last = strlen(line) - 1;
    while (line[last] == ' ' || line[last] == '\t')
    {
        line[last--] = '\0';
    }
    
    // Update and return
    *line_ptr = line;
    return false;
}

void SoundScriptParser::ProcessLine(char* line)
{
    if (this->TrimAndCheckComment(&line))
        return;

    switch (m_expected)
    {
    case SSP_NEXT_NAME:
        if (m_defs.find(line) != m_defs.end())
        {
            char err_buf[200];
            snprintf(err_buf, 200, "WARNING: Duplicate soundscript '%s', skipping.", line);
            this->AddMessage(err_buf);
            m_expected = SSP_NEXT_OPENBRACE_BAD;
        }
        else
        {
            SoundScriptDef def;
            def.name = line;
            def.resource_group = m_resource_group;
            def.is_base = m_base_flag;
            auto result = m_defs.insert({line, def});
            m_last_def = result.first;
            m_expected = SSP_NEXT_OPENBRACE;
        }
        break;

    case SSP_NEXT_OPENBRACE:
        if (line[0] == '{')
            m_expected = SSP_NEXT_DEF;
        break;

    case SSP_NEXT_OPENBRACE_BAD:
        if (line[0] == '{')
            m_expected = SSP_NEXT_DEF_BAD;
        break;

    case SSP_NEXT_DEF:
        if (line[0] == '}')
            m_expected = SSP_NEXT_NAME;
        else
            this->ParseParamLine(line);
        break;

    case SSP_NEXT_DEF_BAD:
        if (line[0] == '}')
            m_expected = SSP_NEXT_NAME;
        break;

    default:
        this->AddMessage("INTERNAL ERROR: Invalid parser state");
    }
}

void SoundScriptParser::ParseParamLine(char* line)
{
    SoundScriptDef* def = &m_last_def->second;
    char* tok = strtok(line, " \t");
    if (strcmp(tok, "trigger_source") == 0)
    {
        tok = strtok(nullptr, " \t");
        if (tok == nullptr)
        {
            this->AddMessage("WARNING: Missing parameter for 'trigger_source'");
            return;
        }
        this->ParseTriggerDef(tok);
    }
    else if (strcmp(tok, "pitch_source") == 0)
    {
        tok = strtok(nullptr, " \t");
        if (tok == nullptr)
        {
            this->AddMessage("WARNING: Missing parameter for 'pitch_source'");
            return;
        }
        this->ParseModulation(&def->pitch, tok);
    }
    else if (strcmp(tok, "gain_source") == 0)
    {
        tok = strtok(nullptr, " \t");
        if (tok == nullptr)
        {
            this->AddMessage("WARNING: Missing parameter for 'gain_source'");
            return;
        }
        this->ParseModulation(&def->gain, tok);
    }
    else if (strcmp(tok, "pitch_factors") == 0)
    {
        char* tok_offset = strtok(nullptr, " \t");
        char* tok_multip = strtok(nullptr, " \t");
        char* tok_square = strtok(nullptr, " \t");
        if (tok_multip == nullptr)
        {
            this->AddMessage("WARNING: Missing parameter for 'pitch_factors'");
            return;
        }
        def->pitch.offset     = PARSEREAL(tok_offset);
        def->pitch.multiplier = PARSEREAL(tok_multip);
        if (tok_square != nullptr)
            def->pitch.square = PARSEREAL(tok_square);
    }
    else if (strcmp(tok, "gain_factors") == 0)
    {
        char* tok_offset = strtok(nullptr, " \t");
        char* tok_multip = strtok(nullptr, " \t");
        char* tok_square = strtok(nullptr, " \t");
        if (tok_multip == nullptr)
        {
            this->AddMessage("WARNING: Missing parameter for 'gain_factors'");
            return;
        }
        def->gain.offset     = PARSEREAL(tok_offset);
        def->gain.multiplier = PARSEREAL(tok_multip);
        if (tok_square != nullptr)
            def->gain.square = PARSEREAL(tok_square);
    }
    else if (strcmp(tok, "start_sound") == 0)
    {
        char* tok_pitch = strtok(nullptr, " \t");
        char* tok_file  = strtok(nullptr, " \t");
        if (tok_file == nullptr)
        {
            this->AddMessage("WARNING: Incomplete param 'start_sound'");
            return;
        }
        def->file_start.pitch = PARSEREAL(tok_pitch); // unparsable (e.g. "unpitched") will result in value 0.0
        def->file_start.filename = tok_file;
    }
    else if (strcmp(tok, "stop_sound") == 0)
    {
        char* tok_pitch = strtok(nullptr, " \t");
        char* tok_file  = strtok(nullptr, " \t");
        if (tok_file == nullptr)
        {
            this->AddMessage("WARNING: Incomplete param 'stop_sound'");
            return;
        }
        def->file_stop.pitch = PARSEREAL(tok_pitch); // unparsable (e.g. "unpitched") will result in value 0.0
        def->file_stop.filename = tok_file;
    }
    else if (strcmp(tok, "sound"))
    {
        char* tok_pitch = strtok(nullptr, " \t");
        char* tok_file  = strtok(nullptr, " \t");
        if (tok_file == nullptr)
        {
            this->AddMessage("WARNING: Incomplete param 'sound'");
            return;
        }

        SoundFileDef file_def;
        file_def.pitch = PARSEREAL(tok_pitch); // unparsable (e.g. "unpitched") will result in value 0.0
        file_def.filename = tok_file;

        // Enforce pitches forming an ascending sequence
        if (!def->files.empty())
        {
            const bool unpitched = (def->files.front().pitch == 0.f);
            if (!unpitched && (def->files.back().pitch >= file_def.pitch))
            {
                this->AddMessage("WARNING: Dropping 'sound' entry; pitch MUST be higher than previous");
                return;
            }
        }
        def->files.push_back(file_def);
    }
}

void SoundScriptParser::AddMessage(const char* text)
{
    char buf[1000];
    snprintf(buf, 1000, "Line %ul: %s", m_line_number, text);
    m_messages.emplace_back<std::string>(buf);
}

#define TEST(_TEXT_, _LEN_) (strncmp(line, _TEXT_, _LEN_) == 0)

void SoundScriptParser::ParseTriggerDef(char* line)
{
    SoundTriggerDef def;
    char err_buf[200];

    if (TEST("avionic_chat_", 13))
    {
        int param = PARSEINT(line + 13); // Returns 0 on error
        if (param < 1 || param > 13)
        {
            snprintf(err_buf, 200, "ERROR: Invalid trigger source '%s'", line);
            this->AddMessage(err_buf);
            return;
        }
        def.source = SoundTriggerDef::SND_TRIG_AVIONIC_CHAT;
        def.source_param = param - 1; // 0-based index
    }
    else if (TEST("aeroengine", 10))
    {
        int param = PARSEINT(line + 10);
        if (param < 1 || param > 8)
        {
            snprintf(err_buf, 200, "ERROR: Invalid trigger source '%s'", line);
            this->AddMessage(err_buf);
            return;
        }
        def.source = SoundTriggerDef::SND_TRIG_AEROENGINE_RPM;
        def.source_param = param - 1; // 0-based index
    }
    else if (TEST("afterburner", 8))
    {
        int param = PARSEINT(line + 11);
        if (param < 1 || param > 10)
        {
            snprintf(err_buf, 200, "ERROR: Invalid trigger source '%s'", line);
            this->AddMessage(err_buf);
            return;
        }
        def.source = SoundTriggerDef::SND_TRIG_AFTERBURNER;
        def.source_param = param - 1; // 0-based index
    }
    else if (TEST("gpws_", 5))
    {
        int param = PARSEINT(line + 5);
        if ((param!=10) || (param!=20) || (param!=30) || (param!=40) || (param!=50) || (param!=100))
        {
            snprintf(err_buf, 200, "ERROR: Invalid trigger source '%s'", line);
            this->AddMessage(err_buf);
            return;
        }
        def.source = SoundTriggerDef::SND_TRIG_GPWS_VALUE;
        def.source_param = param;
    }
    else if (TEST("engine",                 6)) { def.source = SoundTriggerDef::SND_TRIG_ENGINE; }
    else if (TEST("horn",                   4)) { def.source = SoundTriggerDef::SND_TRIG_HORN; }
    else if (TEST("brake",                  5)) { def.source = SoundTriggerDef::SND_TRIG_BRAKE; }
    else if (TEST("pump",                   4)) { def.source = SoundTriggerDef::SND_TRIG_PUMP; }
    else if (TEST("starter",                7)) { def.source = SoundTriggerDef::SND_TRIG_STARTER; }
    else if (TEST("turbo_BOV",              9)) { def.source = SoundTriggerDef::SND_TRIG_TURBO_BOV; }
    else if (TEST("turbo_waste_gate",      16)) { def.source = SoundTriggerDef::SND_TRIG_TURBO_WASTEGATE; }
    else if (TEST("turbo_back_fire",        6)) { def.source = SoundTriggerDef::SND_TRIG_TURBO_BACKFIRE; }
    else if (TEST("always_on",              9)) { def.source = SoundTriggerDef::SND_TRIG_ALWAYS_ON; }
    else if (TEST("repair",                 6)) { def.source = SoundTriggerDef::SND_TRIG_REPAIR; }
    else if (TEST("air",                    3)) { def.source = SoundTriggerDef::SND_TRIG_AIR; }
    else if (TEST("gpws_ap_disconnect",    18)) { def.source = SoundTriggerDef::SND_TRIG_GPWS_APDISCONNECT; }
    else if (TEST("gpws_pull_up",          12)) { def.source = SoundTriggerDef::SND_TRIG_GPWS_PULLUP; }
    else if (TEST("gpws_minimums",         13)) { def.source = SoundTriggerDef::SND_TRIG_GPWS_MINIMUMS; }
    else if (TEST("air_purge",              9)) { def.source = SoundTriggerDef::SND_TRIG_AIR_PURGE; }
    else if (TEST("shift",                  5)) { def.source = SoundTriggerDef::SND_TRIG_SHIFT; }
    else if (TEST("gear_slide",            10)) { def.source = SoundTriggerDef::SND_TRIG_GEARSLIDE; }
    else if (TEST("break",                  5)) { def.source = SoundTriggerDef::SND_TRIG_BREAK; }
    else if (TEST("screetch",               8)) { def.source = SoundTriggerDef::SND_TRIG_SCREETCH; }
    else if (TEST("parking_brake",         13)) { def.source = SoundTriggerDef::SND_TRIG_PARK; }
    else if (TEST("antilock",               8)) { def.source = SoundTriggerDef::SND_TRIG_ALB_ACTIVE; }
    else if (TEST("tractioncontrol",       15)) { def.source = SoundTriggerDef::SND_TRIG_TC_ACTIVE; }
    else if (TEST("aoa_horn",               8)) { def.source = SoundTriggerDef::SND_TRIG_AOA; }
    else if (TEST("ignition",               8)) { def.source = SoundTriggerDef::SND_TRIG_IGNITION; }
    else if (TEST("reverse_gear",          12)) { def.source = SoundTriggerDef::SND_TRIG_REVERSE_GEAR; }
    else if (TEST("turn_signal",           11)) { def.source = SoundTriggerDef::SND_TRIG_TURN_SIGNAL; }
    else if (TEST("turn_signal_tick",      16)) { def.source = SoundTriggerDef::SND_TRIG_TURN_SIGNAL_TICK; }
    else if (TEST("turn_signal_warn_tick", 21)) { def.source = SoundTriggerDef::SND_TRIG_TURN_SIGNAL_WARN_TICK; }
    else if (TEST("main_menu",              9)) { def.source = SoundTriggerDef::SND_TRIG_MAIN_MENU; }

    m_last_def->second.trigger = def;
}

void SoundScriptParser::ParseModulation(SoundModulationDef* def, char* line)
{
    if (TEST("none", 4))
    {
        return;
    }
    else if (TEST("aeroengine", 10))
    {
        if (strncmp(line + 11, "_rpm", 4)==0)
        {
            int param = PARSEINT(line + 10); // Stops at non-number, returns 0 on error;
            if (param < 1 || param > 8)
            {
                char err_buf[200];
                snprintf(err_buf, 200, "ERROR: invalid modulation source '%s'", line);
                this->AddMessage(err_buf);
                return;
            }
            def->source = SoundModulationDef::SND_MOD_AEROENGINE_RPM;
            def->source_param = param;
        }
        else if (strncmp(line + 11, "_throttle", 9)==0)
        {
            int param = PARSEINT(line + 10); // Stops at non-number, returns 0 on error;
            if (param < 1 || param > 8)
            {
                char err_buf[200];
                snprintf(err_buf, 200, "ERROR: invalid modulation source '%s'", line);
                this->AddMessage(err_buf);
                return;
            }
            def->source = SoundModulationDef::SND_MOD_AEROENGINE_THROTTLE;
            def->source_param = param;
        }
    }
    else if (TEST("engine_rpm",             10)) { def->source = SoundModulationDef::SND_MOD_ENGINE; }
    else if (TEST("turbo_rpm",               9)) { def->source = SoundModulationDef::SND_MOD_TURBO; }
    else if (TEST("wheel_speed_kmph",       16)) { def->source = SoundModulationDef::SND_MOD_WHEELSPEED; }
    else if (TEST("injector_ratio",         14)) { def->source = SoundModulationDef::SND_MOD_INJECTOR; }
    else if (TEST("torque_nm",               9)) { def->source = SoundModulationDef::SND_MOD_TORQUE; }
    else if (TEST("gearbox_rpm",            11)) { def->source = SoundModulationDef::SND_MOD_GEARBOX; }
    else if (TEST("break",                   5)) { def->source = SoundModulationDef::SND_MOD_BREAK; }
    else if (TEST("screetch",                8)) { def->source = SoundModulationDef::SND_MOD_SCREETCH; }
    else if (TEST("pump_rpm",                8)) { def->source = SoundModulationDef::SND_MOD_PUMP; }
    else if (TEST("air_speed_knots",        15)) { def->source = SoundModulationDef::SND_MOD_AIRSPEED; }
    else if (TEST("angle_of_attack_degree", 22)) { def->source = SoundModulationDef::SND_MOD_AOA; }
    else if (TEST("linked_command_rate",    19)) { def->source = SoundModulationDef::SND_MOD_LINKED_COMMANDRATE; }
    else if (TEST("music_volume",           12)) { def->source = SoundModulationDef::SND_MOD_MUSIC_VOLUME; }
}
