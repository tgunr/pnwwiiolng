#pragma once

#include "teul/teulNetTimeMS.h"

namespace Teul
{
	template<>
	struct Packer<NetTimeMS>
	{
		NetTimeMS m_value ;
		void packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 bits=0)
		{
			m_value.packer(mode, a, bits) ;
		}
	} ;
} // namespace Teul

