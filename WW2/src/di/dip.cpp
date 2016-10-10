#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "dip.h"
#include "ui/uiMain.h"

#ifdef DIRECT3D
#include "dxvars.h"
#endif	// DIRECT3D

#ifdef OPENGL3D
extern HANDLE			hinst;
extern HWND myhwnd;
#endif	// OPENGL3D

static const SINT32 MAX_JOYSTICKS = 4;
static const SINT32 NUM_JS_RETRIES = 5;

static LPDIRECTINPUT8 lpdi;
static LPDIRECTINPUTDEVICE8	lpkbd;
static LPDIRECTINPUTDEVICE8	lpmd;
static SINT32				numjsd;
static LPDIRECTINPUTDEVICE8	lpjsd[MAX_JOYSTICKS];
static LPGUID				lpjsguid[MAX_JOYSTICKS];
static SINT32				numdidi;
static DIDEVICEINSTANCE		didi[MAX_DIDEVICES];
static HRESULT				retval;

static DIJOYCONFIG joystickconfigs[MAX_JOYSTICKS];
static IDirectInputJoyConfig8* joyconfig8 = nullptr;
static int numjoysticks = 0;


static UINT32 DIDeviceDetails(UINT32 dwDevType);
extern BOOL GetWindowsVersion(DWORD* MajorVersion, DWORD* MinorVersion);

LOGICAL CALLBACK DIEnumDevicesProc(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	if (numdidi >= MAX_DIDEVICES)
	{
		return(DIENUM_STOP);
	}

	didi[numdidi] = *lpddi;
	numdidi += 1;

	return(DIENUM_CONTINUE);
}


LOGICAL dipInit(void)
{
HRESULT				dirval;

	if (lpdi != nullptr)
	{
		Write_Debug("DirectInput device already initialized (0x%x)", lpdi);
		return TRUE;
	}

	// Manage the DirectInput Object
	dirval = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpdi, nullptr);
	if (dirval != DI_OK)
	{
		Write_Debug("Could not create a direct input device");
		return FALSE;
	}

    dirval = lpdi->QueryInterface(IID_IDirectInputJoyConfig8, (void**)&joyconfig8);
    numjoysticks = 0;

    if(dirval == DI_OK)
    {
        int index = 0;
        dirval = -1;

        memset(joystickconfigs, 0, sizeof(DIJOYCONFIG) * MAX_JOYSTICKS);

        for(int i = 0; i < MAX_JOYSTICKS; i++)
        {
			//index = MAX_JOYSTICKS - i - 1; // we reverse it
			index = i;

            joystickconfigs[index].dwSize = sizeof(DIJOYCONFIG);
            dirval = joyconfig8->GetConfig(index, &joystickconfigs[index], DIJC_GUIDINSTANCE);

            if(dirval != DI_OK)
                continue;

            numjoysticks++;
        }

        joyconfig8->Release();
    }

	numdidi = 0;
	dirval = DIError(lpdi->EnumDevices(0, (LPDIENUMDEVICESCALLBACK)DIEnumDevicesProc, nullptr, DIEDFL_ATTACHEDONLY));
	if (dirval != DI_OK)
	{
		Write_Debug("Enumeration of direct input devices failed");
		return FALSE;
	}

	Write_Debug("Direct Input Devices");
	for(int i = 0; i < numdidi; i++)
	{
		Write_Debug("%d - Name: %s, Product: %s", i, uiUTF16toUTF8(didi[i].tszInstanceName), uiUTF16toUTF8(didi[i].tszProductName));
		DIDeviceDetails(didi[i].dwDevType);
	}

	numjsd = 0;

	return TRUE;
}


// Always unacquire the device before calling Release().
void dipKillKeyboard(void)
{
	if (lpkbd != nullptr)
	{
		lpkbd->Unacquire();
		lpkbd->Release();
		lpkbd = nullptr;
	}
}


// Always unacquire the device before calling Release().
void dipKillMouse(void)
{
	if (lpmd != nullptr)
	{
		lpmd->Unacquire();
		lpmd->Release();
		lpmd = nullptr;
	}
}


static void KillJoystick(SINT32 number)
{
	if (lpjsd[number] != nullptr)
	{
		lpjsd[number]->Unacquire();
		lpjsd[number]->Release();
		lpjsd[number] = nullptr;
	}
}


// Always unacquire the device before calling Release().
void dipKillJoystick(void)
{
SINT32		i;

	for (i=0; i < numjsd; i++)
	{
		KillJoystick(i);
	}
}


