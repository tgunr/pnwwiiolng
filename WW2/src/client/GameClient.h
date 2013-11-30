#pragma once

#include <set>

#include "phui/MessageHandler.h"
#include "client/ww2Client.h"
#include "js/jsClient.h"
#include "sdm/StratDB.h"

class jsClient;
class ClientHostConnection;

/************************************************************************************************
	GameClient
*************************************************************************************************/

class GameClient : public ww2::Client
{
public:
	GameClient();

	enum SPAWN_STATUS
	{
		SPAWN_DESPAWNED, // we're in the UI
		SPAWN_PENDING, // we're waiting for a spawn notification
		SPAWN_RECEIVED, // we have received a spawn notification
		SPAWN_VEHICLE, // join our vehicle (init)
		SPAWN_SPAWNED // we have spawned
	};

	virtual int RequestClientSpawn(WEAPON_SERIAL const&, bool wantMobileSpawn);

	virtual void ProcessSpawnNotify(FESpawnNotify const& notify);
	virtual void ProcessPostAccept(FE_POST_ACCEPT const& postAccept);
	virtual void ProcessCrewJoinAccept(FE_JOIN_ACCEPT const&);
	virtual void ProcessCrewJoinDeny(FE_JOIN_DENY const& joindeny);
	virtual void ProcessCrewBailed(FE_CREW_BAILED const& crewbailed);
	virtual void ProcessCrewPostExpired(FE_POST_EXPIRED const& crewexpired);
	virtual void ProcessForcedCellExit(unsigned int exitCode);
	virtual void ProcessGameTimeUpdate(Host::TimeUpdate const&);
	virtual void ProcessWorldUpdate(WORLD_UPDATE const&);
	virtual void ProcessChatHostMessage(Chat::HostMessage const&);
	virtual void ProcessChatTuneToChannel(Chat::TuneToChannel const&);
	virtual void ProcessChatNLA(Chat::UserNLA const&);
	virtual void ProcessChatLocalizedMessage(Chat::LocalizedMessage const&);
	virtual void ProcessChatGCIMessage(Chat::GCIMessage const&);
	virtual void ProcessMissionResults(MISSION_RESULTS const&);
	virtual void ProcessMissionRoomListReply(RoomList const& missroomlist);
	virtual void ProcessPostList(FE_POST_LIST_RESPONSE  const& response);
	virtual void ProcessPostDeny(FE_POST_DENY const& postDeny);
	virtual void ProcessCrewJoinPerm(FE_JOIN_PERM_REQUEST const& joinperm);
	virtual void ProcessPersonaSummaries(PersonaSummaries const& summaries);
	virtual void ProcessMissionList(MissionList const& missionList);
	virtual void ProcessDialog(int dialog);
	virtual void ProcessWeapList(struct ACTION_OBJECT* a);
	virtual void ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const&);
	virtual void ProcessEmptyChatWhoList(int type) ;
	virtual void ProcessChatWho(int type, const Chat::Who&);
	virtual void ProcessSquad(const SquadMsg&);
	virtual void ProcessSquadControl(const SquadControlResult&) ;
	virtual void ProcessSquadInvite(const SquadMsg&) ;
	virtual void ProcessWaypoint(Waypoint const&, int type);
	virtual void ProcessWaypointControl(WaypointControlMsg const&, int type, unsigned long long handle);
	virtual void ProcessMissionPostReply(MISSION_POST_REPLY const& reply);
	virtual void ProcessSwingMeter(SwingMeter const& meter);
	virtual void ProcessOpinionPoll(OpinionPoll const&);
	virtual void ProcessMissionDetail(MissionDetail const&);
	virtual void ProcessMissionInvite(const MissionInvite& invite);
	virtual void ProcessStratEventPacket(const Strat::EventPacket& packet);
	virtual int  RequestDirectSpawn(WEAPON_SPEC const& spec, const FLOAT64VECTOR* at = NULL, FLOAT64 yaw = 0);
	virtual void SendPlayerExit(int exitcode, FULL_OBJ_ID const&);

	virtual void ProcessPositionUpdateMsg(VEHICLE_UPDATE_HEADER const&);
	virtual void ProcessRemoveVehicle(unsigned int hostindex);
	virtual void ProcessVehicleHit(Vehicle::Hit const&) ;
	virtual void ProcessVehicleRequest(VEHICLE_REPLY const&) ;
	virtual void ProcessBrigadeControl(BrigadeControlList const& entry);
	virtual void ProcessBrigadeControlNotice(const BrigadeControlNotice* const notice);
	virtual void ProcessHCUnitLocation(const HCUnitLocation& unitloc);
	virtual void ProcessInstanceResult(const Instance::Result& result);
	virtual void ProcessInstanceList(const Instance::ClientListing* list);
	virtual void ProcessMissionStatus(const Chat::MissionStatus* const);
	virtual bool ReceiveServerTrackedObject(int type, ACTION_OBJECT* a);

	virtual void ActivateCell();
	virtual void ActivateMap();

	virtual void ConnectionLost(const char* description, int errorNo);
	virtual void ConnectionError(ww2::HostState* state, UINT32 actions);
	virtual void KickBanPopup(ww2::HostState* state, UINT32 actions);
	virtual void StatusMessage(const char*);

	virtual void Update();

	virtual void Reconnected();
	virtual void SpawnComplete();
	virtual void RedirectFailed();

	virtual void OnDespawn();

	virtual bool SelectPersona(const PersonaSummary& persona, bool save = false, bool force = false);
	virtual bool SelectBrigade(UINT32 brigadeID, bool save = false, bool force = false);

	void DoVehicleJoin();

	jsClient* jsclient;

	void RegisterHandler(phui::MessageHandler*);
	void UnregisterHandler(phui::MessageHandler*);
	void PostCPUpdate(phui::Message&);
	void Edit(const char* type, const char* title, const char* value);

protected:
	void AddStratObjectToCPUpdate(phui::Message&, FULL_OBJ_ID const&);
	void AddStratObjectToCPUpdate(phui::Message&, StratDB::StratObject const*);

	void SetChildToCPOwner(StratDB::StratObject*, phui::Message&);

	SPAWN_STATUS spawnStatus;
	std::set<phui::MessageHandler*> _handlers;
	FLOAT64 mobilespawnyaw;

private:
	GameClient(const GameClient&);				// Prevent copy ctor.
	GameClient& operator=(const GameClient&);	// prevent copy operator
};

extern int GameClientLogin(unsigned int hostAddress, int playerID, int otp,
					unsigned int appVersion, short port);
extern GameClient* PlayerClient();
extern void GameClientKill();
