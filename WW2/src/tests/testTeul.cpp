// A small testbed for building teulkit code.
// See $(SolutionDir)/Tools/TestTeul/ReadMe.txt for more details.
//
// Copyright (C) PlayNet Inc, 2011.
//
// Originating Programmer: Oliver "kfs1" Smith

#include <syscfg.h>		// Should use the teulKit syscfg.h

#include <sys/types.h>

// For the exception types we use.
#include "teul/teulExceptions.h"

// "teulKit.h" is the kitchen sink of TeulKit includes,
// you should generally try to limit yourself to the
// parts of teulKit you need. However, because this
// app needs the teulOpen family of functions, it has
// to include teulKit.h.
#include "teul/teulKit.h"

// An "ENDPOINT" wraps the native socket structure
// with tracking of state, etc.
// Rule of thumb: If you're including Endpoint, you
// are probably doing something wrong.
// (We need to look inside the ENDPOINT structure
// to check various conditions and states, though).
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"

// Teul::Packet is the new API for sending data
// that you have sized & marshaled outside of
// the normal teulSend -> packer process.
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"

// TeulKit is driven by a callback system: data is
// packaged and then fired to an 'ENDPOINT' as a
// callback, which will be executed on the remote end.
// wwiiCallbacks.h makes registering lots of
// callbacks much, much easier than accessing the
// raw C functionality directly.
#include "wwii/wwiiCallbacks.h"

#include "tests/testTeulTest.h"

// teulVersion.h provides enumerations of various
// feature-versions of teulKit so we can tell what
// capabilities different teuls have.
#include "teul/teulVersion.h"

// Specific tests we want to have run.

SINT32	s_lastError ;

// Store any configuration settings here for cleanliness.
namespace Config
{
	enum
	{
		TestPort			=	27010
	,	ConnectTimeoutMS	=	3 * 1500
	,	Test32BitValue		=	1003450123UL
	} ;

	// Which protocol to use.
	static PROTOCOL protocol = TCP ;

	// If true (with --debug): enables debugging output.
	static bool debugOutput = false ;

	// If true (without --quiet): enables operational output.
	static bool infoOutput = true ;

	// Enumeration of modes we can operate in.
	enum OperatingMode { mUnknown, mServer, mClient } ;
	static OperatingMode mode = mUnknown ;

	// In client mode, this will provide the address
	// of the server to connect to.
	static const char* serverName = "127.0.0.1" ;

	// How many seconds should we sleep for to stall connections?
	static UINT32 stallTime_s = 5 ;

	// String to use for the Teul::Packet tests.
	static const char teulPacketString[] = "Hello, world!" ;

	// How large should the really large packet be?
	// Adding all these just ensures it's big.
	static const size_t largePacketSize_bytes = Teul::MaxAllowedPayload_bytes ;

	// How many times to repeat big tests.
	static size_t iterFactor = 1 ;
}

//////////////////////////////////////////////////////////////////////
#if platOS == OS_WIN32
// Windows support.
// This app is primarily developed for Linux, but the whole point of
// teulKit is that it's cross-platform, so this code should largely
// be able to run on both platforms.
//
// Naturally, there are a few minor differences here and there...

#if platBuild != TEUL_SERVER
	// Under Linux, this is supplied by libteulserver.
	#include "teul/teulConnectionInstruction.h"
	SINT32
	Teul::ConnectionInstruction::receive(SINT32 teulIndex)
	{
		throw Teul::IndexError(teulIndex, "Unexpected connection instruction received.\n", m_hostIP) ;
	}
#endif
// End of windows support section.
#elif platOS == OS_MACOS
// Mac support.
#include "teul/teulConnectionInstruction.h"
SINT32
Teul::ConnectionInstruction::receive(SINT32 teulIndex)
{
	throw Teul::IndexError(teulIndex, "Unexpected connection instruction received.\n", m_hostIP) ;
}

// End of mac support.
#elif platOS == OS_LINUX
// Linux support.

// End of linux support.
#else
	#error("You need a -DplatOS=OS_... for your platform.")
#endif

//////////////////////////////////////////////////////////////////////
// Macro: Only print output if debugOutput is enabled.

#define Dprintf(...) { if ( Config::debugOutput == true ) printf("[D] " __VA_ARGS__) ; }

