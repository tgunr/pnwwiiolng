-- -----------------------------------------------------------------------
/*
Copyright Cornered Rat Software 2018

File name: insert_toe_GarrisonVehicles.sql
Affects table(s): toe.vehicles
Purpose: Insert vehicle entries for the different Garrison headers

Changelog:
Name			JIRA Task #			Date		Description
Chaoswzkd		2098				05/26/2018	Initial Version

*/
-- -----------------------------------------------------------------------

-- Germany -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.vehicles
SELECT
'degrdg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg1for') = 0
AND toe.vehicles.id = 'degrdfor';

INSERT
INTO
toe.vehicles
SELECT
'degrdg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg2for') = 0
AND toe.vehicles.id = 'degrdfor';

INSERT
INTO
toe.vehicles
SELECT
'degrdg3for'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg3for') = 0
AND toe.vehicles.id = 'degrdfor';

INSERT
INTO
toe.vehicles
SELECT
'degrdg4for'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg4for') = 0
AND toe.vehicles.id = 'degrdfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'degrdg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg1int') = 0
AND toe.vehicles.id = 'degrdint';

INSERT
INTO
toe.vehicles
SELECT
'degrdg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg2int') = 0
AND toe.vehicles.id = 'degrdint';

INSERT
INTO
toe.vehicles
SELECT
'degrdg3int'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg3int') = 0
AND toe.vehicles.id = 'degrdint';

INSERT
INTO
toe.vehicles
SELECT
'degrdg4int'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg4int') = 0
AND toe.vehicles.id = 'degrdint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'degrdg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg1sp') = 0
AND toe.vehicles.id = 'degrdint';

INSERT
INTO
toe.vehicles
SELECT
'degrdg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg2sp') = 0
AND toe.vehicles.id = 'degrdint';

INSERT
INTO
toe.vehicles
SELECT
'degrdg3sp'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg3sp') = 0
AND toe.vehicles.id = 'degrdint';

INSERT
INTO
toe.vehicles
SELECT
'degrdg4sp'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'degrdg4sp') = 0
AND toe.vehicles.id = 'degrdint';



-- Air ----------------

INSERT
INTO
toe.vehicles
SELECT
'deairg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'deairg1for') = 0
AND toe.vehicles.id = 'deairfor';

INSERT
INTO
toe.vehicles
SELECT
'deparg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'deparg1for') = 0
AND toe.vehicles.id = 'deparfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'deairg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'deairg1int') = 0
AND toe.vehicles.id = 'deairint';

INSERT
INTO
toe.vehicles
SELECT
'deparg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'deparg1int') = 0
AND toe.vehicles.id = 'deparint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'deairg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'deairg1sp') = 0
AND toe.vehicles.id = 'deairspec';

INSERT
INTO
toe.vehicles
SELECT
'deparg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'deparg1sp') = 0
AND toe.vehicles.id = 'deparspec';



-- Navy ----------------

INSERT
INTO
toe.vehicles
SELECT
'denavg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'denavg1for') = 0
AND toe.vehicles.id = 'deseafor';

INSERT
INTO
toe.vehicles
SELECT
'denavg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'denavg2for') = 0
AND toe.vehicles.id = 'deseafor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'denavg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'denavg1int') = 0
AND toe.vehicles.id = 'deseaint';

INSERT
INTO
toe.vehicles
SELECT
'denavg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'denavg2int') = 0
AND toe.vehicles.id = 'deseaint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'denavg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'denavg1sp') = 0
AND toe.vehicles.id = 'deseaint';

INSERT
INTO
toe.vehicles
SELECT
'denavg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'denavg2sp') = 0
AND toe.vehicles.id = 'deseaint';





