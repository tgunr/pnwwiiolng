#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "imap/iac.h"
#include "imap/imap.h"
#include "str/striws.h"
#include "ui/uiUTF.h"

/************************************************************************************************
	IAC codes
*************************************************************************************************/

struct iacEntry
{
	unsigned int iac;
	const char* name;
	int locID;
} ;

static const iacEntry USKeyboardTable[] =
{
	{IAC_ESCAPE,				"escape",							341},
	{IAC_F1,					"f1",								342},
	{IAC_F2,					"f2",								343},
	{IAC_F3,					"f3",								344},
	{IAC_F4,					"f4",								345},
	{IAC_F5,					"f5",								346},
	{IAC_F6,					"f6",								347},
	{IAC_F7,					"f7",								348},
	{IAC_F8,					"f8",								349},
	{IAC_F9,					"f9",								350},
	{IAC_F10,					"f10",								351},
	{IAC_F11,					"f11",								352},
	{IAC_F12,					"f12",								353},
	{IAC_SYSRQ,					"sysreq",							354},
	{IAC_SCROLL,				"scroll",							355},
	{IAC_PAUSE,					"pause",							356},
	{IAC_GRAVE,					"grave",							357},
	{IAC_1,						"one",								358},
	{IAC_2,						"two",								359},
	{IAC_3,						"three",							360},
	{IAC_4,						"four",								361},
	{IAC_5,						"five",								362},
	{IAC_6,						"six",								363},
	{IAC_7,						"seven",							364},
	{IAC_8,						"eight",							365},
	{IAC_9,						"nine",								366},
	{IAC_0,						"zero",								367},
	{IAC_MINUS,					"minus",							368},
	{IAC_EQUALS,				"equals",							369},
	{IAC_BACKSPACE,				"backspace",						370},
	{IAC_INSERT,				"insert",							371},
	{IAC_HOME,					"home",								372},
	{IAC_PAGEUP,				"pageup",							373},
	{IAC_NUMLOCK,				"numlock",							374},
	{IAC_NUMPADDIVIDE,			"numpaddivide",						375},
	{IAC_NUMPADMULTIPLY,		"numpadmultiply",					376},
	{IAC_NUMPADSUBTRACT,		"numpadsubtract",					377},
	{IAC_TAB,					"tab",								378},
	{IAC_Q,						"q",								379},
	{IAC_W,						"w",								380},
	{IAC_E,						"e",								381},
	{IAC_R,						"r",								382},
	{IAC_T,						"t",								383},
	{IAC_Y,						"y",								384},
	{IAC_U,						"u",								385},
	{IAC_I,						"i",								386},
	{IAC_O,						"o",								387},
	{IAC_P,						"p",								388},
	{IAC_LEFTBRACKET,			"leftbracket",						389},
	{IAC_RIGHTBRACKET,			"rightbracket",						390},
	{IAC_BACKSLASH,				"backslash",						391},
	{IAC_DELETE,				"delete",							392},
	{IAC_END,					"end",								393},
	{IAC_PAGEDOWN,				"pagedown",							394},
	{IAC_NUMPAD7,				"numpadseven",						395},
	{IAC_NUMPAD8,				"numpadeight",						396},
	{IAC_NUMPAD9,				"numpadnine",						397},
	{IAC_NUMPADADD,				"numpadadd",						398},
	{IAC_CAPSLOCK,				"capslock",							399},
	{IAC_A,						"a",								400},
	{IAC_S,						"s",								401},
	{IAC_D,						"d",								402},
	{IAC_F,						"f",								403},
	{IAC_G,						"g",								404},
	{IAC_H,						"h",								405},
	{IAC_J,						"j",								406},
	{IAC_K,						"k",								407},
	{IAC_L,						"l",								408},
	{IAC_SEMICOLON,				"semicolon",						409},
	{IAC_APOSTROPHE,			"apostrophe",						410},
	{IAC_ENTER,					"enter",							411},
	{IAC_NUMPAD4,				"numpadfour",						412},
	{IAC_NUMPAD5,				"numpadfive",						413},
	{IAC_NUMPAD6,				"numpadsix",						414},
	{IAC_LEFTSHIFT,				"leftshift",						415},
	{IAC_Z,						"z",								416},
	{IAC_X,						"x",								417},
	{IAC_C,						"c",								418},
	{IAC_V,						"v",								419},
	{IAC_B,						"b",								420},
	{IAC_N,						"n",								421},
	{IAC_M,						"m",								422},
	{IAC_COMMA,					"comma",							423},
	{IAC_PERIOD,				"period",							424},
	{IAC_SLASH,					"slash",							425},
	{IAC_RIGHTSHIFT,			"rightshift",						426},
	{IAC_UP,					"up",								427},
	{IAC_NUMPAD1,				"numpadone",						428},
	{IAC_NUMPAD2,				"numpadtwo",						429},
	{IAC_NUMPAD3,				"numpadthree",						430},
	{IAC_NUMPADENTER,			"numpadenter",						431},
	{IAC_LEFTCONTROL,			"leftcontrol",						432},
	{IAC_LEFTALT,				"leftalt",							433},
	{IAC_SPACE,					"space",							434},
	{IAC_RIGHTALT,				"rightalt",							435},
	{IAC_RIGHTCONTROL,			"rightcontrol",						436},
	{IAC_LEFT,					"left",								437},
	{IAC_DOWN,					"down",								438},
	{IAC_RIGHT,					"right",							439},
	{IAC_NUMPAD0,				"numpadzero",						440},
	{IAC_NUMPADDECIMAL,			"numpaddecimal",					441},
	{IAC_NUMPADEQUALS,			"numpadequals",						442},
	{0, "", 0} // end
};

