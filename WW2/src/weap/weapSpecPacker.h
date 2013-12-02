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
	void packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 /*bits*/=0)
	{
		m_value.packer(mode, a);
	}
};

} // namespace Teul
