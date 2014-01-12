#pragma once
// Created: Wed Apr 20 14:47:59 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>
#include <set>

#include "auth/authDispatch.h"
#include "mm/mmEnums.h"
#include "chat/chatChannels.h"
#include "chat/chatChannelTypes.h"
#include "chat/chatGroupsType.h"
#include "weap/weapSpec.h"
#include "wwii/wwiiPermissions.h"
#include "chat/chatPrototypes.h"
#include "chat/chatLStringIDs.h"
#include "wwii/wwiiFullObjID.h"
#include "chat/chatDefines.h"

#include "wwii/wwiiPlayer.h"

class SquadInvite;
class PlayerStatusNode;
typedef std::set<ChatHandle> HANDLES;
namespace Chat
{
	class PersonaUpdate;
	struct PlayerInfo;
	class Channel;
	class ChannelSubscription;
}

static const UINT32	MAX_AFK_MESSAGE_LENGTH = 60;

extern ChatHandle systemHandleID;

//////////////////////////////////////////////////////////////////////
// The player structure

//The data I keep on the chat side about every player connected
class Player : public wwiiPlayer
{
public:
	//// Try to keep the funtion list in alpha order
	// Acknowledge channel tuned
	// Channel is detuned if ack-send fails
	bool ackTuneHandle(UCHAR index, const ChatHandle& handle);

	// Complete the process of connection
	void activate(SINT32 newIndex, PlayerStatusNode* psn);

	void addName(std::string& toString, const char* joiner=" ") const;

	// Determine if and for which side a player is an active commander
	WWII_SIDE activeCommandSide() const;

	// Add them to the command list
	void addToCommandList(void);

	// Announce the player to their squad channel
	void announceToSquadChannel();

	// Take me out of my current brigade
	void changeBrigade(UINT32 newID);

	// Set my branch
	void SetBranch(BRANCH_ENUM newBranch);

	// Change my country and side
	virtual void SetCountry(WWII_COUNTRY toCountry, WWII_SIDE toSide) OVERRIDE_FN;

	// If the last group read was less more than a minute ago,
	// then force a re-read of the group memberships
	void checkGroups();

	bool contextualStratCommand(const char* string, const char* arguments, WWII_COUNTRY country, WWII_SIDE side, const class Property* property);

	FLOAT64 currentX() const { return isSpawned() ? lastX() : 0x7fffffff; }
	FLOAT64 currentY() const { return isSpawned() ? lastY() : 0x7fffffff; }
	FLOAT64 currentZ() const { return isSpawned() ? lastZ() : 0x7fffffff; }

	void deactivate();

	// Describe player to a GM
	virtual void describeTo(const Player& player, const FLOAT64& x, const FLOAT64& y, bool includePermissions) const;

	// Remove all non-private channels
	void detuneChannels();

	// Remove all channels of a specific type
	void detuneChannelsByType(CHANNEL_TYPE type);

	// Detune all radios
	void detuneAllRadios();

	// Detune a particular channel index
	void detuneRadio(UCHAR index);

	WWII_SIDE effectiveCommandSide() const;
	WWII_SIDE effectiveOfficerSide() const;

	void Emit(const Chat::LocalizedMessage& lmsg);

	// If the feature is currently locked: Returns true
	// If the feature was not locked, locks the feature
	bool featureLock(const std::string& feature, UINT32 now, UINT32 duration=0);	// In chatHost.cpp

	// Get the last octet of the cell host or 0 if player isn't spawned
	UINT32 getCellHost(class PlayerStatusNode* psn=nullptr) const;

	// Find the first available channel index
	UINT32 getOpenChannelIndex(UCHAR type=NO_CHANNEL);

	UINT32 getPreference(UINT32 prefID) const;

	// Report/log a gm announcement notice
	void gmNote(const SCHAR* level, const std::string& command, const SCHAR* data1, const SCHAR* data2);

	// Set the grab variables
	void grab(Player* rhs);

	// Reference our group list after ensuring it is current
	const CHAT_GROUPS& groups();