// Always unacquire the device before calling Release().
void dipKill(void)
{
	if (lpdi != nullptr)
	{
		dipKillKeyboard();
		dipKillMouse();
		dipKillJoystick();

		lpdi->Release();
		lpdi = nullptr;
	}

    Write_Log(LOG_OLD, LOG_INFO, "[dipKill]");
}


// At certain times the devices might need to be unacquired (but not "killed")
void dipUnacquireKeyboard(void)
{
HRESULT	result;

	if (lpkbd != nullptr)
	{
		result = DIError(lpkbd->Unacquire());
		if (result != DI_OK && result != DI_NOEFFECT)
		{
			Write_Debug("Unacquire for keyboard failed");
		}
	}
}


// At certain times the devices might need to be unacquired (but not "killed")
void dipUnacquireMouse(void)
{
HRESULT	result;

	if (lpmd != nullptr)
	{
		result = DIError(lpmd->Unacquire());
		if (result != DI_OK && result != DI_NOEFFECT)
		{
			Write_Debug("Unacquire for mouse failed");
		}
	}
}


// At certain times the devices might need to be unacquired (but not "killed")
void dipUnacquireJoystick(void)
{
HRESULT	result;
SINT32		i;

	for (i=0; i < numjsd; i++)
	{
		if (lpjsd[i] != nullptr)
		{
			result = DIError(lpjsd[i]->Unacquire());
			if (result != DI_OK && result != DI_NOEFFECT)
			{
				Write_Debug("Unacquire for joystick failed");
			}
		}
	}
}


// At certain times the devices might need to be unacquired (but not "killed")
void dipUnacquire(void)
{
	if (lpdi != nullptr)
	{
		dipUnacquireKeyboard();
		dipUnacquireMouse();
		dipUnacquireJoystick();
	}
}


// At certain times, the devices might need to be acquired
LOGICAL dipAcquireKeyboard(void)
{
LOGICAL	retval=TRUE;
HRESULT	result;

	if (lpkbd != nullptr)
	{
		result = DIError(lpkbd->Acquire());
		if (result != DI_OK && result != S_FALSE)
		{
			retval = FALSE;
			Write_Debug("Acquire for keyboard failed");
		}
	}

	return retval;
}


// At certain times, the devices might need to be acquired
LOGICAL dipAcquireMouse(void)
{
LOGICAL	retval=TRUE;
HRESULT	result;

	if (lpmd != nullptr)
	{
		result = DIError(lpmd->Acquire());
		if (result != DI_OK && result != S_FALSE)
		{
			retval = FALSE;
			Write_Debug("Acquire for mouse failed");
		}
	}

	return retval;
}


// At certain times, the devices might need to be acquired
LOGICAL dipAcquireJoystick(void)
{
LOGICAL	retval=TRUE;
HRESULT	result;
SINT32		i;

	for (i=0; i < numjsd; i++)
	{
		if (lpjsd[i] != nullptr)
		{
			result = DIError(lpjsd[i]->Acquire());
			if (result != DI_OK && result != S_FALSE)
			{
				retval = FALSE;
				Write_Debug("Acquire for joystick failed");
			}
		}
	}

	return retval;
}


// At certain times, the devices might need to be acquired
LOGICAL dipAcquire(void)
{
	if (lpdi != nullptr)
	{
		if (!dipAcquireKeyboard() ||
			!dipAcquireMouse() ||
			!dipAcquireJoystick())
		{
			return FALSE;
		}
	}

	return TRUE;
}


LOGICAL dipSetupKeyboard(void)
{
	HRESULT dirval;
	DIPROPDWORD dipdw;
	LPDIRECTINPUTDEVICE8 lpd;

	// Manage the keyboard object
	dirval = DIError(lpdi->CreateDevice(GUID_SysKeyboard, &lpd, nullptr));
	if (dirval != DI_OK)
	{
		Write_Debug("CreateDevice for keyboard failed");
		return FALSE;
	}

	dirval = DIError(lpd->QueryInterface(IID_IDirectInputDevice8, (void**)&lpkbd));
	lpd->Release();
	if (dirval != DI_OK)
	{
		Write_Debug("QueryInterface for keyboard failed");
		return FALSE;
	}

	// Set the data format
	dirval = DIError(lpkbd->SetDataFormat(&c_dfDIKeyboard));
 	if (dirval != DI_OK)
	{
		Write_Debug("SetDataFormat for keyboard failed");
		return FALSE;
	}

	// Set the cooperative level
//#undef RELEASE_CLIENT
//#define RELEASE_CLIENT 1
#if RELEASE_CLIENT != 0
	dirval = DIError(lpkbd->SetCooperativeLevel(myhwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY));
#else
	dirval = DIError(lpkbd->SetCooperativeLevel(myhwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY));
#endif
 	if (dirval != DI_OK)
	{
		Write_Debug("SetCooperativeLevel for keyboard failed");
		return FALSE;
	}

	// Set the keyboard properties
	dipdw.diph.dwSize		= sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
	dipdw.diph.dwObj		= 0;
	dipdw.diph.dwHow		= DIPH_DEVICE;
	dipdw.dwData			= 256;

	dirval = DIError(lpkbd->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph));
	if (dirval != DI_OK)
	{
		Write_Debug("SetProperty for keyboard failed");
		return FALSE;
	}

	return TRUE;
}


