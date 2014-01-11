#include <syscfg.h> // Path is project specific.
// Date: Thu Jun 15 15:26:30 2000
//
// The roster client
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <sys/poll.h>
#include <fcntl.h>

#include "chat/chatClientMessages.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatLocFile.h"
#include "chat/chatManagementClientMessages.h"
#include "chat/chatTypes.h"
#include "teul/teulKit.h"
#include "wwii/wwiiNetPorts.h"

enum PROG_TYPE
{
	PT_ROSTER_CLIENT	=	0
,	PT_SCRIPT_CLIENT
,	PT_SYSTEM_CLIENT
,	PT_CHAT_CLIENT
} ;

static SCHAR* program ;
static bool bHaveRoster = false ;
static int numClients = 0 ;

SINT32 connection ;

static inline void
_pause(int duration=1000)
{
	teulSyncClock() ;
	const TeulTimeMS end = teulClock64 + duration ;
	while ( teulClock64 < end )
		teulUpdateTransport(5) ;
}

static void
_systemClient(char* string)
{
	static char cmd[129] ;

	if ( *string == ',' )
	{
		string++ ;
		_pause() ;
	}

	if ( *string != 0 )
	{
		snprintf(cmd, 128, ".sys %s", string) ;
		chatSendMessage(connection, 1, cmd) ;
	}
}

static void
_chatClient(SCHAR* string)
{
	if ( *string == ',' )
	{
		string++ ;
		_pause() ;
	}

	if ( *string != 0 )
		chatSendMessage(connection, 1, string) ;
}

static const char* pwnType ;
static SCHAR  pwnCountry[3] ;

static WWII_COUNTRY
_isCountry(SCHAR* string)
{
	char country[3] = "\0\0" ;

	if ( !isalpha(*string) )
		return WWII_NO_COUNTRY ;

	country[0] = string[0] ;
	country[1] = string[1] ;

	if ( strcasecmp(country, "uk") == 0 )
		return WWII_ENGLAND ;
	if ( strcasecmp(country, "de") == 0 )
		return WWII_GERMANY ;
	if ( strcasecmp(country, "fr") == 0 )
		return WWII_FRANCE ;
	if ( strcasecmp(country, "it") == 0 )
		return WWII_ITALY ;
	if ( strcasecmp(country, "cn") == 0 )
		return WWII_CHINA ;
	if ( strcasecmp(country, "ru") == 0 )
		return WWII_RUSSIA ;
	return WWII_NO_COUNTRY ;
}

static void
_scriptClient(SCHAR* string)
{
	static SCHAR cmd[600] ;

	if ( *string == ',' )
	{
		string++ ;
		_pause() ;
	}

	if ( string == nullptr || *string == 0 )
		return ;

	// 2 character string = set country
	if ( _isCountry(string) > WWII_NO_COUNTRY )
	{
		strncpy(pwnCountry, string, 2) ;
		_pause() ;
		return ;
	}

	if ( pwnCountry[0] == 0 )
	{
		fprintf(stderr, "Must set 2-digit country code before specifying %s name: %s", pwnType, string) ;
		teulCloseTransport() ;
		exit(1) ;
	}

	snprintf(cmd, sizeof(cmd) - 1, ";chown%s %s %s", pwnType, pwnCountry, string) ;
	chatSendMessage(connection, 1, cmd) ;
	printf("%s %s => %s\r\n", pwnType, string, pwnCountry) ;

	_pause(500) ;

	return ;
}

void
rosterMessage(Chat::LocalizedMessage& lmsg)
{
	const char* message = chatLString(lmsg) ;
	if ( lmsg.m_message == LSTR_TEXT_START || lmsg.m_message == LSTR_TEXT )
		printf("%s", message) ;
	else
		printf("%s\n", message) ;
	bHaveRoster = true ;
}

extern SINT32 chatSetChatInfo(SINT32, UINT32, UINT16) ;

static void
_usage()
{
	printf("\nUsage: %s [-c 'command' | -C 'command'] [-d delay] -g 'gamename' [HOST_PORT]\n\n", program);
	printf(" -c 'command'    Sends the specified command(s) to the host and then terminates.\n") ;
	printf(" -C 'command'    Sends commands without disconnecting afterwards.\n") ;
	printf(" -d <delay>      Wait 'delay' seconds between submitting command-line commands to the server.\n") ;
	printf(" -g 'gamename'   Sets the name to be used by this client on the host.\n") ;
	printf(" -n <number>     Connect a number of clients and wait for the user to abort.\n") ;
	exit(1);
}