-- France -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.vehicles
SELECT
'frgrdg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg1for') = 0
AND toe.vehicles.id = 'frgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg2for') = 0
AND toe.vehicles.id = 'frgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg3for'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg3for') = 0
AND toe.vehicles.id = 'frgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg4for'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg4for') = 0
AND toe.vehicles.id = 'frgrdfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'frgrdg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg1int') = 0
AND toe.vehicles.id = 'frgrdint';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg2int') = 0
AND toe.vehicles.id = 'frgrdint';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg3int'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg3int') = 0
AND toe.vehicles.id = 'frgrdint';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg4int'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg4int') = 0
AND toe.vehicles.id = 'frgrdint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'frgrdg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg1sp') = 0
AND toe.vehicles.id = 'frgrdint';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg2sp') = 0
AND toe.vehicles.id = 'frgrdint';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg3sp'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg3sp') = 0
AND toe.vehicles.id = 'frgrdint';

INSERT
INTO
toe.vehicles
SELECT
'frgrdg4sp'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frgrdg4sp') = 0
AND toe.vehicles.id = 'frgrdint';



-- Air ----------------

INSERT
INTO
toe.vehicles
SELECT
'frairg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frairg1for') = 0
AND toe.vehicles.id = 'frairfor';

INSERT
INTO
toe.vehicles
SELECT
'frparg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frparg1for') = 0
AND toe.vehicles.id = 'frparfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'frairg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frairg1int') = 0
AND toe.vehicles.id = 'frairint';

INSERT
INTO
toe.vehicles
SELECT
'frparg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frparg1int') = 0
AND toe.vehicles.id = 'frparint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'frairg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frairg1sp') = 0
AND toe.vehicles.id = 'frairspec';

INSERT
INTO
toe.vehicles
SELECT
'frparg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frparg1sp') = 0
AND toe.vehicles.id = 'frparaspec';



-- Navy ----------------

INSERT
INTO
toe.vehicles
SELECT
'frnavg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frnavg1for') = 0
AND toe.vehicles.id = 'frseafor';

INSERT
INTO
toe.vehicles
SELECT
'frnavg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frnavg2for') = 0
AND toe.vehicles.id = 'frseafor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'frnavg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frnavg1int') = 0
AND toe.vehicles.id = 'frseaint';

INSERT
INTO
toe.vehicles
SELECT
'frnavg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frnavg2int') = 0
AND toe.vehicles.id = 'frseaint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'frnavg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frnavg1sp') = 0
AND toe.vehicles.id = 'frseaint';

INSERT
INTO
toe.vehicles
SELECT
'frnavg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'frnavg2sp') = 0
AND toe.vehicles.id = 'frseaint';




-- United Kingdom -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.vehicles
SELECT
'ukgrdg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg1for') = 0
AND toe.vehicles.id = 'ukgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg2for') = 0
AND toe.vehicles.id = 'ukgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg3for'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg3for') = 0
AND toe.vehicles.id = 'ukgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg4for'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg4for') = 0
AND toe.vehicles.id = 'ukgrdfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg1int') = 0
AND toe.vehicles.id = 'ukgrdint';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg2int') = 0
AND toe.vehicles.id = 'ukgrdint';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg3int'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg3int') = 0
AND toe.vehicles.id = 'ukgrdint';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg4int'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg4int') = 0
AND toe.vehicles.id = 'ukgrdint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg1sp') = 0
AND toe.vehicles.id = 'ukgrdint';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg2sp') = 0
AND toe.vehicles.id = 'ukgrdint';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg3sp'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg3sp') = 0
AND toe.vehicles.id = 'ukgrdint';

INSERT
INTO
toe.vehicles
SELECT
'ukgrdg4sp'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukgrdg4sp') = 0
AND toe.vehicles.id = 'ukgrdint';



-- Air ----------------

INSERT
INTO
toe.vehicles
SELECT
'ukairg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukairg1for') = 0
AND toe.vehicles.id = 'ukairfor';

INSERT
INTO
toe.vehicles
SELECT
'ukparg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukparg1for') = 0
AND toe.vehicles.id = 'ukparfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'ukairg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukairg1int') = 0
AND toe.vehicles.id = 'ukairint';

INSERT
INTO
toe.vehicles
SELECT
'ukparg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukparg1int') = 0
AND toe.vehicles.id = 'ukparint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'ukairg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukairg1sp') = 0
AND toe.vehicles.id = 'ukairspec';

