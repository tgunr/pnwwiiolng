#pragma once
// Created: Tue Apr 26 15:31:00 CDT 2011
//
// Structure for creating the creation of a vehicle.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSerial.h"
#include "include/inplace_vector.h"

namespace Server
{
	class ServerInfo;
}
class Player;

namespace Host
{

#pragma pack(push, 1)
struct VehicleRequest
{
	//! MMHost constructor.
	VehicleRequest(const instanceid_t& instanceID, const struct MM_SPAWN_GRANT_RESPONSE& grant, const class Player& commander, const class Player* const crew);
	VehicleRequest(const instanceid_t& instanceID, sortieid_t serial, const WEAPON_SPEC& spec)
		: m_instanceID(instanceID)
		, m_weaponSerial()
		, m_brigadeID(0)
		, m_missionID(0)
		, m_players()
		, m_handle(0ULL)
		, m_decal(0)
		, m_rank(0)
		, m_flags(0)
	{
		m_weaponSerial.source = nullObjID;
		m_weaponSerial.serial = serial;
		m_weaponSerial.spec = spec;
	}

	//! Which instance this is for.
	instanceid_t	m_instanceID;
	//! The weapon serial stuff.
	WEAPON_SERIAL	m_weaponSerial;
	//! Brigade and initial mission ID.
	UINT32			m_brigadeID;
	UINT32			m_missionID;
	//! The origin for the vehicle.
	//! List of players who will be joining it.
	WWII::inplace_vector<UINT32,8>	m_players;
	//! Callsign for the vehicle.
	ChatHandle		m_handle;
	//! Decal for the vehicle.
	UINT32			m_decal;
	//! Rank to show.
	RANK_NO			m_rank;
	//! Vehicle's initial flags.
	UINT32			m_flags;

	UINT32 numPlayers() const { return (m_players.empty() == false ? m_players.size() : 0); }
	UINT32 playerNo(size_t i) const { return ( i < numPlayers() ? m_players[i] : 0 ); }

	void send(const Server::ServerInfo* server);

	// Callback receiver.
	static SINT32 _recvCallback(ACTION_OBJECT* const a)
	{
		if ( a->canRead<Host::VehicleRequest>() == false )
			return -EINVAL;
		auto& req = a->read<Host::VehicleRequest>();
		dbugLog(_FLD, "p:%u v:%u in:%llu rxing host vehicle request", req.m_players[0], req.m_weaponSerial.serial, req.m_instanceID);
		if ( !req.execute() )
			return -1;
		return a->offset;
	}

	static TEUL_CALLBACK* s_callback;

	// Handle receiving a vehicle request.
	bool execute();
};
#pragma pack(pop)


} // namespace
