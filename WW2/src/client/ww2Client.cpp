#include <syscfg.h> // Path is project specific.

#include "cfg/cfgPrefs.h"
#include "chat/chatClientMessages.h"
#include "chat/chatDefines.h"			// For MAX_CLIENT_CHANNELS
#include "chat/chatGroupPermissionTypes.h"
#include "chat/chatHandle.h"
#include "chat/chatMissionInvite.h"
#include "chat/chatServerInfo.h"
#include "chat/chatWaypointExchg.h"
#include "chat/chatWho.h"
#include "client/clientConnections.h"
#include "client/client.h"
#include "client/clientNetError.h"
#include "client/clientSquad.h"
#include "client/GameClient.h"
#include "client/ww2Client.h"
#include "client/ww2HostState.h"
#include "comm/commClientMessages.h"
#include "inst/instClient.h"
#include "inst/instDescriptor.h"
#include "inst/instListing.h"
#include "map/mapClientMessages.h"
#include "map/mapPersonaSummary.h"
#include "miss/missionClientMessages.h"
#include "miss/missionDetail.h"
#include "miss/missionResults.h"
#include "mm/mmSpawnClientMessages.h"
#include "mm/mmSpawnTypes.h"
#include "teul/teulConnectionInstruction.h"
#include "teul/teulKit.h"
#include "tut/TutorialManager.h"
#include "ui/uiChat.h"
#include "ui/uiMain.h"
#include "updt/updtPackers.h"
#include "updt/updtTypes.h"
#include "wwii/wwiiClientMessages.h"
#include "wwii/wwiiOpinionPoll.h"
#include "wwii/wwiiPermissions.h"
#include "wwii/wwiiSquadMsg.h"
#include "wwii/wwiiSquads.h"
#include "wwii/wwiiTypes.h"

// WWIIOL specific.
#include "chat/chatBrigadeControl.h"
#include "chat/chatBrigadeControlList.h"
#include "chat/chatMissionMember.h"
#include "chat/chatMotds.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapMissionList.h"
#include "map/mapSwingMeter.h"
#include "strt/strtCaptureState.h"
#include "strt/strtClientMessages.h"
#include "strt/strtDTL.h"
#include "strt/strtFacilityWeaponList.h"
#include "strt/strtTypes.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiRoomLists.h"
#include "omg/omgChat.hxx"
#include "teul/raknetConn.h"

using namespace ww2;

#include "fio/fio.h"

// How to log something to the WW2_Log.txt file.

void clientErrorLog(const char* const fmt, ...)
{
	FILE* fp = Fio::OpenLogFile();
	if ( fp == nullptr )
		return;

	const time_t now = teulTime();
	const UINT32 milis = (UINT32)(teulClock64 % 1000ULL);
	char myTime[49];
	char myYear[10];
#if platOS == OS_WIN32
	tm* myTm = localtime(&now);
	strftime(myTime, 48, "%a %b %d %X", myTm);
	strftime(myYear, 9, "%Y", myTm);
#else
  struct tm tmStruct;
	strftime(myTime, 48, "%a %b %d %X", localtime_r(&now, &tmStruct));
	strftime(myYear, 9, "%Y", localtime_r(&now, &tmStruct));
#endif
    if ( myTime[0] == 0 )
		strcpy(myTime, "unknown");

	fprintf(fp, "[%s.%03u %s] ", myTime, milis, myYear);

	va_list(args);
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	fprintf(fp, "\n");

	fclose(fp);
}

// default player info
static Teul::PlayerIdent _defaultPlayerIdent;

/************************************************************************************************
	_yield()
	Pause long enough for the OS to do stuff like, oh, receive packets,
	then call teulUpdateTransport to receive them.
	Thanks, teul.
*************************************************************************************************/

static void _yield()
{
	teulUpdateTransport(1);
	Teul::Connection::expire();
}

/************************************************************************************************
	Client
*************************************************************************************************/

struct ClientData
{
	int id; // our client id

	//bool cellClientActive;
	//bool mapClientActive;

	WWII_SIDE 		side;
	WWII_COUNTRY	country;
	FacilityWeaponList weapList;
	WEAPON_SERIAL 	weapSerial;
	WEAPON_SPEC		vehspec;
	UINT8			rank;
	MISSION_POST_REPLY missionpostreply;
	MM_POSTED_WEAPON commander;
	MISSION_RESULTS	missionresults;
	int despawntimer;
	bool enableDespawnTimer;
	ChatServerInfo chatServerInfo;
	bool haveChatServerInfo;
	bool canpost;
	MISS_LAUNCH_UPDATE launchupdate;
	unsigned int missionListUpdate;
	bool areachat;
	VEHICLE_REPLY vehReply;

	double x, y, z;
	double vx, vy, vz;
	double ax, ay, az;
	double pitch, roll, yaw;
	double wx, wy, wz;

	unsigned int state;

	unsigned int fireState;
	int fireOnetime[32];

	unsigned int turretMask;
	turret_t turretTraverse[MAX_TURRETS], turretElev[MAX_TURRETS];

	unsigned int visbits;

	unsigned int RideVehicleHostIndex;
	unsigned short RideSeatNum;

	bool wantChat;
	bool ishc;

	bool vehicleKilled;
	bool crewDead;

	unsigned int host;
	short port;

	ChatHandle	activewaypoint;		// handle of active waypoint
	SINT32	sidebalance;
	SINT32	issidedelayed;
	UCHAR	isspawndelayed;

	int waitmsg[Client::MAX_MSG_WAIT];
	int msgtime[Client::MAX_MSG_TIME];

	bool dontReconnect;

	unsigned int updatesSent;
	unsigned int hitsPrime;
};

struct ClientClassData
{
	struct SavedPersona
	{
		SavedPersona() :
			instance(1),
			country(-1),
			branch(-1),
			brigade(-1),
			mission(-1)
		{
		}

		bool Valid(){ return (country >= 0 && branch >= 0); }

		int instance;
		int country;
		int branch;
		int brigade;
		int mission;
	};

    ~ClientClassData() {} // On macOS, without this line Clang produces a number of link errors. TODO: Why?

	PersonaSummaries 					personas;
	PersonaSelection 					personaselected;
	BrigadeSelection 					brigadeselected;
	BrigadeSummaries					brigadeList;
	MissionDetail						selectedmission;
	BrigadeSelection					brigadedetail;
	FESpawnNotify						spawnnotify;
	std::string							missiondesc;
	std::vector<RoomListEntry> 			missionroom;
	std::vector<COMMANDED_WEAPON> 		commandedweapons;
	std::vector<FE_JOIN_PERM_REQUEST> 	joinrequests;
	std::vector<MissionDetail>			missions;
	std::vector<Waypoint>				waypoints;
	std::vector<MissionMember>			missionmembers;
	std::map<int, BrigadeControlList>	oics; // holds a list of current oics
	std::vector<BrigadeControlNotice>	brigadenotices; // keeps track of all brigade control notices
	ClientSquad							squad;
	SquadInvite							invitation;
	std::wstring						currentorders;
	MOTD_REPLY							motdreply;
	OpinionPoll							opinionpoll;
	MissionDetail						missionlookup;	// queried arbitrary mission
	unsigned int						missionlookupid; // arbitrary mission id
	SquadRecruitment					recruitment; // why does this have to be declared with the keyword struct?
	MissionInvite						missioninvite;
	Instance::Result					instance; // what instance we're currently in
	const Instance::ClientListing*		instances; // our last requested list of instances
	FLOAT64VECTOR						spawnoverride; // for use with tutorials; arbitrary spawning
	SavedPersona						saved; // our saved persona selection
	ChatPermissions						permissions; // our subscription permissions
	unsigned int						accountBalance; // number of seconds left on metered account
	CaptureState						capturestate; // our last received capture state
	ChatHandle							commander; // commander's chat handle of my multicrewed vehicle; 0 otherwise
	const Chat::MissionStatus*			missionstatus;
	bool								killedByAI;	// Were we killed by an AI unit?

	Teul::PlayerIdent					playerIdent; // player login data used to connect to hosts
	Teul::ServerAddr					cellIdent; // which cell server we connected to last (for reconnects)

	// these are our connection states
	HostState*							connections[Teul::Connection::NumHostTypes];
};

static int clientid;
static bool clientinited;

static void errorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error);

#if(0)
static void weapListCopy(FacilityWeaponList*, const FacilityWeaponList*);
#endif

static void weapListClear(FacilityWeaponList*);

//#define TEMP_PERMISSIONS

/************************************************************************************************
	Client
*************************************************************************************************/

Client::Client()
{
	_classdata = new ClientClassData{};
	_data = new ClientData{};
	_data->id = clientid++;
	_data->hitsPrime = 127979;

	// assume we're in the campaign instance
	_classdata->instance.m_status = Instance::_SUCCESS;
	_classdata->instances = nullptr;

	// clear our permissions to start with; the host will send them down
	memset(&_classdata->permissions, 0x0, sizeof(_classdata->permissions));

	_classdata->killedByAI = false;
	_classdata->commander = 0;
	_classdata->missionstatus = 0;

#ifdef TEMP_PERMISSIONS
	// for testing
	_classdata->permissions.m_limitRank[1] = 25;
	_classdata->permissions.m_limitRank[2] = 25;
	_classdata->permissions.m_limitRank[3] = 25;
	_classdata->permissions.m_permissionMask = ~0ULL;
	_classdata->permissions.m_limitSquadRank = Squad::GM;
#endif

	// init our host states; this order is important because the chat and
	// cell lookup the map host state and make it their parent so always
	// create the map state first
	_classdata->connections[Teul::Connection::Map] = new MapHostState(this);
	_classdata->connections[Teul::Connection::Chat] = new ChatHostState(this);
	_classdata->connections[Teul::Connection::Cell] = new CellHostState(this);

	// init out player login info
	_classdata->playerIdent = _defaultPlayerIdent;
}

Client::~Client()
{
	if(clientinited)
	{
		for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
		{
			HostState* state = _classdata->connections[i];

			if(state)
			{
				state->Reset();
				delete state;
			}

			_classdata->connections[i] = nullptr;
		}
	}

	weapListClear(&_data->weapList);

    for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
    {
        if(_classdata->connections[i])
        {
            delete _classdata->connections[i];
            _classdata->connections[i] = nullptr;
        }
    }

    delete _data;
	delete _classdata;
}

/************************************************************************************************
	ClientHostConnection::onError
	Handles errors during the lifetime of a connection from open to close.
*************************************************************************************************/

void ClientHostConnection::onError(Client* client, SINT32 error)
{
    // for raknet errors lets just end things here, its not a real connetion failure until teul goes down
    if (error == RN_SK_FAILED)
    {
        uiChatPrintLineColor(colorYELLOWALPHA, uiLocString("connect.netcode3.disabled").c_str());
        return;
    }
    
    Teul::Connection::onError(client, error);
    
	ww2::HostState* state = client->GetHostStateByType(m_type);
	if(!state) return;

	state->ClearWait();

	if(error < 0)
		error = -error;

	// tell our state about the error code
	state->OnError(this, error);
}

void ClientHostConnection::SetError(ww2::Client* client, SINT32 error)
{
	onError(client, error);
}

/************************************************************************************************
	ClientConnection::onLogin
	Handles a successful login attempt to the map/mission host; client independent
*************************************************************************************************/

void ClientHostConnection::onLogin(Client* const client)
{
	// let the client handle it
	ww2::HostState* state = client->GetHostStateByType(m_type);
	if(!state) return;

	// set our client's host state teul index
	// also flag that we're now logged in.
	if(state->Connect(this, HostState::CONNECTED))
		m_loggedIn = true;
	else
		close();

	// clear any waits
	state->ClearWait();
}

/************************************************************************************************
	ClientConnection::onLoginFail
	Handles a failed attempt to log in to the map/mission host (FATAL); client independent
*************************************************************************************************/

void ClientHostConnection::onLoginFail(Client* const client, SINT32 result)
{
	// let the client handle it
	ww2::HostState* state = client->GetHostStateByType(m_type);
	if(!state) return;

	if ( result < 0 )
		result = -result;

	// record any errors and reset our state data
	// if this is a fatal error
	state->OnError(this, result);

	// clear any waits
	state->ClearWait();
}

/************************************************************************************************
	ClientHostConnection::onConnect
	Handle successful connection open to map mission host, send authentication/login.
*************************************************************************************************/

void ClientHostConnection::onConnect(ww2::Client* client)
{
	ww2::HostState* state = client->GetHostStateByType(m_type);
	if(!state) return;

	if(state->Connect(this, HostState::CONNECTING))
		login();
	else
		close();

	// clear any waits
	state->ClearWait();
}

/************************************************************************************************
	_shutdownCellConnection
	Close down a client's cell host connection and unlink it from the client.
	Does not take responsibility for notifying the player or the client otherwise.
*************************************************************************************************/
#if(0)
static void _shutdownCellConnection(Client* const client, SINT32 teulIndex)
{
	client->CellState()->Reset();
}
#endif

