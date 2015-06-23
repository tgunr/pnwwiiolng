#pragma once

#include "wwii/wwiiServerTrackedObject.h"

namespace Teul
{
template<>
struct Packer<WorldPosition_m>
{
	WorldPosition_m	m_value;
	void packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 /*bits*/=0)
	{
		m_value.packer(mode, a);
	}
};
}
