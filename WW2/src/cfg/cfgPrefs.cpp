#include <syscfg.h> // Path is project specific.
#include "phui/XMLMessage.h"
#include "phui/ControlValueMessage.h"
#include "phui/Message.h"

#include "include/vars.h"
#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include "cfg/cfgPrefs.h"
#include "fio/fio.h"
#include "imap/ww2imap.h"
#include "cfg/cfgw2.h"
#include "ld/ldpix.h"
#include "pi/pi.h"
#include "cfg/cfgLoad.h"
#include "locl/loclLang.h"
#include "veh/vehIcon.h"
#include "pi/piinfo.h"
#include "chat/chatPreference.h"
#include "chat/chatConstants.h"
#include "client/GameClient.h"
#include "ddp.h"
#include "ui/uiDotCommands.h"
#include "trn/trnRadClutter.h"
#include "gr2/gr2Anim.h"
#include "fp/fpPPO.h"
#include "osgImport/ShadowMap.h"
#include "pers/persgunsight.h"
#include "sdm/StratDB.h"
#include "teul/teulPacket.h"

#define USE_NEW_CONFIG "config.authoritative"
#define CONFIG_VERSION 12

phui::Message* gAppPrefs; // main preference repository

// functions
static void cfgPrefsSetDefaults();
static void cfgPrefsSetResolution();
static void cfgPrefsProcessGlobal(_cfgPARAMS key);

// variables
static bool _restartNeeded = false; // true if a non-dynamic global was changed
static bool _prefsInited = false;
static bool _langInited = false;

/*
	To add a new global var that is hooked into the preferences system,
	follow these steps:

	1. Declare the global in vars.h
	2. Add a declaration to the end of the _cfgPARAMS enum in cfgCodes.h
	3. Then add its details declaration in _prefDetails below
	4. Add the default init for the global below in cfgPrefsSetDefaults()
	4. Add any special handling to cfgPrefsProcessGlobal()

	If this global should be settable in the UI layer, then let the
	UI programmer know they should add this variable to the UI settings
	page.

*/

