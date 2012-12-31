#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "include/vars.h"
#include "ml/mltypes.h"
#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "trn/trnmain.h"
#include "js/jsPlayer.h"
#include "js/jsPosition.h"
#include "js/jsClient.h"
#include "js/jsStratDB.h"
#include "client/GameClient.h"

#ifdef DOROTHY
#include "gr2/gr2Main.h"
#include "mech/MechVehicle.h"
#include "js/jsMech.h"
#endif

#if defined(USE_OSG)
#include "osgPlayer/playerMain.h"
#include "osgPlayer/Player.h"
#endif

/************************************************************************************************
	jsPlayer
*************************************************************************************************/

JSClass jsPlayer::jsclass = JS_CLASS_MACRO("player");
JSFunctionSpec jsPlayer::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsPlayer::jsproperties[] =
{
	{"position",			jsPlayer::POSITION_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"blood",				jsPlayer::BLOODLEVEL_PROP, 	JSPROP_PERMANENT | JSPROP_READONLY},
	{"atp",					jsPlayer::ATPLEVEL_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"primaryammocount",	jsPlayer::PRIMARYAMMOCOUNT_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondaryammocount",	jsPlayer::SECONDARYAMMOCOUNT_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"primaryclipcount",	jsPlayer::PRIMARYCLIPCOUNT_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondaryclipcount",	jsPlayer::SECONDARYCLIPCOUNT_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"primaryoversupply",	jsPlayer::PRIMARYOVERSUPPLY_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondaryoversupply",	jsPlayer::SECONDARYOVERSUPPLY_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"primaryammoname",		jsPlayer::PRIMARYAMMONAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondaryammoname",	jsPlayer::SECONDARYAMMONAME_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"state",				jsPlayer::STATE_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},
	{"walkrun",				jsPlayer::WALKRUN_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},
	{"vehicletype",			jsPlayer::VEHICLETYPE_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"nearestcp",			jsPlayer::NEAREST_CP_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"rtb",					jsPlayer::RTB_PROP,					JSPROP_PERMANENT | JSPROP_READONLY},
	{"client",				jsPlayer::CLIENT_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},
	{"rtbfac",				jsPlayer::RTBFAC_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},
	{"primaryclipsize",		jsPlayer::PRIMARYCLIPSIZE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondaryclipsize",	jsPlayer::SECONDARYCLIPSIZE_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"rgammo",				jsPlayer::RGAMMO_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},
	{"rgloaded",			jsPlayer::RGLOADED_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"isoversupplied",		jsPlayer::ISOVERSUPPLIED_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"primarycliptype",		jsPlayer::PRIMARYCLIPTYPE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondarycliptype",	jsPlayer::SECONDARYCLIPTYPE_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"primaryrange",		jsPlayer::PRIMARYRANGE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"secondaryrange",		jsPlayer::SECONDARYRANGE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"nofirezone",					jsPlayer::NOFIREZONE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"capturezone",			jsPlayer::CAPTUREZONE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"hulked",			jsPlayer::HULKED_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"deathcam",			jsPlayer::DEATHCAM_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"country",			jsPlayer::COUNTRY_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
#ifdef DOROTHY
	{"crosshairsx",		jsPlayer::CROSSHAIRSX_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"crosshairsy",		jsPlayer::CROSSHAIRSY_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"gr2vehicle",		jsPlayer::GR2VEHICLE_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
#endif
	{0}
};



/************************************************************************************************
	jsPlayer
*************************************************************************************************/

jsPlayer::jsPlayer() : phui::js::Object(&jsclass, jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;

	jsclient = new jsClient(PlayerClient());
//	DefineProperty("client", jsclient); don't know why this doesn't work
	jsposition = new jsPosition;
}

jsPlayer::~jsPlayer()
{
}

JSBool jsPlayer::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	vehVehicle* Vehicle = persGetPersonaVehiclePtr();

	switch(propid)
	{
		case CLIENT_PROP:
			*val = jsclient->jsVal();
		break;
		case POSITION_PROP:
		{
			FLOAT64VECTOR fvecPlayerPos;
		    FLOAT64VECTOR vecWorldOriginOffset;
			trnGetWorldOriginOffset(&vecWorldOriginOffset);
		    fvecPlayerPos.x = -playpos.x + vecWorldOriginOffset.x;
		    fvecPlayerPos.y = -playpos.y + vecWorldOriginOffset.y;
		    fvecPlayerPos.z = -playpos.z + vecWorldOriginOffset.z;

			jsposition->SetX((float)fvecPlayerPos.x);
			jsposition->SetY((float)fvecPlayerPos.y);

			*val = jsposition->jsVal();
		}
		break;
		case BLOODLEVEL_PROP :
		{
			float BrainOxy, ATPLevel, BloodPressure, BloodLevel;
			persGetHealthStats(&BrainOxy,&ATPLevel,&BloodPressure,&BloodLevel);
			JS_NewDoubleValue(_jsctx, BloodLevel, val);
		}
		break;
		case ATPLEVEL_PROP:
		{
			float BrainOxy, ATPLevel, BloodPressure, BloodLevel;
			persGetHealthStats(&BrainOxy,&ATPLevel,&BloodPressure,&BloodLevel);
			JS_NewDoubleValue(_jsctx, ATPLevel, val);
		}
		break;
		case PRIMARYAMMOCOUNT_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 bulletsInClip = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_CLIPAMMO, &bulletsInClip);
				JS_NewDoubleValue(_jsctx, bulletsInClip, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, VehicleData->CurrentPrimaryAmmoCount, val);
			}
		}
		break;
		case SECONDARYAMMOCOUNT_PROP:
		if(Vehicle && VehicleData)
		{
			if (VehicleData->GroupAmmoCount != -1)
				JS_NewDoubleValue(_jsctx, VehicleData->GroupAmmoCount, val);
			else
				JS_NewDoubleValue(_jsctx, VehicleData->CurrentSecondaryAmmoCount, val);
		}
		break;

		case PRIMARYOVERSUPPLY_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 overSupply = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_OVERSUPPLY, &overSupply);
				JS_NewDoubleValue(_jsctx, overSupply, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, 0, val);
			}
		}
		break;
		case SECONDARYOVERSUPPLY_PROP:
		if(Vehicle && VehicleData)
		{
			JS_NewDoubleValue(_jsctx, 0, val);
		}
		break;

		case PRIMARYCLIPCOUNT_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 clipCount = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_CLIPCOUNT, &clipCount);
				JS_NewDoubleValue(_jsctx, clipCount, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, VehicleData->PrimaryClipCount, val);
			}
		}
		break;

		case SECONDARYCLIPCOUNT_PROP:
			if(Vehicle && VehicleData)
				JS_NewDoubleValue(_jsctx, VehicleData->SecondaryClipCount, val);
		break;

		case PRIMARYCLIPSIZE_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 clipSize = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_CLIPSIZE, &clipSize);
				JS_NewDoubleValue(_jsctx, clipSize, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, -1, val);
			}
		}
		break;

		case SECONDARYCLIPSIZE_PROP:
			if(Vehicle && VehicleData)
				JS_NewDoubleValue(_jsctx, -1, val);
		break;

		case PRIMARYCLIPTYPE_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 clipType = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_CLIPTYPE, &clipType);
				JS_NewDoubleValue(_jsctx, clipType, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, VehicleData->PrimaryClipType, val);
			}
		}
		break;

		case SECONDARYCLIPTYPE_PROP:
			if(Vehicle && VehicleData)
				JS_NewDoubleValue(_jsctx, VehicleData->SecondaryClipType, val);
		break;

		case PRIMARYRANGE_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 range = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_RANGE, &range);
				JS_NewDoubleValue(_jsctx, range, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, -1, val);
			}
		}
		break;

		case SECONDARYRANGE_PROP:
			if(Vehicle && VehicleData)
				JS_NewDoubleValue(_jsctx, -1, val);
		break;

		case PRIMARYAMMONAME_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				char name[30+1];
				UINT32 weaponType = fpFPSGetHighlightedWeaponType();

				strncpy(name, fpFPSWeaponKeyStrings[weaponType], 30);

				switch(weaponType)
				{
					case LTMORTAR_UK:
					case LTMORTAR_FR:
					case LTMORTAR_DE:
					{
						// Covert access
						const UINT16 ammoType = Vehicle->Weapons[fpsTrooper.highlightedWeaponNumber][0].AmmoTypeInUse.Get();
						strcat(name, (ammoType > 0) ? "_smk": "_he");
					}
					break;
					case SATCHEL:
					case SATCHEL_AX:
						weapWeapon& weap = Vehicle->Weapons[fpsTrooper.highlightedWeaponNumber][0];
						weapAmmoType* at = weap.WeaponType->AmmoType[0];
						const char* ammoName = (const char*)at->Name;

						if(_stricmp(ammoName, "satchelgeneric") == 0 || _stricmp(ammoName, "germsatchel") == 0)
							strcat(name, "_heat");
						else // satchelhighexplosive
							strcat(name, "_he");

						/*
						if(Vehicle->VehicleType->WeaponSpec.wType == 9)
							strcat(name, "_heat");
						else
							strcat(name, "_he");
						*/
					break;
				}

				*val = phui::js::NewString(name);
			}
			else
			{
				*val = phui::js::NewString(VehicleData->PrimaryAmmoType);
			}
		}
		break;

		case SECONDARYAMMONAME_PROP:
			if(Vehicle && VehicleData)
				*val = phui::js::NewString(VehicleData->SecondaryAmmoType);
		break;

		case RGAMMO_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 rgAmmo = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_RGAMMO, &rgAmmo);
				JS_NewDoubleValue(_jsctx, rgAmmo, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, -1, val);
			}
		}
		break;
		case RGLOADED_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				SINT32 rgLoaded = -1;
				fpFPSGetHUDAmmoData(HUD_AMMO_DATA_RGLOADED, &rgLoaded);
				JS_NewDoubleValue(_jsctx, rgLoaded, val);
			}
			else
			{
				JS_NewDoubleValue(_jsctx, -1, val);
			}
		}
		break;

		case ISOVERSUPPLIED_PROP:
			*val = BOOLEAN_TO_JSVAL(persIsOverSupplied());
		break;

		case STATE_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				if(fpFPSIsDead())
					*val = phui::js::ConstString("dead");
				else if(Vehicle->RideVehicleHostIndex > 0 && Vehicle->RideSeatNum < 128)
					*val = phui::js::ConstString("riding");
				else if(fpFPSIsTrooperParachuteOpen())
					*val = phui::js::ConstString("parachuting");
				else if(fpFPSIsSwimming())
					*val = phui::js::ConstString("swimming");
				else if(fpFPSIsProne())
					*val = phui::js::ConstString("prone");
				else if(fpFPSIsCrouch())
					*val = phui::js::ConstString("crouch");
				else
					*val = phui::js::ConstString("stand");
			}else
			{
				UINT32 crewpos;
				persGetPersonaPosition(&crewpos);

				if(persGetCrewIsDead(crewpos))
				{
					*val = phui::js::ConstString("dead");
				}else
				{
					*val = phui::js::ConstString("riding");
				}
			}

		}
		break;
		case WALKRUN_PROP:
		if(Vehicle && VehicleData)
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				if(fpFPSIsWalking())
					*val = phui::js::ConstString("walk");
				else if(fpFPSIsFast() && fpFPSIsMoving(4.0f))
					*val = phui::js::ConstString("sprint");
				else
					*val = phui::js::ConstString("trot");
			}else
			{
				UINT32 crewpos;
				persGetPersonaPosition(&crewpos);

				if(persGetCrewIsDead(crewpos))
					*val = phui::js::ConstString("dead");
			}

		}
		break;

		case VEHICLETYPE_PROP:
		{

#if defined(USE_OSG)
			if(!playerGetVehicle()) return JS_TRUE;
			vehType const* vehtype = playerGetVehicle()->Type();
#else
			if(!Vehicle) return JS_TRUE;
			vehType const* vehtype = Vehicle->VehicleType;
#endif
			switch(vehtype->Type)
			{
				case VEH_TYPE_FLOATER:
					*val = phui::js::ConstString("floater");
				break;
				case VEH_TYPE_PERSON:
					*val = phui::js::ConstString("person");
				break;
				case VEH_TYPE_AIR:
					*val = phui::js::ConstString("air");
				break;
				case VEH_TYPE_GROUND:
					*val = phui::js::ConstString("ground");
				break;
				case VEH_TYPE_SEA:
					*val = phui::js::ConstString("sea");
				break;
#ifdef DOROTHY
				case VEH_TYPE_MECH:
					*val = phui::js::ConstString("mech");
				break;
#endif
				default:
				break;
			}
		}
		break;
		case NEAREST_CP_PROP:
		if(Vehicle && VehicleData)
		{
			StratDB::StratObject* stratObj = StratDB::FindStratObject(Vehicle->ModelData->closestCp);

			*val = (stratObj) ? (new jsStratObject(stratObj->FullObjID()))->jsVal(): JSVAL_NULL;
		}
		break;
		case RTB_PROP:
		{
			SINT32 ExitStatus;
			FULL_OBJ_ID id;
			vehDetermineExitStatus(&ExitStatus, &id);
			*val = INT_TO_JSVAL(ExitStatus);
		}
		break;
		case RTBFAC_PROP:
		{
			SINT32 ExitStatus;
			FULL_OBJ_ID id;
			vehDetermineExitStatus(&ExitStatus, &id);
			StratDB::StratObject* stratObj = StratDB::FindStratObject(id);
			*val = (stratObj) ? (new jsStratObject(stratObj->FullObjID()))->jsVal(): JSVAL_NULL;
		}
		break;
		case NOFIREZONE_PROP:
			*val = BOOLEAN_TO_JSVAL(fpFPSIsNoFireZone());
		break;
		case CAPTUREZONE_PROP:
			*val = BOOLEAN_TO_JSVAL(fpFPSIsCaptureZone());
		break;
		case HULKED_PROP:
			*val = BOOLEAN_TO_JSVAL(Vehicle && Vehicle->isHulked());
		break;
		case DEATHCAM_PROP:
			*val = BOOLEAN_TO_JSVAL(persIsInDeathCam());
		break;
		case COUNTRY_PROP:
		{
			if(Online)
			{
				*val = (new jsCountry(PlayerClient()->PersonaSelected().m_countryID))->jsVal();
			}
			else
			{
				// we're not online, see if we have a vehicle and use that
				vehVehicle* veh = persGetPersonaVehiclePtr();

				if(veh)
				{
					*val = (new jsCountry((WWII_COUNTRY)veh->VehicleType->WeaponSpec.country()))->jsVal();
				}
				else
				{
					*val = JSVAL_NULL;
				}
			}
		}
		break;
