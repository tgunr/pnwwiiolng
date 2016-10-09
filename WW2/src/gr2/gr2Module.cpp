#include <syscfg.h> // Path is project specific.

#include <map>

#include "include/vars.h"
#include "include/systypes.h"
#include "gr2/gr2Module.h"
#include "gr2/gr2GameObject.h"
#include "gr2/gr2Data.h"

// available modules
#include "mech/MechRocketPack.h"
#include "mech/MechWeapon.h"
#include "mech/MechModule.h"
#include "mech/MechDriveTrain.h"
#include "mech/MechGyros.h"
#include "mech/MechTurretMotor.h"

using GR2_MODULE_CREATOR = gr2GameObject* (*)(gr2ModuleSpec*);
using CreatorMap = std::map<std::string, GR2_MODULE_CREATOR>;

// local variables

static CreatorMap _creators;

// public functions

void gr2ModuleInit()
{
	if(!_creators.size())
	{
		_creators["MechModule"]			= &MechModule::Create;
		_creators["MechWeapon"]			= &MechWeapon::Create;
		_creators["MechRocketPack"]		= &MechRocketPack::Create;
		_creators["MechDriveTrain"]		= &MechDriveTrain::Create;
		_creators["MechGyros"]			= &MechGyros::Create;
		_creators["MechTurretMotor"]	= &MechTurretMotor::Create;
	}
}

// gr2Module class methods

gr2GameObject* gr2Module::Create(gr2ModuleSpec* spec)
{
	if(spec)
	{
		GR2_MODULE_CREATOR creator = _creators[spec->klass];
		if(creator) return creator(spec);
	}

	return NULL;
}