static void
_processCommands(std::vector<const char*>& commands, UINT32 delay, bool exitAfterCommands)
{
	_pause(250) ;

	for ( UINT32 i = 0 ; i < commands.size() ; ++i )
	{
		chatSendMessage(connection, 1, commands[i]) ;
		_pause(delay * 1000) ;
	}

	_pause() ;

	if ( exitAfterCommands )
	{
		exit(0) ;
	}
}

static bool _connectionOpened = false ;
static void _connectionOpen(ENDPOINT& /*endpoint*/)
{
	_connectionOpened = true ;
}

static bool _connectionError = false ;
static void _errorFunction (SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/)
{
	_connectionError = true ;
}

static int
_openConnection(UINT32 host, UINT32 port, const char* const gameName)
{
	_connectionError = false ;
	SINT32 conn = teulOpenSend(host, (UINT16)port, _connectionOpen, 2000, PROTOCOL_TCP, _errorFunction);
	if ( conn < 0 )
	{
		dbugLog(_FLA, "Error allocating endpoint!");
		exit(-ENOTCONN);
	}

	dbugLog(_FLI, "t:%d Connecting", conn) ;
	while ( _connectionOpened == false )
	{
		teulUpdateTransport(20);

		if ( _connectionError )
		{
			dbugLog(_FLA, "Unable to open connection.\n") ;
			exit(-ENOTCONN) ;
		}
	}

	chatSetChatInfo(conn, host, (UINT16)port);

	chatSendManagementLogin2(conn, gameName);

	return conn ;
}

// Open a number of clients.
static void
_openNumClients(UINT32 host, UINT32 port)
{
	std::vector<const char*> messages = { "/wtf", ".help", ".who", "/roster", "hello", "/m kfs1 test", "/m kfs2 testing", "/pinfo", "/wurble", "/afk", ".time"  } ;
	std::vector<int> connections ;

	for ( int i = 0 ; i < numClients ; ++i )
	{
		printf("\r#%u ", i + 1) ;
		fflush(stdout) ;
		SINT32 conn = _openConnection(host, port, "auto") ;
		connections.push_back(conn) ;
	}

	printf("\r- Opened %u connections\n", numClients) ;

	while ( true )
	{
		teulUpdateTransport(20) ;

		if ( rand() % 5 > 0 )
			continue ;

		int n = rand() % connections.size() ;
		int m = rand() % messages.size() ;

		chatSendMessage(connections[n], 1, messages[m]) ;
	}
}

