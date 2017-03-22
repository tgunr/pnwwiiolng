USE `audit`;
DROP TABLE IF EXISTS `auth_steamcustomer`;
CREATE TABLE `auth_steamcustomer` (
  `steamID` int(11) unsigned NOT NULL,
  `customerID` int(11) unsigned NOT NULL,
  PRIMARY KEY (`steamID`),
  UNIQUE KEY `steamID` (`steamID`),
  KEY `cust_id` (`customerID`),
  KEY `steam_id` (`steamID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

DROP PROCEDURE IF EXISTS `p_authenticate_steam_user_for_arena`; 
CREATE PROCEDURE `p_authenticate_steam_user_for_arena`(
    i_steamid  INT(11) UNSIGNED
  , i_arena_id  INT(11) UNSIGNED
  )
    READS SQL DATA
    SQL SECURITY INVOKER
BEGIN
  DECLARE l_not_found TINYINT(1) UNSIGNED DEFAULT 0;
  DECLARE CONTINUE HANDLER FOR NOT FOUND SET l_not_found = 1;

  SELECT  IF ( s.status = 'active', 1, 0 ) AS is_active
     ,  p.playerID, c.handle
     ,  IF ( c.flags & 2, 1, 0 ) AS have_hw_info
    FROM wwiiol_players p
      INNER JOIN auth_customer c USING (customerID)
      INNER JOIN auth_subscribed s USING (customerID)
      INNER JOIN auth_subarenaassoc a USING (subscriptionID)
      INNER JOIN auth_steamcustomer m USING (customerID)
       WHERE m.steamID = i_steamid
         AND a.arenaID = i_arena_id
       ORDER BY s.status ASC
       LIMIT 1
  ;
END ;;
