#include "syscfg.h"
#include "clientConnections.h"

#include "steam_api.h"

#include "build_info.hpp"
#include "auth/authAppIDs.h"
#include "auth/authConstants.h"
#include "auth/authPack.h"
#include "auth/authPlaygateTypes.h"
#include "rn/rnmain.h"
#include "teul/teulEndpoint.h"
#include "teul/teulKit.h"
#include "ui/uiUTF.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiUTF.h"

//==============================================================================

#if 1 // HACKHACKHACK
// Playgate's MD5 is apparently different than the stuff used by WW2.exe. Blech.
namespace {
	static unsigned char PADDING[64] = {
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	/* F, G, H and I are basic MD5 functions.
	*/
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

	/* ROTATE_LEFT rotates x left n bits.
	*/
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

	/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
	Rotation is separate from addition to prevent recomputation.
	*/
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (UINT32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (UINT32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (UINT32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (UINT32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

	using UINTM = unsigned int;
	using COND = unsigned int;

	struct MD5_CTX {
		UINT32 state[4];                                   /* state (ABCD) */
		UINT32 count[2];        /* number of bits, modulo 2^64 (lsb first) */
		unsigned char buffer[64];                         /* input buffer */
	};

	void MD5Init(MD5_CTX* context) {
		context->count[0] = context->count[1] = 0;
		context->state[0] = 0x67452301;
		context->state[1] = 0xefcdab89;
		context->state[2] = 0x98badcfe;
		context->state[3] = 0x10325476;
	}

	static void MD5_memcpy(unsigned char* output, unsigned char* input, unsigned int len) {
		unsigned int i;

		for ( i = 0; i < len; i++ )
			output[i] = input[i];
	}

	static void MD5_memset(unsigned char* output, int value, unsigned int len) {
		unsigned int i;

		for ( i = 0; i < len; i++ )
			((char *)output)[i] = (char)value;
	}

	static void Decode(UINT32* output, unsigned char* input, unsigned int len) {
		unsigned int i, j;

		for ( i = 0, j = 0; j < len; i++, j += 4 )
			output[i] = ((UINT32)input[j]) | (((UINT32)input[j+1]) << 8) |
			(((UINT32)input[j+2]) << 16) | (((UINT32)input[j+3]) << 24);
	}

	static void MD5Transform(UINT32 state[4], unsigned char block[64]) {
		UINT32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

		Decode(x, block, 64);

		/* Round 1 */
		FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
		FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
		FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
		FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
		FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
		FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
		FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
		FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
		FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
		FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
		FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
		FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
		FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
		FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
		FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
		FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

												 /* Round 2 */
		GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
		GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
		GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
		GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
		GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
		GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
		GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
		GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
		GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
		GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
		GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
		GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
		GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
		GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
		GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
		GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

												 /* Round 3 */
		HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
		HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
		HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
		HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
		HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
		HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
		HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
		HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
		HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
		HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
		HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
		HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
		HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
		HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
		HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
		HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

												 /* Round 4 */
		II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
		II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
		II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
		II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
		II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
		II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
		II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
		II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
		II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
		II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
		II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
		II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
		II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
		II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
		II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
		II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;

		/* Zeroize sensitive information.
		*/
		MD5_memset((unsigned char*)x, 0, sizeof(x));
	}

	void MD5Update(MD5_CTX* context, unsigned char* input, unsigned int inputLen) {
		unsigned int i, index, partLen;

		/* Compute number of bytes mod 64 */
		index = (unsigned int)((context->count[0] >> 3) & 0x3F);

		/* Update number of bits */
		if ( (context->count[0] += ((UINT32)inputLen << 3))
			< ((UINT32)inputLen << 3) )
			context->count[1]++;
		context->count[1] += ((UINT32)inputLen >> 29);

		partLen = 64 - index;

		/* Transform as many times as possible.
		*/
		if ( inputLen >= partLen ) {
			MD5_memcpy
				((unsigned char*)&context->buffer[index], (unsigned char*)input, partLen);
			MD5Transform(context->state, context->buffer);

			for ( i = partLen; i + 63 < inputLen; i += 64 )
				MD5Transform(context->state, &input[i]);

			index = 0;
		} else
			i = 0;

		/* Buffer remaining input */
		MD5_memcpy
			((unsigned char*)&context->buffer[index], (unsigned char*)&input[i],
				inputLen-i);
	}

	static void Encode(unsigned char* output, UINT32* input, unsigned int len) {
		unsigned int i, j;

		for ( i = 0, j = 0; j < len; i++, j += 4 ) {
			output[j] = (unsigned char)(input[i] & 0xff);
			output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
			output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
			output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
		}
	}

	void MD5Final(unsigned char digest[16], MD5_CTX* context) {
		unsigned char bits[8];
		unsigned int index, padLen;

		/* Save number of bits */
		Encode(bits, context->count, 8);

		/* Pad out to 56 mod 64.
		*/
		index = (unsigned int)((context->count[0] >> 3) & 0x3f);
		padLen = (index < 56) ? (56 - index) : (120 - index);
		MD5Update(context, PADDING, padLen);

		/* Append length (before padding) */
		MD5Update(context, bits, 8);
		/* Store state in digest */
		Encode(digest, context->state, 16);

		/* Zeroize sensitive information.
		*/
		MD5_memset((unsigned char*)context, 0, sizeof(*context));
	}

	//--------------

	void getMD5FromString(char *string, unsigned char *digest) {
		MD5_CTX context;
		MD5Init(&context);
		MD5Update(&context, (unsigned char*)string, strlen(string));
		MD5Final(digest, &context);
	}

	int convertMD5StringToHex(const unsigned char *md5string, unsigned char *md5hex) {
		unsigned int i;
		unsigned int q;

		q = 0;

		for ( i = 0; i < 16; i++ ) {
			sprintf((char*)md5hex+q, "%02x", md5string[i]);
			q += 2;
		}

		return q+2; // how many bytes written
	}
}
#endif // MD5 HACK

//==============================================================================

UsernamePassword::UsernamePassword(const std::string& un, const std::wstring& pass)
: username(un) {
	SetPasswordString(pass);
}

void UsernamePassword::SetPasswordString(const std::wstring& str) {
	const size_t MAX_PASSWORD_LENGTH = 32;
	char utf8Password[(MAX_PASSWORD_LENGTH * 4) + 1] = {0};
	WWII::ConvertChars(str.c_str(), utf8Password, sizeof(utf8Password));

	UCHAR digest[256+1] = {0};
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
	const std::string ErrorCode("authconn.error.generic.errorcode");
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

	// I'd love to do exceptions for all of these, but it breaks the flow in the GUI scripts.
	// We can at least leave it for these two, though.
	if ( g_authconn != nullptr )
		throw std::runtime_error("authconn.error.init.multipleconnections");
	if ( CheckTeulCallbacks() != 0 )
		throw std::runtime_error("authconn.error.init.callbackinitfailed");

	UINT32 auth_ip = Teul::GetSingleIP(addr.c_str());
	if ( auth_ip == 0 ) {
		action = {ACT_ERROR, "authconn.error.init.couldnotresolve", uiUTF8toUTF16(addr.c_str())};
		return;
	}

	if ( (teul_index = teulOpenSend(auth_ip, WWII::Net::Ports::AuthHost, &::ConnectionOpened,
	                                3000, PROTOCOL_TCP, &::ConnectionError)) < 0 ) {
		action = {ACT_ERROR, ErrorCode, std::to_wstring(-teul_index)};
		return;
	}

	action = {ACT_CONNECT, States::Waiting};
	g_authconn = this;
}

//--------------

AuthConnection::~AuthConnection() {
	dbugLog(_FLI, "Closing AuthConnection");
	assert ( g_authconn == this );

	if ( teul_index >= 0 )
		teulClose(teul_index);

	g_authconn = nullptr;
}

//==============

bool AuthConnection::ConnectionGood(void) const {
	return teul_index >= 0;
}

bool AuthConnection::Connected(void) const {
	return is_connected;
}

bool AuthConnection::LoggedIn(void) const {
	return ConnectionGood() && is_connected && is_logged_in;
}

//==============

void AuthConnection::Login(const UsernamePassword& info) {
	dbugLog(_FLI, "Sending login request");

	if ( LoggedIn() ) {
		action = {ACT_ERROR, "authconn.error.alreadyloggedin"};
		return;
	}

	if ( info.username.length() < 2 || info.username.length() > 16 ) {
		action = {ACT_ERROR, "authconn.error.badusername.length"};
		return;
	}
	for ( char c : info.username )
		if ( ! (('0' <= c && c <= '9') || ('A' <= c && c <= 'z')) ) {
			action = {ACT_ERROR, "authconn.error.badusername.char", std::to_wstring(c)};
			return;
		}

	login_info = info;

	Auth::LoginRequest req(login_info.username, (const UCHAR*)login_info.password_md5);
	if ( teulSend(teul_index, _LoginXchg, &req) < 0 ) {
		action = {ACT_ERROR, UnknownError};
		return;
	}
	action = {ACT_LOGIN, States::Waiting};
}


//==============

void AuthConnection::PollArenas(void) {
	dbugLog(_FLD, "Polling arenas");
	uint32_t dummy = 0;
	if ( teulSend(teul_index, _GetArenaList, &dummy) < 0 ) {
		action = {ACT_ERROR, UnknownError};
		return;
	}
	action = {ACT_POLL_ARENAS, States::Waiting};
}

//==============

void AuthConnection::SelectArena(const Auth::GameEntry& entry) {
	dbugLog(_FLI, "Requesting to join arena %d", entry.m_arenaID);
	ARENA_REQ req{entry.m_arenaID, 0, false, presenter_version, 0};
	strlcpy(req.username, login_info.username.c_str(), array_size(req.username));
	//req.password ? unused on auth
	if ( teulSend(teul_index, _ArenaReq2, &req) < 0 ) {
		action = {ACT_ERROR, UnknownError};
		return;
	}
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
	strlcpy(req.username, login_info.username.c_str(), array_size(req.username));
	strlcpy((char*)req.password, (const char*)login_info.password_md5, array_size(req.password));
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
