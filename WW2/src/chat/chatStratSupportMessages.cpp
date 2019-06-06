#include <syscfg.h> // Path is project specific.
// Date: Thu Oct  4 14:15:07 2001
//
// Functions the strat system uses to talk to the chat server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatBrigadeMove.h"
#include "chat/chatConstants.h"
#include "chat/chatName.h"
#include "chat/chatSideMessage.h"
#include "chat/chatStratSupportMessages.h"
#include "chat/chatTypes.h"
//#include "cnvy/cnvyMain.h"
#include "server/serverServerInfo.h"
#include "strt/strtCP.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtDotCommands.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHCUnits.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtTypes.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHCUnits.h"

static TEUL_CALLBACK* _SEND_SCMD_RESULT;
static TEUL_CALLBACK* _SEND_SIDE_MESSAGE;

UINT32 g_chatPopulations[MAX_SIDES];

void _chatSendError(SINT32 teulIndex, UCHAR* const /*data*/, SINT32 /*size*/, TEUL_CALLBACK* /*object*/, SINT32 error)
{
	dbugLog(_FLA,"Something wrong in sending chat message: %d error on %d index",error,teulIndex);
}

void _chatRecvError(SINT32 teulIndex, UCHAR* const /*data*/, SINT32 /*size*/, TEUL_CALLBACK* /*object*/, SINT32 error)
{
	dbugLog(_FLA, "recv error %d on connection %d", error, teulIndex);
}

static SINT32
_sendSCMDResult(ACTION_OBJECT* const a)
{
	return ((StratCommand*)a->localRawData)
						-> pack(TEUL_PACK, a);
}

static SINT32
_recvOOBMessage(ACTION_OBJECT* const a)
{
	OOBMessage message(a);

	strtProcessOOBMessage(message);

	return a->offset;
}

static SINT32
_sendSideMessage(ACTION_OBJECT* const a)
{
	Chat::SideMessage* msg = (Chat::SideMessage*)a->localRawData;
	return msg->packer(TEUL_PACK, a);
}

UINT32
chatGetPopulation(WWII_SIDE side)
{
	if ( side >= MAX_SIDES )
		return 0;

	return g_chatPopulations[side];
}

//////////////////////////////////////////////////////////////////////
// Send a side-specific message which can further be limited to a
// specific objective (send nullObjID if you don't want to limit it)
// If prefix is LSTR_NONE then the appropriate of LSTR_ALLIED or
// LSTR_AXIS will be used instead.
//

static bool
_sideMessage(SIDE_MESSAGE_TYPE type
				, WWII_SIDE side
				, const FULL_OBJ_ID objective
				, LSTRING_ID prefix
				, LSTRING_ID message
				, const char* arg1
				, const char* arg2
				)
{
	static Chat::SideMessage packet;

	if ( prefix == LSTR_NONE )
	{
		if ( side == ALLIED ) prefix = LSTR_ALLIED;
		else if ( side == AXIS ) prefix = LSTR_AXIS;
	}

	packet.m_type = type;
	packet.m_side = side;
	packet.m_prefix = prefix;
	packet.m_message = message;
	packet.m_objectiveID = objective;

	packet.m_arg1 = (arg1) ? arg1 : "";
	packet.m_arg2 = (arg2) ? arg2 : "";

	return serverSendCallbackToType(CHAT_HOST, _SEND_SIDE_MESSAGE, &packet);
}

//////////////////////////////////////////////////////////////////////
// Send a message to one side, possibly limited to one CP

bool
chatSendSideMessage(WWII_SIDE side, const FULL_OBJ_ID objective, LSTRING_ID prefix, LSTRING_ID message, const char* arg1, const char* arg2)
{
	return _sideMessage(SM_SIDE, side, objective, prefix, message, arg1, arg2);
}

//////////////////////////////////////////////////////////////////////
// Send a message specifically to one HC

bool
chatSendHCMessage(WWII_SIDE side, LSTRING_ID message, const char* arg1, const char* arg2)
{
	return _sideMessage(SM_HC, side, nullObjID, LSTR_NONE, message, arg1, arg2);
}

//////////////////////////////////////////////////////////////////////

SINT32
StratCommand::send()
{
	if ( serverSendCallbackToType(CHAT_HOST, _SEND_SCMD_RESULT, this) )
		return 0;
	return -EINVAL;
}