static const unsigned int JOY_AXIS_BEGIN[IAC_MAX_JOYSTICKS] =
{IAC_JOYSTICK1_AXIS_BEGIN,	IAC_JOYSTICK2_AXIS_BEGIN, IAC_JOYSTICK3_AXIS_BEGIN, IAC_JOYSTICK4_AXIS_BEGIN};

static const unsigned int JOY_BUTTON_BEGIN[IAC_MAX_JOYSTICKS]	=
{IAC_JOYSTICK1_BUTTON_BEGIN,	IAC_JOYSTICK2_BUTTON_BEGIN,  IAC_JOYSTICK3_BUTTON_BEGIN,IAC_JOYSTICK4_BUTTON_BEGIN};

static const unsigned int JOY_POV_BEGIN[IAC_MAX_JOYSTICKS][IAC_JOYSTICK_POV_MAX] = {
	{IAC_JOYSTICK1_POV1_N, IAC_JOYSTICK1_POV2_N, IAC_JOYSTICK1_POV3_N, IAC_JOYSTICK1_POV4_N},
	{IAC_JOYSTICK2_POV1_N, IAC_JOYSTICK2_POV2_N, IAC_JOYSTICK2_POV3_N, IAC_JOYSTICK2_POV4_N},
	{IAC_JOYSTICK3_POV1_N, IAC_JOYSTICK3_POV2_N, IAC_JOYSTICK3_POV3_N, IAC_JOYSTICK3_POV4_N},
	{IAC_JOYSTICK4_POV1_N, IAC_JOYSTICK4_POV2_N, IAC_JOYSTICK4_POV3_N, IAC_JOYSTICK4_POV4_N},
};

static const char* MOUSE_AXIS_NAMES[IAC_MOUSE_MAX_AXES]	= {"x", "y", "z" };
static const char* JOY_AXIS_NAMES[IAC_JOYSTICK_MAX_AXES]= {"x", "y", "z", "rx", "ry", "rz", "slider1", "slider2"};
static const char* JOY_POV_NAMES[IAC_JOYSTICK_POV_POS] 	= {"n", "ne", "e", "se", "s", "sw", "w", "nw" };

#define MOUSEBUTTON_FORMAT	"mousebutton%d"
#define MOUSEAXIS_FORMAT 	"mouseaxis%1s"
#define JOYAXIS_FORMAT		"joystick%daxis%15s"
#define JOYBUTTON_FORMAT	"joystick%dbutton%d"
#define JOYPOV_FORMAT		"joystick%dpov%d%2s"

#define JOYAXIS_COMMEMT_FORMAT		"joystick%%daxis%s"
#define JOYBUTTON_COMMEMT_FORMAT	"joystick%%dbutton%d"
#define JOYPOV_COMMEMT_FORMAT		"joystick%%dpov%d%s"

#define MOUSE_WHEEL_UP_STR		"mousewheelup"
#define MOUSE_WHEEL_DOWN_STR	"mousewheeldown"

#define WHEEL_UP_STR			"wheelup"
#define WHEEL_DOWN_STR			"wheeldown"

