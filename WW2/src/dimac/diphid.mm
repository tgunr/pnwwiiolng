/* $Id: diphid.mm 3385 2010-04-27 19:02:13Z ahwulf@CRS $ */

#include "sysmac/syscfg.h"
#include "dip.h"
#include "dimain.h"
#include "sysmain.h"
#include "mem.h"
#include "vars.h"
#include "ddp.h"
#include "HID_Utilities.h"
#include "tut/TutorialManager.h"

#include "HID_Queue_Utilities.h"
#import <Cocoa/Cocoa.h>

#define MAX_HID_STICKS 			4
#define MAX_HID_KEYBOARDS 		4
#define MAX_HID_MICE 			4
#define MAX_STICK_ELEMENTS		256
#define MAX_KEYBOARD_ELEMENTS	1024
#define MAX_MICE_ELEMENTS		256
#define IMAP_MAXIMUM			1000
#define IMAP_MINIMUM			-1000

enum
{
	HID_AXIS = 1,
	HID_BUTTON,
	HID_POV
};

struct DIJOYSTATE
{
  SINT32    lX;                
  SINT32    lY;                   
  SINT32    lZ;                   
  SINT32    lRx;                 
  SINT32    lRy;                  
  SINT32    lRz;                 
  SINT32    lSlider1;                 
  SINT32    lSlider2;                 
} ;

typedef struct
{
	pRecDevice			dev;
	pRecElement 		ref;
	UINT32				iac;
	UINT32				dik;
	UINT32				type;
	UINT32				idx;
	UINT32				ignore;
	UINT32				lasttime;
} HID_ELEMENT;

struct KEYCODE_MAP_ELEM
{
	char	ascii;
	UINT32	dik;
	UINT32	iac;
	UINT32 	hid;
} ;

