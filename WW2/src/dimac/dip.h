#pragma once

// Copyright 2001 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

static inline bool dipKEYDOWN(const char *buffer, UINT8 key) { return (buffer[key] & 0x80) != 0; }

// macintosh virtual key definitions
// from inside mac vol 5
// REPLACED with definitions from HIToolbox/Events.h - Gadget
enum {
    DIK_A                    = 0x00,
    DIK_S                    = 0x01,
    DIK_D                    = 0x02,
    DIK_F                    = 0x03,
    DIK_H                    = 0x04,
    DIK_G                    = 0x05,
    DIK_Z                    = 0x06,
    DIK_X                    = 0x07,
    DIK_C                    = 0x08,
    DIK_V                    = 0x09,
    DIK_B                    = 0x0B,
    DIK_Q                    = 0x0C,
    DIK_W                    = 0x0D,
    DIK_E                    = 0x0E,
    DIK_R                    = 0x0F,
    DIK_Y                    = 0x10,
    DIK_T                    = 0x11,
    DIK_1                    = 0x12,
    DIK_2                    = 0x13,
    DIK_3                    = 0x14,
    DIK_4                    = 0x15,
    DIK_6                    = 0x16,
    DIK_5                    = 0x17,
    DIK_EQUAL                = 0x18,
    DIK_9                    = 0x19,
    DIK_7                    = 0x1A,
    DIK_MINUS                = 0x1B,
    DIK_8                    = 0x1C,
    DIK_0                    = 0x1D,
    DIK_RIGHTBRACKET         = 0x1E,
    DIK_O                    = 0x1F,
    DIK_U                    = 0x20,
    DIK_LEFTBRACKET          = 0x21,
    DIK_I                    = 0x22,
    DIK_P                    = 0x23,
    DIK_L                    = 0x25,
    DIK_J                    = 0x26,
    DIK_QUOTE                = 0x27,
    DIK_K                    = 0x28,
    DIK_SEMICOLON            = 0x29,
    DIK_BACKSLASH            = 0x2A,
    DIK_COMMA                = 0x2B,
    DIK_SLASH                = 0x2C,
    DIK_N                    = 0x2D,
    DIK_M                    = 0x2E,
    DIK_PERIOD               = 0x2F,
    DIK_GRAVE                = 0x32,
    DIK_KEYPADDECIMAL        = 0x41,
    DIK_KEYPADMULTIPLY       = 0x43,
    DIK_KEYPADPLUS           = 0x45,
    DIK_KEYPADCLEAR          = 0x47,
    DIK_KEYPADDIVIDE         = 0x4B,
    DIK_KEYPADENTER          = 0x4C,
    DIK_KEYPADMINUS          = 0x4E,
    DIK_KEYPADEQUALS         = 0x51,
    DIK_KEYPAD0              = 0x52,
    DIK_KEYPAD1              = 0x53,
    DIK_KEYPAD2              = 0x54,
    DIK_KEYPAD3              = 0x55,
    DIK_KEYPAD4              = 0x56,
    DIK_KEYPAD5              = 0x57,
    DIK_KEYPAD6              = 0x58,
    DIK_KEYPAD7              = 0x59,
    DIK_KEYPAD8              = 0x5B,
    DIK_KEYPAD9              = 0x5C,

    DIK_RETURN               = 0x24,
    DIK_TAB                  = 0x30,
    DIK_SPACE                = 0x31,
    DIK_DELETE               = 0x33,
    DIK_ESCAPE               = 0x35,
    DIK_COMMAND              = 0x37,
    DIK_SHIFT                = 0x38,
    DIK_CAPSLOCK             = 0x39,
    DIK_OPTION               = 0x3A,
    DIK_CONTROL              = 0x3B,
    DIK_RIGHTSHIFT           = 0x3C,
    DIK_RIGHTOPTION          = 0x3D,
    DIK_RIGHTCONTROL         = 0x3E,
    DIK_FUNCTION             = 0x3F,
    DIK_F17                  = 0x40,
    DIK_VOLUMEUP             = 0x48,
    DIK_VOLUMEDOWN           = 0x49,
    DIK_MUTE                 = 0x4A,
    DIK_F18                  = 0x4F,
    DIK_F19                  = 0x50,
    DIK_F20                  = 0x5A,
    DIK_F5                   = 0x60,
    DIK_F6                   = 0x61,
    DIK_F7                   = 0x62,
    DIK_F3                   = 0x63,
    DIK_F8                   = 0x64,
    DIK_F9                   = 0x65,
    DIK_F11                  = 0x67,
    DIK_F13                  = 0x69,
    DIK_F16                  = 0x6A,
    DIK_F14                  = 0x6B,
    DIK_F10                  = 0x6D,
    DIK_F12                  = 0x6F,
    DIK_F15                  = 0x71,
    DIK_HELP                 = 0x72,
    DIK_HOME                 = 0x73,
    DIK_PAGEUP               = 0x74,
    DIK_FORWARDDELETE        = 0x75,
    DIK_F4                   = 0x76,
    DIK_END                  = 0x77,
    DIK_F2                   = 0x78,
    DIK_PAGEDOWN             = 0x79,
    DIK_F1                   = 0x7A,
    DIK_LEFTARROW            = 0x7B,
    DIK_RIGHTARROW           = 0x7C,
    DIK_DOWNARROW            = 0x7D,
    DIK_UPARROW              = 0x7E,

