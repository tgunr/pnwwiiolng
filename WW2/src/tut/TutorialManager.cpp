#include <syscfg.h> // Path is project specific.
#include <stdarg.h>	// var args

#include "tut/TutorialManager.h"
#include "tut/tutGroupProgress.h"
#include "ui/uiMain.h"
#include "client/GameClient.h"
#include "sdm/sdm.h"
#include "include/vars.h"
#include "spt/Random.h"
#include "ui/uiChat.h"
#include "cfg/cfgPrefs.h"
#include "weap/weapType.h"
#include "math/Vec3d.h"
#include "osgImport/BoundingBox.h"
#include "trn/trnMain.h"

#include "math/mathvect.h"
#include "pers/persmain.h"
#include "veh/vehmain.h"
#include "trn/trncd.h"
#include "ob/obmain.h"

// defines

// included automatically by the manager
#define TUTORIAL_INCLUDE "include/tutorial.lua"

// how often to call script update callbacks (ms)
#define TUTORIAL_UPDATE 50

// private variables

// local functions
static bool _pointInArea(Vec3d& pos, TutorialHandler& handler);

// singleton
static TutorialManager _tutMgr;

/************************************************************************************************
	TutorialHandler
*************************************************************************************************/

bool TutorialHandler::Handle(TUT_EVENT_TYPE t) const
{
	return (eventType == t && state != THS_GARBAGE) ? true: false;
}

/************************************************************************************************
	TutorialManager
*************************************************************************************************/

TutorialManager::TutorialManager() :
	_tutorial(nullptr),
	_eventFlags(0),
	_nextUpdate(0),
	_stop(false),
	_allowStrat(true),
	_prePath("tutorials/")
{
}

TutorialManager::~TutorialManager()
{
}

void TutorialManager::TutorialScript::Reset()
{
	id = "";
	path = "";
}

void TutorialManager::Update()
{
	if(_stop)
	{
		Stop(false);
		return;
	}

	if(_next.id.size())
	{
		Start(_next.id.c_str(), _next.path.c_str());
		return;
	}

	TutorialHandlerMap::iterator it;

	// call the script layer's update method if it has one
	if(frametime >= _nextUpdate)
	{
		if(WantsEvent(TUTORIAL_UPDATE_EVENTS))
		{
			vehVehicle* vehicle = nullptr;

			it = _eventHandlers.begin();

			// handler some event types ourself
			for(; it != _eventHandlers.end(); it++)
			{
				TutorialHandler& handler = it->second;

				if(handler.state == THS_GARBAGE)
					continue;

				switch(handler.eventType)
				{
					case TUT_EVENT_TIMED:
					{
						if(frametime >= handler.data.i && _handleEvent(handler) > 0)
							handler.data.i = frametime + _timerMap[handler.name];
					}
					break;
					case TUT_EVENT_LOCATION:
					{
						if(!vehicle)
							vehicle = playerGetVehicle();

						if(vehicle)
						{
							// old game style of calculating the location
							Vec3d myPos		= Vec3d(vehicle->ModelData->VehiclePos);
							Vec3d& dataPos  = handler.data.l.coords.at(0);
							Vec3d diff		= myPos - dataPos;
							double length2	= (double)(handler.data.l.d * handler.data.l.d);

							//mathSubVectors(&diff, &vehicle->ModelData->VehiclePos, &location);
							TUT_HANDLER_STATE state	= (diff.length2() <= length2) ? THS_EVENT_IN: THS_EVENT_OUT;

							if(state != handler.state)
							{
								handler.state = state;
								_handleEvent(handler);
							}
						}
					}
					break;
					case TUT_EVENT_AREA:
					{
						if(!vehicle)
							vehicle = playerGetVehicle();

						if(vehicle)
						{
							// my position
							Vec3d pos;
							pos.set(vehicle->ModelData->VehiclePos);

							TUT_HANDLER_STATE state = THS_EVENT_OUT;

							if(handler.data.l.coords.size() == 2)
							{
								// world offset
								Vec3d worldOffset;

								FLOAT64VECTOR trnOffset;
								if(trnGetWorldOriginOffset(&trnOffset))
									worldOffset.set(worldOffset);

								Vec3d& minpnt = handler.data.l.coords.at(0);
								Vec3d& maxpnt = handler.data.l.coords.at(1);

								pos		-= worldOffset;
								minpnt	-= worldOffset;
								maxpnt	-= worldOffset;

								if(handler.data.i > 0 && pos.z() < handler.data.l.d)
								{
									state = THS_EVENT_OUT;
								}
								else if(handler.data.i < 0 && pos.z() > handler.data.l.d)
								{
									state = THS_EVENT_OUT;
								}
								else
								{
									// create bbox
									BoundingBox bbox(minpnt, maxpnt);
									state = (bbox.contains(pos)) ? THS_EVENT_IN: THS_EVENT_OUT;
								}
							}
							else if(handler.data.l.coords.size() > 2)
							{
								if(handler.data.i > 0 && pos.z() < handler.data.l.d)
								{
									state = THS_EVENT_OUT;
								}
								else if(handler.data.i < 0 && pos.z() > handler.data.l.d)
								{
									state = THS_EVENT_OUT;
								}
								else
								{
									state = (_pointInArea(pos, handler)) ? THS_EVENT_IN: THS_EVENT_OUT;
								}
							}

							if(state != handler.state)
							{
								handler.state = state;
								_handleEvent(handler);
							}
						}
					}
					break;

                    default:
                        break;
				}
			}
		}

		DoEvent(TUT_EVENT_UPDATE);
		_nextUpdate = frametime + TUTORIAL_UPDATE;
	}

	// go through and kill all handlers that have been flagged to be killed
	// they are flagged when called; its up to the script to re-add it if needed
	if(!_garbage.empty())
	{
		for(int i = 0; i < (int)_garbage.size(); i++)
			_eventHandlers.erase(_garbage[i]);

		// recalculate our event flags
		_resetFlags();

		// clear out the garbage
		_garbage.clear();
	}
}

