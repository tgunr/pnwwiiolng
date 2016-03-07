#pragma once

#include <string>
#include <map>

#include "phui/Javascript.h"

#include "client/ww2Client.h"
#include "miss/missionTypes.h"
#include "map/mapPersonaSummary.h"
#include "map/mapMissionList.h"
#include "wwii/wwiiHCUnits.h"
#include "map/mapBrigadeSummary.h"
#include "strt/strtTypes.h"
#include "miss/missionTypes.h"
#include "wwii/wwiiRoomLists.h"
#include "pers/persrank.h"
#include "mm/mmEnums.h"
#include "miss/missionDetail.h"
#include "chat/chatMissionMember.h"
#include "client/clientSquad.h"
#include "chat/chatWho.h"
#include "wwii/wwiiOpinionPoll.h"
#include "chat/chatBrigadeControl.h"

/************************************************************************************************
	jsClient
*************************************************************************************************/

class jsMissionResults;
class jsBrigade;
class jsHCUnit;
class jsMission;
class jsCountry;
class jsRank;
class jsVehtype;
class jsStratObject;
class jsPersonaSummary;
class jsWeaponSerial;
class jsActiveMission;
class jsPosition;
class jsSquad;
class jsChatWho;
class jsSquadInvite;
class jsMotd;
class jsPoll;
class jsRecruiter;
class jsMissionInvite;
class MissionInvite;
class jsInstance;
class jsClientState;
class jsCaptureState;
class jsScenarioPlayer;
struct ScenarioPlayer;

namespace Instance
{
	class Descriptor;
	class ClientStatus;
};

typedef std::map<UINT32, jsMotd*> MotdList;

// Samples, Threshold, Penalty
void setChatRateLimiting();

class jsClient : public phui::js::Object
{
public:
	jsClient(ww2::Client*);

	JS_CLASS_DATA;
	JS_FUNCTION(getmisscountry);
	JS_FUNCTION(getmissbrigade);
	JS_FUNCTION(acceptmission);
	JS_FUNCTION(spawn);
	JS_FUNCTION(selectpersona);
	JS_FUNCTION(sendchatmessage);
	JS_FUNCTION(selectbrigade);
	JS_FUNCTION(createmission);
	JS_FUNCTION(requestweaponlist);
	JS_FUNCTION(cancelweaponreservation);
	JS_FUNCTION(reserveweapon);
	JS_FUNCTION(requestcrews);
	JS_FUNCTION(requestcommanders);
	JS_FUNCTION(joincrew);
	JS_FUNCTION(acceptcrew);
	JS_FUNCTION(requestpersonalist);
	JS_FUNCTION(requestmissionlist);
	JS_FUNCTION(requestquickmissionlist);
	JS_FUNCTION(exitmission);
	JS_FUNCTION(cancelandexitmission);
	JS_FUNCTION(requestbrigadedetail);
	JS_FUNCTION(requestmotd);
	JS_FUNCTION(createsquad);
	JS_FUNCTION(canaddwaypoint);
	JS_FUNCTION(canaddreport);
	JS_FUNCTION(setmotd);
	JS_FUNCTION(undeployms);
	JS_FUNCTION(requestmissiondetail);
	JS_FUNCTION(acceptoicnomination);
	JS_FUNCTION(findbrigade);
	JS_FUNCTION(requestrecruiters);
	JS_FUNCTION(enabledespawntimer);
	JS_FUNCTION(selectinstance); // joins the instance and sets the persona
	JS_FUNCTION(savepersona);
	JS_FUNCTION(getpersona);
	JS_FUNCTION(getrank);
	JS_FUNCTION(listinstances);
	JS_FUNCTION(createinstance);
	JS_FUNCTION(joininstance); // simply joins the instance without any persona changes
	JS_FUNCTION(isavailable);
	JS_FUNCTION(ismissionleader);

