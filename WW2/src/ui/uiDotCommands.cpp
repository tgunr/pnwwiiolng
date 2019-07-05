#include "syscfg.h" // Path is project specific.

#include "chat/chatBrigadeControl.h"
#include "chat/chatClientMessages.h"
#include "chat/chatDefines.h"
#include "chat/chatDotCommandList.h"
#include "chat/chatHandle.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatSquadControl.h"
#include "chat/chatWaypointExchg.h"
#include "client/clientConnections.h"
#include "client/clientSquad.h"
#include "client/clientSTO.h"
#include "client/GameClient.h"
#include "client/ww2HostState.h"
#include "draw/draw3d.h"
#include "fp/fpFPSdata.h"
#include "fp/fpFPS.h"
#include "fp/fpMain.h"
#include "fp/fpPPO.h"
#include "fp/fpTPSdata.h"
#include "fp/fpTPSPhysics.h"
#include "fx/fxWeather.h"
#include "fx/TransformEffect.h"
#include "gr2/gr2Anim.h"
#include "gr2/gr2layer.h"
#include "imap/ww2func.h"
#include "imap/ww2imap.h"
#include "imaplib/imap.h"
#include "include/vars.h"
#include "js/jsContent.h"
#include "js/jsGame.h"
#include "locl/loclApp.h"
#include "locl/loclLang.h"
#include "map/mapArenaDescription.h"
#include "map/mapPersonaSummary.h"
#include "maptools/GenerateIcons.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathUnitConversion.h"
#include "math/mathvect.h"
#include "miss/missionJoinRequest.h"
#include "ml/mlCompon.h"
#include "ml/mlgauges.h"
#include "ml/mlutility.h"
#include "ml/mlwndtnn.h"
#include "ob/obcs.h"
#include "ob/obdraw.h"
#include "ob/obMgr.h"
#include "ob/obplyr.h"
#include "ob/obpool.h"
#include "ob/stride_ptr.h"
#include "ogl/ogl.h"
#include "ogl/States.h"
#include "ogrp/ogrptype.h"
#include "osgImport/ShadowMap.h"
#include "pers/persgunsight.h"
#include "pers/persmain.h"
#include "pi/pi.h"
#include "spt/Random.h"
#include "rn/rnmain.h"
#include "scen/ScenarioManager.h"
#include "scen/scenMain.h"
#include "sdm/BuildTerrain.h"
#include "sdm/StratDB.h"
#include "spt/sptmain.h"
#include "str/stringLib.h"
#include "teul/teulKit.h"
#include "tools/lightTool.h"
#include "trn/speedgrass.h"
#include "trn/trnautogun.h"
#include "trn/trncd.h"
#include "trn/trncloud.h"
#include "trn/trnclutter.h"
#include "trn/trndata.h"
#include "trn/trnfx.h"
#include "trn/trngrass.h"
#include "trn/trnmain.h"
#include "trn/trnop.h"
#include "trn/trnRadClutter.h"
#include "trn/trntod.h"
#include "trn/trnvars.h"
#include "trn/trnwtr.h"
#include "tut/TutorialManager.h"
#include "uimap/CPRenderer.h"
#include "uimap/SuperCellMapView.h"
#include "uimap/uiMap.h"
#include "ui/uiChat.h"
#include "ui/uiDotCommands.h"
#include "ui/uiGameEvents.h"
#include "ui/uiMain.h"
#include "veh/vehAnimEnum.h"
#include "veh/vehanim.h"
#include "veh/vehcomms.h"
#include "veh/vehfx.h"
#include "veh/vehmain.h"
#include "veh/vehupdate.h"
#include "veh/vehVisDamage.h"
#include "build_info.hpp"
#include "weap/weapammo.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiServerTrackedObject.h"

extern void clientErrorLog(const char* const format, ...);
namespace Chat { extern HostCommands g_hostDotCommands; }

#ifdef MAKE_HOST_TERRAIN
#include "sdm/GenerateHostTerrain.h"
#endif

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

#include "fp/fpTPSKits.h"

#ifdef MULTI_CLIENT
#include "mc/multiClient.h"
#endif

#ifdef GR2_BLEND_EDITOR
#include "pers/pershud.h"
#endif

namespace {
	// FEEDBACK: Bad callsign '%s'
	static SINT32 _badCallsign(const std::wstring& input) {
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_CAUTION, LSTR_INVALID_CALLSIGN, input.c_str());
		return DC_HANDLED;
	}
}

/************************************************************************************************
	uiDotCommand
*************************************************************************************************/

#ifdef BETA_RELEASE

extern LOGICAL ConcussionEnabled;
extern LOGICAL SpallEnabled;
extern LOGICAL ShrapnelEnabled;
extern SINT32  LeakGridInc, LeakAngleInc, LeakYawStart, LeakYawEnd, LeakPitchStart, LeakPitchEnd;
extern LOGICAL DoLeakTest;
extern SINT32  NumLeaks;
extern SINT32  CurLeakYaw, CurLeakPitch;
extern fpFPSTrooperData fpsTrooper;
extern LOGICAL DisplayFoils;
extern LOGICAL DisplayPhysics;
extern LOGICAL DisplayCollideLines;
extern LOGICAL DisableForces;
extern FLOAT32 logTimerFPS;
extern LOGICAL TurretConvergenceElevateOff;
extern LOGICAL TurretConvergenceTraverseOff;
LOGICAL SloMoOn;
extern UINT32 OverrideSpawner;

#endif

extern char BuzzardFollowVehicle[256];
extern FLOAT32 BuzzardFollowDistance;


extern UINT32 _ReadAmmoTypes();
extern UINT32 _ReadBumpAndFrictionTable();

enum UI_COMMAND_TYPE
{
	UICT_CHAT = 0,
	UICT_SLASH,
	UICT_DOT
};

#define SQUAD_COMMAND_TIMEOUT 10000
#define DOT_COMMAND_SIZE 255
static UINT64 _nextsquadcommand = 0;
static piVECTOR posxyz;

#ifdef BETA_RELEASE
static void _PrintHitmap(OBJECT* obj, void* fp);
static void _PrintVehHitmap(vehType const*);
static void _ValidateVehHit(vehType const*);
static void _ValidateVehCmp(vehType const*);
static void _HitmapName(OBJECT* obj, void* p);
#endif

SINT32 dot_move(const char* line);

#ifdef GM_RELEASE
void vehmainToggleLOD();
#endif

static wchar_t _LastCommand[256];

static void CollideDown(Vec3d pos, colliderQuery::CollisionList& collisions, const unsigned int bits = COLLIDER_ALL);

//------------------------------------------------------
// New for ADD_DOT_COMMAND
struct dotCmdInfo
{
	dotCmdInfo() : asciiFunc(nullptr), wideFunc(nullptr), locCmd(nullptr), help(nullptr), key(false)
	{
	}

	// callback to call
	dotCmdFunctionA asciiFunc;
	dotCmdFunctionW wideFunc;
	const wchar_t* locCmd;	// alternate loc name for command
	const wchar_t* help;	// localized help for command
    bool key;				// requires a key file
};

using dotCmdMap = std::map<std::wstring, dotCmdInfo>;
static dotCmdMap _dotCommands;
//------------------------------------------------------

/************************************************************************************************
	ARGV argv( inputChars, numArguments, lastFillArgument )
	Utility that translates an input string into a vector of char* pointers by breaking
	the string up at each series of whitespace characters that are not quoted by matching
	quote characters.

	If fillLastArgument is true then any excess of arguments will all be treated as part of
	the last argument.

	argument [0] is assumed to be the .command type, so argument [1] is the first real argument.

	Return value is the number of* arguments* excluding the presumed command.
 ************************************************************************************************/

using ARGV_BASE = std::vector<std::wstring>;
struct ARGV : public ARGV_BASE
{
	ARGV(const char* source, UINT32 maxArguments, bool fillLastArgument);
	int argc;
private:
	ARGV(const ARGV& rhs) {}
};

ARGV::ARGV(const char* utf8Source, UINT32 maxArguments, bool fillLastArgument)
	: ARGV_BASE()
	, argc(0)
{
	ARGV_BASE::clear();
	if ( utf8Source == nullptr )
		return;

	const wchar_t* source = uiUTF8toUTF16(utf8Source);
	if ( *source == 0 )
		return;

	std::wstring token = L"";

	// Overrun the argument count because we need to check that
	// we exhausted the arguments we were provided
	while ( *source != 0 )
	{
		if ( fillLastArgument == false && this->size() > maxArguments )
			break;

		// Erase groups of whitespace
		while ( *source == L' ' || *source == L'\t' )
			++source;

		if ( *source == 0 )
			break;

		wchar_t terminator = L' ';
		if ( *source == L'"' || *source == L'\'' )
		{
			// Grab but pass the quote character, we don't want to copy it
			terminator = *(source++);
		}

		const wchar_t* start = source;

		while ( *source != 0 && *source != terminator )
			++source;

		if ( this->size() > maxArguments && this->empty() == false )
		{
			this->operator[](this->size() - 1).append(L" ");
			this->operator[](this->size() - 1).append(start, source - start);
		}
		else
		{
			token.assign(start, source - start);
			this->push_back(token);
		}

		// Advance past the terminator, as long as it wasn't
		// the zero byte.
		if ( *source )
			++source;
	}

	// Return the number of arguments resulting.
	argc = (this->empty()) ? 0 : this->size() - 1;
}

/************************************************************************************************
	HelpCmd
*************************************************************************************************/

static SINT32 dot_help(const wchar_t* line)
{
	wchar_t cmd[DOT_COMMAND_SIZE+1];
	cmd[0] = 0;

	int argc = swscanf(line, L"%*ls %255ls", cmd);
	SINT32 result = DC_PASS_TO_HOST;

	if(argc <= 0)	// ISO swscanf can return -1
	{
		std::wstring cmdlist = L"";

		rnmainListLocText(IDE_LANG_UI, 258);

        dotCmdMap::iterator itr = _dotCommands.begin();

        for(; itr != _dotCommands.end(); ++itr)
        {
            if(!itr->second.key || (itr->second.key && HasKeyFile))
            {
				if(cmdlist.empty() == false)
					cmdlist += L", ";

				if(itr->second.locCmd)
					cmdlist += itr->second.locCmd;
				else
					cmdlist += itr->first;
            }
        }

		uiChatPrintLine(cmdlist.c_str());

		if(Online)
		{
			std::wstring cmdlist = L"";

			// now we check our host commands if any were sent down
			for(size_t dc = 0; dc < Chat::g_hostDotCommands.size(); dc++)
			{
				Chat::HostCommand& info = Chat::g_hostDotCommands.at(dc);

				// only show playnet only commands if they have the right flag
				if(info.playnetOnly && !PlayerClient()->IsPermitted(IS_PLAYNET))
					continue;

				if(cmdlist.empty() == false)
					cmdlist += L", ";

				cmdlist += UTF16(info.command.c_str());
			}

			// host commands
			uiChatPrintLineColor(colorGREENALPHA, cmdlist.c_str());
		}

		result = DC_HANDLED;
	}
    else
	{
		// lower case the command
		for(int i = 0; i < (int)wcslen(cmd); i++)
			cmd[i] = towlower(cmd[i]);

		// look for an english version
		dotCmdMap::iterator itr = _dotCommands.find(cmd);

		// english not found, try a localized version
		if(itr == _dotCommands.end())
		{
			itr = _dotCommands.begin();

			for(; itr != _dotCommands.end(); ++itr)
			{
				if(itr->second.locCmd && wcsncasecmp(cmd, itr->second.locCmd, DOT_COMMAND_SIZE) == 0)
					break; // found one!
			}
		}

		if(itr != _dotCommands.end() && (!itr->second.key || HasKeyFile) && itr->second.help)
		{
			uiChatPrintLine(itr->second.help);
			result = DC_HANDLED;
		}

		if(Online)
		{
			// now we check our host commands if any were sent down
			for(size_t dc = 0; dc < Chat::g_hostDotCommands.size(); dc++)
			{
				Chat::HostCommand& info = Chat::g_hostDotCommands.at(dc);

				// only show playnet only commands if they have the right flag
				if(info.playnetOnly && !PlayerClient()->IsPermitted(IS_PLAYNET))
					continue;

				std::wstring hostCommand = UTF16(info.command.c_str());

				if(wcsncasecmp(cmd, hostCommand.c_str(), DOT_COMMAND_SIZE) == 0)
				{
					const std::wstring& usage = uiLocString((const char*)loclLangGetString(IDE_LANG_HOST, (UINT32)info.usage));

					if(usage[0] != L'?')
						uiChatPrintLineColor(colorGREENALPHA, L"%ls: %ls", L"HOST USAGE", usage.c_str());

					const std::wstring& detail = uiLocString((const char*)loclLangGetString(IDE_LANG_HOST, (UINT32)info.detail));

					if(detail[0] != L'?')
						uiChatPrintLineColor(colorGREENALPHA, L"%ls: %ls", L"HOST DETAIL", detail.c_str());

					result = DC_HANDLED;

					break;
				}
			}
		}
	}

	return result;
}

#ifdef MULTI_CLIENT
// multi client command is a public release command via this project because we
// want to run it with as much speed as possible; think of it as a buzzard client

/************************************************************************************************
	MCCmd
*************************************************************************************************/

static SINT32 dot_mc(const wchar_t* line)
{
	const wchar_t* ptr = line + 3;

	// run the command
	multiClientExecuteCommand((char*)UTF8(ptr), true);

	return DC_HANDLED;
}

#endif

/************************************************************************************************
	ConvergenceCmd
*************************************************************************************************/

static SINT32 dot_conv(const wchar_t* line)
{
	UINT32 range = 200;
	int argc = swscanf(line, L"%*ls %u", &range);

	bool ingame = uiInGame();
	const vehType* vtype = nullptr;

	if(ingame)
	{
		vehVehicle* veh = persGetPersonaVehiclePtr();
		if(veh) vtype = veh->VehicleType;
	}
	else
	{
		// use reserved weapon
		const WEAPON_SERIAL& ws = PlayerClient()->WeaponSerial();

		if(!ws.spec.isNull())
			vtype = vehTypeGetBySpec(ws.spec);
	}

	if(argc > 0)
	{
        if(range < 100)
            range = 100;

		/*
		dotcom.conv.setto = Convergence set to
		dotcom.conv.newconvergence = New convergence will take effect next time you spawn this vehicle
		dotcom.conv.currentlyset = Convergence is currently set to
		dotcom.conv.defaultcurrentlyset = Default convergence is currently set to
		*/


		// If we're in a plane, set the convergence for THIS PLANE ONLY
		if(vtype && vtype->Type == VEH_TYPE_AIR)
		{
			// Build setting name unique to vehicle ID"
			UINT16 key = vtype->WeaponSpec.asUINT16();

			char buff[256] = {'\0'};
			strprintf(buff, "convergence.%d", (SINT32)key);

			cfgPrefsSetInt(buff, (SINT32)range);
			persGunsightSetConvergenceRange((FLOAT32)range);

			uiChatPrintLine("%s %s %.0fm.",(const char*)vtype->Description, uiLocString("dotcom.conv.setto").c_str(), persGunsightConvergenceRange());

			if(uiInGame())
				uiChatPrintLine( uiLocString("dotcom.conv.newconvergence").c_str() );
		}
		// If we're not in a plane, set the global convergence ONLY
		else
		{
			cfgPrefsSetGlobal(cfgVEH_CONV, &range, true);
			// this chat line isn't needed because the prefs system spits out a message
			//uiChatPrintLine(L"Default convergence set to %.0fm.", persGunsightConvergenceRange());
		}
	}
	else
	{
		if(vtype && vtype->Type == VEH_TYPE_AIR)
		{
			// Build setting name unique to vehicle ID"
			UINT16 key = vtype->WeaponSpec.asUINT16();

			char buff[256] = {'\0'};
			strprintf(buff, "convergence.%d", (SINT32)key);

			SINT32 range = cfgPrefsGetInt(buff, (SINT32)Convergence);

			uiChatPrintLine("%s %s %dm.",(const char*)vtype->Description, uiLocString("dotcom.conv.currentlyset").c_str(), range);
		}
		else
		{
			uiChatPrintLine(L"%s %dm.", uiLocString("dotcom.conv.defaultcurrentlyset").c_str(), (SINT32)Convergence);
		}
	}

	return DC_HANDLED;
}

/************************************************************************************************
	EmoteCmd
*************************************************************************************************/

static SINT32 dot_emote(const char* line)
{
	char emote[256];
	char handle[256];
	emote[0] = handle[0] = 0;

	int argc = sscanf(line, "%*s %255s %255s", emote, handle);

	if(argc > 0 && !strncasecmp(emote, "ammo", 4))
		fpFPSStartEmote(emote, handle, FALSE);

	return DC_HANDLED;
}

/************************************************************************************************
	EmoteCmd alias
*************************************************************************************************/

static SINT32 dot_em(const char* line)
{
	return dot_emote(line);
}

/************************************************************************************************
	AmmoCmd
*************************************************************************************************/

static SINT32 dot_ammo(const char* line)
{
	SINT32 result = dot_emote("/emote ammo");
	vehmainRequestAmmo();
	return result;
}

/************************************************************************************************
	FuzeCmd
*************************************************************************************************/

#if defined(BETA_RELEASE)
static SINT32 dot_fuze(const char* line)
{
	FLOAT32 interval;
	int argc = sscanf(line, "%*s %f", &interval);

	if(argc > 0)
		weapAmmoSetFuzeInterval(interval);

	interval = weapAmmoGetFuzeInterval();

	if(interval != 0)
		uiChatPrintLine("Fuze set to %.2f", interval);
	else
		uiChatPrintLine("Using default fuze setting");

	return DC_HANDLED;
}
#endif

/************************************************************************************************
	Join mission by callsign
*************************************************************************************************/

static SINT32 dot_join(const char* line)
{
	if ( ! Online ) return DC_HANDLED;
	if ( uiInGame() ) {
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"Cannot /join while in game");
		return DC_HANDLED;
	}

	ARGV argv(line, 1, false);
	if ( argv.argc != 1 ) {
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/join <callsign>");
		return DC_HANDLED;
	}

	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( handle == 0LL )
		return _badCallsign(argv[1]);

	MissionJoinRequest::Request request(handle);
	request.send(PlayerClient()->ChatConnection());

	return DC_HANDLED;
}

static SINT32 dot_j(const char* line) {
	return dot_join(line);
}

/************************************************************************************************
	Mission Auto Leader
*************************************************************************************************/

static SINT32 dot_autoleader(const char* line)
{
	char onoff[9] = "";

	int argc = sscanf(line, "%*s %8s", onoff);
	bool autoleader = false;

	if(argc > 0)
	{
		autoleader = (!strcasecmp(onoff, "on")) ? true: false;
		cfgPrefsSetGlobal(cfgAUTO_LEADER, &autoleader, true);
	}

	cfgPrefsGetGlobal(cfgAUTO_LEADER, &autoleader);
	uiChatPrintLine("Auto Leader: %s", (autoleader) ? "on": "off");

	return DC_HANDLED;
}

/************************************************************************************************
	Squad Management Commands
*************************************************************************************************/

static SINT32 dot_recruit(const char* line)
{
	if(!Online) return DC_HANDLED;
	if(teulClock64 < _nextsquadcommand) return DC_HANDLED;

	ARGV argv(line, 1, false);
	if(argv.argc != 1)
	{
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/recruit <callsign>");
		return DC_HANDLED;
	}

	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( handle == 0LL )
		return _badCallsign(argv[1]);

	SquadControl control;
	control.sendRecruit(PlayerClient()->ChatConnection(), handle);
	_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;

	return DC_HANDLED;
}

static SINT32 dot_recruiting(const char* line)
{
	if(teulClock64 >= _nextsquadcommand)
	{
		char onoff[9] = "";

		int argc = sscanf(line, "%*s %8s", onoff);
		bool recruiting = false;

		if(argc > 0)
		{
			recruiting = (!strcasecmp(onoff, "on")) ? true: false;
			cfgPrefsSetGlobal(cfgIS_RECRUITING, &recruiting, true);
			uiRunScript("game.onchangerecruiting()");
		}

		cfgPrefsGetGlobal(cfgIS_RECRUITING, &recruiting);
		uiChatPrintLine("Recruiting: %s", (recruiting) ? "on": "off");

		_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;
	}

	return DC_HANDLED;
}

static SINT32 dot_decline(const char* line)
{
	if(teulClock64 >= _nextsquadcommand)
	{
		SquadControl control;
		control.sendDecline(PlayerClient()->ChatConnection());
		_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;
		PlayerClient()->InvalidateSquadInvite();
	}

	return DC_HANDLED;
}

static SINT32 dot_accept(const char* line)
{
	if(teulClock64 >= _nextsquadcommand)
	{
		SquadControl control;
		control.sendAccept(PlayerClient()->ChatConnection());
		_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;
		PlayerClient()->InvalidateSquadInvite();
	}

	return DC_HANDLED;
}

static bool ConfirmCmd(const char* line)
{
	std::string cmd(line);

	if(cmd.size() > 7 && cmd.substr(cmd.size() - 7, cmd.size()) == "confirm")
		return true;

	jsval rval;
	jsval argv[1] = { phui::js::NewString(UTF16(line)) };

	uiGame()->CallFunction("confirmdotcommand", 1, argv, &rval);

	return false;
}

static SINT32 dot_leave(const char* line)
{
	if(teulClock64 >= _nextsquadcommand && ConfirmCmd(line))
	{
		SquadControl control;
		control.sendLeave(PlayerClient()->ChatConnection());
		_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;
	}

	return DC_HANDLED;
}

static SINT32 dot_create(const char* line)
{
	if(teulClock64 < _nextsquadcommand) return DC_HANDLED;

	std::vector<ChatHandle> players; // for future use

	ARGV argv(line, 2, false);
	if ( argv.argc != 2 )
	{
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/create <squad callsign> <squad name>");
		return DC_HANDLED;
	}
	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( !handle ) return _badCallsign(argv[1]);

	SquadControl control;
	control.sendCreate(PlayerClient()->ChatConnection(), handle, uiUTF16toUTF8(argv[2].c_str()), players);
	_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;

	return DC_HANDLED;
}

static SINT32 dot_disband(const char* line)
{
	if(teulClock64 >= _nextsquadcommand && ConfirmCmd(line))
	{
		SquadControl control;
		control.sendDisband(PlayerClient()->ChatConnection());
		_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;
	}

	return DC_HANDLED;
}

static SINT32 dot_remove(const char* line)
{
	if(teulClock64 < _nextsquadcommand) return DC_HANDLED;

	ARGV argv(line, 1, false);
	if ( argv.argc != 1 )
	{
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/remove <callsign>");
		return DC_HANDLED;
	}

	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( !handle ) return _badCallsign(argv[1]);

	if ( !ConfirmCmd(line) ) return DC_HANDLED;

	SquadControl control;
	control.sendKick(PlayerClient()->ChatConnection(), handle, "");

	_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;

	return DC_HANDLED;
}

static SINT32 dot_promote(const char* line)
{
	if(teulClock64 < _nextsquadcommand) return DC_HANDLED;

	ARGV argv(line, 1, false);
	if ( argv.argc != 1 )
	{
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/promote <callsign>");
		return DC_HANDLED;
	}

	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( !handle ) return _badCallsign(argv[1]);

	SquadControl control;
	control.sendPromote(PlayerClient()->ChatConnection(), handle);
	_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;

	return DC_HANDLED;
}

static SINT32 dot_demote(const char* line)
{
	if(teulClock64 < _nextsquadcommand) return DC_HANDLED;

	ARGV argv(line, 1, false);
	if ( argv.argc != 1 )
	{
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/demote <callsign>");
		return DC_HANDLED;
	}

	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( !handle ) return _badCallsign(argv[1]);

	SquadControl control;
	control.sendDemote(PlayerClient()->ChatConnection(), handle);
	_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;

	return DC_HANDLED;
}

static SINT32 dot_resign(const char* line)
{
	if(teulClock64 < _nextsquadcommand) return DC_HANDLED;

	ARGV argv(line, 1, false);
	if ( argv.argc != 1 )
	{
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, LSTR_GENERIC, L"/resign <replacement co callsign>");
		return DC_HANDLED;
	}

	ChatHandle handle = chatGetLowercaseHandle(argv[1].c_str());
	if ( !handle ) return _badCallsign(argv[1]);

	if ( !ConfirmCmd(line) ) return DC_HANDLED;

	SquadControl control;
	control.sendResign(PlayerClient()->ChatConnection(), handle);
	_nextsquadcommand = teulClock64 + SQUAD_COMMAND_TIMEOUT;

	return DC_HANDLED;
}

// oic management

// FEEDBACK: Unknown brigade '%s'
static SINT32 _unknownBrigade(const std::wstring& input)
{
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_CAUTION, LSTR_UNKNOWN_UNIT, input.c_str());
	return DC_HANDLED;
}

static HCUnit* _lookupUnit(const std::wstring& name)
{
	ChatHandle handle = chatGetLowercaseHandle(name.c_str());
	HCUnit* unit = HCUnit::Lookup(handle);
	if ( unit == nullptr )
		_unknownBrigade(name);
	return unit;
}

// FEEDBACK:
// USAGE: /command blah
// DETAIL: Explanation
static SINT32 _commandUsage(LSTRING_ID usageId, LSTRING_ID detailId)
{
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_USAGE, usageId);
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_DETAIL, detailId);
	return DC_HANDLED;
}

static SINT32 dot_oicvolunteer(const char* line)
{
	if(!Online)	return DC_HANDLED;

	ARGV argv(line, 1, false);
	if(argv.argc != 1)
		return _commandUsage(LSTR_OICVOL_USAGE, LSTR_OICVOL_DETAIL);

	HCUnit* unit = _lookupUnit(argv[1]);
	if ( unit == nullptr )
		return DC_HANDLED;

	BrigadeControlRequest control;
	control.volunteerOIC(PlayerClient()->ChatConnection(), unit->unitID());

	return DC_HANDLED;
}

static SINT32 dot_oicnominate(const char* line)
{
	if(!Online)	return DC_HANDLED;

	ARGV argv(line, 2, false);

	if(argv.argc != 2)
		return _commandUsage(LSTR_OICNOM_USAGE, LSTR_OICNOM_DETAIL);

	HCUnit* unit = _lookupUnit(argv[1]);
	if ( unit == nullptr )
		return DC_HANDLED;
	if ( chatIsValidCallsign(argv[2].c_str()) == false )
	{
		return _badCallsign(argv[2]);
	}
	ChatHandle handle = chatGetLowercaseHandle(argv[2].c_str());
	if ( handle == 0LL )
	{
		return _badCallsign(argv[2]);
	}

	BrigadeControlRequest control;
	control.nominateOIC(PlayerClient()->ChatConnection(), unit->unitID(), handle);

	return DC_HANDLED;
}

static SINT32 dot_oicveto(const char* line)
{
	if(!Online)	return DC_HANDLED;

	ARGV argv(line, 2, false);
	if ( argv.argc != 2 )
		return _commandUsage(LSTR_OICVETO_USAGE, LSTR_OICVETO_DETAIL);

	HCUnit* unit = _lookupUnit(argv[1]);
	if ( unit == nullptr )
		return DC_HANDLED;

	if ( chatIsValidCallsign(argv[2].c_str()) )
		return _badCallsign(argv[2]);
	ChatHandle handle = chatGetLowercaseHandle(argv[2].c_str());
	if ( handle == 0LL )
		return _badCallsign(argv[2]);

	BrigadeControlRequest control;
	control.vetoOIC(PlayerClient()->ChatConnection(), unit->unitID(), handle);

	return DC_HANDLED;
}

static SINT32 dot_oicresign(const char* line)
{
	if(!Online)	return DC_HANDLED;

	ARGV argv(line, 1, false);
	if ( argv.argc != 1 )
		return _commandUsage(LSTR_OICRES_USAGE, LSTR_OICRES_DETAIL);
	HCUnit* unit = _lookupUnit(argv[1]);
	if ( unit == nullptr )
		return DC_HANDLED;

	BrigadeControlRequest control;
	control.resignOIC(PlayerClient()->ChatConnection(), unit->unitID());

	return DC_HANDLED;
}

