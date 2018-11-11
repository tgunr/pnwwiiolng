#include "syscfg.h" // Path is project specific.
#include "phui/MessageLoop.h"
#include "phui/Message.h"

#include "client/GameClient.h"
#include "client/ww2HostState.h"
#include "comm/commClientMessages.h"
#include "ml/mltypes.h"
#include "mm/mmSpawnTypes.h"
#include "veh/vehtype.h"
#include "ui/uiMain.h"
#include "include/vars.h"
#include "veh/vehmain.h"
#include "fp/fpFPS.h"
#include "pers/pershud.h"
#include "trn/trntod.h"
#include "trn/trnmain.h"
#include "ob/obdraw.h"
#include "sdm/StratDB.h"
#include "chat/chatClientMessages.h"
#include "ui/uiChat.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatHandle.h"
#include "locl/loclMain.h"
#include "locl/loclApp.h"
#include "locl/loclLang.h"
#include "js/jsClient.h"
#include "veh/vehcomms.h"
#include "client/clientNetError.h"
#include "chat/chatWho.h"
#include "rn/rnlogo.h"
#include "uimap/WaypointLayer.h"
#include "uimap/PlayerLayer.h"
#include "chat/chatWaypointExchg.h"
#include "wwii/wwiiSquadMsg.h"
#include "chat/chatSquadControl.h"
#include "client/client.h"
#include "client/clientConnections.h"
#include "ui/uiDotCommands.h"
#include "veh/vehupdate.h"
#include "veh/vehcomms.h"
#include "client/clientServerTrackedObjects.h"
#include "trn/trncloud.h"
#include "trn/trnsky.h"
#include "ui/uiMain.h"
#include "uimap/uiMap.h"
#include "ui/uiGameEvents.h"
#include "math/mathvect.h"
#include "pers/persmain.h"
#include "inst/instListing.h"
#include "tut/TutorialManager.h"
#include "fx/fxWeather.h"
#include "wwii/wwiiUTF.h"
#include "fp/fpPPO.h"
#include "host/hostTimeUpdate.h"
#include "cfg/cfgPrefs.h"
#include "scen/ScenarioManager.h"

// WWIIOL specific.
#include "client/clientStrtQueue.h"
#include "uimap/CPRenderer.h"
#include "strt/strtClientMessages.h"
#include "chat/chatMissionInvite.h"
#include "chat/chatMissionMember.h"
#include "chat/chatBrigadeControl.h"
#include "client/clientBrigadeControlDescriptors.h"
#include "map/mapHCUnitLocations.h"
#include "omg/omgChat.hxx"
#include "strt/strtEventTypes.h"
#include "strt/strtCPFlags.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "wwii/wwiiThemes.h"
#include "spt/sptmain.h"
#include "wwii/wwiiClientMessages.h"

void clientStratControlUpdate(FULL_OBJ_ID const& id, int ownership, UINT32 flags);

using namespace ww2;

static GameClient* _playerclient;

extern PPOSpecLoader specLoader;
extern SINT32 globalMSCoolDownEnd;

/************************************************************************************************/

bool GameClientInitTeul(void) {
	return Client::InitTeul();
}

/************************************************************************************************
	GameClientLogin
	throws ClientNetError
*************************************************************************************************/

int GameClientLogin(unsigned int host, playerid_t playerID, UINT32 otp,
							unsigned int appVersion, UINT16 port)
{
	Client::Init(playerID, otp, appVersion);

	try
	{
		if(PlayerClient()->Login(host, port, true) != 0)
			throw ClientNetError("connect.login.badlogin");

		if(PlayerClient()->ChatConnection() < 0)
			throw ClientNetError(FB_CONN_GONE, "connect.login.nochat");
	}
	catch(ClientNetError& e)
	{
		Write_Log(LOG_COMM, LOG_WARN, "Client::Login generated an error: %d, %s", e.Error(), e.What());
		throw;
	}

	Write_Log(LOG_COMM, LOG_INFO, "Connected to host");

	return _playerclient->MapConnection();
}

void GameClientKill()
{
    if(_playerclient)
    {
        delete _playerclient;
        _playerclient = nullptr;
    }
}

/************************************************************************************************
	PlayerClient
*************************************************************************************************/

GameClient* PlayerClient()
{
	if(!_playerclient) _playerclient = new GameClient;
	return _playerclient;
}

GameClient::GameClient() :
	jsclient(0),
	spawnStatus(SPAWN_DESPAWNED),
	_handlers(),
    mobilespawnyaw(0)
{
}

/************************************************************************************************
	ProcessSpawnNotify
*************************************************************************************************/

void GameClient::ProcessSpawnNotify(FESpawnNotify const& notify)
{
	Client::ProcessSpawnNotify(notify);
	jsclient->RunScript("onreserveweapon()");

	spawnStatus = SPAWN_RECEIVED;

	if(gCellClientActive)
	{
		clientErrorLog("rvj");
		spawnStatus = SPAWN_VEHICLE;
	}
	else
	{
		clientErrorLog("! no cell for rvj");
	}
}

/************************************************************************************************
	RequestClientSpawn
*************************************************************************************************/

int GameClient::RequestClientSpawn(WEAPON_SERIAL const& serial, bool wantMobileSpawn)
{
	spawnStatus = SPAWN_PENDING;
	mobilespawnyaw = 0;

	return Client::RequestClientSpawn(serial, wantMobileSpawn);
}

int GameClient::RequestDirectSpawn(WEAPON_SPEC const& spec, const FLOAT64VECTOR* at, FLOAT64 yaw)
{
	spawnStatus = SPAWN_PENDING;
	mobilespawnyaw = yaw;

	return Client::RequestDirectSpawn(spec, at, yaw);
}

/************************************************************************************************
	DoVehicleJoin
*************************************************************************************************/

void GameClient::DoVehicleJoin()
{
	FLOAT64VECTOR const& spawnoverride = GetSpawnOverride();
	FESpawnNotify const& spawnnotify = SpawnNotify();
	WEAPON_SERIAL const& weaponSerial = spawnnotify.weaponSerial;

	vehType const* vehtype = vehTypeGetBySpec(weaponSerial.spec);
	if(!vehtype)
	{
		clientErrorLog("! dvj->vtgbs");
		return;
	}

	if(false && (spawnoverride.x || spawnoverride.y || spawnoverride.z))
		mlSetMobileSpawnStuff(0, spawnoverride.x, spawnoverride.y, spawnoverride.z, mobilespawnyaw);
	else if(spawnnotify.spawnVehicleID > 0)
		mlSetMobileSpawnStuff(spawnnotify.spawnVehicleID, spawnnotify.x, spawnnotify.y, spawnnotify.z, mobilespawnyaw);
	else
		mlSetSpawnStuff(weaponSerial.source.cell.ox >> 4, weaponSerial.source.cell.oy >> 4, weaponSerial.source.id, 1);

	// reset override after each spawn
	SetSpawnOverride(0, 0, 0);

	vehcommSendVehicleRequest(vehtype->VehicleTypeNum, &weaponSerial);

	SpawnComplete();
	//spawnStatus = SPAWN_SPAWNED;
}

/************************************************************************************************
	ProcessPostAccept
*************************************************************************************************/
void GameClient::ProcessPostAccept(FE_POST_ACCEPT const& postAccept)
{
	Client::ProcessPostAccept(postAccept);
	jsclient->RunScript("onreserveweapon()");
}

/************************************************************************************************
	ProcessCrewJoinAccept
*************************************************************************************************/

