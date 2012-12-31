#pragma once

struct FULL_OBJ_ID;
struct CP_OBJ_EVENT;

extern void clientForcedCellExit(int teulIndex, unsigned int exitCode);
extern void clientActivateCell(int teulIndex, unsigned int on, unsigned int hostIP, unsigned int port);
extern void clientActivateMap(int teulIndex, unsigned int on, unsigned int hostIP, unsigned int port);

// strat
namespace Strat
{
	class Event ;
	class EventPacket ;
	struct EventData ;
}

extern void clientWorldUpdate(int teulIndex, struct WORLD_UPDATE*);
extern void clientApplyStratEventData(int teulIndex, const Strat::EventData&);

// mission
struct MISSION_POST_REPLY;
struct MISS_LAUNCH_UPDATE;
struct MISSION_RESULTS;
class MissionDetail;
class RoomList;
class FacilityWeaponList;

extern void clientMissionPostReply(int teulIndex, struct MISSION_POST_REPLY*);
extern void clientMissionRoomListReply(int teulIndex, RoomList*);
extern void clientMissionLaunchUpdate(int teulIndex, struct MISS_LAUNCH_UPDATE*);
extern void clientMissionResultsReply(int teulIndex, struct MISSION_RESULTS*);
extern void clientProcessWeapList(int teulIndex, struct ACTION_OBJECT*);
extern void clientMissionDetailReply(int teulIndex, MissionDetail&);

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

extern void clientProcessPostAccept(int teulIndex, struct FE_POST_ACCEPT*);
extern void clientProcessPostDeny(int teulIndex, struct FE_POST_DENY*);
extern void clientProcessPostExpired(int teulIndex, struct FE_POST_EXPIRED*);
extern void clientProcessPostList(int teulIndex, struct FE_POST_LIST_RESPONSE*);
extern void clientProcessJoinDeny(int teulIndex, struct FE_JOIN_DENY*);
extern void clientProcessJoinPerm(int teulIndex, struct FE_JOIN_PERM_REQUEST*);
extern void clientProcessJoinAccept(int teulIndex, struct FE_JOIN_ACCEPT*);
extern void clientProcessSpawnNotify(int teulIndex, const FESpawnNotify&);
extern void clientProcessCrewBailed(int teulIndex, struct FE_CREW_BAILED*);

// ww2
extern void clientProcessDialog(int teulIndex, int);

// chat
#include "chat/chatPrototypes.h"		// Common chat forward protos
struct MOTD_REPLY;

extern void clientChatProcessMotdReply(int teulIndex, struct MOTD_REPLY const&);
extern void clientChatProcessHostMessage(int teulIndex, const Chat::HostMessage*);
extern void clientChatTuneToChannel(int teulIndex, const Chat::TuneToChannel*);
extern void clientChatReceiveNLA(int teulIndex, const Chat::UserNLA*);
extern void clientChatProcessLocalizedMessage(int teulIndex, const Chat::LocalizedMessage*);
extern void clientChatProcessGCIMessage(int teulIndex, const Chat::GCIMessage*);
extern void clientChatProcessServerInfo(int teulIndex, const ChatServerInfo*);
extern void clientProcessChatMissionStatus(int teulIndex, const Chat::MissionStatus*);

struct VEHICLE_UPDATE_HEADER;
struct VEHICLE_REPLY;
namespace Vehicle { struct Hit ; }

extern void clientRemoveVehicle(int teulIndex, unsigned int index);
extern void clientProcessPositionUpdateMsg(int teulIndex, VEHICLE_UPDATE_HEADER* VehicleHeader);
extern void clientProcessVehicleRequestResponse(int teulIndex, VEHICLE_REPLY* Reply);
extern void clientProcessVehicleHit(int teulIndex, Vehicle::Hit&);

extern bool clientReceiveServerTrackedObject(int teulIndex, int type, ACTION_OBJECT* a);

extern void clientErrorLog(const char* const format, ...) ;
