#include <syscfg.h> // Path is project specific.
#include <math.h>

#include "include/vars.h"

#include "ml/mlCompon.h"
#include "ml/mlweapons.h"

#include "ml/mlutility.h"
#include "veh/vehmain.h"
#include "ogrp/ogrptype.h"
#include "veh/vehanim.h"
#include "pers/persgunsight.h"
#include "fio/fio.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "mlatmo.h"
#include "pers/persmain.h"
#include "rn/rnmain.h"
#include "weap/weapfx.h"
#include "trn/trnmain.h"
#include "veh/vehVisDamage.h"
#include "ob/obcd.h"
#include "ui/uiMain.h"
#include "ui/uiUTF.h"

/************************************************************************************************
	mlWeapons
*************************************************************************************************/

// Collider types used for ground objects only (non-vehicular)
enum
{
	ML_WEAPON_COLLIDETYPE_ALL= 0, 		// use entire list, expensive
	ML_WEAPON_COLLIDETYPE_FRONT60DEG,	// use only those in front 60 degrees of player's vehicle
	ML_WEAPON_COLLIDETYPE_SHORTOMNI,	// use only those within short range of player's vehicle
	ML_WEAPON_COLLIDETYPE_MEDOMNI, 	// use only those within medium range of player's vehicle

	ML_WEAPON_MAX_COLLIDETYPES
};

// Bomb Mount Types
enum
{
	ML_WEAPON_BOMB_MOUNT_INTERNAL = 1,
	ML_WEAPON_BOMB_MOUNT_EXTERNAL,

	ML_WEAPON_MAX_BOMB_MOUNT_TYPES
};

// Weapon Mount Var Names
enum
{
	ML_WEAPON_TRIGGERTYPE = 1,
	ML_WEAPON_SELECTORTYPE,
	ML_WEAPON_CONFIGNUM,
	ML_WEAPON_MOUNTNUM,
	ML_WEAPON_WEAPONTYPE,
	ML_WEAPON_WEAPONOBJECT,
	ML_WEAPON_MOUNTOBJECT,

	ML_WEAPON_COLLIDERTYPE,
	ML_WEAPON_WEAPONSOUND,
	ML_WEAPON_USECONVERGENCE,

	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT1,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT2,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT3,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT4,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT5,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT6,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT7,
	ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT8,

	ML_WEAPON_POSITIONNUM,
	ML_WEAPON_USEBOMBBAY,
	ML_WEAPON_GUNFIRESMOKELEVEL,

	ML_WEAPON_AMMOCOUNT1,
	ML_WEAPON_AMMOCOUNT2,
	ML_WEAPON_AMMOCOUNT3,
	ML_WEAPON_AMMOCOUNT4,

	ML_WEAPON_BELTFILE1,
	ML_WEAPON_BELTFILE2,
	ML_WEAPON_BELTFILE3,
	ML_WEAPON_BELTFILE4,

	ML_WEAPON_RELOADSOUND,

	ML_WEAPON_BELTNAME1,
	ML_WEAPON_BELTNAME2,
	ML_WEAPON_BELTNAME3,
	ML_WEAPON_BELTNAME4,

	ML_WEAPON_MULTICREWPOS,

	ML_WEAPON_PICKLEMOUNT,
	ML_WEAPON_ADD_TO_GRP_AMMOCOUNT,

	ML_WEAPON_BOMB_MOUNT_TYPE,
	ML_WEAPON_DIVE_ANGLE,
	ML_JETTISON_POSITIONS,
	ML_CHECK_IF_BLOCKED,
	ML_MIN_CONVERGENCE,
	ML_WEAPON_MAXVAR
};

static UCHAR _WeaponMountVarnames[ML_WEAPON_MAXVAR][32] =
{
	"none",
	"triggertype",
	"selectornum",
	"confignum",
	"mountnum",
	"weapon",
	"weaponobject",
	"mountobject",
	"collidertype",
	"weaponsound",
	"usesconvergence",
	"ammoexplosioncomponent1",
	"ammoexplosioncomponent2",
	"ammoexplosioncomponent3",
	"ammoexplosioncomponent4",
	"ammoexplosioncomponent5",
	"ammoexplosioncomponent6",
	"ammoexplosioncomponent7",
	"ammoexplosioncomponent8",
	"position",
	"usebombbay",
	"gunfiresmokelevel",
	"ammocount",
	"ammocount2",
	"ammocount3",
	"ammocount4",
	"ammofile",
	"ammofile2",
	"ammofile3",
	"ammofile4",
	"reloadsound",
	"beltname",
	"beltname2",
	"beltname3",
	"beltname4",
	"multicrewpos",
	"picklemount",
	"addtogrpammocount",
	"bombmounttype",
	"diveangle",
	"jettisonpositions",
	"checkifblocked",
	"minconvergence"
};

static UCHAR _TriggerTypes[ML_MAX_TRIGGERTYPE][32] =
{
	"none",
	"primary",
	"secondary",
	"both",
};

static UCHAR _GroundCollideType[ML_WEAPON_MAX_COLLIDETYPES][32] =
{
	"none",
	"front60deg",
	"short",
	"medium",
};

static UCHAR _BombMountType[ML_WEAPON_MAX_BOMB_MOUNT_TYPES][32] =
{
	"none",
	"internal",
	"external"
};

static void _HandleDamage(mlWeaponMount*, mlComponent*, mlVehicleData*);
static void _HandleReload(mlWeaponMount* Mount, mlComponent*,  OBJECT*, FLOAT64 CurrentTime);
static void _HandlePickle(mlWeaponMount* Mount, UINT32* PrimaryTriggerDown, UINT32* SecondaryTriggerDown);
static void _HandleAmmoCycle(mlWeaponMount* Mount, mlVehicleData* VehicleData, mlComponent* Component,  UINT32 Position);

static UINT32 _DetermineTriggerFiring(mlWeaponMount* Mount, mlVehicleData* VehicleData, UINT32 Position, UINT32 PrimaryTriggerDown,
										UINT32 SecondaryTriggerDown, UINT32* PrimaryIsFiring, UINT32* SecondaryIsFiring);
static SINT32 _CanReleaseBomb(mlWeaponMount*, piMATRIX* AttMat);
static void _SetupConvergence(mlWeaponMount*, vehVehicle* , FLOAT64 ConvergenceRange);

static UINT32 _FireMountUpdate(mlWeaponMount*, mlVehicleData*, FLOAT64 DeltaTime, UINT32 Position, mlComponent*,
							FLOAT64 CurrentTime, OBJECT* PlaneObject, FLOAT64VECTOR* Force, LOGICAL const MountFiring);


static void _UpdateMountWeight(mlWeaponMount* Mount, mlComponent* Component);
static void _UpdateAmmoCount(mlWeaponMount* Mount);
static void _UpdateBombDrag(mlWeaponMount* Mount, vehVehicle* Vehicle, FLOAT64VECTOR* Force);

static void _JettisonMount(mlWeaponMount*, mlVehicleData*, FLOAT64 DeltaTime,
							FLOAT64 CurrentTime, OBJECT* PlaneObject, FLOAT64VECTOR* Force);

static void _FireWeaponInMount(mlWeaponMount* Mount, mlVehicleData* VehicleData, FLOAT64 DeltaTime,
								FLOAT64 CurrentTime, OBJECT* PlaneObject, FLOAT64VECTOR* Force, FLOAT64* LastFireTime, UINT32 i,
								void* FireVehicle);