LOGICAL dipSetupMouse(void)
{
	HRESULT dirval;
	DIPROPDWORD dipdw;
	LPDIRECTINPUTDEVICE8 lpd;

	// Manage the mouse object
	dirval = DIError(lpdi->CreateDevice(GUID_SysMouse, &lpd, nullptr));
	if (dirval != DI_OK)
	{
		Write_Debug("CreateDevice for mouse failed");
		return FALSE;
	}

	dirval = DIError(lpd->QueryInterface(IID_IDirectInputDevice8, (void**)&lpmd));
	lpd->Release();
	if (dirval != DI_OK)
	{
		Write_Debug("QueryInterface for mouse failed");
		return FALSE;
	}

	// Set the data format
	dirval = DIError(lpmd->SetDataFormat(&c_dfDIMouse));
 	if (dirval != DI_OK)
	{
		Write_Debug("SetDataFormat for mouse failed");
		return FALSE;
	}

	// Set the cooperative level
	DWORD MajorVersion, MinorVersion;
	GetWindowsVersion(&MajorVersion, &MinorVersion);

    if(MajorVersion >= 5 && MinorVersion > 0)
		dirval = DIError(lpmd->SetCooperativeLevel(myhwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));
	else
		dirval = DIError(lpmd->SetCooperativeLevel(myhwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));

 	if (dirval != DI_OK)
	{
		Write_Debug("SetCooperativeLevel for mouse failed");
		return FALSE;
	}

	// Set the mouse properties
	dipdw.diph.dwSize		= sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
	dipdw.diph.dwObj		= 0;
	dipdw.diph.dwHow		= DIPH_DEVICE;
	dipdw.dwData			= dipBUFFERSIZE;

	dirval = DIError(lpmd->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph));
	if (dirval != DI_OK)
	{
		Write_Debug("SetProperty (buffer size) for mouse failed");
		return FALSE;
	}

	// Set the mouse properties
	dipdw.dwData			= DIPROPAXISMODE_REL;

	dirval = DIError(lpmd->SetProperty(DIPROP_AXISMODE, &dipdw.diph));
	if (dirval != DI_OK)
	{
		Write_Debug("SetProperty (axis mode) for mouse failed");
		return FALSE;
	}

	return TRUE;
}


