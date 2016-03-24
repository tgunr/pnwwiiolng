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

#include "sdm/sdmLoad.h"
#include "teul/teulTime.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseHandle.h"
#include "wwii/wwiiThreads.h"

#include "sdm/sdmConfigVars.h"
#include "wwii/wwiidbConfigVars.h"
#include "server/serverConfigVars.h"

static bool useIrc = true;
static irc_session_t* ircSession;
static std::atomic<bool> ircConnected { false };
static std::string nick;
const char* g_appName = "stratLoader";
static char g_origin[80] = "private";

struct stat statBuf;

DBCONNECTION dbConnection;
extern DBHANDLE dbHandle;

class IrcThread : public PNThread
{
public:
	IrcThread(void* s)
		: PNThread()
		, m_session(static_cast<irc_session_t*>(s))
	{}

	virtual void run() override
	{
		irc_run(m_session);
	}

	virtual void onCancel() override
	{
		irc_session_t* session = m_session;
		m_session = nullptr;

		irc_disconnect(session);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	virtual ~IrcThread()
	{
		if ( m_session != nullptr )
			irc_disconnect(m_session);
	}

private:
	irc_session_t* m_session;
};

static IrcThread* ircThread = nullptr;

SCHAR* _getValidLine(FILE* f,SINT32* line)
{
	static char s[1024];
	SINT32 len;

	while(!feof(f))
	{
		if ( fgets(s, sizeof(s) ,f) == nullptr )
            return nullptr;

		(*line)++;

		if(feof(f))
			return nullptr;

		// # in first coloumn means a comment
		if(s[0] == '#')
			continue;

		// Whitespace dont matter
		if(isspace(s[0]))
			continue;

		len = strlen(s);
		while ( len > 0 && isspace(s[len-1]) )
			s[--len] = 0;
		if ( len == 0 )
			s[0] = 0;

		return s;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// IRC Functions

static void
_ircConnected(irc_session_t * /*session*/, const char * /*event*/, const char * /*origin*/, const char ** /*params*/, unsigned int /*count*/)
{
	ircConnected = true;
}

static bool
_ircConnect()
{
	if ( useIrc == false )
		return false;

	if ( ircSession != nullptr )
		return true;

	const char* const ircServer = cv_ircServer.GetString();
	if ( ircServer == nullptr || *ircServer == '\0' )
	{
		useIrc = false;
		return false;
	}

	irc_session_t* s = nullptr;
	irc_callbacks_t callbacks;

	memset(&callbacks, 0, sizeof(irc_callbacks_t));
	callbacks.event_connect = _ircConnected;

	s = irc_create_session(&callbacks);
	if ( s == nullptr )
	{
		fprintf(stderr, "could not open IRC session\n");
		return false;
	}

	ircConnected = false;
	if ( irc_connect(s, ircServer, 6667, cv_ircPassword.GetString(), nick.c_str(), nick.c_str(), "Strat Loader") )
	{
		fprintf(stderr, "connect to shivers:irc failed: %s\n", irc_strerror(irc_errno(s)));
		irc_destroy_session(s);
		return false;
	}

	ircThread = new IrcThread(s);
	ircThread->start();

	teulSyncClock();
	UINT32 cutoff = teulTime() + 30;
	while ( ircConnected == false && teulTime() < cutoff )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		teulSyncClock();
	}

	if ( ircConnected == false )
	{
		fprintf(stderr, "IRC session connect timed out\n");
		irc_disconnect(s);
		irc_destroy_session(s);
		return false;
	}

	ircSession = s;

	return true;
}

static void
_ircSendMessage(const char* channel, const char* message)
{
	if ( !_ircConnect() )
		return;

	char channelName[128];
	if ( channel[0] == '-' )
	{
		sprintf(channelName, "#%s%s", cv_clusterName.GetString(), channel);
	}
	else
		strcpy(channelName, channel);

	irc_cmd_msg(ircSession, channelName, message);
}

static void
_ircStart()
{
	if ( useIrc == false )
		return;

	char message[80 + sizeof(g_origin)];
	sprintf(message, "%s terrain: importing new terrain from %s", cv_clusterName.GetString(), g_origin);
	_ircSendMessage("-sys,#servers", message);
}

static void
_ircFinish(const TeulTimeMS& timeTaken)
{
	if ( ircThread == nullptr || ircSession == nullptr || ircConnected == false )
		return;

	FLOAT32 seconds = ((FLOAT32)timeTaken) / 1000.0f;
	char message[128];
	sprintf(message, "%s terrain: import finished - took %.2f seconds", cv_clusterName.GetString(), seconds);

	_ircSendMessage("-sys,#servers", message);

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	delete ircThread;

	ircThread = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Describe usage of the loader.

static NORETURN_FN void
usage()
{
	fprintf(stderr, "Usage: %s [-x tarfile] [-C directory] [-h dbHost] [-I] [-O] [filename]\n", g_appName);
	fprintf(stderr, " -x tarfile      Extract data from tarfile into data directory (default %s) to populate terrain\n", cv_dataDir.GetString());
	fprintf(stderr, " -C directory    Working directory [default: %s]\n", cv_dataDir.GetString());
	fprintf(stderr, " -h dbHost       Specify IP of database [default: specified in %s]\n", cv_wwiiDBAddress.GetString());
	fprintf(stderr, " -I              Disable irc\n");
	fprintf(stderr, " -O              Disable orgin file usage\n");
	fprintf(stderr, " filename        Name of import file [default: %s]\n", cv_importFile.GetString());
	exit(1);
}

//////////////////////////////////////////////////////////////////////
// Get the next argument from a command line.

static char*
getArgument(int argc, char** argv, int& i)
{
	if ( argv[i][2] != 0 )
		return argv[i] + 2;
	else if ( i + 1 < argc && argv[i+1][0] != '-' )
		return argv[++i];
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Check for a data and terrain directory

static void
_checkDirectoryStructure()
{
	int result = stat(cv_terrainSubdir.GetString(), &statBuf);
	if ( result == 0 )
		return;

	result = OS_MKDIR(cv_terrainSubdir.GetString(), 0777);
	if ( result == 0 )
	{
		printf("# Created terrain directory\n");
		return;
	}

	LogAbort("*** Unable to access or create %s/%s", cv_dataDir.GetString(), cv_terrainSubdir.GetString());
}

//////////////////////////////////////////////////////////////////////
// Extract tar file

static void
_extract(const char* tarFile)
{
	if ( tarFile == nullptr )
		return;

	int result = stat(tarFile, &statBuf);
	if ( result != 0 )
	{
		fprintf(stderr, "Cannot stat tar file %s\n", tarFile);
		exit(result);
	}

	std::string cmd = "tar zxf ";
	cmd += tarFile;

	result = system(cmd.c_str());
	if ( result != 0 )
	{
		fprintf(stderr, "Extract failed: [%s returned %d]\n", cmd.c_str(), result);
		exit(result);
	}
}

//////////////////////////////////////////////////////////////////////
// Check for an origin file

static void
_checkOriginFile()
{
	FILE* fp = fopen(cv_originFile.GetString(), "rb");
	if ( fp == nullptr )
	{
		LogAbort("Unable to access %s/%s"
				, cv_dataDir.GetString(), cv_originFile.GetString());
	}

	if ( fgets(g_origin, sizeof(g_origin) - 1, fp) == nullptr )
	{
		LogAbort("Unable to read %s/%s"
			, cv_dataDir.GetString(), cv_originFile.GetString());
	}

	fclose(fp);
}

//////////////////////////////////////////////////////////////////////
//

int
main(int argc,char** argv)
{
	g_appName = argv[0];
	Teul::ConfigVar::InitializeConfigVars();

	SCHAR* filename;
	//LOGICAL moreFiles = TRUE;
	FILE* importFile;
	SINT32 line;

	const char* importFileName = nullptr;
	const char* tarFile = nullptr;

	bool argsOk = true;
#if platOS != OS_WIN32
	bool useOrigin = true;
#else
	bool useOrigin = false;
#endif

	for ( int i = 1; i < argc; ++i )
	{
		char* arg = nullptr;
		if ( argv[i][0] == '-' )
		{
			switch ( argv[i][1] )
			{
				case '?':
					usage();
				break;
				case 'h':		// Database Host
					arg = getArgument(argc, argv, i);
					if ( arg == nullptr )
						usage();
					cv_wwiiDBAddress.SetString(arg);
					if ( cv_wwiiDBAddress.GetUint32() == 0 )
						LogAbort("Invalid database address: %s", arg);
				break;
				case 'd':		// Backwards compatability.
				case 'C':
					arg = getArgument(argc, argv, i);
					if ( arg == nullptr )
						usage();
					cv_dataDir.SetString(arg);
				break;
				case 'x':
					// Extract files from tar file
					tarFile = getArgument(argc, argv, i);
					if ( tarFile == nullptr )
						usage();
				break;
				case 'I':
					useIrc = false;
				break;
				case 'O':
					useOrigin = !useOrigin;
				break;
				case 0:
					fprintf(stderr, "Invalid argument '-'\n");
					argsOk = false;
				break;
				default:
					fprintf(stderr, "Unrecognized switch: %c\n", argv[i][1]);
					argsOk = false;
				break;
			}
		}
		else if ( importFileName != nullptr )
			usage();
		else
			importFileName = argv[i];
	}

	if ( argsOk == false )
		usage();

	if ( cv_wwiiDBAddress.GetString() == nullptr || *cv_wwiiDBAddress.GetString() == 0 )
		LogAbort("No database specified.");

	if ( cv_dataDir.GetString() == nullptr || *cv_dataDir.GetString() == 0 )
		cv_dataDir.SetString(".");

	int result = chdir(cv_dataDir.GetString());
	if ( result < 0 )
		LogAbort("Unable to access working directory %s: %d", cv_dataDir.GetString(), result);

	_checkDirectoryStructure();

	_extract(tarFile);

	if ( useOrigin )
		_checkOriginFile();

	if ( importFileName == nullptr )
		importFileName = cv_importFile.GetString();

	importFile = fopen(importFileName, "r");
	if ( importFile == nullptr )
		LogAbort("Unable to access import file %s/%s", cv_dataDir.GetString(), importFileName);

	nick = cv_clusterName.GetString();
	nick += "Loader";
	nick[0] = toupper(nick[0]);

	_ircStart();
	teulSyncClock();
	TeulTimeMS started = teulClock64;

	threadsReadDBEnvironment();

	if ( dbseInitStratLoader() < 0 )
		LogAbort("Database (%s:%s) connection failed.", cv_wwiiDBAddress.GetString(), cv_wwiiDBDefaultSchema.GetString());

	bool errors = false;
	while( (filename = _getValidLine(importFile,&line)) != nullptr)
	{
		if ( filename[0] != 0 && dbseLoadFile(filename) != 0 )
			errors = true;
	}

	if ( true == errors )
	{
		_ircSendMessage("-sys,#servers", "Load failed");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		fprintf(stderr, "Unable to load terrain because of errors\n");
		dbseLoadFile(nullptr);
		exit(1);
	}

	dbseLoadFile(nullptr);

	dbseResolveReferences();

	// The SDM data doesn't handle its unique IDs very well and will
	// assign a new Unique ID to an object that has its name or
	// position changed, so we maintain our own unique indexes.
	dbseSaveIndexChanges();

	dbseCloseLoader();

	teulSyncClock();
	_ircFinish(teulClock64 - started);

	return 0;
}