	bool ignoring(const Player* rhs) const { return ignoring(rhs->matchHandle()); }
	bool ignoring(const Player& rhs) const { return ignoring(rhs.matchHandle()); }
	bool ignoring(const ChatHandle& handle) const { return m_ignores.find(handle) != m_ignores.end(); }

	// Invalidate the group list forcibly
	void invalidateGroups() { m_lastGroupRead = 0; }

	bool invalidDataDetected() const { return m_invalidData; }

	// Tests against the player's brigade/squad/mission ID
	bool inFacility() const { return (m_inFacility); }

	// Test to see if they are currently a commander
	bool isActiveCommander() const { return isOfficer(); }

	bool isActiveOfficer() const
	{
		if ( m_leadingBrigade && m_leadingBrigade == brigadeID() ) return true;
		return isActiveCommander();
	}

	class SquadList* isAvailableRecruiter(const Player* player) const;

	virtual bool isConnected() const { return teulIndex() >= 0 || isManager(); }

	// Check if we are in GM hide mode and if the viewer has permission to see us
	bool isHiddenFrom(const Player* viewer) const { return ( m_gmHide && !viewer->isGM() ); }
	bool isHiddenFrom(const Player& viewer) const { return ( m_gmHide && !viewer.isGM() ); }

	// Test if the player is tuned to a channel type
	bool isTunedTo(CHANNEL_TYPE chanType) const;

	// See if "viewer" can see me under fairly ordinary constraints
	bool isVisibleTo(const Player* viewer, bool ignoreSide=false) const;
	bool isVisibleTo(const Player& viewer, bool ignoreSide=false) const { return isVisibleTo(&viewer, ignoreSide); }

	// Describe the last position of a player
	const char* lastPosition(const char* prefix, const FLOAT64& x, const FLOAT64& y, bool onlyIfValid) const;

	// Notify* me* if the other player is afk
	void notifyIfAFK(const Player& other) { other.notifyOtherIfAFK(*this); }

	// Check if I am afk, and if so, notify this other person
	void notifyOtherIfAFK(Player& other) const;

	// Return a valid string describing the origin CP of a player
	const char* objectiveCP() const;
	const char* originCP() const;

	// Load this player's group memberships
	void readGroups();
	// Implement the groups returned from an SQL query
	void setGroups(std::vector<UINT32>& groups_);

	void recordEvent(const char* level, const char* index, const char* data1=nullptr, const char* data2=nullptr) const;

	bool recruited(const Player* byPlayer);

	// Extract player from the AWS grid if they're on it
	void removeFromAWS();

	// Remove them from the command list
	void removeFromCommandList(void);

	// Remove them from the chat grid
	void removeFromGrid(void);

	// Send a hostmessage to the player
	SINT32 send(const Chat::HostMessage& message);

	bool approveGCIMessage(UINT32 messageID, const Chat::GCIMessage& message, bool isGlobal);

	void sendSquadCreateInvite(const ChatHandle& handle, const std::string& name, const ChatHandle& from, RANK_NO rank);
	void sendSquadInfo(class SquadList* list=nullptr);
	bool sendSquadInvite(UINT32 squadID, const ChatHandle& from);

	// Send a command to the strat host in the player's context
	bool sendStratCommand(const char* string, const char* arguments, bool useMapTarget=true);

	// Change the AFK state and optionally the message
	void SetAFK(bool to, const char* newMessage=nullptr);

	void SetGMHide(bool to) { m_gmHide = to; }

	void SetHcOff(bool to) { m_hcOff = to; }

	void SetHqOff(bool to) { m_hqOff = to; }

	void SetLeadingBrigade(UINT32 toBrigadeID);

	// Change the personas mission ID
	void SetMission(UINT32 missionID);

	void SetTimer(const char* name, const TeulTimeMS& time);

	// Change the noEnemyChat flag
	void SetNoEnemyChat(bool state) { m_noEnemyChat = state; }

	void SetNoWho(bool state) { m_noWho = state; }

	void SetPreference(UINT32 prefID, UINT32 value, bool fromClient=false);