/************************************************************************************************
	Client::Login
	throws ClientNetError
*************************************************************************************************/

int Client::Login(const char* hostname, UINT16 port, bool wantChat)
{
	unsigned int host = 0;
	teulGetHost(hostname, &host);
	return Login(host, port, wantChat);
}

/************************************************************************************************
	Teul::ConnectionInstruction::receive
	Handles a command from the host to connect to an additional server.
*************************************************************************************************/

///TODO:PERSIST: This needs breaking up; as it's written,
///            : it will cause the client to stall while
///            : making the connection. Yuck.
///            : For now, it doesn't matter, because
///            : it only happens when connecting to the
///            : cell host.

SINT32
Teul::ConnectionInstruction::receive(SINT32 teulIndex)
{
	clientErrorLog("tciRx ro:%u ip:%s:%u", (UINT32)role(), teulGetAddress(hostIP()), port());

	// role() will be one of Unknown, Chat() or Cell(),
	// hostIP() will be the last octet of the IP address,
	// port() will be the port number to connect to.

	Client* const client = Client::FromTeulIndex(teulIndex);
	if(!client)
	{
		clientErrorLog("tciNoClient");
		return -EINVAL;
	}

	ww2::HostState* cellHostState = client->CellState();
	if(!cellHostState || !cellHostState->CanConnect())
	{
		clientErrorLog("tciNoCell");
		return -EINVAL;
	}

	// if we're connected, kill our current connection
	if(cellHostState->Open())
	{
		clientErrorLog("*tciClose*");
		cellHostState->Disconnect();
	}

	// start a connection attempt
	Teul::ServerAddr server = { hostIP(), port() };

	// remember our last cell connection identity
	client->SetCellIdent(hostIP(), port());

	const Teul::PlayerIdent* playerIdent = client->GetPlayerIdent();
	if(!playerIdent)
	{
		clientErrorLog("tciNP");
		return -EINVAL;
	}

	ClientHostConnection* conn = new ClientHostConnection(Teul::Connection::Cell, server, *playerIdent, 30*1000);

	if(!conn->start(client))
	{
		clientErrorLog("tci!Start");
		return -EINVAL;
	}

	// wait for the connection to complete
	if(!cellHostState->Ready(conn->m_timeoutMS + 5000))
	{
		clientErrorLog("tci!Rdy");
		return -EINVAL;
	}

	if (!cellHostState->Connected())
	{
		clientErrorLog("tciNoCnx");
		return -EINVAL;
	}

	// indicate we're connected to the cellhost
	//client->SpawnComplete();

	return 0;
}

#include "include/vars.h"
int Client::Login(unsigned int host, UINT16 port, bool wantChat)
{
    int retval = -EINVAL;

	if(!clientinited) return retval;

	TeulTimeMS timeout = 0LL;
	static const int CONNECTION_TIMEOUT = 30 * 1000; // nobody's gonna wait more than 30 secs before force quitting

	if ( port == 0 || port == 27017 )
		port = WWII::Net::Ports::MapHost;

	_data->wantChat = wantChat;
	_data->ishc = false;

	SetActiveHost(host, port);

	StatusMessage("loading.network.host");

	HostState* mapHostState = MapState();
	if(!mapHostState) return retval;

	Teul::ServerAddr server		= { host, static_cast<UINT16>(port) };
	Teul::PlayerIdent& ident	= _classdata->playerIdent;

	ClientHostConnection* conn = new ClientHostConnection(Teul::Connection::Map, server, ident, 30*1000);
	if (!conn->start(this))
	{
		dbugLog(_FLI, "Failed to start host connection");
		throw ClientNetError("connect.login.auth.failed");
	}

	StatusMessage("loading.network.loggingin");

	while ( conn->complete() == false )
	{
		_yield();
	}

	if ( conn->m_connected == false || conn->m_loggedIn == false )
	{
		if ( conn->m_connected )
		{
			if(mapHostState->retryCount > 0)
				throw ClientNetError("connect.login.auth.reconnectfailed");
			throw ClientNetError("connect.login.auth.failed");
		}
		else
			throw ClientNetError("connect.login.auth.timeout");
	}

	if(wantChat && !ChatState()->connection && !ChatState()->IsReconnecting())
	{
		StatusMessage("loading.network.chat");

		// Wait for the Chat::ServerInfo packet to arrive.
		// Wait for the chat server info stuff to arrive.
		timeout = teulClock64 + CONNECTION_TIMEOUT;
		while(!_data->haveChatServerInfo)
		{
			if ( teulClock64 >= timeout )
				throw ClientNetError("connect.login.chat.timeout");
			_yield();
		}

		dbugLog(_FLI, "connecting to chat");
		///TODO:PERSIST: Not sure if we want to block on the attempt, but...
		if(!ConnectToChatHost(true))
		{
			///TODO:PERSIST: connect.login.chat.timeout is the wrong error.
            throw ClientNetError("connect.login.chat.timeout");
		}
	}
	else
		dbugLog(_FLW, "not looking for chat");

	StatusMessage("loading.network.complete");

	SetUpdatesSent(0);

	return mapHostState->lastError;
}

/************************************************************************************************
	Client::Logout
*************************************************************************************************/

int Client::Logout()
{
	_yield();

	for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
		_classdata->connections[i]->Reset();

	return 0;
}

/************************************************************************************************
	Client::MapConnection
*************************************************************************************************/

int Client::MapConnection() const
{
	return _classdata->connections[Teul::Connection::Map]->Index();
}

/************************************************************************************************
	Client::CellConnection
*************************************************************************************************/

int Client::CellConnection() const
{
	return _classdata->connections[Teul::Connection::Cell]->Index();
}

/************************************************************************************************
	Client::ChatConnection
*************************************************************************************************/

int Client::ChatConnection() const
{
	return _classdata->connections[Teul::Connection::Chat]->Index();
}

/************************************************************************************************
	Client::MapState
*************************************************************************************************/

ww2::HostState* Client::MapState()
{
	return _classdata->connections[Teul::Connection::Map];
}

/************************************************************************************************
	Client::CellState
*************************************************************************************************/

ww2::HostState* Client::CellState()
{
	return _classdata->connections[Teul::Connection::Cell];
}

/************************************************************************************************
	Client::ChatState
*************************************************************************************************/

ww2::HostState* Client::ChatState()
{
	return _classdata->connections[Teul::Connection::Chat];
}

/************************************************************************************************
	Client::Connected
*************************************************************************************************/

bool Client::Connected()
{
	return MapState()->Connected();
}

/************************************************************************************************
	Client::ResetMissionData
*************************************************************************************************/

void Client::ResetMissionData()
{
	_classdata->missionroom.clear();
	_classdata->commandedweapons.clear();
	_classdata->joinrequests.clear();
	_classdata->waypoints.clear();
	_classdata->missionmembers.clear();
	_classdata->currentorders = L"";
}

/************************************************************************************************
	Client::RequestPersonaSummaries
*************************************************************************************************/

int Client::RequestPersonaSummaries()
{
	PersonaSummaries request;
	return request.send(MapConnection());
}

/************************************************************************************************
	Client::RequestPersonaSummaries
*************************************************************************************************/

int Client::RequestPersonaSelection(PersonaSummary const& persona)
{
	PersonaSelection request(persona);
	if(persona.m_personaID == 0)
		request.m_status = PSS_NEW;

	return request.send(MapConnection());
}

/************************************************************************************************
	Client::RequestMissionWeapList
*************************************************************************************************/

int Client::RequestMissionWeapList(FULL_OBJ_ID const& fac, bool allweapons, bool isrefresh)
{
	return  missionSendWeaponListRequest(MapConnection(), fac, allweapons, isrefresh);
}

/************************************************************************************************
	Client::RequestMissionWeapPost
*************************************************************************************************/

int Client::RequestMissionWeapPost(FULL_OBJ_ID const& fac, const WEAPON_SPEC spec, bool immediatespawn)
{
	return mmSendPostRequest(MapConnection(), fac, spec, (LOGICAL)immediatespawn);
}

/************************************************************************************************
	Client::RequestMissionDetail
*************************************************************************************************/

int Client::RequestMissionDetail(int missionID, bool join)
{
	if(join)
		ResetMissionData();

	missionSendDetailRequest(MapConnection(), missionID, join);

	return 0;
}

/************************************************************************************************
	Client::RequestMissionLookup
*************************************************************************************************/

int Client::RequestMissionLookup(int missionID)
{
	_classdata->missionlookup	= MissionDetail();
	_classdata->missionlookupid	= missionID;

	RequestMissionDetail(missionID, false);

	return 0;
}

/************************************************************************************************
	Client::RequestRecruiters
*************************************************************************************************/

int Client::RequestRecruiters(int language)
{
	return chatSendRecruiterRequest(ChatConnection(), (UCHAR)language);
}

/************************************************************************************************
	Client::LookupBrigadeOIC
*************************************************************************************************/

BrigadeControlList* Client::LookupBrigadeOIC(int brigadeID) const
{
	if(_classdata->oics.find(brigadeID) != _classdata->oics.end())
		return &_classdata->oics[brigadeID];

	return nullptr;
}

/************************************************************************************************
	Client::RequestClientSpawn
*************************************************************************************************/

int Client::RequestClientSpawn(WEAPON_SERIAL const& serial, bool wantMobileSpawn)
{
	return mmSendClientSpawn(MapConnection(), serial, wantMobileSpawn);
}

/************************************************************************************************
	Client::RequestBrigadeSelection
*************************************************************************************************/

int Client::RequestBrigadeSelection(unsigned int brigadeID)
{
	BrigadeSelection selection(BRIG_SELECT, brigadeID);
	return selection.send(MapConnection());
}

/************************************************************************************************
	Client::RequestBrigadeDetail
*************************************************************************************************/

int Client::RequestBrigadeDetail(unsigned int brigadeID)
{
	BrigadeSelection selection(BRIG_DETAIL, brigadeID);
	return selection.send(MapConnection());
}

/************************************************************************************************
	Client::RequestBrigadeSelection
*************************************************************************************************/

int Client::RequestMissionList()
{
	_data->missionListUpdate = 0;

	MissionList missionlist;
	return missionlist.send(MapConnection());
}

int Client::RequestQuickMissionList() {
	_data->missionListUpdate = 0;
	MissionList missionlist;
	return missionlist.sendQuick(MapConnection());
}

/************************************************************************************************
	Client::RequestBrigadeList
*************************************************************************************************/

int Client::RequestBrigadeList()
{
	BrigadeSummaries summaries;
	return summaries.send(MapConnection());
}

/************************************************************************************************
	Client::RequestMissionPost
*************************************************************************************************/

int Client::RequestMissionPost(MISSION_POST_REQ const& request)
{
	return missionSendPostRequest(MapConnection(), &request);
}

/************************************************************************************************
	Client::RequestMissionWeapCancel
*************************************************************************************************/

int Client::RequestMissionWeapCancel()
{
	int retval =  mmSendVehicleCancel(MapConnection(), _data->weapSerial);
	memset(&_data->weapSerial, 0x0, sizeof(_data->weapSerial));
	return retval;
}

/************************************************************************************************
	Client::RequestMissionWeapCancel
*************************************************************************************************/

void Client::ClearMissionWeap()
{
	memset(&_data->weapSerial, 0x0, sizeof(_data->weapSerial));
}

/************************************************************************************************
	Client::RequestMissionCrew
*************************************************************************************************/

int Client::RequestMissionCrew(unsigned int numcrew)
{
	return mmSendRequestCrew(MapConnection(), _data->weapSerial, numcrew);
}

/************************************************************************************************
	Client::RequestMissionPostList
*************************************************************************************************/

int Client::RequestMissionPostList(FULL_OBJ_ID const& fac)
{
	return mmSendPostListRequest(MapConnection(), fac);
}

/************************************************************************************************
	Client::RequestMissionPostList
*************************************************************************************************/

int Client::RequestJoin(WEAPON_SERIAL const& serial, ChatHandle const& commander)
{
	ClearJoinRequests();
	_classdata->commander = commander;
	return mmSendJoinRequest(MapConnection(), serial);
}

/************************************************************************************************
	Client::SendAcceptCrew
*************************************************************************************************/

int Client::SendAcceptCrew(unsigned int joinrequestid)
{
	return mmSendAcceptCrew(MapConnection(), joinrequestid);
}

/************************************************************************************************
	Client::ExitMission
*************************************************************************************************/

int Client::ExitMission(bool cancel)
{
	UINT32 cancelID = (cancel) ? _classdata->selectedmission.m_missionID: 0;

	memset(&_data->weapSerial, 0x0, sizeof(_data->weapSerial));

	_classdata->selectedmission.m_isCurrent = false;
	_classdata->waypoints.clear();
	_data->activewaypoint = 0;

	return missionSendExit(MapConnection(), cancelID);
}

/************************************************************************************************
	Client::SendChatMessage
*************************************************************************************************/

