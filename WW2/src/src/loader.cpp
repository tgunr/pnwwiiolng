#include <syscfg.h> // Path is project specific.
// Date: Wed Apr 25 14:24:50 2001
//
// Load the SDM data from files, match it up with existing
// unique indexes from the server database where possible
// and upload the completed data into the server database.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include <libircclient.h>
#include <sys/stat.h>
#include <memory>

#include "sdm/sdmLoad.h"
#include "teul/teulTime.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseHandle.h"
#include "wwii/wwiiThreads.h"

#define DATA_DIRECTORY "data"
#define TERRAIN_DIRECTORY DATA_DIRECTORY "/terrain"
#define ORIGIN_FILE TERRAIN_DIRECTORY "/origin"

static const char* DEFAULT_IMPORT_FILE = DATA_DIRECTORY "/importlist.txt" ;
static const char* STRAT_DB_VAR = "STRAT_DB_ADDRESS" ;
static const char* cluster = (getenv("WWII_CLUSTER") ? getenv("WWII_CLUSTER") : "dev") ;

static bool useIrc = true ;
static irc_session_t* ircSession ;
static volatile bool ircConnected = false ;
static std::string nick ;
const char* g_appName = "stratLoader" ;
static char g_origin[80] = "private";

struct stat statBuf ;

DBCONNECTION dbConnection ;
extern DBHANDLE dbHandle ;

class IrcThread : public OpenThreads::Thread
{
public:
	IrcThread(void* s)
		: m_session(static_cast<irc_session_t*>(s))
	{}

	virtual void run()
	{
		irc_run(m_session);
	}

	virtual int cancel()
	{
		irc_session_t* session = m_session ;
		m_session = NULL ;

		irc_disconnect(session) ;
		OpenThreads::Thread::microSleep(50 * 1000);

		return OpenThreads::Thread::cancel();
	}

	~IrcThread()
	{
		if ( m_session != NULL )
			irc_disconnect(m_session) ;
	}

private:
	irc_session_t* m_session ;
};

static IrcThread* ircThread = NULL ;

