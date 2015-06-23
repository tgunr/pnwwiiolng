#pragma once
// Created: Tue Sep  6 20:20:41 CDT 2005
//
// The network exchange for a group of member updates
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkArray.h"
#include "chat/chatMissionMember.h"

typedef teul_network_array<MissionMember> MISSION_MEMBER_UPDATES;
