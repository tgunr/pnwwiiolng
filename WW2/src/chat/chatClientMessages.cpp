#include <syscfg.h> // Path is project specific.
// Date: Tue Aug  1 16:06:48 2000
//
// Client Side API for chat communications
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatConstants.h"
#include "wwii/wwiiCallbacks.h"
#include "chat/chatTypes.h"
#include "wwii/wwiiPack.h"
#include "chat/chatClientMessages.h"
#include "chat/chatHandle.h"
#include "client/client.h"
#include "chat/chatWhoList.h"
#include "chat/chatWaypointExchg.h"
#include "chat/chatMissionMemberUpdates.h"
#include "wwii/wwiiSquadMsg.h"
#include "chat/chatSquadControl.h"
#include "chat/chatMotds.h"
#include "chat/chatEditStringMsg.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "chat/chatSTOJoules.h"
#include "chat/chatBrigadeControl.h"
#include "chat/chatBrigadeControlList.h"
#include "chat/chatGroupPermissionTypes.h"
#include "chat/chatRecruiterList.h"
#include "chat/chatMissionInviteMsg.h"
#include "chat/chatPreference.h"
#include "chat/chatClientMessage.h"
#include "chat/chatGCIMessage.h"
#include "chat/chatHostMessage.h"
#include "chat/chatLocality.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatTuneToChannel.h"
#include "chat/chatServerInfo.h"
#include "chat/chatUpdateChannel.h"
#include "chat/chatUserNLA.h"
#include "chat/chatHoldFallVote.h"
#include "chat/chatTimeInterval.h"
#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include "omg/omgChat.hxx"
#include "sdm/StratDB.h"
#include "chat/chatDotCommandList.h"
#include "client/ww2client.h"
#include "teul/teulPacket.h"
#include "client/GameClient.h"

///TEMPORARY
#include "map/mapAWS.h"
#include "uimap/AWSManager.h"
#include "tut/TutorialManager.h"

const ChatPermissions nullChatPermissions;

static	TEUL_CALLBACK* _SEND_CLIENT_MESSAGE;
static	TEUL_CALLBACK* _SEND_UPDATE_CHANNEL;
static	TEUL_CALLBACK* _FE_TUNE_TO_CHANNEL;
static	TEUL_CALLBACK* _SERVER_TRACKED_OBJECTS;

static	TEUL_CALLBACK* _MOTD_REQ;
static	TEUL_CALLBACK* _SEND_LOCALITY;
static	TEUL_CALLBACK* _SEND_RECRUITER_REQUEST;

TEUL_CALLBACK* HoldFallVote::s_callback;

static FULL_OBJ_ID _lastTargetFullObjID = {{0,0},0,0,0};

TEUL_CALLBACK* CHAT_SQUAD_CONTROL;

Chat::HostCommands Chat::g_hostDotCommands;

//////////////////////////////////////////////////////////////////////

SINT32 chatSendMessage(SINT32 teulIndex, UINT16 radioIndex, const char* message)
{
	Chat::ClientMessage outGoingMessage;

	//Setup the outgoing message
	outGoingMessage.m_radios = radioIndex;		//set to the radio Index
												//	of the transmitting Index
	outGoingMessage.m_handle = 0;
	outGoingMessage.m_country = 0;

	outGoingMessage.m_length = strlen(message);

	if ( outGoingMessage.m_length >= MAX_MESSAGE_LENGTH )
		outGoingMessage.m_length = MAX_MESSAGE_LENGTH - 1;

	strncpy((char*)outGoingMessage.m_message, message, outGoingMessage.m_length + 1);

	// Let chat know what you targetted last for use with dot commands
	outGoingMessage.m_uiTarget = _lastTargetFullObjID;

	return teulSend(teulIndex, _SEND_CLIENT_MESSAGE, &outGoingMessage);
}

//////////////////////////////////////////////////
// chatTargetSelection(<id>)
// BRIEF: Notify chat of a players map-selection.
// DETAIL:
//  Tracks a player's "last selection" from the map;
//  so that when they submit a dot-command, their
//  selection can be sent along with it. This
//  allows for host-side dot commands which have a
//  higher degree of integration with the UI map.
//////////////////////////////////////////////////
void chatTargetSelection(const FULL_OBJ_ID id)
{
	_lastTargetFullObjID = id;
}

FULL_OBJ_ID chatGetTargetSelection()
{
	return _lastTargetFullObjID;
}

