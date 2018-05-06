#include "syscfg.h" // Path is project specific.
#include <string.h>
#include <algorithm>
#include <locale>

#include "steam_api.h" // HACK

#include "phui/Util.h"

#include "include/vars.h"
#include "auth/authSteam.h"
#include "sys/sysSteam.h"
#include "js/jsClient.h"
#include "js/jsStratDB.h"
#include "js/jsGame.h"
#include "wwii/wwiiTypes.h"
#include "mm/mmEnums.h"
#include "miss/missionTypes.h"
#include "strt/strtTypes.h"
#include "veh/vehtype.h"
#include "js/jsVehicle.h"
#include "ui/uiChat.h"
#include "chat/chatHandle.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatChannelTypes.h"			// For NO_CHANNEL, etc...
#include "map/mapBrigadeSummary.h"
#include "mm/mmSpawnTypes.h"
#include "chat/chatClientMessages.h"
#include "strt/strtFacilityWeaponList.h"
#include "uimap/uiMap.h"
#include "client/GameClient.h"
#include "client/clientNetError.h"
#include "ui/uiMain.h"
#include "js/jsAuthHandler.h"
#include "js/jsPosition.h"
#include "wwii/wwiiSquadMsg.h"
#include "chat/chatSquadControl.h"
#include "chat/chatWaypoint.h"
#include "map/mapArenaDescription.h"
#include "chat/chatMotds.h"
#include "chat/chatEditStringMsg.h"
#include "map/mapClientMessages.h"
#include "ui/uiDotCommands.h"
#include "ui/uiMain.h"
#include "veh/vehmain.h"
#include "strt/strtCPFlags.h"
#include "chat/chatBrigadeControlList.h"
#include "client/clientBrigadeControlDescriptors.h"
#include "chat/chatMissionInvite.h"
#include "miss/missionResults.h"
#include "tut/TutorialManager.h"
#include "inst/instClient.h"
#include "inst/instDescriptor.h"
#include "inst/instTypes.h"
#include "inst/instListing.h"
#include "wwii/wwiiPermissions.h"
#include "weap/weapWeapon.h"
#include "strt/strtCaptureState.h"
#include "pers/persmain.h"
#include "scen/ScenarioManager.h"
#include "rn/rnmain.h"

#ifdef GR2_BLEND_EDITOR
#include "gr2/gr2Anim.h"
#endif

using namespace std;

static int _chatRateSamples		= 0;
static int _chatRateThreshold		= (2 * 60);
static int _chatRatePenalty		= (10 * 60);

static const char* branches[MAX_BRANCHES] = {"none", "army", "airforce", "navy" };

/************************************************************************************************
	jsClient
*************************************************************************************************/

JSClass jsClient::jsclass = JS_CLASS_MACRO("client");
JSFunctionSpec jsClient::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsClient, createauthhandler,0),
	JS_FUNCTION_SPEC(jsClient, connecttoarena,0),
	JS_FUNCTION_SPEC(jsClient, startofflinemode,0),
	JS_FUNCTION_SPEC(jsClient, issteamcopy,0),
	JS_FUNCTION_SPEC(jsClient, steamconnect,2),
	JS_FUNCTION_SPEC(jsClient, loadrootwindow,0),
	JS_FUNCTION_SPEC(jsClient, getmisscountry,1),
	JS_FUNCTION_SPEC(jsClient, getmissbrigade,1),
	JS_FUNCTION_SPEC(jsClient, acceptmission,1),
	JS_FUNCTION_SPEC(jsClient, spawn,1),
	JS_FUNCTION_SPEC(jsClient, selectpersona,1),
	JS_FUNCTION_SPEC(jsClient, sendchatmessage,1),
	JS_FUNCTION_SPEC(jsClient, selectbrigade,1),
	JS_FUNCTION_SPEC(jsClient, createmission,5),
	JS_FUNCTION_SPEC(jsClient, requestweaponlist,1),
	JS_FUNCTION_SPEC(jsClient, cancelweaponreservation,0),
	JS_FUNCTION_SPEC(jsClient, reserveweapon,1),
	JS_FUNCTION_SPEC(jsClient, requestcrews,0),
	JS_FUNCTION_SPEC(jsClient, requestcommanders,0),
	JS_FUNCTION_SPEC(jsClient, joincrew,1),
	JS_FUNCTION_SPEC(jsClient, acceptcrew,1),
	JS_FUNCTION_SPEC(jsClient, requestpersonalist,0),
	JS_FUNCTION_SPEC(jsClient, requestmissionlist,0),
	JS_FUNCTION_SPEC(jsClient, requestquickmissionlist,0),
	JS_FUNCTION_SPEC(jsClient, exitmission,0),
	JS_FUNCTION_SPEC(jsClient, cancelandexitmission,0),
	JS_FUNCTION_SPEC(jsClient, requestbrigadedetail,1),
	JS_FUNCTION_SPEC(jsClient, requestmotd, 1),
	JS_FUNCTION_SPEC(jsClient, createsquad, 6),
	JS_FUNCTION_SPEC(jsClient, canaddwaypoint, 0),
	JS_FUNCTION_SPEC(jsClient, canaddreport, 0),
	JS_FUNCTION_SPEC(jsClient, setmotd, 3),
	JS_FUNCTION_SPEC(jsClient, undeployms, 0),
	JS_FUNCTION_SPEC(jsClient, requestmissiondetail, 1),
	JS_FUNCTION_SPEC(jsClient, acceptoicnomination, 2),
	JS_FUNCTION_SPEC(jsClient, findbrigade, 1),
	JS_FUNCTION_SPEC(jsClient, requestrecruiters, 1),
	JS_FUNCTION_SPEC(jsClient, enabledespawntimer, 1),
	JS_FUNCTION_SPEC(jsClient, selectinstance, 5),
	JS_FUNCTION_SPEC(jsClient, savepersona, 0),
	JS_FUNCTION_SPEC(jsClient, getpersona, 2),
	JS_FUNCTION_SPEC(jsClient, getrank, 3),
	JS_FUNCTION_SPEC(jsClient, listinstances,3),
	JS_FUNCTION_SPEC(jsClient, createinstance,2),
	JS_FUNCTION_SPEC(jsClient, joininstance,4),
	JS_FUNCTION_SPEC(jsClient, isavailable, 0),
	JS_FUNCTION_SPEC(jsClient, ismissionleader, 0),
	{0}
};

JSPropertySpec jsClient::jsproperties[] =
{
	{"country",				jsClient::COUNTRY_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"branch",				jsClient::BRANCH_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"handle",				jsClient::HANDLE_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"missions",			jsClient::MISSIONS_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"personas",			jsClient::PERSONAS_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"brigade",				jsClient::BRIGADE_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"reservedweapon",		jsClient::RESERVEDWEAPON_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"brigades",			jsClient::BRIGADES_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"brigades",			jsClient::BRIGADES_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"chatchannels",		jsClient::CHATCHANNELS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"spawnablevehicles",	jsClient::SPAWNABLE_VEHICLES_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"weaponcounter",		jsClient::WEAPONCOUNTER_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"missionroomlist",		jsClient::MISSIONROOMLIST_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"personalisttime",		jsClient::PERSONALISTTIME_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"sidelocked",			jsClient::SIDELOCKED_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"currentmission",		jsClient::CURRENTMISSION_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"missionresults",		jsClient::MISSIONRESULTS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"despawntimer",		jsClient::DESPAWNTIMER_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"canpostmission",		jsClient::CANPOSTMISSION_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"persona",				jsClient::PERSONA_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"missionlistupdate",	jsClient::MISSIONLISTUPDATE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"lastspawnliststatus",	jsClient::LASTSPAWNLISTSTATUS_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"lastspawnlistsource",	jsClient::LASTSPAWNLISTSOURCE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"activemission",		jsClient::ACTIVEMISSION_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"lastspawnliststate",	jsClient::LASTSPAWNLISTSTATE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"squad",				jsClient::SQUAD_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"squadinvite",			jsClient::SQUADINVITE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"currentorders",		jsClient::CURRENTORDERS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"isplaynet",			jsClient::ISPLAYNET_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"sidebalance",			jsClient::SIDEBALANCE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"sidedelay",			jsClient::SIDEDELAY_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"poll",				jsClient::POLL_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"spawndelay",			jsClient::SPAWNDELAY_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"ismobilespawn",		jsClient::ISMOBILESPAWN_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"cansto",				jsClient::CANSTO_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"brigadenotices",		jsClient::BRIGADENOTICES_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"ishc",				jsClient::ISHC_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"isoic",				jsClient::ISOIC_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"oicof",				jsClient::OICOF_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"havenotices",			jsClient::HAVEBRIGADENOTICES_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"recruiter",			jsClient::RECRUITER_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"recruiters",			jsClient::RECRUITERS_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"haverecruiters",		jsClient::HAVERECRUITERS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"missioninvite",		jsClient::MISSIONINVITE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"nextsupply",			jsClient::NEXTSUPPLY_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"instance",			jsClient::INSTANCE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"state",				jsClient::STATE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"capturestate",		jsClient::CAPTURESTATE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"instances",			jsClient::INSTANCES_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsClient
*************************************************************************************************/

jsClient::jsClient(ww2::Client* c) :
			phui::js::Object(&jsclass, jsfunctions, jsproperties), client(c)
{
	JS_CONSTRUCTOR_LOCK;
	missions = new phui::js::Array;
	spawnablevehicles = new phui::js::Array;
	weaponcounter = new phui::js::Array;
	personas = new phui::js::Array;
	brigades = new phui::js::Array;
	missionroomlist = new phui::js::Array;
	brigade = new jsBrigade(BrigadeDetail());
	chatchannels = new phui::js::Array;
	brigadenotices = new phui::js::Array;
	recruiters = new phui::js::Array;
	instances = new phui::js::Array;

	instance = new jsInstance;
	state = new jsClientState;
	capturestate = new jsCaptureState;

	for(UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
		chatchannels->SetElement(new jsChatChannel(i), i);

	lastmissionresults = 0;
	brigadenoticecount = 0;
	nextsupply = 0;
};

jsClient::~jsClient()
{
}

/************************************************************************************************
	auth connection
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, createauthhandler) {
	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx, jsobj);
	if ( ! jsclient ) return JS_FALSE;

	// HACK: At some point it would be nice to not have the controllers pointing to a global
	// connection and actually have that connection be attached to the client or something.
	*rval = (new jsAuthHandler())->jsVal();
	return JS_TRUE;
}

/************************************************************************************************
	arena joining
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, connecttoarena) {
	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx, jsobj);
	if ( ! jsclient ) return JS_FALSE;

	try {
		// HACK: We should pass in the auth info, but for now it's set semi-globally.
		rnconnectToServer();
	} catch ( ClientNetError& ) {
		// TODO: Error message
		*rval = BOOLEAN_TO_JSVAL(false);
	}

	*rval = BOOLEAN_TO_JSVAL(true);
	return JS_TRUE;
}

/************************************************************************************************
	arena joining
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, startofflinemode) {
	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx, jsobj);
	if ( ! jsclient ) return JS_FALSE;
	rnstartOfflineMode();
	return JS_TRUE;
}

/************************************************************************************************
	checking steam
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, issteamcopy) {
	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx, jsobj);
	if ( ! jsclient ) return JS_FALSE;

	*rval = BOOLEAN_TO_JSVAL(IsSteamCopy());

	return JS_TRUE;
}

/************************************************************************************************
	steam connection
*************************************************************************************************/

// TODO: Move this out of the GUI code
namespace {
	bool IsValidEmail(const char* email) {
		// TEMP: Validating emails meaningfully is arbitrarily difficult.
        // Note that even billing only does this:
        // email+          = ^[^\s]+\@[^\s]+$
        bool hasAt = false;
        bool hasDot = false;
        
		for ( const char* c = email ; *c != '\0' ; ++c ) {
			if ( !std::isprint(*c, std::locale::classic()) ) {
				return false;
			}
            if (*c == '@') { hasAt = true; }
            if (*c == '.') { hasDot = true; }
		}

		return hasAt && hasDot;
	}

	bool IsValidCallsign(const char* callsign) {
		if ( strlen(callsign) < 2 || strlen(callsign) > 10)
			return false;

		for ( const char* c = callsign ; *c != '\0' ; ++c ) {
			if ( !std::isalnum(*c, std::locale::classic()) ) {
				return false;
			}
		}

		return true;
	}
}

namespace {
	// The point in the login process we're at.
	enum { LoginReady, LoginNewUser, LoginNeedEmail } steam_login_state = LoginReady;

	void UpdateLoginState(SteamLoginResponse res) {
		switch ( res ) {
		case SteamLoginResponse::LoginSuccess:
			steam_login_state = LoginReady;
			break;
		case SteamLoginResponse::LoginNewUser:
			assert ( steam_login_state == LoginReady );
			steam_login_state = LoginNewUser;
			break;
		case SteamLoginResponse::LoginEmailRequired:
			assert(steam_login_state == LoginReady);
			steam_login_state = LoginNeedEmail;
			break;

		// Issues/conflicts with user input; don't affect state.
		case SteamLoginResponse::LoginCallsignInvalid:
		case SteamLoginResponse::LoginCallsignDisallowed:
		case SteamLoginResponse::LoginCallsignInUse:
		case SteamLoginResponse::LoginEmailInvalid:
		case SteamLoginResponse::LoginEmailInUse:
		case SteamLoginResponse::LoginEmailNotValidated:
			break;

		// Something went wrong, reset things on our end.
		case SteamLoginResponse::LoginNotValidUser:
		case SteamLoginResponse::LoginUnknownError:
		case SteamLoginResponse::LoginExistingUser:
			steam_login_state = LoginReady;
			break;
		default:
			dbugLog(_FLW, "Unknown login response %u", res);
			steam_login_state = LoginReady;
		}
	}

	//--------------

	// Simple RAII wrapper to ensure proper setup/takedown for Steam auth tickets.
	struct SteamAuthSessionTicket {
		SteamAuthSessionTicket(void)
		{ handle = SteamUser()->GetAuthSessionTicket(data, array_size(data), &length); }
		~SteamAuthSessionTicket() { SteamUser()->CancelAuthTicket(handle); }

		HAuthTicket handle;
		char data[512];
		uint32_t length;
	};

	SteamLoginResult AttemptSteamLogin(uint32_t arena, const char* callsign, const char* email) {
		// Some quick pre-validation to avoid unnecessarily pestering auth.
		if ( (steam_login_state == LoginNewUser || steam_login_state == LoginNeedEmail) &&
		     ! IsValidEmail(email) ) {
			return {SteamLoginResponse::LoginEmailInvalid};
		}
		if ( steam_login_state == LoginNewUser && ! IsValidCallsign(callsign) ) {
			return {SteamLoginResponse::LoginCallsignInvalid};
		}

		//------

		SteamAuthSessionTicket tkt;
		switch ( steam_login_state ) {
		case LoginReady:
			return authRequestSteamLogin(tkt.data, tkt.length, arena);
		case LoginNewUser:
			return authRequestSteamSignup(tkt.data, tkt.length, arena, callsign, email);
		case LoginNeedEmail:
			return authRequestSteamEmailChange(tkt.data, tkt.length, arena, email);
		default:
			assert ( false );
			return {SteamLoginResponse::LoginUnknownError};
		}
	}
}

JS_FUNCTION_DEF(jsClient, steamconnect) {
	assert ( IsSteamCopy() );

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx, jsobj);
	if ( ! jsclient ) return JS_FALSE;
	if ( argc < 1 ) return JS_FALSE;

	uint32_t arena;
	if ( gArena[0] ) // Overridden by command line
		arena = static_cast<uint32_t>(strtol(gArena, nullptr, 10));
	else {
		int32 jsarena;
		JS_ValueToInt32(ctx, argv[0], &jsarena);
		arena = static_cast<uint32_t>(jsarena);
	}

	const char* email = "";
	if ( argc >= 2 )
		email = JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));

	const char* callsign = "";
	if ( argc >= 3 )
		callsign = JS_GetStringBytes(JS_ValueToString(ctx, argv[2]));

	//--------------

	SteamLoginResult result = AttemptSteamLogin(arena, callsign, email);
	UpdateLoginState(result.status);

	if ( result.status == SteamLoginResponse::LoginSuccess )
		rnsetServerInfo(result.serverIP, result.serverPort, result.otp);

	*rval = INT_TO_JSVAL(result.status);
	return JS_TRUE;
}


