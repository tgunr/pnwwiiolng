#include <syscfg.h> // Path is project specific.
// Test the Chat::LocalizedMessage functionality.

#include "tests/testUnits.h"
#include "chat/chatLocalizedMessage.h"
#include "teul/teulPackers.h"

static const char* const testP0 = "King";
static const char* const testP1 = "Italy";
static const char* const testP2 = "Germany";
static const char* const testP3 = "France";
static const char* const testP4 = "Vursprung durch Technik";
static const char* const testP5 = "several words in this sentence";

static const SINT32 PackHeaderBits = 12 + 12 + 16;

// Test unit class.
class ChatLocalizedMessageTestUnit : public TestUnit
{
public:	ChatLocalizedMessageTestUnit() : TestUnit("Chat::LocalizedMessage") {}

public: virtual ~ChatLocalizedMessageTestUnit() {}

public:	virtual bool Run() override;

private:
		char buffer[1200];
		Chat::LocalizedMessage unpackLmsg;

protected:
		bool TestEmptyLmsg(const Chat::LocalizedMessage& lmsg)
		{
			CRITICAL( ExpectEqual( LSTR_NONE, lmsg.m_prefix) );
			CRITICAL( ExpectEqual( LSTR_NONE, lmsg.m_message) );
			CRITICAL( ExpectEqual( lmsg.m_size, 1 ) );
			CRITICAL( ExpectEqual( lmsg.m_data[0], '\0' ) );
			CRITICAL( ExpectTrue( lmsg.m_ptr[0] != nullptr ) );
			return
				ExpectEqual( lmsg.m_ptr[0], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[1], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[2], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[3], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[4], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[5], &lmsg.m_data[0] )
				;
		}

		bool TestLmsgWithIDsOnly(const Chat::LocalizedMessage& lmsg)
		{
			return
				ExpectEqual( lmsg.m_prefix, LSTR_NOTE )
				&& ExpectEqual( lmsg.m_message, LSTR_LABELLED_GENERIC )
				&& ExpectEqual( lmsg.m_size, 1 )
				&& ExpectEqual( lmsg.m_ptr[0], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[1], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[2], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[3], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[4], &lmsg.m_data[0] )
				&& ExpectEqual( lmsg.m_ptr[5], &lmsg.m_data[0] )
				;
		}

		bool TestLmsgWithP0(const Chat::LocalizedMessage& lmsg)
		{
			return
				ExpectEqual( lmsg.m_prefix, LSTR_SYSTEM )
				&& ExpectEqual( lmsg.m_message, LSTR_REPORTED )
				&& ExpectEqual( lmsg.m_size, strlen(testP4) + 6 )
				&& ExpectEqual( lmsg.m_ptr[0], &lmsg.m_data[0] )
				&& ExpectEqual( strcmp(lmsg.m_ptr[0], testP4), 0 )
				&& ExpectEqual( lmsg.m_ptr[1], &lmsg.m_data[0] + strlen(testP4) + 1 )
				&& ExpectEqual( lmsg.m_ptr[2], lmsg.m_ptr[1] + 1 )
				&& ExpectEqual( lmsg.m_ptr[3], lmsg.m_ptr[2] + 1 )
				&& ExpectEqual( lmsg.m_ptr[4], lmsg.m_ptr[3] + 1 )
				&& ExpectEqual( lmsg.m_ptr[5], lmsg.m_ptr[4] + 1 )
				;
		}

