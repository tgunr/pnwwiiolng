#pragma once

namespace Server
{
	//! Codes describing why a player is being released from the cluster or server.
	enum class ReleaseType
	{
		Redirected				//! player transfered to a new server
	,	Deleted					//! another server has deleted the player, delete the player
	,	Disco					//! player timed out on exit list, delete the player
	,	Collision				//! more than one player of same ID on cluster
	,	Error					//! a hard error has occured, delete the player
	,	Exit					//! the player has exited the game gracefully, delete
	,	EConn					//! the players connection is fubar, put on exit server

	,	Max
	};

	const char* GetReleaseType(ReleaseType releaseType);
}

