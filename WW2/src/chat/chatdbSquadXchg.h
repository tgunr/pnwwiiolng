#pragma once
// Created: Fri Sep 16 18:17:40 CDT 2005
//
// Wrapper for exchange Squad
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiSquads.h"
#include "teul/teulNetworkObject.h"

class DBSquad : public Squad
{
public:
	DBSquad() : Squad() {}
	DBSquad(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }
	~DBSquad() {}
};

typedef teul_network_object<DBSquad> CHATDB_SQUAD_XCHG;

