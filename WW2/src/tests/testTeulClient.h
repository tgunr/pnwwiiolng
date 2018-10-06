//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Client code.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

static std::vector<SINT32> g_bulkConnections;
static bool g_inBulkTest = false;

namespace Teul
{
    inline ENDPOINT *GetEndpointIfConnected(const SINT32 teulIndex)
    {
        ENDPOINT *ep = teulGetEndpoint(teulIndex);
        
        return ep && ep->state() == IS_OPEN && !ep->m_closed && ep->m_connected ? ep : nullptr;
    }
}

struct ClientRole : public Role
{
	public:
	ClientRole() : Role(), m_connected(false) {}
	virtual ~ClientRole() {}

	//////////////////////////////////////////////////////////////
	//// Callback hooks.

	static SINT32 sendHello(ACTION_OBJECT* const a)
	{
		UINT32* randomNo = (UINT32*)a->localRawData;
		Iprintf("Sending randomNo = %u\n", *randomNo);
		a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, randomNo, 0);

		return a->offset;
	}

	static SINT32 recvHello(ACTION_OBJECT* const a)
	{
		UINT32 randomNo = 0;
		a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &randomNo, 0);
		Iprintf("Receiving randomNo = %u, a->packed = %p, a->localRawData = %p.\n", randomNo, a->packed, a->localRawData);

		sm_gotHello = true;

		return a->offset;
	}

	static SINT32 sendPacketTest(ACTION_OBJECT* const /*a*/)
	{
		throw Teul::Error("Somehow, we got into sendPacketTest :(", -1);
		return -EINVAL;
	}

	static SINT32 recvPacketTest(ACTION_OBJECT* const a)
	{
		Iprintf("Receiving Teul::Packet, size:%u\n", a->sizeInBytes());
		Role* role = (Role*)teulGetPersonaData(a->teulIndex);
		if ( role )
			role->ValidateTeulPacket(a->packed, a->sizeInBytes());
		return a->offset;
	}

	static SINT32 recvLargePacketAck(ACTION_OBJECT* const a)
	{
		size_t size = a->read<size_t>();
		Iprintf("Received large packet ack, for size %zu\n", size);
		sm_gotLargePacketAck = true;
		return a->offset;
	}

	static SINT32 sendLargePacket(ACTION_OBJECT* const /*a*/)
	{
		throw Teul::Error("Somehow we got into sendLargePacket", -1);
	}

	static SINT32 sendUnencrypted(ACTION_OBJECT* const /*a*/)
	{
		throw Teul::Error("Somehow we got into sendUnencrypted", -1);
	}

	static SINT32 recvUnencrypted(ACTION_OBJECT* const a)
	{
		if ( strcmp((const char*)a->packed, (const char*)"unencrypted") != 0 )
			throw Teul::Error("Unencrypted data was bad", -1);
		Iprintf("Received unencrypted data OK\n");
		sm_gotUnencrypted = true;
		return a->offset;
	}

	static SINT32 recvTestObject(ACTION_OBJECT* a, TestObject& msg)
	{
		if ( memcmp(&msg, &s_sourceTestObject, sizeof(msg)) != 0 )
			throw Teul::Error("Received invalid test object.", -EINVAL);
		Iprintf("Received test object - looked good (%u bytes)\n", (UINT32)sizeof(msg));
		sm_gotTestObject = true;
		return a->offset;
	}

	virtual int RegisterCallbacks() override
	{
		BEGIN_CALLBACK_REGISTRATION(Role::Test_X)
		{
			REGISTER_RECVSEND(Role::Hello_Y, recvHello, sendHello, s_cbHello);

			REGISTER_RECVSEND(Role::HelloRel_Y, recvHello, sendHello, s_cbRelHello);

			REGISTER_RECEIVER(Role::PacketTest_Y, recvPacketTest);

			REGISTER_SENDRECV(Role::Unencrypted_Y, sendUnencrypted, recvUnencrypted, s_cbUnencrypted);

			REGISTER_SENDER(Role::Shutdown_Y, CallbackRegister::NoDataHandler, s_cbShutdown);

			REGISTER_SENDRECV(Role::Echo_Y, EchoPacketHeader::_send, EchoPacketHeader::_recv, s_cbEcho);

			REGISTER_RECEIVEOBJ(Role::ObjTest_Y, TestObject, recvTestObject);

			REGISTER_SENDER(Role::Unregistered_Y, CallbackRegister::NoDataHandler, s_cbUnregistered);

			REGISTER_RECEIVER(Role::LargePacketAck_Y, recvLargePacketAck);
		}
		END_CALLBACK_REGISTRATION(Role::Test_X)
	}

	//////////////////////////////////////////////////////////////
	//// Internal test functions.

	private:
	UINT32 hostAddress;

	private:
	void GetHostAddress()
	{
		// Test hostname lookup.
		UINT32 address = Teul::GetSingleIP(Config::serverName);
		if ( address == 0U || address == ~0U )
			throw Teul::Error("Teul::GetSingleIP failed", address);
		hostAddress = address;
	}

	//////////////////////////////////////////////////////////////
	// Wait for a little longer than the timeout value, to see if
	// the connection dies. (Keepalive test)

	private:
	void Delay()
	{
		if ( tTimeouts.isEnabled() == false )
			return;

		const TeulTimeMS delay = (2 * Config::ConnectTimeoutMS) + 500ULL;
		Dprintf("Keepalive check: %" PR64BitUT " delay.\n", delay);
		teulUpdateTransport(delay);
	}

	//////////////////////////////////////////////////////////////
	// Handle errors during bulk open.

	private:
	static void bulkErrorFn(SINT32 teulIndex, const UCHAR* const, SINT32, const TEUL_CALLBACK*, const SINT32 error)
	{
		if ( error != ENOTCONN )
		{
			throw Teul::IndexError(teulIndex, error);
		}
	}

	//////////////////////////////////////////////////////////////
	// Register which indexes have been opened by the bulk test.

	private:
	static void bulkConnectFn(ENDPOINT& endpoint)
	{
		if ( g_inBulkTest == false )
			throw Teul::Error("Got a bulk connection outside of the bulk test", endpoint.teulIndex());
		g_bulkConnections.push_back(endpoint.teulIndex());
	}

	//////////////////////////////////////////////////////////////
	// Send an echo request.

	private:
	void sendEchoRequest(SINT32 teulIndex, UINT32 type = 0)
	{
		EchoPacketHeader outgoing;
		outgoing.m_direction = EchoPacketHeader::Request;

		if ( type == 0 || type >= EchoPacketHeader::tMax )
			type = (rand() % (EchoPacketHeader::tMax - 1)) + 1;

		outgoing.m_type = type;
		outgoing.m_size = rand();
		outgoing.m_count = 64 + rand() % 64;
		memset(outgoing.m_data, 0, sizeof(outgoing.m_data));

		if ( (EchoPacketHeader::s_requested % 1000) == 0 )
		{
			Iprintf("Echo request #%" PR64BitUT " type %u count %u\n", EchoPacketHeader::s_requested, outgoing.m_type, outgoing.m_count);
		}
		else
		{
			Dprintf("Echo request #%" PR64BitUT " type %u count %u\n", EchoPacketHeader::s_requested, outgoing.m_type, outgoing.m_count);
		}
		teulSend(teulIndex, s_cbEcho, &outgoing);
	}

	//////////////////////////////////////////////////////////////
	// Wait for echo replies.

	private:
	void catchupEchoPackets()
	{
		TeulTimeMS cutoff = teulClock64 + 5000;
		while ( teulClock64 < cutoff && EchoPacketHeader::s_requested > EchoPacketHeader::s_acked )
		{
			teulUpdateTransport(1);
		}

		const SINT32 diff = (SINT32)(EchoPacketHeader::s_requested - EchoPacketHeader::s_acked);
		if ( diff < 0 )
			throw Teul::Error("Too many echo replies", -diff);
		if ( diff > 0 )
			throw Teul::Error("Missing echo replies", diff);
	}

	//////////////////////////////////////////////////////////////
	// Test echo packets - send thousands of them.

	private:
	void TestEchoPackets()
	{
		if ( tEchoTest.isEnabled() == false )
			return;

		Iprintf("Testing echo packets\n");

		for ( size_t iter = 0; iter < Config::iterFactor; ++iter )
		{
			if ( iter > 0 )
				Iprintf("- Iter #%" PRSizeT "\n", iter);

			for ( size_t i = 0; i < 1000; ++i )
			{
				catchupEchoPackets();
				for ( UINT32 type = 1; type < EchoPacketHeader::tMax; ++type )
				{
					const size_t packetsToSend = (rand() % 3) + 1;
					for ( size_t p = 0; p < packetsToSend; ++p )
					{
						sendEchoRequest(0, type);
					}

					std::this_thread::sleep_for(std::chrono::microseconds(50 + rand() % 50));
					teulUpdateTransport(0);
				}
			}
		}

		catchupEchoPackets();

		Iprintf("%" PR64BitUT " echo packets sent and acked\n", EchoPacketHeader::s_requested);
	}

	//////////////////////////////////////////////////////////////
	// Test tearing up, connection and closure of a lot of
	// connections. This is resource intensive, and is designed
	// to try and deplete resources if things aren't working
	// smoothly.

	private:
	void BulkOpenTest()
	{
		if ( tBulkOpen.isEnabled() == false )
			return;

		static const size_t ConnectionsToTest = 2000;
		static const size_t ConnectionsAtATime = 20;

		Iprintf("Testing Open/Close %" PRSizeT " connections %" PRSizeT " at a time\n", ConnectionsToTest, ConnectionsAtATime);

		g_bulkConnections.reserve(ConnectionsAtATime);
		std::vector<SINT32> expectedIndexes;
		expectedIndexes.reserve(ConnectionsAtATime);

		teulSyncClock();
		const UINT64 startTimeMS = teulClock64;

		// Count the amount of time spent on the opens themselves
		// without factoring in the time it takes to close the
		// connections again.
		// We can calculate the average time to open by dividing
		// by the number of batches etc.
		UINT64 totalOpenTimesMS = 0ULL;

		g_inBulkTest = true;

		const bool echoTest = tEchoTest.isEnabled();

		for ( size_t iter = 0; iter < Config::iterFactor; ++iter )
		{
			for ( size_t i = 0; i < ConnectionsToTest; i += ConnectionsAtATime )
			{
				if ( (i % 1000) == 0 )
					Dprintf("-- %" PRSizeT ":%" PRSizeT "\n", iter, i);

				expectedIndexes.clear();
				g_bulkConnections.clear();

				// Open up a batch of connections, store the indexes in the expected
				// index vector, and collect the "opened" messages in the bulk
				// vector so we can confirm we got opened the right indexes.
				const size_t maxIndex = i + ConnectionsAtATime - 1;
				for ( size_t connection = i; connection <= maxIndex; ++connection )
				{
					if ( echoTest && rand() % 15 == 0 )
						sendEchoRequest(0);

					SINT32 newIndex = teulOpenSend(hostAddress, Config::TestPort, bulkConnectFn, Config::ConnectTimeoutMS, Config::protocol, bulkErrorFn);
					if ( newIndex < 0 )
						throw Teul::IterationError(connection, "teulOpenSend failed", newIndex);
					expectedIndexes.push_back(newIndex);
				}

				teulSyncClock();
				const UINT64 batchStartTimeMS = teulClock64;
				const UINT64 cutoff = batchStartTimeMS + (Config::ConnectTimeoutMS * 2);

				// OGS: I'm using goto here as an optimization, because of the weird
				// set of conditions for terminating this loop. While writing this,
				// I found an unfortunate set of cache-misses introduced by trying
				// to write this cleanly with GCC 4.5 that massively impacted the
				// performance of the loop (by a factor of 175%).
				{
	loop:
					teulUpdateTransport(0);
					if ( Teul::GetEndpointIfConnected(0) == nullptr )
						throw Teul::Error("Connection 0 closed unexpectedly.", 0);
					if ( Teul::GetEndpointIfConnected(1) == nullptr )
						throw Teul::Error("Connection 1 closed unexpectedly.", 1);

					if ( g_bulkConnections.size() < expectedIndexes.size() )
					{
						if ( teulClock64 > cutoff )
							throw Teul::RangeError(i, maxIndex, "Bulk Open: Timed out, connects", g_bulkConnections.size());
						goto loop;		/// <<< GOTO: stay in the loop till we get all the indexes.
					}
				}

				const UINT64 batchEndTimeMS = teulClock64;

				totalOpenTimesMS += batchEndTimeMS - batchStartTimeMS;

				// Check that we got opens for all of the connections.
				if ( g_bulkConnections.size() > expectedIndexes.size() )
					throw Teul::RangeError(i, maxIndex, "Bulk Open: Too many connections", g_bulkConnections.size());

				//
				std::sort(g_bulkConnections.begin(), g_bulkConnections.end());
				std::sort(expectedIndexes.begin(), expectedIndexes.end());

				for ( size_t entry = 0; entry < expectedIndexes.size(); ++entry )
				{
					const SINT32 expectIndex = expectedIndexes[entry];
					const SINT32 gotIndex = g_bulkConnections[entry];
					if ( gotIndex != expectIndex )
						throw Teul::RangeError(i, maxIndex, "Bulk Open: Unexpected connection registered", gotIndex);
					teulClose(gotIndex);
				}
			}
		}

		teulSyncClock();
		const UINT64 endTimeMS = teulClock64;

		g_inBulkTest = false;

		// Give things time to clear up.
		teulUpdateTransport(500);

		const TeulTimeMS totalElapsedMS = endTimeMS - startTimeMS;
		const TeulTimeMS avgBatchMS = totalOpenTimesMS / (ConnectionsToTest / ConnectionsAtATime);
		Iprintf("Done [total: %" PR64BitUT "ms, avg batch time: %" PR64BitUT "ms]!\n", totalElapsedMS, avgBatchMS);
	}

	private:
	void WaitForHelloReply()
	{
		sm_gotHello = false;

		if ( tRecv.isEnabled() == false )
			return;

		Dprintf("Waiting %ums for a response.\n", Config::ConnectTimeoutMS);

		teulSyncClock();
		const TeulTimeMS timeout = teulClock64 + Config::ConnectTimeoutMS;

		while ( teulClock64 < timeout && sm_gotHello == false )
		{
			teulUpdateTransport(5);
		}

		if ( sm_gotHello == false )
			throw Teul::Error("Failed to receive response to our hello", 0);
	}

	private:
	void SendRandom(const SINT32 teulIndex, TEUL_CALLBACK* const callback, const char* const detail)
	{
		if ( tSend.isEnabled() == false )
			return;

		UINT32 randomNo = rand();
		Iprintf("Sending [%u] %s\n", randomNo, detail);
		SINT32 result = teulSend(teulIndex, callback, &randomNo);
		if ( result < 0 )
			throw Teul::IndexError(teulIndex, "SendRandom failed", result);

		WaitForHelloReply();

		Delay();
	}

	private:
	void LongDelay()
	{
		if ( tLongDelay.isEnabled() == false )
			return;

		Iprintf("Long delay (5 minutes).\n");
		const TeulTimeMS cutoff = teulClock64 + 5 * 60 * 1000;
		while ( teulClock64 <= cutoff )
		{
			if ( s_lastError != 0 )
				throw Teul::Error("Unexpected s_lastError", s_lastError);
			teulUpdateTransport(50);
		}

		Dprintf("Still connected, looks good.\n");

		Delay();
	}

	private:
	void TestDeadEndpointCollection(const bool firstConnIsOpen, const bool secondConnIsOpen)
	{
		if ( tDeadEp.isEnabled() == false )
			return;

		Iprintf("Recycling dead endpoints a few times.\n");
		teulUpdateTransport(5);
		for ( size_t iter = 0; iter < Config::iterFactor; ++iter )
		{
			for ( size_t i = 0; i < 16; ++i )
			{
				if ( firstConnIsOpen && tEchoTest.isEnabled() )
					sendEchoRequest(0);

				teulUpdateTransport(1);
				teulKillDeadEndpoints();

				// Check the endpoints, see where they are at.
				ENDPOINT* endpoint;

				endpoint = Teul::GetEndpointIfConnected(0);
				if ( (endpoint != nullptr) != firstConnIsOpen )
					throw Teul::IterationError(i, "First connection in wrong state", 0);
				if (!firstConnIsOpen)
					dbugLog(_FLI, "[ignore 'endpoint %d is not in use']", 0);

				endpoint = Teul::GetEndpointIfConnected(1);
				if ( (endpoint != nullptr) != secondConnIsOpen )
					throw Teul::IterationError(i, "Second connection in wrong state", 0);
				if (!secondConnIsOpen)
					dbugLog(_FLI, "[ignore 'endpoint %d is not in use']", 1);
			}
		}
	}

	private:
	void WaitForLargePacketReply()
	{
		sm_gotLargePacketAck = false;

		if ( tRecv.isEnabled() == false )
			return;

		Dprintf("Waiting %ums for large packet reply.\n", Config::ConnectTimeoutMS);

		teulSyncClock();
		const TeulTimeMS timeout = teulClock64 + Config::ConnectTimeoutMS;

		while ( teulClock64 < timeout && sm_gotLargePacketAck == false )
		{
			teulUpdateTransport(5);
		}

		if ( sm_gotLargePacketAck == false )
			throw Teul::Error("Failed to receive response to our large packet", 0);
	}


	public:
	void TestLargePacket(const SINT32 teulIndex)
	{
		// Have to enable both teulpkt and largepkt.
		if ( (tTeulPkt.isEnabled() & tLargePkt.isEnabled()) == false )
			return;

		Dprintf("preparing large Teul::Packet\n");
		Teul::Packet packet(Teul::PacketId(Role::Test_X, Role::LargePacket_Y));
		if (packet.payloadSizeBytes() != 0)
			throw Teul::Error("Expected packet to have 0 size after construction.", packet.payloadSizeBytes());
		if (packet.packetCapacityBytes() < packet.headerSizeBytes())
			throw Teul::Error("Expected packet to have headerSizeBytes capacity after construction.", packet.packetCapacityBytes());
		if (packet.packetCapacityBytes() != Teul::Packet::DefaultLocalDataSize)
			throw Teul::Error("Expected packet to have Default capacity after construction.", packet.packetCapacityBytes());
		packet.reserve(Config::largePacketSize_bytes);
		// We specified a capacity, so payloadSize should be zero.
		if ( packet.payloadSizeBytes() != 0 )
			throw Teul::Error("Expected packet to have 0 size after reservation.", packet.payloadSizeBytes());
		if (packet.packetCapacityBytes() < Config::largePacketSize_bytes)
			throw Teul::Error("Expected packet to have at least largePacketSize capacity after reservation.", packet.packetCapacityBytes());

		const size_t originalCapacity = packet.payloadCapacityBytes();

		Dprintf("populating large packet with deadbeef\n");
		const size_t numDeadCows = Config::largePacketSize_bytes / sizeof(unsigned int);
		for ( size_t i = 0; i < numDeadCows; ++i )
		{
			packet.append((unsigned int)0xdeadbeef);
		}

		if ( packet.payloadSizeBytes() != Config::largePacketSize_bytes )
			throw Teul::Error("largePacket payload size is not requested size", packet.payloadSizeBytes());
		// Make sure it didn't grow.
		if ( packet.payloadCapacityBytes() != originalCapacity )
			throw Teul::Error("Capacity changed when changing size.", packet.payloadCapacityBytes());

		const size_t psb = packet.payloadSizeBytes();
		const SINT32 result = packet.send(teulIndex);
		if ( result < 0 )
			throw Teul::Error("Teul::Packet send failed", result);

		Iprintf("Large packet sent, %" PRSizeT " bytes.\n", psb);

		WaitForLargePacketReply();
	}

	private:
	void WaitForUnencryptedReply()
	{
		sm_gotUnencrypted = false;

		if ( tUnencrypted.isEnabled() == false )
			return;

		Dprintf("Waiting %ums for a response.\n", Config::ConnectTimeoutMS);

		teulSyncClock();
		const TeulTimeMS timeout = teulClock64 + Config::ConnectTimeoutMS;

		while ( teulClock64 < timeout && sm_gotUnencrypted == false )
		{
			teulUpdateTransport(5);
		}

		if ( sm_gotUnencrypted == false )
			throw Teul::Error("Failed to receive response to our unencrypted packet", 0);
	}

	public:
	void TestUnencrypted(const SINT32 firstIndex, const SINT32 secondIndex)
	{
		if ( (tTeulPkt.isEnabled() & tUnencrypted.isEnabled()) == false )
			return;

		Dprintf("testing unencrypted send\n");

		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::Unencrypted_Y));
		outgoing.setUnencrypted();
		outgoing.append("unencrypted", strlen("unencrypted") + 1);

		outgoing.send(firstIndex);
		WaitForUnencryptedReply();

		outgoing.send(secondIndex);
		WaitForUnencryptedReply();
	}

	private:
	void WaitForTestObjectReply()
	{
		sm_gotTestObject = false;

		if ( tObjTest.isEnabled() == false )
			return;

		Dprintf("Waiting %ums for a returned test object.\n", Config::ConnectTimeoutMS);

		teulSyncClock();
		const TeulTimeMS timeout = teulClock64 + Config::ConnectTimeoutMS;

		while ( teulClock64 < timeout && sm_gotTestObject == false )
		{
			teulUpdateTransport(5);
		}

		if ( sm_gotTestObject == false )
			throw Teul::Error("Failed to receive response to our test object", 0);
	}

	public:
	void TestObjectMethod(const SINT32 teulIndex)
	{
		if ( tObjTest.isEnabled() == false )
			return;

		Dprintf("sending object-method test packets\n");

		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::ObjTest_Y), s_sourceTestObject);
		outgoing.send(teulIndex);
		WaitForTestObjectReply();
	}

	private:
	void TestSendUnregistered(const SINT32 teulIndex)
	{
		// This callback is not registered on the server as a receiver,
		// to check this behavior doesn't crash the server.
		if ( tSendUnreg.isEnabled() == false )
			return;

		Dprintf("sending unregistered callback\n");

		teulSend(teulIndex, s_cbUnregistered, nullptr);
	}

	public:
	void TestVaryingPacketSizes(const SINT32 teulIndex)
	{
		if (tSendVariety.isEnabled() == false)
			return;
		size_t numMsgs = 5;
		// Summary: Packet size of 11584 is largest (payload 11576) that can be sent without error
		size_t msgSizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
		size_t numSizes = sizeof(msgSizes) / sizeof(size_t);

		Iprintf("sending %" PRSizeT " messages for each varing size\n", numMsgs);

		uint8_t nextChar = static_cast<uint8_t>('A');
		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::Spam_Y));
		outgoing.setUnencrypted();
		outgoing.reserve(70000);
		for (size_t i = 0; i < numSizes; ++i)
		{
			for (size_t j = 0; j < msgSizes[i]; ++j)
			{
				outgoing.append(nextChar);
				if (++nextChar > static_cast<uint8_t>('Z'))
				nextChar = static_cast<uint8_t>('A');
			}

			Iprintf("sending %" PRSizeT " messages for packet size %" PRSizeT "\n", numMsgs, msgSizes[i]);
			for (size_t j = 0; j < numMsgs; ++j)
			{
				outgoing.send(teulIndex);
			}
			const TeulTimeMS cutoff = teulClock64 + 250;
			while ( teulClock64 <= cutoff )
			{
				if ( s_lastError != 0 )
					throw Teul::Error("Unexpected s_lastError", s_lastError);
				teulUpdateTransport(50);
			}
		}
	}

	public:
	void TestSpam(const SINT32 teulIndex, const UINT32 numUnits, size_t size)
	{
		if (tSendSpam.isEnabled() == false)
			return;

		Iprintf("sending %u spam messages\n", numUnits);

		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::Spam_Y));
		outgoing.setUnencrypted();
		outgoing.reserve(size);
		for (size_t i = 0; i < size; ++i)
		{
			outgoing.append(UCHAR(rand() % 255));
		}

		for (size_t i = 0; i < numUnits; ++i)
		{
			outgoing.send(teulIndex);
		}
	}

	public: virtual void Run() override
	{
		OpenConnection();

		if ( tStallConnect.isEnabled() )
		{
			Iprintf("Stalling connection for %u seconds\n", Config::stallTime_s);
			std::this_thread::sleep_for(std::chrono::seconds(Config::stallTime_s));
		}

		WaitForConnectionToComplete(m_teulIndex);

		Delay();

		SendRandom(m_teulIndex, s_cbHello, "via unrel");
		SendRandom(m_teulIndex, s_cbRelHello, "via rel");

		TestSendUnregistered(m_teulIndex);

		Delay();

		TestDeadEndpointCollection(true, false);

		Dprintf("Opening second connection.\n");
		const SINT32 secondIndex = teulOpenSend(hostAddress, Config::TestPort, connect2Fn, Config::ConnectTimeoutMS, Config::protocol, errorFn);
		if ( secondIndex < 0 )
			throw Teul::Error("teulOpen failed", secondIndex);

		WaitForConnectionToComplete(secondIndex);

		Delay();

		SendRandom(secondIndex, s_cbHello, "via 2nd connection");

		TestPacket(m_teulIndex);

		Delay();

		TestLargePacket(m_teulIndex);

		Delay();

		TestLargePacket(secondIndex);

		Delay();

		TestObjectMethod(m_teulIndex);

		Delay();

		BulkOpenTest();

		LongDelay();

		TestDeadEndpointCollection(true, true);

		Delay();

		TestUnencrypted(0, 1);

		Delay();

		TestEchoPackets();

		Delay();

		TestObjectMethod(secondIndex);

		Delay();

		TestVaryingPacketSizes(m_teulIndex);

		Delay();

		TestSpam(m_teulIndex, 1024, 256);
		TestSpam(secondIndex, 1024, 512);
		TestSpam(m_teulIndex, 1024, 4096);
		TestSpam(secondIndex, 1024, 8192);
		TestSpam(m_teulIndex, 1024, 16384);

		Iprintf("Closing second connection.\n");
		teulClose(secondIndex);

		TestDeadEndpointCollection(true, false);

		SendRandom(m_teulIndex, s_cbHello, "final via unrel");
		SendRandom(m_teulIndex, s_cbRelHello, "final via rel");

		if ( Teul::GetEndpointIfConnected(1) != nullptr )
			throw Teul::IndexError(1, "Second endpoint is still showing 'connected' after close", 0);
		if ( Teul::GetEndpointIfConnected(0) == nullptr )
			throw Teul::IndexError(0, "We lost the primary endpoint", 0);

		// If the "killserver" test is enabled, we send the "shutdown" callback.
		if ( tKillServer.isEnabled() )
		{
			Iprintf("Sending KILLSERVER.\n");
			teulSend(m_teulIndex, s_cbShutdown, nullptr);
			teulUpdateTransport(250);	// Give it time to send.
		}
	}

	static void connectFn(ENDPOINT& endpoint)
	{
		ClientRole* const client = (ClientRole*)endpoint.appConnectionData;
		if ( client == nullptr )
			throw Teul::Error("Connection on index that isn't associated with a Client", endpoint.teulIndex());
		client->Connected();
	}

	static void connect2Fn(ENDPOINT& endpoint)
	{
		Dprintf("t:%d secondary connection connected\n", endpoint.teulIndex());
	}

	void Connected()
	{
		if ( m_connected )
			throw Teul::Error("Got duplicate 'connected'", m_teulIndex);

		m_connected = true;
		Dprintf("t:%d connected\n", m_teulIndex);
	}

	// Open (initiate) the connection to the server.
	// Note: this is a TEUL connection, the underlying
	// socket connection isn't complete yet.
	void OpenConnection()
	{
		GetHostAddress();
		// Now attempt to open a connection to the server.
		const SINT32 teulIndex = teulOpenSend(hostAddress, Config::TestPort, Config::protocol == PROTOCOL_UDB ? nullptr : connectFn, Config::ConnectTimeoutMS, Config::protocol, errorFn);
		if ( teulIndex < 0 )
			throw Teul::Error("teulOpen failed", teulIndex);

		m_teulIndex = teulIndex;
		Dprintf("teulOpenSend() index %d\n", m_teulIndex);

		teulSetPersonaData(m_teulIndex, this);
		Dprintf("t:%d associated with Client %p\n", m_teulIndex, this);
	}

	// Give the connection time to complete.
	void WaitForConnectionToComplete(SINT32 teulIndex)
	{
		// Set a maximum wait time.
		const TeulTimeMS startTime = teulClock64;
		const TeulTimeMS cutoff = teulClock64 + Config::ConnectTimeoutMS + 100;

		while ( s_lastError == 0 && Teul::GetEndpointIfConnected(teulIndex) == nullptr && teulClock64 <= cutoff )
		{
			// Loiter for 100ms letting teulKit do it's thing.
			teulUpdateTransport(5);
		}

		const TeulTimeMS delta = teulClock64 - startTime;
		if ( false == m_connected )
		{
			if ( s_lastError != 0 )
				throw Teul::Error("Connect failed", s_lastError);
			throw Teul::Error("Connection timed out", (SINT32)delta);
		}

		Dprintf("t:%d Getting connected took %" PR64BitUT "ms.\n", teulIndex, delta);
	}

	// Track whether we are currently connected to our primary connection or not.
	bool m_connected;

	// Track whether we have received a "hello".
	static bool sm_gotHello;
	static bool sm_gotUnencrypted;
	static bool sm_gotTestObject;
	static bool sm_gotLargePacketAck;
};

bool ClientRole::sm_gotHello = false;
bool ClientRole::sm_gotUnencrypted = false;
bool ClientRole::sm_gotTestObject = false;
bool ClientRole::sm_gotLargePacketAck = false;

