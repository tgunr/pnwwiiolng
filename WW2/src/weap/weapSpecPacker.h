#pragma once
// Created: Fri Apr 22 12:06:41 CDT 2011
// Author:  Oliver "kfsone" Smith
// Copyright: Copyright (C) PlayNet, Inc, 2011 -- All right reserved.
// Not for redistribution.

#include "teul/teulPackers.h"
#include "weap/weapSpec.h"

namespace Teul
{

template<>
struct Packer<WEAPON_SPEC>
{
	WEAPON_SPEC m_value;
	void packer(PackMode mode, ACTION_OBJECT* const a, SINT32 /*bits*/ = 0) { return (mode == TEUL_PACK) ? pack(a, 0) : unpack(a, 0);  }
	void pack(ACTION_OBJECT* const a, SINT32 /*bits*/ = 0) const { return m_value.pack(a, 0); }
	void unpack(ACTION_OBJECT* const a, SINT32 /*bits*/ = 0) { return m_value.unpack(a, 0); }
};

} // namespace Teul
