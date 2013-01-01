#pragma once

class TestItem
{
private:
	const char* const m_name ;
	const char* const m_description ;
	bool m_enabled ;
	bool m_explicitlyEnabled ;

public:
	typedef std::map<std::string /*name*/, TestItem*> Index ;
	static Index s_index ;

public:
	TestItem(const char* const name_, const char* const description_)
		: m_name(name_)
		, m_description(description_)
		, m_enabled(false)
		, m_explicitlyEnabled(false)
		{
			s_index.insert(std::make_pair(m_name, this)) ;
		}

	static TestItem* Lookup(const char* const name_)
	{
		TestItem::Index::iterator it = s_index.find(std::string(name_)) ;
		return ( it != s_index.end() ) ? it->second : NULL ;
	}

public:
	const char* name() const { return m_name ; }
	const char* description() const { return m_description ; }

	bool isEnabled() const { return m_enabled ; }
	bool isExplicitlyEnabled() const { return m_explicitlyEnabled ; }

	void setEnabled(bool explicitly)
	{
		m_enabled = true ;
		m_explicitlyEnabled = explicitly ;
	}
} ;

TestItem::Index TestItem::s_index ;

// Declare tests here.

static TestItem tRecv("recv", "Test that receive functionality works") ;
static TestItem tSend("send", "Test that sending works") ;

static TestItem tTimeouts("timeouts", "Put delays between tests to check keepalives & timeouts") ;
static TestItem	tDeadEp("deadep", "Dead endpoint garbage collection test") ;

static TestItem	tBulkOpen("bulkopen", "Cycle thousands of connections to test open/close functionality en-mass") ;
static TestItem tLongDelay("longdelay", "Keepalive/timeout test with a very long pause") ;

static TestItem tTeulPkt("teulpkt", "Basic validation of Teul::Packet functionality") ;
static TestItem tLargePkt("largepkt", "Test Teul::Packet with larger-than-teul packet sizes") ;
static TestItem tUnencrypted("unencrypted", "Validate send/receive of unencrypted Teul::Packet data") ;

static TestItem tStallConnect("stallconnect",	"Deliberately stall client's initial open") ;

static TestItem tAnyKey("anykey", "Prompt user to press any key on exit (for visual studio)") ;
static TestItem tKillServer("killserver", "[client] Send kill message to server on completion") ;

static TestItem tEncryption("encrypt", "Test encryption algorithms") ;

static TestItem tEchoTest("echotest", "Send thousands of packets and wait for the server to return them.") ;

static TestItem tObjTest("object", "Test the receive object handler") ;
