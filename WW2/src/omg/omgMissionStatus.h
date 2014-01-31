#pragma once
////////////////////////////////////////////////////////////////////////////////////////
// @file omgMissionStatus.h
// @attention AUTOMATICALLY GENERATED FROM omg/omgMissionStatus.xml BY omg/omg-to-h.py
// @attention DO NOT MODIFY THIS FILE DIRECTLY edit omg/omgMissionStatus.xml instead.
////////////////////////////////////////////////////////////////////////////////////////

// For VarGroup, SyncVar
#include "wwii/wwiiSyncVar.h"
// For Teul::NetTimeMS
#include "teul/teulNetTimeMS.h"
// For WorldPosition_m
#include "wwii/wwiiServerTrackedObjectPacker.h"
// For FULL_OBJ_ID
#include "wwii/wwiiFullObjIDPacker.h"

#if defined(_MSC_VER)
# pragma warning(disable : 4355) // C4355: 'this': used in base member initializer list.
#endif

namespace Chat
{

//! @class Chat::MissionStatus
//! @brief Shared description of a mission.
//! @details
//! MissionStatus is a *shared* representation of the current state of a
//! given mission. Member variables use the VarGroup and SyncVar templates
//! to track when member variables are modified so that updates can be sent.
//!
//! The data is compartmentalized into subgroups for bandwidth reduction.
//!
//! Groups are:
//! 	MissionStatus::Header (semi-static definitions)
//! 	MissionStatus::Status (boolean flags)
//! 	MissionStatus::Origin (origin/spawn)
//! 	MissionStatus::Target (objective/target)
//! 	MissionStatus::Leader (current leader)
//! 	MissionStatus::FRU    (FRU/MSP specifics)
//!
//! Member variables of each structure are implemented with the WWII::SyncVar
//! class and are accessed as either
//! 	missionStatus.group().member()
//! or
//! 	missionStatus.group().member(newValue)
//!
//! Example usage:
//! 	Chat::MissionStatus cms;
//!
//! 	// Read the missionID.
//! 	UINT32 missionID = cms.header().missionID();
//!
//! 	// Change origin facility.
//! 	FULL_OBJ_ID fid = getFullObjID("Schilde Armybase");
//! 	cms.origin().fac(fid);
//!
//! 	// Check to see if the overall object has changed in the last second:
//! 	const bool cmsChanged = cms.lastModified() >= teulClock64 - 1000;
//!
//! 	// Check to see if the header has changed in the last second.
//! 	const bool headerChanged = cms.header().lastModified() >= teulClock64 - 1000;

//! @attention This class is automatically generated from omgMissionStatus.xml

class MissionStatus
{
	// Default constructor.
	public:
	MissionStatus()
		: m_lastModified(0ULL)
		, m_header(m_lastModified)
		, m_status(m_lastModified)
		, m_origin(m_lastModified)
		, m_target(m_lastModified)
		, m_lead(m_lastModified)
		, m_fru(m_lastModified)
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

	//! @struct Header
	//! @brief Outline of this mission entity.
	public:
	struct Header : public WWII::VarGroup
	{
		Header(TeulTimeMS& parent)
			: VarGroup(parent)
			, missionID(this)
			, brigadeID(this)
			, poster(this)
			, postedTime_ms(this)
			{}
		Header& operator = (const Header& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			missionID.operator=(rhs.missionID);
			brigadeID.operator=(rhs.brigadeID);
			poster.operator=(rhs.poster);
			postedTime_ms.operator=(rhs.postedTime_ms);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			missionID.packer(mode, a, lastPack, changes, fieldNo++);
			brigadeID.packer(mode, a, lastPack, changes, fieldNo++);
			poster.packer(mode, a, lastPack, changes, fieldNo++);
			postedTime_ms.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Server's ID for this mission (unique per campaign).
		WWII::SyncVar<UINT32,0> missionID;
		//! ID of the StratHCUnit the mission belongs to.
		WWII::SyncVar<UINT16,0> brigadeID;
		//! ChatHandle of original mission poster.
		WWII::SyncVar<ChatHandle,0ULL> poster;
		//! Timestamp of mission creation.
		WWII::SyncVar<Teul::NetTimeMS,0LL> postedTime_ms;
	} m_header;
	const Header& header() const { return m_header; }
	Header& header() { return m_header; }

	//! @struct Status
	//! @brief Flags indicating the current status of the mission.
	public:
	struct Status : public WWII::VarGroup
	{
		Status(TeulTimeMS& parent)
			: VarGroup(parent)
			, isOpen(this)
			, isOngoing(this)
			, hasMobileSpawn(this)
			, canMobileSpawn(this)
			{}
		Status& operator = (const Status& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			isOpen.operator=(rhs.isOpen);
			isOngoing.operator=(rhs.isOngoing);
			hasMobileSpawn.operator=(rhs.hasMobileSpawn);
			canMobileSpawn.operator=(rhs.canMobileSpawn);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			isOpen.packer(mode, a, lastPack, changes, fieldNo++);
			isOngoing.packer(mode, a, lastPack, changes, fieldNo++);
			hasMobileSpawn.packer(mode, a, lastPack, changes, fieldNo++);
			canMobileSpawn.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Whether mission is currently accepting new takers.
		WWII::SyncVar<bool,false> isOpen;
		//! Whether mission is waiting for people to leave.
		WWII::SyncVar<bool,false> isOngoing;
		//! True when mission has an active mobile spawn.
		WWII::SyncVar<bool,false> hasMobileSpawn;
		//! True when mission is capable of hosting a mobile spawn.
		WWII::SyncVar<bool,false> canMobileSpawn;
	} m_status;
	const Status& status() const { return m_status; }
	Status& status() { return m_status; }

