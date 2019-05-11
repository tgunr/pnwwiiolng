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
toe.vehicles v
WHERE
v.id = 'degrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'degrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'deairfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'deparfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'deairint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'deparint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'deseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'deseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'deseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'deseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();



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
toe.vehicles v
WHERE
v.id = 'frgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'frairfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frparfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frairint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frparint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'frseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'frseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();



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
toe.vehicles v
WHERE
v.id = 'ukgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'ukairfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukparfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukairint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukparint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'ukseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'ukseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();



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
toe.vehicles v
WHERE
v.id = 'usgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*3
	,cycle0 = v.cycle0*3
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usgrdint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*4
	,cycle0 = v.cycle0*4
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'usairfor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usairint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();


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
toe.vehicles v
WHERE
v.id = 'usseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usseafor'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity
	,cycle0 = v.cycle0
	,modified = UTC_TIMESTAMP();

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
toe.vehicles v
WHERE
v.id = 'usseaint'
ON DUPLICATE KEY UPDATE
	capacity = v.capacity*2
	,cycle0 = v.cycle0*2
	,modified = UTC_TIMESTAMP();