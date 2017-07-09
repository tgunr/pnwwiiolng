-- MySQL dump 10.13  Distrib 5.7.18, for osx10.12 (x86_64)
--
-- Host: localhost    Database: auth
-- ------------------------------------------------------
-- Server version	5.7.18

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `auth`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `auth` /*!40100 DEFAULT CHARACTER SET ucs2 */;

USE `auth`;

--
-- Table structure for table `app_host_versions`
--

DROP TABLE IF EXISTS `app_host_versions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `app_host_versions` (
  `appID` int(11) unsigned NOT NULL DEFAULT '0',
  `hostVersion` varchar(20) DEFAULT '0',
  `platform` int(11) unsigned NOT NULL DEFAULT '0',
  `minClientVersion` varchar(20) DEFAULT '0',
  `maxClientVersion` varchar(20) DEFAULT '0',
  `app_index` int(11) NOT NULL,
  UNIQUE KEY `app_index` (`app_index`),
  KEY `appID` (`appID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='version informaton related to each host version and appid';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `app_host_versions`
--

LOCK TABLES `app_host_versions` WRITE;
/*!40000 ALTER TABLE `app_host_versions` DISABLE KEYS */;
/*!40000 ALTER TABLE `app_host_versions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `app_manifest`
--

DROP TABLE IF EXISTS `app_manifest`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `app_manifest` (
  `old` varchar(20) DEFAULT '0',
  `new` varchar(20) DEFAULT '0',
  `appID` int(11) NOT NULL DEFAULT '0',
  `platform` int(10) unsigned DEFAULT NULL,
  `actionType` char(1) DEFAULT '0',
  `webURL` varchar(129) DEFAULT '0',
  `ftpPort` int(11) unsigned DEFAULT '0',
  `ftpProtocol` char(1) DEFAULT '0',
  `ftpURL` varchar(129) DEFAULT '0',
  `ftpDir` varchar(129) DEFAULT '0',
  `ftpFile` varchar(129) DEFAULT '0',
  `manifest_index` int(11) NOT NULL,
  UNIQUE KEY `manifest_index` (`manifest_index`),
  UNIQUE KEY `app_man_unq_idx` (`appID`,`platform`,`old`,`new`),
  KEY `appID` (`appID`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='manifest entries for all applications served by auth';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `app_manifest`
--

LOCK TABLES `app_manifest` WRITE;
/*!40000 ALTER TABLE `app_manifest` DISABLE KEYS */;
/*!40000 ALTER TABLE `app_manifest` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `app_platforms`
--

DROP TABLE IF EXISTS `app_platforms`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `app_platforms` (
  `platformid` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(10) NOT NULL DEFAULT '',
  PRIMARY KEY (`platformid`),
  UNIQUE KEY `platformid` (`platformid`,`name`),
  KEY `platformid_2` (`platformid`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=latin1 COMMENT='Known Platforms';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `app_platforms`
--

LOCK TABLES `app_platforms` WRITE;
/*!40000 ALTER TABLE `app_platforms` DISABLE KEYS */;
INSERT INTO `app_platforms` VALUES (1,'PC'),(2,'Mac');
/*!40000 ALTER TABLE `app_platforms` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_actiontype`
--

DROP TABLE IF EXISTS `auth_actiontype`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_actiontype` (
  `actionTypeID` int(11) NOT NULL AUTO_INCREMENT,
  `description` varchar(25) DEFAULT NULL,
  PRIMARY KEY (`actionTypeID`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_actiontype`
--

LOCK TABLES `auth_actiontype` WRITE;
/*!40000 ALTER TABLE `auth_actiontype` DISABLE KEYS */;
INSERT INTO `auth_actiontype` VALUES (1,'Connect'),(2,'Disconnect');
/*!40000 ALTER TABLE `auth_actiontype` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_apps`
--

DROP TABLE IF EXISTS `auth_apps`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_apps` (
  `appid` int(11) NOT NULL DEFAULT '0',
  `gameid` int(11) NOT NULL DEFAULT '0',
  `description` varchar(255) NOT NULL DEFAULT '',
  `appleID` varchar(5) DEFAULT NULL,
  UNIQUE KEY `appid` (`appid`,`gameid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_apps`
--

LOCK TABLES `auth_apps` WRITE;
/*!40000 ALTER TABLE `auth_apps` DISABLE KEYS */;
INSERT INTO `auth_apps` VALUES (1,1,'WWII Online Blitzkrieg Production','pn01'),(2,2,'WildCards','pn02'),(5,0,'PlayGate','pn00'),(12,1,'WWII Online Blitzkrieg Beta','pn12');
/*!40000 ALTER TABLE `auth_apps` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_arenas`
--

DROP TABLE IF EXISTS `auth_arenas`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_arenas` (
  `arenaid` int(11) unsigned NOT NULL DEFAULT '0',
  `gameid` int(11) unsigned NOT NULL DEFAULT '0',
  `groupid` int(11) unsigned NOT NULL DEFAULT '0',
  `clusterid` int(11) unsigned NOT NULL DEFAULT '0',
  `hostVersion` varchar(20) CHARACTER SET latin1 NOT NULL DEFAULT '',
  `state` enum('online','offline','full','active','inactive') CHARACTER SET latin1 DEFAULT NULL,
  `controlState` enum('online','offline') CHARACTER SET latin1 DEFAULT NULL,
  `controlVisible` enum('hidden','visible') CHARACTER SET latin1 NOT NULL DEFAULT 'hidden',
  `description` varchar(255) CHARACTER SET latin1 DEFAULT NULL,
  `lock_state` smallint(6) NOT NULL DEFAULT '-4',
  `players` mediumint(9) NOT NULL DEFAULT '0',
  `alias` varchar(16) DEFAULT NULL,
  `db` varchar(25) CHARACTER SET ucs2 DEFAULT NULL,
  PRIMARY KEY (`arenaid`),
  KEY `fk_aa_gameid_idx` (`gameid`),
  KEY `fk_aa_grpid_idx` (`groupid`),
  CONSTRAINT `fk_aa_gameid` FOREIGN KEY (`gameid`) REFERENCES `auth_games` (`gameid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_aa_grpid` FOREIGN KEY (`groupid`) REFERENCES `auth_groups` (`groupid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_arenas`
--

LOCK TABLES `auth_arenas` WRITE;
/*!40000 ALTER TABLE `auth_arenas` DISABLE KEYS */;
INSERT INTO `auth_arenas` VALUES (1,1,1,3,'','online',NULL,'visible','WWII Online Blitzkrieg Production',-4,0,'Campaign','livedb'),(99,1,2,2,'','online',NULL,'hidden','WWII Online Blitzkrieg External Beta',-4,0,NULL,NULL),(100,1,2,6,'','offline',NULL,'visible','WWII Online Development',-4,0,'Int',NULL),(222,1,2,5,'','online',NULL,'visible','WWII Online Blitzkrieg Internal Beta',-4,0,'Beta',NULL),(250,1,2,4,'','offline',NULL,'visible','WWII Online Internal Test',-4,0,'Dev',NULL);
/*!40000 ALTER TABLE `auth_arenas` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_checksums`
--

DROP TABLE IF EXISTS `auth_checksums`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_checksums` (
  `check_oid` int(11) NOT NULL AUTO_INCREMENT,
  `appID` int(11) NOT NULL DEFAULT '0',
  `platform` int(11) NOT NULL DEFAULT '0',
  `checksum` varchar(64) DEFAULT NULL,
  `filename` varchar(128) NOT NULL DEFAULT '',
  PRIMARY KEY (`check_oid`),
  UNIQUE KEY `appID` (`appID`,`platform`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_checksums`
--

LOCK TABLES `auth_checksums` WRITE;
/*!40000 ALTER TABLE `auth_checksums` DISABLE KEYS */;
INSERT INTO `auth_checksums` VALUES (1,5,0,'56372e3b5636aeb163cbba59ce3f4e02','tos.txt'),(2,5,1,'56372e3b5636aeb163cbba59ce3f4e02','tos.txt');
/*!40000 ALTER TABLE `auth_checksums` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_connections`
--

DROP TABLE IF EXISTS `auth_connections`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_connections` (
  `occurred` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `customerID` int(11) unsigned NOT NULL,
  `arenaID` int(11) unsigned NOT NULL,
  `ip` int(11) unsigned NOT NULL DEFAULT '0',
  `mac_address` bigint(20) unsigned DEFAULT NULL,
  `udid` char(36) NOT NULL DEFAULT '00000000-0000-0000-0000-000000000000',
  KEY `occurred` (`occurred`,`customerID`),
  KEY `customerid` (`customerID`),
  KEY `ip` (`ip`),
  KEY `occurred_2` (`occurred`,`ip`,`customerID`),
  KEY `idx_mac_address` (`mac_address`),
  KEY `idx_udid` (`udid`),
  KEY `occurred3` (`occurred`,`ip`,`mac_address`,`udid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_connections`
--

LOCK TABLES `auth_connections` WRITE;
/*!40000 ALTER TABLE `auth_connections` DISABLE KEYS */;
/*!40000 ALTER TABLE `auth_connections` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_customer`
--

DROP TABLE IF EXISTS `auth_customer`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_customer` (
  `customerID` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `username` char(16) CHARACTER SET latin1 NOT NULL,
  `password` char(32) CHARACTER SET latin1 NOT NULL,
  `handle` char(10) NOT NULL DEFAULT '',
  `status` enum('new','sync','normal') CHARACTER SET latin1 DEFAULT NULL,
  `flags` int(11) DEFAULT '0',
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`customerID`),
  UNIQUE KEY `username` (`username`),
  KEY `handle` (`handle`(8))
) ENGINE=MyISAM AUTO_INCREMENT=1503419 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_customer`
--

LOCK TABLES `auth_customer` WRITE;
/*!40000 ALTER TABLE `auth_customer` DISABLE KEYS */;
/*!40000 ALTER TABLE `auth_customer` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_games`
--

DROP TABLE IF EXISTS `auth_games`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_games` (
  `gameid` int(11) unsigned NOT NULL DEFAULT '0',
  `description` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`gameid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_games`
--

LOCK TABLES `auth_games` WRITE;
/*!40000 ALTER TABLE `auth_games` DISABLE KEYS */;
INSERT INTO `auth_games` VALUES (1,'WWII Online'),(2,'Wildcards');
/*!40000 ALTER TABLE `auth_games` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_groups`
--

DROP TABLE IF EXISTS `auth_groups`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_groups` (
  `groupid` int(11) unsigned NOT NULL DEFAULT '0',
  `description` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`groupid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_groups`
--

LOCK TABLES `auth_groups` WRITE;
/*!40000 ALTER TABLE `auth_groups` DISABLE KEYS */;
INSERT INTO `auth_groups` VALUES (1,'WWII Online Blitzkrieg Production'),(2,'WWII Online Blitzkrieg Beta');
/*!40000 ALTER TABLE `auth_groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_illegal_users`
--

DROP TABLE IF EXISTS `auth_illegal_users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_illegal_users` (
  `recorded` datetime DEFAULT NULL,
  `ip` int(11) unsigned DEFAULT NULL,
  `username` varchar(40) DEFAULT NULL,
  `reason` varchar(20) DEFAULT NULL,
  KEY `username` (`username`),
  KEY `reason` (`reason`),
  KEY `recorded` (`recorded`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_illegal_users`
--

LOCK TABLES `auth_illegal_users` WRITE;
/*!40000 ALTER TABLE `auth_illegal_users` DISABLE KEYS */;
/*!40000 ALTER TABLE `auth_illegal_users` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_otp`
--

DROP TABLE IF EXISTS `auth_otp`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_otp` (
  `username` varchar(20) NOT NULL DEFAULT '',
  `otp` varchar(20) NOT NULL DEFAULT '',
  `homedir` varchar(20) NOT NULL DEFAULT '/home/ftpchroot/DENY'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_otp`
--

LOCK TABLES `auth_otp` WRITE;
/*!40000 ALTER TABLE `auth_otp` DISABLE KEYS */;
/*!40000 ALTER TABLE `auth_otp` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_population_grades`
--

DROP TABLE IF EXISTS `auth_population_grades`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_population_grades` (
  `min_players` mediumint(8) unsigned NOT NULL,
  `grade_name` varchar(32) NOT NULL,
  PRIMARY KEY (`min_players`),
  UNIQUE KEY `min_players` (`min_players`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_population_grades`
--

LOCK TABLES `auth_population_grades` WRITE;
/*!40000 ALTER TABLE `auth_population_grades` DISABLE KEYS */;
INSERT INTO `auth_population_grades` VALUES (0,'Empty'),(1,'Very Light'),(61,'Low'),(251,'Average'),(351,'Good'),(701,'High'),(1201,'Very High');
/*!40000 ALTER TABLE `auth_population_grades` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_subarenaassoc`
--

DROP TABLE IF EXISTS `auth_subarenaassoc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_subarenaassoc` (
  `subscriptionID` int(11) unsigned NOT NULL DEFAULT '0',
  `arenaID` int(11) unsigned NOT NULL DEFAULT '0',
  UNIQUE KEY `arenaID` (`arenaID`,`subscriptionID`),
  KEY `idx_asaa_sub_arena` (`subscriptionID`,`arenaID`),
  CONSTRAINT `fk_saa_arenaid` FOREIGN KEY (`arenaID`) REFERENCES `auth_arenas` (`arenaid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_saa_subid` FOREIGN KEY (`subscriptionID`) REFERENCES `auth_subscription` (`subscriptionID`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_subarenaassoc`
--

LOCK TABLES `auth_subarenaassoc` WRITE;
/*!40000 ALTER TABLE `auth_subarenaassoc` DISABLE KEYS */;
INSERT INTO `auth_subarenaassoc` VALUES (3,1),(9,1),(134,1),(137,1),(154,1),(174,1),(187,1),(188,1),(189,1),(190,1),(191,1),(192,1),(193,1),(194,1),(195,1),(196,1),(197,1),(200,1),(201,1),(205,1),(214,1),(229,1),(230,1),(2,99),(40,99),(2,100),(3,100),(9,100),(2,222),(3,222),(9,222),(187,222),(188,222),(197,222),(200,222),(205,222),(209,222),(2,250),(9,250);
/*!40000 ALTER TABLE `auth_subarenaassoc` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_subgroupassoc`
--

DROP TABLE IF EXISTS `auth_subgroupassoc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_subgroupassoc` (
  `subscriptionID` int(11) unsigned NOT NULL COMMENT 'Subscription associated being related (see auth_subscription)',
  `groupID` smallint(6) unsigned NOT NULL COMMENT 'group_oid of chat_group being related',
  PRIMARY KEY (`subscriptionID`,`groupID`),
  KEY `idx_asga_inverse` (`groupID`,`subscriptionID`),
  CONSTRAINT `fk_sga_chatgrpid` FOREIGN KEY (`groupID`) REFERENCES `chat_groups` (`group_oid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_sga_subid` FOREIGN KEY (`subscriptionID`) REFERENCES `auth_subscription` (`subscriptionID`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='Relates subscriptions and chat_groups';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_subgroupassoc`
--

LOCK TABLES `auth_subgroupassoc` WRITE;
/*!40000 ALTER TABLE `auth_subgroupassoc` DISABLE KEYS */;
INSERT INTO `auth_subgroupassoc` VALUES (2,4),(2,5),(2,6),(2,7),(2,11),(2,14),(2,15),(2,22),(2,27),(2,28),(3,8),(3,22),(9,1),(9,4),(9,5),(9,6),(9,7),(9,11),(9,14),(9,15),(9,22),(9,27),(9,28),(134,8),(134,22),(154,8),(154,22),(174,8),(174,22),(187,8),(187,22),(187,23),(188,1),(188,11),(188,14),(188,15),(188,22),(188,24),(189,3),(189,22),(190,8),(190,10),(190,22),(191,4),(191,22),(192,4),(192,5),(192,22),(193,4),(193,5),(193,14),(193,22),(194,6),(194,22),(195,6),(195,7),(195,22),(196,6),(196,7),(196,15),(196,22),(197,25),(199,3),(199,4),(199,5),(199,6),(199,7),(199,10),(199,14),(199,15),(199,22),(200,26),(201,8),(201,22),(202,8),(202,22),(205,22),(205,47),(206,8),(206,22),(209,8),(209,22),(209,27),(209,28),(213,8),(213,22),(213,27),(213,28),(214,8),(214,22),(229,48),(230,49);
/*!40000 ALTER TABLE `auth_subgroupassoc` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_subscribed`
--

DROP TABLE IF EXISTS `auth_subscribed`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_subscribed` (
  `customerID` int(11) unsigned NOT NULL DEFAULT '0',
  `subscriptionID` int(11) unsigned NOT NULL DEFAULT '0',
  `status` enum('active','inactive') DEFAULT NULL,
  UNIQUE KEY `customerID` (`customerID`,`subscriptionID`),
  KEY `idx_as_substatcust` (`subscriptionID`,`status`,`customerID`),
  KEY `idx_as_subcust` (`subscriptionID`,`customerID`),
  CONSTRAINT `fk_custid` FOREIGN KEY (`customerID`) REFERENCES `auth_customer` (`customerID`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_subid` FOREIGN KEY (`subscriptionID`) REFERENCES `auth_subscription` (`subscriptionID`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_subscribed`
--

LOCK TABLES `auth_subscribed` WRITE;
/*!40000 ALTER TABLE `auth_subscribed` DISABLE KEYS */;
/*!40000 ALTER TABLE `auth_subscribed` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_subscription`
--

DROP TABLE IF EXISTS `auth_subscription`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_subscription` (
  `subscriptionID` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `subcode` char(4) NOT NULL DEFAULT '' COMMENT 'Human readable identifier',
  `description` char(128) NOT NULL COMMENT 'Internal description of subscription.',
  `type` enum('permissions','access') DEFAULT 'permissions',
  PRIMARY KEY (`subscriptionID`),
  UNIQUE KEY `subcode` (`subcode`)
) ENGINE=InnoDB AUTO_INCREMENT=231 DEFAULT CHARSET=ucs2 COMMENT='Declares the group of subscription sets';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_subscription`
--

LOCK TABLES `auth_subscription` WRITE;
/*!40000 ALTER TABLE `auth_subscription` DISABLE KEYS */;
INSERT INTO `auth_subscription` VALUES (1,'FREE','Playnet Free Games Subscription.','access'),(2,'PERB','Closed Beta','access'),(3,'RETP','Premium','access'),(4,'WCOM','WWII Online Community Subscription','access'),(5,'MEAT','Meat Subcription, do not ADD!!!','access'),(6,'WPAY','WWII Online Monthly','access'),(7,'WPRE','WWII Online 6-Month','access'),(8,'DOGF','Dogfighter Community','access'),(9,'CRS','CRS Staff','access'),(10,'DSMO','Darkspace Monthly','access'),(11,'DS6M','Darkspace 6-Month Prepay','access'),(12,'DSYR','Darkspace 12-Month Prepay','access'),(13,'RVAL','WWII Online Re-Evaluation','access'),(14,'GGEX','WWII Online Gigex Promo','access'),(15,'INT+','WWII Online International Monthly + CD Key','access'),(16,'INT','WWII Online International Monthly','access'),(17,'MBTA','WWII Online Mac Beta','access'),(18,'DS3M','Darkspace 3-Month Prepay','access'),(19,'E3PR','WWII Online E3 Promo','access'),(20,'W1M+','WWII Online Monthly + Activation Key','access'),(21,'W3M+','WWII Online 3-Month + Activation Key','access'),(22,'W6M+','WWII Online 6-Month + Activation Key','access'),(23,'W3M','WWII Online 3-Month','access'),(24,'PSUP','Playnet Support','access'),(25,'IPCG','WWII Online Incite PCGames Promo','access'),(26,'REXT','Special Prepay Offer','access'),(27,'PCGP','PC GAMEPLAY Promo','access'),(28,'SWDN','WWII Online PC GAMER SVENSKA Promo','access'),(29,'WJOY','WWII Online JOYSTICK FRANCE Promo','access'),(30,'JS6M','WWII Online 6-Month Joystick Special','access'),(31,'EALF','WWII Online Events Allied Fighter','access'),(32,'EALB','WWII Online Events Allied Bomber','access'),(33,'EALS','WWII Online Events Allied Ship','access'),(34,'EAXF','WWII Online Events Axis Fighter','access'),(35,'EAXB','WWII Online Events Axis Bomber','access'),(36,'EAXS','WWII Online Events Axis Ship','access'),(37,'IMGP','WWII Online INSIDEMACGAMES Promo','access'),(38,'GIGX','WWII Online Gigex Promo','access'),(39,'GMC','WWII Online GMC Promo','access'),(40,'OPBE','WWII Online Open Beta','access'),(41,'SCAN','WWII Online ScanDH promo','access'),(42,'WYT','WWII Online Yearly (TShirt)','access'),(43,'WYT+','WWII Online Yearly (TShirt) + Activation Key','access'),(44,'WYER','WWII Online Yearly','access'),(45,'WYR+','WWII Online Yearly + Activation Key','access'),(46,'WNGP','WWII Online Netgamer Promo','access'),(47,'WTGP','WWII Online Total Games PC Promo','access'),(48,'WMCS','WWII Online Microsim Promo','access'),(50,'EVNT','WWII Online Events Pass','access'),(54,'W5YR','WWII Online 5-Year','access'),(55,'WLFE','WWII Online Lifetime','access'),(56,'WFEP','WWII Online Free Events Promo','access'),(57,'WATT','WWII Online ATT Demo','access'),(58,'WMAG','WWII Online WW2 Mag Promo','access'),(59,'WPEL','WWII Online Pelaaja Promo','access'),(60,'WMDK','WWII Online Monthly (DK)','access'),(61,'WT2','WWII Online Yearly (T-Shirt)','access'),(62,'WT2+','WWII Online Yearly (T-Shirt) + Activation Key','access'),(63,'W3MS','WWII Online 3-Month Special','access'),(64,'WPCT','WWII Online PC Team Promo','access'),(65,'WFRT','WWII Online Filefront Promo','access'),(66,'WGMC','WWII Online GMC Promo','access'),(67,'WWAR','WWII Online PC Zone Promo','access'),(68,'WJUX','WWII Online PC Jeux Promo','access'),(69,'WMG2','WWII Online WWII Mag Promo','access'),(70,'WSWG','WWII Online SWG 501st Rogue','access'),(71,'WTIR','WWII Online Track IR','access'),(72,'W12B','WWII Online 12-Month Builders','access'),(73,'WFSP','WWII Online FiringSquad','access'),(122,'MMOR','WWII Online MMORPG Free Trial','access'),(134,'PRGR','Press Group','access'),(137,'BELM','BE Limited Trial','access'),(144,'MT14','Matrix Games 14 Day Free Trial','access'),(154,'BE14','BE 14-day Free Trial (K)','access'),(168,'BSEP','Bridges SE 7-Day Free Trial','access'),(169,'WB07','Welcome Back Soldier 2007','access'),(171,'MINC','Mini-Con Sub','access'),(172,'GIGA','Giga TV Trial','access'),(174,'B214','BE 14-day Free Trial (NK)','access'),(183,'WBS8','Welcome Back Soldier 2008','access'),(186,'WBSX','Welcome Back Soldier Christmas 2008','access'),(187,'DARK','CRS Darkglasses','permissions'),(188,'PROD','CRS Producer','permissions'),(189,'GM','GM','permissions'),(190,'TRAI','Trainer','permissions'),(191,'AHC','AHC','permissions'),(192,'AHC1','AHC+','permissions'),(193,'AHC2','AHC++','permissions'),(194,'GHC','GHC','permissions'),(195,'GHC1','GHC+','permissions'),(196,'GHC2','GHC++','permissions'),(197,'FP','Trial Bonus','access'),(199,'EVST','Event Staff','access'),(200,'BLDR','Builders','permissions'),(201,'FAFT','Fighter Ace Free Trial','access'),(202,'131B','1.31 Open Beta','access'),(205,'ALl','14 Day All Access Trial','access'),(206,'WBSY','Welcome Back Soldier v1.32','access'),(209,'BBTA','Builders Beta','access'),(213,'BSQD','Squads Beta','access'),(214,'WBSO','WBS Oct 2011','access'),(215,'s215','Rapid Assault Closed Beta','access'),(221,'s221','Rapid Assault Preorder Beta','access'),(229,'s229','Free Play','access'),(230,'s230','Basic','access');
/*!40000 ALTER TABLE `auth_subscription` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `auth_variables`
--

DROP TABLE IF EXISTS `auth_variables`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth_variables` (
  `name` varchar(30) NOT NULL DEFAULT '',
  `value` text,
  PRIMARY KEY (`name`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `auth_variables`
--

LOCK TABLES `auth_variables` WRITE;
/*!40000 ALTER TABLE `auth_variables` DISABLE KEYS */;
INSERT INTO `auth_variables` VALUES ('last_otp','3119092393');
/*!40000 ALTER TABLE `auth_variables` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `chat_group_members`
--

DROP TABLE IF EXISTS `chat_group_members`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `chat_group_members` (
  `group_oid` smallint(6) unsigned NOT NULL,
  `playerid` int(11) unsigned NOT NULL,
  `added` datetime DEFAULT NULL,
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`group_oid`,`playerid`),
  KEY `cgm_fk_player_id_idx` (`playerid`),
  CONSTRAINT `cgm_fk_group_oid` FOREIGN KEY (`group_oid`) REFERENCES `chat_groups` (`group_oid`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `cgm_fk_player_id` FOREIGN KEY (`playerid`) REFERENCES `wwiionline`.`wwii_player` (`playerid`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `chat_group_members`
--

LOCK TABLES `chat_group_members` WRITE;
/*!40000 ALTER TABLE `chat_group_members` DISABLE KEYS */;
/*!40000 ALTER TABLE `chat_group_members` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `chat_groups`
--

DROP TABLE IF EXISTS `chat_groups`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `chat_groups` (
  `group_oid` smallint(6) unsigned NOT NULL AUTO_INCREMENT COMMENT 'NOTE: Range 1-65535, host stores as 16-bit int',
  `name` char(20) CHARACTER SET ucs2 NOT NULL COMMENT 'Short/descriptive name',
  `description` char(80) CHARACTER SET ucs2 NOT NULL COMMENT 'Explanation of group/purpose',
  `auto_by_rank` enum('N','Y') NOT NULL DEFAULT 'N' COMMENT 'Deprecated: Set to Y to enable auto joining by rank',
  `auto_min_rank` tinyint(3) unsigned NOT NULL DEFAULT '99' COMMENT 'Deprecated: Min. rank for auto-joining',
  `auto_max_rank` tinyint(3) unsigned NOT NULL DEFAULT '99' COMMENT 'Deprecated: Max. rank for auto-joining',
  `auto_side` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT 'Deprecated: 0 or side requried for auto-join',
  `grants` bigint(20) unsigned NOT NULL COMMENT '64-bit bitfield of permissions granted by membership (see wwii_permissions)',
  `max_squad_rank` tinyint(3) unsigned NOT NULL DEFAULT '0' COMMENT 'Highest squad rank this group grants',
  `enabled` enum('N','Y') NOT NULL DEFAULT 'Y' COMMENT 'Set to N to indicate group is no-longer in use instead of deleting while servers are operational',
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Track when entry was last changed',
  PRIMARY KEY (`group_oid`),
  UNIQUE KEY `name` (`name`),
  UNIQUE KEY `uk_cg_name` (`name`),
  KEY `idx_cg_main` (`group_oid`,`enabled`,`grants`,`max_squad_rank`),
  KEY `idx_cg_modified` (`modified`)
) ENGINE=InnoDB AUTO_INCREMENT=50 DEFAULT CHARSET=utf8 COMMENT='Chat groups describe permission sets (permission masks, rank';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `chat_groups`
--

LOCK TABLES `chat_groups` WRITE;
/*!40000 ALTER TABLE `chat_groups` DISABLE KEYS */;
INSERT INTO `chat_groups` VALUES (1,'staff','Staff Accounts','N',99,99,0,4611686544971399714,25,'Y','2011-11-10 22:34:10'),(3,'gms','Game Managers','N',99,99,0,483325378560,0,'Y','2009-07-01 19:08:13'),(4,'ahc','Allied HC rank 14 - Brigade Staff','N',99,99,0,23068674,0,'Y','2009-06-27 19:38:55'),(5,'ahc+','AHC ank 18 - Brigade CO','N',99,99,0,23068674,0,'Y','2009-06-27 19:38:55'),(6,'ghc','Axis HC rank 14 - Brigade Staff','N',99,99,0,39845890,0,'Y','2009-06-27 19:38:56'),(7,'ghc+','GHC rank 18 - Brigade CO','N',99,99,0,39845890,0,'Y','2009-07-13 19:02:26'),(8,'premium','Premium Subscribers','N',99,99,0,515409183232,255,'Y','2011-11-10 22:34:11'),(10,'trainer','Trainers','N',99,99,0,420914659874,0,'Y','2011-11-10 22:34:12'),(11,'beta','Closed Beta Testers','N',0,99,0,482703049248,255,'Y','2011-11-10 22:34:18'),(14,'ahc++','AHC rank 22 - Command Staff','N',99,99,0,559939586,0,'Y','2009-06-27 19:38:59'),(15,'ghc++','GHC rank 22 - Command Staff','N',99,99,0,1113587714,0,'Y','2009-06-27 19:39:00'),(22,'basic','Basic Commands Available to All Players','N',0,99,0,34370224128,0,'Y','2010-10-13 22:11:36'),(23,'darkglasses','Staff Dark Glasses','N',0,99,0,2305843009219985922,5,'Y','2009-05-28 00:56:41'),(24,'producer','Production Team','N',0,99,0,5764608016879452706,5,'Y','2011-11-10 22:34:19'),(25,'trialbonus','Trial Bonus promo','N',0,99,0,68726292480,2,'Y','2012-08-08 18:06:42'),(26,'builder','Builders','N',0,99,0,51546423808,0,'Y','2011-11-10 22:34:21'),(27,'product01','D-Day Bonus Pack','N',0,99,0,549755813888,0,'Y','2011-07-29 21:29:38'),(28,'product02','D-Day Weapon Pack','N',0,99,0,1099511627776,0,'Y','2011-07-29 21:45:59'),(29,'product03','Product Permission Group 03','N',0,99,0,2199023255552,0,'Y','2011-07-21 22:12:21'),(30,'product04','Product Permission Group 04','N',0,99,0,4398046511104,0,'Y','2011-07-21 22:12:23'),(31,'product05','Product Permission Group 05','N',0,99,0,8796093022208,0,'Y','2011-07-21 22:12:29'),(32,'product06','Product Permission Group 06','N',0,99,0,17592186044416,0,'Y','2011-07-21 22:12:36'),(33,'product07','Product Permission Group 07','N',0,99,0,35184372088832,0,'Y','2011-07-21 22:12:42'),(34,'product08','Product Permission Group 08','N',0,99,0,70368744177664,0,'Y','2011-07-21 22:12:45'),(35,'product09','Product Permission Group 09','N',0,99,0,140737488355328,0,'Y','2011-07-21 22:12:46'),(36,'product10','Product Permission Group 10','N',0,99,0,281474976710656,0,'Y','2011-07-21 22:12:49'),(37,'product11','Product Permission Group 11','N',0,99,0,562949953421312,0,'Y','2011-07-21 22:12:52'),(38,'product12','Product Permission Group 12','N',0,99,0,1125899906842624,0,'Y','2011-07-21 22:12:57'),(39,'product13','Product Permission Group 13','N',0,99,0,2251799813685248,0,'Y','2011-07-21 22:12:59'),(40,'product14','Product Permission Group 14','N',0,99,0,4503599627370496,0,'Y','2011-07-21 22:13:01'),(41,'product15','Product Permission Group 15','N',0,99,0,9007199254740992,0,'Y','2011-07-21 22:13:08'),(42,'product16','Product Permission Group 16','N',0,99,0,18014398509481984,0,'Y','2011-07-21 22:13:10'),(43,'product17','Product Permission Group 17','N',0,99,0,36028797018963968,0,'Y','2011-07-21 22:13:12'),(44,'product18','Product Permission Group 18','N',0,99,0,72057594037927936,0,'Y','2011-07-21 22:13:14'),(45,'product19','Product Permission Group 19','N',0,99,0,144115188075855872,0,'Y','2011-07-21 22:13:17'),(46,'product20','Product Permission Group 20','N',0,99,0,288230376151711744,0,'Y','2011-07-21 22:13:27'),(47,'allaccess','All Access Trial','N',0,99,0,377970229760,25,'Y','2011-11-10 22:34:26'),(48,'freeplay','Free Play promo','N',0,99,0,2621952,2,'Y','2012-08-09 19:30:20'),(49,'limited','Limited Play','N',0,99,0,6816257,5,'Y','2012-08-20 20:00:54');
/*!40000 ALTER TABLE `chat_groups` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `info_ogl`
--

DROP TABLE IF EXISTS `info_ogl`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `info_ogl` (
  `card_name` char(48) CHARACTER SET latin1 NOT NULL DEFAULT '',
  `vendor` char(16) CHARACTER SET latin1 NOT NULL DEFAULT '',
  `ogl_version` int(11) NOT NULL DEFAULT '0',
  `num_comp_textures` int(11) NOT NULL DEFAULT '0',
  `comp_texture_descs` text CHARACTER SET latin1,
  `num_extensions` int(11) NOT NULL DEFAULT '0',
  `extensions_bitmap` text CHARACTER SET latin1,
  `format_number` smallint(6) NOT NULL DEFAULT '0',
  `n_size` smallint(6) NOT NULL DEFAULT '0',
  `n_version` smallint(6) NOT NULL DEFAULT '0',
  `dwflags` smallint(6) NOT NULL DEFAULT '0',
  `i_pixel_type` smallint(6) NOT NULL DEFAULT '0',
  `c_color_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_red_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_green_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_blue_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_alpha_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_accum_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_accum_red_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_accum_green_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_accum_blue_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_accum_alpha_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_depth_bits` smallint(6) NOT NULL DEFAULT '0',
  `c_stencil_bits` smallint(6) NOT NULL DEFAULT '0',
  `mem_phys` int(11) unsigned DEFAULT '0',
  `mem_page` int(11) unsigned DEFAULT '0',
  `mem_virtual` int(11) unsigned DEFAULT '0',
  `sound_name` char(48) CHARACTER SET latin1 DEFAULT '',
  `sound_manuf_id` int(11) unsigned DEFAULT '0',
  `sound_prod_id` int(11) unsigned DEFAULT '0',
  `cpu_manufacturer` char(40) CHARACTER SET latin1 NOT NULL DEFAULT '',
  `cpu_brandname` char(40) CHARACTER SET latin1 NOT NULL DEFAULT '',
  `cpu_model` int(6) unsigned DEFAULT '0',
  `cpu_family` int(6) unsigned DEFAULT '0',
  `cpu_stepping` int(6) unsigned DEFAULT '0',
  `cpu_processor_type` int(6) unsigned DEFAULT '0',
  `cpu_brand_index` int(6) unsigned DEFAULT '0',
  `cpu_ext_model` int(11) unsigned DEFAULT '0',
  `cpu_ext_family` int(11) unsigned DEFAULT '0',
  `cpu_features` int(11) unsigned DEFAULT '0' COMMENT 'CPU features bitfield',
  `cpu_ext_features` int(6) unsigned DEFAULT '0' COMMENT 'Additional CPU features bitfield',
  `cpu_apic_id` int(6) unsigned DEFAULT '0' COMMENT 'APIC Physical ID',
  `cpu_l2_assoc` int(6) unsigned DEFAULT '0' COMMENT 'L2 cache associativitiy',
  `cpu_cls` int(6) unsigned DEFAULT '0' COMMENT 'Cache Line Size',
  `cpu_clflush` int(6) unsigned DEFAULT '0' COMMENT 'CLFLUSH Cache Line Size',
  `cpu_cache` int(11) unsigned DEFAULT '0' COMMENT 'Cache Size (K)',
  `cpu_speed` bigint(20) unsigned DEFAULT '0' COMMENT 'Est. CPU Speed (Hz)'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `info_ogl`
--

LOCK TABLES `info_ogl` WRITE;
/*!40000 ALTER TABLE `info_ogl` DISABLE KEYS */;
/*!40000 ALTER TABLE `info_ogl` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Temporary table structure for view `v_customer_active_groups`
--

DROP TABLE IF EXISTS `v_customer_active_groups`;
/*!50001 DROP VIEW IF EXISTS `v_customer_active_groups`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE VIEW `v_customer_active_groups` AS SELECT 
 1 AS `customerid`,
 1 AS `arenaid`,
 1 AS `group_oid`*/;
