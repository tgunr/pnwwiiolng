#pragma once

#include <vector>

#include "chat/chatPrototypes.h"			// Common chat structure protos

struct SHARED_VEHICLE_INFO;
struct VEHICLE_REPLY;
struct VEHICLE_UPDATE_HEADER;
struct FULL_OBJ_ID;
struct WEAPON_SPEC;
struct WEAPON_SERIAL;
class FacilityWeaponList;
struct FE_POST_ACCEPT;
struct FE_POST_DENY;
class PersonaSummaries;
class FESpawnNotify;
class MissionDetail;
class PersonaSelection;
class PersonaSummary;
class BrigadeSelection;
class MissionList;
class BrigadeSummaries;
struct MISSION_POST_REQ;
struct MISSION_POST_REPLY;
struct MOTD_REPLY;
class RoomList;
class RoomListEntry;
struct MISSION_POST_REPLY;
struct FE_POST_LIST_RESPONSE;
struct COMMANDED_WEAPON;
struct FE_JOIN_PERM_REQUEST;
struct FE_JOIN_ACCEPT;
struct FE_JOIN_DENY;
struct FE_POST_EXPIRED;
struct FE_CREW_BAILED;
struct WORLD_UPDATE;
struct MISS_LAUNCH_UPDATE;
struct MISSION_RESULTS;
struct MISS_LAUNCH_UPDATE;
struct ACCEPT_STRUCT;
class ActiveMission;
class Waypoint;
class WaypointControlMsg;
class MissionMember;
class ClientSquad;
class SquadMsg;
class SquadInvite;
class SquadControlResult;
class SwingMeter;
class OpinionPoll;
struct STRAT_SUPERCELL_PACKET;
struct STRAT_CELL_PACKET;
class BrigadeControlList;
class BrigadeControlNotice;
class Recruiter;
struct SquadRecruitment;
class MissionInvite;
class HCUnitLocation;
struct CP_HIT_HEADER;
struct ACTION_OBJECT;
struct ChatPermissions;
class CaptureState;
struct ClientData;
struct ClientClassData;
struct ChatServerInfo;

namespace Chat
{
	struct MissionStatus;
	class Who;
};

namespace Instance
{
	class Result;
	class Descriptor;
	class ClientListing;
};
namespace Strat
{
	class Event;
	class EventPacket;
};
namespace Host
{
	struct TimeUpdate;
};

namespace Vehicle
{
	struct Update;
	struct Hit;
};

namespace Teul
{
	class Connection;
	struct PlayerIdent;
	struct ServerAddr;
};

namespace ww2
{
	enum SAVE_MASK
	{
		SAVE_INSTANCE	= 1<<0,
		SAVE_COUNTRY	= 1<<1,
		SAVE_BRANCH		= 1<<2,
		SAVE_BRIGADE		= 1<<3,
		SAVE_MISSION	= 1<<4
	};

	class Client;
	struct HostState;

#define SAVE_PERSONA SAVE_COUNTRY|SAVE_BRANCH

/************************************************************************************************
	ww2::Client
*************************************************************************************************/

class Client
{
public:
	Client();
	virtual ~Client();

	// Login throws ClientNetError if there's a problem
	int Login(const char* hostname, UINT16 port=0, bool wantchat=false);
	int Login(unsigned int host, UINT16 port=0, bool wantchat=false);

	int Logout();

	// are we connected to the mmhost
	bool Connected();

	// teulIndex accessors
	int MapConnection() const;
	int CellConnection() const;
	int ChatConnection() const;

	// host state accessors
	ww2::HostState* MapState();
	ww2::HostState* ChatState();
	ww2::HostState* CellState();
	ww2::HostState* GetHostStateByIndex(int teulIndex);
	ww2::HostState* GetHostStateByType(int type);