static SINT32 dot_oicreplace(const char* line)
{
	if(!Online)	return DC_HANDLED;

	char brigade[9]	= ""; // brigade callsign
	char player[9]	= ""; // player callsign

	int argc = sscanf(line, "%*s %8s %8s", brigade, player);

	if(argc > 1)
	{
		BrigadeControlRequest control;

		HCUnit* unit = HCUnit::Lookup(brigade);
		ChatHandle handle = chatGetLowercaseHandle(player);

		if(unit && handle)
			control.replaceOIC(PlayerClient()->ChatConnection(), unit->unitID(), handle);
	}

	return DC_HANDLED;
}

static SINT32 dot_oicremove(const char* line)
{
	if(!Online)	return DC_HANDLED;

	ARGV argv(line, 2, false);
	if ( argv.argc != 2 )
		return _commandUsage(LSTR_OICREM_USAGE, LSTR_OICREM_DETAIL);

	HCUnit* unit = _lookupUnit(argv[1]);
	if ( unit == nullptr )
		return DC_HANDLED;

	if ( !chatIsValidCallsign(argv[2].c_str()) )
		return _badCallsign(argv[2]);
	ChatHandle handle = chatGetLowercaseHandle(argv[2].c_str());
	if ( !handle )
		return _badCallsign(argv[2]);

	BrigadeControlRequest control;
	if(handle == chatGetLowercaseHandle(PlayerClient()->Personas().m_callsign))
		control.resignOIC(PlayerClient()->ChatConnection(), unit->unitID());
	else
		control.removeOIC(PlayerClient()->ChatConnection(), unit->unitID(), handle);

	return DC_HANDLED;
}

static SINT32 dot_hccp(const char* line)
{
	if(Online && !uiViewport()->FindView("hccp") && (PlayerClient()->IsHC() || PlayerClient()->IsOIC()))
		uiRunScript("load(\"hccp\");");

	return DC_HANDLED;
}

#if defined(BETA_RELEASE)
static SINT32 dot_edit(const char* line)
{
	char type[10+1]		= "";
	char title[10+1]		= "";
	char value[10+1]		= "";

	sscanf(line, "%*s %10s %10s %10s", type, title, value);

	PlayerClient()->Edit(type, title, value);

	return DC_HANDLED;
}
#endif

static SINT32 dot_graduate(const char* line)
{
	char group[25+1] = "";
	int flag = -1;

	int argc = sscanf(line, "%*s %25s %d", group, &flag);

	// set it if specified
	if(argc > 0)
	{
		if(!strcasecmp(group, "true"))
		{
			const char* groups[] = { "infantry", "armor", "fighter" };

			for(int i = 0; i < 3; i++)
			{
				TUT_MGR->SetComplete(groups[i], -1, true);
			}

			uiChatPrintLine("Basic training is compulsory, all other tutorial groups have been set to complete.");
		}
        else if (!strcasecmp(group, "false"))
        {
            const char* groups[] = { "basic", "infantry", "armor", "fighter" };

            for (int i = 0; i < 4; i++)
            {
                TUT_MGR->SetComplete(groups[i], -1, false);
            }

            uiChatPrintLine("Set all tutorial groups to incomplete");
        }
		else
		{
            uiChatPrintLine("We no longer allow you to set individual tutorials to complete/incomplete.");
		}
	}
	else
	{
		uiChatPrintLine("You must specify true or false.");
	}

	return DC_HANDLED;
}

static int dot_rain(const char* line)
{
    if(Online)
        return DC_HANDLED;

    static int idx = 0;
    int seed = GetRandom(0, 15);

    idx++;
    idx = idx >= 3 ? 0 : idx;

    fxWeatherSetState(idx, seed);

    uiChatPrintLine("WEATHER: weather = %d seed = %d", idx, seed);

    return DC_HANDLED;
}

//////////////////////////////////////////////////////////////////////
//	BenchRemagen
//////////////////////////////////////////////////////////////////////
static SINT32 dot_benchremagen(const char*)
{
#if RELEASE_CLIENT
    if(Online)
    {
        uiChatPrintLine(L".benchremagen for offline use only");
        return DC_HANDLED;
    }

    if(uiInGame())
    {
        uiChatPrintLine(L".benchremagen must be set prior to spawn");
        return DC_HANDLED;
    }

    OverrideSpawner = BENCHRAMAGEN_OVERRIDE;
    uiChatPrintLine(L"Override spawn for benchmarking Remagen");

#elif defined(BETA_RELEASE)
	dot_move("move N50.34.12 E7.14.25 70");
#endif

	return DC_HANDLED;
}

//////////////////////////////////////////////////////////////////////
//	BenchAntwerp
//////////////////////////////////////////////////////////////////////
static SINT32 dot_benchantwerp(const char*)
{
#if RELEASE_CLIENT
    if(Online)
    {
        uiChatPrintLine(L".benchantwerp for offline use only");
        return DC_HANDLED;
    }

    if(uiInGame())
    {
        uiChatPrintLine(L".benchantwerp must be set prior to spawn");
        return DC_HANDLED;
    }

    OverrideSpawner = BENCHANTWERP_OVERRIDE;
    uiChatPrintLine(L"Override spawn for benchmarking Antwerp");

#elif defined(BETA_RELEASE)
	dot_move("move 211414 3070984 300");
#endif

	return DC_HANDLED;
}

static std::set<int> vehicles;

static void _removevehicle(int id)
{
    std::set<int>::iterator find = vehicles.find(id);

    if(find != vehicles.end())
    {
        ServerTrackedObject* sto = ServerTrackedObject::lookup(id);

        if(sto)
        {
            sto->deactivate();

			if(!Online)
				vehmainRemoveSTO(sto->uid());
        }
        else
        {
            rnmainListText("Failed to deactivate ID %d", id);
        }

        vehicles.erase(find);

	    obcsResetSortedList();
	    obcsResetUnsortedList();
        obcsResetMultiTexture();
	    obcsResetSortedCockpitList();
	    obcsResetUnsortedCockpitList();
    }
    else
    {
        rnmainListText("Failed to remove ID %d", id);
    }
}

static void _removeallvehicles()
{
    std::set<int>::iterator b = vehicles.begin();
    std::set<int>::iterator e = vehicles.end();
    std::set<int>::iterator free;

    while(b != e)
    {
        int id = *b;

        ServerTrackedObject* sto = ServerTrackedObject::lookup(id);

        if(sto)
        {
            sto->deactivate();

			if(!Online)
				vehmainRemoveSTO(sto->uid());
        }
        else
        {
            rnmainListText("Failed to deactivate ID %d", id);
        }

        free = b;
        ++b;
        vehicles.erase(free);
    }

	obcsResetSortedList();
	obcsResetUnsortedList();
    obcsResetMultiTexture();
	obcsResetSortedCockpitList();
	obcsResetUnsortedCockpitList();
}