int Client::SendChatMessage(unsigned int txmask, const char* message)
{
	if(_data->areachat)
		txmask |= (1<<MAX_CLIENT_CHANNELS);

	return chatSendMessage(ChatConnection(), (UINT16)txmask, message);
}

/************************************************************************************************
	Client::RequestMotd
*************************************************************************************************/

int Client::RequestMotd(unsigned int id)
{
	return chatSendMotdRequest(ChatConnection(), id);
}

/************************************************************************************************
	ProcessWeapList
*************************************************************************************************/
// clientProcessWeapList
void Client::ProcessWeapList(struct ACTION_OBJECT* a)
{
	weapListClear(&_data->weapList);
	_data->weapList.packer(TEUL_UNPACK, a);
}

/************************************************************************************************
	SetWaypoint
*************************************************************************************************/

int Client::SetWaypoint(Waypoint const& waypoint)
{
	SINT32 result	= 0;
	bool found = false;

	std::vector<Waypoint>::iterator it = _classdata->waypoints.begin();
	for(;it != _classdata->waypoints.end(); ++it)
	{
		if(it->handle() == waypoint.handle())
		{
			*it = waypoint;
			found = true;
			break;
		}
	}

	// if we didn't find it
	if(!found)
	{
		// if it isn't local or it is local and it's not a contact report or if we're not in the campaign instance
		if(!waypoint.local() || waypoint.type() != WPT_ENEMY || !GetInstance().isCampaign())
		{
			// save it
			_classdata->waypoints.push_back(waypoint);

			// tell the tutorial layer about the waypoint
			int type = (waypoint.type() == WPT_ENEMY) ? TUT_MAP_REPORT: TUT_MAP_WAYPOINT;
			TUT_MGR->DoEvent(TUT_EVENT_MAP, &type);
		}
	}

	// most waypoints are shared, but a few are not such as the mission origin and target waypoints
	// they are added locally when the user joins the mission
	if(waypoint.shared())
	{
		WAYPOINT_XFER transfer(waypoint, WPXT_REQUEST);
		result = transfer.send(ChatConnection());
	}

	return result;
}
/************************************************************************************************
	GetKilledByAI
*************************************************************************************************/

bool ww2::Client::GetKilledByAI()
{
	return _classdata->killedByAI;
}

/************************************************************************************************
	ActiveWaypoint
*************************************************************************************************/

void Client::SetActiveWaypoint(ChatHandle handle)
{
	_data->activewaypoint = handle;
	int type = TUT_MAP_ACTIVATE;
	TUT_MGR->DoEvent(TUT_EVENT_MAP, &type);
}

ChatHandle Client::ActiveWaypointHandle() const
{
	return _data->activewaypoint;
}

Waypoint Client::ActiveWaypoint() const
{
	std::vector<Waypoint>::const_iterator it = _classdata->waypoints.begin();
	for(;it != _classdata->waypoints.end(); ++it)
	{
		if(it->handle() == _data->activewaypoint)
			return *it;
	}

	return Waypoint();
}

/************************************************************************************************
	IsMissionLeader
*************************************************************************************************/

bool Client::IsMissionLeader() const
{
	return (_classdata->selectedmission.m_isCurrent) ? _classdata->personas.m_callsign == _classdata->selectedmission.m_poster: false;
}

/************************************************************************************************
	RemoveWaypoint
*************************************************************************************************/

int Client::RemoveWaypoint(ChatHandle handle)
{
	int result = 0;

	if(handle == _data->activewaypoint)
		_data->activewaypoint = 0;

	std::vector<Waypoint>::iterator it = _classdata->waypoints.begin();
	for(;it != _classdata->waypoints.end(); ++it)
	{
		if(it->handle() == handle)
		{
			if(it->local())
			{
				if(it->type() != WPT_ORIGIN && it->type() != WPT_TARGET)
					_classdata->waypoints.erase(it);
			}
			else if(IsMissionLeader())
			{
				WAYPOINT_CONTROL control(handle, WPCTRL_REMOVE);
				result = control.send(ChatConnection());

				// go ahead and kill it so we don't spam
				_classdata->waypoints.erase(it);
			}

			break;
		}
	}

	return result;
}

std::vector<Waypoint> const& Client::Waypoints() const
{
	return _classdata->waypoints;
}

// do we currently have a mobile spawn; only way to tell is by checking out map waypoints
bool Client::MobileSpawnMission() const
{
	for(size_t i = 0; i < _classdata->waypoints.size(); i++)
	{
		Waypoint& wp = _classdata->waypoints.at(i);

		if(wp.type() == WPT_MOBILESPAWN)
			return true;
	}

	return false;
}

const Chat::MissionStatus* Client::MissionStatus() const
{
	return _classdata->missionstatus;
}

const Teul::PlayerIdent* Client::GetPlayerIdent() const
{
	return &_classdata->playerIdent;
}

const Teul::ServerAddr* Client::GetCellIdent() const
{
	return &_classdata->cellIdent;
}

/************************************************************************************************
	GetHostState
*************************************************************************************************/

ww2::HostState* Client::GetHostStateByIndex(int teulIndex)
{
	if(teulIndex < 0)
		return nullptr;

	for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
	{
		if(_classdata->connections[i]->Index() == teulIndex)
			return _classdata->connections[i];
	}

	return nullptr;
}

ww2::HostState* Client::GetHostStateByType(int type)
{
	if(type >= 0 && type < Teul::Connection::NumHostTypes)
		return _classdata->connections[type];

	return nullptr;
}

/************************************************************************************************
	SetPlayerIdent
*************************************************************************************************/

void Client::SetPlayerIdent(playerid_t playerID, UINT32 otp, unsigned int appVersion)
{
	_classdata->playerIdent.m_playerID		= playerID;
	_classdata->playerIdent.m_otp			= otp;
	_classdata->playerIdent.m_clientVersion	= appVersion;
}

/************************************************************************************************
	SetCellIdent
*************************************************************************************************/

void Client::SetCellIdent(int hostIP, int port)
{
	_classdata->cellIdent.m_hostIP	= hostIP;
	_classdata->cellIdent.m_port		= port;
}

/************************************************************************************************
	ProcessWaypoint
*************************************************************************************************/

void Client::ProcessWaypoint(Waypoint const& waypoint, int type)
{
	switch(type)
	{
		case -1: // clear list
			_classdata->waypoints.clear();
			_data->activewaypoint = 0;
		break;
		case WPXT_REQUEST:
		case WPXT_POST:
		{
			if(waypoint.handle() != 0)
			{
				std::vector<Waypoint>::iterator it = _classdata->waypoints.begin();
				for(; it != _classdata->waypoints.end(); ++it)
				{
					if(it->handle() == waypoint.handle())
					{
						*it = waypoint;
						return;
					}
				}
				_classdata->waypoints.push_back(waypoint);
			}
			else
			{
				static ChatHandle handle = 0;

				if(++handle > 1000)
					handle = 1;

				if(type == WPXT_REQUEST && IsMissionLeader())
				{
					// remove any local contact reports sent by this player
					// they can only have one outstanding report at a time
					std::vector<Waypoint>::iterator it = _classdata->waypoints.begin();
					for(; it != _classdata->waypoints.end(); ++it)
					{
						Waypoint& wp = *it;

						if(wp.local() && wp.type() == WPT_ENEMY && wp.playerHandle() == waypoint.playerHandle())
						{
							// should only be one like this
							_classdata->waypoints.erase(it);
							break;
						}
					}
				}

				_classdata->waypoints.push_back(Waypoint(true,handle,waypoint.playerHandle(),waypoint.type(),waypoint.subType(),waypoint.x(),waypoint.y(),waypoint.posted(),waypoint.notes()));
			}
		}
		break;
	}
}

/************************************************************************************************
	ProcessWaypointControl
*************************************************************************************************/

void Client::ProcessWaypointControl(WaypointControlMsg const&, int type, ChatHandle handle)
{
	switch(type)
	{
		case WPCTRL_APPROVE:
		break;
		case WPCTRL_DENIED:
		case WPCTRL_INVALID:
		case WPCTRL_NO_MISSION:
		case WPCTRL_REMOVE:
		{
			if(handle == _data->activewaypoint)
				_data->activewaypoint = 0;

			std::vector<Waypoint>::iterator it = _classdata->waypoints.begin();
			for(; it != _classdata->waypoints.end(); ++it)
			{
				if(it->handle() == handle)
				{
					_classdata->waypoints.erase(it);
					break;
				}
			}
		}
		break;
	}
}

/************************************************************************************************
	ProcessMissionMemberUpdate
*************************************************************************************************/

void Client::ProcessMissionMemberUpdate(MissionMember const& missionMember)
{
	if(missionMember.m_rank == 0) // remove
	{
		std::vector<MissionMember>::iterator it = _classdata->missionmembers.begin();
		for(; it != _classdata->missionmembers.end(); ++it)
		{
			if(it->m_handle == missionMember.m_handle)
			{
				_classdata->missionmembers.erase(it);
				return;
			}
		}

		return;
	}
	else
	{
		std::vector<MissionMember>::iterator it = _classdata->missionmembers.begin();
		for(; it != _classdata->missionmembers.end(); ++it)
		{
			if(it->m_handle == missionMember.m_handle)
			{
				*it = missionMember;
				return;
			}
		}
	}

	_classdata->missionmembers.push_back(missionMember);
}

/************************************************************************************************
	ProcessMissionMemberUpdate
*************************************************************************************************/

void Client::ProcessBrigadeControl(BrigadeControlList const& entry)
{
	_classdata->oics[entry.m_brigade] = entry;
}

void Client::ProcessBrigadeControlNotice(const BrigadeControlNotice* const notice)
{
	if(notice->m_notice == BrigadeControl::_APPOINTED)
	{
		BrigadeControlList* oic = LookupBrigadeOIC(notice->m_brigadeID);

		if(oic)
		{
			oic->m_officer = notice->m_officer;
			//oic->m_officerRank = notice->m_officerRank;
		}
	}
	else if(notice->m_notice == BrigadeControl::_REMOVED)
	{
		std::map<int, BrigadeControlList>::iterator it = _classdata->oics.find(notice->m_brigadeID);

		if(it != _classdata->oics.end())
		{
			it->second.m_officer = 0;
			it->second.m_officerRank = 0;
		}
	}

	// save the notice for display in the UI
	if(notice->m_notice != BrigadeControlNotice::_ERROR)
		_classdata->brigadenotices.push_back(*notice);
}

void Client::ProcessMissionInvite(const MissionInvite& invite)
{
	_classdata->missioninvite = invite;
	_data->waitmsg[MISSION_INVITE_WAIT] = 0;
}

void Client::ProcessInstanceResult(const Instance::Result& result)
{
	_classdata->instance = result;
	_data->waitmsg[INSTANCE_WAIT] = 0;
}

void Client::ProcessInstanceList(const Instance::ClientListing* list)
{
	_classdata->instances = list;
	_data->waitmsg[INSTANCE_WAIT] = 0;
}

void Client::ProcessTutorialFlags(ChatHandle group, UINT64 flags)
{
	TUT_MGR->SetGroupFlags(chatGetCallsign(group), flags);
}

void Client::ProcessChatPermissions(const ChatPermissions& permissions)
{
	_classdata->permissions = permissions;
}

void Client::ProcessCaptureState(const CaptureState& state)
{
	_classdata->capturestate = state;
	_classdata->capturestate.m_valid = true;
}

void Client::ProcessMissionStatus(const Chat::MissionStatus* ms)
{
	_classdata->missionstatus = ms;
}

void Client::ProcessLoginAttempt(bool success, SINT32 type, SINT32 result)
{
	ww2::HostState* state = GetHostStateByType(type);
	if(!state) return;

	switch((Teul::Connection::HostType)type)
	{
		case Teul::Connection::Map:
		{

		}
		break;
		case Teul::Connection::Chat:
		{

		}
		break;
		case Teul::Connection::Cell:
		{

		}
		break;

        default:
            // don't care about other host types.
            break;
	}
}

std::vector<MissionMember> const& Client::MissionMembers() const
{
	return _classdata->missionmembers;
}

std::vector<BrigadeControlNotice> const& Client::BrigadeNotices() const
{
	return _classdata->brigadenotices;
}

ClientSquad const& Client::PlayerSquad() const
{
	return _classdata->squad;
}

SquadRecruitment& Client::PlayerSquadRecruitment() const
{
	return _classdata->recruitment;
}

SquadInvite const& Client::PlayerSquadInvite() const
{
	return _classdata->invitation;
}

CaptureState& Client::GetCaptureState()
{
	return _classdata->capturestate;
}

void Client::InvalidateSquadInvite()
{
	return _classdata->invitation.Invalidate();
}

MissionInvite const& Client::GetMissionInvite() const
{
	return _classdata->missioninvite;
}

const Instance::Descriptor& Client::GetInstance() const
{
	return _classdata->instance.descriptor();
}

// list of instances from the last instance query
const Instance::ClientListing* Client::GetInstances() const
{
	return _classdata->instances;
}