SINT32 chatSendModeNotice(SINT32 teulIndex, UCHAR updateMode)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::PREFERENCE_Y));
	outgoing.construct<Chat::Preference>(CPR_UPDATE_MODE, updateMode);
	return outgoing.send(teulIndex);
}

SINT32 chatSendRecruiterRequest(SINT32 teulIndex, UCHAR language)
{
	return teulSend(teulIndex, _SEND_RECRUITER_REQUEST, &language);
}

//This function tunes the client's radio Index to the channel indicated by
//	channelID
SINT32 chatTuneRadioToChannel(SINT32 teulIndex, UCHAR radioIndex, UINT32 type)
{
	Chat::TuneToChannel tune;

	tune.m_radioIndex = radioIndex;
	tune.m_handle = 0;
	tune.m_type = (CHANNEL_TYPE)type;
	tune.m_rx = true;
	tune.m_tx = true;

	TUT_MGR->DoEvent(TUT_EVENT_CHANNEL, &radioIndex, &type);

	return teulSend(teulIndex, _FE_TUNE_TO_CHANNEL, &tune);
}

//This function tunes the client's radio channel referenced by radioIndex to
//	the handle provided
SINT32 chatTuneRadioToHandle(SINT32 teulIndex, UCHAR radioIndex, ChatHandle handle)
{
	Chat::TuneToChannel tune;

	tune.m_radioIndex = radioIndex;
	tune.m_handle = handle;
	tune.m_type = PRIVATE_CHANNEL;
	tune.m_rx = TRUE;
	tune.m_tx = TRUE;

	TUT_MGR->DoEvent(TUT_EVENT_CHANNEL, &radioIndex, &tune.m_type);

	return teulSend(teulIndex, _FE_TUNE_TO_CHANNEL, &tune);
}

//This function tells the server to toggle receiving for the radio channel
//	indicated by radioIndex either on or off.  If the receiveStatus is 0, then
//	receiving is off, if it is > 0, then receiving is on
SINT32 chatUpdateChannelStatus(SINT32 teulIndex, UCHAR radioIndex, bool receiveStatus)
{
	Chat::UpdateChannel updateChannel(radioIndex, receiveStatus);

	return teulSend(teulIndex, _SEND_UPDATE_CHANNEL, &updateChannel);
}

//This function sends a message to the server to be distributed on the
//	channel referenced by radioIndex

SINT32 chatSendMotdRequest(SINT32 teulIndex, UINT32 id)
{
	MOTD_REQUEST req(id);
	return teulSend(teulIndex, _MOTD_REQ, &req);
}

// FIXME - remove this when time is right!
#if platBuild == BUILD_CLIENT
LOGICAL chatSendLocality(SINT32 teulIndex, UINT32 vehicleHostIndex, const FLOAT64& rawX, const FLOAT64& rawY, const FLOAT64& rawZ, FULL_OBJ_ID nearestCP)
{
	static SINT32 lastTeulIndex = -1;
	static UINT32 lastHostIndex = ~0;
	static FLOAT64 lastX = -(~0), lastY = -(~0), lastZ = -(~0);
	static FULL_OBJ_ID lastCP = nullObjID;

	if ( teulIndex != lastTeulIndex || vehicleHostIndex != lastHostIndex )
	{
		lastX = lastY = lastZ = 0xffffffff;
		memset(&lastCP, 0xfa, sizeof(FULL_OBJ_ID));
	}
	else if ( (SINT32)rawX == (SINT32)lastX
			&& (SINT32)rawY == (SINT32)lastY
			&& (SINT32)rawZ == (SINT32)lastZ
			&& memcmp(&lastCP, &nearestCP, sizeof(FULL_OBJ_ID)) == 0 )
		return FALSE;

	//// Oliver's call to send data up to the host
	Chat::Locality update(rawX, rawY, rawZ, nearestCP);
	teulSend(teulIndex, _SEND_LOCALITY, &update);

	// We sent the update, now store the values
	lastTeulIndex = teulIndex;
	lastHostIndex = vehicleHostIndex;
	lastX = rawX;
	lastY = rawY;
	lastZ = rawZ;

	memcpy(&lastCP, &nearestCP, sizeof(FULL_OBJ_ID));

	return TRUE;
}
#endif

bool
AWS::Request::send(SINT32 chatConnection)
{
	return ( teulSend(chatConnection, AWS::callback, this) >= 0 );
}

