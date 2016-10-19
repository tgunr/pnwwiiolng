#pragma once
// @file omgMissionSortieData.hxx
// @attention Automatically generated FROM omgMissionSortieData.omg
// @attention Manual edits will be lost.

#include "include/inplace_vector.h"
#include "mm/mmConstants.h"
#include "teul/teulNetTimeMS.h"
#include "teul/teulPacket.h"
#include "weap/weapSpecPacker.h"
#include "wwii/wwiiFullObjIDPacker.h"
#include "wwii/wwiiSyncVar.h"

namespace MM
{

struct SortieData
{
	enum { CallX = MM::CallbackX, CallY = 0 };
	static TEUL_CALLBACK* s_callback;
	static TEUL_CALLBACK* Callback() noexcept { return s_callback; }

	SortieData()
		: m_lastModified(0ULL)
		, m_sortie(m_lastModified)
		, m_vehicle(m_lastModified)
		, m_location(m_lastModified)
		, m_scoring(m_lastModified)
	{
	}

	enum { GroupPresenceBits = 8 };
	const TeulTimeMS& lastModified() const { return m_lastModified; }
	bool groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack) const;
	bool groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack);
	bool packer(PackMode mode, ACTION_OBJECT* const a, TeulTimeMS lastPack) { return (mode == TEUL_PACK) ? pack(a, lastPack) : unpack(a, lastPack); }
	bool pack(ACTION_OBJECT* const a, TeulTimeMS lastPack) const;
	bool unpack(ACTION_OBJECT* const a, TeulTimeMS lastPack);
	TeulTimeMS m_lastModified;

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
		Sortie& operator = (const Sortie& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<UINT32, 0> sortieID;
		WWII::SyncVar<Teul::NetTimeMS, 0> spawned_ms;
		WWII::SyncVar<Teul::NetTimeMS, 0> despawned_ms;
		WWII::SyncVar<FULL_OBJ_ID, 0> originFac;
		WWII::SyncVar<FULL_OBJ_ID, 0> target;
		WWII::SyncVar<UINT32, 0> flags;
	} m_sortie;
	const Sortie& sortie() const { return m_sortie; }
	Sortie& sortie() { return m_sortie; }

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
		Vehicle& operator = (const Vehicle& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<WEAPON_SPEC, 0> weaponSpec;
		WWII::SyncVar<RANK_NO, 0> effectiveRank;
		WWII::SyncVar<UINT8, 0> rtbState;
		WWII::SyncVar<UINT8, 0> decal;
		WWII::SyncVar<ChatHandle, 0> killedBy;
		WWII::SyncVar<FULL_OBJ_ID, 0> rtbFacility;
	} m_vehicle;
	const Vehicle& vehicle() const { return m_vehicle; }
	Vehicle& vehicle() { return m_vehicle; }

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
		Location& operator = (const Location& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<SINT32, 0> x;
		WWII::SyncVar<SINT32, 0> y;
		WWII::SyncVar<SINT32, 0> z;
		WWII::SyncVar<UINT32, 0> riding;
		WWII::SyncVar<FULL_OBJ_ID, 0> nearCP;
	} m_location;
	const Location& location() const { return m_location; }
	Location& location() { return m_location; }

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
		Scoring& operator = (const Scoring& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<UINT32, 0> hits;
		WWII::SyncVar<UINT16, 0> kills;
		WWII::SyncVar<UINT16, 0> captures;
		WWII::SyncVar<UINT16, 0> defends;
		WWII::SyncVar<UINT16, 0> aiKills;
		WWII::SyncVar<UINT16, 0> aiRepairs;
		WWII::SyncVar<UINT16, 0> bridgeKills;
		WWII::SyncVar<UINT16, 0> bridgeRepairs;
		WWII::SyncVar<UINT16, 0> factoryKills;
		WWII::SyncVar<UINT16, 0> firebaseKills;
		WWII::SyncVar<UINT16, 0> resupplies;
		WWII::SyncVar<UINT16, 0> resuppliesScored;
		WWII::SyncVar<UINT16, 0> guardingKills;
		WWII::SyncVar<UINT16, 0> cappingKills;
		WWII::SyncVar<UINT16, 0> umsKills;
		WWII::SyncVar<UINT16, 0> guardingPoints;
		WWII::SyncVar<UINT32, 0> spawnees;
		WWII::SyncVar<UINT16, 0> uniqueHits;
		WWII::SyncVar<UINT16, 0> vehtypesHit;
		WWII::SyncVar<UINT16, 0> points;
		WWII::SyncVar<UINT16, 0> assists;
	} m_scoring;
	const Scoring& scoring() const { return m_scoring; }
	Scoring& scoring() { return m_scoring; }

		UINT32 sortieID() const { return sortie().sortieID(); }
		TeulTimeMS duration_ms() const { return sortie().despawned_ms() - sortie().spawned_ms(); }
		UINT32 duration_s() const { return UINT32(duration_ms() / 1000U); }
}; // end message SortieData

struct TestCond
{
	enum { CallX = MM::CallbackX, CallY = 1 };
	static TEUL_CALLBACK* s_callback;
	static TEUL_CALLBACK* Callback() noexcept { return s_callback; }

	TestCond()
		: sortieID()
		, pilotID()
		, pilotName()
		, crewID()
		, array()
		, bitty()
	{
	}

	bool packer(PackMode mode, ACTION_OBJECT* const a, SINT32 size) { return (mode == TEUL_PACK) ? pack(a, size) : unpack(a, size); }
	bool pack(ACTION_OBJECT* const a, SINT32 size) const;
	bool unpack(ACTION_OBJECT* const a, SINT32 size);
	UINT32 sortieID;
	UINT32 pilotID;
	ChatHandle pilotName;
	UINT32 crewID;
	ChatHandle crewHandle;
	UINT8 crewDecal;
	struct Component
	{
		UINT8 id;
		UINT8 damage;
		struct Id
		{
			UINT8 idNo;
			UINT8 damage[4];
			bool pack(ACTION_OBJECT* const a, SINT32) const;
			bool unpack(ACTION_OBJECT* const a, SINT32);
		};
		using Ids = WWII::inplace_vector<Id, 8>;
		Ids ids;
		bool pack(ACTION_OBJECT* const a, SINT32) const;
		bool unpack(ACTION_OBJECT* const a, SINT32);
	};
	using Components = WWII::inplace_vector<Component, 64>;
	Components components;
	UINT32 array[32];
	UINT32 bitty;
}; // end message TestCond


} // namespace MM

