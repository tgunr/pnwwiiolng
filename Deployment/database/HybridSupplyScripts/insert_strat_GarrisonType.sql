-- -----------------------------------------------------------------------
/*
Copyright Cornered Rat Software 2018

File name: insert_strat_GarrisonType.sql
Affects table(s): wwiionline.strat_hc_structure
Purpose: Insert "Garrison" as a type of Flag Structure

Changelog:
Name			JIRA Task #			Date		Description
Chaoswzkd		2098				04/21/2018	Initial Version

*/
-- -----------------------------------------------------------------------

INSERT
INTO
wwiionline.strat_hc_structure
SELECT
169
,'Garrison'
,''
,'Gar'
,0
,UTC_TIMESTAMP()
,UTC_TIMESTAMP()
WHERE
(SELECT COUNT(*) FROM wwiionline.strat_hc_structure WHERE title = 'Garrison') = 0;

INSERT
INTO
wwiionline.strat_hc_hqs
SELECT
169
,0
,3
WHERE
(SELECT COUNT(*) FROM wwiionline.strat_hc_hqs WHERE structure_id = 169) = 0;