#define MOUSEBUTTON_LOCID	0
#define MOUSEAXIS_LOCID		1
#define JOYAXIS_LOCID		2
#define JOYBUTTON_LOCID		3
#define JOYPOV_LOCID		4
#define NOKEY_LOCID			5
#define MOUSE_WHEEL_UP_LOCID 	6
#define MOUSE_WHEEL_DOWN_LOCID 	7

#ifdef WIN32
#define snprintf _snprintf
#endif

static void (*localized_string) (const char*, int, char*, int) = NULL;

static void iac_key_name(const iacEntry* table, unsigned int iac, char* name, int maxlen, int pretty);
static void iac_mouse_name(unsigned int iac, char* name, int maxlen, int pretty);
static void iac_joystick_name(unsigned int iac, char* name, int maxlen, int pretty);

static unsigned int iac_key_code(const iacEntry*, const char*);
static unsigned int iac_mouse_code(const char*);
static unsigned int iac_joystick_code(const char*);

static int get_table_index_from_code(const iacEntry*, unsigned int iac);
static int get_table_index_from_name(const iacEntry*, const char* language, const char* name);

int iac_info(int mode, const char* language, const char* keyboard, unsigned int* iac,
			 char* name, int maxlen, imap_iac_info*);

static void iac_key_info(int mode, const char* language, const char* keyboard, unsigned int* iac, char* name, int maxlen);
static void iac_mouse_info(int mode, unsigned int* iac, char* name, int maxlen, int* isAxis);
static void iac_joystick_info(int mode, unsigned int* iac, char* name, int maxlen, imap_iac_info* iacInfo);

/************************************************************************************************
	iacPrettyName
*************************************************************************************************/

void iacPrettyName(unsigned int iac, char* name, int maxlen)
{
	name[0] = '\0';

	if(IAC_IS_KEY(iac))
	{
		const iacEntry* table = NULL;

		if(IAC_IS_US_KEY(iac)) table = USKeyboardTable;

		if(table)
			iac_key_name(table, iac, name, maxlen, 1);
	}
	else if(IAC_IS_MOUSE(iac))
	{
		iac_mouse_name(iac, name, maxlen, 1);
	}
	else if(IAC_IS_JOYSTICK(iac))
	{
		iac_joystick_name(iac, name, maxlen, 1);
	}
	else
	{
		localized_string(NULL, NOKEY_LOCID, name, maxlen);
	}
}

/************************************************************************************************
	iacCommentName
*************************************************************************************************/

void iacCommentName(unsigned int iac, char* name, int maxlen)
{
	name[0] = '\0';

	if(IAC_IS_KEY(iac))
	{
		const iacEntry* table = NULL;

		if(IAC_IS_US_KEY(iac)) table = USKeyboardTable;

		if(table)
			iac_key_name(table, iac, name, maxlen, 0);
	}
	else if(IAC_IS_MOUSE(iac))
	{
		iac_mouse_name(iac, name, maxlen, 0);
	}
	else if(IAC_IS_JOYSTICK(iac))
	{
		iac_joystick_name(iac, name, maxlen, 0);
	}
}

/************************************************************************************************
	iacCode
*************************************************************************************************/

unsigned int iacCode(const char* name)
{
	unsigned int iac = IAC_NOKEY;

	iac = iac_key_code(USKeyboardTable, name);
	if(iac == IAC_NOKEY) iac = iac_mouse_code(name);
	if(iac == IAC_NOKEY) iac = iac_joystick_code(name);

	return iac;
}

/************************************************************************************************
	iacJoystickNumberAndIndex
*************************************************************************************************/

