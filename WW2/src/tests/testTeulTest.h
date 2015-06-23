#pragma once

class TestItem
{
private:
	const char* const m_name;
	const char* const m_description;
	bool m_enabled;
	bool m_explicitlySet;

public:
	typedef std::map<std::string /*name*/, TestItem*> Index;
	static Index s_index;

public:
	TestItem(const char* const name_, const char* const description_, bool defaultEnabled=true)
		: m_name(name_)
		, m_description(description_)
		, m_enabled(defaultEnabled)
		, m_explicitlySet(false)
		{
			s_index.insert(std::make_pair(m_name, this));
		}

	static TestItem* Lookup(const char* const name_)
	{
		TestItem::Index::iterator it = s_index.find(std::string(name_));
		return ( it != s_index.end() ) ? it->second : nullptr;
	}

public:
	const char* name() const { return m_name; }
	const char* description() const { return m_description; }

	bool isEnabled() const { return m_enabled; }
	bool isExplicitlySet() const { return m_explicitlySet; }

	void setEnabled()
	{
		m_enabled = true;
		m_explicitlySet = true;
	}
	void setDisabled()
	{
		m_enabled = false;
		m_explicitlySet = true;
	}
};

TestItem::Index TestItem::s_index;

// Declare tests here.

static TestItem tRecv("recv", "Test that receive functionality works");
static TestItem tSend("send", "Test that sending works");
static TestItem tSendSpam("sendspam", "Test burst sending of a large number of packets");

static TestItem tTimeouts("timeouts", "Put delays between tests to check keepalives & timeouts", false);
static TestItem	tDeadEp("deadep", "Dead endpoint garbage collection test");

static TestItem	tBulkOpen("bulkopen", "Cycle thousands of connections to test open/close functionality en-mass");
static TestItem tLongDelay("longdelay", "Keepalive/timeout test with a very long pause");

static TestItem tTeulPkt("teulpkt", "Basic validation of Teul::Packet functionality");
static TestItem tLargePkt("largepkt", "Test Teul::Packet with larger-than-teul packet sizes");
static TestItem tUnencrypted("unencrypted", "Validate send/receive of unencrypted Teul::Packet data");

static TestItem tStallConnect("stallconnect", "Deliberately stall client's initial open", false);

static TestItem tAnyKey("anykey", "Prompt user to press any key on exit (for visual studio)", false);
static TestItem tKillServer("killserver", "[client] Send kill message to server on completion", false);

static TestItem tEncryption("encrypt", "Test encryption algorithms");

static TestItem tEchoTest("echotest", "Send thousands of packets and wait for the server to return them");

static TestItem tObjTest("object", "Test the receive object handler");
static TestItem tSendUnreg("sendunreg", "Test sending a callback not registered by the server");