//////////////////////////////////////////////////////////////////////
// Macro: Only print output if infoOutput is enabled.

#define Iprintf(...) { if ( Config::infoOutput == true ) printf("[I] " __VA_ARGS__) ; }

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Header for an echo packet.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct EchoPacketHeader
{
	enum { Request = 1, Reply = 2 } ;
	UINT32		m_direction ;		// Request or reply.

	enum { tSint16 = 1, tUint16, tSint32, tUint32, tUint64, tSint64, tMax } ;
	UINT32		m_type ;			// What type is being used.

	UINT32		m_size ;			// How big the data is.

	UINT16		m_count ;			// Number of entries.

	UCHAR		m_data[1200] ;

	static UINT64 s_acked ;
	static UINT64 s_requested ;

	template<typename Type_>
	void packData(LOGICAL mode, ACTION_OBJECT* const a)
	{
		Type_* data = (Type_*)(m_data) ;
		const size_t typeSize = sizeof(Type_) ;
		for ( UINT16 i = 0 ; i < m_count ; ++i )
		{
			a->packer(mode, data[i], typeSize - (i % typeSize)) ;
		}
	}

	void packer(LOGICAL mode, ACTION_OBJECT* const a)
	{
		a->packer(mode, m_direction, 2) ;
		a->packer(mode, m_type, 6) ;
		a->packer(mode, m_size, 16) ;
		a->packer(mode, m_count) ;

		if ( a->offset < 0 )
			throw std::runtime_error("Invalid echo packet header") ;

		switch ( m_type )
		{
		case tUint16:
			packData<UINT16>(mode, a) ;
		break ;
		case tUint32:
			packData<UINT32>(mode, a) ;
		break ;
		case tUint64:
			packData<UINT64>(mode, a) ;
		break ;

		case tSint16:
			packData<SINT16>(mode, a) ;
		break ;
		case tSint32:
			packData<SINT32>(mode, a) ;
		break ;
		case tSint64:
			packData<SINT64>(mode, a) ;
		break ;

		default:
			Dprintf("Echo type: %u\n", m_type) ;
			throw std::runtime_error("Invalid echo type") ;
		break ;
		}

		if ( a->offset < 0 )
			throw std::runtime_error("Invalid echo packet body") ;

		static const UINT64 base = 0xa010203 ;
		UINT64 final = base + mode ;
		a->packer(mode, final) ;
		if ( mode == TEUL_UNPACK && final != base + TEUL_PACK )
			throw std::runtime_error("Wrong final") ;
	}

	static SINT32 _recv(ACTION_OBJECT* const a)
	{
		EchoPacketHeader inc ;
		inc.packer(TEUL_UNPACK, a) ;

		if ( inc.m_direction == EchoPacketHeader::Request )
		{
			Dprintf("Replying to echo packet\n") ;
			EchoPacketHeader out(inc) ;
			out.m_direction = EchoPacketHeader::Reply ;
			teulSend(a->teulIndex, a->actionObj, &out) ;
		}
		else
		{
			Dprintf("Receiving echo packet\n") ;
			EchoPacketHeader::s_acked++ ;
		}

		return a->offset ;
	}

	static SINT32 _send(ACTION_OBJECT* const a)
	{
		EchoPacketHeader* const out = (EchoPacketHeader*)(a->localRawData) ;

		out->packer(TEUL_PACK, a) ;

		if ( a->offset < 0 )
			throw std::runtime_error("Error sending echo packet header") ;

		if ( out->m_direction == EchoPacketHeader::Request )
			EchoPacketHeader::s_requested++ ;

		return a->offset ;
	}
} ;

UINT64 EchoPacketHeader::s_acked = 0 ;
UINT64 EchoPacketHeader::s_requested = 0 ;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Some object-type messages.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
struct TestObject
{
	static const size_t MaxNameLen = 32;
	char name[MaxNameLen+1];
	UINT32 intValue;
	FLOAT32	floatValue;
	bool setMeTrue;
	bool setMeFalse;
	UCHAR charVal;
	FLOAT64 doubleVal;
};

