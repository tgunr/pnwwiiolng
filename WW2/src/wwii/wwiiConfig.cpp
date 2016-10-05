#include <syscfg.h> // Path is project specific.
// Created: Sun Feb 15 2004
//
// Functionality wrapping the WWII_CONFIG structure
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiConfig.h"
#include "wwii/wwiiConfigMap.h"
#include <limits.h>

//Country names that match up with the country codes
const char* _countryName[MAX_COUNTRIES] =
{
	"NoCountry",
	"England",
	"USA",
	"France",
	"Germany",
	"Italy",
	"China",
	"Russia"
};

//Country names that match up with the country codes
const char* _countryNameAdj[MAX_COUNTRIES] =
{
	"NoCountry",
	"English",
	"US",
	"French",
	"German",
	"Italian",
	"Chinese",
	"Russian"
};

// Side names, normal form
const char* _sideName[MAX_SIDES] =
{
	"NoSide",
	"Allies",
	"Axis",
	"Neutral"
};

// Side names, adjective form
const char* _sideNameAdj[MAX_SIDES] =
{
	"NoSide",
	"Allied",
	"Axis",
	"Neutral"
};

const char* _countryCode[MAX_COUNTRIES] =
{
	"NONE"
,	"UK"
,	"US"
,	"FR"
,	"DE"
,	"IT"
,	"CN"
,	"RU"
};

bool g_activeCountries[MAX_COUNTRIES];

// Structures
WWII_CONFIG host_config;

