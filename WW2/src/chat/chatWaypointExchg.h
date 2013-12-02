#pragma once
// Created: Wed Aug 31 15:01:06 CDT 2005
//
// Definition of waypoint exchange types
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/platforms.h"

#include "chat/chatWaypoint.h"
#include "teul/teulNetworkMsg.h"
#include "teul/teulNetworkArray.h"

// REQUEST: client->host request or host request for approval
// POST: client->host approval of a waypoint or host, defacto "post this waypoint"
enum WPXFR_TYPE { WPXT_INVALID, WPXT_REQUEST, WPXT_POST };

// APPROVE: Host letting you know your request was approved,
// DENIED: Host letting you know your request was denied
// REMOVE: Defacto from host "remove this waypoint"
enum WPCTRL_TYPE { WPCTRL_APPROVE, WPCTRL_DENIED, WPCTRL_INVALID, WPCTRL_REMOVE, WPCTRL_NO_MISSION };

typedef teul_network_msg<Waypoint> WAYPOINT_XFER;

class WaypointControlMsg : public teul_empty_msg {};
typedef teul_network_msg<WaypointControlMsg> WAYPOINT_CONTROL;

typedef teul_network_array<Waypoint> WAYPOINT_XFER_ARRAY;
