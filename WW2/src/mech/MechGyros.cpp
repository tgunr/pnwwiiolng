#include <syscfg.h> // Path is project specific.
#include "mech/MechGyros.h"
#include "mech/MechVehicle.h"

MechGyros::MechGyros(gr2ModuleSpec* spec) : MechModule(spec)
{
}

MechGyros::~MechGyros()
{
}

void MechGyros::HandleDamage(gr2DamageComponent* component)
{
	MechModule::HandleDamage(component);

	MechVehicle* mech = GetParentVehicle();

	if(mech->IsFirstPerson())
		mech->SetMaxTurn(CalculateEfficiency(mech->GetMaxTurn(true)));
}

gr2GameObject* MechGyros::Create(gr2ModuleSpec* spec)
{
	return new MechGyros(spec);
}