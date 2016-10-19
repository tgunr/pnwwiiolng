#pragma once

// Attempts to efficiently pack a delta for a TeulRelTimeMS.

namespace Teul
{
	//! @class Teul::NetTimeMS
	//! @brief Absolute, millisecond time for network transport.

	struct NetTimeMS
	{
		using self_type = NetTimeMS;
		using value_type = TeulTimeMS;

		value_type	m_time;

		NetTimeMS() : m_time(0) {}
		NetTimeMS(const value_type& rhs) : m_time(rhs) {}
		NetTimeMS(const self_type& rhs) : m_time(rhs.m_time) {}

		self_type& operator = ( const value_type& rhs ) { m_time = rhs; return *this; }

		operator const value_type & () const { return m_time; }
		operator value_type& () { return m_time; }

		static size_t SmallDataBits() { return 24; }
		static size_t LargeDataBits() { return 40; }
		static TeulTimeMS Granularity() { return 20ULL; }

		void packer(PackMode mode, ACTION_OBJECT* const a, SINT32 bits = 0) { return (mode == TEUL_PACK) ? pack(a, bits) : unpack(a, bits); }
		void pack(ACTION_OBJECT* const a, SINT32 bits = 0) const;
		void unpack(ACTION_OBJECT* const a, SINT32 bits = 0);
	};
}

#include "teul/teulNetTimeMSPacker.h"
