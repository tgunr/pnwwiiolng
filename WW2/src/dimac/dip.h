#pragma once
// Copyright 2001 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

#define dipKEYDOWN(name,key) (name[key] & 0x80)

// macintosh virtual key definitions
// from inside mac vol 5

#define DIK_A		0
#define DIK_S		1
#define DIK_D		2
#define DIK_F		3
#define DIK_H		4
#define DIK_G		5
#define DIK_Z		6
#define DIK_X		7
#define DIK_C		8
#define DIK_V		9
//#define DIK_¤		10
#define DIK_B		11
#define DIK_Q		12
#define DIK_W		13
#define DIK_E		14
#define DIK_R		15
#define DIK_Y		16
#define DIK_T		17
#define DIK_1		18
#define DIK_2		19
#define DIK_3		20
#define DIK_4		21
#define DIK_6		22
#define DIK_5		23
#define DIK_EQUALS	24
#define DIK_9		25
#define DIK_7		26
#define DIK_MINUS	27
#define DIK_8		28
#define DIK_0		29
#define DIK_RIGHTBRACKET		30
#define DIK_RBRACKET			30
#define DIK_O		31
#define DIK_U		32
#define DIK_LEFTBRACKET		33
#define DIK_LBRACKET		33
#define DIK_I		34
#define DIK_P		35
#define DIK_Return	36
#define DIK_L		37
#define DIK_J		38
#define DIK_QUOTE	39
#define DIK_APOSTROPHE 39
#define DIK_K		40
#define DIK_SEMICOLON	41
#define DIK_BACKSLASH	42
#define DIK_DIVIDE DIK_BACKSLASH
#define DIK_COMMA	43
#define DIK_SLASH	44
#define DIK_N		45
#define DIK_M		46
#define DIK_PERIOD		47
#define DIK_TAB		48
#define DIK_SPACE	49
#define DIK_BACKTICK	50
#define DIK_GRAVE	50
#define DIK_DELETE	51
#define DIK_ENTER	52
#define DIK_ESCAPE	53
#define DIK_LCOMMAND	54
#define DIK_RCOMMAND	55
#define DIK_COMMAND	55
#define DIK_LSHIFT	56
#define DIK_CAPITAL	57
#define DIK_OPTION	58
#define DIK_LMENU	DIK_OPTION
#define DIK_LCONTROL	59
#define DIK_RSHIFT	60
#define DIK_ROPTION	61
#define DIK_RCONTROL	62
#define DIK_PADDOT	65
#define DIK_DECIMAL 65
#define DIK_PADRIGHT	66
#define DIK_PADMULT	67
#define DIK_ADD	69 // apparently no 68...
#define DIK_PADLEFT	70
#define DIK_CLEAR	71
#define DIK_NUMLOCK 71 // two names for the same key
#define DIK_PADDOWN	72
#define DIK_PADSLASH	75
#define DIK_PADENTER	76
#define DIK_NUMPADENTER DIK_PADENTER
#define DIK_PADUP	77
#define DIK_SUBTRACT	78
#define DIK_PADEQUALS	81
#define DIK_NUMPAD0	82
#define DIK_NUMPAD1	83
#define DIK_NUMPAD2	84
#define DIK_NUMPAD3	85
#define DIK_NUMPAD4	86
#define DIK_NUMPAD5	87
#define DIK_NUMPAD6	88
#define DIK_NUMPAD7	89
#define DIK_NUMPAD8	91
#define DIK_NUMPAD9	92	// Pad 9 from outer space...
#define DIK_F5		96
#define DIK_F6		97
#define DIK_F7		98
#define DIK_F3		99
#define DIK_F8		100
#define DIK_F9		101
#define DIK_F11		103
#define DIK_F13		105
#define DIK_F14		107
#define DIK_SCROLL	107	// two names for same key
#define DIK_F10		109
#define DIK_F12		111
#define DIK_F15		113
#define DIK_PAUSE	DIK_F15
#define DIK_HELP	114
#define DIK_INSERT	114
#define DIK_HOME	115
#define DIK_PGUP	116
#define DIK_FWDDELETE	117
#define DIK_F4		118
#define DIK_END		119
#define DIK_F2		120
#define DIK_PGDN	121
#define DIK_F1		122
#define DIK_LEFT	123
#define DIK_RIGHT	124
#define DIK_DOWN	125
#define DIK_UP		126
#define DIK_POWER	127

#ifdef __cplusplus
extern "C" {
#endif

// fills the buffer pointed to by buffer with single bits indicating that
// the key is down. an 0x80 indicates down, 0x7f indicates up.
LOGICAL dipGetKeyboardState(int bufsize, char * buffer);
LOGICAL dipInit(void);
LOGICAL dipSetupKeyboard(void);
LOGICAL dipSetupMouse(void);
LOGICAL dipSetupJoystick(void);
void dipKill(void);

int dipHasKeyboard(void) ;
int dipHasJoystick(void) ;
int dipHasMouse(void) ;

#ifdef __cplusplus
}
#endif