/************************************************************************************************
	gui state
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, loadrootwindow) {
	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx, jsobj);
	if ( ! jsclient ) return JS_FALSE;

	rnloadRootWindow();

	return JS_TRUE;
}


/************************************************************************************************
	mission info retrieval
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, getmisscountry) {
	*rval = INT_TO_JSVAL(-1);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if ( argc < 1 ) return JS_FALSE;
	if ( ! jsclient ) return JS_FALSE;

	int32 miss_id;
	JS_ValueToInt32(ctx, argv[0], &miss_id);

	ww2::Client* client = jsclient->client;
	for ( auto it = client->Missions().begin() ; it != client->Missions().end() ; ++it ) {
		if ( it->m_missionID == miss_id ) {
			*rval = INT_TO_JSVAL(it->m_countryID);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}


JS_FUNCTION_DEF(jsClient, getmissbrigade) {
	*rval = INT_TO_JSVAL(-1);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if ( argc < 1 ) return JS_FALSE;
	if ( ! jsclient ) return JS_FALSE;

	int32 miss_id;
	JS_ValueToInt32(ctx, argv[0], &miss_id);

	ww2::Client* client = jsclient->client;
	for ( auto it = client->Missions().begin() ; it != client->Missions().end() ; ++it ) {
		if ( it->m_missionID == miss_id ) {
			*rval = INT_TO_JSVAL(it->m_brigadeID);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}


/************************************************************************************************
	acceptmission
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, acceptmission)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 missionID;
	JS_ValueToInt32(ctx, argv[0], &missionID);

	jsclient->client->RequestMissionDetail(missionID, true);
	if(jsclient->client->WaitForMessage(ww2::Client::MISSION_DETAIL_WAIT, 10) == 0)
	{
		jsclient->mission = new jsMission(jsclient->client->SelectedMission());
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		uiMapSetMissionOriginAndTarget(jsclient->client->SelectedMission().m_origin, jsclient->client->SelectedMission().m_target);
	}

	return JS_TRUE;
}

/************************************************************************************************
	acceptoicnomination
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, acceptoicnomination)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 2) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 brigadeID;
	JS_ValueToInt32(ctx, argv[0], &brigadeID);

	bool accept = JSVAL_TO_BOOLEAN(argv[1]);

	if(accept)
	{
		BrigadeControlRequest req;
		req.volunteerOIC(PlayerClient()->ChatConnection(), brigadeID);
	}
	else
	{
		// send a veto?
	}

	return JS_TRUE;
}

/************************************************************************************************
	findbrigade
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, findbrigade)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 brigadeID;
	JS_ValueToInt32(ctx, argv[0], &brigadeID);

	HCUnit* unit = HCUnit::Lookup((UINT32)brigadeID);
	if(unit)
		*rval = (new jsBrigade(brigadeID))->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	spawn
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, spawn)
{
	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	bool mobilespawn = (argc > 0) ? JSVAL_TO_BOOLEAN(argv[0]) : false;

	uiSetDespawnInfo("mission");

	jsclient->client->RequestClientSpawn(jsclient->client->WeaponSerial(),mobilespawn);

	return JS_TRUE;
}

/************************************************************************************************
	selectpersona
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, selectpersona)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsPersonaSummary* jsum = phui::js::Cast<jsPersonaSummary*>(ctx, argv[0]);
	if(!jsum) return JS_FALSE;

	if(jsclient->client->SelectPersona(jsum->Summary(), true, true))
	{
		if(SCEN_MGR->IsActive())
		{
			ScenarioPlayer* me = SCEN_MGR->GetPlayer();
			if(me) me->Set("team", wwiiSide(jsum->Summary().m_countryID));
		}

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	sendchatmessage
*************************************************************************************************/

#define CHAT_RATE_SAMPLES 25

JS_FUNCTION_DEF(jsClient, sendchatmessage)
{
	// sync's chat win back to its cycle setting - on/off
	// we call it first because this function returns early on 0 arguments
	uiRunScript("game.onchatdeactivate()");

#ifdef GR2_BLEND_EDITOR
	if(getEditModel())
		uiChatHide();
#endif

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	const wchar_t* msg = phui::js::ValueToWideString(argv[0]);
	if(!msg) return JS_FALSE;

	if(msg[0] == L'\0') return JS_TRUE;

	const char* cmsg = uiUTF16toUTF8(msg);

	TUT_MGR->DoEvent(TUT_EVENT_CHAT, cmsg);

	// check for rate limiting
	{
		static int nextAllowedChat = 0;
		int icons[CHATICON_MAX] = {-1,-1,IDE_ICON_CAUTION};

		if(frametime < nextAllowedChat)
		{
			uiChatPrintUILocLine(icons, colorYELLOWALPHA, "chat.ratelimited");
			return JS_TRUE;
		}
		else if(_chatRateSamples)
		{
			static int lastChat = 0;
			static int chatSamples[CHAT_RATE_SAMPLES]{};

			if(_chatRateSamples > CHAT_RATE_SAMPLES)
				_chatRateSamples = CHAT_RATE_SAMPLES;

			if(!lastChat)
			{
				memset(chatSamples, 0x0, sizeof(chatSamples));
			}
			else
			{
				for(int i = 1; i < _chatRateSamples; i++)
					chatSamples[i - 1] = chatSamples[i];
			}

			chatSamples[_chatRateSamples - 1] = frametime - lastChat;
			lastChat = frametime;

			int matched = 0;

			for(int i = 0; i < _chatRateSamples; i++)
			{
				if(chatSamples[i] && chatSamples[i] <= (_chatRateThreshold * 1000))
					++matched;
			}

			if(matched == _chatRateSamples)
			{
				nextAllowedChat = frametime + (_chatRatePenalty * 1000);
				uiChatPrintUILocLine(icons, colorYELLOWALPHA, "chat.ratelimited");
				return JS_TRUE;
			}
		}
	}

	if(uiDotCommandFormat(msg))
	{
		int result = uiDotCommand(msg);
		if(result <= DC_HANDLED) return JS_TRUE;

		// if we're online, set msg to the last command
		// because it may have been updated to the english
		// variant for processing by the host
		// this allows us to localize host commands on the
		// client
		if(Online && result == DC_PASS_USE_LAST)
        {
			msg = uiDotCommandLast();
            cmsg = uiUTF16toUTF8(msg);
        }
	}

	unsigned int txmask = 0;
	for(int i = 0; i < MAX_CLIENT_CHANNELS; i++)
	{
		if(ChatChannels[i].xmit)
			txmask |= (1<<i);
	}

	jsclient->client->SendChatMessage(txmask, cmsg);

	return JS_TRUE;
}

