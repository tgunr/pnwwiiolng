#pragma once
// Copyright (C) PlayNet Inc 2011 -- Not for redistribution.
//
// Describes incremental SQL revisions to the Auth Database.

#include "dbse/dbseDatabaseSchema.h"

static const char* revisions[] =
{
	// Revision 0: Add the schema table
	//{
		"CREATE TABLE IF NOT EXISTS `auth`.`schema_info`"
		"("
		" `version_no` INT(11) UNSIGNED NOT NULL DEFAULT 0"
		",`implemented` TIMESTAMP NOT NULL"
		")"
		" COMMENT 'Track current database schema revision to allow automatic updates'"
	,	nullptr
	//}
,	// Revision 1: Add the CAN_USE_PPO_TOOL permission.
	//{
		"REPLACE INTO `auth`.`wwii_permissions` "
			"(permissionID, maskvalue, description, modified, internal, details)"
		"VALUES"
			"(36, POW(2,35), 'CAN_USE_PPO_TOOL', NOW(), 0, 'Allows access to the PPO building tool')"
	,	nullptr
	//}
,	// Revision 2: Lets tidy up the auth_customers table.
	//{
		"ALTER TABLE `auth`.`auth_customer`"
			"	MODIFY	`username`		CHAR(16)	CHARACTER SET latin1		NOT NULL"
			",	MODIFY	`password`		CHAR(32)	CHARACTER SET latin1		NOT NULL"
			",	DROP	`first`"
			",	DROP	`last`"
			",	DROP	`email`"
	,	"ALTER TABLE `auth`.`auth_subscription`"
			"	MODIFY `description`	CHAR(128)	NOT NULL					COMMENT 'Internal description of subscription.'"
	,	nullptr
	//}
,	// Revision 3: Clean up on several views to reduce locking times.
	//{
		"CREATE OR REPLACE"
		"	ALGORITHM = TEMPTABLE"
		"	DEFINER = CURRENT_USER"
		"	SQL SECURITY INVOKER"
		"	VIEW `auth`.`v_customer_active_groups`"
		"	AS"
		"	SELECT	ac.customerid, a.arenaid, cg.group_oid"
		"	  FROM	auth.auth_customer ac"
		"			INNER JOIN auth.auth_subscribed s USING (customerid)"
		"			INNER JOIN auth.auth_subarenaassoc a USING (subscriptionid)"
		"			INNER JOIN auth.auth_subgroupassoc g USING (subscriptionid)"
		"			INNER JOIN auth.chat_groups cg ON (g.groupid = cg.group_oid)"
		"	 WHERE	s.status = 'active'"
		"			AND cg.enabled <> 'N'"
	,
		"CREATE OR REPLACE"
		"	ALGORITHM = TEMPTABLE"
		"	DEFINER = CURRENT_USER"
		"	SQL SECURITY INVOKER"
		"	VIEW `auth`.`v_player_active_groups`"
		"	AS"
		"	SELECT	pl.playerid, a.arenaid, cg.group_oid"
		"	  FROM	wwiionline.wwii_player pl"
		"			INNER JOIN auth.auth_subscribed s USING (customerid)"
		"			INNER JOIN auth.auth_subarenaassoc a USING (subscriptionid)"
		"			INNER JOIN auth.auth_subgroupassoc g USING (subscriptionid)"
		"			INNER JOIN auth.chat_groups cg ON (g.groupid = cg.group_oid)"
		"	 WHERE	s.status = 'active'"
		"			AND cg.enabled <> 'N'"
	,
		"CREATE OR REPLACE"
		"	ALGORITHM=TEMPTABLE"
		"	DEFINER=CURRENT_USER"
		"	SQL SECURITY INVOKER"
		"	VIEW `auth`.`v_group_grants`"
		"	AS"
		"	SELECT `cg`.`group_oid`"
		"		,	`b`.`branchID`"
		"		,	`cg`.`grants` AS `permission_bits`"
		"		,	`cg`.`max_squad_rank`"
		"		,	max(if(isnull(`cgr`.`max_rank`), 0, `cgr`.`max_rank`)) AS `max_rank`"
		"	FROM	(`auth`.`chat_groups` `cg` JOIN `wwiionline`.`wwii_branch` `b`)"
		"			LEFT JOIN `wwiionline`.`chat_group_ranks` `cgr` USING (group_oid, branchid)"
		"	GROUP	BY 1, 2"
	,
		"DROP VIEW IF EXISTS `auth`.`v_player_active_group_grants`"
	,	nullptr
	//}
,	// Revision 4: 1.34 product permissions
	//{
		"REPLACE INTO `auth`.`wwii_permissions` "
			"(permissionID, maskvalue, description, modified, internal, details)"
		"VALUES"
			"(40, POW(2,39), 'PRODUCT01', NOW(), 0, 'Product Permission 1')"
			",(41, POW(2,40), 'PRODUCT02', NOW(), 0, 'Product Permission 2')"
			",(42, POW(2,41), 'PRODUCT03', NOW(), 0, 'Product Permission 3')"
			",(43, POW(2,42), 'PRODUCT04', NOW(), 0, 'Product Permission 4')"
			",(44, POW(2,43), 'PRODUCT05', NOW(), 0, 'Product Permission 5')"
			",(45, POW(2,44), 'PRODUCT06', NOW(), 0, 'Product Permission 6')"
			",(46, POW(2,45), 'PRODUCT07', NOW(), 0, 'Product Permission 7')"
			",(47, POW(2,46), 'PRODUCT08', NOW(), 0, 'Product Permission 8')"
			",(48, POW(2,47), 'PRODUCT09', NOW(), 0, 'Product Permission 9')"
			",(49, POW(2,48), 'PRODUCT10', NOW(), 0, 'Product Permission 10')"
			",(50, POW(2,49), 'PRODUCT11', NOW(), 0, 'Product Permission 11')"
			",(51, POW(2,50), 'PRODUCT12', NOW(), 0, 'Product Permission 12')"
			",(52, POW(2,51), 'PRODUCT13', NOW(), 0, 'Product Permission 13')"
			",(53, POW(2,52), 'PRODUCT14', NOW(), 0, 'Product Permission 14')"
			",(54, POW(2,53), 'PRODUCT15', NOW(), 0, 'Product Permission 15')"
			",(55, POW(2,54), 'PRODUCT16', NOW(), 0, 'Product Permission 16')"
			",(56, POW(2,55), 'PRODUCT17', NOW(), 0, 'Product Permission 17')"
			",(57, POW(2,56), 'PRODUCT18', NOW(), 0, 'Product Permission 18')"
			",(58, POW(2,57), 'PRODUCT19', NOW(), 0, 'Product Permission 19')"
			",(59, POW(2,58), 'PRODUCT20', NOW(), 0, 'Product Permission 20')"
	,	nullptr
	//}
} ;