static bool SetupJoystick(SINT32 number, LPGUID lpguid)
{
	HRESULT					dirval;
	DIPROPDWORD				dipdw;
	DIPROPRANGE				diprg;
	DIPROPDWORD				didwd;
	LPDIRECTINPUTDEVICE8	lpd;
	DIDEVCAPS				didevcaps;
	LPDIRECTINPUTDEVICE8	lpjs;

	dirval = DIError(lpdi->CreateDevice(*lpguid, &lpd, nullptr));	// &GUID_Joystick
	if (dirval != DI_OK)
	{
		Write_Debug("CreateDevice for joystick failed");
		return false;
	}

	dirval = DIError(lpd->QueryInterface(IID_IDirectInputDevice8, (void**)&lpjs));
	lpd->Release();
	if (dirval != DI_OK)
	{
		Write_Debug("QueryInterface for joystick failed");
		return false;
	}

	// Set the data format
	dirval = DIError(lpjs->SetDataFormat(&c_dfDIJoystick));
 	if (dirval != DI_OK)
	{
		Write_Debug("SetDataFormat for joystick failed");
		return false;
	}

	// Set the cooperative level
	dirval = DIError(lpjs->SetCooperativeLevel(myhwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));
 	if (dirval != DI_OK)
	{
		Write_Debug("SetCooperativeLevel for joystick failed");
		return false;
	}

	memset(&didevcaps, 0, sizeof(DIDEVCAPS));
	didevcaps.dwSize = sizeof(DIDEVCAPS);
	dirval = DIError(lpjs->GetCapabilities(&didevcaps));
 	if (dirval != DI_OK)
	{
		Write_Debug("GetCapabilities for joystick failed");
		return false;
	}

	// Set the joystick properties
	dipdw.diph.dwSize		= sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
	dipdw.diph.dwObj		= 0;
	dipdw.diph.dwHow		= DIPH_DEVICE;
	dipdw.dwData			= 256;

	dirval = DIError(lpjs->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (buffer size) for joystick failed");
#ifndef OPENGL3D
		return false;
#endif
	}

	// set X range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_X;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (x range) for joystick failed");
		return false;
	}

	// set X axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_X;
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (x deadzone) for joystick failed");
		return false;
	}

	// set Y range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_Y;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (y range) for joystick failed");
		return false;
	}

	// set Y axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_Y;
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (y deadzone) for joystick failed");
		return false;
	}

	// set Z range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_Z;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (z range) for joystick failed");
		return false;
	}

	// set Z axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_Z;
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (z deadzone) for joystick failed");
		return false;
	}

	// set RX range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_RX;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (rx range) for joystick failed");
		return false;
	}

	// set RX axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_RX;
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (rx deadzone) for joystick failed");
		return false;
	}

	// set RY range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_RY;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (ry range) for joystick failed");
		return false;
	}

	// set RY axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_RY;
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (ry deadzone) for joystick failed");
		return false;
	}

	// set RZ range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_RZ;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (rz range) for joystick failed");
		return false;
	}

	// set RZ axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_RZ;
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (rz deadzone) for joystick failed");
		return false;
	}

	// set Slider(0) range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_SLIDER(0);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = 0;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (slider 0 range) for joystick failed");
		return false;
	}

	// set Slider(0) axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_SLIDER(0);
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (slider 0 deadzone) for joystick failed");
		return false;
	}

	// set Slider(1) range
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_SLIDER(1);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = 0;
	diprg.lMax              = +1000;

	dirval = DIError(lpjs->SetProperty(DIPROP_RANGE, &diprg.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (slider 1 range) for joystick failed");
		return false;
	}

	// set Slider(1) axis dead zone to 0% (handle at application layer)
	didwd.diph.dwSize       = sizeof(didwd);
	didwd.diph.dwHeaderSize = sizeof(didwd.diph);
	didwd.diph.dwObj        = DIJOFS_SLIDER(1);
	didwd.diph.dwHow        = DIPH_BYOFFSET;
	didwd.dwData			= 0;

	dirval = DIError(lpjs->SetProperty(DIPROP_DEADZONE, &didwd.diph));
 	if (dirval != DI_OK && dirval != DIERR_NOTFOUND)
	{
		Write_Debug("SetProperty (slider 1 deadzone) for joystick failed");
		return false;
	}

	lpjsd[number] = lpjs;
	lpjsguid[number] = lpguid;

	return true;
}


LOGICAL dipSetupJoystick(void)
{
#if(1)
    numjsd = 0;;

    for(int i = 0; i < numjoysticks; i++)
    {
        if(!SetupJoystick(i, &joystickconfigs[i].guidInstance))
        {
            Write_Debug("SetupJoystick failed for %s", uiUTF16toUTF8(joystickconfigs[i].wszType));
            continue;
        }

        for(int q = 0; q < numdidi; q++)
        {
            if(IsEqualGUID(didi[q].guidInstance, joystickconfigs[i].guidInstance))
            {
		        Write_Debug("Setup to use joystick %s", uiUTF16toUTF8(didi[q].tszInstanceName));
                break;
            }
        }

		numjsd++;
    }
#else
SINT32				number;
SINT32				i;

	number = 0;
	for (i=0; i < numdidi; i++)
	{
		if ((UINT8)didi[i].dwDevType == DI8DEVTYPE_JOYSTICK)
		{
			if (!SetupJoystick(number, &didi[i].guidInstance))
			{
				Write_Debug("SetupJoystick failed for %s", didi[i].tszInstanceName);
				continue;
			}

			Write_Debug("Setup to use joystick %s", didi[i].tszInstanceName);
			number += 1;
			if (number >= MAX_JOYSTICKS)
			{
				break;
			}
		}
	}

	numjsd = number;
#endif

	return TRUE;
}


LOGICAL dipGetKeyboardData(DIDEVICEOBJECTDATA* od, UINT32* dwElements)
{
HRESULT				dirval;

	if (lpkbd == nullptr)
	{
		return FALSE;
	}

	dirval = lpkbd->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), od, (LPDWORD)dwElements, 0);
	if (dirval != DI_OK)
	{
		if (dirval == DIERR_INPUTLOST || dirval == DIERR_NOTACQUIRED)
		{
			lpkbd->Acquire();
		}
		else
		if (dirval == DI_BUFFEROVERFLOW)
		{
			DIError(dirval);
			Write_Debug("dipGetKeyboardData is continuing");
			return TRUE;
		}
		else
		{
			DIError(dirval);
			Write_Debug("GetDeviceData for keyboard failed");
		}

		return FALSE;
	}

	return TRUE;
}