/************************************************************************************************
	selectbrigade
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, selectbrigade)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	UINT32 brigadeID = 0;

	jsBrigade* jsbrig = phui::js::Cast<jsBrigade*>(ctx, argv[0]);

	if(jsbrig)
		brigadeID = jsbrig->Detail().brigadeID();
	else
		JS_ValueToInt32(ctx, argv[0], (int32*)&brigadeID);  // check to see if we passed in the id itself

	if(jsclient->client->SelectBrigade(brigadeID, true, true))
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	selectinstance
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, selectinstance)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online || uiInGame() || argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	bool success = false;
	int32 category = 1; // instance category, defaults to campaign
	int32 type = 0; // instance subtype

	JS_ValueToInt32(ctx, argv[0], &category);

	if(argc > 1)
			JS_ValueToInt32(ctx, argv[1], &type);

	if(jsclient->client->SelectInstance((UINT32)category, (UINT32)type))
	{
		int32 country	= -1;
		int32 branch	= -1;

		if(argc > 2)  JS_ValueToInt32(ctx, argv[2], &country);
		if(argc > 3)  JS_ValueToInt32(ctx, argv[3], &branch);

		const PersonaSummary* persona = jsclient->client->GetPersona(country, branch);

		if(persona && jsclient->client->SelectPersona(*persona))
		{
			int32 brigade = persona->m_brigadeID;

			if(argc > 4)
				JS_ValueToInt32(ctx, argv[5], &brigade);

			if(jsclient->client->SelectBrigade(brigade))
				success = true;
		}
		else if(!persona)
		{
			// if we don't have a persona selected and we got to here,
			// consider it a success because we were able to select the instance
			success = true;
		}
	}

	if(success)
	{
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}
	else
	{
		jsclient->client->LoadSavedPersona(true);
	}

	return JS_TRUE;
}

/************************************************************************************************
	getinstances
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, listinstances)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online || uiInGame() || argc < 1)
		return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 category	= 1;	// instance category, defaults to campaign
	int32 type		= 0;	// instance subtype
	UINT64 owner	= 0;	// owning player

	JS_ValueToInt32(ctx, argv[0], &category);

	if(argc > 1) JS_ValueToInt32(ctx, argv[1], &type);
	if(argc > 2) owner = (UINT64)chatGetHandle(phui::js::ValueToWideString(argv[2]));

	if(Instance::RequestListing((UINT32)category, (UINT32)type, owner, PlayerClient()->MapConnection()))
	{
		if(jsclient->client->WaitForMessage(ww2::Client::INSTANCE_WAIT, 10) == 0)
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	createinstance
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, createinstance)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online || uiInGame() || argc < 1)
		return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 category	= -1;	// instance category, defaults to campaign
	int32 type		= 0;	// instance subtype

	JS_ValueToInt32(ctx, argv[0], &category);
	if(argc > 1) JS_ValueToInt32(ctx, argv[1], &type);

	UINT64 owner = (UINT64)PlayerClient()->Personas().m_callsign;

	if(Instance::RequestSelect((UINT32)category, (UINT32)type, owner, 0, PlayerClient()->MapConnection()))
	{
		if(jsclient->client->WaitForMessage(ww2::Client::INSTANCE_WAIT, 10) == 0)
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	joininstance
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, joininstance)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online || uiInGame() || argc < 1)
		return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 category	= 1;	// instance category, defaults to campaign
	int32 type		= 0;	// instance subtype
	UINT64 owner	= 0;	// instance subtype
	UINT64 id		= 0;	// instance subtype

	JS_ValueToInt32(ctx, argv[0], &category);

	if(argc > 1) JS_ValueToInt32(ctx, argv[1], &type);
	if(argc > 2) owner = (UINT64)chatGetHandle(phui::js::ValueToWideString(argv[2]));
	if(argc > 3)
	{
		const char* strID = JS_GetStringBytes(JS_ValueToString(ctx, argv[3]));
		int intID = atoi(strID);
		id = (UINT64)intID;
	}

	// am I already in this instance?

	if(Instance::RequestSelect((UINT32)category, (UINT32)type, owner, id, PlayerClient()->MapConnection()))
	{
		if(jsclient->client->WaitForMessage(ww2::Client::INSTANCE_WAIT, 10) == 0)
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsClient, savepersona)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online || uiInGame() || argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->SavePersona();
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsClient, getpersona)
{
	*rval = JSVAL_NULL;

	if(!Online || uiInGame() || argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 country = -1;
	int32 branch = -1;

	if(argc > 0) JS_ValueToInt32(ctx, argv[0], &country);
	if(argc > 1) JS_ValueToInt32(ctx, argv[1], &branch);

	const PersonaSummary* summary = jsclient->client->GetPersona(country, branch);

	if(summary)
		*rval = (new jsPersonaSummary(*summary))->jsVal();

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsClient, getrank)
{
	*rval = JSVAL_NULL;

	if(!Online || uiInGame() || argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 country = 0;
	int32 branch = 0;
	int32 level = 0;

	if(argc > 0) JS_ValueToInt32(ctx, argv[0], &country);
	if(argc > 1) JS_ValueToInt32(ctx, argv[1], &branch);
	if(argc > 2) JS_ValueToInt32(ctx, argv[2], &level);

	if(country && branch && level)
		*rval =  (new jsRank((WWII_COUNTRY)country,(BRANCH_ENUM)branch,(RANK_NO)level))->jsVal();
	else
		*rval =  JSVAL_NULL;

	return JS_TRUE;
}

/************************************************************************************************
	createmission
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, createmission)
{
	*rval = INT_TO_JSVAL(-1); // error

	if(argc < 5) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsStratObject* jsorigin = phui::js::Cast<jsStratObject*>(ctx,argv[0]);
	if(!jsorigin || !jsorigin->stratobj) return JS_FALSE;

	jsStratObject* jstarget = phui::js::Cast<jsStratObject*>(ctx, argv[1]);
	if(!jstarget || !jstarget->stratobj) return JS_FALSE;

	int32 type = 0, subtype = 0;
	JS_ValueToInt32(ctx, argv[2], &type);
	JS_ValueToInt32(ctx, argv[3], &subtype);

	const wchar_t* wmissiondesc = phui::js::ValueToWideString(argv[4]);
	std::string missiondesc = UTF8(wmissiondesc);
	if(missiondesc.empty()) return JS_FALSE;

	MISSION_POST_REQ post{};

//	WWII_SIDE playerside = weapLookupSide(jsclient->client->PersonaSelected().m_countryID);

	post.missionType = (MISSION_TYPE)type;
	post.subType = (SINT8)subtype;
	post.origin = *jsorigin->stratobj->FullObjID();
	post.target = *jstarget->stratobj->FullObjID();
	post.descSize = static_cast<SINT16>(strlen(missiondesc.c_str()));
	strsafecpy(post.desc, missiondesc.c_str());

	*rval = INT_TO_JSVAL(0); // trying

	jsclient->client->RequestMissionPost(post);
	if(jsclient->client->WaitForMessage(ww2::Client::MISSION_POST_WAIT, 10) == 0)
	{
		MISSION_POST_REPLY const& reply = jsclient->client->MissionPostReply();
		if(reply.posted)
		{
			*rval = INT_TO_JSVAL(reply.missionID); // success

			// make our mission description the first set of orders
			if(!missiondesc.empty())
			{
				std::string orders = "/orders ";
				orders += missiondesc.c_str();
				PlayerClient()->SendChatMessage(0, orders.c_str());
			}
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	requestweaponlist
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestweaponlist)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsStratObject* jsstrat = phui::js::Cast<jsStratObject*>(ctx,argv[0]);
	if(!jsstrat) return JS_FALSE;

	jsclient->client->RequestMissionWeapList(*jsstrat->stratobj->FullObjID(), true, false);

	return JS_TRUE;
}

/************************************************************************************************
	cancelweaponreservation
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, cancelweaponreservation)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->RequestMissionWeapCancel();

	return JS_TRUE;
}

/************************************************************************************************
	reserveweapon
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, reserveweapon)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsVehtype* jsvehtype = phui::js::Cast<jsVehtype*>(ctx, argv[0]);
	if(!jsvehtype) return JS_FALSE;

	// lets clear out the current weap because the
	// newly reserved weap will reset it if the
	// reservation was accepted
	jsclient->client->ClearMissionWeap();

	jsclient->client->RequestMissionWeapPost(jsclient->mission->Detail().m_origin, jsvehtype->vehtype->WeaponSpec, false);
	if(jsclient->client->WaitForMessage(ww2::Client::WEAP_POST_WAIT, 3) == 0)
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	requestcrew
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestcrews)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->RequestMissionCrew(1);

	return JS_TRUE;
}

/************************************************************************************************
	requestcommanders
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestcommanders)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->RequestMissionPostList(jsclient->mission->Detail().m_origin);

	return JS_TRUE;
}

/************************************************************************************************
	joincrew
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, joincrew)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsMissionRoomEntry* roomentry = phui::js::Cast<jsMissionRoomEntry*>(ctx, argv[0]);
	if(!roomentry) return JS_FALSE;

	std::vector<COMMANDED_WEAPON> const& weapons = jsclient->client->CommandedWeapons();
	for(unsigned int i = 0; i < weapons.size(); i++)
	{
		if ( weapons[i].handle == roomentry->Entry().m_handle )
		{
			jsclient->client->RequestJoin(weapons[i].weaponSerial, roomentry->Entry().m_handle);
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			break;
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	acceptcrew
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, acceptcrew)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsMissionRoomEntry* roomentry = phui::js::Cast<jsMissionRoomEntry*>(ctx, argv[0]);
	if(!roomentry) return JS_FALSE;

	// tmp workaround
	ChatHandle myLowerHandle = chatGetLowercaseHandle(roomentry->Entry().m_handle);

	std::vector<FE_JOIN_PERM_REQUEST> const& joinrequests = jsclient->client->JoinRequests();
	for(unsigned int i = 0; i < joinrequests.size(); i++)
	{
		ChatHandle yourLowerHandle = chatGetLowercaseHandle(joinrequests[i].chatHandle);
		if(myLowerHandle == yourLowerHandle)
		{
			jsclient->client->SendAcceptCrew(joinrequests[i].joinRequestID);
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			break;
		}
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsClient, enabledespawntimer)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	JSBool enable=0;
	JS_ValueToBoolean(ctx, argv[0], &enable);

	jsclient->client->EnableDespawnTimer(enable);

	return JS_TRUE;
}

/************************************************************************************************
	requestpersonalist
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestpersonalist)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->RequestPersonaSummaries();

	return JS_TRUE;
}

/************************************************************************************************
	requestmissionlist
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestmissionlist)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->RequestMissionList();

	return JS_TRUE;
}


JS_FUNCTION_DEF(jsClient, requestquickmissionlist) {
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if ( ! jsclient ) return JS_FALSE;

	jsclient->client->RequestQuickMissionList();
	return JS_TRUE;
}
/************************************************************************************************
	exitmission
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, exitmission)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->ExitMission();
	jsclient->mission = new jsMission(MissionDetail());

	return JS_TRUE;
}

/************************************************************************************************
	cancelandexitmission
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, cancelandexitmission)
{
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsclient->client->ExitMission(true);
	jsclient->mission = new jsMission(MissionDetail());

	return JS_TRUE;
}

/************************************************************************************************
	requestbrigadedetail
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestbrigadedetail)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	jsBrigade* brigade = phui::js::Cast<jsBrigade*>(ctx, argv[0]);
	if(!brigade) return JS_FALSE;

	jsclient->client->RequestBrigadeDetail(brigade->Detail().unitID());
	if(jsclient->client->WaitForMessage(ww2::Client::BRIGADE_SELECT_WAIT, 3) == 0)
	{
		brigade->SetDetail(jsclient->client->BrigadeDetails().m_detail);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	requestmotd
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestmotd)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 mid = 0;
	JS_ValueToInt32(ctx, argv[0], &mid);

	jsMotd* motd = nullptr;

	MotdList::iterator it = jsclient->motds.find((UINT32)mid);
	if(it != jsclient->motds.end())
		motd = (*it).second;

	if(!motd || !motd->Valid())
	{
		jsclient->client->RequestMotd((UINT32)mid);

		if(jsclient->client->WaitForMessage(ww2::Client::MOTD_WAIT, 3) == 0)
		{
			MOTD_REPLY const& motdReply = jsclient->client->MotdReply();

			if(motdReply.type == (UINT32)mid)
			{
				motd = new jsMotd(motdReply);
				jsclient->motds[(UINT32)mid] = motd;
			}
		}
	}

	if(motd)
		*rval = motd->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	setmotd
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, setmotd)
{
	*rval = JSVAL_NULL;

	if(argc < 3) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 type = 0;
	JS_ValueToInt32(ctx, argv[0], &type);

	int32 subtype = 0;
	JS_ValueToInt32(ctx, argv[1], &subtype);

	const wchar_t* wmotd = phui::js::ValueToWideString(argv[2]);
	const char* motd = UTF8(wmotd);
	if(!motd) return JS_FALSE;

	EditString data(type, subtype, motd);
	EDIT_STRING_MSG outgoing(data);

	outgoing.send(jsclient->client->ChatConnection());

	return JS_TRUE;
}

/************************************************************************************************
	createsquad
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, createsquad)
{
	*rval = BOOLEAN_TO_JSVAL(false);

	if(argc < 6) return JS_FALSE;

	ChatHandle squadID = chatGetLowercaseHandle(phui::js::ValueToWideString(argv[0]));
	if(!squadID) return JS_FALSE;

	const char* name = UTF8(phui::js::ValueToWideString(argv[1]));
	if(!name) return JS_FALSE;

	std::vector<ChatHandle> players;
	const wchar_t* handle;

	for(int i = 2; i < 6; i++)
	{
		handle = phui::js::ValueToWideString(argv[i]);

		if(handle)
			players.push_back(chatGetLowercaseHandle(handle));
	}

	if(players.size() < SquadControl::CREATE_PLAYERS_REQUIRED)
		return JS_FALSE;

	SquadControl control;
	control.sendCreate(PlayerClient()->ChatConnection(), squadID, std::string(name), players);

	*rval = BOOLEAN_TO_JSVAL(true);

	return JS_TRUE;
}

/************************************************************************************************
	canaddwaypoint
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, canaddwaypoint)
{
	bool can = (PlayerClient()->SelectedMission().m_isCurrent || TUT_ACTIVE);

	if(can)
	{
		can = false;

		int found				= 0;
		int max_waypoints		= 16;

		std::vector<Waypoint> waypoints = PlayerClient()->Waypoints();
		std::vector<Waypoint>::iterator wit = waypoints.begin();
		for(; wit != waypoints.end(); ++wit)
		{
			if(wit->type() != WPT_ENEMY)
				++found;
		}

		ARENA_VARS::iterator ait = g_arenaDescription.find("max_waypoints");
		if(ait != g_arenaDescription.end())
			max_waypoints = atoi(ait->second.c_str());


		if(PlayerClient()->IsMissionLeader() && found <= max_waypoints)
			can = true;
	}

	*rval = BOOLEAN_TO_JSVAL(can);

	return JS_TRUE;
}

/************************************************************************************************
	canaddreport
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, canaddreport)
{
	bool can = (PlayerClient()->SelectedMission().m_isCurrent || TUT_ACTIVE);

	if(can)
	{
		can = false;

		int found				= 0;
		int mine				= 0;
		int max_reports			= 16;
		int max_user_reports	= 4;
		ChatHandle myHandle			= PlayerClient()->Personas().m_callsign;

		std::vector<Waypoint> waypoints = PlayerClient()->Waypoints();
		std::vector<Waypoint>::iterator wit = waypoints.begin();
		for(; wit != waypoints.end(); ++wit)
		{
			if(wit->type() == WPT_ENEMY)
			{
				if(wit->playerHandle() == myHandle)
					++mine;

				++found;
			}
		}

		ARENA_VARS::iterator ait;

		ait = g_arenaDescription.find("max_reports");
		if(ait != g_arenaDescription.end())
			max_reports = atoi(ait->second.c_str());

		ait = g_arenaDescription.find("max_user_reports");
		if(ait != g_arenaDescription.end())
			max_user_reports = atoi(ait->second.c_str());

		if(found <= max_reports)
		{
			if(PlayerClient()->IsMissionLeader() || mine <= max_user_reports)
				can = true;
		}
	}

	*rval = BOOLEAN_TO_JSVAL(can);

	return JS_TRUE;
}

/************************************************************************************************
	undeployms
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, undeployms)
{
	// so we can undeploy them at despawn
	// and they don't leave a MS behind
	// they are given the option
	vehmainUndeployVehicleForMobileSpawn();

	*rval = BOOLEAN_TO_JSVAL(true);

	return JS_TRUE;
}

/************************************************************************************************
	requestmissiondetail
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestmissiondetail)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 missionID = 0;
	JS_ValueToInt32(ctx, argv[0], &missionID);

	jsclient->client->RequestMissionLookup(missionID);

	if(jsclient->client->WaitForMessage(ww2::Client::MISSION_DETAIL_WAIT, 5) == 0)
		*rval = (new jsMission(jsclient->client->MissionLookup()))->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	requestrecruiters
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, requestrecruiters)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsClient* jsclient = phui::js::Cast<jsClient*>(ctx,jsobj);
	if(!jsclient) return JS_FALSE;

	int32 language	= -1;

	JS_ValueToInt32(ctx, argv[0], &language);

	if(language > -1)
	{
		jsclient->client->RequestRecruiters(language);

		if(jsclient->client->WaitForMessage(ww2::Client::REQUEST_RECRUITERS_WAIT, 10) == 0)
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsClient::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case COUNTRY_PROP:
			*val = (new jsCountry(client->PersonaSelected().m_countryID))->jsVal();
		break;
		case BRANCH_PROP:
		{
			UINT32 idx = (UINT32)client->PersonaSelected().m_branchID;
			if(idx < MAX_BRANCHES)
				*val = phui::js::NewString(branches[idx]);
		}
		break;
		case HANDLE_PROP:
			*val = phui::js::NewString(chatGetWideCallsign(client->Personas().m_callsign));
		break;
		case BRIGADE_PROP:
			*val = brigade->jsVal();
		break;
		case BRIGADES_PROP:
		{
			client->RequestBrigadeList();
			client->WaitForMessage(ww2::Client::BRIGADE_LIST_WAIT, 3);

			UINT32 c = 0;
			brigades->clear();
			BrigadeSummaries const& brigadelist = client->Brigades();

			for(UINT32 i = 0; i < brigadelist.m_count; i++)
			{
				const BrigadeSummary& summary = brigadelist.m_summaries[i];

				if(!summary.isTrainingUnit())
				{
					jsBrigade* brig = new jsBrigade(brigadelist.m_summaries[i]);
					brigades->SetElement(brig, c);
					++c;
				}
			}

			*val =  brigades->jsVal();
		}
		break;
		case MISSIONS_PROP:
		{
			missions->clear();
			std::vector<MissionDetail> const& missionlist = client->Missions();
			for(UINT32 i = 0; i < missionlist.size(); i++)
			{
				jsMission* jsum = new jsMission(missionlist[i]);
				missions->SetElement(jsum, i);
			}
			*val = missions->jsVal();
		}
		break;
		case SPAWNABLE_VEHICLES_PROP:
#ifndef ALLSPAWNHACK
			*val = spawnablevehicles->jsVal();
#else
			Parent()->Parent()->GetProperty(ctx, jsGame::SPAWNABLE_VEHICLES_PROP, val);
#endif
		break;
		case WEAPONCOUNTER_PROP:
			*val = weaponcounter->jsVal();
		break;
		case CHATCHANNELS_PROP:
			*val = chatchannels->jsVal();
		break;
		case PERSONAS_PROP:
		{
			personas->clear();
			PersonaSummaries const& persum = client->Personas();
			for(UINT32 i = 0; i < persum.m_count; i++)
			{
				jsPersonaSummary* jsum = new jsPersonaSummary(persum.m_summaries[i]);
				personas->SetElement(jsum, i);
			}

			*val = personas->jsVal();
		}
		break;
		case PERSONALISTTIME_PROP:
  			*val = INT_TO_JSVAL(client->LastMessageReceivedTime(ww2::Client::PERSONA_SUMMARIES_TIME));
  		break;
  		case MISSIONROOMLIST_PROP:
		{
			missionroomlist->clear();

			// this needs work, currently when you are in the UI you get a list of
			// jsMissionRoomEntry's, but while your in game you get a list
			// of jsMissionMember's. The later holds less data and probably should
			// be the super class of jsMissionRoomEntry
			// not to mention we don't need two lists this similar
			// we should revisit this
			//
			// we now have a persistent mmhost connection
			if(SCEN_MGR->IsActive())
			{
				SCENPLAYER_MAP& players = SCEN_MGR->GetPlayers();
				SCENPLAYER_MAP::iterator it = players.begin();
				SINT32 count = 0;

				for(; it != players.end(); it++)
				{
					if(!it->second.IsActive())
						continue;

					jsScenarioPlayer* player = new jsScenarioPlayer(it->second);
					missionroomlist->SetElement(player, count++);
				}
			}
			/*
			if(false && uiInGame())
			{
				std::vector<MissionMember> const &roomlist = client->MissionMembers();

				for(UINT32 i = 0; i < roomlist.size(); i++)
				{
					jsMissionMember* member = new jsMissionMember(roomlist[i], persona->Summary().m_countryID, persona->Summary().m_branchID);
					missionroomlist->SetElement(member, i);
				}
			}
			*/
			else
			{
				std::vector<RoomListEntry> const &roomlist = client->MissionRoomList(true);

				for(UINT32 i = 0; i < roomlist.size(); i++)
				{
					jsMissionRoomEntry* entry = new jsMissionRoomEntry(roomlist[i], persona->Summary().m_countryID, persona->Summary().m_branchID);
					missionroomlist->SetElement(entry, i);
				}
			}

			*val = missionroomlist->jsVal();
		}
		break;
		case RESERVEDWEAPON_PROP:
		{
			if(!reservedweapon.valid() ||
				memcmp(&client->WeaponSerial(), &reservedweapon->serial, sizeof(WEAPON_SERIAL)) != 0)
			{
				if(client->WeaponSerial().serial != 0)
					reservedweapon = new jsWeaponSerial(client->WeaponSerial());
				else
					reservedweapon = 0;
			}

			if(reservedweapon.valid())
				*val = reservedweapon->jsVal();
			else
				*val = JSVAL_NULL;
		}
		break;
		case SIDELOCKED_PROP:
			*val = INT_TO_JSVAL(client->Personas().m_sidelocked);
		break;
		case CURRENTMISSION_PROP:
			if(mission.valid() && mission->Detail().m_isCurrent)
				*val = mission->jsVal();
			else
				*val = JSVAL_NULL;
		break;
		case ACTIVEMISSION_PROP:
			*val = (new jsActiveMission(client->GetActiveMission()))->jsVal();
		break;
		case MISSIONRESULTS_PROP:
			// only pass it back if its a new one
			if(lastmissionresults != (int)client->LastMessageReceivedTime(ww2::Client::MISSION_RESULTS_TIME))
			{
                /*
                 TODO: Send in whether or not the player was wounded into Mission Results

                 NOTE: unsure whether code here is connected to above statement.

				const PersonaSelection& persona = PlayerClient()->PersonaSelected();
				Player* player = playerGetPlayer();
				vehVehicle* vehicle = playerGetVehicle();
				vehVehicle* player = persGetPersonaVehiclePtr();

				 */

				missionresults = new jsMissionResults(client->MissionResults());
				lastmissionresults = client->LastMessageReceivedTime(ww2::Client::MISSION_RESULTS_TIME);
				*val = missionresults->jsVal();
			}
			else
			{
				*val = JSVAL_NULL;
			}
		break;
		case DESPAWNTIMER_PROP:
			*val = INT_TO_JSVAL(client->DespawnTimer());
		break;
		case CANPOSTMISSION_PROP:
			*val = BOOLEAN_TO_JSVAL(client->CanPostMission());
		break;
		case PERSONA_PROP:
			if(persona.valid())
				*val = persona->jsVal();
			else
				*val = JSVAL_NULL;
		break;
		case MISSIONLISTUPDATE_PROP:
			*val = INT_TO_JSVAL(client->MissionListUpdate());
		break;
		case LASTSPAWNLISTSTATE_PROP:
		{
			int state = -1; // invalid

			if(lastspawnliststatus > FacilityWeaponList::_INVALID && lastspawnliststatus < FacilityWeaponList::_NORMAL)
				state = 2; // can spawn, but no supply
			else if(lastspawnliststatus == FacilityWeaponList::_NORMAL)
				state = 1; // can spawn, with supply
			else if(lastspawnliststatus > FacilityWeaponList::_NORMAL)
				state = 0; // no spawn, no supply

			*val = INT_TO_JSVAL(state);
		}
		break;
		case LASTSPAWNLISTSTATUS_PROP:
		{
			const char* state = "invalid";
			switch(lastspawnliststatus)
			{
				// Spawnable but not supplied states
				case FacilityWeaponList::_CUT_OFF:			state = "cut"; break;			// But still spawnable
				case FacilityWeaponList::_CUT_BY_FB:			state = "cutbyfb"; break;		// Depot's supply is cut-off by enemy FB
				case FacilityWeaponList::_CUT_BY_NO_LINKS:	state = "cutnolinks"; break;	// This depot has* no* strat links at all
				case FacilityWeaponList::_CUT_BY_MATE:		state = "cutbymate"; break;		// Don't own connecting depot
				case FacilityWeaponList::_SUPPLY_IN_TRANSIT: state = "supplyotw"; break;		// Supply is in transit (may not be spawnable)
				// Spawnable state
				case FacilityWeaponList::_NORMAL:			state = "normal"; break;		// Healthy list
				// Non-spawnable states
				case FacilityWeaponList::_ENEMY_OWNS:		state = "nmeowns"; break;		// Enemy now owns this facility
				case FacilityWeaponList::_ENEMY_OWNS_CP:		state = "nmeownscp"; break;		// Enemy has ownership of the CP
				case FacilityWeaponList::_ENEMY_CONTROLS_CP:	state = "nmecontrols"; break;	// Enemy has control of the CP
				case FacilityWeaponList::_CLOSED:			state = "closed"; break;		// This facility is closed
				case FacilityWeaponList::_NOT_LINKED:		state = "notlinked"; break;		// No link for supply
				case FacilityWeaponList::_EMPTY:				state = "empty"; break;			// No equipment at this facility
				case FacilityWeaponList::_NON_SPAWNABLE:		state = "notspawnable"; break;	// Facility not spawnable
				case FacilityWeaponList::_SUPPLY_CLOSED:		state = "supplyclosed"; break;	// Source facility closed
				case FacilityWeaponList::_WRONG_SUPPLY:		state = "wrongsupply"; break;	// Wrong country supplying
				case FacilityWeaponList::_WRONG_OWNER:		state = "wrongowner"; break;	// Owned by an ally
				case FacilityWeaponList::_BDE_DISABLED:		state = "bdedisabled"; break;	// Brigade (unit) is disabled by TOE.
				default: break;
			}
			*val = phui::js::NewString(state);
		}
		break;
		case LASTSPAWNLISTSOURCE_PROP:
		{
			StratDB::StratObject* stratobj = StratDB::FindStratObject(lastspawnlistsource);

			if(stratobj)
				*val = (new jsStratObject( stratobj->FullObjID()))->jsVal();
			else
				*val = JSVAL_NULL;
		}
		break;
		case SQUAD_PROP:
		{
			if(!squad.valid())
			{
				ClientSquad const &mysquad = client->PlayerSquad();

				if(mysquad.squadID())
					squad = new jsSquad(mysquad);
			}

			*val = (squad.valid()) ? squad->jsVal(): JSVAL_NULL;
		}
		break;
		case SQUADINVITE_PROP:
		{
			const SquadInvite& invite = client->PlayerSquadInvite();

			if(!squadinvite.valid())										// brand new invite
				squadinvite = new jsSquadInvite(invite);
			else if(invite.invited() != squadinvite->invite.invited())		// invite has changed
				squadinvite = new jsSquadInvite(invite);

			*val = (squadinvite.valid() && squadinvite->invite.valid()) ? squadinvite->jsVal(): JSVAL_NULL;
		}
		break;
		case CURRENTORDERS_PROP:
			*val = phui::js::NewString(client->CurrentOrders().c_str());
		break;
		case ISPLAYNET_PROP:
		{
			PersonaSummaries const& persum = client->Personas();
			*val = BOOLEAN_TO_JSVAL(persum.isPlaynet());
		}
		break;
		case SIDEBALANCE_PROP:
			*val = INT_TO_JSVAL(client->SideBalance());
		break;
		case SIDEDELAY_PROP:
			*val = BOOLEAN_TO_JSVAL((bool)client->IsSideDelayed());
		break;
		case POLL_PROP:
		{
			if(!poll.valid())
				poll = new jsPoll(client->GetOpinionPoll());

			*val = (poll.valid() && !poll->poll.empty()) ? poll->jsVal(): JSVAL_NULL;
		}
		break;
		case SPAWNDELAY_PROP:
			*val = BOOLEAN_TO_JSVAL((bool)client->IsSpawnDelayed());
		break;
		case ISMOBILESPAWN_PROP:
			*val = BOOLEAN_TO_JSVAL((bool)vehmainIsMobileSpawn());
		break;
		case CANSTO_PROP:
			*val = BOOLEAN_TO_JSVAL((bool)vehmainCanSTO());
		break;
		case BRIGADENOTICES_PROP:
		{
			brigadenotices->clear();

			std::vector<BrigadeControlNotice> const& notices = client->BrigadeNotices();
			UINT32 count = (UINT32)notices.size();

			// we reverse order so last arrived is first
			for(UINT32 i = 0; i < count; i++)
				brigadenotices->SetElement(new jsBrigadeControlNotice(notices[count - i - 1], i), i);

			*val = brigadenotices->jsVal();
		}
		break;
		case ISHC_PROP:
			*val = BOOLEAN_TO_JSVAL(client->IsHC());
		break;
		case ISOIC_PROP:
			*val = BOOLEAN_TO_JSVAL(client->IsOIC());
		break;
		case OICOF_PROP:
		{
			UINT32 brigadeID = client->OICOfBrigade();
			*val = JSVAL_NULL;

			if(brigadeID)
			{
				if(!oicof.valid())
					oicof = new jsBrigade(brigadeID);

				*val = (oicof.valid()) ? oicof->jsVal(): JSVAL_NULL;
			}
		}
		break;
		case HAVEBRIGADENOTICES_PROP:
		{
			int currentCount = (int)client->BrigadeNotices().size();
			bool haveNotices = false;

			if(currentCount > 0 && currentCount != brigadenoticecount)
			{
				haveNotices			= true;
				brigadenoticecount	= currentCount;
			}

			*val = BOOLEAN_TO_JSVAL(haveNotices);
		}
		break;
		case RECRUITER_PROP:
		{
			recruiter = new jsRecruiter(client->PlayerSquadRecruitment().recruiter);
			*val = (recruiter.valid()) ? recruiter->jsVal(): JSVAL_NULL;
		}
		break;
		case RECRUITERS_PROP:
		{
			SquadRecruitment& recruitment = PlayerClient()->PlayerSquadRecruitment();

			if(recruitment.modified)
			{
				UINT32 count = (UINT32)recruitment.recruiters.size();

				recruiters->clear();

				for(UINT32 i = 0; i < count; i++)
					recruiters->SetElement(new jsRecruiter(recruitment.recruiters[i]), i);

				recruitment.modified = false;
			}

			*val = recruiters->jsVal();
		}
		break;
		case HAVERECRUITERS_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(PlayerClient()->PlayerSquadRecruitment().modified);
		}
		break;
		case MISSIONINVITE_PROP:
		{
			missioninvite = new jsMissionInvite(client->GetMissionInvite());
			*val = (missioninvite.valid()) ? missioninvite->jsVal(): JSVAL_NULL;
		}
		break;
		case NEXTSUPPLY_PROP:
		{
			*val = INT_TO_JSVAL(nextsupply);
		}
		break;
		case INSTANCE_PROP:
		{
			*val = instance->jsVal();
		}
		break;
		case STATE_PROP:
		{
			*val = state->jsVal();
		}
		break;
		case CAPTURESTATE_PROP:
		{
			*val = capturestate->jsVal();
		}
		break;
		case INSTANCES_PROP:
			*val = instances->jsVal();
		break;
	}

	return JS_TRUE;
}