bool
chatSendSCMDResult(SINT32 playerID
					, SCMD_ENUM command
					, FULL_OBJ_ID cp
                    , WWII_SIDE side
					, SCMD_RESULT result
					, UINT32 data
					)
{
	try
	{
		StratCommand scmd(playerID
							, command
							, cp
							, side
							, result
							);
		scmd.m_data = data;
		if ( scmd.send() < 0 )
			throw std::logic_error("Send error");

		return true;
	}
	catch (std::exception &e)
	{
		dbugLog(_FLA, "Unable to dispatch scmd result: %s", e.what());
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Used by the stratd host
//////////////////////////////////////////////////////////////////////
void
chatSendBrigadeMoveResult(BRIGADE_MOVE_MSG& outgoing)
{
	ENDPOINT* endpoint = serverPeerEndpointByType(CHAT_HOST);
	if ( endpoint != nullptr )
		outgoing.send(*endpoint);
	if ( outgoing.data().m_action == BMR_APPROVED && outgoing.data().m_actionable )
	{
		endpoint = serverPeerEndpointByType(MAP_HOST);
		if ( endpoint != nullptr )
			outgoing.send(*endpoint);
	}
}

//////////////////////////////////////////////////////////////////////
// Used by the stratd host 
//////////////////////////////////////////////////////////////////////
void stratSendGarrisonCountryChange(GARRISON_COUNTRY_CHANGE_MSG & outgoing)
{
	ENDPOINT* endpoint = serverPeerEndpointByType(CHAT_HOST);
	if (endpoint != nullptr)
		outgoing.send(*endpoint);

	endpoint = serverPeerEndpointByType(MAP_HOST);
	if (endpoint != nullptr)
		outgoing.send(*endpoint);

}

//////////////////////////////////////////////////////////////////////

template<>
SINT32
BRIGADE_MOVE_MSG::receive(SINT32 /*teulIndex*/, BRIGADE_MOVE_MSG& incoming)
{
	dbugLog(_FLD, "Received BRIGADE_MOVE_MSG");
	
	BrigadeMove& request = incoming.data();
	
	if ( incoming.action() != BRIGADE_MOVE_MSG::EXCHANGE || request.m_action != BMR_APPROVED )
	{
		dbugLog(_FLW, "%s: Invalid message type", __FUNCTION__);
		return -EINVAL;
	}

	// The incoming key is the packed* display* handle of the requester
	Chat::Name requester(incoming.key());

	StratHCUnit* unit = StratHCUnit::Lookup(request.m_brigadeID);
	UINT32 origin = (unit && unit->homeCP()) ? unit->homeCP()->uOID : 0;
	CPStruct* cp = strtGetCPFromTarget(request.m_destination);
	UINT32 destination = (cp) ? cp->uOID : 0;
	const char* unitName = (unit ? unit->callsign() : "NULL");
	char fidBuffer[FULL_OBJ_ID::PrintLen];
	const char* cpName = (cp ? cp->name : weapPrintFullObjIDC(request.m_destination, fidBuffer));

	dbugLog(_FLI, "%s: %s requesting %s %s(%u) to %s"
					, __FUNCTION__, requester.c_str(), (request.m_actionable ? "move" : "check")
					, unitName, request.m_brigadeID
					, cpName
					);

	request.m_action = request.process(incoming.key(), requester.c_str(), unit, cp, (bool)incoming.flags());
	
	DBThread::Do(DBThread::Default
				, "INSERT INTO `wwiionline`.`move_requests`"
					"(recorded, requester, action, unit_id, from_cp_oid, to_cp_oid, result)"
					"VALUES"
					"(NOW(), '%s', %u, %u, %u, %u, %d)"
					, requester.c_str(), request.m_actionable, request.m_brigadeID
					, origin, destination
					, request.m_action
					);

	if ( !request.m_actionable || request.m_action != BMR_APPROVED )
		chatSendBrigadeMoveResult(incoming);

	return 1;
}

BRIGADE_MOVE_RESULT
BrigadeMove::process(const ChatHandle mover, const char* callsign, StratHCUnit* unit, CPStruct* cp, bool isGM)
{
	if ( unit == nullptr )
	{
		dbugLog(_FLW, "%s: no such brigade: %u", callsign, m_brigadeID);
		return BMR_FAILED;
	}
	if ( cp == nullptr )
	{
		dbugLog(_FLW, "%s/%s: invalid destination: " PRF_FULL_OBJ_ID "", callsign, unit->callsign()
						, PRA_FULL_OBJ_ID(m_destination)
						);
		return BMR_FAILED_TARGET;
	}

	m_delay = unit->nextMoveIn();
	if ( m_delay > 0 )
	{
		dbugLog(_FLI, "%s/%s->%s: cannot move for another %us", callsign, unit->callsign(), cp->name, m_delay);
		return BMR_FAILED_TIME;
	}

	// If actionable is set, then we actually want to move the brigade
	if (m_actionable)
		return unit->move(callsign, cp);

	// Otherwise, the caller just wants to know if its allowed
	BRIGADE_MOVE_RESULT bmr = unit->deployPermission(cp, true);
	dbugLog(_FLI, "%s/%s->%s: deploy permission = %u", callsign, unit->callsign(), cp->name, bmr);

	return bmr;
}


// Used by Strat host
template<>
SINT32
GARRISON_COUNTRY_CHANGE_MSG::receive(SINT32 /*teulIndex*/, GARRISON_COUNTRY_CHANGE_MSG& incoming)
{
	dbugLog(_FLD, "Received GARRISON_COUNTRY_CHANGE_MSG");

	GarrisonCountryChange& request = incoming.data();
	
	if ( incoming.action() != GARRISON_COUNTRY_CHANGE_MSG::EXCHANGE || request.m_action != GCR_APPROVED )
	{
		dbugLog(_FLW, "%s: Invalid message type", __FUNCTION__);
		return -EINVAL;
	}

	// The incoming key is the packed* display* handle of the requester
	Chat::Name requester(incoming.key());

	StratHCUnit* unit = StratHCUnit::Lookup(request.m_brigadeID);

	const char* unitName = (unit ? unit->callsign() : "NULL");
	const char *countryName = _countryCode[request.m_newCountryId];
	dbugLog(_FLI, "%s: %s requesting changing country of %s(%u) to %s"
					, __FUNCTION__, requester.c_str() 
					, unitName, request.m_brigadeID
					, countryName
					);

	request.m_action = request.process(incoming.key(), requester.c_str(), incoming.data().m_brigadeID, incoming.data().m_newCountryId);

	// Add an entry to the wwiionline.move_requests table so that Manually Changed Garrisons can be reviewed/monitored
	DBThread::Do(DBThread::Default
				, "INSERT INTO `wwiionline`.`move_requests`"
					"(recorded, requester, action, unit_id, from_country_id, to_country_id, result)"
					"VALUES"
					"(NOW(), '%s', %u, %u, %u, %u, %d)"
					, requester.c_str(), request.m_actionable, request.m_brigadeID
					, unit->country(), request.m_newCountryId
					, request.m_action
					);
					
	if ( !request.m_actionable || request.m_action != GCR_APPROVED )
	{
		dbugLog(_FLD, "Calling stratSendGarrisonCountryChange");
		stratSendGarrisonCountryChange(incoming);
	}
	else
	{
		dbugLog(_FLD, "Not actionable or not approved", request.m_actionable, request.m_action);
	}
	
	return 1;
}

// Used by Strat host
GARRISON_COUNTRY_CHANGE_RESULT
GarrisonCountryChange::process(const ChatHandle mover, const char* callsign, UINT32 unitID, WWII_COUNTRY country)
{
	dbugLog(_FLD, "GarrisonCountryChange::process() was called from %s for unit %d and country %d where action=%d",
		callsign, unitID, country, m_action);

	GARRISON_COUNTRY_CHANGE_RESULT result = GCR_FAILED;

	// TODO: here we are to actually perform the strat hosts function of verifying and then
	//       changing the country when "actionable"
	StratHCUnit *unit = StratHCUnit::Lookup(unitID);
	CPStruct *cp = strtGetCP(unit->m_homeCPID);
	if (cp == nullptr)
	{
		dbugLog(_FLI, "GarrisonCountryChangeReq(" PRF_FULL_OBJ_ID ") invalid target", PRA_FULL_OBJ_ID(unit->m_homeCPID));
	}
	else
	{
		if (cp->isObjective() || cp->islinkedToAnAO())
		{
			result = GCR_FAILED_HAS_OBJECTIVE;
            dbugLog(_FLI, "GarrisonCountryChangeReq failed because the CP has an Objective or is linked to one.");
		}
		else if (wwiiSide(country) != wwiiSide(cp->country))
		{
			result = GCR_FAILED_WRONG_OWNER;
            dbugLog(_FLI, "GarrisonCountryChangeReq failed because the CP is owned by the other side.");
		}
        else if (country == cp->country)
        {
            result = GCR_FAILED_OWNER_ALREADY;
            dbugLog(_FLI, "GarrisonCountryChangeReq failed because the CP is owned by the requested country already.");
        }
        else
        {
	        // TODO Add some checks here for percentages of UK, French and US towns already on the front line.

            if (cp->canChangeOwnership(country, true))
            {
				if (m_actionable)
				{
					dbugLog(_FLI, "CP Change country request (vote) from %s to change CP %u to country %u ", callsign,
							unit->m_homeCPID.id, country);

					cp->changeOwnership(country, true, true);

					// Now that the town has changed owner we need to change the Garrison to match.
					cp->changeAllGarrisonCountries(country, true);
				}

                result = GCR_APPROVED;
                dbugLog(_FLI, "Approved GarrisonCountryChangeReq received from %s to change CP %u to country %u ", callsign, unit->m_homeCPID.id, country);
            }
            else
            {
                dbugLog(_FLI, "GarrisonCountryChangeReq cp->changeOwnership failed for %s whilst changing from country %u to %u",
                        cp->name, cp->country, country);
                result = GCR_FAILED;
            }
        }
	}

	return result;
}

//////////////////////////////////////////////////////////////////////
//This function installs all the host callbacks
SINT32
chatInstallStratSupportCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(CHAT_X)
	{
		REGISTER_SENDER(Chat::SIDE_MESSAGE_Y, _sendSideMessage, _SEND_SIDE_MESSAGE);

		REGISTER_RECEIVER(Chat::OOB_MESSAGE_Y, _recvOOBMessage);

		REGISTER_SENDER(Chat::EXCHANGE_SCMD_Y, _sendSCMDResult, _SEND_SCMD_RESULT);

		REGISTER_MSG(Chat::BRIGADE_MOVE_Y, BRIGADE_MOVE_MSG);

		REGISTER_MSG(Chat::GARRISON_COUNTRY_Y, GARRISON_COUNTRY_CHANGE_MSG);
	}
	END_CALLBACK_REGISTRATION(CHAT_X)
}