INSERT
INTO
toe.vehicles
SELECT
'ukparg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'ukparg1sp') = 0
AND toe.vehicles.id = 'ukparspec';



-- Navy ----------------

INSERT
INTO
toe.vehicles
SELECT
'uknavg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'uknavg1for') = 0
AND toe.vehicles.id = 'ukseafor';

INSERT
INTO
toe.vehicles
SELECT
'uknavg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'uknavg2for') = 0
AND toe.vehicles.id = 'ukseafor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'uknavg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'uknavg1int') = 0
AND toe.vehicles.id = 'ukseaint';

INSERT
INTO
toe.vehicles
SELECT
'uknavg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'uknavg2int') = 0
AND toe.vehicles.id = 'ukseaint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'uknavg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'uknavg1sp') = 0
AND toe.vehicles.id = 'ukseaint';

INSERT
INTO
toe.vehicles
SELECT
'uknavg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'uknavg2sp') = 0
AND toe.vehicles.id = 'ukseaint';




-- United States -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.vehicles
SELECT
'usgrdg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg1for') = 0
AND toe.vehicles.id = 'usgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg2for') = 0
AND toe.vehicles.id = 'usgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg3for'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg3for') = 0
AND toe.vehicles.id = 'usgrdfor';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg4for'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg4for') = 0
AND toe.vehicles.id = 'usgrdfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'usgrdg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg1int') = 0
AND toe.vehicles.id = 'usgrdint';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg2int') = 0
AND toe.vehicles.id = 'usgrdint';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg3int'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg3int') = 0
AND toe.vehicles.id = 'usgrdint';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg4int'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg4int') = 0
AND toe.vehicles.id = 'usgrdint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'usgrdg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg1sp') = 0
AND toe.vehicles.id = 'usgrdint';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg2sp') = 0
AND toe.vehicles.id = 'usgrdint';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg3sp'
,vehtype_oid
,capacity*3
,cycle0*3
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg3sp') = 0
AND toe.vehicles.id = 'usgrdint';

INSERT
INTO
toe.vehicles
SELECT
'usgrdg4sp'
,vehtype_oid
,capacity*4
,cycle0*4
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usgrdg4sp') = 0
AND toe.vehicles.id = 'usgrdint';



-- Air ----------------

INSERT
INTO
toe.vehicles
SELECT
'usairg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usairg1for') = 0
AND toe.vehicles.id = 'usairfor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'usairg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usairg1int') = 0
AND toe.vehicles.id = 'usairint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'usairg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usairg1sp') = 0
AND toe.vehicles.id = 'usairspec';



-- Navy ----------------

INSERT
INTO
toe.vehicles
SELECT
'usnavg1for'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usnavg1for') = 0
AND toe.vehicles.id = 'usseafor';

INSERT
INTO
toe.vehicles
SELECT
'usnavg2for'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usnavg2for') = 0
AND toe.vehicles.id = 'usseafor';

-- Intermission Headers --

INSERT
INTO
toe.vehicles
SELECT
'usnavg1int'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usnavg1int') = 0
AND toe.vehicles.id = 'usseaint';

INSERT
INTO
toe.vehicles
SELECT
'usnavg2int'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usnavg2int') = 0
AND toe.vehicles.id = 'usseaint';

-- Special Event Headers --

INSERT
INTO
toe.vehicles
SELECT
'usnavg1sp'
,vehtype_oid
,capacity
,cycle0
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usnavg1sp') = 0
AND toe.vehicles.id = 'usseaint';

INSERT
INTO
toe.vehicles
SELECT
'usnavg2sp'
,vehtype_oid
,capacity*2
,cycle0*2
,UTC_TIMESTAMP()
,resupply_time_m
FROM
toe.vehicles
WHERE
(SELECT COUNT(*) FROM toe.vehicles WHERE id = 'usnavg2sp') = 0
AND toe.vehicles.id = 'usseaint';