	void SetSpawned(bool newValue);

	void SetSquad(UINT32 squadID, RANK_NO rank, const ChatHandle& handle, UINT32 joined=0);
	void SetSquadInvite(SquadInvite* invite);

	// Set the current UI Target CP/Facility ID
	void SetUITarget(const FULL_OBJ_ID& to) { m_uiTarget = to; }

	void SetWeapon(const WEAPON_SPEC& weapon);

	void squelchSquadAnnounce() { m_announcedToSquad = true; }

	void stopRecruiting();

	TeulTimeMS timeOnMission() const;

	// Send message if I am not ignoring sender
	SINT32 trySend(Chat::HostMessage& message);

	// Find an open radio slot and tune it to the channel type
	bool tuneChannelType(CHANNEL_TYPE type) { return tuneRadio(getOpenChannelIndex(type), type, 0); }

	void tuneInitialChannels(bool forcibly=false);

	// Tune a radio slot to a placeholder
	void tunePlaceHolder(UCHAR index, CHANNEL_TYPE);

	// Tune a specific radio to the best match for a channel type/handle
	bool tuneRadio(UCHAR index, CHANNEL_TYPE, const ChatHandle& handle);

	// Tune a specific radio to a specified channel
	bool tuneRadioToChannel(UCHAR index, class Chat::Channel* channel, CHANNEL_TYPE type);

	// Tune a specific radio to a channel-handle
	// (determines if a named, numbered or player channel is required)
	bool tuneRadioToHandle(UCHAR index, const ChatHandle& handle);

	// Tune a specific radio to a named channel
	bool tuneRadioToNamedChannel(UCHAR index, const char* command);

	// Tune a specific radio to a "numbered" channel (0-99, not the channel ID)
	bool tuneRadioToNumberedChannel(UCHAR index, UINT32 channelNo);

	// Clear grab fields
	void ungrabbed() { m_grabbing = m_takenBy = 0; m_grabbed = false; }

	void unrecruit(LSTRING_ID notifyRecruiter=LSTR_NONE);

	void updateAWSPosition(bool onAWS);

	// Modify the receive status of a channel
	bool updateChannelStatus(const Chat::UpdateChannel& updateChannel);

	void updateCommandStatus() { if ( isActiveOfficer() || isEffectiveGM() ) addToCommandList(); else removeFromCommandList(); }

	void updateGridPosition();

	// Handle a new locality update
	void updateLocality(FLOAT64 x, FLOAT64 y, FLOAT64 z, bool cpChanged, const FULL_OBJ_ID& nearCP, UINT32 ride, const FULL_OBJ_ID& inFacility, UINT32 lastPing, bool onAWS, bool inCapBldg, const FULL_OBJ_ID& capFac, SINT32 progress);

	void updatePermMask();

	// Apply a persona update to this player
	bool updatePersona(const Chat::PersonaUpdate& personaUpdate);

	// Assert placeholders/channel tunes across radios
	void updateRadios(bool newInstance, bool newSide, bool newBrigade, bool newMission, bool newOrigin, bool newObjective);

	void updateSquadEntry(bool announceEntrance);

	UINT32 validatePreference(UINT32 prefId, UINT32 value) const;

	// Role-reversed version of "isVisibleTo"
	bool visible(const Player* target) const { return target->isVisibleTo(this); }

	// Describe the locality of a player
	void whereabouts(char* into, const FLOAT64& x, const FLOAT64& y) const;