WWII_CONFIG_MAP configMap[] =
{
	{ nullptr,	TYPE_UNUSED, nullptr, 0, nullptr, nullptr }
,	{ "arena.banCheats",			TYPE_BOOL,	&host_config.arena.banCheats,			0,		"n",	"Ban DamnCheaters detects" }
,	{ "arena.checksums",			TYPE_BOOL,	&host_config.arena.checksums,			0,		"n",	"Detect and eject players with modified clients" }
,	{ "arena.enableIRCCommands",	TYPE_BOOL,	&host_config.arena.enableIRCCommands, 	0,		"y",	"Enable '.command's from IRC management interface" }
,	{ "arena.fixedTimeFactor",		TYPE_SINT,	&host_config.arena.fixedTimeFactor, 	30,		"0",	"Fixed time acceleration rate (0=unlocked)" }
,	{ "arena.freezeClock",			TYPE_SINT,	&host_config.arena.freezeClock, 		2400,	"0000",	"Lock clock to specified HHMM time (0000=unlocked)" }
,	{ "arena.hideBuzzIcons",		TYPE_BOOL,	&host_config.arena.hideBuzzIcons,		0,		"y",	"Show non-GMs 'GameMgr' instead of icons on buzzards" }
,	{ "arena.intermission",			TYPE_BYTE,	&host_config.arena.intermission, 		10, 	"0",	"0=Campaign, 1=Intermission Setup, 2=Running Intermission" }
,	{ "arena.kickModHacks",			TYPE_BOOL,	&host_config.arena.kickModHacks, 		0,		"n",	"Detect and eject client memory tampers with clients" }
,	{ "arena.kickSpeedHacks",		TYPE_BOOL,	&host_config.arena.kickSpeedHacks,		0,		"n",	"Detect and eject speed hacks" }
,	{ "arena.limitedDamage",		TYPE_BOOL,	&host_config.arena.limitedDamage,		0,		"n",	"Enable damage limiting (to 1J per hit)" }
,	{ "arena.lock",					TYPE_BYTE,	&host_config.arena.lock,				255,	"0",	"Lock Arena: (Binary flags: 0=Unlocked, 1=No Spawning, 2=No Cap, 4=Kills, 8=CRS Only)" }
,	{ "arena.rebuildAI",			TYPE_BOOL,	&host_config.arena.rebuildAI,			0,		"y",	"Enable AI rebuilding after destruction" }
,	{ "arena.vehicleRestrictions",	TYPE_BOOL,	&host_config.arena.vehicleRestrictions, 0,		"y","Enable vehicle-access rank restrictions" }
,	{ "arena.manualAIRepair",		TYPE_SINT,	&host_config.arena.manualAIRepair,		64,		"0",	"Number of Repair Kits to rebuild AI (0=disabled)" }
,	{ "arena.minutesPerAAHour",		TYPE_SINT,	&host_config.arena.minutesPerAAHour,	0,		"60",	"Number of real minutes per Anti-Addiction hour" }
,	{ "arena.maxPlayers",			TYPE_SINT,	&host_config.arena.maxPlayers,			3500, 	"3500",	"Maximum players allowed concurrently" }
,	{ "arena.speedHackRatio",		TYPE_FLOAT,	&host_config.arena.speedHackRatio,		0,		"1.0001",	"Minimum drift ratio to be a speed-hack" }
,	{ "arena.mobileSpawnWaypoint",	TYPE_STRING,host_config.arena.mobileSpawnWaypoint, MAX_HANDLE_SIZE, "MSPAWN", "Mobile Spawn Waypoint (gamename)" }
,	{ "arena.despawnTime",			TYPE_SINT,	&host_config.arena.despawnTime_s,		3600,	"10",	"Despawn Timer (seconds)" }

,	{ "balance.above",				TYPE_SINT,	&host_config.balance.above,				0,		"0",	"Minimum players top side must have to get side balanced"  }
,	{ "balance.delaysMax",			TYPE_SINT, 	&host_config.balance.delaysMax, 		0,		"0",	"Maximum side-balancing delay (seconds)" }
,	{ "balance.delaysPctg",			TYPE_SINT,	&host_config.balance.delaysPctg,		0,		"0",	"Side must have this %age more players to start spawn delays" }
,	{ "balance.marginPctg",			TYPE_SINT,	&host_config.balance.marginPctg,		0,		"0",	"Side must have this %age more players to start balancing" }
,	{ "balance.personaDelay",		TYPE_SINT,	&host_config.balance.personaDelay,		0,		"0",	"Seconds delay before you can choose an imbalanced persona" }

,	{ "capture.marginS",			TYPE_SINT,	&host_config.capture.margin_s,			30,		"6",	"Capture messages must occur within this many seconds of 'timeS' to be valid" }
,	{ "capture.tickIntervalS",		TYPE_SINT,	&host_config.capture.tickInterval_s,	200,	"5",	"Interval between capture ticks (seconds)" }
,	{ "capture.startM",				TYPE_SINT,	&host_config.capture.start_m,			3600,	"10",	"Initial delay on a facility becoming capturable (minutes)" }
,	{ "capture.rebuildTimeS",		TYPE_SINT,	&host_config.capture.rebuildTime_s,		0,		"60",	"Table Rebuild Timer (seconds, after capture)" }

,	{ "factoryPeriod",				TYPE_INT,	&host_config.factoryPeriod,				0,		"600",	"Time between factory outputs (seconds)" }
,	{ "factoryWarmupTimeM",			TYPE_SINT,	&host_config.factoryWarmupTimeM,		1440,	"30",	"Time from server-up to first factory output (minutes)" }
,	{ "fbRepairIntervalM",			TYPE_SINT,	&host_config.fbRepairIntervalM,			1440,	"5",	"Time from last damage after which FBs begin to repair (minutes)" }
,	{ "minEWSTrip",					TYPE_BYTE,	&host_config.minEWSTrip,				128,	"1",	"Number of players required to trip EWS" }
,	{ "mobileSpawnRank",			TYPE_SINT,	&host_config.mobileSpawnRank,			26,		"0",	"Rank required to be a deploy mobile spawn" }
,	{ "rpTime",						TYPE_INT,	&host_config.rpTime,					0, 		"60",	"Second cost per Resource Point (should be 60)" }
,	{ "sideLockTimeM",				TYPE_SINT,	&host_config.sideLockTime_m,			45000,	"30",	"Time required out-of-game between non-balanced side changes (minutes)" }
,	{ "sayEnemyRange",				TYPE_SINT,	&host_config.sayEnemyRange,				1500,	"15",	"Enemy \"say\" range in meters" }
,	{ "sayFriendlyRange",			TYPE_SINT,	&host_config.sayFriendlyRange,			1500,	"100",	"Friendly \"say\" range in meters" }
,	{ "whisperEnemyRange",			TYPE_SINT,	&host_config.whisperEnemyRange,			1500,	"2",	"Enemy \"whisper\" range in meters" }
,	{ "whisperFriendlyRange",		TYPE_SINT,	&host_config.whisperFriendlyRange,		1500,	"11",	"Friendly \"whisper\" range in meters" }
,	{ "yellEnemyRange",				TYPE_SINT,	&host_config.yellEnemyRange,			1500,	"150",	"Enemy \"yell\" range in meters" }
,	{ "yellFriendlyRange",			TYPE_SINT,	&host_config.yellFriendlyRange,			1500,	"200",	"Friendly \"yell\" range in meters" }

,	{ "kills.deductFriendly",		TYPE_BOOL,	&host_config.kills.deductFriendly,		0,		"n",	"Enable point deduction for friendly fire kills" }
,	{ "kills.fromBuildings",		TYPE_BOOL,	&host_config.kills.fromBuildings,		0,		"n",	"Enable kill credits from building destruction" }
,	{ "kills.maxValue",				TYPE_SINT,	&host_config.kills.maxValue,			32000,	"4",	"Maximum relative value between two vehicles" }
,	{ "kills.notifyKiller",			TYPE_BOOL,	&host_config.kills.notifyKiller,		0,		"n",	"Enable kill credit notifices" }
,	{ "kills.reportRats",			TYPE_BOOL,	&host_config.kills.reportRats,			0,		"n",	"Enable Kill-A-Rat notices" }
,	{ "kills.reportDamage",			TYPE_BOOL,	&host_config.kills.reportDamage,		0,		"n",		"Enable host reporting of joules damage being sent to your client" }
,	{ "kills.assistWindow_s",		TYPE_BOOL,	&host_config.kills.assistWindow_s,		0,		"90",	"Time damage is remembered for assist credit (seconds)" }

//,	{ "mission.expireM",			TYPE_SINT,	&host_config.mission.expire_m,			15840,	"0",	"No-longer used (minutes)" }
,	{ "mission.launchCycleS",		TYPE_SINT,	&host_config.mission.launchCycle_s,		3600,	"0",	"Time between launch button recycles (seconds,	0=disabled)" }
//,	{ "mission.launchWindowS",		TYPE_SINT,	&host_config.missionLaunchWindow_s,		3600,	"0",	"No-longer used (seconds)" }
,	{ "mission.leaderAFKTime",		TYPE_SINT,	&host_config.mission.leaderAFKTime_m,	1500,	"5","Time mission leaders can AFK before allowing /takelead (minutes)" }
,	{ "mission.minStartS",			TYPE_SINT,	&host_config.mission.minStartTime_s,	64000,	"0",	"Time from mission posting to spawning being enabled (seconds)" }
,	{ "mission.timeoutM",			TYPE_SINT,	&host_config.mission.timeOut_m,			4320,	"4",	"Time a mission remains available without a leader (minutes)" }

,	{ "objective.clearTimeM",		TYPE_SINT,	&host_config.objective.clearTime_m,		1440,	"5",	"Time to execute a /clear (minutes)" }
,	{ "objective.fallbackNoticeM",	TYPE_SINT,	&host_config.objective.fallbackNotice_m,1440,	"2",	"Time from announcing a fallback to execution (minutes)" }
,	{ "objective.gracePeriodM",		TYPE_SINT,	&host_config.objective.gracePeriod_m,	1440,	"10",	"Time from going over AO budget to warning (minutes)" }
,	{ "objective.holdFallLimit",	TYPE_SINT,	&host_config.objective.holdFallLimit,	0,		"2",	"Maximum hold/fall instructions per window" }
,	{ "objective.holdFallWindowM",	TYPE_SINT,	&host_config.objective.holdFallWindow_m,10800,	"60",	"Time across which hold/fall limit is applied (minutes)" }
,	{ "objective.minLifetimeM",		TYPE_SINT,	&host_config.objective.minLifetime_m,	1440,	"30","Minimum duration of objectives (minutes)" }
,	{ "objective.overrunWindowM",		TYPE_SINT,	&host_config.objective.overrunWindow_m,	1440,"3",	"Duration of overrun requests (minutes)" }
,	{ "objective.requestTimeM",		TYPE_SINT,	&host_config.objective.requestTime_m,	1440,	"3",	"Duration of objective requests (minutes)" }
,	{ "objective.warningPeriodM",		TYPE_SINT,	&host_config.objective.warningPeriod_m,1440,"10",	"Duration of AO budet warnings (minutes)" }
,	{ "oic.attackVoteTimeS",		TYPE_SINT,	&host_config.oic.attackVoteTime_s,		3600,	"60",	"Duration of attack requests (seconds)" }
,	{ "oic.moveVoteTimeS",			TYPE_SINT,	&host_config.oic.moveVoteTime_s,		3600,	"60",	"Duration of move request proposals (seconds)" }
,	{ "oic.nominationVoteTimeS",	TYPE_SINT,	&host_config.oic.nominationVoteTime_s,	3600,	"60",	"Duration of nomination proposals (seconds)" }
,	{ "oic.offerTimeS",				TYPE_SINT,	&host_config.oic.offerTime_s,			3600,	"90",	"Duration of OIC offers (seconds)" }
,	{ "oic.removalVoteTimeS",		TYPE_SINT,	&host_config.oic.removalVoteTime_s,		3600,	"60",	"Duration of OIC removal proposals (seconds)" }
,	{ "oic.replaceDelayS",			TYPE_SINT,	&host_config.oic.replaceDelay_s,		3600,	"60",	"Duration of OIC removal notices (seconds)" }
,	{ "oic.withdrawVoteTimeS",		TYPE_SINT,	&host_config.oic.withdrawVoteTime_s,	3600,	"90",	"Duration of attack withdrawl proposals (seconds)" }

,	{ "score.capperKillBonus",		TYPE_SINT,	&host_config.score.capperKillBonus,		0,		"1",	"Bonus points for killing a player who is inside a capture building" }
,	{ "score.capture",				TYPE_SINT,	&host_config.score.capture,				0,		"5",	"Points awarded for capture" }
,	{ "score.guardingKillBonus",	TYPE_SINT,	&host_config.score.guardingKillBonus,	0,		"2",	"Bonus points for scoring a kill FROM inside a capture building" }
,	{ "score.guardPointPer_s",		TYPE_SINT,	&host_config.score.guardPointPer_m,		0,		"2",	"Time required to score one guard point (minutes)" }
,	{ "score.stratDmgInterval_s",	TYPE_SINT,	&host_config.score.stratDmgInterval_s,	0,		"30",	"Minimum time between same strat object awarding points" }
,	{ "score.maxGlobalSharedRank",	TYPE_SINT,	&host_config.score.maxGlobalSharedRank,	0,		"5",	"Rank at which rank-sharing cross-sides stops (ranks below this are shared cross-side by branch)" }
,	{ "score.maxSideSharedRank",	TYPE_SINT,	&host_config.score.maxSideSharedRank,	0,		"12",	"Rank at which rank-sharing same-side stops (ranks below this are shared by branch among personas on the same side)" }

,	{ "squad.electionDuration",		TYPE_SINT,	&host_config.squad.electionDuration,	0,		"1",	"Duration of undecided CO elections (days)" }
,	{ "squad.electionInterval",		TYPE_SINT,	&host_config.squad.electionInterval,	0,		"7",	"Minimum time between elections for a squad (days)" }
,	{ "squad.recruitExpiry",		TYPE_BOOL,	&host_config.squad.recruitExpiry,		0,		"n",	"Enable automatic expiry of recruits?" }
,	{ "squad.recruiterListMax",		TYPE_BYTE,	&host_config.squad.recruiterListMax,	64,		"20",	"Maximum number of recruiters to list at once" }
,	{ "squad.recruiterListMin",		TYPE_BYTE,	&host_config.squad.recruiterListMin,	64,		"8",	"Minimum best matches to list before showing less-suitable matches" }
,	{ "squad.defaultSquad",			TYPE_STRING,&host_config.squad.defaultSquad,MAX_HANDLE_SIZE,"",	"If set: players will be auto-joined to this squad if they have not yet been in a squad" }

,	{ "sto.damageCapacity",			TYPE_INT,	&host_config.sto.damageCapacity,		0,		"15000","Cumulative joules damage to destroy an STO UMS" }
,	{ "sto.damageThreshold",		TYPE_INT,	&host_config.sto.damageThreshold,		0,		"1000",	"Minimum joules for damage to be applied to an STO UMS" }
,	{ "sto.fruDeathCooldownS",		TYPE_INT,	&host_config.sto.fruDeathCooldown_s,	0,		"120",	"Seconds after one FRU is destroyed before another can be placed" }
,	{ "sto.fruPlaceCooldownS",		TYPE_INT,	&host_config.sto.fruPlaceCooldown_s,	0,		"60",	"Seconds after placing one FRU before another can be placed" }
,	{ "sto.hulkLifetime",			TYPE_SINT,	&host_config.sto.hulkLifetime_m,		0,		"30",	"Maximum lifetime of an STO Hulk (minutes)" }
,	{ "sto.hulkDamageCapacity",		TYPE_INT,	&host_config.sto.hulkDamageCapacity,	0,		"0",	"Cumulative joules damage to destroy an STO Hulk" }
,	{ "sto.hulkDamageThreshold",	TYPE_INT,	&host_config.sto.hulkDamageThreshold,	0,		"0",	"Minimum joules for damage to be applied to an STO Hulk" }
,	{ "sto.hulkLimitPerArea",		TYPE_SINT,	&host_config.sto.hulkLimitPerArea,		512,	"52",	"Maximum hulks per Chat Grid Area (6km2)" }
,	{ "sto.hulkLimitPerCell",		TYPE_SINT,	&host_config.sto.hulkLimitPerCell,		512,	"13",	"Maximum hulks per chat Grid Cell (512m2)" }
,	{ "sto.mobileSpawneeValue",		TYPE_INT,	&host_config.sto.mobileSpawneeValue,	0,		"1",	"Points to award to mobile spawn owner when someone spawns thru it." }
,	{ "sto.smokeLifetimeOverrideS",	TYPE_SINT,	&host_config.sto.smokeLifetimeOverrideS,64000,	"0",	"Override default lifetime of STO smoke grenades (0=use weapon default)" }

,	{ "supply.enabled",				TYPE_BOOL,	&host_config.supply.enabled,			0,		"y",	"Supply System Master Switch: Disable for static lists of 1 unit" }
,	{ "supply.debug",				TYPE_BOOL,	&host_config.supply.debug,				0,		"n",	"Enable additional debugging of supply" }
,	{ "supply.fbsToFrontline",		TYPE_BOOL,	&host_config.supply.fbsToFrontline,		0,		"y",	"Enable FBs from Brigades to No-Mans Towns" }
,	{ "supply.listRefreshS",		TYPE_SINT,	&host_config.supply.listRefresh_s,		0,		"10",	"Time between client refreshes of spawn list (seconds)" }
,	{ "supply.overstockLimit",		TYPE_SINT,	&host_config.supply.overstockLimit,		5000,	"25",	"Percentage a supply list can be overstocked" }
,	{ "supply.nomanDepots",			TYPE_BOOL,	&host_config.supply.nomanDepots,		0,		"y",	"Enable spawning at brigade-linked depots in DO towns" }
,	{ "supply.populationAdjust",		TYPE_SINT,	&host_config.supply.populationAdjust,100,	"60",	"Divisor in population-based rate adjustment" }
,	{ "supply.reserveTimeoutM",		TYPE_SINT,	&host_config.supply.reserveTimeout_m,	0,		"10",	"Time vehicle reservations last (minutes)" }
,	{ "supply.rescueTimeM",			TYPE_SINT,	&host_config.supply.rescueTime_m,		0,		"15",	"Time 'rescue'd vehicles take to to return to supply (minutes)" }
,	{ "supply.resupplyTimeM",		TYPE_INT,	&host_config.supply.resupplyTime_m,		0,		"270",	"Baseline time for resupply of spawned vehicles (minutes)" }
,	{ "supply.surrenderInf",		TYPE_BYTE,	&host_config.supply.surrenderInf,		100,	"10",	"Percentage infantry must pass below before allowing 'offmap' surrender from a pocket" }
,	{ "supply.trickleEnabled",		TYPE_BOOL,	&host_config.supply.trickleEnabled,		0,		"y",	"Enable trickling of supply into spawn lists" }
,	{ "supply.trickleSlices",		TYPE_SINT,	&host_config.supply.trickleSlices,		0,		"15",	"Number of slices (intervals) at which supply is trickled" }
,	{ "supply.trickleStartM",		TYPE_SINT,	&host_config.supply.trickleStart_m,		0,		"15",	"Time delay for first trickle (minutes)" }
,	{ "supply.trickleTimeM",		TYPE_INT,	&host_config.supply.trickleTime_m,		0,		"180",	"Time from start to completion of trickle (minutes)" }

,	{ "throttle.enabled",			TYPE_BOOL,	&host_config.throttle.enabled,			0,		"y",	"Enable depot throttling" }
,	{ "throttle.percentage",		TYPE_BYTE,	&host_config.throttle.percentage,		100,	"5",	"Percentage equipment to expose at throttles" }
,	{ "throttle.timeM",				TYPE_BYTE,	&host_config.throttle.timeM,			0,		"10",	"Time after which throttles are released (minutes)" }

,	{ "update.avoidRepeats",		TYPE_BOOL,	&host_config.update.avoidRepeats,		0,		"n",	"Enable re-sending old update data for a vehicle" }
,	{ "update.countingEnabled",		TYPE_BOOL,	&host_config.update.countingEnabled,	0,		"n",	"Enable speed hacker monitoring" }
,	{ "update.defaultUM",			TYPE_BYTE,	&host_config.update.defaultUM,			0,		"0",	"Default Update Mode (0-4)" }
,	{ "update.errorThrottling",		TYPE_BOOL,	&host_config.update.errorThrottling,	0,		"n",	"Enable throttling of updates on congestion" }
,	{ "update.forgivePeriod",		TYPE_SINT,	&host_config.update.sampleForgive_s,	0,		"3",	"Maximum time between alarms before we forget them" }
,	{ "update.ignore",				TYPE_SINT,	&host_config.update.sampleIgnore,		600,	"11",	"Number of initial packets that we don't even count" }
,	{ "update.limitNewVehicles",	TYPE_SINT,	&host_config.update.limitNewVehicles,	128,	"2","Maximum new vehicles in a vehicle update (0=no limit)" }
,	{ "update.riderBiasing",		TYPE_SINT,	&host_config.update.riderBiasing,		8,		"400",	"Factor to reduce rider vis list priority (0=don't)" }
,	{ "update.sampleBadCycleMax",	TYPE_SINT,	&host_config.update.sampleBadCycleMax,	0,		"3",	"Faults to require before kicking someone for being a speed hacker" }
,	{ "update.sampleGrace",			TYPE_SINT,	&host_config.update.sampleGrace_s,		3600,	"16",	"Time before packet counting starts (seconds)" }
,	{ "update.samplePeriod",		TYPE_SINT,	&host_config.update.samplePeriod_s,		3600,	"1",	"Time over which we count packets as a unit (seconds)" }
,	{ "update.starvationLimit",		TYPE_BYTE,	&host_config.update.starvationLimit_s,	255,	"75",	"Time without updates after which we kick a player (seconds)" }
,	{ "update.visLimitThrottling",		TYPE_BYTE,	&host_config.update.visLimitThrottling,	255,"0",	"Number of missed updates before limiting player's vis limit" }
,	{ "update.awsGracePeriodS",		TYPE_SINT,	&host_config.update.awsGracePeriod_s,	60000,	"90",	"Number of seconds from triggering AWS to appearing on it" }
,	{ "update.awsHighThreshold",	TYPE_BYTE,	&host_config.update.awsHighThreshold,	255,	"8",	"Number of planes to trip a 'high' EWS presence" }

,	{ "weather.lockToFog",			TYPE_SINT,	&host_config.weather.lockToFog,			99,	"0",	"Lock to a particular fog pattern number (0=unlocked)" }
,	{ "weather.lockToPattern",		TYPE_SINT,	&host_config.weather.lockToPattern,		99,	"0",	"Lock to a particular weather pattern number (0=unlocked)" }
,	{ "weather.numFogs",			TYPE_SINT,	&host_config.weather.numFogs,		 99,	"16",	"Number of fog patterns [per weather pattern]" }
,	{ "weather.numPatterns",		TYPE_SINT,	&host_config.weather.numPatterns,		99,	"3",	"Number of weather patterns" }
,	{ "weather.chanceOf",			TYPE_SINT,	&host_config.weather.chanceOf,		100,	"10",	"Per-day chance it will be a rainy day (0=never,	100=always)" }

// -- End of list
,	{ nullptr, MAX_WWII_CONFIG_TYPES, nullptr, 0, nullptr, nullptr }
};

