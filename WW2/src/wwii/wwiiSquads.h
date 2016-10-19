#pragma once
// Created: Mon Sep 12 14:34:49 CDT 2005
//
// Classes and definitions for describing squads
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiHash.h"
#include "wwii/wwiiFullObjID.h"

#define DEFAULT_ROOKIE_DAYS 7

//////////////////////////////////////////////////////////////////////
// Squad list includes its own integrated index
class Squad							// Base squad info
{
public:
	Squad(UINT32 squadID=0, const ChatHandle handle=0LL, const std::string& name="");
	Squad(ACTION_OBJECT* const a) { a->offset = packer(TEUL_PACK, a); }

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

public:
	enum { NOT_IN_SQUAD, RECRUIT, MEMBER, STAFFER, XO, CO, GM, MAX_RANK };		// Squad ranks

public:
	enum {
		SHORT_NAME_LEN = 10
	,	NAME_LENGTH = 40
	,	SLOGAN_LENGTH = 80
	,	DESCRIPTION_LENGTH = 255
	,	MOTD_LENGTH = 400
	,	HOMEPAGE_LENGTH = 80
	,	VOICE_INFO_LENGTH = 80
	,	MIN_NIGHT_LENGTH = 5		// 0,0,0
	,	MAX_NIGHT_LENGTH = 8		// 0,23,-23
	};

public:
	const std::string& callsign() const { return m_callsign; }
	UINT32 created() const { return m_created; }
	ChatHandle creatorHandle() const { return m_creatorHandle; }
	ChatHandle coHandle() const { return m_coHandle; }
	const std::string& description() const { return m_description; }
	UINT32 flags() const { return m_flags; }
	const std::string& homepage() const { return m_homepage; }
	UINT32 lastJoined() const { return m_lastJoined; }
	ChatHandle lastJoiner() const { return m_lastJoiner; }
	const std::string& motd() const { return m_motd; }
	const std::string& name() const { return m_name; }
	bool recruiting() const { return m_recruiting; }
	UINT32 rookieDays() const { return m_rookieDays; }
	ChatHandle shortHandle() const { return m_shortHandle; }
	const char* shortName() const { return m_callsign.c_str(); }
	UINT32 squadID() const { return m_squadID; }
	bool votingEnabled() const { return m_votingEnabled; }
	const std::string& voiceInfo() const { return m_voiceInfo; }
	UCHAR squadNight() const { return m_squadNight; }
	UCHAR squadHour() const { return m_squadHour; }
	SCHAR squadTimezone() const { return m_squadTimezone; }
	UCHAR language() const { return m_language; }
	const std::string& slogan() const { return m_slogan; }

	void setCOHandle(const ChatHandle handle) { m_coHandle = handle; }
	void setVotingEnabled(bool state) { m_votingEnabled = state; }

	// Split a squad-night string into constituent fields;
	// returns true if the new value is parseable and differs
	// from the current values
	bool setSquadNight(const char* text);

protected:
	UINT32			m_squadID;					// Squad's ID
	ChatHandle		m_shortHandle;				// short name packed
	ChatHandle		m_creatorHandle;			// guy who owns the squad now
	ChatHandle		m_coHandle;					// guy who owns the squad now
	ChatHandle		m_lastJoiner;
	UINT32			m_flags;
	UINT32			m_created;					// date/time of creation
	UINT32			m_lastJoined;				//
	UINT32			m_rookieDays;
	UCHAR			m_squadNight;				// what day of week sun = 0, sat = 6
	UCHAR			m_squadHour;				// what hour of the day 0 - 23
	UCHAR			m_language;					// squad's language 0 based language index
	SCHAR			m_squadTimezone;			// what timezone -23 to +23 UTC
	bool			m_votingEnabled;			// is voting allowed
	bool			m_recruiting;				// are they recruiting new members?

public:
	std::string	m_name;
	std::string	m_description;
	std::string	m_motd;
	std::string	m_homepage;
	std::string	m_callsign;
	std::string m_voiceInfo; // Voice server info
	std::string m_slogan;

	friend void chatdbSquadInfo(SINT32 teulIndex, UINT32 squadID, bool sendMembers);
	friend class SquadList;
};

//////////////////////////////////////////////////////////////////////
// Class that encapsulates recruiter info
class Recruiter
{
public:
	Recruiter(const ChatHandle withHandle=0LL, const Squad* withSquad=nullptr);
	Recruiter(const class Player* withPlayer, const class SquadList* withSquad, const class ChatMission* withMission);
	Recruiter(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

public:

	ChatHandle handle() const { return m_handle; }
	const char* callsign() const { return chatGetCallsign(m_handle); }
	ChatHandle squadHandle() const { return m_squadHandle; }
	const char* squadCallsign() const { return chatGetCallsign(m_squadHandle); }
	const std::string& squadName() const { return m_squadName; }
	const std::string& squadSlogan() const { return m_slogan; }
	UCHAR country() const { return m_country; }
	UCHAR branch() const { return m_branch; }
	UCHAR language() const { return m_language; }
	FULL_OBJ_ID missionOrigin() const { return m_origin; }
	FULL_OBJ_ID missionTarget() const { return m_target; }
	UCHAR missionType() const { return m_mission; }
	UCHAR missionActivity() const { return m_activity; }
	bool valid() const { return m_handle > 0; }

	void setSquadHandle(const ChatHandle toHandle) { m_squadHandle = toHandle; }
	void setSquadName(const std::string& toName) { m_squadName = toName; }
	void setSquadName(const char* toName) { m_squadName = toName; }
	void setCountry(UCHAR toCountry) { m_country = toCountry; }
	void setBranch(UCHAR toBranch) { m_branch = toBranch; }
	void setLangauge(UCHAR toLanguage) { m_language = toLanguage; }
	void setSlogan(const std::string& toSlogan) { m_slogan = toSlogan; }
	void setSlogan(const char* toSlogan) { m_slogan = toSlogan; }
	void setOrigin(const FULL_OBJ_ID toOrigin) { m_origin = toOrigin; }
	void setMissionType(UCHAR toType) { m_mission = toType; }
	void setActivity(UCHAR toActivity) { m_activity = toActivity; }
	void setTarget(const FULL_OBJ_ID toTarget) { m_target = toTarget; }

protected:
	ChatHandle			m_handle;			// handle of recruiter
	ChatHandle			m_squadHandle;		// Squad's ID
	UCHAR				m_country;			// may be needed at some point
	UCHAR				m_branch;			// may be needed at some point
	UCHAR				m_language;			// language index of squad or recruiter; depends on which criteria was used in the recruiter selection
	UCHAR				m_mission;			// mission type of the recruiter
	UCHAR				m_activity;			// mission population meter or possibly number of squad members on the mission
	std::string			m_squadName;		// squad long name
	std::string			m_slogan;			// they want a recruitment slogan; send the description for now
	FULL_OBJ_ID			m_origin;			// mission origin of the recruiter
	FULL_OBJ_ID			m_target;
};
