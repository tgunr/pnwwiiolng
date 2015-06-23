#include <syscfg.h> // Path is project specific.
//

#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "dip.h"
#include "dimain.h"
#include "tut/TutorialManager.h"
#include "ime/imeMain.h"

#include "di/NPClient.h" // Track IR support

#define MAX_IAC_CODES 256
#define DEF_MAX_KEY_INPUT   128

static LOGICAL				trackIRinit;
extern HWND					myhwnd;

static LOGICAL				g_lastBuffer[NUM_IAC_QUEUE_ENTRIES];
static SINT32				g_xlastaxisms;
static SINT32				g_ylastaxisms;
static SINT32				g_zlastaxisms;
static LOGICAL				g_lastbuttonsms[4];
static SINT32				g_xlastposjs[IAC_MAX_JOYSTICKS];
static SINT32				g_ylastposjs[IAC_MAX_JOYSTICKS];
static SINT32				g_zlastposjs[IAC_MAX_JOYSTICKS];
static SINT32				g_xlastrotjs[IAC_MAX_JOYSTICKS];
static SINT32				g_ylastrotjs[IAC_MAX_JOYSTICKS];
static SINT32		 		g_zlastrotjs[IAC_MAX_JOYSTICKS];
static SINT32				g_lastslider[IAC_MAX_JOYSTICKS][NUM_SLIDERS_PER_JOYSTICK];
static UINT32				g_lastpov[IAC_MAX_JOYSTICKS][IAC_JOYSTICK_POV_MAX];
static LOGICAL				g_lastbuttonsjs[IAC_MAX_JOYSTICKS][IAC_JOYSTICK_MAX_BUTTONS];

static UINT16*				g_di2iac;
static imap_queue_entry*	g_lpiac;
static UINT32				g_numentries;

static CHAR_INPUT g_keyInput[DEF_MAX_KEY_INPUT];
static SINT32 g_iKeyCount = 0;

static UINT32 IAC_JOYSTICK_POSX[] = {IAC_JOYSTICK1_POSX, IAC_JOYSTICK2_POSX, IAC_JOYSTICK3_POSX, IAC_JOYSTICK4_POSX};
static UINT32 IAC_JOYSTICK_POSY[] = {IAC_JOYSTICK1_POSY, IAC_JOYSTICK2_POSY, IAC_JOYSTICK3_POSY, IAC_JOYSTICK4_POSY};
static UINT32 IAC_JOYSTICK_POSZ[] = {IAC_JOYSTICK1_POSZ, IAC_JOYSTICK2_POSZ, IAC_JOYSTICK3_POSZ, IAC_JOYSTICK4_POSZ};

static UINT32 IAC_JOYSTICK_ROTX[] = {IAC_JOYSTICK1_ROTX, IAC_JOYSTICK2_ROTX, IAC_JOYSTICK3_ROTX, IAC_JOYSTICK4_ROTX};
static UINT32 IAC_JOYSTICK_ROTY[] = {IAC_JOYSTICK1_ROTY, IAC_JOYSTICK2_ROTY, IAC_JOYSTICK3_ROTY, IAC_JOYSTICK4_ROTY};
static UINT32 IAC_JOYSTICK_ROTZ[] = {IAC_JOYSTICK1_ROTZ, IAC_JOYSTICK2_ROTZ, IAC_JOYSTICK3_ROTZ, IAC_JOYSTICK4_ROTZ};

static UINT32 IAC_JOYSTICK_BUTTON1[] = {IAC_JOYSTICK1_BUTTON1, IAC_JOYSTICK2_BUTTON1, IAC_JOYSTICK3_BUTTON1, IAC_JOYSTICK4_BUTTON1};
static UINT32 IAC_JOYSTICK_POV_N[IAC_MAX_JOYSTICKS][IAC_JOYSTICK_POV_MAX]  =
{
	{IAC_JOYSTICK1_POV1_N, IAC_JOYSTICK1_POV2_N, IAC_JOYSTICK1_POV3_N, IAC_JOYSTICK1_POV4_N},
	{IAC_JOYSTICK2_POV1_N, IAC_JOYSTICK2_POV2_N, IAC_JOYSTICK2_POV3_N, IAC_JOYSTICK2_POV4_N},
	{IAC_JOYSTICK3_POV1_N, IAC_JOYSTICK3_POV2_N, IAC_JOYSTICK3_POV3_N, IAC_JOYSTICK3_POV4_N},
	{IAC_JOYSTICK4_POV1_N, IAC_JOYSTICK4_POV2_N, IAC_JOYSTICK4_POV3_N, IAC_JOYSTICK4_POV4_N},
};

static LOGICAL AddToQueue(UINT32 iac, SINT32 count, LOGICAL state);

#define IN_WINDOW_MOUSE

static void InitTrackIR();
static void GetTrackIRInput();

