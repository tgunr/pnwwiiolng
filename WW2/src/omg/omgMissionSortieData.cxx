#include <syscfg.h> // path is project dependent
// @file omgMissionSortieData.cxx
// @attention Automatically generated FROM omgMissionSortieData.omg
// @attention Manual edits will be lost.

#include "omgMissionSortieData.hxx"
TEUL_CALLBACK* MM::SortieData::s_callback;

bool MM::SortieData::groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack) const
{
	UCHAR groupPresence{0};
	SINT32 groupPresenceOffset = a->offset;
	a->pack(groupPresence, 8);
	if (m_sortie.pack(a, lastPack))
		groupPresence |= (1 << 0);
	if (m_vehicle.pack(a, lastPack))
		groupPresence |= (1 << 1);
	if (m_location.pack(a, lastPack))
		groupPresence |= (1 << 2);
	if (m_scoring.pack(a, lastPack))
		groupPresence |= (1 << 3);
	if (groupPresence == 0)
		return false;
	teulPackUCHAR(TEUL_PACK, a->packed, groupPresenceOffset, &groupPresence, 8);
	return (a->offset >= 0);
}

bool MM::SortieData::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack)
{
	UCHAR groupPresence(0);
	a->unpack(groupPresence, 8);
	if (PN_LIKELY(groupPresence != 0)) {
		if (groupPresence & (1 << 0))
			m_sortie.unpack(a, lastPack);
		if (groupPresence & (1 << 1))
			m_vehicle.unpack(a, lastPack);
		if (groupPresence & (1 << 2))
			m_location.unpack(a, lastPack);
		if (groupPresence & (1 << 3))
			m_scoring.unpack(a, lastPack);
		return (a->offset >= 0);
	}
	return false;
}

bool MM::SortieData::pack(ACTION_OBJECT* const a, TeulTimeMS lastPack) const
{
	return groupPack(a, lastPack) && (a->offset >= 0);
}

bool MM::SortieData::unpack(ACTION_OBJECT* const a, TeulTimeMS lastPack)
{
	return groupUnpack(a, lastPack) && (a->offset >= 0);
}

MM::SortieData::Sortie& MM::SortieData::Sortie::operator = (const MM::SortieData::Sortie& rhs)
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

void MM::SortieData::Sortie::groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const
{
	sortieID.pack(a, lastPack, changes, 0);
	spawned_ms.pack(a, lastPack, changes, 1);
	despawned_ms.pack(a, lastPack, changes, 2);
	originFac.pack(a, lastPack, changes, 3);
	target.pack(a, lastPack, changes, 4);
	flags.pack(a, lastPack, changes, 5);
}

void MM::SortieData::Sortie::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes)
{
	sortieID.unpack(a, lastPack, changes, 0);
	spawned_ms.unpack(a, lastPack, changes, 1);
	despawned_ms.unpack(a, lastPack, changes, 2);
	originFac.unpack(a, lastPack, changes, 3);
	target.unpack(a, lastPack, changes, 4);
	flags.unpack(a, lastPack, changes, 5);
}

MM::SortieData::Vehicle& MM::SortieData::Vehicle::operator = (const MM::SortieData::Vehicle& rhs)
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

void MM::SortieData::Vehicle::groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const
{
	weaponSpec.pack(a, lastPack, changes, 0);
	effectiveRank.pack(a, lastPack, changes, 1);
	rtbState.pack(a, lastPack, changes, 2);
	decal.pack(a, lastPack, changes, 3);
	killedBy.pack(a, lastPack, changes, 4);
	rtbFacility.pack(a, lastPack, changes, 5);
}

void MM::SortieData::Vehicle::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes)
{
	weaponSpec.unpack(a, lastPack, changes, 0);
	effectiveRank.unpack(a, lastPack, changes, 1);
	rtbState.unpack(a, lastPack, changes, 2);
	decal.unpack(a, lastPack, changes, 3);
	killedBy.unpack(a, lastPack, changes, 4);
	rtbFacility.unpack(a, lastPack, changes, 5);
}

MM::SortieData::Location& MM::SortieData::Location::operator = (const MM::SortieData::Location& rhs)
{
	m_lastModified = rhs.m_lastModified;
	x.operator=(rhs.x);
	y.operator=(rhs.y);
	z.operator=(rhs.z);
	riding.operator=(rhs.riding);
	nearCP.operator=(rhs.nearCP);
	return *this;
}

void MM::SortieData::Location::groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const
{
	x.pack(a, lastPack, changes, 0);
	y.pack(a, lastPack, changes, 1);
	z.pack(a, lastPack, changes, 2);
	riding.pack(a, lastPack, changes, 3);
	nearCP.pack(a, lastPack, changes, 4);
}

void MM::SortieData::Location::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes)
{
	x.unpack(a, lastPack, changes, 0);
	y.unpack(a, lastPack, changes, 1);
	z.unpack(a, lastPack, changes, 2);
	riding.unpack(a, lastPack, changes, 3);
	nearCP.unpack(a, lastPack, changes, 4);
}

MM::SortieData::Scoring& MM::SortieData::Scoring::operator = (const MM::SortieData::Scoring& rhs)
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

