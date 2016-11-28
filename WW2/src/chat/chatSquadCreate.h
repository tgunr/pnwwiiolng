#pragma once
// Created: Thu Sep 22 11:57:12 CDT 2005
//
// Squad creation management
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatSquadControlResults.h"

class SquadControl;
class Player;

void chatProcessSquadCreateRequest(SquadControl&, Player* player);
SQUAD_CONTROL_RESULT chatProcessSquadCreateReply(bool created, const ChatHandle originator, Player* player);
bool chatExpireSquadCreates();
SQUAD_CONTROL_RESULT chatSquadAcceptCreateInvite(Player* player, Player* sender);
void chatSquadDeclineCreateInvite(Player* player, Player* sender);
