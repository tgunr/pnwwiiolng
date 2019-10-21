#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All Rights Reserved
// Wrapper around a clock value.

namespace Teul
{
	//! @class Teul::TimeMS
	//! @brief Wrapper around a 64-bit teul clock value that provides
	//! sanity checking and various delta functions.
	class TimeMS
	{
	private:
		TeulTimeMS m_clock64;
	public:
		explicit TimeMS() : m_clock64(0ULL) {}
		explicit TimeMS(const TimeMS& rhs) : m_clock64(rhs.time()) {}
		explicit TimeMS(TeulTimeMS _clock) : m_clock64(_clock) {}

	public:
		bool operator == (const Teul::TimeMS& rhs) const { return m_clock64 == rhs.time(); }
		bool operator != (const Teul::TimeMS& rhs) const { return m_clock64 != rhs.time(); }

		bool operator > (const Teul::TimeMS& rhs) const { return m_clock64 > rhs.time(); }
		bool operator < (const Teul::TimeMS& rhs) const { return m_clock64 < rhs.time(); }
		bool operator >= (const Teul::TimeMS& rhs) const { return m_clock64 >= rhs.time(); }
		bool operator <= (const Teul::TimeMS& rhs) const { return m_clock64 <= rhs.time(); }

		Teul::TimeMS& operator =  (const Teul::TimeMS& rhs) { m_clock64 = rhs.time(); return *this; }
		Teul::TimeMS& operator += (const Teul::TimeMS& rhs) { m_clock64 += rhs.time(); return *this; }
		Teul::TimeMS& operator -= (const Teul::TimeMS& rhs) { m_clock64 -= rhs.time(); return *this; }

		Teul::TimeMS& operator +  (const Teul::TimeMS& rhs) { m_clock64 += rhs.time(); return *this; }
		Teul::TimeMS& operator -  (const Teul::TimeMS& rhs) { m_clock64 -= rhs.time(); return *this; }

	public:
		TeulTimeMS time() const { return m_clock64; }
		UINT32 timePartial() const { return static_cast<UINT32>(m_clock64) & 0x000fffff; }
//		Teul::TimeMS& time(const TeulTimeMS& _clock) { m_clock64 = _clock; return *this; }
		void makeNever() { m_clock64 = ~0ULL; }
		void makeZero() { m_clock64 = 0ULL; }
		void makeCurrent() { m_clock64 = teulClock64; }

		TeulDeltaMS elapsed() const { return static_cast<TeulDeltaMS>(teulClock64 - m_clock64); }
		SINT32 elapsedPartial() const { return static_cast<SINT32>((teulClock64 & 0x000fffff) - timePartial()); }

		TeulDeltaMS elapsedSince(const Teul::TimeMS& rhs) const { return static_cast<TeulDeltaMS>(time() - rhs.time() ); }
		SINT32 elapsedSincePartial(const Teul::TimeMS& rhs) const { return static_cast<SINT32>(timePartial()) - static_cast<SINT32>(rhs.timePartial()); }
	};
}