	enum{ 	COUNTRY_PROP, BRANCH_PROP, HANDLE_PROP,
			MISSIONS_PROP, SPAWNABLE_VEHICLES_PROP, WEAPONCOUNTER_PROP, PERSONAS_PROP,
			BRIGADES_PROP, BRIGADE_PROP, CHATCHANNELS_PROP,
			MISSIONROOMLIST_PROP,
			PERSONALISTTIME_PROP, RESERVEDWEAPON_PROP,
			SIDELOCKED_PROP, CURRENTMISSION_PROP, MISSIONRESULTS_PROP, DESPAWNTIMER_PROP,
			CANPOSTMISSION_PROP, PERSONA_PROP, MISSIONLISTUPDATE_PROP,
			LASTSPAWNLISTSTATUS_PROP, LASTSPAWNLISTSOURCE_PROP, ACTIVEMISSION_PROP,
			LASTSPAWNLISTSTATE_PROP, SQUAD_PROP, SQUADINVITE_PROP, CURRENTORDERS_PROP,
			ISPLAYNET_PROP, SIDEBALANCE_PROP, SIDEDELAY_PROP, POLL_PROP, SPAWNDELAY_PROP,
			ISMOBILESPAWN_PROP, CANSTO_PROP, BRIGADENOTICES_PROP, ISHC_PROP, ISOIC_PROP,
			OICOF_PROP, HAVEBRIGADENOTICES_PROP, RECRUITER_PROP, RECRUITERS_PROP,
			HAVERECRUITERS_PROP, BRANCHID_PROP, MISSIONINVITE_PROP, NEXTSUPPLY_PROP,
			INSTANCE_PROP, STATE_PROP, CAPTURESTATE_PROP, INSTANCES_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	void ProcessWeapList();
	void ProcessMissionLaunchUpdate(MISS_LAUNCH_UPDATE const&);
	void ProcessMissionLeaderUpdate(Chat::LocalizedMessage const&);
	void UpdatePersonas();
	void UpdateInstance(int status);
	void UpdateInstances(); // returns

	ww2::Client* client;
	MotdList motds;

protected:
	phui::js::array_root_ptr missions;
	phui::js::array_root_ptr spawnablevehicles;
	phui::js::array_root_ptr weaponcounter;
	phui::js::array_root_ptr personas;
	phui::js::array_root_ptr brigades;
	phui::js::array_root_ptr chatchannels;
	phui::js::array_root_ptr missionroomlist;
	phui::js::array_root_ptr brigadenotices;
	phui::js::array_root_ptr recruiters;
	phui::js::array_root_ptr instances; // list of last requested instances

	phui::js::root_ptr<jsBrigade> brigade;
	phui::js::root_ptr<jsMission> mission;
	phui::js::root_ptr<jsMissionResults> missionresults;
	phui::js::root_ptr<jsWeaponSerial> reservedweapon;
	phui::js::root_ptr<jsPersonaSummary> persona;
	phui::js::root_ptr<jsSquad> squad;
	phui::js::root_ptr<jsSquadInvite> squadinvite;
	phui::js::root_ptr<jsPoll> poll;
	phui::js::root_ptr<jsBrigade> oicof; // my current oic position
	phui::js::root_ptr<jsRecruiter> recruiter; // my current oic position
	phui::js::root_ptr<jsMissionInvite> missioninvite;
	phui::js::root_ptr<jsInstance> instance;
	phui::js::root_ptr<jsClientState> state;
	phui::js::root_ptr<jsCaptureState> capturestate;

	int lastmissionresults;
	int lastspawnliststatus;
	int brigadenoticecount;
	UINT16 nextsupply;
	FULL_OBJ_ID lastspawnlistsource;

	~jsClient();
};

class jsHCUnit : public phui::js::Object
{
public:
	jsHCUnit(HCUnit const*);
	jsHCUnit(unsigned int unitID);

	JS_CLASS_DATA;

	enum{ NAME_PROP, ID_PROP, PARENT_PROP, BRANCH_PROP, TYPE_PROP, RANK_PROP,
		  OWNER_PROP, OWNERRANK_PROP, HOME_PROP, CALLSIGN_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	const HCUnit* unit;

	~jsHCUnit();
};

class jsBrigade : public phui::js::Object
{
public:
	jsBrigade(BrigadeSummary const&);
	jsBrigade(BrigadeDetail const&);
	jsBrigade(unsigned int brigadeid);

	JS_CLASS_DATA;
	JS_FUNCTION(nominate);
	JS_FUNCTION(volunteer);
	JS_FUNCTION(replace);
	JS_FUNCTION(remove);
	JS_FUNCTION(resign);
	JS_FUNCTION(move);

