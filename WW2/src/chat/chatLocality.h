#pragma once
// Created: Sun Jan 13 15:55:37 CST 2008
//
// Chat::Locality structure
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiFullObjID.h"			// For FULL_OBJ_ID m_nearCP

#pragma pack(push, 1)
namespace Chat
{
	// Notify the chat host of your location - deprecated.
	struct Locality
	{
		Locality(const FLOAT64& x, const FLOAT64& y, const FLOAT64& z, const FULL_OBJ_ID& nearCP)
			: m_x(x), m_y(y), m_z(z), m_nearCP(nearCP)
			{}

		Locality(ACTION_OBJECT* const a);

		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

		FLOAT64		m_x, m_y, m_z;		// World co-ordinates
		FULL_OBJ_ID	m_nearCP;			// Which CP you are near
	};
}
#pragma pack(pop)