SCHAR* _getValidLine(FILE* f,SINT32* line)
{
static char s[1024];
SINT32 len;

	while(!feof(f))
	{
		if ( fgets(s,512,f) == NULL )
            return NULL;

		(*line)++;

		if(feof(f))
			return NULL;

		// # in first coloumn means a comment
		if(s[0] == '#')
			continue;

		// Whitespace dont matter
		if(isspace(s[0]))
			continue;

		len = strlen(s);
		s[len-1] = '\0';

		return s;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////
// IRC Functions

static void
_ircConnected(irc_session_t * /*session*/, const char * /*event*/, const char * /*origin*/, const char ** /*params*/, unsigned int /*count*/)
{
	ircConnected = true ;
}

static bool
_ircConnect()
{
	if ( useIrc == false )
		return false ;

	if ( ircSession != NULL )
		return true ;

	const char* ircServer = getenv("WWII_IRC_SERVER") ;
	if ( ircServer == NULL )
		return false ;

	irc_session_t* s = NULL ;
	irc_callbacks_t callbacks ;

	memset(&callbacks, 0, sizeof(irc_callbacks_t)) ;
	callbacks.event_connect = _ircConnected ;

	s = irc_create_session(&callbacks) ;
	if ( s == NULL )
	{
		fprintf(stderr, "could not open IRC session\n") ;
		return false ;
	}

	ircConnected = false ;
	if ( irc_connect(s, ircServer, 6667, "blooGit", nick.c_str(), nick.c_str(), "Strat Loader") )
	{
		fprintf(stderr, "connect to shivers:irc failed: %s\n", irc_strerror(irc_errno(s))) ;
		irc_destroy_session(s) ;
		return false ;
	}

	ircThread = new IrcThread(s);
	ircThread->start();

	teulSyncClock() ;
	UINT32 cutoff = teulTime() + 30 ;
	while ( ircConnected == false && teulTime() < cutoff )
	{
		OpenThreads::Thread::microSleep(500 * 1000) ;
		teulSyncClock() ;
	}

	if ( ircConnected == false )
	{
		fprintf(stderr, "IRC session connect timed out\n") ;
		irc_disconnect(s) ;
		irc_destroy_session(s) ;
		return false ;
	}

	ircSession = s ;

	return true ;
}

static void
_ircSendMessage(const char* channel, const char* message)
{
	if ( !_ircConnect() )
		return ;

	char channelName[128] ;
	if ( channel[0] == '-' )
	{
		sprintf(channelName, "#%s%s", cluster, channel) ;
	}
	else
		strcpy(channelName, channel) ;

	irc_cmd_msg(ircSession, channelName, message) ;
}

static void
_ircStart()
{
	if ( useIrc == false )
		return;

	char message[80 + sizeof(g_origin)] ;
	sprintf(message, "%s terrain: importing new terrain from %s", cluster, g_origin) ;
	_ircSendMessage("-sys,#servers", message) ;
}

static void
_ircFinish(const TeulTimeMS& timeTaken)
{
	if ( ircThread == NULL || ircSession == NULL || ircConnected == false )
		return ;

	FLOAT32 seconds = ((FLOAT32)timeTaken) / 1000.0f ;
	char message[128] ;
	sprintf(message, "%s terrain: import finished - took %.2f seconds", cluster, seconds) ;

	_ircSendMessage("-sys,#servers", message) ;

	OpenThreads::Thread::microSleep(500 * 1000) ;
	delete ircThread ;

	ircThread = NULL ;
}

//////////////////////////////////////////////////////////////////////
// Describe usage of the loader.

static NORETURN_FN void
usage()
{
	fprintf(stderr, "Usage: %s [-x tarfile] [-d directory] [-h dbHost] [-I] [-O] [filename]\n", g_appName) ;
	fprintf(stderr, " -x tarfile      Extract data from tarfile into /playnet/ to populate terrain\n") ;
	fprintf(stderr, " -d directory    Change directory to specified before starting\n") ;
	fprintf(stderr, " -h dbHost       Specify IP of database [default: specified in %s]\n", STRAT_DB_VAR) ;
	fprintf(stderr, " -I              Disable irc\n");
	fprintf(stderr, " -O              Disable orgin file usage\n");
	fprintf(stderr, " filename        Name of import file [default: %s]\n", DEFAULT_IMPORT_FILE) ;
	exit(1) ;
}

//////////////////////////////////////////////////////////////////////
// Get the next argument from a command line.

static char*
getArgument(int argc, char** argv, int& i)
{
	if ( argv[i][2] != 0 )
		return argv[i] + 2 ;
	else if ( i + 1 < argc && argv[i+1][0] != '-' )
		return argv[++i] ;
	return NULL ;
}

//////////////////////////////////////////////////////////////////////
// Attempt to set the current working directory

static void
_changeDirectory(const char* cwd)
{
	if ( cwd == NULL )
		return ;

	const int cwdResult = chdir(cwd) ;
	if ( cwdResult != 0 )
	{
		fprintf(stderr, "*** Unable to change directory to %s [%d]\n", cwd, cwdResult) ;
		exit(cwdResult) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Check for a data and terrain directory

static void
_checkDirectoryStructure(const char* cwd)
{
	// Linux directory checking
	int result = stat(DATA_DIRECTORY, &statBuf) ;
	if ( result != 0 )
	{
		fprintf(stderr, "*** Unable to access %s/%s directory, check working directory\n", cwd, DATA_DIRECTORY) ;
		exit(result) ;
	}

	result = stat(TERRAIN_DIRECTORY, &statBuf) ;
	if ( result == 0 )
		return ;

	result = OS_MKDIR(TERRAIN_DIRECTORY, 0777) ;
	if ( result == 0 )
	{
		printf("# Created terrain directory\n") ;
		return ;
	}

	fprintf(stderr, "*** Unable to access %s/%s directory and unable to create", cwd, TERRAIN_DIRECTORY) ;
	exit(result) ;
}

//////////////////////////////////////////////////////////////////////
// Extract tar file

static void
_extract(const char* tarFile)
{
	if ( tarFile == NULL )
		return ;

	int result = stat(tarFile, &statBuf) ;
	if ( result != 0 )
	{
		fprintf(stderr, "Cannot stat tar file %s\n", tarFile) ;
		exit(result) ;
	}

	std::string cmd = "tar zxf " ;
	cmd += tarFile ;

	result = system(cmd.c_str()) ;
	if ( result != 0 )
	{
		fprintf(stderr, "Extract failed: [%s returned %d]\n", cmd.c_str(), result) ;
		exit(result) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Check for an origin file

static void
_checkOriginFile(const char* cwd)
{
	FILE* fp = fopen(ORIGIN_FILE, "rb") ;
	if ( fp == NULL )
	{
		fprintf(stderr, "Unable to access %s/%s\n", cwd, ORIGIN_FILE) ;
		exit(1) ;
	}

	if ( fgets(g_origin, sizeof(g_origin) - 1, fp) == NULL )
	{
	    fprintf(stderr, "Unable to read %s/%s\n", cwd, ORIGIN_FILE) ;
	    exit(1) ;
	}

	fclose(fp) ;
}

//////////////////////////////////////////////////////////////////////
//

int
main(int argc,char** argv)
{
	g_appName = argv[0] ;

	SCHAR* filename;
	//LOGICAL moreFiles = TRUE;
	FILE* importFile;
	SINT32 line;

	const char* cwd = NULL ;
	const char* db = getenv(STRAT_DB_VAR) ;
	const char* importFileName = NULL ;
	const char* tarFile = NULL ;

	bool argsOk = true ;
	bool useOrigin = true;

	for ( int i = 1 ; i < argc ; ++i )
	{
		if ( argv[i][0] == '-' )
		{
			switch ( argv[i][1] )
			{
				case '?':
					usage() ;
				break ;
				case 'h':		// Database Host
					db = getArgument(argc, argv, i) ;
					if ( db == NULL )
						usage() ;
				break ;
				case 'd':
					cwd = getArgument(argc, argv, i) ;
					if ( cwd == NULL )
						usage() ;
				break ;
				case 'x':
					// Extract files from tar file
					tarFile = getArgument(argc, argv, i) ;
					if ( tarFile == NULL )
						usage() ;
				break ;
				case 'I':
					useIrc = false ;
				break ;
				case 'O':
					useOrigin = false;
				break;
				case 0:
					fprintf(stderr, "Invalid argument '-'\n") ;
					argsOk = false ;
				break ;
				default:
					fprintf(stderr, "Unrecognized switch: %c\n", argv[i][1]) ;
					argsOk = false ;
				break ;
			}
		}
		else if ( importFileName != NULL )
			usage() ;
		else
			importFileName = argv[i] ;
	}

	if ( argsOk == false )
		usage() ;

	if ( db == NULL )
	{
		fprintf(stderr, "No database specified or no %s environment variable\n", STRAT_DB_VAR) ;
		exit(1) ;
	}

	if ( cwd == NULL )
		cwd = "." ;

	_changeDirectory(cwd) ;

	_checkDirectoryStructure(cwd) ;

	_extract(tarFile) ;

	if ( useOrigin )
		_checkOriginFile(cwd) ;

	if ( importFileName == NULL )
		importFileName = DEFAULT_IMPORT_FILE ;

	importFile = fopen(importFileName, "r");
	if ( importFile == NULL )
	{
		// We are broke, do not pass go
		fprintf(stderr, "Unable to access %s/%s\n", cwd, importFileName) ;
		exit(1) ;
	}

	nick = cluster ;
	nick += "Loader" ;
	nick[0] = toupper(nick[0]) ;

	_ircStart() ;
	teulSyncClock() ;
	TeulTimeMS started = teulClock64 ;

	threadsReadDBEnvironment("WWII_DB_ADDRESS", db) ;

	if ( dbseInitStratLoader() < 0 )
	{
		fprintf(stderr, "Connection to database %s failed\n", db) ;
		exit(1) ;
	}

	bool errors = false ;
	while( (filename = _getValidLine(importFile,&line)) != NULL)
	{
		if ( dbseLoadFile(filename) != 0 )
			errors = true ;
	}

	if ( true == errors )
	{
		_ircSendMessage("-sys,#servers", "Load failed") ;
		OpenThreads::Thread::microSleep(1 * 1000 * 1000) ;
		fprintf(stderr, "Unable to load terrain because of errors\n") ;
		dbseLoadFile(NULL) ;
		exit(1) ;
	}

	dbseLoadFile(NULL) ;

	dbseResolveReferences();

	// The SDM data doesn't handle its unique IDs very well and will
	// assign a new Unique ID to an object that has its name or
	// position changed, so we maintain our own unique indexes.
	dbseSaveIndexChanges() ;

	dbseCloseLoader();

	teulSyncClock() ;
	_ircFinish(teulClock64 - started) ;

	return 0;
}