const KEYCODE_MAP_ELEM KEYCODE_MAP[] =
{
	{'A',				DIK_A,				IAC_A,					kHIDUsage_KeyboardA},						
	{'S',				DIK_S,				IAC_S,					kHIDUsage_KeyboardS},						
	{'D',				DIK_D,				IAC_D,					kHIDUsage_KeyboardD},						
	{'F',				DIK_F,				IAC_F,					kHIDUsage_KeyboardF},						
	{'H',				DIK_H,				IAC_H,					kHIDUsage_KeyboardH},						
	{'G',				DIK_G,				IAC_G,					kHIDUsage_KeyboardG},						
	{'Z',				DIK_Z,				IAC_Z,					kHIDUsage_KeyboardZ},						
	{'X',				DIK_X,				IAC_X,					kHIDUsage_KeyboardX},						
	{'C',				DIK_C,				IAC_C,					kHIDUsage_KeyboardC},						
	{'V',				DIK_V,				IAC_V,					kHIDUsage_KeyboardV},						
	{'B',				DIK_B,				IAC_B,					kHIDUsage_KeyboardB},						
	{'Q',				DIK_Q,				IAC_Q,					kHIDUsage_KeyboardQ},						
	{'W',				DIK_W,				IAC_W,					kHIDUsage_KeyboardW},						
	{'E',				DIK_E,				IAC_E,					kHIDUsage_KeyboardE},						
	{'R',				DIK_R,				IAC_R,					kHIDUsage_KeyboardR},						
	{'Y',				DIK_Y,				IAC_Y,					kHIDUsage_KeyboardY},						
	{'T',				DIK_T,				IAC_T,					kHIDUsage_KeyboardT},							
	{'1',				DIK_1,				IAC_1,					kHIDUsage_Keyboard1},						
	{'2',				DIK_2,				IAC_2,					kHIDUsage_Keyboard2},						
	{'3',				DIK_3,				IAC_3,					kHIDUsage_Keyboard3},						
	{'4',				DIK_4,				IAC_4,					kHIDUsage_Keyboard4},						
	{'6',				DIK_6,				IAC_6,					kHIDUsage_Keyboard5},						
	{'5',				DIK_5,				IAC_5,					kHIDUsage_Keyboard6},						
	{'=',				DIK_EQUALS,			IAC_EQUALS,				kHIDUsage_KeyboardEqualSign},				
	{'9',				DIK_9,				IAC_9,					kHIDUsage_Keyboard9},						
	{'7',				DIK_7,				IAC_7,					kHIDUsage_Keyboard7},						
	{'-',				DIK_MINUS,			IAC_MINUS,				kHIDUsage_KeyboardHyphen},					
	{'8',				DIK_8,				IAC_8,					kHIDUsage_Keyboard8},						
	{'0',				DIK_0,				IAC_0,					kHIDUsage_Keyboard0},						
	{']',				DIK_RBRACKET,		IAC_RIGHTBRACKET,		kHIDUsage_KeyboardCloseBracket},			
	{'O',				DIK_O,				IAC_O,					kHIDUsage_KeyboardO},						
	{'U',				DIK_U,				IAC_U,					kHIDUsage_KeyboardU},						
	{'[',				DIK_LBRACKET,		IAC_LEFTBRACKET,		kHIDUsage_KeyboardOpenBracket},				
	{'I',				DIK_I,				IAC_I,					kHIDUsage_KeyboardI},						
	{'P',				DIK_P,				IAC_P,					kHIDUsage_KeyboardP},						
	{0,					DIK_Return,			IAC_ENTER,				kHIDUsage_KeyboardReturnOrEnter},			
	{'L',				DIK_L,				IAC_L,					kHIDUsage_KeyboardL},						
	{'J',				DIK_J,				IAC_J,					kHIDUsage_KeyboardJ},						
	{'K',				DIK_K,				IAC_K,					kHIDUsage_KeyboardK},						
	{';',				DIK_SEMICOLON,		IAC_SEMICOLON,			kHIDUsage_KeyboardSemicolon},				
	{'\\',				DIK_BACKSLASH,		IAC_BACKSLASH,			kHIDUsage_KeyboardBackslash},				
	{',',				DIK_COMMA,			IAC_COMMA,				kHIDUsage_KeyboardComma},		
	{'/',				DIK_SLASH,			IAC_SLASH,				kHIDUsage_KeyboardSlash},					
	{'N',				DIK_N,				IAC_N,					kHIDUsage_KeyboardN},						
	{'M',				DIK_M,				IAC_M,					kHIDUsage_KeyboardM},						
	{'.',				DIK_PERIOD,			IAC_PERIOD,				kHIDUsage_KeyboardPeriod},					
	{0,					DIK_TAB,			IAC_TAB,				kHIDUsage_KeyboardTab},						
	{0,					DIK_SPACE,			IAC_SPACE,				kHIDUsage_KeyboardSpacebar},				
	{'`',				DIK_BACKTICK,		IAC_GRAVE,				kHIDUsage_KeyboardGraveAccentAndTilde},		
	{0,					DIK_DELETE,			IAC_BACKSPACE,			kHIDUsage_KeyboardDeleteOrBackspace},		
	{0,					DIK_PADENTER,		IAC_NUMPADENTER,		kHIDUsage_KeypadEnter},						
	{0,					DIK_ENTER,			IAC_ENTER,				kHIDUsage_KeyboardReturnOrEnter},			
	{0,					DIK_ESCAPE,			IAC_ESCAPE,				kHIDUsage_KeyboardEscape},					
	{0,					DIK_LSHIFT,			IAC_LEFTSHIFT,			kHIDUsage_KeyboardLeftShift},				
	{0,					DIK_CAPITAL,		IAC_CAPSLOCK,			kHIDUsage_KeyboardCapsLock},				
	{0,					DIK_OPTION,			IAC_LEFTALT,			kHIDUsage_KeyboardLeftAlt},					
	{0,					DIK_LCONTROL,		IAC_LEFTCONTROL,		kHIDUsage_KeyboardLeftControl},				
	{0,					DIK_RSHIFT,			IAC_RIGHTSHIFT,			kHIDUsage_KeyboardRightShift},				
	{0,					DIK_ROPTION,		IAC_RIGHTALT,			kHIDUsage_KeyboardRightAlt},				
	{0,					DIK_RCONTROL,		IAC_RIGHTCONTROL,		kHIDUsage_KeyboardRightControl},			
	{0,					DIK_PADDOT,			IAC_NUMPADDECIMAL,		kHIDUsage_KeypadPeriod},					
	{0,					DIK_RIGHT,			IAC_RIGHT,				kHIDUsage_KeyboardRightArrow},				
	{0,					DIK_PADMULT,		IAC_NUMPADMULTIPLY,		kHIDUsage_KeypadAsterisk},					
	{0,					DIK_ADD,			IAC_NUMPADADD,			kHIDUsage_KeypadPlus},						
	{0,					DIK_LEFT,			IAC_LEFT,				kHIDUsage_KeyboardLeftArrow},				
	{0,					DIK_CLEAR,			IAC_NUMLOCK,			kHIDUsage_KeyboardClear},					
	{0,					DIK_DOWN,			IAC_DOWN,				kHIDUsage_KeyboardDownArrow},				
	{0,					DIK_PADSLASH,		IAC_NUMPADDIVIDE,		kHIDUsage_KeypadSlash},						
	{0,					DIK_UP,				IAC_UP,					kHIDUsage_KeyboardUpArrow},					
	{0,					DIK_SUBTRACT,		IAC_NUMPADSUBTRACT,		kHIDUsage_KeypadHyphen},					
	{0,					DIK_PADEQUALS,		IAC_NUMPADEQUALS,		kHIDUsage_KeypadEqualSign},					
	{0,					DIK_NUMPAD0,		IAC_NUMPAD0,			kHIDUsage_Keypad0},							
	{0,					DIK_NUMPAD1,		IAC_NUMPAD1,			kHIDUsage_Keypad1},							
	{0,					DIK_NUMPAD2,		IAC_NUMPAD2,			kHIDUsage_Keypad2},							
	{0,					DIK_NUMPAD3,		IAC_NUMPAD3,			kHIDUsage_Keypad3},							
	{0,					DIK_NUMPAD4,		IAC_NUMPAD4,			kHIDUsage_Keypad4},							
	{0,					DIK_NUMPAD5,		IAC_NUMPAD5,			kHIDUsage_Keypad5},							
	{0,					DIK_NUMPAD6,		IAC_NUMPAD6,			kHIDUsage_Keypad6},							
	{0,					DIK_NUMPAD7,		IAC_NUMPAD7,			kHIDUsage_Keypad7},							
	{0,					DIK_NUMPAD8,		IAC_NUMPAD8,			kHIDUsage_Keypad8},							
	{0,					DIK_NUMPAD9,		IAC_NUMPAD9,			kHIDUsage_Keypad9},							
	{0,					DIK_F5,				IAC_F5,					kHIDUsage_KeyboardF5},						
	{0,					DIK_F6,				IAC_F6,					kHIDUsage_KeyboardF6},						
	{0,					DIK_F7,				IAC_F7,					kHIDUsage_KeyboardF7},						
	{0,					DIK_F8,				IAC_F8,					kHIDUsage_KeyboardF8},						
	{0,					DIK_F9,				IAC_F9,					kHIDUsage_KeyboardF9},						
	{0,					DIK_F11,			IAC_F11,				kHIDUsage_KeyboardF11},						
	{0,					DIK_F13,			IAC_SYSRQ,				kHIDUsage_KeyboardF13},						
	{0,					DIK_F14,			IAC_SCROLL,				kHIDUsage_KeyboardF14},						
	{0,					DIK_F10,			IAC_F10,				kHIDUsage_KeyboardF10},						
	{0,					DIK_F12,			IAC_F12,				kHIDUsage_KeyboardF12},						
	{0,					DIK_F15,			IAC_PAUSE,				kHIDUsage_KeyboardF15},						
	{0,					DIK_HELP,			IAC_INSERT,				kHIDUsage_KeyboardInsert},					
	{0,					DIK_HOME,			IAC_HOME,				kHIDUsage_KeyboardHome},					
	{0,					DIK_PGUP,			IAC_PAGEUP,				kHIDUsage_KeyboardPageUp},					
	{0,					DIK_FWDDELETE,		IAC_DELETE,				kHIDUsage_KeyboardDeleteForward},
	{0,					DIK_F3,				IAC_F3,					kHIDUsage_KeyboardF3},				
	{0,					DIK_F4,				IAC_F4,					kHIDUsage_KeyboardF4},						
	{0,					DIK_END,			IAC_END,				kHIDUsage_KeyboardEnd},						
	{0,					DIK_F2,				IAC_F2,					kHIDUsage_KeyboardF2},						
	{0,					DIK_PGDN,			IAC_PAGEDOWN,			kHIDUsage_KeyboardPageDown},				
	{0,					DIK_F1,				IAC_F1,					kHIDUsage_KeyboardF1},						
	{'\'',				DIK_APOSTROPHE,		IAC_APOSTROPHE,			kHIDUsage_KeyboardQuote},					
	{0,					DIK_LCOMMAND,		IAC_LEFTMODKEY,			kHIDUsage_KeyboardLeftGUI},
	{0,					DIK_RCOMMAND,		IAC_RIGHTMODKEY,		kHIDUsage_KeyboardRightGUI},
};

static constexpr const size_t MAX_KEYCODE_MAP_ELEMS = (sizeof(KEYCODE_MAP)/sizeof(KEYCODE_MAP_ELEM));

static UINT32 IAC_JOYSTICK_POSX[] = {IAC_JOYSTICK1_POSX, IAC_JOYSTICK2_POSX, IAC_JOYSTICK3_POSX, IAC_JOYSTICK4_POSX};
static UINT32 IAC_JOYSTICK_POSY[] = {IAC_JOYSTICK1_POSY, IAC_JOYSTICK2_POSY, IAC_JOYSTICK3_POSY, IAC_JOYSTICK4_POSY};
static UINT32 IAC_JOYSTICK_POSZ[] = {IAC_JOYSTICK1_POSZ, IAC_JOYSTICK2_POSZ, IAC_JOYSTICK3_POSZ, IAC_JOYSTICK4_POSZ};

