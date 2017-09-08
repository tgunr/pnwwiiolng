<?php

use Phinx\Migration\AbstractMigration;

// These changes were old schema revision 3 which was never applied to the auth database
// before the new migration system (this one) was emplaced.
//
// It's worth noting that none of these changes seem to actually *do* anything.
class ImproveAuthViews extends AbstractMigration
{
    // Because the MySQL adapter doesn't support multiple databases, can't use change()
    public function up()
    {
        $this->execute(<<<SQL
CREATE OR REPLACE
    ALGORITHM = TEMPTABLE
    DEFINER = CURRENT_USER
    SQL SECURITY INVOKER
    VIEW `auth`.`v_customer_active_groups`
    AS
    SELECT
        `ac`.`customerid`, `a`.`arenaid`, `cg`.`group_oid`
    FROM
        `auth`.`auth_customer` `ac`
        INNER JOIN `auth`.`auth_subscribed` `s` USING (`customerid`)
        INNER JOIN `auth`.`auth_subarenaassoc` `a` USING (`subscriptionid`)
        INNER JOIN `auth`.`auth_subgroupassoc` `g` USING (`subscriptionid`)
        INNER JOIN `auth`.`chat_groups` `cg` ON (`g`.`groupid` = `cg`.`group_oid`)
     WHERE
        `s`.`status` = 'active'
        AND `cg`.`enabled` <> 'N'
SQL
        );
        $this->execute(<<<SQL
CREATE OR REPLACE
    ALGORITHM = TEMPTABLE
    DEFINER = CURRENT_USER
    SQL SECURITY INVOKER
    VIEW `auth`.`v_player_active_groups`
    AS
    SELECT
        `pl`.`playerid`, `a`.`arenaid`, `cg`.`group_oid`
    FROM
        `wwiionline`.`wwii_player` `pl`
        INNER JOIN `auth`.`auth_subscribed` `s` USING (`customerid`)
        INNER JOIN `auth`.`auth_subarenaassoc` `a` USING (`subscriptionid`)
        INNER JOIN `auth`.`auth_subgroupassoc` `g` USING (`subscriptionid`)
        INNER JOIN `auth`.`chat_groups` `cg` ON (`g`.`groupid` = `cg`.`group_oid`)
    WHERE
        `s`.`status` = 'active'
        AND `cg`.`enabled` <> 'N'
SQL
        );
        $this->execute("DROP VIEW IF EXISTS `auth`.`v_group_grants`");
        $this->execute("DROP VIEW IF EXISTS `auth`.`v_player_active_group_grants`");
    }
    
    public function down()
    {
        // It is not possible to restore v_group_grants or v_player_active_group_grants
        // as these views do not exist in any available version of the auth database and
        // therefore there is no reference for their "original" structure.
        $this->execute(<<<SQL
CREATE OR REPLACE
    ALGORITHM=UNDEFINED DEFINER=`wwiiol`@`66.28.224.%` SQL SECURITY INVOKER
    VIEW `v_player_active_groups` AS
    select
        `pl`.`playerid` AS `playerid`,
        `a`.`arenaID` AS `arenaid`,
        `cg`.`group_oid` AS `group_oid`
    from
        ((((`wwiionline`.`wwii_player` `pl`
        join `auth`.`auth_subscribed` `s` on((`pl`.`customerid` = `s`.`customerID`)))
        join `auth`.`auth_subarenaassoc` `a` on((`s`.`subscriptionID` = `a`.`subscriptionID`)))
        join `auth`.`auth_subgroupassoc` `g` on((`s`.`subscriptionID` = `g`.`subscriptionID`)))
        join `auth`.`chat_groups` `cg` on((`g`.`groupID` = `cg`.`group_oid`)))
    where
        ((`s`.`status` = 'active') and (`cg`.`enabled` <> 'N'))
SQL
        );
        $this->execute(<<<SQL
CREATE OR REPLACE
    ALGORITHM=UNDEFINED DEFINER=`wwiiol`@`66.28.224.%` SQL SECURITY INVOKER
    VIEW `auth`.`v_customer_active_groups` AS
    select
        `ac`.`customerID` AS `customerid`,
        `a`.`arenaID` AS `arenaid`,
        `cg`.`group_oid` AS `group_oid`
    from
        ((((`auth_customer` `ac`
            join `auth_subscribed` `s` on((`ac`.`customerID` = `s`.`customerID`)))
            join `auth_subarenaassoc` `a` on((`s`.`subscriptionID` = `a`.`subscriptionID`)))
            join `auth_subgroupassoc` `g` on((`s`.`subscriptionID` = `g`.`subscriptionID`)))
            join `chat_groups` `cg` on((`g`.`groupID` = `cg`.`group_oid`)))
    where
        ((`s`.`status` = 'active') and (`cg`.`enabled` <> 'N'))
SQL
        );
    }
}
