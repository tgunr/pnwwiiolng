#pragma once
// This module provides high-level access to the transport
// functions used in the teul API.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#define DIRECT_MASK 0x000000ff
extern SINT32 teulSendTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK* object, void*  data);