void GameClient::ProcessCrewJoinAccept(FE_JOIN_ACCEPT const& joinaccept)
{
	Client::ProcessCrewJoinAccept(joinaccept);
	jsclient->RunScript("onreserveweapon()");
	jsclient->RunScript("oncrewjoinaccept()");
}

/************************************************************************************************
	ProcessCrewJoinDeny
*************************************************************************************************/

void GameClient::ProcessCrewJoinDeny(FE_JOIN_DENY const& joindeny)
{
	Client::ProcessCrewJoinDeny(joindeny);
	jsclient->RunScript("oncrewjoindeny()");
}

/************************************************************************************************
	ProcessCrewBailed
*************************************************************************************************/

void GameClient::ProcessCrewBailed(FE_CREW_BAILED const& crewbailed)
{
	Client::ProcessCrewBailed(crewbailed);
	jsclient->RunScript("oncrewbailed()");
}

/************************************************************************************************
	ProcessCrewPostExpired
*************************************************************************************************/

void GameClient::ProcessCrewPostExpired(FE_POST_EXPIRED const& crewexpired)
{
	Client::ProcessCrewPostExpired(crewexpired);
	jsclient->RunScript("oncrewpostexpired()");
}

/************************************************************************************************
	ProcessForcedCellExit
*************************************************************************************************/

void GameClient::ProcessForcedCellExit(unsigned int exitCode)
{
	Client::ProcessForcedCellExit(exitCode);
	vehExitVehicle(exitCode);
}

/************************************************************************************************
	ProcessGameTimeUpdate
*************************************************************************************************/

void GameClient::ProcessGameTimeUpdate(Host::TimeUpdate const& update)
{
	Client::ProcessGameTimeUpdate(update);

	// scenarios have custom time of day and weather
	if(!SCEN_MGR->IsActive())
	{
		checkThemeChange(static_cast<UINT32>(update.theme));

		trnTODSetTimeOfDay(update.gametime);
		trnTODSetTimeMultiplier((FLOAT32)update.factor);

		// Set lighting and cloud settings
		// One based to know if valid.
		const SINT32 weather = (SINT32)(update.weather - 1);
		const SINT32 seedindex = (SINT32)(update.rain - 1);

		if(weather >= 0 && seedindex >= 0)
		{
			fxWeatherSetState(weather, seedindex);
		}
	}

	trnmainSetLightingUpdate(TRUE);
	obdrawUpdateTerrainObjectLighting(TRUE);
}

/************************************************************************************************
	ProcessWorldUpdate
*************************************************************************************************/

void GameClient::ProcessWorldUpdate(WORLD_UPDATE const& wu)
{
	if(!Online) return;

	Client::ProcessWorldUpdate(wu);

	persHudReceivedUpdate();
}

/************************************************************************************************
	ProcessChatHostMessage
*************************************************************************************************/

void GameClient::ProcessChatHostMessage(Chat::HostMessage const& message)
{
	Client::ProcessChatHostMessage(message);
	uiChatHostMessage(message);
}

/************************************************************************************************
	ProcessChatTuneToChannel
*************************************************************************************************/

void GameClient::ProcessChatTuneToChannel(Chat::TuneToChannel const& tuneToChannel)
{
	Client::ProcessChatTuneToChannel(tuneToChannel);
	uiChatTuneToChannel(tuneToChannel);
}

/************************************************************************************************
	ProcessChatNLA
*************************************************************************************************/

void GameClient::ProcessChatNLA(Chat::UserNLA const& nla)
{
	Client::ProcessChatNLA(nla);
	uiChatDetuneChannel(nla);
}

/************************************************************************************************
	ProcessChatLocalizedMessage
*************************************************************************************************/

void GameClient::ProcessChatLocalizedMessage(Chat::LocalizedMessage const& msg)
{
	Client::ProcessChatLocalizedMessage(msg);

	// Allocate room on the stack for the UTF16 conversion of all the strings.
	// NOTE: Using 'alloca' allocates on the stack, so no free is required.
	wchar_t strings[Chat::LocalizedMessage::_NUM_ARGUMENTS][MAX_MESSAGE_LENGTH + 1];
	for ( size_t i = 0; i < Chat::LocalizedMessage::_NUM_ARGUMENTS; ++i )
	{
		WWII::ConvertChars(msg.m_ptr[i], strings[i], (MAX_MESSAGE_LENGTH + 1) * sizeof(wchar_t));
	}

	if(msg.m_prefix == LSTR_ORDERS)
	{
		switch(msg.m_message)
		{
			case LSTR_NEW_LEADER:
			{
				vehVehicle* vehicle = nullptr;

				vehicle = vehFindVehicleByChatHandle(SelectedMission().m_poster);
				if(vehicle)
				{
					vehicle->IsMissionLeader = 0;
					vehicle->IsMyMissionLeader = 0;
				}

				ChatHandle newLeader = chatGetHandle(strings[0]);
				vehicle = vehFindVehicleByChatHandle(newLeader);
				if(vehicle)
				{
					vehicle->IsMissionLeader = 1;

					if(vehicle->SameMission)
					{
						vehicle->IsMyMissionLeader = 1;

						if(cfgPrefsGetBool("app.missionwaypoints", true))
							HUD_MGR->AddObject(new PersHUDManager::HUDLeaderWaypoint(vehicle));
					}
				}

				SelectedMission().m_poster = newLeader;

				if(Personas().m_callsign == newLeader)
				{
					SelectedMission().m_amLeader = true;
				}
				else
				{
					SelectedMission().m_amLeader = false;
					fpPPOKill();
				}

				//SetCurrentOrders(orders);

				jsclient->ProcessMissionLeaderUpdate(msg);
			}
			break;
			case LSTR_OBJECT_DESTROYED:
			{
				std::wstring format = uiLocString("mission.orders.objectdestroyed");
				wchar_t orders[255+1];
				swprintf(orders, 255, format.c_str(), strings[0]);
				SetCurrentOrders(orders);
			}
			break;
			case LSTR_BASE_CAPTURED:
			{
				std::wstring format = uiLocString("mission.orders.basecaptured");
				wchar_t orders[255+1];
				swprintf(orders, 255, format.c_str(), strings[0]);
				SetCurrentOrders(orders);
				jsclient->RunScript("onbasecaptured()");
			}
			break;
			case LSTR_GENERIC:
			{
				// custom orders
				SetCurrentOrders(strings[0]);
			}
			break;
            default:
                // we don't care about other types
                break;
		}
	}

	if(SCEN_MGR->IsActive() && msg.m_prefix == LSTR_SYSTEM && msg.m_message == LSTR_GENERIC)
	//if(msg.m_prefix == LSTR_SYSTEM && msg.m_message == LSTR_GENERIC)
	{
		// send this to our message box
		phui::View* hud = uiFindView("hud");
		if(hud)
		{
			static const wchar_t accident = L'>';	// As in, XXX was killed by < accident? >
			static const wchar_t period = L'.';
			static const wchar_t lparenth = L'(';	// As in, (xx hits)


			const wchar_t* str = UTF16(msg.m_data);
			const wchar_t* ptr = wcsstr(str, L" ");

			wchar_t		ptr2[128];			// Copy of ptr for tokenization
			wchar_t*	ptrtok = nullptr;			// For tokenization
			wchar_t*	killerName = nullptr;			// The name of the killer
			int			isFriendly = 0;				// If this is a kill, is the killer on my side? -1 = enemy, 0 = neutral, 1 = friendly
			std::vector<std::wstring> tokens;		// By pushing tokens back, the top entry should be killer's name
			vehVehicle* killer = nullptr;
			vehVehicle* me = persGetPersonaVehiclePtr();

			wchar_t* temp;

			// skip past the first space
			ptr = (ptr) ? ptr + 1 : str;
			wcscpy(ptr2, ptr);

			// Determine whether the killer was friendly or not
			ptrtok = wcstok(ptr2, L" ",&temp);

			// Tokenize, and the last token should be the
			while(ptrtok != nullptr)
			{
				// Stop tokenizing if we run into "(x hits)"
				if(ptrtok[0] == lparenth)
					break;

				tokens.push_back(ptrtok);
				ptrtok = wcstok(nullptr, L" ",&temp);
			}

			killerName = const_cast<wchar_t*>(tokens.back().c_str());   // <----- killerName is not a const, but c_str() is const


			// Strip the period from the end
			if(killerName[wcslen(killerName)-1] == period)
				killerName[wcslen(killerName)-1] = 0;   // <----- bad idea, you are overwriting a std::wstring behind it's back

			killer = vehFindVehicleByChatHandle(chatGetHandle(killerName));

			if(killer)
			{
				WWII_SIDE killerside = wwiiSide(killer->VehicleType->WeaponSpec.country());
				WWII_SIDE myside = wwiiSide(me->VehicleType->WeaponSpec.country());

				if(me == nullptr || (*killerName) == accident)
					isFriendly = 0;		// Neutral case
				else if(myside == killerside)
					isFriendly = 1;		// Killer is my ally
				else if(myside != killerside)
					isFriendly = -1;	// Killer is my enemy
			}

			char script[255+1];
			snprintf(script, 255, "OnKill(\"%ls\", %d)", ptr, isFriendly);

			// run our message script
			uiRunScript(hud, script);
		}
	}

	uiChatPrintLineMessageLoc(IDE_LANG_HOST, msg.m_prefix, msg.m_message, strings[0], strings[1], strings[2], strings[3], strings[4], strings[5]);


	if(msg.m_prefix == LSTR_NOTE && msg.m_message == LSTR_ORDERS_CLEARED)
	{
		SetCurrentOrders(strings[0]);
		jsclient->RunScript("game.onorders(false)");
	}
	else if(msg.m_prefix == LSTR_ORDERS)
	{
		jsclient->RunScript("game.onorders()");
	}
}

