#pragma once
// Date: Fri May 21 2004
//
// Data structures used for exchanges between strat and chat.
// Intended as an include to "strtServerTypes.h"
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdexcept>
#include "chat/chatHandle.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"

static const int OOB_STRING_LEN	= 160 ;
static const int OOB_STRING_SIZE = OOB_STRING_LEN + 1 ;
static const int OOB_CALLSIGN_SIZE = MAX_HANDLE_LEN + 1 ;

// Out of band messages for sending config/command packets between hosts
// basically, a reduction in the number of callbacks required.

//tolua_begin
class OOBMessage
{
public:
	OOBMessage(UINT32 playerID, UINT32 brigadeID, const ChatHandle& handleID, const char* command, const char* arguments
				, const FULL_OBJ_ID& cpID, WWII_SIDE side, WWII_COUNTRY country) ;
	OOBMessage() ;
	OOBMessage(ACTION_OBJECT* const a) ;

public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;
	void send() ;

	const char* originator() const { return chatGetCallsign(m_handleID) ; }

	// Used by the host
	void Reply(int prefix, int msg, const char* text1=NULL, const char* text2=NULL) ;

public:
	UINT32			m_originatorID ;	// Player ID of originator (or 0 for system)
	UINT32			m_brigadeID ;		// ID of the brigade player is in or requesting
	ChatHandle		m_handleID ;		// Packed handle of originator
	FULL_OBJ_ID		m_cpID ;			// Origin facility of the GM
	WWII_COUNTRY	m_country ;			// Country of the sender
	WWII_SIDE		m_side ;			// Side - since they might not have a country selected
	char			m_text[OOB_STRING_SIZE] ;
} ;
//tolua_end

// List of known SCMDs
enum
{
	SC_NO_COMMAND					// You didn't set me
,	SC_OVERRUN						// "Overrun" indicates camped situation
,	SC_HOLD							// "Hold at all costs"
,	SC_FALLBACK						// Retreat and open defensive FBs
,	SC_ATTACK						// Attack objective placement
,	SC_DEFEND						// Defensive objective
,	SC_CLEAR						// Clear objective
// And so we can tell how big the list is :)
,	SC_NUM_COMMANDS
} ;
typedef UINT32 SCMD_ENUM ;

// Result codes for strat commands
enum
{
	SCMD_NO_EFFECT					// Caused nothing to happen
,	SCMD_SUCCESS					// Success
,	SCMD_ALLOWED					// Fallback is allowed but wasn't executed
,	SCMD_NOT_OWNER					// Country doesn't own town
,	SCMD_WRONG_SIDE					// Side doesn't own town
,	SCMD_MUST_OAC					// Must own and control
,	SCMD_NO_ENEMY					// No enemy presence
,	SCMD_NOT_OVERRUN				// Town isn't overrun
,	SCMD_NO_ABS						// No abs to fallback from
,	SCMD_NO_LINKS					// No links to fallback over
,	SCMD_NO_CONTENTION				// Town is not contested
,	SCMD_REPEAT						// Flag is already applied
,	SCMD_OTHER_ORDERS				// Already has other orders
,	SCMD_ORDER_LIMIT				// Side has too many orders posted
,	SCMD_NO_OBJECTIVE				// No objective assigned to %s
,	SCMD_REQUEST_OUTSTANDING		// You already have a request at this place
,	SCMD_OBJECTIVE_FRONTLINE		// Objectives require frontline towns
,	SCMD_FRIENDLY_TOWN				// Our town
,	SCMD_CITY_AUTODEFENSE			// You can only defend bridges
,	SCMD_YOUNG_OBJECTIVE			// Objective cannot be expired yet
,	SCMD_STUCK_UNITS				// Units that are trapped
,	SCMD_SOFTCAP_DENIED				// Must post an engagement AO
,	SCMD_NO_BDE_LINK				// Army brigades must be linked
,	SCMD_NO_DIV_LINK				// No link between division and target
// And finally:
,	SCMD_ERROR						// Must always be the last
}  ;
typedef UINT32 SCMD_RESULT ;

enum
{
	FALLBACK_TEST
,	FALLBACK_START
,	FALLBACK_EXECUTE
} ;

// Maximum length of a strat command
static const int		_MAX_SCMD_LEN = 10 ;

// These are commands issued to strat by the chat host, processed
// and then fired back to the chat host with the result variable set.
class StratCommand
{
public:
	UINT32			m_issuer ;				// Issuer's playerID
	SCMD_ENUM		m_command ;				// List of strat commands
	FULL_OBJ_ID		m_cp ;					// Full ObjID of the CP they issued at
	WWII_SIDE		m_side ;				// Which country they belong to
	SCMD_RESULT		m_result ;				// The result they produced
	UINT32			m_data ;				// For argument passing
	UINT32			m_brigadeID ;

	StratCommand(UINT32 issuer, SCMD_ENUM command, FULL_OBJ_ID cp, WWII_SIDE side, SCMD_RESULT result=SCMD_NO_EFFECT) ;
	StratCommand(ACTION_OBJECT* const a) ;

	// Code that actually handles the SCMD result;
	// eventually this should exist on both sides of the connection
	void process(void) ;

	~StratCommand() { }

	SINT32 send() ;

	SINT32 pack(LOGICAL mode, ACTION_OBJECT* a) ;

	bool describe() const ;
} ;

typedef std::pair<UINT32,UINT32> POPULATION ;

