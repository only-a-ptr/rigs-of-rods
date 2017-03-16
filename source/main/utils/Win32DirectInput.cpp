
//-----------------------------------------------------------------------------
// File: Joystick.cpp
//
// Desc: Demonstrates an application which receives immediate 
//       joystick data in exclusive mode via a dialog timer.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

//  Project Rigs of Rods
//  --------------------
//  Code ported from DirectX examples by Microsoft, MIT licensed
//  See https://github.com/walbourn/directx-sdk-samples

#ifdef _WIN32

#include "Win32DirectInput.h"

#include "RoRPrerequisites.h"

#define STRICT
#define DIRECTINPUT_VERSION 0x0800
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <tchar.h>
#include <basetsd.h>

#pragma warning(push)
#pragma warning(disable:6000 28251)
#include <dinput.h>
#pragma warning(pop)

#include <dinputd.h>
#include <assert.h>
#include "resource.h"

#include <vector>
#include <string>

namespace Win32DI
{

BOOL CALLBACK    EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
BOOL CALLBACK    EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );

struct DI_ENUM_CONTEXT
{
    DIJOYCONFIG* pPreferredJoyCfg;
    bool bPreferredJoyCfgValid;
};

struct Controller
{
    DIJOYSTATE2 state;
    LPDIRECTINPUTDEVICE8 device;
    std::string name;
};

#define LOGSTREAM Ogre::LogManager::getSingleton().stream() << "[RoR|Win32DirectInput] "
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

static HWND                    s_hwnd;
static LPDIRECTINPUT8          s_dinput = nullptr;
static std::vector<Controller> s_controllers;
static std::vector<std::string> s_device_list;

bool Init(const char* hwnd_str)
{
    // Get number as 64 bit and then convert. Handles the case of 32 or 64 bit HWND
    // This snippet is from OIS sources
    unsigned __int64 hwnd_u64 = _strtoui64(hwnd_str, 0, 10);
    s_hwnd = (HWND) hwnd_u64;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    // Create a DInput object
    HRESULT hr;
    if( FAILED( hr = DirectInput8Create( GetModuleHandle( nullptr ), DIRECTINPUT_VERSION,
                                         IID_IDirectInput8, ( VOID** )&s_dinput, nullptr ) ) )
    {
        LOGSTREAM << "ERROR: Failed to create DirectInput device; HRESULT: " << hr;
        return false;
    }

    DIJOYCONFIG PreferredJoyCfg = {0};
    DI_ENUM_CONTEXT enumContext;
    enumContext.pPreferredJoyCfg = &PreferredJoyCfg;
    enumContext.bPreferredJoyCfgValid = false;

    IDirectInputJoyConfig8* pJoyConfig = nullptr;
    if( FAILED( hr = s_dinput->QueryInterface( IID_IDirectInputJoyConfig8, ( void** )&pJoyConfig ) ) )
    {
        LOGSTREAM << "ERROR: Failed to create DirectInput device; HRESULT: " << hr;
        return false;
    }

    PreferredJoyCfg.dwSize = sizeof( PreferredJoyCfg );
    if( SUCCEEDED( pJoyConfig->GetConfig( 0, &PreferredJoyCfg, DIJC_GUIDINSTANCE ) ) ) // This function is expected to fail if no joystick is attached
        enumContext.bPreferredJoyCfgValid = true;
    SAFE_RELEASE( pJoyConfig );

    LOGSTREAM << "Enumerating devices from system...";
    if( FAILED( hr = s_dinput->EnumDevices( DI8DEVCLASS_GAMECTRL,
                                         EnumJoysticksCallback,
                                         &enumContext, DIEDFL_ATTACHEDONLY ) ) )
    {
        LOGSTREAM << "[RoR|Win32DirectInput] ERROR: Failed to enumerate devices; HRESULT: " << hr;
        return false;
    }

    LOGSTREAM << "Done; enumerated " << s_controllers.size() << " devices.";

    if( s_controllers.empty() )
    {
        return false;
    }

    for (auto& con: s_controllers)
    {

        // Set the data format to "simple joystick" - a predefined data format 
        //
        // A data format specifies which controls on a device we are interested in,
        // and how they should be reported. This tells DInput that we will be
        // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
        if( FAILED( hr = con.device->SetDataFormat( &c_dfDIJoystick2 ) ) )
        {
            LOGSTREAM << "[RoR|Win32DirectInput] ERROR: Device \"" << con.name <<"\", failed to `SetDataFormat()`; HRESULT: " << hr;
            continue;
        }

        // Set the cooperative level to let DInput know how this device should
        // interact with the system and with other DInput applications.
        if( FAILED( hr = con.device->SetCooperativeLevel( s_hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND ) ) )
        {
            LOGSTREAM << "[RoR|Win32DirectInput] ERROR: Device \"" << con.name <<"\", failed to `SetCooperativeLevel()`; HRESULT: " << hr;
            return false;
        }

        // Enumerate the joystick objects. The callback function enabled user
        // interface elements for objects that are found, and sets the min/max
        // values property for discovered axes.
        if( FAILED( hr = con.device->EnumObjects( EnumObjectsCallback, ( VOID* )&con, DIDFT_ALL ) ) )
        {
            LOGSTREAM << "[RoR|Win32DirectInput] ERROR: Device \"" << con.name <<"\", failed to `EnumObjects()`; HRESULT: " << hr;
            return false;
        }

    }

    return true;
}

