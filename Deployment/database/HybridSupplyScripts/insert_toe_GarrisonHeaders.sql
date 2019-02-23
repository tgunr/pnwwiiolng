-- -----------------------------------------------------------------------
/*
Copyright Cornered Rat Software 2018

File name: insert_strat_GarrisonHeaders.sql
Affects table(s): toe.headers
Purpose: Insert headers for the different Garrison units

Changelog:
Name			JIRA Task #			Date		Description
Chaoswzkd		2098				04/21/2018	Initial Version

*/
-- -----------------------------------------------------------------------

-- Germany -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.headers
SELECT
'degrdg1for'
,'German Ground Garrison Forces (1AB)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg2for'
,'German Ground Garrison Forces (2AB)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg2for') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg3for'
,'German Ground Garrison Forces (3AB)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg3for') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg4for'
,'German Ground Garrison Forces (4AB)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg4for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'degrdg1int'
,'German Ground Garrison Forces (1AB)(int)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg2int'
,'German Ground Garrison Forces (2AB)(int)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg2int') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg3int'
,'German Ground Garrison Forces (3AB)(int)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg3int') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg4int'
,'German Ground Garrison Forces (4AB)(int)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg4int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'degrdg1sp'
,'German Ground Garrison Forces (1AB)(sp)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg2sp'
,'German Ground Garrison Forces (2AB)(sp)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg2sp') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg3sp'
,'German Ground Garrison Forces (3AB)(sp)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg3sp') = 0;

INSERT
INTO
toe.headers
SELECT
'degrdg4sp'
,'German Ground Garrison Forces (4AB)(sp)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'degrdg4sp') = 0;


-- Air ----------------
INSERT
INTO
toe.headers
SELECT
'deairg1for'
,'German Air Garrison Forces'
,4
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'deairg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'deparg1for'
,'German Airborne Garrison Forces'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'deparg1for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'deairg1int'
,'German Air Garrison Forces(int)'
,4
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'deairg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'deparg1int'
,'German Airborne Garrison Forces(int)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'deparg1int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'deairg1sp'
,'German Air Garrison Forces(sp)'
,4
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'deairg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'deparg1sp'
,'German Airborne Garrison Forces(sp)'
,4
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'deparg1sp') = 0;


-- Navy ----------------
INSERT
INTO
toe.headers
SELECT
'denavg1for'
,'German Naval Garrison Forces (1D)'
,4
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'denavg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'denavg2for'
,'German Naval Garrison Forces (2D)'
,4
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'denavg2for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'denavg1int'
,'German Naval Garrison Forces (1D)(int)'
,4
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'denavg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'denavg2int'
,'German Naval Garrison Forces (2D)(int)'
,4
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'denavg2int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'denavg1sp'
,'German Naval Garrison Forces (1D)(sp)'
,4
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'denavg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'denavg2sp'
,'German Naval Garrison Forces (2D)(sp)'
,4
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'denavg2sp') = 0;



-- France -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.headers
SELECT
'frgrdg1for'
,'French Ground Garrison Forces (1AB)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg2for'
,'French Ground Garrison Forces (2AB)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg2for') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg3for'
,'French Ground Garrison Forces (3AB)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg3for') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg4for'
,'French Ground Garrison Forces (4AB)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg4for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'frgrdg1int'
,'French Ground Garrison Forces (1AB)(int)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg2int'
,'French Ground Garrison Forces (2AB)(int)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg2int') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg3int'
,'French Ground Garrison Forces (3AB)(int)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg3int') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg4int'
,'French Ground Garrison Forces (4AB)(int)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg4int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'frgrdg1sp'
,'French Ground Garrison Forces (1AB)(sp)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg2sp'
,'French Ground Garrison Forces (2AB)(sp)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg2sp') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg3sp'
,'French Ground Garrison Forces (3AB)(sp)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg3sp') = 0;

INSERT
INTO
toe.headers
SELECT
'frgrdg4sp'
,'French Ground Garrison Forces (4AB)(sp)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frgrdg4sp') = 0;


-- Air ----------------
INSERT
INTO
toe.headers
SELECT
'frairg1for'
,'French Air Garrison Forces'
,3
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frairg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'frparg1for'
,'French Airborne Garrison Forces'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frparg1for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'frairg1int'
,'French Air Garrison Forces(int)'
,3
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frairg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'frparg1int'
,'French Airborne Garrison Forces(int)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frparg1int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'frairg1sp'
,'French Air Garrison Forces(sp)'
,3
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frairg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'frparg1sp'
,'French Airborne Garrison Forces(sp)'
,3
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frparg1sp') = 0;


-- Navy ----------------
INSERT
INTO
toe.headers
SELECT
'frnavg1for'
,'French Naval Garrison Forces (1D)'
,3
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frnavg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'frnavg2for'
,'French Naval Garrison Forces (2D)'
,3
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frnavg2for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'frnavg1int'
,'French Naval Garrison Forces (1D)(int)'
,3
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frnavg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'frnavg2int'
,'French Naval Garrison Forces (2D)(int)'
,3
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frnavg2int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'frnavg1sp'
,'French Naval Garrison Forces (1D)(sp)'
,3
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frnavg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'frnavg2sp'
,'French Naval Garrison Forces (2D)(sp)'
,3
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'frnavg2sp') = 0;