LOGICAL dimainInit(void)
{
SINT32 i;
SINT32 j;

	g_lpiac = (imap_queue_entry*)memAlloc(NUM_IAC_QUEUE_ENTRIES * sizeof(imap_queue_entry));

	if(g_lpiac == nullptr)
	{
		return(FALSE);
	}

	g_di2iac = (UINT16*)memAlloc(MAX_IAC_CODES * sizeof(UINT16));

	if(g_di2iac == nullptr)
	{
		return(FALSE);
	}

	g_numentries = 0;

	if(!dipInit())
	{
		return(FALSE);
	}

#if defined(_DEBUG)
	dipSetupKeyboard();
// now we use the define at the head of the file or syscfg
#ifdef IN_WINDOW_MOUSE
	dipSetupMouse();
#endif	// IN_WINDOW_MOUSE
	dipSetupJoystick();
#else	// _DEBUG
	dipSetupKeyboard();
//#ifndef VEHICLE_OBJECT_ANALYSIS
	dipSetupMouse();
//#endif	// VEHICLE_OBJECT_ANALYSIS
	dipSetupJoystick();
#endif	// _DEBUG

	for(i = 0; i < NUM_IAC_QUEUE_ENTRIES; i++)
	{
		g_lastBuffer[i] = 0;
	}

	g_xlastaxisms = 0;
	g_ylastaxisms = 0;
	g_zlastaxisms = 0;
	g_lastbuttonsms[0] = 0;
	g_lastbuttonsms[1] = 0;
	g_lastbuttonsms[2] = 0;
	g_lastbuttonsms[3] = 0;

	for(i = 0; i < IAC_MAX_JOYSTICKS; i++)
	{
		g_xlastposjs[i] = 1;
		g_ylastposjs[i] = 1;
		g_zlastposjs[i] = 1;
		g_xlastrotjs[i] = 1;
		g_ylastrotjs[i] = 1;
		g_zlastrotjs[i] = 1;
		g_lastslider[i][0] = 1;
		g_lastslider[i][1] = 1;
		g_lastpov[i][0] = 1;
		g_lastpov[i][1] = 1;
	}

	for(j = 0; j < IAC_MAX_JOYSTICKS; j++)
	{
		for(i = 0; i < IAC_JOYSTICK_MAX_BUTTONS; i++)
		{
			g_lastbuttonsjs[j][i] = 0;
		}
	}

	// zero indicates "not used"
	for(i=0; i < MAX_IAC_CODES; i++)
	{
		g_di2iac[i] = 0;
	}

	g_di2iac[DIK_ESCAPE]		= IAC_ESCAPE;
	g_di2iac[DIK_1]				= IAC_1;
	g_di2iac[DIK_2]				= IAC_2;
	g_di2iac[DIK_3]				= IAC_3;
	g_di2iac[DIK_4]				= IAC_4;
	g_di2iac[DIK_5]				= IAC_5;
	g_di2iac[DIK_6]				= IAC_6;
	g_di2iac[DIK_7]				= IAC_7;
	g_di2iac[DIK_8]				= IAC_8;
	g_di2iac[DIK_9]				= IAC_9;
	g_di2iac[DIK_0]				= IAC_0;
	g_di2iac[DIK_MINUS]			= IAC_MINUS;
	g_di2iac[DIK_EQUALS]		= IAC_EQUALS;
	g_di2iac[DIK_BACK]			= IAC_BACKSPACE;
	g_di2iac[DIK_TAB]			= IAC_TAB;
	g_di2iac[DIK_Q]				= IAC_Q;
	g_di2iac[DIK_W]				= IAC_W;
	g_di2iac[DIK_E]				= IAC_E;
	g_di2iac[DIK_R]				= IAC_R;
	g_di2iac[DIK_T]				= IAC_T;
	g_di2iac[DIK_Y]				= IAC_Y;
	g_di2iac[DIK_U]				= IAC_U;
	g_di2iac[DIK_I]				= IAC_I;
	g_di2iac[DIK_O]				= IAC_O;
	g_di2iac[DIK_P]				= IAC_P;
	g_di2iac[DIK_LBRACKET]		= IAC_LEFTBRACKET;
	g_di2iac[DIK_RBRACKET]		= IAC_RIGHTBRACKET;
	g_di2iac[DIK_RETURN]		= IAC_ENTER;
	g_di2iac[DIK_LCONTROL]		= IAC_LEFTCONTROL;
	g_di2iac[DIK_A]				= IAC_A;
	g_di2iac[DIK_S]				= IAC_S;
	g_di2iac[DIK_D]				= IAC_D;
	g_di2iac[DIK_F]				= IAC_F;
	g_di2iac[DIK_G]				= IAC_G;
	g_di2iac[DIK_H]				= IAC_H;
	g_di2iac[DIK_J]				= IAC_J;
	g_di2iac[DIK_K]				= IAC_K;
	g_di2iac[DIK_L]				= IAC_L;
	g_di2iac[DIK_SEMICOLON]		= IAC_SEMICOLON;
	g_di2iac[DIK_APOSTROPHE]	= IAC_APOSTROPHE;
	g_di2iac[DIK_GRAVE]			= IAC_GRAVE;
	g_di2iac[DIK_LSHIFT]		= IAC_LEFTSHIFT;
	g_di2iac[DIK_BACKSLASH]		= IAC_BACKSLASH;
	g_di2iac[DIK_Z]				= IAC_Z;
	g_di2iac[DIK_X]				= IAC_X;
	g_di2iac[DIK_C]				= IAC_C;
	g_di2iac[DIK_V]				= IAC_V;
	g_di2iac[DIK_B]				= IAC_B;
	g_di2iac[DIK_N]				= IAC_N;
	g_di2iac[DIK_M]				= IAC_M;
	g_di2iac[DIK_COMMA]			= IAC_COMMA;
	g_di2iac[DIK_PERIOD]		= IAC_PERIOD;
	g_di2iac[DIK_SLASH]			= IAC_SLASH;
	g_di2iac[DIK_RSHIFT]		= IAC_RIGHTSHIFT;
	g_di2iac[DIK_MULTIPLY]		= IAC_NUMPADMULTIPLY;
	g_di2iac[DIK_LMENU]			= IAC_LEFTALT;
	g_di2iac[DIK_SPACE]			= IAC_SPACE;
	g_di2iac[DIK_F1]			= IAC_F1;
	g_di2iac[DIK_F2]			= IAC_F2;
	g_di2iac[DIK_F3]			= IAC_F3;
	g_di2iac[DIK_F4]			= IAC_F4;
	g_di2iac[DIK_F5]			= IAC_F5;
	g_di2iac[DIK_F6]			= IAC_F6;
	g_di2iac[DIK_F7]			= IAC_F7;
	g_di2iac[DIK_F8]			= IAC_F8;
	g_di2iac[DIK_F9]			= IAC_F9;
	g_di2iac[DIK_F10]			= IAC_F10;
	g_di2iac[DIK_NUMLOCK]		= IAC_NUMLOCK;
	g_di2iac[DIK_SCROLL]		= IAC_SCROLL;
	g_di2iac[DIK_NUMPAD7]		= IAC_NUMPAD7;
	g_di2iac[DIK_NUMPAD8]		= IAC_NUMPAD8;
	g_di2iac[DIK_NUMPAD9]		= IAC_NUMPAD9;
	g_di2iac[DIK_CAPSLOCK]		= IAC_CAPSLOCK;
	g_di2iac[DIK_SUBTRACT]		= IAC_NUMPADSUBTRACT;
	g_di2iac[DIK_NUMPAD4]		= IAC_NUMPAD4;
	g_di2iac[DIK_NUMPAD5]		= IAC_NUMPAD5;
	g_di2iac[DIK_NUMPAD6]		= IAC_NUMPAD6;
	g_di2iac[DIK_ADD]			= IAC_NUMPADADD;
	g_di2iac[DIK_NUMPAD1]		= IAC_NUMPAD1;
	g_di2iac[DIK_NUMPAD2]		= IAC_NUMPAD2;
	g_di2iac[DIK_NUMPAD3]		= IAC_NUMPAD3;
	g_di2iac[DIK_NUMPAD0]		= IAC_NUMPAD0;
	g_di2iac[DIK_DECIMAL]		= IAC_NUMPADDECIMAL;
	g_di2iac[DIK_F11]			= IAC_F11;
	g_di2iac[DIK_F12]			= IAC_F12;
	g_di2iac[DIK_NUMPADENTER]	= IAC_NUMPADENTER;
	g_di2iac[DIK_RCONTROL]		= IAC_RIGHTCONTROL;
	g_di2iac[DIK_DIVIDE]		= IAC_NUMPADDIVIDE;
	g_di2iac[DIK_SYSRQ]			= IAC_SYSRQ;
	g_di2iac[DIK_RMENU]			= IAC_RIGHTALT;
	g_di2iac[DIK_PAUSE]			= IAC_PAUSE;
	g_di2iac[DIK_HOME]			= IAC_HOME;
	g_di2iac[DIK_UP]			= IAC_UP;
	g_di2iac[DIK_PRIOR]			= IAC_PAGEUP;
	g_di2iac[DIK_LEFT]			= IAC_LEFT;
	g_di2iac[DIK_RIGHT]			= IAC_RIGHT;
	g_di2iac[DIK_END]			= IAC_END;
	g_di2iac[DIK_DOWN]			= IAC_DOWN;
	g_di2iac[DIK_NEXT]			= IAC_PAGEDOWN;
	g_di2iac[DIK_INSERT]		= IAC_INSERT;
	g_di2iac[DIK_DELETE]		= IAC_DELETE;

    InitTrackIR();

	return(TRUE);
}

