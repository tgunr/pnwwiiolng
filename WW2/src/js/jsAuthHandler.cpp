#include "syscfg.h"
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
: Object(&jsclass, jsfunctions, jsproperties)
, arenas(new phui::js::Array()) {
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
	JS_FUNCTION_SPEC(jsAuthHandler, PollArenas, 3),
	JS_FUNCTION_SPEC(jsAuthHandler, JoinArena, 1),
	{0}
};

//--------------

JS_FUNCTION_DEF(jsAuthHandler, Connect) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler ) return JS_FALSE;

	// HACK: The default really shouldn't be in the GUI code.
	std::string authaddr = "auth.playnet.com";
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

	if ( handler->checking_messages ) {
		handler->disconnect_requested = true;
	} else {
		delete authconn;
		authconn = nullptr;
	}
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
	if ( ! authconn ) return JS_FALSE;

	handler->checking_messages = true;

	const AuthConnection::Action& action = authconn->GetAction();
	assert ( 0 < action.action && action.action < AuthConnection::ACT_COUNT );
	using States = AuthConnection::States;

	jsval dummy;
	if ( action.action == AuthConnection::ACT_IDLE || action.state == States::Waiting ) {
		handler->checking_messages = false; // HACK: Early return
		return JS_TRUE;
	} else if ( action.state == States::Complete ) {
		if ( action.action == AuthConnection::ACT_POLL_ARENAS ) {
			handler->arenas->clear();
			size_t i = 0;
			for ( auto& arena : authconn->GetArenas().m_games )
				handler->arenas->SetElement(new jsGameArena(arena, authconn->GetArenas().isPlaynet), i++);
		}
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

	handler->checking_messages = false;
	if ( handler->disconnect_requested ) { // HACK: Repeated code
		delete authconn;
		authconn = nullptr;
		handler->disconnect_requested = false;
	}

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

//--------------

JS_FUNCTION_DEF(jsAuthHandler, PollArenas) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler || ! authconn ) return JS_FALSE;
	authconn->PollArenas();
	return JS_TRUE;
}

//--------------

JS_FUNCTION_DEF(jsAuthHandler, JoinArena) {
	jsAuthHandler* handler = phui::js::Cast<jsAuthHandler*>(ctx, jsobj);
	if ( ! handler || ! authconn || argc < 1 ) return JS_FALSE;

	jsGameArena* arena = phui::js::Cast<jsGameArena*>(ctx, argv[0]);
	if ( arena == nullptr )
		return JS_FALSE;

	authconn->SelectArena(arena->entry);

	return JS_TRUE;
}

//==============

namespace {
	enum { CONNECTIONGOOD_PROP, CONNECTED_PROP, LOGGEDIN_PROP, ARENAS_PROP, NUM_PROPS };
}

JSPropertySpec jsAuthHandler::jsproperties[] = {
	{"connection_good", CONNECTIONGOOD_PROP, JSPROP_READONLY | JSPROP_PERMANENT},
	{"connected",       CONNECTED_PROP,      JSPROP_READONLY | JSPROP_PERMANENT},
	{"logged_in",       LOGGEDIN_PROP,       JSPROP_READONLY | JSPROP_PERMANENT},
	{"arenas",          ARENAS_PROP,         JSPROP_READONLY | JSPROP_PERMANENT},
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
	case ARENAS_PROP:
		*val = arenas->jsVal();
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
JSFunctionSpec jsGameArena::jsfunctions[] = {{0}};
JSPropertySpec jsGameArena::jsproperties[] = {{0}};

jsGameArena::jsGameArena(const Auth::GameEntry& e, bool is_playnet)
: Object(&jsclass, jsfunctions, jsproperties)
, entry(e) {
	JS_CONSTRUCTOR_LOCK;
	DefineProperty("name", phui::js::ConstString(entry.m_name.c_str()));
	DefineProperty("id", INT_TO_JSVAL(entry.m_arenaID));
	
	const char* state;
	if ( entry.m_lockState >= AL_ENTRY )
		state = "maintenance";
	else if ( entry.m_lockState >= AL_CAPTURE )
		state = "locked";
	else if ( entry.m_lockState == AL_OFF )
		state = "online";
	else if ( entry.m_lockState == AL_SYNCING )
		state = "syncing";
	else if ( entry.m_lockState == AL_INITIALIZING )
		state = "starting";
	else if ( entry.m_lockState == AL_LOADING )
		state = "loading";
	else
		state = "offline";
	DefineProperty("state", phui::js::ConstString(state));

	bool can_join = (AL_OFF <= entry.m_lockState && entry.m_lockState < AL_ENTRY);
	if ( AL_ENTRY <= entry.m_lockState && is_playnet )
		can_join = true;
	DefineProperty("can_join", BOOLEAN_TO_JSVAL(can_join));
}