static const TestObject s_sourceTestObject = { "Hello World", 74738, 3.434f, true, false, 'Q', 666.999 };
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// "Role" base class.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct Role
{
	//////////////////////////////////////////////////////////////
	// Default, base-class constructor.

	Role() : m_teulIndex(-1) {}

	//////////////////////////////////////////////////////////////
	// Base-class destructor: clean things up.

	virtual ~Role()
	{
		if ( m_teulIndex >= 0 )
		{
			const SINT32 teulIndex = m_teulIndex ;
			m_teulIndex = -1 ;
			teulUpdateTransport(250) ;
			Dprintf("Closing index %d.\n", teulIndex) ;
			teulClose(teulIndex) ;
			Dprintf("Closed index %d.\n", teulIndex) ;
		}
	}

	//////////////////////////////////////////////////////////////
	// Perform teulKit initialization.

	private: static void InitializeTeul()
	{
		// Create a name for our instance.
		const char* const moduleName = (Config::mode == Config::mServer) ? "teulServer" : "teulClient" ;

		// Tell teulKit this is what we're called.
		Teul::SetAppName(moduleName) ;
		Dprintf("App Name: %s.\n", moduleName) ;

		// Initialize the teulKit transport, telling it how/where to log,
		// and providing it with our custom error and shutdown handlers.
		SINT32 ret = teulInitTransport(LOG_TO_FILE, moduleName, Role::transportErrorFn, Role::transportKillFn) ;
		if ( ret < 0 )
			throw Teul::Error("teulInitTransport failed", ret) ;

		// Tell teulkit we want all of the debug log output.
		dbugSetLogLevel(Debug::ERR_DBUG) ;

		Dprintf("teulTransport initialized.\n") ;

		dbugLog(_FLI, "teulTransport initialized.\n") ;
	}

	//////////////////////////////////////////////////////////////
	// Initialize and run the test.

	public:	void Start()
	{
		Dprintf("Initializing teul.\n") ;
		InitializeTeul() ;

		Dprintf("Registering callbacks.\n") ;
		RegisterCallbacks() ;

		Dprintf("Running.\n") ;
		Run() ;
	}

	//////////////////////////////////////////////////////////////
	// Each role must provide it's own callback registration
	// routine, as the callbacks it uses/provides may vary.

	virtual int RegisterCallbacks() = 0 ;

	//////////////////////////////////////////////////////////////
	// Each role must provide a Run function, which implements
	// the actual functionality of that role/mode.

	virtual void Run() = 0 ;

	//////////////////////////////////////////////////////////////
	// Our primary teulIndex.

	SINT32	m_teulIndex ;

	//////////////////////////////////////////////////////////////
	// Standard callbacks.

	static TEUL_CALLBACK* s_cbHello ;
	static TEUL_CALLBACK* s_cbRelHello ;
	static TEUL_CALLBACK* s_cbPacketTest ;
	static TEUL_CALLBACK* s_cbLargePacket ;
	static TEUL_CALLBACK* s_cbShutdown ;
	static TEUL_CALLBACK* s_cbUnencrypted ;
	static TEUL_CALLBACK* s_cbEcho ;
	static TEUL_CALLBACK* s_cbUnregistered;	// At least, not as a receiver.

	//////////////////////////////////////////////////////////////////////
	// Handler for fundamental, low-level teulKit errors which occur
	// outside of the normal endpoint-oriented infrastructure.

	public: static void transportErrorFn(const SINT32 teulIndex, const UCHAR*, SINT32, const TEUL_CALLBACK*, SINT32 error)
	{
		throw Teul::IndexError(teulIndex, "transport error", error) ;
	}

	//////////////////////////////////////////////////////////////////////
	// Handle teul transport kill signal.

	public: static void	transportKillFn(const SINT32 teulIndex, const UCHAR*, SINT32, const TEUL_CALLBACK*, SINT32 error)
	{
		printf("[X] t:%d got transportKill Signal [%d]", teulIndex, error) ;
		teulCloseTransport() ;
		throw Teul::IndexError(teulIndex, "transport kill signal received", error) ;
	}

	//////////////////////////////////////////////////////////////
	// Generic error handling for errors on a given connection.

	static void errorFn(SINT32 teulIndex, const UCHAR* const, SINT32, const TEUL_CALLBACK*, const SINT32 error)
	{
		if ( error == ENOTCONN )
		{
			Dprintf("t:%d closed\n", teulIndex) ;
		}
		else
			printf("[W] t:%d error:%d\n", teulIndex, error) ;

		// Save the error.
		s_lastError = error ;
	}

	//// Callback X/Y values.
	// Callbacks are indexed by an X and Y value. This is supposed
	// to make it easier to look a particular callback up.
	enum { Test_X = 123 } ;
	// Enumeration of Y values within our callback X.
	// The "1" callback is nice and easy, I made the second one
	// a number that does not include the same bit.
	enum { Hello_Y = 1, HelloRel_Y = 6  } ;
	// The rest are just incremental.
	enum
	{
		PacketTest_Y = 32
	,	LargePacket_Y
	,	Shutdown_Y
	,	Unencrypted_Y
	,	Echo_Y
	,	ObjTest_Y
		// Callbacks we don't specifically register.
	,	Unregistered_Y = 100	// Neither send nor receive.
	} ;

	public:
	void ValidateTeulPacket(UCHAR* const data, const size_t size)
	{
		static const size_t textSize = sizeof(Config::teulPacketString) ;
		static const size_t expectSize = textSize + Teul::Packet::DefaultLocalDataSize + sizeof(UINT32) + textSize ;
		if ( size != expectSize )
		{
			Iprintf("Expected packet to be %u bytes\n", expectSize) ;
			throw Teul::Error("First Teul::Packet size is wrong.", size) ;
		}
		if ( memcmp(data, Config::teulPacketString, textSize) != 0 )
			throw Teul::Error("Data in Teul::Packet is corrupt.", 0) ;

		static const size_t enlargedSize = textSize + Teul::Packet::DefaultLocalDataSize ;
		for ( UINT32 i = textSize ; i < enlargedSize ; ++i )
		{
			if ( data[i] != (UCHAR)i )
				throw Teul::IterationError(i, "Teul::Packet has bad data", data[i]) ;
		}

		UINT32* testValue = (UINT32*)(data + textSize + Teul::Packet::DefaultLocalDataSize) ;
		if ( *testValue != Config::Test32BitValue )
			throw Teul::Error("Got wrong 32-bit test value", *testValue) ;

		if ( memcmp(data + textSize + Teul::Packet::DefaultLocalDataSize + sizeof(UINT32), Config::teulPacketString, textSize) != 0 )
			throw Teul::Error("Tail copy of test string is wrong", 0) ;

		Iprintf("Teul packet is good. '%s'.\n", (char*)(data)) ;
	}

	public:
	void TestPacket(SINT32 teulIndex)
	{
		if ( tTeulPkt.isEnabled() == false )
			return ;

		// UnencryptedBytes is supposed to be '3'.
		if ( Teul::Packet::UnencryptedBytes != 3 )
			throw Teul::Error("UnencryptedBytes is not 3", Teul::Packet::UnencryptedBytes) ;

		// Prepare the packet!
		Dprintf("preparing Teul::Packet\n") ;
		Teul::Packet packet(0, Role::Test_X, Role::PacketTest_Y) ;

		static const UINT32 expectHeaderBytes = 8 ;
		if ( packet.headerSizeBytes() != expectHeaderBytes )
		{
			printf("headerSizeBytes is %" PRSizeT "\n", packet.headerSizeBytes()) ;
			throw Teul::Error("Teul::Packet header size is different than expected", expectHeaderBytes) ;
		}

		// We start with a zero-byte allocation, lets make sure that's what we got.
		if ( packet.payloadSizeBytes() != 0 )
			throw Teul::Error("Initial payload size should be zero bytes.", packet.payloadSizeBytes()) ;
		if ( packet.packetCapacityBytes() != packet.DefaultLocalDataSize )
			throw Teul::Error("Initial wire capacity doesn't appear to be the default.", packet.packetCapacityBytes()) ;
		if ( packet.packetSizeBytes() != packet.headerSizeBytes() )
			throw Teul::Error("Initial packet size doesn't appear to be the default.", packet.packetSizeBytes()) ;

		// Capture the original data pointer so we can see if it changes later.
		const void* const originalPtr = packet.packet() ;

		static const size_t textSize = sizeof(Config::teulPacketString) ;

		if ( textSize != (strlen(Config::teulPacketString) + 1) )
			throw Teul::Error("Miscalculated the size of textSize", textSize) ;
		if ( textSize > packet.DefaultLocalDataSize )
			throw Teul::Error("Initial small payload is too big to fit.", textSize) ;

		Dprintf("expanding packet to handle text string\n") ;
		packet.setPayloadSizeBytes(textSize) ;
		if ( packet.payloadSizeBytes() != textSize )
			throw Teul::Error("SetPayloadSizeBytes didn't set the correct size", packet.payloadSizeBytes()) ;

		void* const data = packet.payload() ;
		memcpy(data, Config::teulPacketString, packet.payloadSizeBytes()) ;

		Dprintf("expanding packet to handle %u extra bytes\n", Teul::Packet::DefaultLocalDataSize) ;
		// Now make the packet size bigger so it has to allocate extra space.
		static const size_t enlargedSize = textSize + Teul::Packet::DefaultLocalDataSize ;
		packet.setPayloadSizeBytes(enlargedSize) ;
		// The capacity should have grown...
		if ( packet.packetCapacityBytes() != Teul::Packet::DefaultLocalDataSize + Teul::Packet::GrowthRate  )
			throw Teul::Error("Upsizing payload didn't produce correct capacity.", packet.packetCapacityBytes()) ;
		// And the data pointer should have changed.
		if ( packet.packet() == originalPtr )
			throw Teul::Error("Upsizing payload didn't change the data() ptr.", 0) ;
		// The payload size should be what we requested.
		if ( packet.payloadSizeBytes() != enlargedSize )
			throw Teul::Error("Upsizing payload didn't produce correct payload size.", packet.payloadSizeBytes()) ;
		// And the data should still be our string.
		if ( memcmp(packet.payload(), Config::teulPacketString, textSize) != 0 )
			throw Teul::Error("Upsizing payload corrupted the payload.", 0) ;

		UCHAR* const newData = (UCHAR*)packet.payload() ;
		for ( size_t i = textSize ; i < enlargedSize ; ++i )
		{
			newData[i] = (UCHAR)i ;
		}

		// Add a UINT32
		Dprintf("appending UINT32\n") ;
		UINT32 testValue = Config::Test32BitValue ;
		packet.append(testValue) ;
		if ( packet.packetCapacityBytes() != Teul::Packet::DefaultLocalDataSize + Teul::Packet::GrowthRate  )
			throw Teul::Error("Adding UINT32 caused unexpected capacity growth.", packet.packetCapacityBytes()) ;

		// Add another copy of the text string.
		Dprintf("appending another copy of text string\n") ;
		packet.append(Config::teulPacketString, textSize) ;
		if ( packet.packetCapacityBytes() != Teul::Packet::DefaultLocalDataSize + Teul::Packet::GrowthRate  )
			throw Teul::Error("Adding second copy of string caused unexpected capacity growth.", packet.packetCapacityBytes()) ;

		SINT32 result = packet.send(teulIndex) ;
		if ( result < 0 )
			throw Teul::Error("Teul::Packet send failed", result) ;

		Iprintf("Teul::Packet sent: %" PRSizeT " [%" PRSizeT " of %" PRSizeT "].\n", packet.payloadSizeBytes(), packet.packetSizeBytes(), packet.packetCapacityBytes()) ;
	}

} ;