	enum{ MOTD_PROP, NAME_PROP, ID_PROP, DIVISION_PROP, DIVISION_ID_PROP, MISSIONS_PROP,
			POPULATION_RANK_PROP, PARENT_PROP, MOTTO_PROP, NICK_PROP, OFFICERS_PROP, HOME_PROP,
			OWNER_PROP, OWNERRANK_PROP, OBJECTIVE_PROP,
			MOBILESPAWNS_PROP, ISBRIGADE_PROP, ISSQUADBRIGADE_PROP, CALLSIGN_PROP,
			OIC_PROP, OICRANK_PROP, CANVOTE_PROP, CANTAKECHARGE_PROP, ACTIVEOIC_PROP,
			COMMANDER_PROP, COMMANDERRANK_PROP};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	BrigadeDetail const& Detail() const { return detail;}
	void SetDetail(BrigadeDetail const& d) {detail = d; UpdateOfficerArrays(); }

protected:
	HCUnit* unit;
	BrigadeDetail detail;
	phui::js::Array* officerranks;
	phui::js::Array* officernames;

	void UpdateOfficerArrays();

	~jsBrigade();
};

class jsMission : public phui::js::Object
{
public:
	jsMission(MissionDetail const&);
	JS_CLASS_DATA;
	enum
	{
		MOBILESPAWN_PROP,AMLEADER_PROP, TIMENOW_PROP,
		TIMELAUNCH_PROP, STATE_PROP, POSTER_PROP, SPAWNDELAY_PROP,
		ISSQUADMISSION_PROP, POPULATION_PROP, CANMOBILESPAWN_PROP,
		OPEN_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	MissionDetail const& Detail() const { return detail; }
	void LaunchUpdate(MISS_LAUNCH_UPDATE const&);
	void LeaderUpdate(Chat::LocalizedMessage const&);

protected:
	MissionDetail detail;
	int state;
	int starttime;
};

class jsPersonaSummary : public phui::js::Object
{
public:
	jsPersonaSummary(PersonaSummary const&);

	JS_CLASS_DATA;

	enum{ BRANCH_PROP, RANKPERCENT_PROP, LOCKEDTHRU_PROP, LASTUSED_PROP, BRANCHID_PROP, AVAILABILITY_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	PersonaSummary const& Summary() const { return summary; }

protected:
	PersonaSummary summary;
	int timestamp;

	~jsPersonaSummary();
};


class jsCountry : public phui::js::Object
{
public:
	jsCountry(WWII_COUNTRY);
	JS_CLASS_DATA;
};

class jsRank : public phui::js::Object
{
public:
	jsRank(WWII_COUNTRY c, BRANCH_ENUM b, RANK_NO r);
	JS_CLASS_DATA;
	enum{ NAME_PROP, LEVEL_PROP, REALLEVEL_PROP, MAXLEVEL_PROP, LOCKED_PROP, TYPE_PROP };
	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	WWII_COUNTRY country;
	BRANCH_ENUM branch;
	RANK_NO level;
};

class jsChatChannel : public phui::js::Object
{
public:
	jsChatChannel(int index);

	JS_CLASS_DATA;

	enum{ NAME_PROP, RECV_PROP, XMIT_PROP, radioidx_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	int radioidx;

	~jsChatChannel();
};

class jsWeaponCounter : public phui::js::Object
{
public:
	jsWeaponCounter(WeaponCounter const&, int index = -1, char minimumRank = 0);
	jsWeaponCounter(int capacity, int current, int weapIndex, char minRank, int availability);
	JS_CLASS_DATA;
};

class jsScenarioPlayer : public phui::js::Object
{
public:
	jsScenarioPlayer(ScenarioPlayer&);
	JS_CLASS_DATA;
protected:
	//ScenarioPlayer* player;
};

class jsMissionRoomEntry : public phui::js::Object
{
public:
	jsMissionRoomEntry(RoomListEntry const&, WWII_COUNTRY, BRANCH_ENUM);
	JS_CLASS_DATA;
	RoomListEntry const& Entry() const { return entry; }
protected:
	RoomListEntry entry;
};

class jsMissionMember : public phui::js::Object
{
public:
	jsMissionMember(MissionMember const&, WWII_COUNTRY, BRANCH_ENUM);
	JS_CLASS_DATA;
};

class jsMissionResults : public phui::js::Object
{
public:
	jsMissionResults(MISSION_RESULTS const&);
	JS_CLASS_DATA;
};

class jsMotd : public phui::js::Object
{
public:
	jsMotd(MOTD_REPLY const&);
	JS_CLASS_DATA;
	bool Valid();
};

class jsWeaponSerial : public phui::js::Object
{
public:
	jsWeaponSerial(WEAPON_SERIAL const&);
	JS_CLASS_DATA;
	WEAPON_SERIAL const serial;
};

class jsActiveMission : public phui::js::Object
{
public:
	jsActiveMission(ActiveMission const&);
	JS_CLASS_DATA;
};

class jsSquad : public phui::js::Object
{
public:
	jsSquad(const ClientSquad&);

