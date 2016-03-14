#include <syscfg.h> // path is project dependent
// @file omgChat.cxx
// @attention Automatically generated FROM omgChat.omg
// @attention Manual edits will be lost.

#include "omgChat.hxx"
TEUL_CALLBACK* Chat::MissionStatus::s_callback;

bool Chat::MissionStatus::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack) const
{
	UCHAR groupPresence{0};
	SINT32 groupPresenceOffset = a->offset;
	a->pack(groupPresence, 8);
	if (m_header.pack(a, lastPack))
		groupPresence |= (1 << 0);
	if (m_status.pack(a, lastPack))
		groupPresence |= (1 << 1);
	if (m_origin.pack(a, lastPack))
		groupPresence |= (1 << 2);
	if (m_target.pack(a, lastPack))
		groupPresence |= (1 << 3);
	if (m_lead.pack(a, lastPack))
		groupPresence |= (1 << 4);
	if (m_fru.pack(a, lastPack))
		groupPresence |= (1 << 5);
	if (groupPresence == 0)
		return false;
	teulPackUCHAR(TEUL_PACK, a->packed, groupPresenceOffset, &groupPresence, 8);
	return (a->offset >= 0);
}

bool Chat::MissionStatus::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack)
{
	UCHAR groupPresence(0);
	a->unpack(groupPresence, 8);
	if (PN_LIKELY(groupPresence != 0)) {
		if (groupPresence & (1 << 0))
			m_header.unpack(a, lastPack);
		if (groupPresence & (1 << 1))
			m_status.unpack(a, lastPack);
		if (groupPresence & (1 << 2))
			m_origin.unpack(a, lastPack);
		if (groupPresence & (1 << 3))
			m_target.unpack(a, lastPack);
		if (groupPresence & (1 << 4))
			m_lead.unpack(a, lastPack);
		if (groupPresence & (1 << 5))
			m_fru.unpack(a, lastPack);
		return (a->offset >= 0);
	}
	return false;
}

bool Chat::MissionStatus::pack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack) const
{
	return groupPack(a, lastPack) && (a->offset >= 0);
}

bool Chat::MissionStatus::unpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack)
{
	return groupUnpack(a, lastPack) && (a->offset >= 0);
}

Chat::MissionStatus::Header& Chat::MissionStatus::Header::operator = (const Chat::MissionStatus::Header& rhs)
{
	m_lastModified = rhs.m_lastModified;
	missionID.operator=(rhs.missionID);
	brigadeID.operator=(rhs.brigadeID);
	poster.operator=(rhs.poster);
	postedTime_ms.operator=(rhs.postedTime_ms);
	return *this;
}

void Chat::MissionStatus::Header::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const
{
	missionID.pack(a, lastPack, changes, 0);
	brigadeID.pack(a, lastPack, changes, 1);
	poster.pack(a, lastPack, changes, 2);
	postedTime_ms.pack(a, lastPack, changes, 3);
}

void Chat::MissionStatus::Header::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	missionID.unpack(a, lastPack, changes, 0);
	brigadeID.unpack(a, lastPack, changes, 1);
	poster.unpack(a, lastPack, changes, 2);
	postedTime_ms.unpack(a, lastPack, changes, 3);
}

Chat::MissionStatus::Status& Chat::MissionStatus::Status::operator = (const Chat::MissionStatus::Status& rhs)
{
	m_lastModified = rhs.m_lastModified;
	isOpen.operator=(rhs.isOpen);
	isOngoing.operator=(rhs.isOngoing);
	hasMobileSpawn.operator=(rhs.hasMobileSpawn);
	canMobileSpawn.operator=(rhs.canMobileSpawn);
	return *this;
}

void Chat::MissionStatus::Status::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const
{
	isOpen.pack(a, lastPack, changes, 0);
	isOngoing.pack(a, lastPack, changes, 1);
	hasMobileSpawn.pack(a, lastPack, changes, 2);
	canMobileSpawn.pack(a, lastPack, changes, 3);
}