static bool
_isAffirmative(const char* const string)
{
	if ( tolower(*string) == 'y' )
		return true;
	if ( strcasecmp(string, "on") == 0 )
		return true;

	if ( atol(string) == 1 )
		return true;

	return false;
}

bool
wwiiConfigSetINT(UINT32* data, const char* const source, UINT64 max)
{
	if ( data == nullptr || source == nullptr )
	{
		dbugLog(_FLAS, "called with NULL data or source");
		return false;
	}

	*data = 0;

	if ( *source == 0 )
		return false;

	if ( !isdigit(*source) )
		return false;

	*data = static_cast<UINT32>(atol(source));
	if ( max > 0 && *data > static_cast<UINT32>(max) )
		*data = static_cast<UINT32>(max);

	return true;
}

bool
wwiiConfigSetSINT(UINT16* data, const char* const source, UINT64 max)
{
	UINT32 value;
	if ( data == nullptr || source == nullptr )
	{
		dbugLog(_FLAS, "called with NULL data or source");
		return false;
	}

	*data = 0;

	if ( *source == 0 )
		return false;

	if ( !isdigit(*source) )
		return false;

	// Constrain to UINT64
	if ( max <= 0 || max > USHRT_MAX )
		max = USHRT_MAX;

	value = (UINT32)atol(source);
	if ( value > max )
		*data = (UINT16)max;
	else if ( value > 0 )
		*data = (UINT16)value;

	return true;
}

