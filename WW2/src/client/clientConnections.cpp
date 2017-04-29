#include <syscfg.h>
#include "clientConnections.h"

#include <steam_api.h>

#include <build_info.hpp>
#include <auth/authAppIDs.h>
#include <auth/authConstants.h>
#include <auth/authPack.h>
#include <auth/authPlaygateTypes.h>
#include <pa/md5.h>
#include <rn/rnmain.h>
#include <teul/teulKit.h>
#include <wwii/wwiiCallbacks.h>
#include <wwii/wwiiNetPorts.h>
#include <wwii/wwiiUTF.h>


// Placeholder
#define ConnException(...) std::runtime_error("Stuff happened")

//==============================================================================

void UsernamePassword::SetPasswordString(const std::wstring& str) {
	// Oh my god this garbage hurts my brain, but if I change how
	// it's done I'm worried that all current passwords will break.
	const size_t MAX_PASSWORD_LENGTH = 32;
	char utf8Password[(MAX_PASSWORD_LENGTH * 4) + 1] = {0};
	WWII::ConvertChars(str.c_str(), utf8Password, sizeof(utf8Password));

	UCHAR digest[256+1];
	memset(digest, 0, sizeof(digest));
	getMD5FromString(utf8Password, digest);
	convertMD5StringToHex(digest, password_md5);
}

//--------------

UsernamePassword GetSavedUserInfo(void) {
	// TODO: Use OS functionality for saving username/password
	return {};
}

//--------------

void SaveUserInfo(const UsernamePassword& info) {
	// TODO: Use OS functionality for saving username/password
}

//==============================================================================

const std::string AuthConnection::States::Waiting("authconn.waiting");
const std::string AuthConnection::States::Complete("authconn.complete");
const std::string AuthConnection::States::URLResponse("authconn.urlresponse");
const std::string AuthConnection::States::FTPResponse("authconn.ftpresponse");
namespace {
	const std::string ErrorCode("authconn.error.generic.netcode");
	const std::string UnknownError("authconn.error.generic.unknown");
}

namespace {
	// HACK: Teul is C-style garbage. What'cha gonna do? *shrugs*
	AuthConnection* g_authconn = nullptr;

	SINT32 CheckTeulCallbacks(void);
	void ConnectionOpened(ENDPOINT&);
	void ConnectionError(SINT32, const UCHAR*, SINT32, const TEUL_CALLBACK*, SINT32 error);

	TEUL_CALLBACK* _LoginXchg = nullptr;
	TEUL_CALLBACK* _LoginSteamXchg = nullptr;
	TEUL_CALLBACK* _GetArenaList = nullptr;
	TEUL_CALLBACK* _ArenaReq = nullptr;
	TEUL_CALLBACK* _ArenaReq2 = nullptr;
	TEUL_CALLBACK* _LaunchReq = nullptr;


	const uint32_t presenter_version = 0x00010109; //teulGetHost("0.1.1.9", &g_playgateVersion);
}

//==============================================================================

AuthConnection::AuthConnection(const std::string& addr) {
	dbugLog(_FLI, "Creating connection to %s", addr.c_str());

	if ( g_authconn != nullptr )
		throw ConnException("authconn.error.init.multipleconnections");
	if ( CheckTeulCallbacks() != 0 )
		throw ConnException("authconn.error.init.callbackinitfailed");

	UINT32 auth_ip = Teul::GetSingleIP(addr.c_str());
	if ( auth_ip == 0 )
		throw ConnException("authconn.error.init.couldnotresolve", addr);

	if ( (teul_index = teulOpenSend(auth_ip, WWII::Net::Ports::AuthHost, &::ConnectionOpened,
	                                3000, PROTOCOL_TCP, &::ConnectionError)) < 0 )
		throw ConnException(ErrorCode, std::to_wstring(retval));

	g_authconn = this;
}

//--------------

AuthConnection::~AuthConnection() {
	dbugLog(_FLI, "Closing AuthConnection");
	assert ( g_authconn == this );

	if ( teul_index > -1 )
		teulClose(teul_index);

	g_authconn = nullptr;
}

//==============

void AuthConnection::Login(const UsernamePassword& info) {
	dbugLog(_FLI, "Sending login request");

	// TODO: This is correct for callsigns. Verify the same for usernames
	if ( info.username.length() > 10 )
		throw ConnException("authconn.error.badusername.length");
	for ( char c : info.username )
		if ( ! (('0' <= c && c <= '9') || ('A' <= c && c <= 'z')) )
			throw ConnException("authconn.error.badusername.char", c);

	login_info = info;

	Auth::LoginRequest req(login_info.username, (const UCHAR*)login_info.password_md5);
	if ( teulSend(teul_index, _LoginXchg, &req) < 0 )
		throw ConnException(UnknownError);
	action = {ACT_LOGIN, States::Waiting};
}


