#include <syscfg.h> // Path is project specific.
// Created: Mon May 21 18:57:17 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "wwii/wwiiPack.h"
#include "strt/strtTypes.h"
#include "strt/strtUpdaterPack.h"

SINT32 strtPackEnemySpotted(PackMode mode, UCHAR* data, SINT32 offset, STRAT_ENEMY_REPORT* const report, SINT32 size)
{
	offset = wwiiPackFullObjID(mode,data,offset,&(report->reportingFacility),0);
	offset = wwiiPackWeaponSerial(mode,data,offset,&(report->enemyReported),0);
	offset = teulPackUINT32(mode,data,offset,&(report->distance),0);
	offset = teulPackUINT32(mode,data,offset,&(report->heading),0);
	offset = teulPackUINT32(mode,data,offset,&(report->altitude),0);
	offset = teulPackUINT32(mode,data,offset,&(report->speed),0);

	return offset;
}
