-- -----------------------------------------------------------------------
/*
Copyright Cornered Rat Software 2018

File name: insert_toe_GarrisonRDP.sql
Affects table(s): toe.rdp
Purpose: Insert rdp entries for the different Garrison headers

Changelog:
Name			JIRA Task #			Date		Description
Chaoswzkd		2098				07/29/2018	Initial Version

*/
-- -----------------------------------------------------------------------

-- Germany -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.rdp
SELECT
'degrdg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg1for') = 0
AND toe.rdp.id = 'degrdfor';

INSERT
INTO
toe.rdp
SELECT
'degrdg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg2for') = 0
AND toe.rdp.id = 'degrdfor';

INSERT
INTO
toe.rdp
SELECT
'degrdg3for'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg3for') = 0
AND toe.rdp.id = 'degrdfor';

INSERT
INTO
toe.rdp
SELECT
'degrdg4for'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg4for') = 0
AND toe.rdp.id = 'degrdfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'degrdg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg1int') = 0
AND toe.rdp.id = 'degrdint';

INSERT
INTO
toe.rdp
SELECT
'degrdg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg2int') = 0
AND toe.rdp.id = 'degrdint';

INSERT
INTO
toe.rdp
SELECT
'degrdg3int'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg3int') = 0
AND toe.rdp.id = 'degrdint';

INSERT
INTO
toe.rdp
SELECT
'degrdg4int'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg4int') = 0
AND toe.rdp.id = 'degrdint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'degrdg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg1sp') = 0
AND toe.rdp.id = 'degrdint';

INSERT
INTO
toe.rdp
SELECT
'degrdg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg2sp') = 0
AND toe.rdp.id = 'degrdint';

INSERT
INTO
toe.rdp
SELECT
'degrdg3sp'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg3sp') = 0
AND toe.rdp.id = 'degrdint';

INSERT
INTO
toe.rdp
SELECT
'degrdg4sp'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'degrdg4sp') = 0
AND toe.rdp.id = 'degrdint';



-- Air ----------------

INSERT
INTO
toe.rdp
SELECT
'deairg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'deairg1for') = 0
AND toe.rdp.id = 'deairfor';

INSERT
INTO
toe.rdp
SELECT
'deparg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'deparg1for') = 0
AND toe.rdp.id = 'deparfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'deairg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'deairg1int') = 0
AND toe.rdp.id = 'deairint';

INSERT
INTO
toe.rdp
SELECT
'deparg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'deparg1int') = 0
AND toe.rdp.id = 'deparint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'deairg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'deairg1sp') = 0
AND toe.rdp.id = 'deairspec';

INSERT
INTO
toe.rdp
SELECT
'deparg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'deparg1sp') = 0
AND toe.rdp.id = 'deparspec';



-- Navy ----------------

INSERT
INTO
toe.rdp
SELECT
'denavg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'denavg1for') = 0
AND toe.rdp.id = 'deseafor';

INSERT
INTO
toe.rdp
SELECT
'denavg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'denavg2for') = 0
AND toe.rdp.id = 'deseafor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'denavg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'denavg1int') = 0
AND toe.rdp.id = 'deseaint';

INSERT
INTO
toe.rdp
SELECT
'denavg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'denavg2int') = 0
AND toe.rdp.id = 'deseaint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'denavg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'denavg1sp') = 0
AND toe.rdp.id = 'deseaint';

INSERT
INTO
toe.rdp
SELECT
'denavg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'denavg2sp') = 0
AND toe.rdp.id = 'deseaint';





-- France -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.rdp
SELECT
'frgrdg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg1for') = 0
AND toe.rdp.id = 'frgrdfor';

INSERT
INTO
toe.rdp
SELECT
'frgrdg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg2for') = 0
AND toe.rdp.id = 'frgrdfor';

INSERT
INTO
toe.rdp
SELECT
'frgrdg3for'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg3for') = 0
AND toe.rdp.id = 'frgrdfor';

INSERT
INTO
toe.rdp
SELECT
'frgrdg4for'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg4for') = 0
AND toe.rdp.id = 'frgrdfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'frgrdg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg1int') = 0
AND toe.rdp.id = 'frgrdint';

INSERT
INTO
toe.rdp
SELECT
'frgrdg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg2int') = 0
AND toe.rdp.id = 'frgrdint';

