#include <syscfg.h> // Path is project specific.
#include "mech/MechDriveTrain.h"
#include "mech/MechVehicle.h"

MechDriveTrain::MechDriveTrain(gr2ModuleSpec* spec) : MechModule(spec)
{
}

MechDriveTrain::~MechDriveTrain()
{
}

void MechDriveTrain::HandleDamage(gr2DamageComponent* component)
{
	MechModule::HandleDamage(component);

	MechVehicle* mech = GetParentVehicle();

	if(mech->IsFirstPerson())
		mech->SetMaxSpeed(CalculateEfficiency(mech->GetMaxSpeed(true)));
}

gr2GameObject* MechDriveTrain::Create(gr2ModuleSpec* spec)
{
	return new MechDriveTrain(spec);
}