	// set our host connection player login info; if you don't call this
	// the client gets inited with whats in _defaultPlayerIdent
	void SetPlayerIdent(playerid_t playerID, UINT32 otp, unsigned int appVersion);
	void SetCellIdent(int hostIP, int port);

	// updates
	virtual void Update();
	virtual void SendUpdate();

	bool CellClientActive() const;
	bool MapClientActive() const;

	virtual void OnDespawn();
	void OnExit();

	void ResetMissionData();

	virtual int RequestPersonaSummaries();
	virtual int RequestMissionWeapList(FULL_OBJ_ID const&,  bool allweapons, bool isrefresh);
	virtual int RequestMissionWeapPost(FULL_OBJ_ID const&, const WEAPON_SPEC, bool immediatespawn);
	virtual int RequestMissionDetail(int missionID, bool join);
	virtual int RequestMissionLookup(int missionID); // isn't necessarily a mission in your brigade
	virtual int RequestClientSpawn(WEAPON_SERIAL const&, bool wantMobileSpawn);
	virtual int RequestPersonaSelection(PersonaSummary const&);
	virtual int RequestBrigadeSelection(unsigned int brigadeID);
	virtual int RequestBrigadeDetail(unsigned int brigadeID);
	virtual int RequestMissionList();
	virtual int RequestQuickMissionList();
	virtual int RequestBrigadeList();
	virtual int RequestMissionPost(MISSION_POST_REQ const&);
	virtual int RequestMissionWeapCancel();
	virtual int RequestMissionCrew(unsigned int numcrew);
	virtual int RequestMissionPostList(FULL_OBJ_ID const&);
	virtual int RequestJoin(WEAPON_SERIAL const&, ChatHandle const&);
	virtual int SendAcceptCrew(unsigned int joinrequestid);
	virtual int ExitMission(bool cancel=false);
	//virtual int RequestMissionResults();
	virtual int RequestMotd(unsigned int id);
	virtual int RequestVehicleJoin(WEAPON_SERIAL const&, int timestamp);
	virtual int RequestDirectSpawn(WEAPON_SPEC const& spec, const FLOAT64VECTOR* at = nullptr, FLOAT64 yaw = 0);
	virtual int RequestRecruiters(int language);
	virtual void ClearMissionWeap();
	virtual void SetAreaChat(bool on);
	virtual void SendPlayerExit(int exitcode, FULL_OBJ_ID const&);
	virtual void SetHC(bool status);
	virtual void SetKilledByAI(bool status);
	virtual bool GetKilledByAI();
	virtual bool IsHC() const;
	virtual bool IsOIC() const;
	virtual unsigned int OICOfBrigade() const; // return the brigade I'm OIC of or 0

	virtual int SetWaypoint(Waypoint const&);
	virtual int RemoveWaypoint(unsigned long long handle);
	virtual void SetActiveWaypoint(unsigned long long handle = 0);
	virtual ChatHandle ActiveWaypointHandle() const;
	virtual	Waypoint ActiveWaypoint() const;
	virtual bool IsMissionLeader() const;

	int TuneChatToChannel(int radioidx, int type);
	int TuneChatToHandle(int radioidx, const char* handle);
	int SetChatReceive(int radioidx, bool on);
	int SetChatIgnore(const char* handle);
	int SendChatMessage(unsigned int txmask, const char* message);
	void ClearJoinRequests();

	void SendVehicleDamage(Vehicle::Hit const&);

