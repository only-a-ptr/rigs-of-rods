
#pragma once

#ifdef _WIN32

namespace Win32DI // DirectInput wrapper
{
    struct JoyState
    {
        int pos_x;
        int pos_y;
        int pos_z;
        int rot_x;
        int rot_y;
        int rot_z;
    };

    bool        Init(const char* hwnd_str);
    void        Shutdown();
    bool        Update();
    JoyState*   GetJoyState();
};

#endif // _WIN32