//////////////////////////////////////////////////////////////////////
// Shared callbacks etc.

TEUL_CALLBACK* Role::s_cbHello ;
TEUL_CALLBACK* Role::s_cbRelHello ;
TEUL_CALLBACK* Role::s_cbPacketTest ;
TEUL_CALLBACK* Role::s_cbLargePacket ;
TEUL_CALLBACK* Role::s_cbShutdown ;
TEUL_CALLBACK* Role::s_cbUnencrypted ;
TEUL_CALLBACK* Role::s_cbEcho ;
TEUL_CALLBACK* Role::s_cbUnregistered;

// Include the client role.
#include "tests/testTeulClient.h"

// Include the server role.
#include "tests/testTeulServer.h"

//////////////////////////////////////////////////////////////////////
// Print command line usage.

static void
_usage(const char* const argv[], const bool listTests)
{
	printf("FAIL: Usage: %s [options] [--test=<testname>] {client | server}\n", argv[0]) ;
	printf("Options:\n") ;
	printf("  --debug\n\tEnable debug output.\n") ;
	printf("  --host=ad.dr.e.ss\n\tSpecifies the IP address for client mode to connect to.\n") ;
	printf("  --quiet\n\tDisables informational ([I]) output.\n") ;
	printf("  --iterfactor=nnn\n\tNumber of times to repeat iterative tests [default: 1].\n") ;

	if ( listTests == false )
		printf("  --tests\n\tList all of the registered tests.\n") ;
	else
	{
		printf("\nTests:\n") ;
		for ( TestItem::Index::iterator it = TestItem::s_index.begin() ; it != TestItem::s_index.end() ; ++it )
		{
			const TestItem& test = *it->second ;
			printf("--test=%s\n\t%s\n", test.name(), test.description()) ;
		}
	}
	exit(-1) ;
}