/************************************************************************************************
	ProcessChatGCIMessage
*************************************************************************************************/

void GameClient::ProcessChatGCIMessage(Chat::GCIMessage const& pgciMsg)
{
	Client::ProcessChatGCIMessage(pgciMsg);
	uiChatGCIMessage(pgciMsg);
}

/************************************************************************************************
	ProcessMissionResults
*************************************************************************************************/

void GameClient::ProcessMissionResults(MISSION_RESULTS const& results)
{
	//uiChatPrintLine("Prcoessing Mission Results!");
	clientErrorLog("pmr");
	Client::ProcessMissionResults(results);
}

/************************************************************************************************
	ProcessMissionResults
*************************************************************************************************/

void GameClient::ProcessMissionRoomListReply(RoomList const& missroomlist)
{
	Client::ProcessMissionRoomListReply(missroomlist);
	jsclient->RunScript("onmissionroomchange()");
}

/************************************************************************************************
	ProcessPostList
*************************************************************************************************/

void GameClient::ProcessPostList(FE_POST_LIST_RESPONSE  const& response)
{
	Client::ProcessPostList(response);
	jsclient->RunScript("onmissionroomchange()");
}

/************************************************************************************************
	ProcessPostDeny
*************************************************************************************************/

void GameClient::ProcessPostDeny(FE_POST_DENY const& postDeny)
{
	Client::ProcessPostDeny(postDeny);

	static char callback[128];
	strprintf(callback, "onspawnerror(%u)", postDeny.reasonCode);
	jsclient->RunScript(callback);
}

/************************************************************************************************
	ProcessCrewJoinPerm
*************************************************************************************************/

void GameClient::ProcessCrewJoinPerm(FE_JOIN_PERM_REQUEST const& joinperm)
{
	Client::ProcessCrewJoinPerm(joinperm);
	jsclient->RunScript("onmissionroomchange()");
}

/************************************************************************************************
	ProcessPersonaSummaries
*************************************************************************************************/

void GameClient::ProcessPersonaSummaries(PersonaSummaries const& summaries)
{
	Client::ProcessPersonaSummaries(summaries);
	jsclient->RunScript("onpersonas()");
}

/************************************************************************************************
	SelectPersona
*************************************************************************************************/

bool GameClient::SelectPersona(const PersonaSummary& selected, bool save, bool force)
{
	// try to select it
	bool ok = Client::SelectPersona(selected, save, force);

	// update the jsclient
	jsclient->UpdatePersonas();

	// return what happened
	return ok;
}

/************************************************************************************************
	SelectBrigade
*************************************************************************************************/

bool GameClient::SelectBrigade(UINT32 brigadeID, bool save, bool force)
{
	// try to select it
	bool ok = Client::SelectBrigade(brigadeID, save, force);

	// update the jsclient
	jsclient->UpdatePersonas();

	// return what happened
	return ok;
}

/************************************************************************************************
	ProcessMissionList
*************************************************************************************************/

void GameClient::ProcessMissionList(MissionList const& missionList)
{
	Client::ProcessMissionList(missionList);
	if(missionList.m_listEnd)
		jsclient->RunScript("onmissionlistchange()");
}

/************************************************************************************************
	ProcessWeapList
*************************************************************************************************/

void GameClient::ProcessWeapList(struct ACTION_OBJECT* a)
{
	Client::ProcessWeapList(a);
	jsclient->ProcessWeapList();
}

/************************************************************************************************
	ProcessMissionLaunchUpdate
*************************************************************************************************/

void GameClient::ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const& lu)
{
	Client::ProcessMissionLaunchUpdate(lu);
	jsclient->ProcessMissionLaunchUpdate(lu);
}

/************************************************************************************************
	ProcessPostList
*************************************************************************************************/

void GameClient::ProcessMissionPostReply(MISSION_POST_REPLY const& reply)
{
	Client::ProcessMissionPostReply(reply);

	static char buffer[128];
	snprintf(buffer, 127, "onmissionpostreply(%d,%d,%d)", reply.missionID, reply.posted, reply.reasonCode);

	jsclient->RunScript(buffer);
}

/************************************************************************************************
	ActivateCell
*************************************************************************************************/

void GameClient::ActivateCell()
{
	Client::ActivateCell();

	if(!gCellClientActive)
	{
		gCellClientActive = TRUE;

		if(spawnStatus == SPAWN_RECEIVED)
			spawnStatus = SPAWN_VEHICLE;
	}
	else
	{
		clientErrorLog("! cell active");
	}
}

/************************************************************************************************
	ActivateMap
*************************************************************************************************/

void GameClient::ActivateMap()
{
	Client::ActivateMap();

	if(Online && gCellClientActive)
	{
		gCellClientActive = FALSE;

		if(spawnStatus == SPAWN_PENDING)
		{
			jsclient->RunScript("onspawnerror()");
		}
	}

	spawnStatus = SPAWN_DESPAWNED;
}

/************************************************************************************************
	SetChildToCPOwner
*************************************************************************************************/

