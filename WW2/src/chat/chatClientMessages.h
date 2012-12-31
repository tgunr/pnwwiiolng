#pragma once
// Created: Wed Jul 26 11:00:52 2000
//
// The main client-side API for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Installs the chat callbacks for the client
extern SINT32 chatInstallClientCallbacks(void);

// Notify the chat host of a users ui-map target selection
extern void chatTargetSelection(const struct FULL_OBJ_ID& id);

extern FULL_OBJ_ID chatGetTargetSelection();

//Tells the chat host what update mode the client has selected
extern SINT32 chatSendModeNotice(SINT32 teulIndex, UCHAR updateMode);

//This function tunes the client's radio Index to the channel indicated by
//	channelID
extern SINT32 chatTuneRadioToChannel(SINT32 teulIndex, UCHAR radioIndex, UINT32 type);

//This function tunes the client's radio channel referenced by radioIndex to
//	the handle provided
extern SINT32 chatTuneRadioToHandle(SINT32 teulIndex, UCHAR radioIndex, ChatHandle handle);

//This function tells the server to toggle receiving for the radio channel
//	indicated by radioIndex either on or off.  If the receiveStatus is 0, then
//	receiving is off, if it is > 0, then receiving is on
extern SINT32 chatUpdateChannelStatus(SINT32 teulIndex, UCHAR radioIndex, bool receiveStatus);

//This function sends a message to the server to be distributed on the
//	channels referenced by radioIndex
extern SINT32 chatSendMessage(SINT32 teulIndex, UINT16 radioIndex, const char* message);

extern SINT32 chatSendMotdRequest(SINT32 teulIndex, UINT32 id);

extern LOGICAL chatSendLocality(SINT32 teulIndex, UINT32 vehicleHostIndex, const FLOAT64& rawX, const FLOAT64& rawY, const FLOAT64& rawZ, FULL_OBJ_ID nearestCP);

extern SINT32 chatSendRecruiterRequest(SINT32 teulIndex, UCHAR language);