static void _GetFireAttPosVel(mlWeaponMount* Mount, int i, mlVehicleData* VehicleData, piMATRIX* FireAtt,
								FLOAT64VECTOR* RelativePnt, FLOAT64VECTOR* FirePnt,  FLOAT64VECTOR* FireVel);

static LOGICAL _IsMountBlocked(mlWeaponMount* Mount, int MountNum, vehVehicle*);


/************************************************************************************************
	mlWeaponMountAddSpecialVariable
*************************************************************************************************/

UINT32 mlWeaponMountAddSpecialVariable(void* SpecialData, const char * VarName, const char* VarContents,
										 mlComponentList* ComponentList,  mlComponent* Component, void* VehicleMasterPtr)
{
	mlWeaponMount* MountData;
	vehVehicle* VehiclePtr;
	vehType const* VehicleType;
	UINT32 VarNum, CompNum;
	SINT32 MountNumber, ConfigNumber, WeaponNumber;

	if(!SpecialData || !VarName || !VarContents || !VehicleMasterPtr)  return 1;

	VehiclePtr = (vehVehicle*)VehicleMasterPtr;
	VehicleType = VehiclePtr->VehicleType;

	if(VehicleType == nullptr) return 1;

	MountData = (mlWeaponMount*)SpecialData;

	mlUtilityFindVarname(&VarNum, (UCHAR*)VarName, (UCHAR*)&_WeaponMountVarnames[0][0], 32, ML_WEAPON_MAXVAR);

	switch(VarNum)
	{
		case ML_WEAPON_TRIGGERTYPE:
			mlUtilityFindVarname(&MountData->TriggerType, (UCHAR*)VarContents, (UCHAR*)&_TriggerTypes[0][0], 32, ML_MAX_TRIGGERTYPE);
		break;
		case ML_WEAPON_SELECTORTYPE:
			MountData->SelectorNum = atoi(VarContents);
		break;
		case ML_WEAPON_CONFIGNUM:
			MountData->ConfigNum = atoi(VarContents);
		break;
		case ML_WEAPON_MOUNTNUM:
			MountData->MountNum = atoi(VarContents);
		break;
		case ML_WEAPON_WEAPONTYPE:
			MountNumber = MountData->MountNum - 1;
			ConfigNumber = MountData->ConfigNum - 1;
			WeaponNumber = MountData->NumberOfWeaponsInMount;

			if (MountNumber < 0 || ConfigNumber < 0 || WeaponNumber < 0)
				break;

			if (MountNumber >= VEH_MAX_MOUNTS)
				Write_Log(LOG_VEHICLE, LOG_WARN, "Mount Number for %s too big", Component->ComponentName);
			if (ConfigNumber >= VEH_MAX_LOADOUTS)
				Write_Log(LOG_VEHICLE, LOG_WARN, "Config/Loadout number for %d too big", Component->ComponentName);
			if (WeaponNumber >= VEH_MAX_WEAPONS_PER_MOUNT)
				Write_Log(LOG_VEHICLE, LOG_WARN, "Too many weapons in %s", Component->ComponentName);

			if (VehicleType->Loadout[ConfigNumber].WeaponTypes[MountNumber][WeaponNumber] == nullptr)
			{
				Write_Log(LOG_VEHICLE, LOG_WARN, "Loadout file for vehicle has no weapon for mount %d, loadout/config %d, weaponnum %d",
							MountNumber+1, ConfigNumber+1, WeaponNumber+1);
			}else
			{

				weapInitializeWeapon(&MountData->Weapons[WeaponNumber], (weaponType*)VehicleType->Loadout[ConfigNumber].WeaponTypes[MountNumber][WeaponNumber]);
				MountData->NumberOfWeaponsInMount++;
				strcpy((char*)MountData->MountName,(char*)VehicleType->Loadout[ConfigNumber].MountObject[MountNumber]);
				strcpy((char*)MountData->WeaponNames[WeaponNumber], (char*)VehicleType->Loadout[ConfigNumber].WeaponObject[MountNumber][WeaponNumber]);
			}
		break;
		case ML_WEAPON_AMMOCOUNT1:
		case ML_WEAPON_AMMOCOUNT2:
		case ML_WEAPON_AMMOCOUNT3:
		case ML_WEAPON_AMMOCOUNT4:
			if(MountData->NumberOfWeaponsInMount > 0)
			{
				int ammoAmount = atoi(VarContents);
				MountData->Weapons[MountData->NumberOfWeaponsInMount-1].NumTotalRounds[VarNum-ML_WEAPON_AMMOCOUNT1]		= (ammoAmount >= 0) ? ammoAmount: 0;
				MountData->Weapons[MountData->NumberOfWeaponsInMount-1].NumRoundsAmmoLeft[VarNum-ML_WEAPON_AMMOCOUNT1]	= ammoAmount;
			}
		break;
		case ML_WEAPON_WEAPONOBJECT:
		break;
		case ML_WEAPON_WEAPONSOUND:
			if(MountData->NumberOfWeaponsInMount > 0)
				strcpy((char*)&(MountData->WeaponSounds[MountData->NumberOfWeaponsInMount-1][0]),VarContents);
		break;
		case ML_WEAPON_RELOADSOUND:
			if (MountData->NumberOfWeaponsInMount > 0)
				strcpy((char*)&(MountData->WeaponSoundsReload[MountData->NumberOfWeaponsInMount-1][0]),VarContents);
		break;
		case ML_WEAPON_BELTFILE1:
		case ML_WEAPON_BELTFILE2:
		case ML_WEAPON_BELTFILE3:
		case ML_WEAPON_BELTFILE4:
			mlLoadBeltFile(&MountData->Weapons[MountData->NumberOfWeaponsInMount-1],(UCHAR*)VarContents,VarNum-ML_WEAPON_BELTFILE1);
		break;
		case ML_WEAPON_BELTNAME1:
		case ML_WEAPON_BELTNAME2:
		case ML_WEAPON_BELTNAME3:
		case ML_WEAPON_BELTNAME4:
			if(MountData->NumberOfWeaponsInMount > 0)
				strcpy((char*)MountData->Weapons[MountData->NumberOfWeaponsInMount-1].BeltName[VarNum-ML_WEAPON_BELTNAME1], VarContents);
		break;
		case ML_WEAPON_COLLIDERTYPE:
			mlUtilityFindVarname(&MountData->CollideType, (UCHAR*)VarContents, (UCHAR*)&_GroundCollideType[0][0], 32, ML_WEAPON_MAX_COLLIDETYPES);
		break;
		case ML_WEAPON_USECONVERGENCE:
			MountData->UsesConvergence = atoi(VarContents);
		break;
		case ML_WEAPON_POSITIONNUM:
			MountData->PositionNum = atoi(VarContents)-1;
		break;
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT1:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT2:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT3:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT4:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT5:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT6:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT7:
		case ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT8:
			strcpy((char*)MountData->AmmoExplosionDamageComponent[VarNum - ML_WEAPON_AMMOEXPLOSION_DAMAGECOMPONENT1], VarContents);
			vehFindHitmapObjectNumByName(&CompNum, (UCHAR*)VarContents, VehicleType->VehicleTypeNum);
			if(CompNum == 0)
				Write_Log(LOG_VEHICLE, LOG_WARN,"Ammo Explosion damage component %s not found in %s", VarContents, Component->ComponentName);
		break;
		case ML_WEAPON_USEBOMBBAY:
			MountData->UseBombBay = atoi(VarContents);
		break;
		case ML_WEAPON_GUNFIRESMOKELEVEL:
			MountData->GunFireSmokeLevel = (FLOAT32)atof(VarContents);
		break;
		case ML_WEAPON_PICKLEMOUNT:
			MountData->PickleMount = atoi(VarContents);
		break;
		case ML_WEAPON_ADD_TO_GRP_AMMOCOUNT:
			MountData->AddToGroupAmmoCount = atoi(VarContents);
		break;
		case ML_WEAPON_BOMB_MOUNT_TYPE:
			mlUtilityFindVarname(&MountData->BombMountType, (UCHAR*)VarContents, &_BombMountType[0][0], 32, ML_WEAPON_MAX_BOMB_MOUNT_TYPES);
		break;
		case ML_WEAPON_DIVE_ANGLE:
			MountData->DiveAngle = (FLOAT32)atof(VarContents);
		break;
		case ML_JETTISON_POSITIONS:
		{
			char* s;
			char buf[32];
			strncpy(buf, VarContents, 31);
			s = strtok(buf, ",");
			while(s)
			{
				SINT32 Position = atoi(s) - 1;
				if(Position >= 0 && Position < ML_MAX_CREW_POSITIONS)
					MountData->CanJettison[Position] = 1;

				s = strtok(nullptr, " ");
			}
		}
		break;
		case ML_CHECK_IF_BLOCKED:
			MountData->CheckIfBlocked = atoi(VarContents);
		break;
		case ML_MIN_CONVERGENCE:
			MountData->MinConvergence = (FLOAT32)atof(VarContents);
		break;
		default:
			Write_Log(LOG_VEHICLE, LOG_WARN, "Unidentified variable %s in component %s\n", VarName, Component->ComponentName);
		break;

	}

	return 0;
}

