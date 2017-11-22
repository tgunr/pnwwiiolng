#pragma once
// Originating Programmer: Oliver "kfsone" Smith
//
// Provides the global configuration structure and defines
// Reading is provided by dbseWrapper

//	------------------------------	Exposed entities
//tolua_begin

#define MAX_PPOS 20

// Intermission statuses
enum
{
	NOT_INTERMISSION	=	0		//!< Not an intermission
,	NEW_INTERMISSION	=	1		//!< Intermission not started
,	LIVE_INTERMISSION	=	2		//!< Running intermission
};

#pragma pack(push, 1)
struct cfARENA
{
	UCHAR	banCheats;				//!< Should we ban cheat detections.
	UCHAR	checksums;				//!< Are we allowed to enforce checksums?
	UCHAR	hideBuzzIcons;			//!< Should we hide buzzard icons?
	UCHAR	intermission;			//!< Intermission state
	UCHAR	limitedDamage;			//!< True/False: prevent full damage?
	UCHAR	lock;					//!< Arena lock flags
	UCHAR	rebuildAI;				//!< Rebuild AI?
	UCHAR	vehicleRestrictions;	//!< Restrict vehicles by rank?
	UCHAR	kickModHacks;			//!< Do we eject mod-hackers?
	UCHAR	kickSpeedHacks;			//!< Kick detected speedhackers?
	UCHAR	enableIRCCommands;		//!< Enable .commands in the IRC interface
	UINT16	freezeClock;			//!< Lock game-clock to this time
	UINT16	fixedTimeFactor; 		//!< Fix the game-time factor
	UINT16	manualAIRepair;			//!< Allow manual repair of AI
	UINT16	minutesPerAAHour;		//!< Minutes per anti-addiction hour
	UINT32	maxPlayers;				//!< Max players allowed in arena
	FLOAT32	speedHackRatio;			//!< Maximum drift tolerance.
	UINT16	despawnTime_s;			//!< Seconds to wait before despawning
	char	mobileSpawnWaypoint[MAX_HANDLE_SIZE];
	UCHAR	allowDirectSpawn;		//!< Should we allow direct spawning (Multi-Client)
};

struct cfBALANCE
{
	UINT16	personaDelay;			//!< Side-balance delay at persona screen
	UINT16	marginPctg;			//!< Side-balance margin percentage
	UINT16	above;					//!< Side-balance cutoff point
	UINT16	delaysMax;				//!< Maximum the spawn delays can reach
	UINT16	delaysPctg;				//!< Side-balance spawn delays cutoff point
};

struct cfCAPTURE
{
	UINT32	rebuildTime_s;			//!< Seconds for table rebuild
	UINT16	start_m;				//!< Ticks it takes to capture a table
	UINT16	margin_s;				//!< How much error to allow in capture times
	UINT16	tickInterval_s;			//!< Seconds between ticks.
};

struct cfKILLS
{
	UCHAR	deductFriendly;			//!< Deduct friendly fire kill points
	UCHAR	fromBuildings;			//!< Enable kill credit from building destruction
	UINT16	maxValue;				//!< Maximum value of a kill
	UCHAR	notifyKiller;			//!< Tell someone when they get kills
	UCHAR	reportRats;				//!< Anounce Rat kills
	UCHAR	reportDamage;			//!< Report incoming joules of damage?
	UINT16	assistWindow_s;			//!< Damage is an assist if they die within this time.
};

struct cfMISSION
{
	UINT16	timeOut_m;				//!< The time after which missions stop being takeable
//		UINT16	expire_m;			// Factor to decrease rider priority by
	UINT16	leaderAFKTime_m;		//!< Offer mission leadership for grabs after this time
	UINT16	launchCycle_s;			//!< Seconds between Launch buttons
	UINT16	minStartTime_s;			//!< Minimum time before you can spawn
};

struct cfOBJECTIVE
{
	UINT16		clearTime_m;		//!< Minutes to execute a clear request
	UINT16		fallbackNotice_m;	//!< Minutes from fallback placement to execution
	UINT16		gracePeriod_m;		//!< After going over budget, minutes before first warning
	UINT16		holdFallLimit;		//!< hold/fallback capacity
	UINT16		holdFallWindow_m;	//!< Minutes over which the hold/fall rule applies
	UINT16		minLifetime_m;		//!< Minimum number of minutes a capture objective must live for
	UINT16		overrunWindow_m;	//!< Minutes overrun requests are remembered for
	UINT16		requestTime_m;		//!< Minutes to remember objective requests for
	UINT16		warningPeriod_m;	//!< After grace period, minutes before pulling an AO
};

struct cfOIC
{
	// OIC voting/etc windows
	UINT16		nominationVoteTime_s;
	UINT16		offerTime_s;
	UINT16		removalVoteTime_s;
	UINT16		replaceDelay_s;
	UINT16		moveVoteTime_s;
	UINT16		attackVoteTime_s;
	UINT16		withdrawVoteTime_s;
};