bool
wwiiConfigSetFLOAT(FLOAT32* data, const char* const source, UINT64 /*max*/)
{
	if ( data == nullptr || source == nullptr )
	{
		dbugLog(_FLAS, "called with NULL data or source");
		return false;
	}

	*data = 0.0f;

	if ( *source == 0 )
		return false;

	if ( !isdigit(*source) )
		return false;

	(void)sscanf(source, "%f", data);

	return true;
}

bool
wwiiConfigSetSTRING(char* data, const char* const source, UINT64 max)
{
	if ( data == nullptr || source == nullptr )
	{
		dbugLog(_FLAS, "called with NULL data or source");
		return false;
	}

	if ( max == 0 )
	{
		dbugLog(_FLAS, "supplied 0 as string maxlength");
		return false;
	}

	*data = 0;

	if ( *source == 0 )
		return false;

	strsafecpy(data, source, static_cast<size_t>(max));

	return true;
}

bool
wwiiConfigSetBOOL(UCHAR* data, const char* const source, UINT64 /*max*/)
{
	if ( data == nullptr || source == nullptr )
	{
		dbugLog(_FLAS, "called with NULL data or source");
		return false;
	}

	*data = (UCHAR)false;
	if ( *source == 0 )
		return false;

	if ( _isAffirmative(source) )
		*data = (UCHAR)true;

	return true;
}

