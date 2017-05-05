#include <syscfg.h>
#include "jsAuthHandler.h"

#include <js/jsexn.h>

#include <auth/authPlaygateTypes.h> // TODO: Move
#include <client/clientConnections.h>
#include <ui/uiChat.h>



namespace {
	// HACK: Properly handling ownership in the GUI is a major PITA. To keep things simple,
	// we just have this one connection that all handlers act as an interface for.
	AuthConnection* authconn = nullptr;
}

//==============================================================================

JSClass jsAuthHandler::jsclass = JS_CLASS_MACRO("AuthHandler");

jsAuthHandler::jsAuthHandler(void)
: Object(&jsclass, jsfunctions, jsproperties) {
	JS_CONSTRUCTOR_LOCK;
}


jsAuthHandler::~jsAuthHandler() {
}

//==============

JSFunctionSpec jsAuthHandler::jsfunctions[] = {
	JS_FUNCTION_SPEC(jsAuthHandler, Connect, 1),
	JS_FUNCTION_SPEC(jsAuthHandler, Disconnect, 0),
	JS_FUNCTION_SPEC(jsAuthHandler, CheckMessages, 0),
	JS_FUNCTION_SPEC(jsAuthHandler, Login, 3),
	{0}
};

//--------------

JS_FUNCTION_DEF(jsAuthHandler, Connect) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler ) return JS_FALSE;

	// HACK: The default shouldn't be in the GUI code.
	std::string authaddr = "authdev.playnet.com";
	if ( argc > 0 )
		authaddr = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	try {
		delete authconn;
		authconn = new AuthConnection(authaddr);
	} catch ( std::exception& e ) {
		delete authconn;
		authconn = nullptr;
		JS_SetPendingException(ctx, phui::js::NewString(e.what()));
		return JS_FALSE;
	}

	return JS_TRUE;
}

//--------------

JS_FUNCTION_DEF(jsAuthHandler, Disconnect) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler ) return JS_FALSE;

	delete authconn;
	authconn = nullptr;
	return JS_TRUE;
}

//--------------

namespace {
	const char* const completion_callbacks[] = {
		"", "OnError", "OnConnect", "OnLogin", "OnArenaUpdate", "OnArenaJoin"
	};
	static_assert ( array_size(completion_callbacks) == AuthConnection::ACT_COUNT , "array/enum mismatch" );
}

JS_FUNCTION_DEF(jsAuthHandler, CheckMessages) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler ) return JS_FALSE;

	if ( ! authconn ) {
		*rval = BOOLEAN_TO_JSVAL(false);
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(true);

	const AuthConnection::Action& action = authconn->GetAction();
	assert ( 0 < action.action && action.action < AuthConnection::ACT_COUNT );
	using States = AuthConnection::States;

	jsval dummy;
	if ( action.action == AuthConnection::ACT_IDLE || action.state == States::Waiting ) {
		return JS_TRUE;
	} else if ( action.state == States::Complete ) {
		JS_CallFunctionName(ctx, jsobj, completion_callbacks[action.action], 0, nullptr, &dummy);
	} else if ( action.state == States::URLResponse ) {
		jsval argv[] = { phui::js::NewString(action.url.c_str()) };
		JS_CallFunctionName(ctx, jsobj, "OnURLResponse", array_size(argv), argv, &dummy);
	} else if ( action.state == States::FTPResponse ) {
		// TODO: Send URL also? Auth sends both...
		jsval argv[] = { phui::js::NewString(action.ftp.c_str()) };
		JS_CallFunctionName(ctx, jsobj, "OnFTPResponse", array_size(argv), argv, &dummy);
	} else {
		std::wstring msg(uiChatAssembleUILoc(action.state.c_str(), action.arg1.c_str()));
		jsval argv[] = { phui::js::NewString(msg.c_str()) };
		JS_CallFunctionName(ctx, jsobj, completion_callbacks[AuthConnection::ACT_ERROR], array_size(argv), argv, &dummy);
	}

	authconn->ClearAction();
	return JS_TRUE;
}

//--------------

JS_FUNCTION_DEF(jsAuthHandler, Login) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler || ! authconn || argc < 3 ) return JS_FALSE;

	std::string username(JS_GetStringBytes(JS_ValueToString(ctx, argv[0])));
	std::wstring password((wchar_t*)JS_GetStringChars(JS_ValueToString(ctx, argv[1])));
	JSBool save; JS_ValueToBoolean(ctx, argv[2], &save);

	UsernamePassword creds(username, password);
	authconn->Login(creds);

	if ( save && authconn->GetAction().action == AuthConnection::ACT_LOGIN )
		SaveUserInfo(creds);

	return JS_TRUE;
}

//==============

namespace {
	enum { CONNECTIONGOOD_PROP, CONNECTED_PROP, LOGGEDIN_PROP, NUM_PROPS };
}

JSPropertySpec jsAuthHandler::jsproperties[] = {
	{"connection_good", CONNECTIONGOOD_PROP, JSPROP_READONLY | JSPROP_PERMANENT},
	{"connected",       CONNECTED_PROP,      JSPROP_READONLY | JSPROP_PERMANENT},
	{"logged_in",       LOGGEDIN_PROP,       JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};
static_assert ( array_size(jsAuthHandler::jsproperties) == NUM_PROPS+1 , "array/enum mismatch" );


JSBool jsAuthHandler::GetProperty(JSContext* ctx, int propid, jsval* val) {
	switch ( propid ) {
	case CONNECTIONGOOD_PROP:
		*val = BOOLEAN_TO_JSVAL(authconn && authconn->ConnectionGood());
		return JS_TRUE;
	case CONNECTED_PROP:
		return JS_TRUE;
	case LOGGEDIN_PROP:
		*val = BOOLEAN_TO_JSVAL(authconn && authconn->LoggedIn());
		return JS_TRUE;
	default:
		// We actually add callbacks to this in the GUI, so non-standard properties are fine.
		// TODO: It's probably worth explicitly laying out the callbacks to give a little extra safety.
		//JS_ReportError(ctx, "jsAuthHandler has no property with propid %d", propid);
		return JS_TRUE;
	}
}

//==============================================================================

JSClass jsGameArena::jsclass = JS_CLASS_MACRO("GameArena");

jsGameArena::jsGameArena(const Auth::GameEntry& arena)
: Object(&jsclass) {
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("name", phui::js::ConstString(arena.m_name.c_str()));
	DefineProperty("state", INT_TO_JSVAL(arena.m_lockState));
}