// Spawn any vehicle
#if defined(BETA_RELEASE)
static SINT32 dot_spawn(const char* line)
{
	char id[255+1];

	int argc = sscanf(line, "%*s %255s", id);

	/*
	if(uiInGame())
	{
		uiChatPrintLine("");
		return DC_HANDLED;
	}
	*/

	if(argc == 0)
	{
		uiChatPrintLine("You must specify a vehicle id to spawn!");
		return DC_HANDLED;
	}

	vehType const* vehtype = vehTypeGetById(id);
	if(vehtype == nullptr)
	{
		uiChatPrintLine("Invalid vehicle id specified!");
		return DC_HANDLED;
	}

	if(!Online)
		uiSetDespawnInfo("offline");

	if(SCEN_MGR->IsActive())
	{
		FLOAT64VECTOR at = { 0.0f, 0.0f, 0.0f };
		FLOAT64 yaw = 0;

		const ScenarioWorldObject* spawn = SCEN_MGR->VehicleSpawn(vehtype);
		if(!spawn && Online) return JS_TRUE;

		if(spawn)
		{
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

		// send the request
		if(Online)
		{
			PlayerClient()->RequestDirectSpawn(vehtype->WeaponSpec, &at, yaw);
		}
		else
		{
			mlSetSpawnStuff(0,0,-1,1);
			mlSetMobileSpawnStuff(0, at.x, at.y, at.z, yaw);
			WEAPON_SERIAL blah={0};
			vehcommSendVehicleRequest(vehtype->VehicleTypeNum,&blah);
		}

		if(Online)
			uiSetDespawnInfo("scenarios/scenario");
		else
			uiSetDespawnInfo("offline");

		return DC_HANDLED;
	}

	mlSetSpawnStuff(0,0,-1,1);

	WEAPON_SERIAL blah={0};
	vehcommSendVehicleRequest(vehtype->VehicleTypeNum,&blah);

	return DC_HANDLED;
}
#endif

// Spawn vehicle at x,y,z
static SINT32 dot_vspawn(const char* line)
{
	std::string str = line;

//    static int vehicleid = -1000;

	// Remove ID
    UINT32 strpos = str.find(" -r");

    if(strpos != str.npos)
    {
        if(str.find(" -r all") != str.npos)
        {
            _removeallvehicles();
            return DC_HANDLED;
        }

        //-r #
        strpos += 4;

        if(strpos > str.length())
        {
            // -r alone causes removal of all vehicles
            _removeallvehicles();
            return DC_HANDLED;
        }

        // Remove specific vehicle
        std::string s = str.substr(strpos, str.length());

        int id = atol(s.c_str());

        _removevehicle(id);
        return DC_HANDLED;
    }

    strpos = str.find("-num");
    int numveh = 1;

	/*
    if(strpos != str.npos)
    {
        strpos += 5;
        int endpos = strpos;

        while(endpos < (int)str.length() && str.at(endpos) != ' ')
        {
            endpos++;
        }

        if(endpos > (int)strpos)
        {
			// 'atol' will process all numeric digits and then stop.
			numveh = atol(str.c_str() + strpos);
        }
    }
	*/

    char buffer[128];
    // Skip length of "vspawn "
    strcpy(buffer, line + 7);

    char name[64];
//    FLOAT64ATT att = {0, 0 ,0};
    FLOAT64VECTOR pos = {0, 0, 0};

    SINT32 x = 0, y = 0, z = 0, rad = 10;

	if (strpos != str.npos)
        sscanf(buffer, "%64s -num %d %d %d %d %d", name, &numveh, &x, &y, &z, &rad);
    else
        sscanf(buffer, "%64s %d %d %d", name, &x, &y, &z);

    pos.x = (double)x;
    pos.y = (double)y;
    pos.z = (double)z;

	mlVehicleData* modelData = persGetPersonaModelDataPtr();
	FLOAT64VECTOR vehpos = {0,0,0};
	if(modelData) vehpos = modelData->VehiclePos;

	for(int i = 0; i < (int)vehtypeCount(); i++)
	{
        vehType const* vehtype = nullptr;

        vehtype = vehtypeGetByIndex(i);

        if(!vehtype)
        {
            continue;
        }

        if(!strncmp((char*)vehtype->Id, name, strlen(name)))
        {
            double x = pos.x + vehpos.x;
            double y = pos.y + vehpos.y;
            double z = pos.z + vehpos.z;

            double radius = rad;

            if(vehtype->Type == VEH_TYPE_PERSON)
                radius = 4.0f;

            FASTCOLLIDE list[32];
            COLLISION c;
            UINT32 numcollides = 0;
            // For debugging...
            static float offset = 1.0f;

            memset(&c, 0, sizeof(COLLISION));
            memset(list, 0, sizeof(list));

            int col = (int)sqrt((float)numveh);

            int colcnt = 0;

            for(int q = 0; q < numveh; q++)
            {
                // Place on ground at pos (x, y)
                if(vehtype->Type == VEH_TYPE_GROUND && pos.z == 0)
                {
	                trncdIgnoreTreeCollide(TRUE);

                    c.oldpos.set((float)pos.x, (float)pos.y, -10000.0f);
                    c.newpos.set((float)pos.x, (float)pos.y, 10000.0f);

                    if(trncdCollideDownShortList(c, list, numcollides))
                    {
                        piVECTOR p;
                        math3dVectorMultiply(&p, &list[0].CollidePnt, &list[0].CollideObject->mat);
                        z = fabs(p.z) + offset;
                    }
                    else
                    {
                        if(trncdCheckTerrain(&c))
                        {
                            piVECTOR p;

                            math3dVectorMultiply(&p, &c.colpos, &c.colobj->mat);
                            z = fabs(p.z) + offset;
                        }
                    }

	                trncdIgnoreTreeCollide(FALSE);
                }

                WorldPosition_m position(x, y, z);
                AttitudeVector attitude(0, 0, 0); //GetRandom(0.0f, PI2)

                VehicleSTO* vehicle = new VehicleSTO(0, vehtype, position, attitude);
                ServerTrackedObject::addToIndex(vehicle);

                std::pair<std::set<int>::iterator, bool> valid;

                valid = vehicles.insert((int)vehicle->uid());

                if(valid.second == false)
                {
                    uiChatPrintLine("Max vehicles reached (%d)", vehicles.size());
                    break;
                }

				// get the vehicle created
				vehUpdate();

				// look it up
				vehVehicle*veh = vehFindVehicle(vehicle->getSTOVehicleID());

				// flag it
				veh->Flags |= VEH_FLAGS_IS_CLONE;

				// reset our name so we can easily id which vehicle index we're using
				snprintf((char*)veh->Handle, 10, "C%d", (int)vehicle->uid());
				veh->Chat_Handle = chatGetHandle((const char*)veh->Handle);

				if(veh->VehicleType->Type == VEH_TYPE_PERSON && strstr(line , "altmodel") && veh->VehicleObjectData)
				{
					gr2AnimBitPacker bp = gr2AnimBitPacker(veh->AnimationBits);
					bp.useAltTrooperMesh = 1;
					veh->interpolator.SetCloneFlags(bp.pack());
					/*
					veh->AnimationBits = bp.pack();
					veh->VehicleObjectData->PlaneObject->animModel->SetFlag(gr2AnimTrooper::USE_ALT_MODEL);
					veh->VehicleObjectData->PlaneObject->animModel->model.Set(true);
					*/
				}

                if(++colcnt < col)
                {
                    x += radius;
                }
                else
                {
                    colcnt = 0;
                    x = pos.x + vehpos.x;
                    y += radius;
                }
             }

             break;
        }
    }

    return DC_HANDLED;
}

//////////////////////////////////////////////////////////////////////
//	BenchVehicles
//////////////////////////////////////////////////////////////////////
static SINT32 dot_benchvehicles(const char* line)
{
    bool valid = false;

#if RELEASE_CLIENT
    switch(OverrideSpawner)
    {
    case BENCHANTWERP_OVERRIDE:
    case BENCHRAMAGEN_OVERRIDE:
        {
            valid = true;
            break;
        }
    default:
        return DC_HANDLED;
    }
#elif defined(BETA_RELEASE)
    valid = true;
#endif

    if(Online)
    {
        uiChatPrintLine(".benchvehicles for offline use only");
        return DC_HANDLED;
    }

    if(!valid)
        return DC_HANDLED;

    bool allofem = false;

    if(strstr(line, "-allofem"))
    {
        allofem = true;
    }

    if ( allofem )
    {
      dot_vspawn("vspawn oplbltz 180 40 80");
      dot_vspawn("vspawn sdkfz7 200 40 80");
      dot_vspawn("vspawn sdkfz251c 220 40 80");
      dot_vspawn("vspawn flak30 260 40 80");
      dot_vspawn("vspawn de_bofors 280 40 80");
      dot_vspawn("vspawn pak36 20 60 80");
      dot_vspawn("vspawn pak38 40 60 80");
      dot_vspawn("vspawn pak40 60 60 80");
      dot_vspawn("vspawn flak36 80 60 80");
      dot_vspawn("vspawn sdkfz232 100 60 80");
      dot_vspawn("vspawn pziic 120 60 80");
      dot_vspawn("vspawn p38t 140 60 80");
      dot_vspawn("vspawn pziiif 160 60 80");
      dot_vspawn("vspawn pziiih 180 60 80");
      dot_vspawn("vspawn pzivd 200 60 80");
      dot_vspawn("vspawn pzivg 220 60 80");
      dot_vspawn("vspawn pzvie 240 60 80");
      dot_vspawn("vspawn stug3b 260 60 80");
      dot_vspawn("vspawn stug3g 280 60 80");
      dot_vspawn("vspawn bf110c 20 80 80");
      dot_vspawn("vspawn bf110fb 40 80 80");
      dot_vspawn("vspawn bf109e1 60 80 80");
      dot_vspawn("vspawn bf109e 80 80 80");
      dot_vspawn("vspawn bf109f2 100 80 80");
      dot_vspawn("vspawn bf109f4 120 80 80");
      dot_vspawn("vspawn me109g6 140 80 80");
      dot_vspawn("vspawn fw190a4 160 80 80");
      dot_vspawn("vspawn ju87b 180 80 80");
      dot_vspawn("vspawn de_blent 200 80 80");
      dot_vspawn("vspawn he111 220 80 80");
      dot_vspawn("vspawn ju52 240 80 80");
      dot_vspawn("vspawn z34 280 80 0");
      dot_vspawn("vspawn de_freighter 20 100 0");
      dot_vspawn("vspawn bedford 80 120 80");
      dot_vspawn("vspawn morris 100 120 80");
      dot_vspawn("vspawn uk_mle38 120 120 80");
      dot_vspawn("vspawn bofors40mm 140 120 80");
      dot_vspawn("vspawn qf2pdr 160 120 80");
      dot_vspawn("vspawn qf6pdr 180 120 80");
      dot_vspawn("vspawn qf17pdr 200 120 80");
      dot_vspawn("vspawn vickersmkvi 220 120 80");
      dot_vspawn("vspawn daimler 240 120 80");
      dot_vspawn("vspawn a13 260 120 80");
      dot_vspawn("vspawn crusader2 280 120 80");
      dot_vspawn("vspawn crusader3 20 140 80");
      dot_vspawn("vspawn matilda2 40 140 80");
      dot_vspawn("vspawn churchill3 60 140 80");
      dot_vspawn("vspawn churchill7 80 140 80");
      dot_vspawn("vspawn hurri1 100 140 80");
      dot_vspawn("vspawn hurri2b 120 140 80");
      dot_vspawn("vspawn hurri2c 140 140 80");
      dot_vspawn("vspawn spit1 160 140 80");
      dot_vspawn("vspawn spit2b 180 140 80");
      dot_vspawn("vspawn spitvb 200 140 80");
      dot_vspawn("vspawn spit9c 220 140 80");
      dot_vspawn("vspawn bleni 240 140 80");
      dot_vspawn("vspawn blent 260 140 80");
      dot_vspawn("vspawn bleniv 280 140 80");
      dot_vspawn("vspawn boston1 20 160 80");
      dot_vspawn("vspawn c47 40 160 80");
      dot_vspawn("vspawn fairb 60 160 0");
      dot_vspawn("vspawn uk_z34 80 160 0");
      dot_vspawn("vspawn uk_freighter 100 160 0");
      dot_vspawn("vspawn laffly 160 180 80");
      dot_vspawn("vspawn lafflyw15 180 180 80");
      dot_vspawn("vspawn mle38 200 180 80");
      dot_vspawn("vspawn fr_bofors 220 180 80");
      dot_vspawn("vspawn mle25 240 180 80");
      dot_vspawn("vspawn mle47 260 180 80");
      dot_vspawn("vspawn m1m1a3 280 180 80");
      dot_vspawn("vspawn m1a2m5 20 200 80");
      dot_vspawn("vspawn panhard 40 200 80");
      dot_vspawn("vspawn r35 60 200 80");
      dot_vspawn("vspawn h39 80 200 80");
      dot_vspawn("vspawn stuartm3a3 100 200 80");
      dot_vspawn("vspawn s35 120 200 80");
      dot_vspawn("vspawn charb1 140 200 80");
      dot_vspawn("vspawn m4a2 160 200 80");
      dot_vspawn("vspawn m10 180 200 80");
      dot_vspawn("vspawn m4a3 200 200 80");
      dot_vspawn("vspawn hawk75 220 200 80");
      dot_vspawn("vspawn d520 260 200 80");
      dot_vspawn("vspawn h81a2 280 200 80");
      dot_vspawn("vspawn h87b3 20 220 80");
      dot_vspawn("vspawn bell14a 40 220 80");
      dot_vspawn("vspawn p38f 60 220 80");
      dot_vspawn("vspawn fr_blen 80 220 80");
      dot_vspawn("vspawn db7 100 220 80");
      dot_vspawn("vspawn fr_ju52 120 220 80");
      dot_vspawn("vspawn fr_fair 140 220 0");
      dot_vspawn("vspawn fr_z34 160 220 0");
      dot_vspawn("vspawn fr_freighter 180 220 0");
    }
    else
    {
        dot_vspawn("vspawn pak40 -5 5 0");
        dot_vspawn("vspawn m1a2m5 0 5 0");
        dot_vspawn("vspawn qf17pdr 5 5 0");
        dot_vspawn("vspawn pak38 -5 10 0");
        dot_vspawn("vspawn m1m1a3 0 10 0");
        dot_vspawn("vspawn qf6pdr 5 10 0");
        dot_vspawn("vspawn pzvie -5 15 0");
        dot_vspawn("vspawn m4a3 0 15 0");
        dot_vspawn("vspawn churchill7 5 15 0");
        dot_vspawn("vspawn pzivg -5 20 0");
        dot_vspawn("vspawn m4a2 0 20 0");
        dot_vspawn("vspawn churchill3	5 20 0");
        dot_vspawn("vspawn stug3g -5 25 0");
        dot_vspawn("vspawn m10 0 25 0");
        dot_vspawn("vspawn crusader3 5 25 0");
        dot_vspawn("vspawn ju52 -5 25 80");
        dot_vspawn("vspawn p38f 0 25 80");
        dot_vspawn("vspawn c47 5 25 80");
        dot_vspawn("vspawn daimler -5 150 80");
        dot_vspawn("vspawn flak36 0 150 80");
        dot_vspawn("vspawn mle47 5 150 80");
        dot_vspawn("vspawn daimler -5 150 80");
        dot_vspawn("vspawn flak36 0 150 80");
        dot_vspawn("vspawn mle47 5 150 80");
        dot_vspawn("vspawn fr_z34 -50 210 80");
        dot_vspawn("vspawn uk_z34 0 210 80");
        dot_vspawn("vspawn z34 50 210 80");
        dot_vspawn("vspawn he111 -50 210 80");
        dot_vspawn("vspawn fw190a4 0 210 80");
        dot_vspawn("vspawn bf109f4 50 210 80");
        dot_vspawn("vspawn de_freighter -50 260 80");
        dot_vspawn("vspawn de_freighter 0 260 80");
        dot_vspawn("vspawn fr_freighter 50 260 80");
        dot_vspawn("vspawn spit9c -50 260 80");
        dot_vspawn("vspawn spit9c 0 260 80");
        dot_vspawn("vspawn spit9c 50 260 80");
        dot_vspawn("vspawn matilda2 -50 300 80");
        dot_vspawn("vspawn matilda2 0 300 80");
        dot_vspawn("vspawn matilda2 50 300 80");
        dot_vspawn("vspawn h39 -50 320 80");
        dot_vspawn("vspawn h39 0 320 80");
        dot_vspawn("vspawn h39 50 320 80");
        dot_vspawn("vspawn hurri2c -50 520 80");
        dot_vspawn("vspawn hurri2c 0 520 80");
        dot_vspawn("vspawn hurri2c 50 520 80");
    }

	return DC_HANDLED;
}

//////////////////////////////////////////////////////////////////////
//	BenchVehicles
//////////////////////////////////////////////////////////////////////
static SINT32 dot_benchinfantry(const char*)
{
    bool valid = false;

#if RELEASE_CLIENT
    switch(OverrideSpawner)
    {
    case BENCHANTWERP_OVERRIDE:
    case BENCHRAMAGEN_OVERRIDE:
        {
            valid = true;
            break;
        }
    default:
        return DC_HANDLED;
    }
#elif defined(BETA_RELEASE)
    valid = true;
#endif

    if(Online)
    {
        uiChatPrintLine(".benchinfantry for offline use only");
        return DC_HANDLED;
    }

    if(!valid)
        return DC_HANDLED;

	dot_vspawn("vspawn ukrifle -num 50 -15 10 0");

	return DC_HANDLED;
}



#ifdef BETA_RELEASE

/************************************************************************************************
	Connection testing
*************************************************************************************************/

static int dot_conn(const char* line)
{
	if(!Online)
		return DC_HANDLED;

	char command[25+1];
	SINT32 type = -1;
	SINT32 error = 0;

	command[0] = 0;

	if(sscanf(line, "%*s %25s %d %d", command, &type, &error) < 1)
		return DC_HANDLED;

	ww2::HostState* state = PlayerClient()->GetHostStateByType(type);

	if(!strcasecmp(command, "report"))
	{
		uiChatPrintLine(L"%s index is %d", PlayerClient()->MapState()->DisplayName(), PlayerClient()->MapState()->Index());
		uiChatPrintLine(L"%s index is %d", PlayerClient()->ChatState()->DisplayName(), PlayerClient()->ChatState()->Index());
		uiChatPrintLine(L"%s index is %d", PlayerClient()->CellState()->DisplayName(), PlayerClient()->CellState()->Index());
	}
	else if(!state)
	{
		uiChatPrintLine("Invalid host state specified!");
		return DC_HANDLED;
	}
	else if(!strcasecmp(command, "lock"))
	{
		state->active = !state->active;

		if(state->active)
			uiChatPrintLine(L"%ls state is active!", state->DisplayName());
		else
			uiChatPrintLine(L"%ls state is NOT active!", state->DisplayName());
	}
	else if(!strcasecmp(command, "disco"))
	{
		uiChatPrintLine(L"Disconnecting %ls state with error code %d!", state->DisplayName(), error);
		state->Disconnect(error);
	}
	else if(!strcasecmp(command, "error") && state->connection)
	{
		uiChatPrintLine(L"Erroring %ls state with error code %d!", state->DisplayName(), error);
		((ClientHostConnection*)state->connection)->SetError(PlayerClient(), error);
	}
	else if(!strcasecmp(command, "close") && state->connection)
	{
		uiChatPrintLine(L"Closing %ls state!", state->DisplayName());
		teulClose(state->Index());
	}

    return DC_HANDLED;
}

/************************************************************************************************
	Toggle EZ Mode
*************************************************************************************************/

static int dot_ez(const char* line)
{
	if(sscanf(line, "%*s %u", &gEZMode) == 0)
		gEZMode = 0;

    uiChatPrintLine("EZ Mode level is now %u", gEZMode);

    return DC_HANDLED;
}

static void CollideDown(Vec3d pos, colliderQuery::CollisionList& collisions, const unsigned int bits)
{
    static colliderQuery collider;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        collider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
        dGeomID ray = dCreateRay(collider.space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        collider.addGeom(ray, 0, 0);
    }

    collisions.clear();

    pos = physicsGameToPhysical(pos);

    pos.z() += 0.1f;

    colliderQuery::dGeomList& geoms = collider.geoms();
    dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), 0, 0, -1);
    colliderSetBits(geoms[0], 0, bits);

    collider.setDirty(true);

    if(!collider.findCollisions(true))
    {
        uiChatPrintLine("No Collisions");
    }
    else
    {
        colliderQuery::CollisionList& c = collider.getCollisions();

        dGeomID current = nullptr;

        for(unsigned int i = 0; i < c.size(); i++)
        {
            if(current == c[i].g2)
                continue;

            current = c[i].g2;

			// don't use blocks
			if(colliderIsType(c[i].g2, COLLIDER_BLOCK))
				continue;

            if(colliderIsType(c[i].g2, bits))
            {
                collisions.push_back(c[i]);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
//	BenchVehicles
//////////////////////////////////////////////////////////////////////
static SINT32 dot_allinf(const char*)
{
    if(Online)
    {
        uiChatPrintLine(".allinf for offline use only");
        return DC_HANDLED;
    }

	int ccount = 4;

	const char* countries[] =
	{
		"uk",
		"fr",
		"de",
		"us"
	};

	int tcount = 10;

	const char* troops[] =
	{
		"rifle",
		"-smg",
		"-lmg",
		"-eng",
		"-sniper",
		"grenadier",
		"-atr",
		"-ltmortar",
		"-assault",		// US Only
		"-bar"			// US Only
	};

	const char* paras[] =
	{
		"-para-rfl",
		"-para-smg",
		"-para-lmg",
		"-para-eng",
		"-para-snpr",
		nullptr,
		"-para-atr",
		"-para-mrtr"
	};

	char buffer[256];

	SINT32 perRow = 6;
	SINT32 x = 0;
	SINT32 y = 0;
	SINT32 offset = 2;

	for(int c = 0; c < ccount; c++)
	{
		const char* country = countries[c];

		for(int t = 0; t < tcount; t++)
		{
			strprintf(buffer, "vspawn %s%s %d %d 0", country, troops[t], x, y);
			dot_vspawn(buffer);

			x += offset;

			if(x >= offset * perRow)
			{
				x = 0;
				y += offset;
			}

			// chute version
			if(paras[t])
			{
				strprintf(buffer, "vspawn %s%s %d %d 0", country, paras[t], x, y);
				dot_vspawn(buffer);
			}

			x += offset;

			if(x >= offset * perRow)
			{
				x = 0;
				y += offset;
			}

			// no chute version
			if(paras[t])
			{
				strprintf(buffer, "vspawn %s%s %d %d 0 altmodel", country, paras[t], x, y);
				dot_vspawn(buffer);
			}

			x += offset;

			if(x >= offset * perRow)
			{
				x = 0;
				y += offset;
			}
		}
	}

	return DC_HANDLED;
}

//////////////////////////////////////////////////////////////////////
//	decal - set your decal
//////////////////////////////////////////////////////////////////////
static SINT32 dot_decal(const char* line)
{
	UINT32 pDecal = 0;
	UINT32 uDecal = 0;

	int argc = sscanf(line, "%*s %u %u", &pDecal, &uDecal);

	if(argc > 0)
	{
		gPersDecalOverride = pDecal;

		if(argc > 1)
			gUnitDecalOverride = uDecal;
	}

	return DC_HANDLED;
}

//////////////////////////////////////////////////////////////////////
//	gr2debug
//////////////////////////////////////////////////////////////////////
static SINT32 dot_gr2debug(const char* line)
{
	const char* flags[] =
	{
		"model",
		"collider",
		"bodies",
		"joints",
		"bones",
		"fpscagecap",
		"fpscageray",
		"fpsfall",
	};

	char flag[80 + 1] = {0};

	int argc = sscanf(line, "%*s %80s", &flag[0]);

	if(argc)
	{
		uiChatPrintLine("Toggling flag %s", flag);

		for(int i = 0; i < 8; i++)
		{
			if(!strcasecmp(flags[i], flag))
			{
				UINT32 mask = (1<<i);

				if(gr2DrawFlags & mask)
					gr2DrawFlags &= ~mask;
				else
					gr2DrawFlags |= mask;

				break;
			}
		}
	}
	else
	{
		uiChatPrintLine("Toggling all flags");
		gr2DrawFlags = 1;
	}

	return DC_HANDLED;
}

#ifdef GR2_TEXTURE_DEBUG
static int dot_gr2textures(const char* line)
{
	FILE* fp = fopen("gr2textures.txt", "w");

	if(fp)
	{
		std::map<std::string,std::string>& textures = gr2GetTextures();
		std::map<std::string,std::string>::iterator it = textures.begin();

		for(; it != textures.end(); it++)
			fprintf(fp, "%s -> %s\n", it->first.c_str(), it->second.c_str());

		fclose(fp);
	}

    return DC_HANDLED;
}
#endif

//////////////////////////////////////////////////////////////////////
//	bunch
//////////////////////////////////////////////////////////////////////
static SINT32 dot_bunch(const char* line)
{
  if(Online)
  {
    uiChatPrintLine(".bunch for offline use only");
    return DC_HANDLED;
  }

  char t[132];
  int spread = 7;
	int argc = sscanf(line, "%*s %128s %d", t, &spread);

  char s[256];
  for (int i=0; i<10; i++ )
    for (int j=0; j<10; j++ )
    {
      strprintf(s, "vspawn %s %d %d %d", t, j*spread, -i*spread, 0 );
      dot_vspawn(s);
    }

  return DC_HANDLED;
}

/************************************************************************************************
 generate terrain for host for all supercells
 *************************************************************************************************/

#ifdef MAKE_HOST_TERRAIN

static SINT32 dot_hostterrain(const char* line)
{
    generateHostTerrain();

	return DC_HANDLED;
}
#endif

/************************************************************************************************
	key lookup dev command
*************************************************************************************************/

static SINT32 dot_imapkey(const char* line)
{
	ww2KeymapInit(0);

	char func[128+1] = "";
	int type = -1;
	int key = 0;

	int argc = sscanf(line, "%*s %128s %d %d", func, &type, &key);

	// set it if specified
	if(argc > 0)
	{
		unsigned int code	= ww2FunctionCode(func);

		if(code > 0)
		{
			vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

			const char* keyname = ww2imapGetFuncKeyname(-1, (PlayerVehicle) ? PlayerVehicle->VehicleType->VehicleTypeNum: -1, code, key, type);

			if(keyname && *keyname)
				uiChatPrintLine("Keyname at %d/%d is %s", key, type, keyname);
			else
				uiChatPrintLine("Keyname not found at %d/%d!", key, type);
		}
		else
		{
			uiChatPrintLine("Invalid function name!");
		}
	}
	else
	{
		uiChatPrintLine("You must specify an imap function to lookup!");
	}

	ww2KeymapKill();

	return DC_HANDLED;
}

static SINT32 dot_imapdump(const char* line) {
    imapDump();
    uiChatPrintLine("Look in log for results");
    return DC_HANDLED;
}

static SINT32 dot_imapinit(const char *line) {
    uiChatPrintLine("Not ready!");
    ww2imapKill();
    ww2imapInit();
    return DC_HANDLED;
}

/************************************************************************************************
	discohostCmd
*************************************************************************************************/

static SINT32 dot_discohost(const char* line)
{
	teulClose(PlayerClient()->CellConnection());
	return DC_HANDLED;
}

/************************************************************************************************
	discohostCmd
*************************************************************************************************/

static SINT32 dot_discochat(const char* line)
{
	teulClose(PlayerClient()->ChatConnection());
	return DC_HANDLED;
}

/************************************************************************************************
	DoOverCmd
*************************************************************************************************/

static SINT32 dot_doover(const wchar_t* line)
{
	int i, numtimes = 1;

	swscanf(line, L"%*ls %d", &numtimes);
	uiChatPrintLine(L"Executing: %ls %d time(s)", _LastCommand, numtimes);

	for(i = 0; i < numtimes; i++)
		uiDotCommand(_LastCommand);

	return DC_HANDLED;
}

/************************************************************************************************
	DamageCmd
*************************************************************************************************/

static SINT32 dot_damage(const char* line)
{
	vehVehicle* veh = persGetPersonaVehiclePtr();
	if(!veh) return DC_HANDLED;

	char compname[256] = "";
	FLOAT32 damage;
	UINT32 compnum;
	int j, argc;
	mlComponentListIterator it;

	argc = sscanf(line,"%*s %255s %f", compname, &damage);

	if(argc >= 2)
	{
		vehFindHitmapObjectNumByName(&compnum, (UCHAR*)compname, veh->VehicleType->VehicleTypeNum);

		if(compnum == 0)
		{
			uiChatPrintLine("There is no damage component with that name");
		}
		else
		{
			vehApplyDamageToVehicleComponent(veh, compnum, damage,  0);
			uiChatPrintLine("Applying %f J of damage to hitmap component %s", damage, compname);

			mlComponentListBegin(veh->ModelData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				for(j = 0; j < 4; j++)
				{
					if(it.comp->DamageComponentNum[j] == compnum)
					{
						uiChatPrintLine("Applying %f J of damage to component %s", damage, it.comp->ComponentName);
						break;
					}
				}
			}
		}
	}
	return DC_HANDLED;
}

/************************************************************************************************
	SpallCmd
*************************************************************************************************/

static SINT32 dot_spall(const char* line)
{
	SpallEnabled = !SpallEnabled;
	uiChatPrintLine("Spall %s", SpallEnabled ? "on" : "off");
	return DC_HANDLED;
}

/************************************************************************************************
	ConcussionCmd
*************************************************************************************************/

static SINT32 dot_concuss(const char* line)
{
	ConcussionEnabled = !ConcussionEnabled;
	uiChatPrintLine("Concussion %s", ConcussionEnabled ? "on" : "off");
	return DC_HANDLED;
}

/************************************************************************************************
	ShrapnelCmd
*************************************************************************************************/

static SINT32 dot_shrap(const char* line)
{
	ShrapnelEnabled = !ShrapnelEnabled;
	uiChatPrintLine("Shrapnel %s", ShrapnelEnabled ? "on" : "off");
	return DC_HANDLED;
}

/************************************************************************************************
	LogCmd
*************************************************************************************************/

static SINT32 dot_log(const char* line)
{
	char s1[256] ="", s2[256] = "";

	int argc = sscanf(line,"%*s %255s %255s", s1, s2);
    
    if(argc == -1) {
        uiChatPrintLine(".log query|level [fatal|abort|recover|warn|minor|note|info|debug|none]|system|all off");
    }
	if(argc >= 1 && !strcmp(s1, "query")) // query
	{
		UINT32 logmask = Get_Log_Mask();
		int i;

		for(i = 0; i < 31; i++)
		{
			const char* name = Log_System_Name(1 << i);

			if(name)
				uiChatPrintLine("%s %s", name, ((logmask >> i) & 0x1) ? "on" : "off");
			else
				break;
		}
	} else if(argc >= 2 && !strcmp(s1, "level")) // level
    {
		int level = LOG_NONE;

		if(!strcmp(s2, "fatal")) level = LOG_FATAL;
        else if(!strcmp(s2, "abort")) level = LOG_ABORT;
        else if(!strcmp(s2, "recover")) level = LOG_RECOVER;
        else if(!strcmp(s2, "warn")) level = LOG_WARN;
        else if(!strcmp(s2, "minor")) level = LOG_MINOR;
        else if(!strcmp(s2, "note")) level = LOG_NOTE;
        else if(!strcmp(s2, "info")) level = LOG_INFO;
        else if(!strcmp(s2, "debug")) level = LOG_DEBUG;
        else if(!strcmp(s2, "none")) level = LOG_NONE;

		Set_Log_Level(level);
    } else if(argc >= 1 && !strcmp(s1, "duplicates")) // level
    {
        gLogDuplicates =  !gLogDuplicates;
        uiChatPrintLine("logging duplicates %s", gLogDuplicates ? "on" : "off");
    } else if(argc >= 1) // on/off
	{
		UINT32 logmask = 0;
		LOGICAL enable = !strcmp(s2, "on"); // on/off

		logmask = Log_System_Mask(s1);
		if(logmask)
		{
			Enable_Log(logmask,enable);
			uiChatPrintLine("%s logging %s", s1, enable ? "on" : "off");
		}else
		{
			uiChatPrintLine("logging subsystem not recognized");
		}
	}
	return DC_HANDLED;
}

/************************************************************************************************
	VolCmd
*************************************************************************************************/

static SINT32 dot_vol(const char* line)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	FLOAT64 totalBVol = 0, totalExactVol = 0, freeFloodVol = 0;
	mlComponentListIterator it;

	if(VehicleData)
	{
		mlComponentListBegin(VehicleData->ComponentList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			mlComponent* comp = it.comp;

			uiChatPrintLine("%s %.0f m^3 %s", comp->ComponentName, (FLOAT32)comp->Vol,
							comp->IsFreeFlooding || comp->IsInternalMass ? "Free Flood or Internal" : "");

			if (comp->IsFreeFlooding)
				freeFloodVol += comp->Vol;

			if(!comp->IsInternalMass)
				totalExactVol += comp->Vol;
		}

		if(VehicleData->BuoyancyData)
		{
			mlBuoyancyData* bdata = VehicleData->BuoyancyData;

			while(bdata)
			{
				totalBVol += (bdata->top - bdata->bottom) * bdata->width * bdata->len;
				bdata = bdata->next;
			}
		}

		uiChatPrintLine("Total Non internal Volume: %.0f m^3", (FLOAT32)totalExactVol);
		uiChatPrintLine("Total Buoyancy Model Volume: %.0f m^3", (FLOAT32)totalBVol);
		uiChatPrintLine("Total Non Free Flood/Non internal Volume: %.0f m^3", (FLOAT32)totalExactVol - freeFloodVol);
		uiChatPrintLine("Total Buoyancy - Free Flood Volume: %.0f m^3", (FLOAT32)(totalBVol - freeFloodVol));
	}
	return DC_HANDLED;
}


/************************************************************************************************
	MassCmd
*************************************************************************************************/

static SINT32 dot_mass(const char* line)
{
/*	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(!VehicleData) return DC_ERROR;

	FLOAT64 totalMass = 0;
	int argc = 0;
	char compname[256] = "";
	FLOAT32 mass = 0;
	mlComponentListIterator it;

	argc = sscanf(line, "%*s %255s %f", compname, &mass);

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		mlComponent* comp = it.comp;

		if(mass > 0 && !strcmp((char*)comp->ComponentName, compname))
		{
			comp->Mass = mass;
			ptkBodyChangeMass(comp->Body, mass);
		}

		if(argc == 0 || !strcmp((char*)comp->ComponentName, compname))
		{
			uiChatPrintLine("%s %.0f kg %s", comp->ComponentName, (FLOAT32)comp->Mass,
								comp->IsInternalMass ? "Internal" : "");
		}

		totalMass += comp->Mass;
	}

	if(argc == 0)
		uiChatPrintLine("Total mass %.0f kg", (FLOAT32)totalMass);
*/
	uiChatPrintLine("unimplemented fix me");
	return DC_HANDLED;
}

/************************************************************************************************
	LeakCmd
*************************************************************************************************/

static SINT32 dot_leak(const char* line)
{
	if(!VehicleObjectAnalysis)
	{
		VehicleObjectAnalysis = 1;
		uiChatPrintLine("Gizmos and Whizbangs on");
	}

	LeakGridInc = 10; LeakAngleInc = 10; LeakYawStart = -180; LeakYawEnd = 180; LeakPitchStart = -90; LeakPitchEnd = 90;
	sscanf(line,"%*s %d %d %d %d %d %d", &LeakGridInc, &LeakAngleInc, &LeakYawStart, &LeakYawEnd, &LeakPitchStart, &LeakPitchEnd);
	DoLeakTest = TRUE;
	NumLeaks = 0;
	CurLeakYaw = LeakYawStart;
	CurLeakPitch = LeakPitchStart;
	uiChatPrintLine("Starting Leak Test");

	return DC_HANDLED;
}

/************************************************************************************************
	WindTunnelCmd
*************************************************************************************************/

static SINT32 dot_wttest(const char* line)
{
	LOGICAL DoStats, DoHp, DoStall, DoTopSpeed,
			DoClimb, DoRollRate, DoClCdCurves, DoNewThrustFile;
	char argv[8][16];
	int argc = sscanf(line, "%*s %15s %15s %15s %15s %15s %15s %15s %15s",
						argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7]);

	DoStats = DoHp = DoStall = DoTopSpeed =
	DoClimb = DoRollRate = DoClCdCurves = (argc == 0) ? 1 : 0;
	DoNewThrustFile = 0;

	if(argc > 0)
	{
		int i;
		for(i = 0; i < argc; i++)
		{
			if		(!strcmp(argv[i], "stats")) {
				DoStats = 1;
			}else if(!strcmp(argv[i], "hp")) 	{
				DoHp = 1;
			}else if(!strcmp(argv[i], "stall")) {
				DoStall = 1;
			}else if(!strcmp(argv[i], "top")) 	{
				DoTopSpeed = 1;
			}else if(!strcmp(argv[i], "climb")) {
				DoClimb = 1;
			}else if(!strcmp(argv[i], "roll")) 	{
				DoRollRate = 1;
			}else if(!strcmp(argv[i], "clcd")) 	{
				DoClCdCurves = 1;
			}else if(!strcmp(argv[i], "thrust")){
				DoNewThrustFile = 1;
			}
		}
	}

	mlRunWindTunnel(DoStats, DoHp, DoStall, DoTopSpeed,
					DoClimb, DoRollRate, DoClCdCurves, DoNewThrustFile);
	uiChatPrintLine("Finished Wind Tunnel");
	return DC_HANDLED;
}

/************************************************************************************************
	HitmapCmd
*************************************************************************************************/

static SINT32 dot_hitmap(const char* line)
{
	char vehname[256] = "", valid[32] = "";
	int i, all, validhit,validcmp;

	int argc = sscanf(line, "%*s %200s %31s", vehname, valid);
	if(argc < 1) return DC_ERROR;

	all = !strcmp(vehname, "all");
	validhit = !strcmp(valid, "validhit");
	validcmp = !strcmp(valid, "validcmp");

	for(i = 0; i < (int)vehtypeCount(); i++)
	{
		if(all || !strcmp((char*)vehtypeGetByIndex(i)->Id, vehname))
		{
			if(validhit)
				_ValidateVehHit(vehtypeGetByIndex(i));
			else if(validcmp)
				_ValidateVehCmp(vehtypeGetByIndex(i));
			else
				_PrintVehHitmap(vehtypeGetByIndex(i));

			if(!all)
				break;
		}
	}

	return DC_HANDLED;
}

/************************************************************************************************
	MoreAmmoCmd
*************************************************************************************************/

static SINT32 dot_moreammo(const char* line)
{
	vehVehicle* veh = persGetPersonaVehiclePtr();
	if(!veh) return DC_ERROR;

	SINT32 numRounds = 999;
	UINT32 Position = persGetPersonaPosition();

	sscanf(line, "%*s %d", &numRounds);

	// Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse]
	uiChatPrintLine("Not implemented yet");

	return DC_HANDLED;
}

/************************************************************************************************
	CommentCmd
*************************************************************************************************/

static SINT32 dot_c(const char* line)
{
	char cmd[256] = "";
	sscanf(line, "%255s", cmd);
	Write_Log(LOG_ALL, LOG_INFO, "COMMENT:%s", line + strlen(cmd));

	return DC_HANDLED;
}

/************************************************************************************************
	InfColTest
*************************************************************************************************/

static SINT32 dot_infcoltest(const char* line)
{
	gr2ToggleColTest();

	return DC_HANDLED;
}

/************************************************************************************************
	InfStateTest
*************************************************************************************************/

static SINT32 dot_infstatetest(const char* line)
{
	gr2ToggleStateTest();

	return DC_HANDLED;
}

/************************************************************************************************
	InfStateTest
*************************************************************************************************/

static SINT32 dot_infanimtest(const char* line)
{
	gr2ToggleAnimOutput();

	return DC_HANDLED;
}

/************************************************************************************************
	InfAutoReload
*************************************************************************************************/

static SINT32 dot_infautoreload(const char* line)
{
	fpFPSToggleAutoReload();

	return DC_HANDLED;
}


/************************************************************************************************
	InertiaCmd
*************************************************************************************************/

static void _CalcInertia(OBJECT* obj, FILE* f, LOGICAL print, FLOAT64 I[3][3], FLOAT64VECTOR* cm, FLOAT64* vol);

static SINT32 dot_inertia(const char* line)
{
	int argc;
	char vehname[256] = "\0";
	int i;

	argc = sscanf(line, "%*s %255s", vehname);
	if(argc < 1) return DC_ERROR;

	for(i = 0; i < (int)vehtypeCount(); i++)
	{
		if(!strcmp((char*)vehtypeGetByIndex(i)->Id, vehname))
		{
			OBJECT* PlaneObject = nullptr, *massobj, *obj;
			FILE* f;
			char filename[256] = "\0";

			PlaneObject = obmgrCreateModelInstance((char*)vehtypeGetByIndex(i)->Shape);
			if(!PlaneObject) return DC_ERROR;

			snprintf(filename, 255, "%s.mass.xml", (char*)vehtypeGetByIndex(i)->Id);

			f = fopen(filename, "w");
			fprintf(f, "<?xml version='1.0'?>\n"
						"<!DOCTYPE vehicle SYSTEM \"vehicle.dtd\">\n"
						"<masses>");

			if(f)
			{
				obj = nullptr;
				ogrpFindSubObjectByName(&massobj, PlaneObject,"lodc");
				if(massobj)
					ogrpFindSubObjectByName(&massobj, massobj,"mass");
				if(massobj)
					obj = massobj->child;

				while(obj)
				{
					_CalcInertia(obj, f, TRUE, nullptr, nullptr, nullptr);
					obj = obj->sibling;
				}
				fprintf(f, "\n</masses>\n");
				fclose(f);
			}

			ogrpFreeObjectTree(PlaneObject);
		}
	}
	return DC_HANDLED;
}

/************************************************************************************************
	ExhaustCmd
*************************************************************************************************/

static SINT32 dot_exhaust(const char* line)
{
    vehVehicle*pVeh = persGetPersonaVehiclePtr();
	if(pVeh == nullptr) return DC_ERROR;

    SCHAR szBuff[64];
    SINT32 i;
    LOGICAL bOn = FALSE;

    sscanf(line, "%*s%s", szBuff);

    bOn = !strcasecmp(szBuff, "on");

    for(i = 0; i < ML_MAX_ENGINES; i++)
    {
        if(pVeh->StateAnimation.ExhaustSmokeEmitter[i] != nullptr)
        {
            pVeh->StateAnimation.ExhaustSmokeOn[i] = bOn;
        }
    }

    vehfxCheckExhaustState(pVeh->iFXVehicleID);

    return DC_HANDLED;
}

/************************************************************************************************
	ResetCmd
*************************************************************************************************/

void trncdResetFXDamageFlags();

static SINT32 dot_reset(const char* line)
{
    trncdResetFXDamageFlags();

    return DC_HANDLED;
}

static SINT32 dot_treecull(const char* line)
{
    extern void sptToggleTreeCull();
    sptToggleTreeCull();

    return 0;
}

static SINT32 dot_addtree(const char* line)
{
    extern unsigned int treeDataGetBaseSize();

    int base = 0;
  int num = 1;
  int rad = 5;
    char basestr[16];

    int fields = sscanf(line, "%*s %s", basestr);

    if(fields)
    {
        base = atol(basestr);
    }

  const char* p = strstr(line, "-num");

  if(p)
  {
    sscanf(p, "%*s %d", &num);
  }

  p = strstr(line, "-rad");

  if(p)
  {
    sscanf(p, "%*s %d", &rad);
  }

    bool random = false;

    if(strstr(line, "-random"))
    {
        random = true;
    }

    unsigned int size = treeDataGetBaseSize();

    bool allofem = false;

    if(strstr(line, "-allofem"))
    {
        allofem = true;
        num = size;
    }

    piVECTOR pos;
    pos.set(-playpos.x, -playpos.y, -playpos.z);

    int col = (int)sqrt((float)num);
    int colcnt = 0;
    float radius = (float)rad;

    for(int i = 0; i < num; i++)
    {
      piVECTOR ps = pos;
/*
      float wigglex = (float)GetRandom(-radius/2.0f,radius/2.0f);
      float wiggley = (float)GetRandom(-radius/2.0f,radius/2.0f);
      ps.x += wigglex;
      ps.y += wiggley;
*/
        // Drops a tree of type 'base' at my position
        if ( allofem )
        {
            sptAddTreeCommand(i, ps);
            pos.x += 25;
        }
        else
        {
            sptAddTreeCommand(random ? GetRandom(0, size - 1) : base, ps);
            if(++colcnt < col)
            {
                pos.x += radius;
            }
            else
            {
                colcnt = 0;
                pos.x = -playpos.x;
                pos.y += radius;
            }
        }

    }

    return DC_HANDLED;
}

static int dot_grassshader(const char* line)
{
    g_grassShader = !g_grassShader;
    uiChatPrintLine("Grass shaders %s", g_grassShader ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static SINT32 dot_tree(const char* line)
{
    SCHAR* p = (SCHAR*)line;

    if(!p) return DC_ERROR;

    //"tree <0 - 5> "on/off"
    p += 5;

    if(!p) return DC_ERROR;

    SINT32 index;
    SCHAR onoff[16];

    sscanf(p, "%d %s", &index, onoff);

    sptDrawOn(index, !strcasecmp(onoff, "on"));

    return DC_HANDLED;
}

static SINT32 dot_die(const char* line)
{
    if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
    {
		if(SuperFloat)
			SuperFloat = FALSE;

		if(SuperTroop)
			fpFPSSetSuperTroop(FALSE);

        fpFPSKillTrooper();
    }
    else
    {
        persKillCrew();
    }

    //Add here if necessary
    return DC_HANDLED;
}

static SINT32 dot_texture(const char* line)
{
    static bool enabled = TRUE;

    enabled = !enabled;
    rnmainListText("Textures %s", enabled ? "ENABLED" : "DISABLED");

    if(!enabled)
    {
        piDisableTexture();
        piLockTextureState(true);
    }
    else
    {
        piLockTextureState(false);
        piEnableTexture();
    }

    return DC_HANDLED;
}

static SINT32 dot_hulkall(const char* line)
{
    std::set<int>::iterator itr = vehicles.begin();
    std::vector<int> removelist;

    for(; itr != vehicles.end(); ++itr)
    {
        int id = *itr;

        vehVehicle*veh = nullptr;
        vehGetVehicleHead(&veh);

        for(; veh != nullptr; veh = veh->getNext())
        {
            if(veh->VehicleNum == -id)
            {
                break;
            }
        }

        if(!veh)
        {
            rnmainListText("Failed find vehicle ID %d to HULK", id);
            continue;
        }

		veh->Flags |= VEH_FLAGS_UPDATESYSTEM_CONTROLLED;

		vehVisDamageHulkVehicle(veh, TRUE);

		for (UINT32 i=0; i< VEH_MAX_STATE_BITS; i++)
		{
			if(veh->VehicleType->StateType[i].StateType == VehAnim::STATE_TYPE_VISDAMAGE)
				vehAnimChangeVehicleStateByName(veh, (const UCHAR* )veh->VehicleType->StateType[i].Name, TRUE);
		}

		vehAnimUpdateVehicleStateAnimations(veh);

        // Create hulk vehicle
		/*
        vehVehiclePositionData* pos = &veh->PosData;

        AttitudeVector attitude((FLOAT32)pos->Att.Roll,
                                (FLOAT32)pos->Att.Pitch,
                                (FLOAT32)pos->Att.Yaw);
        WorldPosition_m position(pos->Pos);

        HulkSTO* hulk = new HulkSTO(0, veh->VehicleType, position, attitude);

        ServerTrackedObject::addToIndex(hulk);

        vehicles.insert((int)hulk->uid());

        // Remove original vehicle
        removelist.push_back(id);
		*/
    }

    for(unsigned int i = 0; i < removelist.size(); i++)
    {
        _removevehicle(removelist[i]);
    }

    return DC_HANDLED;
}

#include "veh/vehanim.h"

static SINT32 dot_hulk(const char* line)
{
    //hulk id
    char* p = (char*)line;
    p += 5;

    int id = atol(p);

    vehVehicle*veh = nullptr;
    vehGetVehicleHead(&veh);

    for(; veh != nullptr; veh = veh->getNext())
    {
        if(veh->VehicleNum == -id)
        {
            break;
        }
    }

    if(!veh)
    {
        rnmainListText("Failed find vehicle ID %d to HULK", id);
        return DC_HANDLED;
    }

    // Create hulk vehicle
    vehVehiclePositionData* pos = &veh->PosData;

    AttitudeVector attitude((FLOAT32)pos->Att.Roll,
                            (FLOAT32)pos->Att.Pitch,
                            (FLOAT32)pos->Att.Yaw);
    WorldPosition_m position(pos->Pos);

    HulkSTO* hulk = new HulkSTO(0,
                                veh->VehicleType,
                                position,
                                attitude);

    ServerTrackedObject::addToIndex(hulk);

    vehicles.insert((int)hulk->uid());

	// set all vis damage states
	for (int i=0;i<VEH_MAX_STATE_BITS;i++)
	{
		switch(veh->VehicleType->StateType[i].StateType)
		{
			case VehAnim::STATE_TYPE_VISDAMAGE:
				vehAnimChangeVehicleStateByNumber(veh, i, TRUE);
			break;
		}
	}
	vehAnimUpdateVehicleStateAnimations(veh);

    // Remove original vehicle
    _removevehicle(id);

    return DC_HANDLED;
}

static SINT32 dot_fpsanimspeed(const char* line)
{
	float speed;

	if(sscanf(line, "fpsanimspeed %f", &speed) == 1 && speed > 0)
		fpFPSSetAnimationSpeed(speed);

	// print out current speed
	rnmainListText("FPS Animation Speed: %f", fpFPSGetAnimationSpeed());

    return DC_HANDLED;
}

static SINT32 dot_tutorial(const char* line)
{
	char path[255+1]	= "";
	char id[25+1]		= "";

	int argc = sscanf(line,"%*s %25s %255s", id, path);

	if(argc == 2)
	{
		TUT_MGR->Start(id, path);
	}
	else
	{
		argc = sscanf(line,"%*s %255s", path);

		if(argc == 1)
		{
			std::string fullPath = "tutorials/";
			fullPath += path;
			fullPath += ".lua";

			TUT_MGR->Start(fullPath.c_str(), fullPath.c_str());
		}
	}

	return DC_HANDLED;
}

static int dot_lua(const char* line)
{
	char name[256] = "";
	int argc = sscanf(line,"%*s %255s", name);

	if(argc == 1)
	{
		if(TUT_MGR->Start(name))
			uiChatPrintLine("Script succesfuly run.");
		else
			uiChatPrintLine("Unable to run script.");
	}
	else
	{
		if(TUT_ACTIVE && TUT_MGR->Stop(false))
			uiChatPrintLine("Stopped tutorial!");
	}

	return DC_HANDLED;
}

static int dot_mappos(const char* line)
{
	char name[256] = "";
	int argc = sscanf(line,"%*s %255s", name);

	if(argc == 1)
	{
		SuperCellMapView* mapview = uiMapFind(name);

		if(mapview)
		{
			float x, y;
			x = y = 0;

			mapview->PositionXY(&x, &y);

			uiChatPrintLine("Map %s is at position %.2f, %.2f with scale %.2f", name, x, y, mapview->Scale());
		}
		else
		{
			uiChatPrintLine("Unable to find map %s", name);
		}
	}
	else
	{
		uiChatPrintLine("You must specify the name of the map you want to get the position of.");
	}

	return DC_HANDLED;
}

static int dot_whereami(const char* line)
{
	vehVehicle* veh = persGetPersonaVehiclePtr();

	if(veh)
	{
		FLOAT64 fyaw = veh->PosData.Att.Yaw; // forwards
		FLOAT64 byaw = (fyaw < 0) ? fyaw + PI: fyaw - PI; // backwards
		FLOAT64 fdeg = mlRAD_TO_DEG(fyaw);
		FLOAT64 bdeg = mlRAD_TO_DEG(byaw);

		gr2NormalizeAngle(fdeg);
		gr2NormalizeAngle(bdeg);

		uiChatPrintLine("You are at %.2f %.2f %.2f",
							veh->PosData.Pos.x,
							veh->PosData.Pos.y,
							veh->PosData.Pos.z);
		uiChatPrintLine("You are facing front: %d (%.2f radians)   back: %d (%.2f radians)",
							(SINT32)fdeg, fyaw, (SINT32)bdeg, byaw);

		char options[25+1];
		sscanf(line, "%*s %25s", options);

		if(strcasecmp(options, "save") == 0)
		{
			// auto add this to our clipboard
			wchar_t clipboard[128+1];
			swprintf(clipboard, 128, L"%.2f %.2f %.2f", veh->PosData.Pos.x, veh->PosData.Pos.y, veh->PosData.Pos.z);
			piinfoClipboard(clipboard);
		}
		else if(strcasecmp(options, "saveall") == 0)
		{
			// auto add this to our clipboard
			wchar_t clipboard[128+1];
			swprintf(clipboard, 128, L"%.2f %.2f %.2f %.2f %.2f", veh->PosData.Pos.x, veh->PosData.Pos.y, veh->PosData.Pos.z, fyaw, byaw);
			piinfoClipboard(clipboard);
		}
	}

	return DC_HANDLED;
}

static int dot_setextcam(const char* line)
{
	FLOAT32 x, y, z;

	int argc = sscanf(line,"%*s %f %f %f", &x, &y, &z);

	if(argc == 3)
		persSetExternalCameraPos(x, y, z);

	return DC_HANDLED;
}

static int dot_setexttar(const char* line)
{
	FLOAT32 x, y, z, d;

	int argc = sscanf(line,"%*s %f %f %f %f", &x, &y, &z, &d);

	if(argc == 4)
		persSetExternalCameraTarget(x, y, z, d);

	return DC_HANDLED;
}

static SINT32 dot_movevehicle(const char* line)
{
    std::string str = line;

    UINT32 start = str.find("-id");

    if(start == std::string::npos)
    {
        return DC_HANDLED;
    }

    start += 4;
    UINT32 end = str.find(' ', start);

    if(end == std::string::npos)
    {
        return DC_HANDLED;
    }

    int id = atol(str.substr(start, end).c_str());

    std::set<int>::iterator find = vehicles.find(id);

    vehVehicle*veh = nullptr;

    if(find != vehicles.end())
    {
        ServerTrackedObject* sto = ServerTrackedObject::lookup(id);

        if(!sto)
        {
            return DC_HANDLED;
        }

        VehicleSTO* vsto = dynamic_cast<VehicleSTO*>(sto);

        veh = vehFindVehicle(vsto->getSTOVehicleID());
    }

    if(!veh)
    {
        return DC_HANDLED;
    }

    start = str.find("-x");

    if(start != std::string::npos)
    {
        start += 3;
        end = str.find(' ', start);

        double x = 0.0f;

        if(end != std::string::npos)
        {
            x = atof(str.substr(start, end).c_str());
        }
        else
        {
            x = atof(str.substr(start, str.length()).c_str());
        }

        veh->PosData.Pos.x += x;
    }

    start = str.find("-y");

    if(start != std::string::npos)
    {
        start += 3;
        end = str.find(' ', start);

        double y = 0.0f;

        if(end != std::string::npos)
        {
            y = atof(str.substr(start, end).c_str());
        }
        else
        {
            y = atof(str.substr(start, str.length()).c_str());
        }

        veh->PosData.Pos.y += y;
    }

    start = str.find("-z");

    if(start != std::string::npos)
    {
        start += 3;
        end = str.find(' ', start);

        double z = 0.0f;

        if(end != std::string::npos)
        {
            z = atof(str.substr(start, end).c_str());
        }
        else
        {
            z = atof(str.substr(start, str.length()).c_str());
        }

        veh->PosData.Pos.z += z;
    }

    return DC_HANDLED;
}

static int dot_shadowmap(const char* line)
{
    static unsigned int oldflags = 0;
    static int last = 0;

    char on[8];

    int fields = sscanf(line, "%*s %s", on);

    if(fields != 1)
    {
        uiChatPrintLine(".shadowmap <on/off>");
        return DC_HANDLED;
    }

    if(!strncmp(on, "on", 2))
    {
        // Same as last command
        if(last == 1)
            return DC_HANDLED;

        last = 1;

        if(oldflags == 0)
            oldflags = ShadowMapFlags;
        else
            ShadowMapFlags = oldflags;

        uiChatPrintLine("ShadowMaps ENABLED");
    }
    else
    {
        // Same as last command
        if(last == 2)
            return DC_HANDLED;

        last = 2;

        oldflags = ShadowMapFlags;
        ShadowMapFlags = 0;
        uiChatPrintLine("ShadowMaps DISABLED");
    }

    return DC_HANDLED;
}

static int dot_wantblood(const char* line)
{
    WantBloodEffects = !WantBloodEffects;
    uiChatPrintLine("Want blood effects: %s", WantBloodEffects ? "ENABLED" : "DISABLED");

    cfgPrefsSetGlobal(cfgWANT_BLOODEFFECTS, &WantBloodEffects, true);

    return DC_HANDLED;
}

bool myfunction (StratDB::StratObject* s1,StratDB::StratObject* s2) { return wcscmp(s1->DisplayName(),s2->DisplayName())<0; }

static int dot_gencpslist(const char* line)
{
    if(NoBlitzWorPreload)
    {
        uiChatPrintLine("Data must be preloaded. Run game without -nowor");
        return DC_HANDLED;
    }

    StratDB::StratObjList allcps;
    StratDB::AllCPsInWorld(allcps);
    sort(allcps.begin(), allcps.end(),myfunction);
    StratDB::StratObjList::iterator it = allcps.begin();

    while (it!= allcps.end())
    {
        StratDB::StratObject* so = *it;
        printf( "%s|%d|%d\n",so->Name(), so->PosX(), so->PosY());
        it++;
    }

    return DC_HANDLED;
}

static int dot_genscenariomap(const char* line)
{
#if platOS == OS_WIN32
    if(!uiInGame())
        return DC_HANDLED;

    char scenario[128];
    float resolution = 0.0f;

    int fields = sscanf(line, "%*s %s %f", scenario, &resolution);

    if(fields < 1)
        return DC_HANDLED;

    if(fields < 2)
        resolution = 400.0f;

    if(OverrideSpawner != NOSPAWN_OVERRIDE && OverrideSpawner != SCENARIO_OVERRIDE && OverrideSpawner != NO_OVERRIDE)
    {
        uiChatPrintLine("Need to be at .nospawn");
        return DC_HANDLED;
    }

    SCEN_MGR->PushGlobalFlag(ScenarioManager::GLOBAL_GENERATE_MAP);

    std::string cmd = ".scenario start ";
    cmd += scenario;

    uiDotCommand(cmd.c_str());

    cmd = ".scenario events";
    uiDotCommand(cmd.c_str());

    GenerateScenarioMap::GetInstance().Start(resolution);

    SCEN_MGR->PopGlobalFlag(ScenarioManager::GLOBAL_GENERATE_MAP);
#endif

    return DC_HANDLED;
}

UINT32 gValueInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gValue = gValue + .25f;
//    uiChatPrintLine("Set to: %.2f%%", gValue);
    
    return 0;
}

UINT32 gValueDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gValue = gValue - .25f;
    if (gValue <= 1)
        gValue = 1;
//    uiChatPrintLine("Set to: %.2f%%", gValue);
    
    return 0;
}

UINT32 gXValueInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gXValue = gXValue + .25f;
    uiChatPrintLine("X Set to: %.2f%%", gXValue);
    
    return 0;
}

