#include <syscfg.h> // Path is project specific.
// Created: Tue Jul 17 11:30:03 CDT 2007
//
// Provides an API for hosts to connect to the internal IRC server
// Basic usage:
//
//  // Initialize the IRC context with defaults
//  wwiiIRCInitialize(ip, plural) ;
//
//  /* your IRC setup stuff here, any callbacks etc */
//
//  // Start the IRC thread/connection.
//  wwiiIRCActivate() ;
//
//  /* your code here */
//
//  // Notify our cluster's -info channel we are ready
//  wwiiIRCChannelMessage("-info", "Server ready") ;
//
//  /* your server code */
//
//  // Shutdown cleanly
//  wwiiIRCTerminate("End of application") ;
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"							// For host "dbugLog" access

#include <libircclient.h>
#include <libirc_rfcnumeric.h>
#include <signal.h>
#include <OpenThreads/Thread>
#include <set>

#include "wwii/wwiiThreads.h"
#include "wwii/wwiiIRC.h"
#include "server/serverConfigVars.h"

void wwiiIRCDefaultChannelMsg(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) ;

//////////////////////////////////////////////////////////////////////
// Description of a channel a server wishes to belong to, provides
// name, modes (e.g. "+o") and default topic.

struct IRCChannel
{
	IRCChannel() {}
	IRCChannel(const char* channel, const char* mode, const char* topic)
		: m_channel(channel), m_mode(mode), m_topic(topic)
		, m_newChannel(false)
		{}
	std::string m_channel, m_mode, m_topic ;
	bool m_newChannel ;
} ;

typedef std::vector<std::string> _MODES ;
typedef std::map<std::string, IRCChannel> _CHANNELS ;

namespace Teul { extern const char* GetAppName() ; }

//////////////////////////////////////////////////////////////////////
// Our context/IRC description. Describes an IRC session. This is
// constructed by wwiiIRCInitialize and completed by wwiiIRCActivate.
// Includes all the information necessary to re-establish the
// connection if the server barfs.

static const char* _getChannelName(const char* channel, char* into, size_t intoSize) ;

struct IRCImpl : public IRC, public OpenThreads::Thread
{
	IRCImpl()
		:  m_initialized(false), m_active(false), m_running(false), m_joined(false)
		, m_session(NULL)
		, m_app(""), m_nick("")
		, m_modes(), m_channels()
		{}
	virtual ~IRCImpl() {}

	bool			m_initialized ;
	bool			m_active ;
	volatile bool	m_running ;
	bool			m_joined ;			// Finished joining
	irc_session_t*	m_session ;
	std::string		m_app ;
	std::string		m_nick ;
	_MODES			m_modes ;
	_CHANNELS		m_channels ;

	// Dot command index
	typedef std::map<std::string, irc_dotcmd_t> DOTS ;
	DOTS			m_dots ;

	std::set<std::string>	m_ignores ;

	virtual bool ignoring(const char* channel) const
	{
		char realName[128] ;
		return ( m_ignores.find(_getChannelName(channel, realName, sizeof(realName))) != m_ignores.end() ) ;
	}
	virtual const std::string& nick() const { return m_nick ; }

	virtual void run() ;
} ;

irc_callbacks_t ircCallbacks ;
static IRCImpl irc ;

//////////////////////////////////////////////////////////////////////
// The central loop of the IRC thread.