LOGICAL dipGetKeyboardState(int bufsize, LPVOID buffer)
{
HRESULT				dirval;

	if (lpkbd == nullptr)
	{
		return FALSE;
	}

	dirval = lpkbd->GetDeviceState(bufsize, buffer);
	if (dirval != DI_OK)
	{
		if (dirval == DIERR_INPUTLOST || dirval == DIERR_NOTACQUIRED)
		{
			lpkbd->Acquire();
		}
		else
		{
			DIError(dirval);
			Write_Debug("GetDeviceState for keyboard failed");
		}

		return FALSE;
	}

	return TRUE;
}


LOGICAL dipGetMouseData(DIDEVICEOBJECTDATA* od, UINT32* dwElements)
{
HRESULT				dirval;

	if (lpmd == nullptr)
	{
		return FALSE;
	}

	dirval = lpmd->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), od, (LPDWORD)dwElements, 0);
	if (dirval != DI_OK)
	{
		if (dirval == DIERR_INPUTLOST || dirval == DIERR_NOTACQUIRED)
		{
			lpmd->Acquire();
		}
		else
		if (dirval == DI_BUFFEROVERFLOW)
		{
			DIError(dirval);
			Write_Debug("dipGetMouseData is continuing");
			return TRUE;
		}
		else
		{
			DIError(dirval);
			Write_Debug("GetDeviceData for mouse failed");
		}

		return FALSE;
	}

	return TRUE;
}


LOGICAL dipGetMouseState(DIMOUSESTATE* ms)
{
HRESULT				dirval;

	if (lpmd == nullptr)
	{
		return FALSE;
	}

	dirval = lpmd->GetDeviceState(sizeof(DIMOUSESTATE), ms);
	if (dirval != DI_OK)
	{
		if (dirval == DIERR_INPUTLOST || dirval == DIERR_NOTACQUIRED)
		{
			lpmd->Acquire();
		}
		else
		{
			DIError(dirval);
			Write_Debug("GetDeviceState for mouse failed");
		}

		return FALSE;
	}

	return TRUE;
}


static LOGICAL GetJoystickData(SINT32 number, DIDEVICEOBJECTDATA* od, UINT32* dwElements)
{
HRESULT				dirval;
static SINT32		retry[MAX_JOYSTICKS];

	if (lpjsd[number] == nullptr)
	{
		// Incase a joystick disappears during play, fake return of no data
		*dwElements = 0;
		return TRUE;
	}

	dirval = lpjsd[number]->Poll();
	if (dirval != DI_OK)
	{
		if (dirval == DIERR_NOTACQUIRED || dirval == DIERR_INPUTLOST)
		{
			lpjsd[number]->Acquire();
			return FALSE;
		}
		else
		if (dirval == DIERR_UNPLUGGED)
		{
			DIError(dirval);
			retry[number] += 1;
			if (retry[number] == NUM_JS_RETRIES)
			{
				Write_Debug("Poll for joystick %d failed; Kill/Setup attempted", number);
				KillJoystick(number);
				SetupJoystick(number, lpjsguid[number]);
			}
			else
			if (retry[number] > NUM_JS_RETRIES)
			{
				Write_Debug("Poll for joystick %d failed; joystick disabled", number);
				KillJoystick(number);
			}
			else
			{
				Write_Debug("Poll for joystick failed; attempting unacquire/acquire");
				lpjsd[number]->Unacquire();
			}

			return FALSE;
		}
		else
		if (dirval != DI_BUFFEROVERFLOW)
		{
			DIError(dirval);
			Write_Debug("Poll for joystick state failed");
			return FALSE;
		}
		else
		if (DIError(dirval) != DI_OK)
		{
			return FALSE;
		}
	}

	retry[number] = 0;

	dirval = DIError(lpjsd[number]->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), od, (LPDWORD)dwElements, 0));
	if (dirval != DI_OK && dirval != DI_BUFFEROVERFLOW)
	{
		Write_Debug("GetDeviceData for joystick failed");
		return FALSE;
	}

	return TRUE;
}


LOGICAL dipGetJoystickData(DIDEVICEOBJECTDATA* od, UINT32* dwElements)
{
SINT32		i;
UINT32		numelements, numused, num, count;

	numelements = *dwElements;
	numused = 0;
	for (i=0; i < numjsd; i++)
	{
		num = numelements - numused;
		if (!GetJoystickData(i, &od[numused], &num))
		{
			return FALSE;
		}

		for (count=0; count < num; count++, numused++)
		{
			od[numused].dwData = MAKELONG((UINT16)od[numused].dwData, i);
		}
	}

	*dwElements = numused;

	return TRUE;
}