JSBool jsClient::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	return JS_TRUE;
}

#ifndef GM_RELEASE
/************************************************************************************************
    _isRegularVehicle
	Looks up the weapon spec and checks if it is a valid vehicle that does not require
    GM_RELEASE.
*************************************************************************************************/
static bool _isRegularVehicle(const WEAPON_SPEC spec)
{
	const WEAPON* weapon = weapLookup(spec);

	return !weapon || weapon->isPlayerWeapon();
}
#endif

/************************************************************************************************
	ProcessWeapList
*************************************************************************************************/

void jsClient::ProcessWeapList()
{
	spawnablevehicles->clear();
	weaponcounter->clear();

	UINT32 count = 0;

	FacilityWeaponList const& weaplist = client->WeaponList();
	for(UINT32 i = 0; i < weaplist.m_numWeapons; i++)
	{
		vehType const* vehtype = vehTypeGetBySpec(weaplist.m_weapons[i]);
		if(!vehtype) continue;

		WeaponCounter counter = weaplist.m_counters[i];

#ifndef GM_RELEASE
		if(false == _isRegularVehicle(weaplist.m_weapons[i])) {
			counter.spawnable = WeaponCounter::NOT_PERMITTED;
		}
#endif

		jsVehtype* jstype = new jsVehtype(vehtype, counter);
		spawnablevehicles->SetElement(jstype, count);
		weaponcounter->SetElement(new jsWeaponCounter(counter, vehtype->VehicleTypeNum, weaplist.m_levels[i]), count);

		++count;
	}

	lastspawnliststatus = (int)weaplist.m_status;
	lastspawnlistsource = weaplist.m_fullObjID;
	nextsupply = weaplist.m_nextSupplyM;

	RunScript("onweaponlist()");
}

/************************************************************************************************
	ProcessMissionLaunchUpdate
*************************************************************************************************/

void jsClient::ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const& lu)
{
	if(mission.valid() && lu.missionID == mission->Detail().m_missionID)
		mission->LaunchUpdate(lu);
}

/************************************************************************************************
	ProcessMissionLeaderChange
*************************************************************************************************/

void jsClient::ProcessMissionLeaderUpdate(Chat::LocalizedMessage const& lmsg)
{
	if(mission.valid())
		mission->LeaderUpdate(lmsg);
}

/************************************************************************************************
	UpdatePersonas
*************************************************************************************************/

void jsClient::UpdatePersonas()
{
	persona = nullptr;
	personas->clear();

	const PersonaSummaries& persum	= client->Personas();
	const PersonaSelection& persel		= client->PersonaSelected();

	for(UINT32 i = 0; i < persum.m_count; i++)
	{
		jsPersonaSummary* s = new jsPersonaSummary(persum.m_summaries[i]);

		personas->SetElement(s, i);

		if(persel.m_countryID == persum.m_summaries[i].m_countryID &&
		   persel.m_branchID == persum.m_summaries[i].m_branchID)
			persona = s;
	}

	if(persona.valid())
		brigade->SetDetail(client->BrigadeSelected().m_detail);
	else
		brigade->SetDetail(BrigadeDetail());
}

/************************************************************************************************
	UpdatePersonas
*************************************************************************************************/

void jsClient::UpdateInstance(int status)
{
	instance->Update(status);
}

/************************************************************************************************
	UpdateInstances - from a previous query select
*************************************************************************************************/

void jsClient::UpdateInstances()
{
	instances->clear();

	const Instance::ClientListing* list = PlayerClient()->GetInstances();

	if(list)
	{
		UINT32 i = 0;

		Instance::Listing::Results::const_iterator it = list->begin();

		for(; it != list->end(); it++, i++)
		{
			const Instance::ClientStatus& state = *it;
			instances->SetElement(new jsInstanceState(&state), i);
		}
	}
}

/************************************************************************************************
	jsMission
*************************************************************************************************/

JSClass jsMission::jsclass = JS_CLASS_MACRO("missionsummary");
JSFunctionSpec jsMission::jsfunctions[] = {{0}};
JSPropertySpec jsMission::jsproperties[] =
{
	{"mobilespawn",		jsMission::MOBILESPAWN_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"amleader",		jsMission::AMLEADER_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"timelaunch",		jsMission::TIMELAUNCH_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"poster",			jsMission::POSTER_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"spawndelay",		jsMission::SPAWNDELAY_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"issquadmission",	jsMission::ISSQUADMISSION_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"population",		jsMission::POPULATION_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"canmobilespawn",	jsMission::CANMOBILESPAWN_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"open",			jsMission::OPEN_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsMission::jsMission(MissionDetail const& d)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, detail(d)
{
	JS_CONSTRUCTOR_LOCK;

	StratDB::StratObject* stratobj;

	stratobj = StratDB::FindStratObject(detail.m_origin);
	if(stratobj)
		DefineProperty("origin", new jsStratObject(stratobj->FullObjID()));

	stratobj = StratDB::FindStratObject(detail.m_target);
	if(stratobj)
		DefineProperty("target", new jsStratObject(stratobj->FullObjID()));

	starttime = frametime;
	state = MISS_LAUNCH_UPDATE::ACTIVE;

	DefineProperty("id",INT_TO_JSVAL(detail.m_missionID));
	DefineProperty("typeid",INT_TO_JSVAL(detail.m_type));

	switch(detail.m_type)
	{
		case POINT_ATTACK:
			DefineProperty("type",phui::js::ConstString( "Point Attack"));
		break;
		case POINT_DEFENSE:
			DefineProperty("type",phui::js::ConstString("Point Defense"));
		break;
		case AREA_ATTACK:
			DefineProperty("type",phui::js::ConstString( "Area Attack"));
		break;
		case AREA_DEFENSE:
			DefineProperty("type",phui::js::ConstString( "Area Defense"));
		break;
		case RECON:
			DefineProperty("type",phui::js::ConstString( "Recon"));
		break;
		case RESUPPLY:
			DefineProperty("type",phui::js::ConstString( "Resupply"));
		break;
        default:
            // TBD - why isn't SYSTEM_MISSION listed here?
            break;
	}

	DefineProperty("subtype",INT_TO_JSVAL(detail.m_subType));
	//DefineProperty("poster",phui::js::NewString(chatGetWideCallsign(detail.m_poster)));
	DefineProperty("current",BOOLEAN_TO_JSVAL(detail.m_isCurrent));
	const wchar_t* wdescription = UTF16((const unsigned char*)detail.m_desc);
	DefineProperty("description",phui::js::NewString(wdescription));
};

JSBool jsMission::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case MOBILESPAWN_PROP:
			*val = BOOLEAN_TO_JSVAL(detail.m_hasMobileSpawn);
		break;
		case AMLEADER_PROP:
		{
			MissionDetail selectedmission = PlayerClient()->SelectedMission();

			if(detail.m_missionID == selectedmission.m_missionID && PlayerClient()->IsMissionLeader())
				*val = BOOLEAN_TO_JSVAL(true);
			else
				*val = BOOLEAN_TO_JSVAL(false);
		}
		break;
		case TIMELAUNCH_PROP:
			*val = INT_TO_JSVAL(detail.m_timeLaunch);
		break;
		case STATE_PROP:
			*val = INT_TO_JSVAL(state);
		break;
		case POSTER_PROP:
			*val = phui::js::NewString(chatGetWideCallsign(detail.m_poster));
		break;
		case SPAWNDELAY_PROP:
			*val = INT_TO_JSVAL(std::max(detail.launchDelay(), 0U));
		break;
		case ISSQUADMISSION_PROP:
			*val = BOOLEAN_TO_JSVAL(PlayerClient()->PlayerSquad().isSquadMission(detail.m_missionID));
		break;
		case POPULATION_PROP: // will eventually be updated during the launchupdate
			*val = INT_TO_JSVAL(detail.m_population);
		break;
		case CANMOBILESPAWN_PROP:
			*val = BOOLEAN_TO_JSVAL(detail.m_canMobileSpawn);
		break;
		case OPEN_PROP:
			*val = BOOLEAN_TO_JSVAL(detail.m_isOpen);
		break;
	}

	return JS_TRUE;
}

void jsMission::LaunchUpdate(MISS_LAUNCH_UPDATE const& lu)
{
	detail.m_timeLaunch = lu.timeLaunch;
	detail.m_hasMobileSpawn = lu.mobileSpawnAvailable;
	detail.m_canMobileSpawn = lu.m_canMobileSpawn;
	detail.m_isOpen = (lu.state == MISS_LAUNCH_UPDATE::ACTIVE);
	state = lu.state;
	starttime = frametime;
}

void jsMission::LeaderUpdate(Chat::LocalizedMessage const& lmsg)
{
	detail.m_poster = chatGetHandle(lmsg.m_ptr[0]);
}


/************************************************************************************************
	jsPersonaSummary
*************************************************************************************************/

JSClass jsPersonaSummary::jsclass = JS_CLASS_MACRO("personasummary");
JSFunctionSpec jsPersonaSummary::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsPersonaSummary::jsproperties[] =
{
	{"branch",		jsPersonaSummary::BRANCH_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"rankpercent",	jsPersonaSummary::RANKPERCENT_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"lastused",	jsPersonaSummary::LASTUSED_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"lockedthru",	jsPersonaSummary::LOCKEDTHRU_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"branchid",	jsPersonaSummary::BRANCHID_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"availability",jsPersonaSummary::AVAILABILITY_PROP,JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsPersonaSummary
*************************************************************************************************/

jsPersonaSummary::jsPersonaSummary(PersonaSummary const& s)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, summary(s), timestamp(frametime)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("brigade", new jsBrigade(s.m_brigadeID));
	DefineProperty("country", new jsCountry(summary.m_countryID));
	DefineProperty("side", phui::js::ConstString((wwiiSide(summary.m_countryID) == AXIS) ? "axis" : "allies"));
	DefineProperty("rank",  new jsRank(summary.m_countryID,summary.m_branchID,summary.m_rankID));
};

jsPersonaSummary::~jsPersonaSummary()
{
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsPersonaSummary::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case BRANCH_PROP:
			if(summary.m_branchID < MAX_BRANCHES)
				*val = phui::js::NewString(branches[summary.m_branchID]);
		break;
		case BRANCHID_PROP:
			if(summary.m_branchID < MAX_BRANCHES)
				*val = INT_TO_JSVAL(summary.m_branchID);
		break;
		case LASTUSED_PROP:
		{
			int t = (frametime - timestamp)/1000 + summary.m_lastUsed;
			*val = INT_TO_JSVAL(t);
		}
		break;
		case LOCKEDTHRU_PROP:
		{
			int t = (timestamp - frametime)/1000 + summary.m_lockedThru;
			if(t < 0) t = 0;
			*val = INT_TO_JSVAL(t);
		}
		break;
		case RANKPERCENT_PROP:
			*val = INT_TO_JSVAL(summary.m_rankPctg);
		break;
		case AVAILABILITY_PROP:
			*val = phui::js::NewString(summary.availability());
		break;
	}

	return JS_TRUE;
}

JSBool jsPersonaSummary::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	return JS_TRUE;
}

/************************************************************************************************
	jsBrigade
*************************************************************************************************/

JSClass jsBrigade::jsclass = JS_CLASS_MACRO("brigade");
JSFunctionSpec jsBrigade::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsBrigade, nominate, 1),
	JS_FUNCTION_SPEC(jsBrigade, volunteer, 0),
	JS_FUNCTION_SPEC(jsBrigade, replace, 0),
	JS_FUNCTION_SPEC(jsBrigade, remove, 0),
	JS_FUNCTION_SPEC(jsBrigade, resign, 0),
	JS_FUNCTION_SPEC(jsBrigade, move, 2),
	{0}
};

