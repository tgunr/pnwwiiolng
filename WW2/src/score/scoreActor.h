#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

class HostVehicle;
class Player;

#include "weap/weapSpec.h"

#pragma pack(push, 1)
namespace Scoring
{
	namespace Balance { static const SINT32 Under = -1, Even = 0, Over = 1; }

	//! @class	Scoring::Actor
	//! @brief  Describes a participant in an event.
	class Actor
	{
	public:

	public:	Actor()
				: m_instID(0ULL)
				, m_weapon(NULL_WEAP_SPEC), m_sortieID(0)
				, m_playerID(0), m_handle(0ULL)
				, m_inCaptureBldg(false)
				, m_balance(Balance::Even)
			{}

	public: Actor(const instanceid_t instID_, const playerid_t playerID_, const ChatHandle handle_, const WEAPON_SPEC weapon_, const sortieid_t sortieID_, SINT32 balance_)
				: m_instID(instID_)
				, m_weapon(weapon_)
				, m_sortieID(sortieID_)
				, m_playerID(playerID_)
				, m_handle(handle_)
				, m_inCaptureBldg(false)
				, m_balance(balance_)
			{}

		//! Construct from a HostVehicle.
		//! @note Cell host only.
	public: Actor(const HostVehicle* const);
		//! Construct from a HostVehicle and a Player pointer.
		//! @note Cell host only.
	public: Actor(const HostVehicle* const, const Player* const);
		//! Construct from a wwiiPlayer.
	public: Actor(const wwiiPlayer* const);

	public: instanceid_t instID() const { return m_instID; }
	public: playerid_t playerID() const { return m_playerID; }
	public: const ChatHandle handle() const { return m_handle; }
	public: sortieid_t sortieID() const { return m_sortieID; }
	public: bool inCaptureBldg() const { return m_inCaptureBldg; }
	public: SINT32 balance() const { return m_balance; }
	public: WEAPON_SPEC weapon() const { return m_weapon; }

	public: void setSortie(const sortieid_t id_, const WEAPON_SPEC spec_, const bool inCapture_)
	{
		m_sortieID = id_;
		m_weapon = spec_;
		m_inCaptureBldg = inCapture_;
	}

	protected:
		instanceid_t	m_instID;			//!< Instance player was in.
		WEAPON_SPEC		m_weapon;			//!< Vehicle type.
		sortieid_t		m_sortieID;			//!< Sortie (or 0).
		playerid_t		m_playerID;			//!< Player ID value.
		ChatHandle		m_handle;			//!< Name.
		bool			m_inCaptureBldg;	//!< True when player is inside a capture building.
		SINT32			m_balance;			//!< How player is balanced.
	};
}
#pragma pack(pop)