void iacJoystickNumberAndIndex(unsigned int iac, int* joynum, int* idx)
{
	*joynum = 0;
	if(idx) *idx = 0;

	if(IAC_IS_JOYSTICK_BUTTON(iac))
	{
		if		(iac >= IAC_JOYSTICK1_BUTTON_BEGIN 	&& iac < IAC_JOYSTICK1_BUTTON_END) 	*joynum = 1;
		else if	(iac >= IAC_JOYSTICK2_BUTTON_BEGIN 	&& iac < IAC_JOYSTICK2_BUTTON_END) 	*joynum = 2;
		else if	(iac >= IAC_JOYSTICK3_BUTTON_BEGIN 	&& iac < IAC_JOYSTICK3_BUTTON_END) 	*joynum = 3;
		else if	(iac >= IAC_JOYSTICK4_BUTTON_BEGIN 	&& iac < IAC_JOYSTICK4_BUTTON_END) 	*joynum = 4;

		if(*joynum > 0)
		{
			if(idx) *idx = iac - JOY_BUTTON_BEGIN[*joynum-1] + 1;
		}
	}else if(IAC_IS_JOYSTICK_AXIS(iac))
	{
		if		(iac >= IAC_JOYSTICK1_AXIS_BEGIN 	&& iac < IAC_JOYSTICK1_AXIS_END)	*joynum = 1;
		else if	(iac >= IAC_JOYSTICK2_AXIS_BEGIN 	&& iac < IAC_JOYSTICK2_AXIS_END)	*joynum = 2;
		else if	(iac >= IAC_JOYSTICK3_AXIS_BEGIN 	&& iac < IAC_JOYSTICK3_AXIS_END)	*joynum = 3;
		else if	(iac >= IAC_JOYSTICK4_AXIS_BEGIN 	&& iac < IAC_JOYSTICK4_AXIS_END)	*joynum = 4;

		if(*joynum > 0)
		{
			if(idx) *idx = iac - JOY_AXIS_BEGIN[*joynum-1] + 1;
		}
	}else if(IAC_IS_JOYSTICK_POV(iac))
	{
		if		(iac >= IAC_JOYSTICK1_POV_BEGIN 	&& iac < IAC_JOYSTICK1_POV_END)		*joynum = 1;
		else if	(iac >= IAC_JOYSTICK2_POV_BEGIN 	&& iac < IAC_JOYSTICK2_POV_END)		*joynum = 2;
		else if	(iac >= IAC_JOYSTICK3_POV_BEGIN 	&& iac < IAC_JOYSTICK3_POV_END)		*joynum = 3;
		else if	(iac >= IAC_JOYSTICK4_POV_BEGIN 	&& iac < IAC_JOYSTICK4_POV_END)		*joynum = 4;

		if(*joynum > 0)
		{
			int i;
			for(i = 0; i < IAC_JOYSTICK_POV_MAX; i++)
			{
				if(iac >= JOY_POV_BEGIN[*joynum-1][i] && iac < JOY_POV_BEGIN[*joynum-1][i] + IAC_JOYSTICK_POV_POS)
				{
					if(idx) *idx = i + 1;
					break;
				}
			}
		}
	}
}

/************************************************************************************************
	iacSetLocalizationCallback
*************************************************************************************************/

void iacSetLocalizationCallback(void (*func)(const char* language, int locid, char*, int maxlen))
{
	localized_string = func;
}

/************************************************************************************************
	iac_format
*************************************************************************************************/

static void iac_format(char* name, int maxlen, const wchar_t* format, ...)
{
	if(!name || !format || !wcslen(format))
		return;

	*name = 0;

	va_list marker;
	va_start(marker, format);

	wchar_t buffer[MAX_IAC_NAME+1];
	std::wstring f = format;
	std::wstring::size_type pos(0);

	while((pos = f.find(L"%s")) != std::string::npos)
		f.replace(pos, 3, L"%ls");

#if platOS == OS_MACOS
	vswprintf
#else
	vswprintf_s
#endif
	(buffer, MAX_IAC_NAME, f.c_str(), marker);

	strncpy(name, UTF8(buffer), maxlen);

	va_end(marker);
}

/************************************************************************************************
	iac_key_name
*************************************************************************************************/

static void iac_key_name(const iacEntry* table, unsigned int iac, char* name, int maxlen, int pretty)
{
	int i = get_table_index_from_code(table, iac);

	if(i > -1)
	{
		if(pretty)
			localized_string(NULL, table[i].locID, name, maxlen);
		else
			localized_string("english", table[i].locID, name, maxlen);
			//strncpy(name, table[i].name, maxlen);
	}
}

/************************************************************************************************
	iac_mouse_name
*************************************************************************************************/

