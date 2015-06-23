#pragma once
// Created: Sun Feb 16 2004
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

struct SquadMemberUpdate;

// Process a request for squad info
void chatdbSquadInfo(SINT32 teulIndex, UINT32 squadID, bool sendMembers);

void chatdbSquadMemberUpdate(SINT32 teulIndex, SquadMemberUpdate& update);

