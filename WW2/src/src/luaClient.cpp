#include <syscfg.h> // Path is project specific.
// Created: Tue Apr 17 20:27:22 CDT 2007
//
// Command line client for executing arbitrary Lua code
// and invoking stock lua scripts on the strat host.
//
// Copyright (c) 2007 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <fcntl.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <sys/poll.h>

#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#define USE_VARARGS
#define PREFER_STDARG

#include "host/hostConstants.h"
#include "include/luaClient.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiNetPorts.h"

#include <readline/readline.h>
#include <readline/history.h>

SINT32 serverTeulIndex;

bool g_connected = false;
bool g_exitAfterCommands = true;
bool g_waitingForResponse = false;
bool g_waitingForEnd = true;
bool g_deprepRequired = true;
std::string historyFile = "";
std::string srcFile = "";
std::string cmd = "";
std::vector<std::string> g_commands;
bool g_interactive = false;
bool g_finished = false;
static bool g_debugMode = false;
bool g_enteringBlock = false;

static UINT32 minErrType = 0;

//////////////////////////////////////////////////////////////////////
// Handle shutdowns cleanly because:
//  a. We're using readline and want to avoid funky terminal states,

static void terminate(int code)
{
	if ( g_deprepRequired )
	{
		rl_set_prompt("");
		rl_clear_message();
		printf("\n");
		rl_deprep_terminal();
	}

	teulCloseTransport();

	exit(code);
}

static void _print(const char* const p);
static void _printf(const char* const fmt, ...);

//////////////////////////////////////////////////////////////////////
// Report usage and terminate.

static void
_usage()
{
	_printf("Usage: luaClient [-c code | -C code] [-f file] [-v] [hostIP]\n"
		"  -c command  sends 'command' to the host and then exits\n"
		"  -C command  like -c but does not exit after executing the code\n"
		"  -f file     sends the contents of file, executes it as a script and then exits\n"
		"  -v          verbose mode\n"
		"  hostIP      address of the strat host to connect to\n");
	terminate(1);
}

//////////////////////////////////////////////////////////////////////
// Output the supplied text via readline or printf depending on
// whether we are in interactive mode or not.

static void
_print(const char* const p)
{
	if ( g_interactive )
	{
		rl_save_prompt();
		rl_clear_message();

		printf("\033[36m%s\033[0m\n", p);

		rl_restore_prompt();
		rl_redisplay();
	}
	else
	{
		printf("%s\n", p);
	}
}

static void
_printf(const char* const fmt, ...)
{
	if ( g_interactive )
	{
		rl_save_prompt();
		rl_clear_message();
	}

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	if ( g_interactive )
	{
		rl_restore_prompt();
		rl_redisplay();
	}
}

//////////////////////////////////////////////////////////////////////
// Check to see if a string of text looks like a valid script command
// to be sent up to the host, if it is a comment to be ignored or
// if it is a local instruction to exit ("quit").