//The actual send function for Messages
static SINT32 _sendMessage(ACTION_OBJECT* const a)
{
	Chat::ClientMessage* message = (Chat::ClientMessage*)(a->localRawData);
	return message->packer(TEUL_PACK, a);
}

//The send for the tune to Channel
static SINT32 _sendTuneToChannel(ACTION_OBJECT* const a)
{
	Chat::TuneToChannel* tune = (Chat::TuneToChannel*)(a->localRawData);
	return tune->packer(TEUL_PACK, a);
}

//The send for the tune to Channel
static SINT32 _sendUpdateChannel(ACTION_OBJECT* const a)
{
	Chat::UpdateChannel* update = (Chat::UpdateChannel*)(a->localRawData);
	return update->packer(TEUL_PACK, a);
}

static SINT32 _sendMOTD_REQ(ACTION_OBJECT* const a)
{
	MOTD_REQUEST* request = (MOTD_REQUEST*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32 _sendLocality(ACTION_OBJECT* const a)
{
	return ((Chat::Locality*)a->localRawData)->packer(TEUL_PACK, a);
}

static SINT32 _sendSquadControl(ACTION_OBJECT* const a)
{
	return ((SquadControl*)a->localRawData)->packer(TEUL_PACK, a);
}

static SINT32 _sendServerTrackedObject(ACTION_OBJECT* const a)
{
	ServerTrackedObject* sto = (ServerTrackedObject*)(a->localRawData);
	return sto->packer(TEUL_PACK, a);
}

static SINT32 _sendBrigadeControl(ACTION_OBJECT* const a)
{
	BrigadeControlRequest* bcr = (BrigadeControlRequest*)(a->localRawData);
	return bcr->packer(TEUL_PACK, a);
}

static SINT32 _sendRecruiterRequest(ACTION_OBJECT* const a)
{
	return teulPackUCHAR(TEUL_PACK, a->packed, a->offset, (UCHAR*)a->localRawData, 0);
}

static SINT32 _sendAWS(ACTION_OBJECT* const a)
{
	AWS::Request* request = (AWS::Request*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32
_recvMOTD_Reply(ACTION_OBJECT* const a)
{
	clientChatProcessMotdReply(a->teulIndex, MOTD_REPLY(a));
	return a->offset;
}

static SINT32
_recvChatMissionStatus(ACTION_OBJECT* const a)
{
    const TeulTimeMS ts = 0ULL;
    static Chat::MissionStatus receiver;        // must either be allocated or static as later on game client will point to this for future reference, static for now since we are
                                                // already riddled with statics, what is one more.
    receiver.unpack(a, ts);

	clientProcessChatMissionStatus(a->teulIndex, &receiver);

	return a->offset;
}

//The actual recv function for Messages
static SINT32
_recvMessage(ACTION_OBJECT* const a)
{
	Chat::HostMessage message;

	// HostMessage structure uses a pointer (to save the host having
	// to copy data around when forwarding messages)
	static SCHAR msg[MAX_MESSAGE_LENGTH+1];
	message.msg = msg;

	message.packer(TEUL_UNPACK, a);

	clientChatProcessHostMessage(a->teulIndex, &message);

	return a->offset;
}

static SINT32
_recvTuneToChannel(ACTION_OBJECT* const a)
{
	Chat::TuneToChannel tune;
	a->offset = tune.packer(TEUL_UNPACK, a);

	SCHAR name[MAX_HANDLE_SIZE];
	chatGetCallsign(name, tune.m_handle);

	dbugLog(_FLD, "t:%d tuned rid %d to %d [%s] rx %d tx %d"
					, a->teulIndex
					, tune.m_radioIndex
					, tune.m_type
					, name
					, tune.m_rx
					, tune.m_tx
					);

	///////////////////////////////////////////////////
	//Set radio rx,tx etc for tuneToChannel.radioIndex
	///////////////////////////////////////////////////

	clientChatTuneToChannel(a->teulIndex, &tune);

	return a->offset;
}

#ifdef SOMEDAY
static SINT32 _recvUpdateChannel(ACTION_OBJECT* const a)
{
	Chat::UpdateChannel update;
	update.packer(TEUL_UNPACK, a);

	dbugLog(_FLD, "received update channel from %d, radioIndex: %u, status: %d"
					, a->teulIndex, update.radioIndex, update.receiveStatus
					);

	return a->offset;
}
#endif

//The receive for the tuneToChannelAck
static SINT32 _recvUserNLA(ACTION_OBJECT* const a)
{
	Chat::UserNLA nla;
	nla.packer(TEUL_UNPACK, a);

	clientChatReceiveNLA(a->teulIndex, &nla);

	return a->offset;
}

static SINT32 _recvServerInfo(ACTION_OBJECT* const a)
{
	ChatServerInfo serverInfo;
	a->offset = serverInfo.packer(TEUL_UNPACK, a);

	clientChatProcessServerInfo(a->teulIndex, &serverInfo);

	return a->offset;
}

#ifdef CHAT_CLIENT
void rosterMessage(LSTRING_ID, LSTRING_ID, const char*, const char*);
#endif

static SINT32 _recvLocalisedMessage(ACTION_OBJECT* const a)
{
	Chat::LocalizedMessage lmsg;
	lmsg.packer(TEUL_UNPACK, a);

	clientChatProcessLocalizedMessage(a->teulIndex, &lmsg);

	return a->offset;
}

static SINT32 _recvGCIMessage(ACTION_OBJECT* const a)
{
	if ( PN_UNLIKELY(a->canRead<Chat::GCIMessage>() == false) )
		return -EINVAL;
	Chat::GCIMessage& gci = a->read<Chat::GCIMessage>();
	clientChatProcessGCIMessage(a->teulIndex, &gci);

	return a->offset;
}

static SINT32 _recvSquadControl(ACTION_OBJECT* const a)
{
	SquadControlResult incoming(a);

	if ( a->offset >= 0 )
		incoming.receive(a->teulIndex);

	return a->offset;
}

static SINT32 _recvBrigadeControl(ACTION_OBJECT* const a)
{
	BrigadeControlNotice incoming(a);

	incoming.processNotice();

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receiver for Brigade Control List packets.

static SINT32
_recvBrigadeControlList(ACTION_OBJECT* const a)
{
	if ( a->sizeInBytes() < sizeof(UCHAR) + sizeof(UCHAR) + sizeof(PermissionMask) )
		return -EINVAL;

	const UCHAR& forCountry = a->read<UCHAR>();
	/*const UCHAR& forBranch = */a->read<UCHAR>();
	const PermissionMask& permissions = a->read<PermissionMask>();

	const WWII_SIDE forSide = wwiiSide((WWII_COUNTRY)forCountry);

	const bool alliedHC = (forSide == ALLIED) && (permissions & ALLIED_COMMAND_STAFF);
	const bool axisHC = (forSide == AXIS) && (permissions & AXIS_COMMAND_STAFF);

	const bool isHC = alliedHC || axisHC;

	// If this persona is an HC officer, the "owner()" flag
	// of each brigade control list wrapper is set to TRUE.
	PlayerClient()->SetHC(isHC);

	while ( a->canRead<BrigadeControlList>() )
	{
		const BrigadeControlList& entry = a->read<BrigadeControlList>();
		PlayerClient()->ProcessBrigadeControl(entry);
	}

	return 0;
}

static SINT32 _recvAWS(ACTION_OBJECT* const a)
{
	// Construct a grid object from the incoming packet
	AWS::Grid grid(a);

	AWS::Row row(grid.m_row, true, grid.m_startCol);

	UINT32 i = 0;
	for ( row.m_row = grid.m_row; row.m_row < grid.m_row + AWS::MAX_COLUMNS; ++row.m_row, ++i )
	{
		if ( grid.m_hasData == false || grid.m_rowHasData[i] == false )
		{
			row.m_hasData = false;
			row.m_data = nullptr;
		}
		else
		{
			row.m_hasData = true;
			row.m_data = grid.m_data[i];
		}

		// only gets called if rows were sent
		AWSManager::GetInstance()->ReceiveRow(row);
	}

	// ensures the response pending flag is cleared
	// even if no rows of data were returned
	AWSManager::GetInstance()->ReceiveResponse();

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receives host response on an edit string change
template<>
SINT32 EDIT_STRING_MSG::receive(SINT32 teulIndex, EDIT_STRING_MSG& incoming)
{
	if ( PN_UNLIKELY(incoming.action() != EXCHANGE) )
		return -EINVAL;

	return incoming.data().receive(teulIndex);
}

// Not strictly neccessary, but it savings putting EditString:: infront of everything
SINT32 EditString::receive(SINT32 teulIndex)
{
	switch (error())
	{
	case _NO_ERROR:				// Nothing happened
	break;
	case _ACCEPTED:				// Change went thru
	break;
	case _INVALID:				// Host haits j00
	break;
	case _TOO_LONG:				// Quit typing the bible
	break;
	case _PERMISSION_DENIED:	// WTF do you think you're doing?
	break;
	default:
		Write_Debug("invalid esm response");
	break;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a server tracked object packet

SINT32
_recvServerTrackedObjects(ACTION_OBJECT* const a)
{
#if platBuild == BUILD_CLIENT
	// How many STOs are there?
	UINT32 count = 0;
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &count, 8);
	if ( PN_UNLIKELY(count <= 0) )
	{
		Write_Log(LOG_COMM, LOG_WARN, "Invalid STO count");
		return -EINVAL;
	}

	UINT16 type = 0;

	for ( UINT32 i = 0; i < count; ++i )
	{
		// Peak-ahead and get the type; DO NOT MODIFY a->offset
		wwiiPackServerTrackedObjectType(TEUL_UNPACK, a->packed, a->offset, &type, 0);
		if ( !clientReceiveServerTrackedObject(a->teulIndex, type, a) )
		{
			Write_Log(LOG_COMM, LOG_WARN, "Unsupported STO type %d", type);
			break;
		}
	}
#endif

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a packet with information about a hold/fallback vote.

static SINT32
_recvHoldFallInfo(ACTION_OBJECT* const a)
{
	// Tear up an object with the data.
	HoldFallVote incoming(a);

	const char* actionName = "chat.holdfall.tied";
	if ( incoming.m_status != HoldFallVote::_TIED )
	{
		if ( incoming.m_holds >= incoming.m_score )
			actionName = "chat.holdfall.hold";
		else if ( incoming.m_falls >= incoming.m_score )
			actionName = "chat.holdfall.fall";
		else if ( incoming.m_vetos >= incoming.m_score )
			actionName = "chat.holdfall.delay";
	}
	const std::wstring& action = uiLocString(actionName);

	std::wstring eventId = L"unknown";
	//wchar_t eventId[MAX_HANDLE_WSIZE]; // aw - was MAX_HANDLE_WLEN
	wchar_t requester[MAX_HANDLE_WSIZE]; // aw - was MAX_HANDLE_WLEN

	// use fullobjid to find cp name and use this for eventid
	StratDB::StratObject* strat = StratDB::FindStratObject(incoming.m_fullObjID);

	if ( strat )
		eventId = strat->DisplayName(); //swprintf(eventId, MAX_HANDLE_WSIZE-1, L"%ls", strat->DisplayName());

	chatGetCallsign(requester, incoming.m_requester);

	wchar_t numHolds[16];
	wchar_t numFalls[16];
	wchar_t numDelays[16];
	swprintf(numHolds, 15, L"%u", incoming.m_holds);
	swprintf(numFalls, 15, L"%u", incoming.m_falls);
	swprintf(numDelays, 15, L"%u", incoming.m_vetos);


	unsigned int color = colorCYANALPHA;	// Default color
	int icons[CHATICON_MAX] = {-1,-1,IDE_ICON_HC};

	// Decide which format to describe to the player
	switch ( incoming.m_status )
	{
	case HoldFallVote::_VOTED:
		// HoldFallVote hfv(decision, HoldFallVote::_VOTED); // "[{id}] {callsign} requests '{order}' (H:%u/F:%u/D:%u)"
		uiChatPrintUILocLine(icons, color, "chat.holdfall.req"
							, eventId.c_str(), requester, action.c_str()
							, numHolds, numFalls, numDelays);
		break;
	case HoldFallVote::_WAITING:
		// HoldFallVote hfv(this, HoldFallVote::_WAITING);	// "[%s] reporting overrun, awaiting orders"
		uiChatPrintUILocLine(icons, color, "chat.holdfall.report", eventId.c_str());
		break;
	case HoldFallVote::_TIED:
	case HoldFallVote::_STANDS:
		// HoldFallVote hfv(this, HoldFallVote::_STANDS); 		// "[%s] Order stands as '%s'. %s to execution."
		uiChatPrintUILocLine(icons, color, "chat.holdfall.stand"
							, eventId.c_str(), action.c_str()
							, UTF16(chatTimeInterval(incoming.m_remaining))
							, numHolds, numFalls, numDelays
							);
		break;
	}

	return a->offset;
}

// The host uses the destructor differently.
HoldFallVote::~HoldFallVote()
{
}

//////////////////////////////////////////////////////////////////////
// Send a request for a server tracked object to the host

void
ServerTrackedObject::request(SINT32 chatIndex)
{
	teulSend(chatIndex, _SERVER_TRACKED_OBJECTS, this);
}

//////////////////////////////////////////////////////////////////////
// Receive the hosts list of* all* commands it knows.

static SINT32
_recvDotCommandList(ACTION_OBJECT* const a)
{
	Chat::g_hostDotCommands = Chat::GetHostCommandsFromStream(a->packed);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//Installs the client callbacks

SINT32
chatInstallClientCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::CallbackX)
	{
		REGISTER_SENDER(Chat::CLIENT_MESSAGE_Y, _sendMessage, _SEND_CLIENT_MESSAGE);
		REGISTER_SENDER(Chat::FE_TUNE_TO_CHANNEL_Y, _sendTuneToChannel, _FE_TUNE_TO_CHANNEL);
		REGISTER_SENDER(Chat::UPDATE_CHANNEL_Y, _sendUpdateChannel, _SEND_UPDATE_CHANNEL);
		REGISTER_SENDER(Chat::MOTD_REQ_Y, _sendMOTD_REQ, _MOTD_REQ);
		REGISTER_SENDER(Chat::CLIENT_LOCALITY_Y, _sendLocality, _SEND_LOCALITY);

		REGISTER_RECEIVER(Chat::SERVER_MESSAGE_Y, _recvMessage);
		REGISTER_RECEIVER(Chat::SVR_TUNE_TO_CHANNEL_Y, _recvTuneToChannel);
		REGISTER_RECEIVER(Chat::USER_NLA_Y, _recvUserNLA);
		REGISTER_RECEIVER(Chat::SERVER_INFO_Y, _recvServerInfo);
		REGISTER_RECEIVER(Chat::CLIENT_GCI_MESSAGE_Y, _recvGCIMessage);
		REGISTER_RECEIVER(Chat::MOTD_REPLY_Y, _recvMOTD_Reply);
		REGISTER_RECEIVER(Chat::LOCALIZED_MESSAGE_Y, _recvLocalisedMessage);

		REGISTER_SENDRECV(Chat::SQUAD_CONTROL_Y, _sendSquadControl, _recvSquadControl, CHAT_SQUAD_CONTROL);
		REGISTER_SENDRECV(Chat::SERVER_TRACKED_OBJECTS_Y, _sendServerTrackedObject, _recvServerTrackedObjects, _SERVER_TRACKED_OBJECTS);

		REGISTER_MSG_RECEIVER(Chat::WHO_LIST_Y, CHAT_WHO_LIST);
		REGISTER_MSG_RECEIVER(Chat::WAYPOINT_LIST_Y, WAYPOINT_XFER_ARRAY);

		REGISTER_MSG(Chat::WAYPOINT_Y, WAYPOINT_XFER);
		REGISTER_MSG(Chat::WAYPOINT_CTRL_Y, WAYPOINT_CONTROL);

		REGISTER_MSG_RECEIVER(Chat::MISSION_MEMBER_UPDATE_Y, MISSION_MEMBER_UPDATES);
		REGISTER_MSG(Chat::EDIT_STRING_Y, EDIT_STRING_MSG);

		REGISTER_MSG(Chat::SQUAD_Y, SQUAD_MSG);

		REGISTER_SENDRECV(Chat::BRIGADE_CONTROL_Y, _sendBrigadeControl, _recvBrigadeControl, BrigadeControl::s_callback);
		REGISTER_RECEIVER(Chat::BRIGADE_CONTROL_LIST_Y, _recvBrigadeControlList);
		REGISTER_MSG_SENDER(Chat::STO_JOULES_Y, STO_JOULES);

		REGISTER_MSG_RECEIVER(Chat::RECRUITER_LIST_Y, RECRUITER_LIST);
		REGISTER_SENDER(Chat::RECRUITER_REQUEST_Y, _sendRecruiterRequest, _SEND_RECRUITER_REQUEST);

		REGISTER_MSG_RECEIVER(Chat::MISSION_INVITE_Y, MISSION_INVITE_MSG);

		REGISTER_SENDRECV(Chat::AWS_Y, _sendAWS, _recvAWS, AWS::callback);

		REGISTER_RECEIVER(Chat::HOLD_FALL_INFO_Y, _recvHoldFallInfo);

        REGISTER_RECEIVER(Chat::OMG_Y, _recvChatMissionStatus);

		REGISTER_RECEIVER(Chat::CMD_LIST_Y, _recvDotCommandList);
	}
	END_CALLBACK_REGISTRATION(Chat::CallbackX)
}

