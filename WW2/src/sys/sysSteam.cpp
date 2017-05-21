#include <syscfg.h>
#include "sysSteam.h"

#include <steam_api.h>



namespace {
	bool steam_initialized = false;
}

//==============================================================================

bool InitSteam(void) {
	// TODO: Don't initialize Steam for non-steam installations.

	if ( steam_initialized )
		return true;

	steam_initialized = SteamAPI_Init();
	atexit(KillSteam);

	// TODO: Return the result of SteamAPI_RestartAppIfNecessary() if/when we switch to that.
	return true;
}

//--------------

void KillSteam(void) {
	if ( steam_initialized )
		SteamAPI_Shutdown();
	steam_initialized = false;
}

//==============================================================================

bool IsSteamCopy(void) {
	// NOTE: This will trigger even for alternate installs: If they have Steam
	// and own the game, *every* copy is a Steam copy as far as this is concerned.
	return steam_initialized && SteamApps()->BIsSubscribed();
}