static UINT32 IAC_JOYSTICK_ROTX[] = {IAC_JOYSTICK1_ROTX, IAC_JOYSTICK2_ROTX, IAC_JOYSTICK3_ROTX, IAC_JOYSTICK4_ROTX};
static UINT32 IAC_JOYSTICK_ROTY[] = {IAC_JOYSTICK1_ROTY, IAC_JOYSTICK2_ROTY, IAC_JOYSTICK3_ROTY, IAC_JOYSTICK4_ROTY};
static UINT32 IAC_JOYSTICK_ROTZ[] = {IAC_JOYSTICK1_ROTZ, IAC_JOYSTICK2_ROTZ, IAC_JOYSTICK3_ROTZ, IAC_JOYSTICK4_ROTZ};

static UINT32 IAC_JOYSTICK_SLIDER1[] = {IAC_JOYSTICK1_SLIDER1, IAC_JOYSTICK2_SLIDER1, IAC_JOYSTICK3_SLIDER1, IAC_JOYSTICK4_SLIDER1};
static UINT32 IAC_JOYSTICK_SLIDER2[] = {IAC_JOYSTICK1_SLIDER2, IAC_JOYSTICK2_SLIDER2, IAC_JOYSTICK3_SLIDER2, IAC_JOYSTICK4_SLIDER2};

static UINT32 IAC_JOYSTICK_BUTTON1[] = {IAC_JOYSTICK1_BUTTON1, IAC_JOYSTICK2_BUTTON1, IAC_JOYSTICK3_BUTTON1, IAC_JOYSTICK4_BUTTON1};
static UINT32 IAC_JOYSTICK_POV_N[IAC_MAX_JOYSTICKS][IAC_JOYSTICK_POV_MAX] =
{
	{IAC_JOYSTICK1_POV1_N, IAC_JOYSTICK1_POV2_N, IAC_JOYSTICK1_POV3_N, IAC_JOYSTICK1_POV4_N},
	{IAC_JOYSTICK2_POV1_N, IAC_JOYSTICK2_POV2_N, IAC_JOYSTICK2_POV3_N, IAC_JOYSTICK2_POV4_N},
	{IAC_JOYSTICK3_POV1_N, IAC_JOYSTICK3_POV2_N, IAC_JOYSTICK3_POV3_N, IAC_JOYSTICK3_POV4_N},
	{IAC_JOYSTICK4_POV1_N, IAC_JOYSTICK4_POV2_N, IAC_JOYSTICK4_POV3_N, IAC_JOYSTICK4_POV4_N}
};

static UINT32 		numentries;
static imap_queue_entry _lpiac[NUM_IAC_QUEUE_ENTRIES];

static UINT32 		numcharentries;
static CHAR_INPUT 	charInput[NUM_IAC_QUEUE_ENTRIES];

static UCHAR 		_localKeyBuffer[MAX_KEYS];
static UCHAR		_lastKeyBuffer[MAX_KEYS];

static UINT32 		_DIKtoIAC[MAX_KEYS];
static UINT32 		_ASCIItoIAC[MAX_KEYS];
static UCHAR 		_DIKtoASCII[MAX_KEYS];

static UINT32 		_NumSticks;
static pRecDevice 	_StickList[MAX_HID_STICKS];

static UINT32 		_NumKeyboards;
static pRecDevice 	_KeyboardList[MAX_HID_KEYBOARDS];

static UINT32 		_NumStickElements;
static HID_ELEMENT 	_StickElements[MAX_STICK_ELEMENTS];

static UINT32 		_NumKeyboardElements;
static HID_ELEMENT 	_KeyboardElements[MAX_KEYBOARD_ELEMENTS];

static LOGICAL init;

static LOGICAL 	_AddToQueue(UINT32 iac, SINT32 count, LOGICAL state);
static LOGICAL 	_AddToCharQueue(UINT32 iac, UINT32 charCode, LOGICAL state);
static LOGICAL 	_AddModifierKeyToQueue(UINT32 modifiers, UINT32 dik, UINT32 mask);

static void 	dipSendCocoaKeyboardMouseEvents();
static void 	dipSendStickState(void);
static void 	dipSendKeyboardState(void);
static void 	dipSendMouseState(void);

static void 	_SortDeviceList(pRecDevice*, UINT32 numDevices);
static void 	_AssignDeviceNumbers(pRecDevice*, UINT32 numDevices, UINT32* ids, UINT32 numIDs);
static void 	_LoadAxisCalibration(HID_ELEMENT*, int pos);
static void 	_LoadJoystickIDs(UINT32* ids);
static UINT32 	_vcodeToIAC(UINT32 vcode);

static const KEYCODE_MAP_ELEM* _lookupElemByASCII(UINT32 ascii);
static const KEYCODE_MAP_ELEM* _lookupElemByDIK(UINT32 dik);

static bool appHidden = false;
static bool lastmouseinit = false;

LOGICAL dimainInit(void)
{
	return dipInit();
}

LOGICAL dipInit(void)
{
	if(init) return TRUE;
  
	HIDBuildDeviceList(kHIDPage_GenericDesktop,0);
  
	if(!dipSetupKeyboard())
	{
		printf("No keyboard connected!\n");
		return FALSE;
	}
	
	if(!dipSetupMouse())
	{
		printf("No mouse connected!\n");
		return FALSE;
	}
  
  //SetMouseCoalescingEnabled(FALSE, NULL);
	
	dipSetupJoystick();
  
	init = TRUE;
	
	return TRUE;
}

LOGICAL dipSetupKeyboard(void)
{	
	const KEYCODE_MAP_ELEM* elem;
	static LOGICAL first = TRUE;
	pRecDevice pHIDDevice;
	pRecElement pHIDElement;
  
	memset(_localKeyBuffer, 0x7F, sizeof(_localKeyBuffer));
	memset(_lastKeyBuffer, 0x7F, sizeof(_lastKeyBuffer));
  
	if (first)
	{
		for (size_t i = 0; i < MAX_KEYS; i++)
		{
			elem = _lookupElemByDIK(i);
			if(elem)
			{
				_DIKtoIAC[i] = elem->iac;
				_DIKtoASCII[i] = static_cast<UCHAR>(elem->ascii);
			}
			
			elem = _lookupElemByASCII(i);
			if (elem) {
				_ASCIItoIAC[i] = elem->iac;
            }
		}
		first = FALSE;
	}	
  
	if(1)//osVersion < 0x00001020)
		return TRUE;
	
	UINT32 idx = 0, i = 0;
    
    for (idx = 0, pHIDDevice = HIDGetFirstDevice(); idx < MAX_HID_KEYBOARDS && pHIDDevice; pHIDDevice = HIDGetNextDevice(pHIDDevice))
	{
        _KeyboardList[idx] = pHIDDevice;
		if (pHIDDevice->usage == kHIDUsage_GD_Keyboard || pHIDDevice->usage == kHIDUsage_GD_Keypad) {
			++idx;
		}
	}
	_NumKeyboards = idx;
	
	for (idx = 0; idx < _NumKeyboards; idx++)
	{
 		pHIDDevice = _KeyboardList[idx];
 		if (pHIDDevice == NULL) {
            continue;
        }
 		HIDQueueDevice(pHIDDevice);
 		pHIDElement = HIDGetFirstDeviceElement(pHIDDevice, kHIDElementTypeIO);
		while (pHIDElement)
		{
			if (pHIDElement->type == kIOHIDElementTypeInput_Button)
			{
				_KeyboardElements[i].ref = pHIDElement;
				_KeyboardElements[i].dev = pHIDDevice;
				_KeyboardElements[i].idx = idx;
				_KeyboardElements[i].type = HID_BUTTON;
				_KeyboardElements[i].dik = MAX_KEYS;
				
				for (size_t j = 0; j < MAX_KEYCODE_MAP_ELEMS; j++)
				{   
					if(KEYCODE_MAP[j].hid == (UINT32)pHIDElement->usage)
					{
						_KeyboardElements[i].iac = KEYCODE_MAP[j].iac;
						_KeyboardElements[i].dik = KEYCODE_MAP[j].dik;
						break;
					}
				}
				if (++i >= MAX_KEYBOARD_ELEMENTS) {
                    break;
                }
			}
			pHIDElement = HIDGetNextDeviceElement(pHIDElement, kHIDElementTypeIO);
		}
		if (i >= MAX_KEYBOARD_ELEMENTS) {
            break;
        }
	}
	_NumKeyboardElements = i;
	return _NumKeyboards > 0;
}