bool
wwiiConfigSetBYTE(UCHAR* data, const char* const source, UINT64 max)
{
	if ( data == nullptr || source == nullptr )
	{
		dbugLog(_FLAS, "called with NULL data or source");
		return false;
	}

	*data = 0;
	if ( *source == 0 )
		return false;

	if ( !isdigit(*source) )
		return false;

	// Constrain "max" to within a UCHAR
	if ( max <= 0 || max > UCHAR_MAX )
		max = UCHAR_MAX;

	UINT32 value = (UINT32)atol(source);
	if ( value > max )
		*data = (UCHAR)max;
	else if ( value > 0 )
		*data = (UCHAR)value;

	return true;
}

// That's all we provide for now

WWII_COUNTRY
isCountryCode(const char* source)
{
	UINT32 i;
	for ( i = 1; i < MAX_COUNTRIES; i++ )
	{
		if ( strcasecmp(_countryCode[i], source) == 0 )
			return (WWII_COUNTRY) i;
	}
	return WWII_NO_COUNTRY;
}

WWII_COUNTRY
isCountryName(const char* source)
{
	UINT32 i;
	for ( i = 1; i < MAX_COUNTRIES; i++ )
	{
		if ( strcasecmp(_countryName[i], source) == 0 )
			return (WWII_COUNTRY) i;
	}
	return WWII_NO_COUNTRY;
}

WWII_COUNTRY
isCountry(const char* source)
{
	if ( strlen(source) == 2 ) return isCountryCode(source);
	UINT32 i;
	i = isCountryName(source);
	if ( i != WWII_NO_COUNTRY ) return (WWII_COUNTRY) i;
	for ( i = 1; i < MAX_COUNTRIES; ++i )
	{
		if ( strcasecmp(_countryNameAdj[i], source) == 0 )
			return (WWII_COUNTRY) i;
	}
	if ( strncasecmp("brit", source, 4) == 0 )
		return WWII_ENGLAND;
	return WWII_NO_COUNTRY;
}

WWII_SIDE
isSideName(const char* source)
{
	if ( strcasecmp(source, "allied") == 0 ) return ALLIED;
	if ( strcasecmp(source, "axis") == 0 ) return AXIS;
	return WWII_NO_SIDE;
}

