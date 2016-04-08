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

	virtual int RequestClientSpawn(WEAPON_SERIAL const&, bool wantMobileSpawn) override;

	virtual void ProcessSpawnNotify(FESpawnNotify const& notify) override;
	virtual void ProcessPostAccept(FE_POST_ACCEPT const& postAccept) override;
	virtual void ProcessCrewJoinAccept(FE_JOIN_ACCEPT const&) override;
	virtual void ProcessCrewJoinDeny(FE_JOIN_DENY const& joindeny) override;
	virtual void ProcessCrewBailed(FE_CREW_BAILED const& crewbailed) override;
	virtual void ProcessCrewPostExpired(FE_POST_EXPIRED const& crewexpired) override;
	virtual void ProcessForcedCellExit(unsigned int exitCode) override;
	virtual void ProcessGameTimeUpdate(Host::TimeUpdate const&) override;
	virtual void ProcessWorldUpdate(WORLD_UPDATE const&) override;
	virtual void ProcessChatHostMessage(Chat::HostMessage const&) override;
	virtual void ProcessChatTuneToChannel(Chat::TuneToChannel const&) override;
	virtual void ProcessChatNLA(Chat::UserNLA const&) override;
	virtual void ProcessChatLocalizedMessage(Chat::LocalizedMessage const&) override;
	virtual void ProcessChatGCIMessage(Chat::GCIMessage const&) override;
	virtual void ProcessMissionResults(MISSION_RESULTS const&) override;
	virtual void ProcessMissionRoomListReply(RoomList const& missroomlist) override;
	virtual void ProcessPostList(FE_POST_LIST_RESPONSE  const& response) override;
	virtual void ProcessPostDeny(FE_POST_DENY const& postDeny) override;
	virtual void ProcessCrewJoinPerm(FE_JOIN_PERM_REQUEST const& joinperm) override;
	virtual void ProcessPersonaSummaries(PersonaSummaries const& summaries) override;
	virtual void ProcessMissionList(MissionList const& missionList)  override;
	virtual void ProcessWeapList(struct ACTION_OBJECT* a)  override;
	virtual void ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const&)  override;
	virtual void ProcessEmptyChatWhoList(int type)  override;
	virtual void ProcessChatWho(int type, const Chat::Who&) override;
	virtual void ProcessSquad(const SquadMsg&) override;
	virtual void ProcessSquadControl(const SquadControlResult&) override;
	virtual void ProcessSquadInvite(const SquadMsg&) override;
	virtual void ProcessWaypoint(Waypoint const&, int type) override;
	virtual void ProcessWaypointControl(WaypointControlMsg const&, int type, unsigned long long handle) override;
	virtual void ProcessMissionPostReply(MISSION_POST_REPLY const& reply) override;
	virtual void ProcessSwingMeter(SwingMeter const& meter) override;
	virtual void ProcessOpinionPoll(OpinionPoll const&) override;
	virtual void ProcessMissionDetail(MissionDetail const&) override;
	virtual void ProcessMissionInvite(const MissionInvite& invite) override;
	virtual void ProcessStratEventPacket(const Strat::EventPacket& packet) override;
	virtual int  RequestDirectSpawn(WEAPON_SPEC const& spec, const FLOAT64VECTOR* at = nullptr, FLOAT64 yaw = 0) override;
	virtual void SendPlayerExit(int exitcode, FULL_OBJ_ID const&) override;

	virtual void ProcessPositionUpdateMsg(VEHICLE_UPDATE_HEADER const&) override;
	virtual void ProcessRemoveVehicle(unsigned int hostindex) override;
	virtual void ProcessVehicleHit(Vehicle::Hit const&) override;
	virtual void ProcessVehicleRequest(VEHICLE_REPLY const&) override;
	virtual void ProcessBrigadeControl(BrigadeControlList const& entry) override;
	virtual void ProcessBrigadeControlNotice(const BrigadeControlNotice* const notice) override;
	virtual void ProcessHCUnitLocation(const HCUnitLocation& unitloc) override;
	virtual void ProcessInstanceResult(const Instance::Result& result) override;
	virtual void ProcessInstanceList(const Instance::ClientListing* list) override;
	virtual void ProcessMissionStatus(const Chat::MissionStatus* const) override;
	virtual bool ReceiveServerTrackedObject(int type, ACTION_OBJECT* a) override;

	virtual void ActivateCell() override;
	virtual void ActivateMap() override;

	virtual void ConnectionLost(const char* description, int errorNo) override;
	virtual void ConnectionError(ww2::HostState* state, UINT32 actions) override;
	virtual void KickBanPopup(ww2::HostState* state, UINT32 actions) override;
	virtual void StatusMessage(const char*) override;

	virtual void Update() override;

	virtual void Reconnected() override;
	virtual void SpawnComplete() override;
	virtual void RedirectFailed() override;

	virtual bool SelectPersona(const PersonaSummary& persona, bool save = false, bool force = false) override;
	virtual bool SelectBrigade(UINT32 brigadeID, bool save = false, bool force = false) override;

	virtual void OnDespawn() override;

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
