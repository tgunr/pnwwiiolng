#ifndef PN_TUT_TUTORIALMANAGER_H
#define PN_TUT_TUTORIALMANAGER_H 1

#include <map>

#include "lua/luaMain.h"
#include "math/Vec3d.h"

// ----------- defines

#define TUTORIAL_MAX_OBJECTIVES 10
#define TUT_MGR TutorialManager::GetInstance()
#define TUT_ACTIVE TUT_MGR->IsActive()

// ----------- types

/*
	Event types are sent from the game layer to the
	lua layer. The lua layer can tell the tutorial
	manager which events its interested in and what
	callback lua function to run when they are sent.
	Many types will have associated data that tells
	the manager if the sent event is the one the
	tutorial script is interested in. For example,
	the location type will have a position vector
	and distance describing where and how close
	you need to get for the event to be fired.

	These are types as well as flags.
*/

// tolua_begin
enum TUT_EVENT_TYPE
{
	TUT_EVENT_GENERAL			= 1<<0,
	TUT_EVENT_START				= 1<<1,		//
	TUT_EVENT_STOP				= 1<<2,
	TUT_EVENT_SPAWN				= 1<<3,
	TUT_EVENT_DESPAWN			= 1<<4,
	TUT_EVENT_INPUT				= 1<<5,		// which
	TUT_EVENT_STATE_TROOP		= 1<<6,		// troop state we're changing too
	TUT_EVENT_STATE_VEHICLE		= 1<<7,		// vehicle state we're changing too
	TUT_EVENT_WEAPON_FIRE		= 1<<8,		// which weapon is fired
	TUT_EVENT_WEAPON_SWITCH		= 1<<9,		// which weapon we switched to
	TUT_EVENT_LOCATION			= 1<<10,	// radius around an xyz position world position
	TUT_EVENT_COLLISION			= 1<<11,	// collision with a game object
	TUT_EVENT_HIT				= 1<<12,	// shot a game object
	TUT_EVENT_TIMED				= 1<<13,	// send a timed event
	TUT_EVENT_ENTER				= 1<<14,	// enter a named building
	TUT_EVENT_LEAVE				= 1<<15,	// leave a named building
	TUT_EVENT_UPDATE			= 1<<16,	// called every 50ms
	TUT_EVENT_FPS_POSE			= 1<<17,	// pose change
	TUT_EVENT_UI_POPUP			= 1<<18,	// ui clicked OK
	TUT_EVENT_UI_DESTROYED		= 1<<19,	// ui unloaded
	TUT_EVENT_UI_READY			= 1<<20,	// sent when the ui is done giving feedback
	TUT_EVENT_KEY				= 1<<21,	// when a key is pressed
	TUT_EVENT_MAP				= 1<<22,	// various map events
	TUT_EVENT_CHANNEL			= 1<<23,	// chat channel tunning
	TUT_EVENT_CHAT				= 1<<24,	// send a chat
	TUT_EVENT_DIED				= 1<<25,	// player has died
	TUT_EVENT_AREA				= 1<<26,	// bounding box check of position
	TUT_EVENT_DAMAGE			= 1<<27,	// did damage to something
	TUT_EVENT_CAMERA			= 1<<28,	// called when the camera finishes a move of some type
	TUT_EVENT_MESSAGE			= 1<<29,	// arbitrary message
	TUT_EVENT_SET				= 1<<30,	// set a name/value pair
	TUT_EVENT_NUM_TYPES			= 31		// manually updated (always 1 + last one)
};

enum TUT_MAP_EVENT
{
	TUT_MAP_INVALID	= 0,
	TUT_MAP_CENTERME,
	TUT_MAP_ZOOMIN,
	TUT_MAP_ZOOMOUT,
	TUT_MAP_DRAG,
	TUT_MAP_WAYPOINT,
	TUT_MAP_REPORT,
	TUT_MAP_TOGGLE,
	TUT_MAP_ACTIVATE,
	TUT_MAP_NUM
};

// used to filter out which events can be sent by the UI
#define TUTORIAL_UI_EVENTS TUT_EVENT_UI_POPUP|TUT_EVENT_UI_DESTROYED|TUT_EVENT_UI_READY

// TUT_EVENT_UPDATE isn't included because its always updated; these are only updated if the tutorial wants them
#define TUTORIAL_UPDATE_EVENTS TUT_EVENT_TIMED|TUT_EVENT_LOCATION|TUT_EVENT_AREA

enum TUT_HANDLER_STATE
{
	THS_GARBAGE = -1,
	THS_START = 0,
	THS_EVENT_IN,
	THS_EVENT_OUT
};