struct cfSCORE
{
	UINT16		capperKillBonus;		//!< Bonus points for a kill on a player in a cap bldg.
	UINT16		capture;				//!< Points for a capture.
	UINT16		guardingKillBonus;		//!< Bonus points for kills FROM a cap building.
	UINT16		guardPointPer_m;		//!< Minutes per guard point.
	UINT16		stratDmgInterval_s;		//!< Number of seconds between strat damages to get score.
	UINT16		maxGlobalSharedRank;	//!< Cross-side rank sharing.
	UINT16		maxSideSharedRank;		//!< Cross-country rank sharing within a side.
};

struct cfSQUAD
{
	UCHAR	recruitExpiry;		//!< Expire squad recruits?
	UINT16	electionDuration;	//!< Days a squad election is allowed to run for
	UINT16	electionInterval;	//!< Days between squad elections
	UCHAR	recruiterListMax;	//!< Maximum number of recruiters to list
	UCHAR	recruiterListMin;	//!< At least this many before branching out
	char	defaultSquad[MAX_HANDLE_SIZE];	//!< Default squad to join players to.
};

struct cfSTO
{
	UINT32	damageThreshold[MAX_PPOS];	//!< PPO Damage threshold
	UINT32	damageCapacity[MAX_PPOS];	//!< PPO Damage capacity
	UINT32	placeCooldown_s[MAX_PPOS];	//!< Cooldown from placing one PPO to the next
	UINT32	deathCooldown_s[MAX_PPOS];	//!< Cooldown on replacing a destroyed PPO
	UINT16	hulkLifetime_m;		//!< Hulk lifetime and population limits
	UINT16	hulkLimitPerCell; 	//!< Max hulks per grid cell
	UINT16	hulkLimitPerArea;	//!< Max hulks per group of grid cells
	UINT32	hulkDamageThreshold;	//!< Minimum joules to damage a hulk
	UINT32	hulkDamageCapacity;	//!< Total joules to destroy a hulk
	UINT32	mobileSpawneeValue;	//!< How much is someone spawning thru your MSP worth?
	UINT32	smokeLifetimeOverrideS;	//!< Set non-zero to override smoke lifetime
};

struct cfSUPPLY
{
	UCHAR	enabled;			//!< If false, spawn lists are static at 1
	UCHAR	debug;				//!< If true, produces extra debugging of supply system.
	UCHAR	trickleEnabled;		//!< Whether trickle is enabled or not
	UCHAR	fbsToFrontline;		//!< Brigades open up FBs to frontline towns
	UCHAR	surrenderInf;		//!< Percentage infantry must go below before you can move off-map
	UINT16	listRefresh_s;		//!< Seconds between refreshes of the list on the client
	UINT16	overstockLimit;		//!< Maximum percentage a spawn list can be overstocked (10%, 20% etc)
	UINT16	reserveTimeout_m;	//!< Minutes after which a vhicle reservation (post) times out
	UINT16	trickleSlices;		//!< Maximum slices to deliver supply in
	UINT16	trickleStart_m;		//!< Minutes before trickle begins
	UINT16	rescueTime_m;		//!< How long should a vehicle take to rescue?
	UINT32	resupplyTime_m;		//!< How long should a vehicle take to resupply?
	UINT32	trickleTime_m;		//!< Minutes that complete trickle should take
	UINT32	populationAdjust;	//!< Divisor for population-based rate adjustment
	UCHAR	nomanDepots;		//!< Enable spawning at depots in adjacent friendly towns with a DO
};

struct cfTHROTTLE
{
	UCHAR	enabled;			//!< Whether depot throttles are enabled
	UCHAR	percentage;			//!< What percentage of max supply is throttled
	UCHAR	timeM;				//!< Time in minutes before dethrottle
};

