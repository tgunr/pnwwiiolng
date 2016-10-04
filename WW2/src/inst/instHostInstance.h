#pragma once
// Created: Tue Oct  7 15:40:06 CDT 2008
//
// This is the per-request status sent back to the client
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instBasicStatus.h"
#include "host/hostPlayers.h"


namespace Instance
{
	class HostInstanceImpl;
	class HostInstance;

	//! Check for and delete expired instances.
	extern void CheckExpirations();

	//! @brief Functor that takes a HostInstance reference and returns a new HostInstanceImpl
	//! or derived object.
	typedef HostInstanceImpl* (HostInstanceImplFactory) (HostInstance&);

	//! @brief Function to register your host-specific class' new functions.
	void RegisterHostInstanceImplFactory(HostInstanceImplFactory* factory);

	//! @brief The host-specific instance implementation object factory.
	extern HostInstanceImplFactory* s_implFactory;

	class HostInstance : public BasicStatus
	{
	public:
		HostInstance(const playerid_t creator_, const Descriptor& descriptor_
				, UINT16 capacity_, UINT16 gameTime_m_
				, UINT32 createdAt_, UINT32 startedAt_
				, Instance::State state_=Instance::HostCtored
				)
			: BasicStatus(descriptor_, capacity_, gameTime_m_, createdAt_, startedAt_, state_)
			, m_creator(creator_)
			, m_players()
			, m_impl(nullptr)
			, m_expires(0ULL)
			, m_modified(teulClock64)
			, m_sortieMemory()
			{
				m_impl = s_implFactory(*this);
				m_lastRecorded[0] = m_lastRecorded[1] = m_lastRecorded[2] = m_lastRecorded[3] = 0;
			}
		HostInstance()
			: BasicStatus()
			, m_creator(0)
			, m_players()
			, m_impl(nullptr)
			, m_expires(0ULL)
			, m_modified(teulClock64)
			, m_sortieMemory()
			{
				m_impl = s_implFactory(*this);
				m_lastRecorded[0] = m_lastRecorded[1] = m_lastRecorded[2] = m_lastRecorded[3] = 0;
			}

		// Destruction must be implemented on a per-application basis.
		virtual ~HostInstance();

		const PLAYERS& players() const { return m_players; }
		PLAYERS& players() { return m_players; }
		playerid_t creator() const { return m_creator; }

		// Member functions

		bool addPlayer(wwiiPlayer* player);
		void swapPlayerSide(wwiiPlayer* player, UINT8 fromSide);
		void incrementCount(UINT8 forSide);
		void decrementCount(UINT8 forSide);
		bool expiring() const { return (m_expires > 0) & (players().empty()); }
		bool expired() const { return expiring() & (players().empty()); }
		size_t population() const { return players().size(); }

		UINT32 getSquadMemberCount(UINT32 squadID) const;

		bool removePlayer(wwiiPlayer* player);

		void setState(Instance::State newState);

	public:
		static HostInstance* LookupByUID(const instanceid_t _uid);

	private:
		//! PlayerID of the player who created the instance (may differ from handle)
		playerid_t			m_creator;
		//! List of players in the instance.
		PLAYERS				m_players;
		//! Server-process specific implementation.
		HostInstanceImpl*	m_impl;
		//! When the instance is due for expiration.
		TeulTimeMS			m_expires;

	public:
		const HostInstanceImpl& impl() const { return *m_impl; }
		HostInstanceImpl&	impl() { return *m_impl; }

	public:
		UINT32		m_lastRecorded[MAX_SIDES];

	protected:
		//! Time stamp of the last publicly-visible change
		//! to this instance.
		TeulTimeMS			m_modified;

	public:
		struct SortieMemory
		{
			playerid_t		m_playerID;
			ChatHandle		m_handle;
		};
		std::map<sortieid_t /*sortie*/, SortieMemory> m_sortieMemory;
	};
}

