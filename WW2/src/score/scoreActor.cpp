#include <syscfg.h> // Path is project specific.
// Actor functions.

#include "wwii/wwiiPlayer.h"
#include "score/scoreActor.h"

Scoring::Actor::Actor(const wwiiPlayer* player)
	: m_instID(player->instance().id())
	, m_weapon(NULL_WEAP_SPEC)
	, m_sortieID(sortieID())
	, m_playerID(player->playerID())
	, m_handle(player->chatHandle())
	, m_inCaptureBldg(false)
	, m_balance(Scoring::Balance::Even)
	{}

