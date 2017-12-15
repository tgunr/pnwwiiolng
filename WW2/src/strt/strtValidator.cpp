#include <syscfg.h> // Path is project specific.
// Created: Fri Mar 30 16:46:40 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall
// Supporting Programmer:  Marty "Vandal" Poulin

#include "strt/strtData.h"
#include "strt/strtDataErrors.h"
#include "strt/strtIO.h"
#include "wwii/wwiiHCUnits.h"

void HCUnit::setHomeCPID(const FULL_OBJ_ID id)
{
    m_homeCPID = id;
}

int
main(int argc, char** argv)
{
	Teul::SetAppName(argv[0]);
	Teul::ConfigVar::InitializeConfigVars();

	int level = 1;

	if (argc > 1)
		level = atol(argv[1]);

	switch(level)
	{
	case 0:
		dbugInit(LogMode::TO_FILE, "./FatalErrors");
		break;
	case 1:
		dbugInit(LogMode::TO_FILE, "./Warnings");
		break;
	default:
		dbugInit(LogMode::TO_FILE, "./validator");
		break;
	}

	dbugSetLogLevel(Debug::ERR_DBUG | LOG_ALL_TO_STDERR);

	auto cpList = strtImportStratData();
	if ( cpList == nullptr )
	{
		strtReportDataErrors(0);
		dbugLog(_FLA,"[X][X][X] Strat System would not init [X][X][X]");
		exit(1);
	}

	if ( strtVerifyStratSystem(cpList, level) == false )
	{
		dbugLog(_FLA, "[X][X][X] Strat System is not valid [X][X][X]");
		exit(1);
	}

	dbugLog(_FLI, "./ ./ ./ Strat System verified \\. \\. \\.");

	//Setup all the spawning stuff
	//strtInitSpawn();

	dbugClose();
}