bool TutorialManager::IsActive() const
{
	return (_tutorial != nullptr);
}

bool TutorialManager::WantsEvent(UINT32 eventType)
{
	return (_eventFlags & eventType);
}

void TutorialManager::RunScript(const char* path)
{
	if(IsActive())
		luaRunScript(path, _tutorial);
}

void TutorialManager::RunCode(const char* code)
{
	if(IsActive())
		luaRunObject("TutorialManager::RunCode", code, _tutorial);
}

void TutorialManager::DoEvent(UINT32 eventType, ...)
{
	if(IsActive() && WantsEvent(eventType))
	{
		TutorialHandlerMap::iterator it	= _eventHandlers.begin();
		va_list args;

		// loop over events and fire matched types
		for(; it != _eventHandlers.end(); it++)
		{
			TutorialHandler& handler = it->second;

			if(IsActive() && handler.Handle((TUT_EVENT_TYPE)eventType))
			{
				handler.state = THS_EVENT_IN;
				va_start(args, eventType);
				_handleEvent(handler, &args);
				va_end(args);
			}
		}
	}
}

bool TutorialManager::Start(const char* id, const char* script)
{
	// copy these because this called could originate from the script
	// layer; the Stop(false) call a few lines below will trash these
	// variables in that case
	_next.id	= id;
	_next.path	= (script) ? script: "";

	if(IsActive())
	{
		// tell the current script we are stopping
		Stop();

		// Update() will handle actually starting the new tutorial
		return true;
	}

	_tutorial = luaGetVM(false);

	if(IsActive())
	{
		_current.id	= _next.id;
		_eventFlags	= 0;

		if(_next.path.size())
		{
			_current.path = _next.path;
		}
		else
		{
			char buffer[255+1];
			strncpy(buffer, _current.id.c_str(), 255);

			char* dot = strchr(buffer, '.');

			while(dot && *dot == '.')
			{
				*dot = '/';
				dot = strchr(dot, '.');
			}

			_current.path = _prePath;
			_current.path += buffer;
			_current.path += ".lua";
		}

		// load the tutorial lib and script
		if(luaRunScript(TUTORIAL_INCLUDE, _tutorial) && luaRunScript(_current.path.c_str(), _tutorial))
			DoEvent(TUT_EVENT_START);
		else
			Stop();
	}

	_next.Reset();

	return true;
}

bool TutorialManager::Stop(bool delayed)
{
	if(IsActive())
	{
		if(delayed && !_stop)
		{
			_stop = true;
			return false;
		}

		// let the script layer know we've stopped
		DoEvent(TUT_EVENT_STOP);

		// stop the tutorial
		luaStopVM(_tutorial);

		// reset it
		_current.Reset();

		_tutorial	= nullptr;
		_eventFlags	= 0;
		_stop		= false;

		// clear out the current handlers
		RemoveHandlers();

		return true;
	}

	return false;
}

