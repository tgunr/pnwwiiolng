#pragma once
// @file omgChat.hxx
// @attention Automatically generated FROM omgChat.omg
// @attention Manual edits will be lost.

#include "chat/chatConstants.h"
#include "teul/teulNetTimeMS.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiFullObjIDPacker.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "wwii/wwiiSyncVar.h"

namespace Chat
{

struct MissionStatus
{
	enum { CallX = Chat::CallbackX, CallY = 100 };
	static TEUL_CALLBACK* s_callback;
	static TEUL_CALLBACK* Callback() noexcept { return s_callback; }

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

	enum { GroupPresenceBits = 8 };
	const TeulTimeMS& lastModified() const { return m_lastModified; }
	bool groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack) const;
	bool groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack);
	bool packer(LOGICAL mode, ACTION_OBJECT* const a, TeulTimeMS lastPack) { return (mode == TEUL_PACK) ? pack(a, lastPack) : unpack(a, lastPack); }
	bool pack(ACTION_OBJECT* const a, TeulTimeMS lastPack) const;
	bool unpack(ACTION_OBJECT* const a, TeulTimeMS lastPack);
	TeulTimeMS m_lastModified;

	struct Header : public WWII::VarGroup
	{
		Header(TeulTimeMS& parent)
			: VarGroup(parent)
			, missionID(this)
			, brigadeID(this)
			, poster(this)
			, postedTime_ms(this)
		{}
		Header& operator = (const Header& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<UINT32, 0> missionID;
		WWII::SyncVar<UINT16, 0> brigadeID;
		WWII::SyncVar<ChatHandle, 0> poster;
		WWII::SyncVar<Teul::NetTimeMS, 0> postedTime_ms;
	} m_header;
	const Header& header() const { return m_header; }
	Header& header() { return m_header; }

	struct Status : public WWII::VarGroup
	{
		Status(TeulTimeMS& parent)
			: VarGroup(parent)
			, isOpen(this)
			, isOngoing(this)
			, hasMobileSpawn(this)
			, canMobileSpawn(this)
		{}
		Status& operator = (const Status& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<bool, 0> isOpen;
		WWII::SyncVar<bool, 0> isOngoing;
		WWII::SyncVar<bool, 0> hasMobileSpawn;
		WWII::SyncVar<bool, 0> canMobileSpawn;
	} m_status;
	const Status& status() const { return m_status; }
	Status& status() { return m_status; }

	struct Origin : public WWII::VarGroup
	{
		Origin(TeulTimeMS& parent)
			: VarGroup(parent)
			, cp(this)
			, fac(this)
			, inSupply(this)
		{}
		Origin& operator = (const Origin& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<FULL_OBJ_ID, 0> cp;
		WWII::SyncVar<FULL_OBJ_ID, 0> fac;
		WWII::SyncVar<bool, 0> inSupply;
	} m_origin;
	const Origin& origin() const { return m_origin; }
	Origin& origin() { return m_origin; }

	struct Target : public WWII::VarGroup
	{
		Target(TeulTimeMS& parent)
			: VarGroup(parent)
			, cp(this)
			, fac(this)
			, originLink(this)
		{}
		Target& operator = (const Target& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<FULL_OBJ_ID, 0> cp;
		WWII::SyncVar<FULL_OBJ_ID, 0> fac;
		WWII::SyncVar<bool, 0> originLink;
	} m_target;
	const Target& target() const { return m_target; }
	Target& target() { return m_target; }

	struct Lead : public WWII::VarGroup
	{
		Lead(TeulTimeMS& parent)
			: VarGroup(parent)
			, current(this)
			, backup(this)
		{}
		Lead& operator = (const Lead& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<ChatHandle, 0> current;
		WWII::SyncVar<ChatHandle, 0> backup;
	} m_lead;
	const Lead& lead() const { return m_lead; }
	Lead& lead() { return m_lead; }

	struct FRU : public WWII::VarGroup
	{
		FRU(TeulTimeMS& parent)
			: VarGroup(parent)
			, subtype(this)
			, active(this)
			, nextPlaceAvail(this)
			, position(this)
			, creator(this)
		{}
		FRU& operator = (const FRU& rhs);
		void groupPack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) const override;
		void groupUnpack(ACTION_OBJECT* const a, const TeulTimeMS lastPack, UINT32& changes) override;
		WWII::SyncVar<SINT32, 0> subtype;
		WWII::SyncVar<bool, 0> active;
		WWII::SyncVar<Teul::NetTimeMS, 0> nextPlaceAvail;
		WWII::SyncVar<WorldPosition_m, 0> position;
		WWII::SyncVar<ChatHandle, 0> creator;

			//! \brief Calculate the milliseconds remaining until placement allowed.
			//! \returns 0 to indicate "now", otherwise a positive number of milliseconds.
			TeulTimeMS cooldownRemaining() const
			{
				SINT64 cooldown = (SINT64)nextPlaceAvail() - (SINT64)teulClock64;
				return (cooldown > 0) ? (TeulTimeMS)cooldown : (TeulTimeMS)0ULL;
			}
	} m_fru;
	const FRU& fru() const { return m_fru; }
	FRU& fru() { return m_fru; }

}; // end message MissionStatus

struct CellHostUpdate
{
	enum { CallX = Chat::CallbackX, CallY = 101 };
	static TEUL_CALLBACK* s_callback;
	static TEUL_CALLBACK* Callback() noexcept { return s_callback; }

	CellHostUpdate()
		: playerID()
		, cellHost()
	{
	}
	explicit CellHostUpdate(playerid_t playerID_, UINT32 cellHost_) noexcept : playerID{playerID_}, cellHost{cellHost_} {}

	bool packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 size) { return (mode == TEUL_PACK) ? pack(a, size) : unpack(a, size); }
	bool pack(ACTION_OBJECT* const a, SINT32 size) const;
	bool unpack(ACTION_OBJECT* const a, SINT32 size);
	playerid_t playerID;
	UINT32 cellHost;
}; // end message CellHostUpdate


} // namespace Chat