static void iac_mouse_name(unsigned int iac, char* name, int maxlen, int pretty)
{
	unsigned int idx;
	char formatStr[MAX_IAC_NAME+1];
	formatStr[0] = 0;

	if(IAC_IS_MOUSE_AXIS(iac))
	{
		if(pretty)
			localized_string(NULL, MOUSEAXIS_LOCID, formatStr, maxlen);
		else
			strncpy(formatStr, MOUSEAXIS_FORMAT, maxlen);

		idx = iac - IAC_MOUSE_AXIS_BEGIN;

		if(idx < IAC_MOUSE_MAX_AXES)
			iac_format(name, maxlen, UTF16(formatStr), UTF16(MOUSE_AXIS_NAMES[idx]));
	}
	else if(IAC_IS_MOUSE_BUTTON(iac))
	{
		if(pretty)
			localized_string(NULL, MOUSEBUTTON_LOCID, formatStr, maxlen);
		else
			strncpy(formatStr, MOUSEBUTTON_FORMAT, maxlen);

		idx = iac - IAC_MOUSE_BUTTON_BEGIN;

		if(idx < IAC_MOUSE_MAX_BUTTONS)
			iac_format(name, maxlen, UTF16(formatStr), idx+1);
	}
	else if(IAC_IS_MOUSE_WHEEL(iac))
	{
		if(pretty)
		{
			localized_string(NULL, (iac == IAC_MOUSE_WHEEL_UP) ? MOUSE_WHEEL_UP_LOCID : MOUSE_WHEEL_DOWN_LOCID,
								name, maxlen);
		}
		else
		{
			strncpy(name, (iac == IAC_MOUSE_WHEEL_UP) ? MOUSE_WHEEL_UP_STR : MOUSE_WHEEL_DOWN_STR, maxlen);
		}
	}
}

/************************************************************************************************
	iac_joystick_name
*************************************************************************************************/

static void iac_joystick_name(unsigned int iac, char* name, int maxlen, int pretty)
{
	int joynum, idx;
	char formatStr[MAX_IAC_NAME+1];
	formatStr[0] = 0;

	if(iac == IAC_JOYSTICK_TRACK_IR_X)
	{
		strncpy(name,"Track IR X", maxlen);
		return;
	}
	else if(iac == IAC_JOYSTICK_TRACK_IR_Y)
	{
		strncpy(name,"Track IR Y", maxlen);
		return;
	}
	else if(iac == IAC_JOYSTICK_TRACK_IR_Z)
	{
		strncpy(name,"Track IR Z", maxlen);
		return;
	}
	else if(iac == IAC_JOYSTICK_TRACK_IR_TX)
	{
		strncpy(name,"Track IR X Trans", maxlen);
		return;
	}
	else if(iac == IAC_JOYSTICK_TRACK_IR_TY)
	{
		strncpy(name,"Track IR Y Trans", maxlen);
		return;
	}
	else if(iac == IAC_JOYSTICK_TRACK_IR_TZ)
	{
		strncpy(name,"Track IR Z Trans", maxlen);
		return;
	}

	iacJoystickNumberAndIndex(iac, &joynum, &idx);

	if(joynum < 1 || idx < 1) return;

	if(IAC_IS_JOYSTICK_AXIS(iac))
	{
		const char* joyname = JOY_AXIS_NAMES[idx-1];

		if(pretty)
		{
			localized_string(NULL, JOYAXIS_LOCID, formatStr, maxlen);
			iac_format(name, maxlen, UTF16(formatStr), joynum, UTF16(joyname));
		}
		else
		{
			snprintf(name, maxlen, JOYAXIS_COMMEMT_FORMAT, joyname);
		}
	}
	else if(IAC_IS_JOYSTICK_BUTTON(iac))
	{
		if(pretty)
		{
			localized_string(NULL, JOYBUTTON_LOCID, formatStr, maxlen);
			iac_format(name, maxlen, UTF16(formatStr), joynum, idx);
		}
		else
		{
			snprintf(name, maxlen, JOYBUTTON_COMMEMT_FORMAT, idx);
		}
	}
	else if(IAC_IS_JOYSTICK_POV(iac))
	{
		const char* povname = JOY_POV_NAMES[iac - JOY_POV_BEGIN[joynum-1][idx-1]];

		if(pretty)
		{
		 	localized_string(NULL, JOYPOV_LOCID, formatStr, maxlen);
			iac_format(name, maxlen, UTF16(formatStr), joynum, idx, UTF16(povname));
		}
		else
		{
			snprintf(name, maxlen, JOYPOV_COMMEMT_FORMAT, idx, povname);
		}
	}

}

/************************************************************************************************
	iac_key_code
*************************************************************************************************/

static unsigned int iac_key_code(const iacEntry* table, const char* name)
{
	unsigned int iac = IAC_NOKEY;
	int i;

	i = get_table_index_from_name(table, "english", name);

	if(i > -1)
		iac = table[i].iac;

	return iac;
}

/************************************************************************************************
	iac_mouse_code
*************************************************************************************************/