void Shutdown()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    for (auto& con: s_controllers)
    {
        con.device->Unacquire();
        SAFE_RELEASE( con.device );
    }
    s_controllers.clear();

    SAFE_RELEASE( s_dinput );
}

JoyState GetState(size_t index)
{
    auto& src = s_controllers.at(index).state;
    JoyState dst;
    dst.pos_x = src.lX;
    dst.pos_y = src.lY;
    dst.pos_z = src.lZ;
    dst.rot_x = src.lRx;
    dst.rot_y = src.lRy;
    dst.rot_z = src.lRz;
    dst.slider_pos[0] = static_cast<int>(src.rglSlider[0]);
    dst.slider_pos[1] = static_cast<int>(src.rglSlider[1]);
    return dst;
}

std::string GetName(size_t index)
{
    return s_controllers.at(index).name;
}

size_t GetNumControllers()
{
    return s_controllers.size();
}

void EnlistDevice(std::string name)
{
    s_device_list.push_back(name);
}

bool IsEnlisted(std::string name)
{
    for (auto& entry: s_device_list)
    {
        if (entry == name)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated joystick. If we find one, create a
//       device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* pContext )
{
    auto pEnumContext = reinterpret_cast<DI_ENUM_CONTEXT*>( pContext );
    HRESULT hr;

    // Filter enlisted
    if (!IsEnlisted(pdidInstance->tszInstanceName))
    {
        LOGSTREAM << ">   Found device \"" << pdidInstance->tszInstanceName << "\"; not enlisted, skipping";
        return DIENUM_CONTINUE;
    }

    // Obtain an interface to the enumerated joystick.
    Controller con;
    hr = s_dinput->CreateDevice( pdidInstance->guidInstance, &con.device, nullptr );

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( FAILED( hr ) )
    {
        LOGSTREAM << ">   Found device \"" << pdidInstance->tszInstanceName << "\"; ailed to create instance, skipping";
        return DIENUM_CONTINUE;
    }

    LOGSTREAM << ">   Found device \"" << pdidInstance->tszInstanceName << "\"; works OK";

    con.name = pdidInstance->tszInstanceName; // Inspired by OIS
    s_controllers.push_back(con);

    return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Name: EnumObjectsCallback()
// Desc: Callback function for enumerating objects (axes, buttons, POVs) on a 
//       joystick. This function enables user interface elements for objects
//       that are found to exist, and scales axes min/max values.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                   VOID* pContext )
{
    Controller* con = static_cast<Controller*>(pContext);

    static int nSliderCount = 0;  // Number of returned slider controls
    static int nPOVCount = 0;     // Number of returned POV controls

    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if( pdidoi->dwType & DIDFT_AXIS )
    {
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof( DIPROPRANGE );
        diprg.diph.dwHeaderSize = sizeof( DIPROPHEADER );
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin = -1000;
        diprg.lMax = +1000;

        // Set the range for the axis
        HRESULT hr = con->device->SetProperty( DIPROP_RANGE, &diprg.diph );
        if( FAILED( hr ) )
        {
            LOGSTREAM << "[RoR|Win32DirectInput] INTERNAL ERROR: `SetProperty()` failed. HRESULT: " << hr;
            return DIENUM_STOP;
        }

    }

    return DIENUM_CONTINUE;
}

bool Update()
{
    for (auto& con: s_controllers)
    {

        // Poll the device to read the current state
        HRESULT hr;
        hr = con.device->Poll();
        if( FAILED( hr ) )
        {
            // DInput is telling us that the input stream has been
            // interrupted. We aren't tracking any state between polls, so
            // we don't have any special reset that needs to be done. We
            // just re-acquire and try again.
            hr = con.device->Acquire();
            while( hr == DIERR_INPUTLOST )
                hr = con.device->Acquire();

            // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
            // may occur when the app is minimized or in the process of 
            // switching, so just try again later 
            continue;
        }

        // Get the input's device state
        con.device->GetDeviceState( sizeof( DIJOYSTATE2 ), &con.state );
    }

    return true;
}

} // namespace Win32DI

#endif // #ifdef _WIN32

