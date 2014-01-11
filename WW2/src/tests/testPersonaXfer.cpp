#include <syscfg.h> // Path is project specific.
// Created: Wed Apr 27 12:28:43 CDT 2011
//
// PersonaXfer unit tests.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testUnits.h"			// Unit test framework.
#include "server/serverPersonaXfer.h"	// Main include file.
#include "teul/teulEndpointDefs.h"		// For Teul::MaxPacket.

using namespace Server;

class TestPersonaXfer : public TestUnit
{
	//! @brief Test constructor.
	//! @detail Populates outDisp1 and outDisp2 with distinct but arbitrary values.
	public:	TestPersonaXfer() : TestUnit("PersonaXfer")
				, outDisp1(12345, 0xafe2c07e, 0xaabb0022, 0x01020304, 0, "plyr12345")
				, outDisp2(73747, 0x3947886b, 0x31425364, 0x0a0b0c0d, 1, "plyr73747")
	{
	}

	// Outgoing Auth::Dispatch objects.
	private: Auth::Dispatch outDisp1, outDisp2;

	private:
	template<Transfer::XferReason _reason>
	bool TestConstructor(const char* const model, const Auth::Dispatch& dispatch)
	{
		BEGIN_SUB_STAGE( model )

			PersonaXfer xfer(_reason, dispatch, 0, nullptr, 0);

			CRITICAL( ExpectEqual( xfer.xferID(), 0 )
					&& ExpectEqual( xfer.reason(), _reason )
					&& ExpectEqual( xfer.source(), 0 )
					&& ExpectEqual( xfer.dest(), 0 )
					&& ExpectEqual( xfer.timestamp(), BaseClock() )
					&& ExpectTrue( xfer.data.dispatch == dispatch )
					&& ExpectTrue( xfer.data.auxSize == 0 )
					&& ExpectTrue( xfer.data.auxData == nullptr )
					);

			// Trying to call onComplete should get us an EINVAL.
			ExpectEqual( xfer.onComplete(0), -EINVAL );
			ExpectEqual( xfer.onComplete(-1), -EINVAL );
			ExpectEqual( xfer.onComplete(1), -EINVAL );

		END_SUB_STAGE( model )

		return true;
	}

	//! @brief Test body.
	public: virtual bool Run()
	{
		// Test the baseclass constructor.
		BEGIN_TEST_STAGE( "PersonaXfer Construction" )

			CRITICAL( (TestConstructor<Transfer::XferReason::Login>("xrLogin/outDisp1", outDisp1)) );
			CRITICAL( (TestConstructor<Transfer::XferReason::Login>("xrLogin/outDisp2", outDisp2)) );

			CRITICAL( (TestConstructor<Transfer::XferReason::Update>("xrUpdate/outDisp1", outDisp1)) );
			CRITICAL( (TestConstructor<Transfer::XferReason::Update>("xrUpdate/outDisp2", outDisp2)) );

			CRITICAL( (TestConstructor<Transfer::XferReason::Relogin>("xrRelogin/outDisp1", outDisp1)) );
			CRITICAL( (TestConstructor<Transfer::XferReason::Relogin>("xrRelogin/outDisp2", outDisp2)) );

			CRITICAL( (TestConstructor<Transfer::XferReason::Reassign>("xrReassign/outDisp1", outDisp1)) );
			CRITICAL( (TestConstructor<Transfer::XferReason::Reassign>("xrReassign/outDisp2", outDisp2)) );

			CRITICAL( (TestConstructor<Transfer::XferReason::Mirror>("xrMirror/outDisp1", outDisp1)) );
			CRITICAL( (TestConstructor<Transfer::XferReason::Mirror>("xrMirror/outDisp2", outDisp2)) );

			CRITICAL( (TestConstructor<Transfer::XferReason::Exit>("xrExit/outDisp1", outDisp1)) );
			CRITICAL( (TestConstructor<Transfer::XferReason::Exit>("xrExit/outDisp2", outDisp2)) );

		END_TEST_STAGE( "PersonaXfer Construction" )

		BEGIN_TEST_STAGE( "PersonaXfer Packing" )

			PersonaXfer xferOut(Transfer::XferReason::Reassign, outDisp1);
			PersonaXfer xferIn;
			char packBuffer[Teul::MaxPacket];
			memset(packBuffer, 0, sizeof(packBuffer));
			ACTION_OBJECT aOut(packBuffer, 0, 0);
			ACTION_OBJECT aIn(packBuffer, 0, 0);

			BEGIN_SUB_STAGE( "Pack" )

				xferOut.packer(TEUL_PACK, &aOut);
				// Check for an error or a 0 size pack.
				CRITICAL( ExpectTrue( aOut.offset > 0 ) );
				// Check that what we produced isn't bigger than the object itself.
				CRITICAL( ExpectTrue( (UINT32)aOut.offsetInBytes() <= sizeof(xferOut) ) );

			END_SUB_STAGE( "Pack" )

			BEGIN_SUB_STAGE( "Unpack" )

				xferIn.packer(TEUL_UNPACK, &aIn);
				CRITICAL( ExpectTrue( aIn.offset > 0 ) );
				CRITICAL( ExpectTrue( (UINT32)aIn.offsetInBytes() <= sizeof(xferIn) ) );

			END_SUB_STAGE( "Unpack" )

			BEGIN_SUB_STAGE( "Compare" )

				CRITICAL( ExpectEqual( xferIn.xferID(), xferOut.xferID() )
						&& ExpectEqual( xferIn.reason(), xferOut.reason() )
						&& ExpectEqual( xferIn.source(), xferOut.source() )
						&& ExpectEqual( xferIn.dest(), xferOut.dest() )
						&& ExpectEqual( xferIn.timestamp(), xferOut.timestamp() )
						&& ExpectTrue( xferIn.data.dispatch == xferOut.data.dispatch )
						&& ExpectEqual( xferIn.data.auxSize, xferOut.data.auxSize )
						&& ExpectEqual( xferIn.data.auxData, xferOut.data.auxData )
						);

				CRITICAL( ExpectEqual( aOut.offsetInBits(),  aIn.offsetInBits() ) );

			END_SUB_STAGE( "Compare" )

		END_TEST_STAGE( "PersonaXfer Packing" )

		// Test complete.
		return true;
	}
};

static TestPersonaXfer test;