static bool
_sendCmd(const std::string& command)
{
	if ( command.empty() )
		return false;

	if ( command[0] == '#' || (command[0] == '-' && command[1] == '-') )
		return false;

	if ( command.compare("quit") == 0 || command.compare("exit") == 0 )
		terminate(0);

	Teul::Packet outgoing(command.length(), Host::CallbackX, (UCHAR)Host::Callbacks::LuaPacket_Y);
	outgoing.setUnencrypted();
	outgoing.append(command.c_str(), command.length());
	outgoing.send(serverTeulIndex);

	g_waitingForResponse = true;
	g_waitingForEnd = true;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Take an array of text strings to be sent up to the host via
// sendCmd (which filters out comment/blank lines) with a slight pause
// between each to allow the host to process them.
// Each string in the vector is sent and processed independently; a
// string may contain an entire script that will be processed as a
// single object but you should expect the remote Lua environment to
// change /between strings in the vector/.
// In short: if you are sending a script, put it in one string.

static void
_processCommands(std::vector<std::string>& commands, bool exitAfterCommands)
{
	for ( UINT32 i = 0; i < commands.size(); ++i )
	{
		if ( g_debugMode )
			_printf("cmds: %s\n", commands[i].c_str());

		if ( !_sendCmd(commands[i]) )
			continue;

		const TeulTimeMS cutoff = teulClock64 + 10;
		while ( cutoff > teulClock64 && g_waitingForResponse )
		{
			teulUpdateTransport(5);
		}
	}

	TeulTimeMS cutoff = teulClock64 + ((exitAfterCommands) ? 30 : 5) * 1000;
	while ( cutoff > teulClock64 && g_waitingForEnd )
	{
		teulUpdateTransport(5);
	}

	if ( exitAfterCommands )
	{
		terminate(0);
	}
}

//////////////////////////////////////////////////////////////////////
// Callback from readline to handle something the player typed.

void
inputHandler(char* input)
{
	if ( input == NULL || strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0 )
	{
		rl_crlf();
		rl_callback_handler_remove();
		g_finished = true;
		return;
	}
	while ( isspace(*input) )
		++input;
	if ( !*input )
		return;
	add_history(input);
	FILE* fp = fopen(historyFile.c_str(), "a+");
	if ( fp )
	{
		fprintf(fp, "%s\n", input);
		fclose(fp);
	}

	if ( g_enteringBlock == false && strcmp(input, "do") == 0 )
	{
		g_enteringBlock = true;
		rl_set_prompt("+ ");
		rl_redisplay();
		cmd = "";
	}
	else if ( g_enteringBlock && strcmp(input, ";") == 0 )
	{
		g_enteringBlock = false;
		rl_set_prompt("> ");
		rl_redisplay();
	}
	else if ( g_enteringBlock == true )
	{
		if ( cmd.empty() == false )
			cmd += "\n";
		cmd += input;
		rl_set_prompt("+ ");
	}
	else
	{
		cmd = input;
	}
}

//////////////////////////////////////////////////////////////////////
// Read input file stream for commands to be eecuted once we are
// connected.

static void
_readCommandsFromFile(std::istream& file)
{
	// Put all of the meaningful content into a single string.
	std::string command = "";
	std::string line;
	while ( file.good() )
	{
		line = "";
		if ( !getline(file, line) )
			break;

		// Trim leading whitespace out
		size_t i = 0;
		while ( i < line.length() && isspace(line[i]) )
			++i;
		if ( i > 0 )
			line.erase(0, i);
		// Ignore blank lines and comments
		if ( line.empty() || line.substr(0, 2) == "--" )
			continue;
		command += line + "\n";
	}

	// Pop the string onto the commands stack to be sent
	// as a single entry.
	g_commands.push_back(command);
}

//////////////////////////////////////////////////////////////////////
// Reads an entire lua script into one of the g_command slots so that
// it is subsequently sent up to the host and processed in a single
// _sendCmd.

static void
_readSourceFile()
{
	// You can't specify a file AND specify a -C...
	if ( g_exitAfterCommands == false )
	{
		_printf("-f and -C conflict.\n");
		terminate(2);
	}

	if ( srcFile == "-" )
	{
		_readCommandsFromFile(std::cin);
		return;
	}

	std::ifstream file;
	file.open(srcFile.c_str(), std::ifstream::in);
	if ( !file.is_open() && srcFile.find('/') == srcFile.npos )
	{
		srcFile.insert(0, "/lua/");
		srcFile.insert(0, getenv("HOME"));
 		file.open(srcFile.c_str(), std::ifstream::in);
	}

	if ( !file.is_open() )
	{
		// Is it a filename without a path?
		_printf("Unable to open %s\n", srcFile.c_str());
		terminate(3);
	}

	_readCommandsFromFile(file);
}

//////////////////////////////////////////////////////////////////////
// Load in the history file and populate the readline history from it

static void
_readHistory()
{
	if ( historyFile.empty() )
	{
		historyFile = getenv("HOME");
		historyFile += "/.lua_history";
	}

	std::ifstream file;
	std::string input;

	file.open(historyFile.c_str(), std::ifstream::in);
	if ( !file.is_open() )
		return;

	while ( file.good() )
	{
		if ( !getline(file, input) )
			break;
		if ( !input.empty() )
			add_history(input.c_str());
	}

	file.close();

	stifle_history(1000);
}

//////////////////////////////////////////////////////////////////////
// Get the user to select which host they want to connect to.

static UINT32
_getHost(std::string ipAddress)
{
	if ( ipAddress.empty() && g_interactive )
	{
		const char* name;
//		rl_save_prompt();
//		rl_clear_message();
		printf("Please select host to connect to: (d)ev, (b)eta, (t)raining, (l)ive or (q)uit.\n");
		do
		{
			name = readline("Host (d/b/t/l/q): ");
			if ( !name )
				continue;

			const char* p = name;
			while ( *p && isspace(*p) )
				++p;
			switch ( *p )
			{
			case 'd':
				ipAddress = "192.168.0.61";
			break;
			case 'b':
				ipAddress = "66.28.224.151";
			break;
			case 'l':
				ipAddress = "66.28.224.141";
			break;
			case 'q':
			case 'x':
				terminate(0);
			break;
			case 0:
			break;
			default:
				printf("Unrecognized host, '%s'\n", name);
			break;
			}
			rl_free((void*)name);
		}
		while ( ipAddress.empty() );

//		rl_restore_prompt();
//		rl_clear_message();
//		rl_set_prompt("");
//		rl_clear_pending_input();
//		rl_redisplay();
//		rl_crlf();
//		rl_reset_line_state();
	}

	UINT32 ip = ntohl(inet_addr(ipAddress.c_str()));
	if ( ip == 0 )
	{
		printf("Invalid IP address: %s\n", ipAddress.c_str());
		terminate(4);
	}

	return ip;
}

//////////////////////////////////////////////////////////////////////
// Handle a general teulkit error

static void
_teulErrorFn(const SINT32 teulIndex, const UCHAR*, SINT32, const TEUL_CALLBACK*, SINT32 error)
{
	if ( error == ENOTCONN || error == -ENOTCONN )
	{
		if ( g_finished == false )
		{
			_printf("Disconnected.\n");
		}
		g_connected = false;
		return;
	}
	_printf("General Teul error: t:%d e:%d", teulIndex, error);
}

//////////////////////////////////////////////////////////////////////
// When a lua command generates output, the text is batched up by the
// host until the command completes, at which point the host appends
// a marker (LUA_END_SEQ). The resulting text may be sent down to us
// in chunks/batches with LUA_END_SEQ providing us the feedback that
// the last command we sent completed.

static SINT32
_recvLuaPacket(ACTION_OBJECT* const a)
{
	g_waitingForResponse = false;
	char* text = (char*)(a->packed);

	// When we send the commands up, we append a command to print
	// a particular sequence that tells us all the commands were
	// executed -- the LUA_END_SEQ.
	if ( strncmp(text, LUA_END_SEQ, sizeof(LUA_END_SEQ)-1) == 0 )
	{
		g_waitingForEnd = false;
		return a->offset;
	}
	char* p = strstr(text, LUA_END_SEQ);
	if ( p != NULL )
	{
		g_waitingForEnd = false;
		*p = 0;
	}

	char* z = p = text;
	do
	{
		p = z;
		z = strchr(p, '\n');
		if ( z )
			*(z++) = 0;
		if ( g_interactive )
		{
			_print((const char*)p);
		}
		else
			_printf("%s\n", (const char*)p);
	}
	while ( z != NULL && *z != 0 );

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Callbacks.

static SINT32
_registerCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION((UCHAR)Host::CallbackX)
		REGISTER_RECEIVER((UCHAR)Host::Callbacks::LuaPacket_Y, _recvLuaPacket);
	END_CALLBACK_REGISTRATION
}

//////////////////////////////////////////////////////////////////////
// Handle connection established.

static void
_teulConnectFn(ENDPOINT& endpoint)
{
	if ( g_debugMode )
		_printf("Connected t:%d\n", endpoint.teulIndex());
	g_connected = true;
}

//////////////////////////////////////////////////////////////////////
// Handle connection established.

int
main(int argc, char* argv[])
{
	Teul::SetAppName("luaClient");
	Teul::ConfigVar::InitializeConfigVars();

	// Command line argument processing.
	int arg;
	for ( arg = 1; arg < argc && argv[arg][0] == '-'; ++arg )
	{
		switch ( argv[arg][1] )
		{
		case 'C':
			g_exitAfterCommands = false;
		case 'c':
			++arg;
			if ( arg >= argc ) _usage();
			g_commands.push_back(argv[arg]);
		break;
		case 'f':
			++arg;
			if ( arg >= argc ) _usage();
			if ( !srcFile.empty() ) _usage();
			srcFile = argv[arg];
		break;
		case 'v':
			g_debugMode = true;
		break;
		case '?':
		case 'h':
		default:
			_usage();
		break;
		}
	}

	// If we stopped short of the end of the argument list, inputs are invalid.
	if ( argc > arg + 1 )
		_usage();

	g_interactive = ((g_commands.empty() && srcFile.empty()) || g_exitAfterCommands == false);

	if ( g_interactive )
	{
		// Readline provides command-line editing functionality and history capabilities.
		rl_initialize();

		// Hand posession of the terminal to readline; as of this point we should no-longer
		// do any terminal IO ourselves (printf, etc) because it will confuse readline.
		rl_prep_terminal(1);
		g_deprepRequired = true;

		printf("\033[1mBattleground Europe Lua client [%s %s]\033[0m\n", __DATE__, __TIME__);
		rl_set_prompt("");
	}

	// Check that we were supplied a numeric IP address.
	///TODO: Allow canonical addresses.
	UINT32 ip = _getHost((argc == arg + 1) ? argv[arg] : "");
	UINT32 nip = htonl(ip);

	// If the user specified a script, load it in.
	if ( !srcFile.empty() )
		_readSourceFile();

	if ( g_debugMode )
		_printf("Initializing teulTransport\n");

	Teul::SetAppName("luaClient");
	teulInitTransport(LOG_TO_FILE, "./luaClient", _teulErrorFn);
	dbugSetLogLevel(Debug::ERR_WARN);

	_registerCallbacks();

	minErrType = 4;

	if ( g_debugMode )
		_printf("Connecting to host\n");
	serverTeulIndex = teulOpenSend(ip, WWII::Net::Ports::LuaInterface, _teulConnectFn, 30000, PROTOCOL_TCP, _teulErrorFn);
	if ( serverTeulIndex < 0 )
	{
		_printf("Unable to connect: %d\n", serverTeulIndex);
		terminate(serverTeulIndex);
	}

	// Give the connection time to go through
	if ( g_debugMode )
		_printf("Waiting on connection\n");
	TeulTimeMS timeout = teulClock64 + 15 * 1000;
	while ( g_connected == false && teulClock64 < timeout )
	{
		teulUpdateTransport(5);
	}
	if ( g_connected == false )
	{
		_printf("Failed to connect\n");
		terminate(5);
	}

	// g_commands is the 'command stack'; if we have content in there from -c or -C
	// throw the entries up to the host now.
	if ( g_commands.empty() == false )
	{
		if ( g_debugMode )
			_printf("Processing %zu commands\n", g_commands.size());
		_processCommands(g_commands, g_exitAfterCommands);
	}

	printf("Connected to %s.\n", inet_ntoa(*(struct in_addr*)&nip));
	printf("To load a script from 'scriptname.lua', type;scriptname\n");
	printf("To enter a multi-line block type \"do\" and end the block with a \";\".\n");

	// As commands are typed, we save them to a file so we can recover them next time
	// to provide a cross-session history. We have to provide this "memory" feature.
	_readHistory();

	minErrType = 5;

	// Set the readline prompt and input handler.
	rl_callback_handler_install("> ", inputHandler);

	// File descriptions to poll and the events to watch for.
	pollfd pfd = { STDIN_FILENO, POLLIN|POLLPRI|POLLERR|POLLHUP|POLLNVAL, 0 };

	do
	{
		teulUpdateTransport(5);

		// The network connection here means we can't just say "wait on readline".
		// This loop checks the network every 2 miliseconds (3rd argument)
		while ( poll(&pfd, 1, 2) == 0 )
		{
			teulUpdateTransport();
			if ( g_connected == false )
			{
				if ( !g_finished )
				{
					_printf("host went away\n");
					terminate(6);
				}
				break;
			}
		}

		if ( pfd.revents & (POLLERR|POLLHUP|POLLNVAL) )
		{
			// Application being killed
			break;
		}

		// Some kind of input event, read it
		rl_callback_read_char();

		if ( cmd.empty() || g_enteringBlock == true )
		{
			// User hasn't typed anything or is entering a
			// multi-line block, and we are only interested
			// in completed inputs.
			continue;
		}

		// Send what they've typed upto the host; they may have
		// typed an entire script or a single line. Comments and
		// blank lines are skipped by sendCmd because they may
		// be pasting from a script or piping/redirecting script
		// content to us and sending "-- This is a comment" to
		// the host is a waste of bandwidth and server-side CPU.
		_sendCmd(cmd);
		cmd = "";
	} while ( !g_finished );

	teulClose(serverTeulIndex);
	teulCloseTransport();

	terminate(0);
}