JSPropertySpec jsBrigade::jsproperties[] =
{
	{"name",			jsBrigade::NAME_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"motd",			jsBrigade::MOTD_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"id",				jsBrigade::ID_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"division",		jsBrigade::DIVISION_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"divisionid",		jsBrigade::DIVISION_ID_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"missions",		jsBrigade::MISSIONS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"populationrank",	jsBrigade::POPULATION_RANK_PROP,JSPROP_READONLY | JSPROP_PERMANENT},
	{"parent",			jsBrigade::PARENT_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"motto",			jsBrigade::MOTTO_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"nick",			jsBrigade::NICK_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"officers",		jsBrigade::OFFICERS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"home",			jsBrigade::HOME_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"owner",			jsBrigade::OWNER_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"ownerrank",		jsBrigade::OWNERRANK_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"objective",		jsBrigade::OBJECTIVE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"mobilespawns",	jsBrigade::MOBILESPAWNS_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"isbrigade",		jsBrigade::ISBRIGADE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"issquadbrigade",	jsBrigade::ISSQUADBRIGADE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"callsign",		jsBrigade::CALLSIGN_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"oic",				jsBrigade::OIC_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"oicrank",			jsBrigade::OICRANK_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"canvote",			jsBrigade::CANVOTE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"cantakecharge",	jsBrigade::CANTAKECHARGE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"activeoic",		jsBrigade::ACTIVEOIC_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"commander",		jsBrigade::COMMANDER_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"commanderrank",	jsBrigade::COMMANDERRANK_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsBrigade
*************************************************************************************************/

jsBrigade::jsBrigade(BrigadeDetail const& d)
			: phui::js::Object(&jsclass, jsfunctions,jsproperties)
			, detail(d), officerranks(0), officernames(0)
{
	JS_CONSTRUCTOR_LOCK;

	DefineProperty("officernames", officernames = new phui::js::Array);
	DefineProperty("officerranks", officerranks = new phui::js::Array);
	DefineProperty("country", new jsCountry(detail.country()));

	if(detail.branch() < MAX_BRANCHES)
	{
		DefineProperty("branch", phui::js::NewString(branches[detail.branch()]));
		DefineProperty("branchid", INT_TO_JSVAL(detail.branch()));
	}

	UpdateOfficerArrays();
}

jsBrigade::jsBrigade(BrigadeSummary const& s)
			: phui::js::Object(&jsclass, jsfunctions,jsproperties)
			, officerranks(0), officernames(0)

{
	JS_CONSTRUCTOR_LOCK;
	*(BrigadeSummary*)(&detail) = s;
	detail.m_motd[0] = '\0';

	DefineProperty("officernames", officernames = new phui::js::Array);
	DefineProperty("officerranks", officerranks = new phui::js::Array);
	DefineProperty("country", new jsCountry(detail.country()));

	if(detail.branch() < MAX_BRANCHES)
	{
		DefineProperty("branch", phui::js::NewString(branches[detail.branch()]));
		DefineProperty("branchid", INT_TO_JSVAL(detail.branch()));
	}
};

jsBrigade::jsBrigade(unsigned int brigadeid)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, officerranks(0), officernames(0)
{
	JS_CONSTRUCTOR_LOCK;
	HCUnit* unit = HCUnit::Lookup(brigadeid);
	if(unit)
		*(HCUnit*)(&detail) = *unit;

	DefineProperty("officernames", officernames = new phui::js::Array);
	DefineProperty("officerranks", officerranks = new phui::js::Array);
	DefineProperty("country", new jsCountry(detail.country()));

	if(detail.branch() < MAX_BRANCHES)
	{
		DefineProperty("branch", phui::js::NewString(branches[detail.branch()]));
		DefineProperty("branchid", INT_TO_JSVAL(detail.branch()));
	}
};

jsBrigade::~jsBrigade()
{
}

/************************************************************************************************
	UpdateOfficerArrays
*************************************************************************************************/

void jsBrigade::UpdateOfficerArrays()
{
	officernames->clear();
	officerranks->clear();

	for(unsigned int i = 0; i < detail.officers() && i < BrigadeDetail::_MAX_OFFICERS; i++)
	{
		officernames->SetElement(phui::js::NewString(chatGetWideCallsign(detail.m_officerHandles[i])), i);
		officerranks->SetElement(new jsRank(detail.m_countryID, detail.m_branchID, detail.m_officerRanks[i]), i);
	}
}

/************************************************************************************************
	nominate
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigade, nominate)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!PlayerClient()->IsHC()) return JS_FALSE;
	if(argc < 1) return JS_FALSE;

	jsBrigade* jsbrigade = phui::js::Cast<jsBrigade*>(ctx,jsobj);
	if(!jsbrigade) return JS_FALSE;

	const char* nominee = UTF8(phui::js::ValueToWideString(argv[0]));
	if(!nominee) return JS_FALSE;

	BrigadeControlRequest req;
	const BrigadeDetail& detail = jsbrigade->Detail();

	req.nominateOIC(
		PlayerClient()->ChatConnection(),
		detail.brigadeID(),
		chatGetHandle(nominee));

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	volunteer
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigade, volunteer)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsBrigade* jsbrigade = phui::js::Cast<jsBrigade*>(ctx,jsobj);
	if(!jsbrigade) return JS_FALSE;

	const BrigadeDetail& detail	= jsbrigade->Detail();
	BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
	BrigadeControlRequest req;

	if(!oic || !oic->m_officer)
		req.volunteerOIC(
			PlayerClient()->ChatConnection(),
			detail.brigadeID());

	else if(PlayerClient()->IsHC())
		req.replaceOIC(
			PlayerClient()->ChatConnection(),
			detail.brigadeID(),
			oic->m_officer);

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	replace
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigade, replace)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!PlayerClient()->IsHC()) return JS_FALSE;

	jsBrigade* jsbrigade = phui::js::Cast<jsBrigade*>(ctx,jsobj);
	if(!jsbrigade) return JS_FALSE;

	const BrigadeDetail& detail	= jsbrigade->Detail();
	BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());

	if(oic && oic->m_officer != PlayerClient()->Personas().m_callsign)
	{
		BrigadeControlRequest req;
		req.replaceOIC(
			PlayerClient()->ChatConnection(),
			detail.brigadeID(),
			oic->m_officer);

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	remove
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigade, remove)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!PlayerClient()->IsHC()) return JS_FALSE;

	jsBrigade* jsbrigade = phui::js::Cast<jsBrigade*>(ctx,jsobj);
	if(!jsbrigade) return JS_FALSE;

	const BrigadeDetail& detail	= jsbrigade->Detail();
	BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());

	if(oic && oic->m_officer)
	{
		BrigadeControlRequest req;

		if(PlayerClient()->Personas().m_callsign == oic->m_officer)
			req.resignOIC(
				PlayerClient()->ChatConnection(),
				detail.brigadeID());
		else
			req.removeOIC(
				PlayerClient()->ChatConnection(),
				detail.brigadeID(),
				oic->m_officer);

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	resign
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigade, resign)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsBrigade* jsbrigade = phui::js::Cast<jsBrigade*>(ctx,jsobj);
	if(!jsbrigade) return JS_FALSE;

	const BrigadeDetail& detail	= jsbrigade->Detail();
	UINT32 myOICBrigadeID	= PlayerClient()->OICOfBrigade();

	if(myOICBrigadeID == (UINT32)detail.brigadeID())
	{
		BrigadeControlRequest req;
		req.resignOIC(
			PlayerClient()->ChatConnection(),
			detail.brigadeID());

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	move
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigade, move)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 2) return JS_FALSE;

	jsBrigade* jsbrigade = phui::js::Cast<jsBrigade*>(ctx,jsobj);
	if(!jsbrigade) return JS_FALSE;

	const BrigadeDetail& detail	= jsbrigade->Detail();

	if(PlayerClient()->IsHC() || PlayerClient()->OICOfBrigade() > 0)
	{
		jsStratObject* origin = phui::js::Cast<jsStratObject*>(ctx, argv[0]);
		jsStratObject* target = phui::js::Cast<jsStratObject*>(ctx, argv[1]);
		if(origin && target)
		{
			BrigadeControlRequest req;
			req.requestBrigadeMove(
				PlayerClient()->ChatConnection(),
				detail.brigadeID(),
				*target->stratobj->FullObjID(),		// target
				*origin->stratobj->FullObjID());	//origin

			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsBrigade::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case NAME_PROP:
			*val = phui::js::NewString(UTF16(detail.m_name));
		break;
		case MOTD_PROP:
			*val = phui::js::NewString(UTF16(detail.m_motd));
		break;
		case ID_PROP:
			*val = INT_TO_JSVAL(detail.brigadeID());
		break;
		case PARENT_PROP: {
			// TODO: This ID can be cached
			HCUnit* unit = HCUnit::Lookup(detail.brigadeID());
			if(unit != nullptr && unit->parent())
				*val = (new jsHCUnit(*unit->parent()))->jsVal();
			else
				*val = JSVAL_NULL;
		}
		break;
		case DIVISION_PROP: {
			// TODO: This ID can be cached
			HCUnit* unit = HCUnit::Lookup(detail.brigadeID());
			if (unit && unit->parent())
				*val = phui::js::NewString(UTF16(unit->parent()->name()));
			else
				*val = JS_GetEmptyStringValue(ctx);
		}
		break;
		case DIVISION_ID_PROP:
			*val = INT_TO_JSVAL(detail.m_parentID);
		break;
		case MISSIONS_PROP:
			*val = INT_TO_JSVAL(detail.missions());
		break;
		case POPULATION_RANK_PROP:
			*val = INT_TO_JSVAL(detail.players());
		break;
		case MOTTO_PROP:
			*val = phui::js::NewString(UTF16(detail.m_motto));
		break;
		case NICK_PROP:
			*val = phui::js::NewString(UTF16(detail.shortName()));
		break;
		case CALLSIGN_PROP:
			*val = phui::js::NewString(UTF16(detail.callsign()));
		break;
		case OFFICERS_PROP:
			*val = INT_TO_JSVAL(detail.officers());
		break;
		case HOME_PROP: {
				HCUnit* unit = HCUnit::Lookup(detail.m_unitID);

				if(unit)
					detail.setHomeCPID(unit->homeCPID());

				StratDB::StratObject* stratobj = StratDB::FindStratObject(detail.m_homeCPID);
				if(stratobj)
					*val = (new jsStratObject(stratobj->FullObjID()))->jsVal();
				else
					*val = JSVAL_NULL;
			}
		break;
		case OBJECTIVE_PROP:
		{
			StratDB::StratObject* stratobj = StratDB::FindStratObject(detail.objectiveCPID());
			if(stratobj)
			{
				*val = (new jsStratObject(stratobj->FullObjID()))->jsVal();
			}
			else
			{
				// lets check our home cp for defense objective flags
				stratobj = StratDB::FindStratObject(detail.homeCPID());
				if(stratobj && ((detail.side() == ALLIED && stratobj->StratFlags(true) & CP_STAT_ALLIED_DEFEND_OBJ) ||
							   (detail.side() == AXIS && stratobj->StratFlags(true) & CP_STAT_AXIS_DEFEND_OBJ)))
				{
					*val = (new jsStratObject(stratobj->FullObjID()))->jsVal();
				}
				else
				{
					*val = JSVAL_NULL;
				}
			}
		}
		break;
		case MOBILESPAWNS_PROP:
			*val = INT_TO_JSVAL(detail.m_mobileSpawns);
		break;
		case ISBRIGADE_PROP:
			*val = BOOLEAN_TO_JSVAL(detail.isBrigade());
		break;
		case ISSQUADBRIGADE_PROP:
			*val = BOOLEAN_TO_JSVAL(PlayerClient()->PlayerSquad().isSquadBrigade(detail.brigadeID()));
		break;
		case OWNER_PROP: // permanent owner of the brigade
			*val = phui::js::NewString(chatGetWideCallsign(detail.owner()));
		break;
		case OWNERRANK_PROP:
			*val = (new jsRank(detail.country(), detail.branch(), detail.ownerRank()))->jsVal();
		break;
		case OIC_PROP: // player who is currently in charge (may be null)
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic && oic->m_officer > 0)
				*val = phui::js::NewString(chatGetWideCallsign(oic->m_officer));
			else
				*val = phui::js::NewString(L"");
		}
		break;
		case OICRANK_PROP:
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic && oic->m_officer > 0)
				*val = (new jsRank(detail.country(), detail.branch(), oic->m_officerRank))->jsVal();
			else
				*val = (new jsRank(detail.country(), detail.branch(), 0))->jsVal();
		}
		break;
		case COMMANDER_PROP: // will return oic first, then owner
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic && oic->m_officer > 0)
				*val = phui::js::NewString(chatGetWideCallsign(oic->m_officer));
			else
				*val = phui::js::NewString(chatGetWideCallsign(detail.owner()));
		}
		break;
		case COMMANDERRANK_PROP:
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic && oic->m_officer > 0)
				*val = (new jsRank(detail.country(), detail.branch(), oic->m_officerRank))->jsVal();
			else
				*val = (new jsRank(detail.country(), detail.branch(), detail.ownerRank()))->jsVal();
		}
		break;
		case CANVOTE_PROP:
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic)
				*val = BOOLEAN_TO_JSVAL(oic->m_canVote);
			else
				*val = BOOLEAN_TO_JSVAL(false);
		}
		break;
		case CANTAKECHARGE_PROP:
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic)
				*val = BOOLEAN_TO_JSVAL(oic->m_canTakeCharge);
			else
				*val = BOOLEAN_TO_JSVAL(false);
		}
		break;
		case ACTIVEOIC_PROP:
		{
			BrigadeControlList* oic = PlayerClient()->LookupBrigadeOIC(detail.brigadeID());
			if(oic && oic->m_officer > 0)
				*val = BOOLEAN_TO_JSVAL(true);
			else
				*val = BOOLEAN_TO_JSVAL(false);
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsBrigade::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	return JS_TRUE;
}

/************************************************************************************************
	jsCountry
*************************************************************************************************/

static const char* countries[MAX_COUNTRIES] = {"None", "Britain", "USA", "France", "Germany", "Italy",
		"China", "Russia" };
static const char* countryadj[MAX_COUNTRIES] = {"None", "British", "American", "French", "German", "Italian",
		"Chinese", "Russian" };
static const char* countrycodes[MAX_COUNTRIES] = { "NONE", "UK", "US", "FR", "DE", "IT",
		"CN", "RU" };
static const WWII_SIDE countrysides[MAX_COUNTRIES] = { WWII_NO_SIDE, ALLIED, ALLIED, ALLIED, AXIS, AXIS,
		ALLIED, ALLIED };

JSClass jsCountry::jsclass = JS_CLASS_MACRO("country");
JSFunctionSpec jsCountry::jsfunctions[] = {{0}};
JSPropertySpec jsCountry::jsproperties[] = {{0}};

jsCountry::jsCountry(WWII_COUNTRY countryID)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	if(countryID < MAX_COUNTRIES)
	{
		DefineProperty("name", phui::js::NewString(countries[countryID]));
		DefineProperty("adjective", phui::js::NewString(countryadj[countryID]));
		DefineProperty("id",INT_TO_JSVAL(countryID));
		DefineProperty("code",phui::js::NewString(countrycodes[countryID]));
		DefineProperty("side", INT_TO_JSVAL(countrysides[countryID]));
	}
}

/************************************************************************************************
	jsRank
*************************************************************************************************/

JSClass jsRank::jsclass = JS_CLASS_MACRO("rank");
JSFunctionSpec jsRank::jsfunctions[] = {{0}};

JSPropertySpec jsRank::jsproperties[] =
{
	{"name",		jsRank::NAME_PROP,			JSPROP_PERMANENT | JSPROP_PERMANENT},
	{"level",		jsRank::LEVEL_PROP,		JSPROP_PERMANENT | JSPROP_PERMANENT},
	{"reallevel",	jsRank::REALLEVEL_PROP,	JSPROP_PERMANENT | JSPROP_PERMANENT},
	{"maxlevel",	jsRank::MAXLEVEL_PROP,	JSPROP_PERMANENT | JSPROP_PERMANENT},
	{"locked",	jsRank::LOCKED_PROP,		JSPROP_PERMANENT | JSPROP_PERMANENT},
	{"type",		jsRank::TYPE_PROP,			JSPROP_PERMANENT | JSPROP_PERMANENT},
	{0}
};

jsRank::jsRank(WWII_COUNTRY c, BRANCH_ENUM b, RANK_NO r)
			: phui::js::Object(&jsclass, jsfunctions,jsproperties), country(c), branch(b), level(0)
{
	JS_CONSTRUCTOR_LOCK;

	persRank const* rank = persrankFind(r);

	if(!rank) rank = persrankFind(1); // fallback to rank level 1 if need be
	if(rank) level = rank->Level(); // save the found rank

	// permanently save it
	//DefineProperty("level", INT_TO_JSVAL(level));
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsRank::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	// look it up from client state data sent from host
	UINT32 maxLevel = PlayerClient()->MaxRank(country, branch);

	// calculate effective rank
	UINT32 effectiveLevel = (level <= maxLevel) ? level: maxLevel;

	switch(propid)
	{
		case NAME_PROP:
		{
			persRank const* rank = persrankFind(effectiveLevel);
			if(rank) *val = phui::js::NewString(rank->WideName(country, branch));
		}
		break;
		case LEVEL_PROP:
		{
			*val = INT_TO_JSVAL(effectiveLevel);
		}
		break;
		case REALLEVEL_PROP:
		{
			*val = INT_TO_JSVAL(level);
		}
		break;
		case MAXLEVEL_PROP:
		{
			*val = INT_TO_JSVAL(maxLevel);
		}
		break;
		case LOCKED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(level >= maxLevel);
		}
		break;
		case TYPE_PROP:
		{
			persRank const* rank = persrankFind(effectiveLevel);
			if(rank) *val = INT_TO_JSVAL(rank->Type());
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsChatChannel
*************************************************************************************************/

JSClass jsChatChannel::jsclass = JS_CLASS_MACRO("chatchannel");
JSFunctionSpec jsChatChannel::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsChatChannel::jsproperties[] =
{
	{"name",	jsChatChannel::NAME_PROP,		JSPROP_PERMANENT},
	{"recv",	jsChatChannel::RECV_PROP,		JSPROP_PERMANENT},
	{"xmit",	jsChatChannel::XMIT_PROP,		JSPROP_PERMANENT},
	{"radioidx",jsChatChannel::radioidx_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsChatChannel
*************************************************************************************************/

jsChatChannel::jsChatChannel(int i)
			: phui::js::Object(&jsclass, jsfunctions,jsproperties)
			, radioidx(i)
{
	JS_CONSTRUCTOR_LOCK;
	if(radioidx >= MAX_CLIENT_CHANNELS) radioidx = 0;
};

jsChatChannel::~jsChatChannel()
{
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsChatChannel::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case NAME_PROP:
			*val = phui::js::NewString(ChatChannels[radioidx].name.c_str());
		break;
		case RECV_PROP:
			*val = BOOLEAN_TO_JSVAL(ChatChannels[radioidx].recv);
		break;
		case XMIT_PROP:
			*val = BOOLEAN_TO_JSVAL(ChatChannels[radioidx].xmit);
		break;
	}
	return JS_TRUE;
}

JSBool jsChatChannel::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case NAME_PROP:
		{
			const char* name = JS_GetStringBytes(JS_ValueToString(ctx, *val));

			if(name)
			{
				int type = NO_CHANNEL;

				if(strcasecmp(name,"brigade") == 0)
				{
					type = BRIGADE_CHANNEL;
				}
				else if(strcasecmp(name,"mission") == 0)
				{
					type = MISSION_CHANNEL;
				}
				else if(strcasecmp(name,"squad") == 0)
				{
					type = SQUAD_CHANNEL;
				}
				else if(strcasecmp(name,"gci") == 0)
				{
					type = GCI_CHANNEL;
				}
				else if(strcasecmp(name,"cp") == 0)
				{
					type = CP_CHANNEL;
				}
				else if(strcasecmp(name,"admin") == 0)
				{
					type = ADMIN_CHANNEL;
				}
				else if(strcasecmp(name,"origin") == 0)
				{
					type = ORIGIN_CP_CHANNEL;
				}
				else if(strcasecmp(name,"objective") == 0)
				{
					type = OBJECTIVE_CP_CHANNEL;
				}
				else if(strcasecmp(name,"side") == 0)
				{
					type = SIDE_GENERAL_CHANNEL;
				}
				else if(strcasecmp(name,"help") == 0)
				{
					type = SIDE_HELP_CHANNEL;
				}
				else if(strcasecmp(name,"ops") == 0)
				{
					type = SIDE_OPS_CHANNEL;
				}

				if(type != NO_CHANNEL)
				{
					chatTuneRadioToChannel(PlayerClient()->ChatConnection(), radioidx, type);
				}
				else
				{
					// Host will figure out if this is a numbered channel
					ChatHandle handleID = chatGetHandle(name);
					chatTuneRadioToHandle(PlayerClient()->ChatConnection(), radioidx, handleID);
				}
			}
			else
			{
				chatTuneRadioToChannel(PlayerClient()->ChatConnection(), radioidx, NO_CHANNEL);
			}
		}
		break;
		case RECV_PROP:
		{
			JSBool recv;
			if(JS_ValueToBoolean(ctx, *val, &recv))
			{
				ChatChannels[radioidx].recv = recv;
				chatUpdateChannelStatus(PlayerClient()->ChatConnection(), radioidx, recv);
			}
		}
		break;
		case XMIT_PROP:
		{
			JSBool xmit;
			if(JS_ValueToBoolean(ctx, *val, &xmit))
				ChatChannels[radioidx].xmit = xmit;
		}
		break;
	}
	return JS_TRUE;
}

/************************************************************************************************
	jsWeaponCounter
*************************************************************************************************/

JSClass jsWeaponCounter::jsclass = JS_CLASS_MACRO("weaponcounter");
JSFunctionSpec jsWeaponCounter::jsfunctions[] = {{0}};
JSPropertySpec jsWeaponCounter::jsproperties[] = {{0}};

jsWeaponCounter::jsWeaponCounter(WeaponCounter const& counter, int _index, char minimumRank)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("capacity", INT_TO_JSVAL(counter.capacity));
	DefineProperty("current", INT_TO_JSVAL(counter.current));
	DefineProperty("minimumrank", INT_TO_JSVAL(minimumRank));
	DefineProperty("index", INT_TO_JSVAL(_index));

	const char* spawnable = "invalid";
	switch(counter.spawnable)
	{
		case WeaponCounter::SPAWNABLE:			spawnable = "spawnable"; break;
		case WeaponCounter::NOT_DEPLOYED:	spawnable = "notdeployed"; break;
		case WeaponCounter::NOT_PRESENT:		spawnable = "notpresent"; break;
		case WeaponCounter::NOT_PERMITTED:	spawnable = "notpermitted"; break;
		//case WeaponCounter::LOCKED:				spawnable = "slocked"; break;
		default: break;
	}

	if(counter.restricted)
		spawnable = "slocked";

	DefineProperty("spawnable", phui::js::NewString(spawnable));
}

jsWeaponCounter::jsWeaponCounter(int capacity, int current, int weapIndex, char minRank, int availability)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("capacity", INT_TO_JSVAL(capacity));
	DefineProperty("current", INT_TO_JSVAL(current));
	DefineProperty("minimumrank", INT_TO_JSVAL(minRank));
	DefineProperty("index", INT_TO_JSVAL(weapIndex));

	const char* spawnable = "invalid";

	switch(availability)
	{
		case WeaponCounter::SPAWNABLE:		spawnable = "spawnable"; break;
		case WeaponCounter::NOT_DEPLOYED:	spawnable = "notdeployed"; break;
		case WeaponCounter::NOT_PRESENT:	spawnable = "notpresent"; break;
		case WeaponCounter::NOT_PERMITTED:	spawnable = "notpermitted"; break;
		//case WeaponCounter::LOCKED:		spawnable = "slocked"; break;
		default: break;
	}

	DefineProperty("spawnable", phui::js::NewString(spawnable));
}

