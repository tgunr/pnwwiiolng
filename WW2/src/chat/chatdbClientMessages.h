#pragma once
// Created: Sun Feb 16 2004
//
// Protos and definitions for chatdbClientMessages
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

struct SquadMemberUpdate ;

extern SINT32 chatdbInstallClientCallbacks();
extern SINT32 chatdbRequestSquad(UINT32 squadID) ;
extern SINT32 chatdbSendSquadMemberUpdate(SquadMemberUpdate& smu) ;