void AuthConnection::Login(uint32_t steamid) {
	static_assert (std::is_same<AccountID_t, decltype(steamid)>::value &&
	               std::is_same<decltype(steamid), decltype(Auth::LoginSteamRequest::m_account)>::value,
	               "typdef mismatch");

	dbugLog(_FLI, "Sending Steam login request");
	Auth::LoginSteamRequest req(SteamApps()->GetAppOwner().GetAccountID());
	if ( teulSend(teul_index, _LoginSteamXchg, &req) < 0 )
		throw ConnException(UnknownError);
	action = {ACT_LOGIN, States::Waiting};
}

//==============

void AuthConnection::PollArenas(void) {
	dbugLog(_FLD, "Polling arenas");
	uint32_t dummy = 0;
	if ( teulSend(teul_index, _GetArenaList, &dummy) < 0 )
		throw ConnException(UnknownError);
	action = {ACT_POLL_ARENAS, States::Waiting};
}

//==============

void AuthConnection::SelectArena(const Auth::GameEntry& entry) {
	dbugLog(_FLI, "Requesting to join arena %d", entry.m_arenaID);
	ARENA_REQ req{entry.m_arenaID, 0, false, presenter_version, 0, steam_id};
	strlcpy(req.username, login_info.username.c_str(), ArraySize(req.username));
	//req.password ? unused on auth
	action = {ACT_CHOOSE_ARENA, States::Waiting};
}

//==============================================================================
//                               TEUL CALLBACKS

void AuthConnection::ConnectionOpened(const ENDPOINT& endpoint) {
	if ( action.action != AuthConnection::ACT_CONNECT )
		return;
	dbugLog(_FLI, "Auth connection opened.");
	is_connected = true;
	teul_index = endpoint.teulIndex();
	conn_error = 0;
	action.state = States::Complete;
}

//--------------

void AuthConnection::ConnectionError(SINT32 error) {
	dbugLog(_FLW, "Auth connection error: %d", error);
	is_connected = false;
	teul_index = -1; // Close connection? Need to check if all errors do this on their own.
	conn_error = error;
	action = {ACT_ERROR, ErrorCode, std::to_wstring(error)};
}

//--------------

void AuthConnection::LoginReplyReceived(const Auth::LoginReply& reply) {
	if ( action.action != ACT_LOGIN )
		return;
	dbugLog(_FLI, "Login reply %s[%c]", reply.m_code.c_str(), (reply.m_playnet) ? 'y' : 'n');

	if ( reply.m_code.length() == 0 )
		action.state = States::Complete;
	else if ( reply.m_code == "full")
		action.state = "authconn.error.login.arenafull";
	else if ( reply.m_code == "login" || reply.m_code == "internal" || reply.m_code == "unknown" )
		action.state = "authconn.error.login.system";
	else if ( reply.m_code == "game" )
		action.state = "authconn.error.login.game";
	else if ( reply.m_code == "install" )
		action.state = "authconn.error.login.version";
	else if ( reply.m_code == "locked" )
		action.state = "authconn.error.login.locked";
	else if ( reply.m_code == "invuser" )
		action.state = "authconn.error.login.username";
	else if ( reply.m_code == "invpass" )
		action.state = "authconn.error.login.password";
	else if ( reply.m_code == "inactive" )
		action.state = "authconn.error.login.subscription";
	else
		action.state = "authconn.error.login.unknown";
}

//--------------

void AuthConnection::ArenaUpdateReceived(ACTION_OBJECT* a) {
	if ( action.action != ACT_POLL_ARENAS )
		return;
	dbugLog(_FLD, "Arena update received");
	arenas.packer(TEUL_UNPACK, a);
	action.state = States::Complete;
}

//--------------

void AuthConnection::AppVerifyReceived(const APP_VERIFY& verify) {
	if ( action.action != ACT_CHOOSE_ARENA )
		return;
	dbugLog(_FLI, "AppVerify received");

	APP_LAUNCH_REQ req;
	strlcpy(req.username, login_info.username.c_str(), ArraySize(req.username));
	//req.password ? unused on auth
	req.steamID = steam_id;
	req.arenaID = verify.arenaID;
	req.sessionID = verify.sessionID;
	req.appID = verify.appID;
	static_assert ( std::numeric_limits<decltype(req.appVersion)>::max() >= std::numeric_limits<decltype(PN_build_version_value)>::max() ,
	                "Unable to store the app version in the assigned variable" );
	req.appVersion = PN_build_version_value;
	req.status = 0;
	req.cdKeySize = 0;
	req.ignoreMinorUpdates = false; // ???
	req.oVersion = 2; // ???

	if ( teulSend(teul_index, _LaunchReq, &req) < 0 )
		action.state = UnknownError;

	// No state change since we still need to wait for auth to process this new info
}

//--------------

namespace {
	const size_t MAX_URL = 256;
}

