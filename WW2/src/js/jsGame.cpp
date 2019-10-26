#include "syscfg.h" // Path is project specific.
#include <string.h>

#include "phui/View.h"
#include "phui/DOM.h"

#include "build_info.hpp"
#include "include/vars.h"
#include "js/jsPlayer.h"
#include "js/jsGame.h"
#include "js/jsStratDB.h"
#include "js/jsVehicle.h"
#include "js/jsClient.h"
#include "include/systypes.h"
#include "veh/vehtype.h"
#include "ml/mltypes.h"
#include "ui/uiMain.h"
#include "ui/uiSound.h"
#include "ui/uiMovieView.h"
#include "sysmain.h"
#include "veh/vehmain.h"
#include "imap/ww2imap.h"
#include "cfg/cfgPrefs.h"
#include "map/mapArenaDescription.h"
#include "js/jsKeymap.h"
#include "imap/ww2func.h"
#include "trn/trntod.h"
#include "imap/ww2imap.h"
#include "js/jsContent.h"
#include "veh/vehcomms.h"
#include "imap/iac.h"
#include "pers/persmain.h"
#include "pers/persgunsight.h"
#include "phui/GraphicsContext.h"
#include "phui/ConsoleView.h"
#include "sys/sysLaunchUrl.h"
#include "pi/piinfo.h"
#include "locl/loclApp.h"
#include "ui/uiChat.h"
#include "tut/TutorialManager.h"
#include "trn/trnmain.h"
#include "dimain.h"
#include "trn/trncd.h"
#include "client/GameClient.h"
#include "client/clientSTO.h"
#include "fp/fpFPS.h"
#include "fp/fpTPS.h"
#include "fp/fpPPO.h"
#include "gr2/gr2Anim.h"
#include "build_info.hpp"
#include "scen/ScenarioManager.h"
#include "ml/mlutility.h"

/************************************************************************************************
	jsGame
*************************************************************************************************/

JSClass jsGame::jsclass = JS_CLASS_MACRO("game");
JSFunctionSpec jsGame::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsGame, load, 2),
	JS_FUNCTION_SPEC(jsGame, savesettings, 2),
	JS_FUNCTION_SPEC(jsGame, restoresettings, 2),
	JS_FUNCTION_SPEC(jsGame, close, 1),
	JS_FUNCTION_SPEC(jsGame, quit, 0),
	JS_FUNCTION_SPEC(jsGame, spawn, 2),
	JS_FUNCTION_SPEC(jsGame, despawn, 1),
	JS_FUNCTION_SPEC(jsGame, findview, 1),
	JS_FUNCTION_SPEC(jsGame, arena, 1),
	JS_FUNCTION_SPEC(jsGame, consoleprint, 1),
	JS_FUNCTION_SPEC(jsGame, playsound, 4),
	JS_FUNCTION_SPEC(jsGame, setsoundvolume, 2),
	JS_FUNCTION_SPEC(jsGame, stopsound, 2),
	JS_FUNCTION_SPEC(jsGame, getkeymaps, 2),
	JS_FUNCTION_SPEC(jsGame, savekeymaps, 0),
	JS_FUNCTION_SPEC(jsGame, initkeymaps, 0),
	JS_FUNCTION_SPEC(jsGame, killkeymaps, 0),
	JS_FUNCTION_SPEC(jsGame, getkeymapvehicles, 1),
	JS_FUNCTION_SPEC(jsGame, keymapisdirty, 2),
	JS_FUNCTION_SPEC(jsGame, keymapremoveall, 2),
	JS_FUNCTION_SPEC(jsGame, keymaprestore, 2),
	JS_FUNCTION_SPEC(jsGame, hasuserkeymap, 2),
	JS_FUNCTION_SPEC(jsGame, clearkeymap, 2),
	JS_FUNCTION_SPEC(jsGame, resetkeymap, 2),
	JS_FUNCTION_SPEC(jsGame, htmlencode, 2),
	JS_FUNCTION_SPEC(jsGame, getcontent, 1),
	JS_FUNCTION_SPEC(jsGame, getkeymapcalibration,1),
	JS_FUNCTION_SPEC(jsGame, setkeymapcalibration,2),
	JS_FUNCTION_SPEC(jsGame, enableimapinput,1),
	JS_FUNCTION_SPEC(jsGame, getlocstring,1),
	JS_FUNCTION_SPEC(jsGame, consoleclear, 0),
	JS_FUNCTION_SPEC(jsGame, launchurl, 1),
	JS_FUNCTION_SPEC(jsGame, getpref, 2),
	JS_FUNCTION_SPEC(jsGame, setpref, 3),
	JS_FUNCTION_SPEC(jsGame, getrgbcolor, 0),
	JS_FUNCTION_SPEC(jsGame, gethexcolor, 0),
	JS_FUNCTION_SPEC(jsGame, reloadchat, 0),
	JS_FUNCTION_SPEC(jsGame, okpref, 1),
	JS_FUNCTION_SPEC(jsGame, tutorialevent, 2),
	JS_FUNCTION_SPEC(jsGame, starttutorial, 4),
	JS_FUNCTION_SPEC(jsGame, tutorialcompleted, 2),
	JS_FUNCTION_SPEC(jsGame, settutorialcompleted, 3),
	JS_FUNCTION_SPEC(jsGame, setkeymappreset, 2),
	JS_FUNCTION_SPEC(jsGame, getkeymappresets, 1),
	JS_FUNCTION_SPEC(jsGame, getkeymappreset, 1),
	JS_FUNCTION_SPEC(jsGame, getkeymapkey, 1),
	JS_FUNCTION_SPEC(jsGame, getworldtoscreen, 3),
	JS_FUNCTION_SPEC(jsGame, resetcontrollers, 0),
	JS_FUNCTION_SPEC(jsGame, stoptutorial, 1),
	JS_FUNCTION_SPEC(jsGame, spawnAt, 5),
	JS_FUNCTION_SPEC(jsGame, setcamera, 4),
	JS_FUNCTION_SPEC(jsGame, utf8, 1),
	JS_FUNCTION_SPEC(jsGame, utf16, 1),
	JS_FUNCTION_SPEC(jsGame, chatprint, 1),
	JS_FUNCTION_SPEC(jsGame, uicallback, JS_MAX_CALL_ARGS),
	JS_FUNCTION_SPEC(jsGame, cancelppo, 0),
	JS_FUNCTION_SPEC(jsGame, bailout, 0),
	JS_FUNCTION_SPEC(jsGame, saveconvergence, 3),
	JS_FUNCTION_SPEC(jsGame, getconvergence, 1),
#ifdef GR2_BLEND_EDITOR
	JS_FUNCTION_SPEC(jsGame, addblend, 5),
	JS_FUNCTION_SPEC(jsGame, validblend, 5),
	JS_FUNCTION_SPEC(jsGame, blendname, 5),
	JS_FUNCTION_SPEC(jsGame, saveblend, 1),
	JS_FUNCTION_SPEC(jsGame, removeblend, 1),
	JS_FUNCTION_SPEC(jsGame, addanimation, 2),
	JS_FUNCTION_SPEC(jsGame, addanimevent, 3),
	JS_FUNCTION_SPEC(jsGame, playanimation, 1),
	JS_FUNCTION_SPEC(jsGame, removeanimation, 2),
	JS_FUNCTION_SPEC(jsGame, setblendweapon, 1),
	JS_FUNCTION_SPEC(jsGame, updateblends, 0),
	JS_FUNCTION_SPEC(jsGame, setblendweights, 5),
	JS_FUNCTION_SPEC(jsGame, reorderblend, 3),
	JS_FUNCTION_SPEC(jsGame, usedblend, 5),
	JS_FUNCTION_SPEC(jsGame, clearanimcache, 0),
#endif
	JS_FUNCTION_SPEC(jsGame, startscenario, 2),
	JS_FUNCTION_SPEC(jsGame, stopscenario, 0),
	JS_FUNCTION_SPEC(jsGame, setscenariocomplete, 0),
	JS_FUNCTION_SPEC(jsGame, haspermission, 1),
	JS_FUNCTION_SPEC(jsGame, setplayerdata, 2), // scenario player updates
	JS_FUNCTION_SPEC(jsGame, addplayerdata, 2), // scenario player updates
	{0}
};