	// override these to do things in response to requests
	// if you override you must call the base function first
	// as it sets the request state
	virtual void SetActiveHost(unsigned int hostIP, unsigned int port);
	virtual void ActivateMap();
	virtual void ActivateCell();
	virtual void ProcessWeapList(struct ACTION_OBJECT* a);
	virtual void ProcessPostAccept(FE_POST_ACCEPT const&);
	virtual void ProcessPostDeny(FE_POST_DENY const&);
	virtual void ProcessPersonaSummaries(PersonaSummaries const&);
	virtual void ProcessSpawnNotify(FESpawnNotify const&);
	virtual void ProcessMissionDetail(MissionDetail const&);
	virtual void ProcessPersonaSelection(PersonaSelection const&);
	virtual void ProcessBrigadeSelection(BrigadeSelection const&);
	virtual void ProcessChatServerInfo(ChatServerInfo const&);
	virtual void ProcessMissionList(MissionList const&);
	virtual void ProcessBrigadeSummaries(BrigadeSummaries const&);
	virtual void ProcessMissionPostReply(MISSION_POST_REPLY const&);
	virtual void ProcessMissionRoomListReply(RoomList const&);
	virtual void ProcessPostList(FE_POST_LIST_RESPONSE const&);
	virtual void ProcessCrewJoinPerm(FE_JOIN_PERM_REQUEST const&);
	virtual void ProcessCrewJoinAccept(FE_JOIN_ACCEPT const&);
	virtual void ProcessCrewJoinDeny(FE_JOIN_DENY const&);
	virtual void ProcessCrewPostExpired(FE_POST_EXPIRED const&);
	virtual void ProcessCrewBailed(FE_CREW_BAILED const&) {}
	virtual void ProcessForcedCellExit(unsigned int exitCode) {}
	virtual void ProcessGameTimeUpdate(Host::TimeUpdate const&) {}
	virtual void ProcessWorldUpdate(WORLD_UPDATE const&);
	virtual void ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const&);
	virtual void ProcessMissionResults(MISSION_RESULTS const&);
	virtual void ProcessChatMotdReply(struct MOTD_REPLY const&);
	virtual void ProcessChatHostMessage(Chat::HostMessage const&){}
	virtual void ProcessChatTuneToChannel(Chat::TuneToChannel const&) {}
	virtual void ProcessChatNLA(Chat::UserNLA const&) {}
	virtual void ProcessChatLocalizedMessage(Chat::LocalizedMessage const&){}
	virtual void ProcessChatGCIMessage(Chat::GCIMessage const&) {}
	virtual void ProcessMissionListUpdate(unsigned int brigadeID);
	virtual void ProcessEmptyChatWhoList(int type) {}
	virtual void ProcessChatWho(int type, const Chat::Who&);
	virtual void ProcessSquad(const SquadMsg&);
	virtual void ProcessRecruiter(const Recruiter& rec, bool reset=false);
	virtual void ProcessRecruiter(bool reset=true);
	virtual void ProcessSquadControl(const SquadControlResult&) {}
	virtual void ProcessSquadInvite(const SquadMsg&);
	virtual void ProcessWaypoint(Waypoint const&, int type);
	virtual void ProcessWaypointControl(WaypointControlMsg const&, int type, unsigned long long handle);
	virtual void ProcessMissionMemberUpdate(MissionMember const&);
	virtual void ProcessSwingMeter(SwingMeter const& meter);
	virtual void ProcessOpinionPoll(OpinionPoll const&);
	virtual void ProcessStratUpdate(FULL_OBJ_ID const& id, int ownership, unsigned int flags) {}
	virtual void ProcessStratEventPacket(const Strat::EventPacket& packet) {}
	virtual void ProcessPositionUpdateMsg(VEHICLE_UPDATE_HEADER const&){}
	virtual void ProcessRemoveVehicle(unsigned int hostindex){}
	virtual void ProcessVehicleHit(Vehicle::Hit const&) {}
	virtual void ProcessVehicleRequest(VEHICLE_REPLY const&);
	virtual void ProcessBrigadeControl(BrigadeControlList const& entry);
	virtual void ProcessBrigadeControlNotice(const BrigadeControlNotice* const notice);
	virtual void ProcessMissionInvite(const MissionInvite& invite);
	virtual void ProcessHCUnitLocation(const HCUnitLocation& unitloc){}
	virtual void ProcessInstanceResult(const Instance::Result& result);
	virtual void ProcessInstanceList(const Instance::ClientListing* list);
	virtual void ProcessTutorialFlags(ChatHandle group, UINT64 flags);
	virtual void ProcessChatPermissions(const ChatPermissions& permissions);
	virtual void ProcessCaptureState(const CaptureState& state);
	virtual void ProcessMissionStatus(const Chat::MissionStatus* const);
	virtual void ProcessLoginAttempt(bool success, SINT32 type, SINT32 result = 0);