void AuthConnection::LaunchReplyReceived(const APP_LAUNCH& launch) {
	if ( action.action != ACT_CHOOSE_ARENA )
		return;
	dbugLog(_FLI, "AppLaunch received");

	switch ( launch.appID ) {
	case Auth::Apps::WWIIOL:
		rnsetServerInfo(launch.serverIP, launch.serverPort, launch.otp);
		action.state = States::Complete;
		break;
	case Auth::Apps::LOGIN_DENIED:
		action.state = "authconn.error.launch.denied";
		break;
	case Auth::Apps::BROWSER:
	case Auth::Apps::WEB_DOWNLOAD:
	case Auth::Apps::WEB_REDIRECT:
		action.state = States::URLResponse;
		action.url = launch.data;
		if ( (size_t)launch.dataSize > action.url.size() ) {
			action.state = States::FTPResponse;
			action.ftp = launch.data + action.url.size() + 1;
		}
		// ftpStart()
		break;
	default:
		dbugLog(_FLW, "Unknown APP_LAUNCH flag '%d'", launch.appID);
		action.state = UnknownError;
	}
}

//------------------------------------------------------------------------------

namespace {

void ConnectionOpened(ENDPOINT& endpoint) {
	if ( g_authconn )
		g_authconn->ConnectionOpened(endpoint);
}

void ConnectionError(SINT32 /*teulIndex*/, const UCHAR* /*data*/, SINT32 /*size*/,
                     const TEUL_CALLBACK* /*object*/, SINT32 error) {
	if ( g_authconn )
		g_authconn->ConnectionError(error);
}

//--------------

SINT32 SendLoginRequest(ACTION_OBJECT* a) {
	return ((Auth::LoginRequest*)a->localRawData)->packer(TEUL_PACK, a);
}

SINT32 SendLoginSteamRequest(ACTION_OBJECT* a) {
	return ((Auth::LoginSteamRequest*)a->localRawData)->packer(TEUL_PACK, a);
}

SINT32 RecvLoginReply(ACTION_OBJECT* a) {
	if ( ! g_authconn )
		return 0;

	Auth::LoginReply reply(a);
	g_authconn->LoginReplyReceived(reply);
	return a->offset;
}

//--------------

SINT32 RequestArenaList(ACTION_OBJECT* a) {
	return a->offset;
}

SINT32 RecvArenaList(ACTION_OBJECT* a) {
	if ( ! g_authconn )
		return 0;
	g_authconn->ArenaUpdateReceived(a);
	return a->offset;
}

//--------------

SINT32 SendArenaReq(ACTION_OBJECT* a) {
	return authPackARENA_REQ(TEUL_PACK, a->packed, a->offset,(ARENA_REQ *)a->localRawData, 0);
}

SINT32 SendArenaReq2(ACTION_OBJECT* a) {
	return authPackARENA_REQ2(TEUL_PACK, a->packed, a->offset, (ARENA_REQ *)a->localRawData, 0);
}

//--------------

SINT32 SendLaunchReq(ACTION_OBJECT* a) {
	APP_LAUNCH_REQ* AppLaunchReq = (APP_LAUNCH_REQ*)a->localRawData;
	AppLaunchReq->status = 0;
	return authPackAPP_LAUNCH_REQ2(TEUL_PACK, a->packed, a->offset, AppLaunchReq, 0);
}

//--------------

SINT32 RecvAppVerify(ACTION_OBJECT* a) {
	if ( !g_authconn )
		return 0;
	APP_VERIFY verify;
	a->offset = authPackAPP_VERIFY(TEUL_UNPACK, a->packed, a->offset, &verify, 0);
	g_authconn->AppVerifyReceived(verify);
	return a->offset;
}

//--------------

SINT32 RecvAppLaunch(ACTION_OBJECT* a) {
	if ( ! g_authconn )
		return 0;
	APP_LAUNCH launch;
	a->offset = authPackAPP_LAUNCH(TEUL_UNPACK, a->packed, a->offset, &launch, 0);
	g_authconn->LaunchReplyReceived(launch);
	return a->offset;
}

//------------------------------------------------------------------------------

SINT32 CheckTeulCallbacks(void) {
	if ( _LoginXchg != nullptr )
		return 0;

	BEGIN_CALLBACK_REGISTRATION(Auth::CallbackX) {
		REGISTER_SENDRECV(AUTH_XCHG_LOGIN_Y, SendLoginRequest, RecvLoginReply, _LoginXchg);
		REGISTER_SENDRECV(AUTH_XCHG_LOGIN_STEAM_Y, SendLoginSteamRequest, RecvLoginReply, _LoginSteamXchg);
		//REGISTER_RECEIVER(APP_PLAYGATE_OK_Y, ReceivePlaygateOk);
		REGISTER_SENDRECV(APP_XCHG_GAME_LIST_Y, RequestArenaList, RecvArenaList, _GetArenaList);
		REGISTER_SENDER(ARENA_REQ_Y, SendArenaReq, _ArenaReq);
		REGISTER_SENDER(ARENA_REQ2_Y, SendArenaReq2, _ArenaReq2);
		REGISTER_SENDER(APP_LAUNCH_REQ_Y, SendLaunchReq, _LaunchReq);
		REGISTER_RECEIVER(APP_VERIFY_Y, RecvAppVerify);
		REGISTER_RECEIVER(APP_LAUNCH_Y, RecvAppLaunch);
	} END_CALLBACK_REGISTRATION(Auth::CallbackX)
}

}