JSPropertySpec jsGame::jsproperties[] =
{
	{"game",					jsGame::GAME_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"cps", 					jsGame::CPS_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"time",					jsGame::TIME_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"ingame",					jsGame::INGAME_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"online",					jsGame::ONLINE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"spawnablevehicles",		jsGame::SPAWNABLEVEHICLES_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"settings",				jsGame::SETTINGS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"screenwidth",				jsGame::SCREENWIDTH_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"screenheight",			jsGame::SCREENHEIGHT_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"training",				jsGame::TRAINING_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"personavehicles",			jsGame::PERSONAVEHICLES_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"region",					jsGame::REGION_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"lastentered",				jsGame::LASTENTERED_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"canbail",					jsGame::CANBAIL_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"bailedout",				jsGame::BAILEDOUT_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"scenario",				jsGame::SCENARIO_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"customterrain",			jsGame::CUSTOMTERRAIN_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"scenariocomplete",		jsGame::SCENARIOCOMPLETE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"scenariospawns",			jsGame::SCENARIOSPAWNS_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"ezmode",					jsGame::EZMODE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
#ifdef GR2_BLEND_EDITOR
	{"blendeditor",				jsGame::BLENDEDITOR_PROP,					JSPROP_READONLY | JSPROP_PERMANENT},
#endif
	{0}
};

/************************************************************************************************
	jsGame
*************************************************************************************************/

jsGame::jsGame() : phui::js::Object(&jsclass, nullptr, nullptr,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	phui::js::SetGlobalObject(this);

	DefineProperty("player", jsplayer = new jsPlayer);
	DefineProperty("cursor", new jsCursor);
	//DefineProperty("settings", new jsSettings);
	DefineProperty("screenwidth", INT_TO_JSVAL(uiViewport()->Width()));
	DefineProperty("screenheight", INT_TO_JSVAL(uiViewport()->Height()));

	// scenario interface
	DefineProperty("scenariostate", new jsScenarioState);

#if defined(BETA_RELEASE)
	DefineProperty("betarelease", INT_TO_JSVAL(1));
#else
	DefineProperty("betarelease", INT_TO_JSVAL(0));
#endif

#if platOS == OS_WIN32
	DefineProperty("ostype", phui::js::ConstString("win"));
#else
	DefineProperty("ostype", phui::js::ConstString("mac"));
#endif
	std::string hostversion;
	getBuildInfo(hostversion);
	DefineProperty("version", phui::js::NewString(PN_product_version_string));
	DefineProperty("buildinfo", phui::js::NewString(hostversion.c_str()));
	DefineProperty("revision", phui::js::NewString((std::string(PN_code_git_commitish_string)+PN_data_git_commitish_string).c_str()));
	cps = new phui::js::Array;
	keymaps = new phui::js::Array;
	spawnablevehicles = new phui::js::Array;
	worldtoscreen = new phui::js::Array;
	scenariospawns = new phui::js::Array;
	settings = new jsSettings;
#ifdef GR2_BLEND_EDITOR
	blendeditor = new jsBlendEditor;
#endif
};

jsGame::~jsGame()
{
}

/************************************************************************************************
	getBuildInfo
*************************************************************************************************/

void jsGame::getBuildInfo(std::string& display)
{
	ARENA_VARS::iterator itr = g_arenaDescription.find("host_rev");

	if(Online && itr != g_arenaDescription.end())
	{
		display = " Host: ";
		display += itr->second.c_str();
	}
	else if(Online)
	{
		display = " Host unknown";
	}
	else
	{
		display = " Host offline";
	}
}

/************************************************************************************************
	cancelppo
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, cancelppo)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	fpPPOCancel();

	return JS_TRUE;
}

/************************************************************************************************
	bailout
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, bailout)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	vehmainBailout();

	return JS_TRUE;
}


/************************************************************************************************
	load
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame,load)
{
    
	phui::View* view = uiLoadView(JS_GetStringBytes(JS_ValueToString(ctx, argv[0])));

	if(view)
	{
		if(argc == 2)
		{
			phui::DOM::ElementDOM* dom = phui::js::Cast<phui::DOM::ElementDOM*>(ctx,argv[1]);

			if(dom)
			{
				uiViewport()->RemoveChild(view);
				dom->_view->AddChild(view);
			}
		}

		phui::js::Object* obj = dynamic_cast<phui::js::Object*>(view->UserData(DOM_VIEW_USERDATA));

		if(obj)
			*rval = obj->jsVal();
		else
			*rval = JSVAL_NULL;
	}

	return JS_TRUE;
}

/************************************************************************************************
	savesettings
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, savesettings)
{
	if(argc < 1) return JS_TRUE;

	phui::DOM::ElementDOM* dom = phui::js::Cast<phui::DOM::ElementDOM*>(ctx,argv[0]);
	if(dom)
	{
		// what is a bitfield; see cfgPrefs.h for more details
		int32 what = SAVE_VIEW_VALUE;

		if(argc > 1)
			JS_ValueToInt32(ctx, argv[1], &what);

		cfgPrefsSaveFromView(dom->_view, what);
	}

	return JS_TRUE;
}

/************************************************************************************************
	restoresettings
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, restoresettings)
{
	if(argc < 1) return JS_TRUE;

	phui::DOM::ElementDOM* dom = phui::js::Cast<phui::DOM::ElementDOM*>(ctx,argv[0]);
	if(dom)
	{
		// what is a bitfield; see cfgPrefs.h for more details
		int32 what = SAVE_VIEW_VALUE;

		if(argc > 1)
			JS_ValueToInt32(ctx, argv[1], &what);

		cfgPrefsRestoreToView(dom->_view, what);
	}

	return JS_TRUE;
}

/************************************************************************************************
	close
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, close)
{
	if(argc < 1) return JS_TRUE;

	const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	phui::View* view = uiViewport()->FindView(name);
	if(view)
		view->Close();

	return JS_TRUE;
}

/************************************************************************************************
	quit
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, quit)
{
	// let the client know we're quitting
	if(Online)
		PlayerClient()->OnExit();

	sysmainQuit();

	return JS_TRUE;
}

/************************************************************************************************
	consoleprint
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, consoleprint)
{
	if(argc < 1) return JS_FALSE;

	const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);
	if(wstr)
		uiConsoleprintf(UTF8(wstr));

	return JS_TRUE;
}

/************************************************************************************************
	consoleclear
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, consoleclear)
{
	phui::View* view = uiViewport()->FindView("terminal");

	if(view)
	{
		phui::ConsoleView* terminal = dynamic_cast<phui::ConsoleView*>(view->FindView("terminal-display"));

		if(terminal)
			terminal->SetText("");
	}

	return JS_TRUE;
}

/************************************************************************************************
	playsound
*************************************************************************************************/

#define SOUND_PLAY_LIMIT 300

JS_FUNCTION_DEF(jsGame, playsound)
{
	if(argc < 3) return JS_FALSE;

	static int lastplaytime = 0;
	const SCHAR* file		= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	SINT32 ID				= -1;
	JSBool finite			= 1;
	JSBool limit			= 1; // do we time limit ui sounds to prevent double clicks, etc...
	JSFloat64 volume		= 1.0;
	const SCHAR* callback	= nullptr;

	JS_ValueToBoolean(ctx, argv[1], &finite);
	JS_ValueToNumber(ctx, argv[2], &volume);

	if(argc >= 4)
		callback = JS_GetStringBytes(JS_ValueToString(ctx, argv[3]));

	if(argc >= 5)
		JS_ValueToBoolean(ctx, argv[4], &limit);

	if(!limit || !lastplaytime || (frametime - lastplaytime > SOUND_PLAY_LIMIT))
	{
		ID = uiPlaySound(file, finite, volume, callback);
		lastplaytime = frametime;
	}

	*rval = INT_TO_JSVAL(ID);

	return JS_TRUE;
}

/************************************************************************************************
	setsoundvolume
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, setsoundvolume)
{
	if(argc < 2) return JS_FALSE;

	int32 ID = 0;

	JS_ValueToInt32(ctx, argv[0], &ID);

	JSFloat64 volume = 0.0;

	JS_ValueToNumber(ctx, argv[1], &volume);

	uiSoundSetVolume(ID, volume);

	return JS_TRUE;
}

/************************************************************************************************
	stopsound
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, stopsound)
{
	if(argc < 1) return JS_FALSE;

	int32 ID		= 0;
	JSBool callback	= 1;

	JS_ValueToInt32(ctx, argv[0], &ID);

	if(argc == 2)
		JS_ValueToBoolean(ctx, argv[1], &callback);

	uiSoundStop(ID, callback);

	return JS_TRUE;
}

const StratDB::StratObject* jsGame::GetOfflineSpawn(const StratDB::StratObject* origin, const void* vt)
{
	if(!origin) return nullptr;

	vehType* vehtype = (vehType*)vt;

	static WEAPON_SPEC destroyer = NULL_WEAP_SPEC;
	static WEAPON_SPEC tank = NULL_WEAP_SPEC;
	static WEAPON_SPEC truck = NULL_WEAP_SPEC;
	static WEAPON_SPEC gun = NULL_WEAP_SPEC;
	static WEAPON_SPEC artillery = NULL_WEAP_SPEC;

	static bool firsttime = true;

	if(firsttime)
	{
		firsttime = false;

		destroyer = weapCreateSpec("SV_sea.1");
		tank = weapCreateSpec("GV_tank.1");
		truck = weapCreateSpec("GV_truck.1");
		artillery = weapCreateSpec("GV_artillery.1");
		gun = weapCreateSpec("GV_gun.1");
	}

	StratDB::StratObject const* spawner = nullptr;

	StratDB::StratObject::ChildList children = origin->ChildStratObjects();
	StratDB::StratObject::ChildList::iterator it = children.begin();

	int vtype = vehtype->Type;

	for(; it != children.end(); ++it)
	{
		if((*it)->StratClass() == SDM_CLASS_BUILDING && (*it)->StratType() == SDM_BUILDING_SPAWN)
		{
			spawner = *it;

			if(vtype == VEH_TYPE_GROUND && memcmp(spawner->Filename(), "gv", 2) == 0)
			{
				// Make sure we have the right spawner for this class of vehicle (i.e., not a fire base spawner)
				if(vehtype->WeaponSpec.wClass == tank.wClass ||
				   vehtype->WeaponSpec.wClass == truck.wClass ||
				   vehtype->WeaponSpec.wClass == artillery.wClass ||
				   vehtype->WeaponSpec.wClass == gun.wClass)
				{
					if(strncmp(spawner->Filename(), "gv_spawng", 9))
					{
						continue;
					}
				}

				break;
			}
			else if(vtype == VEH_TYPE_PERSON && memcmp(spawner->Filename(), "in", 2) == 0)
				break;
			else if(vtype == VEH_TYPE_SEA && memcmp(spawner->Filename(), "sv", 2) == 0)
			{
				if(vehtype->WeaponSpec.wClass == destroyer.wClass)
				{
					// Ticket: 4151
					// Need correct spawn for big boats
					if(strncmp(spawner->Filename(), "svz", 3))
					{
						continue;
					}

					break;
				}

				break;
			}
#if(0)
			// No longer uses a spawn object. Buildings use DOFs for spawn objects.
			else if(vtype == VEH_TYPE_AIR && memcmp(spawner->Filename(), "ac", 2) == 0)
				break;
#endif
			else if(vtype == VEH_TYPE_FLOATER)
				break;

			spawner = nullptr;
		}
	}

	if(!Online && !spawner)
	{
		StratDB::StratObject::ChildList::iterator itr = children.begin();

		for(; itr != children.end(); ++itr)
		{
			if((*itr)->StratClass() == SDM_CLASS_BUILDING && (*itr)->StratClass() == SDM_BUILDING_SPAWN)
			{
				// Offline spawning for buildings with DOF spawners. (buildings in spawn.xml)
				if(trncdSpawnDataExists((*itr)->Filename()))
				{
					spawner = *itr;

					switch(vtype)
					{
					case VEH_TYPE_AIR:
						{
							// Make sure we have the right building
							if(spawner->StratType() == SDM_FACILITY_AIRBASE)
								break;
						}
					default:
						break;
					}
				}
			}
		}
	}

	return spawner;
}

jsval jsGame::SetWorldToScreen(double x, double y, double z)
{
	worldtoscreen->clear();

	piMATRIX world = *(piGetWorldMatrix());
	piSetWorldMatrix(&IdentityMatrix);

	FLOAT64VECTOR worldOffset;
	trnGetWorldOriginOffset(&worldOffset);

	x -= worldOffset.x;
	y -= worldOffset.y;
	z = -z;//-= worldOffset.z;

	double sx, sy, sz;
	sx = sy = sz = 0;

	// our screen coords for the position passed in
	piGetWorldToScreen(x, y, z, sx, sy, sz);

	jsval dval;

	JS_NewDoubleValue(this->jsContext(), sx, &dval);
	worldtoscreen->SetElement(dval, 0);

	JS_NewDoubleValue(this->jsContext(), sy, &dval);
	worldtoscreen->SetElement(dval, 1);

	JS_NewDoubleValue(this->jsContext(), sz, &dval);
	worldtoscreen->SetElement(dval, 2);

	piSetWorldMatrix(&world);

	return worldtoscreen->jsVal();
}

/************************************************************************************************
	spawn
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, spawn)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online)
		uiSetDespawnInfo("offline");

	if(argc < 1) return JS_FALSE;
	jsVehtype* jsvehtype = phui::js::Cast<jsVehtype*>(ctx, argv[0]);
	if(!jsvehtype) return JS_FALSE;

	vehType const* vehtype = jsvehtype->vehtype;

	if(SCEN_MGR->IsActive())
	{
		/*
		//uiConsoleprintf("SCENARIO: >>>>>>>>>>>>>>>>>");

		FLOAT64VECTOR at = { 0.0f, 0.0f, 0.0f };
		FLOAT64 yaw = 0;

		const ScenarioWorldObject* spawn = SCEN_MGR->VehicleSpawn(vehtype);
		if(!spawn && Online) return JS_TRUE;

		if(spawn)
		{
			//uiConsoleprintf("SCENARIO: Spawn object found!");
			at.x	= spawn->position.x();
			at.y	= spawn->position.y();
			at.z	= spawn->position.z();
			yaw		= spawn->attitude.Yaw;

			if(vehtype->Type != VEH_TYPE_PERSON)
			{
				// yaw needs to be adjusted for non-troopers
				yaw = mlRAD_TO_DEG(yaw);
				yaw -= 180.0f;
				gr2NormalizeAngle(yaw);
				yaw = mlDEG_TO_RAD(yaw);
			}
		}
		else
		{
			//uiConsoleprintf("SCENARIO: Spawn object not found!");
		}

		// send the request
		if(Online)
		{
			PlayerClient()->RequestDirectSpawn(vehtype->WeaponSpec, &at, yaw);
		}
		else
		{
			//uiConsoleprintf("SCENARIO: Spawning at %.2f, %.2f, %.2f", &at.x, &at.y, &at.z);
			mlSetSpawnStuff(0,0,-1,1);
			mlSetMobileSpawnStuff(0, at.x, at.y, at.z, yaw);
			WEAPON_SERIAL blah={0};
			vehcommSendVehicleRequest(vehtype->VehicleTypeNum,&blah);
		}

		// save our respawn weapon spec
		SCEN_MGR->SetRespawnWeapon(vehtype->WeaponSpec);

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

		if(Online)
			uiSetDespawnInfo("scenarios/scenario");
		else
			uiSetDespawnInfo("offline");
		*/

		if(SCEN_MGR->Spawn(vehtype))
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

		return JS_TRUE;
	}

	mlSetSpawnStuff(0,0,-1,1);

	if(argc > 1)
	{
		jsStratObject* jsstratobj = phui::js::Cast<jsStratObject*>(ctx, argv[1]);
		if(jsstratobj)
		{
			const StratDB::StratObject* spawnobj = GetOfflineSpawn(jsstratobj->stratobj, (void*)vehtype);

			if(spawnobj)
			{
				mlSetSpawnStuff(spawnobj->SupercellX(),spawnobj->SupercellY(), spawnobj->Id(),1);
				*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			}
		}
	}

	WEAPON_SERIAL blah={0};
	vehcommSendVehicleRequest(vehtype->VehicleTypeNum,&blah);

	return JS_TRUE;
}

