#include <syscfg.h>
#include "sysSteam.h"

#include <steam_api.h>

#include <vars.h>



namespace {
	bool steam_initialized = false;
}

//==============================================================================

bool InitSteam(void) {
	if ( steam_initialized || UseSteamLogin == false )
		return true;

	// TODO: SteamAPI_RestartAppIfNecessary()? It's probably better to just use
	// the Steam Wrapper...
	if ( ! SteamAPI_Init() )
		return false;
	// Unfortunately, we can't trust the program to actually shut down properly,
	// so we just force this to run at exit ourselves.
	atexit(KillSteam);

	steam_initialized = true;
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
	return steam_initialized && SteamApps()->BIsSubscribed();
}