struct cfUPDATE
{
		// Limit how many new vehicles you get per update
	UCHAR	avoidRepeats;		//!< Try to avoid sending vehicles we haven't received updates for.
	UCHAR	countingEnabled;	//!< Enable packing counting
	UCHAR	starvationLimit_s;	//!< Seconds without updates before we kick them
	UINT16	accumDelayLimit_ms;	//!< Maxium Accumulated Delay allowed before considering a player kick
	UINT16	timeAtLimitBeforeKick_s;//!< Number of seconds accumulated delay must remain at maximum before player is kicked
	UCHAR	errorThrottling;	//!< Throttle updates on errors/lag
	UCHAR	visLimitThrottling;	//!< Whether or not to enable vis limit throttling
	UCHAR	defaultUM;			//!< Which update framing mode to use by default
	UINT16	limitNewVehicles;	//!< Limit how many new vehicles we send in a single update
	UINT16	riderBiasing;		//!< De-prioritize riders when building a vis list
	UINT16	sampleGrace_s;		//!< Number of seconds of "grace" to allow before we start counting
								//!< update packets after a counter is created by the host.
	UINT16	samplePeriod_s;		//!< Number of seconds over which we perform packet sampling,
								//!< at the end of a sample all counters get reset and we ignore
								//!< any packets that were marked to be sent before the beginning
								//!< of the new window
	UINT16	sampleBadCycleMax;	//!< How many cycles a counter can be bad in before we finally
								//!< declare it to be illegal
	UINT16	sampleForgive_s;	//!< After this many seconds, we will forgive any bad samples;
								//!< if a player is online for multiple hours, they may trip
								//!< some "bad" samples. At some point, they may even trip
								//!< a bad cycle or two. This is the window during which we
								//!< say "3 strikes and you're out".
	UINT16	sampleIgnore;		//!< How many packets to ignore before we commence counting at all
	UINT16	awsGracePeriod_s;	//!< How long you get from tripping AWS to appearing on it
	UCHAR	awsHighThreshold;	//!< How many planes present to trip high AWS?
};

struct cfWEATHER
{
	UINT16	lockToFog;			//!< Which fog pattern to lock to
	UINT16	lockToPattern;		//!< Which weather pattern to lock to
	UINT16	numFogs;			//!< Number of fog patterns
	UINT16	numPatterns;		//!< Number of weather patterns
	UINT16	chanceOf;			//!< Percentage chance of rainy days (0-100)
};

// Structure that gets populated from the database and then
// communicated between hosts
struct	WWII_CONFIG
{
	bool isArenaLocked(UINT32 level) const { return ( (arena.lock & level) != 0 ); }
	bool isArenaOpen() const { return ( arena.lock == 0 ); }
	bool isIntermission() const { return arena.intermission > 0; }

	UINT32		lastModified;		//!< Last time we got a config

	// Try to ensure the UCHARs are grouped in fours to byte-align
	UCHAR		minEWSTrip;			//!< Minimum players to trip EWS

	UINT32		rpTime;				//!< Seconds per RP uint

	UINT32		factoryPeriod;		//!< Seconds between factory outputs

	UINT16		factoryWarmupTimeM;	//!< Minutes it takes factories to get going

	UINT16		sayFriendlyRange;	//!< Say range to friendlies
	UINT16		sayEnemyRange;		//!< Say range to enemies
	UINT16		yellFriendlyRange;	//!< Yell range to friendlies
	UINT16		yellEnemyRange;		//!< Yell range to enemies
	UINT16		whisperFriendlyRange;	//!< Whisper range to friendlies
	UINT16		whisperEnemyRange;	//!< Whisper range to enemies

	UINT16		mobileSpawnRank;	//!< Minimum rank for mobile spawning

	UINT16		sideLockTime_m;		//!< Duration of sidelock

	UINT32		despawnPromotionTime_s;	//!< Unused
	UINT16		fbRepairIntervalM;	//!< Minutes between first FB self-repair ticks

	UINT64		routDestination[MAX_COUNTRIES];	//!< FullObjID key of off-map destination for brigades by country.

	cfARENA		arena;
	cfBALANCE	balance;
	cfCAPTURE	capture;
	cfKILLS		kills;
	cfMISSION	mission;
	cfOBJECTIVE	objective;
	cfOIC		oic;
	cfSCORE		score;
	cfSQUAD		squad;
	cfSTO		sto;
	cfSUPPLY	supply;
	cfTHROTTLE	throttle;
	cfUPDATE	update;
	cfWEATHER	weather;

	UCHAR       netcode3Enabled;
};
#pragma pack(pop)

extern WWII_CONFIG host_config;

extern const char* _countryName[MAX_COUNTRIES];
extern const char* _countryNameAdj[MAX_COUNTRIES];
extern const char* _sideName[MAX_SIDES];
extern const char* _sideNameAdj[MAX_SIDES];
extern const char* _countryCode[MAX_COUNTRIES];

extern WWII_COUNTRY isCountryName(const char* source);
extern WWII_COUNTRY isCountryCode(const char* source);
extern WWII_COUNTRY isCountry(const char* source);	// Name, Code or Adjective
extern WWII_SIDE isSideName(const char* source);

//tolua_end
//	------------------------------	Non-exposed entities

extern bool wwiiConfigSetINT(UINT32* data, const char* const source, UINT64 max);
extern bool wwiiConfigSetSINT(UINT16* data, const char* const source, UINT64 max);
extern bool wwiiConfigSetFLOAT(FLOAT32* data, const char* const source, UINT64 max);
extern bool wwiiConfigSetSTRING(SCHAR* data, const char* const source, UINT64 max);
extern bool wwiiConfigSetBOOL(UCHAR* data, const char* const source, UINT64 max);
extern bool wwiiConfigSetBYTE(UCHAR* data, const char* const source, UINT64 max);