SET character_set_client = @saved_cs_client;

--
-- Temporary table structure for view `v_player_active_groups`
--

DROP TABLE IF EXISTS `v_player_active_groups`;
/*!50001 DROP VIEW IF EXISTS `v_player_active_groups`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE VIEW `v_player_active_groups` AS SELECT 
 1 AS `playerid`,
 1 AS `arenaid`,
 1 AS `group_oid`*/;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `wwii_permissions`
--

DROP TABLE IF EXISTS `wwii_permissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `wwii_permissions` (
  `permissionID` int(11) unsigned NOT NULL COMMENT 'Table index',
  `maskValue` bigint(20) unsigned NOT NULL COMMENT '64-bit binary bit value of this permission',
  `description` varchar(64) NOT NULL COMMENT 'Host name for this permission (English)',
  `modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `added` datetime NOT NULL COMMENT 'Initial add datetime',
  `internal` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '1 indicates this value is only for use internally within the host',
  `details` varchar(128) DEFAULT NULL COMMENT 'Extended description of this permission',
  PRIMARY KEY (`permissionID`),
  UNIQUE KEY `maskValue` (`maskValue`),
  UNIQUE KEY `description` (`description`),
  UNIQUE KEY `perm_mask_unq` (`maskValue`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `wwii_permissions`
--

LOCK TABLES `wwii_permissions` WRITE;
/*!40000 ALTER TABLE `wwii_permissions` DISABLE KEYS */;
INSERT INTO `wwii_permissions` VALUES (1,1,'POST_MISSIONS','2009-02-18 20:20:01','2001-04-28 03:14:14',0,'Allows player full mission posting privs'),(2,2,'BREVET_POSTING','2009-02-18 20:19:09','2001-04-28 03:14:14',0,'Allows posting in brigades with an HC officer present'),(3,4,'SQUAD_MISSIONS','2009-02-18 20:19:18','2001-04-28 03:14:14',0,'Allows creation of Squad Missions'),(4,8,'SQUAD_CO','2009-02-18 20:19:42','2001-04-28 03:14:14',1,'Indicates player is in a squad with rank of CO'),(5,16,'IS_SPAWNED','2009-02-18 20:19:51','2001-04-28 03:14:14',1,'Indicates player is currently spawned'),(6,32,'CROSS_TALK','2009-02-18 20:20:17','2001-04-28 03:14:14',0,'Allows player to message players on the opposite side'),(7,64,'CGI','2009-02-18 20:20:36','2001-04-28 03:14:14',1,'Allows access to CGI in web tools'),(8,128,'MISSION_LEADER','2009-02-18 20:20:44','2001-04-28 03:14:14',1,'Indicates player is leading a mission'),(9,256,'BRIGADE_LEADER','2009-02-18 20:42:02','2001-04-28 03:14:14',1,'Indicates player is leading a brigade'),(10,512,'CAN_CAPTURE','2009-02-18 20:21:30','2009-02-06 17:30:27',0,'Allows player to capture facilities'),(11,1024,'VEHICLE_LEVEL4','2009-02-18 20:42:10','2001-04-28 03:14:14',1,'Indicates access to vehicles L4'),(12,2048,'VEHICLE_LEVEL5','2009-02-18 20:42:17','2001-04-28 03:14:14',1,'Indicates access to vehicles L5'),(13,4096,'VEHICLE_LEVEL6','2009-02-18 20:42:24','2001-04-28 03:14:14',1,'Indicates access to vehicles L6'),(14,8192,'VEHICLE_LEVEL7','2009-02-18 20:42:29','2001-04-28 03:14:14',1,'Indicates access to vehicles L7'),(15,16384,'SUBSCRIPTION_ACTIVE','2009-03-17 21:27:50','2009-03-17 15:27:50',1,'Indicates player has active subscriptions to one or more groups for the current arena'),(16,32768,'VEHICLE_LEVEL1','2009-02-18 20:42:43','2001-04-28 03:14:14',1,'Indicates access to vehicles L1'),(17,65536,'VEHICLE_LEVEL2','2009-02-18 20:43:41','2001-04-28 03:14:14',1,'Indicates access to vehicles L2'),(18,131072,'VEHICLE_LEVEL3','2009-02-18 20:43:46','2001-04-28 03:14:14',1,'Indicates access to vehicles L3'),(19,262144,'ACTIVE_COMMANDER','2009-02-18 20:44:08','2004-04-01 14:01:01',1,'Indicates ALLIED_COMMAND_STAFF/AXIS_COMMAND_STAFF permission in effect'),(20,524288,'UNMETERED_ACCESS','2009-03-04 01:32:32','2009-03-03 18:32:32',0,'Indicates account is not using pay-to-play minute balancing'),(21,1048576,'FREE_SIDE_SWITCH','2009-02-18 20:22:14','2001-11-08 18:32:19',0,'Allows player to change sides with no delays'),(22,2097152,'IS_MATURE','2009-02-18 20:22:47','2002-01-07 13:03:50',0,'Allows player Mature feature access (no AA timer)'),(23,4194304,'USE_BINOCS','2010-06-26 23:23:46','2010-06-26 21:23:46',0,'Allows use of binoculars'),(24,8388608,'CAN_OIC','2010-06-26 23:23:46','2010-06-26 21:23:46',0,'Allows player to OIC brigades'),(25,16777216,'ALLIED_COMMAND_STAFF','2009-02-18 20:23:05','2002-01-25 15:09:10',0,'Allows ACTIVE_OFFICER while playing Allied'),(26,33554432,'AXIS_COMMAND_STAFF','2009-02-18 20:23:19','2002-01-25 15:10:33',0,'Allows ACTIVE_OFFICER while playing Axis'),(27,67108864,'VEHICLE_LEVEL8','2009-02-18 20:27:04','2002-02-28 20:59:15',0,'Allows spawning of L8 vehicles (Skull)'),(28,134217728,'VEHICLE_LEVEL9','2009-02-18 20:24:31','2002-02-28 21:00:19',0,'Allows spawning of L9 and above vehicles (Buzzard)'),(29,268435456,'ALL_SPAWN','2009-02-18 20:27:58','2002-10-16 12:03:13',0,'Allows non-supply based spawning of all vehicles'),(30,536870912,'ALLIED_MOTD','2009-02-18 20:28:54','2003-01-27 21:57:49',0,'Allows player to set the Allied MOTD while Allied'),(31,1073741824,'AXIS_MOTD','2009-02-18 20:29:00','2003-01-27 21:57:52',0,'Allows player ot set the Axis MOTD while Axis'),(32,2147483648,'GAME_MANAGER','2009-02-18 20:29:14','2003-06-16 14:21:10',0,'Allows player access to GM channel and features'),(33,4294967296,'CAN_BE_EJECTED','2009-02-18 20:30:45','2003-08-05 12:35:41',0,'Allows server to eject player instead of logging suspicious behavior'),(34,8589934592,'TRAINER','2009-02-18 20:31:29','2004-01-08 12:23:08',0,'Allows access to the trainer channel and features'),(35,17179869184,'IS_BUILDER','2009-02-18 20:32:18','2004-12-10 13:30:14',0,'Allows access to builder programme features and adds rank icon'),(36,34359738368,'CAN_USE_PPO_TOOL','2010-10-13 23:22:19','0000-00-00 00:00:00',0,'Allows access to the PPO building tool'),(37,68719476736,'UNMASKED_VEHICLES','2009-02-18 20:32:34','2009-02-06 17:30:27',0,'Allows access to vehicles without applying subscription vehicle limits'),(38,137438953472,'NO_ADS','2009-02-18 20:32:58','2009-02-06 17:30:27',0,'Allows gameplay without upsell advertisements'),(39,274877906944,'FULL_TRAINING','2009-02-18 20:36:07','2009-02-06 17:30:27',0,'Allows player access to tutorial/training features'),(40,549755813888,'PRODUCT01','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 1'),(41,1099511627776,'PRODUCT02','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 2'),(42,2199023255552,'PRODUCT03','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 3'),(43,4398046511104,'PRODUCT04','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 4'),(44,8796093022208,'PRODUCT05','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 5'),(45,17592186044416,'PRODUCT06','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 6'),(46,35184372088832,'PRODUCT07','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 7'),(47,70368744177664,'PRODUCT08','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 8'),(48,140737488355328,'PRODUCT09','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 9'),(49,281474976710656,'PRODUCT10','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 10'),(50,562949953421312,'PRODUCT11','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 11'),(51,1125899906842624,'PRODUCT12','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 12'),(52,2251799813685248,'PRODUCT13','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 13'),(53,4503599627370496,'PRODUCT14','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 14'),(54,9007199254740992,'PRODUCT15','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 15'),(55,18014398509481984,'PRODUCT16','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 16'),(56,36028797018963968,'PRODUCT17','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 17'),(57,72057594037927936,'PRODUCT18','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 18'),(58,144115188075855872,'PRODUCT19','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 19'),(59,288230376151711744,'PRODUCT20','2012-07-13 21:40:12','0000-00-00 00:00:00',0,'Product Permission 20'),(61,1152921504606846976,'IS_PRODUCER','2009-02-18 20:37:37','2005-07-15 11:13:50',0,'Allows game to register executable checksums used by this player'),(62,2305843009213693952,'DARK_GLASSES','2009-02-18 20:40:34','2005-07-15 11:13:50',0,'Allows IS_PLAYNET features without exposing account as staff (private accts)'),(63,4611686018427387904,'IS_PLAYNET','2009-02-18 20:41:27','2004-05-06 18:35:41',0,'Allows access to /crs channel and staff features');
/*!40000 ALTER TABLE `wwii_permissions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Current Database: `auth`
--

USE `auth`;

--
-- Final view structure for view `v_customer_active_groups`
--

/*!50001 DROP VIEW IF EXISTS `v_customer_active_groups`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8 */;
/*!50001 SET character_set_results     = utf8 */;
/*!50001 SET collation_connection      = utf8_general_ci */;
/*!50001 CREATE ALGORITHM=TEMPTABLE */
/*!50013 DEFINER=`root`@`localhost` SQL SECURITY INVOKER */
/*!50001 VIEW `v_customer_active_groups` AS select `ac`.`customerID` AS `customerid`,`a`.`arenaID` AS `arenaid`,`cg`.`group_oid` AS `group_oid` from ((((`auth_customer` `ac` join `auth_subscribed` `s` on((`ac`.`customerID` = `s`.`customerID`))) join `auth_subarenaassoc` `a` on((`s`.`subscriptionID` = `a`.`subscriptionID`))) join `auth_subgroupassoc` `g` on((`s`.`subscriptionID` = `g`.`subscriptionID`))) join `chat_groups` `cg` on((`g`.`groupID` = `cg`.`group_oid`))) where ((`s`.`status` = 'active') and (`cg`.`enabled` <> 'N')) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;

--
-- Final view structure for view `v_player_active_groups`
--

/*!50001 DROP VIEW IF EXISTS `v_player_active_groups`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8 */;
/*!50001 SET character_set_results     = utf8 */;
/*!50001 SET collation_connection      = utf8_general_ci */;
/*!50001 CREATE ALGORITHM=TEMPTABLE */
/*!50013 DEFINER=`root`@`localhost` SQL SECURITY INVOKER */
/*!50001 VIEW `v_player_active_groups` AS select `pl`.`playerid` AS `playerid`,`a`.`arenaID` AS `arenaid`,`cg`.`group_oid` AS `group_oid` from ((((`wwiionline`.`wwii_player` `pl` join `auth`.`auth_subscribed` `s` on((`pl`.`customerid` = `s`.`customerID`))) join `auth`.`auth_subarenaassoc` `a` on((`s`.`subscriptionID` = `a`.`subscriptionID`))) join `auth`.`auth_subgroupassoc` `g` on((`s`.`subscriptionID` = `g`.`subscriptionID`))) join `auth`.`chat_groups` `cg` on((`g`.`groupID` = `cg`.`group_oid`))) where ((`s`.`status` = 'active') and (`cg`.`enabled` <> 'N')) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2017-07-08 20:16:34