INSERT
INTO
toe.rdp
SELECT
'frgrdg3int'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg3int') = 0
AND toe.rdp.id = 'frgrdint';

INSERT
INTO
toe.rdp
SELECT
'frgrdg4int'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg4int') = 0
AND toe.rdp.id = 'frgrdint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'frgrdg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg1sp') = 0
AND toe.rdp.id = 'frgrdint';

INSERT
INTO
toe.rdp
SELECT
'frgrdg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg2sp') = 0
AND toe.rdp.id = 'frgrdint';

INSERT
INTO
toe.rdp
SELECT
'frgrdg3sp'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg3sp') = 0
AND toe.rdp.id = 'frgrdint';

INSERT
INTO
toe.rdp
SELECT
'frgrdg4sp'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frgrdg4sp') = 0
AND toe.rdp.id = 'frgrdint';



-- Air ----------------

INSERT
INTO
toe.rdp
SELECT
'frairg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frairg1for') = 0
AND toe.rdp.id = 'frairfor';

INSERT
INTO
toe.rdp
SELECT
'frparg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frparg1for') = 0
AND toe.rdp.id = 'frparfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'frairg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frairg1int') = 0
AND toe.rdp.id = 'frairint';

INSERT
INTO
toe.rdp
SELECT
'frparg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frparg1int') = 0
AND toe.rdp.id = 'frparint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'frairg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frairg1sp') = 0
AND toe.rdp.id = 'frairspec';

INSERT
INTO
toe.rdp
SELECT
'frparg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frparg1sp') = 0
AND toe.rdp.id = 'frparaspec';



-- Navy ----------------

INSERT
INTO
toe.rdp
SELECT
'frnavg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frnavg1for') = 0
AND toe.rdp.id = 'frseafor';

INSERT
INTO
toe.rdp
SELECT
'frnavg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frnavg2for') = 0
AND toe.rdp.id = 'frseafor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'frnavg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frnavg1int') = 0
AND toe.rdp.id = 'frseaint';

INSERT
INTO
toe.rdp
SELECT
'frnavg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frnavg2int') = 0
AND toe.rdp.id = 'frseaint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'frnavg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frnavg1sp') = 0
AND toe.rdp.id = 'frseaint';

INSERT
INTO
toe.rdp
SELECT
'frnavg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'frnavg2sp') = 0
AND toe.rdp.id = 'frseaint';




-- United Kingdom -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.rdp
SELECT
'ukgrdg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg1for') = 0
AND toe.rdp.id = 'ukgrdfor';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg2for') = 0
AND toe.rdp.id = 'ukgrdfor';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg3for'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg3for') = 0
AND toe.rdp.id = 'ukgrdfor';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg4for'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg4for') = 0
AND toe.rdp.id = 'ukgrdfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'ukgrdg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg1int') = 0
AND toe.rdp.id = 'ukgrdint';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg2int') = 0
AND toe.rdp.id = 'ukgrdint';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg3int'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg3int') = 0
AND toe.rdp.id = 'ukgrdint';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg4int'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg4int') = 0
AND toe.rdp.id = 'ukgrdint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'ukgrdg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg1sp') = 0
AND toe.rdp.id = 'ukgrdint';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg2sp') = 0
AND toe.rdp.id = 'ukgrdint';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg3sp'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg3sp') = 0
AND toe.rdp.id = 'ukgrdint';

INSERT
INTO
toe.rdp
SELECT
'ukgrdg4sp'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukgrdg4sp') = 0
AND toe.rdp.id = 'ukgrdint';



-- Air ----------------

INSERT
INTO
toe.rdp
SELECT
'ukairg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukairg1for') = 0
AND toe.rdp.id = 'ukairfor';

INSERT
INTO
toe.rdp
SELECT
'ukparg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukparg1for') = 0
AND toe.rdp.id = 'ukparfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'ukairg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukairg1int') = 0
AND toe.rdp.id = 'ukairint';

INSERT
INTO
toe.rdp
SELECT
'ukparg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukparg1int') = 0
AND toe.rdp.id = 'ukparint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'ukairg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukairg1sp') = 0
AND toe.rdp.id = 'ukairspec';