/************************************************************************************************
	mlLoadBeltFile
*************************************************************************************************/

UINT32 mlLoadBeltFile(weapWeapon* Weapon,UCHAR* VarContents,SINT32 AmmoGroupNum)
{
	if(!Weapon || !VarContents) return 1;

	char Buffer[256];

	// Use 'Buffer' for file name while we open the file.
	sprintf(Buffer, "vehicles\\ammo\\%s", VarContents);
	SINT32 File = fioOpen(Buffer);
	if (File == -1)
	{
		Write_Log(LOG_VEHICLE, LOG_WARN,"Belt file %s not found", Buffer);
		return 1;
	}

	// Now use Buffer as a data buffer.
	UINT16 numRounds = 0;
	memset(Buffer, 0, sizeof(Buffer));
	while(fioGetString(File, Buffer, 255) > 1)
	{
		weapFindAmmoTypeByName(&Weapon->BeltSequence[AmmoGroupNum][numRounds], (UCHAR*)Buffer);
		if (Weapon->BeltSequence[AmmoGroupNum][numRounds] != nullptr)
			++numRounds;
	}

	Weapon->NumRoundsInBeltSequence[AmmoGroupNum] = numRounds;

	fioClose(File);

	return 0;
}

/************************************************************************************************
	mlUpdateWeaponMount
*************************************************************************************************/

UINT32 mlUpdateWeaponMount(FLOAT64VECTOR* Force, FLOAT64ATT* Torque,
						   mlComponent* Component, mlVehicleData* VehicleData,
						   FLOAT64 DeltaTime, FLOAT64 CurrentTime,
						   UINT32* PrimaryIsFiring, UINT32* SecondaryIsFiring, OBJECT* PlaneObject,
						   mlComponentList* ComponentList, FLOAT64 ConvergenceRange, UINT32* MountFired)
{
	mlWeaponMount* Mount;
	UINT32 MountFiring;
	UINT32 PrimaryTriggerDown, SecondaryTriggerDown;
	vehVehicle* Vehicle = persGetPersonaVehiclePtr();

	Force->x = Force->y = Force->z = 0;
	Torque->Roll = Torque->Pitch = Torque->Yaw = 0;

	if(Component == nullptr || Component->SpecialData == nullptr) return (UINT32)-1;

	Mount = (mlWeaponMount*)(Component->SpecialData);
	if(Mount->MountObject == nullptr || Mount->MountObject->dof == nullptr) return (UINT32)-1;

	if(ConvergenceRange < Mount->MinConvergence)
		ConvergenceRange =  Mount->MinConvergence;

	PrimaryTriggerDown 	= VehicleData->PrimaryTriggerDown;
	SecondaryTriggerDown= VehicleData->SecondaryTriggerDown;

	UINT32 Position = persGetPersonaPosition();

	*PrimaryIsFiring = *SecondaryIsFiring = 0;

	_HandleDamage(Mount, Component, VehicleData);
	_HandleReload(Mount, Component, PlaneObject, CurrentTime);

	if(VehicleData->JettisonTriggerDown)
	{
		if(Mount->BombMountType && Mount->CanJettison[Position] && _CanReleaseBomb(Mount, &VehicleData->VehicleAttMat) &&
			(VehicleData->MultiCrewPos == 0 ||VehicleData->MultiCrewPos == Vehicle->VehicleType->MultiCrewPos[Mount->PositionNum])	)
			_JettisonMount(Mount, VehicleData, DeltaTime, CurrentTime, PlaneObject, Force);
	}
	else
	{
		// determine if mount is firing
		_HandlePickle(Mount, &PrimaryTriggerDown, &SecondaryTriggerDown);
		_HandleAmmoCycle(Mount, VehicleData, Component, Position);

		MountFiring = _DetermineTriggerFiring(Mount, VehicleData, Position, PrimaryTriggerDown,
												SecondaryTriggerDown, PrimaryIsFiring, SecondaryIsFiring);

		if(Mount->UseBombBay == 1 && VehicleData->BombBayDoor == 0)
			MountFiring = 0;

		if(Mount->WeaponJammed == 1)
			MountFiring = 0;

		if(MountFiring && Mount->BombMountType)
			MountFiring = _CanReleaseBomb(Mount, &VehicleData->VehicleAttMat);

		if(Mount->UsesConvergence != 0 && Mount->ConvergenceMatSetup == FALSE)
			_SetupConvergence(Mount, (vehVehicle*)VehicleData->VehicleMasterPtr, ConvergenceRange);

		// Fire Mount
		*MountFired = _FireMountUpdate(Mount, VehicleData, DeltaTime, Position, Component, CurrentTime,
										PlaneObject, Force, MountFiring);
	}

	// Update info related to mount
	_UpdateMountWeight(Mount, Component);
	_UpdateAmmoCount(Mount);
	if(Mount->BombMountType == ML_WEAPON_BOMB_MOUNT_EXTERNAL)
		_UpdateBombDrag(Mount, Vehicle, Force);

	return 0;
}

/************************************************************************************************
	_HandleDamage
*************************************************************************************************/

