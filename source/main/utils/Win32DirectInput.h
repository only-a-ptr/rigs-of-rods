
#pragma once

#ifdef _WIN32

struct Win32DIJoyState // DirectInput joystick state
{
    int pos_x;
    int pos_y;
    int pos_z;
    int rot_x;
    int rot_y;
    int rot_z;
};

class Win32DirectInput
{
public:
    bool Init(const char* hwnd_str);
    void Shutdown();
    bool Update();

    inline Win32DIJoyState GetJoyState() { return m_joy_state; }

private:
    unsigned __int64 m_w_handle;
    Win32DIJoyState  m_joy_state;
};

#endif // _WIN32