const FLOAT64VECTOR& Client::GetSpawnOverride() const
{
	return _classdata->spawnoverride;
}

UINT32 Client::UpdatesSent(bool increment)
{
	return (increment) ? _data->updatesSent++: _data->updatesSent;
}

UINT32 Client::HitsPrime() const
{
	return _data->hitsPrime;
}

ChatHandle& Client::MultiCrewCommander() const
{
	return _classdata->commander;
}

// wwiiPermissions.h
bool Client::IsPermitted(UINT64 flag, bool checkSubscription) const
{
	return ((_classdata->permissions.m_permissionMask & flag) && (!checkSubscription || SubscriptionExpires(false) != 0));
}

UINT32 Client::MaxRank(UINT32 country, UINT32 branch) const
{
	if(branch > 0 && branch < MAX_BRANCHES)
		return (UINT32)_classdata->permissions.m_limitRank[branch];

	return persrankMax();
}

UINT32 Client::MaxSquadRank() const
{
	return (UINT32)_classdata->permissions.m_limitSquadRank;
}

// -2 means unmetered
// -1 means don't alert
// 0 means expired
//  > 0 is the number of minutes left
SINT32 Client::SubscriptionExpires(bool asStage) const
{
	if(IsPermitted(UNMETERED_ACCESS, false))
		return -2;

 	/* test stuff
	// calculated amount of minutes left
	static int startTime = 0;
	static int minutes = 35;

	if(false && startTime == 0)
		startTime = frametime + (1000 * 60 * minutes);
	*/

	int expires = 0;

	if(_classdata->accountBalance > teulTime())
		expires = (int)((_classdata->accountBalance - teulTime()) / 60);

	if(asStage)
	{
		static int stage = 0;

		if(expires <= 0)
		{
			if(stage != 0)
			{
				stage = 0;
				return 0;
			}
		}
		else if(expires <= 5)
		{
			if(stage != 1)
			{
				stage = 1;
				return 5;
			}
		}
		else if(expires <= 15)
		{
			if(stage != 2)
			{
				stage = 2;
				return 15;
			}
		}
		else if(expires <= 30)
		{
			if(stage != 3)
			{
				stage = 3;
				return 30;
			}
		}
		else if(expires <= 60)
		{
			if(stage != 4)
			{
				stage = 4;
				return 60;
			}
		}
		else if(expires > 60)
		{
			stage = 5; // waiting for time to come down
		}
	}
	else if(_classdata->accountBalance)
	{
		return (expires <= 0) ? 0: expires;
	}

	return -1;
}

std::wstring const& Client::CurrentOrders() const
{
	return _classdata->currentorders;
}

void Client::SetCurrentOrders(const wchar_t* o)
{
	_classdata->currentorders = o;
}

int Client::SideBalance() const
{
	return _data->sidebalance;
}

int Client::IsSideDelayed() const
{
	return _data->issidedelayed;
}

int Client::IsSpawnDelayed() const
{
	return (int)_data->isspawndelayed;
}

/************************************************************************************************
	ClearJoinRequests
*************************************************************************************************/

void Client::ClearJoinRequests()
{
	if(_classdata->joinrequests.size() > 0)
	{
		for(unsigned int i = 0; i < _classdata->joinrequests.size(); ++i)
		{
			ChatHandle joinHandle = _classdata->joinrequests[i].chatHandle;
			for(unsigned int j = 0; j < _classdata->missionroom.size(); j++)
			{
				if(_classdata->missionroom[j].m_handle == joinHandle)
					_classdata->missionroom[j].m_flags &= ~RMLIST_CREW;
			}
		}
	}

	_classdata->joinrequests.clear();
}

/************************************************************************************************
	ProcessPostAccept
*************************************************************************************************/
// clientProcessPostAccept
void Client::ProcessPostAccept(FE_POST_ACCEPT const& postAccept)
{
	_data->weapSerial = postAccept.weaponSerial;
	_data->waitmsg[WEAP_POST_WAIT] = 0;
	ClearJoinRequests();
}

/************************************************************************************************
	ProcessPostDeny
*************************************************************************************************/
// clientProcessPostDeny
void Client::ProcessPostDeny(FE_POST_DENY const& postDeny)
{
	_data->waitmsg[WEAP_POST_WAIT] = -1;
	ClearJoinRequests();
}

/************************************************************************************************
	ProcessPersonaSummaries
*************************************************************************************************/

void Client::ProcessPersonaSummaries(PersonaSummaries const& summaries)
{
	_data->waitmsg[PERSONAS_REQUEST_WAIT] = 0;
	_classdata->personas = summaries;
	_classdata->permissions = summaries.m_chatPermissions;
	_data->msgtime[PERSONA_SUMMARIES_TIME] = teulTime();
	_classdata->accountBalance = summaries.m_accountGoodUntil;
}

/************************************************************************************************
	ProcessSpawnNotify
*************************************************************************************************/

void Client::ProcessSpawnNotify(FESpawnNotify const& spawnnotify)
{
	_data->waitmsg[SPAWN_NOTIFY_WAIT] = 0;
	_classdata->spawnnotify = spawnnotify;
	_data->weapSerial = spawnnotify.weaponSerial;
	ClearJoinRequests();
}

/************************************************************************************************
	SpawnNotify
*************************************************************************************************/

FESpawnNotify const& Client::SpawnNotify() const
{
	return _classdata->spawnnotify;
}

/************************************************************************************************
	DespawnTimer
*************************************************************************************************/

int Client::DespawnTimer() const
{
	return (_classdata->commander) ? 0 : _data->despawntimer;
}

/************************************************************************************************
	ProcessMissionDetail
*************************************************************************************************/

void Client::ProcessMissionDetail(MissionDetail const& reply)
{
	_data->waitmsg[MISSION_DETAIL_WAIT] = (reply.m_result >= MC_ERROR) ? -1 : 0;

	// for querying arbitrary missions
	// missions not nessecarily in your brigade
	if(reply.m_missionID == _classdata->missionlookupid)
	{
		_classdata->missionlookup	= reply;
		_classdata->missionlookupid	= 0;
		return;
	}

	// set selected mission
	if(reply.m_result == MC_JOIN)
		_classdata->selectedmission = reply;

	std::vector<MissionDetail>::iterator it = _classdata->missions.begin();
	for(; it != _classdata->missions.end(); ++it)
	{
		if(it->m_missionID == reply.m_missionID)
		{
			*it = reply;
			break;
		}
	}
}

/************************************************************************************************
	ProcessCrewJoinPerm
*************************************************************************************************/

void Client::ProcessCrewJoinPerm(FE_JOIN_PERM_REQUEST const& joinperm)
{
	ChatHandle joinHandle = joinperm.chatHandle;
	for(unsigned int j = 0; j < _classdata->missionroom.size(); j++)
	{
		if(_classdata->missionroom[j].m_handle == joinHandle)
			_classdata->missionroom[j].m_flags |= RMLIST_CREW;
	}
	_classdata->joinrequests.push_back(joinperm);
}

/************************************************************************************************
	ProcessCrewJoinAccept
*************************************************************************************************/

void Client::ProcessCrewJoinAccept(FE_JOIN_ACCEPT const& joinaccept)
{
	_data->weapSerial = joinaccept.postedWeapon.weaponSerial;
	_data->commander = joinaccept.postedWeapon;
	ClearJoinRequests();
}

/************************************************************************************************
	ProcessCrewJoinDeny
*************************************************************************************************/

void Client::ProcessCrewJoinDeny(FE_JOIN_DENY const& joindeny)
{
	ClearJoinRequests();
	_classdata->commander = 0;
}

/************************************************************************************************
	ProcessCrewPostExpired
*************************************************************************************************/

void Client::ProcessCrewPostExpired(FE_POST_EXPIRED const& postexpired)
{
	ClearJoinRequests();
	_classdata->commander = 0;
}

/************************************************************************************************
	ProcessWorldUpdate
*************************************************************************************************/

void Client::ProcessWorldUpdate(WORLD_UPDATE const&	wu)
{
	_data->despawntimer = wu.header.timerInfo;
}

/************************************************************************************************
	ProcessMissionLaunchUpdate
*************************************************************************************************/

void Client::ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const& launchupdate)
{
	_data->launchupdate = launchupdate;
}

/************************************************************************************************
	ProcessMissionLaunchUpdate
*************************************************************************************************/

MISS_LAUNCH_UPDATE const& Client::LaunchUpdate() const
{
	return _data->launchupdate;
}

/************************************************************************************************
	ProcessMissionResults
*************************************************************************************************/

void Client::ProcessMissionResults(MISSION_RESULTS const& missionresults)
{
	ProcessChatPermissions(missionresults.chatPermissions);
	_classdata->accountBalance = missionresults.accountGoodUntil;
	_data->missionresults = missionresults;
	_data->msgtime[MISSION_RESULTS_TIME] = teulTime();

	if(missionresults.rtbReservedWeapon.serial > 0)
		_data->weapSerial = missionresults.rtbReservedWeapon;
	else
		memset(&_data->weapSerial, 0x0, sizeof(_data->weapSerial));

	if(missionresults.promoted)
	{
		_classdata->personaselected.m_rank = missionresults.rank;

		for(unsigned int i = 0; i < _classdata->personas.m_count; ++i)
		{
			PersonaSummary& summary = _classdata->personas.m_summaries[i];

			if(_classdata->personaselected.m_countryID == summary.m_countryID &&
			   _classdata->personaselected.m_branchID == summary.m_branchID)
			{
				summary.m_rankID = missionresults.rank;
				break;
			}
		}

		PlayerClient()->jsclient->UpdatePersonas();
	}
}

/************************************************************************************************
	OrderReply
*************************************************************************************************/

MOTD_REPLY const& Client::MotdReply() const
{
	return _classdata->motdreply;
}


/************************************************************************************************
	ProcessChatOrderReply
*************************************************************************************************/

void Client::ProcessChatMotdReply(MOTD_REPLY const& reply)
{
	_data->waitmsg[MOTD_WAIT] = 0;
	_classdata->motdreply = reply;
}

/************************************************************************************************
	ProcessMissionListUpdate
*************************************************************************************************/

void Client::ProcessMissionListUpdate(unsigned int brigadeID)
{
	_data->missionListUpdate = brigadeID;
}

/************************************************************************************************
	MissionListUpdate
*************************************************************************************************/

unsigned int Client::MissionListUpdate() const
{
	return _data->missionListUpdate;
}

/************************************************************************************************
    ActivateHost
*************************************************************************************************/
void Client::SetActiveHost(unsigned int hostIP, unsigned int port)
{
	_data->host = hostIP;
	_data->port = port;
}

/************************************************************************************************
	ActivateCell
*************************************************************************************************/
// clientActivateCell
void Client::ActivateCell()
{
	//_data->cellClientActive = true;
	_data->enableDespawnTimer = false;
	_data->waitmsg[CELL_CLIENT_WAIT] = 0;
}

/************************************************************************************************
	ActivateMap
*************************************************************************************************/
// clientActivateMap
void Client::ActivateMap()
{
	//_data->cellClientActive = false;
	_data->enableDespawnTimer = false;
	_data->waitmsg[MAP_CLIENT_WAIT] = 0;
}

/************************************************************************************************
	ProcessPersonaSelection
*************************************************************************************************/

void Client::ProcessPersonaSelection(PersonaSelection const& selection)
{
	// See mapPersonaSummary.h::SELECTION_STATUS for possible m_status codes
	switch (selection.m_status)
	{
		case PSS_NEW:			// Persona creation was requested and successful
			///PASSTHRU
		case PSS_SELECT:		// Persona selection was requested and successful
		{
			_classdata->personaselected = selection;
			_classdata->permissions = selection.m_chatPermissions;

			_data->waitmsg[PERSONA_SELECT_WAIT] = 0;
			// save the latest data from the selection
			for(unsigned int i = 0; i < _classdata->personas.m_count; ++i)
			{
				if(selection.m_countryID == _classdata->personas.m_summaries[i].m_countryID &&
				   selection.m_branchID == _classdata->personas.m_summaries[i].m_branchID)
				{
					_classdata->personas.m_summaries[i].m_rankID = selection.m_rank;
					_classdata->personas.m_summaries[i].m_rankPctg = selection.m_rankPctg;

					if(_classdata->personas.m_summaries[i].m_personaID == 0)
						_classdata->personas.m_summaries[i].m_personaID = selection.m_personaID;

					break;
				}
			}

			_classdata->accountBalance = selection.m_accountGoodUntil;

			return;
		}
		break;
		case PSS_DELETE:		// Persona deletion/reset was requested and successful
			_data->waitmsg[PERSONA_SELECT_WAIT] = 0;
		break;
		case PSS_ERROR:			// Undefined host error occured
		case PSS_DUPLICATE:		// Requested creation country/branch combination already exists for this player
		case PSS_MISSING:		// Requested creation or deletion of a personaID that doesn't exist
		case PSS_PERSONA_LOCKED: // Requested selection of a persona that is not accessible (side locked)
		default:				// Garbage data
			_data->waitmsg[PERSONA_SELECT_WAIT] = -1;
		break;
	}
}

