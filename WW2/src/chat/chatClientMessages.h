#pragma once

#define BUFFERSIZE 16

#include "build_info.hpp"

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
SINT32 chatInstallClientCallbacks();

// Notify the chat host of a users ui-map target selection
void chatTargetSelection(const struct FULL_OBJ_ID id);

FULL_OBJ_ID chatGetTargetSelection();

//Tells the chat host what update mode and netcode the client has selected
SINT32 chatSendModeNotice(SINT32 teulIndex, UCHAR updateMode);
SINT32 chatSendNetcodeNotice(SINT32 teulIndex, bool netcode);

//This function tunes the client's radio Index to the channel indicated by
//	channelID
SINT32 chatTuneRadioToChannel(SINT32 teulIndex, UCHAR radioIndex, UINT32 type);

//This function tunes the client's radio channel referenced by radioIndex to
//	the handle provided
SINT32 chatTuneRadioToHandle(SINT32 teulIndex, UCHAR radioIndex, ChatHandle handle);

//This function tells the server to toggle receiving for the radio channel
//	indicated by radioIndex either on or off.  If the receiveStatus is 0, then
//	receiving is off, if it is > 0, then receiving is on
SINT32 chatUpdateChannelStatus(SINT32 teulIndex, UCHAR radioIndex, bool receiveStatus);

//This function sends a message to the server to be distributed on the
//	channels referenced by radioIndex
SINT32 chatSendMessage(SINT32 teulIndex, UINT16 radioIndex, const char* message);

SINT32 chatSendMotdRequest(SINT32 teulIndex, UINT32 id);

LOGICAL chatSendLocality(SINT32 teulIndex, UINT32 vehicleHostIndex, const FLOAT64& rawX, const FLOAT64& rawY, const FLOAT64& rawZ, FULL_OBJ_ID nearestCP);

SINT32 chatSendRecruiterRequest(SINT32 teulIndex, UCHAR language);

extern char GetOSMajorVersion();
extern char GetOSMinorVersion();
extern char GetOSRevision();
extern char GetOSBuild();

enum {
    INFO_OFFSET_PLATFORM = 0,

    INFO_OFFSET_CLIENT_OS_VERSION = 1,

    INFO_OFFSET_CLIENT_VERSION = 5,

    INFO_MAX_RAW_SIZE = 9,
    INFO_MAX_ENCODE_SIZE = 12
};

struct ChatClientInfo
{
    // Message Header sent with every chat message indicating the client specific information
    ChatClientInfo() {

#if platBuild == BUILD_CLIENT
            m_clientInfo[INFO_OFFSET_PLATFORM] = platOS;
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION] = GetOSMajorVersion();
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION+1] = GetOSMinorVersion();
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION+2] = GetOSRevision();
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION+3] = GetOSBuild();
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION] = PN_build_version_major;
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION+1] = PN_build_version_minor;
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION+2] = PN_build_version_patch;
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION+3] = PN_build_version_build;
#endif

    }

    // Decode string from client
    ChatClientInfo(char *decodeStr) {
        decode(decodeStr);
    }

    char m_clientInfo[INFO_MAX_RAW_SIZE];
    char m_output[INFO_MAX_ENCODE_SIZE+1];

    void decode(char *decodeStr) {
        size_t decodeLen = sizeof(m_clientInfo);
        Base64Decode(decodeStr, &m_clientInfo[0], &decodeLen);
    }

    // Member functions
    char *encode() {
        std::string output = Base64Encode(&m_clientInfo[0], sizeof(m_clientInfo));
        strlcpy(&m_output[0], output.data(), sizeof(m_output));
        return m_output;
    }
};

