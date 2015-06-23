#include <syscfg.h> // Path is project specific.
// Created: Tue Feb 16 23:02:55 PST 2010
//
// Tests for Chat::MissionStatus
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "tests/testUnits.h"
#include "omg/omgChat.hxx"

// Test unit class.
class ChatMissionStatus : public TestUnit
{
public: ChatMissionStatus() : TestUnit("Chat::MissionStatus") {}
public: virtual ~ChatMissionStatus() {}

	// Main body of the test.
public: virtual bool Run() OVERRIDE_FN
	{
		// Test values.
		static const unsigned int MID = 10, BID = 577; // , SID = 42949673;
		static const ChatHandle PID = 429496729999ULL; // Random chat handle.
		static const TeulTimeMS PTMS = 111;			// Posting time.

		START_TEST_STAGE( "Constructor/destructor" )

			// Create a new Chat::MissionStatus
			Chat::MissionStatus cms;

			static const TeulTimeMS UninitializedTimeStamp = 0ULL;

			// Components should default to appearing un-modified.
			// This is correct, because we haven't set any values yet.
			CRITICAL( ExpectEqual( cms.lastModified(), UninitializedTimeStamp ) );
			CRITICAL( ExpectEqual( cms.header().lastModified(), UninitializedTimeStamp ) );
			CRITICAL( ExpectEqual( cms.status().lastModified(), UninitializedTimeStamp ) );
			CRITICAL( ExpectEqual( cms.origin().lastModified(), UninitializedTimeStamp ) );

			//////////
			// Verify that all of the fields look properly defaulted.

			ExpectEqual( cms.header().missionID(), 0 );
			ExpectEqual( cms.header().brigadeID(), 0 );
			ExpectEqual( cms.header().poster(), 0 );
			ExpectEqual( cms.header().postedTime_ms(), UninitializedTimeStamp );

			ExpectFalse( cms.status().isOpen() );
			ExpectFalse( cms.status().isOngoing() );
			ExpectFalse( cms.status().hasMobileSpawn() );
			ExpectFalse( cms.status().canMobileSpawn() );

			ExpectEqual( cms.origin().fac(), nullObjID );
			ExpectEqual( cms.origin().cp(), nullObjID );
			ExpectFalse( cms.origin().inSupply() );

			ExpectEqual( cms.target().fac(), nullObjID );
			ExpectEqual( cms.target().cp(), nullObjID );
			ExpectFalse( cms.target().originLink() );

			ExpectEqual( cms.lead().current(), (ChatHandle)0 );
			ExpectEqual( cms.lead().backup(), (ChatHandle)0 );

			ExpectFalse( cms.fru().active() );
			ExpectEqual( cms.fru().creator(), (ChatHandle)0 );
			ExpectEqual( cms.fru().nextPlaceAvail(), UninitializedTimeStamp );

			// Lastly, all of the modified times should still be zero.
			// (ensure none of the accessors inadvertantly changed anything)
			CRITICAL( ExpectEqual( cms.lastModified(), UninitializedTimeStamp ) );
			CRITICAL( ExpectEqual( cms.header().lastModified(), UninitializedTimeStamp ) );
			CRITICAL( ExpectEqual( cms.status().lastModified(), UninitializedTimeStamp ) );
			CRITICAL( ExpectEqual( cms.origin().lastModified(), UninitializedTimeStamp ) );

		END_TEST_STAGE( "Constructor/destructor" )

		AdvanceClock_s(1);

		START_TEST_STAGE( "Population" )

			// Now test the effects of populating/nodifying fields.
			Chat::MissionStatus cms;
			const TeulTimeMS firstClock64 = teulClock64;

			// Populate the status() members and make sure they read ok after.
			cms.header().missionID(MID);
			cms.header().brigadeID(BID);
			cms.header().poster(PID);
			cms.header().postedTime_ms(PTMS);
//			cms.header().squadID(SID);

			ExpectEqual( cms.header().missionID(), MID );
			ExpectEqual( cms.header().brigadeID(), BID );
			ExpectEqual( cms.header().poster(), PID );
			ExpectEqual( cms.header().postedTime_ms(), PTMS );
//			ExpectEqual( cms.header().squadID(), SID );

			// Advance the clock so we can verify that additional changes
			// are properly stamped.
			AdvanceClock_ms(1);

			// Verify that the lastModified times were correctly altered.
			CRITICAL( ExpectEqual( cms.lastModified(), firstClock64 ) );

			// Advance the clock again to ensure old times don't creep in.
			AdvanceClock_ms(1);
			const TeulTimeMS secondClock64 = teulClock64;

			// Now test the bools.
			cms.status().isOpen(true);
			cms.status().isOngoing(false);

			// Again advance the clock so we know we're not getting
			// teulClock64s returned anywhere.
			AdvanceClock_ms(1);

			// The structure's lastModified should be recent.
			CRITICAL( ExpectEqual( cms.lastModified(), secondClock64 ) );
			// So should the status()'s.
			CRITICAL( ExpectEqual( cms.status().lastModified(), secondClock64 ) );
			// But the header()'s should not have advanced.
			CRITICAL( ExpectEqual( cms.header().lastModified(), firstClock64 ) );

			// Now check that the isOpen change took effect.
			ExpectTrue( cms.status().isOpen() );
			// Whereas isOngoing should not have.
			ExpectFalse( cms.status().isOngoing() );

			AdvanceClock_ms(1);

			// Make sure we can change it now.
			cms.status().isOngoing(true);
			ExpectTrue( cms.status().isOngoing() );

			// The status should have the current timestamp.
			CRITICAL( ExpectEqual( cms.status().lastModified(), teulClock64 ) );
			// Header should still be unmodified.
			CRITICAL( ExpectEqual( cms.header().lastModified(), firstClock64 ) );
			// But origin, target, lead and fru should be unmodified.
			CRITICAL( ExpectEqual( cms.origin().lastModified(), 0) );
			CRITICAL( ExpectEqual( cms.target().lastModified(), 0) );
			CRITICAL( ExpectEqual( cms.lead().lastModified(), 0) );
			CRITICAL( ExpectEqual( cms.fru().lastModified(), 0) );

		END_TEST_STAGE( "Population" )

		ResetClock();

		START_TEST_STAGE( "Packing" )

			Chat::MissionStatus cms;

			ACTION_OBJECT a;
			a.packed = (UCHAR*)alloca(1024);

			// The default packer should only pack one character, a UCHAR
			// saying that nothing has changed.
			a.offset = 0;
			cms.packer(TEUL_PACK, &a, BaseClock());
			CRITICAL( ExpectEqual( a.offset, 8 ) );

			// If we modify the mission ID, brigadeID and squad ID, we
			// should get a UCHAR with a value of 1 and a UINT32 + a UINT16
			// Note: deliberately only changing these two fields to
			// verify that the rest of the struct isn't sent.
			cms.header().missionID(MID);
			cms.header().brigadeID(BID);

			CRITICAL( ExpectEqual( cms.lastModified(), teulClock64 ) );

			// A pack more recent than the current time should produce the
			// same 1-byte pack.
			a.offset = 0;
			cms.packer(TEUL_PACK, &a, teulClock64 + 1);
			CRITICAL( ExpectEqual( a.offset, cms.GroupPresenceBits ) );
			CRITICAL( ExpectEqual( a.packed[0], 0 ) );

			// But packing with the current timestamp should produce
			// more data.
			{
				a.offset = 0;
				cms.packer(TEUL_PACK, &a, teulClock64);
				static const SINT32 FIELDS_CHANGED_BITS = 8;			// Only using 8 bits here.
				const SINT32 expectedBits = cms.GroupPresenceBits		// flags indicating which groups changed
										  + FIELDS_CHANGED_BITS			// which fields changed
										  + sizeof(cms.header().missionID()) * 8			// missionID
										  + sizeof(cms.header().brigadeID()) * 8			// brigadeID
										  + sizeof(cms.header().poster()) * 8		// poster
										  + Teul::NetTimeMS::SmallDataBits()	// postedTime_ms
										;
				CRITICAL( ExpectEqual( a.offset, expectedBits ) );
				CRITICAL( ExpectEqual( a.packed[0], 1 ) );
			}

			AdvanceClock_ms(100);

			// Test effect of bools.
			cms.status().isOpen(true);
			cms.status().isOngoing(true);
			cms.status().hasMobileSpawn(true);
			cms.status().canMobileSpawn(true);

			CRITICAL( ExpectEqual( cms.lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( cms.status().lastModified(), teulClock64 ) );

			{
				a.offset = 0;
				cms.packer(TEUL_PACK, &a, teulClock64 );
				static const SINT32 FIELDS_CHANGED_BITS = 8;
				static const SINT32 NUM_BOOLS = 4;
				const SINT32 expectedBits = cms.GroupPresenceBits + FIELDS_CHANGED_BITS + 1 * NUM_BOOLS;
				CRITICAL( ExpectEqual( a.offset, expectedBits ) );
			}

			// If we also change the FRU's active flag, we should need two sections packed.
			cms.fru().active(true);
			cms.fru().active(false);

			CRITICAL( ExpectEqual( cms.lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( cms.fru().lastModified(), teulClock64 ) );

			{
				a.offset = 0;
				cms.packer(TEUL_PACK, &a, teulClock64 );
				static const SINT32 FIELDS_CHANGED_BITS = 8 + 8;		// Two groups.
				static const SINT32 STATUS_NUM_BOOLS = 4;
				static const SINT32 ROUNDING = ((STATUS_NUM_BOOLS + 7) & ~7) - STATUS_NUM_BOOLS;
				static const SINT32 FRU_BITS = 1;
				const SINT32 expectedBits = cms.GroupPresenceBits + FIELDS_CHANGED_BITS + 1 * STATUS_NUM_BOOLS + ROUNDING + FRU_BITS;
				CRITICAL( ExpectEqual( a.offset, expectedBits ) );
			}

		END_TEST_STAGE( "Packing" )

		ResetClock();
		AdvanceClock_ms(1234);

		START_TEST_STAGE( "Unpacking" )

			const FULL_OBJ_ID fid = { { -23, 37 }, 99, 10, 39 };
			const ChatHandle leader = chatGetHandle("Leader");
			const ChatHandle backup = chatGetHandle("Backup");
			const TeulTimeMS testPlaceAvail = 12320;

			Chat::MissionStatus packed, unpacked;

			ACTION_OBJECT a;
			a.packed = (UCHAR*)alloca(1024);

			packed.header().missionID(MID);
			packed.header().brigadeID(BID);

			packed.status().isOngoing(true);
			packed.status().canMobileSpawn(true);

			packed.origin().cp(fid);
			packed.origin().fac(fid);

			packed.target().cp(fid);
			packed.target().fac(fid);
			packed.target().originLink(true);

			packed.lead().current(leader);
			packed.lead().backup(backup);

			packed.fru().active(true);
			packed.fru().creator(leader);
			packed.fru().nextPlaceAvail(teulClock64 + testPlaceAvail);

			CRITICAL( ExpectEqual( packed.lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( packed.header().lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( packed.status().lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( packed.origin().lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( packed.target().lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( packed.lead().lastModified(), teulClock64 ) );
			CRITICAL( ExpectEqual( packed.fru().lastModified(), teulClock64 ) );

			for ( SINT32 i = 0; i <= 64; ++i )
			{
				TestFocus("offset %d", i);

				a.offset = i;
				packed.packer(TEUL_PACK, &a, teulClock64);
				const SINT32 packedOffset = a.offset;

				a.offset = i;
				unpacked.packer(TEUL_UNPACK, &a, teulClock64);
				const SINT32 unpackedOffset = a.offset;

				CRITICAL( ExpectEqual( unpackedOffset, packedOffset ) );

				ExpectEqual( unpacked.header().missionID(), MID );
				ExpectEqual( unpacked.header().brigadeID(), BID );

				ExpectFalse( unpacked.status().isOpen() );
				ExpectTrue( unpacked.status().isOngoing() );
				ExpectFalse( unpacked.status().hasMobileSpawn() );
				ExpectTrue( unpacked.status().canMobileSpawn() );

				ExpectEqual( unpacked.origin().fac(), fid );
				ExpectEqual( unpacked.origin().cp(), fid );
				ExpectFalse( unpacked.origin().inSupply() );

				ExpectEqual( unpacked.target().fac(), fid );
				ExpectEqual( unpacked.target().cp(), fid );
				ExpectTrue( unpacked.target().originLink() );

				ExpectEqual( unpacked.lead().current(), leader );
				ExpectEqual( unpacked.lead().backup(), backup );

				ExpectTrue( unpacked.fru().active() );
				ExpectEqual( unpacked.fru().creator(), leader );
				ExpectEqual( unpacked.fru().nextPlaceAvail(), teulClock64 + testPlaceAvail );
			}

		END_TEST_STAGE( "Unpacking" )

		return true;
	}
};

// Instantiate the test class.
static ChatMissionStatus test;