/************************************************************************************************
	ProcessPersonaSelection
*************************************************************************************************/

void Client::ProcessBrigadeSelection(BrigadeSelection const& selection)
{
	switch (selection.m_status)
	{
		case BRIG_DETAIL:
		{
			_data->waitmsg[BRIGADE_SELECT_WAIT] = 0;

			_classdata->brigadedetail = selection;

//			HCUnit* unit = HCUnit::Lookup(selection.m_detail.brigadeID());
//			if(unit)
//				*unit = *(HCUnit*)(&selection.m_detail);

		}
		break;
		case BRIG_SELECT:
		{
			_data->waitmsg[BRIGADE_SELECT_WAIT] = 0;

			_classdata->brigadeselected = selection;

//			HCUnit* unit = HCUnit::Lookup(selection.m_detail.brigadeID());
//			if(unit)
//				*unit = *(HCUnit*)(&selection.m_detail);

			// save the latest data from the selection
			for(unsigned int i = 0; i < _classdata->personas.m_count; ++i)
			{
				if(selection.m_detail.m_countryID == _classdata->personas.m_summaries[i].m_countryID &&
				   selection.m_detail.m_branchID == _classdata->personas.m_summaries[i].m_branchID)
				{
					_classdata->personas.m_summaries[i].m_brigadeID = selection.m_detail.brigadeID();
					break;
				}
			}
		}
		break;
		case BRIG_ERROR:
		case BRIG_MISSING:
		case BRIG_INACTIVE:
		case BRIG_RESTRICTED:
			_data->waitmsg[BRIGADE_SELECT_WAIT] = -1;
		break;

        default:
            // don't care about other BRIG status
            break;
	}
}

/************************************************************************************************
	BrigadeDetails
*************************************************************************************************/

BrigadeSelection const& Client::BrigadeDetails() const
{
	return _classdata->brigadedetail;
}

/************************************************************************************************
	GetActiveMission
*************************************************************************************************/

ActiveMission const& Client::GetActiveMission() const
{
    return _classdata->personas.m_activeMission;
}

/************************************************************************************************
	ProcessMissionList
*************************************************************************************************/

void Client::ProcessMissionList(MissionList const& missionList)
{
	if(missionList.m_status != MLR_OK)
	{
		return;
	}

	if(missionList.m_listStart)
		_classdata->missions.clear();

	for(int i = 0; i < missionList.m_count; i++)
	{
		MissionDetail detail;
		*(MissionSynopsis*)&detail = missionList.m_missions[i];
		_classdata->missions.push_back(detail);
	}

	if(missionList.m_listEnd)
	{
	}
	_data->missionListUpdate = 0;
	_data->canpost = missionList.m_canPost;
}

/************************************************************************************************
	CanPostMission
*************************************************************************************************/

bool Client::CanPostMission() const
{
	return _data->canpost;
}

/************************************************************************************************
	ProcessBrigadeSummaries
*************************************************************************************************/

void Client::ProcessBrigadeSummaries(BrigadeSummaries const& summaries)
{
	_classdata->brigadeList = summaries;
	_data->waitmsg[BRIGADE_LIST_WAIT] = 0;

	for(UINT32 i = 0; i < summaries.m_count; i++)
	{
		HCUnit* unit = HCUnit::Lookup(summaries.m_summaries[i].brigadeID());
		if(unit)
			*(HCUnit*)(&_classdata->brigadeList.m_summaries[i]) = *unit;
	}
}

/************************************************************************************************
	ProcessChatServerInfo
	Receives a chat server info packet from the mmhost and stowes the information
	for use when we want to open the chat connection.
*************************************************************************************************/

void Client::ProcessChatServerInfo(ChatServerInfo const& chatInfo)
{
	if(!_data->wantChat) return;

	_data->chatServerInfo = chatInfo;
	_data->haveChatServerInfo = true;

	// update our otp
	_classdata->playerIdent.m_otp = chatInfo.m_otp;
}

bool Client::WantsChat() const
{
	if(!clientinited || !_data->wantChat)
		return false;

	// Have to have received a ChatServerInfo packet.
	if(!_data->haveChatServerInfo)
		return false;

	return true;
}

/************************************************************************************************
	ConnectToChatHost
	Tries to use the received ChatServerInfo to establish a connection
	to the game's chat server.
	Specify blocking=true if you want to wait for the connection to complete.
	Returns: false if the attempt fails.
*************************************************************************************************/

bool Client::ConnectToChatHost(bool blocking)
{
	if(!clientinited || !_data->wantChat) return false;

	// Have to have received a ChatServerInfo packet.
	if(!_data->haveChatServerInfo) return false;

	Teul::ServerAddr server = { _data->chatServerInfo.m_ipAddress, _data->chatServerInfo.m_port };
	Teul::PlayerIdent& ident = _classdata->playerIdent;

	ClientHostConnection* const conn = new ClientHostConnection(Teul::Connection::Chat, server, ident, 30*1000);
	if(!conn->start(this))
		return false;

	// In non-blocking mode, this counts as a success.
	if(blocking == false)
		return true;

	TeulTimeMS cutoff = teulClock64 + (30 * 1000);
	while(conn->complete() == false)
	{
		if(teulClock64 >= cutoff)
		{
			clientErrorLog("failed connecting to chat");
			return false;
		}
		_yield();
	}

	return conn->isInService();
}

/************************************************************************************************
	ProcessMissionPostReply
*************************************************************************************************/

void Client::ProcessMissionPostReply(MISSION_POST_REPLY const& reply)
{
	// check error codes
	_data->missionpostreply = reply;
	_classdata->selectedmission.m_missionID = reply.missionID;
	_data->waitmsg[MISSION_POST_WAIT] = 0;
}

/************************************************************************************************
	ProcessChatWho
*************************************************************************************************/

void Client::ProcessChatWho(int type, const Chat::Who& who)
{
	if (!_classdata->squad.squadID()) return;
	if(who.inSameSquad() && who.squadRank() > Squad::NOT_IN_SQUAD)
	{
		_classdata->squad.addMember(who);
		return;
	}
	if (type == CWT_SQUAD)	// Anyone appearing in a squad list but not "in same squad" is a removal
		_classdata->squad.removeMember(who);
}

/************************************************************************************************
	ProcessSquad
*************************************************************************************************/

void Client::ProcessSquad(const SquadMsg& msg)
{
	if(msg.status() == SquadMsg::INFO_ONLY || msg.status() == SquadMsg::BOTH)
	{
		// must retain members list
		ClientSquad oldsquad = _classdata->squad;

		_classdata->squad = ClientSquad(msg.squad());

		if(oldsquad.squadID() && oldsquad.size())
			_classdata->squad.addMembers(oldsquad.members());
	}
	else if(msg.status() == SquadMsg::NO_SQUAD)
	{
		_classdata->squad = ClientSquad();
	}
}

/************************************************************************************************
	ProcessRecruiter
*************************************************************************************************/

void Client::ProcessRecruiter(const Recruiter& rec, bool reset)
{
	if(reset)
	{
		_classdata->recruitment.modified = true;
		_classdata->recruitment.recruiters.clear();

	}

	_classdata->recruitment.recruiters.push_back(rec);
	_data->waitmsg[REQUEST_RECRUITERS_WAIT] = 0;
}

void Client::ProcessRecruiter(bool reset)
{
	if(reset)
	{
		_classdata->recruitment.modified = true;
		_classdata->recruitment.recruiters.clear();
	}

	_data->waitmsg[REQUEST_RECRUITERS_WAIT] = 0;
}

/************************************************************************************************
	ProcessSquadInvite
*************************************************************************************************/

void Client::ProcessSquadInvite(const SquadMsg& msg)
{
	_classdata->invitation = SquadInvite(msg);
}

/************************************************************************************************
	ProcessMissionRoomListReply
*************************************************************************************************/

void Client::ProcessMissionRoomListReply(RoomList const& missroomlist)
{
	for(int i = 0; i < (int)missroomlist.m_numEntries; i++)
	{
		RoomListEntry const* entry = &missroomlist.m_entry[i];
		switch(entry->m_action)
		{
			case RMLIST_INIT:
				_classdata->missionroom.clear();
			break;

			case RMLIST_DELETE:
			{
				std::vector<RoomListEntry>::iterator it = _classdata->missionroom.begin();
				for(; it != _classdata->missionroom.end(); ++it)
				{
					if(it->m_handle == entry->m_handle)
					{
						_classdata->missionroom.erase(it);
						break;
					}
				}
			}
			break;

			case RMLIST_ADD:
			case RMLIST_UPDATE:
			{
				bool found = false;

				std::vector<RoomListEntry>::iterator it = _classdata->missionroom.begin();
				for(; it != _classdata->missionroom.end(); ++it)
				{
					if(it->m_handle == entry->m_handle)
					{
						*it = *entry;
						found = true;
						break;
					}
				}

				if(!found && entry->m_action == RMLIST_ADD)
					_classdata->missionroom.push_back(*entry);
			}
			break;

            default:
                // don't care about other RMLIST commands
                break;
		}
	}
}

/************************************************************************************************
	ProcessPostList
*************************************************************************************************/

void Client::ProcessPostList(FE_POST_LIST_RESPONSE  const& response)
{
	// this is temporary just to set flags on mission room list
	for(int i = 0; i < response.numWeapons; i++)
	{
		if(response.commandedWeapon[i].action == COMMANDED_WEAPON_ADD)
		{
			// try to remove the commanded weapon first
			std::vector<COMMANDED_WEAPON>::iterator it = _classdata->commandedweapons.begin();
			for(; it != _classdata->commandedweapons.end(); ++it)
			{
				if(it->handle == response.commandedWeapon[i].handle)
				{
					_classdata->commandedweapons.erase(it);
					break;
				}
			}

			_classdata->commandedweapons.push_back(response.commandedWeapon[i]);
		}
		/* this never gets called because action is apparently always COMMANDED_WEAPON_ADD
		else
		{
			std::vector<COMMANDED_WEAPON>::iterator it = _classdata->commandedweapons.begin();
			for(; it != _classdata->commandedweapons.end(); ++it)
			{
				if(it->handle == response.commandedWeapon[i].handle)
				{
					_classdata->commandedweapons.erase(it);
					break;
				}
			}
		}
		*/

		for(unsigned int j = 0; j < _classdata->missionroom.size(); j++)
		{
			if(response.commandedWeapon[i].handle == _classdata->missionroom[j].m_handle)
			{
				if(response.commandedWeapon[i].action == COMMANDED_WEAPON_ADD)
					_classdata->missionroom[j].m_flags |= RMLIST_WANTS_CREW;
				else
					_classdata->missionroom[j].m_flags &= ~RMLIST_WANTS_CREW;
				break;
			}
		}
	}
}

/************************************************************************************************
	ProcessSwingMeter
*************************************************************************************************/

void Client::ProcessSwingMeter(SwingMeter const& meter)
{
	_data->sidebalance = meter.meter();
	_data->issidedelayed = meter.inEffect();
	_data->isspawndelayed = meter.delays();
}

/************************************************************************************************
	ProcessOpinionPoll
*************************************************************************************************/

void Client::ProcessOpinionPoll(OpinionPoll const& p)
{
	_classdata->opinionpoll = p;
}

/************************************************************************************************
	weapList
*************************************************************************************************/
#if(0)
static void weapListCopy(FacilityWeaponList* dst , const FacilityWeaponList* src)
{
	memcpy(dst, src, sizeof(FacilityWeaponList));
	assert( src->m_numWeapons < FacilityWeaponList::_MAX_WEAPONS );
}
#endif

static void weapListClear(FacilityWeaponList* weapList)
{
	memset(weapList, 0x0, sizeof(FacilityWeaponList));
}

bool Client::CellClientActive() const
{
	//return false;		//CellState->Connected();
	//Check to see if we are connected to a cell
	return _classdata->connections[Teul::Connection::Cell]->Connected();
}

bool Client::MapClientActive() const
{
	//return _data->mapClientActive;
	return true;
}

void Client::OnDespawn()
{
	for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
	{
		HostState* state = _classdata->connections[i];
		if(state) state->OnDespawn();
	}
}

void Client::OnExit()
{
	for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
	{
		HostState* state = _classdata->connections[i];
		if(state) state->OnExit();
	}
}

void Client::Update()
{
	// update our host states
	for(int i = 0; i < Teul::Connection::NumHostTypes; i++)
		_classdata->connections[i]->Update();
}

/************************************************************************************************
	SendUpdate
*************************************************************************************************/

