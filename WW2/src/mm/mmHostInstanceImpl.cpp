#include <syscfg.h>
// Created: Fri Jun 24 12:12:02 CDT 2011
//
// Implements the MMHost specific instancing overrides.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmHostInstanceImpl.h"
#include "miss/mission.h"					// For missDeleteInstanceMissions

namespace Instance
{

void
MMHostInstanceImpl::PostConstructionHandler()
{
	///TODO:INST: Create missions?
}

void
MMHostInstanceImpl::PreDestructionHandler()
{
	// Don't try to do anything if we haven't been initialized.
	if ( inst().descriptor().id() == 0 )
		return;

	dbugLog(_FLD, "in:%u.%u.%" PR64BitUT ".%" PR64BitUT " host instance destructor"
				, inst().descriptor().category(), inst().descriptor().type(), inst().descriptor().handle(), inst().descriptor().id());

	// Tell the mission system to delete all missions associated with
	// this instance.
	missDeleteInstanceMissions(inst().descriptor().id());
}

} // namespace Instance