/************************************************************************************************
	jsScenarioPlayer
*************************************************************************************************/

JSClass jsScenarioPlayer::jsclass = JS_CLASS_MACRO("scenarioplayer");
JSFunctionSpec jsScenarioPlayer::jsfunctions[] = {{0}};
JSPropertySpec jsScenarioPlayer::jsproperties[] = {{0}};

jsScenarioPlayer::jsScenarioPlayer(ScenarioPlayer& player)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("rank", new jsRank(WWII_NO_COUNTRY,BRANCH_NOT_USED,player.GetRank()));

	// check to make
	vehType const* vehtype = vehTypeGetBySpec(player.GetWeaponSpec());
	if(vehtype)
	{
		// we check the spec to make sure its a real vehicle
		const WEAPON_SPEC& spec = vehtype->WeaponSpec;
		if(spec.wCountry + spec.wCategory + spec.wClass + spec.wType > 0)
			DefineProperty("weapon", new jsVehtype(vehtype));
	}

	DefineProperty("callsign",phui::js::NewString(chatGetWideCallsign(player.handle)));
	DefineProperty("lastupdate",INT_TO_JSVAL(player.lastUpdate));
	DefineProperty("team",INT_TO_JSVAL(player.GetTeam()));
	DefineProperty("kills",INT_TO_JSVAL(player.GetKills()));
	DefineProperty("deaths",INT_TO_JSVAL(player.GetDeaths()));
	DefineProperty("score",INT_TO_JSVAL(player.GetScore()));
	DefineProperty("ready",BOOLEAN_TO_JSVAL(player.GetReady()));
	DefineProperty("spawned",BOOLEAN_TO_JSVAL(player.GetSpawned()));
	DefineProperty("friendly",BOOLEAN_TO_JSVAL(player.GetTeam() == SCEN_MGR->GetTeam()));
	DefineProperty("me",BOOLEAN_TO_JSVAL(player.IsMe()));

	//DefineProperty("missionleader",INT_TO_JSVAL((player.m_flags & RMLIST_ROOM_OWNER)));
	//DefineProperty("spawned",INT_TO_JSVAL((entry.m_flags & RMLIST_SPAWNED)));
	//DefineProperty("commander",INT_TO_JSVAL((entry.m_flags & RMLIST_PILOT)));
	//DefineProperty("wantscrew",INT_TO_JSVAL((entry.m_flags & RMLIST_WANTS_CREW)));
	//DefineProperty("iscrew", INT_TO_JSVAL((entry.m_flags & RMLIST_CREW)));
	//DefineProperty("serial", INT_TO_JSVAL(entry.m_serial));

	// define our arbitrary properties
	//std::map<std::string,std::string>::iterator it = player.attributes.begin();
	//for(; it != player.attributes.end(); it++)
	//	DefineProperty(it->first.c_str(),phui::js::NewString(it->second.c_str()));
}

/************************************************************************************************
	jsMissionRoomEntry
*************************************************************************************************/

JSClass jsMissionRoomEntry::jsclass = JS_CLASS_MACRO("missionroomentry");
JSFunctionSpec jsMissionRoomEntry::jsfunctions[] = {{0}};
JSPropertySpec jsMissionRoomEntry::jsproperties[] = {{0}};

jsMissionRoomEntry::jsMissionRoomEntry(RoomListEntry const& e, WWII_COUNTRY c, BRANCH_ENUM b)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties)
	, entry(e)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("rank", new jsRank(c,b,e.m_rank));

	// check to make
	vehType const* vehtype = vehTypeGetBySpec(entry.m_weapon);
	if(vehtype)
	{
		// we check the spec to make sure its a real vehicle
		const WEAPON_SPEC& spec = vehtype->WeaponSpec;
		if(spec.wCountry + spec.wCategory + spec.wClass + spec.wType > 0)
			DefineProperty("weapon", new jsVehtype(vehtype));
	}

	DefineProperty("callsign",phui::js::NewString(chatGetWideCallsign(entry.m_handle)));
	DefineProperty("missionleader",INT_TO_JSVAL((entry.m_flags & RMLIST_ROOM_OWNER)));
	DefineProperty("spawned",INT_TO_JSVAL((entry.m_flags & RMLIST_SPAWNED)));
	DefineProperty("commander",INT_TO_JSVAL((entry.m_flags & RMLIST_PILOT)));
	DefineProperty("wantscrew",INT_TO_JSVAL((entry.m_flags & RMLIST_WANTS_CREW)));
	DefineProperty("iscrew", INT_TO_JSVAL((entry.m_flags & RMLIST_CREW)));
	DefineProperty("serial", INT_TO_JSVAL(entry.m_serial));
}

/************************************************************************************************
	jsMissionMember
*************************************************************************************************/

// jsMissionMember and jsMissionRoomEntry probably need to be combined or one should
// be a subclass of the other ala jsMissionMember -> jsMissionRoomEntry, they are
// very similar

JSClass jsMissionMember::jsclass = JS_CLASS_MACRO("missionmember");
JSFunctionSpec jsMissionMember::jsfunctions[] = { {0} };
JSPropertySpec jsMissionMember::jsproperties[] = { {0} };

jsMissionMember::jsMissionMember(MissionMember const& member, WWII_COUNTRY c, BRANCH_ENUM b)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("rank",  new jsRank(c, b, member.m_rank));
	DefineProperty("position", new jsPosition((float)member.m_x, (float)member.m_y));
	DefineProperty("callsign",  phui::js::NewString(chatGetWideCallsign(member.m_handle)));
	DefineProperty("spawned", BOOLEAN_TO_JSVAL(member.isSpawned()));

	vehType const* vehtype = vehTypeGetBySpec(member.m_weapon);
	if(vehtype)
	{
		const WEAPON_SPEC& spec = vehtype->WeaponSpec;
		if(spec.wCountry + spec.wCategory + spec.wClass + spec.wType > 0)
			DefineProperty("weapon",  new jsVehtype(vehtype));
	}
}

/************************************************************************************************
	jsHCUnit
*************************************************************************************************/

JSClass jsHCUnit::jsclass = JS_CLASS_MACRO("hcunit");
JSFunctionSpec jsHCUnit::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsHCUnit::jsproperties[] =
{
	{"name",		jsHCUnit::NAME_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"id",			jsHCUnit::ID_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"parent",		jsHCUnit::PARENT_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"branch",		jsHCUnit::BRANCH_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"type",		jsHCUnit::TYPE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"owner",		jsHCUnit::OWNER_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"ownerrank",	jsHCUnit::OWNERRANK_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"home",		jsHCUnit::HOME_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"callsign",	jsHCUnit::CALLSIGN_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsHCUnit
*************************************************************************************************/

jsHCUnit::jsHCUnit(HCUnit& h)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, unitid(h.unitID())
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("country",  new jsCountry(h.country()));
};

jsHCUnit::jsHCUnit(UINT32 id)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, unitid(id)
{
	JS_CONSTRUCTOR_LOCK;
	HCUnit* unit = HCUnit::Lookup(unitid);
	assert(unit);
	if ( ! unit )
		return;

	DefineProperty("country",  new jsCountry(unit->country()));
};

jsHCUnit::~jsHCUnit()
{
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsHCUnit::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	*val = JSVAL_NULL;

	// This *was* a pointer, but the old team abused the privilage so we had to take that away.
	// TODO: A lot of this can be cached since it never changes.
	HCUnit* unit = HCUnit::Lookup(unitid);
	assert(unit);
	if ( ! unit )
		return JS_FALSE;

	switch(propid)
	{
		case NAME_PROP:
			if(unit != nullptr)
				*val = phui::js::NewString(uiUTF8toUTF16(unit->name()));
		break;

		case ID_PROP:
			if(unit != nullptr)
				*val = INT_TO_JSVAL(unit->unitID());
		break;

		case PARENT_PROP:
			if(unit != nullptr && unit->parent())
				*val = (new jsHCUnit(*unit->parent()))->jsVal();
		break;

		case BRANCH_PROP:
			if(unit != nullptr && unit->branch() < MAX_BRANCHES)
				*val = phui::js::NewString(branches[unit->branch()]);
		break;

		case TYPE_PROP:
			if(unit != nullptr)
				*val = INT_TO_JSVAL((UINT32)unit->type());
		break;

		case RANK_PROP:
			if(unit != nullptr)
				*val = INT_TO_JSVAL((UINT32)unit->rank());
		break;

		case OWNER_PROP:
			if(unit != nullptr)
				*val = phui::js::NewString(chatGetWideCallsign(unit->owner()));
		break;

		case OWNERRANK_PROP:
			if(unit != nullptr)
				*val = (new jsRank(unit->country(), unit->branch(), unit->ownerRank()))->jsVal();
		break;
		case HOME_PROP:
            {
				*val = JSVAL_NULL;

				if(unit != nullptr)
				{
					StratDB::StratObject* stratobj = StratDB::FindStratObject(unit->homeCPID());
					if(stratobj)
						*val = (new jsStratObject(stratobj->FullObjID()))->jsVal();
				}
            }
		break;

		case CALLSIGN_PROP:
			if(unit != nullptr)
				*val = phui::js::NewString(unit->callsign());
		break;

	}

	return JS_TRUE;
}

JSBool jsHCUnit::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	return JS_TRUE;
}


/************************************************************************************************
	jsMissionResults
*************************************************************************************************/

JSClass jsMissionResults::jsclass = JS_CLASS_MACRO("missionresults");
JSFunctionSpec jsMissionResults::jsfunctions[] = {{0}};
JSPropertySpec jsMissionResults::jsproperties[] = {{0}};

jsMissionResults::jsMissionResults(MISSION_RESULTS const& results)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	phui::js::Array* typehit, *numhit, *numkilled;

	DefineProperty("typehit", typehit = new phui::js::Array);
	DefineProperty("numhit", numhit = new phui::js::Array);
	DefineProperty("numkilled", numkilled = new phui::js::Array);

	StratDB::StratObject* stratobj = StratDB::FindStratObject(results.rtbFacility);
	if(stratobj)
		DefineProperty("rtbfac",new jsStratObject(stratobj->FullObjID()));

	for(UINT32 i = 0; i < results.numHits; i++)
	{
		vehType const* vehtype = vehTypeGetBySpec(results.hitArray[i].vehType);
		if(vehtype)
		{
			typehit->SetElement(new jsVehtype(vehtype),i);
			numhit->SetElement(INT_TO_JSVAL(results.hitArray[i].numVehiclesHit),i);
			numkilled->SetElement(INT_TO_JSVAL(results.hitArray[i].numVehiclesKilled),i);
		}
	}

	if(results.rtbReservedWeapon.serial > 0)
		DefineProperty("reservedweapon", new jsWeaponSerial(results.rtbReservedWeapon));

	DefineProperty("elapsedtime", INT_TO_JSVAL(results.elapsedTime));
	DefineProperty("rtbcode", INT_TO_JSVAL(results.rtb));
	DefineProperty("numkills", INT_TO_JSVAL(results.kills));
//	DefineProperty("originside", INT_TO_JSVAL(results.side));
	DefineProperty("rank", INT_TO_JSVAL(results.rank));
	DefineProperty("missionsuccess", INT_TO_JSVAL(results.missionSuccess));
	DefineProperty("handle", phui::js::NewString(chatGetWideCallsign(results.handleID)));
	DefineProperty("missionid", INT_TO_JSVAL(results.missionID));
	DefineProperty("numcapevents", INT_TO_JSVAL(results.captureEvents));
	DefineProperty("numcapassists", INT_TO_JSVAL(results.captureAssists));
	DefineProperty("promoted", BOOLEAN_TO_JSVAL(results.promoted));
	DefineProperty("missioninvalid",  BOOLEAN_TO_JSVAL(results.missionInvalid));
//	DefineProperty("facilityside", INT_TO_JSVAL(results.side));
	DefineProperty("mobilespawnees",INT_TO_JSVAL(results.mobileSpawnees));

	// latest additions
	DefineProperty("aishit",INT_TO_JSVAL(results.aisHit));
	DefineProperty("bridgeshit",INT_TO_JSVAL(results.bridgesHit));
	DefineProperty("bridgesrepaired",INT_TO_JSVAL(results.bridgesRepaired));
	DefineProperty("factorieshit",INT_TO_JSVAL(results.factorysHit));
	DefineProperty("firebaseshit",INT_TO_JSVAL(results.firebasesHit));
	DefineProperty("pointsscored",INT_TO_JSVAL(results.pointsScored));
	DefineProperty("pointsaccumulated",INT_TO_JSVAL(results.pointsAccumulated));
	DefineProperty("pointsnextrank",INT_TO_JSVAL(results.pointsNextRank));

	// latest latest additions
	DefineProperty("resupplies",INT_TO_JSVAL(results.m_resupplies));
	DefineProperty("scoredresupplies",INT_TO_JSVAL(results.m_resuppliesScored));

	const PersonaSelection& persona = PlayerClient()->PersonaSelected();

	if(results.nextRank)
		DefineProperty("nextrank",new jsRank(persona.m_countryID, persona.m_branchID, results.nextRank));
	else
		DefineProperty("nextrank",JSVAL_NULL);

	// latest latest latest additions
	// Number of kills scored near a table (as a guard)
	DefineProperty("guardingkills", INT_TO_JSVAL(results.m_guardingKills));
	// Number of kills on an enemy who was near a table (a capper)
	DefineProperty("capperkills", INT_TO_JSVAL(results.m_capperKills));
	// Callsign of the person who killed me
	if(results.m_killedBy) // Killed by another player?
		DefineProperty("killedby", phui::js::NewString(chatGetWideCallsign(results.m_killedBy)));
	else if(PlayerClient()->GetKilledByAI()) // Killed by AI?
		DefineProperty("killedby", phui::js::NewString(uiLocString("mission.aartab.aars.aar.ai").c_str()));
	else // Killed self, or MIA, RTB, etc...
		DefineProperty("killedby", phui::js::NewString("-"));
	// Bonus points for time spent defending a table
	DefineProperty("guardingpoints", INT_TO_JSVAL(results.m_guardingPoints));

	// Number of Unmanned Mobile Spawns they killed
	DefineProperty("umskills", INT_TO_JSVAL(results.m_umsKills));
}

/************************************************************************************************
	jsMotd
*************************************************************************************************/

#define MOTD_LIFETIME 300

JSClass jsMotd::jsclass = JS_CLASS_MACRO("motd");
JSFunctionSpec jsMotd::jsfunctions[] = { {0} };
JSPropertySpec jsMotd::jsproperties[] = { {0} };

jsMotd::jsMotd(MOTD_REPLY const& motd)
	: phui::js::Object(&jsclass, jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
//	requested = (UINT32)teulTime();
	DefineProperty("type", INT_TO_JSVAL(motd.type));
	DefineProperty("added", INT_TO_JSVAL(motd.added));
	DefineProperty("poster", phui::js::NewString(chatGetWideCallsign(motd.poster)));
	const wchar_t* wmessage = UTF16(motd.message.c_str());
	DefineProperty("message", phui::js::NewString(wmessage));
};

bool jsMotd::Valid()
{
	return false; //(requested + MOTD_LIFETIME > (UINT32)teulTime()) ? true: false;
}

/************************************************************************************************
	jsWeaponSerial
*************************************************************************************************/

JSClass jsWeaponSerial::jsclass = JS_CLASS_MACRO("weaponserial");
JSFunctionSpec jsWeaponSerial::jsfunctions[] = { {0} };
JSPropertySpec jsWeaponSerial::jsproperties[] = { {0} };

jsWeaponSerial::jsWeaponSerial(WEAPON_SERIAL const& s)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties)
	, serial(s)
{
	JS_CONSTRUCTOR_LOCK;
	vehType const* vehtype = vehTypeGetBySpec(serial.spec);
	if(vehtype)
		DefineProperty("type", new jsVehtype(vehtype));

	StratDB::StratObject* stratobj = StratDB::FindStratObject(serial.source);
	if(stratobj)
		DefineProperty("location", new jsStratObject(stratobj->FullObjID()));

	DefineProperty("serial", INT_TO_JSVAL(serial.serial));
}

/************************************************************************************************
	jsActiveMission
*************************************************************************************************/

