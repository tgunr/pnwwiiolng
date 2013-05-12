#pragma once

#include "dbse/dbseDatabaseSchema.h"

static const char* revisions[] =
{
	// Revision 0: Add the schema table.
	//{
		"CREATE TABLE IF NOT EXISTS `wwii`.`schema_info`"
		"("
		" `version_no` INT(11) UNSIGNED NOT NULL DEFAULT 0"
		",`implemented` TIMESTAMP NOT NULL"
		")"
		" COMMENT 'Track current database schema revision to allow automatic updates'"
	,	NULL
	//}
,	// Revision 1: Update/add the wthr_report table
	//{
		"DROP TABLE IF EXISTS `wwiionline`.`wthr_report`"
	,	"CREATE TABLE `wwiionline`.`wthr_report`"
		"("
		" `real_start` DATETIME NOT NULL UNIQUE PRIMARY KEY COMMENT 'Real-world time of start'"
		",`game_day` INT(11) UNSIGNED NOT NULL COMMENT 'Game day number'"
		",`start_time` INT(11) UNSIGNED NOT NULL COMMENT 'In-game start time'"
		",`weather_pattern` INT(11) UNSIGNED NOT NULL COMMENT 'Weather (sky) pattern'"
		",`rain_pattern` INT(11) UNSIGNED NOT NULL COMMENT 'Rain pattern (if applicable)'"
		",`weather_forecast` INT(11) UNSIGNED NOT NULL COMMENT 'Prediction for tomorrow'"
		")"
	,	NULL
	//}
,	// Revision 2: Move the weather report stuff to the 'strat' database.
	//{
		"DROP TABLE IF EXISTS `strat`.`wthr_report`"
	,	"CREATE TABLE `strat`.`wthr_report` LIKE `wwiionline`.`wthr_report`"
	,	"DROP TABLE IF EXISTS `wwiionline`.`wthr_report`"
	,	NULL
	//}
,	// Revision 3: Change the layout of the recordings table to match new enums.
	//{
		"TRUNCATE TABLE `sys`.`cell_sessions`"
	,	"TRUNCATE TABLE `sys`.`cell_recording`"
	,	"ALTER TABLE `sys`.`cell_recording` MODIFY `event_type` ENUM('Misc', 'Removal', 'Update', '1Time')"
	,	NULL
	//}
,	// Revision 4: Add the audit.sto_history table.
	//{
		"CREATE TABLE `audit`.`sto_history`"
		"("
		"  `uid` INT(11) UNSIGNED NOT NULL PRIMARY KEY"
		", `clock64` BIGINT UNSIGNED NOT NULL COMMENT 'milisecond time of event'"
		", `event` enum ('Creation', 'Destruction') NOT NULL"
		", `instance_id` BIGINT(11) UNSIGNED NOT NULL"
		", `type` INT(11) UNSIGNED NOT NULL"
		", `subtype` INT(11) UNSIGNED NOT NULL"
		", `player_id` INT(11) UNSIGNED NOT NULL"
		", `mission_id` INT(11) UNSIGNED NOT NULL"
		", `pos_x` FLOAT NOT NULL COMMENT 'Position X'"
		", `pos_y` FLOAT NOT NULL COMMENT 'Position Y'"
		", `pos_z` FLOAT NOT NULL COMMENT 'Position Z'"
		", `att_x` FLOAT NOT NULL COMMENT 'Attitude X'"
		", `att_y` FLOAT NOT NULL COMMENT 'Attitude Y'"
		", `att_z` FLOAT NOT NULL COMMENT 'Attitude Z'"
		", `flags` INT(11) UNSIGNED NOT NULL"
		", `lifespan` INT(11) UNSIGNED NOT NULL"
		") COMMENT 'Audit trail of STO creation/destruction'"
	,	NULL
	//}
,	// Revision 5: Fix the primary key on the audit.sto_history table.
	//{
		"ALTER TABLE `audit`.`sto_history` DROP PRIMARY KEY, ADD INDEX (`uid`, `event`), ADD INDEX (`clock64`)"
	,	NULL
	//}
,	// Revision 6: Enable country 2.
	//{
		"UPDATE `wwiionline`.`wwii_country` SET abbr = 'US', fullName = 'USA', sideID = 1, is_active = 1 WHERE countryID = 2"
	,	NULL
	//}
,	// Revision 7: Add US training HC Units.
	//{
		"ALTER TABLE `wwiionline`.`strat_hc_units` ADD UNIQUE INDEX `shu_unq_name_idx` (`name`)"
	,	"INSERT IGNORE INTO `wwiionline`.`strat_hc_units`"
			"( parent_id, structure_id, branch_id, country_id, owner_id"
			", `name`, `nick`, `title`, `motd`, `added`, `modified`"
			", home_cp_oid, start_home_cp_oid, fallback_cp_oid, start_fallback_cp_oid"
			", last_moved, next_move, toe_template, movement_type, last_moved_by"
			")"
		" VALUES"
			"( 0, 168, 1, 2, 0"
			", 'trngusarmy', 'U.S. Army', 'United States Army', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 1, 'SYSTEM'"
			"),"
			"( 0, 168, 2, 2, 0"
			", 'trngusair', 'U.S. Air Force', 'United States Air Forces', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 2, 'SYSTEM'"
			"),"
			"( 0, 168, 3, 2, 0"
			", 'trngusnavy', 'U.S. Navy', 'United States Navy', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 3, 'SYSTEM'"
			")"
	,	NULL
	//}
,	// Revision 8: Make the sto history table less inaccurate.
	//{
		"ALTER TABLE `audit`.`sto_history`"
		"	MODIFY `pos_x` DECIMAL(12,4) NOT NULL COMMENT 'X co-ordinate'"
		",	MODIFY `pos_y` DECIMAL(12,4) NOT NULL COMMENT 'Y co-ordinate'"
		",	MODIFY `pos_z` DECIMAL(12,4) NOT NULL COMMENT 'Y co-ordinate'"
	,	NULL
	//}
,	// Revision 9: Add American structure.
	//{
		"DELETE FROM `wwiionline`.`strat_hc_units` WHERE `country_id` = 2 AND `structure_id` = 165"
	,	"INSERT IGNORE INTO `wwiionline`.`strat_hc_units`"
			"( parent_id, structure_id, branch_id, country_id, owner_id"
			", `name`, `nick`, `title`, `motd`, `added`, `modified`"
			", home_cp_oid, start_home_cp_oid, fallback_cp_oid, start_fallback_cp_oid"
			", last_moved, next_move, toe_template, movement_type, last_moved_by"
			")"
		" VALUES"
			"( 0, 165, 1, 2, 0"
			", 'us1army', '1st Army', 'U.S. 1st Army', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 1, 'SYSTEM'"
			"),"
			"( 0, 165, 1, 2, 0"
			", 'us2army', '2nd Army', 'U.S. 2nd Army', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 1, 'SYSTEM'"
			"),"
			"( 0, 165, 2, 2, 0"
			", 'us8air', '8th Air Force', 'U.S. 8th Air Force', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 2, 'SYSTEM'"
			"),"
			"( 0, 165, 3, 2, 0"
			", 'usnaf', 'Atlantic Fleet', 'U.S. Atlantic Fleet', '', now(), now()"
			", 0, 0, 0, 0"
			", 0, 0, '', 3, 'SYSTEM'"
			")"
	,	NULL
	//}
,	// Revision 10: Add clientRevision to the client_terminations table.
	//{
		"ALTER TABLE `wwii`.`client_terminations`"
			" ADD COLUMN `clientRevision` int(11) unsigned NOT NULL DEFAULT '0'"
	,	NULL
	//}
,	// Revision 11: Create wwiionline.chat_messages table.
	//{
		"CREATE TABLE `audit`.`chat_messages`"
		"(id bigint unsigned auto_increment primary key not null unique"
		",player_id int(11) unsigned not null"
		",message text"
		",logged timestamp NOT NULL"
		")"
	,	NULL
	//}
,	// Revision 12: Rename wwiionline.wwii_config and replace with a view.
	//{
		"RENAME TABLE `wwiionline`.`wwii_config` TO `game`.`host_config`"
	,	"CREATE SQL SECURITY INVOKER VIEW `wwiionline`.`wwii_config` AS SELECT * FROM `game`.`host_config`"
	,	NULL
	//}
,	// Revision 13: Update wwii_country
	//{
		"DELETE FROM `wwiionline`.`wwii_country` WHERE abbr IN ('JP', 'CW')"
	,	"UPDATE `wwiionline`.`wwii_country` SET countryID = 6 WHERE abbr = 'CN'"
	,	"UPDATE `wwiionline`.`wwii_country` set countryID = 7 WHERE abbr = 'RU'"
	,	NULL
	//}
,	// Revision 14: Update countries.
	//{
		"UPDATE `wwiionline`.`strat_cp` SET country = country - 2 WHERE country > 7"
	,	"UPDATE `wwiionline`.`strat_cp` SET originalcountry = originalcountry - 2 WHERE originalcountry > 7"
	,	"UPDATE `wwiionline`.`strat_cp` SET concountry = concountry - 2 WHERE concountry > 7"
	,	"UPDATE `wwiionline`.`strat_facility` SET country = country - 2 WHERE country > 7"
	,	"UPDATE `wwiionline`.`strat_facility` SET originalcountry = originalcountry - 2 WHERE originalcountry > 7"
	,	"UPDATE `wwiionline`.`strat_object` SET country = country - 2 WHERE country > 7"
	,	"UPDATE `wwiionline`.`strat_object` SET originalcountry = originalcountry - 2 WHERE originalcountry > 7"
	,	NULL
	//}
,	// Revision 15: Add weapon spec column to vehicle table so we don't have to keep calculating it.
	//{
		"ALTER TABLE `wwiionline`.`wwii_vehtype` ADD COLUMN `weapon_spec` INT(11) UNSIGNED DEFAULT NULL"
	,	"UPDATE `wwiionline`.`wwii_vehtype` SET `weapon_spec` = wwii.f_weapon_spec(countryid, categoryid, classid, vehtype_oid)"
	,	"ALTER TABLE `wwiionline`.`wwii_vehtype` MODIFY COLUMN `weapon_spec` INT(11) UNSIGNED NOT NULL"
	,	"ALTER TABLE `wwiionline`.`wwii_vehtype` ADD INDEX `vt_os` (weapon_spec)"
	,	NULL
	//}
,	// Revision 16: Fixes from playnet database.
	//{
		"ALTER TABLE `wwiionline`.`wwii_sortie` MODIFY `successful` TINYINT(1) UNSIGNED DEFAULT 0"
	,	"ALTER TABLE `wwiionline`.`wwii_sortie` MODIFY `capture_assists` SMALLINT(5) UNSIGNED DEFAULT 0"
	,	"ALTER TABLE `wwiionline`.`wwii_sortie` MODIFY `resupplies` SMALLINT(5) UNSIGNED DEFAULT 0"
	,	NULL
	//}
} ;