static void _HandleDamage(mlWeaponMount* Mount, mlComponent* Component, mlVehicleData* VehicleData)
{
	UINT32 NumHits = 0;
	UINT32 i,j,k;
	FLOAT64 DamagePerHit = 0;
	UINT32 AmmoOriginal = 0, AmmoCurrent = 0;
	FLOAT64 Probability = 0;
	FLOAT64 Damage = 0;
	weapAmmoType* Ammo = nullptr;
	UINT32 TempInt = 0, TempInt1 = 0;
	SINT32 DamageComponentNum = 0;
	vehVehicle* Vehicle = nullptr;

	Vehicle = (vehVehicle*)VehicleData->VehicleMasterPtr;

	// check mount for damage
	if (Component->NewDamageNumHits[0] > 0)
	{
		Component->CurrentDamage += Component->NewDamage[0];

		if (Component->CurrentDamage > Component->DamageLevels[0])
		{
			if (Mount->WeaponJammed == 0) Write_Log(LOG_WEAP, LOG_INFO, "Weapon jammed due to damage to weapon");

			Mount->WeaponJammed = 1;
		}
	}

	if (VehicleData->MultiCrewPos != 0 && VehicleData->MultiCrewPos != Vehicle->VehicleType->MultiCrewPos[Mount->PositionNum]
		)
	{
		return;
	}

	// check ammo for damage
	NumHits = 0;
	for (i=1;i<3;i++)
	{
		if (Component->NewDamageNumHits[i] > 0)
		{
			NumHits++;
			Mount->AmmoDamage += Component->NewDamage[i];
		}
	}
	if (NumHits > 0)
	{
		if (Mount->AmmoDamage > Component->DamageLevels[1])
		{
			if (Mount->Weapons[0].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT)
			{
				if (Mount->WeaponJammed == 0)
				{
					Write_Log(LOG_WEAP, LOG_INFO, "Weapon jammed due to damage to ammo");
				}
				Mount->WeaponJammed = 1;
			}
		}
	}

	// now check for ammo explosion
	if (NumHits > 0)
	{
		DamagePerHit = 0.0;
		for (i=1;i<3;i++)
		{
			DamagePerHit += Component->NewDamage[i];
		}
		DamagePerHit /= NumHits;

		for (i=0;i<NumHits;i++)
		{
			Probability = sqrt(DamagePerHit) / 300.0;
			if (mathGetFloat32ChaoticNumber() < 0.5)
			{
				if (mathGetFloat32ChaoticNumber() < Probability)
				{
					AmmoCurrent = 0;
					AmmoOriginal = 0;
					// ok check ammo load
					for (j=0;j<Mount->NumberOfWeaponsInMount;j++)
					{
						for (k=0;k<4;k++)
						{
							AmmoCurrent += Mount->Weapons[j].NumRoundsAmmoLeft[k];
							AmmoOriginal += Mount->Weapons[j].NumTotalRounds[k];
						}
					}

					if (AmmoOriginal > 0)
					{
						Probability = (FLOAT64)AmmoCurrent / (FLOAT64)AmmoOriginal;

						if (mathGetFloat32ChaoticNumber() < Probability)
						{

							// ammo explosion
							// find a weapon and ammo grouping that still has ammo
							TempInt1 = 4;
							for (j=0;j<4;j++)
							{
								for (k=0;k<4;k++)
								{
									if (Mount->Weapons[j].NumRoundsAmmoLeft[k] != 0)
									{
										TempInt1 = j;
									}
								}
							}
							if (TempInt1 == 4)
							{
								Write_Log(LOG_WEAP, LOG_WARN, "Error! Exploding ammo for weapon that is emtpy(1)!");
								TempInt1 = 0;
							}

							TempInt = 0;
							while (Mount->Weapons[TempInt1].NumRoundsAmmoLeft[TempInt] == 0 &&
										TempInt < 4)
							{
								TempInt++;
							}
							if (TempInt == 4)
							{
								TempInt = 0;
								Write_Log(LOG_WEAP, LOG_WARN, "Error! Exploding ammo for weapon that is empty(2)!");
								++Mount->Weapons[TempInt1].NumRoundsAmmoLeft[TempInt];
							}
							Ammo = Mount->Weapons[TempInt1].BeltSequence[TempInt][Mount->Weapons[TempInt1].BeltSequencePosition[TempInt]];

							// note: real energy from propellant would be much higher,
							// however most wouldn't go directly into damaging stuff
							// so we're going to use this for an approximation
							if (Ammo != nullptr)
							{
								Damage = Ammo->InitVelocity * Ammo->InitVelocity * Ammo->ProjectileWeight * 0.05 * 0.1;

								// 30% chance of a "chain reaction"
								if (mathGetFloat32ChaoticNumber() > 0.3)
								{
									--Mount->Weapons[TempInt1].NumRoundsAmmoLeft[TempInt];
									if (Mount->Weapons[TempInt1].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT)
									{
										Mount->WeaponJammed = 1;
									}
								}
								else
								{
									Mount->WeaponJammed = 1;

									Damage *= AmmoCurrent;

									for (j=0;j<Mount->NumberOfWeaponsInMount;j++)
									{
										for (k=0;k<4;k++)
										{
											Mount->Weapons[j].NumRoundsAmmoLeft[k] = 0;
										}
									}
								}

								vehAnimChangeVehicleStateByName(Vehicle, (UCHAR*)"tankfire", TRUE);

								vehVisDamageGroupDestroyed(Vehicle, "ammocook", TRUE, TRUE);
								vehVisDamageHulkVehicle(Vehicle, FALSE);

								for (j=0;j<8;j++)
								{
									if(Mount->AmmoExplosionDamageComponent[j][0])
									{
										DamageComponentNum = 0;
										vehFindHitmapObjectNumByName((UINT32*)&DamageComponentNum,
																	Mount->AmmoExplosionDamageComponent[j],
																	Vehicle->VehicleType->VehicleTypeNum);
										if (DamageComponentNum > 0)
										{
											vehApplyDamageToVehicleComponent(Vehicle, DamageComponentNum,  Damage, 0);

										}
										else
										{
											Write_Log(LOG_WEAP, LOG_WARN,"Attempting to fuel-damage hit location %s, component %s, hit location not found",
															Mount->AmmoExplosionDamageComponent[j],
															Component->ComponentName);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

/************************************************************************************************
	_HandleReload
*************************************************************************************************/

static void _HandleReload(mlWeaponMount* Mount, mlComponent* Component, OBJECT* PlaneObject, FLOAT64 CurrentTime)
{
	UINT32 Reloading;
	UINT32 i;

	for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
	{
		Reloading = weapIsWeaponReloading(&Mount->Weapons[i], CurrentTime / 1000.0);

		if(!Mount->LastUpdateWasReloading[i] && Reloading)
			weapfxMountReloadSound(Mount, i, &Component->OriginalRelativePos, PlaneObject);

		Mount->LastUpdateWasReloading[i] = Reloading;
	}
}

/************************************************************************************************
	_HandlePickle
*************************************************************************************************/

static void _HandlePickle(mlWeaponMount* Mount, UINT32* PrimaryTriggerDown, UINT32* SecondaryTriggerDown)
{
	SINT32 Pickle;

	if (Mount->TriggerType == ML_TRIGGERTYPE_PRIMARY &&
		Mount->PickleMount != 0 &&
		*PrimaryTriggerDown)
	{
		Pickle = mlGetPrimaryPickleSetting();
		if (Pickle == 0)
			*PrimaryTriggerDown = 0;
	}

	if (Mount->TriggerType == ML_TRIGGERTYPE_SECONDARY &&
		Mount->PickleMount != 0 &&
		*SecondaryTriggerDown)
	{
		Pickle = mlGetSecondaryPickleSetting();
		if (Pickle == 0)
			*SecondaryTriggerDown = 0;
	}

	if (Mount->TriggerType == ML_TRIGGERTYPE_BOTH &&
		Mount->PickleMount != 0 &&
		(*PrimaryTriggerDown || *SecondaryTriggerDown))
	{
		Pickle = mlGetPrimaryPickleSetting();
		if (Pickle == 0)
		{
			Pickle = mlGetSecondaryPickleSetting();
			if (Pickle == 0)
			{
				*PrimaryTriggerDown = 0;
				*SecondaryTriggerDown = 0;
			}
		}
	}
}

/************************************************************************************************
	_HandleAmmoCycle
*************************************************************************************************/

static void _HandleAmmoCycle(mlWeaponMount* Mount, mlVehicleData* VehicleData, mlComponent* Component, UINT32 Position)
{
	UINT32 i;
	UINT32 CurAmmo, NewAmmo;

	for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
	{
		if (VehicleData->CycleAmmo == 1 && Position == Mount->PositionNum)
		{
			NewAmmo = Mount->Weapons[i].NextAmmoToUse.Get();	// Covert access
			if (NewAmmo!= 0)
			{
				--NewAmmo;
			}
			else
			{
				NewAmmo = Mount->Weapons[i].AmmoTypeInUse;
			}
			CurAmmo = NewAmmo;

			do
			{
				NewAmmo++;
				if (NewAmmo >= 4)
				{
					NewAmmo = 0;
				}

				if (Mount->Weapons[i].NumRoundsAmmoLeft[NewAmmo] != 0)
				{
					break;
				}
			}
			while (NewAmmo != CurAmmo);

			if (NewAmmo != CurAmmo)
			{
				if(Mount->Weapons[i].BeltName[NewAmmo][0])
				{
					const std::wstring& format = uiLocString("vehicles.reload");
					rnmainListText(format.c_str(), UTF16(Mount->Weapons[i].BeltName[NewAmmo])); // , UTF16(Component->ComponentName) <-- no longer shown; needed to be generic
				}

				Mount->Weapons[i].NextAmmoToUse = NewAmmo + 1;
			}
		}
	}
}

/************************************************************************************************
	_DetermineTriggerFiring
*************************************************************************************************/

static UINT32 _DetermineTriggerFiring(mlWeaponMount* Mount, mlVehicleData* VehicleData, UINT32 Position, UINT32 PrimaryTriggerDown,
										UINT32 SecondaryTriggerDown, UINT32* PrimaryIsFiring, UINT32* SecondaryIsFiring)
{
	UINT32 MountFiring = 0;
	vehVehicle* Vehicle = (vehVehicle*)VehicleData->VehicleMasterPtr;
	UINT32 i;

	if ((Mount->TriggerType == ML_TRIGGERTYPE_PRIMARY ||
		 Mount->TriggerType == ML_TRIGGERTYPE_BOTH)
		 && PrimaryTriggerDown &&
		 Position == Mount->PositionNum &&
		 (VehicleData->MultiCrewPos == 0 || VehicleData->MultiCrewPos == Vehicle->VehicleType->MultiCrewPos[Mount->PositionNum]
		 ))
	{

		MountFiring = 1;
		for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
		{
			if (Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] > 0 &&
				Mount->Weapons[i].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT &&
				!Mount->WeaponJammed)
			{
				*PrimaryIsFiring = 1;
			}
		}
	}

	if ((Mount->TriggerType == ML_TRIGGERTYPE_SECONDARY ||
		 Mount->TriggerType == ML_TRIGGERTYPE_BOTH)
		 && SecondaryTriggerDown &&
		 Position == Mount->PositionNum &&
		 (VehicleData->MultiCrewPos == 0 || VehicleData->MultiCrewPos == Vehicle->VehicleType->MultiCrewPos[Mount->PositionNum]
		 ))
	{
		// a selector num of 99 means that the mount will always fire when 2ndary trigger pressed
		if (Mount->SelectorNum == VehicleData->OrdnanceSelected + 1 ||
			Mount->SelectorNum == 99)
		{
			MountFiring = 1;
			for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
			{
				if (Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] > 0 &&
					Mount->Weapons[i].WeaponFireType != WEAP_FIRETYPE_SINGLESHOT &&
					!Mount->WeaponJammed)
				{
					*SecondaryIsFiring = 1;
				}
			}
		}
	}

	return MountFiring;
}

/************************************************************************************************
	_CanReleaseBomb
*************************************************************************************************/

static SINT32 _CanReleaseBomb(mlWeaponMount* Mount, piMATRIX* AttMat)
{
	SINT32 MountFiring = 1;
	FLOAT32 DiveAngle, MaxRoll;
	piVECTOR dir;
	FLOAT32 pitch, roll;

	DiveAngle = (Mount->DiveAngle) ? Mount->DiveAngle : 90.0f;
	MaxRoll = (Mount->BombMountType == ML_WEAPON_BOMB_MOUNT_INTERNAL) ? 30.0f : 90.0f;

	pitch = 90.0f, roll = 90.0f;

	// Could also use the mathEul functions here if I could figure them out.
	dir.y = 1.0f; dir.x = dir.z = 0.0f;
 	math3dMultiplyRotation(&dir, &dir, AttMat);
 	if(dir.x || dir.y)
		pitch = (FLOAT32)atan(dir.z / sqrt(dir.x * dir.x + dir.y * dir.y)) * 180.0f / PI;

	dir.x = 1.0f; dir.y = dir.z = 0.0f;
 	math3dMultiplyRotation(&dir, &dir, AttMat);
 	if(dir.x || dir.y)
		roll = (FLOAT32)fabs(atan(dir.z / sqrt(dir.x * dir.x + dir.y * dir.y))) * 180.0f / PI;

	dir.z = 1.0f; dir.x = dir.y = 0.0f;
	math3dMultiplyRotation(&dir, &dir, AttMat);
	if(dir.z < 0.0f) // inverted
		roll += 180.0f; // not* actual* roll angle, but sufficent for purposes of roll check.

	if(roll > MaxRoll || pitch < -DiveAngle || pitch > DiveAngle)
		MountFiring = 0;

	return MountFiring;
}

/************************************************************************************************
	_SetupConvergence
*************************************************************************************************/
// only uses 1st ammo type of 1st weapon's belt

static void _SetupConvergence(mlWeaponMount * Mount,vehVehicle* Vehicle,FLOAT64 ConvergenceRange)
{
	piMATRIX TempMat;
	piVECTOR Att;
	OBJECT * FireDOF;
	OBJECT * ViewDOF;
	OBJECT * Object;
	SINT32 i;

	Mount->ConvergenceMatSetup = TRUE;
	Mount->ConvergenceMat = IdentityMatrix;

	// first find the gunsight view for the position
	char s[32];
	strprintf(s, "gview%d", Mount->UsesConvergence); // UsesConvergence specifies position number
	Object = nullptr;
	ogrpFindSubObjectByName(&Object,Vehicle->VehicleObjectData->PlaneObject,s);
	if (Object == nullptr)
		return;

	ViewDOF = Object;

	// next find the fire DOF
	FireDOF = Mount->WeaponFirePoints[0];

	// now determine alignment angle
	persGunsightAlignDOF(&Mount->ConvergenceMat,FireDOF,ViewDOF,(FLOAT32)ConvergenceRange);

	// finally adjust this alignment for ammunition drop at that range
	Att.y = Att.z = 0.0f;
	persGunsightCalcSightAngle(&Att.x,(char*)Mount->Weapons[0].BeltSequence[0][0]->Name,(FLOAT32)ConvergenceRange);
	mathEul_ToMatrix(&TempMat,EulOrdXYZs,&Att);

	math3dMatrixMultiply(&Mount->ConvergenceMat,&TempMat,&Mount->ConvergenceMat);

	for (i=0;i<ML_MAX_WEAPONS_PER_MOUNT;i++)
	{
		if (Mount->WeaponFirePoints[i] != nullptr)
		{
			Mount->WeaponFirePoints[i]->dof->dofposmatrix = Mount->ConvergenceMat;
			Mount->WeaponFirePoints[i]->dof->dofflags |= DOF_USE_DOFPOSMATRIX;
			Mount->WeaponFirePoints[i]->flag2 |= OBJECT_DOF_USECHILDMATRIX;
		}
	}
}

/************************************************************************************************
	_FireMountUpdate
*************************************************************************************************/

static UINT32 _FireMountUpdate(mlWeaponMount* Mount, mlVehicleData* VehicleData, FLOAT64 DeltaTime, UINT32 Position, mlComponent* Component,
							FLOAT64 CurrentTime, OBJECT* PlaneObject, FLOAT64VECTOR* Force, LOGICAL const MountFiring)
{
	UINT32 i, j;
	SINT32 RoundsLeft = 0;
	UINT32 MountFired = 0;
	FLOAT64 LastFireTime = 0;
	SINT32 RecoilNum = 0;
	vehVehicle* Vehicle = nullptr;
    FLOAT64VECTOR TempForce = {0, 0, 0};
	LOGICAL MountWeaponFiring = 0;

	MountFired = 0;
	Vehicle = (vehVehicle*)VehicleData->VehicleMasterPtr;

	for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
	{
		MountWeaponFiring = MountFiring;

		if(weapIsWeaponReloading(&Mount->Weapons[i], CurrentTime / 1000.0))
			MountWeaponFiring = 0;

		if(MountWeaponFiring && Mount->CheckIfBlocked)
			MountWeaponFiring = !_IsMountBlocked(Mount, i, (vehVehicle*)VehicleData->VehicleMasterPtr);

// $#@$! hack since we don't have a $!@$!$ fire message
		if(MountWeaponFiring && Mount->Weapons[i].WeaponType->GhostAmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO)
		{
			FLOAT64 speed = mathExactVectorLength(&VehicleData->VehicleRealVel);
			if(speed > 70 || VehicleData->VehiclePos.z > 60)
				MountWeaponFiring = FALSE;
		}

        if(!MountWeaponFiring &&
           Mount->Weapons[i].NextAmmoToUse != 0 &&
           Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].NextAmmoToUse-1] != 0)
        {
			Mount->Weapons[i].AmmoTypeInUse = Mount->Weapons[i].NextAmmoToUse-1;
			Mount->Weapons[i].NextAmmoToUse = 0;
			Mount->Weapons[i].ReloadStartTime = (FLOAT32)(CurrentTime / 1000.0);
        }

		if(MountWeaponFiring)
		{
			RoundsLeft = Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse];

            _FireWeaponInMount(Mount, VehicleData, DeltaTime, CurrentTime,
							PlaneObject, &TempForce, &LastFireTime, i, VehicleData->VehicleMasterPtr);

			// handle cycling weapon ammo types
			if (Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] == 0 &&
				RoundsLeft != 0)
			{
				if (Mount->Weapons[i].NextAmmoToUse != 0 &&
					Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].NextAmmoToUse-1] != 0)
				{
					Mount->Weapons[i].AmmoTypeInUse = Mount->Weapons[i].NextAmmoToUse-1;
					Mount->Weapons[i].NextAmmoToUse = 0;
					Mount->Weapons[i].ReloadStartTime = (FLOAT32)(CurrentTime / 1000.0);
				}
				else
				{
					for (j=0;j<4;j++)
					{
						if (Mount->Weapons[i].NumRoundsAmmoLeft[j] != 0)
						{
							if(Mount->Weapons[i].BeltName[Mount->Weapons[i].AmmoTypeInUse][0] &&
								Mount->Weapons[i].BeltName[j][0])
							{
								rnmainListText(uiLocString("loc.english.851").c_str(),
												UTF16(Mount->Weapons[i].BeltName[Mount->Weapons[i].AmmoTypeInUse]),
												UTF16(Component->ComponentName),
												UTF16(Mount->Weapons[i].BeltName[j]));
							}

							Mount->Weapons[i].AmmoTypeInUse = j;
							Mount->Weapons[i].ReloadStartTime = (FLOAT32)(CurrentTime / 1000.0);
							break;
						}
					}
				}
			}

			if (fabs(TempForce.x) > 0.01 || fabs(TempForce.y) > 0.01 || fabs(TempForce.z) > 0.01)
			{
				MountFired = 1;

				if (Mount->PickleMount != 0)
				{
					if(Mount->TriggerType == ML_TRIGGERTYPE_PRIMARY || Mount->TriggerType == ML_TRIGGERTYPE_BOTH)
						mlResetPrimaryPickleSetting();

					if(Mount->TriggerType == ML_TRIGGERTYPE_SECONDARY || Mount->TriggerType == ML_TRIGGERTYPE_BOTH)
						mlResetSecondaryPickleSetting();
				}

				if (Mount->GunFireSmokeLevel > 0.0f)
				{
					((vehVehicle*)VehicleData->VehicleMasterPtr)->StateAnimation.MainGunSmokeLevel += Mount->GunFireSmokeLevel;
					((vehVehicle*)VehicleData->VehicleMasterPtr)->StateAnimation.MainGunLastUpdateTime = frametime;
				}

				weapfxMountFireSound(Mount, i, RoundsLeft, &Component->OriginalRelativePos, PlaneObject);

				Force->x += TempForce.x;
				Force->y += TempForce.y;
				Force->z += TempForce.z;
			}else if(RoundsLeft < 1)
			{
				weapfxMountStopFireSound(Mount, i, RoundsLeft);
			}
		}else
		{
			weapfxMountStopFireSound(Mount, i, RoundsLeft);
		}
	}

	if(MountFired)
	{
		// trigger recoil
		if (Vehicle != nullptr && Mount->MountNum > 0)
		{
			RecoilNum = Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].RecoilNum[Mount->MountNum-1]-1;
			if (RecoilNum >= 0)
			{
				Vehicle->StateAnimation.RecoilStartTime[RecoilNum] = (SINT32)((LastFireTime) * 1000.0);
				Vehicle->StateAnimation.RecoilTime[RecoilNum] = Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].RecoilTime[Mount->MountNum-1];
			}
		}
	}

	return MountFired;
}

/************************************************************************************************
	_UpdateMountWeight
*************************************************************************************************/

static void _UpdateMountWeight(mlWeaponMount* Mount, mlComponent* Component)
{
	UINT32 i;
	UINT32 Accumulator;
	FLOAT64 Mass,TempMass;
	UINT32 NumRounds;

    piMATRIX Matrix1;

	// compute mount weight
	Mass = 0;
	for (i=0;i<Mount->NumberOfWeaponsInMount;i++)
	{
		if (Mount->Weapons[i].WeaponType != nullptr)
		{
			Mass += Mount->Weapons[i].WeaponType->Mass;
		}

		if (Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] > 0)
		{
			if (Mount->Weapons[i].NumRoundsInBeltSequence[Mount->Weapons[i].AmmoTypeInUse] == 0)
			{
				if (Mount->Weapons[i].BeltSequence[Mount->Weapons[i].AmmoTypeInUse][0] != nullptr)
				{
					Mass += Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] *
							Mount->Weapons[i].BeltSequence[Mount->Weapons[i].AmmoTypeInUse][0]->AmmoWeight;
				}
				else
				{
					if (Mount->Weapons[i].WeaponType->AmmoType[0] != nullptr)
					{
						Mass += Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] *
									Mount->Weapons[i].WeaponType->AmmoType[0]->AmmoWeight;
					}
				}
			}
			else
			{
				NumRounds = Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse];
				Accumulator = 0;

				TempMass = 0.0;
				while (Accumulator < Mount->Weapons[i].NumRoundsInBeltSequence[Mount->Weapons[i].AmmoTypeInUse])
				{
					if (Mount->Weapons[i].BeltSequence[Accumulator] != nullptr)
					{
						TempMass += Mount->Weapons[i].BeltSequence[Mount->Weapons[i].AmmoTypeInUse][Accumulator]->AmmoWeight;
					}

					Accumulator++;
				}

				Mass += TempMass * (NumRounds / Mount->Weapons[i].NumRoundsInBeltSequence[Mount->Weapons[i].AmmoTypeInUse]);
				NumRounds %= Mount->Weapons[i].NumRoundsInBeltSequence[Mount->Weapons[i].AmmoTypeInUse];

				Accumulator = 0;
				while (NumRounds > 0)
				{
					if (Mount->Weapons[i].BeltSequence[Mount->Weapons[i].AmmoTypeInUse][Accumulator] != nullptr)
					{
						Mass += Mount->Weapons[i].BeltSequence[Mount->Weapons[i].AmmoTypeInUse][Accumulator]->AmmoWeight;
					}

					Accumulator++;
					NumRounds--;
				}

			}
		}
	}

	Component->Mass = Mass;
	// set the position of the mass at weapon mount DOF point
	if (!Component->OverridePos)
	{
		Matrix1 = Mount->MountObject->dof->dofmatrix2;
		math3dMatrixMultiply(&Matrix1,&Matrix1,&(Mount->MountObject->matint));
		mlUtilityGetModelPositionFromObjectMatrix(&Component->OriginalRelativePos,&Matrix1);
	}
}

