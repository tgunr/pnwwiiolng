#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "js/jsMech.h"
#include "js/jsVehicle.h"
#include "mech/MechVehicle.h"
#include "js/jsDamageModel.h"

/************************************************************************************************
	jsMech
*************************************************************************************************/

JSClass jsMech::jsmechclass = JS_CLASS_MACRO("mech");

/************************************************************************************************
	jsMech
*************************************************************************************************/

jsMech::jsMech(MechVehicle* m) : jsGR2Object(&jsmechclass, m), mech(m), currentTarget(NULL)
{
	JS_CONSTRUCTOR_LOCK;
	InitJSProps();
};

void jsMech::InitJSProps(int index)
{
	// our properties
	char* propNames[] =
	{
		"target", "damagemodel", "vehicle", "callsign",
		"type", "description"
	};

	// loop over and add our properties to the JS object
	JSPropertySpec spec;

	for(int i = 0; i < jsMech::TOTAL_MECH_PROPS; i++)
	{
		spec.name		= propNames[i];
		spec.tinyid		= i+index;
		spec.flags		= JSPROP_PERMANENT|JSPROP_READONLY;
		spec.setter		= NULL;
		spec.getter		= NULL;

		// add it
		AddGR2Property(spec);
	}

	// let the base class set up properties
	jsGR2Object::InitJSProps(jsMech::TOTAL_MECH_PROPS+index);
}

jsMech::~jsMech()
{
}

JSBool jsMech::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case TARGET_PROP:
		{
			const MechVehicle::MechTarget& current = mech->GetMechTarget();

			if(current.vehicle)
			{
				if(currentTarget != current.vehicle)
				{
					currentTarget = current.vehicle;
					target = new jsMech(dynamic_cast<MechVehicle*>(current.vehicle));
					*val = target->jsVal();
				}
			}
			else
			{
				currentTarget = NULL;
				*val = JSVAL_NULL;
			}
		}
		break;
		case DAMAGEMODEL_PROP:
		{
			if(!damageModel.valid())
			{
				gr2DamageModel* dm = mech->GetDamageModel();

				if(dm && dm->GetComponents().size())
					damageModel = new jsDamageModel(dm);
			}

			*val = (damageModel.valid()) ? damageModel->jsVal(): JSVAL_NULL;
		}
		break;
		case VEHICLE_PROP:
		{
			if(!vehicle.valid())
				vehicle = new jsVehtype(mech->GetGameVehicle()->VehicleType);

			*val = (vehicle.valid()) ? vehicle->jsVal(): JSVAL_NULL;
		}
		break;
		case CALLSIGN_PROP:
		{
			*val = phui::js::ConstString((const char*)mech->GetGameVehicle()->Handle);
		}
		break;
		case TYPE_PROP:
		{
			*val = phui::js::ConstString((const char*)mech->GetGameVehicle()->VehicleType->Id);
		}
		break;
		case DESCRIPTION_PROP:
		{
			*val = phui::js::ConstString((const char*)mech->GetGameVehicle()->VehicleType->Description);
		}
		break;
		default:
			return jsGR2Object::GetProperty(ctx, propid-jsMech::TOTAL_MECH_PROPS, val);
		break;
	}

	return JS_TRUE;
}
