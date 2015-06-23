#pragma once
// Created: Wed Apr 27 16:06:58 CDT 2005
//
// Defines the MISSION_LIST_UPDATE class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkMsg.h"

class MissionListUpdateMsg : public teul_empty_msg {};
typedef teul_network_msg<MissionListUpdateMsg> MISSION_LIST_UPDATE;