/************************************************************************************************
	_UpdateAmmoCounteight
*************************************************************************************************/

static void _UpdateAmmoCount(mlWeaponMount* Mount)
{
	UINT32 i, NumRounds = 0;

	for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
		NumRounds += Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse];

	mlUpdateAmmoCounter(NumRounds, Mount->MountNum-1);
}

/************************************************************************************************
	_UpdateBombDrag
*************************************************************************************************/

static void _UpdateBombDrag(mlWeaponMount* Mount, vehVehicle* Vehicle, FLOAT64VECTOR* F)
{
	UINT32 i, NumRounds = 0;
	UINT32 MachIdx;
	FLOAT64 Alt, Speed, Mach, Cd, Density, Drag;
	weapAmmoType const* AmmoType;

	Alt = Vehicle->ModelData->VehiclePos.z;

	// Get mach number for drag
	Speed = mathExactVectorLength(&Vehicle->ModelData->VehicleRealVel);
	mlGetAltSpeedOfSound(&Mach, Alt);
	Mach = Speed / Mach;
	MachIdx = (UINT32)(floor(Mach / G1_CHART_MACH_DELTA));

	// Drag Coefficient
	if(MachIdx + 1 < G1_CHART_NUM_DIVISIONS)
	{
		FLOAT64 Delta = (Mach / G1_CHART_MACH_DELTA) - MachIdx;
		mlInterpolate(&Cd ,G1MachChart[MachIdx],G1MachChart[MachIdx+1], Delta);
	}else
	{
		Cd = G1MachChart[G1_CHART_NUM_DIVISIONS - 1];
	}

	mlGetAltDensity(&Density, Alt);

	for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
	{
		NumRounds = Mount->Weapons[i].NumRoundsAmmoLeft[0]; // assume bombs will only have one belt
		if(NumRounds < 1) continue;

		AmmoType = Mount->Weapons[i].WeaponType->AmmoType[0]; // and one ammo type
		Drag = Density * Cd * Speed / (AmmoType->BallisticCoefficient * 1790.0f);

		F->x += -AmmoType->ProjectileWeight * NumRounds * Drag * Vehicle->ModelData->VehicleRealVel.x;
		F->y += -AmmoType->ProjectileWeight * NumRounds * Drag * Vehicle->ModelData->VehicleRealVel.y;
		F->z += -AmmoType->ProjectileWeight * NumRounds * Drag * Vehicle->ModelData->VehicleRealVel.z;
	}
}