static unsigned int iac_mouse_code(const char* name)
{
	char axis[2] = "";
	int buttonnum = 0;
	unsigned int i;

	// button
	sscanf(name, MOUSEBUTTON_FORMAT, &buttonnum);
	if(buttonnum > 0 && buttonnum <= IAC_MOUSE_MAX_BUTTONS)
		return IAC_MOUSE_BUTTON_BEGIN + buttonnum - 1;

	// axis
	sscanf(name, MOUSEAXIS_FORMAT, axis);
	for(i = 0; i < IAC_MOUSE_MAX_AXES; i++)
	{
		if(!strcmp(axis, MOUSE_AXIS_NAMES[i]))
			return IAC_MOUSE_AXIS_BEGIN + i;
	}

	// mouse wheel
	if(!strcmp(name, MOUSE_WHEEL_UP_STR))
		return IAC_MOUSE_WHEEL_UP;

	if(!strcmp(name, MOUSE_WHEEL_DOWN_STR))
		return IAC_MOUSE_WHEEL_DOWN;

	return IAC_NOKEY;
}

/************************************************************************************************
	iac_joystick_code
*************************************************************************************************/

static unsigned int iac_joystick_code(const char* name)
{
	unsigned joynum = 0, idx = 0;
	char str[16] = "";
	int i;

	if(!strcmpiws(name, "trackirx"))
	{
		return IAC_JOYSTICK_TRACK_IR_X;
	}
	else if(!strcmpiws(name, "trackiry"))
	{
		return IAC_JOYSTICK_TRACK_IR_Y;
	}
	else if(!strcmpiws(name, "trackirz"))
	{
		return IAC_JOYSTICK_TRACK_IR_Z;
	}
	else if(!strcmpiws(name, "trackirxtrans"))
	{
		return IAC_JOYSTICK_TRACK_IR_TX;
	}
	else if(!strcmpiws(name, "trackirytrans"))
	{
		return IAC_JOYSTICK_TRACK_IR_TY;
	}
	else if(!strcmpiws(name, "trackirztrans"))
	{
		return IAC_JOYSTICK_TRACK_IR_TZ;
	}

	// button
	sscanf(name, JOYBUTTON_FORMAT, &joynum, &idx);
	if(joynum > 0 && joynum <= IAC_MAX_JOYSTICKS && idx > 0 && idx <= IAC_JOYSTICK_MAX_BUTTONS)
		return JOY_BUTTON_BEGIN[joynum-1] + idx - 1;

	// axis
	sscanf(name, JOYAXIS_FORMAT, &joynum, str);
	if(joynum > 0 && joynum <= IAC_MAX_JOYSTICKS)
	{
		for(unsigned int i = 0; i < IAC_JOYSTICK_MAX_AXES; i++)
		{
			if(!strcmp(str, JOY_AXIS_NAMES[i])) return JOY_AXIS_BEGIN[joynum-1] + i;
		}
	}

	// pov
	sscanf(name, JOYPOV_FORMAT, &joynum, &idx, str);
	if(joynum > 0 && joynum <= IAC_MAX_JOYSTICKS && idx > 0 && idx <= IAC_JOYSTICK_POV_MAX)
	{
		for(i = 0; i < IAC_JOYSTICK_POV_POS; i++)
		{
			if(!strcmp(str, JOY_POV_NAMES[i])) return JOY_POV_BEGIN[joynum-1][idx-1] + i;
		}
	}

	return IAC_NOKEY;
}

/************************************************************************************************
	get_table_index_from_code
*************************************************************************************************/

static int get_table_index_from_code(const iacEntry* table, unsigned int iac)
{
	int i;

	for(i = 0; table[i].iac != IAC_NOKEY; i++)
	{
		if(iac == table[i].iac)
			return i;
	}

	return -1;
}

/************************************************************************************************
	get_table_index_from_name
*************************************************************************************************/

static int get_table_index_from_name(const iacEntry* table, const char* language, const char* name)
{
	char str[MAX_IAC_NAME+1];

	for(int i = 0; table[i].iac != 0; i++)
	{
		if(language == NULL)
		{
			if(!strcmpiws(name, table[i].name))
				return i;
		}
		else
		{
			localized_string("english", table[i].locID, str, MAX_IAC_NAME);

			if(!strcmpiws(str, name))
			{
				return i;
			}

		}
	}

	return -1;
}

/************************************************************************************************
	iac_info
*************************************************************************************************/

