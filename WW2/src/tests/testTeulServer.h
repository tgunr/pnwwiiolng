//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Server code.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct ServerRole : public Role
{
	ServerRole() : Role(), m_connections(0), m_running(true) {}
	virtual ~ServerRole() {}

	//////////////////////////////////////////////////////////////
	//// Callback hooks.

	static SINT32 recvHello(ACTION_OBJECT* const a)
	{
		UINT32 randomNo = 0;
		a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &randomNo, 0);
		Iprintf("Receiving random = %u.\n", randomNo);

		if ( tSend.isEnabled() )
		{
			randomNo = rand();
			const SINT32 result = teulSend(a->teulIndex, a->actionObj, &randomNo);
			if ( result < 0 )
				throw Teul::Error("Returning random number failed", result);
		}

		Role* role = (Role*)teulGetPersonaData(0);
		if ( role )
		{
			role->TestPacket(a->teulIndex);
		}

		return a->offset;
	}

	static SINT32 sendHello(ACTION_OBJECT* const a)
	{
		UINT32* randomNo = (UINT32*)a->localRawData;
		Iprintf("Sending my random number [%u].\n", *randomNo);
		a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, (UINT32*)a->localRawData, 0);
		return a->offset;
	}

	static SINT32 sendPacketTest(ACTION_OBJECT* const /*a*/)
	{
		throw Teul::Error("Somehow, we got into sendPacketTest :(", -1);
	}

	static SINT32 recvPacketTest(ACTION_OBJECT* const a)
	{
		Iprintf("Got a Teul::Packet test of size %d\n", a->sizeInBytes());
		Role* role = (Role*)teulGetPersonaData(0);
		if ( role )
			role->ValidateTeulPacket(a->packed, a->sizeInBytes());
		return a->offset;
	}

	static SINT32 recvLargePacket(ACTION_OBJECT* const a)
	{
		Iprintf("Got large Teul::Packet, size %d bytes\n", a->sizeInBytes());
		if ( a->sizeInBytes() != (SINT32) Config::largePacketSize_bytes )
			throw Teul::Error("Large packet was wrong size", a->sizeInBytes());

		// It should contain deadbeef.
		const size_t numDeadCows = Config::largePacketSize_bytes / sizeof(unsigned int);
		unsigned int* payload = (unsigned int*)(a->packed);
		for ( size_t i = 0; i < numDeadCows; ++i )
		{
			if ( payload[i] != 0xdeadbeef )
				throw Teul::IterationError(i, "Expected deadbeef", (SINT32)payload[i]);
		}

		Dprintf("LargePacket looks good.\n");

		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::LargePacketAck_Y));
		outgoing.append((size_t)a->sizeInBytes());
		outgoing.send(*a->endpoint);

		return a->offset;
	}

	static SINT32 recvShutdown(ACTION_OBJECT* const a)
	{
		ServerRole* role = (ServerRole*)teulGetPersonaData(0);
		if ( role != nullptr )
		{
			Iprintf("Got shutdown.\n");
			role->m_running = false;
		}
		return a->offset;
	}

	static SINT32 recvUnencrypted(ACTION_OBJECT* const a)
	{
		if ( strcmp((const char*)a->packed, (const char*)"unencrypted") != 0 )
			throw Teul::Error("Unencrypted data was bad", -1 );
		Dprintf("Unencrypted data looked good, sending some back\n");

		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::Unencrypted_Y));
		outgoing.setUnencrypted();
		outgoing.append("unencrypted", strlen("unencrypted") + 1);
		outgoing.send(*a->endpoint);

		return a->offset;
	}

	static SINT32 sendUnencrypted(ACTION_OBJECT* const /*a*/)
	{
		throw Teul::Error("Somehow we got into sendUnencrypted", -1);
	}

	static SINT32 recvTestObject(ACTION_OBJECT* a, TestObject& msg)
	{
		if ( memcmp(&msg, &s_sourceTestObject, sizeof(msg)) != 0 )
			throw Teul::Error("Received invalid test object.", -EINVAL);

		Iprintf("Received valid test object (%u bytes) - sending reply\n", (UINT32)sizeof(msg));

		Teul::Packet outgoing(Teul::PacketId(Role::Test_X, Role::ObjTest_Y), s_sourceTestObject);
		outgoing.send(*a->endpoint);

		return a->offset;
	}

	virtual int RegisterCallbacks() OVERRIDE_FN
	{
		BEGIN_CALLBACK_REGISTRATION(Role::Test_X)
			REGISTER_RECVSEND(Role::Hello_Y, recvHello, sendHello, s_cbHello);

			REGISTER_RECVSEND(Role::HelloRel_Y, recvHello, sendHello, s_cbRelHello);

			REGISTER_RECVSEND(Role::PacketTest_Y, recvPacketTest, sendPacketTest, s_cbPacketTest);
			REGISTER_RECEIVER(Role::LargePacket_Y, recvLargePacket);
			REGISTER_SENDRECV(Role::Unencrypted_Y, sendUnencrypted, recvUnencrypted, s_cbUnencrypted);

			REGISTER_RECEIVER(Role::Shutdown_Y, recvShutdown);

			REGISTER_SENDRECV(Role::Echo_Y, EchoPacketHeader::_send, EchoPacketHeader::_recv, s_cbEcho);

			REGISTER_RECEIVEOBJ(Role::ObjTest_Y, TestObject, recvTestObject);

			REGISTER_RECEIVER(Role::Spam_Y, [](ACTION_OBJECT* a) { return a->offset; });
		END_CALLBACK_REGISTRATION
	}

	//////////////////////////////////////////////////////////////
	// Called by teulkit when a connection has been accepted.
	// We look up the appropriate server and forward it to the
	// Accept() member of that instance.

	static void acceptFn(ENDPOINT& endpoint)
	{
		if ( endpoint.m_parent == nullptr )
			throw Teul::Error("Accepted endpoint doesn't have a parent!", endpoint.teulIndex());
		ServerRole* const server = (ServerRole*)teulGetPersonaData(endpoint.m_parent->teulIndex());
		if ( server == nullptr )
			throw Teul::Error("Incoming connection's teulIndex doesn't appear to match a server", endpoint.m_parent->teulIndex());

		server->Accept(endpoint);
	}

	void Accept(ENDPOINT& endpoint)
	{
		Iprintf("t:%d Accepting connection t:%d.\n", endpoint.m_parent->teulIndex(), endpoint.teulIndex());
		++m_connections;
	}

	//////////////////////////////////////////////////////////////
	// Open the listen interface.

	void OpenListener()
	{
		const SINT32 teulIndex = teulOpenListen(Config::TestPort, acceptFn, Config::ConnectTimeoutMS, Config::protocol, errorFn);
		if ( teulIndex < 0 )
			throw Teul::Error("teulOpenListen failed", teulIndex);

		m_teulIndex = teulIndex;
		Dprintf("t:%d teulOpenListen() index.\n", m_teulIndex);

		teulSetPersonaData(m_teulIndex, this);
		Dprintf("t:%d associated with Server %p.\n", m_teulIndex, this);
	}

	//////////////////////////////////////////////////////////////
	// Wait for an incoming connection.

	void ServerLoop()
	{
		if ( tTimeouts.isEnabled() )
			std::this_thread::sleep_for(std::chrono::milliseconds(Config::ConnectTimeoutMS * 2));

		Iprintf("Listening...\n");
		while ( m_running )
		{
			// Sit around letting teulkit do it's thing for 100ms.
			teulUpdateTransport(100);
		}
	}

	//////////////////////////////////////////////////////////////
	// Main body of the server role.

	virtual void Run() OVERRIDE_FN
	{
		OpenListener();

		ServerLoop();
	}

	// Count of number of connections.
	UINT32	m_connections;

	// Flag indicating whether we are running or not.
	bool	m_running;
};