/************************************************************************************************
	_JettisonMount
*************************************************************************************************/

static void _JettisonMount(mlWeaponMount* Mount, mlVehicleData* VehicleData, FLOAT64 DeltaTime,
							FLOAT64 CurrentTime, OBJECT* PlaneObject, FLOAT64VECTOR* Force)
{
	UINT32 i, j;
	UINT32 NumRounds;
	FLOAT64 LastFireTime;

	// check  position
	for (i = 0; i < Mount->NumberOfWeaponsInMount; i++)
	{
		NumRounds = Mount->Weapons[i].NumRoundsAmmoLeft[0]; // assume bombs will only have one belt
		for(j = 0; j < NumRounds; j++)
		{
			Mount->Weapons[i].LastFireTime = 0; // To get around reload time check
			_FireWeaponInMount(Mount, VehicleData,  DeltaTime,  CurrentTime,
								 PlaneObject, Force, &LastFireTime, i, VehicleData->VehicleMasterPtr);
		}
	}
}

/************************************************************************************************
	_FireWeaponInMount
*************************************************************************************************/

static void _FireWeaponInMount(mlWeaponMount* Mount, mlVehicleData* VehicleData, FLOAT64 DeltaTime, FLOAT64 CurrentTime,
								OBJECT* PlaneObject, FLOAT64VECTOR* TempForce, FLOAT64* LastFireTime, UINT32 i, void* FireVehicle)
{
	FLOAT64VECTOR FirePnt, FireVel, RelativePnt;
	piMATRIX Matrix1;
	UINT32 Flags;
	SINT32 RoundsLeft;
	vehVehicle* lpVeh = (vehVehicle*)FireVehicle;

    memset(&FirePnt, 0, sizeof(FLOAT64VECTOR));
    memset(&RelativePnt, 0, sizeof(FLOAT64VECTOR));
    memset(&FireVel, 0, sizeof(FLOAT64VECTOR));

	if(!Mount->WeaponFirePoints[i] || !Mount->WeaponFirePoints[i]->dof) return;

	_GetFireAttPosVel(Mount, i, VehicleData, &Matrix1, &RelativePnt, &FirePnt, &FireVel);

	Flags = 0;
	if (Mount->CollideType == ML_WEAPON_COLLIDETYPE_FRONT60DEG)
		Flags |= WEAP_AMMOFLAG_USE_MED60DEG_COLLIDELIST;
	if (Mount->CollideType == ML_WEAPON_COLLIDETYPE_SHORTOMNI)
		Flags |= WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST;
	if (Mount->CollideType == ML_WEAPON_COLLIDETYPE_MEDOMNI)
		Flags |= WEAP_AMMOFLAG_USE_MEDCOLLIDELIST;

	// player cannot usually be hit by own weapons
	Flags |= WEAP_AMMOFLAG_CANTHITPLAYER;

    // Need this for correct positioning of muzzleflash. This switch will be added to as
    // more vehicles are moved to new muzzle flashes which use the WeaponFirePoints,
    // instead of the PlaneObject.
    if(lpVeh)
    {
        if(lpVeh->VehicleType)
        {
            switch(lpVeh->VehicleType->Type)
            {
            case VEH_TYPE_AIR:
            case VEH_TYPE_GROUND:
            case VEH_TYPE_SEA:
                {
                    PlaneObject = Mount->WeaponFirePoints[i];
                    break;
                }
            }
        }
    }

	RoundsLeft = Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse];

	weapFireWeapon(TempForce,
                   &(Mount->Weapons[i]),
				   &FirePnt,
                   &FireVel,
                   &Matrix1,
                   DeltaTime / 1000.0,
                   CurrentTime / 1000.0,
				   1,
                   0,
                   Flags,
                   PlaneObject,
                   &RelativePnt,
                   LastFireTime,
                   FireVehicle);

	// handle hiding weapon object upon last round fired
	if (Mount->Weapons[i].NumRoundsAmmoLeft[Mount->Weapons[i].AmmoTypeInUse] == 0 && RoundsLeft != 0)
		ogrpHideObjectAndSubtree(Mount->WeaponConnectPoints[i]);
}

