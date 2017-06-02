#pragma once



/// @note This will automatically add a KillSteam call for when the program exits.
/// @return If the application should continue running. This does *not* mean Steam is actually running.
bool InitSteam(void);
void KillSteam(void);

bool IsSteamCopy(void);