	// CONST Accessors
	bool active() const { return m_active; }
	const Chat::ChannelSubscription& getChannelSub(UINT32 index) const { return m_channels[index]; }
	bool gmHide() const { return m_gmHide; }
	UINT32 goneAFK() const { return m_goneAFK; }
	const ChatHandle& grabbing() const { return m_grabbing; }
	SINT32 gridX() const { return m_gridX; }
	SINT32 gridY() const { return m_gridY; }
	bool haveChannels() const { return m_haveChannels; }
	bool hcOff() const { return m_hcOff; }
	bool hqOff() const { return m_hqOff; }
	const HANDLES& ignores() const { return m_ignores; }
	bool initialized() const { return m_initialized; }
	const Property* insideFacility() const { return m_insideFacility; }
	bool inGrab() const { return m_grabbed; }
	bool isAFK() const { return m_goneAFK != 0; }
	bool isBuzzard() const { return m_isBuzzard; }
	virtual bool isManager() const { return false; }
	bool isSpawned() const { return m_isSpawned; }	///TODO:134: Move to wwiiPlayer, replace with permissions(IS_SPAWNED)
	const TeulTimeMS& joinedMission() const { return m_joinedMission; }
	UINT32 joinedSquad() const { return (squadID() > 0) ? m_joinedSquad : 0; }
	const Property* lastCP() const { return m_lastCP; }
	UINT32 lastMessageID() const { return m_lastMessageID; }
	const TeulTimeMS& lastSquadInfoMS() const { return m_lastSquadInfo_ms; }
	const FLOAT64& lastX() const { return m_lastX; }
	const FLOAT64& lastY() const { return m_lastY; }
	const FLOAT64& lastZ() const { return m_lastZ; }
	UINT32 leadingBrigade() const { return m_leadingBrigade; }
	bool checkTimerActive(const char*) const;			// Check if a timer has been set and is active.
	bool checkTimerElapsed(const char*) const;			// Check if a timer is unset or has elapsed.
	bool noEnemyChat() const { return m_noEnemyChat; }
	bool noWho() const { return m_noWho; }
	const FULL_OBJ_ID& objectiveCpID() const { return m_objectiveCpID; }
	const FULL_OBJ_ID& originCpID() const { return m_originCpID; }
//	const ChatHandle& squadHandle() const { return m_squadHandle; }
	SquadInvite* squadInvite() { return m_squadInvite; }
	const SquadInvite* squadInvite() const { return m_squadInvite; }
	bool rejectLead() const;
	const ChatHandle& takenBy() const { return m_takenBy; }
	const FULL_OBJ_ID& uiTarget() const { return m_uiTarget; }
	const WEAPON_SPEC& weapon() const { return m_weapon; }

	// Non-const accessors
	Chat::ChannelSubscription& getChannelSub(UINT32 index) { return m_channels[index]; }
	HANDLES& ignores() { return m_ignores; }
	HANDLES& friends() { return m_friends; }

public:
	Player(UINT32 playerid, bool isManager=false);
	Player(const PlayerStatusNode* psn, const Chat::PlayerInfo* playerInfo);
	virtual ~Player();

	//// Members
protected:
	ChatHandle		m_takenBy;
	ChatHandle		m_grabbing;
	ChatHandle		m_joinedMission;

//	PermissionMask	m_permissionMask;		// Raw + group permissions
	PermissionMask	m_playerMask;		// The player's own 'raw' permissions
//	PermissionMask	m_chatMask;		// Chat groups permissions

	TeulTimeMS		m_lastSquadInfo_ms;	// When we last sent squad info
public:
	TeulTimeMS		m_lastAWSRequest;

protected:
	// Last known location of the player
	FLOAT64			m_lastX, m_lastY, m_lastZ;
	FULL_OBJ_ID		m_originCpID;		//The CP ID of the CP they spawned from
	FULL_OBJ_ID		m_objectiveCpID;	//The CP ID of the destination of their mission
	FULL_OBJ_ID		m_uiTarget;			// Where the player has targetted for UI ops

	const struct ChatPermissions* m_chatPermissions;

// begin stats logging
	UINT32			m_loginTime;
	UINT32			m_messagesSent;
	UINT32			m_shortestMessage;
	UINT32			m_longestMessage;

	UINT32			m_bytesSent;
// end stats logging

public:
	UINT32			m_lastMessageID;	// Last message ID used
protected:
	WWII_SIDE		m_prevSide;		// Last valid side they had

	UINT32			m_goneAFK;				// Time stamp player went AFK
	UINT32			m_lastAfkNoticeFrom;	// Last player we saw an afk notice from