    DIK_ISO_SECTION          = 0x0A,

    DIK_JIS_YEN              = 0x5D,
    DIK_JIS_UNDERSCORE       = 0x5E,
    DIK_JIS_KEYPADCOMMA      = 0x5F,
    DIK_JIS_EISU             = 0x66,
    DIK_JIS_KANA             = 0x68,

    MAX_KEYS                 = 0x80,
    
    // Original Windows DirectInput names for things:
    DIK_LMENU = DIK_OPTION,
    DIK_UP = DIK_UPARROW,
    DIK_DOWN = DIK_DOWNARROW,
    DIK_LEFT = DIK_LEFTARROW,
    DIK_RIGHT = DIK_RIGHTARROW,
    DIK_EQUALS = DIK_EQUAL,
    DIK_RBRACKET = DIK_RIGHTBRACKET,
    DIK_LBRACKET = DIK_LEFTBRACKET,
    DIK_Return = DIK_RETURN,
    DIK_BACKTICK = DIK_GRAVE,
    DIK_PADENTER = DIK_KEYPADENTER,
    DIK_LSHIFT = DIK_SHIFT,
    DIK_CAPITAL = DIK_CAPSLOCK,
    DIK_LCONTROL = DIK_CONTROL,
    DIK_RSHIFT = DIK_RIGHTSHIFT,
    DIK_ROPTION = DIK_RIGHTOPTION,
    DIK_RCONTROL = DIK_RIGHTCONTROL,
    DIK_PADDOT = DIK_KEYPADDECIMAL,
    DIK_PADMULT = DIK_KEYPADMULTIPLY,
    DIK_ADD = DIK_KEYPADPLUS,
    DIK_CLEAR = DIK_KEYPADCLEAR,
    DIK_PADSLASH = DIK_KEYPADDIVIDE,
    DIK_SUBTRACT = DIK_KEYPADMINUS,
    DIK_PADEQUALS = DIK_KEYPADEQUALS,
    DIK_NUMPAD0 = DIK_KEYPAD0,
    DIK_NUMPAD1 = DIK_KEYPAD1,
    DIK_NUMPAD2 = DIK_KEYPAD2,
    DIK_NUMPAD3 = DIK_KEYPAD3,
    DIK_NUMPAD4 = DIK_KEYPAD4,
    DIK_NUMPAD5 = DIK_KEYPAD5,
    DIK_NUMPAD6 = DIK_KEYPAD6,
    DIK_NUMPAD7 = DIK_KEYPAD7,
    DIK_NUMPAD8 = DIK_KEYPAD8,
    DIK_NUMPAD9 = DIK_KEYPAD9,
    DIK_PGUP = DIK_PAGEUP,
    DIK_FWDDELETE = DIK_FORWARDDELETE,
    DIK_PGDN = DIK_PAGEDOWN,
    DIK_APOSTROPHE = DIK_QUOTE,
    DIK_LCOMMAND = DIK_COMMAND,
    DIK_RCOMMAND = DIK_COMMAND,
    DIK_ENTER = 0x34, // not in the Carbon headers
};

#ifdef __cplusplus
extern "C" {
#endif

// fills the buffer pointed to by buffer with single bits indicating that
// the key is down. an 0x80 indicates down, 0x7f indicates up.
LOGICAL dipGetKeyboardState(size_t bufsize, char * buffer);
LOGICAL dipInit(void);
LOGICAL dipSetupKeyboard(void);
LOGICAL dipSetupMouse(void);
LOGICAL dipSetupJoystick(void);
void dipKill(void);

int dipHasKeyboard(void);
int dipHasJoystick(void);
int dipHasMouse(void);

void checkIfInFront();

#ifdef __cplusplus
}
#endif
