-- -----------------------------------------------------------------------
/*
Copyright Cornered Rat Software 2018

File name: insert_strat_GarrisonUnits.sql
Affects table(s): wwiionline.strat_hc_units, toe.supply
Purpose: Insert units that use the Garrison structure type

Changelog:
Name			JIRA Task #			Date		Description
Chaoswzkd		2098				04/21/2018	Initial Version

*/
-- -----------------------------------------------------------------------

DROP PROCEDURE IF EXISTS wwiionline.insert_update_wwiiol_stratHCunits_toe_supply;
DELIMITER //
CREATE PROCEDURE wwiionline.insert_update_wwiiol_stratHCunits_toe_supply(
    branchid INT,
    countryid INT,
    name CHAR(10),
    title VARCHAR(128),
    nick VARCHAR(25),
    cp_oid INT,
    toe_template VARCHAR(10),
    movement_type SMALLINT,
    structureid TINYINT(3) UNSIGNED
)
BEGIN
	DECLARE existingTemplate VARCHAR(10);
    DECLARE existingID INT;
    
    -- Check to see if we already have a matching garrison of the correct type in the town
    SELECT
		u.toe_template,
        u.unit_id
	INTO
		existingTemplate,
        existingID
	FROM
		wwiionline.strat_hc_units u
	WHERE
		u.home_cp_oid = cp_oid
		AND u.structure_id = structureid
        AND u.branch_id = branchid
        AND u.country_id = country_id
        AND u.movement_type = movement_type;
	
    
    SELECT
		@newid,
        branchid,
		countryid,
		name,
		title,
		nick,
		cp_oid,
		toe_template,
		movement_type,
		structureid;
    
	-- If a matching garrison of the correct type exists, but isn't the correct template
	IF (existingID IS NOT NULL AND existingTemplate IS NOT NULL AND existingTemplate <> toe_template) THEN

    SELECT CONCAT('Updating ', existingID);
    
	-- Delete existing supply
		DELETE
		FROM
			toe.supply
		WHERE
			unit_id = existingID;
	
    
	-- Change the existing template to the new one
		UPDATE
			wwiionline.strat_hc_units u
		SET
			u.name = name
			,u.title = title
			,u.nick = nick
			,u.toe_template = toe_template
			,u.last_moved_by = 'SYSTEM'
		WHERE
			u.unit_id = existingID;
	
    
	-- Insert the correct supply into the toe table
		INSERT
		INTO
			toe.supply
		SELECT
			existingID,
			veh.vehtype_oid,
			veh.capacity
		FROM
			toe.vehicles veh
		WHERE
			veh.id = toe_template;
            
	
	-- Otherwise, if an existing garrison doesn't exist
	ELSEIF (existingID IS NULL) THEN
	
    SELECT 'Inserting';
    
		INSERT
		INTO
			wwiionline.strat_hc_units
		SELECT
			@newid,
			0,
			structureid,
			branchid,
			countryid,
			0,
			name,
			title,
			nick,
			'Default Brigade Motto',
			'Brigade Message of the Day',
			UTC_TIMESTAMP(),
			UTC_TIMESTAMP(),
			cp_oid,
			cp_oid,
			cp_oid,
			cp_oid,
			0,
			null,
			toe_template,
			movement_type,
			'SYSTEM';
    
		INSERT
		INTO
			toe.supply
		SELECT
			@newid,
			veh.vehtype_oid,
			veh.capacity
		FROM
			toe.vehicles veh
		WHERE
			veh.id = toe_template;
    
		SET @newid = @newid + 1;
    
	END IF;
END; //
DELIMITER ;



	DROP TEMPORARY TABLE IF EXISTS wwiionline.cities;
	CREATE TEMPORARY TABLE wwiionline.cities (city_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY)
	SELECT
		cp.cp_oid,
		cp.country,
		cp.name,
		(SELECT COUNT(*) FROM wwiionline.strat_facility WHERE cp_oid = cp.cp_oid AND facility_type = 9) as 'ABCount',
		(SELECT COUNT(*) FROM wwiionline.strat_facility WHERE cp_oid = cp.cp_oid AND facility_type = 10) as 'DOCount',
		(SELECT COUNT(*) FROM wwiionline.strat_facility WHERE cp_oid = cp.cp_oid AND facility_type = 8) as 'AFCount'
	FROM
		wwiionline.strat_cp cp
	WHERE
		cp.cp_type <> 5 -- 5 is reserved for bridges
		and cp.name not like 'camp %'
		and cp.name not like 'fort %'
		and cp.name not like '%offline%'
		and cp.name not like '%training%'
		and cp.name not like '% flak %';

    SET @newid = ((SELECT MAX(unit_id) FROM wwiionline.strat_hc_units) + 1);

