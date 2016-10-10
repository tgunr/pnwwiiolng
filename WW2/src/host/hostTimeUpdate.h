#pragma once
// Created: Wed Dec 22 17:14:13 CST 2010
// Copyright: (C) Playnet Inc 2010 All rights reserved.
// Not for redistribution except as part of a precompiled API.

/// @file host/hostTimeUpdate.h
/// @author Oliver 'kfs1' Smith <osmith@playnet.com>
/// @brief Describes the Host::TimeUpdate struct.

#pragma pack(push, 1)
namespace Host
{
struct TimeUpdate
{
	TimeUpdate(UINT32 _gametime, UINT32 _factor, UINT32 _weather, UINT32 _rain)
		: gametime(_gametime)
		, factor(_factor)
		, weather(_weather)
		, rain(_rain)
		{}
	TimeUpdate() : gametime(0), factor(0), weather(0), rain(0) {}
	TimeUpdate(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }

	void packer(PackMode mode, ACTION_OBJECT* const a)
	{
		a->packer(mode, gametime);
		a->packer(mode, factor);
		a->packer(mode, weather);
		a->packer(mode, rain);
	}

	UINT32	gametime;
	UINT8	factor;
	UINT8	weather;
	UINT8	rain;

	static TEUL_CALLBACK* Callback;
	static SINT32 SendFunction(ACTION_OBJECT* const a);
	static void Send(SINT32 teulIndex);
	void Broadcast();
};

}
#pragma pack(pop)