static void mapDevice(pRecDevice pHIDDevice)
{
  const char* s = "";
  char t[256];
	pRecElement pHIDElement;
  pHIDElement = HIDGetFirstDeviceElement (pHIDDevice, kHIDElementTypeIO);
  while(pHIDElement)
  {
    switch(pHIDElement->usagePage)
    {
      case kHIDPage_GenericDesktop:
      {
        switch(pHIDElement->usage)
        {
          case kHIDUsage_GD_X: s = "kHIDUsage_GD_X"; break; 
          case kHIDUsage_GD_Y: s = "kHIDUsage_GD_Y"; break; 
          case kHIDUsage_GD_Z: s = "kHIDUsage_GD_Z"; break; 
          case kHIDUsage_GD_Rx: s = "kHIDUsage_GD_Rx"; break; 
          case kHIDUsage_GD_Ry: s = "kHIDUsage_GD_Ry"; break; 
          case kHIDUsage_GD_Rz: s = "kHIDUsage_GD_Rz"; break; 
          case kHIDUsage_GD_Slider: s = "kHIDUsage_GD_Slider"; break; 
          case kHIDUsage_GD_Dial: s = "kHIDUsage_GD_Dial"; break; 
          case kHIDUsage_GD_Wheel: s = "kHIDUsage_GD_Wheel"; break; 
          case kHIDUsage_GD_Hatswitch: s = "kHIDUsage_GD_Hatswitch"; break; 
          case kHIDUsage_GD_CountedBuffer: s = "kHIDUsage_GD_CountedBuffer"; break; 
          case kHIDUsage_GD_ByteCount: s = "kHIDUsage_GD_ByteCount"; break; 
          case kHIDUsage_GD_MotionWakeup: s = "kHIDUsage_GD_MotionWakeup"; break; 
          case kHIDUsage_GD_Start: s = "kHIDUsage_GD_Start"; break; 
          case kHIDUsage_GD_Select: s = "kHIDUsage_GD_Select"; break; 
          case kHIDUsage_GD_Vx: s = "kHIDUsage_GD_Vx"; break; 
          case kHIDUsage_GD_Vy: s = "kHIDUsage_GD_Vy"; break; 
          case kHIDUsage_GD_Vz: s = "kHIDUsage_GD_Vz"; break; 
          case kHIDUsage_GD_Vbrx: s = "kHIDUsage_GD_Vbrx"; break; 
          case kHIDUsage_GD_Vbry: s = "kHIDUsage_GD_Vbry"; break; 
          case kHIDUsage_GD_Vbrz: s = "kHIDUsage_GD_Vbrz"; break; 
          case kHIDUsage_GD_Vno: s = "kHIDUsage_GD_Vno"; break; 
          default: s = "[other]"; break; 
        }
      }
        break;
      case kHIDPage_Button:
        sprintf(t,"Button %ld",pHIDElement->usage);
        s = t;
        break;
    }
    
    printf("%s\n",s);
    pHIDElement = HIDGetNextDeviceElement (pHIDElement, kHIDElementTypeIO);
  }
}

LOGICAL dipSetupJoystick(void)
{
	pRecDevice pHIDDevice;
	pRecElement pHIDElement;
	UINT32 i = 0, idx = 0;
	UINT32 joyID[4];
	
	pHIDDevice = _StickList[0] = HIDGetFirstDevice ();
	idx = 0;
	while(pHIDDevice)
	{		
		if(pHIDDevice->usage == kHIDUsage_GD_Joystick || pHIDDevice->usage == kHIDUsage_GD_GamePad)
		{
			idx++;
			if(idx >= MAX_HID_STICKS) break;
		}
		pHIDDevice = _StickList[idx] = HIDGetNextDevice (pHIDDevice);
	}
	_NumSticks = idx;
  
    _SortDeviceList(_StickList, _NumSticks);
	_LoadJoystickIDs(joyID);  	
	_AssignDeviceNumbers(_StickList, _NumSticks, joyID, 4);
  
	i = 0;
	for(idx = 0; idx < 4; idx++)
	{
 		int pos = 0;
    
 		pHIDDevice = _StickList[idx];
 		
 		if(pHIDDevice == NULL) continue;
    
    //mapDevice(pHIDDevice);
 		
 		pHIDElement = HIDGetFirstDeviceElement (pHIDDevice, kHIDElementTypeIO);
		while(pHIDElement)
		{
			int povnum = 0;
			int sliderNum = 0;
			_StickElements[i].ref = pHIDElement;
			_StickElements[i].dev = pHIDDevice;
			_StickElements[i].idx = idx;
			
			switch(pHIDElement->usagePage)
			{
				case kHIDPage_GenericDesktop:
					switch(pHIDElement->usage)
                    {
                        case kHIDUsage_GD_X:
                            _StickElements[i].type = HID_AXIS;
                            _StickElements[i].iac = IAC_JOYSTICK_POSX[idx];
                            break;
                        case kHIDUsage_GD_Y:
                            _StickElements[i].type = HID_AXIS;
                            _StickElements[i].iac = IAC_JOYSTICK_POSY[idx];
                            break;
                        case kHIDUsage_GD_Z:
                            _StickElements[i].type = HID_AXIS;
                            _StickElements[i].iac = IAC_JOYSTICK_POSZ[idx];
                            break;
                        case kHIDUsage_GD_Slider:
                            if ( sliderNum == 0 ) {
                                _StickElements[i].type = HID_AXIS;
                                _StickElements[i].iac = IAC_JOYSTICK_SLIDER1[idx];
                                sliderNum = 1;
                            } else if ( sliderNum == 1 ) {
                                _StickElements[i].type = HID_AXIS;
                                _StickElements[i].iac = IAC_JOYSTICK_SLIDER2[idx];
                            }
                            break;
                        case kHIDUsage_GD_Rx:
                            _StickElements[i].type = HID_AXIS;
                            _StickElements[i].iac = IAC_JOYSTICK_ROTX[idx];
                            break;
                        case kHIDUsage_GD_Ry:
                            _StickElements[i].type = HID_AXIS;
                            _StickElements[i].iac = IAC_JOYSTICK_ROTY[idx];
                            break;
                        case kHIDUsage_GD_Rz:
                            _StickElements[i].type = HID_AXIS;
                            _StickElements[i].iac = IAC_JOYSTICK_ROTZ[idx];
                            break;
                        case kHIDUsage_GD_Hatswitch:
                            if(povnum < IAC_JOYSTICK_POV_MAX) {
                                _StickElements[i].type = HID_POV;
                                _StickElements[i].iac = IAC_JOYSTICK_POV_N[idx][povnum];
                                povnum++;
                            }
                            break;
                    }
					// calibrate
					if(_StickElements[i].type == HID_AXIS) {
						_LoadAxisCalibration(&_StickElements[i], pos);
                    }
                    break;
          
				case kHIDPage_Button:
					_StickElements[i].type = HID_BUTTON;
					_StickElements[i].iac = IAC_JOYSTICK_BUTTON1[idx] + static_cast<UINT32>(_StickElements[i].ref->usage) - 1;
                    // hack for Cyborg
                    {
                        unsigned int id = static_cast<unsigned int>((_StickElements[i].dev->vendorID << 16) | _StickElements[i].dev->productID);
                        if(id == 111345670 && _StickElements[i].ref->usage >= 11 && _StickElements[i].ref->usage <= 14) {
                            _StickElements[i].ignore = 1;
                        }
                    }
                    break;
			}
      
			pos++;
			i++;
			if(i >= MAX_STICK_ELEMENTS) {
                break;
            }
			pHIDElement = HIDGetNextDeviceElement(pHIDElement, kHIDElementTypeIO);
		}
		if(i >= MAX_STICK_ELEMENTS) {
            break;
        }
	}
	_NumStickElements = i;
	return TRUE;
}

