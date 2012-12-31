#pragma once

#include "phui/Javascript.h"

// event types; their order is important; add to the end
enum
{
	GAMEEVENT_INVALID = 0,
	GAMEEVENT_POPUP_OK,				// generic popup type (1)
	GAMEEVENT_POPUP_BUILDER,		// when we want to show a builder popup
	GAMEEVENT_POPUP_CONTENT,		// when we want to show a popup that defines its own popup
	GAMEEVENT_OVERSUPPLY_RECEIVED,	// when we've first been oversupplied
	GAMEEVENT_OVERSUPPLY_WARNING,	// when we're close to the oversupply radius check (5)
	GAMEEVENT_OVERSUPPLY_LOST,		// when we've lost oversupply due to radius check
	GAMEEVENT_RESUPPLY_NA,			// criteria not met to resupply (not moving, not near truck, not firing, etc...)
	GAMEEVENT_RESUPPLY_READY,		// close enough, but other criteria not met
	GAMEEVENT_RESUPPLY_RESUPPLYING,	// something has delayed us
	GAMEEVENT_RESUPPLY_RECEIVED,	// each time we get resupply (10)
	GAMEEVENT_ALERT,				// general messages
	GAMEEVENT_ALERT_STRAT,			// strategic messages
	GAMEEVENT_ALERT_HC,				// HC message - fallback, overrun, hold at all costs, etc..
	GAMEEVENT_ALERT_CONTACT,		// enemy near your cp
	GAMEEVENT_ALERT_ORDERS,			// mission orders updated
	GAMEEVENT_ALERT_LEADER,			// mission leader changed
	GAMEEVENT_INFSUPPLY_RECEIVED,	// when we've received resupply from an nfantry type
	GAMEEVENT_ALERT_CP,				// cp changed hands (target or origin)
	GAMEEVENT_ALERT_FACILITY,		// facility changed hands (target or origin)
	GAMEEVENT_ALERT_BRIGADE,		// brigade alerts - moved
	GAMEEVENT_ALERT_OBJECTIVE,		// objective messages
	GAMEEVENT_OVERSUPPLY_INRANGE,	// when we're within the oversupply radius check
	GAMEEVENT_ALERT_CPATTACKED,		// CP under attack
	GAMEEVENT_ALERT_ENEMY,			// enemy spotted near CP
	GAMEEVENT_NEED_AMMO,			// friendly needs ammo nearby
	GAMEEVENT_TUTORIAL,				// tutorial event
	GAMEEVENT_TIMELEFT,				// hourly guy; time left alert
	GAMEEVENT_NOFIRE,				// alert that they can't fire in this area
	GAMEEVENT_PPO,					// player placed object event
	GAMEEVENT_NOBAILOUT,			// can't bailout at this altitude
	// add more types here
	GAMEEVENT_NUM_TYPES
};

// used to call game events from other files
extern int uiGameEvent(int e, const char* args = NULL, ...);
extern int uiGameEvent(jsval* argv, int argc);
extern int uiGameEvent(int e, int argc, ...); // assumes variable args are wide strings
