#pragma once
////////////////////////////////////////////////////////////////////////////////////////
// @file omgMissionSortieData.h
// @attention AUTOMATICALLY GENERATED FROM omg/omgMissionSortieData.xml BY omg/omg-to-h.py
// @attention DO NOT MODIFY THIS FILE DIRECTLY edit omg/omgMissionSortieData.xml instead.
////////////////////////////////////////////////////////////////////////////////////////

// For VarGroup, SyncVar
#include "wwii/wwiiSyncVar.h"
// For Teul::NetTimeMS
#include "teul/teulNetTimeMS.h"
// For FULL_OBJ_ID
#include "wwii/wwiiFullObjIDPacker.h"
// For WEAPON_SPEC
#include "weap/weapSpecPacker.h"

#if defined(_MSC_VER)
# pragma warning(disable : 4355) // C4355: 'this': used in base member initializer list.
#endif

namespace MM
{

//! @class MM::SortieData
//! @brief Describes an ongoing or complete sortie.
//! @details
//! Encapsulates various pieces of sortie data into various objects.

//! @attention This class is automatically generated from omgMissionSortieData.xml

class SortieData
{
	// Default constructor.
	public:
	SortieData()
		: m_lastModified(0ULL)
		, m_sortie(m_lastModified)
		, m_vehicle(m_lastModified)
		, m_location(m_lastModified)
		, m_scoring(m_lastModified)
		{
		}

	//! User implemented: somewhere to provide a reference to the Teul Callback for this object.
	public: static TEUL_CALLBACK* Callback(void);

	//! Last modified time of the structure or a member var group.
	protected: TeulTimeMS m_lastModified;

	//! Get milisecond timestamp of last modification.
	//! @return Local teulClock64 timestamp of the last modification of a managed member of the structure.
	public: const TeulTimeMS& lastModified() const { return m_lastModified; }

	//! How many bits required to describe number of groups changed.
	public: UINT32 groupsChangedBits() const { return 8; }

	///! User implemented: Routine for serializing/deserializing this data.
	public: bool packer(LOGICAL mode, ACTION_OBJECT* a, const TeulTimeMS& lastPack);

	// Members

	//! @struct Sortie
	//! @brief Outline of this sortie.
	public:
	struct Sortie : public WWII::VarGroup
	{
		Sortie(TeulTimeMS& parent)
			: VarGroup(parent)
			, sortieID(this)
			, spawned_ms(this)
			, despawned_ms(this)
			, originFac(this)
			, target(this)
			, flags(this)
			{}
		Sortie& operator = (const Sortie& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			sortieID.operator=(rhs.sortieID);
			spawned_ms.operator=(rhs.spawned_ms);
			despawned_ms.operator=(rhs.despawned_ms);
			originFac.operator=(rhs.originFac);
			target.operator=(rhs.target);
			flags.operator=(rhs.flags);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			sortieID.packer(mode, a, lastPack, changes, fieldNo++);
			spawned_ms.packer(mode, a, lastPack, changes, fieldNo++);
			despawned_ms.packer(mode, a, lastPack, changes, fieldNo++);
			originFac.packer(mode, a, lastPack, changes, fieldNo++);
			target.packer(mode, a, lastPack, changes, fieldNo++);
			flags.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Server's ID for this sortie (unique per campaign).
		WWII::SyncVar<UINT32,0> sortieID;
		//! TimeStamp of the sortie's spawn.
		WWII::SyncVar<Teul::NetTimeMS,0LL> spawned_ms;
		//! Despawn time.
		WWII::SyncVar<Teul::NetTimeMS,0LL> despawned_ms;
		//! Spawn facility. May differ from mission.
		WWII::SyncVar<FULL_OBJ_ID> originFac;
		//! Target of player's current mission.
		WWII::SyncVar<FULL_OBJ_ID> target;
		//! List of flags.
		WWII::SyncVar<UINT32,0> flags;
	} m_sortie;
	const Sortie& sortie() const { return m_sortie; }
	Sortie& sortie() { return m_sortie; }