void GameClient::SetChildToCPOwner(StratDB::StratObject* parent, phui::Message& msg)
{
	AddStratObjectToCPUpdate(msg, parent);

	StratDB::StratObject::ChildList children = parent->ChildStratObjects();
	StratDB::StratObject::ChildList::iterator it = children.begin();
	for(; it != children.end(); ++it)
	{
		StratDB::StratObject* stratobj = *it;
		stratobj->SetCountry(parent->Country());
		stratobj->SetUpdateTime(frametime);
		if(	stratobj->StratClass() == SDM_CLASS_FACILITY &&
			stratobj->StratType() == SDM_FACILITY_FIREBASE )
		{
			stratobj->SetStratFlags(stratobj->StratFlags() & ~CP_STAT_OPEN);
		}
		AddStratObjectToCPUpdate(msg, stratobj);
	}
}

/************************************************************************************************
	ProcessCPUpdateList
*************************************************************************************************/

/*
void GameClient::ProcessCPUpdateList(CPUpdateList const& list)
{
	Client::ProcessCPUpdateList(list);

	FULL_OBJ_ID selectedCP = chatGetTargetSelection();

	phui::Message msg(CPUPDATE_MSG);

	UINT32 i;
	for(i = 0; i < list.m_updates && i < CPUpdateList::_MAX_ENTRIES; ++i)
	{
		// this update's FBs that aren't on the front or contended
		if((list.m_entries[i].m_flags & (CP_STAT_ENEMY_LINKED|CP_STAT_CONTENTION)) == 0)
		{
			StratDB::StratObject* stratobj = StratDB::FindStratObject(list.m_entries[i].m_fullObjID);
			if(stratobj)
				SetChildToCPOwner(stratobj, msg);
		}

		// this updates the CP's ownership and flags
		clientStratControlUpdate(list.m_entries[i].m_fullObjID,
							list.m_entries[i].m_controller,
							list.m_entries[i].m_flags);

		// make sure this CP get's updated
		AddStratObjectToCPUpdate(msg, list.m_entries[i].m_fullObjID);
	}

	PostCPUpdate(msg);
}
*/

/************************************************************************************************
	ProcessCPFacilitiesUpdate
*************************************************************************************************/

void GameClient::AddStratObjectToCPUpdate(phui::Message& msg, FULL_OBJ_ID const& fullid)
{
	AddStratObjectToCPUpdate(msg, StratDB::FindStratObject(fullid));
}

void GameClient::AddStratObjectToCPUpdate(phui::Message& msg, StratDB::StratObject const* stratobj)
{
	if(_handlers.empty())  return;
	if(!stratobj) return;

	msg.AddLong(CPUPDATE_SX, stratobj->SupercellX());
	msg.AddLong(CPUPDATE_SY, stratobj->SupercellY());
	msg.AddLong(CPUPDATE_ID, stratobj->FullObjID()->id);
	msg.AddLong(CPUPDATE_OWNER, stratobj->Country());
	msg.AddLong(CPUPDATE_FLAGS, stratobj->StratFlags());
}

void GameClient::PostCPUpdate(phui::Message& msg)
{
	std::set<phui::MessageHandler*>::iterator it = _handlers.begin();
	for(; it != _handlers.end(); ++it)
		uiMessageLoop()->PostMessage(&msg, *it);
}

static void _applyEventToStratDB(const Strat::Event& entry, StratDB::StratObject* const stratobj)
{
	stratobj->SetCountry(entry.ownership());
	stratobj->SetStratFlags(entry.flags());
	stratobj->SetUpdateTime(frametime);
}

static void _setBuildingOwnershipsToMatchFacility(const StratDB::StratObject* const facility, Strat::EventData& data)
{
	const StratDB::StratObject::ChildList& children = facility->ChildStratObjects();
	StratDB::StratObject::ChildList::const_iterator it = children.begin();
	StratDB::StratObject::ChildList::const_iterator last = children.end();

	// create a new strat event for each building's ownership
	for(; it != last; ++it)
	{
		const StratDB::StratObject* building = *it;
		if(building->Country() != facility->Country())
		{
			data.fid	= *(building->FullObjID());
			data.owner	= facility->Country();
			data.level	= building->VisLevel();
			clientApplyStratEventData(data);
		}
	}
}

static void _setFacilityOwnershipsToMatchCP(const StratDB::StratObject* const cp, Strat::EventData& data)
{
	const StratDB::StratObject::ChildList& children = cp->ChildStratObjects();
	StratDB::StratObject::ChildList::const_iterator it = children.begin();
	StratDB::StratObject::ChildList::const_iterator last = children.end();

	for(; it != last; ++it)
	{
		StratDB::StratObject* const facility = *it;
		if(facility->Country() != cp->Country())
		{
			facility->SetCountry(cp->Country());
			facility->SetUpdateTime(frametime);

			data.fid	= *(facility->FullObjID());
			data.owner	= cp->Country();
			data.level	= -1;
			clientApplyStratEventData(data);

			_setBuildingOwnershipsToMatchFacility(facility, data);
		}
	}
}

extern UINT32 trnUpdateObjectStatus(SINT32 x, SINT32 y, UINT32 id, SINT32 country, SINT32 vislevel);
void GameClient::ProcessStratEventPacket(const Strat::EventPacket& packet)
{
	phui::Message msg(CPUPDATE_MSG);
	Strat::EventData data; // temp
	Strat::EventPacket extra;

	SINT32 activeMaps = SuperCellMapView::Instances();

	const size_t numEvents = packet.size();

	for(size_t i = 0; i < numEvents; ++i)
	{
		const Strat::Event& entry = packet[i];

		StratDB::StratObject* const stratobj = StratDB::FindStratObject(entry.fid());
		if(!stratobj) 
		{
			Write_Debug("Strat object has no match on client for %d.%d.%u", PRA_FULL_OBJ_ID(entry.fid()));
			continue; // We can't go trashing the whole packet because of one bad ID
		}

		switch(stratobj->StratClass())
		{
			case SDM_CLASS_CP:
			{
				_applyEventToStratDB(entry, stratobj);

				if(activeMaps > 0)
					AddStratObjectToCPUpdate(msg, stratobj);

				// Bridges are CPs with special handling: their ownership
				// determines whether they are destroyed or not.
				///TODO:TROY: OGS: Don't know that this is the right way to do this, please check.
				if(stratobj->StratType() == SDM_CP_BRIDGE)
				{
					trnUpdateObjectStatus(stratobj->SupercellX(), stratobj->SupercellY(), stratobj->Id(), entry.ownership(), -1);
				}
				else if (stratobj->StratClass() == SDM_CLASS_CP && (entry.flags() & CP_STAT_CONTENTION) == 0)
				{
					_setFacilityOwnershipsToMatchCP(stratobj, data);
				}
			}
			break;
			case SDM_CLASS_FACILITY:
			{
				_applyEventToStratDB(entry, stratobj);

				if(activeMaps > 0)
					AddStratObjectToCPUpdate(msg, stratobj);

				_setBuildingOwnershipsToMatchFacility(stratobj, data);
			}
			break;
			case SDM_CLASS_BUILDING:
			{
				// building events always have vis levels in them
				stratobj->SetVisLevel(entry.flags());

				// if we found a fac, use its owner as our building's owner
				StratDB::StratObject* fac = stratobj->ParentStratObject();

				// set our data
				data.fid	= entry.fid();
				data.owner	= (fac) ? fac->Country() : stratobj->Country();
				data.level	= stratobj->VisLevel();

				clientApplyStratEventData(data);
			}
			break;
			default:
			break;
		}
	}

	if(activeMaps > 0)
		PostCPUpdate(msg);
}

