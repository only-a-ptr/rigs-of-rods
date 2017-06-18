
    Copyright 2013-2017 Petr Ohlidal & contributors
#pragma once
// TODO: dummy ----- to be reimplemented

#include "Application.h"

#include <future>
#include <memory>
#include <thread>
#include <vector>
namespace RoR{
namespace GUI {
struct MpServerlistData; // Forward declaration, private implementation.
class MultiplayerSelector
{
public:

    ~MultiplayerSelector();

    void         SetVisible(bool v);
    inline bool  IsVisible()                           { return m_is_visible; }
    void         RefreshServerlist();                  /// Launch refresh from main thread
    bool         IsRefreshThreadRunning() const;       /// Check status from main thread
    void         CheckAndProcessRefreshResult();       /// To be invoked periodically from main thread if refresh is in progress.
    void         Draw();

    enum class Mode { ONLINE, DIRECT, SETUP };

    std::future<MpServerlistData*> m_serverlist_future;
    std::unique_ptr<MpServerlistData> m_serverlist_data;
    int                            m_selected_item;
    Mode                           m_mode;
    bool                           m_is_refreshing;
    char                           m_window_title[100];
    bool                           m_is_visible;
    Str<200>                       m_user_token_buf;
    Str<100>                       m_player_name_buf;
    Str<100>                       m_password_buf;
    Str<200>                       m_server_host_buf;
};

} // namespace GUI
} // namespace RoR