void Chat::MissionStatus::Status::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	isOpen.unpack(a, lastPack, changes, 0);
	isOngoing.unpack(a, lastPack, changes, 1);
	hasMobileSpawn.unpack(a, lastPack, changes, 2);
	canMobileSpawn.unpack(a, lastPack, changes, 3);
}

Chat::MissionStatus::Origin& Chat::MissionStatus::Origin::operator = (const Chat::MissionStatus::Origin& rhs)
{
	m_lastModified = rhs.m_lastModified;
	cp.operator=(rhs.cp);
	fac.operator=(rhs.fac);
	inSupply.operator=(rhs.inSupply);
	return *this;
}

void Chat::MissionStatus::Origin::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const
{
	cp.pack(a, lastPack, changes, 0);
	fac.pack(a, lastPack, changes, 1);
	inSupply.pack(a, lastPack, changes, 2);
}

void Chat::MissionStatus::Origin::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	cp.unpack(a, lastPack, changes, 0);
	fac.unpack(a, lastPack, changes, 1);
	inSupply.unpack(a, lastPack, changes, 2);
}

Chat::MissionStatus::Target& Chat::MissionStatus::Target::operator = (const Chat::MissionStatus::Target& rhs)
{
	m_lastModified = rhs.m_lastModified;
	cp.operator=(rhs.cp);
	fac.operator=(rhs.fac);
	originLink.operator=(rhs.originLink);
	return *this;
}

void Chat::MissionStatus::Target::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const
{
	cp.pack(a, lastPack, changes, 0);
	fac.pack(a, lastPack, changes, 1);
	originLink.pack(a, lastPack, changes, 2);
}

void Chat::MissionStatus::Target::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	cp.unpack(a, lastPack, changes, 0);
	fac.unpack(a, lastPack, changes, 1);
	originLink.unpack(a, lastPack, changes, 2);
}

Chat::MissionStatus::Lead& Chat::MissionStatus::Lead::operator = (const Chat::MissionStatus::Lead& rhs)
{
	m_lastModified = rhs.m_lastModified;
	current.operator=(rhs.current);
	backup.operator=(rhs.backup);
	return *this;
}

void Chat::MissionStatus::Lead::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const
{
	current.pack(a, lastPack, changes, 0);
	backup.pack(a, lastPack, changes, 1);
}

void Chat::MissionStatus::Lead::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	current.unpack(a, lastPack, changes, 0);
	backup.unpack(a, lastPack, changes, 1);
}

Chat::MissionStatus::FRU& Chat::MissionStatus::FRU::operator = (const Chat::MissionStatus::FRU& rhs)
{
	m_lastModified = rhs.m_lastModified;
	active.operator=(rhs.active);
	nextPlaceAvail.operator=(rhs.nextPlaceAvail);
	position.operator=(rhs.position);
	creator.operator=(rhs.creator);
	return *this;
}

void Chat::MissionStatus::FRU::groupPack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes) const
{
	active.pack(a, lastPack, changes, 0);
	nextPlaceAvail.pack(a, lastPack, changes, 1);
	position.pack(a, lastPack, changes, 2);
	creator.pack(a, lastPack, changes, 3);
}

void Chat::MissionStatus::FRU::groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	active.unpack(a, lastPack, changes, 0);
	nextPlaceAvail.unpack(a, lastPack, changes, 1);
	position.unpack(a, lastPack, changes, 2);
	creator.unpack(a, lastPack, changes, 3);
}

TEUL_CALLBACK* Chat::CellHostUpdate::s_callback;

bool Chat::CellHostUpdate::pack(ACTION_OBJECT* const a, SINT32 size) const
{
	a->pack(playerID, 0);
	a->pack(cellHost, 0);
	return (a->offset >= 0);
}

bool Chat::CellHostUpdate::unpack(ACTION_OBJECT* const a, SINT32 size)
{
	a->unpack(playerID, 0);
	a->unpack(cellHost, 0);
	return (a->offset >= 0);
}