	UINT32			m_lastGroupRead;	// When we loaded their permissions
	UINT32			m_joinedSquad;			// When we joined our squad

	SINT32			m_gridX, m_gridY;

	Chat::ChannelSubscription	m_channels[MAX_CLIENT_CHANNELS];

private:
	HANDLES			m_friends;
	HANDLES			m_ignores;

private:
	WEAPON_SPEC		m_weapon;			// Player's current weapon

protected:
	class PlayerImpl* m_impl;
	SquadInvite*	m_squadInvite;
	Property*		m_lastCP;
	Property*		m_insideFacility;

public:
	Player*			m_snoopBy;			// Someone watching me
	Player*			m_snoopOn;			// Someone we are watching
	Player*			m_crsSnoopBy;		// CRS watching me
	Player*			m_crsSnoopOn;		// Someone I am snooping on

	struct Tunes*	m_tunes;

	class PropFacility*	m_capping;
	SINT32			m_capProgress;

protected:
	const char*		m_state;

	UINT32			m_leadingBrigade;	// ID of brigade they are leading

	bool			m_initialized	: 1;
	bool			m_hqOff			: 1;
	bool			m_hcOff			: 1;
	bool			m_gmHide		: 1;
	bool			m_gmNote		: 1;
	bool			m_active		: 1;
	bool			m_inFacility	: 1;
	bool			m_feReady		: 1;
	bool			m_rxAllHQ		: 1;
	bool			m_grabbed		: 1;
	bool			m_isSpawned		: 1;	// True when we know he's spawned
	bool			m_onCommandList	: 1;	// True when we are on the command list
	bool			m_noWho			: 1;	// True when they turn "who" off.
	bool			m_noEnemyChat	: 1;	// True disable enemy chat receipt
	bool			m_invalidData	: 1;	// Set to true if they mess with Covert
	bool			m_haveChannels	: 1;	// True once we've initialized channels
	bool			m_announcedToSquad : 1;	// Have they been announced to the squad channel?
	bool			m_isBuzzard 	: 1;	// True when player has reserved/spawned a buzzard

	char			m_afkMessage[MAX_AFK_MESSAGE_LENGTH + 1];

// Private functions
protected:
	void _construct(UINT32 playerID);

private:
	bool _applyPersonaUpdate(const Chat::PersonaUpdate&, bool sideChanged);
	bool _setChannel(UCHAR index, CHANNEL_TYPE type, Chat::Channel* channel, bool rx);

	void _recordLogin();
	void _recordLogout();
};

extern void* teulGetPersonaData(SINT32);

class PlayerInterface
{
public:
	static Player* Lookup(UINT32);
	static Player* Lookup(SINT32 teulIndex) { return (Player*)teulGetPersonaData(teulIndex); }
	static Player* Lookup(const ChatHandle&);
	static Player* Lookup(const char* callsign) { return Lookup(chatGetLowercaseHandle(callsign)); }
};

extern bool chatCheckSpawningPlayers();
extern UINT32 chatGetNumberOfRecruits(UINT32 playerID);
extern bool chatSaveLogRecords();
extern void chatUpdateAWSSnapshot();

#define PLAYER_LOGA(fmt, ...) dbugLog(_FLA, "t:%d p:%u c:%s " fmt, player->teulIndex(), player->playerID(), player->callsign(), __VA_ARGS__)
#define PLAYER_LOGW(fmt, ...) dbugLog(_FLW, "t:%d p:%u c:%s " fmt, player->teulIndex(), player->playerID(), player->callsign(), __VA_ARGS__)
#define PLAYER_LOGI(fmt, ...) dbugLog(_FLI, "t:%d p:%u c:%s " fmt, player->teulIndex(), player->playerID(), player->callsign(), __VA_ARGS__)
#define PLAYER_LOGD(fmt, ...) dbugLog(_FLD, "t:%d p:%u c:%s " fmt, player->teulIndex(), player->playerID(), player->callsign(), __VA_ARGS__)