int iac_info(int mode, const char* language, const char* keyboard, unsigned int* iac, char* name, int maxlen,
					imap_iac_info* iacInfo)
{
	if(mode == IMAP_HAVE_CODE)
	{
		if(name) name[0] = '\0';

		// Type
		if(IAC_IS_MOUSE(*iac))
			iacInfo->deviceType = IMAP_IAC_IS_MOUSE;
		else if(IAC_IS_JOYSTICK(*iac))
			iacInfo->deviceType = IMAP_IAC_IS_JOYSTICK;
		else
			iacInfo->deviceType = IMAP_IAC_IS_KEYBOARD;

		// Name
		switch(iacInfo->deviceType)
		{
			case IMAP_IAC_IS_KEYBOARD:
				iacInfo->deviceIdx = 0;
				iacInfo->isAxis = 0;
				iacInfo->axisType = 0;
				iac_key_info(mode, language, keyboard, iac, name, maxlen);
			break;

			case IMAP_IAC_IS_MOUSE:
				iacInfo->deviceIdx = 0;
				iac_mouse_info(mode, iac, name, maxlen, &iacInfo->isAxis);
				iacInfo->axisType = (iacInfo->isAxis) ? IMAP_DELTA_AXIS : 0;
			break;

			case IMAP_IAC_IS_JOYSTICK:
				iac_joystick_info(mode, iac, name, maxlen, iacInfo);
			break;
		}
	}else
	{
		*iac = IAC_NOKEY;

		switch(iacInfo->deviceType)
		{
			case IMAP_IAC_IS_KEYBOARD:
				iac_key_info(mode, language, keyboard, iac, name, maxlen);
			break;

			case IMAP_IAC_IS_MOUSE:
				iac_mouse_info(mode, iac, name, maxlen, &iacInfo->isAxis);
			break;

			case IMAP_IAC_IS_JOYSTICK:
				iac_joystick_info(mode, iac, name, maxlen, iacInfo);
			break;
		}
	}

	return 0;
}

/************************************************************************************************
	iac_key_info
*************************************************************************************************/

static void iac_key_info(int mode, const char* language, const char* keyboard, unsigned int* iac, char* name, int maxlen)
{
	int i;

	if(mode == IMAP_HAVE_CODE)
	{
		if(name)
		{
			i = get_table_index_from_code(USKeyboardTable, *iac);
			if(i > -1)
				localized_string("english", USKeyboardTable[i].locID, name, maxlen);
		}
	}
	else
	{
		i = get_table_index_from_name(USKeyboardTable, "english", name);
		if(i > -1)
			*iac = USKeyboardTable[i].iac;
	}
}

/************************************************************************************************
	iac_mouse_info
*************************************************************************************************/

static void iac_mouse_info(int mode, unsigned int* iac, char* name, int maxlen, int* isAxis)
{
	if(mode == IMAP_HAVE_CODE)
	{

		*isAxis = IAC_IS_MOUSE_AXIS(*iac);

		if(name)
		{
			name[0] = '\0';

			if(*isAxis)
				snprintf(name, maxlen, "%s", MOUSE_AXIS_NAMES[*iac - IAC_MOUSE_AXIS_BEGIN]);
			else if(IAC_IS_MOUSE_BUTTON(*iac))
				snprintf(name, maxlen, "%d", *iac - IAC_MOUSE_BUTTON_BEGIN + 1);
			else if(IAC_IS_MOUSE_WHEEL(*iac))
				snprintf(name, maxlen, (*iac == IAC_MOUSE_WHEEL_UP) ? WHEEL_UP_STR : WHEEL_DOWN_STR);
		}
	}
	else // IMAP_HAVE_NAME
	{
		*iac = IAC_NOKEY;

		if(*isAxis)
		{
			for(unsigned int i = 0; i < IAC_MOUSE_MAX_AXES; i++)
			{
				if(!strcmpiws(name, MOUSE_AXIS_NAMES[i]))
				{
					*iac = IAC_MOUSE_AXIS_BEGIN + i;
					break;
				}
			}
		}
		else
		{
			int idx = atoi(name);
			if(idx > 0)
			{
				*iac = IAC_MOUSE_BUTTON_BEGIN + idx - 1;
			}else
			{
				if(!strcmpiws(name, WHEEL_UP_STR))
					*iac = IAC_MOUSE_WHEEL_UP;
				else if(!strcmpiws(name, WHEEL_DOWN_STR))
					*iac = IAC_MOUSE_WHEEL_DOWN;
			}
		}
	}
}

/************************************************************************************************
	iac_joystick_info
*************************************************************************************************/