// never called
void Client::SendUpdate()
{
	if(!CellClientActive()) return;

	VEHICLE_UPDATE_HEADER vupHeader;
	SHARED_VEHICLE_INFO& vehInfo = vupHeader.vehInfo;
	int i;

	memset(&vehInfo, 0x0, sizeof(vehInfo));
	memset(&vupHeader, 0x0, sizeof(vupHeader));

	vupHeader.present = (PRESENT_FLAGS)(vupHeader.present | PR_STATE0 | PR_STATE1 | PR_VEH_VIS | PR_FIRE_FLAGS | PR_POSITION);

	if(_data->vehicleKilled) vupHeader.present |= PR_KILLED;
	if(_data->crewDead) vupHeader.present |= PR_DEAD;

	vehInfo.index = _data->weapSerial.serial;
	vehInfo.timeStamp = vehInfo.posTimeStamp = teulClock();

	// position
	vupHeader.position.m_x = (int)(_data->x * 100.0);
	vupHeader.position.m_y = (int)(_data->y * 100.0);
	vupHeader.position.m_z = (int)((_data->z + 2000.0) * 100.0);

	vehInfo.attitude.attX = (SINT16)(_data->pitch * 10000.0 + 0.5);
	vehInfo.attitude.attY = (SINT16)(_data->roll * 10000.0 + 0.5);
	vehInfo.attitude.attZ = (SINT16)(_data->yaw * 10000.0 + 0.5);

	vehInfo.velocity.velX = (SINT16)(_data->vx * 50.0 + 0.5);
	vehInfo.velocity.velY = (SINT16)(_data->vy * 50.0 + 0.5);
	vehInfo.velocity.velZ = (SINT16)(_data->vz * 50.0 + 0.5);

	vehInfo.acceleration.accelX = (SINT16)(_data->ax * 100.0 + 0.5);
	vehInfo.acceleration.accelY = (SINT16)(_data->ay * 100.0 + 0.5);
	vehInfo.acceleration.accelZ = (SINT16)(_data->az * 100.0 + 0.5);

	vehInfo.rotational.velX = (SINT16)(_data->wx * 1000.0 + 0.5);
	vehInfo.rotational.velY = (SINT16)(_data->wy * 1000.0 + 0.5);
	vehInfo.rotational.velZ = (SINT16)(_data->wz * 1000.0 + 0.5);

	vehInfo.stateFlags1 = (_data->state & 0xffff0000) >> 16;
	vehInfo.stateFlags0 = _data->state & 0xffff;

	vehInfo.fire.state = _data->fireState;
	vupHeader.fireMask = _data->fireState;

	for(i = 0; i < 32; i++)
	{
		if(_data->fireOnetime[i])
			_data->fireState &= ~(1 << i);
	}

	for(i = 0; i < MAX_TURRETS; i++)
	{
		if(_data->turretMask & (1 << i))
		{
			vehInfo.turret.pos[i].x = _data->turretTraverse[i];
			vehInfo.turret.pos[i].y = _data->turretElev[i];
		}
	}

	//vupHeader.vt.numTurrets = NumTurrets;
	vupHeader.vt.turretMask = _data->turretMask;

	if (_data->turretMask != 0)
		 vupHeader.present = (PRESENT_FLAGS)(vupHeader.present | PR_TURRET);

	vehInfo.visBits = _data->visbits;

	if(_data->RideVehicleHostIndex > 0)
	{
		vupHeader.present = (PRESENT_FLAGS) (vupHeader.present | PR_RIDING);
		vehInfo.rideIndex = _data->RideVehicleHostIndex;
		vehInfo.seatNumber = _data->RideSeatNum;

/*		if(Vehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
		{
			// send position relative to ship (in cm)
			VehicleHdr.present |= PR_RELPOS;
			VehicleInfo.relPos.relX = (SINT16)(Vehicle->RelRidePosition.x * 100.0);
			VehicleInfo.relPos.relY = (SINT16)(Vehicle->RelRidePosition.y * 100.0);
			VehicleInfo.relPos.relZ = (SINT16)(Vehicle->RelRidePosition.z * 100.0);
			VehicleInfo.relPos.yaw = (SINT8)(RAD_TO_DEG(Vehicle->RelRideYaw) * 255.0 / 360.0);
		}*/
	}

	if(_data->enableDespawnTimer)
	{
		vupHeader.present |= PRX_DESPAWN_TIMER;
		vehInfo.timerInfo = 1;
	}
	else
	{
		vupHeader.present &= ~PRX_DESPAWN_TIMER;
	}

	vupHeader.updateNo = UpdatesSent(true);
	vupHeader.updatePrime = wwiiHitsPrime(); //_data->hitsPrime;

	COMM_UPDATE_BUFFER cub;
	cub.size = updtPackSHARED_VEHICLE_INFO(TEUL_PACK, cub.data, 0, &vupHeader, sizeof(cub.data));
	if ( cub.size >= 0 )
	{
		teulSend(CellConnection(),commUPDATE,&cub);
	}
	_data->turretMask = 0;
}


/************************************************************************************************
	Accessors
*************************************************************************************************/

Client* Client::FromTeulIndex(int teulIndex)
{
	return (Client*)teulGetPersonaData(teulIndex);
}

FacilityWeaponList const& Client::WeaponList() const
{
	return _data->weapList;
}

PersonaSummaries& Client::Personas() const
{
	return _classdata->personas;
}

WEAPON_SERIAL const& Client::WeaponSerial( ) const
{
	return _data->weapSerial;
}

PersonaSelection const& Client::PersonaSelected() const
{
	return _classdata->personaselected;
}

bool Client::SelectInstance(UINT32 category, UINT32 type, bool save, bool force)
{
	static UINT32 lastCategory = Instance::_CAMPAIGN;
	static UINT32 lastType = 0;

	// move us to the new instance
	if(force || lastCategory != category || lastType != type)
	{
		// reset our descriptor
		_classdata->instance.m_status = Instance::_BAD_DATA;

		Instance::RequestSelect(category, type, 0, 0, MapConnection());
		WaitForMessage(ww2::Client::INSTANCE_WAIT, 10);

		if(_classdata->instance.status() == Instance::_SUCCESS)
		{
			lastCategory = category;
			lastType = type;

#ifdef BETA_RELEASE
			uiChatPrintLine("Instance set to %u.%u", _classdata->instance.descriptor().m_category, _classdata->instance.descriptor().m_type);
#endif

			ResetMissionData();

			if(save)
				SavePersona(SAVE_INSTANCE);

			return true;
		}

#ifdef BETA_RELEASE
		uiChatPrintLine("Unable to set instance to %u.%u: %d", category, type, _classdata->instance.status());
#endif

		// leave it at what it was
		//lastCategory = Instance::_INVALID;
		//lastType = Instance::_INVALID;

		return false;
	}

	return true;
}

bool Client::SelectPersona(const PersonaSummary& persona, bool save, bool force)
{
	const PersonaSummary* current = GetPersona();

	if(force || !current || *current != persona)
	{
		current = nullptr;

		RequestPersonaSelection(persona);

		if(WaitForMessage(ww2::Client::PERSONA_SELECT_WAIT, 10) == 0)
			current = GetPersona();

		ResetMissionData();
	}

	if(current && *current == persona)
	{
		if(save)
			SavePersona(SAVE_PERSONA);

		return SelectBrigade(current->m_brigadeID, save, force);
	}

	return false;
}

bool Client::SelectBrigade(UINT32 brigadeID, bool save, bool force)
{
	const PersonaSummary* current = GetPersona();

	if(current)
	{
		if(brigadeID > 0 && (force || current->m_brigadeID != brigadeID))
		{
			RequestBrigadeSelection(brigadeID);

			if(WaitForMessage(ww2::Client::BRIGADE_SELECT_WAIT, 10) != 0)
				brigadeID = 0;
		}

		if(current->m_brigadeID == brigadeID)
		{
			if(save)
				SavePersona(SAVE_BRIGADE);

			if(current->m_brigadeID > 0)
			{
				BrigadeControlRequest req;
				req.requestList(PlayerClient()->ChatConnection(), current->m_countryID, current->m_branchID); // lets get a new list of OIC's
			}

			return true;
		}
	}

	return false;
}

// save our current persona in case we need to reload it later
void Client::SavePersona(int type)
{
	if(type == -1 || type & SAVE_INSTANCE)
		_classdata->saved.instance = _classdata->instance.descriptor().category();

	if(type == -1 || type & SAVE_COUNTRY)
		_classdata->saved.country = _classdata->personaselected.m_countryID;

	if(type == -1 || type & SAVE_BRANCH)
		_classdata->saved.branch = _classdata->personaselected.m_branchID;

	if(type == -1 || type & SAVE_BRIGADE)
		_classdata->saved.brigade = _classdata->brigadeselected.m_detail.brigadeID();

	if(type == -1 || type & SAVE_MISSION)
		_classdata->saved.mission = _classdata->selectedmission.m_missionID;
}

bool Client::LoadSavedPersona(bool force)
{
	if(SelectInstance(1, 0, false, force))
	{
		const PersonaSummary* saved = GetSavedPersona();

		if(saved && !SelectPersona(*saved, false, force))
			return false;

		// don't fail on brigade because they may not have one
		//SelectBrigade(_classdata->saved.brigade);
	}

	return true;
}

// defaults to getting the current persona
const PersonaSummary* Client::GetPersona(int country, int branch) const
{
	if(country == -1) country = _classdata->personaselected.m_countryID;
	if(branch == -1) branch = _classdata->personaselected.m_branchID;

	for(unsigned int i = 0; i < _classdata->personas.m_count; ++i)
	{
		const PersonaSummary& summary = _classdata->personas.m_summaries[i];

		if(country == summary.m_countryID && branch == summary.m_branchID)
			return &summary;
	}

	return nullptr;
}

// defaults to getting the saved persona
const PersonaSummary* Client::GetSavedPersona() const
{
	if(_classdata->saved.Valid())
		return GetPersona(_classdata->saved.country, _classdata->saved.branch);

	return nullptr;
}

BrigadeSelection const& Client::BrigadeSelected() const
{
	return _classdata->brigadeselected;
}

std::vector<MissionDetail> const&  Client::Missions() const
{
	return _classdata->missions;
}

BrigadeSummaries const&  Client::Brigades() const
{
	return _classdata->brigadeList;
}

static bool SortRoomByRank(RoomListEntry const& lhs, RoomListEntry const& rhs)
{
	return lhs.m_rank < rhs.m_rank;
}

std::vector<RoomListEntry> const& Client::MissionRoomList(bool sorted) const
{
	if(sorted)
	{
		// descending order by rank
		std::sort(_classdata->missionroom.rbegin(), _classdata->missionroom.rend(), SortRoomByRank);
	}

	return _classdata->missionroom;
}

std::vector<COMMANDED_WEAPON> const& Client::CommandedWeapons() const
{
	return _classdata->commandedweapons;
}

std::vector<FE_JOIN_PERM_REQUEST> const& Client::JoinRequests() const
{
	return _classdata->joinrequests;
}

MissionDetail& Client::SelectedMission() const
{
	return _classdata->selectedmission;
}

MissionDetail& Client::MissionLookup() const
{
	return _classdata->missionlookup;
}

MISSION_RESULTS const& Client::MissionResults() const
{
	return _data->missionresults;
}

MISSION_POST_REPLY const& Client::MissionPostReply() const
{
	return _data->missionpostreply;
}

const char* Client::MissionDescription() const
{
	return _classdata->missiondesc.c_str();
}

OpinionPoll const& Client::GetOpinionPoll() const
{
	return _classdata->opinionpoll;
}

void Client::SetPosition( double x, double y, double z)
{
	_data->x = x;
	_data->y = y;
	_data->z = z;
}

void Client::SetSpawnOverride(double x, double y, double z)
{
	_classdata->spawnoverride.x = x;
	_classdata->spawnoverride.y = y;
	_classdata->spawnoverride.z = z;
}

void Client::SetVelocity( double vx, double vy, double vz)
{
	_data->vx = vx;
	_data->vy = vy;
	_data->vz = vz;
}

void Client::SetAcceleration( double ax, double ay, double az)
{
	_data->ax = ax;
	_data->ay = ay;
	_data->az = az;
}

void Client::SetAttitude( double pitch, double roll, double yaw)
{
	_data->pitch = pitch;
	_data->roll = roll;
	_data->yaw = yaw;
}

void Client::SetAngularVelocity( double wx, double wy, double wz)
{
	_data->wx = wx;
	_data->wy = wy;
	_data->wz = wz;
}

void Client::SetStateFlagBit( int bitnum, bool on)
{
	if(on)
		_data->state |= 1 << bitnum;
	else
		_data->state &= ~(1 << bitnum);
}

void Client::SetStateFlag( unsigned int flag)
{
	_data->state = flag;
}

void Client::SetFireState( int bitnum, int value, bool onetime)
{
	if(value)
	{
		_data->fireState |= 1 << bitnum;
		_data->fireOnetime[bitnum] = onetime;
	}
	else
	{
		_data->fireState &= ~(1 << bitnum);
	}
}

void Client::SetTurretPosition(int bitnum, unsigned char traverse, unsigned char elevate)
{
	_data->turretMask |= 1 << bitnum;
	_data->turretTraverse[bitnum] = traverse;
	_data->turretElev[bitnum] = elevate;
}