/************************************************************************************************
	ProcessWaypoint
*************************************************************************************************/

void GameClient::ProcessWaypoint(Waypoint const& waypoint, int type)
{
	if(type == WPXT_REQUEST && !waypoint.handle() && IsMissionLeader())
	{
		uiChatPrintUILocLine(nullptr, colorGREENALPHA, "mission.contact", chatGetWideCallsign(waypoint.playerHandle()));
		jsclient->RunScript("game.oncontact()");
	}

	// what to do with the waypoint?
	switch(waypoint.type())
	{
		case WPT_AMMO:
		{
			// range the player must be from the requestor before they get a game event
			float validRange = 0;

			// temp variables
			vehVehicle* player = persGetPersonaVehiclePtr();
			vehVehicle* requestor = nullptr;

			// lookup the ammo requestor
			requestor = vehFindVehicleByChatHandle(waypoint.playerHandle());

			if(player && player->VehicleType && requestor && requestor->VehicleType && player != requestor)
			{
				UINT32 requestorType		= requestor->VehicleType->Type;
				const WEAPON_SPEC& playerSpec = player->VehicleType->WeaponSpec;
				bool iamTruck				= (playerSpec.wCategory == 2 && playerSpec.wClass == 6);
				bool iamInfResupplier		= (playerSpec.wCategory == 4 && playerSpec.wClass == 9 && playerSpec.wType == 1);

				// enforce gamerules
				if((requestorType == VEH_TYPE_GROUND || requestorType == VEH_TYPE_PERSON) && iamTruck)
					validRange = 1000; // meters
				else if(requestorType == VEH_TYPE_PERSON && iamInfResupplier)
					validRange = 200; // meters

				// now we need to check if they are in range
				if(validRange)
				{
					FLOAT64VECTOR diff = {0,0,0};
					mathSubVectors(&diff, &player->ModelData->VehiclePos, &requestor->PosData.Pos);

					if(mathVectDotProduct(&diff, &diff) <= (validRange*validRange))
					{
						vehIconSetPulsate(requestor, 30000);
						uiGameEvent(GAMEEVENT_NEED_AMMO, 1, chatGetWideCallsign(requestor->Chat_Handle));
					}
				}
			}

			uiMapProcessPlayerEventToAllMaps(waypoint.playerHandle(), MAP_PLAYER_EVENT_AMMO, frametime + 30000);
		}
		break;
		default:
			Client::ProcessWaypoint(waypoint, type);
		break;
	}
}

/************************************************************************************************
	ProcessWaypointControl
*************************************************************************************************/

void GameClient::ProcessWaypointControl(WaypointControlMsg const& msg, int type, ChatHandle handle)
{
	Client::ProcessWaypointControl(msg, type, handle);

	switch (type)
	{
		case WPCTRL_APPROVE:
			uiChatPrintLineMessageLoc(IDE_LANG_UI, LSTR_NONE, 547);
		break;
		case WPCTRL_DENIED:
			uiChatPrintLineMessageLoc(IDE_LANG_UI, LSTR_NONE, 548);
		break;
		case WPCTRL_INVALID: // bad name
			uiChatPrintLineMessageLoc(IDE_LANG_UI, LSTR_NONE, 549);
		break;
		case WPCTRL_NO_MISSION: // can get this when the mission is closed
			uiChatPrintLineMessageLoc(IDE_LANG_UI, LSTR_NONE, 550);
		break;
	}
}

void GameClient::ProcessBrigadeControl(BrigadeControlList const& entry)
{
	Client::ProcessBrigadeControl(entry);
}

#define MAX_LOC_KEY_LENGTH 50

void GameClient::ProcessBrigadeControlNotice(const BrigadeControlNotice* const notice)
{
	Client::ProcessBrigadeControlNotice(notice);

	// kill the hccp if its up when you get removed and are not an HC
	if(notice->m_notice == BrigadeControl::_REMOVED && notice->m_officer == Personas().m_callsign  && !IsHC())
	{
		phui::View* hccp = uiViewport()->FindView("hccp");
		if(hccp)
			hccp->Close();
	}

	// send to the chat system
	unsigned int color = colorCYANALPHA;
	int icons[CHATICON_MAX] = {-1,-1,-1};

	if(notice->m_notice == BrigadeControl::_ERROR)
	{
		color = colorORANGEALPHA;
		icons[2] = IDE_ICON_CAUTION;
	}
	else if ( notice->m_notice == BrigadeControl::_VETOED )
	{
		color = colorORANGEALPHA;
		HCUnit* brigade = HCUnit::Lookup(notice->m_brigadeID);
		icons[1] = (brigade && brigade->side() == ALLIED) ? IDE_ICON_ALLIED: IDE_ICON_AXIS;
		icons[2] = IDE_ICON_CAUTION;
	}
	else if ( notice->m_notice == BrigadeControl::_PROPOSAL )
	{
		HCUnit* brigade = HCUnit::Lookup(notice->m_brigadeID);
		icons[1] = (brigade && brigade->side() == ALLIED) ? IDE_ICON_ALLIED: IDE_ICON_AXIS;
		icons[2] = IDE_ICON_HC;
	}
	else
	{
		color = colorWHITEALPHA;
		icons[0] = (notice->isTargettedAction()) ? IDE_ICON_GCI : IDE_ICON_BRIGADE;
		HCUnit* brigade = HCUnit::Lookup(notice->m_brigadeID);
		icons[1] = (brigade && brigade->side() == ALLIED) ? IDE_ICON_ALLIED: IDE_ICON_AXIS;
		icons[2] = IDE_ICON_HC;

		if(notice->m_notice == BrigadeControlNotice::_OFFERING &&
		   notice->m_action == BrigadeControlNotice::REQ_NOMINATE &&
		   notice->m_officer == Personas().m_callsign)
		{
			char script[40];
			snprintf(script, 40, "game.onoicnomination(%u, %u)", notice->m_brigadeID, notice->m_time);

			// run the callback
			uiRunScript(script);
		}
	}

	BrigadeControlDescriptor bcd = notice->getDescription();

	std::wstring text = uiChatAssembleUILoc(bcd.label.c_str(), bcd.arg1.c_str(), bcd.arg2.c_str(), bcd.arg3.c_str(), bcd.arg4.c_str());
	if ( bcd.major_broadcast )
		uiGameEvent(GAMEEVENT_ALERT, 1, text.c_str());
	uiChatPrintLineIcon(icons, color, L"%ls", text.c_str());
}

void GameClient::ProcessHCUnitLocation(const HCUnitLocation& unitloc)
{
	HCUnit* unit = HCUnit::Lookup(unitloc.m_unitID);

	if(unit)
	{
		// we send out update messages so the map updates itself
		FULL_OBJ_ID oldHome = unit->homeCPID();
		unit->setHomeCPID(unitloc.m_homeCPID);

		// create some update messages
		phui::Message oldHomeMsg(CPUPDATE_MSG);
		phui::Message newHomeMsg(CPUPDATE_MSG);

		// add the cps to be updated
		AddStratObjectToCPUpdate(oldHomeMsg, oldHome);
		AddStratObjectToCPUpdate(newHomeMsg, unitloc.m_homeCPID);

		// send out the message
		PostCPUpdate(oldHomeMsg);
		PostCPUpdate(newHomeMsg);
	}
}

