#pragma once
// Created: Sat Oct 11 21:03:20 CDT 2008
//
// Base Player class for the host
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmEnums.h"					// For BRANCH_ENUM
#include "inst/instDescriptor.h"		// For Inst::Descriptor
#include "auth/authDispatch.h"
#include "wwii/wwiiPermissions.h"
#include "miss/missFlags.h"				// For MISS_FLAGS

extern void* teulGetPersonaData(SINT32);

namespace Instance { class HostInstance; }
namespace Chat { struct LocalizedMessage; }
namespace Server { enum class ReleaseType; }

class wwiiPlayer
{
protected:
	SINT32				m_teulIndex;
	UINT32				m_playerID;
	UINT32				m_personaID;
	UINT32				m_squadID;
	UINT8				m_updateMode;
	bool				m_netcode;

	UINT32				m_brigadeID;		//! Current brigade, if they are in one, or 0.
	UINT32				m_missionID;		//! Current mission, if they are on one, or 0.
	UINT32				m_sortieID;		//! Current sortie, if they are on one, or 0.

	UINT16				m_weaponSpecNo;	//! If the player has a weapon, this is it's spec.

#pragma pack(push, 1)
	WWII_SIDE			m_side : 8;		//! So we can tell which side when no country is selected.
	WWII_COUNTRY		m_country : 8;		//! Current country.
	BRANCH_ENUM			m_branch : 8;		//! Which branch they're currently in.

	RANK_NO				m_maxRank;			//! Max game-rank granted by subscription
	RANK_NO				m_maxSquadRank;	//! Max squad-rank granted by subscription
	RANK_NO				m_realRank;		//! Rank player would have if no limit imposed.
	RANK_NO				m_realSquadRank;	//! Squad-rank player would have if no limit imposed.
#pragma pack(pop)

public:
	ChatHandle			m_chatHandle;
	ChatHandle			m_matchHandle;
	ChatHandle			m_squadHandle;

	PermissionMask		m_permissionMask;

	Instance::Descriptor	m_instance;
	Instance::HostInstance*	m_hostInstance;

	MISS_FLAGS			m_flags;	//! Mission flags.

private:
	Auth::Dispatch		m_dispatch;
protected:
	void SetDispatch(const Auth::Dispatch& source, UINT32 overridingCustomerID = 0);

public:
	void* operator new(size_t size);
	void operator delete(void* ptr);

	wwiiPlayer() : m_teulIndex(-1) {}
	wwiiPlayer(const wwiiPlayer* rhs) = delete;
	wwiiPlayer(const wwiiPlayer& rhs) = delete;

public:
	wwiiPlayer& operator = (const wwiiPlayer* rhs);
	virtual ~wwiiPlayer() {}

	//// Member accessors
public:
	SINT32 teulIndex() const { return m_teulIndex; }
	UINT32 playerID() const { return m_playerID; }
	UINT32 personaID() const { return m_personaID; }
	UINT32 squadID() const { return m_squadID; }

	UINT32 missionID() const { return m_missionID; }
	UINT32 brigadeID() const { return m_brigadeID; }
	UINT32 sortieID() const { return m_sortieID; }
	RANK_NO maxRank() const { return m_maxRank; }
	RANK_NO maxSquadRank() const { return m_maxSquadRank; }
	RANK_NO	realRank() const { return m_realRank; }
	RANK_NO	effectiveRank() const { return (m_realRank <= m_maxRank) ? m_realRank : m_maxRank; }
	RANK_NO squadRank() const { return m_realSquadRank; }

	ChatHandle chatHandle() const { return m_chatHandle; }
	ChatHandle matchHandle() const { return m_matchHandle; }
	ChatHandle squadHandle() const { return m_squadHandle; }

	UINT32 weaponSpecNo() const { return m_weaponSpecNo; }
	WEAPON_SPEC weaponSpec() const { return m_weaponSpecNo; }

	const PermissionMask& permissionMask() const { return m_permissionMask; }

