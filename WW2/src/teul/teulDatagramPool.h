#pragma once
// Created: Thu Aug 25 22:22:05 CDT 2005
//
// Class for managing a pool of datagrams and the
// buffers that go with them
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

extern struct DATA_GRAM* teulGetDatagram(const UINT32 dataSize);
extern void teulReleaseDatagram(struct DATA_GRAM* const gram);

// Call on shutdown
extern void teulFreeDatagramPool();