/*
	This array describes all globals. Each element represents
	a global specified in cfgCodes.h > _cfgPARAMS.

	name - is the key name of the global
	var - is the address of the global variable
	type - is the _cfgPARAM_TYPES as defined in cfgCodes.h
	dynamic - is whether this variable can be updated on the fly, otherwise a restart is needed
	min - the minimum value of this var; strings and bools = 0; numbers - any number
	max - the maximum value of this var; strings = max size of string; bools = 1; numbers - any number
*/
static cfgPARAM_DETAILS _prefDetails[cfgNUM_TOTAL_PARAMS] =
{
	// name						var pointer (can be null)	type				dynamic	min max active
	{"app.shademode",			&ShadeMode,					cfgPARAM_UINT,		1,		0,	0, false },
	{"app.bdithering",			&bDithering,				cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.bspecular",			&WantSpecularMaps,			cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.sound",				&WantedSoundCard,			cfgPARAM_STRING,	0,		0,	(int)sizeof(WantedSoundCard) - 1, true	},
	{"app.bsoundenable",		&bSoundEnable,				cfgPARAM_BOOLEAN,	0,		0,	1, true	},
	{"app.b3dsoundenable",		&b3DSoundEnable,			cfgPARAM_BOOLEAN,	0,		0,	1, true	},
	{"app.video",				&WantedVideoCard,			cfgPARAM_STRING,	0,		0,	(int)sizeof(WantedVideoCard) - 1, true	},
	{"app.resolution",			&WantedVideoResolution,		cfgPARAM_STRING,	0,		0,	(int)sizeof(WantedVideoResolution) - 1, true	},
	{"app.bnoframerunahead",	&bNoFrameRunahead,			cfgPARAM_BOOLEAN,	0,		0,	1, false	},
#if platOS == OS_MACOS
	{"app.enablevsync",	        &g_enableVSync,				cfgPARAM_BOOLEAN,	1,		0,	1, true	},
#else
	{"app.enablevsync",	        &g_enableVSync,				cfgPARAM_BOOLEAN,	0,		0,	1, true	},
#endif
	{"app.gammared",			&GammaRed,					cfgPARAM_FLOAT,		1,		0,	0, false	},
	{"app.gammagreen",			&GammaGreen,				cfgPARAM_FLOAT,		1,		0,	0, false	},
	{"app.gammablue",			&GammaBlue,					cfgPARAM_FLOAT,		1,		0,	0, false	},
	{"app.bshowinfo",			&bShowInfo,					cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.shadowsizeselect",	&ShadowSizeSelect,			cfgPARAM_SINT,		1,		0,	8, false	},
	{"app.volumemultiplier",	&VolumeMultiplier,			cfgPARAM_FLOAT,		1,		0,	0, true	},
	{"app.musicmultiplier",		&MusicMultiplier,			cfgPARAM_FLOAT,		1,		0,	0, true	},
	{"app.hudvisible",			&HUDVisible,				cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.fogscaleselector",	&FogScaleSelector,			cfgPARAM_SINT,		1,		0,	0, false	},
	{"app.maxnumbersounds",		&MaxNumberOfSounds,			cfgPARAM_UINT,		0,		0,	0, true	},
	{"app.suppressmuzzlesmoke",	&SuppressMuzzleSmoke,		cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.suppressmuzzlelight", &SuppressMuzzleLight,       cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.suppresstracersmoke",	&SuppressTracerSmoke,		cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.lodfieldofview",		&LODFieldOfView,			cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.lodlimitperframe",	&LODLimitPerFrame,			cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.bcursorenabled",      &g_bCursorOffAtSpawn,       cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.friendlycolor",		&gIconFriendlyColor,		cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.enemycolor",			&gIconEnemyColor,			cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.squadcolor",			&gIconSquadColor,			cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.missioncolor",        &gIconMissionColor,         cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.missionleadercolor",  &gIconMissionLeaderColor,   cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.language",			&gLanguage,					cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.ambientlevel",        &g_iAmbLevel,               cfgPARAM_SINT,		0,		0,	0, false	},
	{"app.radialclutter",       nullptr,                       cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.radialdensity",       &g_fRadialDensity,          cfgPARAM_FLOAT,		1,		0,	0, true	},
	{"app.radialradius",        &g_fRadialRadius,           cfgPARAM_FLOAT,		1,		0,	0, true	},
	{"app.visplayerlimit",      &g_iVisiblePlayerLimit,     cfgPARAM_SINT,		1,		0,	0, true	},
	{"app.usenetcode",			&g_wantNetcode,				cfgPARAM_BOOLEAN,   1,      0,  1, true },
	{"app.fsaasamples",      	&g_iFSSASamples,			cfgPARAM_SINT,		1,		0,	0, false	},
	{"app.fsaahighquality",		&g_bFSSAHighQuality,        cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.soundhighquality",    &g_bSoundHighQuality,       cfgPARAM_BOOLEAN,	1,		0,	1, false	},
	{"app.soundforcestereo",    &g_bSoundForceStereo,       cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.soundconvertonload",  &g_bSoundConvertOnLoad,     cfgPARAM_BOOLEAN,	0,		0,	1, false	},
	{"app.capturealldisplays",  &g_bCaptuerAllDisplays,     cfgPARAM_BOOLEAN,	0,		0,	1, false	},
	{"app.enableh2oshaders",    &gWaterShadersEnabled,      cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.postrenderfilter",    &g_postRenderFilter,        cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.grassshader",         &g_grassShader,             cfgPARAM_BOOLEAN,	0,		0,	1, true	},
	{"app.normalmaps",          &g_wantNormalMaps,          cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.waterreflections",    &g_wantWaterReflections,    cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.reflectionupdate",    &g_updateReflectionRate,    cfgPARAM_SINT,		1,		0,	0, true	},
	{"app.wantmessagealert",    &g_wantMessageAlert,        cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.usenetworkroute",     &g_useNetworkRoute,         cfgPARAM_SINT,		0,		0,	0, true	},
	{"app.wantsse2",            &g_wantSSE2,				cfgPARAM_BOOLEAN,	0,		0,	1, false	},
	{"app.fontsize",			nullptr,						cfgPARAM_UINT,		1,		9,	12, true	},
	{"app.squadinvites",        &SquadInvites,				cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.tooltips",			nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.showreadout",			nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.iconmode",			nullptr,						cfgPARAM_SINT,		1,		0,	2, true	},
	{"app.playmusic",			nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.isrecruiting",		nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.autoleader",			nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.awslow",				nullptr,						cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.awshigh",				nullptr,						cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.awsalpha",			nullptr,						cfgPARAM_FLOAT,		1,		0,	1, true	},
	{"app.ragdolls",			nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.setdecal",			nullptr,						cfgPARAM_UINT,		1,		0,	0, true	},
	{"app.waterrefractions",    &wantWaterRefractions,      cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.wantbloodeffects",    &WantBloodEffects,          cfgPARAM_BOOLEAN,   1,      0,  1 , true  },
	{"app.shadowreceivers",		&ShadowMapFlags,			cfgPARAM_UINT,		1,      0,  1 , true  },
	{"app.corpses",				nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.ragdollweapons",		nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.combatsmoke",			&WantCombatSmoke,			cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.radialclutterwhenflying",		&g_bRadialWhenFlying,	cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.lowmemory",			nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.corpsetime",			nullptr,						cfgPARAM_FLOAT,		1,		0,	1, true	},
	{"app.corpsecount",			nullptr,						cfgPARAM_FLOAT,		1,		0,	1, true	},
	{"app.corpsedetail",		nullptr,						cfgPARAM_BOOLEAN,	1,		0,	1, true	},
	{"app.ppook",				nullptr,						cfgPARAM_UINT,		1,		0,	1, true	},
	{"app.pponok",				nullptr,						cfgPARAM_UINT,		1,		0,	1, true	},
	{"app.macsound",			nullptr,						cfgPARAM_UINT,		1,		0,	1, true	},
	{"app.inflodmod",			&gTrooperMOD,				cfgPARAM_FLOAT,		1,		0,	4, true	},
	{"app.conv",				&Convergence,				cfgPARAM_UINT,		1,		0,	800, true },
	{"app.aimScalar",			&aimMouse,					cfgPARAM_FLOAT,		1,		0, 8, true }
	// add more vars here
};

/*************************************************************************************************
	cfgPrefsRestartNeeded
*************************************************************************************************/

// was a non-dynamic global changed this sesion?
// when you check the variable, it resets
bool cfgPrefsRestartNeeded()
{
	bool restart = _restartNeeded;
	_restartNeeded = false;
	return restart;
}

/*************************************************************************************************
	cfgPrefsSendSharedPrefs - currently only handles ints
*************************************************************************************************/

void cfgPrefsSendSharedPrefs()
{
	if(PlayerClient()->ChatConnection() < 0)
		return;

	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::PREFERENCE_Y));

	const char* name;
	name = cfgPrefsGetGlobalName(cfgVISIBLE_PLAYER_LIMIT);
	outgoing.construct<Chat::Preference>(CPR_UPDATE_MODE, cfgPrefsGetInt(name, 2));

	name = cfgPrefsGetGlobalName(cfgNETCODE);
	outgoing.construct<Chat::Preference>(CPR_SET_NETCODE, cfgPrefsGetBool(name, true));

	name = cfgPrefsGetGlobalName(cfgLANGUAGE_CHOICE);
	outgoing.construct<Chat::Preference>(CPR_CLIENT_LANGUAGE, cfgPrefsGetInt(name, 0));
	outgoing.construct<Chat::Preference>(CPR_PLAYER_LANGUAGE, cfgPrefsGetInt(name, 0));

	name = cfgPrefsGetGlobalName(cfgIS_RECRUITING);
	outgoing.construct<Chat::Preference>(CPR_IS_RECRUITING, cfgPrefsGetBool(name, false));

	name = cfgPrefsGetGlobalName(cfgAUTO_LEADER);
	outgoing.construct<Chat::Preference>(CPR_AUTO_LEADER, cfgPrefsGetBool(name, true));

	outgoing.send(PlayerClient()->ChatConnection());
}

/*************************************************************************************************
	cfgPrefsSendSharedPref - currently only handles ints
*************************************************************************************************/

void cfgPrefsSendSharedPref(CHAT_PREFERENCE key)
{
	if(PlayerClient()->ChatConnection() < 0)
		return;

	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::PREFERENCE_Y));
	switch(key)
	{
		case CPR_UPDATE_MODE:
		{
			const char* name = cfgPrefsGetGlobalName(cfgVISIBLE_PLAYER_LIMIT);
			outgoing.append(Chat::Preference(key, cfgPrefsGetInt(name, 2)));
		}
		break;
		case CPR_SET_NETCODE:
		{
			const char* name = cfgPrefsGetGlobalName(cfgNETCODE);
			outgoing.append(Chat::Preference(key, cfgPrefsGetBool(name, true)));
		}
		break;
		case CPR_CLIENT_LANGUAGE: // my ui language
		case CPR_PLAYER_LANGUAGE: // my spoken language
		{
			const char* name = cfgPrefsGetGlobalName(cfgLANGUAGE_CHOICE);
			outgoing.append(Chat::Preference(key, cfgPrefsGetInt(name, 0)));
		}
		break;
		case CPR_IS_RECRUITING:
		{
			const char* name = cfgPrefsGetGlobalName(cfgIS_RECRUITING);
			outgoing.append(Chat::Preference(key, cfgPrefsGetBool(name, false)));
		}
		break;
		case CPR_AUTO_LEADER:
		{
			const char* name = cfgPrefsGetGlobalName(cfgAUTO_LEADER);
			outgoing.append(Chat::Preference(key, cfgPrefsGetBool(name, true)));
		}
		break;
		case CPR_SET_DECAL:
		{
			const char* name = cfgPrefsGetGlobalName(cfgSETDECAL);
			int decal = cfgPrefsGetInt(name, 0);

			if(decal)
				outgoing.append(Chat::Preference(key, decal));
		}
		break;
		default:
			// don't send anything
			break;
	}

	if(outgoing.empty() == false)
	{
		outgoing.send(PlayerClient()->ChatConnection());
	}
}

/*************************************************************************************************
	cfgPrefsLoad
*************************************************************************************************/

// called to load preferences from disk
// if the new config stuff is authoritative, it will load
// from data/wwiiol.xml, otherwise it will load the
// old config stuff from wwiiol.cfg
bool cfgPrefsLoad()
{
	// create the preferences repository
	if(!gAppPrefs)
		gAppPrefs = new phui::Message;

	// load in the prefs from data/wwiiol.xml
	phui::XMLMessage xml(&cfgioCallbacks);
	xml.Restore(gAppPrefs, PREF_FILE_NAME);

	/*
	// by default, the new stuff is not authoritative
	if(cfgPrefsGetBool(USE_NEW_CONFIG, false))
	{
		// lets set some defaults first
		cfgPrefsSetDefaults();

		// now we sync the globals
		// special handling also gets
		// called for each global as well
		cfgPrefsSyncGlobals();

		// we're good
		return true;
	}
	else
	{
#if platOS == OS_MACOS
		char path[4192];
		strprintf(path, "%swwiiol.cfg", macPreferencesPath());
#else
		static const char path[] = "wwiiol.cfg";
#endif
		// the new stuff isn't authoritative yet,
		// so we'll load the old stuff
		if(cfgLoadGlobals(path))
		{
			// we now make sure our globals repository
			// is synced to the old stuff
			// this call sets the pref named
			// by USE_NEW_CONFIG to true, meaning
			// the next time the game is run, the
			// new config stuff will be used
			cfgPrefsSyncPrefs();

			// we're good
			return true;
		}
	}
	*/

	// lets set some defaults first
	cfgPrefsSetDefaults();

	// now we sync the globals
	// special handling also gets
	// called for each global as well
	cfgPrefsSyncGlobals();

	// set this flag for legacy purposes
	cfgPrefsSetBool(USE_NEW_CONFIG, true);

	// preferences we're loaded for some reason
	// either from the new or old stuff
	return true;
}

/*************************************************************************************************
	cfgPrefsSave
*************************************************************************************************/

void cfgPrefsSave()
{
	if(!gAppPrefs) return;

	// uncomment the call below to ensure globals
	// in memory get saved even if they were
	// changed outside of the prefs system
	// cfgPrefsSyncPrefs();

	phui::XMLMessage xml(&cfgioCallbacks);
	xml.Archive(gAppPrefs, PREF_FILE_NAME);
}

/*************************************************************************************************
	cfgPrefsKill
*************************************************************************************************/

void cfgPrefsKill()
{
	if(gAppPrefs)
	{
		delete gAppPrefs;
		gAppPrefs = nullptr;
	}
}

/*************************************************************************************************
	cfgPrefsInit
*************************************************************************************************/

void cfgPrefsInit()
{
	if(!_prefsInited)
	{
		_prefsInited = true;

		cfgPrefsProcessGlobal(cfgWANT_SSE2);
		cfgPrefsProcessGlobal(cfgENABLE_H2O_SHADERS);
		cfgPrefsProcessGlobal(cfgH2O_REFLECTIONS);
		cfgPrefsProcessGlobal(cfgH2O_REFRACTIONS);
		cfgPrefsProcessGlobal(cfgPOST_RENDER_FILTER);
		cfgPrefsProcessGlobal(cfgGRASS_SHADER);
		cfgPrefsProcessGlobal(cfgNORMAL_MAPS);
		cfgPrefsProcessGlobal(cfgVIDEO_NO_FRAME_RUN_AHEAD);
		cfgPrefsProcessGlobal(cfgVIDEO_ENABLE_VSYNC);
		cfgPrefsProcessGlobal(cfgSHADOW_RECEIVERS);
	}
}

/*************************************************************************************************
	cfgPrefsSyncGlobals (disk to memory)
*************************************************************************************************/

void cfgPrefsSyncGlobals()
{
	// gAppPrefs -> globals
	for(int i = 0; i < cfgNUM_TOTAL_PARAMS; i++)
	{
		if(_prefDetails[i].var)
			cfgPrefsGetGlobal((_cfgPARAMS)i, _prefDetails[i].var);

		// special handling is called for all
		// globals the first time they are loaded
		cfgPrefsProcessGlobal((_cfgPARAMS)i);
	}
}

/*************************************************************************************************
	cfgPrefsSyncPrefs (memory to disk)
*************************************************************************************************/

void cfgPrefsSyncPrefs()
{
	// globals -> gAppPrefs
	for(int i = 0; i < cfgNUM_TOTAL_PARAMS; i++)
	{
		if(_prefDetails[i].var)
			cfgPrefsSetGlobal((_cfgPARAMS)i, _prefDetails[i].var, false);
	}

	// make the new xml config authoritative over the old stuff
	cfgPrefsSetBool(USE_NEW_CONFIG, true);
}

/*************************************************************************************************
	cfgPrefsSetGlobal
*************************************************************************************************/

// parameter 'update' defaults to true and is used to determine whether to
// update the global var associated with this key

bool cfgPrefsSetGlobal(_cfgPARAMS key, void* value, bool update)
{
	bool success = false; // defaults to an error

	UINT32 lang = gLanguage; // special handling for switch from non-china to china

	// save pref to disk and memory
	if(gAppPrefs && key >= 0 && key < cfgNUM_TOTAL_PARAMS)
	{
		switch(_prefDetails[key].type)
		{
			case cfgPARAM_BOOLEAN:
			{
				success = cfgPrefsSetBool(_prefDetails[key].name, *(bool*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					*(bool*)_prefDetails[key].var = *(bool*)value;
			}
			break;
			case cfgPARAM_UINT:
			{
				success = cfgPrefsSetInt(_prefDetails[key].name, *(int*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					*(unsigned int*)_prefDetails[key].var = *(unsigned int*)value;
			}
			break;
			case cfgPARAM_SINT:
			{
				success = cfgPrefsSetInt(_prefDetails[key].name, *(int*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					*(int*)_prefDetails[key].var = *(int*)value;
			}
			break;
			case cfgPARAM_LONG:
			{
				success = cfgPrefsSetLong(_prefDetails[key].name, *(long*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					*(long*)_prefDetails[key].var = *(long*)value;
			}
			break;
			case cfgPARAM_FLOAT:
			{
				success = cfgPrefsSetFloat(_prefDetails[key].name, *(float*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					*(float*)_prefDetails[key].var = *(float*)value;
			}
			break;
			case cfgPARAM_DOUBLE:
			{
				success = cfgPrefsSetDouble(_prefDetails[key].name, *(double*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					*(double*)_prefDetails[key].var = *(double*)value;
			}
			break;
			case cfgPARAM_STRING:
			{
				success = cfgPrefsSetString(_prefDetails[key].name, (const char*)value);

				if(success && update && _prefDetails[key].var && _prefDetails[key].dynamic)
					snprintf((char*)_prefDetails[key].var, _prefDetails[key].max, "%s", (const char*)value);
			}
			break;
		}

		if(success)
		{
			if(_prefDetails[key].dynamic) // if its dynamic we try to special process it
					cfgPrefsProcessGlobal(key);
			else if(update) // if its not dynamic and we were told to update, then a restart is needed
				_restartNeeded = true;
		}
	}

	return success;
}

/*************************************************************************************************
	cfgPrefsGetGlobal
*************************************************************************************************/

// parameter 'config' defaults to true and is used to determine whether to
// pull the value from the actual in memory global or from the pref system's
// variable repository(the config file); these can be different because some
// globals can be changed in settings, but because they require a restart,
// the actual in memory global won't get updated until the game is restarted

bool cfgPrefsGetGlobal(_cfgPARAMS key, void* value, bool config)
{
	bool success = false; // defaults to an error when setting

	// get pref
	if(gAppPrefs && key >= 0 && key < cfgNUM_TOTAL_PARAMS)
	{
		if(config) // we want the global from the config
		{
			switch(_prefDetails[key].type)
			{
				case cfgPARAM_BOOLEAN:
					success = cfgPrefsGetBool(_prefDetails[key].name, (bool*)value);
					break;
				case cfgPARAM_UINT:
				case cfgPARAM_SINT:
					success = cfgPrefsGetInt(_prefDetails[key].name, (int*)value);
					break;
				case cfgPARAM_LONG:
					success = cfgPrefsGetLong(_prefDetails[key].name, (long*)value);
					break;
				case cfgPARAM_FLOAT:
					success = cfgPrefsGetFloat(_prefDetails[key].name, (float*)value);
					break;
				case cfgPARAM_DOUBLE:
					success = cfgPrefsGetDouble(_prefDetails[key].name, (double*)value);
					break;
				case cfgPARAM_STRING:
				{
					const char* sval = nullptr;
					success = cfgPrefsGetString(_prefDetails[key].name, &sval);

					if(success)
						snprintf((char*)value, _prefDetails[key].max, "%s", sval);
				}
				break;
			}
		}

		if(_prefDetails[key].var != nullptr && (!config || !success))  // we want the global from memory
		{
			success = true;

			switch(_prefDetails[key].type)
			{
				case cfgPARAM_BOOLEAN:
					*(bool*)value = *(bool*)_prefDetails[key].var;
					break;
				case cfgPARAM_UINT:
					*(unsigned int*)value = *(unsigned int*)_prefDetails[key].var;
					break;
				case cfgPARAM_SINT:
					*(int*)value = *(int*)_prefDetails[key].var;
					break;
				case cfgPARAM_LONG:
					*(long*)value = *(long*)_prefDetails[key].var;
					break;
				case cfgPARAM_FLOAT:
					*(float*)value = *(float*)_prefDetails[key].var;
					break;
				case cfgPARAM_DOUBLE:
					*(double*)value = *(double*)_prefDetails[key].var;
					break;
				case cfgPARAM_STRING:
					snprintf((char*)value, _prefDetails[key].max, "%s", (char*)_prefDetails[key].var);
					break;
				default:
					success = false;
					break;
			}
		}
	}

	return success;
}

/*************************************************************************************************
	cfgPrefsGetGlobalDetails - by key
*************************************************************************************************/

cfgPARAM_DETAILS* cfgPrefsGetGlobalDetails(_cfgPARAMS key)
{
	if(gAppPrefs && key >= 0 && key < cfgNUM_TOTAL_PARAMS)
		return &_prefDetails[key];

	return nullptr;
}

/*************************************************************************************************
	cfgPrefsGetGlobalDetails - by name
*************************************************************************************************/

cfgPARAM_DETAILS* cfgPrefsGetGlobalDetails(const char* name)
{
	if(gAppPrefs)
	{
		for(int i = 0; i < cfgNUM_TOTAL_PARAMS; ++i)
		{
			if(strcasecmp(name, _prefDetails[i].name) == 0)
				return &_prefDetails[i];
		}
	}

	return nullptr;
}

/*************************************************************************************************
	cfgPrefsGetGlobalKey - by name
*************************************************************************************************/

int cfgPrefsGetGlobalKey(const char* name)
{
	if(gAppPrefs)
	{
		for(int i = 0; i < cfgNUM_TOTAL_PARAMS; ++i)
		{
			if(strcasecmp(name, _prefDetails[i].name) == 0)
				return i;
		}
	}

	return -1;
}

/*************************************************************************************************
	cfgPrefsGetGlobalDetails
*************************************************************************************************/

const char* cfgPrefsGetGlobalName(_cfgPARAMS key) // gets a global's name
{
	if(gAppPrefs && key >= 0 && key < cfgNUM_TOTAL_PARAMS)
		return _prefDetails[key].name;

	return nullptr;
}

// set arbitrary preferences
bool cfgPrefsSetBool(const char* key, bool value)
{
	return (!gAppPrefs->ReplaceOrAddBool(key, value)) ? true: false;
}

bool cfgPrefsSetInt(const char* key, int value)
{
	return cfgPrefsSetLong(key, (long)value);
}

bool cfgPrefsSetLong(const char* key, long value)
{
	return (!gAppPrefs->ReplaceOrAddLong(key, value)) ? true: false;
}

bool cfgPrefsSetFloat(const char* key, float value)
{
	return (!gAppPrefs->ReplaceOrAddFloat(key, value)) ? true: false;
}

bool cfgPrefsSetDouble(const char* key, double value)
{
	return (!gAppPrefs->ReplaceOrAddDouble(key, value)) ? true: false;
}

bool cfgPrefsSetString(const char* key, const char* value)
{
	return (!gAppPrefs->ReplaceOrAddString(key, value)) ? true: false;
}

bool cfgPrefsRemove(const char* key)
{
	if(key)
		return (!gAppPrefs->RemoveName(key)) ? true: false;

	// if no key was specified, delete all keys
	gAppPrefs->MakeEmpty();

	return true;
}

// get arbitrary preference by pointer
bool cfgPrefsGetBool(const char* key, bool* value)
{
	return (!gAppPrefs->FindBool(key, value)) ? true: false;
}

bool cfgPrefsGetInt(const char* key, int* value)
{
	long lval = 0;

	if(cfgPrefsGetLong(key, &lval))
	{
		*value = (int)lval;
		return true;
	}

	return false;
}

bool cfgPrefsGetLong(const char* key, long* value)
{
	return (!gAppPrefs->FindLong(key, value)) ? true: false;
}

bool cfgPrefsGetFloat(const char* key, float* value)
{
	return (!gAppPrefs->FindFloat(key, value)) ? true: false;
}

bool cfgPrefsGetDouble(const char* key, double* value)
{
	return (!gAppPrefs->FindDouble(key, value)) ? true: false;
}

bool cfgPrefsGetString(const char* key, const char** value)
{
	return (!gAppPrefs->FindString(key, value)) ? true: false;
}

// get arbitrary preferences by value
bool cfgPrefsGetBool(const char* key, bool defaultValue)
{
	cfgPrefsGetBool(key, &defaultValue);
	return defaultValue;
}

int cfgPrefsGetInt(const char* key, int defaultValue)
{
	cfgPrefsGetInt(key, &defaultValue);
	return defaultValue;
}

long cfgPrefsGetLong(const char* key, long defaultValue)
{
	cfgPrefsGetLong(key, &defaultValue);
	return defaultValue;
}

float cfgPrefsGetFloat(const char* key, float defaultValue)
{
	cfgPrefsGetFloat(key, &defaultValue);
	return defaultValue;
}

double cfgPrefsGetDouble(const char* key, double defaultValue)
{
	cfgPrefsGetDouble(key, &defaultValue);
	return defaultValue;
}

const char* cfgPrefsGetString(const char* key, const char* defaultValue)
{
	cfgPrefsGetString(key, &defaultValue);
	return defaultValue;
}

/*************************************************************************************************
	cfgPrefsRestoreToView
*************************************************************************************************/

#define VIEW_BUFFER_SIZE 128+1

void cfgPrefsRestoreToView(phui::View* v, int what)
{
	if(!gAppPrefs || !v->Name()[0])
		return;

	if(what & SAVE_VIEW_VALUE)
		phui::RestoreControlValues(v, gAppPrefs);

	float val		= 0;
	float xScale	= (float)WantedWidth / 1024;
	float yScale	= (float)WantedHeight / 768;
	char buffer[VIEW_BUFFER_SIZE];

	memset(buffer, 0x0, VIEW_BUFFER_SIZE); // 127 chars max

	if(what & SAVE_VIEW_POSITION) // need to account for resolution scaling
	{
		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.x", v->Name());
		if(cfgPrefsGetFloat(buffer, &val))
			v->MoveTo(val*xScale, v->Y());

		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.y", v->Name());
		if(cfgPrefsGetFloat(buffer, &val))
			v->MoveTo(v->X(), val*yScale);
	}

	if(what & SAVE_VIEW_SIZE)  // need to account for resolution scaling
	{
		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.width", v->Name());
		if(cfgPrefsGetFloat(buffer, &val))
			v->ResizeTo(val*xScale, v->Height());

		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.height", v->Name());
		if(cfgPrefsGetFloat(buffer, &val))
			v->ResizeTo(v->Width(), val*yScale);
	}

	if(what & SAVE_VIEW_VISIBILITY)
	{
		bool bval = false;
		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.visible", v->Name());
		if(cfgPrefsGetBool(buffer, &bval))
		{
			if(bval)
				v->Show();
			else
				v->Hide();
		}
	}
}

/*************************************************************************************************
	cfgPrefsSaveFromView
*************************************************************************************************/

void cfgPrefsSaveFromView(phui::View* v, int what)
{
	if(!gAppPrefs || !v->Name()[0])
		return;

	if(what & SAVE_VIEW_VALUE)
		phui::ArchiveControlValues(v, gAppPrefs);

	char buffer[VIEW_BUFFER_SIZE];
	float xScale	= (float)WantedWidth / 1024;
	float yScale	= (float)WantedHeight / 768;

	memset(buffer, 0x0, VIEW_BUFFER_SIZE); // 127 chars max

	if(what & SAVE_VIEW_POSITION)
	{
		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.x", v->Name());
		cfgPrefsSetFloat(buffer, v->X()/xScale);

		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.y", v->Name());
		cfgPrefsSetFloat(buffer, v->Y()/yScale);
	}

	if(what & SAVE_VIEW_SIZE)
	{
		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.width", v->Name());
		cfgPrefsSetFloat(buffer, v->Width()/xScale);

		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.height", v->Name());
		cfgPrefsSetFloat(buffer, v->Height()/yScale);
	}

	if(what & SAVE_VIEW_VISIBILITY)
	{
		snprintf(buffer, VIEW_BUFFER_SIZE, "view.%s.visible", v->Name());
		cfgPrefsSetBool(buffer, v->IsVisible());
	}
}

/*************************************************************************************************
	cfgPrefsDefaults
*************************************************************************************************/

void cfgPrefsSetDefaults()
{
	int versionDiff = CONFIG_VERSION - cfgPrefsGetInt("app.version", 0);

	if(versionDiff)
	{
		// set our config's version
		cfgPrefsSetInt("app.version", CONFIG_VERSION);

		// we're now reset
	}

	bZEnable            = piZENABLE_DEFAULT;				// Z buffer enable
	bZWrite             = piZWRITEENABLE_DEFAULT;			// Z buffer write enable
	ZFunction           = piZFUNC_DEFAULT;					// Z compare function
	ShadeMode           = piSHADEMODE_DEFAULT;				// flat, gouraud, phong?
	TextureMagFilter    = piTEXTUREMAG_DEFAULT;				// texture mag filter
	TextureMinFilter    = piTEXTUREMIN_DEFAULT;				// texture min filter
	TextureMipFilter	= piTEXTUREMIP_DEFAULT;				// texture mipmap filter
	FillMode            = piFILLMODE_DEFAULT;				// solid, lines or points?
	bDithering          = piDITHERENABLE_DEFAULT;			// dithering is on
	bSpecular           = piSPECULARENABLE;					// specular highlights are on
	SrcBlend            = piSRCBLEND_DEFAULT;				// source blend mode
	DestBlend           = piDESTBLEND_DEFAULT;				// destination blend mode

	bFogEnabled         = piFOGENABLE_DEFAULT;				// fog is on
	FogColor            = piFOGCOLOR_DEFAULT;				// fog color
	FogMode             = piFOGMODE_DEFAULT;				// linear, exp. etc.
	FogStart            = piFOGSTART_DEFAULT;				// begining depth
	FogEnd              = piFOGEND_DEFAULT;					// ending depth
	FogDensity          = piFOGDENSITY_DEFAULT;				// fog density

	bPerFrameBSP        = TRUE;								// generate BSP sorting tree per frame
	bClearViewport      = FALSE;								// clear the viewport each frame
	bClearZBuffer       = TRUE;								// clear the z-buffer each frame
	bMipmapEnable       = TRUE;								// mipmap enable
	bMipmapSoftware     = FALSE;							// mipmap using software
	MipmapDistance      = 500.0f;							// mipmap distance
	MinTextureDim       = 1;								// minimum texture dimensions
	bShowInfo           = FALSE;							// show window information at the top right
	bResizingDisabled   = FALSE;							// do not allow resizing
	bTexturesDisabled   = FALSE;							// textures are disabled
	bOnlySystemMemory   = FALSE;							// all surfaces forced into system memory
	bOnlyEmulation      = FALSE;							// no hardware DD or D3D devices allowed
	bBackdrop           = TRUE;								// usually need backdrop for fog to look correct
	bSoundEnable        = TRUE;							// most of the time, sound is nice
	b3DSoundEnable      = FALSE;						// all the time, 3D sound is better
	g_wantMessageAlert  = TRUE;

	GammaRed            = 1.0f;								// gamma for screen
	GammaGreen          = 1.0f;								// gamma for screen
	GammaBlue           = 1.0f;								// gamma for screen

	CrossHairs          = piCROSSHAIRS_DEFAULT;				// crosshairs shape
	StaticLighting		= FALSE;
	DynamicLighting		= TRUE;
	ScaleFactor         = 1.0f;
	bAnimatedWater		= TRUE;
	FieldOfView			= 90.0f;
	bVehicleMirror		= FALSE;
	bNoFrameRunahead	= TRUE;
	g_enableVSync		= FALSE;
	ShadowSizeSelect	= 6;
	MultiTextureEnable	= TRUE;

	VolumeMultiplier	= 1.0f;
	MusicMultiplier		= 1.0f; // defaults to low volume

	HUDVisible			= 1; // default HUD
	FogScaleSelector	= 0;
	MaxNumberOfSounds	= 32;
	g_iNumBullets       = 128;

	SuppressMuzzleSmoke		= FALSE;
	SuppressMuzzleLight     = FALSE;
	SuppressTracerSmoke		= FALSE;
	SuppressSmallClutter	= FALSE;
	SuppressPlaneClutter	= FALSE;
	LODFieldOfView			= FALSE;
	LODLimitPerFrame		= FALSE;

	g_iFSSASamples = 0;
	g_bFSSAHighQuality = FALSE;
	g_bSoundHighQuality = TRUE;
	g_bSoundForceStereo = FALSE;
	g_bSoundConvertOnLoad = FALSE;
	g_bCaptuerAllDisplays = TRUE;

	gIconFriendlyColor		= colorBLUE;	// DEFAULT_FRIENDLY_COLOR
	gIconEnemyColor			= colorRED;		// DEFAULT_ENEMY_COLOR
	gIconSquadColor			= colorWHITE;		// DEFAULT_SQUAD_COLOR
	gIconMissionColor		= colorCYAN;
	gIconMissionLeaderColor = colorMAGENTA;

	DamageLevel			= 0;
	FrameFactor			= 1.0f;
	InvScaleFactor      = 1.0f / ScaleFactor;

	piSetMinTextureDim(MinTextureDim);

	MipmapDistance = MipmapDistance * MipmapDistance * ScaleFactor;

	g_iAmbLevel = 1;

	g_bRadialClutter = TRUE;
	g_fRadialDensity = .5f;
	g_fRadialRadius = .5f;


	// These are permanently disabled, should remove completely from code at some point.
	SuppressPlaneClutter = FALSE;
	SuppressSmallClutter = FALSE;

	// Always use multitexture
	MultiTextureEnable = TRUE;

	TextureMagFilter    = piTEXTUREMAG_DEFAULT;				// texture mag filter
	TextureMinFilter    = piTEXTUREMIN_DEFAULT;				// texture min filter
	TextureMipFilter	= piTEXTUREMIP_DEFAULT;				// texture mipmap filter

	// These are no longer available in settings.exe.
	SquadInvites = TRUE;
	bMipmapEnable = TRUE;
	bTexturesDisabled = FALSE;
	g_bCursorOffAtSpawn = TRUE;
	g_iNumBullets = 128;

	g_iVisiblePlayerLimit = 2; // medium
	g_useNetworkRoute = 0;
	g_wantSSE2 = FALSE;

	g_bTreeShader = TRUE;
	gWaterShadersEnabled = TRUE;
	g_wantWaterReflections = FALSE;
	g_postRenderFilter = TRUE;
	g_grassShader = TRUE;
	g_wantNormalMaps = FALSE;
	WantSpecularMaps = TRUE;

	WantBloodEffects = (gRegion == IDE_LANGUAGE_CHINESE) ? FALSE : TRUE;
	ShadowMapFlags = (1|4|16|32);
	WantCombatSmoke = FALSE;
	LowMemorySystem = FALSE;

	g_updateReflectionRate = 3; // 0 - 5 seconds

#if platOS == OS_MACOS
	g_bTreeShader = FALSE;
#endif

	gTrooperLOD = 1.0f;
	gTrooperMOD = 1.0f;
	gRagdolls = TRUE;
	gVCache = 0; // disabled by default

	// are we running custom scenario terrain
	gCustomTerrain = FALSE;
	gEZMode = 0;

#ifdef BETA_RELEASE
	gPersDecalOverride = 0;
	gUnitDecalOverride = 0;
	gr2DrawFlags = 1;
#endif

	snprintf(WantedVideoResolution, sizeof(WantedVideoResolution), "800x600x32");

	// special processing gets handled by a call to cfgPrefsSyncGlobals() next
}

/*************************************************************************************************
	cfgPrefsSetResolution
*************************************************************************************************/

void cfgPrefsSetResolution()
{
	SCHAR str[16];
	UINT32 Position = 0;
	UINT32 Length = 0;

	// Load up the resolution stuff
	// Width
	Length = strcspn(WantedVideoResolution, "x");
	strncpy(str, WantedVideoResolution, Length);
	str[Length] = 0;
	Position += Length + 1;
	WantedWidth = atoi(str);

	// Height
	Length = strcspn(WantedVideoResolution + Position, "x");
	strncpy(str, WantedVideoResolution + Position, Length);
	str[Length] = 0;
	Position += Length + 1;
	WantedHeight = atoi(str);

	// Depth
	Length = strcspn(WantedVideoResolution + Position, "x");
	strncpy(str, WantedVideoResolution + Position, Length);
	str[Length] = 0;
	Position += Length + 1;
	WantedBpp = atoi(str);

	// Refresh Rate
	Length = strcspn(WantedVideoResolution + Position, "x");
	if(Length > 0)
	{
		strncpy(str, WantedVideoResolution + Position, Length);
		str[Length] = 0;
		WantedRefreshRate = atoi(str);
	}
}

/*************************************************************************************************
	cfgPrefsProcessGlobal
*************************************************************************************************/

/*
	This function is called after two conditions. Its called unconditionally for each global
	variable on startup so that all globals that need to be processed in some special way can be.
	Its also called after a global is set to some new value successfully and that global is
	specified as a dynamically updated global variable in the _prefDetails array above.
*/

void cfgPrefsProcessGlobal(_cfgPARAMS key)
{
	// do extra post-set processing here
	switch(key)
	{
	case cfgRADIAL_CLUTTER_ENABLE:
	case cfgRADIAL_CLUTTER_DENSITY:
	case cfgRADIAL_CLUTTER_RADIUS:
		{
			radclutterDirty();
			break;
		}
		case cfgVIDEO_CARD_RESOLUTION:
		{
			cfgPrefsSetResolution();
		}
		break;
		case cfgFONT_SIZE:
		{
			uiSetFontSize(cfgPrefsGetInt(cfgPrefsGetGlobalName(cfgFONT_SIZE), UI_FONT_SIZE));
		}
		break;
		case cfgCORPSES:
		{
			if(!cfgPrefsGetBool("app.corpses", true))
				gr2AnimExpireCorpses();
		}
		break;
		case cfgLANGUAGE_CHOICE:
		{
			loclLangInit((ENUM_LANGUAGES)gLanguage);
			if(_langInited) locInitStrings(); // don't do it the first time; uiInit handles the first init to maintain proper game init ordering
			_langInited = true; // let locInitStrings() be called next time through
			StratDB::InitDisplayNames();
			uiDotCommandsInitLoc(); // update dot loc names and help strings
			uiSetFontSize(cfgPrefsGetInt(cfgPrefsGetGlobalName(cfgFONT_SIZE), UI_FONT_SIZE));
			uiRunScript("game.onlanguagechange()"); // let the ui know the language has changed
			cfgPrefsSendSharedPref(CPR_CLIENT_LANGUAGE); // send it to the host
			cfgPrefsSendSharedPref(CPR_PLAYER_LANGUAGE); // send it to the host
			if(Online) uiChatInitWordFilter();
			// reset the font we're currently using
		}
		break;
		case cfgENABLE_TOOLTIPS:
		{
			uiSetToolTips(cfgPrefsGetBool(cfgPrefsGetGlobalName(cfgENABLE_TOOLTIPS), true));
		}
		break;
		case cfgICON_MODE:
		{
			vehSetIconMode(cfgPrefsGetInt(cfgPrefsGetGlobalName(cfgICON_MODE), 0));
		}
		break;
		case cfgMUSIC_MULTIPLIER:
		case cfgENABLE_MUSIC:
		{
			uiRunScript("game.onmusicchange()"); // let the ui know music options have changed
		}
		break;
		case cfgWANT_SSE2: // boolean supported checks
		case cfgENABLE_H2O_SHADERS:
		case cfgH2O_REFLECTIONS:
		case cfgH2O_REFRACTIONS:
		case cfgPOST_RENDER_FILTER:
		case cfgGRASS_SHADER:
		case cfgNORMAL_MAPS:
		case cfgVIDEO_NO_FRAME_RUN_AHEAD:
		case cfgVIDEO_ENABLE_VSYNC:
		{
			/*
			if(_prefsInited && !piinfoIsSupportedPref(key)) // defaults to true!
			{
				// update the global, if one
				if(_prefDetails[key].var)
					*(bool*)_prefDetails[key].var = false;

				// update the pref object
				cfgPrefsSetBool(_prefDetails[key].name, false);
			}
			*/
#if platOS == OS_MACOS
			if(key == cfgVIDEO_ENABLE_VSYNC) ddpHandleVsync(); // update on the fly
#endif

		}
		break;
		case cfgIS_RECRUITING:
		{
			cfgPrefsSendSharedPref(CPR_IS_RECRUITING); // send it to the host
		}
		break;
		case cfgAUTO_LEADER:
		{
			cfgPrefsSendSharedPref(CPR_AUTO_LEADER); // send it to the host
		}
		break;
		case cfgVISIBLE_PLAYER_LIMIT:
		{
			cfgPrefsSendSharedPref(CPR_UPDATE_MODE); // send it to the host
		}
		break;
		case cfgNETCODE:
		{
			cfgPrefsSendSharedPref(CPR_SET_NETCODE); // send it to the host
		}
		break;
		case cfgAWS_LOW:
		{
			int color = 0;

			if(!cfgPrefsGetInt(_prefDetails[key].name, &color))
			{
				color = piRGB_MAKE(255,255,0);
				cfgPrefsSetInt(_prefDetails[key].name, color);
			}
		}
		break;
		case cfgAWS_HIGH:
		{
			int color = 0;

			if(!cfgPrefsGetInt(_prefDetails[key].name, &color))
			{
				color = piRGB_MAKE(255,0,0);
				cfgPrefsSetInt(_prefDetails[key].name, color);
			}
		}
		break;
		case cfgPPO_OK:
		{
			int color = 0;

			if(!cfgPrefsGetInt(_prefDetails[key].name, &color))
			{
				color = piRGB_MAKE(0,255,0);
				cfgPrefsSetInt(_prefDetails[key].name, color);
			}

			fpPPOResetMaterial();
		}
		break;
		case cfgPPO_NOK:
		{
			int color = 0;

			if(!cfgPrefsGetInt(_prefDetails[key].name, &color))
			{
				color = piRGB_MAKE(255,0,0);
				cfgPrefsSetInt(_prefDetails[key].name, color);
			}

			fpPPOResetMaterial();
		}
		break;
		case cfgSHADOW_SIZE_SELECT:
		{
			// we're only allowing 0 (off) and 6 (on);
			// shadow selections is from 1 to 8, but the
			// other selections are not real useful, so
			// only allow 6 if its on
			if(_prefDetails[key].var)
			{
				int selection = *(int*)_prefDetails[key].var;

				// if its set and its not 6, set it to 6
				if(selection && selection != 6)
				{
					selection = 6;

					// update the pref's var
					*(int*)_prefDetails[key].var = selection;

					// update the pref object
					cfgPrefsSetInt(_prefDetails[key].name, selection);
				}
			}
		}
		break;
		case cfgSETDECAL:
		{
			cfgPrefsSendSharedPref(CPR_SET_DECAL); // send it to the host
			break;
		}
		case cfgSHADOW_RECEIVERS:
		{
#if RELEASE_CLIENT != 0
			if(ShadowMapFlags == 0)
			{
				ShadowMapFlags = (SHADOW_TERRAIN | SHADOW_VEHICLES | SHADOW_CORPSES);
			}
#endif
			break;
		}

		break;
		case cfgVEH_CONV:
		{
			persGunsightSetConvergenceRange((float)Convergence);
			uiChatPrintLine(L"Default convergence set to %.0fm ", persGunsightConvergenceRange());
			break;
		}

		case cfgAIM_SENS:
			{
				// Set me?
				break;
			}
        default:
            // nothing else applies
            break;


	}
}
