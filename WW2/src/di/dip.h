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

	LOGICAL dipInit(void);

	void dipKillKeyboard(void);
	void dipKillMouse(void);
	void dipKillJoystick(void);
	void dipKill(void);

	void dipUnacquireKeyboard(void);
	void dipUnacquireMouse(void);
	void dipUnacquireJoystick(void);
	void dipUnacquire(void);

	LOGICAL dipAcquireKeyboard(void);
	LOGICAL dipAcquireMouse(void);
	LOGICAL dipAcquireJoystick(void);
	LOGICAL dipAcquire(void);

	LOGICAL dipSetupKeyboard(void);
	LOGICAL dipSetupMouse(void);
	LOGICAL dipSetupJoystick(void);

	LOGICAL dipGetKeyboardData(DIDEVICEOBJECTDATA* od, UINT32* dwElements);
	LOGICAL dipGetKeyboardState(int bufsize, LPVOID buffer);
	LOGICAL dipGetMouseData(DIDEVICEOBJECTDATA* od, UINT32* dwElements);
	LOGICAL dipGetMouseState(DIMOUSESTATE* ms);
	LOGICAL dipGetJoystickData(DIDEVICEOBJECTDATA* didod, UINT32* dwElements);
	LOGICAL dipGetJoystickState(SINT32 number, DIJOYSTATE* js);
	SINT32 dipGetNumberJoysticks(void);

	LOGICAL dipHasKeyboard();
	LOGICAL dipHasMouse();
	LOGICAL dipHasJoystick();