TutorialHandler* TutorialManager::FindHandler(const char* name)
{
	TutorialHandlerMap::iterator found = _eventHandlers.find(name);

	if(found != _eventHandlers.end())
		return &found->second;

	return nullptr;
}

TutorialHandler* TutorialManager::AddHandler(UINT32 eventType, const char* callback, const char* name)
{
	if(callback && *callback)
	{
		TutorialHandler handler;

		char buffer[128+1];
		strncpy(buffer, callback, 128);

		// now we check to see if the callback is scoped
		// callbacks can come in like OnUpdate or they
		// can come in like tutorial.HandlePoseEvent
		// the second version is scoped, so lets
		// check for that possibility

		// look for a point
		char* point = strstr(buffer, ".");

		// init out callback (c) and scope (s) strings
		const char* c = buffer;
		const char* s = nullptr;

		if(point)
		{
			// we found a point; null it
			*point = '\0';

			// set callback to the second part
			c = ++point;

			// set scope to the 1st part
			s = buffer;
		}

		// init our handler
		handler.eventType	= (TUT_EVENT_TYPE)eventType;
		handler.name		= (name && *name) ? name: callback;
		handler.callback	= c;
		handler.scope		= (s) ? s: "";

		_eventHandlers[handler.name] = handler;

		TutorialHandlerMap::iterator found = _eventHandlers.find(handler.name);

		if(found != _eventHandlers.end())
		{
			_eventFlags |= handler.eventType;
			TutorialHandler& h = found->second;
			return &h;
		}
	}

	return nullptr;
}

TutorialHandler* TutorialManager::AddTimedHandler(const char* callback, const char* name, int duration)
{
	TutorialHandler* handler = AddHandler(TUT_EVENT_TIMED, callback, name);

	if(handler)
	{
		int msduration = duration * 1000;
		handler->data.i = frametime + msduration;
		_timerMap[handler->name] = msduration;
	}

	return handler;
}

TutorialHandler* TutorialManager::AddLocationHandler(const char* callback, const char* name, double x, double y, double z, float d)
{
	TutorialHandler* handler = AddHandler(TUT_EVENT_LOCATION, callback, name);

	if(handler)
	{
		handler->data.l.coords.push_back(Vec3d(x, y, z));
		handler->data.l.d = d;
	}

	return handler;
}

TutorialHandler* TutorialManager::AddAreaHandler(const char* callback, const char* name, double x, double y, double z, double x2, double y2, double z2)
{
	TutorialHandler* handler = AddHandler(TUT_EVENT_AREA, callback, name);

	if(handler)
	{
		Vec3d minpt;
		Vec3d maxpt;

		minpt.x() = (x <= x2 ) ? x: x2;
		minpt.y() = (y <= y2 ) ? y: y2;
		minpt.z() = (z <= z2 ) ? z: z2;
		maxpt.x() = (x > x2 ) ? x: x2;
		maxpt.y() = (y > y2 ) ? y: y2;
		maxpt.z() = (z > z2 ) ? z: z2;

		handler->data.l.coords.push_back(minpt);
		handler->data.l.coords.push_back(maxpt);

        handler->data.i = 0;
        handler->data.l.d = 0;
	}

	return handler;
}

TutorialHandler* TutorialManager::AddAreaHandler(const char* callback, const char* name)
{
	return AddHandler(TUT_EVENT_AREA, callback, name);
}

void TutorialManager::RemoveHandler(const char* name)
{
	TutorialHandlerMap::iterator it = _eventHandlers.find(name);

	if(it != _eventHandlers.end())
	{
		it->second.state = THS_GARBAGE;
		_garbage.push_back(name);
	}
}

void TutorialManager::RemoveHandlers()
{
	_eventHandlers.clear();
	_timerMap.clear();
	_garbage.clear();
	_eventFlags = 0;
}

