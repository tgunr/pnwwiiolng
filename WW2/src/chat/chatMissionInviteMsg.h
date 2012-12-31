#pragma once
// Created: Tue Mar 13 15:43:27 CDT 2007
//
// RPC Wrapper (teulNetworkMsg) for MissionInvites
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"
#include "chat/chatMissionInvite.h"

// Doesn't need a network_msg because we have no key or flags
typedef teul_network_object<MissionInvite> MISSION_INVITE_MSG ;
