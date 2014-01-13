#pragma once

namespace Server
{
//! Codes describing why a player is being released from the cluster or server.
enum ReleaseType
{
	RELEASE_REDIRECTED		//! player transfered to a new server
,	RELEASE_DELETED			//! another server has deleted the player, delete the player
,	RELEASE_DISCO			//! player timed out on exit list, delete the player
,	RELEASE_COLLISION		//! more than one player of same ID on cluster
,	RELEASE_ERROR			//! a hard error has occured, delete the player
,	RELEASE_EXIT			//! the player has exited the game gracefully, delete
,	RELEASE_ECONN			//! the players connection is fubar, put on exit server

,	MAX_RELEASE_TYPES
};

const char* GetReleaseType(ReleaseType releaseType);
}