LOGICAL dipSetupMouse(void)
{
	return TRUE;
}

void checkIfInFront()
{
    appHidden = NSApp.hidden;
    if (appHidden) {
        lastmouseinit = FALSE;
    }
}

LOGICAL dipGetKeyboardState(size_t bufsize, char * buffer)
{
	if(appHidden) {
		memset(buffer, 0x0, bufsize);
	} else {
		memcpy(buffer, _localKeyBuffer, bufsize);
	}
    
	return TRUE;
}

LOGICAL dimainGetInputs(imap_queue_entry** plpiac, UINT32 *pnumentries)
{
	numentries = 0;
	numcharentries = 0;
	
	if(init) {
        dipSendCocoaKeyboardMouseEvents();
        dipSendStickState();
	} else {
        static time_t lastRetry = 0;

		if(time(nullptr) - lastRetry > 60) {
			dipInit();
			lastRetry = time(nullptr);
		}
	}
  
	*plpiac = _lpiac;
	*pnumentries = numentries;
  
	return TRUE;
}

void dimainGetTextInputs(CHAR_INPUT**lpBuf, SINT32 *pNumElements)
{
	*lpBuf = charInput;
	*pNumElements = numcharentries;
}

static int lastmousex,lastmousey;

static void realDipSendCocoaKeyboardMouseEvents()
{
	static UINT32 lastmodifiers = 0;

	UINT32 vcode = 0;
	UINT16 charcode = 0;
    int mousex = 0, mousey = 0, diffx = 0, diffy = 0;
    
    if (!lastmouseinit) {
        lastmouseinit = true;
        sysGetMousePosition(&lastmousex, &lastmousey);
    }
    
    if (cursorHidden && !appHidden)
    {
        sysGetMousePosition(&mousex,&mousey);
        diffx = mousex - lastmousex;
        diffy = mousey - lastmousey;

        if (cursorHidden && ((mousex < 100 || mousex > (ddpGetWidth() - 100)) || (mousey < 100 || mousey > (ddpGetHeight() - 100))))
        {
            CGPoint c = { static_cast<CGFloat>(ddpGetWidth() / 2), static_cast<CGFloat>(ddpGetHeight() / 2) };
            
            CGAssociateMouseAndMouseCursorPosition(false);
            /* Why is this here? We aren't posting a synthetic event, that's in
               the documentation of CGWarpMouseCursorPosition(), it's the whole
               point of doing it this way. Updated to get rid of the deprecated
               (and, as it turns out, dangerous) API, but nonetheless disabling
               for now, unless it can be shown to be needed. - Gadget
            */
            //NSEvent *nextEvent = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:NO];
            //CGEventSourceSetLocalEventsSuppressionInterval(
            //    static_cast<CGEventSourceRef>(CFBridgingRelease(CGEventCreateSourceFromEvent(nextEvent.CGEvent))),
            //    0.0
            //);
            CGWarpMouseCursorPosition(c);
            CGAssociateMouseAndMouseCursorPosition(true); 
            lastmousex = ddpGetWidth() / 2;
            lastmousey = ddpGetHeight() / 2;
        }
        else
        {
          lastmousex = mousex;
          lastmousey = mousey;
        }
    }

    NSEvent *event = nil;
    
	while ((event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES]))
	{
		NSEventType type = event.type;
		UINT32 modifiers = event.modifierFlags;

		switch (type)
		{
			case NSKeyDown:
			case NSKeyUp:
			{
				vcode = event.keyCode;
				if ((modifiers & NSCommandKeyMask) && type == NSKeyUp)
				{
					if (vcode == DIK_PADENTER) {
                        ddpToggleWindowedMode();
                    } else if (vcode == DIK_TAB || vcode == DIK_H) {
                        ddpHideApplication();
                    }
				}
				if ((modifiers & NSCommandKeyMask) != 0) {
					[NSApp sendEvent:event];
                }
				if (vcode < MAX_KEYS) {
					if (![event isARepeat]) {
						_localKeyBuffer[vcode] = (type == NSKeyUp) ? 0x7F : 0x80;
						_AddToQueue(_DIKtoIAC[vcode], 1, (type == NSKeyUp) ? 0 : 1);
					}
					
					if ([[event characters] length] > 0) {
						[[event characters] getCharacters:&charcode range:NSMakeRange(0,1)];
						_AddToCharQueue(_DIKtoIAC[vcode], charcode, (type == NSKeyUp) ? 0 : 1);
					}
				}
                break;
			}
            
            case NSFlagsChanged:
			{
				if ((modifiers & NSAlphaShiftKeyMask) != (lastmodifiers & NSAlphaShiftKeyMask)) {
					_localKeyBuffer[DIK_CAPITAL] = (modifiers & NSAlphaShiftKeyMask) ? 0x80 : 0x7F;
					_AddToQueue(IAC_CAPSLOCK, 1, (modifiers & NSAlphaShiftKeyMask) ? 1 : 0);
				}
                if ((modifiers & NSCommandKeyMask) != (lastmodifiers & NSCommandKeyMask)) {
					_localKeyBuffer[DIK_LCOMMAND] = (modifiers & NSCommandKeyMask) ? 0x80 : 0x7F;
					_AddToQueue(IAC_LEFTMODKEY, 1, (modifiers & NSCommandKeyMask) ? 1 : 0);
				}
				if ((modifiers & NX_DEVICELSHIFTKEYMASK) != (lastmodifiers & NX_DEVICELSHIFTKEYMASK)) {
					_localKeyBuffer[DIK_LSHIFT] = (modifiers & NX_DEVICELSHIFTKEYMASK) ? 0x80 : 0x7F;
					_AddToQueue(IAC_LEFTSHIFT, 1, (modifiers & NX_DEVICELSHIFTKEYMASK) ? 1 : 0);
				}
				if ((modifiers & NX_DEVICELALTKEYMASK) != (lastmodifiers & NX_DEVICELALTKEYMASK)) {
					_localKeyBuffer[DIK_OPTION] = (modifiers & NX_DEVICELALTKEYMASK) ? 0x80 : 0x7F;
					_AddToQueue(IAC_LEFTALT, 1, (modifiers & NX_DEVICELALTKEYMASK) ? 1 : 0);
				}
				if ((modifiers & NX_DEVICELCTLKEYMASK) != (lastmodifiers & NX_DEVICELCTLKEYMASK)){
					_localKeyBuffer[DIK_LCONTROL] = (modifiers & NX_DEVICELCTLKEYMASK) ? 0x80 : 0x7F;
					_AddToQueue(IAC_LEFTCONTROL, 1, (modifiers & NX_DEVICELCTLKEYMASK) ? 1 : 0);
				}
				if ((modifiers & NX_DEVICERSHIFTKEYMASK) != (lastmodifiers & NX_DEVICERSHIFTKEYMASK)) {
					_localKeyBuffer[DIK_RSHIFT] = (modifiers & NX_DEVICERSHIFTKEYMASK) ? 0x80 : 0x7F;
					_AddToQueue(IAC_RIGHTSHIFT, 1, (modifiers & NX_DEVICERSHIFTKEYMASK) ? 1 : 0);
				}
				if ((modifiers & NX_DEVICERALTKEYMASK) != (lastmodifiers & NX_DEVICERALTKEYMASK)) {
					_localKeyBuffer[DIK_ROPTION] = (modifiers & NX_DEVICERALTKEYMASK) ? 0x80 : 0x7F;
					_AddToQueue(IAC_RIGHTALT, 1, (modifiers & NX_DEVICERALTKEYMASK) ? 1 : 0);
				}
				if ((modifiers & NX_DEVICERCTLKEYMASK) != (lastmodifiers & NX_DEVICERCTLKEYMASK)) {
					_localKeyBuffer[DIK_RCONTROL] = (modifiers & NX_DEVICERCTLKEYMASK) ? 0x80 : 0x7F;
					_AddToQueue(IAC_RIGHTCONTROL, 1, (modifiers & NX_DEVICERCTLKEYMASK) ? 1 : 0);
				}
				lastmodifiers = modifiers;
                break;
			}
            /*
            case NSMouseMoved:
            case NSLeftMouseDragged:
            case NSRightMouseDragged:
            case NSOtherMouseDragged:
            {
                CGMouseDelta ddx, ddy;
                CGGetLastMouseDelta(&ddx, &ddy);
                dx += ddx;
                dy += ddy;	
                break;
            }
            */
			case NSLeftMouseDown:
			case NSRightMouseDown:
			case NSOtherMouseDown:
			case NSLeftMouseUp:
			case NSRightMouseUp:
			case NSOtherMouseUp:
			{			
				int button = event.buttonNumber;
                
				if ((modifiers & NSControlKeyMask) != 0 && button == 0) { // emulate right click for one button mice
					button = 1;
                }
                _AddToQueue(IAC_MOUSEBUTTON1 + button, 1, (type == NSLeftMouseDown || type == NSRightMouseDown || type == NSOtherMouseDown) ? 1 : 0);
                break;
			}
			case NSScrollWheel:
			{
                if (event.deltaY < -0.05 || event.deltaY > 0.5) {
                    _AddToQueue(event.deltaY < 0.0 ? IAC_MOUSE_WHEEL_DOWN : IAC_MOUSE_WHEEL_UP, 1, 1);
                }
                break;
			}
			case NSAppKitDefined:
            {
				if (event.subtype == NSApplicationActivatedEventType) {
					ddpShowApplication();
                }
                break;
            }
		}
	}
	
    if (cursorHidden) {
        // arbitrary speed up
        if (diffx != 0) {
            _AddToQueue(IAC_MOUSEX, 2 * (diffx), 0);
        }
        if (diffy != 0) {
            _AddToQueue(IAC_MOUSEY, 2 * (diffy), 0);
        }
    }
}

