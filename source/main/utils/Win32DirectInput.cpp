
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

namespace Win32DI
{

BOOL CALLBACK    EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
BOOL CALLBACK    EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );

struct DI_ENUM_CONTEXT
{
    DIJOYCONFIG* pPreferredJoyCfg;
    bool bPreferredJoyCfgValid;
};

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

static HWND                    s_hwnd;
static JoyState                s_joy_state;
static LPDIRECTINPUT8          s_dinput = nullptr;
static LPDIRECTINPUTDEVICE8    s_joy = nullptr;

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
        return false;
    }

    DIJOYCONFIG PreferredJoyCfg = {0};
    DI_ENUM_CONTEXT enumContext;
    enumContext.pPreferredJoyCfg = &PreferredJoyCfg;
    enumContext.bPreferredJoyCfgValid = false;

    IDirectInputJoyConfig8* pJoyConfig = nullptr;
    if( FAILED( hr = s_dinput->QueryInterface( IID_IDirectInputJoyConfig8, ( void** )&pJoyConfig ) ) )
    {
        return false;
    }

    PreferredJoyCfg.dwSize = sizeof( PreferredJoyCfg );
    if( SUCCEEDED( pJoyConfig->GetConfig( 0, &PreferredJoyCfg, DIJC_GUIDINSTANCE ) ) ) // This function is expected to fail if no joystick is attached
        enumContext.bPreferredJoyCfgValid = true;
    SAFE_RELEASE( pJoyConfig );

    // Look for a simple joystick we can use for this sample program.
    if( FAILED( hr = s_dinput->EnumDevices( DI8DEVCLASS_GAMECTRL,
                                         EnumJoysticksCallback,
                                         &enumContext, DIEDFL_ATTACHEDONLY ) ) )
    {
        return false;
    }

    // Make sure we got a joystick
    if( !s_joy )
    {
        return false;
    }

    // Set the data format to "simple joystick" - a predefined data format 
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    if( FAILED( hr = s_joy->SetDataFormat( &c_dfDIJoystick2 ) ) )
        return false;

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    if( FAILED( hr = s_joy->SetCooperativeLevel( s_hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND ) ) )
        return false;

    // Enumerate the joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    if( FAILED( hr = s_joy->EnumObjects( EnumObjectsCallback, ( VOID* )s_hwnd, DIDFT_ALL ) ) )
        return false;

    return true;
}

void Shutdown()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    if( s_joy )
        s_joy->Unacquire();

    // Release any DirectInput objects.
    SAFE_RELEASE( s_joy );
    SAFE_RELEASE( s_dinput );
}

JoyState* GetJoyState()
{
    return &s_joy_state;
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

    // Skip anything other than the perferred joystick device as defined by the control panel.  
    // Instead you could store all the enumerated joysticks and let the user pick.
    if( pEnumContext->bPreferredJoyCfgValid &&
        !IsEqualGUID( pdidInstance->guidInstance, pEnumContext->pPreferredJoyCfg->guidInstance ) )
        return DIENUM_CONTINUE;

    // Obtain an interface to the enumerated joystick.
    hr = s_dinput->CreateDevice( pdidInstance->guidInstance, &s_joy, nullptr );

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( FAILED( hr ) )
        return DIENUM_CONTINUE;

    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
    return DIENUM_STOP;
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
    HWND hDlg = ( HWND )pContext;

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
        if( FAILED( s_joy->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
            return DIENUM_STOP;

    }

    return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Name: UpdateInputState()
// Desc: Get the input device's state and display it.
//-----------------------------------------------------------------------------
bool Update()
{
    HRESULT hr;
    TCHAR strText[512] = {0}; // Device state text
    DIJOYSTATE2 js;           // DInput joystick state 

    if( !s_joy )
        return false;

    // Poll the device to read the current state
    hr = s_joy->Poll();
    if( FAILED( hr ) )
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = s_joy->Acquire();
        while( hr == DIERR_INPUTLOST )
            hr = s_joy->Acquire();

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of 
        // switching, so just try again later 
        return S_OK;
    }

    // Get the input's device state
    if( FAILED( hr = s_joy->GetDeviceState( sizeof( DIJOYSTATE2 ), &js ) ) )
        return false; // The device should have been acquired during the Poll()

    // Display joystick state to dialog

    // Axes
    s_joy_state.pos_x = js.lX;
    s_joy_state.pos_y = js.lY;
    s_joy_state.pos_z = js.lZ;

    s_joy_state.rot_x = js.lRx;
    s_joy_state.rot_y = js.lRy;
    s_joy_state.rot_z = js.lRz;

    return true;
}

} // namespace Win32DI

#endif // #ifdef _WIN32

