#pragma once
// Created: Fri Oct 14 14:09:34 CDT 2005
//
// Routine(s) to banish players
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// ban a player, if client is not null then the player is notified
void chatBanPlayer(const ChatHandle& handle, UINT32 duration, const char* message, UINT32 issuer, const ChatHandle& issuerHandle);

// force the smiting of a player who is not currently connected to chat (requires db lookup)
void chatForceKickPlayer(const ChatHandle& handle, const char* message, UINT32 issuer, const ChatHandle& issuerHandle);

// Thread for banning players
void chatBanQueueThread();