JSClass jsActiveMission::jsclass = JS_CLASS_MACRO("activemission");
JSFunctionSpec jsActiveMission::jsfunctions[] = { {0} };
JSPropertySpec jsActiveMission::jsproperties[] = { {0} };

jsActiveMission::jsActiveMission(ActiveMission const& activeMission)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("country", INT_TO_JSVAL(activeMission.m_countryID));
	DefineProperty("branch",  INT_TO_JSVAL(activeMission.m_branchID));
	DefineProperty("brigade", INT_TO_JSVAL(activeMission.m_brigadeID));
	DefineProperty("mission", INT_TO_JSVAL(activeMission.m_missionID));
}

/************************************************************************************************
	jsSquad
*************************************************************************************************/

static bool SortSquadMembersByRank(Chat::Who const& lhs, Chat::Who const& rhs)
{
	if(!lhs.online() && rhs.online())
		return true;
	else if(lhs.online() && !rhs.online())
		return false;

	return lhs.squadRank() < rhs.squadRank();
}

JSClass jsSquad::jsclass = JS_CLASS_MACRO("squad");
JSFunctionSpec jsSquad::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsSquad, updatefield, 2),
	JS_FUNCTION_SPEC(jsSquad, requestmembers, 0),
	{0}
};

JSPropertySpec jsSquad::jsproperties[] =
{
	{"id",				jsSquad::ID_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"name",			jsSquad::NAME_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"shortname",		jsSquad::SHORTNAME_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"co",				jsSquad::CO_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"voting",			jsSquad::VOTING_PROP,		JSPROP_PERMANENT},
	{"recruiting",		jsSquad::RECRUITING_PROP,	JSPROP_PERMANENT},
	{"created",			jsSquad::CREATED_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"description",		jsSquad::DESCRIPTION_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"motd",			jsSquad::MOTD_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"homepage",		jsSquad::HOMEPAGE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"members",			jsSquad::MEMBERS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"rookiedays",		jsSquad::ROOKIEDAYS_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"modified",		jsSquad::MODIFIED_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"onlinemembers",	jsSquad::ONLINEMEMBERS_PROP,JSPROP_READONLY | JSPROP_PERMANENT},
	{"totalcount",		jsSquad::TOTALCOUNT_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"onlinecount",		jsSquad::ONLINECOUNT_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"night",			jsSquad::NIGHT_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"hour",			jsSquad::HOUR_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"timezone",		jsSquad::TIMEZONE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"language",		jsSquad::LANGUAGE_PROP,		JSPROP_PERMANENT},
	{"slogan",			jsSquad::SLOGAN_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsSquad
*************************************************************************************************/

jsSquad::jsSquad(const ClientSquad& s)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, squad(s)
{
	JS_CONSTRUCTOR_LOCK;
	members = new phui::js::Array;
};

jsSquad::~jsSquad()
{
}

/************************************************************************************************
	updatefield
*************************************************************************************************/

JS_FUNCTION_DEF(jsSquad, updatefield)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsSquad* squad = phui::js::Cast<jsSquad*>(ctx, jsobj);
	if(!squad) return JS_FALSE;

	const char* field = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!field) return JS_FALSE;

	const wchar_t* text = phui::js::ValueToWideString(argv[1]);
	if(!text) return JS_FALSE;

	SquadControl control;
	UINT32 type = 0;

	if(!strcmp(field, "name"))
		type = SquadControl::TEXT_NAME;
	else if(!strcmp(field, "homepage"))
		type = SquadControl::TEXT_HOMEPAGE;
	else if(!strcmp(field, "description"))
		type = SquadControl::TEXT_DESCRIPTION;
	else if(!strcmp(field, "motd"))
		type = SquadControl::TEXT_MOTD;
	else if(!strcmp(field, "night"))
		type = SquadControl::TEXT_NIGHT;
	else if(!strcmp(field, "slogan"))
		type = SquadControl::TEXT_SLOGAN;

	if(*text && type)
		control.sendText(PlayerClient()->ChatConnection(), type, std::string(UTF8(text)));

	return JS_TRUE;
}

/************************************************************************************************
	requestmembers
*************************************************************************************************/

JS_FUNCTION_DEF(jsSquad, requestmembers)
{
	*rval = JSVAL_NULL;

	SQUAD_MSG msg;

	msg.request(PlayerClient()->ChatConnection());

	return JS_TRUE;
}

/************************************************************************************************
	Property
*************************************************************************************************/

#define MAX_SQUADNIGHT_TEXT 64
#define MAX_LOCKEY_TEXT 25

JSBool jsSquad::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case ID_PROP:
			*val = INT_TO_JSVAL(squad.squadID());
		break;
		case NAME_PROP:
			*val = phui::js::NewString(UTF16(squad.name().c_str()));
		break;
		case SHORTNAME_PROP:
			*val = phui::js::NewString(UTF16(squad.callsign().c_str()));
		break;
		case CO_PROP:
		{
			ChatHandle coHandle = squad.coHandle();

			if(!coHandle)
			{
				const SquadMembers& squaddies = squad.members();
				SquadMembers::const_iterator it = squaddies.begin();
				for(; it != squaddies.end(); ++it)
				{
					if(it->second.squadRank() == 5) // CO
					{
						coHandle = it->second.handle();
						break;
					}
				}
			}

			*val = phui::js::NewString(chatGetWideCallsign(coHandle));
		}
		break;
		case VOTING_PROP:
			*val = BOOLEAN_TO_JSVAL(squad.votingEnabled());
		break;
		case RECRUITING_PROP:
			*val = BOOLEAN_TO_JSVAL(squad.recruiting());
		break;
		case CREATED_PROP:
		{
			time_t stamp(squad.created());

			char created[31];
			struct tm tmStruct;
			strftime(created, 30, "%a %b %d %X %Y", localtime_r(&stamp, &tmStruct));
			*val = phui::js::NewString(created);
		}
		break;
		case DESCRIPTION_PROP:
			*val = phui::js::NewString(UTF16(squad.description().c_str()));
		break;
		case MOTD_PROP:
			*val = phui::js::NewString(UTF16(squad.motd().c_str()));
		break;
		case HOMEPAGE_PROP:
			*val = phui::js::NewString(squad.homepage().c_str());
		break;
		case SLOGAN_PROP:
			*val = phui::js::NewString(UTF16(squad.slogan().c_str()));
		break;
		case ONLINEMEMBERS_PROP:
		case MEMBERS_PROP:
		{
			members->clear();

			int count = 0;
			std::vector<Chat::Who> whos;

			const SquadMembers& squaddies = squad.members(true);
			SquadMembers::const_iterator it = squaddies.begin();

			for(; it != squaddies.end(); ++it)
			{
				if(propid == MEMBERS_PROP || it->second.online())
					whos.push_back(it->second);
			}

			std::sort(whos.rbegin(), whos.rend(), SortSquadMembersByRank);
			std::vector<Chat::Who>::const_iterator who = whos.begin();

			for(; who != whos.end(); ++who)
			{
				jsChatWho* jswho = new jsChatWho(*who);
				members->SetElement(jswho, count++);
			}

			*val = members->jsVal();
		}
		break;
		case TOTALCOUNT_PROP:
			*val = INT_TO_JSVAL(squad.members(false).size());
		break;
		case ONLINECOUNT_PROP:
		{
			int online = 0;
			const SquadMembers& squaddies = squad.members(false);
			SquadMembers::const_iterator it = squaddies.begin();

			for(; it != squaddies.end(); ++it)
			{
				if(it->second.online())
					++online;
			}

			*val = INT_TO_JSVAL(online);
		}
		break;
		case ROOKIEDAYS_PROP:
			*val = INT_TO_JSVAL(squad.rookieDays());
		break;
		case MODIFIED_PROP:
			*val = BOOLEAN_TO_JSVAL(squad.modified());
		break;
		case DAYOFWEEK_PROP:
			*val = INT_TO_JSVAL(squad.squadNight());
		break;
		case HOUR_PROP:
			*val = INT_TO_JSVAL(squad.squadHour());
		break;
		case TIMEZONE_PROP:
			*val = INT_TO_JSVAL(squad.squadTimezone());
		break;
		case NIGHT_PROP:
		{
			// print friendly version of squad night
			char locKey[MAX_LOCKEY_TEXT+100];
			wchar_t friendlyNight[MAX_SQUADNIGHT_TEXT+100];

			*locKey = 0;
			*friendlyNight = 0;

			snprintf(locKey, 50, "common.squads.details.night.dow%d", squad.squadNight());

			const std::wstring& key = uiLocString(locKey);

			swprintf(friendlyNight,
				MAX_SQUADNIGHT_TEXT,
				L"%ls %.2u:00 %+.2d:00 UTC",
				key.c_str(),
				(unsigned int)squad.squadHour(),
				(int)squad.squadTimezone());

			*val = phui::js::NewString(friendlyNight);

			break;
		}
		case LANGUAGE_PROP:
			*val = INT_TO_JSVAL(squad.language());
		break;
	}

	return JS_TRUE;
}

JSBool jsSquad::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case RECRUITING_PROP:
		{
			JSBool enabled;
			if(JS_ValueToBoolean(ctx, *val, &enabled))
			{
				SquadControl control;
				control.sendInteger(PlayerClient()->ChatConnection(),
									SquadControl::INT_RECRUITING,
									(enabled) ? 1: 0);
			}
		}
		break;
		case LANGUAGE_PROP:
		{
			int32 language = 0;
			if(JS_ValueToInt32(ctx, *val, &language))
			{
				SquadControl control;
				control.sendInteger(PlayerClient()->ChatConnection(),
									SquadControl::INT_LANGUAGE,
									language);
			}
		}
		break;
		case VOTING_PROP:
		{
			JSBool voting;
			if(JS_ValueToBoolean(ctx, *val, &voting))
			{
				SquadControl control;
				control.sendInteger(PlayerClient()->ChatConnection(),
									SquadControl::INT_VOTING,
									(voting) ? 1: 0);
			}
		}
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsSquadInvite
*************************************************************************************************/

JSClass jsSquadInvite::jsclass = JS_CLASS_MACRO("squadinvite");
JSFunctionSpec jsSquadInvite::jsfunctions[] = { {0} };
JSPropertySpec jsSquadInvite::jsproperties[] =
{
	{"invited",			jsSquadInvite::INVITED_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsSquadInvite::jsSquadInvite(SquadInvite const& i)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, invite(i)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("valid",  BOOLEAN_TO_JSVAL(invite.valid()));
	DefineProperty("inviter",  phui::js::NewString(invite.inviter()));
	DefineProperty("squadname",phui::js::NewString(UTF16(invite.msg().squad().name().c_str())));
};

JSBool jsSquadInvite::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case INVITED_PROP:
			*val = INT_TO_JSVAL((UINT32)teulTime() - invite.invited());
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsChatWho
*************************************************************************************************/

JSClass jsChatWho::jsclass = JS_CLASS_MACRO("chatwho");
JSFunctionSpec jsChatWho::jsfunctions[] = { {0} };
JSPropertySpec jsChatWho::jsproperties[] =
{
	{"mission",	jsChatWho::MISSION_PROP, JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsChatWho::jsChatWho(Chat::Who const& who) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("handle", phui::js::NewString(chatGetWideCallsign(who.handle())));
	DefineProperty("insquad",   BOOLEAN_TO_JSVAL(who.inSameSquad()));
	DefineProperty("online", BOOLEAN_TO_JSVAL(who.online()));

	int rank = who.squadRank();
	bool locked = false;

	// is this me?
	if(who.handle() == PlayerClient()->Personas().m_callsign)
	{
		int maxRank = PlayerClient()->MaxSquadRank();

		if(rank >= maxRank)
		{
			rank = maxRank;
			locked = true;
		}
	}

	DefineProperty("squadrank", INT_TO_JSVAL(rank));
	DefineProperty("squadlocked", BOOLEAN_TO_JSVAL((locked) ? JSVAL_TRUE: JSVAL_FALSE));

	if(who.online())
	{
		DefineProperty("extended",  BOOLEAN_TO_JSVAL(who.extendedInfo()));
		DefineProperty("hc",  BOOLEAN_TO_JSVAL(who.highCommander()));
		DefineProperty("afk",   BOOLEAN_TO_JSVAL(who.afk()));

		if(who.country() && who.branch())
		{
			UINT32 id = (UINT32)who.branch();
			if(id < MAX_BRANCHES)
				DefineProperty("branch", phui::js::NewString(branches[id]));

			DefineProperty("country", new jsCountry(who.country()));
			DefineProperty("rank", new jsRank(who.country(), who.branch(), who.rank()));

			if(who.hcUnitID())
			{
				DefineProperty("brigade", new jsBrigade(who.hcUnitID()));

				if(who.inSameSquad())
				{
					missionID = who.missionID();

					DefineProperty("spawned", BOOLEAN_TO_JSVAL(who.spawned()));
					DefineProperty("leader", BOOLEAN_TO_JSVAL(who.leadingMission()));
					DefineProperty("missionid", INT_TO_JSVAL(missionID));

					StratDB::StratObject* stratobj = StratDB::FindStratObject(who.nearCP());
					if(stratobj)
						DefineProperty("nearestcp", new jsStratObject(stratobj->FullObjID()));
				}
			}
		}
	}

	if(who.inSameSquad())
	{
		UINT32 units = 0;

		DefineProperty("recruiting", BOOLEAN_TO_JSVAL(who.recruiting()));
		DefineProperty("joinedtf", INT_TO_JSVAL(getJoinedSquad(who.joinedSquad(), units)));
		DefineProperty("joined", INT_TO_JSVAL(units));
	}
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsChatWho::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case MISSION_PROP:
		{
			if(!mission.valid())
			{
				PlayerClient()->RequestMissionLookup(missionID);

				if(PlayerClient()->WaitForMessage(ww2::Client::MISSION_DETAIL_WAIT, 5) == 0)
					mission = new jsMission(PlayerClient()->MissionLookup());
			}

			*val = (mission.valid()) ? mission->jsVal(): JSVAL_NULL;
		}
		break;
	}

	return JS_TRUE;
}

jsChatWho::CHATWHO_TIMEFRAME
jsChatWho::getJoinedSquad(UINT32 joined, UINT32& numUnits) const
{
	if(!joined)
		return CWT_INVALID;

	numUnits = (UINT32)((teulTime() - joined)/Chat::Who::_SECONDS_PER_DAY);

	if(numUnits == 0)
		return CWT_HOURS;
	else if(numUnits < Chat::Who::_MAX_DELTA_DAYS)
		return CWT_DAYS;

	numUnits = (UINT32)(numUnits/Chat::Who::_DAYS_PER_MONTH);

	return CWT_MONTHS;
}

/************************************************************************************************
	jsPoll
*************************************************************************************************/

JSClass jsPoll::jsclass = JS_CLASS_MACRO("poll");
JSFunctionSpec jsPoll::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsPoll, vote, 1),
	{0}
};
JSPropertySpec jsPoll::jsproperties[] = { {0} };

jsPoll::jsPoll(OpinionPoll const& p)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties), poll(p)
{
	JS_CONSTRUCTOR_LOCK;

	options = new phui::js::Array;

	if(!poll.empty())
	{
		DefineProperty("title", phui::js::NewString(UTF16(poll.title())));
		DefineProperty("empty", BOOLEAN_TO_JSVAL(poll.empty()));

		for(unsigned int i = 0; i < poll.size(); ++i)
			options->SetElement(phui::js::NewString(UTF16(poll.entry(i))), i);
	}
	else
	{
		DefineProperty("title", phui::js::NewString(L""));
		DefineProperty("empty", BOOLEAN_TO_JSVAL(true));
	}

	DefineProperty("options",  options->jsVal());
}

/************************************************************************************************
	vote
*************************************************************************************************/

JS_FUNCTION_DEF(jsPoll, vote)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1) return JS_FALSE;

	jsPoll* jspoll = phui::js::Cast<jsPoll*>(ctx, jsobj);
	if(!jspoll) return JS_FALSE;

	int32 selection = 0;

	JS_ValueToInt32(ctx, argv[0], &selection);

	if(!jspoll->poll.empty() && selection >= 0 && selection < (int32)jspoll->poll.size())
		mapSendOpinionPollResult(PlayerClient()->MapConnection(), jspoll->poll.pollID(), selection);

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	jsBrigadeControlNotice
*************************************************************************************************/

JSClass jsBrigadeControlNotice::jsclass = JS_CLASS_MACRO("brigadenotice");
JSFunctionSpec jsBrigadeControlNotice::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsBrigadeControlNotice, veto, 1),
	{0}
};