static void iac_joystick_info(int mode, unsigned int* iac, char* name, int maxlen, imap_iac_info* iacInfo)
{
	if(mode == IMAP_HAVE_CODE)
	{
		int idx = 0;

		if(IAC_IS_TRACK_IR(*iac))
		{
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			if(name)
			{
				if(*iac == IAC_JOYSTICK_TRACK_IR_X)
					strncpy(name, "trackirx", maxlen);
				else if(*iac == IAC_JOYSTICK_TRACK_IR_Y)
					strncpy(name, "trackiry", maxlen);
				else if(*iac == IAC_JOYSTICK_TRACK_IR_Z)
					strncpy(name, "trackirz", maxlen);
				else if(*iac == IAC_JOYSTICK_TRACK_IR_TX)
					strncpy(name, "trackirxtrans", maxlen);
				else if(*iac == IAC_JOYSTICK_TRACK_IR_TY)
					strncpy(name, "trackirytrans", maxlen);
				else if(*iac == IAC_JOYSTICK_TRACK_IR_TZ)
					strncpy(name, "trackirztrans", maxlen);
			}
			return;
		}

		iacJoystickNumberAndIndex(*iac, &iacInfo->deviceIdx, &idx);

		if(name) name[0] = '\0';

		if(iacInfo->deviceIdx > 0 && idx > 0)
		{
			if(IAC_IS_JOYSTICK_AXIS(*iac))
			{
				if(name) snprintf(name, maxlen, "%s", JOY_AXIS_NAMES[idx-1]);
				iacInfo->isAxis = 1;
				iacInfo->axisType = IMAP_SYMMETRIC_AXIS; // how to check for asymmetric axes?
			}
			else if(IAC_IS_JOYSTICK_BUTTON(*iac))
			{
				if(name) snprintf(name, maxlen, "%d", idx);
				iacInfo->isAxis = iacInfo->axisType = 0;
			}else if(IAC_IS_JOYSTICK_POV(*iac))
			{
				if(name) snprintf(name, maxlen, "POV%d%s", idx, JOY_POV_NAMES[*iac - JOY_POV_BEGIN[iacInfo->deviceIdx-1][idx-1]]);
				iacInfo->isAxis = iacInfo->axisType = 0;
			}
		}
	}else // IMAP_HAVE_NAME
	{
		int i;

		if(!strcmpiws("trackirx", name))
		{
			*iac = IAC_JOYSTICK_TRACK_IR_X;
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			return;
		}else if(!strcmpiws("trackiry", name))
		{
			*iac = IAC_JOYSTICK_TRACK_IR_Y;
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			return;
		}else if(!strcmpiws("trackirz", name))
		{
			*iac = IAC_JOYSTICK_TRACK_IR_Z;
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			return;
		}else if(!strcmpiws("trackirxtrans", name))
		{
			*iac = IAC_JOYSTICK_TRACK_IR_TX;
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			return;
		}else if(!strcmpiws("trackirytrans", name))
		{
			*iac = IAC_JOYSTICK_TRACK_IR_TY;
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			return;
		}else if(!strcmpiws("trackirztrans", name))
		{
			*iac = IAC_JOYSTICK_TRACK_IR_TZ;
			iacInfo->isAxis = 1;
			iacInfo->axisType = IMAP_SYMMETRIC_AXIS;
			iacInfo->deviceIdx = 1;
			return;
		}

		*iac = IAC_NOKEY;
		if(iacInfo->deviceIdx < 1) return;

		if(iacInfo->isAxis)
		{
			for(unsigned i = 0; i < IAC_JOYSTICK_MAX_AXES; i++)
			{
				if(!strcmpiws(name, JOY_AXIS_NAMES[i]))
				{
					*iac = JOY_AXIS_BEGIN[iacInfo->deviceIdx-1] + i;
					break;
				}
			}
		}else
		{
			int idx = atoi(name);
			if(idx > 0 && idx <= IAC_JOYSTICK_MAX_BUTTONS ) // button
			{
				*iac = JOY_BUTTON_BEGIN[iacInfo->deviceIdx-1] + idx - 1;
			}else // pov then?
			{
				char* s = strstriws(name, "POV");

				if(s)
				{
					idx = s[0] - '0'; s++;
					if(idx > 0 && idx <= IAC_JOYSTICK_POV_MAX)
					{
						char povpos[3] = "";
						strncpy(povpos, s, 2);

						for(i = 0; i < IAC_JOYSTICK_POV_POS; i++)
						{
							if(!strcmpiws(povpos, JOY_POV_NAMES[i]))
							{
								*iac = JOY_POV_BEGIN[iacInfo->deviceIdx-1][idx-1] + i;
								break;
							}
						}
					}
				}
			}
		}
	}
}
