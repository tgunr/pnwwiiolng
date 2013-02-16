#pragma once
// Copyright (C) PlayNet Inc 2012 -- All rights reserved.
// Created: Thu Apr 19 17:18:15 CDT 2012
// Originating Programmer: Oliver "kfsone" Smith
// Structure for describing where a player's tick counting is at

namespace Teul
{
#pragma pack(push, 1)
	struct TickSample
	{
		TickSample(const TeulTimeMS& host_, const TeulTimeMS& client_)
			: host_ms(host_), client_ms(client_)
			, delta((TeulDeltaMS)host_ - (TeulDeltaMS)client_)
			, ratio((double)client_ / (double)host_)
			, source(0)
			, playerID(0)
			{}

		TeulTimeMS	host_ms ;		//!< Time elapsed on the host.
		TeulTimeMS	client_ms ;		//!< Time that appears to have elapsed on the client.
		TeulDeltaMS	delta ;			//!< Discrepancy
		double		ratio ;			//!< client_ms / host_ms .. so > 1.0 = cheat

		UINT32		source ;		//!< server type enum, e.g. CHAT_HOST.
		playerid_t	playerID ;		//!< PlayerID
	} ;
#pragma pack(pop)
}
