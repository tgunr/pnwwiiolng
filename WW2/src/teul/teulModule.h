#pragma once
// Provides prototypes for mostly internal TeulKit functions.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

extern TeulTimeMS lastUpdateTime;

extern UINT32 totalPacketsRecv;
extern UINT32 totalPacketsXfer;

/* Basic functions */
extern SINT32		teulModIdle(const TeulTimeMS& duration_);
extern SINT32		teulModShutdown(void);

extern SINT32		teulConnectENDPOINT( ENDPOINT* inEndpoint );

/* CALLBACK functions */

extern SINT32		teulModResendPACKET(ENDPOINT* inEndpoint, struct PACKET_HEAD* packet);

extern SINT32		teulModSendCALLBACKe(ENDPOINT& inEndpoint, TEUL_CALLBACK& object);
extern SINT32 		teulModSendCallbackTo(SINT32 teulIndex, UINT32 address, UINT16 port,  TEUL_CALLBACK& object);

extern SINT32 teulMRecvAddToPoll(ENDPOINT* endpoint);
extern void teulMRecvUpdatePoll(ENDPOINT* endpoint);
extern void teulMRecvRemoveFromPoll(ENDPOINT* endpoint);

extern UINT32 teulElapsedTotal(UINT32 newTotal, UINT32 oldTotal);

extern void teulModRecvInit(void);
