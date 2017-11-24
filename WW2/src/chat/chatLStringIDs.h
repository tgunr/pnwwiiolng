#pragma once
// Created: Tue Apr 12 20:52:51 CDT 2005
//
// Localized String IDs
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

// ID's for localised strings
enum LSTRING_ID
{
	LSTR_TEXT_START = -1000			// start of a long text string.
,	LSTR_TEXT_LINE					// text string element separated by \n
,	LSTR_TEXT						// ongoing text string element
,	LSTR_TEXT_END					// end of a long text string.
,	LSTR_NONE	=	-1
	//// --- 0 ----
,	LSTR_HQ			=	0
,	LSTR_NOTE
,	LSTR_SYSTEM
,	LSTR_WARNING
,	LSTR_AFK						// You are now afk
,	LSTR_NOT_AFK					// You are no-longer afk
,	LSTR_IS_AFK					// %s is afk
,	LSTR_IS_AFK_MSG				// %s is afk: %s
,	LSTR_BOOT_REMINDER			// You have boot channel off
,	LSTR_USER_UNAVAILABLE		// User %s unavailable

// 10
,	LSTR_HAVE_GRABBED			// You have grabbed by %s
,	LSTR_BEEN_GRABBED			// You have been grabbed %s
,	LSTR_BEEN_GRABBED_MSG		// You have been grabbed by %s: %s
,	LSTR_HAVE_UNGRABBED			// You have ungrabbed %s
,	LSTR_BEEN_UNGRABBED			// You have been ungrabbed by %s
,	LSTR_HAVE_DOGGED				// You have Duty Officer grabbed %s.
,	LSTR_BEEN_DOGGED				// You have been grabbed by Duty Officer %s.
,	LSTR_BEEN_DOGGED_MSG			// You have been grabbed by Duty Officer %s: %s
,	LSTR_HAVE_UNDOGGED			// You have un-grabbed %s.
,	LSTR_BEEN_UNDOGGED			// You have been released by Duty Officer %s.

// 20
,	LSTR_GENERIC					// %s
,	LSTR_BEEN_KICKED				// The Game Manager has killed you.
,	LSTR_BEEN_KICKED_MSG			// Kicked by Game Manager: %s
,	LSTR_BOOTCAMP_SUBS			// Subscribed to bootcamp channel
,	LSTR_BOOTCAMP_REQ_SIDE		// Must pick a side before tuning
,	LSTR_HQ_SUBS					// Subscribed to HQ channel
,	LSTR_HQ_UNSUBS				// Unsubscribed from HQ channel
,	LSTR_UNIGNORING				// Unignoring %s
,	LSTR_IGNORING				// Ignoring %s
,	LSTR_GM						// GM

// 30
,	LSTR_GM_ON					// GM mode on
,	LSTR_GM_OFF					// GM mode off
,	LSTR_HAVE_KICKED_MSG		// Have kicked %s - %s.
,	LSTR_HAVE_KICKED			// Have kicked %s - no message.
,	LSTR_PARTY_UNAVAILABLE		// Message not sent, ungrabbing
,	LSTR_CHANNEL_UNAVAILABLE	// Channel Unvailable
,	LSTR_FEATURE_LOCKED			// ".%s" only available once every %s minutes
,	LSTR_REPORTING				// %s> %s
,	LSTR_NOW_SPARE				// Was report help
,	LSTR_REPORTED				// Reported: %s

// 40
,	LSTR_REPORT					// REPORT
,	LSTR_COMMANDS_LOGGED		// All GM commands are logged.
,	LSTR_SELECT_FACILITY		// %s: Must select a facility first
,	LSTR_AXIS_OFFICERS			// AXIS Officers
,	LSTR_ALLIED_OFFICERS		// ALLIED Officers
,	LSTR_NO_MATCHES				// "No Matches"
,	LSTR_AXIS_HC				// GHC
,	LSTR_ALLIED_HC				// AHC
,	LSTR_LABELLED_GENERIC		// %s: %s
,	LSTR_BETA					// BETA

// 50
,	LSTR_SELECT_SIDE			// You must select a side first.
,	LSTR_NOBODY_GRABBED			// You don't have anyone .grab'd?
,	LSTR_USAGE					// USAGE
,	LSTR_DETAIL					// DETAIL (for dot commands)
,	LSTR_AFK_USAGE				// .Dot command usage/detail strings
,	LSTR_AFK_DETAIL
,	LSTR_ALL_USAGE,		LSTR_ALL_DETAIL
,	LSTR_ALLIED_USAGE,	LSTR_ALLIED_DETAIL

// 60
,	LSTR_AXIS_USAGE,	LSTR_AXIS_DETAIL
,	LSTR_CRS_USAGE,		LSTR_CRS_DETAIL
,	LSTR_GM_USAGE,		LSTR_GM_DETAIL
,	LSTR_GMMODE_USAGE,	LSTR_GMMODE_DETAIL
,	LSTR_GRAB_USAGE,	LSTR_GRAB_DETAIL

// 70
,	LSTR_HC_STAFF_USAGE, LSTR_HC_STAFF_DETAIL
,	LSTR_HC_USAGE,		LSTR_HC_DETAIL
,	LSTR_HCOFF_USAGE,	LSTR_HCOFF_DETAIL
,	LSTR_HCON_USAGE,	LSTR_HCON_DETAIL
,	LSTR_HELP_USAGE,	LSTR_HELP_DETAIL

// 80
,	LSTR_HQOFF_USAGE,	LSTR_HQOFF_DETAIL
,	LSTR_HQON_USAGE,	LSTR_HQON_DETAIL
,	LSTR_KICK_USAGE,	LSTR_KICK_DETAIL
,	LSTR_KICKALL_USAGE,	LSTR_KICKALL_DETAIL
,	LSTR_M_USAGE,		LSTR_M_DETAIL

// 90
,	LSTR_N_USAGE,		LSTR_N_DETAIL
,	LSTR_PINFO_USAGE,	LSTR_PINFO_DETAIL
,	LSTR_REPORT_USAGE,	LSTR_REPORT_DETAIL
,	LSTR_ROSTER_USAGE,	LSTR_ROSTER_DETAIL
,	LSTR_SYS_USAGE,		LSTR_SYS_DETAIL

// 100
,	LSTR_SYSMOTD_STAFF_USAGE,	LSTR_SYSMOTD_STAFF_DETAIL
,	LSTR_SYSMOTD_USAGE,	LSTR_SYSMOTD_DETAIL
,	LSTR_UNGRAB_USAGE,	LSTR_UNGRAB_DETAIL
,	LSTR_WARN_USAGE,	LSTR_WARN_DETAIL
,	LSTR_WARNALL_USAGE,	LSTR_WARNALL_DETAIL

// 110
,	LSTR_WHO_USAGE,		LSTR_WHO_DETAIL
,	LSTR_CPDUMP_USAGE,	LSTR_CPDUMP_DETAIL
,	LSTR_CPINFO_USAGE,	LSTR_CPINFO_DETAIL
,	LSTR_OWN_USAGE,		LSTR_OWN_DETAIL
,	LSTR_HC_OFF
,	LSTR_HC_ON

// 120
,	LSTR_GRABBED_ALREADY			// Can't grab while grabbed
,	LSTR_GRABBED_RELEASING			// Releasing current .grab
,	LSTR_NO_SUCH_COMMAND			// No such .comand: %s
,	LSTR_NO_SQUAD					// You aren't currently in a squad
,	LSTR_FBS_USAGE,		LSTR_FBS_DETAIL
,	LSTR_SPECIFY_TOWN
,	LSTR_DELAY_USAGE,	LSTR_DELAY_DETAIL
,	LSTR_FALLBACK_USAGE

// 130
,	LSTR_FALLBACK_DETAIL
,	LSTR_HOLD_USAGE,	LSTR_HOLD_DETAIL
,	LSTR_RESYNC_USAGE,	LSTR_RESYNC_DETAIL
,	LSTR_FIND_USAGE,	LSTR_FIND_DETAIL
,	LSTR_FRIENDS_USAGE,	LSTR_FRIENDS_DETAIL
,	LSTR_OVERRUN_USAGE

// 140
,	LSTR_OVERRUN_DETAIL
,	LSTR_OVERRUN_EXPIRED			// OVERRUN report has expired or already been dealt with
,	LSTR_CMD_SUCCESS				// %s at %s completed.
,	LSTR_CMD_ALLOWED				// %s at %s requested.
,	LSTR_ENEMY_OWNS					// %s belongs to the enemy.
,	LSTR_MUST_OAC					// Must both own and control %s to perform %s.
,	LSTR_DONT_OWN_ABS				// We don't hold any armybases in %s.
,	LSTR_ABS_NOT_LINKED				// No supply links to armybases in %s.
,	LSTR_NO_CONTENTION				// %s is not contested.
,	LSTR_ALREADY_REPORTING			// %s is already reporting %s.

// 150
,	LSTR_OTHER_ORDERS				// %s already has other orders.
,	LSTR_CMD_FAILED					// '%s' failed at %s.
,	LSTR_ALREADY_VOTED				// You have already voted on that item.
,	LSTR_RAISING_OVERRUN			// Raising the alarm: Base overrun!
,	LSTR_GO_HOLD					// HOLD over for %s approved.
,	LSTR_REQ_APPROVED				// Requested '%s' at %s.
,	LSTR_MUST_ISSUE_FROM			// Orders for %s must be issued from %s.
,	LSTR_NO_SUCH_ORDER				// '%s' does not exist or has expired.
,	LSTR_ORDER_IN_PROGRESS			// %s: '%s' order is being issued.
,	LSTR_NO_ATTACK					// %s not reporting under attack

// 160
,	LSTR_NO_OVERRUN					// %s not reporting overrun
,	LSTR_SEL_TARGET					// Please select a target on the map first.
,	LSTR_TARGET_CP					// Target must be a CP (town or bridge).
,	LSTR_DEFEND_TOWN				// Defend objectives can only be placed for bridges.
,	LSTR_NOT_FRONT_TOWN				// %s: is not on the front-line.
,	LSTR_OUT_OF_OBJECTIVES			// All available objectives are currently placed.
,	LSTR_PREVIOUS_OBJ_LIVES			// Your previous request at %s has not expired yet.
,	LSTR_NO_OBJECTIVE				// No objective assigned to %s.
,	LSTR_FRIENDLY_TOWN				// %s: is not enemy held.
,	LSTR_CAUTION					// Caution symbol

// 170
,	LSTR_CAPTURE_ORDERS				// Orders issued to take %s
,	LSTR_AUTODEFENSE_ORDERS			// Orders issued to defend %s
,	LSTR_OBJECTIVE_WITHDRAWING		// %s objective is being withdrawn
,	LSTR_ENEMY_WITHDRAWING			// Enemy forces being withdrawn from %s
,	LSTR_WITHDRAW_REMINDER			// %s objective being withdrawn! RTB!
,	LSTR_OBJECTIVE_WITHDRAWN		// %s objective has been withdrawn
,	LSTR_NOWHO_USAGE, LSTR_NOWHO_DETAIL
,	LSTR_NOWHO_OFF					// Now visible to .who
,	LSTR_NOWHO_ON					// Now hidden from .who

// 180
,	LSTR_NO_CHANNELS_FOR_AREA		// No channels available for area chat.
,	LSTR_OBJECTIVES_USAGE, LSTR_OBJECTIVES_DETAIL
,	LSTR_NOECHAT_USAGE, LSTR_NOECHAT_DETAIL
,	LSTR_NOECHAT_ON, LSTR_NOECHAT_OFF
,	LSTR_AWAITING_CONFIRMATION		// Acknowledged - waiting for confirmation
,	LSTR_ATTACK_USAGE, LSTR_ATTACK_DETAIL

// 190
,	LSTR_DEFEND_USAGE, LSTR_DEFEND_DETAIL
,	LSTR_CLEAR_USAGE, LSTR_CLEAR_DETAIL
,	LSTR_KICKING_ALL				// Sending .kick to ALL players
,	LSTR_AREA_ONLY_WHILE_SPAWNED	// Area chat only available while spawned
,	LSTR_NO_ACTIVE_FIREBASES		// No active firebases.
,	LSTR_ALLIED						// ALLIED (with HQ + ALLIED icons)
,	LSTR_AXIS						// AXIS (with HQ + AXIS icons)
,	LSTR_ATTACK_ORDERS				// Orders to attack (e.g. bridge)

// 200
,	LSTR_DEFEND_ORDERS				// Bridge defense orders
,	LSTR_CLEAR_TOO_SOON				// Cannot cancel %s attack for %s.
,	LSTR_SIGNED_ON					// %s has logged in.
,	LSTR_SETRANK_USAGE
,	LSTR_SETRANK_DETAIL
,	LSTR_RANK_SET					// Rank set to %s. Logout
,	LSTR_INVALID_RANK
,	LSTR_NO_ACTIVE_MISSIONS			// No active missions here
,	LSTR_WHISPER_USAGE, LSTR_WHISPER_DETAIL

// 210
,	LSTR_YELL_USAGE, LSTR_YELL_DETAIL
,	LSTR_NOT_ACTIVE_HC
,	LSTR_ACTION_LIMITED
,	LSTR_HCMOTD_USAGE, LSTR_HCMOTD_DETAIL
,	LSTR_FRIENDS_LIST_EMPTY
,	LSTR_FRIENDS_ERROR
,	LSTR_OFFLINE_LIST				// Offline: %s
,	LSTR_ONLINE_LIST				// Online: %s

// 220
,	LSTR_TX_NO_BRIGADE				// No brigade, no brigade channel
,	LSTR_TX_NO_MISSION				// No mission, no mission channel
,	LSTR_TX_NO_MISSION_CITYS		// No mission, no city channel
,	LSTR_TX_NO_MISSION_ORIGIN		// No mission, no origin channel
,	LSTR_TX_NO_MISSION_OBJECTIVE	// No mission, no objective channel
,	LSTR_TX_NO_SQUAD				// No squad, no squad channel
,	LSTR_TX_SQUAD_UNAVAILABLE		// No persona, no squad channel
,	LSTR_TX_CH_RX_ONLY				// %s channel is RX only
,	LSTR_TX_NO_SIDE					// No side, no side channels
,	LSTR_HELP_CHANNEL_GREETING		// Welcome to the help channel.

// 230
,	LSTR_LOCAL_TIME					// Local Time: %s
,	LSTR_SERVER_TIME				// Server Time: %s
,	LSTR_INFO_BRIDGE_ATTACK			// "%s: Attack/Destroy"
,	LSTR_INFO_BRIDGE_DEFEND			// "%s: Defend/Repair"
,	LSTR_INFO_BRIDGE_OUT			// "Bridge is currently out"
,	LSTR_INFO_CITY_ATTACK			// "%s: Attack/Capture"
,	LSTR_INFO_CITY_DEFEND_HAAC		// "%s: Defend - Hold at all costs"
,	LSTR_INFO_CITY_DEFEND_FALL		// "%s: Defend from fallback positions"
,	LSTR_INFO_CITY_DEFEND			// "%s: Defend against capture"
,	LSTR_INFO_CITY_CONTESTED		// "%s is under contention"

// 240
,	LSTR_INFO_NEW_OBJECTIVE			// "Newly issued objective"
,	LSTR_INFO_OBJECTIVE_PLACED		// Objectived was placed %sh, %sm ago"
,	LSTR_MOTD						// "MOTD"
,	LSTR_HOME_USAGE, LSTR_HOME_DETAIL
,	LSTR_NO_BRIGADE					// Must be in a brigade to use that command
,	LSTR_NOT_BRIGADE_LEADER			// Must be a brigade leader to use that command
,	LSTR_GAME_COMING_DOWN_IN		// Game coming down in %s
,	LSTR_ORDERS_USAGE, LSTR_ORDERS_DETAIL

// 250
,	LSTR_NOT_ON_MISSION
,	LSTR_NOT_MISSION_LEADER
,	LSTR_NO_STANDING_ORDERS
,	LSTR_ORDERS
,	LSTR_ORDERS_CLEARED
,	LSTR_RECONNECTED
,	LSTR_NO_TRAINERS				// No trainers online.
,	LSTR_TRAINERS_USAGE, LSTR_TRAINERS_DETAIL
,	LSTR_MISSION_CLOSED

// 260
,	LSTR_UNKNOWN_COMMAND			// Unknown command '%s'
,	LSTR_HCLIST_USAGE, LSTR_HCLIST_DETAIL
,	LSTR_SQUAD_DISBANDED			// %s has disbanded %s.
,	LSTR_SQUAD_ADDITION				// %s has joined %s.
,	LSTR_CO_RESIGNED				// %s has handed CO to %s.
,	LSTR_XO_RESIGNED				// %s has handed XO to %s.
,	LSTR_PROMO_TO_MEMBER			// *%s has promoted %s to member.
,	LSTR_PROMO_TO_STAFFER			// *%s has promoted %s to staffer.
,	LSTR_PROMO_TO_XO				// *%s has promoted %s to XO.

// 270
,	LSTR_PROMO_TO_CO				// *%s has promoted %s to CO.
,	LSTR_DEMO_TO_MEMBER				// *%s has demoted %s to member.
,	LSTR_DEMO_TO_STAFFER			// *%s has demoted %s to staffer.
,	LSTR_DEMO_TO_XO					// *%s has demoted %s to XO.
,	LSTR_SERVER_SHUTDOWN_NOW		// *GAME SERVER COMING DOWN: NOW
,	LSTR_DECLINED_INVITE			// %s has declined your squad invite.
,	LSTR_ACCEPTED_INVITE			// %s has accepted your squad invite.
,	LSTR_USER_ALREADY_IN_SQUAD		// %s is already in a squad.
,	LSTR_SQUAD_KICK					// %s has been removed from the squad by %s.
,	LSTR_SQUAD_LEAVER				// %s has left the squad.

// 280
,	LSTR_SQUAD						// SQUAD
,	LSTR_SQUAD_ELECTION				// CO election in progress. Votes: %u. Candidates: %s
,	LSTR_SQUAD_NOT_VOTED			// You haven't voted yet
,	LSTR_SQUAD_VOTE_FAIL			// Election failed
,	LSTR_SQUAD_VOTE_SAME			// Voted for same CO
,	LSTR_SQUAD_VOTE_NEW				// Voted for new CO
,	LSTR_SQUAD_VOTE_FAIL_LEAVER		// Vote failed, nominee left squad
,	LSTR_VOTECO_USAGE, LSTR_VOTECO_DETAIL
,	LSTR_PLAYER_NOT_IN_SQUAD		// Player %s is not in your squad.

// 290
,	LSTR_SQUAD_VOTE_CLOSED			// Squad has already voted. Voting next allowed in %s.
,	LSTR_SQUAD_ALREADY_VOTED
,	LSTR_SQUAD_VOTE_REGISTERED
,	LSTR_NOT_IGNORING_ANYONE
,	LSTR_PLAYERS_IGNORED
,	LSTR_INVALID_CALLSIGN			// Invalid callsign: "%s".
,	LSTR_ALREADY_IGNORING			// Already ignoring %s.
,	LSTR_EXPECTED_DOWNTIME			// *Expected downtime: %s.
,	LSTR_MISSION
,	LSTR_MOBILE_SPAWN_AVAILABLE		// %s has deployed a mobile spawn for this mission.

// 300
,	LSTR_MOBILE_SPAWN_UNAVAILABLE	// mobile spawn is no-longer available.
,	LSTR_SQUAD_NOT_VOTING			// Your squad is not currently running a vote.
,	LSTR_VOTING_ALREADY_ENABLED		// Voting is already enabled.
,	LSTR_VOTING_ALREADY_DISABLED	// Voting is already disabled.
,	LSTR_CO_ENABLED_ELECTIONS		// %s has enabled Squad-CO elections for %s.
,	LSTR_CO_DISABLED_ELECTIONS		// %s has disabled Squad-CO elections for %s.
,	LSTR_VOTING_DISABLED			// Elections have been disabled by the CO.
,	LSTR_MOVEBRIGADE_USAGE, LSTR_MOVEBRIGADE_DETAIL
,	LSTR_MOVEDIVISION_USAGE

// 310
, 	LSTR_MOVEDIVISION_DETAIL
,	LSTR_UNIT_ALREADY_AT			// %s is already at %s.
,	LSTR_INTERNAL_ERROR				// Unknown error
,	LSTR_UNIT_MOVE_FAILED			// Couldn't move unit
,	LSTR_INVALID_TARGET				// Invalid destination for this unit (%s).
,	LSTR_UNIT_MOVE_DELAY			// Cannot move this unit for %s
,	LSTR_NEW_LEADER					// %s is now leading the mission.
,	LSTR_MAKELEADER_USAGE, LSTR_MAKELEADER_DETAIL
,	LSTR_NOT_ON_SAME_MISSION

// 320
,	LSTR_CANT_VERB_SELF				// Can't %s yourself.
,	LSTR_NEWLEADER_1
,	LSTR_NEWLEADER_2
,	LSTR_NEWLEADER_3
,	LSTR_NEWLEADER_4
,	LSTR_NEWLEADER_EGG
,	LSTR_HQU_ONE_LINK
,	LSTR_HQU_FAILED_CONTACT
,	LSTR_HQU_FAILED_ABS
,	LSTR_HQU_FAILED_NAVAL

// 330
,	LSTR_HQU_FAILED_AIR
,	LSTR_HQU_FAILED_FRONTLINE
,	LSTR_NEWLEADER_5
,	LSTR_BACKUP_LEADER_LEFT
,	LSTR_BACKUP_LEADER_SET
,	LSTR_BACKUPLEADER_USAGE, LSTR_BACKUPLEADER_DETAIL
,	LSTR_NEWLEADER_6
,	LSTR_LEADER_USAGE, LSTR_LEADER_DETAIL

// 340
,	LSTR_LEADER_STATE_ON			// You will accept automatic leader promotes
,	LSTR_LEADER_STATE_OFF			// You won't accept automatic leader promotes
,	LSTR_NOT_ACCEPTING				// %s is not accepting leader promotes.
,	LSTR_UMS_KILLED
,	LSTR_HELP_CHANNEL_NOTE			// Reminder about using the help channel
,	LSTR_SQUAD_RENAMED				// GM has renamed your squad %s
,	LSTR_IGNORE_ERROR				// Error accessing ignore list. Try again shortly.
,	LSTR_FRIEND_ADDED				// '%s' added to friends
,	LSTR_FRIEND_REMOVED				// '%s' removed from friends
,	LSTR_ALREADY_FRIENDS			// '%s' already in your friends list

// 350
,	LSTR_FRIENDS_LIST_FULL			// Your friends list is full, remove someone first.
,	LSTR_IGNORE_LIST_FULL			// Your ignore list is full, remove someone first.
,	LSTR_SQUAD_RECRUITING_ENABLED	// Recruiting has been enabled by %s.
,	LSTR_SQUAD_RECRUITING_DISABLED	// Recruiting has been disabled by %s.
,	LSTR_SQUAD_VOTING_ENABLED		// Voting has been enabled by %s.
,	LSTR_SQUAD_VOTING_DISABLED		// Voting has been disabled by %s.
,	LSTR_RECRUIT_LOGGED_OFF			// Your recruit '%s' has logged out
,	LSTR_RECRUIT_CHANGED_SIDE		// Your recruit '%s' has changed sides
,	LSTR_RECRUITER_UNAVAILABLE		// Your recruiter '%s' is no-longer available/logged out
,	LSTR_SQUAD_RECRUITED_BY_RECRUITER	// {recruit} has been recruited to your squad by {recruiter}!

// 360
,	LSTR_SQUAD_REMOVED_LAPSED		// Your membership of '%s' has expired (you were not promoted)
,	LSTR_SQUAD_RECRUIT_LAPSED		// '%s' has been removed from the squad because they were not promoted
,	LSTR_TAKELEAD_USAGE, LSTR_TAKELEAD_DETAIL
,	LSTR_ALREADY_LEADER				// You are already mission leader.
,	LSTR_NO_AFK_TAKELEAD			// You can't takelead while you are AFK
,	LSTR_USURPED					// Your lead has been usurped.
,	LSTR_USURPING					// You are usurping leader
,	LSTR_CANNOT_TAKELEAD			// You cannot takelead from %s for %s.
,	LSTR_MLIST_USAGE

// 370
, 	LSTR_MLIST_DETAIL
,	LSTR_TIME_USAGE, LSTR_TIME_DETAIL
,	LSTR_IGNORE_USAGE, LSTR_IGNORE_DETAIL
,	LSTR_STATUS_USAGE, LSTR_STATUS_DETAIL
,	LSTR_VOTING_USAGE, LSTR_VOTING_DETAIL
,	LSTR_NO_FALLBACK_UNIT			// '%s' cannot fallback: not a brigade.

// 380
,	LSTR_NO_FALLBACK_DEST			// '%s' cannot fallback: no fallback destination.
,	LSTR_BASE_OVERRUN				// '%s' is reporting overrun
,	LSTR_FALLBACK_ISSUED			// '%s' issued fallback
,	LSTR_RESERVED_AT				// %s reserved as %s.
,	LSTR_STUCK_UNITS				// Some units at %s cannot /%s.
,	LSTR_ANTI_ADDICTION_HOUR1		// 1st hour
,	LSTR_ANTI_ADDICTION_HOUR2		// 2nd hour
,	LSTR_ANTI_ADDICTION_HOUR3		// 3rd hour
,	LSTR_ANTI_ADDICTION_TIRED1		// 4th hour
,	LSTR_ANTI_ADDICTION_TIRED2		// 5th hour

// 390
,	LSTR_ANTI_ADDICTION_PENALTY		// 6th hour (penalty time)
,	LSTR_ANTI_ADDICTION_PREFIX		// Prefix for anti-addiction messages.
,	LSTR_HQU_FAILED_NFL				// Can't move a unit into a non-factory-linked pocket
,	LSTR_HQU_FAILED_OFFMAP			// Only army units can move off-map
,	LSTR_HQU_FAILED_OFFMAP_NP		// Cannot move from Factory Link to off-map
,	LSTR_HQU_FAILED_PREM_SURDR		// Cannot surrender prematurely (move off-map above %s%% inf)
,	LSTR_HQU_FAILED_ENEMY_DEPOT		// Must control the depot at both ends of the link
,	LSTR_HQU_FAILED_ENEMY_FB		// FB blocking movement
,	LSTR_ERROR_VERB_SUBJECT			// There was a problem trying to '%s' player %s.
,	LSTR_ACCOUNT_SUSPENDED_NOTICE	// Your account has been suspended (banned) for %s! <%s>

// 400
,	LSTR_LIMITED_DAMAGE_ENABLED		// All damage is limited to 1 joule of damage.
,	LSTR_CRS_MOTD					// [CRS:MOTD] %s
,	LSTR_GM_MOTD					// [GM:MOTD] %s
,	LSTR_CANT_START_SOFTCAP_NOW		// Not enough hardcap AOs to start a softcap.
,	LSTR_CANT_ISSUE_HF_ORDERS_FOR	// Can't issue hold/fallback orders for another %s.
,	LSTR_FALLBACK_SURRENDER_IN		// %s fallback in %s.
,	LSTR_HOLDFALL_ORDER_FAILED		// '%s' order failed to execute.
,	LSTR_NUKEPERSONAS_USAGE,	LSTR_NUKEPERSONAS_DETAIL
,	LSTR_PERSONAS_NUKED				// You asked, we deleted.

// 410
,	LSTR_RESETSTRAT_USAGE,	LSTR_RESETSTRAT_DETAIL
,	LSTR_RESETSTRAT_5MINUTE_LIMIT	// Cannot restart strat more than once every 5 minutes.
,	LSTR_RESETSTRAT_ACK			// Acknowledge you typed /restartstrat but so must someone else
,	LSTR_RESETSTRAT_SYS			// SYSTEM: %s typed /restartstrat, need confirmation from another player.
,	LSTR_RESETSTRAT_SELF_APPROVAL	// You can't approve your own restartstrat.
,	LSTR_RESETSTRAT_APPROVED		// '%s' has approved a strat server restart.
,	LSTR_BUZZARDSPAWN_USAGE,	LSTR_BUZZARDSPAWN_DETAIL
,	LSTR_ALREADY_SPAWNED			// Can't do that while spawned.

// 420
,	LSTR_PLAYER_NOT_SPAWNED_YET		// Player '%s' hasn't spawned yet during this session.
,	LSTR_NO_HELP					// No help for that command.
,	LSTR_INVALID_TIME				// Invalid time: %s
,	LSTR_TRAINER_LIST				// Trainers: %s

,	LSTR_UM_USAGE,				LSTR_UM_DETAIL
,	LSTR_INVALID_UM					// '%s' is not a valid /um setting.

,	LSTR_MINIMUM_5MINUTE_BAN		// Can't ban someone for less than 5 minutes.
,	LSTR_KICKBAN_PLAYER				// Kicking '%s' from server.
,	LSTR_BAN_ONLINE_PLAYER			// '%s' is currently online. Use '/kick' to disconnect them.

// 430
,	LSTR_BAN_USAGE,				LSTR_BAN_DETAIL
,	LSTR_BUMP_USAGE,			LSTR_BUMP_DETAIL

,	LSTR_ONLY_WHILE_BUZZARD			// Sorry, you can only do that while flying a buzzard.
,	LSTR_PLAYER_NOT_SPAWNED_NOW		// Player '%s' is not currently spawned.
,	LSTR_CANT_BUMP_BUZZARD			// You can't bump another buzzard
,	LSTR_BUMP_75M_LIMIT				// Must be within 75m of the target.
,	LSTR_GM_MOVED_YOU				// You have been forcibly moved by a Game Manager: %s.
,	LSTR_DESPAWNALL_USAGE

// 440
,	LSTR_DESPAWNALL_DETAIL
,	LSTR_DESPAWN_YOU				// Individual despawn
,	LSTR_DESPAWN_ALL				// All players being despawned

,	LSTR_KICKBAN_USAGE,			LSTR_KICKBAN_DETAIL

,	LSTR_LASTCAP_USAGE,			LSTR_LASTCAP_DETAIL
,	LSTR_LASTCAP_NO_MATCH			// No captures matching that query.
,	LSTR_DB_QUERY_FAILED			// '%s' database query failed. Please try again later.

,	LSTR_LASTDEATH_USAGE

// 450
,	LSTR_LASTDEATH_DETAIL
,	LSTR_LAST_DEATHS				// Last 5 deaths of '%s'
,	LSTR_MAKECO_USAGE,			LSTR_MAKECO_DETAIL
,	LSTR_PLAYER_NOT_IN_A_SQUAD		// Player '%s' is not currently in a squad.
,	LSTR_PLAYER_ALREADY_CO			// Player '%s' is already the Commanding Officer of '%s' squad.
,	LSTR_SQUAD_NOT_ONLINE			// Squad '%s' not loaded (nobody online)
,	LSTR_SQUAD_CO_ONLINE			// Squad %s CO %s is online.
,	LSTR_MAKECO_USE_FORCE			// Squad has a CO, use -force to promote.
,	LSTR_SQUAD_MAKECO_APPOINTED		// %s has been appointed CO of %s squad by a game manager.

// 460
,	LSTR_MAKECO_APPOINTED			// You have appointed %s as the Commanding officer of %s squad.
,	LSTR_ALL_CHAT_PERMS_RELOADED	// All chat permissions reloaded.
,	LSTR_PLAYER_CHAT_PERMS_RELOADED	// Player '%s' chat permissions reloaded.
,	LSTR_SHUTDOWN_STOP_USAGE,	LSTR_SHUTDOWN_STOP_DETAIL
,	LSTR_SHUTDOWN_IN_USAGE,		LSTR_SHUTDOWN_IN_USAGE1, LSTR_SHUTDOWN_IN_USAGE2
,	LSTR_SHUTDOWN_DOWNTIME_USAGE, LSTR_SHUTDOWN_DOWNTIME_DETAIL

// 470
,	LSTR_UNKNOWN_SHUTDOWN_COMMAND	// Unrecognized /shutdown subcommand '%s'.
,	LSTR_SHUTDOWN_WARNING,	LSTR_SHUTDOWN_USAGE, LSTR_SHUTDOWN_DETAIL
,	LSTR_SHUTDOWN_LIMITS			// Shutdown must be between %s minutes and %s hours.
,	LSTR_SHUTDOWN_SCHEDULED			// Shutdown scheduled for %s.
,	LSTR_SHUTDOWN_ANNOUNCEMENT		// [*] Game Server will be coming down in %s: %s. [*]
,	LSTR_SNOOP_USAGE,			LSTR_SNOOP_DETAIL
,	LSTR_NOT_SNOOPING				// You aren't %sing on anyone.

// 480
,	LSTR_CURRENT_SNOOP				// You are currently /%sing on %s.
,	LSTR_USE_SNOOP_OFF				// Use '/%s off' first to end current snoop on %s.
,	LSTR_NO_SNOOP_RATS				// Can't snoop on staff.
,	LSTR_PNOTE_USAGE,			LSTR_PNOTE_DETAIL

,	LSTR_NO_PNOTES_FOUND			// No notes found on %s's record.
,	LSTR_PNOTE_TOO_LONG				// Invalid note: cannot be more than 399 bytes long.
,	LSTR_WARN_ALL_PLAYERS			// ALL PLAYERS: %s
,	LSTR_UNIT_HAS_NO_HOME			// %s: Unassigned.
,	LSTR_LASTMOVE_USAGE

// 490
,	LSTR_LASTMOVE_DETAIL
,	LSTR_NO_MOVES_FOUND
,	LSTR_LISTCOS_USAGE,			LSTR_LISTCOS_DETAIL
,	LSTR_LISTSQUAD_USAGE
,	LSTR_LISTSQUAD_DETAIL
,	LSTR_NO_SUCH_SQUAD				// Squad %s doesn't exist.
,	LSTR_ARENALOCK_USAGE,		LSTR_ARENALOCK_DETAIL
,	LSTR_DUMPRESUPLY_USAGE

// 500
,	LSTR_DUMPRESUPLY_DETAIL
,	LSTR_PWNCP_USAGE,			LSTR_PWNCP_DETAIL
,	LSTR_RDP_USAGE,				LSTR_RDP_DETAIL

,	LSTR_REDEPLOY_USAGE,		LSTR_REDEPLOY_DETAIL
,	LSTR_SETTIME_USAGE,			LSTR_SETTIME_DETAIL
,	LSTR_NEXTMOVE_USAGE

// 510
,	LSTR_NEXTMOVE_DETAIL
,	LSTR_RENAMESQUAD_USAGE,		LSTR_RENAMESQUAD_DETAIL
,	LSTR_SQUAD_ALREADY_EXISTS		// Squad '%s' already exists.
,	LSTR_INVALID_SQUAD_NAME			// '%s' is not a valid name for a squad.

,	LSTR_SQUAD_OLD_AND_NEW_SAME		// Old and new name are the same.
,	LSTR_IGNORE_SYSTEM1				// Can't ignore system
,	LSTR_IGNORE_SYSTEM2				// Can't ignore system
,	LSTR_FRIEND_SYSTEM				// Can't /friend system
,	LSTR_FRIEND_IGNORE_RDP			// Can't %s RDP.

// 520
,	LSTR_SOCIAL_LIMIT_OF_10			// Limit of 10 names at a time.
,	LSTR_GRIDSNAPSHOT_USAGE,	LSTR_GRIDSNAPSHOT_DETAIL
,	LSTR_SHUTDOWN_CANCELLED			// Game server shutdown has been cancelled!
,	LSTR_SHUTDOWN_DELAYED			// Game server shutdown delayed.
,	LSTR_SHUTDOWN_ADVANCED			// Game server shutdown has been advanced by %s.
,	LSTR_SHUTDOWN_NOW				// [X] GAME SERVER COMING DOWN NOW: You have %s seconds to despawn [X]
,	LSTR_MOTD_STRING				// MOTD: %s
,	LSTR_LISTOIC_USAGE,			LSTR_LISTOIC_DETAIL

// 530
,	LSTR_NO_OICS					// No brigades have active OICs at this time.
,	LSTR_PINFO_LOGGING_INOUT		// Player %s is logging in/out
,	LSTR_PINFO_NOT_ON_CELLS			// Player %s not found on cell hosts.
,	LSTR_LEADERCHANGE_FAIL			// Unable to change the leader at this time.
,	LSTR_RECRUIT_JOINED_MISSION		// Recruit '%s' has joined your mission.
,	LSTR_RECRUIT_LEFT_MISSION		// Recruit '%s' has left your mission.
,	LSTR_MSP_USAGE,				LSTR_MSP_DETAIL
,	LSTR_MSP_FROM_MAP				// To use /%s from the map screen you must have spawned first.
,	LSTR_MSP_REMINDER				// REMINDER: /%s is not a release feature and is a testing tool only.

// 540
,	LSTR_MSP_FAILED					// Unable to place MSP for some reason.
,	LSTR_MSP_DMG_IGNORED			// Damage to MSP ignored, it's already dead.
,	LSTR_STO_DMG_THRESHOLD			// %s to server tracked object below object's %s joules threshold
,	LSTR_HULK_INVULNERABLE			// Ignoring damage to hulk: capacity 0 (invulnerable)
,	LSTR_HULK_ALREADY_DEAD			// Ignoring damage to already dead hulk.
,	LSTR_HULK_DESTROYED				// Hulk destroyed.
,	LSTR_CHECKSTOS_USAGE,		LSTR_CHECKSTOS_DETAIL
,	LSTR_CHECKSTOS_OK				// All the STOs were fine
,	LSTR_CHECKSTOS_CANCELLED		// We cancelled these stos: %s.

// 550
,	LSTR_LISTSTOS_USAGE,		LSTR_LISTSTOS_DETAIL
,	LSTR_LISTSTOS_NONE				// No stos to list
,	LSTR_LISTSTOS_LIST				// STOs: %s
,	LSTR_SPAM_USAGE,			LSTR_SPAM_DETAIL
,	LSTR_SPAM_CLEAR					// Clearing the spam can.
,	LSTR_SPAM_START_USAGE,		LSTR_SPAM_START_DETAIL
,	LSTR_SPAM_INTERVAL_MIN			// Minimum 30s between repetitions

// 560
,	LSTR_SPAM_OPENING				// Opening spam can %s times %s between each repetition.
,	LSTR_SPAM_CLOSEING				// Closeing the can
,	LSTR_SPAM_ADDED					// Added: %s
,	LSTR_SPAM_EMPTY					// Spam can is currently empty.
,	LSTR_SPAM_STATUS				// Spam can %s lines. Repetitions: %s apart.
,	LSTR_SQUAD_VOTES_WITHDRAWN		// Election cancelled because of votes being withdrawn.
,	LSTR_SQUAD_ALL_VOTES_WITHDRAWN	// All votes for a specific player were withdrawn.
,	LSTR_KILLED_BY_BUILDING			// Killed by building %s destruction [%s]
,	LSTR_ENTERED_BUILDING			// You have entered %s.
,	LSTR_ENTERED_UNKNOWN_BUILDING	// You have entered unknown building %s.

// 570
,	LSTR_LEFT_BUILDING				// You have left the building.
,	LSTR_BDESEL_REFUSED_ID0			// Brigade selection refused: Brigade ID of zero (0).
,	LSTR_BDESEL_REFUSED_PERSONA		// Brigade selection refused without a persona selected first.
,	LSTR_BDESEL_REFUSED_BDE			// Brigade selection refused: invalid brigade.
,	LSTR_FIELD_PROMOTION			// FIELD PROMOTION!
,	LSTR_UNABLE_TO_SPAWN			// Unable to spawn: %s.
,	LSTR_KILL_CREDIT				// Kill credit awared to you for %s!
,	LSTR_FRIENDLY_FIRE				// FRIENDLY FIRE kill on %s!
,	LSTR_KILLED_A_DEV				// Player %s has killed developer %s!
,	LSTR_TOO_MANY_AOS				// Too many Attack Objectives: One will be withdrawn in %u minutes unless one is removed manually.

// 580
,	LSTR_CANNOT_USE_BRIDGE			// Target cannot be a bridge.
,	LSTR_INVALID_CP_NAME			// Unrecognized CP name '%s'.
,	LSTR_ALLIED_FBS					// Allied FBs: %s
,	LSTR_AXIS_FBS					// Axis FBs: %s
,	LSTR_UNKNOWN_UNIT				// Unrecognized HCUnit %s.
,	LSTR_JOIN_OR_SPECIFY_BDE		// You must join or specify a brigade to use %s.
,	LSTR_BDE_AND_DIV_ONLY			// Can only do that to Brigades and Divisions.
,	LSTR_EQUIPMENT_DELIVERED		// Equipment manually supplied to %s.

,	LSTR_CRS_MSG					// [CRS:%s] %s
,	LSTR_GM_MSG						// [GM:%s] %s

// 590
,	LSTR_LAST_KILLS					// Last 5 kills for %s this campaign:
,	LSTR_GRIDSNAPSHOT_ENABLED		// Grid snapshots enabled
,	LSTR_GRIDSNAPSHOT_DISABLED		// Grid snapshots disabled
,	LSTR_GRIDSNAPSHOT_FULL			// Grid snapshots set to FULL
,	LSTR_LASTKILLS_USAGE,		LSTR_LASTKILLS_DETAIL
,	LSTR_ONLY_WHILE_SPAWNED			// Sorry, you can only do that while you are spawned.
,	LSTR_SCMD_NO_BDE_LINK			// Ground brigades must have a direct link to their objective.
,	LSTR_SCMD_NO_DIV_LINK			// Ground divisions require one or more divisional members to have a link.
,	LSTR_HOLD_AT_ALL_COSTS			// Hold %s at all costs!

// 600
,	LSTR_FALLBACK_START				// Defenders at %s begin falling back to more secure positions.
,	LSTR_PLAYER_ALT_F4ED			// %s has alt-f4ed.
,	LSTR_OBJECT_DESTROYED			// %s destroyed.
,	LSTR_AUTO_REPAIR				// Auto-repair contribution to %s.
,	LSTR_FALLBACK_FAILED			// Fallback from %s failed.
,	LSTR_FALLING_BACK				// forces in {1} redeploying to defensive fallback positions.
,	LSTR_ENEMY_FALLING_BACK			// Enemy forces in {1} falling back
,	LSTR_FORCED_OWNERSHIP			// *FORCED* ownership of {1} to {2}
,	LSTR_CHANGED_OWNERSHIP			// Changed ownership of {1} to {2}
,	LSTR_GM_CHANGED_OWNERSHIP		// GM changed ownership of %s to %s.

// 610
,	LSTR_NO_FACILIITIES_CHANGED		// No failities at %s changed.
,	LSTR_STRAT_EXITED				// Strat server exiting
,	LSTR_STRAT_STARTING				// Strat server starting
,	LSTR_STRAT_READY				// Strat server ready
,	LSTR_GM_SET_GAMETIME			// GM set game time to {1}:{2}
,	LSTR_CPS_REBROADCAST			// CP statuses rebroadcast
,	LSTR_CMD_ONCE_PER_MINUTE		// {1} limited to once per minute.
,	LSTR_BROADCAST_USAGE,		LSTR_BROADCAST_DETAIL
,	LSTR_CP_REBROADCAST

// 620
,	LSTR_CEASEFIRE_ON				// [!] Cease fire no killing [!]
,	LSTR_NO_LINK_TO_BRIGADE			// No link to brigade
,	LSTR_BASE_CAPTURED				// {1} has been captured.
,	LSTR_ERROR						// ERROR: Prefix
,	LSTR_FRIEND_LOGGING_IN			// Friend {1} is logging in.
,	LSTR_OVERRUN_REPORTED			// {1}: Overrun code {2}
,	LSTR_OPS_CHANNEL_NOTE			// Reminder: "ops" channel is for ops.
,	LSTR_KILLED_BY_DEV				// When Rats Attack: {2} killed by {1}.
,	LSTR_RAT_KILLED_RAT				// Rat killed rat: {2} killed {1}.
,	LSTR_OICNOM_USAGE

// 630
, 	LSTR_OICNOM_DETAIL
,	LSTR_OICVOL_USAGE, LSTR_OICVOL_DETAIL
,	LSTR_OICVETO_USAGE, LSTR_OICVETO_DETAIL
,	LSTR_OICRES_USAGE, LSTR_OICRES_DETAIL
,	LSTR_OICREM_USAGE, LSTR_OICREM_DETAIL
,	LSTR_RESET_RADIO_USAGE

// 640
, 	LSTR_RESET_RADIO_DETAIL
,	LSTR_RESET_RADIO_DONE
,	LSTR_WOULD_HAVE_SCORED

,	LSTR_BRDMG_NO_OBJECTIVE			// Cannot damage {1}: No attack objective.
,	LSTR_BRDMG_ALLOWING				// Applying {2} joules of damage to {1}.
,	LSTR_BRREP_NO_OBJECTIVE			// Cannot repair {1}: No defense objective.
,	LSTR_BRREP_ALLOWING				// Repairing {1} for {2} joules.

,	LSTR_MISSION_POST_DENIED
,	LSTR_MPD_EXISTING_MISSION		// Already have an active mission
,	LSTR_MPD_NO_BRIGADE				// You don't have a brigade selected

// 650
,	LSTR_MPD_RANK_AD				// Insufficient rank - go buy it!
,	LSTR_MPD_RANK_NO_AD				// Insufficient rank, but we <3 you
,	LSTR_MPD_INVALID_FAC			// Invalid origin facility: {1}
,	LSTR_MPD_DONT_OWN				// Origin facility not under your control
,	LSTR_MPD_NON_BDE_ORIG			// Can't use origin with this brigade

,	LSTR_MISSION_POST_APPROVED

,	LSTR_CP_SURRENDERED_ENEMY		// {1} has surrendered to the enemy.
,	LSTR_CP_SURRENDERED_US			// {1} has surrendered to us!

,	LSTR_CP_SURRENDER_IN_HOURS		// {1} will surrender in {2} hours.
,	LSTR_CP_SURRENDER_IN_MINS		// {1} will surrender in {2} minutes.

// 660
,	LSTR_SOLDIERS_NEEDED_AT			// INTEL: Soldiers needed at {1}
,	LSTR_SOLDIERS_NEEDED_AT_AND		// INTEL: Soldiers needed at {1} and {2}.
,	LSTR_THREAT_REPORT			// Threat Report: {1}
,	LSTR_KILL_AWARD_TO			// Kill awarded to {1}.
,	LSTR_ASSIST_AWARD			// Assist on {1} (killed by {2}).

// NOTE: ui.dtd files have 665 and 666 defined
// 665 = <!ENTITY loc.host.665 "Game channel unavailable: you&#39;re not in a game.">
// 666 = <!ENTITY loc.host.666 "Team channel unavailable: you&#39;re not on a team.">
//     skipping to give time to resolve if these should exist

,	LSTR_BUG = 667				// BUG REPORT
,	LSTR_BUG_USAGE,	LSTR_BUG_DETAIL

// 670
,   LSTR_UPTIME_USAGE, LSTR_UPTIME_DETAIL

, LSTR_TROFF_USAGE, LSTR_TROFF_DETAIL
, LSTR_TRON_USAGE, LSTR_TRON_DETAIL
, LSTR_TR_OFF
, LSTR_TR_ON

,	MAX_LSTRING_IDS
};

//tolua_end