void Client::SetVisBits(unsigned int visbits)
{
	_data->visbits = visbits;
}

void Client::SetRide(unsigned int RideVehicleHostIndex, unsigned short RideSeatNum)
{
	_data->RideVehicleHostIndex = RideVehicleHostIndex;
	_data->RideSeatNum = RideSeatNum;
}

void Client::SetRecruiter(const Recruiter& r)
{
	_classdata->recruitment.recruiter = r;
}

/************************************************************************************************
	SetKilledByAI
*************************************************************************************************/

void ww2::Client::SetKilledByAI(bool status)
{
	_classdata->killedByAI = status;
}
/************************************************************************************************
	Init
*************************************************************************************************/

int ww2::Client::Init(int playerID, int otp, unsigned int appVersion)
{
	char dbugFileName[21];
	if(clientinited) return 0;

#if defined(CONNECTION_TEST_BUILD) || defined(ALPHA_RELEASE)
	static const LogMode DBUGLOG_MODE = LogMode::TO_FILE;
#else
	static const LogMode DBUGLOG_MODE = LogMode::OFF;
#endif

	// update our default player info data
	_defaultPlayerIdent.m_playerID		= playerID;
	_defaultPlayerIdent.m_otp			= otp;
	_defaultPlayerIdent.m_clientVersion	= appVersion;

	// other calls before this call can create the player client, so
	// go ahead and set the player ident info in it just to make sure
	PlayerClient()->SetPlayerIdent(playerID, otp, appVersion);

	strprintf(dbugFileName, "wwiiClient.%u", playerID);
	int err = teulInitTransport(DBUGLOG_MODE, dbugFileName, errorFn, nullptr, clientErrorLog);

	if(err > -1)
	{
		err = 	commInstallClientCallbacks() 	|
				chatInstallClientCallbacks()	|
				strtInstallClientCallbacks() 	|
				mapInstallClientCallbacks()		|
				missionInstallCallbacks()		|
				mmClientInstallCallbacks()		|
				wwiiClientInstallCallbacks()	|
				Instance::InstallClientCallbacks();
	}

	if(err == 0)
		clientinited = true;

	return err;
}

void ww2::Client::Kill()
{
	clientinited = false;
	teulCloseTransport();
}

/************************************************************************************************
	NetUpdate
*************************************************************************************************/

//extern void mapProcessSuperCellVislevels();

void ww2::Client::NetUpdate(int /*wait*/)
{
	if(clientinited)
	{
		teulUpdateTransport();
		Teul::Connection::expire();
		//mapProcessSuperCellVislevels();
	}
}

/************************************************************************************************
	SetAreaChat
*************************************************************************************************/

void ww2::Client::SetAreaChat(bool on)
{
	_data->areachat = on;
}

/************************************************************************************************
	SetHCStatus
*************************************************************************************************/

void ww2::Client::SetHC(bool status)
{
	_data->ishc = status;
}

/************************************************************************************************
	IsHC
*************************************************************************************************/

bool ww2::Client::IsHC() const
{
	return _data->ishc;
}

/************************************************************************************************
	IsOIC
*************************************************************************************************/

bool ww2::Client::IsOIC() const
{
	return (OICOfBrigade() > 0);
}

/************************************************************************************************
	OICOfBrigade
*************************************************************************************************/

unsigned int ww2::Client::OICOfBrigade() const
{
	// check to see if I'm a an OIC
	std::map<int, BrigadeControlList>::iterator it = _classdata->oics.begin();

	while(it != _classdata->oics.end())
	{
		if(it->second.m_officer == _classdata->personas.m_callsign)
			return it->second.m_brigade;

		++it;
	}

	return 0;
}

/************************************************************************************************
	teul callbacks
*************************************************************************************************/

static void errorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	if(!clientinited)
		return;

	Client* client = Client::FromTeulIndex(teulIndex);
	if(!client)
	{
		clientErrorLog("t:%d err:%d transport error/no client", teulIndex, error);
		return;
	}

	if(error < 0)
		error = -error;

	ww2::HostState* state = client->GetHostStateByIndex(teulIndex);
	if(!state)
	{
		clientErrorLog("t:%d err:%d transport error/no state", teulIndex, error);
		return;
	}

	// let the host state know about the error
	clientErrorLog("t:%d err:%d transport error", teulIndex, error);
	state->OnError(state->connection, error);
}

void Client::SendPlayerExit(int exitcode, FULL_OBJ_ID const& rtbfac)
{
	if(CellState()->Connected())
	{
		SINT32 result = commSendPlayerExit(CellConnection(), PDEST_MAP, (PLAYER_EXIT_CODE)exitcode, rtbfac);
		clientErrorLog("t:%d sending pEx with result: %d", CellConnection(), result);
		teulUpdateTransport(50);
		//uiChatPrintLineColor(colorORANGEALPHA, L"Player exit result is %d", result);

		// close our cell connection
		//CellState()->Disconnect();
	}
	else
	{
		clientErrorLog("pEx/no cell");
		//uiChatPrintLineColor(colorORANGEALPHA, L"Player exit result is NOT CONNECTED");
	}

	// reset our crew commander if we had one
	_classdata->commander = 0;
}

int Client::RequestVehicleJoin(WEAPON_SERIAL const& serial, int timestamp)
{
	VEHICLE_REQUEST vRequest;
	memset(&vRequest,0,sizeof(vRequest));

	vRequest.weapon = serial;
	commSendVehicleJoin(CellConnection(), &vRequest);

	return 0;
}

int Client::RequestDirectSpawn(WEAPON_SPEC const& spec, const FLOAT64VECTOR* at, FLOAT64 yaw)
{
	return mmSendDirectSpawn(MapConnection(), (WEAPON_SPEC&)spec, at);
}

int Client::LastMessageReceivedTime(MSG_TIME i)
{
	return _data->msgtime[i];
}

void Client::SpawnComplete()
{
	_data->waitmsg[REDIRECT_WAIT] = 0;
}

int Client::WaitForMessage(MSG_WAIT msg, int timeout)
{
	TeulTimeMS endtime = teulClock64 + (unsigned int)timeout * 1000;
	_data->waitmsg[msg] = 1;

	for(;teulClock64 < endtime;)
	{
		NetUpdate();

		if(_data->waitmsg[msg] != 1)
			return _data->waitmsg[msg];
	}

	return -1;
}

void Client::SetWait(MSG_WAIT msg)
{
	_data->waitmsg[msg] = 1;
}

void Client::ClearWaits()
{
	memset(_data->waitmsg,0,sizeof(_data->waitmsg));
}

void Client::ClearWait(MSG_WAIT msg)
{
	_data->waitmsg[msg] = 0;
}

int Client::GetWait(MSG_WAIT msg)
{
	return _data->waitmsg[msg];
}


/**
 * Check for messages against a bit mask. Set different fields in the bit mask to test for different conditions
*/
int Client::WaitForMessages(int timeout)
{
	//set an end time
	TeulTimeMS endtime = teulClock64 + (unsigned int)timeout * 1000;		// 1000;

	//initialize the bit mask - using ints here
	int waits[MAX_MSG_WAIT];
	memcpy(waits,_data->waitmsg,sizeof(waits));

	//From now until the end
	for(;teulClock64 < endtime;)
	{
		//check for current status
		NetUpdate();

		bool done = true;
		for(int i = 0; i < MAX_MSG_WAIT; i++)
		{
			if((waits[i] && _data->waitmsg[i]) != 0)		//if the waits array and the waitmsg are 1 - follow the if and set done to false
			{
				done = false;
				break;
			}
		}

		if (done)
			return 0;
	}



	//display the offending mask
	//for (int i = 0; i < MAX_MSG_WAIT; i++)
	//{
	//	if ((waits[i] && _data->waitmsg[i]) != 0)		//if the waits array and the waitmsg are 1 - follow the if and set done to false
	//	{
	//		printf("Failed mask is %d\n", i);
	//		break;
	//	}
	//}




	return -1;
}

unsigned int Client::NetTime()
{
	return teulClock();
}

void Client::ProcessVehicleRequest(VEHICLE_REPLY const& reply)
{
	_data->vehReply = reply;
	_data->waitmsg[VEHICLE_REPLY_WAIT] = 0;
}

VEHICLE_REPLY const& Client::VehicleReply() const
{
	return _data->vehReply;
}

void Client::SetAutoReconnect(bool reconnect)
{
	_data->dontReconnect = !reconnect;
}
void Client::EnableDespawnTimer(bool b)
{
	_data->enableDespawnTimer = b;
}

void Client::SetUpdatesSent(UINT32 us)
{
	_data->updatesSent = us;
}

void Client::SetHitsPrime(UINT32 hp)
{
	_data->hitsPrime = hp;
}

void Client::SendVehicleDamage(Vehicle::Hit const& hit)
{
	teulSend(CellConnection(), commCLIENT_HIT, (void*)&hit);
}


/************************************************************************************************
	Client::HostState
*************************************************************************************************/

void HostState::Reset()
{
	connectState = DISCONNECTED;

	if(connection)
		connection->close();

	connection = nullptr;
	lastError = 0;
	errorName = "";
}

bool HostState::Connect(Teul::Connection* conn, SINT32 cond)
{
	if(!CanConnect())
		return false;

	// make sure any old connection data is cleared
	switch(cond)
	{
		case HostState::CONNECTING:
		{
			// we shouldn't already have a connection
			// report and close it
			if(connection)
			{
#ifdef BETA_RELEASE
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: connecting with existing connection", DisplayName());
#endif
				Disconnect();
			}
		}
		break;
		case HostState::CONNECTED:
		{
			// report this anomalous situation
#ifdef BETA_RELEASE
			if(conn && connection && conn != connection)
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: connected with existing connection", DisplayName());
#endif

			// reset this because its a new connection
			retryCount = 0;

			// increase our connect count
			connectCount += 1;
		}
		break;
		default:
			return false;
		break;
	}

	connection		= conn;
	connectState	= cond;
	lastError		= 0;

	// save our last connected index for logging purposes
	if(conn)
		lastIndex = conn->m_teulIndex;

	return true;
}

void HostState::Disconnect(SINT32 errorCode)
{
	Reset();
	lastError = errorCode;
	connectState = (lastError) ? ECONDITION : DISCONNECTED;
}

bool HostState::OnTransition()
{
	if(connectState != lastConnectState)
	{
		connectState = lastConnectState;
		return true;
	}

	return false;
}

// this call returns true it completed without expiring
bool HostState::Ready(TeulTimeMS timeout)
{
	// determine the wait type
	ww2::Client::MSG_WAIT wait = (ww2::Client::MSG_WAIT)GetWait();

	// set our wait flag
	SetWait();

	const TeulTimeMS expires = teulClock64 + timeout;

	// wait for a response
	while ( client->GetWait(wait) && connectState != ECONDITION )
	{
		if ( teulClock64 >= expires )
		{
			Disconnect(1);
			clientErrorLog("Connection timed out!");
			return false; // we timed out or other error condition
		}

		_yield();
	}

	return true; // we didn't time out
}

void HostState::Init(ww2::Client* c, int type)
{
	client = c;
	hostType = type;
}

void HostState::SetParent(HostState* hs)
{
	parent = hs;
}

bool HostState::Open() const
{
	return (Index() >= 0);
}

bool HostState::Error() const
{
	return (!Open() && connectState == ECONDITION);
}

bool HostState::Connecting() const
{
	return (Open() && connectState == CONNECTING);
}

bool HostState::Connected() const
{
	return (Open() && connectState == CONNECTED);
}

bool HostState::IsType(int type) const
{
	return (hostType == type);
}

bool HostState::IsReconnecting() const
{
	return (retryCount > 0);
}

bool HostState::CanConnect() const
{
	if(!active || exiting)
		return false;

	if(parent && !parent->Connected())
		return false;

	return true;
}

void HostState::SetWait()
{
	client->SetWait((ww2::Client::MSG_WAIT)GetWait());
}

SINT32 HostState::GetWait() const
{
	return ww2::Client::MAP_CONNECTION_WAIT + hostType;
}

void HostState::ClearWait()
{
	client->ClearWait((ww2::Client::MSG_WAIT)GetWait());
}

int HostState::Index() const
{
	return (connection) ? connection->m_teulIndex : -1;
}

int HostState::LastIndex() const
{
	return lastIndex;
}

const char* HostState::HostName()
{
	return "connect.host";
}

const wchar_t* HostState::DisplayName()
{
	return uiLocString(HostName()).c_str();
}

const char* HostState::ErrorName(SINT32 error)
{
	errorName = "connect.error.";
	errorName += Teul::GetErrorName(error);

	// make it lowercase
	std::transform(errorName.begin(), errorName.end(), errorName.begin(), ::tolower);

	return errorName.c_str();
}