void dipSendCocoaKeyboardMouseEvents()
{
    @autoreleasepool {
        realDipSendCocoaKeyboardMouseEvents();
    }
}

void dipSendStickState(void)
{
  // switch to hid queue
	static SINT32 lastbuttons[MAX_STICK_ELEMENTS];
	static DIJOYSTATE lastjoyState[MAX_HID_STICKS];
  
	SINT32 i, idx;
	DIJOYSTATE joyState[MAX_HID_STICKS];
  
	memset(joyState, 0x0, sizeof(joyState));
	
	for(i = 0; i < (int)_NumStickElements; i++)
	{
		SINT32 sval;
    
		if(!HIDIsValidDevice(_StickElements[i].dev) || !HIDIsValidElement(_StickElements[i].dev, _StickElements[i].ref))
		{
			init = FALSE;
			return;
		}
		
        sval = HIDGetElementValue(_StickElements[i].dev,_StickElements[i].ref);
    
		if(_StickElements[i].type == HID_AXIS)
		{
			int center = (_StickElements[i].ref->maxReport + _StickElements[i].ref->minReport) / 2;
			float scale = 0;
			
			if(abs(_StickElements[i].ref->maxReport - _StickElements[i].ref->minReport) > 10)
				scale = 2000.0f / (float)(_StickElements[i].ref->maxReport - _StickElements[i].ref->minReport);
      
			sval = scale * (sval - center);
		}
    
		idx = _StickElements[i].idx;
		
		switch(_StickElements[i].type)
		{
			case HID_AXIS:
                switch(_StickElements[i].iac)
                {
                    case IAC_JOYSTICK1_SLIDER1: case IAC_JOYSTICK2_SLIDER1: case IAC_JOYSTICK3_SLIDER1: case IAC_JOYSTICK4_SLIDER1:
                        joyState[idx].lSlider1 += sval;
                        break;
                    case IAC_JOYSTICK1_SLIDER2: case IAC_JOYSTICK2_SLIDER2: case IAC_JOYSTICK3_SLIDER2: case IAC_JOYSTICK4_SLIDER2:
                        joyState[idx].lSlider2 += sval;
                        break;
                    case IAC_JOYSTICK1_POSX: case IAC_JOYSTICK2_POSX: case IAC_JOYSTICK3_POSX: case IAC_JOYSTICK4_POSX:
                        joyState[idx].lX += sval;
                        break;
                    case IAC_JOYSTICK1_POSY: case IAC_JOYSTICK2_POSY: case IAC_JOYSTICK3_POSY: case IAC_JOYSTICK4_POSY:
                        joyState[idx].lY -= sval;
                        break;
                    case IAC_JOYSTICK1_POSZ: case IAC_JOYSTICK2_POSZ: case IAC_JOYSTICK3_POSZ: case IAC_JOYSTICK4_POSZ:
                        joyState[idx].lZ -= sval;
                        break;
                    case IAC_JOYSTICK1_ROTX: case IAC_JOYSTICK2_ROTX: case IAC_JOYSTICK3_ROTX: case IAC_JOYSTICK4_ROTX:
                        joyState[idx].lRx += sval;
                        break;
                    case IAC_JOYSTICK1_ROTY: case IAC_JOYSTICK2_ROTY: case IAC_JOYSTICK3_ROTY: case IAC_JOYSTICK4_ROTY:
                        joyState[idx].lRy += sval;
                        break;
                    case IAC_JOYSTICK1_ROTZ: case IAC_JOYSTICK2_ROTZ: case IAC_JOYSTICK3_ROTZ: case IAC_JOYSTICK4_ROTZ:
                        joyState[idx].lRz += sval;
                        break;
                    default:
                        break;
                }
                break;
        
			case HID_BUTTON:
			{
				if(lastbuttons[i] != sval && !_StickElements[i].ignore) {
					_AddToQueue(_StickElements[i].iac, 1, sval);
                }
                lastbuttons[i] = sval;
                break;
			}

			case HID_POV:
				if(sval >= _StickElements[i].ref->min && sval <=  _StickElements[i].ref->max)
				{
					if(lastbuttons[i] != sval) {
						if(lastbuttons[i] > -1) {
							_AddToQueue(_StickElements[i].iac + lastbuttons[i] - _StickElements[i].ref->min, 1, 0);
                        }
						_AddToQueue(_StickElements[i].iac + sval - _StickElements[i].ref->min, 1, 1);
					}
					lastbuttons[i] = sval;
                } else {
					if(lastbuttons[i] > -1) {
						_AddToQueue(_StickElements[i].iac + lastbuttons[i] - _StickElements[i].ref->min, 1, 0);
                    }
                    lastbuttons[i] = -1;
                }
                break;
        }
	}
	
	for(i = 0; i < (int)_NumSticks; i++)
	{
		// clip values if necessary
		if(joyState[i].lX < IMAP_MINIMUM) joyState[i].lX = IMAP_MINIMUM;
		else if(joyState[i].lX > IMAP_MAXIMUM) joyState[i].lX = IMAP_MAXIMUM;
    
		if(joyState[i].lSlider1 < IMAP_MINIMUM) joyState[i].lSlider1 = IMAP_MINIMUM;
		else if(joyState[i].lSlider1 > IMAP_MAXIMUM) joyState[i].lSlider1 = IMAP_MAXIMUM;
    
		if(joyState[i].lSlider2 < IMAP_MINIMUM) joyState[i].lSlider2 = IMAP_MINIMUM;
		else if(joyState[i].lSlider2 > IMAP_MAXIMUM) joyState[i].lSlider2 = IMAP_MAXIMUM;
    
		if(joyState[i].lY < IMAP_MINIMUM) joyState[i].lY = IMAP_MINIMUM;
		else if(joyState[i].lY > IMAP_MAXIMUM) joyState[i].lY = IMAP_MAXIMUM;
    
		if(joyState[i].lZ < IMAP_MINIMUM) joyState[i].lZ = IMAP_MINIMUM;
		else if(joyState[i].lZ > IMAP_MAXIMUM) joyState[i].lZ = IMAP_MAXIMUM;
    
		if(joyState[i].lRx < IMAP_MINIMUM) joyState[i].lRx = IMAP_MINIMUM;
		else if(joyState[i].lRx > IMAP_MAXIMUM) joyState[i].lRx = IMAP_MAXIMUM;
    
		if(joyState[i].lRy < IMAP_MINIMUM) joyState[i].lRy = IMAP_MINIMUM;
		else if(joyState[i].lRy > IMAP_MAXIMUM) joyState[i].lRy = IMAP_MAXIMUM;
    
		if(joyState[i].lRz < IMAP_MINIMUM) joyState[i].lRz = IMAP_MINIMUM;
		else if(joyState[i].lRz > IMAP_MAXIMUM) joyState[i].lRz = IMAP_MAXIMUM;
		
		// add to queue	
		if(lastjoyState[i].lSlider1 != joyState[i].lSlider1)
			_AddToQueue(IAC_JOYSTICK_SLIDER1[i],joyState[i].lSlider1,0);
    
		if(lastjoyState[i].lSlider2 != joyState[i].lSlider2)
			_AddToQueue(IAC_JOYSTICK_SLIDER2[i],joyState[i].lSlider2,0);
    
		if(lastjoyState[i].lX != joyState[i].lX)
			_AddToQueue(IAC_JOYSTICK_POSX[i],joyState[i].lX,0);
    
		if(lastjoyState[i].lY != joyState[i].lY)
			_AddToQueue(IAC_JOYSTICK_POSY[i],joyState[i].lY,0);	
    
		if(lastjoyState[i].lZ != joyState[i].lZ)
			_AddToQueue(IAC_JOYSTICK_POSZ[i],joyState[i].lZ,0);	
    
		if(lastjoyState[i].lRx != joyState[i].lRx)
			_AddToQueue(IAC_JOYSTICK_ROTX[i],joyState[i].lRx,0);
		
		if(lastjoyState[i].lRy != joyState[i].lRy)
			_AddToQueue(IAC_JOYSTICK_ROTY[i],joyState[i].lRy,0);
    
		if(lastjoyState[i].lRz != joyState[i].lRz)
			_AddToQueue(IAC_JOYSTICK_ROTZ[i],joyState[i].lRz,0);
		
		lastjoyState[i] = joyState[i];
	}
}