/************************************************************************************************
	ProcessInstanceResult
*************************************************************************************************/

void GameClient::ProcessInstanceResult(const Instance::Result& result)
{
	if(result.status() == Instance::_SUCCESS)
		Client::ProcessInstanceResult(result);

	// update the jsclient so the UI knows
	jsclient->UpdateInstance(result.status());
}

/************************************************************************************************
	ProcessInstanceList
*************************************************************************************************/

void GameClient::ProcessInstanceList(const Instance::ClientListing* list)
{
	Client::ProcessInstanceList(list);

	// make sure the jsclient knows about this query
	jsclient->UpdateInstances();
}


/************************************************************************************************
	ProcessMissionStatus
*************************************************************************************************/

void GameClient::ProcessMissionStatus(const Chat::MissionStatus* ms)
{
	Client::ProcessMissionStatus(ms);

	SINT32 remaining = SINT32(ms->fru().cooldownRemaining());

	if(remaining > 0)
	{
		// now see if we are currently building an fru
		// interrupt if we are because there was a mission state change
        PPOSpec *spec = specLoader.ppoGetSpec(ms->fru().subtype.get());
		if(spec && spec->stotype == ServerTrackedObject::_UMS)
		{
            globalMSCoolDownEnd = frametime + remaining;

            PlayerPlacedObject* ppo = fpPPOGet();
			if(ppo && ppo->buildAt > 0)
				ppo->Interrupt();
		}
	}
}


/************************************************************************************************
	Update
*************************************************************************************************/

void GameClient::Update()
{
	Client::Update();

	bool cellConnected = CellState()->Connected();

	if(spawnStatus == SPAWN_VEHICLE && cellConnected)
		DoVehicleJoin();

	// check for a cell disconnect and bail if we found one
	if(uiInGame() && !cellConnected && CellState()->OnTransition()) // !CellState()->IsReconnecting() &&
		ProcessForcedCellExit(CellState()->lastError);
}

/************************************************************************************************
	RedirectComplete
*************************************************************************************************/

void GameClient::SpawnComplete()
{
	clientErrorLog("redired");
	Client::SpawnComplete();
	spawnStatus = SPAWN_SPAWNED;
}

void GameClient::SendPlayerExit(int exitcode, FULL_OBJ_ID const& fac)
{
	//uiChatPrintLineColor(colorORANGEALPHA, L"Sending client player exit!");
	clientErrorLog("spe");
	spawnStatus = SPAWN_DESPAWNED;
	Client::SendPlayerExit(exitcode, fac);
}

/************************************************************************************************
	RedirectFailed
*************************************************************************************************/

void GameClient::RedirectFailed()
{
	clientErrorLog("!redir");
	spawnStatus = SPAWN_DESPAWNED;
	//uiChatPrintLine("Despawn Failed");
}

void GameClient::OnDespawn()
{
	Client::OnDespawn();
	spawnStatus = SPAWN_DESPAWNED;
}

/************************************************************************************************
	ConnectionError
*************************************************************************************************/

extern LOGICAL piFlip();
extern void UpdateGameClocks();
extern void gmlpProcessInputs();

void GameClient::ConnectionError(ww2::HostState* state, UINT32 actions)
{
	if(!Online || !state || state->exiting)
		return;

	const char*	hostLoc		= state->HostName();
	const char* errorLoc	= state->ErrorName(state->lastError);
	SINT32 index			= state->LastIndex(); // get our last connected index

	// localized strings
	std::wstring hostName	= uiLocString(hostLoc);
	std::wstring errorMsg	= uiLocString(errorLoc);

	// log it if needed
	if(actions & HostState::LOG)
	{
		clientErrorLog("%s connection error (#%d): %d - %s",
			UTF8(hostName.c_str()),
			index,
			state->lastError,
			UTF8(errorMsg.c_str()));
	}

	// send it to chat if needed
	if(actions & HostState::CHAT)
	{
		uiChatPrintLineColor(colorYELLOWALPHA, L"%ls (%d): %ls",
							hostName.c_str(),
							state->lastError,
							errorMsg.c_str());
	}

	// a more severe error
	if(actions & (HostState::POPUP|HostState::EXIT))
	{
		char script[255+1];

		// if we have a fatal error, send our fatal callback along to the ui layer
		const char* uicall = (actions & HostState::EXIT) ? state->FatalCallback() : "";

		// create our js call
		snprintf(script, 255, "game.neterror(\"%s\", \"%s\", %d, \"%s\")",
			hostLoc,
			errorLoc,
			state->lastError,
			uicall);

		// call it
		jsclient->RunScript(script);

		// we're going to bail
		state->SetFatalError();

		/*
		do
		{
			//OGS: This is really awful, we should figure out a better way to do this :)
			UpdateGameClocks();
			uiUpdate();
			uiDraw();
			piFlip();
			gmlpProcessInputs();

#if platOS == OS_WIN32
			Sleep(1);
#else
			usleep(1000 * 1000);
#endif
		}
		while((actions & HostState::EXIT) && !state->exiting);
		*/
	}
}

void GameClient::ConnectionLost(const char* description, int errorNo)
{
	if(!Online) return;

	const std::wstring& wdesc = uiLocString(description);

	// Convert negative numbers into high-looking numbers
	if ( errorNo < 0 )
		errorNo = -errorNo;

	if(MapConnection() < 0)
	{
		uiChatPrintLineColor(colorYELLOWALPHA, uiLocString("connect.lost").c_str(), wdesc.c_str(), errorNo);

		// currently defined in lib/common.uixml
		if(_playerclient->jsclient)
			_playerclient->jsclient->RunScript("game.cthlerror()");
	}
	else
	{
		uiChatPrintLine(uiLocString("connect.lost.reconnect").c_str());
	}
}

/************************************************************************************************
	KickBanPopup
*************************************************************************************************/

void GameClient::KickBanPopup(ww2::HostState* state, UINT32 actions)
{
	if(!Online || !state || state->exiting)
		return;

	const char*	hostLoc		= state->HostName();
	const char* errorLoc	= state->ErrorName(state->lastError);
	SINT32 index			= state->LastIndex(); // get our last connected index

	// localized strings
	std::wstring hostName	= uiLocString(hostLoc);
	std::wstring errorMsg	= uiLocString(errorLoc);

	// log it if needed
	if(actions & HostState::LOG)
	{
		clientErrorLog("%s kickban (#%d): %d - %s",
			UTF8(hostName.c_str()),
			index,
			state->lastError,
			UTF8(errorMsg.c_str()));
	}

	// send it to chat if needed
	if(actions & HostState::CHAT)
	{
		uiChatPrintLineColor(colorYELLOWALPHA, L"%ls (%d): %ls",
							hostName.c_str(),
							state->lastError,
							errorMsg.c_str());
	}

	// a more severe error
	if(actions & (HostState::POPUP|HostState::EXIT))
	{
		char script[255+1];

		// if we have a fatal error, send our fatal callback along to the ui layer
		const char* uicall = (actions & HostState::EXIT) ? state->FatalCallback() : "";

		// create our js call
		snprintf(script, 255, "game.neterror(\"%s\", \"%s\", %d, \"%s\")",
			hostLoc,
			errorLoc,
			state->lastError,
			uicall);

		// call it
		jsclient->RunScript(script);

		// we're going to bail
		state->SetFatalError();

	}
}



/************************************************************************************************
	ProcessEmptyChatWhoList
*************************************************************************************************/