	//! @struct Vehicle
	//! @brief Description of the vehicle used.
	public:
	struct Vehicle : public WWII::VarGroup
	{
		Vehicle(TeulTimeMS& parent)
			: VarGroup(parent)
			, weaponSpec(this)
			, effectiveRank(this)
			, rtbState(this)
			, decal(this)
			, killedBy(this)
			, rtbFacility(this)
			{}
		Vehicle& operator = (const Vehicle& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			weaponSpec.operator=(rhs.weaponSpec);
			effectiveRank.operator=(rhs.effectiveRank);
			rtbState.operator=(rhs.rtbState);
			decal.operator=(rhs.decal);
			killedBy.operator=(rhs.killedBy);
			rtbFacility.operator=(rhs.rtbFacility);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			weaponSpec.packer(mode, a, lastPack, changes, fieldNo++);
			effectiveRank.packer(mode, a, lastPack, changes, fieldNo++);
			rtbState.packer(mode, a, lastPack, changes, fieldNo++);
			decal.packer(mode, a, lastPack, changes, fieldNo++);
			killedBy.packer(mode, a, lastPack, changes, fieldNo++);
			rtbFacility.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Vehicle used for this sortie.
		WWII::SyncVar<WEAPON_SPEC> weaponSpec;
		//! Rank attributed to the vehicle.
		WWII::SyncVar<RANK_NO> effectiveRank;
		//! Vehicle's RTB status (or 0 for not yet despawned).
		WWII::SyncVar<UINT8,0> rtbState;
		//! Decal of the player leading the sortie
		WWII::SyncVar<UINT8,0> decal;
		//! Callsign of killer or 0.
		WWII::SyncVar<ChatHandle,0ULL> killedBy;
		//! Where the vehicle is RTBing to.
		WWII::SyncVar<FULL_OBJ_ID> rtbFacility;
	} m_vehicle;
	const Vehicle& vehicle() const { return m_vehicle; }
	Vehicle& vehicle() { return m_vehicle; }

	//! @struct Location
	//! @brief Tracking of where the vehicle is (or was) in-game.
	public:
	struct Location : public WWII::VarGroup
	{
		Location(TeulTimeMS& parent)
			: VarGroup(parent)
			, x(this)
			, y(this)
			, z(this)
			, riding(this)
			, nearCP(this)
			{}
		Location& operator = (const Location& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			x.operator=(rhs.x);
			y.operator=(rhs.y);
			z.operator=(rhs.z);
			riding.operator=(rhs.riding);
			nearCP.operator=(rhs.nearCP);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			x.packer(mode, a, lastPack, changes, fieldNo++);
			y.packer(mode, a, lastPack, changes, fieldNo++);
			z.packer(mode, a, lastPack, changes, fieldNo++);
			riding.packer(mode, a, lastPack, changes, fieldNo++);
			nearCP.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! x co-ordinate in cm
		WWII::SyncVar<SINT32,0> x;
		//! y co-ordinate in cm
		WWII::SyncVar<SINT32,0> y;
		//! z co-ordinate in cm
		WWII::SyncVar<SINT32,0> z;
		//! 0 or ID of sortie being ridden.
		WWII::SyncVar<UINT32,0> riding;
		//! Closest CP to vehicle.
		WWII::SyncVar<FULL_OBJ_ID> nearCP;
	} m_location;
	const Location& location() const { return m_location; }
	Location& location() { return m_location; }

