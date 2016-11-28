DELIMITER ;;
DROP PROCEDURE IF EXISTS upgrade_1_35_0_to_1_35_1;
DROP PROCEDURE IF EXISTS upgrade_1_35_6_0_to_1_35_6_1;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    PROCEDURE upgrade_1_35_6_0_to_1_35_6_1 ()
    LANGUAGE SQL
    CONTAINS SQL
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
BEGIN
    -- Upgrade game database from 1.35.6.0 to 1.35.6.1
    -- 11/28/2016 kfs1

    -- We removed 2 countries (JP and CW)
    DELETE FROM `wwiionline`.`wwii_country` WHERE abbr IN ('JP', 'CW');
    UPDATE `wwiionline`.`wwii_country` SET countryID = 6 WHERE abbr = 'CN';
    UPDATE `wwiionline`.`wwii_country` set countryID = 7 WHERE abbr = 'RU';
    UPDATE `wwiionline`.`strat_cp` SET country = country - 2 WHERE country > 7;
    UPDATE `wwiionline`.`strat_cp` SET originalcountry = originalcountry - 2 WHERE originalcountry > 7;
    UPDATE `wwiionline`.`strat_cp` SET concountry = concountry - 2 WHERE concountry > 7;
    UPDATE `wwiionline`.`strat_facility` SET country = country - 2 WHERE country > 7;
    UPDATE `wwiionline`.`strat_facility` SET originalcountry = originalcountry - 2 WHERE originalcountry > 7;
    UPDATE `wwiionline`.`strat_object` SET country = country - 2 WHERE country > 7;
    UPDATE `wwiionline`.`strat_object` SET originalcountry = originalcountry - 2 WHERE originalcountry > 7;

    -- We added a weapon_spec column to the vehicle type table.
    IF NOT EXISTS (SELECT NULL FROM information_schema.columns WHERE table_schema='wwiionline' AND table_name='wwii_vehtype' AND column_name='weapon_spec') THEN
        ALTER TABLE `wwiionline`.`wwii_vehtype` ADD COLUMN `weapon_spec` INT(11) UNSIGNED DEFAULT NULL;
        UPDATE `wwiionline`.`wwii_vehtype` SET `weapon_spec` = `wwii`.`f_weapon_spec`(countryid, categoryid, classid, vehtype_oid);
        ALTER TABLE `wwiionline`.`wwii_vehtype` MODIFY COLUMN `weapon_spec` INT(11) UNSIGNED NOT NULL;
        ALTER TABLE `wwiionline`.`wwii_vehtype` ADD INDEX `vt_os` (weapon_spec);
    END IF;

    -- Several columns became unsigned with a default so we don't always have to supply them.
    ALTER TABLE `wwiionline`.`wwii_sortie` MODIFY `successful` TINYINT(1) UNSIGNED DEFAULT 0;
    ALTER TABLE `wwiionline`.`wwii_sortie` MODIFY `capture_assists` SMALLINT(5) UNSIGNED DEFAULT 0;
    ALTER TABLE `wwiionline`.`wwii_sortie` MODIFY `resupplies` SMALLINT(5) UNSIGNED DEFAULT 0;

    -- Optimization of the vehicle_vs_vehicle view.
    DROP VIEW IF EXISTS `wwii`.`v_vehicle_vs_vehicle`;
    CREATE 
        ALGORITHM = UNDEFINED 
        DEFINER = `wwiiol`@`localhost` 
        SQL SECURITY INVOKER 
        VIEW `wwii`.`v_vehicle_vs_vehicle` AS
    select 
        `kv`.`vehtype_oid` AS `killer`, 
        `vv`.`vehtype_oid` AS `victim`, 
        `k`.`killer_country` AS `killer_country`, 
        `k`.`victim_country` AS `victim_country`, 
        `kv`.`fullName` AS `killer_name`, 
        `vv`.`fullName` AS `victim_name`, 
        count(0) AS `kills` 
        from 
        `wwii`.`kills` `k`, 
        `wwiionline`.`wwii_vehtype` `vv`, 
        `wwiionline`.`wwii_vehtype` `kv` 
        where 
        ((`k`.`victim_vehtype_oid` = `vv`.`vehtype_oid`) 
        and (`k`.`killer_vehtype_oid` = `kv`.`vehtype_oid`)) 
        group by 1, 2
    ;
END ;;
