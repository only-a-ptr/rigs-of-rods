
#pragma once

#ifdef _WIN32

#include <string>

namespace Win32DI // DirectInput wrapper
{
    struct JoyState
    {
        // Axis
        int pos_x, pos_y, pos_z;
        int rot_x, rot_y, rot_z;
        // Sliders
        int slider_pos[2];
    };

    bool        Init(const char* hwnd_str);
    void        Shutdown();
    bool        Update();
    void        EnlistDevice(std::string name);
    bool        IsEnlisted(std::string name);
    JoyState    GetState(size_t index);
    std::string GetName(size_t index);
    size_t      GetNumControllers();

    // Force feedback
    bool        IsFFbAvailable();
    bool        CreateFFbConstEffect();
    bool        UpdateFFbConstEffect(int force);
};

#endif // _WIN32
