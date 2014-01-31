#pragma once
// Simple macro header file which defines a couple of beta-release
// functions that record an event occuring on a particular teulIndex

#include "versionNo.h"
#include "map/mapPlayer.h"

#ifdef BETA_RELEASE
# define BETALog(t, msg) _betaInfo(t, __FUNCTION__, msg)
# define BETAInfo(t) _betaInfo(t, __FUNCTION__, "<mark>")
static inline
void _betaInfo(SINT32 teulIndex, const char* fn, const char* msg)
{
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player != nullptr )
		dbugLog(_FLI, "t:%d p:%d c:%s %s(): %s", teulIndex, player->playerID(), player->callsign(), fn, msg);
	else
		dbugLog(_FLI, "t:%d %s [unknown]: %s", teulIndex, fn, msg);
}
#else
# define BETALog(t, msg)
# define BETAInfo(t)
#endif