		bool TestLmsgWithP0ThruP5(const Chat::LocalizedMessage& lmsg)
		{
			const size_t predictedSize = strlen(testP0) + 1 + strlen(testP1) + 1 + strlen(testP2) + 1 + strlen(testP3) + 1 + strlen(testP4) + 1 + strlen(testP5) + 1;
			return
				ExpectEqual( lmsg.m_prefix, LSTR_WARNING )
				&& ExpectEqual( lmsg.m_message, LSTR_LABELLED_GENERIC )
				&& ExpectEqual( lmsg.m_size, predictedSize )
				&& ExpectEqual( lmsg.m_data[0], testP0[0] )
				&& ExpectEqual( lmsg.m_ptr[0], &lmsg.m_data[0] )
				&& ExpectEqual( strcmp(lmsg.m_ptr[0], testP0), 0 )
				&& ExpectEqual( lmsg.m_ptr[1], &lmsg.m_data[0] + strlen(testP0) + 1 )
				&& ExpectEqual( strcmp(lmsg.m_ptr[1], testP1), 0 )
				&& ExpectEqual( lmsg.m_ptr[2], lmsg.m_ptr[1] + strlen(testP1) + 1 )
				&& ExpectEqual( strcmp(lmsg.m_ptr[2], testP2), 0 )
				&& ExpectEqual( lmsg.m_ptr[3], lmsg.m_ptr[2] + strlen(testP2) + 1 )
				&& ExpectEqual( strcmp(lmsg.m_ptr[3], testP3), 0 )
				&& ExpectEqual( lmsg.m_ptr[4], lmsg.m_ptr[3] + strlen(testP3) + 1 )
				&& ExpectEqual( strcmp(lmsg.m_ptr[4], testP4), 0 )
				&& ExpectEqual( lmsg.m_ptr[5], lmsg.m_ptr[4] + strlen(testP4) + 1 )
				&& ExpectEqual( strcmp(lmsg.m_ptr[5], testP5), 0 )
			;
		}

		bool PackAndUnpack(Chat::LocalizedMessage& in)
		{
			CRITICAL( ExpectTrue( in.m_size < sizeof(in.m_data) ) );
			ACTION_OBJECT a;

			a.packed  = (UCHAR*)buffer;

			memset(buffer, 0xbe, sizeof(buffer));
			memset(&unpackLmsg, 0xfa, sizeof(unpackLmsg));

			const SINT32 expectPackSize = PackHeaderBits + (in.m_size) * 8;

			TestFocus("Packing");
			a.offset = 0;
			SINT32 packRetval = in.packer(TEUL_PACK, &a);
			CRITICAL( ExpectEqual(packRetval, expectPackSize) );

			TestFocus("Unpacking");
			a.offset = 0;
			SINT32 unpackRetval = unpackLmsg.packer(TEUL_UNPACK, &a);
			CRITICAL( ExpectEqual(unpackRetval, expectPackSize) );
			CRITICAL( unpackLmsg.m_size < sizeof(unpackLmsg.m_data) );

			return
				ExpectEqual(unpackLmsg.m_prefix, in.m_prefix)
				&& ExpectEqual(unpackLmsg.m_message, in.m_message)
				&& ExpectEqual(unpackLmsg.m_size, in.m_size)
				;
		}
};