void
IRCImpl::run()
{
	threadLog(_FLI, "IRC: started [thread #%u]", getThreadId()) ;

	static const char* IRC_PASS = "blooGit" ;

	const char* ircPass = getenv("WWII_IRC_PASSWD") ;
	if ( ircPass == NULL || *ircPass == 0 )
		ircPass = IRC_PASS ;

	UINT32 backOff = 30 ;
	// Running indicates we have not yet made a pass thru the loop.
	m_running = false ;
	do
	{
		// If running didn't just get set to false, then we have been
		// disconnected.
		if ( m_running )
		{
			threadLog(_FLN, "IRC: Reconnecting") ;
			if ( backOff < 90 )
				++backOff ;
		}
		else
			m_running = true ;

		static const UINT32 IRC_PORT = 6667 ;

		irc.m_joined = false ;
			threadLog(_FLI, "IRC: Connecting %s:%u %s", cv_ircServer.GetString(), IRC_PORT, m_nick.c_str()) ;
		if ( irc_connect(m_session, cv_ircServer.GetString(), IRC_PORT, ircPass, m_nick.c_str(), m_nick.c_str(), m_nick.c_str()) )
		{
			threadLog(_FLM, "IRC: Unable to connect: %s", irc_strerror(irc_errno(m_session))) ;
			irc_disconnect(m_session) ;
			irc.m_joined = true ;
		}
		else
		{
			OpenThreads::Thread::YieldCurrentThread() ;
			// Reset the backoff
			backOff = 0 ;

			// Enter the central irc loop
			irc_run(m_session) ;

			// If the loop exits, cleanly disconnect.
			irc_disconnect(m_session) ;
		}

		OpenThreads::Thread::YieldCurrentThread() ;
		const TeulTimeMS backoff_us = (TeulTimeMS)(backOff + 10) * 1000ULL * 1000ULL ;
		OpenThreads::Thread::microSleep(static_cast<unsigned int>(backoff_us)) ;
	}
	while ( m_running ) ;
}

//////////////////////////////////////////////////////////////////////
// Shutdown the IRC thread cleanly and wait for it to end

static void
_terminate(const char* reason)
{
	if ( irc.m_session == NULL || irc.m_active == false || irc.m_running == false )
		return ;

	irc.m_running = false ;

	irc_cmd_quit(irc.m_session, reason) ;

	OpenThreads::Thread::microSleep(250 * 1000) ;

	irc.cancel() ;

	OpenThreads::Thread::microSleep(250 * 1000) ;

	threadsLogging() ;
}

//////////////////////////////////////////////////////////////////////
// IRC-aware signal handler

typedef void (*SignalHandler)(int);
#if !defined(HAVE_SIGACTION)
struct sigaction { SignalHandler sa_handler; };
#endif
static const int MaxSignals = 32 ;
static struct sigaction oldActions[MaxSignals] ;

