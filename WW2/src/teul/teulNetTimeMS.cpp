#include <syscfg.h>	// Path is project specific.

#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "teul/teulNetTimeMS.h"

void
Teul::NetTimeMS::packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 bits /*= 0*/)
{
	// The ...DataBits() values return the total number of bits
	// to use /including/ the sizing bit and sign bits.
	// The actual payload size is ...DataBits() - 1.
	static const size_t LARGE_WITH_SIGN_BITS = LargeDataBits() - 1 ;
	static const size_t SMALL_WITH_SIGN_BITS = SmallDataBits() - 1 ;

	SINT64 delta = 0LL ;

	if ( TEUL_PACK == mode )
	{
		delta = ((SINT64)m_time - (SINT64)teulClock64) / (SINT64)Granularity() ;
	}

	if ( bits )
	{
		a->packer(mode, delta, bits) ;
	}
	else
	{
		bool smallValue = Teul::FitSignedBitValue(delta, SMALL_WITH_SIGN_BITS) ;
		a->packer(mode, smallValue) ;
		a->packer(mode, delta, smallValue ? SMALL_WITH_SIGN_BITS : LARGE_WITH_SIGN_BITS) ;
	}

	if ( TEUL_UNPACK == mode )
	{
		delta *= (SINT64)Granularity() ;
		m_time = (value_type)(delta + (SINT64)teulClock64) ;
	}
}
