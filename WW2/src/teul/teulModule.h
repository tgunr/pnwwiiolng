#pragma once
// Provides prototypes for mostly internal TeulKit functions.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

extern TeulTimeMS lastUpdateTime;

/* Basic functions */
SINT32 teulModIdle(const TeulTimeMS duration);
SINT32 teulModShutdown();

SINT32 teulConnectENDPOINT(ENDPOINT& inEndpoint);

/* CALLBACK functions */

SINT32 teulModSendCALLBACKe(ENDPOINT& inEndpoint, TEUL_CALLBACK& object, const UCHAR protocol= PROTOCOL_UNDEF);
SINT32 teulModSendCallbackTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK& object);

void teulMRecvAddToPoll(ENDPOINT& endpoint);
void teulMRecvRemoveFromPoll(ENDPOINT& endpoint, bool permitCompaction = true);

UINT32 teulElapsedTotal(UINT32 newTotal, UINT32 oldTotal);

void teulConfigureEndpointSocket(ENDPOINT& endpoint);
