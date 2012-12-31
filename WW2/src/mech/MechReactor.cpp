#include <syscfg.h> // Path is project specific.
#include "mech/MechReactor.h"
#include "mech/MechVehicle.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2DamageModel.h"
#include "veh/vehmain.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathUnitConversion.h"

MechReactor::MechReactor(gr2ModuleSpec* spec) :
	MechModule(spec),
	_power(0),
	_maxPower(0)
{
}

MechReactor::~MechReactor()
{
}

void MechReactor::AddConfigParam(std::string name, std::string value)
{
	// 1st crack goes to MechModule
	MechModule::AddConfigParam(name, value);

	// now lets see if we're interested
	if(!stricmp(name.c_str(), "power"))
		_power = atoi(value.c_str());
	else if(!stricmp(name.c_str(), "maxpower"))
		_maxPower = atoi(value.c_str());
}

gr2GameObject* MechReactor::Create(gr2ModuleSpec* spec)
{
	return new MechReactor(spec);
}