void GameClient::ProcessEmptyChatWhoList(int type)
{
	Client::ProcessEmptyChatWhoList(type);

	// Handle receiving a chat who list that is entirely empty
	switch ( type )
	{
	case CWT_SQUAD:
		// We're being told squad has disbanded or something?
	break;
	case CWT_TRAINERS:
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_CAUTION, LSTR_NO_TRAINERS);
	break;
	case CWT_WHO:
	default:
		uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_NONE, LSTR_NO_MATCHES);
	break;
	}
}

/************************************************************************************************
	ProcessChatWho
*************************************************************************************************/

void GameClient::ProcessChatWho(int type, const Chat::Who& who)
{
	Client::ProcessChatWho(type, who);

	// Not a displayable list
	if(type == CWT_SQUAD)
		return;

	// Valid data?
	if ( who.handle() == 0 )
		return;

	// Offline?
	persRank const* rank = nullptr;
	if ( who.online() && who.rank() > 0 )
		rank = persrankFind(who.rank());

	const wchar_t* callsign = chatGetWideCallsign(who.handle());

	if ( !who.online() || who.country() == WWII_NO_COUNTRY || who.rank() == 0 || rank == nullptr )
	{
		uiChatPrintLineMessageLoc(IDE_LANG_UI, LSTR_NONE, 534, callsign);
		return;
	}

	std::wstring name = callsign;

	if ( who.afk() )
	{
		name += L"(";
		name += uiLocString("chat.afk");
		name += L")";
	}
	else if ( who.leadingMission() )
	{
		name += L"(";
		name += uiLocString("chat.ml");
		name += L")";
	}
	else if ( who.highCommander() )
	{
		name += L"(";
		name += uiLocString("chat.hc");
		name += L")";
	}

	std::wstring description = L"";

	///FIXME
	static const int UI_BASE_COUNTRY_ADJECTIVE = 535;
	static const int UI_BASE_BRANCH_NAME = 544;

	// The country is 1 based, the strings are 0 based.
	description += rank->WideName(who.country(), who.branch());
	description += L" (";
	description += uiLocString(IDE_LANG_UI, UI_BASE_COUNTRY_ADJECTIVE + who.country() - 1);
	description += L" ";
	description += uiLocString(IDE_LANG_UI, UI_BASE_BRANCH_NAME + who.branch() - 1);
	description += L")";

	HCUnit* brigade = HCUnit::Lookup(who.hcUnitID());
	if ( !brigade )
	{
		description += L" ";
		description += uiLocString("chat.brigade.select");
	}
	else
	{
		if ( brigade->name() )
		{
			description += L" ";
			description += uiUTF8toUTF16(brigade->name());
		}
		if ( who.extendedInfo() && who.spawned() )
		{
			StratDB::StratObject* so = StratDB::FindStratObject(who.nearCP());
			if ( so != nullptr )
			{
				description += L" ";
				description += uiLocString("chat.near");
				description += L" ";
				description += so->DisplayName();
			}
			else
			{
				description += L" (";
				description += uiLocString("chat.ingame");
				description += L")";
			}
		}
	}

	uiChatPrintLine("%ls: %ls", name.c_str(), description.c_str());
}

/************************************************************************************************
	ProcessSquad
*************************************************************************************************/

void GameClient::ProcessSquad(const SquadMsg& msg)
{
	Client::ProcessSquad(msg);
}

/************************************************************************************************
	ProcessSquadControl
*************************************************************************************************/

void GameClient::ProcessSquadControl(const SquadControlResult& result)
{
	const int MAXJSFUNC = 32;
	int argc = 0;
	jsval argv[3];
	char jsfunc[MAXJSFUNC+1];
	char keyBuffer[SquadControl::MAX_TEXT_LEN+1];

	snprintf(keyBuffer, SquadControl::MAX_TEXT_LEN, "squad.control.%s", result.printResult());
	const std::wstring& locmsg = uiLocString(keyBuffer);

	if(result.m_action == SquadControl::_CREATE && result.m_result == SQR_ACCEPTED)
	{
		uiChatPrintLine(uiLocString("squad.control.newaccept").c_str(), chatGetWideCallsign(result.m_handle));
	}
	else
	{
		if ( locmsg.find('%') < locmsg.size() )
		{
			wchar_t buffer[SquadControl::MAX_TEXT_LEN+1];
			buffer[0] = 0;

			swprintf(buffer, SquadControl::MAX_TEXT_LEN, locmsg.c_str(), chatGetWideCallsign(result.m_handle));

			uiChatPrintLine(buffer);
		}
		else
		{
			uiChatPrintLine(locmsg.c_str());
		}
	}

	if(result.m_action == SquadControl::_CREATE)
	{
		if(result.m_result == SQR_ACCEPTED) // a user has excepted
		{
			snprintf(jsfunc, MAXJSFUNC, "%s", "onsquadaccept");
			argv[0] = phui::js::NewString(chatGetWideCallsign(result.m_handle));
			argv[1] = BOOLEAN_TO_JSVAL(true);
			argc = 2;
		}
		else if(result.m_result == SQR_DECLINED) // done just means it was submitted
		{
			snprintf(jsfunc, MAXJSFUNC, "%s", "onsquadaccept");
			argv[0] = phui::js::NewString(chatGetWideCallsign(result.m_handle));
			argv[1] = BOOLEAN_TO_JSVAL(false);
			argc = 2;
		}
		else if(result.m_result != SQR_DONE) // done just means it was submitted
		{
			snprintf(jsfunc, MAXJSFUNC, "%s", "onsquadcreate");
			argv[0] = phui::js::NewString(locmsg.c_str());
			argc = 1;
		}

		if(argc)
			uiCallJS(jsfunc, argc, argv);
	}

	if(result.m_action == SquadControl::_JOIN)
	{
		// clear the blocker
		ClearWait(SQUAD_RECRUITMENT_WAIT);

		if(result.m_result == SQR_ACCEPTED)
		{
			snprintf(jsfunc, MAXJSFUNC, "%s", "onrecruited");
			argv[0] = phui::js::NewString(chatGetWideCallsign(result.m_handle));
			argc = 1;

			uiCallJS(jsfunc, argc, argv);
		}
	}

	uiClearSquadCommandTimeout();

	Client::ProcessSquadControl(result);
}

/************************************************************************************************
	ProcessSquadInvite
*************************************************************************************************/

void GameClient::ProcessSquadInvite(const SquadMsg& msg)
{
	if(SquadInvites)
	{
		if (msg.isCreationInvite())
		{
			// The messages are fairly long so they're split into two entities
			{
				// First message
				const std::wstring& locmsg = uiLocString("squad.control.createinvite1");

				uiChatPrintLine(locmsg.c_str()
							, chatGetWideCallsign(msg.inviter())
							, uiUTF8toUTF16(msg.squad().callsign().c_str())
							, uiUTF8toUTF16(msg.squad().name().c_str())
							, SquadControl::CREATE_PLAYERS_REQUIRED+1
					);
			}

			{
				const std::wstring& locmsg = uiLocString("squad.control.createinvite2");
				const char* positionRank = (msg.inviteRank() == Squad::XO) ? "common.squads.ranks.rank4" : "common.squads.ranks.rank2";
				const std::wstring& position = uiLocString(positionRank);

				uiChatPrintLine(locmsg.c_str(), position.c_str());
			}
		}
		else
		{
			const std::wstring& locmsg = uiLocString("squad.control.joininvite");
			uiChatPrintLine(locmsg.c_str()
				, chatGetWideCallsign(msg.inviter())
				, uiUTF8toUTF16(msg.squad().callsign().c_str())
				, uiUTF8toUTF16(msg.squad().name().c_str()));
		}

		// lets save it
		Client::ProcessSquadInvite(msg);
	}
	else
	{
		// we're not accepting new invites, so let's shortcut and send a decline right away
		SquadControl control;
		control.sendDecline(PlayerClient()->ChatConnection());
	}
}

