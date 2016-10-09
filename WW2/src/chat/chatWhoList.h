#pragma once
// Created: Thu Jul  7 14:25:39 CDT 2005
//
// Mechanism for passing a list of Chat::Who messages around
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "teul/teulNetworkArray.h"
#include "chat/chatWho.h"

using CHAT_WHO_LIST = teul_network_array<Chat::Who>;