LOGICAL dipGetJoystickState(SINT32 number, DIJOYSTATE* js)
{
HRESULT				dirval;
static SINT32		retry[MAX_JOYSTICKS];

	if (lpjsd[number] == nullptr)
	{
		return FALSE;
	}

	dirval = lpjsd[number]->Poll();
	if (dirval != DI_OK)
	{
		if (dirval == DIERR_NOTACQUIRED || dirval == DIERR_INPUTLOST)
		{
			lpjsd[number]->Acquire();
			return FALSE;
		}
		else
		if (dirval == DIERR_UNPLUGGED)
		{
			DIError(dirval);
			retry[number] += 1;
			if (retry[number] == NUM_JS_RETRIES)
			{
				Write_Debug("Poll for joystick %d failed; Kill/Setup attempted", number);
				KillJoystick(number);
				SetupJoystick(number, lpjsguid[number]);
			}
			else
			if (retry[number] > NUM_JS_RETRIES)
			{
				Write_Debug("Poll for joystick %d failed; joystick disabled", number);
				KillJoystick(number);
			}
			else
			{
				Write_Debug("Poll for joystick failed; attempting unacquire/acquire");
				lpjsd[number]->Unacquire();
			}

			return FALSE;
		}
		else
		if (dirval != DI_BUFFEROVERFLOW)
		{
			DIError(dirval);
			Write_Debug("Poll for joystick state failed");
			return FALSE;
		}
		else
		if (DIError(dirval) != DI_OK)
		{
			return FALSE;
		}
	}

	retry[number] = 0;

	dirval = DIError(lpjsd[number]->GetDeviceState(sizeof(DIJOYSTATE), js));
	if (dirval != DI_OK && dirval != DI_BUFFEROVERFLOW)
	{
		Write_Debug("GetDeviceState for joystick failed");
		return FALSE;
	}

	return TRUE;
}


SINT32 dipGetNumberJoysticks(void)
{
	return(numjsd);
}

LOGICAL dipHasKeyboard()
{
	return (lpkbd) ? 1: 0;
}

LOGICAL dipHasMouse()
{
	return (lpmd) ? 1: 0;
}

LOGICAL dipHasJoystick()
{
	return dipGetNumberJoysticks();
}

