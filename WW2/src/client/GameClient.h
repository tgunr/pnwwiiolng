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

	virtual int RequestClientSpawn(WEAPON_SERIAL const&, bool wantMobileSpawn) OVERRIDE_FN;

	virtual void ProcessSpawnNotify(FESpawnNotify const& notify) OVERRIDE_FN;
	virtual void ProcessPostAccept(FE_POST_ACCEPT const& postAccept) OVERRIDE_FN;
	virtual void ProcessCrewJoinAccept(FE_JOIN_ACCEPT const&) OVERRIDE_FN;
	virtual void ProcessCrewJoinDeny(FE_JOIN_DENY const& joindeny) OVERRIDE_FN;
	virtual void ProcessCrewBailed(FE_CREW_BAILED const& crewbailed) OVERRIDE_FN;
	virtual void ProcessCrewPostExpired(FE_POST_EXPIRED const& crewexpired) OVERRIDE_FN;
	virtual void ProcessForcedCellExit(unsigned int exitCode) OVERRIDE_FN;
	virtual void ProcessGameTimeUpdate(Host::TimeUpdate const&) OVERRIDE_FN;
	virtual void ProcessWorldUpdate(WORLD_UPDATE const&) OVERRIDE_FN;
	virtual void ProcessChatHostMessage(Chat::HostMessage const&) OVERRIDE_FN;
	virtual void ProcessChatTuneToChannel(Chat::TuneToChannel const&) OVERRIDE_FN;
	virtual void ProcessChatNLA(Chat::UserNLA const&) OVERRIDE_FN;
	virtual void ProcessChatLocalizedMessage(Chat::LocalizedMessage const&) OVERRIDE_FN;
	virtual void ProcessChatGCIMessage(Chat::GCIMessage const&) OVERRIDE_FN;
	virtual void ProcessMissionResults(MISSION_RESULTS const&) OVERRIDE_FN;
	virtual void ProcessMissionRoomListReply(RoomList const& missroomlist) OVERRIDE_FN;
	virtual void ProcessPostList(FE_POST_LIST_RESPONSE  const& response) OVERRIDE_FN;
	virtual void ProcessPostDeny(FE_POST_DENY const& postDeny) OVERRIDE_FN;
	virtual void ProcessCrewJoinPerm(FE_JOIN_PERM_REQUEST const& joinperm) OVERRIDE_FN;
	virtual void ProcessPersonaSummaries(PersonaSummaries const& summaries) OVERRIDE_FN;
	virtual void ProcessMissionList(MissionList const& missionList)  OVERRIDE_FN;
	virtual void ProcessWeapList(struct ACTION_OBJECT* a)  OVERRIDE_FN;
	virtual void ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const&)  OVERRIDE_FN;
	virtual void ProcessEmptyChatWhoList(int type)  OVERRIDE_FN;
	virtual void ProcessChatWho(int type, const Chat::Who&) OVERRIDE_FN;
	virtual void ProcessSquad(const SquadMsg&) OVERRIDE_FN;
	virtual void ProcessSquadControl(const SquadControlResult&) OVERRIDE_FN;
	virtual void ProcessSquadInvite(const SquadMsg&) OVERRIDE_FN;
	virtual void ProcessWaypoint(Waypoint const&, int type) OVERRIDE_FN;
	virtual void ProcessWaypointControl(WaypointControlMsg const&, int type, unsigned long long handle) OVERRIDE_FN;
	virtual void ProcessMissionPostReply(MISSION_POST_REPLY const& reply) OVERRIDE_FN;
	virtual void ProcessSwingMeter(SwingMeter const& meter) OVERRIDE_FN;
	virtual void ProcessOpinionPoll(OpinionPoll const&) OVERRIDE_FN;
	virtual void ProcessMissionDetail(MissionDetail const&) OVERRIDE_FN;
	virtual void ProcessMissionInvite(const MissionInvite& invite) OVERRIDE_FN;
	virtual void ProcessStratEventPacket(const Strat::EventPacket& packet) OVERRIDE_FN;
	virtual int  RequestDirectSpawn(WEAPON_SPEC const& spec, const FLOAT64VECTOR* at = nullptr, FLOAT64 yaw = 0) OVERRIDE_FN;
	virtual void SendPlayerExit(int exitcode, FULL_OBJ_ID const&) OVERRIDE_FN;

	virtual void ProcessPositionUpdateMsg(VEHICLE_UPDATE_HEADER const&) OVERRIDE_FN;
	virtual void ProcessRemoveVehicle(unsigned int hostindex) OVERRIDE_FN;
	virtual void ProcessVehicleHit(Vehicle::Hit const&) OVERRIDE_FN;
	virtual void ProcessVehicleRequest(VEHICLE_REPLY const&) OVERRIDE_FN;
	virtual void ProcessBrigadeControl(BrigadeControlList const& entry) OVERRIDE_FN;
	virtual void ProcessBrigadeControlNotice(const BrigadeControlNotice* const notice) OVERRIDE_FN;
	virtual void ProcessHCUnitLocation(const HCUnitLocation& unitloc) OVERRIDE_FN;
	virtual void ProcessInstanceResult(const Instance::Result& result) OVERRIDE_FN;
	virtual void ProcessInstanceList(const Instance::ClientListing* list) OVERRIDE_FN;
	virtual void ProcessMissionStatus(const Chat::MissionStatus* const) OVERRIDE_FN;
	virtual bool ReceiveServerTrackedObject(int type, ACTION_OBJECT* a) OVERRIDE_FN;

	virtual void ActivateCell() OVERRIDE_FN;
	virtual void ActivateMap() OVERRIDE_FN;

	virtual void ConnectionLost(const char* description, int errorNo) OVERRIDE_FN;
	virtual void ConnectionError(ww2::HostState* state, UINT32 actions) OVERRIDE_FN;
	virtual void KickBanPopup(ww2::HostState* state, UINT32 actions) OVERRIDE_FN;
	virtual void StatusMessage(const char*) OVERRIDE_FN;

	virtual void Update() OVERRIDE_FN;

	virtual void Reconnected() OVERRIDE_FN;
	virtual void SpawnComplete() OVERRIDE_FN;
	virtual void RedirectFailed() OVERRIDE_FN;

	virtual bool SelectPersona(const PersonaSummary& persona, bool save = false, bool force = false) OVERRIDE_FN;
	virtual bool SelectBrigade(UINT32 brigadeID, bool save = false, bool force = false) OVERRIDE_FN;

	virtual void OnDespawn() OVERRIDE_FN;

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

int GameClientLogin(unsigned int hostAddress, int playerID, int otp,
					unsigned int appVersion, short port);
GameClient* PlayerClient();
void GameClientKill();
