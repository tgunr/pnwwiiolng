#pragma once

#include <phui/Javascript.h>

namespace Auth {
	struct GameEntry;
}
class AuthConnection;


// HACK: The whole handler/manager thing is a bit of an anti-pattern, but it's so painful
// to add functionality elsewhere that in this case it's probably for the best... :/
class jsAuthHandler : public phui::js::Object {
public:
	jsAuthHandler(void);
	virtual ~jsAuthHandler();

	JS_CLASS_DATA;
	JS_FUNCTION(Connect);
	JS_FUNCTION(Disconnect);
	JS_FUNCTION(CheckMessages); // HACK: Shouldn't be called from GUI.
	JS_FUNCTION(Login);
	JS_FUNCTION(PollArenas);
	JS_FUNCTION(JoinArena);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val) override;

	phui::js::array_root_ptr arenas;

	// HACK: For disconnecting as a result of a login request being approved
	bool checking_messages = false;
	bool disconnect_requested = false;
};


class jsGameArena : public phui::js::Object {
public:
	jsGameArena(const Auth::GameEntry&, bool is_playnet);
	JS_CLASS_DATA;

	const Auth::GameEntry& entry;
};