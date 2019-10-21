#pragma once
//! @file wwiiFullObjIDPacker.h
//! @brief Standard packer for FULL_OBJ_ID struct.

#include "teul/teulPackers.h"
#include "wwii/wwiiFullObjID.h"

namespace Teul
{
	template<>
	struct Packer<FULL_OBJ_ID>
	{
		FULL_OBJ_ID m_value;
		void packer(PackMode mode, ACTION_OBJECT* const a, SINT32 /*bits*/ = 0) { return (mode == TEUL_PACK) ? pack(a, 0) : unpack(a, 0); }
		void pack(ACTION_OBJECT* const a, SINT32 /*bits*/ = 0) const
		{
			UINT64 key = m_value.Key();
			a->pack(key, 0);
		}
		void unpack(ACTION_OBJECT* const a, SINT32 /*bits*/ = 0)
		{
			UINT64 key = 0;
			a->packer(TEUL_UNPACK, key, 0);
			m_value.FromKey(key);
		}
	};
}	// Teul namespace.