void mlWeaponGetFireAttPosVel(mlWeaponMount* Mount, int i, mlVehicleData* VehicleData, piMATRIX* FireAtt,
								FLOAT64VECTOR* RelativePnt, FLOAT64VECTOR* FirePnt,  FLOAT64VECTOR* FireVel)
{
	_GetFireAttPosVel(Mount, i, VehicleData, FireAtt, RelativePnt, FirePnt, FireVel);
}

/************************************************************************************************
	_GetFireAttPosVel
*************************************************************************************************/

static void _GetFireAttPosVel(mlWeaponMount* Mount, int i, mlVehicleData* VehicleData, piMATRIX* Matrix1,
							FLOAT64VECTOR* RelativePnt, FLOAT64VECTOR* FirePnt,  FLOAT64VECTOR* FireVel)
{
	piMATRIX Matrix2;
	piVECTOR TempPiVec;
	FLOAT64VECTOR TempVec1, TempVec2;

	// compute fire point
	// first get the position relative to the vehicle
	*Matrix1 = IdentityMatrix;
	Matrix1->_41 = Mount->WeaponFirePoints[i]->dof->dofmatrix2._41;
	Matrix1->_42 = Mount->WeaponFirePoints[i]->dof->dofmatrix2._42;
	Matrix1->_43 = Mount->WeaponFirePoints[i]->dof->dofmatrix2._43;
	Matrix2 = Mount->WeaponFirePoints[i]->matint;
	math3dMatrixMultiply(Matrix1,Matrix1,&Matrix2);

	mlUtilityGetModelPositionFromObjectMatrix(FirePnt,Matrix1);
	TempVec1 = *RelativePnt = *FirePnt;

	// now apply the attitude of the vehicle
	mathVectorMultiply64(FirePnt, FirePnt, &VehicleData->VehicleAttMat);

	// finally add the vehicles overall position
	mathAddVectors(FirePnt, FirePnt, &VehicleData->AdjustedForCGPos);

	// compute fire velocity
	// first find the attitude rotational velocity component
	TempPiVec.x = -(FLOAT32)(VehicleData->VehicleRealAttVel.Pitch * 0.001);
	TempPiVec.y = -(FLOAT32)(VehicleData->VehicleRealAttVel.Roll * 0.001);
	TempPiVec.z = (FLOAT32)(VehicleData->VehicleRealAttVel.Yaw * 0.001);
	mathEul_ToMatrix(Matrix1,EulOrdXYZs,&TempPiVec);
	mathVectorMultiply64(&TempVec2,&TempVec1,Matrix1);
	FireVel->x = (TempVec2.x - TempVec1.x) * (1/0.001);
	FireVel->y = (TempVec2.y - TempVec1.y) * (1/0.001);
	FireVel->z = (TempVec2.z - TempVec1.z) * (1/0.001);
	// then add the vehicle's overall velocity
	mathAddVectors(FireVel, FireVel, &VehicleData->VehicleRealVel);

	// now fire the weapon
	*Matrix1 = Mount->WeaponFirePoints[i]->dof->dofmatrix2;
	math3dMatrixMultiply(Matrix1,Matrix1,&(Mount->WeaponFirePoints[i]->child->matint));
	Matrix1->_13 = -Matrix1->_13;
	Matrix1->_23 = -Matrix1->_23;
	Matrix1->_33 = -Matrix1->_33;
	Matrix1->_43 = -Matrix1->_43;
	math3dMatrixMultiply(Matrix1,Matrix1,&VehicleData->VehicleAttMat);

	Matrix1->_41 = Matrix1->_42 = Matrix1->_43 = 0.0f;
}

