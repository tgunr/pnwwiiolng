#pragma once
// Class and functions for synchronizing other servers with our data.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

//////////////////////////////////////////////////////////////////////
// Class for managing the synchronization queue

// Add a new host to the list of servers needing to be sync'd
extern void strtAddSyncPeer(UINT32 address);

// Remove a host from the sync list
extern void strtRemoveSyncPeer(UINT32 address);

// Check the synclist for servers waiting on another supercell
extern UINT32 strtSyncPeers();

