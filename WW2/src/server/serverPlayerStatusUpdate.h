#pragma once

#include "auth/authDispatch.h"

namespace Server
{
	enum class PlayerStatus;

	struct PlayerStatusUpdate
	{
		PlayerStatusUpdate(const PlayerStatusNode& psn);
		UINT64			serverID;		// mac address of eth0 + pid
		PlayerStatus	status;
		Auth::Dispatch	dispatch;
		UINT32			source;		// source adress of the update
		UINT32			id;			// update id from this server
	};

}
