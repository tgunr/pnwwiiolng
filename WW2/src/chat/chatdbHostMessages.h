#pragma once
// Created: Sun Feb 16 2004
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

struct SquadMemberUpdate ;

extern SINT32 chatdbInstallHostCallbacks();
extern SINT32 chatdbSendSquadMemberUpdateResult(SINT32 teulIndex, SquadMemberUpdate& result) ;