int
main(int argc, char* argv[])
{
	Teul::SetAppName("chatClient");
	Teul::ConfigVar::InitializeConfigVars();

	SINT32 retval;
	SCHAR hostAddress[1024], hostPort[1024];
	UINT32 i;
	PROG_TYPE progType ;
	std::vector<const char*> commands ;
	// Seconds to wait between commands
	UINT32 commandDelay = 1 ;
	bool exitAfterCommands = true ;
	char gameName[MAX_HANDLE_SIZE] = "auto" ;

	chatReadHostLocFile() ;

	program = strrchr(argv[0], '/') ;
	if ( program == nullptr )
		program = argv[0] ;
	else
		program = program + 1 ;

	int arg ;
	for ( arg = 1 ; arg < argc && argv[arg][0] == '-' ; ++arg )
	{
		switch ( argv[arg][1] )
		{
		case 'g':
			++arg ;
			if ( arg >= argc ) _usage() ;
			if ( argv[arg][0] == '-' || chatIsValidCallsign(argv[arg]) == false || (int)strlen(argv[arg]) > (int)MAX_HANDLE_LEN )
			{
				printf("Invalid gamename: %s\n", argv[arg]) ;
				_usage() ;
			}
			strcpy(gameName, argv[arg]) ;
		break ;

		case 'C':
			exitAfterCommands = false ;
		case 'c':
			++arg;
			if ( arg >= argc ) _usage() ;
			commands.push_back(argv[arg]) ;
		break ;
		case 'd':
			++arg ;
			if ( arg >= argc ) _usage() ;
			if ( !isdigit(argv[arg][0]) ) _usage() ;
			commandDelay = atoi(argv[arg]) ;
		break ;
		case 'n':
			++arg ;
			if ( arg >= argc ) _usage() ;
			if ( !isdigit(argv[arg][0]) ) _usage() ;
			numClients = atoi(argv[arg]) ;
		break ;
		case '?':
		case 'h':
		default:
			_usage() ;
		break ;
		}
	}

	if ( argc < arg + 1 )
		_usage() ;

	strncpy(hostAddress,argv[arg],1024);

	UINT32 host=0 ;
	if(teulGetHost(hostAddress, &host) == 0)
	{
		fprintf(stderr, "invalid IP address: %s", hostAddress) ;
		exit(-EINVAL);
	}

	UINT32 port = WWII::Net::Ports::ChatManagement ;
	if (argc > arg+1)
	{
		strncpy(hostPort,argv[arg+1],1024);
		port = (UINT32)atol(hostPort);
	}

	teulInitTransport(LOG_OFF, "./chatRoster", _errorFunction);
	dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_NOTE));
	dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_NOTE));

	retval = chatInstallClientCallbacks();
	if (retval < 0)
	{
		dbugLog(_FLA,"problem installing chat client callbacks!");
		exit(1);
	}

	retval = chatInstallClientManagementCallbacks();
	if (retval < 0)
	{
		dbugLog(_FLA,"problem installing chat client callbacks!");
		exit(1);
	}

	if ( numClients > 0 )
	{
		_openNumClients(host, port) ;
		exit(0) ;
	}

	connection = _openConnection(host, port, gameName) ;

	if ( !commands.empty() )
		_processCommands(commands, commandDelay, exitAfterCommands) ;

	if ( strcmp(program, "pwncps") == 0 )
	{
		progType = PT_SCRIPT_CLIENT ;
		pwnType = "cp" ;
	}
	else if ( strcmp(program, "pwnfacs") == 0 )
	{
		progType = PT_SCRIPT_CLIENT ;
		pwnType = "fac" ;
	}
	else if ( strcmp(program, "chatClient") == 0 )
		progType = PT_CHAT_CLIENT ;
	else
		progType = PT_SYSTEM_CLIENT ;

//	handleID = chatGetHandle("99");
//	chatTuneRadioToHandle(0,handleID);

	if ( strncmp(program, "roster", 6) == 0 )
	{
		dbugLog(_FLA,"sending roster command waiting for reply");
		chatSendMessage(connection, 1,".roster");

		for ( i = 0 ; !bHaveRoster && i < 100 ; ++i )
		{
			teulUpdateTransport(10);
		}
	}
	else
	{
		SCHAR buf[MAX_MESSAGE_LENGTH + 8] ;
		SCHAR* p ;
		SCHAR* e ;
		pollfd pfd = { STDIN_FILENO, POLLIN|POLLPRI|POLLERR|POLLHUP|POLLNVAL, 0 } ;

		printf("WW2 Online %s client\n", program) ;

		do
		{
			size_t bytesRead ;

			while ( poll(&pfd, 1, 25) == 0 )
				teulUpdateTransport() ;

			if ( pfd.revents & (POLLERR|POLLHUP|POLLNVAL) )
				break ;

			bytesRead = read(STDIN_FILENO, buf, sizeof(buf)-1) ;
			if ( bytesRead <= 0 )
				break ;
			buf[bytesRead] = 0 ;

			p = buf ;
			while ( isspace(*p) && *p != 0 )
				p++ ;
			e = p ;
			while ( *e != 0 && *e != '\r' && *e != '\n' )
				e++ ;
			*e = 0 ;

			if ( strcasecmp(p, "quit") == 0 || strcasecmp(p, "exit") == 0 )
				break ;

			if ( *p == '#' )
				continue ;

			switch ( progType )
			{
				case PT_SYSTEM_CLIENT:
					_systemClient(p) ;
					_pause() ;
				break ;

				case PT_SCRIPT_CLIENT:
					_scriptClient(p) ;
				break ;

				case PT_CHAT_CLIENT:
					_chatClient(p) ;
				break ;

				default:
					exit(0) ;
				break ;
			}
			teulUpdateTransport() ;
		} while (1) ;
	}

	teulCloseTransport();
	exit(0);

}