INSERT
INTO
toe.rdp
SELECT
'ukparg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'ukparg1sp') = 0
AND toe.rdp.id = 'ukparspec';



-- Navy ----------------

INSERT
INTO
toe.rdp
SELECT
'uknavg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'uknavg1for') = 0
AND toe.rdp.id = 'ukseafor';

INSERT
INTO
toe.rdp
SELECT
'uknavg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'uknavg2for') = 0
AND toe.rdp.id = 'ukseafor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'uknavg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'uknavg1int') = 0
AND toe.rdp.id = 'ukseaint';

INSERT
INTO
toe.rdp
SELECT
'uknavg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'uknavg2int') = 0
AND toe.rdp.id = 'ukseaint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'uknavg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'uknavg1sp') = 0
AND toe.rdp.id = 'ukseaint';

INSERT
INTO
toe.rdp
SELECT
'uknavg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'uknavg2sp') = 0
AND toe.rdp.id = 'ukseaint';




-- United States -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.rdp
SELECT
'usgrdg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg1for') = 0
AND toe.rdp.id = 'usgrdfor';

INSERT
INTO
toe.rdp
SELECT
'usgrdg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg2for') = 0
AND toe.rdp.id = 'usgrdfor';

INSERT
INTO
toe.rdp
SELECT
'usgrdg3for'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg3for') = 0
AND toe.rdp.id = 'usgrdfor';

INSERT
INTO
toe.rdp
SELECT
'usgrdg4for'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg4for') = 0
AND toe.rdp.id = 'usgrdfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'usgrdg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg1int') = 0
AND toe.rdp.id = 'usgrdint';

INSERT
INTO
toe.rdp
SELECT
'usgrdg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg2int') = 0
AND toe.rdp.id = 'usgrdint';

INSERT
INTO
toe.rdp
SELECT
'usgrdg3int'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg3int') = 0
AND toe.rdp.id = 'usgrdint';

INSERT
INTO
toe.rdp
SELECT
'usgrdg4int'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg4int') = 0
AND toe.rdp.id = 'usgrdint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'usgrdg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg1sp') = 0
AND toe.rdp.id = 'usgrdint';

INSERT
INTO
toe.rdp
SELECT
'usgrdg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg2sp') = 0
AND toe.rdp.id = 'usgrdint';

INSERT
INTO
toe.rdp
SELECT
'usgrdg3sp'
,cycle
,vehtype_oid
,capacity*3
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg3sp') = 0
AND toe.rdp.id = 'usgrdint';

INSERT
INTO
toe.rdp
SELECT
'usgrdg4sp'
,cycle
,vehtype_oid
,capacity*4
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usgrdg4sp') = 0
AND toe.rdp.id = 'usgrdint';



-- Air ----------------

INSERT
INTO
toe.rdp
SELECT
'usairg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usairg1for') = 0
AND toe.rdp.id = 'usairfor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'usairg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usairg1int') = 0
AND toe.rdp.id = 'usairint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'usairg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usairg1sp') = 0
AND toe.rdp.id = 'usairspec';



-- Navy ----------------

INSERT
INTO
toe.rdp
SELECT
'usnavg1for'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usnavg1for') = 0
AND toe.rdp.id = 'usseafor';

INSERT
INTO
toe.rdp
SELECT
'usnavg2for'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usnavg2for') = 0
AND toe.rdp.id = 'usseafor';

-- Intermission Headers --

INSERT
INTO
toe.rdp
SELECT
'usnavg1int'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usnavg1int') = 0
AND toe.rdp.id = 'usseaint';

INSERT
INTO
toe.rdp
SELECT
'usnavg2int'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usnavg2int') = 0
AND toe.rdp.id = 'usseaint';

-- Special Event Headers --

INSERT
INTO
toe.rdp
SELECT
'usnavg1sp'
,cycle
,vehtype_oid
,capacity
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usnavg1sp') = 0
AND toe.rdp.id = 'usseaint';

INSERT
INTO
toe.rdp
SELECT
'usnavg2sp'
,cycle
,vehtype_oid
,capacity*2
FROM
toe.rdp
WHERE
(SELECT COUNT(*) FROM toe.rdp WHERE id = 'usnavg2sp') = 0
AND toe.rdp.id = 'usseaint';