/************************************************************************************************
	_IsMountBlocked
*************************************************************************************************/

static LOGICAL _IsMountBlocked(mlWeaponMount* Mount, int i, vehVehicle* Vehicle)
{
//	LOGICAL IsBlocked = FALSE;
	FLOAT64VECTOR RelPnt, FirePnt, FireVel;
	piMATRIX FireAtt;
	FLOAT64VECTOR CollideRay = {0,1000,0};
	FASTCOLLIDE CollideList[32], *CollideListPtr;
	UINT32 NumCollisions;
	FLOAT64VECTOR WorldOriginOffset;
	piVECTOR start, end;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	_GetFireAttPosVel(Mount, i, Vehicle->ModelData, &FireAtt, &RelPnt, &FirePnt, &FireVel);

	// Fire out a ray and see if it collides with our own vehicle
	mathVectorMultiply64(&CollideRay, &CollideRay, &FireAtt);

	start.x = (FLOAT32)(FirePnt.x - WorldOriginOffset.x);
	start.y = (FLOAT32)(FirePnt.y - WorldOriginOffset.y);
	start.z = (FLOAT32)(FirePnt.z - WorldOriginOffset.z);
	end.x = start.x + (FLOAT32)CollideRay.x;
	end.y = start.y + (FLOAT32)CollideRay.y;
	end.z = start.z + (FLOAT32)CollideRay.z;

	// randomly flip z's <sigh>
	start.z = -start.z;
	end.z = -end.z;

	CollideListPtr = CollideList;
	NumCollisions = 32;

	weapCollideRoundWithVehicles(&CollideListPtr, &NumCollisions,
									&start, &end, frametime/1000.0, frametime/1000.0,
									TRUE, TRUE, &WorldOriginOffset, FALSE, 0);
	NumCollisions = 32 - NumCollisions;

	return NumCollisions;
}
