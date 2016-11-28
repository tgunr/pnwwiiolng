#include <syscfg.h> // Path is project specific.
// Date: Thu Apr  7 19:54:18 CDT 2005
//
// Description of the arena
//
// Copyright (C) PlayNet, Inc. 2005 - All rights reserved.
// Not for redistribution without permission as part of an API.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"
#include "map/mapArenaDescription.h"

// Public, global
ArenaDescription g_arenaDescription;

SINT32
ArenaDescription::_pack(ACTION_OBJECT* const a)
{
	UINT32 vars = (empty()) ? 0 : size();
	a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &vars, 8);

	if ( vars <= 0 )
		return a->offset;

	UINT32 nameLen, valLen;

	for ( auto it = begin(), endIt = end(); it != endIt; ++it )
	{
		nameLen = it->first.size();
		valLen = it->second.size();
		if ( nameLen == 0 || valLen == 0 )
			continue;

		a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &nameLen, 5);
		a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &valLen, 8);

		a->offset = teulPackVoid(TEUL_PACK, a->packed, a->offset, (void*)it->first.data(), nameLen);
		a->offset = teulPackVoid(TEUL_PACK, a->packed, a->offset, (void*)it->second.data(), valLen);
	}

	return a->offset;
}

SINT32
ArenaDescription::_unpack(ACTION_OBJECT* const a)
{
	UINT32 vars;
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &vars, 8);

	if ( vars <= 0 )
		return a->offset;

	UINT32 nameLen, valLen;
	char name[_MAX_NAME_LEN+1], value[_MAX_VALUE_LEN+1];

	name[_MAX_NAME_LEN] = 0;
	value[_MAX_VALUE_LEN] = 0;

	for ( UINT32 i = 0; i < vars; ++i )
	{
		a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &nameLen, 5);
		a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &valLen, 8);

		a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, name, nameLen);
		a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, value, valLen);

		name[nameLen] = 0;
		value[valLen] = 0;

		insert(std::make_pair(name, value));
	}

	return a->offset;
}

SINT32
ArenaDescription::packer(PackMode mode, ACTION_OBJECT* a)
{
	if ( mode == TEUL_PACK )
		return _pack(a);
	else
		return _unpack(a);
}