static void
_ircSigHandler(int signo)
{
	char buffer[100] ;
	const char* format = "*** PROCESS CRASHED: %s ***" ;
	const char* sigName = NULL ;
	switch ( signo )
	{
	case SIGINT:	format = "Shutdown [%s]" ; sigName = "sigint" ; break ;
	case SIGABRT:	sigName = "abort()" ; break ;
	case SIGILL:	sigName = "Illegal Instruction" ; break ;
	case SIGFPE:	sigName = "Float.Pt Error" ; break ;
	case SIGSEGV:	sigName = "Segment Fault" ; break ;
	case SIGTERM:	sigName = "Terminated" ; break ;
#if defined(SIGHUP)
	case SIGHUP:	format = "Shutdown [%s]" ; sigName = "hangup" ; break ;
	case SIGBUS:	sigName = "Bus Error" ; break ;
#endif
	default: break ;
	}

	if ( sigName != NULL )
		sprintf(buffer, format, sigName) ;
	else
		sprintf(buffer, "*** PROCESS CRASHED with signal %d ***", signo) ;

	_terminate(buffer) ;

	if ( signo >= 0 && signo < MaxSignals )
	{
		SignalHandler handler = oldActions[signo].sa_handler ;
		if ( handler != NULL && handler != SIG_IGN && handler != SIG_DFL )
			handler(signo) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Set a signal handler and a fallback for a given signal.

static void
_setSig(int signum)
{
#if defined(HAVE_SIGACTION)
	struct sigaction sa ;
	sa.sa_handler = _ircSigHandler ;
	sigfillset(&sa.sa_mask) ;
	sa.sa_flags = SA_RESETHAND ;

	struct sigaction* oldact = (signum >= 0 and signum < MaxSignals) ? &oldActions[signum] : NULL ;
	sigaction(signum, &sa, oldact) ;
#else
	oldActions[signum].sa_handler = signal(signum, _ircSigHandler);
#endif
}

//////////////////////////////////////////////////////////////////////
// Cleanly ask the IRC thread to terminate.

static void
_shutdown()
{
	_terminate("Shutdown [cleanly]") ;
}

//////////////////////////////////////////////////////////////////////
// If the supplied channel name begins with "-", it is exapended to
// a cluster-name cluster channel name.

static const char*
_getChannelName(const char* const channel, char* const into, size_t intoSize)
{
	assert( channel != NULL ) ;

	const char* src = channel ;
	char* dst = into ;
	char* const end = into + intoSize - 1 ;
	while ( *src && dst < end )
	{
		if ( *src == '-' )
			dst += snprintf(dst, (size_t)(end - dst), "#%s", cv_clusterName.GetString()) ;
		else if ( *src != '#' )
			*(dst++) = '#' ;
		char copied ;
		do
		{
			copied = *(dst++) = *(src++) ;
			if ( copied == 0 )
				return into ;
		} while ( copied != ',' && dst < end ) ;
	}
	*end = 0 ;

	return into ;
}

//////////////////////////////////////////////////////////////////////
// Initialize the IRC Context and fill out all defaults.

bool
wwiiIRCInitialize(UINT32 ip, bool plural)
{
	if ( irc.m_initialized )
		LogAbort("Tried to reinitialize IRC") ;

	if ( cv_clusterName.GetString() == NULL )
	{
		dbugLog(_FLA, "WWII_CLUSTER not defined") ;
		return false ;
	}

	if ( cv_ircServer.GetString() == NULL || cv_ircServer.GetString() == 0 )
	{
		dbugLog(_FLA, "WWII_IRC_SERVER not defined") ;
		return false ;
	}

	const char* const appName = Teul::GetAppName() ;
	irc.m_app = appName ;

	char buf[128] ;

	// Plural indicates there is more than one of this type of host
	if ( plural )
		sprintf(buf, "%c%c%s%c%s%u", IRC::_SERVER_PREFIX, toupper(cv_clusterName.GetString()[0]), cv_clusterName.GetString()+1, toupper(appName[0]), appName+1, ip & 255) ;
	else
		sprintf(buf, "%c%c%s%c%s", IRC::_SERVER_PREFIX, toupper(cv_clusterName.GetString()[0]), cv_clusterName.GetString()+1, toupper(appName[0]), appName+1) ;

	irc.m_nick = buf ;

	ircCallbacks.event_nick = wwiiIRCDumpEvent ;
	ircCallbacks.event_quit = wwiiIRCDumpEvent ;
	ircCallbacks.event_part = wwiiIRCDumpEvent ;
	ircCallbacks.event_mode = wwiiIRCDumpEvent ;
	ircCallbacks.event_topic = wwiiIRCDumpEvent ;
	ircCallbacks.event_kick = wwiiIRCDumpEvent ;
	ircCallbacks.event_notice = wwiiIRCDumpEvent ;
	ircCallbacks.event_invite = wwiiIRCDumpEvent ;
	ircCallbacks.event_umode = wwiiIRCDumpEvent ;
	ircCallbacks.event_ctcp_rep = wwiiIRCDumpEvent ;
	ircCallbacks.event_ctcp_action = wwiiIRCDumpEvent ;
	ircCallbacks.event_unknown = wwiiIRCDumpEvent ;
	ircCallbacks.event_channel = wwiiIRCDefaultChannelMsg ;

	ircCallbacks.event_join = wwiiIRCDefaultJoin ;
	ircCallbacks.event_connect = wwiiIRCDefaultConnect ;
	ircCallbacks.event_numeric = wwiiIRCDefaultNumeric ;
	ircCallbacks.event_privmsg = wwiiIRCDefaultPrivMsg ;

	irc.m_initialized = true ;

	// All server notices
	wwiiIRCAddInitialChannel("#servers", "-nt", "Crticial notices from all clusters") ;
	wwiiIRCAddChannelIgnore("#servers") ;

	// Cluster server notices
	sprintf(buf, "#%s-sys", cv_clusterName.GetString()) ;
	char title[256] ;
	snprintf(title, 255, "CRS, GM and System messages from %s servers", cv_clusterName.GetString()) ;
	wwiiIRCAddInitialChannel(buf, "-nt", title) ;

	// Initial modes
	wwiiIRCAddInitialMode("-b") ;
	wwiiIRCAddInitialMode("-c") ;
	wwiiIRCAddInitialMode("-d") ;
	wwiiIRCAddInitialMode("-k") ;
	wwiiIRCAddInitialMode("-l") ;
	wwiiIRCAddInitialMode("-n") ;
	wwiiIRCAddInitialMode("-r") ;
	wwiiIRCAddInitialMode("-s") ;
	wwiiIRCAddInitialMode("-u") ;
	wwiiIRCAddInitialMode("-y") ;
	wwiiIRCAddInitialMode("-z") ;

	dbugLog(_FLI, "IRC: Initialized") ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// If want chat on a particular channel to be discarded by the default
// privmsg handler, you need to specify it here. For instance, the
// #servers channel is added automatically because it is joined by
// ALL processes from ALL clusters.

void
wwiiIRCAddChannelIgnore(const char* name)
{
	char realName[128] ;
	_getChannelName(name, realName, sizeof(realName)) ;
	irc.m_ignores.insert(realName) ;
}

//////////////////////////////////////////////////////////////////////
// Initial Channels are automatically joined when the server connects
// or reconnects to the IRC server. Any channels you irc_cmd_join
// manually will not be automatically rejoined.

void
wwiiIRCAddInitialChannel(std::string name, const char* mode, const char* topic)
{
	if ( irc.m_initialized == false )
		return ;

	if ( irc.m_active )
		LogAbort("Tried to add channel after initialization") ;

	if ( name[0] == '-' )
	{
		name.insert(0, cv_clusterName.GetString()) ;
		name.insert(0, "#") ;
	}

	IRCChannel addition(name.c_str(), mode, topic) ;
	irc.m_channels[name] = addition ;

	dbugLog(_FLI, "IRC: +chan(%s, %s, %s)", name.c_str(), mode, topic) ;
}

//////////////////////////////////////////////////////////////////////
// Specify initial user-modes to be set each time you connect or
// reconnect to the IRC server

void
wwiiIRCAddInitialMode(const char* mode)
{
	if ( irc.m_initialized == false )
		return ;

	if ( irc.m_active )
		LogAbort("Tried to add mode after initialization") ;
	dbugLog(_FLI, "IRC: added mode %s", mode) ;
	irc.m_modes.push_back(mode) ;
}

//////////////////////////////////////////////////////////////////////
// Dump the details of an event to the thread log.

void
wwiiIRCDumpEvent(irc_session_t* /*session*/, const char* event, const char* origin, const char** params, unsigned int count)
{
	static std::string text ;
	text = "" ;

	for ( UINT32 i = 0 ; i < count && text.length() < 1000 ; ++i )
	{
		if ( i )
			text += "|" ;
		text += params[i] ;
	}

	if ( text.length() > 1000 )
		text.replace(997, 3, "...") ;

	threadLog(_FLI, "IRC e:%s o:%s p:%u[%s]", event, origin ? origin : "NULL", count, text.c_str()) ;
}

//////////////////////////////////////////////////////////////////////
// Default handler for the connect event -- sets all modes supplied
// via wwiiIRCAddInitialMode and connects to all the channels
// specified via wwiiIRCAddInitialChannel.

static void
_joinChannel(irc_session_t* session, _CHANNELS::iterator it)
{
	if ( it == irc.m_channels.end() )
	{
		wwiiIRCChannelMessage("-sys", "== Process Online ==") ;
		irc.m_joined = true ;
		return ;
	}

	IRCChannel& chan = it->second ;

	threadLog(_FLI, "IRC: Attempting to join %s", chan.m_channel.c_str()) ;

	chan.m_newChannel = true ;
	irc_cmd_join(session, chan.m_channel.c_str(), 0) ;
}

void
wwiiIRCDefaultConnect(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	// Record
	wwiiIRCDumpEvent(session, event, origin, params, count) ;

	// First, request operator mode.
	irc_send_raw(session, "oper god crummy") ;

	irc.m_joined = false ;

	// Set user modes
	for ( _MODES::iterator it = irc.m_modes.begin() ; it != irc.m_modes.end() ; ++it )
	{
		irc_cmd_user_mode(session, it->c_str()) ;
	}

	_joinChannel(session, irc.m_channels.begin()) ;
}

void
wwiiIRCDefaultChannelMsg(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	if ( origin[0] == IRC::_SERVER_PREFIX )
		return ;

	// Record
	wwiiIRCDumpEvent(session, event, origin, params, count) ;
}

//////////////////////////////////////////////////////////////////////
// Default handler for privmsg and channelmsg messages: looks for
// messages with start with "." for dot commands except for ignored
// channels.
// You can add new dot commands via wwiiIRCRegisterCommand.

void
wwiiIRCDefaultPrivMsg(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	if ( origin[0] == IRC::_SERVER_PREFIX )
		return ;

	// Record
	wwiiIRCDumpEvent(session, event, origin, params, count) ;

	if ( count < 2 || params[1][0] != '.' )
		return ;
	if ( params[1][1] == '.' )
		return ;

	const char* input = params[1] + 1 ;
	if ( irc.m_ignores.find(input) != irc.m_ignores.end() )
		return ;

	char nick[129] ;
	irc_target_get_nick(origin, nick, 128) ;
	const char* recipient = (params[0][0] == '#') ? params[0] : nick ;

	IRC_Command iCmd(nick, recipient, input) ;

	IRCImpl::DOTS::iterator it ;

	// ".help" produces a list of commands.
	if ( iCmd.cmd == "help" )
	{
		std::string reply = "Commands: help" ;
		for ( it = irc.m_dots.begin() ; it != irc.m_dots.end() ; ++it )
		{
			reply += ", " ;
			reply += it->first ;
		}
		reply += "." ;
		irc_cmd_msg(session, recipient, reply.c_str()) ;
		return ;
	}

	it = irc.m_dots.find(iCmd.cmd) ;
	if ( it == irc.m_dots.end() )
	{
		char retort[64] ;
		sprintf(retort, "I don't understand '%s'", iCmd.Command()) ;
		irc_cmd_msg(session, recipient, retort) ;
		return ;
	}

	it->second(iCmd) ;
}

//////////////////////////////////////////////////////////////////////
// Return 'true' if we can handle this message for you.

bool
wwiiIRCHandleJoinMessage(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	wwiiIRCDumpEvent(session, event, origin, params, count) ;

	char nick[65] ;
	irc_target_get_nick(origin, nick, 64) ;

	// Is this a message about me?
	if ( irc.m_nick.compare(nick) == 0 )
	{
		// So I've joined a channel.
		_CHANNELS::iterator it = irc.m_channels.find(params[0]) ;
		if ( it != irc.m_channels.end() )
		{
			const IRCChannel& chan = it->second ;
			std::string modeCmd = "+o " ;
			modeCmd += irc.m_nick ;
			irc_cmd_channel_mode(session, chan.m_channel.c_str(), modeCmd.c_str()) ;

			irc_cmd_user_mode(session, "+i") ;
		}

		// We've done the work.
		return true ;
	}

	// We did no work, up to you if you want to handle it.
	return false ;
}

//////////////////////////////////////////////////////////////////////
// Default handler for the join message (indicating someone has
// joined a channel)

void
wwiiIRCDefaultJoin(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	wwiiIRCHandleJoinMessage(session, event, origin, params, count) ;
}

//////////////////////////////////////////////////////////////////////
// Default handler for a numeric event

void
wwiiIRCDefaultNumeric(irc_session_t * session, unsigned int event, const char* origin, const char** params, unsigned int count)
{
	char buf[32] ;
	sprintf(buf, "num#%u", event) ;
	wwiiIRCDumpEvent(session, buf, origin, params, count) ;

	if ( count < 2 )
		return ;

	const char* nick = params[0] ;
	const char* channel = params[1] ;

	if ( channel == NULL || channel[0] != '#' || irc.m_nick.compare(nick) != 0 )
		return ;

	_CHANNELS::iterator it = irc.m_channels.find(channel) ;
	if ( it == irc.m_channels.end() )
		return ;
	IRCChannel& chan = it->second ;

	switch ( event )
	{
	case 333:	// Channel creation info, means channel existed
	case LIBIRC_RFC_RPL_TOPIC:	// Topic notification for a channel
		chan.m_newChannel = false ;
	break ;

	case LIBIRC_RFC_RPL_ENDOFNAMES:	// End of user list
		if ( chan.m_newChannel == true )
		{
			threadLog(_FLI, "IRC: Created %s", channel) ;
			if ( chan.m_mode.empty() == false )
				irc_cmd_channel_mode(session, chan.m_channel.c_str(), chan.m_mode.c_str()) ;
			if ( chan.m_topic.empty() == false )
				irc_cmd_topic(session, chan.m_channel.c_str(), chan.m_topic.c_str()) ;
		}
		else
		{
			threadLog(_FLI, "IRC: Joined %s", channel) ;
		}
		++it ;
		_joinChannel(session, it) ;
	break ;
	}
}

//////////////////////////////////////////////////////////////////////
// Creates the IRC session and starts the thread to operate it. You must
// have called Initialize and specified any callbacks you want to override
// by this point.
// If you change ircCallbacks after this, it will have no effect.

bool
wwiiIRCActivate(bool includeSigHup)
{
	if ( cv_ircServer.GetString() == NULL || *cv_ircServer.GetString() == 0 )
	{
		dbugLog(_FLA, "IRC: disabled") ;
		return true ;
	}

	if ( irc.m_initialized == false )
	{
		dbugLog(_FLA, "IRC: Tried to activate but initialization failed") ;
		return false ;
	}

	irc.m_session = irc_create_session(&ircCallbacks) ;
	if ( irc.m_session == NULL )
	{
		dbugLog(_FLA, "IRC: Unable to create session") ;
		return false ;
	}

	irc_set_ctx(irc.m_session, &irc) ;

	irc.m_active = true ;

	atexit(_shutdown) ;

	irc.start();

	_setSig(SIGINT) ;
	_setSig(SIGABRT) ;
	_setSig(SIGILL) ;
	_setSig(SIGFPE) ;
	_setSig(SIGSEGV) ;
	_setSig(SIGTERM) ;

#if defined(SIGHUP)
	_setSig(SIGBUS) ;

	if ( includeSigHup == true )
		_setSig(SIGHUP) ;
#endif

	dbugLog(_FLN, "IRC: Activated as %s", irc.m_nick.c_str()) ;

	// Allow IRC 5 seconds to initialize
	for ( UINT32 i = 0 ; i < 5 * 100 ; ++i )
	{
		OpenThreads::Thread::microSleep(10 * 100) ;
		threadsLogging() ;
		if ( irc.m_joined == true )
			break ;
	}

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Shut down the IRC session.

void
wwiiIRCTerminate(const char* reason)
{
	if ( reason == NULL )
		reason = "Terminated" ;
	_terminate(reason) ;
}

//////////////////////////////////////////////////////////////////////
// Send a plain-text message to an IRC channel

void
wwiiIRCChannelMessage(const char* channel, const char* message)
{
	assert( channel != NULL ) ;
	assert( message != NULL ) ;

	if ( irc.m_running )
	{
		char name[128] ;
		irc_cmd_msg(irc.m_session, _getChannelName(channel, name, sizeof(name)), message) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Send a printf-formatted message to an IRC channel

void
wwiiIRCMessage(const char* channel, const char* fmt, ...)
{
	if ( irc.m_running == false )
		return ;

	assert( channel != NULL ) ;

	if ( fmt == NULL )
		dbugLog(_FLW, "called wwiiIRCMessage(%s, NULL)...", channel) ;

	static char buffer[1202] ;
	va_list args ;
	va_start(args, fmt) ;

	vsnprintf(buffer, 1200, fmt, args) ;
	va_end(args) ;

	buffer[1200] = 0 ;

	char name[128] ;
	irc_cmd_msg(irc.m_session, _getChannelName(channel, name, sizeof(name)), buffer) ;
}


//////////////////////////////////////////////////////////////////////
// Send a NOTICE to an IRC channel

void
wwiiIRCChannelNotice(const char* channel, const char* message)
{
	assert( channel != NULL ) ;
	assert( message != NULL ) ;

	if ( irc.m_running )
	{
		char name[128] ;
		irc_cmd_notice(irc.m_session, _getChannelName(channel, name, sizeof(name)), message) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Register a new "." command.

void
wwiiIRCRegisterCommand(const char* name, irc_dotcmd_t function)
{
	assert( name != NULL ) ;

	if ( function != NULL )
	{
		dbugLog(_FLI, "IRC: Registering .%s", name) ;
		irc.m_dots[name] = function ;
	}
	else
	{
		dbugLog(_FLI, "IRC: Disabling .%s", name) ;
		irc.m_dots.erase(name) ;
	}
}

//////////////////////////////////////////////////////////////////////
// IRC_Command constructor

namespace Host
{
	CommandMsg::CommandMsg(const char* fromNick, const char* withRecipient, const char* withInput, UINT64 withInstID)
		: nick(fromNick)
		, recipient((withRecipient ? withRecipient : fromNick))
		, cmd("")
		, args()
		, input(withInput ? withInput : "")
		, instID(withInstID)
{
	const char* src = withInput ;
	char terminator = ' ' ;
	const char* arguments = NULL ;
	do
	{
		if ( src[0] == '"' || src[0] == '\'' )
		{
			terminator = src[0] ;
			++src ;
		}

		const char* space = strchr(src, terminator) ;
		if ( space == NULL )
		{
			args.push_back(src) ;
			break ;
		}

		args.push_back(std::string(src, (size_t)(space - src))) ;

		src = space + 1 ;
		terminator = ' ' ;

		while ( *src && isspace(*src) )
		{
			++src ;
		}

		if ( arguments == NULL )
			arguments = src ;
	}
	while ( *src ) ;

	cmd = args[0] ;
	args.erase(args.begin()) ;

	if ( cmd.size() > 32 )
	{
		cmd.resize(32) ;
	}

	for ( UINT32 i = 0 ; i < cmd.size() ; ++i )
	{
		cmd[i] = tolower(cmd[i]) ;
	}

	input = arguments ? arguments : "" ;
}

}

void
IRC_Command::Reply(Host::CellCommand::ResponseType, const char* format, ...) const
{
	if ( irc.m_running == false )
		return ;

	if ( format == NULL )
		dbugLog(_FLW, "called %s with NULL format", __FUNCTION__) ;

	static char buffer[1202] ;
	va_list vargs ;
	va_start(vargs, format) ;

	vsnprintf(buffer, 1200, format, vargs) ;
	va_end(vargs) ;

	buffer[1200] = 0 ;

	char name[128] ;
	irc_cmd_msg(irc.m_session, _getChannelName(Tell(), name, sizeof(name)), buffer) ;
}