void MM::SortieData::Scoring::groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const
{
	hits.pack(a, lastPack, changes, 0);
	kills.pack(a, lastPack, changes, 1);
	captures.pack(a, lastPack, changes, 2);
	defends.pack(a, lastPack, changes, 3);
	aiKills.pack(a, lastPack, changes, 4);
	aiRepairs.pack(a, lastPack, changes, 5);
	bridgeKills.pack(a, lastPack, changes, 6);
	bridgeRepairs.pack(a, lastPack, changes, 7);
	factoryKills.pack(a, lastPack, changes, 8);
	firebaseKills.pack(a, lastPack, changes, 9);
	resupplies.pack(a, lastPack, changes, 10);
	resuppliesScored.pack(a, lastPack, changes, 11);
	guardingKills.pack(a, lastPack, changes, 12);
	cappingKills.pack(a, lastPack, changes, 13);
	umsKills.pack(a, lastPack, changes, 14);
	guardingPoints.pack(a, lastPack, changes, 15);
	spawnees.pack(a, lastPack, changes, 16);
	uniqueHits.pack(a, lastPack, changes, 17);
	vehtypesHit.pack(a, lastPack, changes, 18);
	points.pack(a, lastPack, changes, 19);
	assists.pack(a, lastPack, changes, 20);
}

void MM::SortieData::Scoring::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes)
{
	hits.unpack(a, lastPack, changes, 0);
	kills.unpack(a, lastPack, changes, 1);
	captures.unpack(a, lastPack, changes, 2);
	defends.unpack(a, lastPack, changes, 3);
	aiKills.unpack(a, lastPack, changes, 4);
	aiRepairs.unpack(a, lastPack, changes, 5);
	bridgeKills.unpack(a, lastPack, changes, 6);
	bridgeRepairs.unpack(a, lastPack, changes, 7);
	factoryKills.unpack(a, lastPack, changes, 8);
	firebaseKills.unpack(a, lastPack, changes, 9);
	resupplies.unpack(a, lastPack, changes, 10);
	resuppliesScored.unpack(a, lastPack, changes, 11);
	guardingKills.unpack(a, lastPack, changes, 12);
	cappingKills.unpack(a, lastPack, changes, 13);
	umsKills.unpack(a, lastPack, changes, 14);
	guardingPoints.unpack(a, lastPack, changes, 15);
	spawnees.unpack(a, lastPack, changes, 16);
	uniqueHits.unpack(a, lastPack, changes, 17);
	vehtypesHit.unpack(a, lastPack, changes, 18);
	points.unpack(a, lastPack, changes, 19);
	assists.unpack(a, lastPack, changes, 20);
}

TEUL_CALLBACK* MM::TestCond::s_callback;

bool MM::TestCond::pack(ACTION_OBJECT* const a, SINT32 size) const
{
	a->pack(sortieID);
	a->pack(pilotID);
	a->pack(pilotName);
	a->pack(crewID);
	if (crewID != 0) {
		a->pack(crewHandle);
		if (crewHandle != 0) {
			a->pack(crewDecal);
		}
		ABORT_IF(components.size() > 64);
		UINT32 Component_count((UINT32)components.size());
		a->pack(Component_count, 8);
		for (UINT32 Component_idx = 0; Component_idx < Component_count; ++Component_idx) {
			components[Component_idx].pack(a, 0);
		}
	}
	a->packBytes(&array[0], sizeof(array));
	a->pack(bitty, 24);
	return (a->offset >= 0);
}

bool MM::TestCond::unpack(ACTION_OBJECT* const a, SINT32 size)
{
	a->unpack(sortieID);
	a->unpack(pilotID);
	a->unpack(pilotName);
	a->unpack(crewID);
	if (crewID != 0) {
		a->unpack(crewHandle);
		if (crewHandle != 0) {
			a->unpack(crewDecal);
		}
		UINT32 Component_count{0};
		a->unpack(Component_count, 8);
		if (Component_count > 64) { a->offset = -EINVAL; return false; }
		components.resize(Component_count);
		for (UINT32 Component_idx = 0; Component_idx < Component_count; ++Component_idx) {
			components[Component_idx].unpack(a, 0);
		}
	}
	a->unpackBytes(&array[0], sizeof(array));
	a->unpack(bitty, 24);
	return (a->offset >= 0);
}

bool MM::TestCond::Component::Id::pack(ACTION_OBJECT* const a, SINT32) const
{
	a->pack(idNo);
	if (idNo) {
		a->packBytes(&damage[0], sizeof(damage));
	}
	return (a->offset >= 0);
}

bool MM::TestCond::Component::Id::unpack(ACTION_OBJECT* const a, SINT32)
{
	a->unpack(idNo);
	if (idNo) {
		a->unpackBytes(&damage[0], sizeof(damage));
	}
	return (a->offset >= 0);
}

bool MM::TestCond::Component::pack(ACTION_OBJECT* const a, SINT32) const
{
	a->pack(id);
	a->pack(damage);
	if (damage > 0) {
		ABORT_IF(ids.size() > 8);
		UINT32 Id_count((UINT32)ids.size());
		a->pack(Id_count, 8);
		for (UINT32 Id_idx = 0; Id_idx < Id_count; ++Id_idx) {
			ids[Id_idx].pack(a, 0);
		}
	}
	return (a->offset >= 0);
}

bool MM::TestCond::Component::unpack(ACTION_OBJECT* const a, SINT32)
{
	a->unpack(id);
	a->unpack(damage);
	if (damage > 0) {
		UINT32 Id_count{0};
		a->unpack(Id_count, 8);
		if (Id_count > 8) { a->offset = -EINVAL; return false; }
		ids.resize(Id_count);
		for (UINT32 Id_idx = 0; Id_idx < Id_count; ++Id_idx) {
			ids[Id_idx].unpack(a, 0);
		}
	}
	return (a->offset >= 0);
}