////////////////////////////////////////
// Function: dimainVKToIAC
// Description: Virtual Key to IAC code
////////////////////////////////////////
UINT32 dimainVKToIAC(UINT32 uVK)
{
    switch(uVK)
    {
    case VK_BACK:       return IAC_BACKSPACE;
    case VK_TAB:        return IAC_TAB;
    case VK_RETURN:     return IAC_ENTER;
    case VK_SHIFT:      return IAC_LEFTSHIFT;
    case VK_CONTROL:    return IAC_LEFTCONTROL;
    case VK_MENU:       return IAC_LEFTALT;
    case VK_PAUSE:      return IAC_PAUSE;
    case VK_CAPITAL:    return IAC_CAPSLOCK;
    case VK_ESCAPE:     return IAC_ESCAPE;
    case VK_SPACE:      return IAC_SPACE;
    case VK_PRIOR:      return IAC_PAGEUP;
    case VK_NEXT:       return IAC_PAGEDOWN;
    case VK_END:        return IAC_END;
    case VK_HOME:       return IAC_HOME;
    case VK_LEFT:       return IAC_LEFT;
    case VK_UP:         return IAC_UP;
    case VK_RIGHT:      return IAC_RIGHT;
    case VK_DOWN:       return IAC_DOWN;
    case VK_INSERT:     return IAC_INSERT;
    case VK_DELETE:     return IAC_DELETE;
    case 48:            return IAC_0;
    case 49:            return IAC_1;
    case 50:            return IAC_2;
    case 51:            return IAC_3;
    case 52:            return IAC_4;
    case 53:            return IAC_5;
    case 54:            return IAC_6;
    case 55:            return IAC_7;
    case 56:            return IAC_8;
    case 57:            return IAC_9;
    case 65:            return IAC_A;
    case 66:            return IAC_B;
    case 67:            return IAC_C;
    case 68:            return IAC_D;
    case 69:            return IAC_E;
    case 70:            return IAC_F;
    case 71:            return IAC_G;
    case 72:            return IAC_H;
    case 73:            return IAC_I;
    case 74:            return IAC_J;
    case 75:            return IAC_K;
    case 76:            return IAC_L;
    case 77:            return IAC_M;
    case 78:            return IAC_N;
    case 79:            return IAC_O;
    case 80:            return IAC_P;
    case 81:            return IAC_Q;
    case 82:            return IAC_R;
    case 83:            return IAC_S;
    case 84:            return IAC_T;
    case 85:            return IAC_U;
    case 86:            return IAC_V;
    case 87:            return IAC_W;
    case 88:            return IAC_X;
    case 89:            return IAC_Y;
    case 90:            return IAC_Z;
    case VK_NUMPAD0:    return IAC_NUMPAD0;
    case VK_NUMPAD1:    return IAC_NUMPAD1;
    case VK_NUMPAD2:    return IAC_NUMPAD2;
    case VK_NUMPAD3:    return IAC_NUMPAD3;
    case VK_NUMPAD4:    return IAC_NUMPAD4;
    case VK_NUMPAD5:    return IAC_NUMPAD5;
    case VK_NUMPAD6:    return IAC_NUMPAD6;
    case VK_NUMPAD7:    return IAC_NUMPAD7;
    case VK_NUMPAD8:    return IAC_NUMPAD8;
    case VK_NUMPAD9:    return IAC_NUMPAD9;
    case VK_MULTIPLY:   return IAC_NUMPADMULTIPLY;
    case VK_ADD:        return IAC_NUMPADADD;
    case VK_SUBTRACT:   return IAC_NUMPADSUBTRACT;
    case VK_DECIMAL:    return IAC_NUMPADDECIMAL;
    case VK_DIVIDE:     return IAC_NUMPADDIVIDE;
    case VK_F1:         return IAC_F1;
    case VK_F2:         return IAC_F2;
    case VK_F3:         return IAC_F3;
    case VK_F4:         return IAC_F4;
    case VK_F5:         return IAC_F5;
    case VK_F6:         return IAC_F6;
    case VK_F7:         return IAC_F7;
    case VK_F8:         return IAC_F8;
    case VK_F9:         return IAC_F9;
    case VK_F10:        return IAC_F10;
    case VK_F11:        return IAC_F11;
    case VK_F12:        return IAC_F12;
    case VK_NUMLOCK:    return IAC_NUMLOCK;
    case VK_SCROLL:     return IAC_SCROLL;
    case VK_LSHIFT:     return IAC_LEFTSHIFT;
    case VK_RSHIFT:     return IAC_RIGHTSHIFT;
    case VK_LCONTROL:   return IAC_LEFTCONTROL;
    case VK_RCONTROL:   return IAC_RIGHTCONTROL;
    case VK_LMENU:      return IAC_LEFTALT;
    case VK_RMENU:      return IAC_RIGHTALT;
    case VK_OEM_1:      return IAC_SEMICOLON;
    case VK_OEM_3:      return IAC_GRAVE;
    case VK_OEM_4:      return IAC_LEFTBRACKET;
    case VK_OEM_5:      return IAC_BACKSLASH;
    case VK_OEM_6:      return IAC_RIGHTBRACKET;
    case VK_OEM_COMMA:  return IAC_COMMA;
    case VK_OEM_PERIOD: return IAC_PERIOD;
    case VK_OEM_2:      return IAC_SLASH;
    case VK_OEM_7:      return IAC_APOSTROPHE;
    case VK_OEM_PLUS:   return IAC_EQUALS;
    case VK_OEM_MINUS:  return IAC_MINUS;
    case VK_OEM_8:      return IAC_OEM_8;
    case VK_OEM_102:    return IAC_OEM_102;
	case VK_PROCESSKEY:
		{
			UINT32 vk = imeClient()->ProcessKey();

			if(vk != VK_PROCESSKEY)
				return dimainVKToIAC(vk);

			bool stop = true;
		}
    default:            return 0;
    }
}