/************************************************************************************************
	spawnAt
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, spawnAt)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsVehtype* jsvehtype = phui::js::Cast<jsVehtype*>(ctx, argv[0]);
	if(!jsvehtype) return JS_FALSE;

	vehType const* vehtype = jsvehtype->vehtype;

	if(!uiInGame() && argc >= 4)
	{
		FLOAT64VECTOR at;
		at.x = at.y = at.z = 0.0f;
		FLOAT64 yaw = 0;

		JS_ValueToNumber(ctx, argv[1], &at.x);
		JS_ValueToNumber(ctx, argv[2], &at.y);
		JS_ValueToNumber(ctx, argv[3], &at.z);

		if(argc >= 5)
			JS_ValueToNumber(ctx, argv[4], &yaw);

		if(Online)
		{
			// send the request
			PlayerClient()->RequestDirectSpawn(vehtype->WeaponSpec, &at, yaw);
		}
		else
		{
			// do old game spawn stuff here
			mlSetSpawnStuff(0, 0, -1, 1);
			mlSetMobileSpawnStuff(0, at.x, at.y, at.z, yaw);
			WEAPON_SERIAL blah={0};
			vehcommSendVehicleRequest(vehtype->VehicleTypeNum, &blah);
		}

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	setcamera
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, setcamera)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc <= 1)
	{
		int32 mode = 0;
		JS_ValueToInt32(ctx, argv[0], &mode);

		if(persGetCameraMode() != mode)
			persToggleCamera();

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}
	else if(argc == 3)
	{
		jsdouble x, y, z;

		JS_ValueToNumber(ctx, argv[0], &x);
		JS_ValueToNumber(ctx, argv[1], &y);
		JS_ValueToNumber(ctx, argv[2], &z);

		persSetExternalCameraPos((FLOAT32)x, (FLOAT32)y, (FLOAT32)z);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}
	else if(argc == 4)
	{
		jsdouble x, y, z, d;

		JS_ValueToNumber(ctx, argv[0], &x);
		JS_ValueToNumber(ctx, argv[1], &y);
		JS_ValueToNumber(ctx, argv[2], &z);
		JS_ValueToNumber(ctx, argv[3], &d);

		persSetExternalCameraTarget((FLOAT32)x, (FLOAT32)y, (FLOAT32)z, (FLOAT32)d);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	despawn
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, despawn)
{
	// make sure this gets cleared because they actively clicked the despawn button
	SCEN_MGR->SetFlag(ScenarioManager::RESPAWN_ME, false);

	if(argc > 0)
	{
		JSBool respawn = 0;
		JS_ValueToBoolean(ctx, argv[0], &respawn);

		if(respawn)
			SCEN_MGR->SetFlag(ScenarioManager::RESPAWN_ME, true);
	}

	persSetDeathCamForNextUpdate();
	vehExitVehicle();
	return JS_TRUE;
}

/************************************************************************************************
	findview
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, findview)
{
	if(argc < 1) return JS_FALSE;

	const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	phui::View* view = uiViewport()->FindView(name);

	if(view)
	{
		phui::DOM::ElementDOM* viewdom = dynamic_cast<phui::DOM::ElementDOM*>(view->UserData(DOM_VIEW_USERDATA));
		if(viewdom)
			*rval = OBJECT_TO_JSVAL(viewdom->jsObject());
	}

	return JS_TRUE;
}

/************************************************************************************************
	arena -- lookup a variable name in the g_arenaDescription map
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, arena)
{
	if(argc < 1) return JS_FALSE;

	const char* key = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(key != nullptr && *key != 0)
	{
		ARENA_VARS::iterator it = g_arenaDescription.find(key);
		if(it != g_arenaDescription.end())
		{
			*rval = phui::js::NewString(it->second.c_str());
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	getkeymaps
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getkeymaps)
{
	jsGame* jsgame = phui::js::Cast<jsGame*>(ctx, jsobj);
	if(!jsgame) return JS_FALSE;

	int32 category = 0;
	int32 vehidx = -1;

	if(argc > 0)
		JS_ValueToInt32(ctx, argv[0], &category);
	if(argc > 1)
		 JS_ValueToInt32(ctx, argv[1], &vehidx);

	jsgame->keymaps->clear();

	for(unsigned int i = 0;; i++)
	{
		unsigned int functioncode = ww2funcGetCode(category, vehidx, i);
		if(functioncode == IMAP_NO_FUNCTION) break;

		jsKeymap* keymap = new jsKeymap(ctx, jsobj, category, vehidx, i);
		jsgame->keymaps->SetElement(keymap, i);
	}
	*rval = jsgame->keymaps->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	savekeymaps
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, savekeymaps)
{
	jsGame* jsgame = phui::js::Cast<jsGame*>(ctx, jsobj);
	if(!jsgame) return JS_FALSE;

	ww2KeymapSaveAll();

	if(uiInGame())
	{
		vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
		if(PlayerVehicle)
			ww2imapSetKeymap(PlayerVehicle->VehicleType->VehicleTypeNum);
	}
	return JS_TRUE;
}

/************************************************************************************************
	saveconvergence
*************************************************************************************************/
JS_FUNCTION_DEF(jsGame, saveconvergence)
{
	jsGame* jsgame = phui::js::Cast<jsGame*>(ctx, jsobj);
	if(!jsgame) return JS_FALSE;

	jsVehtype* jsvehtype = nullptr;
	int32 isDirty = 0;		// True if the convergence value has changed
	int32 value = 200;		// Convergence value

	if(argc > 0)
	{
		//JS_ValueToInt32(ctx, argv[0], &category);
		jsvehtype = phui::js::Cast<jsVehtype*>(ctx, argv[0]);

		if(argc > 1) JS_ValueToInt32(ctx, argv[1], &value);
		if(argc > 2) JS_ValueToInt32(ctx, argv[2], &isDirty);
	}

	// create our pref key
	if(jsvehtype)
	{
		char buff[256] = {'\0'};
		strprintf(buff, "convergence.%d", (SINT32)jsvehtype->vehtype->WeaponSpec.asUINT16());

		if(isDirty == 1)		// Is there a pending convergence setting change?
			cfgPrefsSetInt(buff, value);
		else if(isDirty == 0)
			cfgPrefsSetInt(buff, (SINT32)Convergence);

		if(uiInGame())
		{
			uiChatPrintLine("%s convergence set to  %dm.",
				(const char*)(jsvehtype->vehtype->Id),
				cfgPrefsGetInt(buff, (SINT32)Convergence));

			uiChatPrintLine("New convergence will take effect next time you spawn this vehicle.");
		}

		/*
		vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

		// are we a plane and in game?
		if(PlayerVehicle && PlayerVehicle->VehicleType->Type == VEH_TYPE_AIR && uiInGame())
		{
			// If this is the vehicle we're currently in, let's update the convergence right away, too.
			if(PlayerVehicle->VehicleType->WeaponSpec.asShort() == jsvehtype->vehtype->WeaponSpec.asUINT16())
				persGunsightSetConvergenceRange((FLOAT32)cfgPrefsGetInt(buff, (SINT32)Convergence));
		}
		*/
	}

	return JS_TRUE;
}
/************************************************************************************************
	getconvergence
*************************************************************************************************/
JS_FUNCTION_DEF(jsGame, getconvergence)
{
	jsGame* jsgame = phui::js::Cast<jsGame*>(ctx, jsobj);
	if(!jsgame) return JS_FALSE;

	int value = (SINT32)Convergence;

	*rval = INT_TO_JSVAL(value);

	if(argc > 0)
	{
		jsVehtype* jsvehtype = phui::js::Cast<jsVehtype*>(ctx, argv[0]);

		if(jsvehtype != nullptr)
		{
			UINT16 key = jsvehtype->vehtype->WeaponSpec.asUINT16();

			char buff[256] = {'\0'};
			strprintf(buff, "convergence.%d", (SINT32)key);

			value = cfgPrefsGetInt(buff, (SINT32)Convergence);

			*rval = INT_TO_JSVAL(value);

			return JS_TRUE;
		}
		else return JS_FALSE;
	}
	else return JS_FALSE;
}