//////////////////////////////////////////////////////////////////////
// Handle command line arguments.

static void
_processArguments(const int argc, const char* const argv[])
{
	bool explicitTests = false ;

	int argn = 1 ;
	for ( ; argn < argc ; ++argn )
	{
		if ( argv[argn][0] == '-' && argv[argn][1] == '-' )
		{
			const char* arg = argv[argn] + 2 ;
			if ( strncmp(arg, "debug", 5) == 0 )
			{
				Config::debugOutput = !Config::debugOutput ;
				continue ;
			}
			else if ( strncmp(arg, "quiet", 5) == 0 )
			{
				Config::infoOutput = false ;
				continue ;
			}
			else if ( strncmp(arg, "udp", 3) == 0 )
			{
				Config::protocol = UDP ;
				continue ;
			}
			else if ( strncmp(arg, "udb", 3) == 0 )
			{
				Config::protocol = UDB ;
				continue ;
			}
			else if ( strncmp(arg, "test=", 5) == 0 )
			{
				// 2: for '--', 5 for 'test='.
				const char* const testName = (arg + 5) ;
				if ( *testName == 0 )
					_usage(argv, true) ;

				TestItem* test = TestItem::Lookup(testName) ;
				if ( test == NULL )
				{
					printf("ERROR: Unknown test: %s\n", testName) ;
					_usage(argv, true) ;
				}

				test->setEnabled(true) ;

				explicitTests = true ;

				continue ;
			}
			else if ( strncmp(arg, "tests", 5) == 0 )
			{
				_usage(argv, true) ;
			}
			else if ( strncmp(arg, "host=", 5) == 0 )
			{
				Config::serverName = arg + 5 ;

				continue ;
			}
			else if ( strncmp(arg, "iterfactor=", 11) == 0 )
			{
				Config::iterFactor = atol(arg + 11) ;
				if ( Config::iterFactor <= 0 )
				{
					printf("ERROR: Invalid iterfactor: %s\n", arg + 11) ;
					_usage(argv, false) ;
				}

				continue ;
			}
			else
				_usage(argv, false) ;
		}

		if ( Config::mode != Config::mUnknown )
			_usage(argv, false) ;

		if ( strcasecmp(argv[argn], "server") == 0 )
		{
			Config::mode = Config::mServer ;
			Dprintf("Selected server mode.\n") ;
		}
		else if ( strcasecmp(argv[argn], "client") == 0 )
		{
			Config::mode = Config::mClient ;
			Dprintf("Selected client mode.\n") ;
		}
		else
			_usage(argv, false) ;
	}

	if ( Config::mode == Config::mUnknown )
	{
		_usage(argv, false) ;
	}

	if ( explicitTests == false )
	{
		TestItem::Index::iterator it ;
		for ( it = TestItem::s_index.begin() ; it != TestItem::s_index.end() ; ++it )
		{
			it->second->setEnabled(false) ;
		}
	}

	if ( Config::debugOutput )
	{
		Dprintf("Tests:\n") ;
		TestItem::Index::iterator it ;
		for ( it = TestItem::s_index.begin() ; it != TestItem::s_index.end() ; ++it )
		{
			Dprintf("%20s Enabled: %3s Explicit: %3s\n"
						, it->second->name()
						, (it->second->isEnabled() ? "yes" : "no")
						, (it->second->isExplicitlyEnabled() ? "yes" : "no")
						) ;
		}
	}
}

