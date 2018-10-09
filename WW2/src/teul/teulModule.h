#pragma once
// Provides prototypes for mostly internal TeulKit functions.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

/* Basic functions */
SINT32 teulModIdle(const TeulTimeMS duration);
SINT32 teulModShutdown();

SINT32 teulConnectENDPOINT(ENDPOINT& inEndpoint);

/* CALLBACK functions */

SINT32 teulModSendCALLBACKe(ENDPOINT& inEndpoint, TEUL_CALLBACK& object, const UCHAR protocol= PROTOCOL_UNDEF);
SINT32 teulModSendCallbackTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK& object);

void teulMRecvAddToPoll(ENDPOINT& endpoint);
void teulMRecvRemoveFromPoll(ENDPOINT& endpoint, bool permitCompaction = true);

void teulConfigureEndpointSocket(ENDPOINT& endpoint);

SINT32 teulOpenConnection(const bool listening, const SINT32 timeout, const PROTOCOL protocol, const UINT32 host, const UINT16 port,
                          CONNECT_FUNCTION connectFn, ERROR_FUNCTION errorFn, const bool initBackChannel);
void teulModStartup(ERROR_FUNCTION);

// If this flag is set, instead of doing the Teul crypto operations purely "in-place", the
// code will copy the data into a properly aligned data buffer, do the crypto on that buffer,
// and then copy the data back into the expected place. This is highly inefficient, but in
// some cases may still actually be less expensive than the guaranteed misaligned loads and
// stores the crypto performs on the data otherwise.
//#define FIX_CRYPTO_MISALIGNMENT