static UINT32 DIDeviceDetails(UINT32 dwDevType)
{
SINT8*			ptxt;

    switch (dwDevType & 0xff)
	{
    case DI8DEVTYPE_MOUSE:
    {
		ptxt = "A mouse or mouse-like device (such as a trackball). ";
		Write_Debug("%s (%x)", ptxt, dwDevType & 0xff);

		// Mouse subtypes
		switch ((dwDevType >> 8) & 0xff)
		{
		case DI8DEVTYPEMOUSE_UNKNOWN:
		{
			ptxt = "The subtype could not be determined. ";
			break;
		}
		case DI8DEVTYPEMOUSE_TRADITIONAL:
		{
			ptxt = "The device is a traditional mouse. ";
			break;
		}
		case DI8DEVTYPEMOUSE_FINGERSTICK:
		{
			ptxt = "The device is a fingerstick. ";
			break;
		}
		case DI8DEVTYPEMOUSE_TOUCHPAD:
		{
			ptxt = "The device is a touchpad. ";
			break;
		}
		case DI8DEVTYPEMOUSE_TRACKBALL:
		{
			ptxt = "The device is a trackball. ";
			break;
		}
		default:
		{
			ptxt = "Unknown mouse subtype";
			break;
		}
		}

		Write_Debug("%s (%x)", ptxt, (dwDevType >> 8) & 0xff);
		break;
    }
    case DI8DEVTYPE_KEYBOARD:
    {
		ptxt = "A keyboard or keyboard-like device. ";
		Write_Debug("%s (%x)", ptxt, dwDevType & 0xff);

		// Keyboard subtypes
		switch ((dwDevType >> 8) & 0xff)
		{
		case DI8DEVTYPEKEYBOARD_UNKNOWN:
		{
			ptxt = "The subtype could not be determined. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_PCXT:
		{
			ptxt = "IBM PC/XT 83-key keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_OLIVETTI:
		{
			ptxt = "Olivetti 102-key keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_PCAT:
		{
			ptxt = "IBM PC/AT 84-key keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_PCENH:
		{
			ptxt = "IBM PC Enhanced 101/102-key or Microsoft Natural?keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_NOKIA1050:
		{
			ptxt = "Nokia 1050 keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_NOKIA9140:
		{
			ptxt = "Nokia 9140 keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_NEC98:
		{
			ptxt = "Japanese NEC PC98 keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_NEC98LAPTOP:
		{
			ptxt = "Japanese NEC PC98 laptop keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_NEC98106:
		{
			ptxt = "Japanese NEC PC98 106-key keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_JAPAN106:
		{
			ptxt = "Japanese 106-key keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_JAPANAX:
		{
			ptxt = "Japanese AX keyboard. ";
			break;
		}
		case DI8DEVTYPEKEYBOARD_J3100:
		{
			ptxt = "Japanese J3100 keyboard. ";
			break;
		}
		default:
		{
			ptxt = "Unknown keyboard subtype";
			break;
		}
		}

		Write_Debug("%s (%x)", ptxt, (dwDevType >> 8) & 0xff);
		break;
    }
#if(0)
    case DI8DEVTYPE_JOYSTICK:
    {
		ptxt = "A joystick or similar device, such as a steering wheel. ";
		Write_Debug("%s (%x)", ptxt, dwDevType & 0xff);

#if(0)
		// Joystick Subtypes
		switch ((dwDevType >> 8) & 0xff)
		{
		case DIDEVTYPEJOYSTICK_UNKNOWN:
		{
			ptxt = "The subtype could not be determined. ";
			break;
		}
		case DIDEVTYPEJOYSTICK_TRADITIONAL:
		{
			ptxt = "A traditional joystick. ";
			break;
		}
		case DIDEVTYPEJOYSTICK_FLIGHTSTICK:
		{
			ptxt = "A joystick optimized for flight simulation. ";
			break;
		}
		case DIDEVTYPEJOYSTICK_GAMEPAD:
		{
			ptxt = "A device whose primary purpose is to provide button input. ";
			break;
		}
		case DIDEVTYPEJOYSTICK_RUDDER:
		{
			ptxt = "A device for yaw control. ";
			break;
		}
		case DIDEVTYPEJOYSTICK_WHEEL:
		{
			ptxt = "A steering wheel. ";
			break;
		}
		case DIDEVTYPEJOYSTICK_HEADTRACKER:
		{
			ptxt = "A device that tracks the movement of the user's head. ";
			break;
		}
		default:
		{
			ptxt = "Unknown joystick subtype";
			break;
		}
		}
#endif

		Write_Debug("%s (%x)", ptxt, (dwDevType >> 8) & 0xff);
		break;
    }
#endif
    case DI8DEVTYPE_DEVICE:
    {
		ptxt = "A device that does not fall into the previous categories. ";
		Write_Debug("%s (%x)", ptxt, dwDevType & 0xff);
		break;
    }
	default:
	{
		ptxt = "Unknown device type";
		Write_Debug("%s (%x)", ptxt, dwDevType & 0xff);
		break;
	}
    }

	// Flags in the High Word
    if (dwDevType & DIDEVTYPE_HID)
    {
		ptxt = "The device uses the Human Interface Device (HID) protocol. ";
		Write_Debug("%s (%x)", ptxt, dwDevType);
    }

    for(int i = 0; i < numjoysticks; i++)
    {
        for(int q = 0; q < numdidi; q++)
        {
            if(IsEqualGUID(didi[q].guidInstance, joystickconfigs[i].guidInstance))
            {
                Write_Log(LOG_OLD, LOG_INFO, "Joystick found: %s", uiUTF16toUTF8(didi[q].tszInstanceName));
                break;
            }
        }
    }

	return(dwDevType);
}


HRESULT DIErrorSummary(HRESULT code)
{
#ifdef EVENT_LOG
SINT8*			ptxt;
#endif

    if (SUCCEEDED(code))
	{
		retval = DI_OK;
	}
	else
	{
		retval = code;
	}

#ifdef EVENT_LOG

    switch (retval)
	{
    case DI_OK:
    {
		return retval;
    }
    case DI_BUFFEROVERFLOW:
    {
		ptxt = "The device buffer overflowed and some input was lost. This value is equal to the S_FALSE standard COM return value. ";
		break;
    }
    case DI_DOWNLOADSKIPPED:
    {
		ptxt = "The parameters of the effect were successfully updated, but the effect could not be downloaded because the associated device was not acquired in exclusive mode. ";
		break;
    }
    case DI_EFFECTRESTARTED:
    {
		ptxt = "The effect was stopped, the parameters were updated, and the effect was restarted. ";
		break;
    }
    case DI_POLLEDDEVICE:
    {
		ptxt = "The device is a polled device. As a result, device buffering will not collect any data and event notifications will not be signaled until the IDirectInputDevice2::Poll method is called. ";
		break;
    }
    case DI_TRUNCATED:
    {
		ptxt = "The parameters of the effect were successfully updated, but some of them were beyond the capabilities of the device and were truncated to the nearest supported value. ";
		break;
    }
    case DI_TRUNCATEDANDRESTARTED:
    {
		ptxt = "Equal to DI_EFFECTRESTARTED | DI_TRUNCATED. ";
		break;
    }
    case DIERR_ACQUIRED:
    {
		ptxt = "The operation cannot be performed while the device is acquired. ";
		break;
    }
    case DIERR_ALREADYINITIALIZED:
    {
		ptxt = "This object is already initialized ";
		break;
    }
    case DIERR_BADDRIVERVER:
    {
		ptxt = "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components. ";
		break;
    }
    case DIERR_BETADIRECTINPUTVERSION:
    {
		ptxt = "The application was written for an unsupported prerelease version of DirectInput. ";
		break;
    }
    case DIERR_DEVICEFULL:
    {
		ptxt = "The device is full. ";
		break;
    }
    case DIERR_DEVICENOTREG:
    {
		ptxt = "The device or device instance is not registered with DirectInput. This value is equal to the REGDB_E_CLASSNOTREG standard COM return value. ";
		break;
    }
    case DIERR_EFFECTPLAYING:
    {
		ptxt = "The parameters were updated in memory but were not downloaded to the device because the device does not support updating an effect while it is still playing. ";
		break;
    }
    case DIERR_HASEFFECTS:
    {
		ptxt = "The device cannot be reinitialized because there are still effects attached to it. ";
		break;
    }
    case DIERR_GENERIC:
    {
		ptxt = "An undetermined error occurred inside the DirectInput subsystem. This value is equal to the E_FAIL standard COM return value. ";
		break;
    }
    case DIERR_HANDLEEXISTS:
    {
		ptxt = "The device already has an event notification associated with it. This value is equal to the E_ACCESSDENIED standard COM return value. ";
		break;
    }
    case DIERR_INCOMPLETEEFFECT:
    {
		ptxt = "The effect could not be downloaded because essential information is missing. For example, no axes have been associated with the effect, or no type-specific information has been supplied. ";
		break;
    }
    case DIERR_INPUTLOST:
    {
		ptxt = "Access to the input device has been lost. It must be reacquired. ";
		break;
    }
    case DIERR_INVALIDPARAM:
    {
		ptxt = "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called. This value is equal to the E_INVALIDARG standard COM return value. ";
		break;
    }
    case DIERR_MOREDATA:
    {
		ptxt = "Not all the requested information fitted into the buffer. ";
		break;
    }
    case DIERR_NOAGGREGATION:
    {
		ptxt = "This object does not support aggregation. ";
		break;
    }
    case DIERR_NOINTERFACE:
    {
		ptxt = "The specified interface is not supported by the object. This value is equal to the E_NOINTERFACE standard COM return value. ";
		break;
    }
    case DIERR_NOTACQUIRED:
    {
		ptxt = "The operation cannot be performed unless the device is acquired. ";
		break;
    }
    case DIERR_NOTBUFFERED:
    {
		ptxt = "The device is not buffered. Set the DIPROP_BUFFERSIZE property to enable buffering. ";
		break;
    }
    case DIERR_NOTDOWNLOADED:
    {
		ptxt = "The effect is not downloaded. ";
		break;
    }
    case DIERR_NOTEXCLUSIVEACQUIRED:
    {
		ptxt = "The operation cannot be performed unless the device is acquired in DISCL_EXCLUSIVE mode. ";
		break;
    }
    case DIERR_NOTFOUND:
    {
		ptxt = "The requested object does not exist. ";
		break;
    }
    case DIERR_NOTINITIALIZED:
    {
		ptxt = "This object has not been initialized. ";
		break;
    }
    case DIERR_OLDDIRECTINPUTVERSION:
    {
		ptxt = "The application requires a newer version of DirectInput. ";
		break;
    }
    case DIERR_OUTOFMEMORY:
    {
		ptxt = "The DirectInput subsystem couldn't allocate sufficient memory to complete the call. This value is equal to the E_OUTOFMEMORY standard COM return value. ";
		break;
    }
    case DIERR_UNSUPPORTED:
    {
		ptxt = "The function called is not supported at this time. This value is equal to the E_NOTIMPL standard COM return value. ";
		break;
    }
    case E_PENDING:
    {
		ptxt = "Data is not yet available. ";
		break;
    }
    case DIERR_UNPLUGGED:
    {
		ptxt = "The operation could not be completed because the device is not plugged in. ";
		break;
    }
	default:
	{
		ptxt = "Unknown error in DIP.";
		break;
	}
	}

    Write_Debug("%s (%x)", ptxt, retval);

#endif

    return retval;
}