	//! @struct Scoring
	//! @brief Provides summary tracking of the vehicle's actions.
	public:
	struct Scoring : public WWII::VarGroup
	{
		Scoring(TeulTimeMS& parent)
			: VarGroup(parent)
			, hits(this)
			, kills(this)
			, captures(this)
			, defends(this)
			, aiKills(this)
			, aiRepairs(this)
			, bridgeKills(this)
			, bridgeRepairs(this)
			, factoryKills(this)
			, firebaseKills(this)
			, resupplies(this)
			, resuppliesScored(this)
			, guardingKills(this)
			, cappingKills(this)
			, umsKills(this)
			, guardingPoints(this)
			, spawnees(this)
			, uniqueHits(this)
			, vehtypesHit(this)
			, points(this)
			, assists(this)
			{}
		Scoring& operator = (const Scoring& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			hits.operator=(rhs.hits);
			kills.operator=(rhs.kills);
			captures.operator=(rhs.captures);
			defends.operator=(rhs.defends);
			aiKills.operator=(rhs.aiKills);
			aiRepairs.operator=(rhs.aiRepairs);
			bridgeKills.operator=(rhs.bridgeKills);
			bridgeRepairs.operator=(rhs.bridgeRepairs);
			factoryKills.operator=(rhs.factoryKills);
			firebaseKills.operator=(rhs.firebaseKills);
			resupplies.operator=(rhs.resupplies);
			resuppliesScored.operator=(rhs.resuppliesScored);
			guardingKills.operator=(rhs.guardingKills);
			cappingKills.operator=(rhs.cappingKills);
			umsKills.operator=(rhs.umsKills);
			guardingPoints.operator=(rhs.guardingPoints);
			spawnees.operator=(rhs.spawnees);
			uniqueHits.operator=(rhs.uniqueHits);
			vehtypesHit.operator=(rhs.vehtypesHit);
			points.operator=(rhs.points);
			assists.operator=(rhs.assists);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			hits.packer(mode, a, lastPack, changes, fieldNo++);
			kills.packer(mode, a, lastPack, changes, fieldNo++);
			captures.packer(mode, a, lastPack, changes, fieldNo++);
			defends.packer(mode, a, lastPack, changes, fieldNo++);
			aiKills.packer(mode, a, lastPack, changes, fieldNo++);
			aiRepairs.packer(mode, a, lastPack, changes, fieldNo++);
			bridgeKills.packer(mode, a, lastPack, changes, fieldNo++);
			bridgeRepairs.packer(mode, a, lastPack, changes, fieldNo++);
			factoryKills.packer(mode, a, lastPack, changes, fieldNo++);
			firebaseKills.packer(mode, a, lastPack, changes, fieldNo++);
			resupplies.packer(mode, a, lastPack, changes, fieldNo++);
			resuppliesScored.packer(mode, a, lastPack, changes, fieldNo++);
			guardingKills.packer(mode, a, lastPack, changes, fieldNo++);
			cappingKills.packer(mode, a, lastPack, changes, fieldNo++);
			umsKills.packer(mode, a, lastPack, changes, fieldNo++);
			guardingPoints.packer(mode, a, lastPack, changes, fieldNo++);
			spawnees.packer(mode, a, lastPack, changes, fieldNo++);
			uniqueHits.packer(mode, a, lastPack, changes, fieldNo++);
			vehtypesHit.packer(mode, a, lastPack, changes, fieldNo++);
			points.packer(mode, a, lastPack, changes, fieldNo++);
			assists.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Number of vehicles hit.
		WWII::SyncVar<UINT32,0> hits;
		//! Number of vehicles killed.
		WWII::SyncVar<UINT32,0> kills;
		//! Number of facilities captured.
		WWII::SyncVar<UINT32,0> captures;
		//! Number of facilities defended.
		WWII::SyncVar<UINT32,0> defends;
		//! Number of AIs killed.
		WWII::SyncVar<UINT32,0> aiKills;
		//! Number of AIs repaired.
		WWII::SyncVar<UINT32,0> aiRepairs;
		//! Number of bridges damaged.
		WWII::SyncVar<UINT32,0> bridgeKills;
		//! Number of bridges repaired.
		WWII::SyncVar<UINT32,0> bridgeRepairs;
		//! Number of factories damaged.
		WWII::SyncVar<UINT32,0> factoryKills;
		//! Number of firebases damaged.
		WWII::SyncVar<UINT32,0> firebaseKills;
		//! Number of resupply actions.
		WWII::SyncVar<UINT32,0> resupplies;
		//! Number of resupplies awarded score.
		WWII::SyncVar<UINT32,0> resuppliesScored;
		//! Number of kills scored from inside a cap bldg.
		WWII::SyncVar<UINT32,0> guardingKills;
		//! Number of kills scored on enemies inside a cap bldg.
		WWII::SyncVar<UINT32,0> cappingKills;
		//! Number of un-manned mobile spawns killed.
		WWII::SyncVar<UINT32,0> umsKills;
		//! Amount of time spent 'guarding'.
		WWII::SyncVar<UINT32,0> guardingPoints;
		//! Number of mobile spawns awarded you.
		WWII::SyncVar<UINT32,0> spawnees;
		//! Number of unique vehicles hit.
		WWII::SyncVar<UINT32,0> uniqueHits;
		//! Number of vehicle types hit.
		WWII::SyncVar<UINT32,0> vehtypesHit;
		//! Points scored (will be modified by RTB).
		WWII::SyncVar<UINT32,0> points;
		//! Number of kill-assists awarded.
		WWII::SyncVar<UINT16,0> assists;
	} m_scoring;
	const Scoring& scoring() const { return m_scoring; }
	Scoring& scoring() { return m_scoring; }

	// Member Functions.


	//! @brief Returns the sortieID.
	UINT32 sortieID() const
	{
		return m_sortie.sortieID();
	}

	//! @brief Returns the duration the sortie has been spawned for.
	UINT32 duration_s() const
	{
		return (UINT32)(sortie().despawned_ms() - sortie().spawned_ms()) / 1000U;
	}

	//! @brief Returns the duration the sortie has been spawned for.
	TeulTimeMS duration_ms() const
	{
		return sortie().despawned_ms() - sortie().spawned_ms();
	}
}; // class SortieData

} // namespace MM