/************************************************************************************************
	initkeymaps
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, initkeymaps)
{
	jsGame* jsgame = phui::js::Cast<jsGame*>(ctx, jsobj);
	if(!jsgame) return JS_FALSE;

	ww2KeymapInit(0);

	return JS_TRUE;
}

/************************************************************************************************
	savekeymaps
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, killkeymaps)
{
	jsGame* jsgame = phui::js::Cast<jsGame*>(ctx, jsobj);
	if(!jsgame) return JS_FALSE;

	ww2KeymapKill();

	return JS_TRUE;
}

/************************************************************************************************
	getkeymapvehicles
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getkeymapvehicles)
{
	int32 category = 0;

	if(argc > 0)
		JS_ValueToInt32(ctx, argv[0], &category);

	phui::js::Array* vehicles = new phui::js::Array;
	const vehType* vehtype = nullptr;

	for(int i = 0;; i++)
	{
		vehtype = ww2imapGetVehTypeByIndex(category, i);

		if(vehtype == nullptr)
			break;

		vehicles->SetElement(new jsVehtype(vehtype), i);
	}

	*rval = vehicles->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	getkeymapkey
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getkeymapkey)
{
	if(argc < 1) return JS_FALSE;

	const char* control = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	int32 key = 0;
	int32 type = -1;

	if(argc > 1) JS_ValueToInt32(ctx, argv[1], &key);
	if(argc > 2) JS_ValueToInt32(ctx, argv[2], &type);

	const char* keyname = nullptr;
	unsigned int code	= ww2FunctionCode(control);

	if(code > 0)
	{
		vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
		keyname = ww2imapGetFuncKeyname(-1, (PlayerVehicle) ? PlayerVehicle->VehicleType->VehicleTypeNum: -1, code, key, type);
	}

	if(keyname && *keyname)
		*rval = phui::js::NewString(uiUTF8toUTF16(keyname));
	else
		*rval = phui::js::NewString(control);

	return JS_TRUE;
}

/************************************************************************************************
	keymapisdirty
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, keymapisdirty)
{
	int32 category = 0, vehicle = -1;

	if(argc > 0)
	   JS_ValueToInt32(ctx, argv[0], &category);
	if(argc > 1)
	   JS_ValueToInt32(ctx, argv[1], &vehicle);

	*rval = BOOLEAN_TO_JSVAL(ww2KeymapIsDirty(category, vehicle));

	return JS_TRUE;
}

/************************************************************************************************
	keymapremoveall
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, keymapremoveall)
{
	int32 category = 0, vehicle = -1;

	if(argc > 0)
	   JS_ValueToInt32(ctx, argv[0], &category);
	if(argc > 1)
	   JS_ValueToInt32(ctx, argv[1], &vehicle);

	ww2KeymapRemoveAll(category, vehicle);

	return JS_TRUE;
}

/************************************************************************************************
	keymaprestore
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, keymaprestore)
{
	int32 category = 0, vehicle = -1;

	if(argc > 0)
	   JS_ValueToInt32(ctx, argv[0], &category);
	if(argc > 1)
	   JS_ValueToInt32(ctx, argv[1], &vehicle);

	ww2KeymapRestore(category, vehicle);


	return JS_TRUE;
}

/************************************************************************************************
	hasuserkeymap
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, hasuserkeymap)
{
	int32 category = 0, vehicle = -1;

	if(argc > 0)
	   JS_ValueToInt32(ctx, argv[0], &category);
	if(argc > 1)
	   JS_ValueToInt32(ctx, argv[1], &vehicle);

	*rval = BOOLEAN_TO_JSVAL(ww2KeymapHasUserKeymap(category, vehicle));


	return JS_TRUE;
}

/************************************************************************************************
	clearkeymap
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, clearkeymap)
{
	if(argc < 1) return JS_FALSE;
	jsKeymap* jkeymap = phui::js::Cast<jsKeymap*>(ctx,argv[0]);
	if(!jkeymap) return JS_FALSE;

	int32 keyidx;
	JS_ValueToInt32(ctx, argv[1], &keyidx);

	int realidx = ww2KeymapForIdiotsGetIndex(jkeymap->functionCode, keyidx);
	int keymapType = ww2KeymapForIdiotsGetType(jkeymap->functionCode, keyidx);

	ww2KeymapClear(jkeymap->category, jkeymap->vehicleIdx, jkeymap->functionCode, realidx, keymapType);

	return JS_TRUE;
}

/************************************************************************************************
	resetkeymap
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, resetkeymap)
{
	if(argc < 1) return JS_FALSE;
	jsKeymap* jkeymap = phui::js::Cast<jsKeymap*>(ctx,argv[0]);
	if(!jkeymap) return JS_FALSE;

	int32 keyidx;
	JS_ValueToInt32(ctx, argv[1], &keyidx);

	int realidx = ww2KeymapForIdiotsGetIndex(jkeymap->functionCode, keyidx);
	int keymapType = ww2KeymapForIdiotsGetType(jkeymap->functionCode, keyidx);

	ww2KeymapRemove(jkeymap->category, jkeymap->vehicleIdx, jkeymap->functionCode, realidx, keymapType);

	return JS_TRUE;
}

/************************************************************************************************
	setimappreset
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, setkeymappreset)
{
	if(argc > 1)
	{
		int32 category = 0, preset = 0;

		JS_ValueToInt32(ctx, argv[0], &category);
		JS_ValueToInt32(ctx, argv[1], &preset);

		*rval = BOOLEAN_TO_JSVAL(ww2imapSetPreset(category, preset));
	}

	return JS_TRUE;
}

/************************************************************************************************
	getkeymappresets
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getkeymappresets)
{
	phui::js::Array* names = new phui::js::Array;

	if(argc > 0)
	{
		int32 category = 0;
		JS_ValueToInt32(ctx, argv[0], &category);

		imap_presets* presets = ww2imapGetPresets(category);

		if(presets)
		{
			for(int i = 0; i < (int)presets->size(); i++)
				names->SetElement(phui::js::NewString(presets->at(i).name.c_str()), i);
		}
	}

	*rval = names->jsVal();

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, getkeymappreset)
{
	*rval = INT_TO_JSVAL(-1);

	if(argc > 0)
	{
		int32 category = 0;
		JS_ValueToInt32(ctx, argv[0], &category);

		imap_presets* presets = ww2imapGetPresets(category);

		if(presets)
			*rval = INT_TO_JSVAL(presets->selected);
	}

	return JS_TRUE;
}

/************************************************************************************************
	htmlencode
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, htmlencode)
{
	if(argc < 1) return JS_FALSE;
	const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);

	*rval = phui::js::NewString(phui::html_encode(wstr));

	return JS_TRUE;
}

/************************************************************************************************
	getcontent
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getcontent)
{
	if(argc < 1) return JS_FALSE;

	const char* path = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	jsContent* content = (*path) ? jsContent::GetContent(ctx, jsobj, path): nullptr;

	*rval = (content) ? content->jsVal() : JSVAL_NULL;

	return JS_TRUE;
}

/************************************************************************************************
	getkeymapcalibration
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getkeymapcalibration)
{
	if(argc < 1) return JS_FALSE;

	const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(strcmp(name,"mouse") == 0)
	{
		JS_NewDoubleValue(ctx, ww2KeymapGetMouseSensitivity(), rval);
	}
	else if(strcmp(name,"aim") == 0)
	{
		JS_NewDoubleValue(ctx, ww2KeymapGetMouseAimSensitivity(), rval);
	}

	return JS_TRUE;
}

/************************************************************************************************
	setkeymapcalibration
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, setkeymapcalibration)
{
	if(argc < 2) return JS_FALSE;

	const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(strcmp(name,"mouse") == 0)
	{
		jsdouble scale;
		JS_ValueToNumber(ctx, argv[1], &scale);
		ww2KeymapSetMouseSensitivity((float)scale);
	}
	else if(strcmp(name,"aim") == 0)
	{
		jsdouble scale;
		JS_ValueToNumber(ctx, argv[1], &scale);
		ww2KeymapSetMouseAimSensitivity((float)scale);
	}

	return JS_TRUE;
}

/************************************************************************************************
	enableimapinput
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, enableimapinput)
{
	if(argc < 1) return JS_FALSE;

	JSBool enable;
	JS_ValueToBoolean(ctx, argv[0], &enable);

	disableButtonInput = (enable ? FALSE : TRUE);
	disableAxisInput = (enable ? FALSE : TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	getlocstring -- lookup a localized string
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getlocstring)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;

	const char* key = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(key && *key)
		*rval = phui::js::NewString((uiLocString(key)).c_str()); // uiLocString always returns something

	return JS_TRUE;
}

/************************************************************************************************
	launchurl
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, launchurl)
{
	*rval = BOOLEAN_TO_JSVAL(false);

	if(argc < 1) return JS_TRUE;

	const SCHAR* url = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(syslaunchurl(url))
		*rval = BOOLEAN_TO_JSVAL(true);

	return JS_TRUE;
}

/************************************************************************************************
	utf8 -- convert a utf16 string to a utf8 string
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, utf8)
{
	if(argc < 1) return JS_FALSE;

	const char* in = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	const wchar_t* win = (const wchar_t*)in;

	*rval = phui::js::NewString(UTF8(win));

	return JS_TRUE;
}

/************************************************************************************************
	utf16 -- convert a utf8 string to a utf16 string
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, utf16)
{
	if(argc < 1) return JS_FALSE;

	const char* in = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	*rval = phui::js::NewString(UTF16(in));

	return JS_TRUE;
}

/************************************************************************************************
	uicallback -- callback into the c++ game layer
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, uicallback)
{
	*rval = JSVAL_NULL;

	// call the callback
	uiJSCallback(ctx, argc, argv);

	return JS_TRUE;
}

/************************************************************************************************
	chatprint
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, chatprint)
{
	if(argc < 1) return JS_FALSE;

	const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);

	uiChatPrintLine(wstr);

	return JS_TRUE;
}


/************************************************************************************************
	getpref
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getpref)
{
	*rval = (argc > 2) ? argv[2]: JSVAL_NULL; // optional 3rd argument is the default value

	if(argc < 2) return JS_FALSE;

	const SCHAR* name	= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	int32 type			= -1;

	JS_ValueToInt32(ctx, argv[1], &type);

	switch(type)
	{
		case cfgPARAM_BOOLEAN:
		{
			bool bval = false;
			if(cfgPrefsGetBool(name, &bval))
				*rval = BOOLEAN_TO_JSVAL(bval);
		}
		break;
		case cfgPARAM_UINT:
		case cfgPARAM_SINT:
		{
			int ival = 0;
			if(cfgPrefsGetInt(name, &ival))
				*rval = INT_TO_JSVAL(ival);
		}
		break;
		case cfgPARAM_LONG:
		{
			long lval = 0;
			if(cfgPrefsGetLong(name, &lval))
				*rval = INT_TO_JSVAL((int)lval);
		}
		break;
		case cfgPARAM_FLOAT:
		{
			float fval = 0.0f;
			if(cfgPrefsGetFloat(name, &fval))
				JS_NewDoubleValue(ctx, fval, rval);
		}
		break;
		case cfgPARAM_DOUBLE:
		{
			double dval = 0.0f;
			if(cfgPrefsGetDouble(name, &dval))
				JS_NewDoubleValue(ctx, dval, rval);
		}
		break;
		case cfgPARAM_STRING:
		{
			const char* sval = nullptr;
			if(cfgPrefsGetString(name, &sval))
				*rval = phui::js::NewString(sval);
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	okpref
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, okpref)
{
	*rval = JSVAL_FALSE;

	if(argc < 1) return JS_FALSE;

	const SCHAR* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	int key = cfgPrefsGetGlobalKey(name);

	if(key >= 0)
		*rval = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(key));

	return JS_TRUE;
}

/************************************************************************************************
	tutorialevent
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, tutorialevent)
{
	*rval = JSVAL_FALSE;

	if(argc < 1) return JS_FALSE;

	int32 ival = 0;
	JS_ValueToInt32(ctx, argv[0], &ival);

	if((UINT32)ival == TUT_EVENT_MAP)
	{
		int32 param = 0;

		if(argc > 1)
			JS_ValueToInt32(ctx, argv[1], &param);

		TUT_MGR->DoEvent(TUT_EVENT_MAP, &param);
		*rval = JSVAL_TRUE;
	}
	else if(ival && ((UINT32)ival & TUTORIAL_UI_EVENTS))
	{
		int ok = true;

		// UI events can pass an optional true/false value along
		if(argc > 1)
		{
			JSBool bval = true;
			JS_ValueToBoolean(ctx, argv[1], &bval);
			ok = (int)bval;
		}

		TUT_MGR->DoEvent((UINT32)ival, &ok);
		*rval = JSVAL_TRUE;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, starttutorial)
{
	*rval = JSVAL_FALSE;

	if(argc < 1) return JS_FALSE;

	const SCHAR* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(argc >= 2)
	{
		const SCHAR* despawnto = JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));

		int32 tab = 0;
		JSBool reset = 1;

		if(argc >= 3)
			JS_ValueToInt32(ctx, argv[2], &tab);

		if(argc >= 4)
			JS_ValueToBoolean(ctx, argv[3], &reset);

		uiSetDespawnInfo(despawnto, (int)tab, (reset == 1));
	}

	if(TUT_MGR->Start(name))
		*rval = JSVAL_TRUE;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, stoptutorial)
{
	*rval = JSVAL_TRUE;

	// stop the current tutorial
	TUT_MGR->Stop();

	if(argc > 0)
	{
		JSBool reset = 1;
		JS_ValueToBoolean(ctx, argv[0], &reset);

		if(reset)
			PlayerClient()->LoadSavedPersona();
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, tutorialcompleted)
{
	*rval = JSVAL_FALSE;

	if(argc < 2) return JS_FALSE;

	const SCHAR* id = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(id && *id)
	{
		int32 ival = -1;
		JS_ValueToInt32(ctx, argv[1], &ival);
		int key = (int)ival;

		if(TUT_MGR->IsComplete(id, key))
			*rval = JSVAL_TRUE;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, settutorialcompleted)
{
	*rval = JSVAL_FALSE;

	if(argc < 2) return JS_FALSE;

	const SCHAR* id = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	if(id && *id)
	{
		int key = -1;
		bool completed = true;

		int32 ival = -1;
		JS_ValueToInt32(ctx, argv[1], &ival);
		key = (int)ival;

		if(argc > 2)
		{
			JSBool bval = 1;
			JS_ValueToBoolean(ctx, argv[2], &bval);
			completed = (bval != 0);
		}

		TUT_MGR->SetComplete(id, key, completed);
		*rval = JSVAL_TRUE;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, startscenario)
{
	*rval = JSVAL_FALSE;

	if(argc < 1) return JS_FALSE;

	const SCHAR* path = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	bool init = true;

	if(argc > 1)
	{
		JSBool bval = 1;
		JS_ValueToBoolean(ctx, argv[1], &bval);
		init = (bval != 0);
	}

	if(SCEN_MGR->Start(path, init))
		*rval = JSVAL_TRUE;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, stopscenario)
{
	*rval = JSVAL_TRUE;

	// stop the current tutorial
	SCEN_MGR->Stop();

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, setscenariocomplete)
{
	*rval = JSVAL_TRUE;

	// stop the current tutorial
	SCEN_MGR->SetComplete();

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, haspermission)
{
	if(Online)
	{
		*rval = JSVAL_FALSE;

		if(argc >= 1)
		{
			const SCHAR* perm = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

			if(perm && *perm && PlayerClient()->IsPermitted(ScenarioPermission::GetValue(perm), false))
				*rval = JSVAL_TRUE;
		}
	}
	else
	{
		*rval = JSVAL_TRUE;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, setplayerdata)
{
	*rval = JSVAL_FALSE;

	if(argc < 2 || !SCEN_MGR->IsActive())
		return JS_FALSE;

	std::string name	= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	std::string value	= JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));
	ScenarioPlayer* me	= SCEN_MGR->GetPlayer();

	if(me)
	{
		if(strcasecmp(name.c_str(), "weapon") == 0)
		{
			WEAPON_SPEC spec;
			spec.fromShort((UINT16)atoi(value.c_str()));

			if(vehTypeGetBySpec(spec))
				SCEN_MGR->SetRespawnWeapon(spec);
		}

		me->Set(name.c_str(), value.c_str());
		*rval = JSVAL_TRUE;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, addplayerdata)
{
	*rval = JSVAL_FALSE;

	if(argc < 2 || !SCEN_MGR->IsActive())
		return JS_FALSE;

	std::string name	= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	std::string value	= JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));
	ScenarioPlayer* me	= SCEN_MGR->GetPlayer();

	if(me)
	{
		me->Add(name.c_str(), value.c_str());
		*rval = JSVAL_TRUE;
	}

	return JS_TRUE;
}

/************************************************************************************************
	setpref
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, setpref)
{
	bool result = false;

	if(argc >= 3)
	{
		const SCHAR* name	= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
		int32 type			= -1;

		JS_ValueToInt32(ctx, argv[1], &type);

		switch(type)
		{
			case cfgPARAM_BOOLEAN:
			{
				JSBool bval = false;
				JS_ValueToBoolean(ctx, argv[2], &bval);
				result = cfgPrefsSetBool(name, bval);
			}
			break;
			case cfgPARAM_UINT:
			case cfgPARAM_SINT:
			{
				int32 ival = 0;
				JS_ValueToInt32(ctx, argv[2], &ival);
				result = cfgPrefsSetInt(name, ival);
			}
			break;
			case cfgPARAM_LONG:
			{
				int32 ival = 0;
				JS_ValueToInt32(ctx, argv[2], &ival);
				result = cfgPrefsSetLong(name, (long)ival);
			}
			break;
			case cfgPARAM_FLOAT:
			{
				double dval = 0.0f;
				JS_ValueToNumber(ctx, argv[2], &dval);
				result = cfgPrefsSetFloat(name, (float)dval);
			}
			break;
			case cfgPARAM_DOUBLE:
			{
				double dval = 0.0f;
				JS_ValueToNumber(ctx, argv[2], &dval);
				result = cfgPrefsSetDouble(name, dval);
			}
			break;
			case cfgPARAM_STRING:
			{
				std::string key = name;
				const char* value = JS_GetStringBytes(JS_ValueToString(ctx, argv[2]));
				result = cfgPrefsSetString(key.c_str(), value);

			}
			break;
		}
	}

	*rval = BOOLEAN_TO_JSVAL(result);

	return JS_TRUE;
}

/************************************************************************************************
	getrgbcolor
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getrgbcolor)
{
	int32 r,g,b;
	r = g = b = 255; // defaults to white

	if(argc == 3)
	{
		JS_ValueToInt32(ctx, argv[0], &r);
		JS_ValueToInt32(ctx, argv[1], &g);
		JS_ValueToInt32(ctx, argv[2], &b);
	}

	*rval = (new jsColor(ctx, jsobj, r, g, b))->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	gethexcolor
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, gethexcolor)
{
	const SCHAR* color = nullptr;

	if(argc == 1)
		color = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	*rval = (new jsColor(ctx, jsobj, color))->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	reloadchat
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, reloadchat)
{
	if(uiChatWindow())
	{
		bool visible = uiChatWindow()->IsVisible();

		uiChatHide(true); // close it
		uiChatShow(); // reload it

		if(!visible)
			uiChatHide(); // resync it back to hidden
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

/************************************************************************************************
	getworldtoscreen
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, getworldtoscreen)
{
	double x, y, z;
	x = y = z = 0;

	if(argc >= 3)
	{
		JS_ValueToNumber(ctx, argv[0], &x);
		JS_ValueToNumber(ctx, argv[1], &y);
		JS_ValueToNumber(ctx, argv[2], &z);
	}

	*rval = uiGame()->SetWorldToScreen(x, y, z);

	return JS_TRUE;
}

/************************************************************************************************
	getworldtoscreen
*************************************************************************************************/

JS_FUNCTION_DEF(jsGame, resetcontrollers)
{
	// Un-acquire and release any current joysticks before searching for
	// and initialising any that are now connected.
	dimainKillJoysticks();
	dimainInitJoysticks();

	dimainInitTrackIR();

	// reselect presets based on new devices and current selections
	imapLoadPresets();

	return JS_TRUE;
}

#ifdef GR2_BLEND_EDITOR

#include "fio/fiod.h"