void dimainKill(void)
{
	if(g_lpiac != nullptr)
	{
		memFree(g_lpiac);
		g_lpiac = nullptr;
	}

	if(g_di2iac != nullptr)
	{
		memFree(g_di2iac);
		g_di2iac = nullptr;
	}

	dipKill();

    Write_Log(LOG_OLD, LOG_INFO, "[dimainKill]");
}

//////////////////////////////////////////////////////////
// Function: dimainAddChar
// Description: Handles WM_CHAR event from dxmain
//////////////////////////////////////////////////////////
bool dimainAddChar(UINT32 ucChar)
{
    if(g_iKeyCount <= 0) return false;
    if(g_iKeyCount >= DEF_MAX_KEY_INPUT) return false;

    if(g_keyInput[g_iKeyCount - 1].uIac != 0)
    {
        g_keyInput[g_iKeyCount - 1].uChar = ucChar;
		return true;
    }

	return false;
}

//////////////////////////////////////////////////////////
// Function: dimainVKUp
// Description: Handles WM_KEYUP event from dxmain
//////////////////////////////////////////////////////////
bool dimainVKUp(UINT32 uVK, UINT32 uScanCode)
{
    if(g_iKeyCount >= DEF_MAX_KEY_INPUT) return false;

    memset(&g_keyInput[g_iKeyCount], 0, sizeof(CHAR_INPUT));

    g_keyInput[g_iKeyCount].uScanCode = uScanCode;
    g_keyInput[g_iKeyCount].uVK = uVK;
    g_keyInput[g_iKeyCount].uIac = dimainVKToIAC(uVK);
    g_keyInput[g_iKeyCount].bState = FALSE;

	bool rval = (g_keyInput[g_iKeyCount].uIac) ? true : false;

    g_iKeyCount++;

	return rval;
}

