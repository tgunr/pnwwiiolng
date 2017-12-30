#include "syscfg.h" // Path is project specific.
#include <string.h>

#include "imap/ww2func.h"
#include "imap/ww2imap.h"
#include "imap/imap.h"
#include "str/striws.h"
#include "veh/vehType.h"

/************************************************************************************************
	ww2func
*************************************************************************************************/

struct functionEntry
{
	unsigned int functionCode;
	const char* vehcntrlName;
	int locID;
	int isAxis;
	int categoryMask;
	unsigned int flags;
	int headerLocID;
	int ezLevel;
};

enum
{
	WW2_FUNC_HEADER_EZ = 2000
,	WW2_FUNC_HEADER_MOVEMENT
,	WW2_FUNC_HEADER_VIEW
,	WW2_FUNC_HEADER_EMOTE
,	WW2_FUNC_HEADER_WEAPON
,	WW2_FUNC_HEADER_COCKPIT
,	WW2_FUNC_HEADER_TORPEDO
,	WW2_FUNC_HEADER_BOMBSIGHT
,	WW2_FUNC_HEADER_CHAT
,	WW2_FUNC_HEADER_INGAME
,	WW2_FUNC_HEADER_EXTERNAL
};

enum
{
	WW2_FUNC_GENERAL= 0x00000001,
	WW2_FUNC_VIEW 	= 0x00000002,
	WW2_FUNC_AIR 	= 0x00000004,
	WW2_FUNC_TANK 	= 0x00000008,
	WW2_FUNC_TRUCK 	= 0x00000010,
	WW2_FUNC_ATGUN 	= 0x00000020,
	WW2_FUNC_PERSON = 0x00000040,
	WW2_FUNC_SEA 	= 0x00000080,
	WW2_FUNC_EZ 	= 0x00000100,

	WW2_FUNC_VEHICLE_SPECIFIC = 0x01000000,

	WW2_FUNC_GROUND = WW2_FUNC_TANK | WW2_FUNC_TRUCK | WW2_FUNC_ATGUN,
	WW2_FUNC_MOTORIZED_GROUND = WW2_FUNC_TANK | WW2_FUNC_TRUCK,
	WW2_FUNC_COMBAT_GROUND = WW2_FUNC_TANK | WW2_FUNC_ATGUN,

	WW2_FUNC_ALL_VEHICLES = WW2_FUNC_AIR | WW2_FUNC_GROUND | WW2_FUNC_SEA,
	WW2_FUNC_ALL_COMBAT_VEHICLES = WW2_FUNC_COMBAT_GROUND | WW2_FUNC_AIR | WW2_FUNC_SEA,

	WW2_FUNC_ALL_PERSONA_TYPES = WW2_FUNC_ALL_VEHICLES | WW2_FUNC_PERSON,
	WW2_FUNC_ALL_COMBAT_PERSONA_TYPES = WW2_FUNC_ALL_COMBAT_VEHICLES | WW2_FUNC_PERSON,

	WW2_FUNC_ALL = WW2_FUNC_GENERAL | WW2_FUNC_VIEW | WW2_FUNC_EZ | WW2_FUNC_ALL_PERSONA_TYPES
};