int dipHasKeyboard()
{
	// Look at all of our keyboards and see if we have any valid right now.
	for(UINT32 i = 0; i < _NumKeyboards; i++)
	{
		if(HIDIsValidDevice(_KeyboardList[i]))
			return 1 ;
	}
	
	return 0 ;
}

int dipHasMouse()
{
	// Because 'false' would mean you are a fucktard.
	return 1 ;
}

int dipHasJoystick()
{
	// Look at all of our joysticks and see if we have any valid right now.
	for(UINT32 i = 0 ; i < _NumSticks ; ++i )
	{
		if(HIDIsValidDevice(_StickList[i]))
			return 1 ;
	}
  
	return 0 ;
}

static void dipSendKeyboardState(void)
{
	IOHIDEventStruct event;
	int i, j;
  
	for(i = 0; i < (int)_NumKeyboards; i++)
	{
		if(!HIDIsValidDevice(_KeyboardList[i]))
		{
			init = FALSE;
			return;
		}
		
		while(HIDGetEvent(_KeyboardList[i], &event))
		{
			for(j = 0; j < (int)_NumKeyboardElements; j++)
			{
				if(_KeyboardElements[j].dev == _KeyboardList[i] && _KeyboardElements[j].ref->cookie == event.elementCookie)
				{
					_AddToQueue(_KeyboardElements[j].iac, 1, event.value);	
					if(_KeyboardElements[j].dik >= 0 && _KeyboardElements[j].dik < MAX_KEYS)
						_localKeyBuffer[_KeyboardElements[j].dik] = event.value ? 0x80 : 0x7F;
					break;
				}
			}
		}
	}
}