bool
ChatLocalizedMessageTestUnit::Run()
{
	START_TEST_STAGE( "Constructors" )

		START_SUB_STAGE( "default constructor" )

			Chat::LocalizedMessage lmsg;

			CRITICAL( TestEmptyLmsg(lmsg) );

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				CRITICAL( TestEmptyLmsg(unpackLmsg) );
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "default constructor" )

		START_SUB_STAGE( "Just IDs" )

			Chat::LocalizedMessage lmsg(LSTR_NOTE, LSTR_LABELLED_GENERIC);

			TestLmsgWithIDsOnly(lmsg);

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				TestLmsgWithIDsOnly(unpackLmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "Just IDs" )

		START_SUB_STAGE( "IDs + P0" )

			Chat::LocalizedMessage lmsg(LSTR_SYSTEM, LSTR_REPORTED, testP4);

			TestLmsgWithP0(lmsg);

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				TestLmsgWithP0(unpackLmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "IDs + P0" )

		START_SUB_STAGE( "IDs + P0 to P5" )

			Chat::LocalizedMessage lmsg(LSTR_WARNING, LSTR_LABELLED_GENERIC, testP0, testP1, testP2, testP3, testP4, testP5);

			TestLmsgWithP0ThruP5(lmsg);

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				TestLmsgWithP0ThruP5(unpackLmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "IDs + P0 to P5" )

		START_SUB_STAGE( "Copy constructor" )

			Chat::LocalizedMessage orig(LSTR_WARNING, LSTR_LABELLED_GENERIC, testP0, testP1, testP2, testP3, testP4, testP5);
			Chat::LocalizedMessage copy(orig);

			START_SUB_STAGE( "Original" )
				TestLmsgWithP0ThruP5(orig);
				CRITICAL( PackAndUnpack(orig) );
				TestLmsgWithP0ThruP5(unpackLmsg);
			END_SUB_STAGE( "Original" )

			START_SUB_STAGE( "Copy" )
				TestLmsgWithP0ThruP5(copy);
			END_SUB_STAGE( "Copy" )

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(copy) );
				TestLmsgWithP0ThruP5(unpackLmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "Copy constructor" )

	END_TEST_STAGE( "Constructors" )

	START_TEST_STAGE( "Populate" )

		START_SUB_STAGE( "Just IDs" )

			Chat::LocalizedMessage lmsg;

			lmsg.Populate(LSTR_NOTE, LSTR_LABELLED_GENERIC);

			TestLmsgWithIDsOnly(lmsg);

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				TestLmsgWithIDsOnly(lmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "Just IDs" )

		START_SUB_STAGE( "IDs + P0" )

			Chat::LocalizedMessage lmsg;

			lmsg.Populate(LSTR_SYSTEM, LSTR_REPORTED, testP4);

			TestLmsgWithP0(lmsg);

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				TestLmsgWithP0(unpackLmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "IDs + P0" )

		START_SUB_STAGE( "IDs + P0 to P5" )

			Chat::LocalizedMessage lmsg;
			lmsg.Populate(LSTR_WARNING, LSTR_LABELLED_GENERIC, testP0, testP1, testP2, testP3, testP4, testP5);

			TestLmsgWithP0ThruP5(lmsg);

			START_SUB_STAGE( "Packer" )
				CRITICAL( PackAndUnpack(lmsg) );
				TestLmsgWithP0ThruP5(unpackLmsg);
			END_SUB_STAGE( "Packer" )

		END_SUB_STAGE( "IDs + P0 to P5" )

	END_TEST_STAGE( "Populate" )

	START_TEST_STAGE( "Overload" )
		const char* testString = "1, 2, 3, 4, 5, 6, afk, allied, attack, autoleader, axis, backupleader, ban, bump, buzzardspawn, clear, cpdump, cpinfo, crs, crssnoop, defend, delay, dumpresupply, fallback, fbs, find, friends, gm, gmmode, gmtakelead, grab, hc, hclist, hcmotd, hcoff, hcon, help, hold, home, hqoff, hqon, ignore, kick, kickban, lastcap, lastdeaths, lastkills, lastmoves, listcos, listoics, listsquad, m, makeco, makeleader, mlist, movebrigade, movedivision, nextmove, noechat, nowho, objectives, orders, overrun, own, pinfo, pnote, rdp, renamesquad, report, roster, setmotd, setrank, snoop, spam, status, sys, sysmotd";
		Chat::LocalizedMessage lmsg( -1, -1000, testString);
		CRITICAL( ExpectTrue( PackAndUnpack(lmsg) ) );

		CRITICAL( ExpectTrue( lmsg.m_prefix == -1 ) && ExpectTrue ( lmsg.m_message == -1000 ) );
		CRITICAL( ExpectTrue( strncmp(lmsg.m_ptr[0], "1, 2, 3, 4, 5, 6", 16) == 0 ) );
		CRITICAL( ExpectEqual( strlen(lmsg.m_ptr[0]), MAX_MESSAGE_LENGTH - 1 ) );
		CRITICAL( ExpectEqual( strlen(lmsg.m_ptr[0]), lmsg.m_size - 1)  );
		CRITICAL( ExpectTrue( strcmp(lmsg.m_ptr[0], testString) == 0 ) );

	END_TEST_STAGE( "Overload" )

	return true;
}

static ChatLocalizedMessageTestUnit test;