static const functionEntry functionTable[] =
{
	{IMAP_TMP_EXIT_VEHICLE,	 					"exitvehicle",					239,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_SHOW_GUI_CURSOR,					"guicursor",					201,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_SHOW_IN_GAME_MAP,					"showingamemap",				731,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_TOGGLE_HUD,						"changehudmode",				708,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_RESET_HUD,						"resethud",						710,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_TOGGLE_CHATHEIGHT,				"changechatheight",				709,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_UI_MAP_ZOOMIN,					"mapin",						732,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_UI_MAP_ZOOMOUT,					"mapout",						733,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_CYCLE_ICON_MODE,						"cycleiconmode",				8,				0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_SHOW_STATS,							"showstats",					6,				0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_SHOW_INFO,							"showinfo",						5,				0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_MOM_TAKE_SCREEN_SHOT,					"screenshot",					704,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_CENTER_AXIS,							"centeraxis",					4,				0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_CONTINUE_TUTORIAL,					"continuetutorial",				9,				0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},
	{IMAP_CLONE_VEHICLE,						"clonevehicle",					10,				0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_INGAME, -1},

	{IMAP_MOM_EXTERNAL_LOCKED,					"externallocked",				196,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS,	"toggleinternalexternalviews",	183,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_MOM_EXTERNAL_RUBBERBAND,				"externalrubberband",			197,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_MOM_EXTERNAL_FIXED,					"externalfixed",				198,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_MOM_SET_FIXED_CAMERA,					"setfixedcamera",				199,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_SLEWVIEWLEFT,					"slewviewleft",					243,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_SLEWVIEWRIGHT,					"slewviewright",				244,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_SLEWVIEWUP,						"slewviewup",					245,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_SLEWVIEWDOWN,					"slewviewdown",					246,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_SLEWVIEWIN,						"slewviewin",					316,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_SLEWVIEWOUT,						"slewviewout",					317,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_TMP_CHANGE_FOV,						"changefov",					236,			0,	WW2_FUNC_GENERAL, 	0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_TMP_CHANGE_FOV_UP,					"changefovup",					-1,				0,	0, 					0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_TMP_CHANGE_FOV_DOWN,					"changefovdown",				-1,				0,	0, 					0, WW2_FUNC_HEADER_EXTERNAL, -1},
	{IMAP_HOLD_FP_FORWARD,						"fpforward",					261,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_BACKWARD,						"fpbackward",					262,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_STRAFE_LEFT,					"fpstrafeleft",					270,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_STRAFE_RIGHT,					"fpstraferight",				269,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_CROUCH,						"fpcrouch",						263,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_PRONE,						"fpprone",						259,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_SPRINT,						"fpfast",						264,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_AUTORUN,						"fpautorun",					265,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_HOLD_FP_WALK,							"fpwalk",						267,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_MOM_FP_RIDE_UNRIDE_VEHICLE,			"fprideunridevehicle",			287,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_ASX_FP_TORSO_VERTICAL_AXIS,			"fptorsoverticalaxis",			314,			1,	WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_MOVEMENT, -1},
	{IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS,			"fptorsohorizontalaxis",		315,			1,	WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_MOVEMENT, -1},

	{IMAP_HOLD_FP_LOOK_LEFT,					"fplookleft",					274,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_HOLD_FP_LOOK_RIGHT,					"fplookright",					275,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_VIEW, -1},

	{IMAP_TOGGLE_MOUSE_LOOK,					"togglemouselook",				917,			0,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_MOUSE_LOOK_X,							"mouselookx",					918,			1,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_MOUSE_LOOK_Y,							"mouselooky",					919,			1,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_MOUSE_LOOK_Z,							"mouselookz",					920,			1,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_VIEW, -1},

	{IMAP_LOOK_FORWARD,							"lookviewone",					204,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_LEFT,							"lookvieweight",				211,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_RIGHT,							"lookviewseven",				210,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_RIGHT,					"lookviewfive",					208,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_LEFT,					"lookviewsix",					209,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK_RIGHT,						"lookviewnine",					212,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK_LEFT,						"lookviewten",					213,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_UP,								"lookviewthree",				206,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_DOWN,							"lookviewfour",					207,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK,							"lookviewtwo",					845,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_UP,						"lookvieweleven",				214,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_RIGHT_UP,				"lookviewtwelve",				215,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_LEFT_UP,					"lookviewthirteen",				216,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_RIGHT_UP,						"lookviewfourteen",				217,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_LEFT_UP,							"lookviewfifteen",				218,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK_RIGHT_UP,					"lookviewsixteen",				219,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK_LEFT_UP,					"lookviewseventeen",			220,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
//	{IMAP_LOOK_FORWARD_DOWN,					"lookvieweighteen",				221,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_RIGHT_DOWN,				"lookviewnineteen",				222,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_FORWARD_LEFT_DOWN,				"lookviewtwenty",				223,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_RIGHT_DOWN,						"lookviewtwentyone",			224,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_LEFT_DOWN,						"lookviewtwentytwo",			225,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK_RIGHT_DOWN,					"lookviewtwentythree",			226,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},
	{IMAP_LOOK_BACK_LEFT_DOWN,					"lookviewtwentyfour",			227,			0,	WW2_FUNC_VIEW, 0, WW2_FUNC_HEADER_VIEW, -1},

//	{IMAP_HEAD_TRANSLATE_X,						"headtransx",					923,			1,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_VIEW, -1},
//	{IMAP_HEAD_TRANSLATE_Y,						"headtransy",					924,			1,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_VIEW, -1},
//	{IMAP_HEAD_TRANSLATE_Z,						"headtransz",					925,			1,	WW2_FUNC_VIEW|WW2_FUNC_PERSON, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_VIEW, -1},

	{IMAP_HOLD_FP_FIRE,							"fpfire",						288,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_GUNSIGHT,						"fpgunsight",					276,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_NEXT_WEAPON,					"fpnextweapon",					1100,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_PREV_WEAPON,					"fpprevweapon",					1101,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_1,						"fpweapon0",					277,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_2,						"fpweapon1",					278,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_3,						"fpweapon2",					279,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_4,						"fpweapon3",					280,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_5,						"fpweapon4",					281,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_6,						"fpweapon5",					282,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_7,						"fpweapon6",					283,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_8,						"fpweapon7",					284,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_9,						"fpweapon8",					285,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_WEAPON_10,					"fpweapon9",					286,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_RELOAD,						"fpreload",						273,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_DEPLOY,						"fpdeploy",						260,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_FP_RELAXED,						"fprelaxed",					266,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},

	{IMAP_ASX_PITCH,							"elevator",					290,			1,	WW2_FUNC_AIR, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_ROLL,								"aileron",					289,			1,	WW2_FUNC_AIR, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_YAW,								"rudder",					291,			1,	WW2_FUNC_AIR, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_THROTTLE,							"throttle",					292,			1,	WW2_FUNC_AIR, WW2_FUNC_ASYMMETRIC_AXIS, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_FLAPCONTROL,						"flapcontrol",				302,			1,	WW2_FUNC_AIR, WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_ELEV_TRIM,						"elevtrim",					309,			1,	WW2_FUNC_AIR, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_KP, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_AILERON_TRIM,						"ailerontrim",				310,			1,	WW2_FUNC_AIR, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_KP, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_ASX_RUDDER_TRIM,						"ruddertrim",				311,			1,	WW2_FUNC_AIR, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_KP, WW2_FUNC_HEADER_COCKPIT, -1},

	{IMAP_MOM_ADJUST_PROP_UP,					"adjustpropup",				193,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_COCKPIT, 0},
	{IMAP_MOM_ADJUST_PROP_DOWN,					"adjustpropdown",			194,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_COCKPIT, 0},
	{IMAP_MOM_TOGGLEWEP,						"togglewep",				195,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_MOM_TOGGLE_TAILLOCK,					"toggletaillock",			185,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_COCKPIT, 1},

	// axes
	{IMAP_ASX_STEERING,							"steering",					306,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_SEA, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_ATGUN_MOVEMENT_LEFTRIGHT,			"atgunleftright",    		531,			1,	WW2_FUNC_ATGUN, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_ATGUN_MOVEMENT_FORWARDBACKWARDS,	"atgunforwardbackwards",	532,			1,	WW2_FUNC_ATGUN, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},

	{IMAP_ASX_GAS,								"gas",						293,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_SEA, WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_LEFTTRACKCLUTCH,					"lefttrackclutch",			295,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN, 	WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_RIGHTTRACKCLUTCH,					"righttrackclutch",			296,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN, 	WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_LEFTBRAKE,						"leftbrake",				303,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_AIR, 	WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_RIGHTBRAKE,						"rightbrake",				304,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_AIR, 	WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_ASX_CENTERBRAKE,						"bothbrake",				305,			1,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN, WW2_FUNC_ASYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_COCKPIT, 1},

	{IMAP_MOM_SHIFTUP,							"shiftup",						189,			0,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_MOM_SHIFTDOWN,						"shiftdown",					190,			0,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_MOM_REVERSE,							"reverse",						191,			0,	WW2_FUNC_MOTORIZED_GROUND | WW2_FUNC_ATGUN | WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_COCKPIT, 1},
	{IMAP_MOM_STARTER_ON,						"engstart",						184,			0,	WW2_FUNC_ALL_VEHICLES | WW2_FUNC_ATGUN , 0, WW2_FUNC_HEADER_COCKPIT, 1},

// position
	{IMAP_MOM_POSITION_1,						"positionone",					249,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_2,						"positiontwo",					250,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_3,						"positionthree",				251,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_4,						"positionfour",					252,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_5,						"positionfive",					253,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_6,						"positionsix",					254,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_7,						"positionseven",				255,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_8,						"positioneight",				256,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_9,						"positionnine",					257,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},
	{IMAP_MOM_POSITION_10,						"positionten",					258,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_COCKPIT, -1},

	{IMAP_MOM_TOGGLE_DEPLOYWEAPON,				"deployweapon",					187,			0,	WW2_FUNC_ALL_COMBAT_VEHICLES, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_MOM_TOGGLE_BOMBBAYDOOR,				"bombbaydoor",					186,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},

// turretraverse misspell
	{IMAP_ASX_TURRET_TRAVERSE,					"turretraverse",			307,			1,	WW2_FUNC_ALL_COMBAT_VEHICLES | WW2_FUNC_TRUCK, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_ASX_TURRET_ELEVATE,					"turretelevate",			308,			1,	WW2_FUNC_ALL_COMBAT_VEHICLES | WW2_FUNC_TRUCK, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_ASX_TURRET_TRAVERSE_SECONDARY,		"turrettraversesecondary",	843,			1,	WW2_FUNC_TANK, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_ASX_TURRET_ELEVATE_SECONDARY,			"turretelevatesecondary",	844,			1,	WW2_FUNC_TANK, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_MAINTRIGGER,						"primarytrigger",			241,			0,	WW2_FUNC_ALL_COMBAT_VEHICLES | WW2_FUNC_TRUCK, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_SECONDTRIGGER,					"secondarytrigger",			242,			0,	WW2_FUNC_ALL_COMBAT_VEHICLES | WW2_FUNC_TRUCK, 0, WW2_FUNC_HEADER_WEAPON, -1},

	{IMAP_TMP_CYCLE_AMMO,						"cycleammo",					240,			0,	WW2_FUNC_ALL_COMBAT_VEHICLES | WW2_FUNC_TRUCK , 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_TMP_GUNSIGHT_VIEW,					"gunsightview",					238,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_JETTISON_ORDNANCE,					"jettison",						7,				0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_TMP_INSTRUMENT_VIEW,					"instrumentview",				237,			0,	WW2_FUNC_ALL_VEHICLES, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_MOM_CANOPYCONTROL,					"canopycontrol",				176,			0,	WW2_FUNC_GROUND | WW2_FUNC_AIR | WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_MOM_HATCHCONTROL,						"hatchcontrol",					175,			0,	WW2_FUNC_GROUND | WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_MOM_TOGGLE_TOWBAR,					"toggletowbar",					188,			0,	WW2_FUNC_ATGUN, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_RIDE_TRANSPORT,						"ridetransport",				916,			0,	WW2_FUNC_GROUND,0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_MOM_GEARHANDLE,						"gearhandle",					162,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_AUTOPILOT,							"autopilot",					921,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_MOM_STUKASIREN,						"stukasiren",					192,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_MOM_DEPLOYDIVEBRAKES,					"deploydivebrakes",				179,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},

	{IMAP_HOLD_BOMBSIGHT_SPEED_DEC,				"bombsightspeeddec",			839,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_HOLD_BOMBSIGHT_SPEED_INC,				"bombsightspeedinc",			840,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_HOLD_BOMBSIGHT_ALT_DEC,				"bombsightaltdec",				841,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_HOLD_BOMBSIGHT_ALT_INC,				"bombsightaltinc",				842,			0,	WW2_FUNC_AIR, 0, WW2_FUNC_HEADER_WEAPON, 1},

    {IMAP_MOBILE_SPAWN,                         "mobilespawn",                  922,            0,  WW2_FUNC_TRUCK, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_ASX_WINCH,							"winch",						915,			1,	WW2_FUNC_SEA, WW2_FUNC_ASYMMETRIC_AXIS, WW2_FUNC_HEADER_WEAPON, 1},

	{IMAP_HOLD_GUNSIGHT_RANGE_DEC,				"gunsightrangedec",				833,			0,	WW2_FUNC_GROUND | WW2_FUNC_SEA |WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_HOLD_GUNSIGHT_RANGE_INC,				"gunsightrangeinc",				834,			0,	WW2_FUNC_GROUND | WW2_FUNC_SEA |WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_WEAPON, -1},
	{IMAP_TMP_RANGEFIND,						"rangefind",					835,			0,	WW2_FUNC_GROUND | WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_WEAPON, 1},
	{IMAP_TARGET_SPEED_INC,						"targetspeedinc",				900,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TARGET_SPEED_DEC,						"targetspeeddec",				901,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TARGET_ANGLE_OFF_INC,					"targetangleinc",				902,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TARGET_ANGLE_OFF_DEC,					"targetangledec",				903,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TARGET_MARK_BEARING,					"targetmarkbearing",			904,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TARGET_CALC_SOLUTION,					"targetcalcsolution",			907,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TORPEDO_DEPTH_INCREASE,				"torpedodepthinc",				905,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	{IMAP_TORPEDO_DEPTH_DECREASE,				"torpedodepthdec",				906,			0,	WW2_FUNC_SEA, 0, WW2_FUNC_HEADER_BOMBSIGHT, 1},
	/*
	Unused emotes since 1.33 (except ammo, uncommented below)
	{IMAP_HOLD_FP_CE_DOUBLE,					"fpcedouble",					1000,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},
	{IMAP_HOLD_FP_CE_GETDOWN,					"fpcegetdown",					1001,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},
	{IMAP_HOLD_FP_CE_HALT,						"fpcehalt",						1002,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},
	{IMAP_HOLD_FP_CE_MOVEOUT,					"fpcemoveout",					1003,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},
	{IMAP_HOLD_FP_CE_AMMO,						"fpceammo",						1004,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},
	{IMAP_HOLD_FP_CE_JUMP,						"fpcejump",						1005,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},
	*/
	{IMAP_HOLD_FP_CE_AMMO,						"fpceammo",						1004,			0,	WW2_FUNC_PERSON, 0, WW2_FUNC_HEADER_EMOTE, -1},

	{IMAP_EZ_MOVE,							"ezmove",								4000,			1,	WW2_FUNC_GENERAL, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_TURN,							"ezturn",								4001,			1,	WW2_FUNC_GENERAL, WW2_FUNC_SYMMETRIC_AXIS | WW2_FUNC_USE_ONRELEASE | WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_0_100, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_ACTION1,						"ezaction1",							4002,			0,	WW2_FUNC_GENERAL, 0, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_ACTION2,						"ezaction2",							4003,			0,	WW2_FUNC_GENERAL, 0, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_FREELOOK,						"ezfreelook",							4004,			0,	WW2_FUNC_GENERAL, 0, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_CAMERA_X,						"ezcamerax",							4005,			1,	WW2_FUNC_GENERAL, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_CAMERA_Y,						"ezcameray",							4006,			1,	WW2_FUNC_GENERAL, WW2_FUNC_DELTA_AXIS, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_NEXTPOS,						"eznextpos",							4007,			0,	WW2_FUNC_GENERAL, 0, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_PREVPOS,						"ezprevpos",							4008,			0,	WW2_FUNC_GENERAL, 0, WW2_FUNC_HEADER_EZ, 2},
	{IMAP_EZ_ENGINE,						"ezengine",								4009,			0,	WW2_FUNC_GENERAL, 0, WW2_FUNC_HEADER_EZ, 2},

	{IMAP_NO_FUNCTION,"",0,0,0,(unsigned int)-1}, // end
};

static void (*localized_string) (const char*, int, char*, int) = nullptr;

static int get_function_index_from_name(const char* language, const char* name);
static int get_function_index_from_code(unsigned int functionCode);

/************************************************************************************************
	ww2funcName
*************************************************************************************************/

void ww2funcName(unsigned int functionCode, char* name, int maxlen)
{
	int i = get_function_index_from_code(functionCode);

	name[0] = '\0';

	if(i > -1)
		localized_string(nullptr, functionTable[i].locID, name, maxlen);
}

const char* ww2GetFuncName(unsigned int functionCode)
{
	static char name[128+1];
	ww2funcName(functionCode, name, 128);
	return name;
}

int ww2GetFuncLoc(unsigned int functionCode)
{
	int i = get_function_index_from_code(functionCode);
	return (i > -1) ? functionTable[i].locID: -1;
}

int ww2funcGetBestCat(int functionCode, int vehType)
{
	int funcIndex = get_function_index_from_code(functionCode);

	if(funcIndex > -1 && functionTable[funcIndex].functionCode == (unsigned int)functionCode)
	{
		const functionEntry& entry = functionTable[funcIndex];

		if(entry.categoryMask & WW2_FUNC_GENERAL)
			return WW2_FUNC_CATEGORY_GENERAL;
		else if(entry.categoryMask & WW2_FUNC_VIEW)
			return WW2_FUNC_CATEGORY_VIEW;
		else if(entry.categoryMask & WW2_FUNC_AIR && (vehType == VEH_TYPE_AIR || vehType == VEH_TYPE_NONE))
			return WW2_FUNC_CATEGORY_AIR;
		else if(entry.categoryMask & WW2_FUNC_TANK && (vehType == VEH_TYPE_GROUND || vehType == VEH_TYPE_NONE))
			return WW2_FUNC_CATEGORY_TANK;
		else if(entry.categoryMask & WW2_FUNC_TRUCK && (vehType == VEH_TYPE_GROUND || vehType == VEH_TYPE_NONE))
			return WW2_FUNC_CATEGORY_TRUCK;
		else if(entry.categoryMask & WW2_FUNC_ATGUN && (vehType == VEH_TYPE_GROUND || vehType == VEH_TYPE_NONE))
			return WW2_FUNC_CATEGORY_ATGUN;
		else if(entry.categoryMask & WW2_FUNC_PERSON && (vehType == VEH_TYPE_PERSON || vehType == VEH_TYPE_NONE))
			return WW2_FUNC_CATEGORY_PERSON;
		else if(entry.categoryMask & WW2_FUNC_SEA && (vehType == VEH_TYPE_SEA || vehType == VEH_TYPE_NONE))
			return WW2_FUNC_CATEGORY_SEA;
		else if(entry.categoryMask & WW2_FUNC_EZ)
			return WW2_FUNC_CATEGORY_EZ;
	}

	return -1;
}

/************************************************************************************************
	ww2funcGetCode
*************************************************************************************************/

unsigned int ww2funcGetCode(int category, int vehicleIdx, int idx)
{
	int i, functionIdx = 0;
	unsigned int categoryMask = 1 << category;

 	for(i = 0; functionTable[i].functionCode != IMAP_NO_FUNCTION; i++)
	{
		const functionEntry& entry = functionTable[i];

		if(entry.categoryMask & categoryMask)
		{
			if(functionIdx == idx)
				return entry.functionCode;

			 functionIdx++;
		}
	}

	return IMAP_NO_FUNCTION;
}

/************************************************************************************************
	ww2funcGetCode
*************************************************************************************************/

int ww2funcGetEZLevel(unsigned int functionCode)
{
	int i = get_function_index_from_code(functionCode);
	if(i > -1) return functionTable[i].ezLevel;

	return -1;
}

/************************************************************************************************
	ww2funcIsAxis
*************************************************************************************************/

int ww2funcIsAxis(unsigned int functionCode)
{
	int isAxis = 0;
	int i = get_function_index_from_code(functionCode);

	if(i > -1)
		isAxis = functionTable[i].isAxis;

	return isAxis;
}

/************************************************************************************************
	ww2funcFlags
*************************************************************************************************/

unsigned int ww2funcFlags(unsigned int functionCode)
{

	unsigned int flags = 0x0;

	int i = get_function_index_from_code(functionCode);

	if(i > -1)
		flags = functionTable[i].flags;

	return flags;
}

/************************************************************************************************
	ww2funcSetLocalizationCallback
*************************************************************************************************/

void ww2funcSetLocalizationCallback(void (*func)(const char* language, int locid, char*, int maxlen))
{
	localized_string = func;
}

/************************************************************************************************
	get_function_index_from_code
*************************************************************************************************/

int get_function_index_from_code(unsigned int functionCode)
{
	int i;
	for(i = 0; functionTable[i].functionCode != IMAP_NO_FUNCTION; i++)
	{
		if(functionTable[i].functionCode == functionCode)
		{
			return i;
		}
	}

	return -1;
}

/************************************************************************************************
	get_function_index_from_name
*************************************************************************************************/

int get_function_index_from_name(const char* language, const char* name)
{
	char str[256];

	for(int i = 0; functionTable[i].functionCode != IMAP_NO_FUNCTION; i++)
	{
		localized_string("english", functionTable[i].locID, str, 255);

		if(!strcmpiws(str, name))
			return i;
	}

	return -1;
}

/************************************************************************************************
	ww2func_register
*************************************************************************************************/

void ww2func_register()
{
	int i;
 	for(i = 0; functionTable[i].functionCode != IMAP_NO_FUNCTION; i++)
 	{
		imapRegisterFunction(functionTable[i].functionCode, functionTable[i].isAxis);
 	}
}

/************************************************************************************************
	ww2func_info
*************************************************************************************************/

int ww2func_info(int mode, const char* language, unsigned int* functionCode, char* name, int maxlen,
					imap_function_info* funcInfo)
{
	int i;

	if(mode == IMAP_HAVE_CODE)
	{
		i = get_function_index_from_code(*functionCode);

		if(name)
		{
			name[0] = '\0';

			if(i > -1)
				localized_string("english", functionTable[i].locID, name, maxlen);
		}
	}
	else
	{
		*functionCode = IMAP_NO_FUNCTION;

		i = get_function_index_from_name("english", name);
		if(i > -1)
			*functionCode = functionTable[i].functionCode;
	}

	if(i > -1)
	{
		funcInfo->isAxis = functionTable[i].isAxis;
		funcInfo->axisType = 0;
		if (functionTable[i].flags & WW2_FUNC_SYMMETRIC_AXIS)  	funcInfo->axisType = IMAP_SYMMETRIC_AXIS;
		if (functionTable[i].flags & WW2_FUNC_ASYMMETRIC_AXIS)  funcInfo->axisType = IMAP_ASYMMETRIC_AXIS;
		if (functionTable[i].flags & WW2_FUNC_DELTA_AXIS)  		funcInfo->axisType = IMAP_DELTA_AXIS;
	}

	return 0;
}

/************************************************************************************************
	ww2funcHeader
*************************************************************************************************/

const char* ww2funcHeader(unsigned int functionCode)
{
	static char name[64];
	int i = get_function_index_from_code(functionCode);

	name[0] = '\0';

	if(i > -1)
		localized_string("english", functionTable[i].headerLocID, name, 63);

	return name;
}


const std::wstring& ww2funcHeaderLoc(unsigned int functionCode)
{
	int i = get_function_index_from_code(functionCode);
	return ww2imapGetLoc((i > -1) ? functionTable[i].headerLocID: -1);
}

unsigned int ww2funcHeaderLocID(unsigned int functionCode)
{
	int i = get_function_index_from_code(functionCode);
	return (i > -1) ? functionTable[i].headerLocID : -1;
}

const char* ww2FunctionControlName(unsigned int functionCode)
{
	int i = get_function_index_from_code(functionCode);

	if(i > -1)
		return functionTable[i].vehcntrlName;

	return "";
}

unsigned int ww2FunctionCode(const char* controlName)
{
	unsigned int i;
	for(i = 0; functionTable[i].functionCode != IMAP_NO_FUNCTION; i++)
	{
		if(strcmp(controlName, functionTable[i].vehcntrlName) == 0)
			return functionTable[i].functionCode;
	}

	return 0;
}
