#include <syscfg.h>	// Path is project specific.

#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "teul/teulNetTimeMS.h"

// The ...DataBits() values return the total number of bits
// to use /including/ the sizing bit and sign bits.
// The actual payload size is ...DataBits() - 1.
static const size_t LARGE_WITH_SIGN_BITS = Teul::NetTimeMS::LargeDataBits() - 1;
static const size_t SMALL_WITH_SIGN_BITS = Teul::NetTimeMS::SmallDataBits() - 1;

void
Teul::NetTimeMS::pack(ACTION_OBJECT* const a, SINT32 bits /*= 0*/) const
{
	SINT64 delta = ((SINT64)m_time - (SINT64)teulClock64) / (SINT64)Granularity();

	if ( bits )
		a->pack(delta, bits);
	else
	{
		bool smallValue = Teul::FitSignedBitValue(delta, SMALL_WITH_SIGN_BITS);
		a->pack(smallValue);
		a->pack(delta, static_cast<SINT32>(smallValue ? SMALL_WITH_SIGN_BITS : LARGE_WITH_SIGN_BITS));
	}
}

void
Teul::NetTimeMS::unpack(ACTION_OBJECT* const a, SINT32 bits /*= 0*/)
{
	SINT64 delta = 0LL;

	if (bits)
		a->unpack(delta, bits);
	else
	{
		bool smallValue = Teul::FitSignedBitValue(delta, SMALL_WITH_SIGN_BITS);
		a->unpack(smallValue);
		a->unpack(delta, static_cast<SINT32>(smallValue ? SMALL_WITH_SIGN_BITS : LARGE_WITH_SIGN_BITS));
	}

	delta *= (SINT64)Granularity();
	m_time = (value_type)(delta + (SINT64)teulClock64);
}