	const Instance::Descriptor& instance() const { return m_instance; }
	const Auth::Dispatch& dispatch() const { return m_dispatch; }

	const char* callsign() const { return dispatch().m_callsign; }

	WWII_SIDE side() const { return static_cast<WWII_SIDE>(m_side); }
	WWII_COUNTRY country() const { return static_cast<WWII_COUNTRY>(m_country); }
	BRANCH_ENUM branch() const { return m_branch; }

	//// Member functions
public:
	bool hasHandle(const ChatHandle testHandle) const
	{
		return this->m_matchHandle == testHandle || this->m_chatHandle == testHandle;
	}
	const Instance::HostInstance* hostInstance() const { return this->m_hostInstance; }
	Instance::HostInstance* hostInstance() { return this->m_hostInstance; }
	bool inSquad(UINT32 rhs) const { return 0 != rhs && this->m_squadID == rhs; }
	bool inSquad(const wwiiPlayer* rhs) const { return rhs->inSquad(this->squadID()); }

	bool isBuilder() const { return this->permissions(IS_BUILDER); }
	bool isCommander() const { return this->permissions(ALLIED_COMMAND_STAFF | AXIS_COMMAND_STAFF); }
	bool isEffectiveGM() const;
	bool isExplicitGM() const { return this->permissions(GAME_MANAGER); }
	bool isGM() const { return this->permissions(GAME_MANAGER | IS_PLAYNET); }
	bool isMissionLeader() const { return this->permissions(MISSION_LEADER); }
	bool isOfficer() const;	//! If the player is CURRENTLY an officer.
	bool isPlaynet() const { return this->permissions(IS_PLAYNET); }
	bool isTrainer() const { return this->permissions(GAME_TRAINER); }

	bool GetNetcode() { return m_netcode; }
	UINT8 GetUpdateMode() { return m_updateMode; }

	virtual bool message(int prefix, int msgid, const char* const p0="", const char* const p1="", const char* const p2="", const char* const p3="", const char* const p4="", const char* const p5="") const;
	bool message(int prefix, int msgid, const std::string& text) const { return message(prefix, msgid, text.c_str()); }
	virtual bool message(const Chat::LocalizedMessage& lmsg) const;

	// printf-format and send a message
	bool messagef(int prefix, int msgid, const char* fmt, ...);

	// Check for the player having no side/an invalid side.
	bool noSide() const;

	UINT32 OTP() const { return dispatch().m_OTP; }

	bool permissions(const PermissionMask& mask) const { return (this->m_permissionMask & mask) != 0; }

	virtual bool SetInstance(const Instance::Descriptor& toInstance);

	///TODO:INST: This should NOT be exposed
	void SetTeulIndex(SINT32 newIndex) { m_teulIndex = newIndex; }

	void log(const char* message) const;
	void log(const char* filename, int lineno, unsigned int errType, const char* mask, ...) const;

	void Release(SINT32 teulIndex, Server::ReleaseType releaseType);

protected:
	void _setCallsign(const char* to, bool isPlayerPlaynet);
public:	///TODO:INST make me protected
	virtual void ExitBrigade(UINT32 /*exitState*/) { m_brigadeID = 0; }
	virtual void SetCountry(WWII_COUNTRY toCountry, WWII_SIDE toSide);

	void SetNetcode(bool netcode) { m_netcode = netcode; }
	void SetUpdateMode(UINT8 updateMode) { m_updateMode = updateMode; }

public:
	void setDisconnected() { m_teulIndex = -1; }

public:
	static void logStats();
};

extern SINT32 wwiiRegisterPlayerMessageCallback();

#define PF_PLAYER "t:%d p:%u c:%s "
#define VAPF_PLAYER(player) (player)->teulIndex(), (player)->playerID(), (player)->callsign()
#define VAPF_PLAYER_INFO(teulIndex, playerID, callsign) (SINT32)(teulIndex), (UINT32)(playerID), (callsign)
