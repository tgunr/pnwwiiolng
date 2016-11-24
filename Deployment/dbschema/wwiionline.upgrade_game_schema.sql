-- Copyright (C) PlayNet Inc 2016.
-- Originating Programmer: Oliver 'kfs1' Smith
--
-- Various helpers for managing the database and an upgrade_game_schema proc.
--
-- Functions:
--  wwiionline.table_exists(schema, table) :- true/false
--  wwiionline.view_exists(schema, table)  :- true/false
--
-- Procedures:
--  wwiionline.move_table(from_schema, from_name,
--                  to_schema or '', to_name or '')
--  wwiionline.move_view(from_schema, from_name,
--                  to_schema or '', to_name or '')

USE `wwiionline`;

-- Simple check for whether a named schema exists.
DROP FUNCTION IF EXISTS `schema_exists`;
DELIMITER ;;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    FUNCTION schema_exists (sch TEXT)
    RETURNS int
BEGIN
    RETURN EXISTS (
        SELECT  NULL
          FROM  `information_schema`.`schemata`
         WHERE  `schema_name` = sch
         LIMIT  1
    );
END;;
DELIMITER ;

-- Simple check for whether a named table exists in a given schema.
DROP FUNCTION IF EXISTS `table_exists`;
DELIMITER ;;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    FUNCTION table_exists (sch TEXT, tab TEXT)
    RETURNS int
BEGIN
    RETURN EXISTS (
        SELECT  NULL
          FROM  `information_schema`.`tables`
         WHERE  `table_schema` = sch
           AND  `table_name` = tab
         LIMIT  1
    );
END;;
DELIMITER ;

-- Simple check for whether a named view exists in a given schema.
DROP FUNCTION IF EXISTS `view_exists`;
DELIMITER ;;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    FUNCTION `view_exists` (sch TEXT, vw TEXT)
    RETURNS int
BEGIN
    RETURN EXISTS (
        SELECT  NULL
          FROM  `information_schema`.`views`
         WHERE  `table_schema` = sch
           AND  `table_name` = vw
         LIMIT  1
    );
END;;
DELIMITER ;

-- Move a table from one schema to another.
-- If tsch is empty, acts as a rename within the same schema.
-- If ttab is empty, keeps the same table name.
DROP PROCEDURE IF EXISTS `move_table`;
DELIMITER ;;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    PROCEDURE `move_table` (fsch TEXT, ftab TEXT, tsch TEXT, ttab TEXT)
    LANGUAGE SQL
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
move_table:BEGIN
    DECLARE _stmt TEXT;

    IF NOT tsch > '' THEN SELECT fsch INTO tsch; END IF;
    IF NOT ttab > '' THEN SELECT ftab INTO ttab; END IF;

    SET @operation := CONCAT(fsch,'.',ftab, " to ", tsch,'.',ttab);

    IF NOT wwiionline.table_exists(fsch, ftab) THEN
        IF wwiionline.table_exists(tsch, ttab) THEN
            SELECT CONCAT(@operation, " skipped: already done?")
                AS Info;
        ELSE
	        SELECT CONCAT(@operation, " failed: source does not exist.")
	            AS Warning;
        END IF;
        LEAVE move_table;
    END IF;

    IF wwiionline.table_exists(tsch, ttab) THEN
        SELECT CONCAT("Can't move ", @operation, ": destination exists.")
            AS Note;
        LEAVE move_table;
    END IF;

    SELECT CONCAT("Moving ", @operation)
        AS Info;
    SET @stmt := CONCAT(
            'ALTER TABLE `', fsch, '`.`', ftab,
            '` RENAME `', tsch, '`.`', ttab, '`'
    );
    PREPARE _stmt FROM @stmt;
    EXECUTE _stmt;
    DEALLOCATE PREPARE _stmt;
END;;
DELIMITER ;

-- Transfer a view from one schema to another.
-- If tsch is empty, acts as a rename within the same schema.
-- If tvw is empty, keeps the same view name.
DROP PROCEDURE IF EXISTS `move_view`;
DELIMITER ;;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    PROCEDURE `move_view` (fsch TEXT, fvw TEXT, tsch TEXT, tvw TEXT)
    LANGUAGE SQL
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
move_view:BEGIN
    DECLARE _stmt TEXT;
    DECLARE _definer VARCHAR(128);
    DECLARE _invoker VARCHAR(128);
    DECLARE _definition TEXT;

    IF NOT tsch > '' THEN SELECT fsch INTO tsch; END IF;
    IF NOT tvw > '' THEN SELECT fvw INTO tvw; END IF;

    SET @operation := CONCAT(fsch,'.',fvw, " to ", tsch,'.',tvw);

    IF NOT wwiionline.view_exists(fsch, fvw) THEN
        IF wwiionline.view_exists(tsch, tvw) THEN
            SELECT CONCAT(@operation, " skipped: already done?")
                AS Info;
        ELSE
	        SELECT CONCAT(@operation, " failed: source does not exist.")
	            AS Warning;
        END IF;
        LEAVE move_view;
    END IF;

    IF wwiionline.view_exists(tsch, tvw) THEN
        SELECT CONCAT("Can't move ", @operation, ": destination exists.")
            AS Note;
        LEAVE move_view;
    END IF;

    SELECT CONCAT("Moving ", @operation)
        AS Info;
    SELECT `definer`, `security_type`, `view_definition` INTO _definer, _invoker, _definition
      FROM `information_schema`.`views`
     WHERE `table_schema` = fsch AND `table_name` = `fvw`
     LIMIT 1;
    SET @stmt := CONCAT(
            "CREATE DEFINER = '",_definer,"'",
            " SQL SECURITY ", _invoker,
            " VIEW `",tsch,"`.`",tvw,"`",
            " AS ", _definition
    );
    PREPARE _stmt FROM @stmt;
    EXECUTE _stmt;
    DEALLOCATE PREPARE _stmt;
    SET @stmt := CONCAT("DROP VIEW `",fsch,"`.`",fvw,"`");
    PREPARE _stmt FROM @stmt;
    EXECUTE _stmt;
    DEALLOCATE PREPARE _stmt;
END;;
DELIMITER ;

DROP PROCEDURE IF EXISTS `upgrade_game_schema`;
DELIMITER ;;
CREATE
    DEFINER = 'wwiiol'@'localhost'
    PROCEDURE `upgrade_game_schema` ()
    LANGUAGE SQL
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
BEGIN
	IF wwiionline.schema_exists('strat') THEN
		SELECT CONCAT("Depopulating `strat` -> `wwiionline`")
			AS Note;
    	CALL wwiionline.move_table('strat', 'brigade_move_results', 'wwiionline', '');
    	CALL wwiionline.move_table('strat', 'move_requests', 'wwiionline', '');
    	CALL wwiionline.move_table('strat', 'wthr_report', 'wwiionline', '');
    	CALL wwiionline.move_view('strat', 'v_last_captures', 'wwiionline', '');
    	DROP SCHEMA IF EXISTS `strat`;
    ELSE
    	SELECT CONCAT("`strat` schema already removed.")
    		AS Info;
    END IF;
END;;
DELIMITER ;