	//! @struct Origin
	//! @brief Details of the mission's current origin.
	public:
	struct Origin : public WWII::VarGroup
	{
		Origin(TeulTimeMS& parent)
			: VarGroup(parent)
			, fac(this)
			, cp(this)
			, inSupply(this)
			{}
		Origin& operator = (const Origin& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			fac.operator=(rhs.fac);
			cp.operator=(rhs.cp);
			inSupply.operator=(rhs.inSupply);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			fac.packer(mode, a, lastPack, changes, fieldNo++);
			cp.packer(mode, a, lastPack, changes, fieldNo++);
			inSupply.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Spawn facility.
		WWII::SyncVar<FULL_OBJ_ID> fac;
		//! Spawn CP.
		WWII::SyncVar<FULL_OBJ_ID> cp;
		//! True if origin is in supply.
		WWII::SyncVar<bool,false> inSupply;
	} m_origin;
	const Origin& origin() const { return m_origin; }
	Origin& origin() { return m_origin; }

	//! @struct Target
	//! @brief Details of the mission's current target/objective.
	public:
	struct Target : public WWII::VarGroup
	{
		Target(TeulTimeMS& parent)
			: VarGroup(parent)
			, fac(this)
			, cp(this)
			, originLink(this)
			{}
		Target& operator = (const Target& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			fac.operator=(rhs.fac);
			cp.operator=(rhs.cp);
			originLink.operator=(rhs.originLink);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			fac.packer(mode, a, lastPack, changes, fieldNo++);
			cp.packer(mode, a, lastPack, changes, fieldNo++);
			originLink.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Target facility (if applicable).
		WWII::SyncVar<FULL_OBJ_ID> fac;
		//! Target CP.
		WWII::SyncVar<FULL_OBJ_ID> cp;
		//! Does origin have a link to the target.
		WWII::SyncVar<bool,false> originLink;
	} m_target;
	const Target& target() const { return m_target; }
	Target& target() { return m_target; }

	//! @struct Lead
	//! @brief Current mission leadership.
	public:
	struct Lead : public WWII::VarGroup
	{
		Lead(TeulTimeMS& parent)
			: VarGroup(parent)
			, current(this)
			, backup(this)
			{}
		Lead& operator = (const Lead& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			current.operator=(rhs.current);
			backup.operator=(rhs.backup);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			current.packer(mode, a, lastPack, changes, fieldNo++);
			backup.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Current leader.
		WWII::SyncVar<ChatHandle,0ULL> current;
		//! NULL or backup leader's gamename.
		WWII::SyncVar<ChatHandle,0ULL> backup;
	} m_lead;
	const Lead& lead() const { return m_lead; }
	Lead& lead() { return m_lead; }

	//! @struct FRU
	//! @brief FRU/MSP parameters.
	public:
	struct FRU : public WWII::VarGroup
	{
		FRU(TeulTimeMS& parent)
			: VarGroup(parent)
			, position(this)
			, creator(this)
			, nextPlaceAvail(this)
			, active(this)
			{}
		FRU& operator = (const FRU& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			position.operator=(rhs.position);
			creator.operator=(rhs.creator);
			nextPlaceAvail.operator=(rhs.nextPlaceAvail);
			active.operator=(rhs.active);
			return *this;
		}
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			position.packer(mode, a, lastPack, changes, fieldNo++);
			creator.packer(mode, a, lastPack, changes, fieldNo++);
			nextPlaceAvail.packer(mode, a, lastPack, changes, fieldNo++);
			active.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Absolute world co-ordinates.
		WWII::SyncVar<WorldPosition_m> position;
		//! GameName of player who placed it.
		WWII::SyncVar<ChatHandle,0ULL> creator;
		//! Next time FRU can be placed.
		WWII::SyncVar<Teul::NetTimeMS,0LL> nextPlaceAvail;
		//! FRU active?
		WWII::SyncVar<bool,false> active;

		//! @brief Calculate the miliseconds remaining until placement allowed.
		//! @returns 0 to indicate "now", otherwise a positive number of miliseconds until placement is allowed.
		TeulTimeMS cooldownRemaining() const
		{
			SINT64 cooldown = (SINT64)nextPlaceAvail() - (SINT64)teulClock64;
			return (cooldown > 0) ? (TeulTimeMS)cooldown : (TeulTimeMS)0ULL;
		}
	} m_fru;
	const FRU& fru() const { return m_fru; }
	FRU& fru() { return m_fru; }

}; // class MissionStatus

} // namespace Chat