/************************************************************************************************
	ProcessSwingMeter
*************************************************************************************************/

void GameClient::ProcessSwingMeter(SwingMeter const& meter)
{
	Client::ProcessSwingMeter(meter);
}

/************************************************************************************************
	ProcessOpinionPoll
*************************************************************************************************/

void GameClient::ProcessOpinionPoll(OpinionPoll const& p)
{
	Client::ProcessOpinionPoll(p);
}

void GameClient::Reconnected()
{
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_SYSTEM, LSTR_RECONNECTED);
}

/************************************************************************************************
	StatusMessage
*************************************************************************************************/

void GameClient::StatusMessage(const char* status)
{
	rnlogoUpdateStatus(status);
}

/************************w************************************************************************
	RegisterHandler
*************************************************************************************************/

void GameClient::RegisterHandler(phui::MessageHandler* handler)
{
	_handlers.insert(handler);
}

/************************************************************************************************
	UnregisterHandler
*************************************************************************************************/

void GameClient::UnregisterHandler(phui::MessageHandler* handler)
{
	std::set<phui::MessageHandler*>::iterator it = _handlers.find(handler);
	if(it != _handlers.end())
		_handlers.erase(it);
}

void GameClient::ProcessMissionDetail(MissionDetail const& reply)
{
	Client::ProcessMissionDetail(reply);

	if(reply.m_result == MC_JOIN)
	{
		StratDB::StratObject* origin = StratDB::FindStratObject(reply.m_origin);
		StratDB::StratObject* target = StratDB::FindStratObject(reply.m_target);

		if(origin)
		{
			if(!target || (target->PosX() != origin->PosX() && target->PosY() != origin->PosY()))
			{
				static const ChatHandle handle = chatGetHandle("ORIGIN");
				Waypoint waypoint(true, handle, reply.m_poster, WPT_ORIGIN, 0, origin->PosX(), origin->PosY(), teulTime(),
									uiLocString2("waypoints.origin.tip").c_str());
				waypoint.setShared(false);
				SetWaypoint(waypoint);
			}
		}

		if(target)
		{
			static const ChatHandle handle = chatGetHandle("TARGET");
			Waypoint waypoint(true, handle, reply.m_poster, WPT_TARGET, 0, target->PosX(), target->PosY(), teulTime(), reply.m_desc);
			waypoint.setShared(false);
			SetWaypoint(waypoint);
			SetActiveWaypoint(handle);
		}
	}
}

void GameClient::ProcessMissionInvite(const MissionInvite& invite)
{
	Client::ProcessMissionInvite(invite);

	// HACK: Squad recruiter invites are mandatory and use a different
	// path because derp.
	if ( invite.m_isDirective == 0 )
		jsclient->RunScript("game.onmissioninvite()");
}

void clientStratControlUpdate(FULL_OBJ_ID const& id, int ownership, UINT32 flags)
{
	StratDB::StratObject* stratobj = StratDB::FindStratObject(id);
	if(stratobj)
	{
		stratobj->SetCountry(ownership);
		stratobj->SetUpdateTime(frametime);

		// Set the flags to -1 (0xffffffff) to not interfere with them
		if ( flags != (UINT32)~0 )
			stratobj->SetStratFlags((UINT32)flags);
	}
}

void GameClient::ProcessVehicleRequest(VEHICLE_REPLY const& reply)
{
	vehcommProcessVehicleRequestResponse(&reply);
}

void GameClient::ProcessPositionUpdateMsg(VEHICLE_UPDATE_HEADER const& vupHeader)
{
	vehupdateProcessPositionUpdateMsg((VEHICLE_UPDATE_HEADER*)&vupHeader);
}

void GameClient::ProcessRemoveVehicle(unsigned int hostindex)
{
	vehicleRemoveVehicle(hostindex);
}

void GameClient::ProcessVehicleHit(Vehicle::Hit const& hit)
{
	vehProcessVehicleHit(hit);
}

bool GameClient::ReceiveServerTrackedObject(int type, ACTION_OBJECT* a)
{
	return clientReceiveServerTrackedObject(type, a);
}

static void _editCallback(JSContext* ctx, UINT32 argc, jsval* argv)
{
    /*
     TBD - I expect something was intended here, but nothing is done currently.

	const char* type = (argc > 0) ? JS_GetStringBytes(JS_ValueToString(ctx, argv[0])) : nullptr;
	const char* value = (argc > 1) ? JS_GetStringBytes(JS_ValueToString(ctx, argv[1])) : nullptr;

	// do something with the values
	if(strcmp(type, "capture") == 0)
	{
		// we're editing the capture lua
		// send it to the host!
	}
     */
}

void GameClient::Edit(const char* type, const char* title, const char* value)
{
	uiCallJS("DoEdit", _editCallback, 3, type, title, value);
}

//////////////////////////////////////////////////////////////////////
// Deploy a HCUnit to a CP

void HCUnit::setHomeCPID(const FULL_OBJ_ID newHomeID)
{
	// Is this actually a change?
	if ( m_homeCPID == newHomeID)
		return;

	// let the old cp know we've moved
	StratDB::StratObject* from = StratDB::FindStratObject(m_homeCPID);
	if(from)
	{
		from->RemoveUnit(m_unitID);
		from->SetUpdateTime(frametime);
	}
	m_homeCPID = newHomeID;

	// let the new cp know we're now there
	StratDB::StratObject* to = StratDB::FindStratObject(m_homeCPID);
	if(to)
	{
		to->AddUnit(m_unitID);
		to->SetUpdateTime(frametime);
	}
}

void GameClient::ProcessHCUnitCountryChange(const HCUnitCountryChange& unitloc)
{
	HCUnit* unit = HCUnit::Lookup(unitloc.m_unitID);

	if (unit != nullptr)
	{
		if (unit->isGarrison())
		{
			if (unitloc.m_country != WWII_NO_COUNTRY)
			{
				unit->setCountry(unitloc.m_country);

				//If we have this unit selected then we need to update our selected persona country.
				if (Client::BrigadeSelected().m_detail.brigadeID() == unitloc.m_unitID)
				{
					if (WeaponSerial().serial != 0)
						RequestMissionWeapCancel();

					if (SelectedMission().m_brigadeID == unitloc.m_unitID)
						ExitMission();

					// we don't want to do anything else if the countries don't match (there would be a side switch)
					WWII_SIDE new_side = wwiiSide(unitloc.m_country);
					WWII_SIDE my_side = wwiiSide(Client::BrigadeSelected().m_detail.country());
					if (new_side == my_side)
					{
						Client::SetSelectedPersonaCountry(unitloc.m_country);
						SavePersona(SAVE_COUNTRY);
						SelectBrigade(Client::BrigadeSelected().m_detail.brigadeID(), true, true);
					}
					else
					{
						// ask server to choose for us
						SelectBrigade(0, true, true);
					}
				}
			}
		}
	}
	else
	{
		clientErrorLog("Unit Invalid in country change.");
	}
}