JS_FUNCTION_DEF(jsGame, addblend)
{
	if(argc)
	{
		int32 type	= -1;
		int32 state = -1;

		GR2EditBlend::BLEND_STATES states;

		JS_ValueToInt32(ctx, argv[0], &type);

		for(int i = 1; i < argc; i++)
		{
			JS_ValueToInt32(ctx, argv[i], &state);
			if(state < 0) break;

			if(type == GR2EditBlend::FPS && i == 3)
				state = getEditWeaponTypeToNum(state);

			states.push_back((SINT32)state);
		}

		addEditBlend((GR2EditBlend::TYPE)type, states);
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, validblend)
{
	*rval = JSVAL_FALSE;

	if(argc)
	{
		int32 type	= -1;
		int32 state = -1;

		GR2EditBlend::BLEND_STATES states;

		JS_ValueToInt32(ctx, argv[0], &type);

		for(int i = 1; i < argc; i++)
		{
			JS_ValueToInt32(ctx, argv[i], &state);
			if(state < 0) break;

			if(type == GR2EditBlend::FPS && i == 3)
				state = getEditWeaponTypeToNum(state);

			states.push_back((SINT32)state);
		}

		*rval = BOOLEAN_TO_JSVAL(validEditBlend((GR2EditBlend::TYPE)type, states));
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, clearanimcache)
{
	*rval = JSVAL_TRUE;

	clearAnimCache();

	return JS_TRUE;
}


JS_FUNCTION_DEF(jsGame, usedblend)
{
	*rval = JSVAL_TRUE;

	if(argc >= 5)
	{
		int32 type	= -1;
		int32 state = -1;

		GR2EditBlend::BLEND_STATES states;

		JS_ValueToInt32(ctx, argv[0], &type);

		for(int i = 1; i < argc; i++)
		{
			state = -1;

			JS_ValueToInt32(ctx, argv[i], &state);
			if(state < 0) break;

			if(type == GR2EditBlend::FPS && i == 3)
				state = getEditWeaponTypeToNum(state);

			states.push_back((SINT32)state);
		}

		static bool inited = false;
		static std::map<UINT64,bool> valids;

		if(!inited)
		{
			for (int pose = 0; pose < TPS_POSETYPE_NUM_TYPES; pose++)
			{
				for (int move = 0; move < TPS_MOVETYPE_NUM_TYPES; move++)
				{
					for (int weap = 0; weap < TPS_WEAPONTYPE_NUM_TYPES; weap++)
					{
						for (int point = 0; point < TPS_POINTTYPE_NUM_TYPES; point++)
						{
							UINT64 id = fpTPSCreateBlendID(pose, move, weap, point);

							if(pose == TPS_POSETYPE_SIT)
							{
								if(move != TPS_MOVETYPE_STILL || move != TPS_MOVETYPE_IDLE)
									continue;

								if(point != TPS_POINTTYPE_RELAXED && point != TPS_POINTTYPE_RELOAD)
								{
									if(weap != TPS_WEAPONTYPE_MAP || point != TPS_POINTTYPE_READY)
										continue;
								}
							}
							else if(pose == TPS_POSETYPE_SWIM)
							{
								if(move != TPS_MOVETYPE_STILL || move != TPS_MOVETYPE_IDLE || move != TPS_MOVETYPE_FORWARD || move != TPS_MOVETYPE_BACKWARD)
									continue;

								if(point != TPS_POINTTYPE_RELAXED)
									continue;
							}
							else if(pose == TPS_POSETYPE_PRONE)
							{
								if(move != TPS_MOVETYPE_STILL || move != TPS_MOVETYPE_IDLE || move != TPS_MOVETYPE_FORWARD || move != TPS_MOVETYPE_BACKWARD)
									continue;

								if(point != TPS_POINTTYPE_RELAXED)
									continue;
							}
							else if(pose == TPS_POSETYPE_CROUCH)
							{
								if(move == TPS_MOVETYPE_WALK)
									continue;

								if(point != TPS_POINTTYPE_AIM)
								{
									if(move != TPS_MOVETYPE_STILL ||
									   move != TPS_MOVETYPE_FORWARD ||
									   move != TPS_MOVETYPE_TURN ||
									   move != TPS_MOVETYPE_LEAN_LEFT ||
									   move != TPS_MOVETYPE_LEAN_RIGHT)
										continue;
								}
							}
							else if(pose == TPS_POSETYPE_STAND)
							{
								if(point == TPS_POINTTYPE_AIM)
								{
									if(move != TPS_MOVETYPE_STILL ||
									   move != TPS_MOVETYPE_TURN ||
									   move != TPS_MOVETYPE_LEAN_LEFT ||
									   move != TPS_MOVETYPE_LEAN_RIGHT)
										continue;
								}

								if(move == TPS_MOVETYPE_DEPLOYED || (point != TPS_POINTTYPE_READY || point != TPS_POINTTYPE_RELOAD))
									continue;

								if((weap == TPS_WEAPONTYPE_MAP || weap == TPS_WEAPONTYPE_BINOCS) && point != TPS_POINTTYPE_READY)
									continue;

								if(move != TPS_MOVETYPE_DEPLOYED &&
								  (weap == TPS_WEAPONTYPE_ATR || weap == TPS_WEAPONTYPE_LTMORTAR) &&
								  (point != TPS_POINTTYPE_RELAXED || point != TPS_POINTTYPE_RELOAD))
									continue;


							}

							valids[id] = true;
						}
					}
				}
			}

			for (int pose = 0; pose < TPS_POSETYPE_NUM_TYPES; pose++)
			{
				for (int move = 0; move < TPS_MOVETYPE_NUM_TYPES; move++)
				{
					for (int weap = 0; weap < NUM_WEAPONS; weap++)
					{
						for (int point = 0; point < TPS_POINTTYPE_NUM_TYPES; point++)
							valids[fpTPSCreateBlendID(pose, move, weap, point, 1)] = true;
					}
				}
			}

			inited = true;
		}

		UINT64 id = fpTPSCreateBlendID(states[0], states[1], states[2], states[3], type);

		std::map<UINT64,bool>::iterator found = valids.find(id);

		if(found == valids.end())
			*rval = JSVAL_FALSE;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, blendname)
{
	const char* name = "";

	if(argc)
	{
		int32 type	= -1;
		int32 state = -1;

		GR2EditBlend::BLEND_STATES states;

		JS_ValueToInt32(ctx, argv[0], &type);

		for(int i = 1; i < argc; i++)
		{
			JS_ValueToInt32(ctx, argv[i], &state);
			if(state < 0) break;

			if(type == GR2EditBlend::FPS && i == 3)
				state = getEditWeaponTypeToNum(state);

			states.push_back((SINT32)state);
		}

		name = getEditBlendName((GR2EditBlend::TYPE)type, states);
	}

	*rval = phui::js::NewString(name);

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, saveblend)
{
	if(argc > 0)
	{
		int32 index = -1;

		JS_ValueToInt32(ctx, argv[0], &index);

		if(index >= 0)
			saveEditBlend(index);
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, removeblend)
{
	if(argc > 0)
	{
		int32 index = -1;

		JS_ValueToInt32(ctx, argv[0], &index);

		if(index >= 0)
			removeEditBlend(index);
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, addanimation)
{
	*rval = JSVAL_NULL;

	if(argc > 1)
	{
		int32 blend = -1;
		JSFloat64 weight = 1.0f;

		JS_ValueToInt32(ctx, argv[0], &blend);

		const SCHAR* filename = JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));

		GR2EditBlend* eb = getEditBlend(blend);

		if(eb && filename && strlen(filename))
		{
			gr2AnimTrooper* et = getEditModel();
			if(!et) return JS_TRUE;

			et->skeleton->ClearBlends();

			int boneCount = et->skeleton->GetBoneCount();

			eb->blend->animations[eb->blend->numBlends] = gr2LoadAnimation((SCHAR*)filename, 1);

			if(eb->blend->animations[eb->blend->numBlends])
			{
				eb->blend->animations[eb->blend->numBlends]->mask = GrannyNewTrackMask((granny_real32)weight, boneCount);
				//eb->blend->numBlends++;

				// mark the first animation of the blend as the baseloop
				// NOTE: probably will need some more intelligence here
				if(eb->blend->numBlends == 0)
					eb->blend->animations[0]->animType = GR2_ANIMATION_BASELOOP;

				eb->blend->numBlends++;
			}

			eb->modified = true;

			updateEditBlends();
		}
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, addanimevent)
{
	*rval = JSVAL_NULL;

	if(argc > 1)
	{
		int32 anim = -1;
		int32 type = -1;
		JSFloat64 time = 0.0f;

		JS_ValueToInt32(ctx, argv[0], &anim);
		JS_ValueToInt32(ctx, argv[1], &type);
		JS_ValueToNumber(ctx, argv[2], &time);

		GR2EditBlend* eb = getSelectedBlend();

		if(eb && anim >= 0 && anim < eb->blend->numBlends)
		{
			gr2Animation* animation = eb->blend->animations[anim];

			if(animation)
			{
				bool updated = false;

				if(animation->events)
				{
					for(UINT32 index = 0; index < animation->events->size(); index++)
					{
						if(animation->events->at(index)->eventType == type)
						{
							updated = true;
							animation->events->at(index)->eventTime = time;
							break;
						}
					}
				}

				if(!updated)
				{
					// add a new one
					gr2Event* newEvent = gr2GetNewEvent(animation);

					newEvent->eventType		= type;
					newEvent->eventTime		= time;
					newEvent->eventData[0]	= 0;

					updated = true;
				}

				if(updated)
					eb->modified = true;
			}

			updateEditBlends();
		}
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, playanimation)
{
	*rval = JSVAL_NULL;

	if(argc > 0)
	{
		const SCHAR* filename = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

		if(filename)
			setEditAnimation(filename);
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, removeanimation)
{
	if(argc > 1)
	{
		int32 blend = -1;
		int32 index = -1;

		JS_ValueToInt32(ctx, argv[0], &blend);
		JS_ValueToInt32(ctx, argv[1], &index);

		GR2EditBlend* eb = getEditBlend(blend);

		if(eb && eb->blend->numBlends && index >= 0 && index < eb->blend->numBlends)
		{
			gr2AnimTrooper* et = getEditModel();
			if(!et) return JS_TRUE;

			et->skeleton->ClearBlends();

			gr2FreeAnimation(eb->blend->animations[index]);
			eb->blend->animations[index] = nullptr;

			for(int i = index + 1; i < eb->blend->numBlends; i++)
			{
				// move all animations down
				eb->blend->animations[i - 1] = eb->blend->animations[i];
				eb->blend->animations[i] = nullptr;
			}

			eb->blend->numBlends--;
			eb->modified = true;

			updateEditBlends();
		}
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, setblendweapon)
{
	if(argc > 0)
	{
		int32 type = -1;

		JS_ValueToInt32(ctx, argv[0], &type);

		if(type >= 0)
			setEditBlendWeapon(type);
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, reorderblend)
{
	if(argc > 2)
	{
		int32 blend		= -1;
		int32 animation = -1;
		int32 direction	= 0;

		JS_ValueToInt32(ctx, argv[0], &blend);
		JS_ValueToInt32(ctx, argv[1], &animation);
		JS_ValueToInt32(ctx, argv[2], &direction);

		reorderEditBlend(blend, animation, direction);
	}

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, setblendweights)
{
	int32 index		= -1;
	double weight1	= 1.0f;
	double weight2	= 0.0f;
	double weight3	= 0.0f;
	double weight4	= 0.0f;

	if(argc > 0) JS_ValueToInt32(ctx, argv[0], &index);
	if(argc > 1) JS_ValueToNumber(ctx, argv[1], &weight1);
	if(argc > 2) JS_ValueToNumber(ctx, argv[2], &weight2);
	if(argc > 3) JS_ValueToNumber(ctx, argv[3], &weight3);
	if(argc > 4) JS_ValueToNumber(ctx, argv[4], &weight4);

	setEditBlendWeights(index, weight1, weight2, weight3, weight4);

	*rval = JSVAL_NULL;

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsGame, updateblends)
{
	updateEditBlends();
	*rval = JSVAL_NULL;

	return JS_TRUE;
}

#endif

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsGame::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case ONLINE_PROP:
			*val =INT_TO_JSVAL(Online);
		break;
		case GAME_PROP:
			*val = jsVal();
		break;
		case CPS_PROP:
		{
			if(!cps->size())
			{
				StratDB::StratObjList allcps;
				StratDB::AllCPsInWorld(allcps);
				StratDB::StratObjList::iterator it = allcps.begin();

				unsigned int i = 0;
				for(; it != allcps.end(); ++it, i++)
					cps->SetElement(new jsStratObject((*it)->FullObjID()), i);
			}
			*val = cps->jsVal();
		}
		break;
		case TIME_PROP:
		{
			UINT32 time;
			trnTODGetTimeOfDay(&time);
			*val = INT_TO_JSVAL(time);
		}
		break;
		case INGAME_PROP:
			*val = BOOLEAN_TO_JSVAL(uiInGame());
		break;
		case PERSONAVEHICLES_PROP:
		case SPAWNABLEVEHICLES_PROP:
		{
			spawnablevehicles->clear();
			scenariospawns->clear();

			// if we're running a scenario; let it determine which vehicles
			// are available
			if(SCEN_MGR->IsActive())
			{
				UINT32 index = 0;

				//const PersonaSummaries& persum	= PlayerClient()->Personas();
				const PersonaSelection& persona	= PlayerClient()->PersonaSelected();

				for(UINT32 i = 0; i < vehtypeCount(); i++)
				{
					const vehType* vtype = vehtypeGetByIndex(i);

					if(Online && !SCEN_MGR->ValidVehicle(vtype))
						continue;

					if(Online && wwiiSide(persona.m_countryID) != wwiiSide(vtype->WeaponSpec.wCountry))
						continue;

					// add a spawnable vehicle
					spawnablevehicles->SetElement(new jsVehtype(vtype), index);

					// determine its availability
					int capacity = SCEN_MGR->VehicleLimit(vtype); // -1 means no limit
					int current = SCEN_MGR->VehicleCount(vtype);
					int availability = (capacity == -1 || current < capacity) ? WeaponCounter::SPAWNABLE : WeaponCounter::NOT_PRESENT;

					// now we check for whether the vehicle is permitted or not
					if(!SCEN_MGR->VehiclePermitted(vtype))
						availability = WeaponCounter::NOT_PERMITTED;

					jsWeaponCounter* jscounter = new jsWeaponCounter(capacity, current, index, 0, availability);

					// add a weap counter for UI purposes
					scenariospawns->SetElement(jscounter, index);

					++index;
				}
			}
			else // otherwise do the normal stuff
			{
				UINT32 index = 0;

				for(UINT32 i = 0; i < vehtypeCount(); i++)
				{
					const vehType* vtype = vehtypeGetByIndex(i);

					if(Online && propid == PERSONAVEHICLES_PROP)
					{
						// check if this vehicle fits our persona
						const PersonaSelection& persona = PlayerClient()->PersonaSelected();

						if(persona.m_countryID != vtype->WeaponSpec.wCountry) // our countries don't match
							continue;
						else if(vtype->WeaponSpec.wCategory > 4) // munitions, can't spawn
							continue;
						else if((vtype->WeaponSpec.wCategory == 2 || vtype->WeaponSpec.wCategory == 4) && persona.m_branchID != 1) // infantry, but not in the army
							continue;
						else if(vtype->WeaponSpec.wCategory == 1 && persona.m_branchID != 2) // air
							continue;
						else if(vtype->WeaponSpec.wCategory == 3 && persona.m_branchID != 3) // navy
							continue;
					}

					if(!vtype->OnlineOnly || fioQuery() == ISDIRECTORY)
					{
						jsVehtype* jstype = new jsVehtype(vtype);
						spawnablevehicles->SetElement(jstype, index);
						++index;
					}
				}
			}

			*val = spawnablevehicles->jsVal();
		}
		break;
		case SETTINGS_PROP:
			*val = settings->jsVal();
		break;
		case SCREENWIDTH_PROP:
			*val = INT_TO_JSVAL(ddpGetWidth());
		break;
		case SCREENHEIGHT_PROP:
			*val = INT_TO_JSVAL(ddpGetHeight());
		break;
		case TRAINING_PROP:
			*val = BOOLEAN_TO_JSVAL(TUT_ACTIVE && !SCEN_MGR->IsActive());
		break;
		case REGION_PROP:
			*val = INT_TO_JSVAL(gRegion);
		break;
		case LASTENTERED_PROP:
		{
			*val = JSVAL_NULL;
			StratDB::StratObject* le = vehmainLastEntered();

			if(le)
			{
				if(!lastentered.valid() || strcasecmp(le->Name(), lastentered.ptr()->stratobj->Name()))
					lastentered = new jsStratObject(le->FullObjID());

				*val = lastentered->jsVal();
			}
		}
		break;
		case CANBAIL_PROP:
		{
			vehVehicle* player		= persGetPersonaVehiclePtr();
			FLOAT64VECTOR& vpos		= player->ModelData->VehiclePos;
			FLOAT32 altitude		= collideRay(COLLIDER_TILE, 10000.0f, vpos.x, vpos.y, vpos.z + 1.0f);

			*val = BOOLEAN_TO_JSVAL(!altitude || altitude > BailoutSTO::SAFE_DISTANCE());
		}
		break;
		case BAILEDOUT_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(vehmainHasBailedOut());
		}
		break;
		case SCENARIO_PROP:
			*val = BOOLEAN_TO_JSVAL(SCEN_MGR->IsActive());
		break;
		case SCENARIOCOMPLETE_PROP:
			*val = BOOLEAN_TO_JSVAL(SCEN_MGR->IsScenarioComplete());
		break;
		case SCENARIOSPAWNS_PROP:
			*val = scenariospawns->jsVal();
		break;
		case CUSTOMTERRAIN_PROP:
			*val = BOOLEAN_TO_JSVAL(gCustomTerrain);
		break;
		case EZMODE_PROP:
			*val =INT_TO_JSVAL(gEZMode);
		break;
#ifdef GR2_BLEND_EDITOR
		case BLENDEDITOR_PROP:
			*val = blendeditor->jsVal();
		break;
#endif
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsCursor
*************************************************************************************************/

JSClass jsCursor::jsclass = JS_CLASS_MACRO("cursor");
JSFunctionSpec jsCursor::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsCursor, update, 1),
	{0}
};

JSPropertySpec jsCursor::jsproperties[] =
{
	{"x",			jsCursor::X_PROP,			JSPROP_PERMANENT},
	{"y",			jsCursor::Y_PROP,			JSPROP_PERMANENT},
	{"visible",	jsCursor::VISIBLE_PROP,	JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsCursor
*************************************************************************************************/

jsCursor::jsCursor() :
			phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
};

jsCursor::~jsCursor()
{
}

/************************************************************************************************
	update
*************************************************************************************************/

JS_FUNCTION_DEF(jsCursor,update)
{
	if(uiInGame())
	{
		const char* exclude = nullptr;

		if(argc > 0)
			exclude = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

		uiSetCursor((int)uiViewport()->HasModalChild(exclude, true));
	}
	else
	{
		uiSetCursor(1);
	}

	return JS_TRUE;
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsCursor::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case X_PROP:
		{
			int x, y;
			sysGetMousePosition(&x,&y);
			*val = INT_TO_JSVAL(x);
		}
		break;
		case Y_PROP:
		{
			int x, y;
			sysGetMousePosition(&x,&y);
			*val = INT_TO_JSVAL(y);
		}
		break;
		case VISIBLE_PROP:
			*val = BOOLEAN_TO_JSVAL(uiCursorState() ? JS_TRUE : JS_FALSE);
		break;
	}

	return JS_TRUE;
}

JSBool jsCursor::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case X_PROP:
		{
			int x, y;
			sysGetMousePosition(&x,&y);

			if(JS_ValueToInt32(ctx, *val, (int32*)&x))
				sysSetMousePosition(x, y);
		}
		break;
		case Y_PROP:
		{
			int x, y;
			sysGetMousePosition(&x,&y);

			if(JS_ValueToInt32(ctx, *val, (int32*)&y))
				sysSetMousePosition(x, y);
		}
		break;
		case VISIBLE_PROP:
		{
			JSBool b;
			JS_ValueToBoolean(_jsctx, *val, &b);
			uiSetCursor(b ? 1 : 0);
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsSettings
*************************************************************************************************/

JSClass jsSettings::jsclass = JS_CLASS_MACRO("settings");
JSFunctionSpec jsSettings::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsSettings::jsproperties[TOTAL_SETTINGS+1] =
{
	{"restartneeded",			jsSettings::RESTARTNEEDED_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"videocards",				jsSettings::VIDEOCARDS_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"soundcards",				jsSettings::SOUNDCARDS_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"languages",				jsSettings::LANGUAGES_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},
	{"sse2supported",			jsSettings::SSE2_SUPPORTED_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"watersupported",			jsSettings::WATER_SUPPORTED_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"reflectionssupported",	jsSettings::REFLECTIONS_SUPPORTED_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"postrendersupported",		jsSettings::POSTRENDER_SUPPORTED_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"grassshadersupported",	jsSettings::GRASSSHADER_SUPPORTED_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"normalmapssupported",		jsSettings::NORMALMAPS_SUPPORTED_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"refractionssupported",    jsSettings::REFRACTIONS_SUPPORTED_PROP, JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsSettings::jsSettings() : phui::js::Object(&jsclass, jsfunctions)
{
	JS_CONSTRUCTOR_LOCK;

	// init jsproperties
	_videocards = new phui::js::Array;
	_soundcards = new phui::js::Array;
	_languages = new phui::js::Array;

	// move the initial params down as define dabove
	for(int i = 0; i < TOTAL_SETTINGS - cfgNUM_TOTAL_PARAMS; ++i)
		jsproperties[cfgNUM_TOTAL_PARAMS + i] = jsproperties[i];

	// add in the global params to the front of the array
	for(int8 i = 0; i < cfgNUM_TOTAL_PARAMS; ++i)
	{
		cfgPARAM_DETAILS* details = cfgPrefsGetGlobalDetails((_cfgPARAMS)i);

		if(details)
		{
			// most global names are in the form of cat.name
			// we'll use the string after the period for the
			// settings js property name
			char* name = strchr(details->name, '.');
			JSPropertySpec spec;

			name = (!name) ? details->name: name + 1;

			spec.name	= name;
			spec.tinyid = i;
			spec.flags	= JSPROP_PERMANENT;
			spec.setter	= nullptr;
			spec.getter	= nullptr;

			jsproperties[i] = spec;
		}
	}

	// define the properties
	DefineProperties(jsSettings::jsproperties);

}

jsSettings::~jsSettings()
{
}

JSBool jsSettings::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	switch(propid)
	{
		// do special handling first
		case cfgFRIENDLY_COLOR: // gIconFriendlyColor
		{
			char colorBuffer[8];
			snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(gIconFriendlyColor), piRGBA_GETGREEN(gIconFriendlyColor), piRGBA_GETBLUE(gIconFriendlyColor));
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case cfgENEMY_COLOR: // gIconEnemyColor
		{
			char colorBuffer[8];
			snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(gIconEnemyColor), piRGBA_GETGREEN(gIconEnemyColor), piRGBA_GETBLUE(gIconEnemyColor));
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case cfgSQUAD_COLOR: // gIconSquadColor
		{
			char colorBuffer[8];
			snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(gIconSquadColor), piRGBA_GETGREEN(gIconSquadColor), piRGBA_GETBLUE(gIconSquadColor));
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case cfgMISSION_COLOR: // gIconMissionColor
		{
			char colorBuffer[8];
			snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(gIconMissionColor), piRGBA_GETGREEN(gIconMissionColor), piRGBA_GETBLUE(gIconMissionColor));
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case cfgMISSIONLEADER_COLOR: // gIconMissionLeaderColor
		{
			char colorBuffer[8];
			snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(gIconMissionLeaderColor), piRGBA_GETGREEN(gIconMissionLeaderColor), piRGBA_GETBLUE(gIconMissionLeaderColor));
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case cfgNOOB_COLOR: // gIconNoobColor
		{
			char colorBuffer[8];
			snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(gIconNoobColor), piRGBA_GETGREEN(gIconNoobColor), piRGBA_GETBLUE(gIconNoobColor));
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case cfgAWS_LOW:
		case cfgAWS_HIGH:
		case cfgPPO_OK:
		case cfgPPO_NOK:
		{
			int color = 0;
			if(cfgPrefsGetGlobal((_cfgPARAMS)propid, &color))
			{
				char colorBuffer[8];
				snprintf(colorBuffer, 8, "#%02x%02x%02x", piRGBA_GETRED(color), piRGBA_GETGREEN(color), piRGBA_GETBLUE(color));
				*val = phui::js::NewString(colorBuffer);
			}
		}
		break;
		case RESTARTNEEDED_PROP:
			*val = BOOLEAN_TO_JSVAL(cfgPrefsRestartNeeded());
		break;
		case VIDEOCARDS_PROP:
		{
			if(!_videocards->size())
			{
				DeviceList cards;
				if(piinfoGetVideoCards(&cards))
				{
					for(int i = 0; i < (int)cards.size(); i++)
						_videocards->SetElement(new jsVideoCard(i, cards.at(i)), i);
				}
			}
			*val = _videocards->jsVal();
		}
		break;
		case SOUNDCARDS_PROP:
		{
			if(!_soundcards->size())
			{
				DeviceList cards;
				if(piinfoGetSoundCards(&cards))
				{
					for(int i = 0; i < (int)cards.size(); i++)
						_soundcards->SetElement(new jsSoundCard(i, cards.at(i)), i);
				}
			}
			*val = _soundcards->jsVal();
		}
		break;
		case LANGUAGES_PROP:
		{
			if(!_languages->size())
			{
				for(int i = 0; i < NUMBER_OF_LANGUAGES; i++)
					_languages->SetElement(phui::js::NewString(g_szLanguages[i]), i);
			}
			*val = _languages->jsVal();
		}
		break;
		case SSE2_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgWANT_SSE2));
		}
		break;
		case WATER_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgENABLE_H2O_SHADERS));
		}
		break;
		case REFLECTIONS_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgH2O_REFLECTIONS));
		}
		break;
		case REFRACTIONS_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgH2O_REFRACTIONS));
		}
		break;
		case POSTRENDER_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgPOST_RENDER_FILTER));
		}
		break;
		case GRASSSHADER_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgGRASS_SHADER));
		}
		break;
		case NORMALMAPS_SUPPORTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(piinfoIsSupportedPref(cfgNORMAL_MAPS));
		}
		break;
		default:
		{
			if(propid >= 0 && propid < cfgNUM_TOTAL_PARAMS)
			{
				cfgPARAM_DETAILS* details = cfgPrefsGetGlobalDetails((_cfgPARAMS)propid);

				if(details)
				{
					switch(details->type)
					{
						case cfgPARAM_BOOLEAN:
						{
							bool bval = false;
							if(cfgPrefsGetGlobal((_cfgPARAMS)propid, &bval))
								*val = BOOLEAN_TO_JSVAL(bval);
						}
						break;
						case cfgPARAM_UINT:
						case cfgPARAM_SINT:
						{
							int ival = 0;
							if(cfgPrefsGetGlobal((_cfgPARAMS)propid, &ival))
								*val = INT_TO_JSVAL(ival);
						}
						break;
						case cfgPARAM_LONG:
						{
							long lval = 0;
							if(cfgPrefsGetGlobal((_cfgPARAMS)propid, &lval))
								*val = INT_TO_JSVAL((int)lval);
						}
						break;
						case cfgPARAM_FLOAT:
						{
							float fval = 0.0f;
							if(cfgPrefsGetGlobal((_cfgPARAMS)propid, &fval))
								JS_NewDoubleValue(_jsctx, fval, val);
						}
						break;
						case cfgPARAM_DOUBLE:
						{
							double dval = 0.0f;
							if(cfgPrefsGetGlobal((_cfgPARAMS)propid, &dval))
								JS_NewDoubleValue(_jsctx, dval, val);
						}
						break;
						case cfgPARAM_STRING:
						{
							// cfgPrefsGetGlobal() populates the string
							// so we have to allocate the buffer based
							// on the max string size defined for the
							// global string variable
							char* sval = new char[details->max];
							if(sval)
							{
								if(cfgPrefsGetGlobal((_cfgPARAMS)propid, sval))
									*val = phui::js::NewString(UTF16(sval));

								delete [] sval;
								sval = nullptr;
							}
						}
						break;
					}
				}
			}
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsSettings::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		// do special handling first
		case cfgFRIENDLY_COLOR: // gIconFriendlyColor
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				gIconFriendlyColor = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal(cfgFRIENDLY_COLOR, &gIconFriendlyColor, false);
			}
		}
		break;
		case cfgENEMY_COLOR: // gIconEnemyColor
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				gIconEnemyColor = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal(cfgENEMY_COLOR, &gIconEnemyColor, false);
			}
		}
		break;
		case cfgSQUAD_COLOR: // gIconSquadColor
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				gIconSquadColor = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal(cfgSQUAD_COLOR, &gIconSquadColor, false);
			}
		}
		break;
		case cfgMISSION_COLOR: // gIconMissionColor
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				gIconMissionColor = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal(cfgMISSION_COLOR, &gIconMissionColor, false);
			}
		}
		break;
		case cfgMISSIONLEADER_COLOR: // gIconMissionLeaderColor
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				gIconMissionLeaderColor = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal(cfgMISSIONLEADER_COLOR, &gIconMissionLeaderColor, false);
			}
		}
		break;
		case cfgNOOB_COLOR: // gIconNoobColor
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				gIconNoobColor = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal(cfgNOOB_COLOR, &gIconNoobColor, false);
			}
		}
		break;
		case cfgAWS_LOW:
		case cfgAWS_HIGH:
		case cfgPPO_OK:
		case cfgPPO_NOK:
		{
			const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			UINT32 r, g, b;
			int color = 0;

			if(sscanf(buffer,"#%02x%02x%02x", &r, &g, &b) == 3)
			{
				color = piRGB_MAKE(r, g, b);
				cfgPrefsSetGlobal((_cfgPARAMS)propid, &color, false);
			}
		}
		break;
		default:
		{
			if(propid >= 0 && propid < cfgNUM_TOTAL_PARAMS)
			{
				cfgPARAM_DETAILS* details = cfgPrefsGetGlobalDetails((_cfgPARAMS)propid);

				if(details)
				{
					switch(details->type)
					{
						case cfgPARAM_BOOLEAN:
						{
							JSBool bval = false;
							JS_ValueToBoolean(ctx, *val, &bval);
							cfgPrefsSetGlobal((_cfgPARAMS)propid, &bval);
						}
						break;
						case cfgPARAM_UINT:
						case cfgPARAM_SINT:
						{
							int32 ival = 0;
							JS_ValueToInt32(ctx, *val, &ival);
							cfgPrefsSetGlobal((_cfgPARAMS)propid, &ival);
						}
						break;
						case cfgPARAM_LONG:
						{
							int32 ival = 0;
							long lval = 0;
							JS_ValueToInt32(ctx, *val, &ival);
							lval = (long)ival;
							cfgPrefsSetGlobal((_cfgPARAMS)propid, &lval);
						}
						break;
						case cfgPARAM_FLOAT:
						{
							double dval = 0.0f;
							JS_ValueToNumber(ctx, *val, &dval);
							float fval = (float)dval;
							cfgPrefsSetGlobal((_cfgPARAMS)propid, &fval);
						}
						break;
						case cfgPARAM_DOUBLE:
						{
							double dval = 0.0f;
							JS_ValueToNumber(ctx, *val, &dval);
							cfgPrefsSetGlobal((_cfgPARAMS)propid, &dval);
						}
						break;
						case cfgPARAM_STRING:
						{
							const wchar_t* wstr = phui::js::ValueToWideString(*val);
							//const char* buffer = JS_GetStringBytes(JS_ValueToString(ctx, *val));
							const char* buffer = UTF8(wstr);
							cfgPrefsSetGlobal((_cfgPARAMS)propid, (void*)buffer);
						}
						break;
					}
				}
			}
		}
		break;
	}

	return JS_TRUE;
}
/*
JSPropertySpec jsSettings::jsproperties[] =
{
	{"soundvolume",		jsSettings::SOUNDVOLUME_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"musicvolume",		jsSettings::MUSICVOLUME_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"fontsize",		jsSettings::FONTSIZE_PROP,			JSPROP_PERMANENT},
	{"squadinvites",	jsSettings::SQUADINVITES_PROP,		JSPROP_PERMANENT},
	{"wantalerts",		jsSettings::WANTALERTS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"friendcolor",		jsSettings::FRIENDCOLOR_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"enemycolor",		jsSettings::ENEMYCOLOR_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"squadcolor",		jsSettings::SQUADCOLOR_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"missioncolor",	jsSettings::MISSIONCOLOR_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"leadercolor",		jsSettings::LEADERCOLOR_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"noobcolor",		jsSettings::NOOBCOLOR_PROP,         JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};
*/

