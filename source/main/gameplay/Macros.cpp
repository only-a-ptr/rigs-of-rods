#include "Macros.h"

#include "Application.h"
#include "GUI_GameConsole.h"
#include "Utils.h"

#include <cstring>

static const char* CONSOLE_TOK_DELIM = "\t\n ";

void RoR::MacroManager::ProcessConsole(const char* cmd, const char* arg)
{
    if (cmd == nullptr)
    {
        this->ReplyConsole("Usage: macro [list|record <name>|play <name>|stop]");
        return;
    }

    if (std::strcmp(cmd, "list") == 0)
    {
        this->ReplyConsole("Available macroscripts:");
        auto itor = m_all_macros.begin();
        auto endi = m_all_macros.end();
        for (; itor != endi; ++itor)
        {
            this->ReplyConsole("\t%s", itor->get()->ms_name.c_str());
        }
    }
    else if (std::strcmp(cmd, "record") == 0)
    {
        if (m_state == State::REC_PAUSED)
        {
            m_state = State::RECORDING;
            this->ReplyConsole("Recording '%s' resumed", m_active_macro->ms_name.c_str());
        }
        else if (m_state == State::RECORDING)
        {
            this->ReplyConsole("Already recording '%s'", m_active_macro->ms_name.c_str());
        }
        else if (m_state == State::IDLE)
        {
            if (arg == nullptr)
            {
                this->ReplyConsole("Missing macro name");
            }
            else
            {
                m_all_macros.push_back(std::make_unique<MacroScript>());
                m_active_macro = m_all_macros.back().get();
                m_active_macro->ms_name = arg;
                m_state = State::RECORDING;
                this->ReplyConsole("Recording '%s'", m_active_macro->ms_name.c_str());
                //FUTURE App::GetInputEngine()-> // Record initial event states
            }
        }
        else
        {
            this->ReplyConsole("Cannot start recording now");
        }
    }
    else if (std::strcmp(cmd, "stop") == 0)
    {
        if (m_state == State::RECORDING || m_state == State::REC_PAUSED)
        {
            this->ReplyConsole("Finished recording '%s'", m_active_macro->ms_name.c_str());
            m_state = State::IDLE;
            m_active_macro = nullptr;
        }
        else if (m_state == State::PLAYBACK || m_state == State::PLAY_PAUSED)
        {
            this->ReplyConsole("Stopped playback of '%s'", m_active_macro->ms_name.c_str());
            m_state = State::IDLE;
            m_active_macro = nullptr;
        }
        else
        {
            this->ReplyConsole("Nothing to do");
        }
    }
    else if (std::strcmp(cmd, "play") == 0)
    {
        if (m_state == State::PLAYBACK)
        {
            this->ReplyConsole("Stopped playing '%s'", m_active_macro->ms_name.c_str());
        }
        else if (m_state == State::PLAY_PAUSED)
        {
            this->ReplyConsole("Resumed playing '%s'", m_active_macro->ms_name.c_str());
        }
        else if (m_state == State::IDLE)
        {
            if (arg == nullptr)
            {
                this->ReplyConsole("Missing macro name");
            }
            else
            {
                auto itor = m_all_macros.begin();
                auto endi = m_all_macros.end();
                for (; itor != endi; ++itor)
                {
                    if (itor->get()->ms_name == arg)
                    {
                        m_active_macro = itor->get();
                        m_state = State::PLAYBACK;
                        this->ReplyConsole("Playing back '%s'", arg);
                        m_step_counter = 0;
                        return;
                    }
                }
                this->ReplyConsole("Macro '%s' not found", arg);
            }
        }
        else
        {
            this->ReplyConsole("Cannot start playback now");
        }
    }
}

void RoR::MacroManager::ReplyConsole(const char* format, ...)
{
    char buffer[2000] = {};

    va_list args;
    va_start(args, format);
        vsprintf(buffer, format, args);
    va_end(args);

    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, buffer);
}