JSPropertySpec jsBrigadeControlNotice::jsproperties[] =
{
	{"isvetoable",	jsBrigadeControlNotice::ISVETOABLE_PROP, JSPROP_READONLY | JSPROP_PERMANENT},
	{"expires",		jsBrigadeControlNotice::EXPIRES_PROP, JSPROP_READONLY | JSPROP_PERMANENT},
	{"expired",		jsBrigadeControlNotice::EXPIRED_PROP, JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsBrigadeControlNotice::jsBrigadeControlNotice(BrigadeControlNotice const& n, int i)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties), notice(n), index(i)
{
	JS_CONSTRUCTOR_LOCK;

	DefineProperty("index", INT_TO_JSVAL(index));
	DefineProperty("notice", INT_TO_JSVAL(notice.m_notice));
	DefineProperty("error", INT_TO_JSVAL(notice.m_error));
	DefineProperty("action", INT_TO_JSVAL(notice.m_action));
	DefineProperty("sponsor", phui::js::NewString(chatGetWideCallsign(notice.m_sponsor)));
	DefineProperty("officer", phui::js::NewString(chatGetWideCallsign(notice.m_officer)));
	DefineProperty("brigade", new jsBrigade(notice.m_brigadeID));
	DefineProperty("country", new jsCountry((WWII_COUNTRY)notice.m_country));
	DefineProperty("branch", INT_TO_JSVAL(notice.m_branch));
	DefineProperty("istargetted", BOOLEAN_TO_JSVAL(notice.isTargettedAction()));
	DefineProperty("window", INT_TO_JSVAL(notice.m_time));

	BrigadeControlDescriptor desc = notice.getDescription();

	DefineProperty("lockey", phui::js::NewString(desc.label.c_str()));
	DefineProperty("locarg1", phui::js::NewString(desc.arg1.c_str()));
	DefineProperty("locarg2", phui::js::NewString(desc.arg2.c_str()));
	DefineProperty("locarg3", phui::js::NewString(desc.arg3.c_str()));
	DefineProperty("locarg4", phui::js::NewString(desc.arg4.c_str()));

	StratDB::StratObject* object;
	object = StratDB::FindStratObject(notice.m_origin);
	if(object)
		DefineProperty("origin", new jsStratObject(object->FullObjID()));
	object = StratDB::FindStratObject(notice.m_target);
	if(object)
		DefineProperty("target", new jsStratObject(object->FullObjID()));
}

/************************************************************************************************
	veto
*************************************************************************************************/

JS_FUNCTION_DEF(jsBrigadeControlNotice, veto)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsBrigadeControlNotice* jsnotice = phui::js::Cast<jsBrigadeControlNotice*>(ctx, jsobj);
	if(!jsnotice) return JS_FALSE;

	bool vetoable = jsnotice->notice.isVetoable(PlayerClient()->OICOfBrigade(), PlayerClient()->IsHC());
	bool expired = (jsnotice->notice.m_received + jsnotice->notice.m_time < teulTime());

	if(jsnotice->notice.m_notice == BrigadeControlNotice::_PROPOSAL && vetoable && !expired)
	{
		BrigadeControlRequest req;

		switch(jsnotice->notice.m_action)
		{
			case BrigadeControlNotice::REQ_REMOVE:
			case BrigadeControlNotice::REQ_NOMINATE:
				req.vetoOIC(PlayerClient()->ChatConnection(),
					        jsnotice->notice.m_brigadeID,
							jsnotice->notice.m_officer);
				break;
			case BrigadeControlNotice::REQ_ATTACK:
			case BrigadeControlNotice::REQ_DEFEND:
			case BrigadeControlNotice::REQ_CLEAR:
				req.vetoObjective(PlayerClient()->ChatConnection(),
							jsnotice->notice.m_brigadeID,
							jsnotice->notice.m_target);
				break;
			case BrigadeControlNotice::REQ_MOVE:
				req.vetoBrigadeMove(PlayerClient()->ChatConnection(),
					        jsnotice->notice.m_brigadeID,
							jsnotice->notice.m_target,
							jsnotice->notice.m_origin);
				break;
		}

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsBrigadeControlNotice::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case ISVETOABLE_PROP:
			*val = BOOLEAN_TO_JSVAL(notice.isVetoable(PlayerClient()->OICOfBrigade(), PlayerClient()->IsHC()));
			break;
		case EXPIRES_PROP:
			*val = INT_TO_JSVAL((notice.m_received + notice.m_time) - teulTime());
			break;
		case EXPIRED_PROP:
			*val = BOOLEAN_TO_JSVAL((notice.m_received + notice.m_time < teulTime()));
			break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsRecruiter
*************************************************************************************************/

JSClass jsRecruiter::jsclass = JS_CLASS_MACRO("recruiter");
JSFunctionSpec jsRecruiter::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsRecruiter, join, 1),
	{0}
};

JSPropertySpec jsRecruiter::jsproperties[] =
{
	{0}
};

jsRecruiter::jsRecruiter(Recruiter const& r)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties), recruiter(r)
{
	JS_CONSTRUCTOR_LOCK;

	DefineProperty("valid", BOOLEAN_TO_JSVAL(recruiter.valid()));
	DefineProperty("handle", INT_TO_JSVAL(recruiter.handle()));
	DefineProperty("callsign", phui::js::NewString(chatGetWideCallsign(recruiter.handle())));
	DefineProperty("squadhandle", INT_TO_JSVAL(recruiter.squadHandle()));
	DefineProperty("squadshort", phui::js::NewString(chatGetWideCallsign(recruiter.squadHandle())));
	DefineProperty("squadname", phui::js::NewString(UTF16(recruiter.squadName().c_str())));
	DefineProperty("slogan", phui::js::NewString(UTF16(recruiter.squadSlogan().c_str())));
	DefineProperty("branchid", INT_TO_JSVAL(recruiter.branch()));
	DefineProperty("language", INT_TO_JSVAL(recruiter.language()));
	DefineProperty("country", new jsCountry((WWII_COUNTRY)recruiter.country()));
	DefineProperty("mission", INT_TO_JSVAL(recruiter.missionType()));
	DefineProperty("activity", INT_TO_JSVAL(recruiter.missionActivity()));

	StratDB::StratObject* strat = StratDB::FindStratObject(recruiter.missionOrigin());
	if(strat) DefineProperty("origin", new jsStratObject(strat->FullObjID()));

	strat = StratDB::FindStratObject(recruiter.missionTarget());
	if(strat) DefineProperty("target", new jsStratObject(strat->FullObjID()));

	if(recruiter.branch() < MAX_BRANCHES)
		DefineProperty("branch", phui::js::NewString(branches[recruiter.branch()]));
}

/************************************************************************************************
	join
*************************************************************************************************/

JS_FUNCTION_DEF(jsRecruiter, join)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	jsRecruiter* jsrecruiter = phui::js::Cast<jsRecruiter*>(ctx, jsobj);
	if(!jsrecruiter) return JS_FALSE;

	SquadControl control;

	// leave your current squad if you have one
	if(PlayerClient()->PlayerSquad().squadID())
		control.sendLeave(PlayerClient()->ChatConnection());

	// send the join request
	control.sendJoin(PlayerClient()->ChatConnection(), jsrecruiter->recruiter.handle());

	// init blocking
	PlayerClient()->SetWait(ww2::Client::SQUAD_RECRUITMENT_WAIT);

	// save this recruiter as your recruiter
	PlayerClient()->SetRecruiter(jsrecruiter->recruiter);

	// wait for a response
	if(PlayerClient()->WaitForMessage(ww2::Client::SQUAD_RECRUITMENT_WAIT, 100) == 0)
	{
		// are we now in the squad?
		if(PlayerClient()->PlayerSquad().shortHandle() == jsrecruiter->recruiter.squadHandle())
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsMissionInvite
*************************************************************************************************/

JSClass jsMissionInvite::jsclass = JS_CLASS_MACRO("missioninvite");
JSFunctionSpec jsMissionInvite::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsMissionInvite::jsproperties[] =
{
	{0}
};

jsMissionInvite::jsMissionInvite(MissionInvite const& i)
	: phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	DefineProperty("force", BOOLEAN_TO_JSVAL(i.m_isDirective));
	DefineProperty("handle", INT_TO_JSVAL(i.m_fromHandle));
	DefineProperty("callsign", phui::js::NewString(chatGetWideCallsign(i.m_fromHandle)));
	DefineProperty("squad", phui::js::NewString(chatGetWideCallsign(i.m_squadHandle)));
	DefineProperty("country", INT_TO_JSVAL(i.m_countryID));
	DefineProperty("branch", INT_TO_JSVAL(i.m_branchID));
	DefineProperty("brigade", INT_TO_JSVAL(i.m_brigadeID));
	DefineProperty("mission", INT_TO_JSVAL(i.m_missionID));
}

/************************************************************************************************
	jsInstance
*************************************************************************************************/

JSClass jsInstance::jsclass = JS_CLASS_MACRO("instance");
JSFunctionSpec jsInstance::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsInstance::jsproperties[] =
{
	{"category",	jsInstance::CATEGORY_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"type",		jsInstance::TYPE_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"handle",		jsInstance::HANDLE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"owner",		jsInstance::OWNER_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"id",			jsInstance::ID_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"result",		jsInstance::RESULT_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"mine",		jsInstance::MINE_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsInstance::jsInstance() : phui::js::Object(&jsclass,jsfunctions,jsproperties),
	lastResult(Instance::_SUCCESS), descriptor(&(PlayerClient()->GetInstance()))
{
	JS_CONSTRUCTOR_LOCK;
}

jsInstance::jsInstance(const Instance::Descriptor* d) : phui::js::Object(&jsclass,jsfunctions,jsproperties),
	lastResult(Instance::_SUCCESS), descriptor(d)
{
	JS_CONSTRUCTOR_LOCK;
}

void jsInstance::Update(int result)
{
	lastResult = result;
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsInstance::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case CATEGORY_PROP:
			*val = INT_TO_JSVAL(descriptor->category());
		break;
		case TYPE_PROP:
			*val = INT_TO_JSVAL(descriptor->type());
		break;
		case HANDLE_PROP:
			*val = INT_TO_JSVAL(descriptor->handle());
		break;
		case OWNER_PROP:
			*val = phui::js::NewString(chatGetWideCallsign(descriptor->handle()));
		break;
		case ID_PROP:
		{
			char idBuffer[32+1];
			strprintf(idBuffer, "%" PR64BitUT "", descriptor->id());
			*val = phui::js::NewString(idBuffer);
		}
		break;
		case RESULT_PROP:
			*val = INT_TO_JSVAL(lastResult);
		break;
		case MINE_PROP:
			*val = BOOLEAN_TO_JSVAL(descriptor->handle() == PlayerClient()->Personas().m_callsign);
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsInstanceState
*************************************************************************************************/

JSClass jsInstanceState::jsclass = JS_CLASS_MACRO("instancestate");
JSFunctionSpec jsInstanceState::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsInstanceState::jsproperties[] =
{
	{"description",	jsInstanceState::DESCRIPTION_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"age",			jsInstanceState::AGE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"runningfor",	jsInstanceState::RUNNINGFOR_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"team1",		jsInstanceState::TEAM1_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{"team2",		jsInstanceState::TEAM2_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsInstanceState::jsInstanceState(const Instance::ClientStatus* s) :
	phui::js::Object(&jsclass,jsfunctions,jsproperties), state(s)
{
	JS_CONSTRUCTOR_LOCK;

	DefineProperty("allies", INT_TO_JSVAL(state->allies()));
	DefineProperty("axis", INT_TO_JSVAL(state->axis()));
	DefineProperty("neutral", INT_TO_JSVAL(state->neutral()));
	DefineProperty("banned", INT_TO_JSVAL(state->banned()));
	DefineProperty("capacity", INT_TO_JSVAL(state->capacity()));
	DefineProperty("created", INT_TO_JSVAL(state->createdAt()));
	DefineProperty("started", INT_TO_JSVAL(state->startedAt()));
	DefineProperty("gametime", INT_TO_JSVAL(state->gameTime_m()));
	DefineProperty("squadmates", INT_TO_JSVAL(state->squadMates()));
	DefineProperty("full", BOOLEAN_TO_JSVAL(state->isFull()));
	DefineProperty("empty", BOOLEAN_TO_JSVAL(state->isEmpty()));
	DefineProperty("running", BOOLEAN_TO_JSVAL(state->isRunning()));
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsInstanceState::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case DESCRIPTION_PROP:
			*val = (new jsInstance(&state->descriptor()))->jsVal();
		break;
		case AGE_PROP:
			*val = INT_TO_JSVAL(state->ageS());
		break;
		case RUNNINGFOR_PROP:
			*val = INT_TO_JSVAL(state->runningForS());
		break;
		case TEAM1_PROP:
			*val = INT_TO_JSVAL(SCEN_MGR->GetPlayerCount(1));
		break;
		case TEAM2_PROP:
			*val = INT_TO_JSVAL(SCEN_MGR->GetPlayerCount(2));
		break;
	}

	return JS_TRUE;
}


/************************************************************************************************
	jsClientState
*************************************************************************************************/

JSClass jsClientState::jsclass = JS_CLASS_MACRO("clientstate");
JSFunctionSpec jsClientState::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsClientState, paccess, 1),
	{0}
};

JSPropertySpec jsClientState::jsproperties[] =
{
	{"upsells",				jsClientState::UPSELLS_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"cancapture",			jsClientState::CANCAPTURE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"cantrain",				jsClientState::CANTRAIN_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"maxsquadrank",		jsClientState::MAXSQUADRANK_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"timeleft",				jsClientState::TIMELEFT_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"subactive",			jsClientState::SUBACTIVE_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"cancapturecheck",		jsClientState::CANCAPTURECHECK_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsClientState::jsClientState() : phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
}

/************************************************************************************************
	paccess
*************************************************************************************************/

JS_FUNCTION_DEF(jsClientState, paccess)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1)
		return JS_TRUE;

	jsClientState* clientState = phui::js::Cast<jsClientState*>(ctx, jsobj);
	if(!clientState) return JS_FALSE;

	jsPersonaSummary* summary = phui::js::Cast<jsPersonaSummary*>(ctx, argv[0]);
	if(!summary) return JS_FALSE;

	if(PlayerClient()->MaxRank(summary->Summary().m_countryID, summary->Summary().m_branchID) > 0)
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	isavailable
*************************************************************************************************/

JS_FUNCTION_DEF(jsClient, isavailable)
{

#define AVAIL "available"
#define INACT "inactive"

	jsPersonaSummary* summary = phui::js::Cast<jsPersonaSummary*>(ctx, argv[0]);
	if(!summary) return JS_FALSE;
	else if(strncmp(summary->Summary().availability(), AVAIL, strlen(AVAIL)) == 0 ||
            strncmp(summary->Summary().availability(), INACT, strlen(INACT)) == 0)
        return JS_TRUE;
	else
        return JS_FALSE; // "onleave", "banned", "delayed", "noswap", "arenalock", "notsubbed"
}


/************************************************************************************************
	ismissionleader
*************************************************************************************************/
JS_FUNCTION_DEF(jsClient, ismissionleader)
{
	if(PlayerClient()->IsMissionLeader())
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	else
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	return JS_TRUE;
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsClientState::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case UPSELLS_PROP:
			// Upsells is true when NO_ADS is false
			*val = BOOLEAN_TO_JSVAL(!(PlayerClient()->IsPermitted(NO_ADS)));
		break;
		case CANCAPTURE_PROP:
			*val = BOOLEAN_TO_JSVAL(PlayerClient()->IsPermitted(CAN_CAPTURE));
		break;
		case CANTRAIN_PROP:
			*val = BOOLEAN_TO_JSVAL(PlayerClient()->IsPermitted(FULL_TRAINING));
		break;
		case MAXSQUADRANK_PROP:
			*val = INT_TO_JSVAL(PlayerClient()->MaxSquadRank());
		break;
		case TIMELEFT_PROP:
			*val = INT_TO_JSVAL(PlayerClient()->SubscriptionExpires(false));
		break;
		case SUBACTIVE_PROP:
			*val = BOOLEAN_TO_JSVAL(PlayerClient()->IsPermitted(SUBSCRIPTION_ACTIVE, false));
		break;
		case CANCAPTURECHECK_PROP:
		{
			ARENA_VARS::iterator itr = g_arenaDescription.find("can_capture_check");
			int delay = 120;

			if(itr != g_arenaDescription.end())
				delay = atol(itr->second.c_str());

			*val = INT_TO_JSVAL(delay * 1000);
		}
		break;
	}

	return JS_TRUE;
}


/************************************************************************************************
	jsCaptureState
*************************************************************************************************/

JSClass jsCaptureState::jsclass = JS_CLASS_MACRO("capturestate");
JSFunctionSpec jsCaptureState::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsCaptureState::jsproperties[] =
{
	{"valid",					jsCaptureState::VALID_PROP,						JSPROP_READONLY | JSPROP_PERMANENT},
	{"stage",				jsCaptureState::STAGE_PROP,					JSPROP_READONLY | JSPROP_PERMANENT},
	{"capturable",			jsCaptureState::CAPTURABLE_PROP,			JSPROP_READONLY | JSPROP_PERMANENT},
	{"pending",				jsCaptureState::PENDING_PROP,					JSPROP_READONLY | JSPROP_PERMANENT},
	{"pendingseconds",	jsCaptureState::PENDINGSECONDS_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{"progress",				jsCaptureState::PROGRESS_PROP,				JSPROP_READONLY | JSPROP_PERMANENT},
	{"rate",					jsCaptureState::RATE_PROP,						JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

jsCaptureState::jsCaptureState() : phui::js::Object(&jsclass,jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
}

/************************************************************************************************
	GetProperty
*************************************************************************************************/

JSBool jsCaptureState::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	const CaptureState& state = PlayerClient()->GetCaptureState();

	switch(propid)
	{
		case VALID_PROP:
			*val = BOOLEAN_TO_JSVAL(state.m_valid);
		break;
		case STAGE_PROP:
			*val = INT_TO_JSVAL(state.GetStage());
		break;
		case CAPTURABLE_PROP:
			*val = BOOLEAN_TO_JSVAL(state.IsCapturable());
		break;
		case PENDING_PROP:
			*val = BOOLEAN_TO_JSVAL(state.IsPending());
		break;
		case PENDINGSECONDS_PROP:
			*val = INT_TO_JSVAL(state.GetPendingSeconds());
		break;
		case PROGRESS_PROP:
			*val = INT_TO_JSVAL(state.GetProgress());
		break;
		case RATE_PROP:
			*val = INT_TO_JSVAL(state.GetLastContribution());
		break;
	}

	return JS_TRUE;
}

void setChatRateLimiting()
{
	ARENA_VARS::iterator it;

	it = g_arenaDescription.find("chat_rate_samples");
	if(it != g_arenaDescription.end())
		sscanf(it->second.c_str(), "%d", &_chatRateSamples);

	it = g_arenaDescription.find("chat_rate_threshold");
	if(it != g_arenaDescription.end())
		sscanf(it->second.c_str(), "%d", &_chatRateThreshold);

	it = g_arenaDescription.find("chat_rate_penalty");
	if(it != g_arenaDescription.end())
		sscanf(it->second.c_str(), "%d", &_chatRatePenalty);
}