static void
_doCryptTests()
{
	if ( tEncryption.isEnabled() == false )
		return ;

	Iprintf("Encryption tests:\n") ;

	teulSyncClock() ;
	srand((UINT32)teulClock64) ;

	static const size_t bufSize = 1207 ;
	UCHAR buf1[bufSize + 4] ;
	UCHAR buf2[bufSize + 4] ;
	UCHAR buf3[bufSize + 4] ;

	ENDPOINT ep ;
	EndpointImpl impl ;
	ep.m_impl = &impl ;

	for ( UINT32 version = Teul::BaseVersion ; version <= std::max((size_t)Teul::VersionNumber, (size_t)Teul::TeulV2) ; ++version )
	{
		for ( size_t i = 0 ; i < bufSize ; ++i )
		{
			buf1[i] = rand() ;
			buf2[i] = 0 ;
			buf3[i] = ~0 ;
		}

		UINT16 len1 = bufSize ;

		teulSyncClock() ;

		ep.m_impl->m_rxCrypt.newSeed() ;
		ep.m_impl->m_txCrypt.setSeed(ep.m_impl->m_rxCrypt.seed()) ;

		Dprintf("- ver %04x. rxSeed = %llu, txSeed = %llu\n", version, ep.m_impl->m_rxCrypt.seed(), ep.m_impl->m_txCrypt.seed()) ;

		memcpy(buf1, buf2, len1) ;
		ep.m_impl->m_version = ep.m_impl->m_cryptVersion = version ;
		ep.encrypt(buf2, len1) ;
		if ( version > Teul::BaseVersion && memcmp(buf1, buf2, bufSize) == 0 )
			throw Teul::Error("encryption had no effect", version) ;
		else if ( version <= Teul::BaseVersion && memcmp(buf1, buf2, bufSize) != 0 )
			throw Teul::Error("encryption should have had no effect", version) ;

		UINT16 len2 = len1 ;
		memcpy(buf3, buf2, len1) ;
		ep.decrypt(buf3, len2) ;

		if ( version >= Teul::TeulV2 && len1 != len2 )
		{
			Dprintf("len mismatch: len1:%u, len2:%u\n", len1, len2) ;
			throw Teul::Error("len1 and len2 don't match", version) ;
		}
		if ( memcmp(buf1, buf3, len2) != 0 )
			throw Teul::Error("Mismatch between encryption and decryption", version) ;
	}
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

int
main(int argc, const char* const argv[])
{
	// Handle the command line.
	_processArguments(argc, argv) ;

	int result = 0 ;
	try
	{
		_doCryptTests() ;

		Role* const role = (Config::mode == Config::mServer) ? (Role*)(new ServerRole) : (Role*)(new ClientRole) ;
		role->Start() ;
		delete role ;

		printf("SUCCESS: Execution complete.\n") ;
	}
	catch (Teul::RangeError& error)
	{
		printf("FAIL: %s [%" PRSizeT "-%" PRSizeT "]: %d\n", error.msg, error.start, error.end, error.error) ;
		result = -1 ;
	}
	catch (Teul::IterationError& error)
	{
		printf("FAIL: %s [iter #%" PRSizeT "]: %d\n", error.msg, error.iter, error.error) ;
		result = -1 ;
	}
	catch (Teul::IndexError& error)
	{
		printf("FAIL: t:%d err:%d %s\n", error.teulIndex, error.error, error.msg) ;
		result = -1 ;
	}
	catch (Teul::Error& error)
	{
		printf("FAIL: %s: %d\n", error.msg, error.error) ;
		result = -1 ;
	}
	catch (std::exception& e)
	{
		printf("FAIL: Exception: %s\n", e.what()) ;
		result = -2 ;
	}

	// This is so you don't have to set a breakpoint in visual studio to see what
	// was said in the output window on a clean & successful debug run :)
	// run with --test=anykey
	if ( tAnyKey.isExplicitlyEnabled() )
	{
		printf("\nPress enter to continue: ") ;
		getchar() ;
	}

	return result ;
}

