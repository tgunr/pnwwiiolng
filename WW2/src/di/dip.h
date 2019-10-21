#pragma once

	#undef DIRECTINPUT_VERSION
	#define DIRECTINPUT_VERSION         0x0800
	#include <dinput.h>
    #include <dinputd.h>

	#define dipBUFFERSIZE				64
	#define dipKEYDOWN(name,key)		(name[key] & 0x80)
	#define MAX_DIDEVICES				32

enum {

	MAX_KEYS   = 256  // not really, but other stuff breaks because it hardcodes this value
};

	LOGICAL dipInit();

	LOGICAL dipInitJoysticksOnly();

	void dipKillKeyboard();
	void dipKillMouse();
	void dipKillJoystick();
	void dipKill();

	void dipUnacquireKeyboard();
	void dipUnacquireMouse();
	void dipUnacquireJoystick();
	void dipUnacquire();

	LOGICAL dipAcquireKeyboard();
	LOGICAL dipAcquireMouse();
	LOGICAL dipAcquireJoystick();
	LOGICAL dipAcquire();

	LOGICAL dipSetupKeyboard();
	LOGICAL dipSetupMouse();
	LOGICAL dipSetupJoystick();

	LOGICAL dipGetKeyboardData(DIDEVICEOBJECTDATA* od, UINT32* dwElements);
	LOGICAL dipGetKeyboardState(int bufsize, LPVOID buffer);
	LOGICAL dipGetMouseData(DIDEVICEOBJECTDATA* od, UINT32* dwElements);
	LOGICAL dipGetMouseState(DIMOUSESTATE* ms);
	LOGICAL dipGetJoystickData(DIDEVICEOBJECTDATA* didod, UINT32* dwElements);
	LOGICAL dipGetJoystickState(SINT32 number, DIJOYSTATE* js);
	SINT32 dipGetNumberJoysticks();

	LOGICAL dipHasKeyboard();
	LOGICAL dipHasMouse();
	LOGICAL dipHasJoystick();