void TutorialManager::SetComplete(const char* id, int key, bool completed)
{
	if(key < -1 || key >= 64) return;

	if(_completed.find(id) == _completed.end())
		_completed[id] = 0;

	UINT64 oldFlags	= _completed[id];
	UINT64 newFlags = oldFlags;

	if(completed)
	{
		if(key == -1)
			newFlags = 0xffffffff;
		else
			newFlags |= ((UINT64)1<<key);
	}
	else
	{
		if(key == -1)
			newFlags = 0;
		else
			newFlags &= ~((UINT64)1<<key);
	}

	_completed[id] = newFlags;

	//std::string fullID = "tutorial-"; fullID += id;
	//cfgPrefsSetInt(fullID.c_str(), _completed[id]);

	if(oldFlags != newFlags)
	{
		// need to tell the host of the change
		Tutorial::GroupProgress progress(chatGetHandle(id), newFlags);
		progress.send(PlayerClient()->ChatConnection());
		uiRunScript("game.ongraduated()");
	}
}

bool TutorialManager::IsComplete(const char* id, int key)
{
	if(key < 0 || key >= 64) return false;

	if(_completed.find(id) == _completed.end())
		_completed[id] = 0;

	UINT64 flags = _completed[id];
	UINT64 flag = (UINT64)1<<key;

	bool completed = (flags & flag) != 0;

	//return ((_completed[id] & ((UINT64)1<<key)) != 0);
	return completed;
}

bool TutorialManager::AllowStrat() const
{
	return (!IsActive() || _allowStrat);
}

void TutorialManager::SetStratAllow(bool allow)
{
	_allowStrat = allow;
}


void TutorialManager::SetGroupFlags(const char* id, UINT64 flags)
{
	_completed[id] = flags;
}

void TutorialManager::SetPrePath(const char* path)
{
	_prePath = path;
}

TutorialManager* TutorialManager::GetInstance()
{
	return &_tutMgr;
}

unsigned int TutorialManager::Input(int param, unsigned int functionCode, unsigned int unused)
{
	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionCode, &param);
	return 1;
}

void TutorialManager::_resetFlags()
{
	_eventFlags = 0;

	TutorialHandlerMap::iterator it = _eventHandlers.begin();

	for(; it != _eventHandlers.end(); it++)
		_eventFlags |= it->second.eventType;
}