	virtual bool ReceiveServerTrackedObject(int type, ACTION_OBJECT* a) { return false; }
	virtual void Reconnected(){}

	virtual void SpawnComplete();
	virtual void RedirectFailed(){}

	// Request connection to the chat host, requires ChatServerInfo being received;
	// set blocking = true if you want to wait for the connection to happen.
	// returns false if the attempt failed.
	virtual bool WantsChat() const;
	virtual bool ConnectToChatHost(bool blocking);
	virtual void ConnectionLost(const char* description, int errorNo){}
	virtual void ConnectionError(ww2::HostState* state, UINT32 actions){}
	virtual void KickBanPopup(ww2::HostState* state, UINT32 actions){}
	virtual void StatusMessage(const char*){}

	virtual bool SelectInstance(UINT32 category, UINT32 type = 0, bool save = false, bool force = false);
	virtual bool SelectPersona(const PersonaSummary& persona, bool save = false, bool force = false);
	virtual bool SelectBrigade(UINT32 brigadeID, bool save = false, bool force = false);
	virtual bool LoadSavedPersona(bool force = false);

	FacilityWeaponList const& WeaponList() const;
	PersonaSummaries& Personas() const;
	WEAPON_SERIAL const& WeaponSerial() const;
	PersonaSelection const& PersonaSelected() const;
	BrigadeSelection const& BrigadeSelected() const;
	std::vector<MissionDetail> const&  Missions() const;
	BrigadeSummaries const& Brigades() const;
	const char* MissionDescription() const;
	std::vector<RoomListEntry> const& MissionRoomList(bool sorted = false) const;
	MISSION_POST_REPLY const& MissionPostReply() const;
	std::vector<COMMANDED_WEAPON> const& CommandedWeapons() const;
	std::vector<FE_JOIN_PERM_REQUEST> const& JoinRequests() const;
	MissionDetail& SelectedMission() const;
	MISSION_RESULTS const& MissionResults() const;
	FESpawnNotify const& SpawnNotify() const;
	int DespawnTimer() const;
	BrigadeSelection const& BrigadeDetails() const;
	struct MOTD_REPLY const& MotdReply() const;
	bool CanPostMission() const;
	MISS_LAUNCH_UPDATE const& LaunchUpdate() const;
	unsigned int MissionListUpdate() const;
	ActiveMission const& GetActiveMission() const;
	std::vector<Waypoint> const& Waypoints() const;
	std::vector<MissionMember> const& MissionMembers() const;
	std::vector<BrigadeControlNotice> const& BrigadeNotices() const;
	ClientSquad const& PlayerSquad() const;
	SquadInvite const& PlayerSquadInvite() const;
	void InvalidateSquadInvite();
	std::wstring const& CurrentOrders() const;
	void SetCurrentOrders(const wchar_t* o);
	int SideBalance() const;
	int IsSideDelayed() const;
	int IsSpawnDelayed() const;
	OpinionPoll const& GetOpinionPoll() const;
	VEHICLE_REPLY const& VehicleReply() const;
	MissionDetail& MissionLookup() const;
	BrigadeControlList* LookupBrigadeOIC(int brigadeID) const;
	SquadRecruitment& PlayerSquadRecruitment() const;
	MissionInvite const& GetMissionInvite() const;
	const Instance::Descriptor& GetInstance() const;
	const Instance::ClientListing* GetInstances() const;
	const FLOAT64VECTOR& GetSpawnOverride() const;
	CaptureState& GetCaptureState(); // leave non-const because it gets updated by the client too
	UINT32 UpdatesSent(bool increment = true);
	UINT32 HitsPrime() const;
	ChatHandle& MultiCrewCommander() const;
	bool MobileSpawnMission() const;
	const Chat::MissionStatus* MissionStatus() const;
	const Teul::PlayerIdent* GetPlayerIdent() const;
	const Teul::ServerAddr* GetCellIdent() const;

