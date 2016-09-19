#pragma once

#define BUFFERSIZE 16

#include <b64/encode.h>
#include <b64/decode.h>

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
SINT32 chatInstallClientCallbacks(void);

// Notify the chat host of a users ui-map target selection
void chatTargetSelection(const struct FULL_OBJ_ID& id);

FULL_OBJ_ID chatGetTargetSelection();

//Tells the chat host what update mode the client has selected
SINT32 chatSendModeNotice(SINT32 teulIndex, UCHAR updateMode);

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
        
#if !defined(AS_SERVER)
            m_clientInfo[INFO_OFFSET_PLATFORM] = platOS;
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION] = GetOSMajorVersion();
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION+1] = GetOSMinorVersion();
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION+2] = GetOSRevision();
            m_clientInfo[INFO_OFFSET_CLIENT_OS_VERSION+3] = GetOSBuild();
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION] = PN_WW2_PRODUCT_NO;
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION+1] = PN_WW2_VERSION_NO;
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION+2] = PN_WW2_PATCH_NO;
            m_clientInfo[INFO_OFFSET_CLIENT_VERSION+3] = PN_WW2_BUILD_NO;
#endif
        
    }

    // Decode string from client
    ChatClientInfo(char *decodeStr) {
        decode(decodeStr);
    }
    
    char m_clientInfo[INFO_MAX_RAW_SIZE];
    char m_output[INFO_MAX_ENCODE_SIZE+1];
    
    base64::base64_encodestate state_encode;
    base64::base64_decodestate state_decode;
    
    void decode(char *decodeStr) {
        
        base64_init_decodestate(&state_decode);
        base64::base64_decode_block(decodeStr, static_cast<int>(strlen(decodeStr)), m_clientInfo, &state_decode);
    }
    
    // Member functions
    char *encode() {
        base64_init_encodestate(&state_encode);
        
        int cnt = 0;
        char* c = m_output;
        
        /* gather data from the input and send it to the output */
        cnt = base64::base64_encode_block(m_clientInfo, static_cast<int>(sizeof(m_clientInfo)), m_output, &state_encode);
        c += cnt;
        
        /* since we have encoded the entire input string, we know that
         there is no more input data; finalise the encoding */
        cnt = base64::base64_encode_blockend(c, &state_encode);
        c += cnt;
        *c = 0;
        
        return m_output;
    }
};