//////////////////////////////////////////////////////////
// Function: dimainVKDown
// Description: Handles WM_KEYDOWN event from dxmain
//////////////////////////////////////////////////////////
bool dimainVKDown(UINT32 uVK, UINT32 uScanCode)
{
    if(g_iKeyCount >= DEF_MAX_KEY_INPUT) return false;
	if(uVK == VK_PROCESSKEY) return false;

    memset(&g_keyInput[g_iKeyCount], 0, sizeof(CHAR_INPUT));

    g_keyInput[g_iKeyCount].uScanCode = uScanCode;
    g_keyInput[g_iKeyCount].uVK = uVK;
    g_keyInput[g_iKeyCount].uIac = dimainVKToIAC(uVK);
    g_keyInput[g_iKeyCount].bState = TRUE;

	bool rval = (g_keyInput[g_iKeyCount].uIac) ? true : false;

    g_iKeyCount++;

	return rval;
}

//////////////////////////////////////////////////////////
// Function: dimainGetTextInputs
// Description:
//////////////////////////////////////////////////////////
void dimainGetTextInputs(CHAR_INPUT** lpBuf, SINT32* pNumElements)
{
    *lpBuf = g_keyInput;
    *pNumElements = g_iKeyCount;

    g_iKeyCount = 0;
}

/*--------------------------------------------------------
This code is left here, since it is an alternative way of
providing mapped virtual keys.
//////////////////////////////////////////////////////////
// Function: AddTextInput
// Description:
//////////////////////////////////////////////////////////
static void AddTextInput(SINT32 uScanCode, LOGICAL bState)
{
    HKL hkl = GetKeyboardLayout(0);
    UINT32 uVK = MapVirtualKeyEx(uScanCode, 1, hkl);
    UINT16 nChar;
    UCHAR ucKBState[256];
    SINT32 iRetCode = 0;

    memset(ucKBState, 0, sizeof(UCHAR) * 256);

    GetKeyboardState(ucKBState);

    iRetCode = ToAsciiEx(uVK, uScanCode, ucKBState, &nChar, 0, hkl);

    g_keyInput[g_iKeyCount].uVK = uVK;
    g_keyInput[g_iKeyCount].uIac = dimainVKToIAC((SINT32)uVK);
    g_keyInput[g_iKeyCount].uChar = (UCHAR)nChar;
    g_keyInput[g_iKeyCount].uScanCode = uScanCode;
    g_keyInput[g_iKeyCount].bState = bState;

    g_iKeyCount++;
}
----------------------------------------------------------*/

static LOGICAL AddToQueue(UINT32 iac, SINT32 count, LOGICAL state)
{
	if(iac == 0)
	{
		Write_Debug("IAC was null; skipped");
		return(FALSE);
	}

	if(g_numentries >= NUM_IAC_QUEUE_ENTRIES)
	{
		Write_Debug("Number of entries in the IAC queue has been exceeded");
		return(FALSE);
	}

	TUT_MGR->DoEvent(TUT_EVENT_KEY, &iac, &state, &count);

	g_lpiac[g_numentries].iac = iac;
	g_lpiac[g_numentries].count = count;
	g_lpiac[g_numentries].state = state;
	g_numentries += 1;

	return(TRUE);
}

