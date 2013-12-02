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
		void packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 /*bits*/=0)
		{
			if ( mode == TEUL_UNPACK )
			{
				UINT64 key = 0;
				a->packer(TEUL_UNPACK, key, 0);
				m_value.FromKey(key);
			}
			else
			{
				UINT64 key = m_value.Key();
				a->packer(TEUL_PACK, key, 0);
			}
		}
	};
}	// Teul namespace.

