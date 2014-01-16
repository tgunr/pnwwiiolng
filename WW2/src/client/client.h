#pragma once

struct FULL_OBJ_ID;
struct CP_OBJ_EVENT;

void clientForcedCellExit(int teulIndex, unsigned int exitCode);
void clientActivateCell(int teulIndex, unsigned int on, unsigned int hostIP, unsigned int port);
void clientActivateMap(int teulIndex, unsigned int on, unsigned int hostIP, unsigned int port);

// strat
namespace Strat
{
	class Event;
	class EventPacket;
	struct EventData;
}

void clientWorldUpdate(int teulIndex, struct WORLD_UPDATE*);
void clientApplyStratEventData(int teulIndex, const Strat::EventData&);

// mission
struct MISSION_POST_REPLY;
struct MISS_LAUNCH_UPDATE;
struct MISSION_RESULTS;
class MissionDetail;
class RoomList;
class FacilityWeaponList;

void clientMissionPostReply(int teulIndex, struct MISSION_POST_REPLY*);
void clientMissionRoomListReply(int teulIndex, RoomList*);
void clientMissionLaunchUpdate(int teulIndex, struct MISS_LAUNCH_UPDATE*);
void clientMissionResultsReply(int teulIndex, struct MISSION_RESULTS*);
void clientProcessWeapList(int teulIndex, struct ACTION_OBJECT*);
void clientMissionDetailReply(int teulIndex, MissionDetail&);

// mm
struct FE_POST_ACCEPT;
struct FE_POST_DENY;
struct FE_POST_EXPIRED;
struct FE_POST_LIST_RESPONSE;
struct FE_JOIN_DENY;
struct FE_JOIN_PERM_REQUEST;
struct FE_JOIN_ACCEPT;
class FESpawnNotify;
struct FE_CREW_BAILED;
struct ChatServerInfo;

void clientProcessPostAccept(int teulIndex, struct FE_POST_ACCEPT*);
void clientProcessPostDeny(int teulIndex, struct FE_POST_DENY*);
void clientProcessPostExpired(int teulIndex, struct FE_POST_EXPIRED*);
void clientProcessPostList(int teulIndex, struct FE_POST_LIST_RESPONSE*);
void clientProcessJoinDeny(int teulIndex, struct FE_JOIN_DENY*);
void clientProcessJoinPerm(int teulIndex, struct FE_JOIN_PERM_REQUEST*);
void clientProcessJoinAccept(int teulIndex, struct FE_JOIN_ACCEPT*);
void clientProcessSpawnNotify(int teulIndex, const FESpawnNotify&);
void clientProcessCrewBailed(int teulIndex, struct FE_CREW_BAILED*);

// chat
#include "chat/chatPrototypes.h"		// Common chat forward protos
struct MOTD_REPLY;

void clientChatProcessMotdReply(int teulIndex, struct MOTD_REPLY const&);
void clientChatProcessHostMessage(int teulIndex, const Chat::HostMessage*);
void clientChatTuneToChannel(int teulIndex, const Chat::TuneToChannel*);
void clientChatReceiveNLA(int teulIndex, const Chat::UserNLA*);
void clientChatProcessLocalizedMessage(int teulIndex, const Chat::LocalizedMessage*);
void clientChatProcessGCIMessage(int teulIndex, const Chat::GCIMessage*);
void clientChatProcessServerInfo(int teulIndex, const ChatServerInfo*);
void clientProcessChatMissionStatus(int teulIndex, const Chat::MissionStatus*);

struct VEHICLE_UPDATE_HEADER;
struct VEHICLE_REPLY;
namespace Vehicle { struct Hit; }

void clientRemoveVehicle(int teulIndex, unsigned int index);
void clientProcessPositionUpdateMsg(int teulIndex, VEHICLE_UPDATE_HEADER* VehicleHeader);
void clientProcessVehicleRequestResponse(int teulIndex, VEHICLE_REPLY* Reply);
void clientProcessVehicleHit(int teulIndex, Vehicle::Hit&);

bool clientReceiveServerTrackedObject(int teulIndex, int type, ACTION_OBJECT* a);

void clientErrorLog(const char* const format, ...);
