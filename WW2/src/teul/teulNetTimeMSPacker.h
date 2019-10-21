#pragma once

#include "teul/teulNetTimeMS.h"

namespace Teul
{
	template<>
	struct Packer<NetTimeMS>
	{
		NetTimeMS m_value;
		void packer(PackMode mode, ACTION_OBJECT* const a, SINT32 bits=0)
		{
			return (mode == TEUL_PACK) ? pack(a, bits) : unpack(a, bits);
		}
		void pack(ACTION_OBJECT* const a, SINT32 bits = 0) const
		{
			m_value.pack(a, bits);
		}
		void unpack(ACTION_OBJECT* const a, SINT32 bits = 0)
		{
			m_value.unpack(a, bits);
		}
	};
} // namespace Teul