UINT32 gXValueDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gXValue = gXValue - .25f;
    uiChatPrintLine("X Set to: %.2f%%", gXValue);
    
    return 0;
}

UINT32 gYValueInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gYValue = gYValue + .25f;
    uiChatPrintLine("Y Set to: %.2f%%", gYValue);
    
    return 0;
}

UINT32 gYValueDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gYValue = gYValue - .25f;
    uiChatPrintLine("Y Set to: %.2f%%", gYValue);
    
    return 0;
}

UINT32 gZValueInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gZValue = gZValue + .25f;
    uiChatPrintLine("Z Set to: %.2f%%", gZValue);
    
    return 0;
}

UINT32 gZValueDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gZValue = gZValue - .25f;
    uiChatPrintLine("Z Set to: %.2f%%", gZValue);
    
    return 0;
}

UINT32 gValueToggle(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gToggle = !gToggle;
    sptDrawOn(0, gToggle);
    
    return 0;
}

UINT32 gValueToggle1(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gToggle1 = !gToggle1;
    uiChatPrintLine("gToggle1");

    return 0;
}

UINT32 gValueToggle2(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gToggle2 = !gToggle2;
    uiChatPrintLine("gToggle2");

    return 0;
}

UINT32 gValueToggle3(SINT32 Param,UINT32 Id,UINT32 Detach)
{
    OBJECT*    pObjectData = persGetPersonaVehicleObjectDataPtr();
    if (!pObjectData)
        return 0;
    
    gToggle3 = !gToggle3;
    uiChatPrintLine("gToggle3");

    return 0;
}

static int dot_gv(const char * line) {
    FLOAT32 gv;
    int argc = sscanf(line, "%*s %f", &gv);
    rnmainListLocText(IDE_LANG_UI, 854);

    if(argc > 0)
        gValue = gv;
    else
        uiChatPrintLine("gValue: %d", gValue);
    
    return DC_HANDLED;
}

#ifdef SPATIAL_DB
extern bool SDB_DrawDebugFlag;