static void dipSendMouseState(void)
{
}

void dimainKill(void)
{
	dipKill();
}

void dipKill(void)
{
	static LOGICAL deinit = FALSE;
	
	if(deinit) return;
  
	HIDReleaseDeviceList();
	HIDReleaseAllDeviceQueues();
	
	deinit = TRUE;
}

static LOGICAL _AddToQueue(UINT32 iac, SINT32 count, LOGICAL state)
{
	if(numentries >= NUM_IAC_QUEUE_ENTRIES || iac == IAC_NOKEY) return FALSE;
  
	TutorialManager::GetInstance()->DoEvent(TUT_EVENT_KEY, &iac, &state, &count) ;
  
	_lpiac[numentries].iac = iac;
	_lpiac[numentries].count = count;
	_lpiac[numentries].state = state;
	numentries++;
  
	return TRUE;
}

static LOGICAL _AddToCharQueue(UINT32 iac, UINT32 charcode, LOGICAL state)
{
	if(numcharentries >= NUM_IAC_QUEUE_ENTRIES) return FALSE;
  
	charInput[numcharentries].uIac = iac;
	charInput[numcharentries].uChar = charcode;
	charInput[numcharentries].bState = state;
	numcharentries++;
  
	return TRUE;
}

static LOGICAL _AddModifierKeyToQueue(UINT32 modifiers, UINT32 dik, UINT32 mask)
{	
	_localKeyBuffer[dik] = (modifiers & mask)  ? 0x80 :  0x7F;
  
	if(_localKeyBuffer[dik] != _lastKeyBuffer[dik])
		_AddToQueue(_DIKtoIAC[dik], 1,  (modifiers & mask) ? 1 : 0);
  
	_lastKeyBuffer[dik] = _localKeyBuffer[dik];
  
	return TRUE;
}

void _SortDeviceList(pRecDevice* devices, UINT32 numDevices)
{
	int i, j;
	UINT32 devid[MAX_HID_STICKS];
	pRecDevice tmpRef;
	UINT32 tmpID;
	
	if(numDevices < 2) return;	
	
	for(i = 0; i < (int)numDevices; i++)
	{
		devid[i] = (devices[i]->vendorID << 16) | devices[i]->productID;
	}
	
	for(i = 0; i < (int)numDevices; i++)
	{
		for(j = i + 1; j < (int)numDevices; j++)
		{
			if(devid[i] >= devid[j])
			{
				tmpID = devid[j];
				tmpRef = devices[j];
				
				devid[j] = devid[i];
				devices[j] = devices[i];
				
				devid[i] = tmpID;
				devices[i] = tmpRef;
			}
		}
	}
}

// Assign device to a specific numbered position
void _AssignDeviceNumbers(pRecDevice* devices, UINT32 numDevices, UINT32* ids, UINT32 numIDs)
{
	UINT32 devid, i,j, k;
	pRecDevice* assignedDevices = (pRecDevice*)memAlloc(sizeof(pRecDevice) * numIDs);
	pRecDevice tmpRef;
	
	memset(assignedDevices, 0x0, sizeof(pRecDevice) * numIDs);
	
	for(i = 0; i < numDevices; i++)
	{
		devid = (devices[i]->vendorID << 16) | devices[i]->productID;
		
		if(devid == 0) continue; // can't identify
		
		for(j = 0; j < numIDs; j++)
		{
			if(ids[j] == 0) continue; // don't care what goes in this slot
			
			if(ids[j] == devid)
			{
        // take out of device list and put in assigned device list
				assignedDevices[j] = devices[i];
				devices[i] = NULL;
			}
		}
	}
	
	// Assign to devices and fill in any gaps, this is a little hairy
	for(i = 0; i < numIDs; i++)
	{
		tmpRef = devices[i];
		devices[i] = assignedDevices[i];
		assignedDevices[i] = tmpRef;
	}
	
	// fill in the gaps
	j = 0;
	k = numIDs;
	for(i = 0; i < numIDs; i++)
	{
		if(devices[i] == NULL)
		{
			// look in assigned list
			for(; j < numIDs; j++)
			{
				if(assignedDevices[j] != NULL)
				{
					devices[i] = assignedDevices[j]; // move it
					assignedDevices[j] = NULL;
					break;
				}
			}
		}
		
		// still NULL look in device list
		if(devices[i] == NULL)
		{
			for(; k < numDevices; k++)
			{
				if(devices[k] != NULL)
				{
					devices[i] = devices[k]; // move it
					devices[j] = NULL;
					break;
				}
			}
		}
	}
	
	// whew that was complicated. anyone think of something simpler?
	memFree(assignedDevices);
}

static void _LoadAxisCalibration(HID_ELEMENT* elem, int pos)
{	
	unsigned int devid = (elem->dev->vendorID << 16) | elem->dev->productID;
	
	elem->ref->minReport = [[NSUserDefaults standardUserDefaults] 
                          integerForKey:[NSString stringWithFormat:@"%d-%d-min", devid, pos]];
	elem->ref->maxReport = [[NSUserDefaults standardUserDefaults] 
                          integerForKey:[NSString stringWithFormat:@"%d-%d-max", devid, pos]];	
}

void _LoadJoystickIDs(UINT32* ids)
{
	for(int i = 0; i < 4; i++)
	{
		ids[i] = [[NSUserDefaults standardUserDefaults]	
              integerForKey:[NSString stringWithFormat:@"joy%d", i+1]];
	}
}

static UINT32 _vcodeToIAC(UINT32 vcode)
{
	static UINT32 state = 0;
	static 	void* kptr = NULL;
  
	UINT32 iac, keytrans;
	UCHAR c;
	
	iac = 0;
	
	if(!kptr)
		kptr  = (void*)GetScriptManagerVariable(smKCHRCache);
  
	if(_DIKtoASCII[vcode]) // NOTE: this assumes that all ASCII keys are always grouped in the same spot on the keyboard
	{
		keytrans = KeyTranslate(kptr, vcode, (unsigned long*)&state);
		c = ((UCHAR*)&keytrans)[3];
		
		if(isascii(c) && c < MAX_KEYS)
			iac = _ASCIItoIAC[c];
	}
  
	if(iac == 0)
		iac  = _DIKtoIAC[vcode];
  
	return iac;	
}

static const KEYCODE_MAP_ELEM* _lookupElemByDIK(UINT32 dik)
{
	int j;
	
	for(j = 0; j < (int)MAX_KEYCODE_MAP_ELEMS; j++)
	{   
		if(KEYCODE_MAP[j].dik == dik)
		{
			return &KEYCODE_MAP[j];
		}
	}
  
	return NULL;
}

static const KEYCODE_MAP_ELEM* _lookupElemByASCII(UINT32 ascii)
{
	int j;
	
	for(j = 0; j < (int)MAX_KEYCODE_MAP_ELEMS; j++)
	{   
		if(tolower(KEYCODE_MAP[j].ascii) == (int)ascii)
		{
			return &KEYCODE_MAP[j];
		}
	}
  
	return NULL;
}