LOGICAL dimainGetInputs(imap_queue_entry**plpiac, UINT32* pnumentries)
{
    LOGICAL				direction;
    SINT32				i, count, len, ofs;
    SINT32				xpos, ypos, zpos;
    SINT32				xrot, yrot, zrot;
    SINT32				JoystickNumber;
    SINT32				slider[2];
    DIMOUSESTATE		ms;
    DIDEVICEOBJECTDATA	od[_DIMAIN_INPUT_BUFFERSIZE];
    UINT32				pov[IAC_MAX_JOYSTICKS][4];

	if(g_lpiac == nullptr || g_di2iac == nullptr)
	{
		return FALSE;
	}

	// Reset the number of queue entries
	g_numentries = 0;

	count = _DIMAIN_INPUT_BUFFERSIZE;

	// Process the keyboard inputs
	///Note: dipGetKeyboardData actually expects a pointer to a UINT32!
	while(dipGetKeyboardData(od, (UINT32*)&count) && count != 0)
	{
		// Handle keyboard input
		for(i=0; i < count; i++)
		{
			ofs = od[i].dwOfs;

			if(od[i].dwData & 0x80)
			{
				if(!g_lastBuffer[ofs])
				{
					g_lastBuffer[ofs] = TRUE;
 				    AddToQueue(g_di2iac[ofs], 1, TRUE);
				}
			}
			else
			{
				if(g_lastBuffer[ofs])
				{
					g_lastBuffer[ofs] = FALSE;
                    AddToQueue(g_di2iac[ofs], 1, FALSE);
				}
			}
		}
	}

	// Process the mouse inputs
	if(dipGetMouseState(&ms))
	{
        static SINT32 iWheelHistory = 0;

		// Handle mouse input
		if(ms.lX)
			AddToQueue(IAC_MOUSEX, ms.lX, 0);

		if(ms.lY)
			AddToQueue(IAC_MOUSEY, ms.lY, 0);

		if(ms.lZ)
        {
            AddToQueue(ms.lZ > 0 ? IAC_MOUSE_WHEEL_UP : IAC_MOUSE_WHEEL_DOWN, 1, 1);
        }

        if(iWheelHistory != 0 && iWheelHistory != ms.lZ)
        {
            AddToQueue(iWheelHistory > 0 ? IAC_MOUSE_WHEEL_UP : IAC_MOUSE_WHEEL_DOWN, 1, 0);
        }

        iWheelHistory = ms.lZ;

		for(i=0; i < 4; i++)
		{
			if(ms.rgbButtons[i] & 0x80)	// down
			{
				if(!g_lastbuttonsms[i])
				{
					g_lastbuttonsms[i] = TRUE;
					AddToQueue(IAC_MOUSEBUTTON1 + i, 1, TRUE);
				}
			}
			else
			{
				if((g_lastbuttonsms[i]) && !(ms.rgbButtons[i] & 0x80))
				{
					g_lastbuttonsms[i] = FALSE;
					AddToQueue(IAC_MOUSEBUTTON1 + i, 1, FALSE);
				}
			}
		}
	}

	// Process the JOYSTICK_ inputs
	len = _DIMAIN_INPUT_BUFFERSIZE;

	///NOTE: Expects a UINT32 pointer
	if(dipGetJoystickData(od, (UINT32*)&len))
	{
		for(i=0; i < len; i++)
		{
			// Extract joystick number from data and convert data back to signed 32 bits
			JoystickNumber = HIWORD(od[i].dwData);
			od[i].dwData =(SINT32)(SINT16)LOWORD(od[i].dwData);

			if(od[i].dwOfs >= DIJOFS_BUTTON0 &&	od[i].dwOfs <= DIJOFS_BUTTON31)
			{
				count = od[i].dwOfs - DIJOFS_BUTTON0;

				if(od[i].dwData & 0x80)
				{
					if(!g_lastbuttonsjs[JoystickNumber][count])
					{
						g_lastbuttonsjs[JoystickNumber][count] = TRUE;

						AddToQueue(IAC_JOYSTICK_BUTTON1[JoystickNumber] + count, 1 ,TRUE);
					}
				}
				else
				{
					if(g_lastbuttonsjs[JoystickNumber][count])
					{
						g_lastbuttonsjs[JoystickNumber][count] = FALSE;

						AddToQueue(IAC_JOYSTICK_BUTTON1[JoystickNumber] + count, 1 ,FALSE);
					}
				}
			}
			else
			switch(od[i].dwOfs)
			{
			case DIJOFS_POV(0):		// A point-of-view hat 1
				pov[JoystickNumber][0] = od[i].dwData;

				if(pov[JoystickNumber][0] != g_lastpov[JoystickNumber][0])
				{
					direction = FALSE;
					count = 1;

					switch(g_lastpov[JoystickNumber][0])
					{
						case 0:		// North
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 0, count, direction);
							break;
						case 4500:	// Northeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 1, count, direction);
							break;
						case 9000:	// East
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 2, count, direction);
							break;
						case 13500:	// Southeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 3, count, direction);
							break;
						case 18000:	// South
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 4, count, direction);
							break;
						case 22500:	// Southwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 5, count, direction);
							break;
						case 27000:	// West
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 6, count, direction);
						break;
						case 31500:	// Northwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 7, count, direction);
							break;
						default:
							Write_Debug("Unrecognized pov hat postion on joystick %d", JoystickNumber);
							break;
					}

					direction = TRUE;

					switch(pov[JoystickNumber][0])
					{
						case 0:		// North
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 0, count, direction);
							break;
						case 4500:	// Northeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 1, count, direction);
							break;
						case 9000:	// East
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 2, count, direction);
							break;
						case 13500:	// Southeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 3, count, direction);
							break;
						case 18000:	// South
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 4, count, direction);
							break;
						case 22500:	// Southwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 5, count, direction);
							break;
						case 27000:	// West
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 6, count, direction);
						break;
						case 31500:	// Northwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][0] + 7, count, direction);
							break;
						default:
							Write_Debug("Unrecognized pov hat postion on joystick %d", JoystickNumber);
							break;
					}

					g_lastpov[JoystickNumber][0] = pov[JoystickNumber][0];
				}

				break;
			case DIJOFS_POV(1):		// A point-of-view hat 2
				pov[JoystickNumber][1] = od[i].dwData;

				if(pov[JoystickNumber][1] != g_lastpov[JoystickNumber][1])
				{
					direction = FALSE;
					count = 1;

					switch(g_lastpov[JoystickNumber][1])
					{
						case 0:		// North
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 0, count, direction);
							break;
						case 4500:	// Northeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 1, count, direction);
							break;
						case 9000:	// East
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 2, count, direction);
							break;
						case 13500:	// Southeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 3, count, direction);
							break;
						case 18000:	// South
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 4, count, direction);
							break;
						case 22500:	// Southwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 5, count, direction);
							break;
						case 27000: // West
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 6, count, direction);
							break;
						case 31500:	// Northwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 7, count, direction);
							break;
						default:
							Write_Debug("Unrecognized pov hat postion on joystick %d", JoystickNumber);
							break;
					}

					direction = TRUE;

					switch(pov[JoystickNumber][1])
					{
						case 0:		// North
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 0, count, direction);
							break;
						case 4500:	// Northeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 1, count, direction);
							break;
						case 9000:	// East
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 2, count, direction);
							break;
						case 13500:	// Southeast
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 3, count, direction);
							break;
						case 18000:	// South
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 4, count, direction);
							break;
						case 22500:	// Southwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 5, count, direction);
							break;
						case 27000: // West
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 6, count, direction);
							break;
						case 31500:	// Northwest
							AddToQueue(IAC_JOYSTICK_POV_N[JoystickNumber][1] + 7, count, direction);
							break;
						default:
							Write_Debug("Unrecognized pov hat postion on joystick %d", JoystickNumber);
							break;
					}

					g_lastpov[JoystickNumber][1] = pov[JoystickNumber][1];
				}

				break;
			case DIJOFS_RX:			// The x-axis rotation.
				xrot = od[i].dwData;

				// scale to -1000 thru 1000
				if(xrot != g_xlastrotjs[JoystickNumber])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_ROTX, xrot, 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_ROTX, xrot, 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_ROTX, xrot, 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_ROTX, xrot, 0);
						break;
					default:
						Write_Debug("Invalid joystick number in RX: %d", JoystickNumber);
						break;
					}

					g_xlastrotjs[JoystickNumber] = xrot;
				}
				break;
			case DIJOFS_RY:			// The y-axis rotation.
				yrot = od[i].dwData;

				// scale to -1000 thru 1000
				if(yrot != g_ylastrotjs[JoystickNumber])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_ROTY, yrot, 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_ROTY, yrot, 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_ROTY, yrot, 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_ROTY, yrot, 0);
						break;
					default:
						Write_Debug("Invalid joystick number in RY: %d", JoystickNumber);
						break;
					}

					g_ylastrotjs[JoystickNumber] = yrot;
				}
				break;
			case DIJOFS_RZ:			// The z-axis rotation(rudder).
				zrot = od[i].dwData;
				// scale to -1000 thru 1000
				if(zrot != g_zlastrotjs[JoystickNumber])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_ROTZ, zrot, 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_ROTZ, zrot, 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_ROTZ, zrot, 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_ROTZ, zrot, 0);
						break;
					default:
						Write_Debug("Invalid joystick number in RZ: %d", JoystickNumber);
						break;
					}

					g_zlastrotjs[JoystickNumber] = zrot;
				}
				break;
			case DIJOFS_X:			// The x-axis.
				xpos = od[i].dwData;
				// scale to -1000 thru 1000
				if(xpos != g_xlastposjs[JoystickNumber])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_POSX, xpos, 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_POSX, xpos, 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_POSX, xpos, 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_POSX, xpos, 0);
						break;
					default:
						Write_Debug("Invalid joystick number in X: %d", JoystickNumber);
						break;
					}

					g_xlastposjs[JoystickNumber] = xpos;
				}
				break;
			case DIJOFS_Y:			// The y-axis.
				ypos = od[i].dwData;
				// scale to -1000 thru 1000

				ypos = -ypos; // win95 y axis is scaled, however it's backwards

				if(ypos != g_ylastposjs[JoystickNumber])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_POSY, ypos, 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_POSY, ypos, 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_POSY, ypos, 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_POSY, ypos, 0);
						break;
					default:
						Write_Debug("Invalid joystick number in Y: %d", JoystickNumber);
						break;
					}

					g_ylastposjs[JoystickNumber] = ypos;
				}
				break;
			case DIJOFS_Z:			// The z-axis.
				zpos = od[i].dwData;
				zpos = -zpos;

				// scale to -1000 thru 1000
				if(zpos != g_zlastposjs[JoystickNumber])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_POSZ, zpos, 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_POSZ, zpos, 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_POSZ, zpos, 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_POSZ, zpos, 0);
						break;
					default:
						Write_Debug("Invalid joystick number in Z: %d", JoystickNumber);
						break;
					}

					g_zlastposjs[JoystickNumber] = zpos;
				}
				break;
			case DIJOFS_SLIDER(0):	// Slider 1
				slider[0] = od[i].dwData;
				slider[0] -= 500;
				slider[0] *= 2;
				slider[0] = -slider[0];

				// scale to -1000 thru 1000
				if(slider[0] != g_lastslider[JoystickNumber][0])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_SLIDER1, slider[0], 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_SLIDER1, slider[0], 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_SLIDER1, slider[0], 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_SLIDER1, slider[0], 0);
						break;
					default:
						Write_Debug("Invalid joystick number in Slider 1: %d", JoystickNumber);
						break;
					}

					g_lastslider[JoystickNumber][0] = slider[0];
				}
				break;
			case DIJOFS_SLIDER(1):	// Slider 2
				slider[1] = od[i].dwData;
				slider[1] -= 500;
				slider[1] *= 2;
				slider[1] = -slider[1];

				// scale to -1000 thru 1000
				if(slider[1] != g_lastslider[JoystickNumber][1])
				{
					switch(JoystickNumber)
					{
					case 0:
						AddToQueue(IAC_JOYSTICK1_SLIDER2, slider[1], 0);
						break;
					case 1:
						AddToQueue(IAC_JOYSTICK2_SLIDER2, slider[1], 0);
						break;
					case 2:
						AddToQueue(IAC_JOYSTICK3_SLIDER2, slider[1], 0);
						break;
					case 3:
						AddToQueue(IAC_JOYSTICK4_SLIDER2, slider[1], 0);
						break;
					default:
						Write_Debug("Invalid joystick number in Slider 2: %d", JoystickNumber);
						break;
					}

					g_lastslider[JoystickNumber][1] = slider[1];
				}
				break;
			default:
				break;
			}
		}
	}

	if(trackIRinit)
		GetTrackIRInput();

	*plpiac = g_lpiac;
	*pnumentries = g_numentries;

	return TRUE;
}