static int dot_drawsdb(const char* line)
{
    if(!uiInGame())
        return DC_HANDLED;

    SDB_DrawDebugFlag = ! SDB_DrawDebugFlag;

    uiChatPrintLine("SDB data drawing is %s", SDB_DrawDebugFlag ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}
#endif

#ifdef GENICONS

static int dot_genicons(const char* line)
{
  if(!uiInGame())
    return DC_HANDLED;

  if(OverrideSpawner != NOSPAWN_OVERRIDE)
  {
    uiChatPrintLine("Need to be at .nospawn");
    return DC_HANDLED;
  }

  if(NoBlitzWorPreload)
  {
    uiChatPrintLine("Data must be preloaded. Run game without -nowor");
    return DC_HANDLED;
  }

  if(GetIconGenerator().IsEnabled())
  {
    GetIconGenerator().End();
    uiChatPrintLine("Generate Icons DISABLED");
  }
  else
  {
    GetIconGenerator().Start(true);
    uiChatPrintLine("Generate Icons ENABLED");
  }

  return DC_HANDLED;
}

static int dot_gentiles(const char* line)
{
  if(!uiInGame())
    return DC_HANDLED;

  if(OverrideSpawner != NOSPAWN_OVERRIDE)
  {
    uiChatPrintLine("Need to be at .nospawn");
    return DC_HANDLED;
  }

  if(NoBlitzWorPreload)
  {
    uiChatPrintLine("Data must be preloaded. Run game without -nowor");
    return DC_HANDLED;
  }

  if(GetIconGenerator().IsEnabled())
  {
    GetIconGenerator().End();
    uiChatPrintLine("Generate Icons DISABLED");
  }
  else
  {
    GetIconGenerator().Start(false);
    uiChatPrintLine("Generate Icons ENABLED");
  }

  return DC_HANDLED;
}

#endif
static int dot_getcell(const char* line)
{
    int sx, sy, r, c;
    persGetMySupercellAndOctet(sx, sy, r, c);

    uiChatPrintLine("SuperCell: %d %d  Octet: %d %d", sx, sy, r, c);

    return DC_HANDLED;
}

static int dot_drawheightmap(const char* line)
{
    extern void obdrawDrawHeightMap(bool);

    static bool toggle = false;

    toggle = !toggle;
    obdrawDrawHeightMap(toggle);
    uiChatPrintLine("Draw Height Map: %s", toggle ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static int dot_duplicate(const char* line)
{
	vehVehicle*veh = persGetPersonaVehiclePtr();

    veh->VehicleHostIndex;

    vehVehicle* list = nullptr;

    vehGetVehicleHead(&list);

    uiChatPrintLine("Check for duplicate vehicle ids");
    bool found = false;

    for(; list != nullptr; list = list->getNext())
    {
        if(list->IsMe())
            continue;

        if(list->VehicleHostIndex == veh->VehicleHostIndex)
        {
            found = true;
            uiChatPrintLine("Duplicate VehicleHostIndex: %d User: %s IsHulked: %d IsSTO: %d InUse: %d Their Pos: %f %f %f MyPos: %f %f %f",
                list->VehicleHostIndex,
                list->Handle,
                list->isHulked(),
                list->isSTO(),
                list->InUse,
                list->PosData.Pos.x,
                list->PosData.Pos.y,
                list->PosData.Pos.z,
                veh->ModelData->VehiclePos.x,
                veh->ModelData->VehiclePos.y,
                veh->ModelData->VehiclePos.z);

            Enable_Log(LOG_OLD, 1);
            Write_Debug("Duplicate VehicleHostIndex: %d User: %s IsHulked: %d IsSTO: %d InUse: %d Their Pos: %f %f %f MyPos: %f %f %f",
                list->VehicleHostIndex,
                list->Handle,
                list->isHulked(),
                list->isSTO(),
                list->InUse,
                list->PosData.Pos.x,
                list->PosData.Pos.y,
                list->PosData.Pos.z,
                veh->ModelData->VehiclePos.x,
                veh->ModelData->VehiclePos.y,
                veh->ModelData->VehiclePos.z);
            Enable_Log(LOG_OLD, 0);
        }
    }

    if(!found)
        uiChatPrintLine("No duplicates found");

    return DC_HANDLED;
}

static int dot_ppo(const char* line)
{
	int id = -1;
	sscanf(line, "%*s %d", &id);

	fpPPOSet(fpPPOCreate(id));

	return DC_HANDLED;
}

static int dot_bailout(const char* line)
{
	int country = -1;
	sscanf(line, "%*s %d", &country);

	if(country == -1)
		return DC_HANDLED;

	vehVehicle* me = persGetPersonaVehiclePtr();
	FLOAT64VECTOR& pos = me->ModelData->VehiclePos;
	FLOAT64 yaw = me->ModelData->VehicleAtt.Yaw;

	if(me->VehicleType->Type != VEH_TYPE_PERSON)
		yaw = -yaw;

    WorldPosition_m position(pos.x, pos.y, pos.z);
	AttitudeVector attitude(0, 0, (FLOAT32)yaw);

	const char* vid = nullptr;

	switch(country)
	{
		case WWII_ENGLAND:
			vid = "uk-para-rfl";
			break;
		case WWII_USA:
			vid = "us-para-rfl";
			break;
		case WWII_FRANCE:
			vid = "fr-para-rfl";
			break;
		case WWII_GERMANY:
			vid = "de-para-rfl";
			break;
	}

	if(vid == nullptr)
		return DC_HANDLED;

	const vehType* vt = vehTypeGetById(vid);

	if(!vt)
		return DC_HANDLED;

	BailoutSTO* sto = new BailoutSTO(0, vt, position, attitude);

    int id = ServerTrackedObject::addToIndex(sto);

	return DC_HANDLED;
}

static int dot_wtfamistandingon(const char* line)
{
    vehVehicle* me = playerGetVehicle();

    if(!me || !me->ModelData)
    {
        uiChatPrintLine("ERROR: User vehicle not initialized");
        return DC_HANDLED;
    }

    Vec3d pos(me->ModelData->VehiclePos);

    colliderQuery::CollisionList collisions;

    pos.z() += 0.1f;

    CollideDown(pos, collisions);

    if(collisions.empty())
    {
        return DC_HANDLED;
    }

    dGeomID current = nullptr;

    uiChatPrintLine("BEGIN");

    for(unsigned int i = 0; i < collisions.size(); i++)
    {
        struct colliderQuery::Collision& c = collisions[i];

        if(current == c.g2)
            continue;

        OBJECT* o = colliderData<OBJECT>(c.g2);

        if(o)
		{
			if(o->modelInstance)
			{
				if(colliderIsType(c.g2, COLLIDER_VEHICLE_MASK|COLLIDER_VEH_HULK))
				{
					vehVehicle* veh = colliderSpaceData<vehVehicle>(c.g2);

					uiChatPrintLine("Vehicle Model: %s%s Texture: %s ",
									o->modelInstance->GetModelDirectory(),
									o->modelInstance->GetModelName(),
									o->texname ? o->texname : "No Texture");

					if(veh)
					{
						double distance = me->DistanceTo(veh->PosData.Pos.x, veh->PosData.Pos.y, veh->PosData.Pos.z);

						uiChatPrintLine("     Player: %s Index: %u Used: %s Type: %s Pos: %.2f,%.2f,%.2f Dist: %.2f",
									(const char*)veh->Handle,
									veh->VehicleHostIndex,
									(veh->InUse) ? "true": "false",
									(const char*)veh->VehicleType->Id,
									veh->PosData.Pos.x,
									veh->PosData.Pos.y,
									veh->PosData.Pos.z,
									distance);
					}
				}
				else
				{
					trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(c.g2);
					std::vector<std::string> cnames = colliderGetDescription(dGeomGetCategoryBits(c.g2));
					bool onMedium = false;
					bool onShort = false;
					bool hasBox = (TO && TO->physicsBoxCollider) ? true: false;

					for(int i = 0; i < ML_NUM_MED_LIST_ELEMENTS; i++)
					{
						if(!me->ModelData->MedTrnList[i])
							break;

						if(me->ModelData->MedTrnList[i] == TO)
						{
							onMedium = true;
							break;
						}
					}

					for(int i = 0; i < ML_NUM_SHORT_LIST_ELEMENTS; i++)
					{
						if(!me->ModelData->ShortTrnList[i])
							break;

						if(me->ModelData->ShortTrnList[i] == TO)
						{
							onShort = true;
							break;
						}
					}

					uiChatPrintLine("Terrain Model:%s%s Texture:%s PlaceMode:%d Height:%f Med:%s Short:%s Box:%s",
									o->modelInstance->GetModelDirectory(),
									o->modelInstance->GetModelName(),
									o->texname ? o->texname : "No Texture",
									o->modelInstance->GetPlaceMode(),
									c.pos[2],
									(onMedium) ? "y": "n",
									(onShort) ? "y": "n",
									(hasBox) ? "y": "n");

					if(!cnames.empty())
					{
						std::string cname = "          ODE Colliders: ";
						int count = cnames.size();

						for(int i = 0; i < count; i++)
						{
							cname += cnames[i];

							if(i < count - 1)
								cname += ", ";
						}

						uiChatPrintLine(cname.c_str());
					}
				}
			}
			else
			{
				uiChatPrintLine("%s Texture", o->texname ? o->texname : "No Texture");
			}
		}
		else
		{
			uiChatPrintLine("We got a hit, but no object data was found in the geom!");
		}

        current = c.g2;
    }

    uiChatPrintLine("END");

    return DC_HANDLED;
}

static int dot_buildterrain(const char* line)
{
#if platOS == OS_WIN32
	if(uiInGame())
	{
		rnmainListText("Terrain cannot be built after entering the game world!");
		return DC_HANDLED;
	}

	int cellX, cellY;

	sscanf(line, "%*s %d %d", &cellX, &cellY);
    g_buildTerrain = TRUE;
	BuildTerrain(false, cellX, cellY);
    g_buildTerrain = FALSE;
	uiChatPrintLine("SuperCell %d,%d has been built.", cellX, cellY);
#endif
    return DC_HANDLED;
}

static int dot_timemultiplier(const char* line)
{
    int multi = 0;
    int fields = sscanf(line, "%*s %d", &multi);

    if(fields != 1)
        return 0;

    trnTODSetTimeMultiplier((float)multi);

    uiChatPrintLine("Time multiplier is %d", multi);

    return DC_HANDLED;
}

static int dot_setfalls(const char* line)
{
    int ns = 0;
    int fields = sscanf(line, "%*s %d", &ns);

    if(fields != 1)
        return DC_HANDLED;

	fpTPSSetFallSounds(ns);

    uiChatPrintLine("Set fall sounds to %d", ns);

    return DC_HANDLED;
}

static int dot_setfm(const char* line)
{
    int start = -1;
	int stop = -1;
	int duration = -1;

    int fields = sscanf(line, "%*s %d %d, %d", &start, &stop, &duration);

    if(fields != 3)
	{
		uiChatPrintLine("Set friction model to %d %d %d", start, stop, duration);
        return DC_HANDLED;
	}

	fpTPSSetFrictionModel(start, stop, duration);

    uiChatPrintLine("Set friction model to %d %d %d", start, stop, duration);

    return DC_HANDLED;
}

/*
static int dot_freesound(const char* line)
{
	char path[128+1] = "";

	int argc = sscanf(line, "%*s %128s", path);

	if(!argc)
		return DC_HANDLED;

    snSound* s = FindNamedSound(path);

    if(!s)
    {
		uiChatPrintLine("Sound %s not found!", path);
        return DC_HANDLED;
    }

	if(sndsFreeSound(s))
		uiChatPrintLine("Sound %s freed!", path);

    return DC_HANDLED;
}
*/

static int dot_drawvbo(const char* line)
{
    static bool vbo = true;
    extern void oglMeshToggleDraw(bool);
    extern void oglVertexBufferToggleDraw(bool);

    vbo = !vbo;
    oglMeshToggleDraw(vbo);
    oglVertexBufferToggleDraw(vbo);

    uiChatPrintLine("VBO drawing %s", vbo ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static int dot_gr2memstats(const char* line)
{
	gr2AnimMemStats();
    return DC_HANDLED;
}

static int dot_speedgrass(const char* line)
{
    static bool SpeedGrass = true;

    SpeedGrass = !SpeedGrass;
    speedgrassToggle(SpeedGrass);

    uiChatPrintLine("Speedgrass %s", SpeedGrass ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static int dot_mypos(const char* line)
{
    uiChatPrintLine("Player Position: x = %f y = %f z = %f", -playpos.x, -playpos.y, -playpos.z);
    return DC_HANDLED;
}

static int dot_noshaderreload(const char* line)
{
    extern void glslToggleShaderReload();

    glslToggleShaderReload();

    return DC_HANDLED;
}

static int dot_vehiclestate(const char* line)
{
    // Change state of your vehicle for testing effects.
    // Probably have to be in 'float' mode so the engine does not
    // reset these states. These states can be found in the .state
    // file for the particular vehicle.

    char state[32];
    int on = 0;

    int fields = sscanf(line, "%*s %s %d", state, &on);

    if(fields != 2)
    {
        return DC_HANDLED;
    }

    vehVehicle* v = persGetPersonaVehiclePtr();

    if(v)
    {
        vehAnimChangeVehicleStateByName(v, (unsigned char*)state, on);
        uiChatPrintLine("STATE %s set to %d", state, on);
    }
    else
    {
        uiChatPrintLine("STATE %s FAILED", state);
    }

    return DC_HANDLED;
}

static int dot_damagevehicle(const char* line)
{
    char state[32];
    int id = 0;
    int on = 0;

    int fields = sscanf(line, "%*s %d %s %d", &id, state, &on);

    if(fields != 3)
    {
        return DC_HANDLED;
    }

    if(vehicles.find(id) == vehicles.end())
    {
        return DC_HANDLED;
    }

    ServerTrackedObject* sto = ServerTrackedObject::lookup(id);

    if(!sto) return DC_HANDLED;

    VehicleSTO* vsto = dynamic_cast<VehicleSTO*>(sto);

    if(!vsto) return DC_HANDLED;

    vehVehicle* veh = vehFindVehicle(vsto->getSTOVehicleID());

    if(!veh) return DC_HANDLED;

    veh->Flags |= VEH_FLAGS_UPDATESYSTEM_CONTROLLED;

    vehAnimChangeVehicleStateByName(veh, (unsigned char*)state, on);

    return DC_HANDLED;
}

// Spawn a building at 0,0,0
static OBJECT* buildingobj = nullptr;

static SINT32 dot_tspawn(const char* input)
{
    using omap = std::set<OBJECT*>;

	static omap instances;
    std::vector<omap::iterator> remove;

	int							modelType		= 0;
	static OBJECT*				masterInstance	= nullptr;
	PoolModel*					masterModel		= nullptr;
	piMATRIX					transform		= IdentityMatrix;
	unsigned int				subModelIndex	= 0;

    extern void trnopSetSC(int);

    if(!input || *input == 0)
        return DC_HANDLED;

	if(!instances.empty())
	{
        omap::iterator itr = instances.begin();

	    for(; itr != instances.end(); ++itr)
	    {
	        trncdRemoveTerrainObject(*itr, 0, 0, 0, nullptr);
	        obpoolFreeObjectCallback(*itr);
	    }
		instances.clear();
        buildingobj = nullptr;
		trndtl[12].terrain->flag2 &= ~OBJECT_VISIBLE;
	}

    piVECTOR rotxyz;
    rotxyz.set(0, 0, 0);
    int fields = 0;

    // -rot <x> or -rot <x y> or -rot <x y z> in degrees
    char* p = strstr((char*)input, "-rot");

    if(p)
    {
        fields = sscanf(p, "%*s %f %f %f", &rotxyz.x, &rotxyz.y, &rotxyz.z);

        rotxyz.x = DEG_TO_RAD(rotxyz.x);
        rotxyz.y = DEG_TO_RAD(rotxyz.y);
        rotxyz.z = DEG_TO_RAD(rotxyz.z);

	    math3dMatrixRotate(&transform, &rotxyz);
    }

    p = strstr((char*)input, "-pos");

	// reset this
    posxyz.set(0, 0, 0);

    if(p)
    {
        fields = sscanf(p, "%*s %f %f %f", &posxyz.x, &posxyz.y, &posxyz.z);
    }

    transform._41 = posxyz.x;
    transform._42 = posxyz.y;
    transform._43 = posxyz.z;

    char model[128] = {0};

    if(!sscanf(input, "%*s %127s", model))
    {
        return DC_HANDLED;
    }

    Write_Debug("[%s] x = %f y = %f z = %f", model, posxyz.x, posxyz.y, posxyz.z);

    std::string topmodel = model;

	masterModel = obmgrGetPoolModel(model);
	if(!masterModel)
	{
		rnmainListText("%s not found", model);
		return DC_HANDLED;
	}

    if(masterModel->modelType == ModelTypes::TERRAIN_TILE)
    {
        // Z fighting
        transform._43 -= 0.1f;
#ifdef GENICONS
        transform._43 -= 1.0f;
#endif
    }

    int sx, sy, r, c;
    persGetMySupercellAndOctet(sx, sy, r, c);
    int sc = 12;

    for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
    {
        if(!activeTerrainDetail[i])
            continue;

        if(trndtl[i].terrain == nullptr)
            continue;

        if(trndtl[i].scx == sx && trndtl[i].scy == sy)
        {
            sc = i;
            break;
        }
    };

	while(true)
	{
		OBJECT* obj = obmgrCreateModelInstance(model, transform, true);

        if(!obj)
        {
			rnmainListText("%s not found", model);
			return DC_HANDLED;
		}

		buildingobj = obj;

        if(subModelIndex == 0)
            masterInstance = buildingobj;
        else
        {
            strprintf(obj->name, "%s_%d", model, subModelIndex);
        }

        if(!buildingobj)
		{
			rnmainListText("%s not found", model);
			return DC_HANDLED;
		}

        trndtl[sc].terrain->flags |= OBJECT_VISIBLE;
        trnopSetSC(sc);

        if(masterModel->IsStaticTerrainModel())
	    {
		    obmgrHandleSpecialObjects(buildingobj);

            if(masterModel->hastrees)
                trnopHandleSpeedTree(buildingobj->child);
        }
        else
	    {
		    // translate the object to its new position
            posxyz.x += (float)WorldOriginOffset.x;
            posxyz.y += (float)WorldOriginOffset.y;

            buildingobj->trnum = sc;

		    if(!trnopOrientSubobjects(buildingobj, &posxyz, true))
		    {
			    obpoolFreeObjectCallback(buildingobj);
			    return DC_HANDLED;
		    }

            posxyz.x -= (float)WorldOriginOffset.x;
            posxyz.y -= (float)WorldOriginOffset.y;
	    }

		trncdAddTerrainObject(buildingobj, 0, 0, 0, nullptr);

		OBJECT* o = buildingobj->child;

		trnCDTerrainObject* to = trncdFindTerrainObject(o);

		obmainSetFlag2InObjectTree(buildingobj, OBJECT_TERRAIN_OBJECT);

		rnmainListText("spawned %s", buildingobj->modelInstance->GetModelName());

        instances.insert(buildingobj);

		if(masterModel->subModelReferences.size() <= subModelIndex)	break;
		strcpy(model, masterModel->subModelReferences[subModelIndex].poolModel->modelname.c_str());
		transform = masterModel->subModelReferences[subModelIndex].transform;

		subModelIndex++;
	}

    // Force rebuild of inverse matrices.
	trnUpdateTerrainOriginOffset(TRUE);
    trnCDUpdateCollideLists(true);

    ShadowMap::GetInstance().Reset();

    // Need this for bdamage command
    buildingobj = obmainFindTopmostObject(topmodel.c_str());

    return DC_HANDLED;
}

#if platOS != OS_MACOS
static SINT32 dot_heightmap(const char* in)
{
    extern void ddpSaveImgAsBitmap(unsigned char* img, int w, int h, wchar_t* filename);

    char model[128] = {0};
    int res = 0;

    if(!sscanf(in, "%*s %127s %d", model, &res))
    {
        return DC_HANDLED;
    }

    OBJECT* o = obmainFindTopmostObject(model);

    if(!o)
    {
        uiChatPrintLine("%s not found. Was this tspawn'd first?", model);
        return DC_HANDLED;
    }

    float max_box_height = -o->modelInstance->GetBounds().zMin();

    if(res == 0)
        res = 800;

    unsigned char* img = (unsigned char*)memAlloc(res * res * 3);

    float step = SDM_OCTET_SIZE / res;

    double x = -SDM_OCTET_SIZE_2;
    double y = SDM_OCTET_SIZE_2;

    Vec3d pos = Vec3d(x, y, max_box_height + 1.0f);
    float heightscale = max_box_height / 255.0f;

    colliderQuery::CollisionList collisions;

    int idx = 0;

    for(int r = 0; r < res; r++)
    {
        for(int c = 0; c < res; c++)
        {
            CollideDown(pos, collisions, COLLIDER_TILE);

            if(!collisions.empty())
            {
                float h = (float)collisions[0].pos.z() / max_box_height;
                h *= 255.0f;

                unsigned char color = (unsigned char)h;

                img[idx++] = color;
                img[idx++] = color;
                img[idx++] = color;
            }

            pos.x() += step;
        }

        pos.y() -= step;
        pos.x() = x;
    }

    ddpSaveImgAsBitmap(img, res, res, L"heightmap.bmp");

    return DC_HANDLED;
}
#endif

#include "trn/trnScenario.h"
static int dot_enablemouseclicktodamagebuildings(const char* line)
{
#ifndef BETA_RELEASE
    uiChatPrintLine("Not a beta executable");
    return DC_HANDLED;
#endif

    if(Online)
    {
        uiChatPrintLine("Must be offline");
        return DC_HANDLED;
    }

    EnableMouseClickToDamageBuildings = !EnableMouseClickToDamageBuildings;
    uiChatPrintLine("Damage buildings with mouse click: %s", EnableMouseClickToDamageBuildings ? "ENABLED" : "DISALBED");

    return DC_HANDLED;
}

static int dot_alert(const char* line)
{
	char* alert = strstr((char*)line, " ");

	if(alert)
	{
		alert++;
		uiGameEvent(11, 1, UTF16(alert));
	}

	return DC_HANDLED;
}

static int dot_ortho(const char* line)
{
	static bool orthoOn = false;
	static Matrixf projection;
	static FLOAT32 zfar = 0;
    static FLOAT32 znear = 0;
	static UINT32 shadows = 0;

	if(orthoOn)
	{
		oglLockProjectionMatrix(FALSE);
		oglSetProjectionMatrix(projection);
		BackClip = zfar;
		FrontClip = znear;
		ShadowMapFlags = shadows;
		orthoOn = false;
	}
	else
	{
		vehVehicle* player = persGetPersonaVehiclePtr();
		if(!player) return DC_HANDLED;

		Vec3d down = Vec3d(0, 0, -1.0f);
		Vec3d pos = Vec3d(player->ModelData->VehiclePos);

		float z			= (float)pos.z();
		float top		= z;
		float bottom	= -z;
		float right		= z;
		float left		= -z;

		shadows = ShadowMapFlags;
		zfar = BackClip;
		znear = FrontClip;
		projection = *oglGetProjectionMatrix();

		FLOAT32 newZfar = z * 2.0f;
		FLOAT32 newZnear = 1.0f;

		ShadowMapFlags = 0;

		oglSetOrthoProjectionMatrix(right, left, top, bottom, newZnear, newZfar);
		oglLockProjectionMatrix(TRUE);

		orthoOn = true;
	}

    return DC_HANDLED;
}

static std::vector<TerrainSTO*> buildingSTO;

static SINT32 dot_benchbuilding(const char* line)
{
	char bld[64];

    if(!buildingSTO.empty())
    {
        for(unsigned int j = 0; j < buildingSTO.size(); j++)
        {
            TerrainSTO* sto = buildingSTO[j];
            sto->setExpired();
        }

        buildingSTO.clear();
    }

    int fields = sscanf(line, "%*s %s", bld);

    if(fields == 0)
    {
        return DC_HANDLED;
    }

	int num = 1;

	const char* c = strstr(line, "-num");

    if(c)
    {
        sscanf(c, "%*s %d", &num);
    }

	PoolModel* model = obmgrGetPoolModel(bld);

    AttitudeVector attitude;

    double radius = model->bounds.radius();
    int col = (int)sqrt((float)num);
    int colcnt = 0;

    double x = -playpos.x;
    double y = -playpos.y;
    double z = 0.0;

    for(int i = 0; i < num; i++)
    {
        WorldPosition_m position(x, y, z);

	    TerrainSTO* sto = new TerrainSTO(bld, position, attitude);
	    ServerTrackedObject::addToIndex(sto);
        buildingSTO.push_back(sto);

        if(++colcnt < col)
        {
           x += radius;
        }
        else
        {
            colcnt = 0;
            x = -playpos.x;
            y += radius;
        }
    }

    trnCDUpdateCollideLists(true);

	return DC_HANDLED;
}

/*
static SINT32 dot_scspawn(const char* line)
{
    extern void DotCommandLoadSuperCell(int x, int y, bool release = false);

    int x, y;

    int fields = sscanf(line, "%*s %d %d", &x, &y);

    if(fields != 2)
    {
        DotCommandLoadSuperCell(0, 0, true);
        return DC_HANDLED;
    }

    DotCommandLoadSuperCell(x, y);

    return DC_HANDLED;
}
*/
// Damage all buildings in my octet
static SINT32 dot_damageoctet(const char* input)
{
    int sx, sy, r, c;

    if(!persGetMySupercellAndOctet(sx, sy, r, c))
        return DC_HANDLED;

    StratDB::StratObjList& list = StratDB::BuildingsInOctet(sx, sy, c, r);

    if(list.empty())
        return DC_HANDLED;

    StratDB::StratObjList::iterator itr = list.begin();

    for(; itr != list.end(); ++itr)
    {
        StratDB::StratObject* so = *itr;

        if(so->StratClass() == SDM_CLASS_BUILDING)
        {
            if(so->StratType() == SDM_BUILDING_GENERIC || so->StratType() == SDM_BUILDING_AUTOGUNNER)
            {
                char buff[128];

                strprintf(buff, "%s_%d_%d", so->Filename(), so->PosX(), so->PosY());

                OBJECT* top = obmainFindTopmostObject(buff);

                if(!top)
                    continue;

                OBJECT* o = top->child;

                while(o)
                {
                    trnCDTerrainObject* TO = trncdFindTerrainObject(o);

                    if(TO)
                    {
                        trnSetDamageLevel(TO, 1);
                    }

                    o = o->sibling;
                }

                char model[128];

                PoolModel* pm = obmgrGetPoolModel(top->modelInstance->GetModelName());

                for(unsigned int i = 0; i < pm->subModelReferences.size(); i++)
                {
                    strcpy(model, pm->subModelReferences[i].poolModel->modelname.c_str());

                    strprintf(buff, "%s_%d_%d", model, so->PosX(), so->PosY());

                    OBJECT* o = obmainFindTopmostObject(buff);

                    if(!o)
                        continue;

                    o = o->child;

                    while(o)
                    {
                        trnCDTerrainObject* TO = trncdFindTerrainObject(o);

                        if(TO)
                        {
                            trnSetDamageLevel(TO, 1);
                        }

                        o = o->sibling;
                    }
                }
            }
        }
    }

    return DC_HANDLED;
}

// Reset all damaged buildings to LOD1
static SINT32 dot_resetoctet(const char* input)
{
    int sx, sy, r, c;

    if(!persGetMySupercellAndOctet(sx, sy, r, c))
        return DC_HANDLED;

    StratDB::StratObjList& list = StratDB::BuildingsInOctet(sx, sy, c, r);

    if(list.empty())
        return DC_HANDLED;

    StratDB::StratObjList::iterator itr = list.begin();

    for(; itr != list.end(); ++itr)
    {
        StratDB::StratObject* so = *itr;

        if(so->StratClass() == SDM_CLASS_BUILDING)
        {
            if(so->StratType() == SDM_BUILDING_GENERIC || so->StratType() == SDM_BUILDING_AUTOGUNNER)
            {
                char buff[128];
                strprintf(buff, "%s_%d_%d", so->Filename(), so->PosX(), so->PosY());

                OBJECT* top = obmainFindTopmostObject(buff);

                if(!top)
                    continue;

                OBJECT* o = top->child;

                while(o)
                {
                    trnCDTerrainObject* TO = trncdFindTerrainObject(o);

                    if(TO)
                    {
                        trnSetDamageLevel(TO, 0);
                    }

                    o = o->sibling;
                }

                char model[128];

                PoolModel* pm = obmgrGetPoolModel(top->modelInstance->GetModelName());

                for(unsigned int i = 0; i < pm->subModelReferences.size(); i++)
                {
                    strcpy(model, pm->subModelReferences[i].poolModel->modelname.c_str());

                    strprintf(buff, "%s_%d_%d", model, so->PosX(), so->PosY());

                    OBJECT* o = obmainFindTopmostObject(buff);

                    if(!o)
                        continue;

                    o = o->child;

                    while(o)
                    {
                        trnCDTerrainObject* TO = trncdFindTerrainObject(o);

                        if(TO)
                        {
                            trnSetDamageLevel(TO, 0);
                        }

                        o = o->sibling;
                    }
                }
            }
        }
    }

    trncdResetFXDamageFlags();

    return DC_HANDLED;
}

static SINT32 dot_bdamage(const char* input)
{
    if(!buildingobj)
        return DC_HANDLED;

    int damage = 0;
    int fields = sscanf(input, "%*s %d", &damage);

    if(fields < 1)
        return DC_HANDLED;

    int submodel = -1;
    OBJECT* masterInstance = buildingobj;
    OBJECT* o = buildingobj->child;

    while(o)
    {
        trnCDTerrainObject* TO = trncdFindTerrainObject(o);

        if(TO)
        {
            trnSetDamageLevel(TO, damage);
        }

        o = o->sibling;
    }

    char model[128];

    PoolModel* pm = obmgrGetPoolModel(buildingobj->modelInstance->GetModelName());

    for(unsigned int i = 0; i < pm->subModelReferences.size(); i++)
    {
        strprintf(model, "%s_%d", pm->subModelReferences[i].poolModel->modelname.c_str(), i + 1);

        OBJECT* o = obmainFindTopmostObject(model);

        if(!o)
            continue;

        o = o->child;

        while(o)
        {
            trnCDTerrainObject* TO = trncdFindTerrainObject(o);

            if(TO)
            {
                trnSetDamageLevel(TO, damage);
            }

            o = o->sibling;
        }
    }

    if(damage == 0)
    {
        dot_reset(nullptr);
    }

    rnmainListText("damage at %d", damage);

    return DC_HANDLED;
}

// Display builder plaque
static SINT32 dot_builder(const char* line)
{
    SCHAR* p = (SCHAR*)line;

    SINT32 len1 = strlen("builder");

    SINT32 len2 = strlen(line);

    // Format
    // .builder #

    // Skip builder
    p += len1 + 1;

    std::string func;

    SINT32 id = 0;

    if(p && len2 > len1)
    {
        id = atol(p);
    }

    if(!id)
    {
        func = "onbuilder()";
    }
    else
    {
		char buff[32]{};
        strprintf(buff, id < 10 ? "builder00%d" : id < 20 ? "builder0%d" : "builder%d", id);

        func = "onbuilder('";
        func += buff;
        func += "')";
    }

    uiGame()->RunScript(func.c_str());

    return DC_HANDLED;
}

static SINT32 dot_fogon(const char* line)
{
    static LOGICAL toggle = bFogEnabled;

    toggle = !toggle;

    bFogEnabled = toggle;

    piSetRenderState(piRENDERSTATE_FOGENABLE, toggle);

    uiChatPrintLine("fog %s", toggle ? "on" : "off");

    return DC_HANDLED;
}

static SINT32 dot_cull(const char* line)
{
    static bool toggle = true;

    toggle = !toggle;

    obcsDoCull(toggle);

    uiChatPrintLine("cull %s", toggle ? "on" : "off");

    return DC_HANDLED;
}

static SINT32 dot_clutter(const char* line)
{
    static bool toggle = true;

    toggle = !toggle;

    trnClutterSetDrawFlag(toggle);

    uiChatPrintLine("clutter %s", toggle ? "on" : "off");

    return DC_HANDLED;
}

static SINT32 dot_drawunsorted(const char* line)
{
    static bool draw = true;

    draw = !draw;
    obdrawToggleUnsortedDraw(draw);
    uiChatPrintLine("UNSORTED draw is %s", draw ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static SINT32 dot_drawsorted(const char* line)
{
    static bool draw = true;

    draw = !draw;
    obdrawToggleSortedDraw(draw);
    uiChatPrintLine("SORTED draw is %s", draw ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static SINT32 dot_drawmultitexture(const char* line)
{
    static bool draw = true;

    draw = !draw;
    obdrawToggleMultitextureDraw(draw);
    uiChatPrintLine("MULTITEXTURE draw is %s", draw ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static SINT32 dot_drawsortedcockpit(const char* line)
{
    static bool draw = true;

    draw = !draw;
    obdrawToggleSortedCockpitDraw(draw);
    uiChatPrintLine("SORTED COCKPIT draw is %s", draw ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static SINT32 dot_drawunsortedcockpit(const char* line)
{
    static bool draw = true;

    draw = !draw;
    obdrawToggleUnsortedCockpitDraw(draw);
    uiChatPrintLine("UNSORTED COCKPIT draw is %s", draw ? "ENABLED" : "DISABLED");

    return DC_HANDLED;
}

static SINT32 dot_terraindraw(const char* line)
{
    static bool toggle = true;

    toggle = !toggle;

    trnmainSetDraw(toggle);

    uiChatPrintLine("terrain %s", toggle ? "on" : "off");

    return DC_HANDLED;
}

static SINT32 dot_radclutter(const char* line)
{
    static bool toggle = false;
    static bool firsttime = true;

    if(firsttime)
    {
        toggle = g_bRadialClutter;
        firsttime = false;
    }

    toggle = !toggle;

    g_bRadialClutter = toggle;

    uiChatPrintLine("radial clutter %s", toggle ? "on" : "off");

    return DC_HANDLED;
}

static int dot_drawalloff(const char* line)
{
    dot_drawsorted(line);
    dot_drawunsorted(line);
    dot_drawmultitexture(line);
    dot_drawvbo(line);
    dot_speedgrass(line);
    dot_terraindraw(line);
    dot_drawsortedcockpit(line);
    dot_drawunsortedcockpit(line);

    dot_tree("tree 0 off");

    return DC_HANDLED;
}

static SINT32 dot_loadtrees(const char* line)
{
    static bool toggle = true;

    toggle = !toggle;

    trnopLoadTrees(toggle);

    uiChatPrintLine("loading trees %s", toggle ? "on" : "off");

    return DC_HANDLED;
}

static SINT32 dot_deploy(const char* line)
{
    vehmainDeployVehicleForMobileSpawn();

    return DC_HANDLED;
}

static SINT32 dot_undeploy(const char* line)
{
    vehmainUndeployVehicleForMobileSpawn();

    return DC_HANDLED;
}

/************************************************************************************************
	TreadDust
*************************************************************************************************/

static SINT32 dot_treaddust(const char* line)
{
	vehVehicle*pVeh = persGetPersonaVehiclePtr();
	SINT32 i;


    for(i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        if(pVeh->StateAnimation.TreadDustEmitter[i])
        {
            pVeh->StateAnimation.TreadDustOn[i] = !pVeh->StateAnimation.TreadDustOn[i];
        }
    }

    vehfxCheckTreadDustState(pVeh->iFXVehicleID);

    return DC_HANDLED;
}

/************************************************************************************************
	ForceCmd
*************************************************************************************************/

static SINT32 dot_force(const char* line)
{
    DisableForces = !DisableForces;
    uiChatPrintLine("Forces %s", DisableForces ? "disabled" : "enabled");

    return DC_HANDLED;
}

/************************************************************************************************
	SetPhysicsCmd
*************************************************************************************************/

static SINT32 dot_set(const char* line)
{
	vehVehicle*veh = persGetPersonaVehiclePtr();
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(veh == nullptr || VehicleData == nullptr) return DC_ERROR;

	char cmd[16];
	FLOAT32 x,y,z;
	sscanf(line, "%*s %15s %f %f %f", cmd, &x, &y, &z);

 	if(!strcmp(cmd, "vel"))
 	{
 		VehicleData->VehicleVel.x = x;
 		VehicleData->VehicleVel.y = y;
 		VehicleData->VehicleVel.z = z;
 	}else if(!strcmp(cmd, "rotvel"))
 	{
 		VehicleData->VehicleAttVel.Pitch = x;
 		VehicleData->VehicleAttVel.Roll  = y;
 		VehicleData->VehicleAttVel.Yaw 	 = z;
 	}

  	return DC_HANDLED;
}

/************************************************************************************************
	MoveCmpCmd
*************************************************************************************************/

static SINT32 dot_mvcmp(const char* line)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(VehicleData == nullptr) return DC_ERROR;
	FLOAT32 x,y,z;
	int argc;
	mlComponentListIterator it;

	x = y = z = 0.0f;
	argc = sscanf(line, "%*s %f %f %f", &x, &y, &z);

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		it.comp->OverridePos = 1;
		it.comp->OriginalRelativePos.x += x;
		it.comp->OriginalRelativePos.y += y;
		it.comp->OriginalRelativePos.z += z;
	}

	return DC_HANDLED;
}

/************************************************************************************************
	FrameProfCmd
*************************************************************************************************/

static SINT32 dot_frameprof(const char* line)
{
	logTimerFPS = 0;
	sscanf(line, "%*s %f", &logTimerFPS);

	if(logTimerFPS)
		uiChatPrintLine("Frame profiling on at %.2f FPS", logTimerFPS);
	else
		uiChatPrintLine("Frame profiling off");

	return DC_HANDLED;
}


/************************************************************************************************
	TurretConvCmd
*************************************************************************************************/

static SINT32 dot_televateconv(const char* line)
{
	char type[16];

	int argc = sscanf(line, "%s", type);

	if(argc == 1)
	{
		if(strcmp(type, "televateconv") == 0)
		{
			TurretConvergenceElevateOff = !TurretConvergenceElevateOff;
			uiChatPrintLine("Turret Convergence Elevate %s", TurretConvergenceElevateOff ? "off" : "on");
		}else if(strcmp(type, "ttraverseconv") == 0)
		{
			TurretConvergenceTraverseOff = !TurretConvergenceTraverseOff;
			uiChatPrintLine("Turret Convergence Traverse %s", TurretConvergenceTraverseOff ? "off" : "on");
		}
	}
	return DC_HANDLED;
}

/************************************************************************************************
	VehStatsCmd
*************************************************************************************************/
// current justs prints out vehicle bounding box
extern UINT32 _UpdateVehicleBoundingBox(mlObjectData* ObjectData,OBJECT * PlaneObject);

SINT32 dot_vehstats(const char* line)
{
	int i;

	for(i = 0; i < (int)vehtypeCount(); i++)
	{
		vehType const* vehtype;
		OBJECT* PlaneObject = nullptr;
		mlObjectData objdata;
		FLOAT64VECTOR pos = {0,0,0};
		FLOAT64ATT att = {0,0,0};

		vehtype = vehtypeGetByIndex(i);
		if(!vehtype) continue;

		obmgrCreateModelInstance((char*)vehtype->Shape);
		PlaneObject = obmainFindNamedObject((char*)vehtype->Shape);
		if(!PlaneObject) continue;

		memset(&objdata, 0x0, sizeof(objdata));
		mlMoveAndRotateObject(PlaneObject, &pos, &att);
		_UpdateVehicleBoundingBox(&objdata,PlaneObject);
		ogrpFreeObjectTree(PlaneObject);

		Write_Log(LOG_VEHICLE, LOG_INFO, "%s width: %.1f length: %.1f height: %.1f", vehtype->Id,
					objdata.BBox2.x - objdata.BBox1.x,
					objdata.BBox2.y - objdata.BBox1.y,
					objdata.BBox2.z - objdata.BBox1.z);
	}
	return DC_HANDLED;
}

/************************************************************************************************
	SloMoCmd
*************************************************************************************************/

static SINT32 dot_slomo(const char* line)
{
	SloMoOn = !SloMoOn;
	return DC_HANDLED;
}

/************************************************************************************************
	RankCmd
*************************************************************************************************/

static SINT32 dot_rank(const char* line)
{
	vehVehicle*_VehicleHead = nullptr, *CurrentVehicle;
	UINT32 Rank;
	char handle[16];

	sscanf(line, "%*s %15s %d", handle, &Rank);
	vehGetVehicleHead(&_VehicleHead);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(strcmp((char*)CurrentVehicle->Handle, handle) == 0)
		{
			CurrentVehicle->Rank = Rank;
			break;
		}
	}

	return DC_HANDLED;
}


/************************************************************************************************
	RideCmd
*************************************************************************************************/

static SINT32 dot_ride(const char* line)
{
	vehVehicle*PlayerVehicle = persGetPersonaVehiclePtr();
	if(PlayerVehicle == nullptr) return DC_ERROR;

	vehVehicle*_VehicleHead = nullptr, *CurrentVehicle;
	char handle[16] = "";
	UINT32 seatnum = 0;
	UINT32 RideVehicleHostIndex = 0;

	sscanf(line, "%*s %15s %d", handle, &seatnum);

	vehGetVehicleHead(&_VehicleHead);
	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(strcmp((char*)CurrentVehicle->Handle, handle) == 0)
		{
			RideVehicleHostIndex = CurrentVehicle->VehicleHostIndex;
			break;
		}
	}

	PlayerVehicle->RideSeatNum = seatnum;
	PlayerVehicle->RideVehicleHostIndex = RideVehicleHostIndex;

	return DC_HANDLED;
}

/************************************************************************************************
	ReloadClutter
*************************************************************************************************/

static SINT32 dot_reloadclutter(const char* line)
{
	_ReadBumpAndFrictionTable();
	trnClutterReset();
	return DC_HANDLED;
}

/************************************************************************************************
	ReloadAmmo
*************************************************************************************************/

static SINT32 dot_reloadammo(const char* line)
{
	_ReadAmmoTypes();
	return DC_HANDLED;
}

/************************************************************************************************
	Resupply
*************************************************************************************************/

static SINT32 dot_fpsresupply(const char* line)
{
	vehVehicle* veh = persGetPersonaVehiclePtr();

	if(veh)
	{
		if(!Online && !veh->Chat_Handle)
			veh->Chat_Handle = chatGetHandle("Self");

		// number of times to resupply yourself
		int count = 0;

		if(sscanf(line, "%*s %d", &count) == 1 && count > 0 && count < 10)
		{
			while(count--)
				fpFPSResupplySelf(veh);
		}

		for(int i = 0; i < VEH_MAX_MOUNTS; i++)
		{
			weapWeapon* weapon = &veh->Weapons[i][0];

			if(!weapon->WeaponType)
				continue;

			uiChatPrintLine("Weapon Supply %s (%d)", weapon->WeaponType->Name, i);

			for(int j = 0; j < VEH_MAX_WEAPONS_PER_MOUNT; j++)
			{
				if(!weapon->NumTotalRounds[j])
					continue;

				int ntr = weapon->NumTotalRounds[j];
				int nosr = weapon->NumOverSupplyRounds[j];
				int nral = weapon->NumRoundsAmmoLeft[j];

				uiChatPrintLine("     Ammo %d: %d/%d/%d", j, ntr, nosr, nral);
			}
		}
	}

	return DC_HANDLED;
}

/************************************************************************************************
	_CalcInertia
*************************************************************************************************/

static void _CalcInertia(OBJECT* obj, FILE* f, LOGICAL print, FLOAT64 I[3][3], FLOAT64VECTOR* cm, FLOAT64* vol)
{
	uiChatPrintLine("unimplemented fix me");

/*	FLOAT64 Iobj[3][3];
	FLOAT64VECTOR cmObj;
	FLOAT64 volObj;

	if(obj->flags & OBJECT_GROUP)
	{
		FLOAT64 Isub[3][3];
		FLOAT64VECTOR cmSub, cmNew;
		FLOAT64 volSub, volNew;
		FLOAT64VECTOR relCM;
		OBJECT* subObj;
		int i, j;

		memset(Iobj, 0, 9 * sizeof(FLOAT64));
		cmObj.x = cmObj.y = cmObj.z = 0;
		volObj = 0;

		subObj = obj->child;
		while(subObj)
		{
			_CalcInertia(subObj, f, FALSE,Isub, &cmSub, &volSub);

			// calculate new cm and vol
			volNew = volObj + volSub;

			cmNew.x = (cmObj.x * volObj + cmSub.x * volSub) / volNew;
			cmNew.y = (cmObj.y * volObj + cmSub.y * volSub) / volNew;
			cmNew.z = (cmObj.z * volObj + cmSub.z * volSub) / volNew;

			// adjust moment to new cm
			relCM.x = cmNew.x - cmSub.x;
			relCM.y = cmNew.y - cmSub.y;
			relCM.z = cmNew.z - cmSub.z;
			ptkMatrix3MultiplyScalar(Isub, volSub);
			ptkRotationalInertiaTranslate(Isub, Isub, volSub,(ptkVector*)&relCM, TRUE);

			relCM.x = cmNew.x - cmObj.x;
			relCM.y = cmNew.y - cmObj.y;
			relCM.z = cmNew.z - cmObj.z;
			ptkMatrix3MultiplyScalar(Iobj, volObj);
			ptkRotationalInertiaTranslate(Iobj, Iobj, volObj,(ptkVector*)&relCM, TRUE);

			// then add and normalize new moment
			for(i = 0; i < 3; i++)
			{
				for(j = 0; j < 3; j++)
				{
					Iobj[i][j] = (Iobj[i][j] + Isub[i][j]) / volNew;
					if(Iobj[i][j] > -.000001 && Iobj[i][j] < .000001)
						Iobj[i][j] = +0.0;

					if(Iobj[i][j] < 0)
						uiChatPrintLine("Negative moment of inertia values for %s calculated! Invalid, do not use!!!", subObj->name);

					if(i == j && Iobj[i][j] == 0)
						uiChatPrintLine("Zero diagonal inertia values for %s calculated! Invalid, do not use!!!", subObj->name);
				}
			}

			cmObj = cmNew;
			volObj = volNew;

			if(volObj < 0)
				uiChatPrintLine("Negative volume for %s calculated! Invalid, do not use!!!", subObj->name);

			subObj = subObj->sibling;
		}
	}else if(obj->flags & OBJECT_LOD)
	{
		uiChatPrintLine("LOD in mass object, error!!!");

	}else if(!(obj->flags & OBJECT_NOTOBJECT))
	{
		mlCalcObjectRotationalInertia(obj, 1.0, Iobj, &cmObj, &volObj);
	}else
	{
		uiChatPrintLine("unsupported object type in mass object, error!!!");
	}

	if(print)
	{
		fprintf(f, 	"\n<mass name=\"%s\"\n"
					"\tmass=\"1\" volume=\"%lf\"\n"
					 "\tmodelcmx=\"%+lf\" modelcmy=\"%+lf\" modelcmz=\"%+lf\"\n"
					 "\tunitinertiaxx=\"%+lf\" unitinertiayy=\"%+lf\" unitinertiazz=\"%+lf\"\n"
					 "\tunitinertiaxy=\"%+lf\" unitinertiaxz=\"%+lf\" unitinertiayz=\"%+lf\"/>",
			obj->name, volObj, cmObj.x, cmObj.y, cmObj.z,
			Iobj[0][0],Iobj[1][1],Iobj[2][2],
			Iobj[0][1],Iobj[0][2],Iobj[1][2]);
	}

	if(I)
		memcpy(I, Iobj, 9 * sizeof(FLOAT64));
	if(cm)
		*cm = cmObj;
	if(vol)
		*vol = volObj;*/
}

/************************************************************************************************
	_PrintHitmap
*************************************************************************************************/

static void _PrintHitmap(OBJECT* obj, void* fp)
{
	FILE* f = (FILE*)fp;

	if(obj->flags & OBJECT_NOTOBJECT) return;

	fprintf(f, "%s,15,0,0,0,0\r\n", obj->name);
}

/************************************************************************************************
	_PrintVehHitmap
*************************************************************************************************/

void _PrintVehHitmap(vehType const* vehtype )
{
	OBJECT* PlaneObject = nullptr, *hitmap, *obj;
	FILE* f;
	char filename[256] = "\0";

	if(!vehtype) return;
	obmgrCreateModelInstance((char*)vehtype->Shape);
	PlaneObject = obmainFindNamedObject((char*)vehtype->Shape);

	if(!PlaneObject) return;

	snprintf(filename, 255, "%s.hit", vehtype->Id);

	f = fopen(filename, "w");
	if(f)
	{
		ogrpFindSubObjectByName(&hitmap, PlaneObject,"hitmap");
		if(hitmap)
		{
			fprintf(f, "#External\r\n");
			ogrpFindSubObjectByName(&obj,hitmap,"outside");
			if(obj)
				mlForEachSubObject(obj, _PrintHitmap, f, 0);

			fprintf(f, "\r\n#Internal\r\n");
			ogrpFindSubObjectByName(&obj,hitmap,"inside");
			if(obj)
				mlForEachSubObject(obj, _PrintHitmap, f, 0);
		}
		ogrpFreeObjectTree(PlaneObject);

		fclose(f);
	}
	return;
}

struct hitmapNameList
{
	int num;
	char* name[256];
	LOGICAL infile[256];
};

/************************************************************************************************
	_ValidateVehHitmap
*************************************************************************************************/

static void _ValidateVehHit(vehType const* vehtype)
{
	if(!vehtype) return;

	OBJECT* PlaneObject = nullptr, *hitmap, *obj;
	char str[256] = { 0 };
	hitmapNameList list{};
	int fd, i;
	FILE* f;
	LOGICAL found;

	obmgrCreateModelInstance((char*)vehtype->Shape);
	PlaneObject = obmainFindNamedObject((char*)vehtype->Shape);

	if(!PlaneObject) return;

	// get components from obj
	ogrpFindSubObjectByName(&hitmap, PlaneObject,"hitmap");
	if(hitmap)
	{
		ogrpFindSubObjectByName(&obj,hitmap,"outside");
		if(obj)
			mlForEachSubObject(obj, _HitmapName, &list, 0);

		ogrpFindSubObjectByName(&obj,hitmap,"inside");
		if(obj)
			mlForEachSubObject(obj, _HitmapName, &list, 0);
	}

	// get components in hit file
	snprintf(str, 255, "%s.validhit", vehtype->Id);
	f = fopen(str, "w");
	if(f)
	{
		snprintf(str, 255, "vehicles\\%s\\%s.hit", vehtype->ModelDirectory, vehtype->Id);
		fd = fioOpen(str);
		if(fd > -1)
		{
			while(fioGetLine(fd, str, 255) > 0)
			{
				found = FALSE;
				if(!strncmp(str, "d_", 2))
				{
					strtok(str, ",");
					for(i = 0; i < list.num; i++)
					{
						if(!strcmp(list.name[i], str))
						{
							list.infile[i] = TRUE;
							found = TRUE;
							break;
						}
					}
					if(!found)
					{
						uiChatPrintLine("%s in %s.hit was not found in %s.mdl\n", str, vehtype->Id, vehtype->Shape);
						fprintf(f, "%s in %s.hit was not found in %s.mdl\n", str, vehtype->Id, vehtype->Shape);
					}
				}
			}
			// check if any unused
			for(i = 0; i < list.num; i++)
			{
				if(!list.infile[i])
				{
					uiChatPrintLine("%s is not in %s.hit\n", list.name[i], vehtype->Id);
					fprintf(f, "%s is not in %s.hit\n", list.name[i], vehtype->Id);
				}
			}
			fioClose(fd);
		}

		fclose(f);
	}

	ogrpFreeObjectTree(PlaneObject);

	return;
}

/************************************************************************************************
	_ValidateVehCmp
*************************************************************************************************/

static void _ValidateVehCmp(vehType const* vehtype)
{
	if (!vehtype) return;

	char str[256] = { 0 };
	hitmapNameList list{};
	int fd, i;
	FILE* f;
	LOGICAL found;

	snprintf(str, 255, "%s.validcmp", vehtype->Id);
	f = fopen(str, "w");
	if(f)
	{
		// parse .hit
		snprintf(str, 255, "vehicles\\%s\\%s.hit", vehtype->ModelDirectory, vehtype->Id);
		fd = fioOpen(str);
		if(fd > -1)
		{
			while(fioGetLine(fd, str, 255) > 0)
			{
				if(!strncmp(str, "d_", 2))
				{
					strtok(str, ",");
					if(list.num < 256)
					{
						char* compname = (char*)memAlloc(strlen(str)+1);
						strcpy(compname, str);
						list.name[list.num++] = compname;
					}
				}
			}
			fioClose(fd);
		}

		// parse .cmp
		snprintf(str, 255, "vehicles\\%s\\%s.cmp", vehtype->ModelDirectory, vehtype->Id);
		fd = fioOpen(str);
		if(fd > -1)
		{
			while(fioGetLine(fd, str, 255) > 0)
			{
				char* varname, *varcontents;
				mlUtilityGetVarnameAndContents((UCHAR**)&varname, (UCHAR**)&varcontents, (UCHAR*)str);
				if(varname && varcontents && !strncmp(varname, "hitmapcomponent", strlen("hitmapcomponent")))
				{
					found = FALSE;
					for(i = 0; i < list.num; i++)
					{
						if(!strcmp(list.name[i], varcontents))
						{
							list.infile[i] = TRUE;
							found = TRUE;
							break;
						}
					}
					if(!found)
					{
						uiChatPrintLine("%s in %s.cmp was not found in %s.hit\n", varcontents, vehtype->Id, vehtype->Id);
						fprintf(f, "%s in %s.cmp was not found in %s.hit\n", varcontents, vehtype->Id, vehtype->Id);
					}
				}
			}

			// check if any unused
			for(i = 0; i < list.num; i++)
			{
				if(!list.infile[i])
				{
					uiChatPrintLine("%s in %s.hit is not in %s.cmp\n", list.name[i], vehtype->Id, vehtype->Id);
					fprintf(f, "%s in %s.hit is not in %s.cmp\n", list.name[i], vehtype->Id, vehtype->Id);
				}
				if(list.name[i])
					memFree(list.name[i]);
			}

			fioClose(fd);
		}
		fclose(f);
	}
	return;
}

/************************************************************************************************
	_HitmapName
*************************************************************************************************/

static void _HitmapName(OBJECT* obj, void* p)
{
	hitmapNameList* list = (hitmapNameList*)p;

	if(obj->flags & OBJECT_NOTOBJECT) return;
	if(list->num >= 256) return;

	list->name[list->num] = obj->name;
	list->num++;
}

#include "phui/Periodical.h"
#include "imaplib/imap.h"

static SINT32 TurretTest;
static SINT32 TurretTestStartTime;
static phui::Periodical* TurretTestUpdate;

static void TurretTestStop(mlVehicleData *VehicleData, UINT32 Position) {
    VehicleData->TurretTraverseRate[Position] = 0;
    disableAxisInput = 0;
    TurretTest = 0;
    TurretTestUpdate->StopExecuting();
}

static void TurretTestCallback(phui::Periodical*, unsigned int, void*)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	UINT32 Position = persGetPersonaPosition();

	if(VehicleData && VehicleData->LastTurretTraverse[Position] < 0 && VehicleData->TurretTraverse[Position] > 0)
	{
        TurretTestStop(VehicleData, Position);
        uiChatPrintLine("Turret rotation time: %d secs",  (frametime - TurretTestStartTime)/1000);
	}
}

static SINT32 dot_turrettest(const char*)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	UINT32 Position = persGetPersonaPosition();

    if (TurretTest == 1) {
        TurretTestStop(VehicleData, Position);
    } else {
        TurretTest = 1;
        TurretTestStartTime = frametime;
        if(VehicleData)
        {
            VehicleData->TurretTraverseRate[Position] =  1.0;
            VehicleData->TurretElevateRate[Position] = 0;
            VehicleData->TurretTraverse[Position] = 0;
            VehicleData->LastTurretTraverse[Position] = 0;
            VehicleData->TurretElevate[Position] = 0;
            VehicleData->LastTurretElevate[Position] = 0;
        }
        
        if(!TurretTestUpdate) TurretTestUpdate = new phui::Periodical(TurretTestCallback);
        TurretTestUpdate->StartExecuting();
        
        disableAxisInput = 1;
    }
	return DC_HANDLED;
}

static SINT32 dot_speed(const char*)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(VehicleData)
	{
		FLOAT64 Speed = mathExactVectorLength(&VehicleData->VehicleVel);
		uiChatPrintLine("Speed %lf m/s", Speed);
	}

	return DC_HANDLED;
}

#endif // BETA_RELEASE



#ifdef GM_RELEASE

/************************************************************************************************
	MoveCmd
*************************************************************************************************/
SINT32 dot_move(const char* line)
{
	vehVehicle* player = persGetPersonaVehiclePtr();
	if(!player) return DC_ERROR;

	mlVehicleData* VehicleData = player->ModelData;
	if(!VehicleData) return DC_ERROR;

	FLOAT64VECTOR pos = {0,0,0};

	if(strstr(line, "N") || strstr(line, "S"))
	{
		int latdeg=0, latmin=0, latsec=0, longdeg=0, longmin=0, longsec=0;
		char NS,EW;
		if(sscanf(line, "%*s %c%d.%d.%d %c%d.%d.%d %lf", &NS, &latdeg, &latmin, &latsec, &EW, &longdeg, &longmin, &longsec, &pos.z) != 9)  return DC_ERROR;
		trnmainLatLongToPosition(latdeg, latmin, latsec, (NS=='N'), longdeg, longmin, longsec, (EW='E'), &pos.x, &pos.y);
	}
	else if(sscanf(line, "%*s %lf %lf %lf", &pos.x , &pos.y, &pos.z) < 2)
	{
		SuperCellMapView* map = uiMapFind("mapview");
		bool failed = true;

		if(map)
		{
			SuperCellMapView::MapObject* mo = map->SelectedMapObject();

			if(mo)
			{
				StratMapObject* smo = dynamic_cast<StratMapObject*>(mo);

				if(smo)
				{
					failed = false;
					pos.x = (FLOAT64)smo->_cp.PosX();
					pos.y = (FLOAT64)smo->_cp.PosY();
					pos.z = VehicleData->VehiclePos.z;
				}
			}
		}

		if(failed)
			return DC_ERROR;
	}

	if(VehicleData->VehicleMasterPtr->VehicleType->Type != VEH_TYPE_FLOATER)
		SuperFloat = TRUE;

	VehicleData->VehicleLastPos = FLOAT64VECTOR{ 0,0,0 };
	VehicleData->RenderPos = VehicleData->VehiclePos = pos;

	return DC_HANDLED;
}

#ifdef BETA_RELEASE
//////////////////////////////////////////////////////////////////////
//	CheckPlacement
//////////////////////////////////////////////////////////////////////
static SINT32 dot_checkplacement(const char*)
{
	gCheckPlanarPlacement = TRUE;
	rnmainListText("Checking planar placement of loading terrain objects from now on ...");
	rnmainListText("Correctly flagged as non planar:   GREEN");
	rnmainListText("Incorrectly flagged as non planar: YELLOW");
	rnmainListText("Non planar placement NOT flagged:  RED");

	return DC_HANDLED;
}
#endif //ifdef BETA_RELEASE


/************************************************************************************************
	FollowCmd
*************************************************************************************************/

static SINT32 dot_follow(const char* line)
{
	BuzzardFollowVehicle[0] = '\0';
	sscanf(line,"%*s %255s %f", BuzzardFollowVehicle, &BuzzardFollowDistance);

	return DC_HANDLED;
}

/************************************************************************************************
	InfLODCmd
*************************************************************************************************/

static SINT32 dot_inflod(const char* line)
{
	FLOAT32 newLODsetting;
	sscanf(line, "%*s %f", &newLODsetting);

	gr2AnimForceLOD(newLODsetting);

	if(newLODsetting)
		uiChatPrintLine("Forcing infantry LOD to %.2f meters", newLODsetting);
	else
		uiChatPrintLine("Resetting infantry LOD to normal behavior.");

	return DC_HANDLED;
}

/************************************************************************************************
InfLODModCmd
*************************************************************************************************/

#if defined(BETA_RELEASE)
static SINT32 dot_inflodmod(const char* line)
{
	if (sscanf(line, "%*s %f %f", &gTrooperLOD, &gTrooperMOD) > 0)
		uiChatPrintLine("Setting infantry LOD mod to %.2f%% and a mod of %.2f", (gTrooperLOD * 100.0f), gTrooperMOD);
	else
		uiChatPrintLine("You must set a percent (0 to 1) in the 1st parameter and an optional mod in the 2nd one.");

	return DC_HANDLED;
}

static int dot_sf(const char* line)
{
    FLOAT32 x = 0.0;
    if (sscanf(line, "%*s %f", &x) > 0) {
        ScaleFactor = x;
    } else
        uiChatPrintLine("Scalefactor: %.2f", ScaleFactor);
    return DC_HANDLED;
}

static int dot_gav(const char* line)
{
    FLOAT32 x = 0.0, y = 0.0, z = 0.0;
    if (sscanf(line, "%*s %f %f %f", &x, &y, &z) > 0) {
               gXValue = x;
               gYValue = y;
               gZValue = z;
    } else
        uiChatPrintLine("gX: %.2f gY: %.2f gZ: %.2f", gXValue, gYValue, gZValue);
    return DC_HANDLED;
}

static int dot_grv(const char* line)
{
    FLOAT32 d = 0.0, e = 0.0, h = 0.0;
    if (sscanf(line, "%*s %f %f %f", &d, &e, &h) > 0) {
               gDValue = d;
               gEValue = e;
               gHValue = h;
    } else
        uiChatPrintLine("gD: %.2f gE: %.2f gH: %.2f", gDValue, gEValue, gHValue);
    return DC_HANDLED;
}

static int dot_rate(const char* line)
{
    if (sscanf(line, "%*s %f", &gRate) > 0)
        uiChatPrintLine("Setting turrate to %.2f%%", gRate);
    else
        uiChatPrintLine("Set to: %.2f%%", gRate);

    return DC_HANDLED;
    
}

#endif

/************************************************************************************************
	else
	ScenarioCmd
*************************************************************************************************/

#if defined(BETA_RELEASE)
static SINT32 dot_scenario(const char* line)
{
	if(Online)
		return DC_HANDLED;

	const char* editContent = "scenarios/edit.xml";

	char command[25+1];
	char argument[255+1];
	UINT32 team = 1;
	UINT32 round = 1;
	UINT32 stage = 1;

	command[0] = 0;
	argument[0] = 0;

	sscanf(line, "%*s %25s %255s %u %u %u", command, argument, &team, &round, &stage);

	if(!strcasecmp(command, "start"))
	{
		uiChatPrintLine("Starting scenario: %s as team %d", argument, team);

		SCEN_MGR->SetOfflineData(team, round, stage);

		std::string path = "";
		Scenario* root = scenMainLoad("scenarios/index.xml");

		if(root)
		{
			jsContentData::LinkList* clink = root->links;

			for(size_t i = 0; i < clink->size(); i++)
			{
				std::string cpath = clink->at(i).first;
				cpath += "/index.xml";

				Scenario* cat = scenMainLoad(cpath.c_str());

				if(cat)
				{
					jsContentData::LinkList* slink = cat->links;

					for(size_t s = 0; s < slink->size(); s++)
					{
						Scenario* scenario = scenMainLoad(slink->at(s).first.c_str());

						if(scenario)
						{
							if(scenario && strcmp(scenario->name.c_str(), argument) == 0)
							{
								path = slink->at(s).first;
								break;
							}
							else
							{
								const char* ptr = strstr(scenario->name.c_str(), "/");

								if(ptr)
								{
									ptr += 1;

									if(strcmp(ptr, argument) == 0)
									{
										path = slink->at(s).first;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		if(path.empty())
			path = argument;

		SCEN_MGR->Start(path.c_str());
	}
	else if(!strcasecmp(command, "stop"))
	{
		uiChatPrintLine("Stopping scenario!");
		SCEN_MGR->Stop();
	}
	else if(!strcasecmp(command, "reload"))
	{
		Scenario* running = SCEN_MGR->GetScenario();

		if(running)
		{
			std::string path = running->path;
			std::string name = running->name;

			if(SCEN_MGR->Start(path.c_str()))
				uiChatPrintLine("Reloaded scenario: %s", name.c_str());
			else
				uiChatPrintLine("Failed to reload scenario: %s", name.c_str());
		}
		else
		{
			uiChatPrintLine("No scenario running!");
		}
	}
	else if(!strcasecmp(command, "edit"))
	{
		if(!SCEN_MGR->IsActive())
			SCEN_MGR->Start(editContent);

		uiChatPrintLine("Toggling scenario content editor!");
        bool mode = SCEN_MGR->GetEditMode();
        SCEN_MGR->SetEditMode(!mode);
	}
	else if(!strcasecmp(command, "export"))
	{
		SCEN_MGR->ExportEditPoints();
		uiChatPrintLine("Exporting current edit area!");
	}
	else if(!strcasecmp(command, "events"))
	{
		if(SCEN_MGR->HasFlag(ScenarioManager::RUN_EVENTS))
		{
			uiChatPrintLine("Scenario Events Disabled!");
			SCEN_MGR->SetFlag(ScenarioManager::RUN_EVENTS, false);
		}
		else
		{
			uiChatPrintLine("Scenario Events Enabled!");
			SCEN_MGR->SetFlag(ScenarioManager::RUN_EVENTS, true);
		}
	}
	else if(!strcasecmp(command, "icons"))
	{
		if(SCEN_MGR->HasFlag(ScenarioManager::ENEMY_ICONS))
		{
			uiChatPrintLine("Scenario enemy icons Disabled!");
			SCEN_MGR->SetFlag(ScenarioManager::ENEMY_ICONS, false);
		}
		else
		{
			uiChatPrintLine("Scenario enemy icons Enabled!");
			SCEN_MGR->SetFlag(ScenarioManager::ENEMY_ICONS, true);
		}
	}
	else if(!strcasecmp(command, "team"))
	{
		uiChatPrintLine("Setting team to %d", team);
		SCEN_MGR->SetTeam(team);
	}
	else if(!strcasecmp(command, "reload"))
	{
		uiChatPrintLine("Reloading scenario content!");
		SCEN_MGR->LoadContent();
		SCEN_MGR->LoadObjects();
	}

	return DC_HANDLED;
}
#endif

#ifdef BETA_RELEASE

/************************************************************************************************
	JavascriptCmd
*************************************************************************************************/

static SINT32 dot_js(const char* line)
{
	char script[255+1];
	script[0] = 0;

	sscanf(line, "%*s %255s", script);

	if(script[0])
		uiRunScript(script);

	return DC_HANDLED;
}
#endif

#if platOS != OS_MACOS

static int dot_audiochatlogging(const char* line)
{
#if XAUDIO
    extern void audioToggleChatLogging();

    audioToggleChatLogging();
#endif
    return DC_HANDLED;
}

static int dot_audiowritelog(const char* line)
{
#if XAUDIO
    extern void audioLogMinDistances();

    audioLogMinDistances();
#endif
    return DC_HANDLED;
}

static int dot_audiomindistance(const char* line)
{
#if XAUDIO
    extern void audioSetMinDistance(const char* name, float value);

    char name[32];
    float min;

    sscanf(line, "%*s %s %f", name, &min);

    audioSetMinDistance(name, min);
#endif

    return DC_HANDLED;
}

#endif

/************************************************************************************************
	VehLODCmd
*************************************************************************************************/

static SINT32 dot_vehlod(const char* line)
{
	vehmainToggleLOD();
	return DC_HANDLED;
}

#endif // GM_RELEASE

#ifdef BETA_RELEASE
/************************************************************************************************
	TrnTextureCmd
*************************************************************************************************/
static SINT32 dot_trntex(const char* line)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(!VehicleData) return DC_ERROR;

	OBJECT* CollideObject = nullptr;
	piVECTOR V1,V2;
	FLOAT64VECTOR worldOriginOffset;

	trnGetWorldOriginOffset(&worldOriginOffset);
	V1.x = (FLOAT32)(VehicleData->VehiclePos.x - worldOriginOffset.x);
	V1.y = (FLOAT32)(VehicleData->VehiclePos.y - worldOriginOffset.y);
	V2 = V1; V2.z = 10000; V1.z = -10000;

	trncdCollideWithVehiclesAndTerrain(&V1,&V2,nullptr,&CollideObject,nullptr,nullptr,nullptr,FALSE,TRUE, FALSE);

	if(CollideObject)
		uiChatPrintLine("Texture: %s", CollideObject->texname);

	return DC_HANDLED;
}

static SINT32 dot_drawoctettris(const char* line)
{
    static bool toggle = false;

    toggle = !toggle;

    trnmainDrawOctetTriangles(toggle);

    return DC_HANDLED;
}

static SINT32 dot_drawobtris(const char* line)
{
    static LOGICAL toggle = false;

    toggle = !toggle;

    obdrawObjectTriangles(toggle);

    return DC_HANDLED;
}

static SINT32 dot_drawobnormals(const char* line)
{
    static LOGICAL toggle = false;

    toggle = !toggle;

    obdrawObjectNormals(toggle);

    return DC_HANDLED;
}

static SINT32 dot_drawcolnormals(const char* line)
{
    static LOGICAL toggle = false;

    toggle = !toggle;

    obdrawCollisionNormals(toggle);

    return DC_HANDLED;
}

static SINT32 dot_drawbbox(const char* line)
{
    static LOGICAL toggle = false;

    toggle = !toggle;

    rnmainListText("Draw bounding boxes %s", toggle ? "ON" : "OFF");

    obdrawDrawBoundBox(toggle);

    return DC_HANDLED;
}

static SINT32 dot_drawcollisionobjects(const char* line)
{
    extern void obdrawDrawCollisionObjects(LOGICAL);

    static LOGICAL toggle = false;

    toggle = !toggle;

    rnmainListText("Draw collision objects %s", toggle ? "ON" : "OFF");

    obdrawDrawCollisionObjects(toggle);

    return DC_HANDLED;
}

static SINT32 dot_drawtobbox(const char* line)
{
    static LOGICAL toggle = false;

    toggle = !toggle;

    rnmainListText("Draw TO bounding boxes %s", toggle ? "ON" : "OFF");

    obdrawDrawTOBoundBox(toggle);

    return DC_HANDLED;
}

static SINT32 dot_tps(const char* line)
{
    const char* p = line;
    p += 4;

    char buff[2] = {0};
    strncpy(buff, p, 1);

    int id = -atol(buff);

    vehVehicle*head = nullptr;
    vehVehicle*v = nullptr;

    vehGetVehicleHead(&head);

    for(; head != nullptr; head = head->getNext())
    {
        if(head->VehicleNum == id)
        {
            v = head;
            break;
        }
    }

    p += 2;

    if(p && v)
    {
        gr2AnimBitPacker pack;

        v->AnimationBits = 0;

#if(0)
        if(strcmp(p, "crouchdeployedrelaxed") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_RELAXED;
        }
        else if(strcmp(p, "crouchdeployedupleft") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_UP_LEFT;
        }
        else if(strcmp(p, "crouchdeployedupright") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_UP_RIGHT;
        }
        else if(strcmp(p, "crouchdeployeddownleft") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_DOWN_LEFT;
        }
        else if(strcmp(p, "crouchdeployeddownright") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_DOWN_RIGHT;
        }
        else if(strcmp(p, "crouchdeployedreload") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_RELOAD;
        }
        else if(strcmp(p, "crouchdeployedup") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_UP;
        }
        else if(strcmp(p, "crouchdeployeddown") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_CROUCH;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_DOWN;
        }
        else if(strcmp(p, "standdeployedrelaxed") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_RELAXED;
        }
        else if(strcmp(p, "standdeployedup") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_UP;
        }
        else if(strcmp(p, "standdeployeddown") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_DOWN;
        }
        else if(strcmp(p, "standdeployedreload") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_DEPLOYED;
            pack.point = (int)TPS_POINTTYPE_RELOAD;
        }
        else if(strcmp(p, "standidleup") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_IDLE;
            pack.point = (int)TPS_POINTTYPE_UP;
        }
        else if(strcmp(p, "standidledown") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_IDLE;
            pack.point = (int)TPS_POINTTYPE_DOWN;
        }
        else if(strcmp(p, "standidlerelaxed") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_IDLE;
            pack.point = (int)TPS_POINTTYPE_RELAXED;
        }
        else if(strcmp(p, "standidlereload") == 0)
        {
            pack.pose = (int)TPS_POSETYPE_STAND;
            pack.movement = (int)TPS_MOVETYPE_IDLE;
            pack.point = (int)TPS_POINTTYPE_RELOAD;
        }
        else
        {
            p = "ERROR: nothing set";
        }
#endif

        v->AnimationBits = pack.pack();

        if(1)
        {
            rnmainListText("TPS: %s", p);
        }
    }

    return DC_HANDLED;
}

static SINT32 dot_printweather(const char* line)
{
    trndataPrint();
    return DC_HANDLED;
}

static SINT32 dot_weather(const char* line)
{
    // "weather setting (light and cloud)
    SCHAR* p = (char*)line + 8;

    if(!p) return DC_HANDLED;

    SINT32 setting = atoi(p);

    setting = setting < 1 ? 1 : setting > 3 ? 3 : setting;

    // One based
    setting--;

    trnskyChangeSetting(setting, trnskyGetFogOption(), true);

    rnmainListText("type '.printweather' to see valid entries");

    return DC_HANDLED;
}

static SINT32 dot_fog(const char* line)
{
    // "fog setting
    SCHAR* p = (char*)line + 4;

    if(!p) return DC_HANDLED;

    SINT32 option = atoi(p);

    option = option < 1 ? 1 : option > 3 ? 3 : option;

    // One based
    option--;

    trnskyChangeSetting(trnskyGetSetting(), option, true);

    rnmainListText("type '.printweather' to see valid entries");

    return DC_HANDLED;
}

/************************************************************************************************
	ReloadVehiclesCmd
*************************************************************************************************/

static SINT32 dot_reloadvehicles(const char* line)
{
	vehtypeInit();
	fpFPSDataReadFPSFile();
	_ReadAmmoTypes();
	
	uiChatPrintLine("All vehicles reloaded.");
	
	return DC_HANDLED;
}

static SINT32 dot_np(const char*)
{
extern bool _newpredictor;
	_newpredictor = !_newpredictor;

	uiChatPrintLine(_newpredictor ? "New Infantry Predictor" : "Old Infantry Predictor");

	return DC_HANDLED;
}

static SINT32 dot_lu(const char* line)
{
extern int _simlowupdate;
	_simlowupdate = 0;
	sscanf(line,"%*s %d", &_simlowupdate);

	uiChatPrintLine(_simlowupdate ? "Simulate Low Update Rate" : "Normal Update Rate");

	return DC_HANDLED;
}

static SINT32 dot_weaplog(const char* line)
{
extern FILE* weaplogFile;
	if(weaplogFile)
	{
		fclose(weaplogFile);
		weaplogFile = 0;

		uiChatPrintLine("weap log off");
	}else
	{
		weaplogFile = fopen("weap.log", "a");

		uiChatPrintLine("weap log on");
	}

	return DC_HANDLED;
}

static int dot_nospawn(const char* line)
{
    OverrideSpawner = NOSPAWN_OVERRIDE;
    uiChatPrintLine("Override spawn to nospawn");

    return DC_HANDLED;
}

static int dot_oldspawn(const char* line)
{
    OverrideSpawner = OLDSPAWN_OVERRIDE;
    uiChatPrintLine("Override spawn to oldspawn");

    return DC_HANDLED;
}

static int dot_area51(const char* line)
{
    OverrideSpawner = AREA_51_OVERRIDE;
    uiChatPrintLine("Override spawn to area 51");

    return DC_HANDLED;
}

static int dot_resetspawn(const char* line)
{
    OverrideSpawner = NO_OVERRIDE;
    uiChatPrintLine("Override spawn has been reset");

    return DC_HANDLED;
}

static int dot_postfilter(const char* line)
{
    // FIXME
    oglTogglePostFilter();

    return DC_HANDLED;
}

static int dot_specularshader(const char* line)
{
    WantSpecularMaps = !WantSpecularMaps;
    uiChatPrintLine(L"Specular shaders %s", WantSpecularMaps ? L"ENABLED" : L"DISABLED");

    return DC_HANDLED;
}

static int dot_normalshader(const char* line)
{
    g_wantNormalMaps = !g_wantNormalMaps;
    uiChatPrintLine(L"Normal shaders %s", g_wantNormalMaps ? L"ENABLED" : L"DISABLED");

    return DC_HANDLED;
}

static int dot_wingvortex(const char* line)
{
	vehVehicle* v = persGetPersonaVehiclePtr();

    if(v->StateAnimation.WingtipVortexSmokeHandle[0] || v->StateAnimation.WingtipVortexSmokeHandle[1])
    {
        v->StateAnimation.WingtipVortexSmokeOn[0] = 0;
        v->StateAnimation.WingtipVortexSmokeOn[1] = 0;

        vehfxCheckWingVortexState(v->iFXVehicleID);

        return DC_HANDLED;
    }

    v->StateAnimation.WingtipVortexSmokeOn[0] = 1;
    v->StateAnimation.WingtipVortexSmokeOn[1] = 1;

    vehfxCheckWingVortexState(v->iFXVehicleID);

    return DC_HANDLED;
}

static int dot_effect(const char* line)
{
    char effect[32];

    int fields = sscanf(line, "%*s %s", effect);

    if(fields == 0)
    {
        return 0;
    }

    TransformEffect* te = nullptr;

    int lifetime = 0;
    Vec3d pos;
    Vec3f n = Vec3f(0, 0, -1);

    const char* p = strstr(line, "-pos");

    if(p)
    {
        double x, y, z;
        sscanf(p, "%*s %lf %lf %lf", &x, &y, &z);

        pos.set(x + WorldOriginOffset.x, y + WorldOriginOffset.y, z);
    }

    // Transform is not attached to any object/ammo, etc
    bool standalone = false;
    p = strstr(line, "-standalone");

    if(p)
    {
        standalone = true;
    }

    p = strstr(line, "-normal");

    if(p)
    {
        float x, y, z;
        sscanf(p, "%*s %f %f %f", &x, &y, &z);

        n.set(x, y, z);

        if(!standalone)
            n.fastnormalize();
    }

    p = strstr(line, "-lifetime");

    if(p)
    {
        sscanf(p, "%*s %d", &lifetime);
    }

    if(standalone)
    {
        te = TransformEffect::AddStandAloneEffect(effect, n, lifetime);

        p = strstr(line, "-gravity");
        if(p) sscanf(p, "%*s %f", &te->_gravcoef);

        p = strstr(line, "-drag");
        if(p) sscanf(p, "%*s %f", &te->_dragcoef);
    }
    else
    {
        te = TransformEffect::AddEffect(effect, n, lifetime, nullptr);
    }

    if(te)
    {
        te->enableemitters(true);;
        te->setmatrix(Matrixd::translate(pos));
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
    }

    return 0;
}

static int dot_fire(const char* line)
{
    mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(!VehicleData) return DC_ERROR;

    Write_Log(LOG_WEAPON, LOG_DEBUG, "Vx: %f Vy: %f Vz: %f Ap: %f Ar: %f Ay: %f",
			VehicleData->VehiclePos.x, VehicleData->VehiclePos.y, VehicleData->VehiclePos.z,
            VehicleData->VehicleAtt.Pitch, VehicleData->VehicleAtt.Roll, VehicleData->VehicleAtt.Yaw);
	static char LastAmmoName[64];
	static FLOAT32 lastX, lastY, lastZ, lasttraverse, lastelevate;
	char AmmoName[64] = "";
	FLOAT64VECTOR Recoil, Pos, Vel = {0,0,0};
	weapAmmoType* AmmoType;
	piMATRIX FireAtt = IdentityMatrix;
	UINT32 Flags;
	FLOAT32 x,y,z,traverse,elevate;
	piVECTOR AttVec;
	LOGICAL ghost = FALSE;
	int argc;

	x = y = z = traverse = elevate = 0.0f;
	argc = sscanf(line, "%*s %63s %f %f %f %f %f", AmmoName, &x, &y, &z, &traverse, &elevate);
	if(argc < 1) return DC_ERROR;

	if(!strcmp(AmmoName, "again"))
	{
		strncpy(AmmoName, LastAmmoName,63);
		x = lastX; y = lastY; z = lastZ;
		traverse = lasttraverse, elevate = lastelevate;
	}else if(!strcmp(AmmoName, "last"))
	{
		strncpy(AmmoName, LastAmmoName,63);
	}

	weapFindAmmoTypeByName(&AmmoType, (UCHAR*)AmmoName);
	if(!AmmoType)
	{
		uiChatPrintLine("ammo type %s not found", AmmoName);
		return DC_ERROR;
	}

	draw3dRemoveAllDebugObjects();

	Pos = VehicleData->AdjustedForCGPos;
	Pos.x += x; Pos.y += y; Pos.z += z;

	Flags = WEAP_AMMOFLAG_USE_MEDCOLLIDELIST | WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_ONLY_CHECKPLAYER;
	if(ghost)
		Flags |= WEAP_AMMOFLAG_IS_GHOSTROUND;

	AttVec.x = DEG_TO_RAD(elevate);
	AttVec.y = 0.0f;
	AttVec.z = DEG_TO_RAD(traverse);
	mathEul_ToMatrix(&FireAtt,EulOrdXYZs,&AttVec);

	weapFireRound(&Recoil, AmmoType, &Pos, &Vel, &FireAtt, frametime/ 1000,  Flags, nullptr, &Pos, 0, nullptr);

	strncpy(LastAmmoName, AmmoName,63);
	lastX = x; lastY = y; lastZ = z;
	lasttraverse = traverse; lastelevate = elevate;

	return DC_HANDLED;
}

static int dot_float(const char* line)
{
	SuperFloat = !SuperFloat;

	uiChatPrintLine(L"Super Float %s", SuperFloat ? L"on" : L"off");

 	if(!SuperFloat)
 	{
 		vehVehicle* veh = persGetPersonaVehiclePtr();

        if(veh)
		{
			if(veh->VehicleType->Type == VEH_TYPE_PERSON)
			{
				fpFPSAreaChecks(nullptr, 0.0, true);

				if(fpsTrooper.parachuteWorn)
				{
					veh->RideVehicleHostIndex = 1;

					// force apara drop
					fpFPSRideUnrideVehicle();
				}
			}

			veh->RideVehicleHostIndex = 0;
		}
 	}

	return DC_HANDLED;
}

static int dot_altspd(const char* line)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(!VehicleData) return DC_ERROR;

	SINT32 alt, speed;
	int argc = sscanf(line, "%*s %d %d", &alt, &speed);

    if(argc > 0)
		VehicleData->VehiclePos.z = (FLOAT64)alt;

	if(argc > 1)
	{
		VehicleData->VehicleAtt.Pitch = VehicleData->VehicleRealAtt.Pitch = 0;
		VehicleData->VehicleAtt.Roll = VehicleData->VehicleRealAtt.Roll = 0;
		VehicleData->VehicleAtt.Yaw = VehicleData->VehicleRealAtt.Yaw = 0;
		VehicleData->VehicleAttMat = VehicleData->VehicleAttInvMat = IdentityMatrix;
		VehicleData->VehicleVel.x = 0;
		VehicleData->VehicleVel.y = speed;
		VehicleData->VehicleVel.z = 0;
	}

	return DC_HANDLED;
}

static int dot_ai(const char* line)
{
    trnautogunToggleAI();

    return DC_HANDLED;
}

static int dot_gizmo(const char* line)
{
	mlVehicleData* Vehicle = persGetPersonaModelDataPtr();
	if(Vehicle == nullptr) return DC_ERROR;

	int argc;
	char subcmd[32] = "", state[32];
	LOGICAL on;

	argc = sscanf(line, "%*s %31s %31s", subcmd, state);

	if(argc >= 2)
	{
		on = !strcmp(state, "on");

		if(!VehicleObjectAnalysis && on)
			VehicleObjectAnalysis = TRUE;

		if(!strcmp(subcmd, "foils"))
			DisplayFoils = on;
		else if(!strcmp(subcmd, "physics"))
			DisplayPhysics = on;
		else if(!strcmp(subcmd, "lines"))
			DisplayCollideLines = on;
	}
    else
	{
		if(!strcmp(subcmd, "clearlines"))
		{
			draw3dRemoveAllDebugObjects();
			return DC_HANDLED;
		}

		VehicleObjectAnalysis = !VehicleObjectAnalysis;
		on = VehicleObjectAnalysis;
		DisplayFoils = VehicleObjectAnalysis;;
		DisplayPhysics = VehicleObjectAnalysis;
		DisplayCollideLines = VehicleObjectAnalysis;
	}

	uiChatPrintLine("Gizmos and Whizbangs %s %s", subcmd, on ? "on" : "off");

	if(Vehicle && Vehicle->VehicleObjectData->CollideInternObject)
	{
		if(VehicleObjectAnalysis)
			Vehicle->VehicleObjectData->CollideInternObject->flag2 &= ~OBJECT_DONT_UPDATE;
		else
			Vehicle->VehicleObjectData->CollideInternObject->flag2 |= OBJECT_DONT_UPDATE;
	}

	if(Vehicle && Vehicle->VehicleObjectData->CollideExternObject)
	{
		if(VehicleObjectAnalysis)
			Vehicle->VehicleObjectData->CollideExternObject->flag2 &= ~OBJECT_DONT_UPDATE;
		else
			Vehicle->VehicleObjectData->CollideExternObject->flag2 |= OBJECT_DONT_UPDATE;
	}

	return DC_HANDLED;
}

static int dot_watershader(const char* line)
{
    trnwtrToggle();
    return DC_HANDLED;
}

static int dot_waterreflection(const char* line)
{
    g_wantWaterReflections = !g_wantWaterReflections;
    uiChatPrintLine("Water Reflections %s", g_wantWaterReflections ? "ENABLED" : "DISABLED");
    return DC_HANDLED;
}

static int dot_drawparticles(const char* line)
{
    extern void DisableParticleDraw(bool draw);
    static bool dontdraw = false;

    dontdraw = !dontdraw;
    DisableParticleDraw(dontdraw);


    return DC_HANDLED;
}

static int dot_supertroop(const char* line)
{
	fpFPSSetSuperTroop(!SuperTroop);
	uiChatPrintLine("SuperTroop %s", SuperTroop ? "on" : "off");

	return DC_HANDLED;
}

static int dot_aion(const char* line)
{
    trnautogunToggleAI();
    return DC_HANDLED;
}

static SINT32 dot_view(const char* line)
{
    phui::View* canvas = 0;
    const char *command = "view ";
    char *view;
    size_t commandLen = sizeof(command);
    int rc = strncmp(command, line, commandLen);
    if (rc == 0) {
        view = (char *)line + commandLen + 1;
        canvas = uiViewport()->FindView(view);
    }
    
    if(canvas)
    {
        canvas->Close();
    }
    else
    {
        canvas = uiLoadView(view);
        
        if(canvas)
        {
            char parent[32] = "";
            sscanf(line, "%*s %31s", parent);
            
            std::string script = "setParent('";
            script += parent;
            script += "');";
            
            phui::DOM::ElementDOM* viewdom = dynamic_cast<phui::DOM::ElementDOM*>(canvas->UserData(DOM_VIEW_USERDATA));
            if(viewdom) viewdom->RunScript(script.c_str());
        }
    }
    
    return DC_HANDLED;
}

#endif // BETA_RELEASE

#if RELEASE_CLIENT == 0
/************************************************************************************************
	D-I-Y dot command: If you need a temporary dot command, put your code here.
	Just remember not to share.
*************************************************************************************************/

//extern SINT32 mmSendDirectSpawn(SINT32 connection, const WEAPON_SPEC weapon, const FLOAT64VECTOR* location);
static SINT32 dot_dev(const char* line)
{
	const char* p = strchr(line, ' ');
	if(p == nullptr)
	{
		uiChatPrintLine("/dev <weapon name>");
		return DC_HANDLED;
	}
	WEAPON_SPEC weapon = weapFindWeaponByName(++p);
	if(weapon.wType == 0)
	{
		uiChatPrintLine("Unknown weapon type, '%s'?", p);
		return DC_HANDLED;
	}

	PlayerClient()->RequestDirectSpawn(weapon);

	uiChatPrintLine("Not implemented");		// Replaceme

	return DC_HANDLED;
}

static SINT32 dot_canvas(const char* line)
{
	phui::View* canvas = uiViewport()->FindView("canvas");

	if(canvas)
	{
		canvas->Close();
	}
	else
	{
		canvas = uiLoadView("canvas");

		if(canvas)
		{
			char parent[32] = "";
			sscanf(line, "%*s %31s", parent);

			std::string script = "setParent('";
			script += parent;
			script += "');";

			phui::DOM::ElementDOM* viewdom = dynamic_cast<phui::DOM::ElementDOM*>(canvas->UserData(DOM_VIEW_USERDATA));
			if(viewdom) viewdom->RunScript(script.c_str());
		}
	}

	return DC_HANDLED;
}

#endif // RELEASE_CLIENT

#ifdef GR2_BLEND_EDITOR
static SINT32 dot_blendeditor(const char* line)
{
	static SINT32 editSTOIndex = -1;
	static SINT32 cursorState = -1;
	phui::View* editor = uiViewport()->FindView("blendedit");

	if(Online)
		return DC_HANDLED;

	if(editor)
	{
		// close the ui
		editor->Close();

		// reset blend editing variables
		killBlendEditor();

		// remove the vehicle sto
		_removevehicle(editSTOIndex);

		// reset vehicle sto
		editSTOIndex = -1;

		persHUDShowHUD();
		uiChatShow();

		uiSetCursor(cursorState);
	}
	else
	{
		const char* p = strchr(line, ' ');
		if(p == nullptr || (p + 1) == nullptr || !strlen(p + 1))
		{
			uiLoadView("blendeditselect");
			//uiChatPrintLine("/blendedit <vid>");
			return DC_HANDLED;
		}

		WEAPON_SPEC spec = weapFindWeaponByName(++p);
		if(spec.wType == 0)
		{
			uiChatPrintLine("Unknown weapon type, '%s'?", p);
			return DC_HANDLED;
		}

		const vehType* vt = vehTypeGetBySpec(spec);

		if(!vt)
		{
			uiChatPrintLine("Unknown vehicle type!");
			return DC_HANDLED;
		}

		vehVehicle* player	= persGetPersonaVehiclePtr();
		FLOAT64VECTOR vpos	= player->ModelData->VehiclePos;
		FLOAT64ATT vatt		= player->ModelData->VehicleAtt;

		vatt.Pitch = 0;
		vatt.Roll = 0;

		if(player->VehicleType->Type == VEH_TYPE_PERSON)
			vatt.Yaw = -vatt.Yaw;

		piVECTOR AttVec;
		piMATRIX AttMat = IdentityMatrix;

		AttVec.x = 0.0f;
		AttVec.y = 0.0f;
		AttVec.z = vatt.Yaw;

		mathEul_ToMatrix(&AttMat,EulOrdXYZs,&AttVec);

		// move us out in front
		Vec3d forward = Vec3d(0,1.0f,0);
		Matrixd rotate = Matrixd(AttMat);

		forward = forward * rotate;
		forward *= 3.0f;

		vpos.x += forward.x();
		vpos.y += forward.y();
		vpos.z += forward.z();

		if(player->VehicleType->Type == VEH_TYPE_FLOATER)
			vpos.z -= 1.0f;

		WorldPosition_m position(vpos.x, vpos.y, vpos.z);
		AttitudeVector attitude(vatt.Pitch, vatt.Roll, vatt.Yaw);

		VehicleSTO* vehicle = new VehicleSTO(0, vt, position, attitude);
		ServerTrackedObject::addToIndex(vehicle);

		vehicle->setIcon("EditTarget");

		editSTOIndex = (int)vehicle->uid();

		// so we can manage them via dot command
        uiDotCommandAddClone(editSTOIndex);

		vehUpdate();

		vehVehicle* veh = vehFindVehicle(vehicle->getSTOVehicleID());

        veh->Flags |= VEH_FLAGS_IS_CLONE;
		veh->PosData.Pos.z = .1f;

		editor = uiLoadView("blendedit");
		if(editor) uiRunScript(editor, "initEditor()");

		gr2AnimTrooper* anim = veh->VehicleObjectData->PlaneObject->animModel;

		if(anim)
		{
			initBlendEditor(editor, anim);

			char script[255+1];

			for(int i = 0; i < anim->skeleton->skeleton->BoneCount; i++)
			{
				granny_bone& bone = anim->skeleton->skeleton->Bones[i];
				snprintf(script, 255, "addBone(%d, '%s', %d)", i, bone.Name, bone.ParentIndex);
				uiRunScript(editor, script);
			}

			uiRunScript(editor, "processBones()");
		}

		persHUDHideHUD();
		uiChatHide();

		cursorState = uiCursorState();

		uiSetCursor(1);
	}

	return DC_HANDLED;
}

static SINT32 dot_kit(const char* line)
{
	gr2AnimTrooper* model = getEditModel();
	if(!model) return DC_HANDLED;

	char kit[32] = "";

	sscanf(line, "%*s %31s", kit);

	int weapType = getEditBlendWeaponType();

	setEditBlendWeapon(-1);

	if(strlen(kit) > 0)
	{
		fpTPSKitsReset(model);
		fpTPSKitsInit(true);
		fpTPSKitsSetByName(model, kit);
		uiChatPrintLine("Set trooper kit to %s", kit);
	}
	else
	{
		fpTPSKitsReset(model);
		uiChatPrintLine("Cleared trooper kit!");
	}

	setEditBlendWeapon(weapType);

	return DC_HANDLED;
}

#endif

/************************************************************************************************
	Dot Time -- display the current time
*************************************************************************************************/

static SINT32 dot_time(const char* line)
{
	if (strchr(line, ' ') != nullptr) return DC_PASS_TO_HOST;

	// Grab the formatted "ctime" and remove the trailing eol
	time_t now = time(nullptr);
	wchar_t myTime[31];
	struct tm tmStruct;
	wcsftime(myTime, 30, L"%a %b %d %X %Y", localtime_r(&now, &tmStruct));
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_NOTE, LSTR_LOCAL_TIME, myTime);

	return DC_PASS_TO_HOST;
}

#if(0)
static int dot_font(const char* line)
{
	int argc;
	char font[32]	= "";
	int size		= 11;

	argc = sscanf(line, "%*s %31s %d", font, &size);

	if(argc)
		uiSetFontSize(size, font);

	return DC_HANDLED;
}
#endif

void uiClearSquadCommandTimeout()
{
	_nextsquadcommand = 0;
}

/************************************************************************************************
	SquadInvitesCmd
*************************************************************************************************/

static SINT32 dot_invites(const char* line)
{
	char onoff[9] = "";

	int argc = sscanf(line, "%*s %8s", onoff);
	bool invites = false;

	if(argc > 0)
	{
		invites = (!strcasecmp(onoff, "on")) ? true: false;
		cfgPrefsSetGlobal(cfgSQUAD_INVITES, &invites, true);
		uiRunScript("game.onchangeinvites()");
	}

	cfgPrefsGetGlobal(cfgSQUAD_INVITES, &invites);
	uiChatPrintLine("Squad Invites: %s", (invites) ? "on": "off");

	return DC_HANDLED;
}

// Allow the user to record a note to the WW2_Log.txt file.
static int dot_lognote(const char* line)
{
	clientErrorLog("USER NOTE: %s", line);
	uiChatPrintLine("Logged: %s", line);

	return DC_HANDLED;
}

#if defined(BETA_RELEASE)
static int dot_ragdolls(const char* line)
{
	char newval[32+1];

	if(sscanf(line, "%*s %32s", newval))
	{
		if(!strcasecmp(newval, "on"))
			cfgPrefsSetBool("app.ragdolls", true);
		else if(!strcasecmp(newval, "off"))
			cfgPrefsSetBool("app.ragdolls", false);
	}

	bool onoff = cfgPrefsGetBool("app.ragdolls", true);
	uiChatPrintLine("Ragdolls are %s.", (onoff) ? "on": "off");

    return DC_HANDLED;
}
#endif

#if defined(BETA_RELEASE)
static int dot_version(const char* line)
{
	std::string display = "Client version = ";
	display += PN_product_version_string;

	uiChatPrintLine(display.c_str());

	ARENA_VARS::iterator itr = g_arenaDescription.find("host_rev");

	if(Online && itr != g_arenaDescription.end())
	{
		display = "Host version = ";
		display += itr->second.c_str();
		uiChatPrintLine(display.c_str());
	}

    return DC_HANDLED;
}
#endif

static int dot_vcache(const char* line)
{
	UINT32 mode = gVCache;

	if(sscanf(line, "%*s %u", &mode))
	{
		if(mode != gVCache && mode <= 2)
			gVCache = mode;
	}

	if(!gVCache)
		vehmainClearVehicleCache();

	switch(gVCache)
	{
		case 0: uiChatPrintLine("Vehicle Cache: disabled"); break;
		case 1: uiChatPrintLine("Vehicle Cache: enabled (troops only)"); break;
		case 2: uiChatPrintLine("Vehicle Cache: enabled (troops and vehicles)"); break;
	}

	return DC_HANDLED;
}

#if defined(BETA_RELEASE)
static int dot_vcount(const char* line)
{
	vehVehicle*_VehicleHead = nullptr, *CurrentVehicle;

	char handle[16];
	strcpy(handle, "unknown");

	int argc = sscanf(line, "%*s %15s", handle);

	vehGetVehicleHead(&_VehicleHead);

	bool found = false;
	int vcount = 0;
	ChatHandle ch = (argc) ? chatGetLowercaseHandle(handle) : 0;
	chatGetCallsign(handle, ch);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(ch == CurrentVehicle->Chat_Handle)
			found = true;

		++vcount;
	}

	if(argc)
	{
		if(found)
			uiChatPrintLine("Found %s in the %d vehicles in my vis list!", handle, vcount);
		else
			uiChatPrintLine("Did not find %s in the %d vehicles in my vis list!", handle, vcount);

		if(vehmainIsVehicleCached(ch))
			uiChatPrintLine("%s is in the vehicle cache!", handle);
		else
			uiChatPrintLine("%s is not in the vehicle cache!", handle);
	}

	int fcount = 0; // free count
	int rcount = 0; // ready map count

	// force a report of the cache stats
	vehmainReportCache(fcount, rcount, false);

	static int _lastTotal = MAX_POSSIBLE_VIS_VEHICLES;
	int total = vcount + fcount + rcount;

	uiChatPrintLine("Vehicle Counts: visible(%d) + ready(%d) + free(%d) = total(%d)", vcount, rcount, fcount, total);

	if(total != MAX_POSSIBLE_VIS_VEHICLES)
	{
		uiChatPrintLine("************* TOTAL VEHICLE COUNT ANOMALIE: %d -> %d != %d", _lastTotal, total, MAX_POSSIBLE_VIS_VEHICLES);
		_lastTotal = total;
	}

	return DC_HANDLED;
}
#endif

#ifdef MODEL_INFO
static int dot_texinfo(const char* line)
{
  extern std::map<GLuint, std::string> reverseTextureIndex;

  for( int i=1; i<3200; i++ )
  {
    if ( glIsTexture(i) )
    {
        TextureState::Bind(i, GL_TEXTURE_2D);

      GLint W, H, F;

      glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH, &W );
      glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT, &H );
      glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTERNAL_FORMAT, &F );

      std::string textureName = reverseTextureIndex.find(i) == reverseTextureIndex.end() ? "unknown" : reverseTextureIndex[i];
      std::transform(textureName.begin(), textureName.end(),textureName.begin(), ::tolower);
      if ( textureName.size() > 50 )
      {
        textureName.resize(50);
      }

      int format = (int) F, bytes = 1;
      const char* s = "OTHER";
      switch(format)
      {
        case GL_ALPHA8_EXT:                      s= "GL_ALPHA8_EXT"                  ;
          break;
        case GL_LUMINANCE8_EXT:                  s= "GL_LUMINANCE8_EXT"              ;
          break;
        case GL_RGB8_EXT:                        s= "GL_RGB8_EXT"                     ; bytes=3;
          break;
        case GL_RGBA8_EXT:                       s= "GL_RGBA8_EXT"                    ; bytes=4;
          break;
        case GL_DEPTH_COMPONENT16_SGIX:          s= "GL_DEPTH_COMPONENT16_SGIX"      ;
          break;
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:    s= "GL_COMPRESSED_RGB_S3TC_DXT1_EXT" ; bytes=3;
          break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:   s= "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT"; bytes=4;
          break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:   s= "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT"; bytes=4;
          break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:   s= "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT"; bytes=4;
          break;
      }
      Write_Debug("%4d %50s [%4ld x %4ld x %1d = %'12ld, (%s)]\n", i, textureName.c_str(),  W, H, bytes, W*H*bytes, s);
    }
  }
  return DC_HANDLED;
}

#endif

struct VehPrintStats
{
	FILE* fp;
	int total;
	int notobj;
	int ob;
	int hidden;
	int skip;
	int visible;
	int nonup;
	int ode;
	int mis;

	void Reset()
	{
		total		= 0;
		notobj		= 0;
		ob			= 0;
		hidden	= 0;
		skip		= 0;
		visible	= 0;
		nonup	= 0;
		ode		= 0;
		mis		= 0;
	}
};

/************************************************************************************************
	_PrintObjectFlags
*************************************************************************************************/

static void _PrintObjectFlags(OBJECT* obj, void* s)
{
	VehPrintStats* stats = (VehPrintStats*)s;

	int notobj = (obj->flags & OBJECT_NOTOBJECT) ? 1 : 0;
	int ob = (notobj) ? 0 : 1;
	int hidden = (obj->flag2 & OBJECT_HIDDEN) ? 1 : 0;
	int skip = (obj->flags & OBJECT_SKIPDRAWING) ? 1 : 0;
	int visible = (obj->flags & OBJECT_VISIBLE) ? 1 : 0;
	int nonup = (obj->flag2 & OBJECT_DONT_UPDATE) ? 1 : 0;
	int ode = (obj->collider) ? 1: 0;
	int mis = (obj->flag2 & OBJECT_MISSING) ? 1 : 0;

	std::string name = obj->name;

	if(name.size() < 20)
	{
		for(int s = 0; s < 20 - (int)name.size(); s++)
			name += " ";
	}

	fprintf(stats->fp, "%s\t\to=%d no=%d vis=%d hid=%d skip=%d mis=%d noup=%d ode=%d\n", name.c_str(), ob, notobj, visible, hidden, skip, mis, nonup, ode);

	stats->total++;
	stats->ob += ob;
	stats->notobj += notobj;
	stats->visible += visible;
	stats->hidden += hidden;
	stats->skip += skip;
	stats->nonup += nonup;
	stats->ode += ode;
	stats->mis += mis;
}

static int dot_vehprint(const char* line)
{
	char callsign[10+1];
	char key[32+1];

	if(sscanf(line, "%*s %10s %32s", callsign, key) == 2)
	{
		vehVehicle* v = nullptr;
		vehGetVehicleHead(&v);

		for(; v != nullptr; v = v->getNext())
		{
			if(strcasecmp(callsign, (const char*)v->Handle) == 0)
				break;
		}

		if(v)
		{
			OBJECT* PlaneObject = v->VehicleObjectData->PlaneObject;
			std::string filename = key;
			filename += ".txt";
			VehPrintStats stats;

			stats.Reset();

			stats.fp = fopen(filename.c_str(), "w");

			if(stats.fp)
			{
				fprintf(stats.fp, "Vehicle flag stats for %s\n", v->Handle);
				OBJECT* top = nullptr;

				ogrpFindSubObjectByName(&top, PlaneObject,"visdamage");
				if(top)
				{
					fprintf(stats.fp, "\n\n########### visdamage ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				ogrpFindSubObjectByName(&top, PlaneObject,"lodhulk");
				if(top)
				{
					fprintf(stats.fp, "\n\n########### lodhulk ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				ogrpFindSubObjectByName(&top, PlaneObject,"hitmap");
				if(top)
				{
					fprintf(stats.fp, "\nr\n########### hitmap ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				ogrpFindSubObjectByName(&top, PlaneObject,"lod1");
				if(top)
				{
					fprintf(stats.fp, "\n\n########### lod1 ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				ogrpFindSubObjectByName(&top, PlaneObject,"lod2");
				if(top)
				{
					fprintf(stats.fp, "\n\n########### lod2 ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				ogrpFindSubObjectByName(&top, PlaneObject,"lod3");
				if(top)
				{
					fprintf(stats.fp, "\n\n########### lod3 ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				ogrpFindSubObjectByName(&top, PlaneObject,"lod4");
				if(top)
				{
					fprintf(stats.fp, "\n\n########### lod4 ##########\n");
					mlForEachSubObject(top, _PrintObjectFlags, &stats, 0);

					fprintf(stats.fp, "Objects: %d\n", stats.ob);
					fprintf(stats.fp, "Not Objects: %d\n", stats.notobj);
					fprintf(stats.fp, "Visibles: %d\n", stats.visible);
					fprintf(stats.fp, "Hiddens: %d\n", stats.hidden);
					fprintf(stats.fp, "Skip Drawings: %d\n", stats.skip);
					fprintf(stats.fp, "Missing: %d\n", stats.mis);
					fprintf(stats.fp, "Don't Updates: %d\n", stats.nonup);
					fprintf(stats.fp, "ODE Colliders: %d\n", stats.ode);

					stats.Reset();
				}

				fclose(stats.fp);
			}

			return DC_HANDLED;
		}
	}

	// feedback
	uiChatPrintLine("You must specify a vehicle handle to print!");

    return DC_HANDLED;
}

// Used for .report and .bug, which need extra tinkering before the command is sent.
static int dot_clientinfo_command(const wchar_t *line)
{
    char *encodedText = ChatClientInfo().encode();
#if platOS == OS_WIN32
	swprintf(_LastCommand, (sizeof(_LastCommand) / sizeof(_LastCommand[0])) - 1, L"\x01%S\x01.%ls", encodedText, line);
#else
    swprintf(_LastCommand, (sizeof(_LastCommand) / sizeof(_LastCommand[0])) - 1, L"\x01%s\x01.%ls", encodedText, line);
#endif
    return DC_PASS_USE_LAST;
}

static int dot_host_command(const wchar_t* line)
{
	wchar_t cmd[256] = L"";

	int argc = swscanf(line, L"%255ls", cmd);
	if(argc == 0) return DC_PASS_TO_HOST;

	const wchar_t* pcmd = cmd;

	dotCmdMap::iterator it = _dotCommands.begin();

	for(; it != _dotCommands.end(); ++it)
	{
		if(it->second.locCmd && wcsncasecmp(pcmd, it->second.locCmd, DOT_COMMAND_SIZE) == 0)
		{
			// make the command english
			std::wstring command = L"/";
			command += it->first;

			// reset the pointer
			pcmd = line;

			// search for the rest of the command, if any
			pcmd = wcsstr(pcmd, L" ");
			if(pcmd) command += pcmd;

			// save this as the last command
			wcsncpy(_LastCommand, command.c_str(), 255);

			// causes the value of _LastCommand to be sent to the host
			// instead of the original slash commands
			return DC_PASS_USE_LAST;
		}
	}

	return DC_PASS_TO_HOST;
}

#include <string>
#include <map>
#include <vector>
#include <algorithm>

#include "phui/Util.h"
#include "ui/uiDotCommands.h"
#include "ui/uiMain.h"

// ascii callback
void uiDotCommandAdd(const char* cmdName, dotCmdFunctionA ascii, bool key)
{
	dotCmdInfo info;

	info.asciiFunc	= ascii;
	info.locCmd		= nullptr;
	info.help		= nullptr;
    info.key		= key;

	// convert it to a wide string for easy lookup later
	// incoming chats are wide strings
	_dotCommands[uiUTF8toUTF16(cmdName)] = info;
}

// wide string callback
void uiDotCommandAdd(const char* cmdName, dotCmdFunctionW wide, bool key)
{
	dotCmdInfo info;

	info.wideFunc	= wide;
	info.locCmd		= nullptr;
	info.help		= nullptr;
    info.key		= key;

	// convert it to a wide string for easy lookup later
	// incoming chats are wide strings
	_dotCommands[uiUTF8toUTF16(cmdName)] = info;
}

int uiDotCommand(const char* line)
{
	return uiDotCommand(uiUTF8toUTF16(line));
}

int uiDotCommand(const wchar_t* line)
{
	if(!line || (line[0] != L'.' && line[0] != L'/' && line[0] != L'\u3002'))
		return DC_PASS_TO_HOST;

	wchar_t cmd[DOT_COMMAND_SIZE+1] = L"";
	wchar_t* pcmd = nullptr;
	int result = DC_PASS_TO_HOST;

	if(swscanf(line, L"%255ls", cmd) == 0)
		return DC_PASS_TO_HOST;

	// strip slash or dot
	pcmd = cmd + 1;

	// look for an english version
	dotCmdMap::iterator it = _dotCommands.find(pcmd);

	// english not found, try a localized version
	if(it == _dotCommands.end())
	{
		it = _dotCommands.begin();

		for(; it != _dotCommands.end(); ++it)
		{
			if(it->second.locCmd && wcsncasecmp(pcmd, it->second.locCmd, DOT_COMMAND_SIZE) == 0)
				break; // found one!
		}
	}

    if(it != _dotCommands.end() && (!it->second.key || (it->second.key && true)))
    {
        if(wcsncasecmp(pcmd, L"doover", DOT_COMMAND_SIZE) != 0)
			wcsncpy(_LastCommand, line, 255);

		if(it->second.asciiFunc)
		{
			const char* lineA = uiUTF16toUTF8(line);
			result = it->second.asciiFunc(lineA+1); // +1 strips slash or dot
		}
		else
		{
			result = it->second.wideFunc(line+1); // +1 strips slash or dot
		}
    }

	return result;
}

const wchar_t* uiDotCommandMatch(const char* search, bool again)
{
	return uiDotCommandMatch(uiUTF8toUTF16(search), again);
}

// previous version of function was causing a CTD when you hit TAB in chat (OSG version)
const wchar_t* uiDotCommandMatch(const wchar_t* search, bool again)
{
	static unsigned int possiblesIndex = 0; // our current possible index

	static std::vector<std::wstring> possibles; // possible matches
	static int commandType = UICT_CHAT; // what we last searched for
	static std::wstring match = L"";

	match = L"";

	if(again && possibles.size())
	{
		++possiblesIndex;

		if(possiblesIndex >= possibles.size())
			possiblesIndex = 0;

		match = possibles[possiblesIndex];
		match += L" ";
	}
	else
	{
		match = search;

		commandType = UICT_CHAT;
		possiblesIndex = 0;
		possibles.clear();

		if(match.size() > 1 && (match[0] == L'/' || match[0] == L'.' || match[0] == L'\u3002'))
		{
			commandType = (match[0] == '/') ? UICT_SLASH: UICT_DOT;
			match.erase(match.begin()); // get rid of the / or .

			unsigned int matchSize = (unsigned int)match.size();

			if(matchSize)
			{
				dotCmdMap::iterator it = _dotCommands.begin();

				for(; it != _dotCommands.end(); ++it)
				{
					//dotCmdInfo& info = it->second;

					// only auto-complete debug commands if they have
					// a keyfile
					if(it->second.key && !HasKeyFile)
						continue;

					const wchar_t* command = it->first.c_str();
					unsigned int cmdSize = (unsigned int)wcslen(command);

					if(cmdSize >= matchSize) // worth taking a look?
					{
						unsigned int matchedSize = 0;

						for(unsigned int s = 0; s < matchSize; s++)
						{
							if(tolower(match[s]) != tolower(command[s]))
								break;

							++matchedSize;
						}

						if(matchedSize == matchSize)
							possibles.push_back(command);
					}

					// lets check any localized versions
					command = it->second.locCmd;
					if(!command) continue;

					cmdSize = (unsigned int)wcslen(command);

					if(cmdSize >= matchSize) // worth taking a look?
					{
						unsigned int matchedSize = 0;

						for(unsigned int s = 0; s < matchSize; s++)
						{
							if(tolower(match[s]) != tolower(command[s]))
								break;

							++matchedSize;
						}

						if(matchedSize == matchSize)
							possibles.push_back(command);
					}
				}

				if(Online)
				{
					// now we check our host commands if any were sent down
					std::wstring hostCommand = L"";

					for(size_t dc = 0; dc < Chat::g_hostDotCommands.size(); dc++)
					{
						Chat::HostCommand& info = Chat::g_hostDotCommands.at(dc);

						// only show playnet only commands if they have the right flag
						if(info.playnetOnly && !PlayerClient()->IsPermitted(IS_PLAYNET))
							continue;

						hostCommand = UTF16(info.command.c_str());

						const wchar_t* command = hostCommand.c_str();
						unsigned int cmdSize = (unsigned int)wcslen(command);

						if(cmdSize >= matchSize) // worth taking a look?
						{
							unsigned int matchedSize = 0;

							for(unsigned int s = 0; s < matchSize; s++)
							{
								if(tolower(match[s]) != tolower(command[s]))
									break;

								++matchedSize;
							}

							if(matchedSize == matchSize)
							{
								// check for duplicates because some commands can exist on both
								// the client and the host
								bool found = false;

								for(size_t p = 0; p < possibles.size(); p++)
								{
									if(wcsncasecmp(command, possibles.at(p).c_str(), DOT_COMMAND_SIZE) == 0)
									{
										found = true;
										break;
									}
								}

								if(!found)
									possibles.push_back(command);
							}
						}
					}
				}
			}

			// we found some matches, use the best (asc)
			if(possibles.size())
			{
				// sort possibles in alphabetical order
				std::sort(possibles.begin(), possibles.end());

				match = possibles[possiblesIndex];
				match += L" ";
			}
		}
	}

	switch(commandType)
	{
		case UICT_SLASH:
			match.insert(0, L"/");
			break;
		case UICT_DOT:
			match.insert(0, L".");
			break;
		default:
			// do nothing
			break;
	}

	return match.c_str();
}

bool uiDotCommandFormat(const char* message)
{
	return uiDotCommandFormat(uiUTF8toUTF16(message));
}

bool uiDotCommandFormat(const wchar_t* message)
{
	if(message[0] != L'.' && message[0] != L'/' && message[0] != L'\u3002') return false;
	if(message[0] == message[1] || message[1] == 0 || message[1] == L' ') return false;
	if(message[2] == L'!') return false;	// so /s! works
	return true;
}

const wchar_t* uiDotCommandLast()
{
	return _LastCommand;
}

piVECTOR& uiDotGetPosXYZ()
{
	return posxyz;
}

void uiDotCommandsInit()
{
    // ADD_DOT_COMMAND("command", needskey)
    // needskey means gizmo.key
	uiDotCommandAdd("remove", dot_remove, false); // couldn't use ADD_DOT_COMMAND because it conflicts with a function

	// public dot commands
	ADD_DOT_COMMAND(help, false);
#ifdef MULTI_CLIENT
	ADD_DOT_COMMAND(mc, false);
#endif
	ADD_DOT_COMMAND(conv, false);
	ADD_DOT_COMMAND(emote, false);
	ADD_DOT_COMMAND(em, false);
	ADD_DOT_COMMAND(join, false);
	ADD_DOT_COMMAND(j, false);
	ADD_DOT_COMMAND(autoleader, false);
	ADD_DOT_COMMAND(recruit, false);
	ADD_DOT_COMMAND(recruiting, false);
	ADD_DOT_COMMAND(decline, false);
	ADD_DOT_COMMAND(accept, false);
	ADD_DOT_COMMAND(leave, false);
	ADD_DOT_COMMAND(create, false);
	ADD_DOT_COMMAND(disband, false);
	ADD_DOT_COMMAND(promote, false);
	ADD_DOT_COMMAND(demote, false);
	ADD_DOT_COMMAND(resign, false);
	ADD_DOT_COMMAND(oicvolunteer, false);
	ADD_DOT_COMMAND(oicnominate, false);
	ADD_DOT_COMMAND(oicveto, false);
	ADD_DOT_COMMAND(oicresign, false);
	ADD_DOT_COMMAND(oicreplace, false);
	ADD_DOT_COMMAND(oicremove, false);
	ADD_DOT_COMMAND(hccp, false);
	ADD_DOT_COMMAND(time, false);
	ADD_DOT_COMMAND(invites, false);
	ADD_DOT_COMMAND(graduate, false);
	ADD_DOT_COMMAND(ammo, false);
    ADD_DOT_COMMAND(rain, false);
	ADD_DOT_COMMAND(benchremagen, false);
	ADD_DOT_COMMAND(benchantwerp, false);
	ADD_DOT_COMMAND(benchvehicles, false);
	ADD_DOT_COMMAND(vcache, false);
	ADD_DOT_COMMAND(vehprint, false);
	ADD_DOT_COMMAND(benchinfantry, false);

#if RELEASE_CLIENT == 0
	ADD_DOT_COMMAND(dev, false);
#endif

	// floater commands
#ifdef GM_RELEASE
	ADD_DOT_COMMAND(move, false);
	ADD_DOT_COMMAND(follow, false);
	ADD_DOT_COMMAND(inflod, false);
	ADD_DOT_COMMAND(vehlod, false);
#endif

	// beta release commands
#ifdef BETA_RELEASE
	uiDotCommandAdd("float", dot_float, false); // couldn't use ADD_DOT_COMMAND because of float keyword
	uiDotCommandAdd("log", dot_log, false); // couldn't use ADD_DOT_COMMAND because of log is a function name

#ifdef MAKE_HOST_TERRAIN
	ADD_DOT_COMMAND(hostterrain, false);
#endif

	ADD_DOT_COMMAND(imapkey, false);
    ADD_DOT_COMMAND(imapdump, false);
    ADD_DOT_COMMAND(imapinit, false);
    ADD_DOT_COMMAND(fuze, true);
	ADD_DOT_COMMAND(discohost, false);
	ADD_DOT_COMMAND(discochat, false);
	ADD_DOT_COMMAND(doover, true);
	ADD_DOT_COMMAND(damage, true);
	ADD_DOT_COMMAND(spall, true);
	ADD_DOT_COMMAND(concuss, false);
	ADD_DOT_COMMAND(shrap, false);
	ADD_DOT_COMMAND(vol, true);
	ADD_DOT_COMMAND(mass, true);
	ADD_DOT_COMMAND(leak, true);
	ADD_DOT_COMMAND(wttest, true);
	ADD_DOT_COMMAND(hitmap, true);
	ADD_DOT_COMMAND(moreammo, true);
	ADD_DOT_COMMAND(c, false);
	ADD_DOT_COMMAND(infcoltest, false);
	ADD_DOT_COMMAND(infstatetest, true);
	ADD_DOT_COMMAND(infanimtest, false);
	ADD_DOT_COMMAND(infautoreload, false);
	ADD_DOT_COMMAND(inertia, true);
	ADD_DOT_COMMAND(exhaust, true);
	ADD_DOT_COMMAND(reset, true);
    ADD_DOT_COMMAND(tree, true);
	ADD_DOT_COMMAND(die, true);
	ADD_DOT_COMMAND(texture, true);
	ADD_DOT_COMMAND(hulkall, true);
	ADD_DOT_COMMAND(hulk, true);
	ADD_DOT_COMMAND(fpsanimspeed, true);
	ADD_DOT_COMMAND(tutorial, false);
	ADD_DOT_COMMAND(lua, true);
	ADD_DOT_COMMAND(whereami, true);
	ADD_DOT_COMMAND(setextcam, true);
	ADD_DOT_COMMAND(setexttar, true);
	ADD_DOT_COMMAND(movevehicle, true);
	ADD_DOT_COMMAND(vspawn, true);
	ADD_DOT_COMMAND(spawn, true);
#ifdef GR2_BLEND_EDITOR
	ADD_DOT_COMMAND(tspawn, false);
#else
	ADD_DOT_COMMAND(tspawn, true);
#endif
	ADD_DOT_COMMAND(bdamage, true);
	ADD_DOT_COMMAND(builder, true);
	ADD_DOT_COMMAND(fogon, true);
	ADD_DOT_COMMAND(fuze, true);
	ADD_DOT_COMMAND(clutter, true);
	ADD_DOT_COMMAND(drawunsorted, true);
	ADD_DOT_COMMAND(drawsorted, true);
	ADD_DOT_COMMAND(drawmultitexture, true);
	ADD_DOT_COMMAND(drawsortedcockpit, true);
	ADD_DOT_COMMAND(drawunsortedcockpit, true);
	ADD_DOT_COMMAND(terraindraw, true);
	ADD_DOT_COMMAND(radclutter, true);
	ADD_DOT_COMMAND(loadtrees, true);
	ADD_DOT_COMMAND(deploy, true);
	ADD_DOT_COMMAND(undeploy, true);
	ADD_DOT_COMMAND(treaddust, true);
	ADD_DOT_COMMAND(force, true);
    ADD_DOT_COMMAND(set, true);
	ADD_DOT_COMMAND(mvcmp, true);
	ADD_DOT_COMMAND(frameprof, true);
	ADD_DOT_COMMAND(televateconv, true);
	ADD_DOT_COMMAND(vehstats, true);
	ADD_DOT_COMMAND(slomo, true);
	ADD_DOT_COMMAND(rank, false);
	ADD_DOT_COMMAND(ride, true);
	ADD_DOT_COMMAND(reloadclutter, true);
    ADD_DOT_COMMAND(reloadammo, true);
	ADD_DOT_COMMAND(fpsresupply, true);
	ADD_DOT_COMMAND(turrettest, false);
	ADD_DOT_COMMAND(speed, false);
	ADD_DOT_COMMAND(checkplacement, false);
	ADD_DOT_COMMAND(trntex, true);
	ADD_DOT_COMMAND(drawoctettris, true);
	ADD_DOT_COMMAND(drawobtris, true);
	ADD_DOT_COMMAND(drawobnormals, true);
	ADD_DOT_COMMAND(drawcolnormals, true);
	ADD_DOT_COMMAND(drawbbox, true);
	ADD_DOT_COMMAND(drawtobbox, true);
    ADD_DOT_COMMAND(drawcollisionobjects, true);
	ADD_DOT_COMMAND(tps, false);
	ADD_DOT_COMMAND(printweather, true);
	ADD_DOT_COMMAND(weather, true);
	ADD_DOT_COMMAND(fog, true);
	ADD_DOT_COMMAND(reloadvehicles, true);
	ADD_DOT_COMMAND(np, false);
	ADD_DOT_COMMAND(lu, false);
	ADD_DOT_COMMAND(weaplog, true);
    ADD_DOT_COMMAND(nospawn, false);
	ADD_DOT_COMMAND(oldspawn, true);
	ADD_DOT_COMMAND(area51, true);
	ADD_DOT_COMMAND(resetspawn, true);
    ADD_DOT_COMMAND(postfilter, true);
    ADD_DOT_COMMAND(normalshader, true);
    ADD_DOT_COMMAND(specularshader, true);
    ADD_DOT_COMMAND(wingvortex, true);
    ADD_DOT_COMMAND(fire, true);
    ADD_DOT_COMMAND(altspd, true);
    ADD_DOT_COMMAND(ai, true);
    ADD_DOT_COMMAND(gizmo, true);
    ADD_DOT_COMMAND(watershader, true);
    ADD_DOT_COMMAND(waterreflection, true);
    ADD_DOT_COMMAND(supertroop, false);
	ADD_DOT_COMMAND(mappos, false);
	ADD_DOT_COMMAND(aion, false);
    ADD_DOT_COMMAND(drawparticles, true);
    ADD_DOT_COMMAND(treecull, true);
    ADD_DOT_COMMAND(addtree, true);
    ADD_DOT_COMMAND(grassshader, true);
    ADD_DOT_COMMAND(damagevehicle, true);
    ADD_DOT_COMMAND(effect, true);
    ADD_DOT_COMMAND(vehiclestate, true);
    ADD_DOT_COMMAND(cull, true);
    ADD_DOT_COMMAND(noshaderreload, true);
    ADD_DOT_COMMAND(shadowmap, false);
	ADD_DOT_COMMAND(ragdolls, true);
    ADD_DOT_COMMAND(mypos, true);
    ADD_DOT_COMMAND(speedgrass, true);
    ADD_DOT_COMMAND(timemultiplier, true);
	ADD_DOT_COMMAND(version, false);
    ADD_DOT_COMMAND(wantblood, false);
    ADD_DOT_COMMAND(damageoctet, true);
    ADD_DOT_COMMAND(resetoctet, true);
	ADD_DOT_COMMAND(setfalls, false);
	ADD_DOT_COMMAND(setfm, false);
    ADD_DOT_COMMAND(drawvbo, true);
	ADD_DOT_COMMAND(gr2memstats, false);
	ADD_DOT_COMMAND(getcell, false);
	ADD_DOT_COMMAND(buildterrain, true);
	ADD_DOT_COMMAND(drawalloff, true);
	ADD_DOT_COMMAND(bunch, false);
	ADD_DOT_COMMAND(edit, false);
    ADD_DOT_COMMAND(wtfamistandingon, false);
	ADD_DOT_COMMAND(decal, false);
	ADD_DOT_COMMAND(vcount, false);
    ADD_DOT_COMMAND(duplicate, false);
    ADD_DOT_COMMAND(drawheightmap, true);
	ADD_DOT_COMMAND(ppo, false);
	ADD_DOT_COMMAND(bailout, false);
    ADD_DOT_COMMAND(benchbuilding, false);
	ADD_DOT_COMMAND(gr2debug, false);
	ADD_DOT_COMMAND(allinf, false);
	ADD_DOT_COMMAND(inflodmod, false);
	ADD_DOT_COMMAND(js, false);
    ADD_DOT_COMMAND(genscenariomap, true);
#if platOS != OS_MACOS
    ADD_DOT_COMMAND(audiochatlogging, true);
    ADD_DOT_COMMAND(audiowritelog, true);
    ADD_DOT_COMMAND(audiomindistance, true);
#endif

	ADD_DOT_COMMAND(scenario, true);
	ADD_DOT_COMMAND(ez, false);
	ADD_DOT_COMMAND(conn, true);

    ADD_DOT_COMMAND(enablemouseclicktodamagebuildings, true);
	ADD_DOT_COMMAND(ortho, true);

	ADD_DOT_COMMAND(alert, false);
    ADD_DOT_COMMAND(view, false);
    
#if platOS != OS_MACOS
    ADD_DOT_COMMAND(heightmap, true);
#endif

#ifdef GR2_TEXTURE_DEBUG
	ADD_DOT_COMMAND(gr2textures, false);
#endif

#ifdef GR2_BLEND_EDITOR
	ADD_DOT_COMMAND(blendeditor, false);
	ADD_DOT_COMMAND(kit, false);
#endif

    ADD_DOT_COMMAND(gencpslist, true);

#ifdef SPATIAL_DB
    ADD_DOT_COMMAND(drawsdb, true);
#endif

#ifdef GENICONS
    ADD_DOT_COMMAND(genicons, true);
    ADD_DOT_COMMAND(gentiles, true);
#endif

#ifdef MODEL_INFO
	ADD_DOT_COMMAND(texinfo, false);
#endif

  //  ADD_DOT_COMMAND(scspawn, true);

#if RELEASE_CLIENT == 0
	//ADD_DOT_COMMAND(font, false);
	ADD_DOT_COMMAND(canvas, false);
#endif
#endif

	ADD_DOT_COMMAND(lognote, false);

	// host commands; this allows for
	// client localization of host commands
	ADD_HOST_COMMAND(who, false);

    // allows extra info to be sent with these commands
    uiDotCommandAdd("bug", dot_clientinfo_command, false);
    uiDotCommandAdd("report", dot_clientinfo_command, false);

    ADD_DOT_COMMAND(rate, true);
    ADD_DOT_COMMAND(gv, true);
    ADD_DOT_COMMAND(gav, true);
    ADD_DOT_COMMAND(grv, true);
    ADD_DOT_COMMAND(sf, true);

	uiDotCommandsInitLoc();
}

void uiDotCommandsInitLoc()
{
	dotCmdMap::iterator it = _dotCommands.begin();
	std::string locKey;

	for(; it != _dotCommands.end(); ++it)
	{
		locKey = "dotcom.";
		locKey += uiUTF16toUTF8(it->first.c_str());

		const std::wstring& loc = uiLocString(locKey.c_str());

		it->second.locCmd	= nullptr;
		it->second.help	= nullptr;

		if(loc.size() && loc[0] != L'?')
			it->second.locCmd = loc.c_str();

		locKey += ".help";

		const std::wstring& help = uiLocString(locKey.c_str());

		if(help.size() && help[0] != L'?')
			it->second.help = help.c_str();
	}
}

void uiDotCommandAddClone(int id)
{
	vehicles.insert(id);
}