/************************************************************************************************
	jsColor
*************************************************************************************************/

JSClass jsColor::jsclass = JS_CLASS_MACRO("color");
JSFunctionSpec jsColor::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsColor::jsproperties[] =
{
	{"r",			jsColor::RED_PROP,			JSPROP_PERMANENT},
	{"g",			jsColor::GREEN_PROP,		JSPROP_PERMANENT},
	{"b",			jsColor::BLUE_PROP,			JSPROP_PERMANENT},
	{"rhex",		jsColor::REDHEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"ghex",		jsColor::GREENHEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"bhex",		jsColor::BLUEHEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"color",		jsColor::COLOR_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"colorhex",	jsColor::COLORHEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"colorhexcss",	jsColor::COLORHEXCSS_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsColor::jsColor(JSContext* ctx, JSObject* parent) :
	phui::js::Object(&jsclass, ctx, parent, jsfunctions, jsproperties),
	_red(255),
	_green(255),
	_blue(255)
{
	JS_CONSTRUCTOR_LOCK;
}

jsColor::jsColor(JSContext* ctx, JSObject* parent, int r, int g, int b) :
	phui::js::Object(&jsclass, ctx, parent, jsfunctions, jsproperties),
	_red(r),
	_green(g),
	_blue(b)
{
	JS_CONSTRUCTOR_LOCK;

	if(_red < 0 || _red > 255)
		_red = 255;

	if(_green < 0 || _green > 255)
		_green = 255;

	if(_blue < 0 || _blue > 255)
		_blue = 255;
}

jsColor::jsColor(JSContext* ctx, JSObject* parent, const char* hexcolor) :
	phui::js::Object(&jsclass, ctx, parent, jsfunctions, jsproperties),
	_red(255),
	_green(255),
	_blue(255)
{
	JS_CONSTRUCTOR_LOCK;

	const char* color = (*hexcolor && hexcolor[0] == '#') ? hexcolor + 1: hexcolor;

	if(color && sscanf(color,"%02x%02x%02x", (UINT32*)&_red, (UINT32*)&_green, (UINT32*)&_blue) == 3)
	{
		if(_red < 0 || _red > 255)
			_red = 255;

		if(_green < 0 || _green > 255)
			_green = 255;

		if(_blue < 0 || _blue > 255)
			_blue = 255;
	}
}

jsColor::~jsColor()
{
}

JSBool jsColor::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	switch(propid)
	{
		case RED_PROP:
		{
			*val = INT_TO_JSVAL(_red);
		}
		break;
		case GREEN_PROP:
		{
			*val = INT_TO_JSVAL(_green);
		}
		break;
		case BLUE_PROP:
		{
			*val = INT_TO_JSVAL(_blue);
		}
		break;
		case REDHEX_PROP:
		{
			char colorBuffer[2+1];
			snprintf(colorBuffer, 3, "%02x", (UINT32)_red);
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case GREENHEX_PROP:
		{
			char colorBuffer[2+1];
			snprintf(colorBuffer, 3, "%02x", (UINT32)_green);
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case BLUEHEX_PROP:
		{
			char colorBuffer[2+1];
			snprintf(colorBuffer, 3, "%02x", (UINT32)_blue);
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case COLOR_PROP:
		{
			*val = INT_TO_JSVAL(piRGB_MAKE(_red, _green, _blue));
		}
		break;
		case COLORHEX_PROP:
		{
			char colorBuffer[13] = {0};
			snprintf(colorBuffer, sizeof(colorBuffer)-1, "%02x%02x%02x", (UINT32)_red, (UINT32)_green, (UINT32)_blue);
			*val = phui::js::NewString(colorBuffer);
		}
		break;
		case COLORHEXCSS_PROP:
		{
			char colorBuffer[13] = {0};
			snprintf(colorBuffer, sizeof(colorBuffer)-1, "#%02x%02x%02x", (UINT32)_red, (UINT32)_green, (UINT32)_blue);
			*val = phui::js::NewString(colorBuffer);
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsColor::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case RED_PROP:
		{
			JS_ValueToInt32(ctx, *val, &_red);

			if(_red < 0 || _red > 255)
				_red = 255;
		}
		break;
		case GREEN_PROP:
		{
			JS_ValueToInt32(ctx, *val, &_green);

			if(_green < 0 || _green > 255)
				_green = 255;
		}
		break;
		case BLUE_PROP:
		{
			JS_ValueToInt32(ctx, *val, &_blue);

			if(_blue < 0 || _blue > 255)
				_blue = 255;
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsVideoCard
*************************************************************************************************/

JSClass jsVideoCard::jsclass = JS_CLASS_MACRO("videocard");
JSFunctionSpec jsVideoCard::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsVideoCard::jsproperties[] =
{
	{"index",	jsVideoCard::INDEX_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"name",	jsVideoCard::NAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"modes",	jsVideoCard::MODES_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"id",		jsVideoCard::ID_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsVideoCard::jsVideoCard(int32 index, PIDeviceInfo& device) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties),
	_device(device),
	_index(index)
{
	JS_CONSTRUCTOR_LOCK;

	_videomodes	= new phui::js::Array;
}

jsVideoCard::~jsVideoCard()
{
}

JSBool jsVideoCard::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	switch(propid)
	{
		case INDEX_PROP:
		{
			*val = INT_TO_JSVAL(_index);
		}
		break;
		case NAME_PROP:
		{
			*val = phui::js::NewString(_device.description.c_str());
		}
		break;
		case MODES_PROP:
		{
			if(!_videomodes->size())
			{
				InfoList modes;
				if(piinfoGetSupportedResolutions(&modes, _index))
				{
					for(int i = 0; i < (int)modes.size(); i++)
						_videomodes->SetElement(phui::js::NewString(modes.at(i).c_str()), i);
				}
			}
			*val = _videomodes->jsVal();
		}
		break;
		case ID_PROP:
		{
			*val = phui::js::NewString(_device.id.c_str());
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsSoundCard
*************************************************************************************************/

JSClass jsSoundCard::jsclass = JS_CLASS_MACRO("soundcard");
JSFunctionSpec jsSoundCard::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsSoundCard::jsproperties[] =
{
	{"index",	jsSoundCard::INDEX_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"name",	jsSoundCard::NAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"id",		jsSoundCard::ID_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsSoundCard::jsSoundCard(int32 index, PIDeviceInfo& device) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties),
	_device(device),
	_index(index)
{
	JS_CONSTRUCTOR_LOCK;
}

jsSoundCard::~jsSoundCard()
{
}

JSBool jsSoundCard::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	switch(propid)
	{
		case INDEX_PROP:
		{
			*val = INT_TO_JSVAL(_index);
		}
		break;
		case NAME_PROP:
		{
			*val = phui::js::NewString(UTF16(_device.description.c_str()));
		}
		break;
		case ID_PROP:
		{
			*val = phui::js::NewString(UTF16(_device.id.c_str()));
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsScenarioState
*************************************************************************************************/

JSClass jsScenarioState::jsclass = JS_CLASS_MACRO("scenariostate");
JSFunctionSpec jsScenarioState::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsScenarioState::jsproperties[] =
{
	{"status",	jsScenarioState::STATUS_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"active",	jsScenarioState::ACTIVE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"ready1",	jsScenarioState::READY1_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"ready2",	jsScenarioState::READY2_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"team",	jsScenarioState::TEAM_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsScenarioState::jsScenarioState() : phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsScenarioState::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case STATUS_PROP:
			*val = INT_TO_JSVAL(0);
		break;
		case ACTIVE_PROP:
			*val = BOOLEAN_TO_JSVAL(SCEN_MGR->IsActive());
		break;
		case READY1_PROP:
			*val = BOOLEAN_TO_JSVAL(SCEN_MGR->GetReadyCount(1));
		break;
		case READY2_PROP:
			*val = BOOLEAN_TO_JSVAL(SCEN_MGR->GetReadyCount(2));
		break;
		case TEAM_PROP:
			*val = INT_TO_JSVAL(SCEN_MGR->GetTeam());
		break;
	}

	return JS_TRUE;
}

#ifdef GR2_BLEND_EDITOR

/************************************************************************************************
	jsBlendEditor
*************************************************************************************************/

JSClass jsBlendEditor::jsclass = JS_CLASS_MACRO("blendeditor");
JSFunctionSpec jsBlendEditor::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsBlendEditor::jsproperties[] =
{
	{"blends",		jsBlendEditor::BLENDS_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"animations",	jsBlendEditor::ANIMATIONS_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"types",		jsBlendEditor::TYPES_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"state1",		jsBlendEditor::STATE1_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"state2",		jsBlendEditor::STATE2_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"state3",		jsBlendEditor::STATE3_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"state4",		jsBlendEditor::STATE4_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"rotatemodel",	jsBlendEditor::ROTATEMODEL_PROP, JSPROP_PERMANENT},
	{"oscillation",	jsBlendEditor::OSCILLATION_PROP, JSPROP_PERMANENT},
	{"type",		jsBlendEditor::TYPE_PROP, JSPROP_PERMANENT},
	{0}
};

jsBlendEditor::jsBlendEditor() :
	phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	_blends = new phui::js::Array;
	_animations = new phui::js::Array;
	_types = new phui::js::Array;
	_state1 = new phui::js::Array;
	_state2 = new phui::js::Array;
	_state3 = new phui::js::Array;
	_state4 = new phui::js::Array;

	_type = 0;
}

jsBlendEditor::~jsBlendEditor()
{
}

#define ANIMATION_PATH "DATA\\gr2Data\\TPS\\animations"

JSBool jsBlendEditor::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	switch(propid)
	{
		case BLENDS_PROP:
		{
			_blends->clear();

			for(int i = 0; i < 10; i++)
			{
				GR2EditBlend* eb = getEditBlend(i);
				if(!eb) break;

				_blends->SetElement(new jsBlend(i), i);
			}

			*val = _blends->jsVal();
		}
		break;
		case ANIMATIONS_PROP:
		{
			_animations->clear();

			// create a list of scripts to display
			SCHAR** fileList = nullptr;
			SINT32 numfiles = 0;
			SINT32 f = 0;
			SINT32 i = 0;

			fiodGetDirFileList(ANIMATION_PATH, &fileList, &numfiles);

			if(fileList)
			{
				for(f = 0; f < numfiles; f++)
				{
					const char* s = fileList[f];
					const char* ptr = strstr(s, ".");

					if(!strcasecmp(ptr, ".gr2"))
						_animations->SetElement(phui::js::NewString(s), i++);
				}
			}

			*val = _animations->jsVal();
		}
		break;
		case TYPES_PROP:
		{
			_types->clear();
			_types->SetElement(phui::js::NewString("TPS"), 0);
			_types->SetElement(phui::js::NewString("FPS"), 1);

			*val = _types->jsVal();
		}
		break;
		case TYPE_PROP:
		{
			*val = INT_TO_JSVAL(_type);
		}
		break;
		case STATE1_PROP:
		{
			_state1->clear();

			for(int i = 0; i < TPS_POSETYPE_NUM_TYPES; i++)
				_state1->SetElement(phui::js::NewString(fpTPSPoseKeyStrings[i]), i);

			*val = _state1->jsVal();
		}
		break;
		case STATE2_PROP:
		{
			_state2->clear();

			for(int i = 0; i < TPS_MOVETYPE_NUM_TYPES; i++)
				_state2->SetElement(phui::js::NewString(fpTPSMoveKeyStrings[i]), i);

			*val = _state2->jsVal();
		}
		break;
		case STATE3_PROP:
		{
			_state3->clear();

			for(int i = 0; i < TPS_WEAPONTYPE_NUM_TYPES; i++)
				_state3->SetElement(phui::js::NewString(fpTPSWeaponKeyStrings[i]), i);

			*val = _state3->jsVal();
		}
		break;
		case STATE4_PROP:
		{
			_state4->clear();

			for(int i = 0; i < TPS_POINTTYPE_NUM_TYPES; i++)
				_state4->SetElement(phui::js::NewString(fpTPSPointKeyStrings[i]), i);

			*val = _state4->jsVal();
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsBlendEditor::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	gr2AnimTrooper* model = getEditModel();
	if(!model) return JS_TRUE;

	switch(propid)
	{
		case ROTATEMODEL_PROP:
		{
			JSBool b = 0;
			JS_ValueToBoolean(_jsctx, *val, &b);
			setRotateEditModel((bool)b);
		}
		break;
		case OSCILLATION_PROP:
		{
			int32 o = 0;
			JS_ValueToInt32(_jsctx, *val, &o);
			setEditModelOscillation(o);
		}
		break;
		case TYPE_PROP:
		{
			int32 t = 0;
			JS_ValueToInt32(_jsctx, *val, &t);
			_type = t;
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsBlend
*************************************************************************************************/

JSClass jsBlend::jsclass = JS_CLASS_MACRO("blend");
JSFunctionSpec jsBlend::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsBlend::jsproperties[] =
{
	{"index",		jsBlend::INDEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"filename",	jsBlend::FILENAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"animations",	jsBlend::ANIMATIONS_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"weight",		jsBlend::WEIGHT_PROP,		JSPROP_PERMANENT},
	{"modified",	jsBlend::MODIFIED_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"selected",	jsBlend::SELECTED_PROP,		JSPROP_PERMANENT},
	{"speed",		jsBlend::SPEED_PROP,		JSPROP_PERMANENT},
	{"time",		jsBlend::TIME_PROP,			JSPROP_PERMANENT},
	{"length",		jsBlend::LENGTH_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"type",		jsBlend::TYPE_PROP,			JSPROP_PERMANENT},
	{"play",		jsBlend::PLAY_PROP,			JSPROP_PERMANENT},
	{"active",		jsBlend::ACTIVE_PROP,		JSPROP_PERMANENT},
	{0}
};

jsBlend::jsBlend(int blend) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	_blend = blend;
	_animations = new phui::js::Array;
}

jsBlend::~jsBlend()
{
}

JSBool jsBlend::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	GR2EditBlend* eb = getEditBlend(_blend);
	if(!eb) return JS_TRUE;

	switch(propid)
	{
		case INDEX_PROP:
		{
			*val = INT_TO_JSVAL(_blend);
		}
		break;
		case FILENAME_PROP:
		{
			*val = (eb) ? phui::js::NewString(eb->blend->blendFile) : JSVAL_NULL;
		}
		break;
		case ANIMATIONS_PROP:
		{
			_animations->clear();

			for(int a = 0; a < eb->blend->numBlends; a++)
				_animations->SetElement(new jsAnimation(_blend, a), a);

			*val = _animations->jsVal();
		}
		break;
		case WEIGHT_PROP:
		{
			JS_NewDoubleValue(_jsctx, eb->weight, val);
		}
		break;
		case MODIFIED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(eb->modified);
		}
		break;
		case SELECTED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(eb->selected);
		}
		break;
		case SPEED_PROP:
		{
			JS_NewDoubleValue(_jsctx, 1.0f, val);
		}
		break;
		case TIME_PROP:
		{
			gr2AnimTrooper* et = getEditModel();
			FLOAT32 t = (et && eb->blend->numBlends) ? eb->blend->GetTime(et->skeleton, 0) : 0.0f;
			JS_NewDoubleValue(_jsctx, t, val);
		}
		break;
		case LENGTH_PROP:
		{
			FLOAT32 len = (eb->blend->numBlends) ? eb->blend->GetLength(0) : 0.0f;
			JS_NewDoubleValue(_jsctx, len, val);
		}
		break;
		case TYPE_PROP:
		{
			*val = INT_TO_JSVAL(eb->blend->type);
		}
		break;
		case PLAY_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(eb->play);
		}
		break;
		case ACTIVE_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(eb->active);
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsBlend::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	gr2AnimTrooper* model = getEditModel();
	GR2EditBlend* eb = getEditBlend(_blend);

	if(!eb || !model) return JS_TRUE;

	switch(propid)
	{
		case WEIGHT_PROP:
		{
			JSFloat64 w;
			JS_ValueToNumber(_jsctx, *val, &w);
			eb->weight = w;

			// reset this when we adjust the weight
			setEditAnimation(nullptr);

			setEditBlendWeights(_blend, eb->weights[0], eb->weights[1], eb->weights[2], eb->weights[3]);

			if(eb->blend->type == gr2AnimationBlend::TRANSITION)
				gr2BlendControlSetModifier(model->skeleton, eb->blend, eb->weight, 1.0f);

			// set our new blend weight
			//gr2BlendControlSetWeight(model->skeleton, eb->blend, eb->weight);
		}
		break;
		case SELECTED_PROP:
		{
			JSBool selected = 0;
			JS_ValueToBoolean(_jsctx, *val, &selected);
			selectEditBlend((selected) ? _blend : -1);
		}
		break;
		case PLAY_PROP:
		{
			JSBool play = 0;
			JS_ValueToBoolean(_jsctx, *val, &play);
			eb->play = (play) ? true : false;
		}
		break;
		case ACTIVE_PROP:
		{
			JSBool active = 0;
			JS_ValueToBoolean(_jsctx, *val, &active);
			eb->active = (active) ? true : false;
		}
		break;
		case SPEED_PROP:
		{
			JSFloat64 s;
			JS_ValueToNumber(_jsctx, *val, &s);

			gr2BlendControlSetSpeed(model->skeleton, eb->blend, (FLOAT32)s);
		}
		break;
		case TIME_PROP:
		{
			JSFloat64 t;
			JS_ValueToNumber(_jsctx, *val, &t);

			gr2BlendControlSetStartOffset(model->skeleton, eb->blend, (FLOAT32)t);
		}
		break;
		case TYPE_PROP:
		{
			int32 type = gr2AnimationBlend::BASE;
			JS_ValueToInt32(_jsctx, *val, &type);

			bool different = eb->blend->type != type;
			eb->blend->type = type;

			if(eb->selected && different)
				updateEditBlends();
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsAnimation
*************************************************************************************************/

JSClass jsAnimation::jsclass = JS_CLASS_MACRO("animation");
JSFunctionSpec jsAnimation::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsAnimation::jsproperties[] =
{
	{"index",		jsAnimation::INDEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"filename",	jsAnimation::FILENAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"weights",		jsAnimation::WEIGHTS_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"time",		jsAnimation::TIME_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"length",		jsAnimation::LENGTH_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"events",		jsAnimation::EVENTS_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsAnimation::jsAnimation(int blend, int animation) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	_blend = blend;
	_animation = animation;
	_weights = new phui::js::Array;
	_events = new phui::js::Array;
}

jsAnimation::~jsAnimation()
{
}

JSBool jsAnimation::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	GR2EditBlend* eb = getEditBlend(_blend);
	if(!eb) return JS_TRUE;

	switch(propid)
	{
		case INDEX_PROP:
		{
			*val = INT_TO_JSVAL(_animation);
		}
		break;
		case FILENAME_PROP:
		{
			*val = (eb) ? phui::js::NewString(eb->blend->animations[_animation]->name) : JSVAL_NULL;
		}
		break;
		case WEIGHTS_PROP:
		{
			_weights->clear();

			gr2AnimTrooper* et = getEditModel();

			if(et)
			{
				for(int b = 0; b < et->skeleton->GetBoneCount(); b++)
					_weights->SetElement(new jsWeight(_blend, _animation, b), b);
			}

			*val = _weights->jsVal();
		}
		break;
		case TIME_PROP:
		{
			gr2AnimTrooper* et = getEditModel();
			FLOAT32 t = (et && eb->blend->numBlends) ? eb->blend->GetTime(et->skeleton, _animation) : 0.0f;
			JS_NewDoubleValue(_jsctx, t, val);
		}
		break;
		case LENGTH_PROP:
		{
			FLOAT32 l = (eb->blend->numBlends) ? eb->blend->GetLength(_animation) : 0.0f;
			JS_NewDoubleValue(_jsctx, l, val);
		}
		break;
		case EVENTS_PROP:
		{
			_events->clear();

			if(eb->blend->animations[_animation]->events)
			{
				for(int i = 0; i < eb->blend->animations[_animation]->events->size(); i++)
					_events->SetElement(new jsAnimEvent(_blend, _animation, i), i);
			}

			*val = _events->jsVal();
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsAnimEvent
*************************************************************************************************/

JSClass jsAnimEvent::jsclass = JS_CLASS_MACRO("animevent");
JSFunctionSpec jsAnimEvent::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsAnimEvent::jsproperties[] =
{
	{"index",		jsAnimEvent::INDEX_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"type",		jsAnimEvent::TYPE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"time",		jsAnimEvent::TIME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsAnimEvent::jsAnimEvent(int blend, int animation, int evt) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	_blend = blend;
	_animation = animation;
	_event = evt;
}

jsAnimEvent::~jsAnimEvent()
{
}

JSBool jsAnimEvent::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	GR2EditBlend* eb = getEditBlend(_blend);
	if(!eb) return JS_TRUE;

	switch(propid)
	{
		case INDEX_PROP:
		{
			*val = INT_TO_JSVAL(_event);
		}
		break;
		case TYPE_PROP:
		{
			*val = INT_TO_JSVAL(eb->blend->animations[_animation]->events->at(_event)->eventType);
		}
		break;
		case TIME_PROP:
		{
			JS_NewDoubleValue(_jsctx, eb->blend->animations[_animation]->events->at(_event)->eventTime, val);
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsWeight
*************************************************************************************************/

JSClass jsWeight::jsclass = JS_CLASS_MACRO("weight");
JSFunctionSpec jsWeight::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsWeight::jsproperties[] =
{
	{"index",		jsWeight::INDEX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"value",		jsWeight::VALUE_PROP,		JSPROP_PERMANENT},
	{0}
};

jsWeight::jsWeight(int blend, int animation, int bone) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	_blend = blend;
	_animation = animation;
	_bone = bone;
}

jsWeight::~jsWeight()
{
}

JSBool jsWeight::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	GR2EditBlend* eb = getEditBlend(_blend);
	if(!eb) return JS_TRUE;

	switch(propid)
	{
		case INDEX_PROP:
		{
			*val = INT_TO_JSVAL(_blend);
		}
		break;
		case VALUE_PROP:
		{
			FLOAT32 v = (FLOAT32)GrannyGetTrackMaskBoneWeight(eb->blend->animations[_animation]->mask, _bone);
			JS_NewDoubleValue(_jsctx, v, val);
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsWeight::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	GR2EditBlend* eb = getEditBlend(_blend);
	if(!eb) return JS_TRUE;

	switch(propid)
	{
		case VALUE_PROP:
		{
			JSFloat64 w;
			JS_ValueToNumber(_jsctx, *val, &w);
			GrannySetTrackMaskBoneWeight(eb->blend->animations[_animation]->mask, _bone, w);
			eb->modified = true;
		}
		break;
	}

	return JS_TRUE;
}

#endif