#ifdef DOROTHY
		case CROSSHAIRSX_PROP:
		{
			gr2Vehicle* vehicle = gr2GetPersonaVehicle();
			if(vehicle)
			{
				piVECTOR coords = {0,0,0};
				//vehicle->GetAimPosition(&coords);
				JS_NewDoubleValue(_jsctx, coords.x, val);
			}
		}
		break;
		case CROSSHAIRSY_PROP:
		{
			gr2Vehicle* vehicle = gr2GetPersonaVehicle();
			if(vehicle)
			{
				piVECTOR coords = {0,0,0};
				//vehicle->GetAimPosition(&coords);
				JS_NewDoubleValue(_jsctx, coords.y, val);
			}
		}
		break;
		case GR2VEHICLE_PROP:
		{
			gr2Vehicle* vehicle = gr2GetPersonaVehicle();

			if(vehicle && (!jsgr2vehicle.valid() || jsgr2vehicle.ptr()->object != vehicle))
			{
				if(vehicle->GetGameVehicle()->VehicleType->Type == VEH_TYPE_MECH)
					jsgr2vehicle = new jsMech(dynamic_cast<MechVehicle*>(vehicle));
				else
					jsgr2vehicle = new jsGR2Object(vehicle);
			}

			*val = (jsgr2vehicle.valid()) ? jsgr2vehicle->jsVal(): JSVAL_NULL;
		}
		break;
#endif
		default:
			return JS_FALSE;
		break;
	}
	return JS_TRUE;
}