SINT32 HostState::ErrorActions(SINT32 error)
{
	if ( error < 0 )
		error = -error;

	// we should have a connection
	// or never got connected (probably a connection failure)
	if(!connection || !connection->m_connected)
		return alABORT;

	// If the connection has been shut down, the host state will need
	// to make a reconnect attempt.
	const UINT32 reconnectNeeded = (connection->terminated() ? (DISCONNECT|RECONNECT) : 0);

	if(error >= Teul::Errors::LoginCategory)
		return alABORT;

	switch(error)
	{
		case 1:			// For stuff that just returns -1.
		case EINVAL:	// Generally a function return value.
			return DISREGARD | reconnectNeeded;
		break;
		case ETEUL_UNDEF_CALLBACK:
			// Someone forgot to register a callback properly...
			// Ultimately this should just be LOG, but right
			// now I'd like to see them...
			return alREPORT | reconnectNeeded;
		break;
		case ETEUL_CONNECTION_STALLED:
		case ENOTCONN:
			return alRETRY;
		break;
		case ETEUL_CB_REQ_DATA:
		case ECONNREFUSED:
#ifdef EPFNOSUPPORT
		case EPFNOSUPPORT:
#endif
		case EAFNOSUPPORT:
		case ENETDOWN:
		case ENETUNREACH:
		case ENETRESET:
#ifdef EHOSTDOWN
		case EHOSTDOWN:
#endif
		case EHOSTUNREACH:
		case ETEUL_SERVER_SHUTDOWN:
			return alABORT;
		break;
		default:
		break;
	}

	if(retryCount < 0)
		return alABORT;

	return alRETRY;
}

void HostState::DoActions(SINT32 error, SINT32 actions)
{
	// nothing to do
	if(actions == HostState::DISREGARD)
		return;

	// we should disco our connection
	if(actions & HostState::DISCONNECT)
		Disconnect(error);

	// if we have a parent that is still connecting or is trying
	// to reconnect, wait for that to resolve itself before doing
	// anything here
	if(parent && (parent->Connecting() || parent->IsReconnecting()))
	{
		pendingActions = actions;
		return;
	}

	// clear this
	pendingActions = 0;

	// if this isn't already a fatal error
	// this will start a series of reconnect attempts
	// but will fall through after 3 tries
	if(!(actions & HostState::EXIT))
	{
		if(actions & HostState::RECONNECT)
		{
			if(Reconnect())
				return;

			// if we experienced an error time to bail
			if(retryCount == -1)
				actions = HostState::alABORT;
		}

		// if we are reconnecting, don't do anything until all attempts have failed
		if(IsReconnecting())
			return;
	}

	// handle any feedback
	client->ConnectionError(this, actions);
}

const char* HostState::FatalCallback() const
{
	return "quit";
}

void HostState::OnError(Teul::Connection* conn, SINT32 error)
{
	// nothing to do if we're exiting the game
	if(exiting)
		return;

	// make sure the error for our current connection object
	//if(!connection || !conn || connection != conn)
	//	return;

	if ( error < 0 )
		error = -error;

	// remember our last error
	lastError = error;

	// how do we handle this error?
	SINT32 actions = ErrorActions(error);

	// do the specificed actions
	DoActions(error, actions);
}

void HostState::OnExit()
{
	active = false;
	exiting = true;
	//Disconnect();
	//connectState = HostState::EXITING;
}

// can't reconnect by default
bool HostState::Reconnect()
{
	retryCount = -1;
	return false;
}

void HostState::Update()
{
	if(pendingActions)
		DoActions(lastError, pendingActions);

#ifdef BETA_RELEASE
	if(reportState != connectState)
	{
		reportState = connectState;

		switch(reportState)
		{
			case ECONDITION:
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: error condition", DisplayName());
			break;
			case DISCONNECTED:
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: disconnecting", DisplayName());
			break;
			case CONNECTING:
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: connecting", DisplayName());
			break;
			case CONNECTED:
                if (hostType == Teul::Connection::Cell)
                    uiChatPrintLineColor(colorORANGEALPHA, L"%ls: connected to %d", DisplayName(), connection->m_serverAddr.m_hostIP - (parent->connection->m_serverAddr.m_hostIP & 0xff));
                else
                    uiChatPrintLineColor(colorORANGEALPHA, L"%ls: connected", DisplayName());
                break;
			case FATAL:
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: fatal condition", DisplayName());
			break;
			case EXITING:
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: exiting", DisplayName());
			break;
			case PENDING:
				uiChatPrintLineColor(colorORANGEALPHA, L"%ls: pending", DisplayName());
			break;
		}
	}
#endif
}

void HostState::SetFatalError()
{
	exiting = true;
	active = false;
}

MapHostState::MapHostState(ww2::Client* c) : HostState(c, Teul::Connection::Map)
{
}

void MapHostState::OnDespawn()
{
	if(Connected())
		mmSendDespawn(connection);
}

SINT32 MapHostState::ErrorActions(SINT32 error)
{
	// map host specific handling here
	return HostState::ErrorActions(error);
}

const char* MapHostState::HostName()
{
	return "connect.host.map";
}

bool MapHostState::CanConnect() const
{
	if(client->_data->dontReconnect)
		return false;

	return HostState::CanConnect();
}

bool MapHostState::Reconnect()
{
	if(!CanConnect())
	{
		retryCount = -1;
		return false;
	}

	// if we're connected, kill our current connection
	Disconnect(0);

	retryCount++;

	if(retryCount < 4)
	{
		try
		{
			if(client->Login(client->_data->host, client->_data->port, client->_data->wantChat) == 0)
			{
				client->Reconnected();
				retryCount = 0;
				return true;
			}
		}
		catch (ClientNetError& e)
		{
			clientErrorLog( "Network connection error#%d: %s", e.Error(), e.What());
			// this is a failure
			retryCount = -1;
			//client->ConnectionError(this, HostState::alABORT);
		}
	}
	else
	{
		// this is a failure
		retryCount = -1;
	}

	return false;
}

ChatHostState::ChatHostState(ww2::Client* c) : HostState(c, Teul::Connection::Chat)
{
	SetParent(c->MapState());
}

SINT32 ChatHostState::ErrorActions(SINT32 error)
{
	// chat host specific handling here
	return HostState::ErrorActions(error);
}

const char* ChatHostState::HostName()
{
	return "connect.host.chat";
}

bool ChatHostState::CanConnect() const
{
	if(!client->WantsChat())
		return false;

	return HostState::CanConnect();
}

bool ChatHostState::Reconnect()
{
	if(!CanConnect())
	{
		retryCount = -1;
		return false;
	}

	// if we're connected, kill our current connection
	Disconnect(0);

	retryCount++;

	if(retryCount < 4)
	{
		try
		{
			if(client->ConnectToChatHost(true))
			{
				retryCount = 0;
				return true;
			}
		}
		catch (ClientNetError& e)
		{
			clientErrorLog( "Network error#%d: %s", e.Error(), e.What());
			retryCount = -1; // bail now
		}
	}
	else
	{
		retryCount = -1; // bail now
	}

	return false;
}

CellHostState::CellHostState(ww2::Client* c) : HostState(c, Teul::Connection::Cell)
{
	SetParent(c->MapState());
}

void CellHostState::OnDespawn()
{
	if(Connected())
		Disconnect();
}

bool CellHostState::Reconnect()
{
	//return HostState::Reconnect();

	if(!CanConnect())
	{
		retryCount = -1;
		return false;
	}

	// if we're connected, kill our current connection
	Disconnect();

	retryCount++;

	if(retryCount < 4)
	{
		// start a connection attempt
		const Teul::ServerAddr* serverIdent = client->GetCellIdent();
		const Teul::PlayerIdent* playerIdent = client->GetPlayerIdent();

		if(!playerIdent || !serverIdent)
		{
			retryCount = -1; // bail now
			return false;
		}

		ClientHostConnection* conn = new ClientHostConnection(Teul::Connection::Cell, *serverIdent, *playerIdent, 30*1000);

		if(conn->start(client))
		{
			// wait for the connection to finish one way or another
			while ( conn->complete() == false )
				_yield();

			// good connection!
			if(Connected())
			{
				retryCount = 0; // bail now
				return true;
			}
		}
	}
	else
	{
		retryCount = -1; // bail now
	}

	return false;
}

SINT32 CellHostState::ErrorActions(SINT32 error)
{
	// cell host specific handling here
	// Cell host is not allowed to cause a client shutdown,
	// only a fallback to the map screen.
	return HostState::ErrorActions(error) & ~(HostState::EXIT);

}

const char* CellHostState::HostName()
{
	return "connect.host.cell";
}

const char* CellHostState::FatalCallback() const
{
	return "despawn";
}

#include "teul/teulUtils.h"

#if defined(USE_LOCALIZATIONS)
# define LocOrStr(_loc, _str) _loc
#else
# define LocOrStr(_loc, _str) _str
#endif

/*
	Example of how you could present network state.

	If we went with this code, you'd need to advise production
	of the following new loc strings:

New localization text messages.

"label" messages should aim to be 4 characters long.

network.state.offline
	To be displayed when the client is not online, e.g.
	"Network: Offline."

network.state.hosts.map
	Prefix for the map host state, e.g.
	"Map:"

network.state.hosts.chat
	Prefix for the chat host state, e.g.
	"Chat:"

network.state.hosts.cell
	Prefix for the cell host state, e.g.
	"World:"

network.state.label.nostate
	Label indicating client has no "state" for a given host, e.g.
	" na "

network.state.label.connecting
	Label indicating this connection is trying to connect, e.g.
	"++++"

network.state.label.reconnecting
	Label indicating this connection is trying to reconnect, e.g.
	"~~~~"

network.state.label.disc
	Label indicating this connection has been disconnected, e.g.
	"----"

"network.state.error" entries provide a label for an error case where
the client state believes it is connected, but teulkit believes otherwise.

network.state.error.einval
	The teulIndex for the state is illegal, suggest
	"INV!"

network.state.error.enoent
	The teulIndex doesn't reference a valid connection any more
	"BAD!"

network.state.error.econnaborted
	The teulindex references a connection that has been disconnected and will shortly be destroyed.
	"DSC!"

network.state.error.enotconn
	The teulIndex is in the process of connecting, but the state hasn't noticed.
	"NCN!"

network.state.error.unknown
	We got an error code we didn't recognize.
	"ERR!"

*/
static void _describeState(const wchar_t* const hostName, std::wstring& into, const HostState* const state)
{
	if ( into.empty() == false )
		into += L" ";

	into += LocOrStr(L"?network.state.hosts.", L"");
	into += hostName;
	into += LocOrStr(L"? ", L": ");

	if ( state == nullptr )
	{
		into += LocOrStr(L"?network.state.label.nostate?", L"----");
		return;
	}

	if ( state->IsReconnecting() )
	{
		into += LocOrStr(L"?network.state.label.reconnecting?", L"~~~~");
		return;
	}

	if ( state->Connecting() == true )
	{
		into += LocOrStr(L"?network.state.label.connecting?", L"++++");
		return;
	}

	if ( state->Connected() == false )
	{
		into += LocOrStr(L"?network.state.label.disc?", L"nocn");
		return;
	}

	SINT32 reason = 0;
	TeulDeltaMS lastRx_ms = 0;
	const bool success = Teul::GetEndpointStatus(state->Index(), &reason, &lastRx_ms);
	if ( success == false )
	{
		if ( reason < 0 )
			reason = -reason;
		switch ( reason )
		{
		case EINVAL:
			into += LocOrStr(L"?network.state.error.einval?", L"INV!");
		break;
		case ENOENT:
			into += LocOrStr(L"?network.state.error.enoent?", L"BAD!");
		break;
		case ECONNABORTED:
			into += LocOrStr(L"?network.state.error.econnaborted?", L"DSC!");
		break;
		case ENOTCONN:
			into += LocOrStr(L"?network.state.error.enotconn?", L"NCN!");
		break;
		default:
			into += LocOrStr(L"?network.state.error.unknown?", L"ERR!");
		break;
		}

		return;
	}

	if ( lastRx_ms <= 5000 )
		into += LocOrStr(L"?network.state.label.ok?", L"-OK-");
	else
	{
		wchar_t lastRx[65];
		swprintf(lastRx, sizeof(lastRx) - 1, L"%3us", (UINT32)(lastRx_ms / 1000ULL));
		into += lastRx;
	}
}

extern SINT32 g_lastPing, g_averagePing;

std::wstring clientGetNetworkState()
{
	Client* const client = PlayerClient();
	if ( !Online || client == nullptr )
		return LocOrStr(L"?network.state.offline?", L"Network: Offline\n");

	std::wstring states(L"");

	_describeState(L"map", states, client->MapState());
	_describeState(L"chat", states, client->ChatState());
	_describeState(L"cell", states, client->CellState());
	if ( client->CellState()->Connected() && g_lastPing > 0 )
	{
		wchar_t ping[65];
		swprintf(ping, sizeof(ping) - 1, L" Ping: %u(%u)ms", g_lastPing, g_averagePing);
		states += ping;
	}
	else
	{
		g_lastPing = 0;
		g_averagePing = 0;
	}

	states += L"\n";

	return states;
}