	// subscription permissions api
	bool IsPermitted(UINT64 flag, bool checkSubscription = true) const; // wwiiPermissions.h
	UINT32 MaxRank(UINT32 country, UINT32 branch) const;
	UINT32 MaxSquadRank() const;
	SINT32 SubscriptionExpires(bool asStage = true) const; // when our subscription expires

	void SavePersona(int type = -1); // -1 == save all attributes
	const PersonaSummary* GetPersona(int country = -1, int branch = -1) const;
	const PersonaSummary* GetSavedPersona() const;

	void SetSpawnOverride(double x, double y, double z);
	void SetPosition(double x, double y, double z);
	void SetVelocity(double vx, double vy, double vz);
	void SetAcceleration(double ax, double ay, double az);

	void SetAttitude(double pitch, double roll, double yaw);
	void SetAngularVelocity(double wx, double wy, double wz);

	void SetStateFlagBit(int bitnum, bool on);
	void SetStateFlag(unsigned int flag);

	void SetFireState(int bitnum, int value, bool onetime);
	void SetTurretPosition(int bitnum, unsigned char traverse, unsigned char elev);
	void SetVisBits(unsigned int visbits);
	void SetRide(unsigned int RideVehicleHostIndex, unsigned short RideSeatNum);
	void SetRecruiter(const Recruiter&);
	void SetVehicleKilled(bool);
	void SetCrewDead(bool);
	void EnableDespawnTimer(bool);

	void SetUpdatesSent(UINT32 us);
	void SetHitsPrime(UINT32 hp);

	static Client* FromTeulIndex(int teulIndex);
	static int Init(int playerID = 0, int otp = 0, unsigned int appVersion = 0);
	static void Kill();
	static void NetUpdate(int wait=0);
	static unsigned int NetTime();

	void SetAutoReconnect(bool);

enum MSG_WAIT
{
	MISSION_DETAIL_WAIT
,	PERSONA_SELECT_WAIT
,	BRIGADE_SELECT_WAIT
,	MISSION_POST_WAIT
,	WEAP_POST_WAIT
,	MOTD_WAIT
,	BRIGADE_LIST_WAIT
,	SPAWN_NOTIFY_WAIT
,	CELL_CLIENT_WAIT
,	MAP_CLIENT_WAIT
,	REDIRECT_WAIT
,	VEHICLE_REPLY_WAIT
,	REQUEST_RECRUITERS_WAIT
,	MISSION_INVITE_WAIT
,	SQUAD_RECRUITMENT_WAIT
,	INSTANCE_WAIT
,	PERSONAS_REQUEST_WAIT
,	MAP_CONNECTION_WAIT
,	CHAT_CONNECTION_WAIT
,	CELL_CONNECTION_WAIT

// ^-- Insert stuff up here --^
,	MAX_MSG_WAIT
// X-- No, don't insert stuff down here. --X //
};

	int WaitForMessage(MSG_WAIT msg, int timeout);
	void SetWait(MSG_WAIT);
	int GetWait(MSG_WAIT);
	void ClearWaits();
	void ClearWait(MSG_WAIT);
	int WaitForMessages(int timeout);

enum MSG_TIME
{
	MISSION_RESULTS_TIME
,	PERSONA_SUMMARIES_TIME
,	MAX_MSG_TIME
};

	int LastMessageReceivedTime(MSG_TIME);

	ClientData* _data;
	ClientClassData* _classdata;
};

}

std::wstring clientGetNetworkState();