struct TutorialHandler
{
	// tolua_end
	TutorialHandler() :
		name(""),
		scope(""),
		callback(""),
        eventType(TUT_EVENT_GENERAL),
		state(THS_START),
        called(0)
	{
	}

	// should we handle this type and state?
	bool Handle(TUT_EVENT_TYPE t) const;

	struct HandlerLocation
	{
		using COORDINATES = std::vector<Vec3d>;

		COORDINATES coords; // our world positions
		float d;			// location radius or buffer
	};

	struct HandlerData
	{
		int i;
		char s[255+1];
		HandlerLocation l;
	};

// tolua_begin

	// name of callback; used to index handler
	std::string name;

	// scope of callback; could be the name of a table
	std::string scope;

	// callback and name of handler
	std::string callback;

	// type of event to look for
	TUT_EVENT_TYPE eventType;

	// current handler state
	TUT_HANDLER_STATE state;

	// how many times has this handler been called
	int called;
// tolua_end

	// used for some special event types
	HandlerData data;
}; // tolua_export

using TutorialHandlerMap = std::map<std::string,TutorialHandler>;
using TutorialTimerMap = std::map<std::string,int>;
using TutorialCompletionMap = std::map<std::string,UINT64>;
using TutorialHandlerGarbage = std::vector<std::string>;

/************************************************************************************************
	TutorialManager
*************************************************************************************************/
// tolua_begin
class TutorialManager
{
public:
// tolua_end

	struct TutorialScript
	{
		TutorialScript() : id(""), path("")
		{
		}

		void Reset();

		std::string id;
		std::string path;
	};

    TutorialManager();
	~TutorialManager();

	// performs
	void Update();

	// is a tutorial active?
	bool IsActive() const;

	// do we handle this event type
	bool WantsEvent(UINT32 eventType);

	// send an event to the manager
	void DoEvent(UINT32 eventType, ...);

	// run a lua script in the context of our tutorial
	void RunScript(const char* path);
	void RunCode(const char* code);

	// tolua_begin

	// script interface

	// id of the tutorial and script to run
	bool Start(const char* id, const char* script = nullptr);

	// stops the current tutorial
	bool Stop(bool delayed = true);

	// add an event handler
	TutorialHandler* AddHandler(UINT32 eventType, const char* callback, const char* name); // regular events
	TutorialHandler* AddTimedHandler(const char* callback, const char* name, int duration); // timed events
	TutorialHandler* AddLocationHandler(const char* callback, const char* name, double x, double y, double z, float d); // location events
	TutorialHandler* AddAreaHandler(const char* callback, const char* name, double x, double y, double z, double x2, double y2, double z2); // area events
	// tolua_end

	TutorialHandler* AddAreaHandler(const char* callback, const char* name); // area events; this version is used when you need to populate the handler with points yourself

	// find a named handler
	TutorialHandler* FindHandler(const char* name);

	// tolua_begin

	// remove one
	void RemoveHandler(const char* name);

	// remove all handlers
	void RemoveHandlers();

	// set a tutorial as completed or not
	void SetComplete(const char* id, int key, bool completed);

	// returns whether a tutorial has been completed or not
	bool IsComplete(const char* id, int key);

	// does the current tutorial allow strat or not
	bool AllowStrat() const;
	void SetStratAllow(bool allow);

	// tolua_end

	// init a tutorial group's completion flags
	void SetGroupFlags(const char* id, UINT64 flags);

	// override the default script loading path
	void SetPrePath(const char* path);

	// gets the master instance of the manager
	static TutorialManager* GetInstance(); // tolua_export

	// tutorial specific input handling
	static unsigned int Input(int param, unsigned int functionCode, unsigned int unused);

private:

	void _resetFlags();
	int _handleEvent(TutorialHandler& handler, va_list* args=nullptr);

	// script currently running
	TutorialScript _current;

	// next script to run
	TutorialScript _next;

	// lua state of tutorial
	lua_State* _tutorial;

	// event handlers registered by the current tutorial
	TutorialHandlerMap _eventHandlers;

	// event handlers to kill at the next update
	TutorialHandlerGarbage _garbage;

	// holds timer durations
	TutorialTimerMap _timerMap;

	// what flags we are currently handling
	UINT32 _eventFlags;

	// when to call the script layer update callback
	int _nextUpdate;

	// flag for stopping the tutorial
	bool _stop;

	// tutorials we have completed
	TutorialCompletionMap _completed;

	// strat control
	bool _allowStrat;

	// prepath
	std::string _prePath;

}; // tolua_export

#endif // PN_TUT_TUTORIALMANAGER_H