int TutorialManager::_handleEvent(TutorialHandler& handler, va_list* args)
{
	int nargs	= 1;
	int keep	= 0;
	int top		= lua_gettop(_tutorial); // save the current stack position

	// first lookup the scope if we have one
	if(handler.scope.length())
	{
		lua_getglobal(_tutorial, handler.scope.c_str());
		lua_pushstring(_tutorial, handler.callback.c_str());
		lua_gettable(_tutorial, -2);
	}
	else
	{
		lua_getglobal(_tutorial, handler.callback.c_str());
	}

	if(!lua_isfunction(_tutorial, -1))
	{
		RemoveHandler(handler.name.c_str());
		uiChatPrintLine("Error calling tutorial handler %s: %s", handler.callback.c_str(), "lua function not found");
		return 0;
	}

	tolua_pushusertype(_tutorial, (void*)&handler, "TutorialHandler");

	if (args)
	{
		switch (handler.eventType)
		{
		case TUT_EVENT_SET:		// name/value pair
		{
			lua_pushstring(_tutorial, va_arg(*args, const char*));
			lua_pushstring(_tutorial, va_arg(*args, const char*));
			nargs = 3;
		}
		break;
		case TUT_EVENT_MESSAGE:		// arbitrary string message
		{
			lua_pushstring(_tutorial, va_arg(*args, const char*));
			nargs = 2;
		}
		break;
		case TUT_EVENT_CHAT:		// chat sending
		{
			lua_pushstring(_tutorial, va_arg(*args, const char*));
			nargs = 2;
		}
		break;
		case TUT_EVENT_CHANNEL:		// chat channel tuning
		{
			unsigned char radio = *(va_arg(*args, unsigned char*));
			unsigned int channel = *(va_arg(*args, unsigned int*));

			lua_pushinteger(_tutorial, (int)radio);
			lua_pushinteger(_tutorial, (int)channel);
			nargs = 3;
		}
		break;
		case TUT_EVENT_KEY:			// iac code, state and count
		{
			lua_pushinteger(_tutorial, *(va_arg(*args, unsigned int*)));
			lua_pushinteger(_tutorial, *(va_arg(*args, unsigned int*)));
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			nargs = 4;
		}
		break;
		case TUT_EVENT_INPUT:			// function code and param
		{
			lua_pushinteger(_tutorial, *(va_arg(*args, unsigned int*)));
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			nargs = 3;
		}
		break;
		case TUT_EVENT_WEAPON_SWITCH:	// game weapon type and fps weapon type
		{
			tolua_pushusertype(_tutorial, va_arg(*args, weaponType*), "weaponType");
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			nargs = 3;
		}
		break;
		case TUT_EVENT_STATE_TROOP:		// fps weapon state and fps weapon type
		{
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			nargs = 3;
		}
		break;
		case TUT_EVENT_FPS_POSE:		// fps pose type
		{
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			nargs = 2;
		}
		break;
		case TUT_EVENT_ENTER:				// strat building entered
		case TUT_EVENT_LEAVE:				// strat building left
		{
			tolua_pushusertype(_tutorial, va_arg(*args, StratDB::StratObject*), "StratDB::StratObject");
			nargs = 2;
		}
		break;
		case TUT_EVENT_WEAPON_FIRE:			// fired weapon type
		{
			tolua_pushusertype(_tutorial, va_arg(*args, weaponType*), "weaponType");
			nargs = 2;
		}
		break;
		case TUT_EVENT_HIT:					// terrain object shot
		{
			tolua_pushusertype(_tutorial, va_arg(*args, trnCDTerrainObject*), "trnCDTerrainObject"); // trn cd object
			tolua_pushusertype(_tutorial, va_arg(*args, OBJECT*), "OBJECT"); // collision object
			lua_pushnumber(_tutorial, *(va_arg(*args, double*))); // x
			lua_pushnumber(_tutorial, *(va_arg(*args, double*))); // y
			lua_pushnumber(_tutorial, *(va_arg(*args, double*))); // z
			nargs = 6;
		}
		break;
		case TUT_EVENT_COLLISION:			// terrain object collided with
		{
			tolua_pushusertype(_tutorial, va_arg(*args, trnCDTerrainObject*), "trnCDTerrainObject");
			nargs = 2;
		}
		break;
		case TUT_EVENT_LOCATION:			// true = in range; false = out of range
		{
			lua_pushboolean(_tutorial, (handler.state == THS_EVENT_IN) ? 1 : 0);
			nargs = 2;
		}
		break;
		case TUT_EVENT_UI_POPUP:			// true = ok; false = cancel
		case TUT_EVENT_UI_READY:			// true = ready; false = not ready
		{
			lua_pushboolean(_tutorial, *(va_arg(*args, int*)));
			nargs = 2;
		}
		break;
		case TUT_EVENT_MAP:					// number representing map event subtype
		{
			lua_pushinteger(_tutorial, *(va_arg(*args, int*)));
			nargs = 2;
		}
		break;
		case TUT_EVENT_DAMAGE:					// terrain object shot
		{
			lua_pushnumber(_tutorial, *(va_arg(*args, double*))); // x
			tolua_pushusertype(_tutorial, va_arg(*args, vehVehicle*), "vehVehicle"); // vehicle object
			nargs = 3;
		}
		break;
		default:							// data not used
		{
			// no arguments for default handled events
		}
		break;
		}
	}

	if(lua_pcall(_tutorial, nargs, 1, 0) == 0) // call the handler's callback
	{
		if(lua_isnumber(_tutorial, -1) && lua_tonumber(_tutorial, -1) > 0)
			keep = 1; // handler keeps going
	}
	else
	{
		const char* error = lua_tostring(_tutorial, -1);
		lua_pop(_tutorial, 1);
		uiChatPrintLine("Error calling tutorial handler %s: %s", handler.callback.c_str(), error);
	}

	if(!keep)
		RemoveHandler(handler.name.c_str());

	lua_settop(_tutorial, top);

	return keep;
}

bool _pointInArea(Vec3d& pos, TutorialHandler& handler)
{
	TutorialHandler::HandlerLocation::COORDINATES& points = handler.data.l.coords;

	int sides	= points.size();
	int i		= 0;
	int j		= sides - 1;

	bool inArea = false;

	for(i = 0; i < sides; i++)
	{
		Vec3d& pt1 = points.at(i);
		Vec3d& pt2 = points.at(j);

		if((pt1.y() < pos.y() && pt2.y() >= pos.y()) ||  (pt2.y() < pos.y() && pt1.y() >= pos.y()))
		{
			if(pt1.x() + (pos.y() - pt1.y()) / (pt2.y() - pt1.y()) * (pt2.x() - pt1.x()) < pos.x())
			{
				inArea = !inArea;
			}
		}

		j = i;
	}

	return inArea;
}