static void GetTrackIRInput()
{
	static unsigned long NPFrameSignature;
	TRACKIRDATA tid;
	NPRESULT result = NP_GetData( &tid );

	if( NP_OK == result &&
		tid.wNPStatus == NPSTATUS_REMOTEACTIVE &&
		NPFrameSignature != tid.wPFrameSignature)
	{
		float	stupidPitchValue;

		NPFrameSignature = tid.wPFrameSignature;

		stupidPitchValue = tid.fNPPitch <=0 ? -tid.fNPPitch / 8191.0f : -tid.fNPPitch / 7500.0f;

		AddToQueue(IAC_JOYSTICK_TRACK_IR_X, (SINT32)(-tid.fNPYaw   / 12287.0 * 1000.0), 0);
		AddToQueue(IAC_JOYSTICK_TRACK_IR_Y, (SINT32)(stupidPitchValue * 1000.0), 0);
		AddToQueue(IAC_JOYSTICK_TRACK_IR_Z, (SINT32)(-tid.fNPRoll  / 12287.0 * 1000.0), 0);
//		Write_Log (LOG_FPS, LOG_INFO, "yaw = %6.2f  pitch = %6.2f roll = %6.2f", tid.fNPYaw, tid.fNPPitch, tid.fNPRoll);
	}
}

static void InitTrackIR()
{
	NPRESULT result;
	unsigned int DataFields;

	result = NPClient_Init();
	if( NP_OK != result ) return;

	result = NP_RegisterProgramProfileID(3301);
	if( NP_OK != result ) return;

	result = NP_RegisterWindowHandle(myhwnd);
	if( NP_OK != result ) return;

	DataFields = NPPitch | NPYaw;

	NP_RequestData(DataFields);

	result = NP_StartDataTransmission();
	if (result != NP_OK) return;

	trackIRinit = TRUE;
}