	JS_CLASS_DATA;

	JS_FUNCTION(updatefield);
	JS_FUNCTION(requestmembers);

	enum
	{
		ID_PROP, NAME_PROP, SHORTNAME_PROP, CO_PROP, VOTING_PROP,
		RECRUITING_PROP, CREATED_PROP, DESCRIPTION_PROP, MOTD_PROP,
		HOMEPAGE_PROP, MEMBERS_PROP, ROOKIEDAYS_PROP, MODIFIED_PROP,
		ONLINEMEMBERS_PROP, TOTALCOUNT_PROP, ONLINECOUNT_PROP,
		DAYOFWEEK_PROP, HOUR_PROP, TIMEZONE_PROP, NIGHT_PROP,
		LANGUAGE_PROP, SLOGAN_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	const ClientSquad& squad;

protected:

	~jsSquad();

	phui::js::array_root_ptr members;
};

class jsSquadInvite : public phui::js::Object
{
public:
	jsSquadInvite(SquadInvite const&);

	JS_CLASS_DATA;
	enum{ INVITED_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	SquadInvite invite;
};

class jsChatWho : public phui::js::Object
{
public:
	jsChatWho(Chat::Who const&);
	JS_CLASS_DATA;

	enum{ MISSION_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	UINT32 missionID; // saved in case we need it
	phui::js::root_ptr<jsMission> mission;

	enum CHATWHO_TIMEFRAME
	{
		CWT_INVALID = 0,
		CWT_HOURS,
		CWT_DAYS,
		CWT_MONTHS,
		CWT_YEARS,
		CWT_NUM_TIMEFRAMES,
	};

private:

	CHATWHO_TIMEFRAME getJoinedSquad(UINT32 joined, UINT32& numUnits) const;
};

class jsPoll : public phui::js::Object
{
public:
	jsPoll(OpinionPoll const&);
	JS_CLASS_DATA;
	JS_FUNCTION(vote);

	const OpinionPoll poll;

protected:
	phui::js::array_root_ptr options;
};

class jsBrigadeControlNotice : public phui::js::Object
{
public:
	jsBrigadeControlNotice(BrigadeControlNotice const&, int i);
	JS_CLASS_DATA;
	JS_FUNCTION(veto);

	enum{ ISVETOABLE_PROP, EXPIRES_PROP, EXPIRED_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	const BrigadeControlNotice notice;
	const int index;
};

class jsRecruiter : public phui::js::Object
{
public:
	jsRecruiter(Recruiter const&);
	JS_CLASS_DATA;
	JS_FUNCTION(join);

	const Recruiter recruiter;
};

class jsMissionInvite : public phui::js::Object
{
public:
	jsMissionInvite(MissionInvite const&);
	JS_CLASS_DATA;
};

class jsInstance : public phui::js::Object
{
public:
	jsInstance();
	jsInstance(const Instance::Descriptor* d);
	JS_CLASS_DATA;

	enum
	{
		CATEGORY_PROP,
		TYPE_PROP,
		HANDLE_PROP,
		OWNER_PROP,
		ID_PROP,
		RESULT_PROP,
		MINE_PROP // did I create this?
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	// update which instance we're in
	void Update(int result);

	int lastResult;
	const Instance::Descriptor* descriptor;
};

class jsInstanceState : public phui::js::Object
{
public:
	jsInstanceState(const Instance::ClientStatus* s);
	JS_CLASS_DATA;

	enum
	{
		DESCRIPTION_PROP,
		AGE_PROP,
		RUNNINGFOR_PROP,
		TEAM1_PROP,
		TEAM2_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	const Instance::ClientStatus* state;
};


class jsClientState : public phui::js::Object
{
public:
	jsClientState();
	JS_CLASS_DATA;
	JS_FUNCTION(paccess); // persona access

	enum
	{
		UPSELLS_PROP,
		CANCAPTURE_PROP,
		CANTRAIN_PROP,
		MAXSQUADRANK_PROP,
		TIMELEFT_PROP,
		SUBACTIVE_PROP,
		CANCAPTURECHECK_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
};

class jsCaptureState : public phui::js::Object
{
public:
	jsCaptureState();
	JS_CLASS_DATA;

	enum
	{
		VALID_PROP,
		STAGE_PROP,
		CAPTURABLE_PROP,
		PENDING_PROP,
		PENDINGSECONDS_PROP,
		PROGRESS_PROP,
		RATE_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
};

