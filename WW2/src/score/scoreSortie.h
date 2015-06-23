#pragma once
// Copyright (C) PlayNet Inc 2011 -- All rights reserved.

#include "wwii/wwiiRTBCodes.h"
#include "wwii/wwiiFullObjID.h"
#include "score/scoreActor.h"

namespace Scoring
{

class Sortie
{
public:
	Sortie(Player& pilot);
	~Sortie();

public:
	//! Return the sortie for a given sortie ID.
	static Sortie* Lookup(sortieid_t sortieID);

public:
	struct HitEntry : public Scoring::Actor
	{
		HitEntry(const Scoring::Actor& actor_, const UINT64& joules_)
			: Actor(actor_)
			, m_joules(joules_)
			, m_timestamp(teulClock64)
			{}

		UINT64		m_joules;
		TeulTimeMS	m_timestamp;
	};

public:
	UINT32 spawnTime() const { return m_spawnTime; }
	UINT32 despawnTime() const { return m_despawnTime; }
	sortieid_t sortieID() const { return m_sortieID; }
	playerid_t pilotID() const { return m_pilotID; }
	UINT32 crewID() const { return m_crewID; }
	const WEAPON_SPEC& spec() const { return m_spec; }
	CODE_RTB rtbCode() const { return m_rtbCode; }
	const std::vector<HitEntry>& hitsTaken() const { return m_hitsTaken; }
	const ChatHandle& chatHandle() const { return m_chatHandle; }
	const ChatHandle& killedBy() const { return m_killedBy; }
	sortieid_t lastHitBySortieID() const { return m_lastHitBySortieID; }
	const TeulTimeMS& lastHitTime() const { return m_lastHitTime; }
	bool isPlaynet() const { return m_isPlaynet; }
	const FLOAT32& value() const { return m_value; }
	const FULL_OBJ_ID& despawnedTo() const { return m_despawnedTo; }

	const Player* pilot() const { return m_pilot; }
	const Player* crew() const { return m_crew; }
	Player* pilot() { return m_pilot; }
	Player* crew() { return m_crew; }

	const Player* activePlayer() const { return m_pilot ? m_pilot : m_crew; }
	Player* activePlayer() { return m_pilot ? m_pilot : m_crew; }

	WWII_COUNTRY country() const { return spec().country(); }
	WWII_SIDE side() const { return (WWII_SIDE)spec().side(); }

	void attachCrew(Player& crew_);

	bool addHitBy(const Scoring::Actor& actor, const UINT64& joules_);
	void addHitTo(const Scoring::Actor& actor, const UINT64& joules_);
	void addKill(const sortieid_t sortieID_, const WEAPON_SPEC& spec_);
	void addInteraction(const WEAPON_SPEC& spec_);

	//! Set the RTB code and destination of this sortie.
	void setRTB(const CODE_RTB rtbCode_, const FULL_OBJ_ID& rtbDest_);

	//! Flag this sortie as having despawned.
	void despawned();

	void hitBy(sortieid_t sortieID_) { m_lastHitBySortieID = sortieID_; m_lastHitTime = teulClock64; }

protected:
	UINT32			m_sortieID;
	UINT32			m_spawnTime;
	UINT32			m_despawnTime;
	FULL_OBJ_ID		m_despawnedTo;
	playerid_t		m_pilotID;
	playerid_t		m_crewID;
	WEAPON_SPEC		m_spec;
	CODE_RTB		m_rtbCode;
	sortieid_t		m_lastHitBySortieID;
	TeulTimeMS		m_lastHitTime;

	// Decision on who got the kill.
	ChatHandle		m_chatHandle;
	ChatHandle		m_killedBy;
	bool			m_isPlaynet;

	FLOAT32			m_value;			//! Value of this vehicle/sortie.

protected:
	std::vector<HitEntry>	m_hitsTaken;

	Player*			m_pilot;
	Player*			m_crew;

public:
	std::set<sortieid_t>			m_sortiesHit;
	std::set<sortieid_t>			m_sortiesKilled;
	std::map<UINT32, UINT32>		m_typesHit;
	std::map<UINT32, UINT32>		m_typesKilled;
	std::set<UINT32>				m_typesInteractedWith;
};

	//! Lookup a player's sortie by sortie id, checking their player pointer
	//! for a pointer to it first to avoid a map lookup.
	//! (Most of the time we expect to look up the player's current sortie)
	Sortie* FindSortieForPlayer(const Player* const Player, const sortieid_t sortieID);
}