-- United Kingdom -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.headers
SELECT
'ukgrdg1for'
,'UK Ground Garrison Forces (1AB)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg2for'
,'UK Ground Garrison Forces (2AB)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg2for') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg3for'
,'UK Ground Garrison Forces (3AB)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg3for') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg4for'
,'UK Ground Garrison Forces (4AB)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg4for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'ukgrdg1int'
,'UK Ground Garrison Forces (1AB)(int)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg2int'
,'UK Ground Garrison Forces (2AB)(int)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg2int') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg3int'
,'UK Ground Garrison Forces (3AB)(int)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg3int') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg4int'
,'UK Ground Garrison Forces (4AB)(int)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg4int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'ukgrdg1sp'
,'UK Ground Garrison Forces (1AB)(sp)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg2sp'
,'UK Ground Garrison Forces (2AB)(sp)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg2sp') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg3sp'
,'UK Ground Garrison Forces (3AB)(sp)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg3sp') = 0;

INSERT
INTO
toe.headers
SELECT
'ukgrdg4sp'
,'UK Ground Garrison Forces (4AB)(sp)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukgrdg4sp') = 0;


-- Air ----------------
INSERT
INTO
toe.headers
SELECT
'ukairg1for'
,'UK Air Garrison Forces'
,1
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukairg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'ukparg1for'
,'UK Airborne Garrison Forces'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukparg1for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'ukairg1int'
,'UK Air Garrison Forces(int)'
,1
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukairg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'ukparg1int'
,'UK Airborne Garrison Forces(int)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukparg1int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'ukairg1sp'
,'UK Air Garrison Forces(sp)'
,1
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukairg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'ukparg1sp'
,'UK Airborne Garrison Forces(sp)'
,1
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'ukparg1sp') = 0;


-- Navy ----------------
INSERT
INTO
toe.headers
SELECT
'uknavg1for'
,'UK Naval Garrison Forces (1D)'
,1
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'uknavg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'uknavg2for'
,'UK Naval Garrison Forces (2D)'
,1
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'uknavg2for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'uknavg1int'
,'UK Naval Garrison Forces (1D)(int)'
,1
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'uknavg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'uknavg2int'
,'UK Naval Garrison Forces (2D)(int)'
,1
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'uknavg2int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'uknavg1sp'
,'UK Naval Garrison Forces (1D)(sp)'
,1
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'uknavg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'uknavg2sp'
,'UK Naval Garrison Forces (2D)(sp)'
,1
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'uknavg2sp') = 0;




-- United States -------------------------------------------------

-- Ground ----------------
INSERT
INTO
toe.headers
SELECT
'usgrdg1for'
,'US Ground Garrison Forces (1AB)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg2for'
,'US Ground Garrison Forces (2AB)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg2for') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg3for'
,'US Ground Garrison Forces (3AB)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg3for') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg4for'
,'US Ground Garrison Forces (4AB)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg4for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'usgrdg1int'
,'US Ground Garrison Forces (1AB)(int)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg2int'
,'US Ground Garrison Forces (2AB)(int)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg2int') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg3int'
,'US Ground Garrison Forces (3AB)(int)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg3int') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg4int'
,'US Ground Garrison Forces (4AB)(int)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg4int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'usgrdg1sp'
,'US Ground Garrison Forces (1AB)(sp)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg2sp'
,'US Ground Garrison Forces (2AB)(sp)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg2sp') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg3sp'
,'US Ground Garrison Forces (3AB)(sp)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg3sp') = 0;

INSERT
INTO
toe.headers
SELECT
'usgrdg4sp'
,'US Ground Garrison Forces (4AB)(sp)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,9
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usgrdg4sp') = 0;


-- Air ----------------
INSERT
INTO
toe.headers
SELECT
'usairg1for'
,'US Air Garrison Forces'
,2
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usairg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'usparg1for'
,'US Airborne Garrison Forces'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usparg1for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'usairg1int'
,'US Air Garrison Forces(int)'
,2
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usairg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'usparg1int'
,'US Airborne Garrison Forces(int)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usparg1nt') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'usairg1sp'
,'US Air Garrison Forces(sp)'
,2
,2
,UTC_TIMESTAMP()
,0
,0
,0
,2
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usairg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'usparg1sp'
,'US Airborne Garrison Forces(sp)'
,2
,1
,UTC_TIMESTAMP()
,0
,0
,0
,14
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usparg1sp') = 0;


-- Navy ----------------
INSERT
INTO
toe.headers
SELECT
'usnavg1for'
,'US Naval Garrison Forces (1D)'
,2
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usnavg1for') = 0;

INSERT
INTO
toe.headers
SELECT
'usnavg2for'
,'US Naval Garrison Forces (2D)'
,2
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usnavg2for') = 0;

-- Intermission Headers --

INSERT
INTO
toe.headers
SELECT
'usnavg1int'
,'US Naval Garrison Forces (1D)(int)'
,2
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usnavg1int') = 0;

INSERT
INTO
toe.headers
SELECT
'usnavg2int'
,'US Naval Garrison Forces (2D)(int)'
,2
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usnavg2int') = 0;

-- Special Event Headers --

INSERT
INTO
toe.headers
SELECT
'usnavg1sp'
,'US Naval Garrison Forces (1D)(sp)'
,2
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usnavg1sp') = 0;

INSERT
INTO
toe.headers
SELECT
'usnavg2sp'
,'US Naval Garrison Forces (2D)(sp)'
,2
,3
,UTC_TIMESTAMP()
,0
,0
,0
,13
WHERE(SELECT COUNT(*) FROM toe.headers WHERE id = 'usnavg2sp') = 0;

