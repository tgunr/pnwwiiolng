#include <syscfg.h> // Path is project specific.
#include "mech/MechTurretMotor.h"
#include "mech/MechVehicle.h"

MechTurretMotor::MechTurretMotor(gr2ModuleSpec* spec) : MechModule(spec), _turretIndex(-1)
{
}

MechTurretMotor::~MechTurretMotor()
{
}

#define IS_PARAM(x) (!stricmp(name.c_str(), x))

void MechTurretMotor::AddConfigParam(const std::string& name, const std::string& value)
{
	// 1st crack goes to MechModule
	MechModule::AddConfigParam(name, value);

	// now lets see if we're interested
	if(IS_PARAM("turret")) _turretIndex = atoi(value.c_str());
}

void MechTurretMotor::HandleDamage(gr2DamageComponent* component)
{
	MechModule::HandleDamage(component);

	MechVehicle* mech = GetParentVehicle();

	if(mech->IsFirstPerson())
	{
		gr2Vehicle::gr2Turret* turret = mech->GetTurret(_turretIndex);
		if(turret) turret->efficiency = _efficiency;
	}
}

gr2GameObject* MechTurretMotor::Create(gr2ModuleSpec* spec)
{
	return new MechTurretMotor(spec);
}