#pragma once
// Created: Thu Oct 16 19:41:57 CDT 2008
//
// A request to spawn a weapon at a specific location
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Map
{
#pragma pack(push, 1)
	struct DirectSpawn
	{
		// Location they want to spawn at
		FLOAT64VECTOR	m_location;
		// Weapon the user wants
		WEAPON_SPEC		m_weapon;
		// Do they want to use the custom location
		bool			m_useLocation;

		SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a);
	};
#pragma pack(pop)
}

