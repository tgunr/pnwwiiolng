#pragma once
// Created: Thu Apr  7 19:50:20 CDT 2005
//
// ArenaDescription allows the host to provide clients
// with arbitrarily labelled parameters describing the
// current arena/game environment. These are either
// stored in the wwii_arena table or they are generated
// by the host.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

using ARENA_VARS = std::map<std::string, std::string>;

class ArenaDescription : public ARENA_VARS
{
public:
	ArenaDescription() {}
	ArenaDescription(ACTION_OBJECT* const a) { a->offset = _unpack(a); }
	~ArenaDescription() {}

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);
	SINT32 send(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

public:
	static const UINT32 _MAX_NAME_LEN = 25;
	static const UINT32 _MAX_VALUE_LEN = 250;

private:
	SINT32 _unpack(ACTION_OBJECT* const a);
	SINT32 _pack(ACTION_OBJECT* const a);
};

extern ArenaDescription g_arenaDescription;