DROP PROCEDURE IF EXISTS wwiionline.insert_update_wwiiol_stratHCunits_toe_supply_loop;
DELIMITER //
CREATE PROCEDURE wwiionline.insert_update_wwiiol_stratHCunits_toe_supply_loop()
BEGIN

	SET @iterator = (SELECT MIN(city_id) FROM wwiionline.cities);
	SET @final = (SELECT MAX(city_id) FROM wwiionline.cities);
    
    SET @structureid = (SELECT structure_id FROM wwiionline.strat_hc_structure WHERE title = 'Garrison');
    
    SET autocommit = 0;
    
    START TRANSACTION;

	WHILE @iterator <= @final DO
	BEGIN
		DECLARE countryid INT;
    	DECLARE cityname VARCHAR(64);
    	DECLARE cp_oid INT;
        DECLARE countryprefix CHAR(2);
        
        DECLARE ABCount INT;
        DECLARE DOCount INT;
        DECLARE AFCount INT;
        
        DECLARE baseUnitName CHAR(10);
        DECLARE unitName CHAR(10);
    	DECLARE title VARCHAR(128);
        DECLARE baseNick VARCHAR(25);
    	DECLARE nick VARCHAR(25);
    	DECLARE toe_template VARCHAR(10);
        DECLARE branchid INT;
    	DECLARE movement_type SMALLINT;
        
		DECLARE nameType CHAR(3);
        
        SELECT
			cities.country,
            cities.name,
            cities.cp_oid,
            LCASE(country.abbr),
            cities.ABCount,
            cities.DOCount,
            cities.AFCount
		INTO
			countryid,
            cityname,
            cp_oid,
            countryprefix,
            ABCount,
            DOCount,
            AFCount
		FROM
			wwiionline.cities cities
            INNER JOIN wwiionline.wwii_country country ON country.countryID = cities.country
		WHERE
			cities.city_id = @iterator;
    
		
        IF ABCount > 3 THEN SET ABCount = 3; END IF;
        IF DOCount > 2 THEN SET DOCount = 2; END IF;
        IF AFCount > 1 THEN SET AFCount = 1; END IF; -- Future proofing, just in case
		
		IF cityName REGEXP '[.]' THEN
			SET baseUnitName = REPLACE(CONCAT('Gsn', SUBSTRING(cityName, 1, 4)), '.', '');
		ELSE
			SET baseUnitName = CONCAT('Gsn', SUBSTRING(cityName, 1, 3));
		END IF;
		
        SET baseNick = CONCAT('Gsn', SUBSTRING(cityName, 1, 18));
        
        SELECT CONCAT('Processing ', cityname, '; ABCount: ', ABCount, '; DOCount: ', DOCount, '; AFCount: ', AFCount);
        
		-- Army
        IF ABCount > 0 THEN
        BEGIN
			SET nameType = "grd";
			SET toe_template = CONCAT(countryprefix, nameType, 'g', ABCount, 'for');
			SET branchid = 1;
			SET movement_type = 1;
			
			SET unitName = CONCAT(baseUnitName, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.name like CONCAT(baseUnitName, '%')));
			SET nick = CONCAT(baseNick, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.nick like CONCAT(baseNick, '%')));
			
			SET title = CONCAT(SUBSTRING(cityName, 1, 114), ' Army Garrison');
			
			CALL wwiionline.insert_update_wwiiol_stratHCunits_toe_supply(
				branchid,
				countryid,
				unitName,
				title,
				nick,
				cp_oid,
				toe_template,
				movement_type,
				@structureid);
		END;
		END IF;
        
        -- Navy
        IF DOCount > 0 THEN
        BEGIN
			SET nameType = "nav";
			SET toe_template = CONCAT(countryprefix, nameType, 'g', DOCount, 'for');
			SET branchid = 3;
			SET movement_type = 3;
			
			SET unitName = CONCAT(baseUnitName, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.name like CONCAT(baseUnitName, '%')));
			SET nick = CONCAT(baseNick, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.nick like CONCAT(baseNick, '%')));
			
			SET title = CONCAT(SUBSTRING(cityName, 1, 114), ' Navy Garrison');
			
			CALL wwiionline.insert_update_wwiiol_stratHCunits_toe_supply(
				branchid,
				countryid,
				unitName,
				title,
				nick,
				cp_oid,
				toe_template,
				movement_type,
				@structureid);
        
        END;
        END IF;
        
        -- Air
        IF AFCount > 0 THEN
        BEGIN
			SET nameType = "air";
			SET toe_template = CONCAT(countryprefix, nameType, 'g', AFCount, 'for');
			SET branchid = 2;
			SET movement_type = 2;
			
			SET unitName = CONCAT(baseUnitName, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.name like CONCAT(baseUnitName, '%')));
			SET nick = CONCAT(baseNick, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.nick like CONCAT(baseNick, '%')));
			
			SET title = CONCAT(SUBSTRING(cityName, 1, 109), ' Air Force Garrison');
			
			CALL wwiionline.insert_update_wwiiol_stratHCunits_toe_supply(
				branchid,
				countryid,
				unitName,
				title,
				nick,
				cp_oid,
				toe_template,
				movement_type,
				@structureid);
        
        END;
        END IF;
        
        -- Paras
        IF AFCount > 0 THEN
        BEGIN
			SET nameType = "par";
			SET toe_template = CONCAT(countryprefix, nameType, 'g', AFCount, 'for');
			SET branchid = 1;
			SET movement_type = 2;
			
			SET unitName = CONCAT(baseUnitName, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.name like CONCAT(baseUnitName, '%')));
			SET nick = CONCAT(baseNick, (SELECT COUNT(*) FROM wwiionline.strat_hc_units u WHERE u.nick like CONCAT(baseNick, '%')));
			
			SET title = CONCAT(SUBSTRING(cityName, 1, 110), ' Airborne Garrison');
			
			CALL wwiionline.insert_update_wwiiol_stratHCunits_toe_supply(
				branchid,
				countryid,
				unitName,
				title,
				nick,
				cp_oid,
				toe_template,
				movement_type,
				@structureid);
        
        END;
        END IF;
        
        SET @iterator = @iterator + 1;
        
	END;
	END WHILE;
    
    COMMIT;
END; //
DELIMITER ;

CALL wwiionline.insert_update_wwiiol_stratHCunits_toe_supply_loop();
DROP PROCEDURE IF EXISTS wwiionline.insert_update_wwiiol_stratHCunits_toe_supply;
DROP PROCEDURE IF EXISTS wwiionline.insert_update_wwiiol_stratHCunits_toe_supply_loop;
