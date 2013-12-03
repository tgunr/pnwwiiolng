#include <syscfg.h> // Path is project specific.
#include <math.h>

#include "include/vars.h"
#include "pi/pi.h"
#include "math/Matrixf.h"

#include "fio/fio.h"
#include "ob/obmain.h"
#include "ob/obpool.h"
#include "ob/obcd.h"
#include "ogrp/ogrptype.h"
#include "list/list.h"
#include "trn/trncd.h"
#include "trn/trnmain.h"
#include "trn/trnautogun.h"
#include "mem/mem.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "ml/mlgauges.h"
#include "ml/mlutility.h"
#include "ml/mlweapons.h"
#include "ml/mlAtmo.h"
#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "fx/fxMgr.h"
#include "fx/fxSnd.h"
#include "weap/weapammo.h"
#include "weap/weapons.h"
#include "weap/weapList.h"
#include "strt/strtDTL.h"
#include "fp/fpFPS.h"
#include "draw/draw3d.h"
#include "gr2/gr2Anim.h"
#include "ob/obMgr.h"
#include "ui/uiChat.h"
#include "tut/TutorialManager.h"
#include "sdm/StratDB.h"
#include "miss/missionUtils.h"

#define _MAX_AUTOGUN_TYPES 32
#define _MAX_GROUPS 128
#define _MAX_IN_A_GROUP 16
#define _MAX_AG_MODELLED 32
#define _TARGET_SELECTION_TIME 1000
#define _TRACKING_SELECTION_TIME 10000
#define TIME_INCREMENT 0.05

#ifdef DEBUG_WW2
// For debugging ai guns only
#define HAVE_AI_GUNS_ALWAYS_FIRE 0
#endif

// Just to be safe...
#ifndef DEBUG_WW2
#if HAVE_AI_GUNS_ALWAYS_FIRE
#error HAVE_AI_GUNS_ALWAYS_FIRE Defined!!!!
#endif
#endif

static WEAPON_SPEC _AutoGunTargetGroups[_MAX_GROUPS][_MAX_IN_A_GROUP];
trnAutoGunType _AutoGunType[_MAX_AUTOGUN_TYPES];
trnAutoGun _AutoGunHead;

static LOGICAL _MoveGun(trnAutoGun * AG,piVECTOR* AimAtt);
static LOGICAL _SetupElevAndTraverse(OBJECT * Object, piMATRIX * TravMat, piMATRIX * ElevMat);
static LOGICAL _ReadAndSetupAutogunTypes(void);
static LOGICAL _SetupWeapon(weapWeapon* Weapon,SINT8 * WeaponName, SINT8 * BeltFile);
static LOGICAL _UpdateWeapon(trnAutoGun * AG);
static LOGICAL _AimGunAtTargetPnt(trnAutoGun * AG,FLOAT64VECTOR * AimPnt);
static LOGICAL _CalculateActualAimPnt(trnAutoGun * AG,FLOAT64VECTOR * AimPnt);
static LOGICAL _UpdateFireStatus(trnAutoGun * AG);
static LOGICAL _DetermineTarget(trnAutoGun * AG);
static LOGICAL _DetermineOptimumAimPnt(trnAutoGun * AG, FLOAT64VECTOR * AimPnt);
static LOGICAL _IsVehicleInLineOfSight(trnAutoGun * AG,vehVehicle* Vehicle,FLOAT32* Dist32,LOGICAL AddFudge);
static LOGICAL _FireInstantRoundAndGetDisplacement(trnAutoGun* AG,piVECTOR * AimVec,FLOAT64VECTOR * AimPnt,piVECTOR * NewDisplacement);

static bool _updateAI = true;

LOGICAL trnInitAutogunSystem(void)
{
	_ReadAndSetupAutogunTypes();

    _updateAI = Online ? true : false;

	return TRUE;
}

static LOGICAL _ReadAndSetupAutogunTypes(void)
{
    trnAutoGunType * AGT = NULL;
    UINT32 Country = 0;
    UINT32 i = 0;
    SINT32 AutoGunFile = 0;
    SCHAR* BufferPtr = NULL;
    SCHAR* Ptr1 = NULL;
    LOGICAL ReadingAutoguns = FALSE;
    SINT32 GroupNum = 0;
    SINT32 ElementNum = 0;
    WEAPON_SPEC Spec = NULL_WEAP_SPEC;
    static SCHAR Buffer[1024] = {0};
    static SINT8 TempStr1[64] = {0};

	for (i=0;i<_MAX_AUTOGUN_TYPES;i++)
	{
		_AutoGunType[i].Type = i;
	}

	AutoGunFile = fioOpen("vehicles\\autogun.txt");
	if (AutoGunFile == -1)
	{
		Write_Debug("Unable to read in autogun file");
		return FALSE;
	}

	ReadingAutoguns = FALSE;
	GroupNum = -1;

	memset(&(_AutoGunTargetGroups[0][0]),0,sizeof(WEAPON_SPEC) * _MAX_GROUPS * _MAX_IN_A_GROUP);

	while (fioGetString(AutoGunFile,Buffer,1023) != 0)
	{
		if (Buffer[0] == '#' || Buffer[0] == '/')
		{
			continue;
		}

		if (strlen(Buffer) < 2)
		{
			continue;
		}

		if (ReadingAutoguns != TRUE)
		{
			if (strcmp(Buffer,"autoguns") == 0)
			{
				ReadingAutoguns = TRUE;
				continue;
			}

			if (strncmp(Buffer,"group ",6) == 0)
			{
				sscanf(Buffer,"group %d",&GroupNum);
				ElementNum = 0;
				continue;
			}

			if (GroupNum >= 0 && GroupNum < _MAX_GROUPS && ElementNum < _MAX_IN_A_GROUP)
			{
				BufferPtr = strtok(Buffer,",");
				if (BufferPtr == NULL)
				{
					continue;
				}
				strcpy(TempStr1,BufferPtr);
				BufferPtr = strtok(NULL,",");
				if (BufferPtr == NULL)
				{
					continue;
				}
				strcat(TempStr1,BufferPtr);
				strcat(TempStr1,".0");

				Spec = weapCreateSpec(TempStr1);
				if (Spec.wCategory == 0 && Spec.wClass == 0)
				{
					continue;
				}

				_AutoGunTargetGroups[GroupNum][ElementNum] = Spec;
				ElementNum++;
			}
		}
		else
		{
			BufferPtr = strtok(Buffer,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			if (atoi(BufferPtr) == 0)
			{
				continue;
			}
			AGT = &_AutoGunType[atoi(BufferPtr)];


			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			if (atoi(BufferPtr) == 0)
			{
				continue;
			}
			Country = atoi(BufferPtr);


			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			while (*BufferPtr == ' ')
			{
				BufferPtr++;
			}
			while ((Ptr1 = strrchr(BufferPtr,' ')) != NULL)
			{
				*Ptr1 = '\0';
			}
			strcpy(AGT->ObjectName[Country],BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			while (*BufferPtr == ' ')
			{
				BufferPtr++;
			}
			while ((Ptr1 = strrchr(BufferPtr,' ')) != NULL)
			{
				*Ptr1 = '\0';
			}
			strcpy(AGT->WeaponName[Country],BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			while (*BufferPtr == ' ')
			{
				BufferPtr++;
			}
			while ((Ptr1 = strrchr(BufferPtr,' ')) != NULL)
			{
				*Ptr1 = '\0';
			}
			strcpy(AGT->BeltFile[Country],BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->MaxConeAngle[Country] = mlDEG_TO_RAD((FLOAT32)atof(BufferPtr));

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->MinConeAngle[Country] = mlDEG_TO_RAD((FLOAT32)atof(BufferPtr));

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->TimeToMinCone[Country] = (FLOAT32)atof(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->AngleOffMult[Country] = (FLOAT32)atof(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->DegreesOffMult[Country] = (FLOAT32)atof(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->VisAdditionalAngle[Country] = mlDEG_TO_RAD((FLOAT32)atof(BufferPtr));

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->MaxEngagementRange[Country] = AGT->MaxEffectiveRange[Country] = (FLOAT32)atof(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->EngageVehiclePriority[Country][0] = atoi(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->EngageVehiclePriority[Country][1] = atoi(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->EngageVehiclePriority[Country][2] = atoi(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->GunFiresContinuously[Country] = atoi(BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			while (*BufferPtr == ' ')
			{
				BufferPtr++;
			}
			while ((Ptr1 = strrchr(BufferPtr,' ')) != NULL)
			{
				*Ptr1 = '\0';
			}
			strcpy(AGT->FireSound[Country],BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			while (*BufferPtr == ' ')
			{
				BufferPtr++;
			}
			while ((Ptr1 = strrchr(BufferPtr,' ')) != NULL)
			{
				*Ptr1 = '\0';
			}
			strcpy(AGT->ReloadSound[Country],BufferPtr);

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->ElevateRate[Country] = mlDEG_TO_RAD((FLOAT32)atof(BufferPtr));

			BufferPtr = strtok(NULL,",");
			if (BufferPtr == NULL)
			{
				continue;
			}
			AGT->TraverseRate[Country] = mlDEG_TO_RAD((FLOAT32)atof(BufferPtr));
		}
	}

	fioClose(AutoGunFile);

	return TRUE;
}


LOGICAL trnKillAutogunSystem(void)
{
	// note: if there are any autoguns left to kill, we've got a problem
	if (_AutoGunHead.next != NULL)
	{
		Write_Debug("******************");
		Write_Debug("Autoguns remaining after trnldrkill!!!!!");
		Write_Debug("******************");
	}

	return TRUE;
}

LOGICAL trnKillAutogun(trnAutoGun** ppAutoGun)
{
trnAutoGun* pAutoGun = NULL;

	pAutoGun = *ppAutoGun;

	if (pAutoGun == NULL)
	{
		return FALSE;
	}

	if (pAutoGun->FireSoundId > 0)
	{
		fxSndStopSoundDelayed(pAutoGun->FireSoundId, 1);
		pAutoGun->FireSoundId = 0;
	}

	if (pAutoGun->TO != NULL)
	{
		pAutoGun->TO->TrnAutoGun = NULL;
	}

	if (pAutoGun->Object != NULL)
	{
		obpoolFreeObjectCallback(pAutoGun->Object);
	}

	listDeleteItem((LISTHEADER*) (pAutoGun));

	delete pAutoGun->Weapon;
	memFree(pAutoGun);

	return TRUE;
}

LOGICAL trnAddAutoGun(trnCDTerrainObject * TO)
{
	if (TO == NULL)
	{
		return FALSE;
	}

	trnAutoGun * AG = NULL;
	FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};
	piMATRIX AttMat = IdentityMatrix;
	FLOAT64VECTOR Pos = {0, 0, 0};
	OBJECT * PlaceObject = NULL;
	FLOAT32 Num1 = 0;
	FLOAT32 Num2 = 0;
	FLOAT32 Num3 = 0;
	BUILDING* bld = NULL;
	static SINT8 ObjectName[64];
	static SINT8 Buffer[128];

    memset(Buffer, 0, sizeof(SINT8) * 128);
    memset(ObjectName, 0, sizeof(SINT8) * 32);

	static UINT32 Counter = 0;

	// find somewhere to put the autogun
	ogrpParseDOFParameters(TO->TerrainObject,(UCHAR*)(&mlDOFVarNameTable[0][0]),ML_DOF_MAXNUM,32);
	ogrpFindSubObjectByDOFVar(&PlaceObject,TO->TerrainObject,ML_DOF_AG_LOCATE);
	if (PlaceObject == NULL)
	{
		Write_Debug("Unable to find placement point in base object %s",
					TO->TerrainMasterObject->name);
		return FALSE;
	}

	// initialize an autogun
	AG = (trnAutoGun*)memAlloc(sizeof(trnAutoGun));
	if (AG == NULL)
	{
		Write_Debug("Unable to allocate memory for autogun");
		return FALSE;
	}
	memset(AG,0,sizeof(trnAutoGun));

	AG->Weapon = new weapWeapon;
	if (AG->Weapon == NULL)
	{
		Write_Debug("Unable to allocate weapon space memory for autogun");
		memFree(AG);
		return FALSE;
	}

	if (trnFindStratBuildingByTO(&bld, TO) == FALSE)
	{
		Write_Debug("Unable to determine autogun type from base object");
		delete AG->Weapon;
		memFree(AG);
		return FALSE;
	}

	AG->Type = &_AutoGunType[bld->subType];
	if (Online == 0)
	{
        FULL_OBJ_ID foi;

        populateFullObjID(&foi, TO->SupercellX(), TO->SupercellY(), bld->oid);
        StratDB::StratObject* so = StratDB::FindStratObject(foi);

        if(so)
        {
            so = so->ParentStratObject();

            if(so)
                AG->Country = so->Country();
            else
                AG->Country = WWII_ENGLAND;
        }
        else
        {
            AG->Country = WWII_ENGLAND;
        }
	}
	else
	{
		AG->Country = TO->Country;
	}

	if (_SetupWeapon((weapWeapon*)(AG->Weapon),
						AG->Type->WeaponName[AG->Country],
						AG->Type->BeltFile[AG->Country]) == FALSE)
	{
		delete AG->Weapon;
		memFree(AG);
		return FALSE;
	}

	TO->TrnAutoGun = AG;

	listAddItem((LISTHEADER*)&_AutoGunHead,(LISTHEADER*)AG);

	// get and setup object
	strcpy(ObjectName,AG->Type->ObjectName[AG->Country]);

    if (strlen(ObjectName) == 0 || !obmgrCreateModelInstance(ObjectName))
	{
		listDeleteItem((LISTHEADER*)AG);
		delete AG->Weapon;
		memFree(AG);
		TO->TrnAutoGun = NULL;
		return FALSE;
	}

	AG->Object = obmainFindNamedObject(ObjectName);
	obmainSetFlag2InObjectTree(AG->Object,OBJECT_SPECIAL_LOD);
	if (AG->Object == NULL)
	{
		listDeleteItem((LISTHEADER*)AG);
		delete AG->Weapon;
		memFree(AG);
		TO->TrnAutoGun = NULL;
		return FALSE;
	}

	sprintf(AG->Object->name,"%s%d",ObjectName,Counter);
	ogrpParseDOFParameters(AG->Object,(UCHAR*)(&mlDOFVarNameTable[0][0]),ML_DOF_MAXNUM,32);

	ogrpFindSubObjectByDOFVar(&(AG->FirePnt),AG->Object,ML_DOF_AG_FIRE);
	if (AG->FirePnt == NULL)
	{
		Write_Debug("Unable to find fire point in autogun %s",AG->Object->name);
	}
	ogrpFindSubObjectByDOFVar(&(AG->ViewPnt),AG->Object,ML_DOF_AG_VIEW);

    if (AG->ViewPnt == NULL)
	{
		Write_Debug("[trnAddAutoGun] ** FAILED ** Unable to find view point in autogun %s",AG->Object->name);
	}

	// ok now set up position of autogun in the world
	trnGetWorldOriginOffset(&WorldOffsetOrigin);

	AttMat = PlaceObject->dof->dofmatrix2;
	math3dMatrixMultiply(&AttMat,&AttMat,&PlaceObject->child->mat);
	AttMat._13 = -AttMat._13;
	AttMat._23 = -AttMat._23;
	AttMat._33 = -AttMat._33;
	AttMat._43 = -AttMat._43;
	Pos.x = AttMat._41;
	Pos.y = AttMat._42;
	Pos.z = AttMat._43;
	AttMat._41 = AttMat._42 = AttMat._43 = 0.0f;

	Pos.x += WorldOffsetOrigin.x;
	Pos.y += WorldOffsetOrigin.y;
	Pos.z += WorldOffsetOrigin.z;

	// adjust height based on place mode
	Pos.z += -(PlaceObject->child->MutablePositions()[0].z - PlaceObject->child->ImmutablePositions()[0].z);

    if(AG->ViewPnt && AG->ViewPnt->dof)
    {
	    AG->EyePos.x = AG->ViewPnt->dof->dofmatrix2._41;
	    AG->EyePos.y = AG->ViewPnt->dof->dofmatrix2._42;
	    AG->EyePos.z = -AG->ViewPnt->dof->dofmatrix2._43;
    }
/*
    static int count = 0;
    if(bld->subType == 8)
    {
        if(count++ == 5)
        {
            int stop = 0;
        }
        else
        {
		    listDeleteItem((LISTHEADER*)AG);
		    delete AG->Weapon;
		    memFree(AG);
		    TO->TrnAutoGun = NULL;
            return FALSE;
        }
    }
    else
    {
	    listDeleteItem((LISTHEADER*)AG);
	    delete AG->Weapon;
	    memFree(AG);
	    TO->TrnAutoGun = NULL;
        return FALSE;
    }
*/
	AG->Pos = Pos;

	// extract field of fire
	sscanf(PlaceObject->comment,"%s\n%f,%f,%f",
			Buffer,
			&Num1,&Num2,&Num3);

	AG->MaxElevate = mlDEG_TO_RAD(Num2);
	AG->MinElevate = mlDEG_TO_RAD(Num3);
	AG->LeftRightAngle = mlDEG_TO_RAD(Num1);

    Vec3d from(PlaceObject->dof->dofmatrix2._12, PlaceObject->dof->dofmatrix2._22, PlaceObject->dof->dofmatrix2._32);

    // Shearing screws this up. Force to not shear in this case.
    AttMat._13 = 0.0f;
    AttMat._23 = 0.0f;

    Vec3d to = from * Matrixf(AttMat);

    Matrixf rot;

    rot.makeRotate(from, to);

    rot.get(AG->rotateMatrix);

    AG->MinTraverse = AG->MaxTraverse = (FLOAT32)atan2(to.x(), to.y());

    AG->MinTraverse -= mlDEG_TO_RAD(Num1);
	AG->MaxTraverse += mlDEG_TO_RAD(Num1);

    AG->CenterTraverse = (AG->MinTraverse + AG->MaxTraverse) * 0.5f;

	obmainClearFlagsInThisTree(AG->Object, OBJECT_VISIBLE);

	AG->CollisionObject = NULL;
	trnFindCollisionObject(&(AG->CollisionObject), AG->Object);
	if (AG->CollisionObject == NULL)
	{
		AG->CollisionObject = AG->Object;
	}

	AG->Object->child->lpspare = (void*)TO;

	AG->TO = TO;

	return TRUE;
}

#include "rn/rnmain.h"
void trnautogunToggleAI()
{
#ifdef BETA_RELEASE
    if(!Online)
    {
        _updateAI = !_updateAI;
        uiChatPrintLine(L"AI %s", _updateAI ? L"ON" : L"OFF");
    }
#endif
}

LOGICAL trnUpdateAutoGuns(void)
{
    trnAutoGun* AG = NULL;
    trnAutoGun* AGSaved = NULL;
    trnAutoGun* AGTempPtr = NULL;

    FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};
    FLOAT64VECTOR ViewPnt = {0, 0, 0};
    FLOAT64VECTOR TempVec = {0, 0, 0};
    FLOAT64VECTOR AimPnt = {0, 0, 0};

    piMATRIX TempMat = IdentityMatrix;

    piVECTOR Att = {0, 0, 0};

    UINT32 Count = 0;

	if(!_updateAI || TUT_ACTIVE)
    {
#if HAVE_AI_GUNS_ALWAYS_FIRE
        if(!_updateAI)
        {
            for(AG = _AutoGunHead.next; AG != NULL; AG = AG->next)
            {
                if(AG->FireSoundId > 0)
                    fxSndStopSound(AG->FireSoundId);

                AG->GunIsFiring = FALSE;
            }
        }
#endif
        return TRUE;
    }

	trnGetWorldOriginOffset(&WorldOffsetOrigin);


	ViewPnt.x = (-playpos.x + WorldOffsetOrigin.x);
	ViewPnt.y = (-playpos.y + WorldOffsetOrigin.y);
	ViewPnt.z = (-playpos.z + WorldOffsetOrigin.z);

	AG = _AutoGunHead.next;

	_AutoGunHead.DistSqr = 0.0f;

	// recalculate distance squared for all autoguns
	// and resort
	if (AG != NULL)
	{
		mathSubVectors(&TempVec,&ViewPnt,&AG->Pos);

		AG->DistSqr = (FLOAT32)(TempVec.x * TempVec.x + TempVec.y * TempVec.y);

		AG = AG->next;
	}

	while (AG != NULL)
	{
		AGSaved = AG->next;

		mathSubVectors(&TempVec,&ViewPnt,&AG->Pos);

		AG->DistSqr = (FLOAT32)(TempVec.x * TempVec.x + TempVec.y * TempVec.y);

		while (AG->prev->DistSqr > AG->DistSqr &&
				AG->prev != &_AutoGunHead)
		{
			AGTempPtr = AG->prev->prev;
			if (AGTempPtr != NULL)
			{
				listDeleteItem((LISTHEADER*)AG);
				listAddItem((LISTHEADER*)AGTempPtr,(LISTHEADER*)AG);
			}
		}

		AG = AGSaved;
	}


	AG = _AutoGunHead.next;
	Count = 0;
	while (AG != NULL)
	{
		if (AG->AGDeadOrHidden != 0)
		{
			AG = AG->next;
			continue;
		}

		if (Count >= _MAX_AG_MODELLED)
		{
			if (AG->AGRunModel == 1)
			{
				obmainClearFlagsInThisTree(AG->Object, OBJECT_VISIBLE);

				AG->AGRunModel = 0;
			}

			AG = AG->next;
			continue;
		}

		Count++;
		if (AG->AGRunModel == 0)
		{
			obmainSetFlagsInThisTree(AG->Object, OBJECT_VISIBLE);

			AG->AGRunModel = 1;
		}

        AimPnt.x = 0.0f;
        AimPnt.y = 0.0f;
        AimPnt.z = 0.0f;

		_DetermineTarget(AG);
		_DetermineOptimumAimPnt(AG,&AimPnt);
		_UpdateFireStatus(AG);
		_CalculateActualAimPnt(AG,&AimPnt);
		_AimGunAtTargetPnt(AG,&AimPnt);

        TempMat = AG->rotateMatrix;

		TempMat._41 = (FLOAT32)(AG->Pos.x - WorldOffsetOrigin.x);
		TempMat._42 = (FLOAT32)(AG->Pos.y - WorldOffsetOrigin.y);
		TempMat._43 = -(FLOAT32)(AG->Pos.z - WorldOffsetOrigin.z);

		AG->Object->matint = IdentityMatrix;
        // I guess we do this every frame in case the world origin offset moves???
		ogrpMoveAndRotateSubObject(AG->Object,&TempMat,&TempMat);

		_UpdateWeapon(AG);

		AG->LastGunUpdateTime = frametime;

		AG = AG->next;
	}

	return TRUE;
}

static LOGICAL _UpdateWeapon(trnAutoGun * AG)
{
	weapWeapon * Weapon = NULL;
	piMATRIX AttMat = IdentityMatrix;
    FLOAT64VECTOR TempForce = {0, 0, 0};
	FLOAT64VECTOR Pos = {0, 0, 0};
	FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};
	FLOAT64VECTOR FireVel = {0, 0, 0};
	UINT32 GhostFire = 0;
	UINT32 Flags = 0;
	UINT32 LastReloading = 0;
	FLOAT64 FireDelta = 0;
	UINT32 CanCauseDamage = 0;

	Weapon = (weapWeapon*)AG->Weapon;
	if(Weapon == NULL) return FALSE;

	// trick used to guarantee always ammo to fire, yet reload sequences occur
	if (Weapon->NumRoundsAmmoLeft[0] <= (SINT32)Weapon->WeaponType->NumRoundsInClip)
	{
		Weapon->NumRoundsAmmoLeft[0] += Weapon->WeaponType->NumRoundsInClip;
	}

	LastReloading = weapIsWeaponReloading(Weapon,frametime / 1000.0);
	if (AG->GunIsFiring == TRUE)
	{
		// fire pnt
		trnGetWorldOriginOffset(&WorldOffsetOrigin);

		AttMat = AG->FirePnt->dof->dofmatrix2;
		math3dMatrixMultiply(&AttMat,&AttMat,&AG->FirePnt->child->mat);
		AttMat._13 = -AttMat._13;
		AttMat._23 = -AttMat._23;
		AttMat._33 = -AttMat._33;
		AttMat._43 = -AttMat._43;
		Pos.x = AttMat._41;
		Pos.y = AttMat._42;
		Pos.z = AttMat._43;
		AttMat._41 = AttMat._42 = AttMat._43 = 0.0f;

		Pos.x += WorldOffsetOrigin.x;
		Pos.y += WorldOffsetOrigin.y;
		Pos.z += WorldOffsetOrigin.z;

		FireVel.x = FireVel.y = FireVel.z = 0.0f;

		// now fire the weapon
		Flags = WEAP_AMMOFLAG_USE_MEDCOLLIDELIST | WEAP_AMMOFLAG_DONT_SEND_DAMAGE |
					WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_ONLY_CHECKPLAYER;

		// use ghost fire effects for guns that aren't shooting player
		if (AG->Target != NULL && ((vehVehicle*)(AG->Target))->ModelData == NULL)
			GhostFire = 1;
		else
			GhostFire = 0;

		// no friendly fire
		vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
		if(PlayerVehicle && wwiiSide(AG->Country) == wwiiSide(PlayerVehicle->Country))
			CanCauseDamage = 0;
		else
			CanCauseDamage = 1;

        // Fire on you if offline. Offline only fires if ".aion" is enabled (Ticket # 5376)
        if(!Online)
            CanCauseDamage = 1;

		Weapon = (weapWeapon*)AG->Weapon;

		FireDelta =  (frametime - AG->LastGunUpdateTime) / 1000.0;
		if(AG->LastGunUpdateTime == 0) FireDelta = 0.0f;

		if(AG->Target && ((vehVehicle*)AG->Target)->VehicleType->Type == VEH_TYPE_AIR)
			Flags |= WEAP_AMMOFLAG_DONTCHECKGROUNDOBJECTS | WEAP_AMMOFLAG_DONTCHECKTERRAIN;

		weapFireWeapon(&TempForce,
					  (weapWeapon*)AG->Weapon,
					  &Pos,
					  &FireVel,
					  &AttMat,
					  FireDelta,
					  frametime / 1000.0,
					  CanCauseDamage,
					  GhostFire,
					  Flags | WEAP_AMMOFLAG_IS_AUTOGUNROUND,
					  NULL,
					  NULL,
					  NULL,
					  NULL); // note: FireDelta is in seconds

		if (TempForce.x > 0.001 || TempForce.x < -0.001 ||
			TempForce.y > 0.001 || TempForce.y < -0.001 ||
			TempForce.z > 0.001 || TempForce.z < -0.001)
		{
			if (Weapon->WeaponType != NULL)
			{
				if (Weapon->WeaponType->FireType == WEAP_FIRETYPE_SINGLESHOT)
				{
                    SINT32 iID = fxSndGetNewSound();

                    if(iID > 0)
                    {
                        fxSndSetSoundName(iID, AG->Type->FireSound[AG->Country]);
                        fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                        fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
                        fxSndSetPitch(iID, 1.0f);
                        fxSndSetVolume(iID, 1.0f);
                        fxSndSetPosition(iID, AG->Pos.x, AG->Pos.y, AG->Pos.z);
                        fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
                        fxSndSetMinDistance(iID, 25.0f);
                        fxSndSetMaxDistance(iID, 500.0f);

                        fxSndInitSound(iID);
                    }
				}
				else
				{
					// start infinite sound
					if (AG->FireSoundId == 0)
					{
                        SINT32 iID = fxSndGetNewSound();

                        if(iID > 0)
                        {
                            fxSndSetSoundName(iID, AG->Type->FireSound[AG->Country]);
                            fxSndSetPriority(iID, IDE_TP_PRIORITY_MEDIUM);
                            fxSndSetSoundType(iID, TYPE_INFINITE_EXTERNAL_SOUND);
                            fxSndSetPitch(iID, 1.0f);
                            fxSndSetVolume(iID, 1.0f);
                            fxSndSetPosition(iID, AG->Pos.x, AG->Pos.y, AG->Pos.z);
                            fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
                            fxSndSetMinDistance(iID, 25.0f);
                            fxSndSetMaxDistance(iID, 500.0f);

                            if(fxSndInitSound(iID))
                            {
                                AG->FireSoundId = iID;
                            }
                        }
 					}
				}
			}
		}
	}
	else
	{
		if (AG->FireSoundId > 0 &&
			AG->GunIsFiring == FALSE)
		{
			fxSndStopSoundDelayed(AG->FireSoundId, 1);
			AG->FireSoundId = 0;
		}
	}

	if (LastReloading == FALSE &&
		weapIsWeaponReloading(Weapon,frametime / 1000.0) == TRUE)
	{
		if (AG->FireSoundId > 0)
		{
			fxSndStopSoundDelayed(AG->FireSoundId, 1);
			AG->FireSoundId = 0;
		}

		// play reload sound
		if (strlen(AG->Type->ReloadSound[AG->Country]) > 0)
		{
            SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
                fxSndSetSoundName(iID, AG->Type->ReloadSound[AG->Country]);
                fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
                fxSndSetPitch(iID, 1.0f);
                fxSndSetVolume(iID, 1.0f);
                fxSndSetPosition(iID, AG->Pos.x, AG->Pos.y, AG->Pos.z);
                fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
                fxSndSetMinDistance(iID, 5.0f);
                fxSndSetMaxDistance(iID, 40.0f);

                fxSndInitSound(iID);
            }
		}
	}

	return TRUE;
}

static LOGICAL _SetupElevAndTraverse(OBJECT * Object, piMATRIX * TravMat, piMATRIX * ElevMat)
{
OBJECT * ObjSib;

	if (Object == NULL)
	{
		return TRUE;
	}

	ObjSib = Object;

	do
	{
		if (ObjSib->flags & OBJECT_DOF)
		{
			if (ObjSib->dof->dofvarnum == ML_DOF_AG_TRAVERSE)
			{
				ObjSib->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

				ObjSib->dof->dofposmatrix = *TravMat;
			}

			if (ObjSib->dof->dofvarnum == ML_DOF_AG_ELEVATE)
			{
				ObjSib->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

				ObjSib->dof->dofposmatrix = *ElevMat;
			}
		}

		if (ObjSib->child != NULL)
		{
			_SetupElevAndTraverse(ObjSib->child,TravMat,ElevMat);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != NULL);

	return TRUE;
}


static LOGICAL _MoveGun(trnAutoGun * AG,piVECTOR* AimAtt)
{
    FLOAT32 DeltaTime = 0;
    FLOAT32 Delta = 0;
    FLOAT32 MaxMove = 0;

	DeltaTime = (frametime - AG->LastGunUpdateTime) * 0.001f;

    float traverse = AG->Traverse;

	// traverse
	if (AG->Type->TraverseRate[AG->Country] < 0.0f)
	{
		traverse = AimAtt->z;
	}
	else
	{
		Delta = AimAtt->z - traverse;
		if (Delta > 0.0f)
		{
			if (Delta > PI)
			{
				Delta = -(Delta - PI);
			}
		}
		else
		{
			if (Delta < -PI)
			{
				Delta = -(Delta + PI);
			}
		}

		MaxMove = AG->Type->TraverseRate[AG->Country] * DeltaTime;
		if (Delta > 0.0f)
		{
			if (MaxMove < Delta)
			{
				Delta = MaxMove;
			}
		}
		else
		{
			if (-MaxMove > Delta)
			{
				Delta = -MaxMove;
			}
		}

		traverse += Delta;
	}

    traverse = std::min(traverse, AG->MaxTraverse);
    traverse = std::max(traverse, AG->MinTraverse);

    float elevate = AG->Elevate;

	// elevation
	if (AG->Type->ElevateRate[AG->Country] < 0.0f)
	{
		elevate = AimAtt->x;
	}
	else
	{
		Delta = AimAtt->x - AG->Elevate;

		MaxMove = AG->Type->ElevateRate[AG->Country] * DeltaTime;
		if (Delta > 0.0f)
		{
			if (MaxMove < Delta)
			{
				Delta = MaxMove;
			}
		}
		else
		{
			if (-MaxMove > Delta)
			{
				Delta = -MaxMove;
			}
		}

		elevate += Delta;
	}

    elevate = std::min(elevate, AG->MaxElevate);
    elevate = std::max(elevate, AG->MinElevate);

	AG->Traverse = traverse;
    AG->Elevate = elevate;

	return TRUE;
}

static LOGICAL _SetupWeapon(weapWeapon* Weapon,SINT8 * WeaponName, SINT8 * BeltFile)
{
weaponType * WeaponType = NULL;

	if (Weapon == NULL || WeaponName == NULL || BeltFile == NULL)
	{
		return FALSE;
	}

	WeaponType = NULL;
	weapFindWeaponTypeByName(&WeaponType,(UCHAR*)WeaponName);
	if (WeaponType == NULL)
	{
		return FALSE;
	}

	weapInitializeWeapon(Weapon,WeaponType);

	mlLoadBeltFile(Weapon,(UCHAR*)BeltFile,0);

	Weapon->AmmoTypeInUse = 0;
	Weapon->NumRoundsAmmoLeft[0] = Weapon->WeaponType->NumRoundsInClip * 2;
	if (Weapon->NumRoundsAmmoLeft[0] == 0)
	{
		Weapon->NumRoundsAmmoLeft[0] = -1;
	}
	Weapon->NumTotalRounds[0] = Weapon->NumRoundsAmmoLeft[0];

	return TRUE;
}

static LOGICAL _AimGunAtTargetPnt(trnAutoGun * AG,FLOAT64VECTOR * AimPnt)
{
	if (AG == NULL || AimPnt == NULL)
	{
		return 0;
	}

    FLOAT64VECTOR TempVec1 = {0, 0, 0};
    FLOAT64VECTOR TempVec2 = {0, 0, 0};
    piVECTOR OldDisplacement = {0, 0, 0};
	piMATRIX TravMat = IdentityMatrix;
    piMATRIX ElevMat = IdentityMatrix;
    piVECTOR AimAtt = {0, 0, 0};
    piVECTOR Att = {0, 0, 0};
    piVECTOR AimVec = {0, 0, 0};

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	if (AG->Target == NULL)
	{
		AimAtt.x = AimAtt.y = AimAtt.z = 0.0f;
		AimAtt.z = AG->CenterTraverse;
	}
	else
	{
		// principal is this:  fire the "instant" round on the last guess relative position
		// and then adjust by how much it misses
		// this should cause successive firings to be closer and closer to hitting the target

		TempVec1 = *AimPnt;
		mathAddVectors(&TempVec1,&TempVec1,&AG->AimPntLead);

		OldDisplacement = AG->AimPntLead;

		mathAddVectors(&TempVec2,&AG->Pos,&AG->EyePos);

		AimVec.x = (FLOAT32)(TempVec1.x - TempVec2.x);
		AimVec.y = (FLOAT32)(TempVec1.y - TempVec2.y);
		AimVec.z = (FLOAT32)(TempVec1.z - TempVec2.z);


		if(AG->Target != PlayerVehicle || ((vehVehicle*)AG->Target)->VehicleType->Type == VEH_TYPE_AIR)
		{
			if(frametime - AG->LastLeadCalcTime > 250)
				_FireInstantRoundAndGetDisplacement(AG,&AimVec,AimPnt,&AG->AimPntLead);
		}else
		{
			_FireInstantRoundAndGetDisplacement(AG,&AimVec,AimPnt,&AG->AimPntLead);
		}

		mathSubVectors(&AimVec,&AimVec,&OldDisplacement);
		mathAddVectors(&AimVec,&AimVec,&AG->AimPntLead);

#ifdef BETA_RELEASE
{
    FLOAT64VECTOR WOO = {0, 0, 0};
    piVECTOR P = {0, 0, 0};
    piVECTOR V1 = {0, 0, 0};
    piVECTOR V2 = {0, 0, 0};

	trnGetWorldOriginOffset(&WOO);

	P.x = (FLOAT32)(AimPnt->x + AG->AimPntLead.x - WOO.x);
	P.y = (FLOAT32)(AimPnt->y + AG->AimPntLead.y - WOO.y);
	P.z = -(FLOAT32)((AimPnt->z + AG->AimPntLead.z));

	V1 = V2 = P;
	V1.x -= 1.0f;
	V2.x += 1.0f;
	draw3dLineDebug(&V1,&V2,colorGREENALPHA);
	V1 = V2 = P;
	V1.y -= 1.0f;
	V2.y += 1.0f;
	draw3dLineDebug(&V1,&V2,colorGREENALPHA);
	V1 = V2 = P;
	V1.z -= 1.0f;
	V2.z += 1.0f;
	draw3dLineDebug(&V1,&V2,colorGREENALPHA);

}
#endif
		AimAtt.z = (FLOAT32)atan2(AimVec.x,AimVec.y);
		AimAtt.x = (FLOAT32)atan2(AimVec.z,sqrt(AimVec.y * AimVec.y + AimVec.x * AimVec.x));
		AimAtt.y = 0.0f;
	}

	// now physically move the gun
	_MoveGun(AG,&AimAtt);

	Att.z = AG->Traverse - AG->CenterTraverse;
	Att.x = Att.y = 0.0f;
	mathEul_ToMatrix(&TravMat,EulOrdXYZs,&Att);

	Att.x = -AG->Elevate;
	Att.z = 0.0f;
	mathEul_ToMatrix(&ElevMat,EulOrdXYZs,&Att);

	_SetupElevAndTraverse(AG->Object,&TravMat,&ElevMat);

	return TRUE;
}


static LOGICAL _CalculateActualAimPnt(trnAutoGun * AG,FLOAT64VECTOR * AimPnt)
{
    FLOAT64VECTOR DistVec = {0, 0, 0};
    FLOAT64 Dist = 0;
    FLOAT64 AngleSpeed = 0;
    FLOAT64 TempNum1 = 0;
    FLOAT64VECTOR TempVec = {0, 0, 0};
    FLOAT64VECTOR TempVec1 = {0, 0, 0};
    FLOAT64VECTOR Velocity = {0, 0, 0};
    FLOAT32 Size = 0;
    FLOAT32 DeltaTime = 0;
    piVECTOR Vec = {0, 0, 0};
    piVECTOR Vec1 = {0, 0, 0};
    FLOAT32 TempNum = 0;
    FLOAT32 Shrink = 0;
    piVECTOR V1 = {0, 0, 0};
    UINT32 PartNum = 0;
    OBJECT * Object = NULL;
    piMATRIX TempMat = IdentityMatrix;
    piMATRIX TempMat1 = IdentityMatrix;
    piVECTOR Att = {0, 0, 0};
    vehVehicle* Target = NULL;
    OBJECT*	pAnimationObject = NULL;


	if (AG == NULL || AimPnt == NULL)
	{
		return FALSE;
	}

	if (AG->Target == NULL)
	{
		return FALSE;
	}

	mathSubVectors(&DistVec,AimPnt,&AG->Pos);
	Dist = mathExactVectorLength(&DistVec);

	if (((vehVehicle*)(AG->Target))->Flags & VEH_FLAGS_UPDATESYSTEM_CONTROLLED ||
		((vehVehicle*)(AG->Target))->ModelData == NULL)
	{
		Velocity = ((vehVehicle*)(AG->Target))->PosData.Vel;
	}
	else
	{
		Velocity = ((vehVehicle*)(AG->Target))->ModelData->VehicleRealVel;
	}
	// compute angle-off effects

	mathVectDoCrossProduct(&TempVec1,&DistVec,&Velocity);
	mathVectDoCrossProduct(&TempVec,&DistVec,&TempVec1);

	if (TempVec.x < 0.001f && TempVec.x > -0.001f &&
		TempVec.y < 0.001f && TempVec.y > -0.001f &&
		TempVec.z < 0.001f && TempVec.z > -0.001f)
	{
		TempVec.x = TempVec.y = 0.0f;
		TempVec.z = 1.0f;
	}
	else
	{
		TempNum1 = 1.0 / mathExactVectorLength(&TempVec);
		TempVec.x *= TempNum1;
		TempVec.y *= TempNum1;
		TempVec.z *= TempNum1;
	}

	AngleSpeed = mathVectDotProduct(&TempVec,&Velocity);
	if (AngleSpeed < 0.0)
	{
		AngleSpeed = -AngleSpeed;
	}

	// shrink cone
	if (AG->ConeSize < 0.00001f)
	{
		AG->ConeSize = AG->Type->MaxConeAngle[AG->Country];
		AG->StormTrooperFire = 1;
	}

	DeltaTime = (frametime - AG->LastGunUpdateTime) / 1000.0f;

	TempNum = AG->Type->MaxConeAngle[AG->Country] - AG->Type->MinConeAngle[AG->Country];
	TempNum /= AG->Type->TimeToMinCone[AG->Country];

	Shrink = TempNum * DeltaTime;

	if (AG->FireAtTarget == TRUE)
	{
		AG->ConeSize -= Shrink;
	}
	else
	{
		// make accuracy worse if target not visible
		AG->ConeSize += Shrink;
	}

	if (AG->Type->AngleOffMult[AG->Country] > 0.0001)
	{
		TempNum = (FLOAT32)(AngleSpeed / AG->Type->AngleOffMult[AG->Country] * Shrink);
		AG->ConeSize += TempNum;
	}

	// now handle gun aiming in wrong direction effects
	// compute angle wrong
	if (AG->Type->DegreesOffMult[AG->Country] > 0.0001)
	{
		Att.z = AG->Traverse;
		Att.x = Att.y = 0.0f;
		mathEul_ToMatrix(&TempMat,EulOrdXYZs,&Att);
		Att.x = -AG->Elevate;
		Att.z = 0.0f;
		mathEul_ToMatrix(&TempMat1,EulOrdXYZs,&Att);
		TempVec.x = TempVec.z = 0.0f;
		TempVec.y = 1.0f;
		mathVectorMultiply64(&TempVec,&TempVec,&TempMat1);
		mathVectorMultiply64(&TempVec,&TempVec,&TempMat);

		TempVec1.x = DistVec.x / Dist;
		TempVec1.y = DistVec.y / Dist;
		TempVec1.z = DistVec.z / Dist;
		TempNum1 = mathVectDotProduct(&TempVec,&TempVec1);
		if (TempNum1 > 1.0)
		{
			TempNum1 = 1.0;
		}
		if (TempNum1 < -1.0)
		{
			TempNum1 = -1.0;
		}

		TempNum1 = acos(TempNum1);

		AG->ConeSize += (FLOAT32)(Shrink * (TempNum1 / mlDEG_TO_RAD(AG->Type->DegreesOffMult[AG->Country])));
	}

	if (AG->ConeSize < AG->Type->MinConeAngle[AG->Country])
	{
		AG->ConeSize = AG->Type->MinConeAngle[AG->Country];
	}

	if (AG->ConeSize > AG->Type->MaxConeAngle[AG->Country])
	{
		AG->ConeSize = AG->Type->MaxConeAngle[AG->Country];
	}

	if (AG->GunIsFiring == FALSE ||
		AG->Type->GunFiresContinuously[AG->Country] == TRUE)
	{
		// calculate a new random aim point variance
		if (AG->ReAimStartTime - frametime < 0)
		{

			Vec.x = mathGetFloat32ChaoticNumber() - 0.5f;
			Vec.y = mathGetFloat32ChaoticNumber() - 0.5f;
			Vec.z = mathGetFloat32ChaoticNumber() - 0.5f;
			math3dNormalize(&Vec);

			if (AG->StormTrooperFire == 1)
			{
				Size = (FLOAT32)(sin(AG->Type->MaxConeAngle[AG->Country]) * Dist);

				Vec1.x = (FLOAT32)DistVec.x;
				Vec1.y = (FLOAT32)DistVec.y;
				Vec1.z = (FLOAT32)DistVec.z;

				math3dCrossProduct(&Vec,&Vec,&Vec1);
				math3dNormalize(&Vec);

				if (Vec.z < 0.0f)
				{
					Vec.x = -Vec.x;
					Vec.y = -Vec.y;
					Vec.z = -Vec.z;
				}
			}
			else
			{
				Size = (FLOAT32)(sin(AG->ConeSize) * Dist * mathGetFloat32ChaoticNumber());

			}

			if (AG->Type->GunFiresContinuously[AG->Country] == TRUE)
			{
				AG->StormTrooperFire = 0;
			}

			AG->AimPntDisplacement.x = Vec.x * Size;
			AG->AimPntDisplacement.y = Vec.y * Size;
			AG->AimPntDisplacement.z = Vec.z * Size;

			AG->ReAimStartTime = frametime + 500 + mathGetChaoticNumber() % 1000;

			// get new aim point on vehicle/person
			// pick a random internal component to aim for and apply its position
			Target = (vehVehicle*)AG->Target;
			if (Target->VehicleType != NULL)
			{
				// if a person aim at the center of the person
				if (Target->VehicleType->Type == VEH_TYPE_PERSON)
				{
					if (Target->VehicleObjectData != NULL)
					{
						if (Target->VehicleObjectData->PlaneObject != NULL)
						{
							vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

							pAnimationObject = Target->VehicleObjectData->PlaneObject;

							if (pAnimationObject == NULL)
							{
								Write_Debug("Error! Unable to find Animation Model in _CalculateActualAimPnt()");
								return FALSE;
							}

							V1.x = (pAnimationObject->bbox[0].x + pAnimationObject->bbox[1].x) / 2;
							V1.y = (pAnimationObject->bbox[0].y + pAnimationObject->bbox[1].y) / 2;
							V1.z = (pAnimationObject->bbox[0].z + pAnimationObject->bbox[1].z) / 2;

							TempMat = pAnimationObject->mat;

							TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
							math3dVectorMultiply(&V1,&V1,&TempMat);

							AG->AimPntDisplacement.x += V1.x;
							AG->AimPntDisplacement.y += V1.y;

							if(Target == PlayerVehicle && fpFPSIsProne())
								AG->AimPntDisplacement.z = V1.z + 1.0f; // never hit prone trooper
							else
								AG->AimPntDisplacement.z += V1.z;
						}
					}
				}
				else
				{
					// otherwise pick a part on the vehicle and shoot at it
					if (Target->VehicleType->NumDamageObjects != 0)
					{
						PartNum = mathGetChaoticNumber() % Target->VehicleType->NumDamageObjects + 1;
						ogrpFindSubObjectByName(&Object,
											Target->VehicleObjectData->HitMap,
											(SCHAR*)Target->VehicleType->DamageObjects[PartNum]);
						if (Object != NULL)
						{
							V1 = Object->pos;
							TempMat = Object->mat;
							TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
							math3dVectorMultiply(&V1,&V1,&TempMat);

							AG->AimPntDisplacement.x += V1.x;
							AG->AimPntDisplacement.y += V1.y;
							AG->AimPntDisplacement.z -= V1.z;
						}
					}

				}
			}
		}
	}
	else
	{
		AG->StormTrooperFire = 0;
	}

	AimPnt->x += AG->AimPntDisplacement.x;
	AimPnt->y += AG->AimPntDisplacement.y;
	AimPnt->z += AG->AimPntDisplacement.z;

	return TRUE;
}

static LOGICAL _UpdateFireStatus(trnAutoGun * AG)
{
	if (AG == NULL)
	{
		return FALSE;
	}

#if HAVE_AI_GUNS_ALWAYS_FIRE
    AG->GunIsFiring = TRUE;
    return 1;
#endif

	if (AG->Target == NULL)
	{
		AG->GunIsFiring = FALSE;

		return TRUE;
	}

	if (AG->FireAtTarget == FALSE)
	{
		AG->GunIsFiring = FALSE;

		return TRUE;
	}


	if (AG->NextBurstStartTime - frametime < 0)
	{
		AG->BurstEndTime = frametime + (UINT32)((0.125f + mathGetFloat32ChaoticNumber() * 0.375f) * 1000.0f + 0.5f);

		AG->NextBurstStartTime = frametime + (UINT32)((1.0f + mathGetFloat32ChaoticNumber()) * 1000.0f + 0.5f);
	}

	if (AG->Type->GunFiresContinuously[AG->Country] == TRUE)
	{
		AG->GunIsFiring = TRUE;
		return TRUE;
	}

	if (AG->BurstEndTime - frametime > 0)
	{
		AG->GunIsFiring = TRUE;
	}
	else
	{
		AG->GunIsFiring = FALSE;
	}

	return TRUE;
}

static LOGICAL _DetermineTarget(trnAutoGun * AG)
{
    vehVehicle* CurrentVehicle = NULL;
    vehVehicle* TempVehicle = NULL;
    vehVehicle* NoVis = NULL;
    UINT32 Found = 0;
    FLOAT32 Dist = 0;
    SINT32 Position = 0;
    UINT32 i = 0;
    UINT32 j = 0;
    WEAPON_SPEC Spec;

    memset(&Spec, 0, sizeof(WEAPON_SPEC));

	if (AG == NULL)
	{
		return FALSE;
	}

	// check for tracking list adjustment
	if (frametime - AG->NextTrackingAdjustmentTime > 0)
	{
        FLOAT32 Distances[TRN_MAX_AUTOGUN_TRACK];
        UINT32 Priorities[TRN_MAX_AUTOGUN_TRACK];
        UINT32 Visible[TRN_MAX_AUTOGUN_TRACK];
	    UINT32 VisLevel = 0;
	    UINT32 Priority = 0;
	    FLOAT32 TempDist = 0;
	    UINT32 TempPriority = 0;
	    UINT32 TempVis = 0;
	    LOGICAL IsDead = 0;
	    UINT32 Break = 0;

        void*  LastTrackingList[TRN_MAX_AUTOGUN_TRACK] = {NULL};

		// traverse through list of vehicles
		vehGetVehicleHead(&CurrentVehicle);

		// clear list
		for (i=0;i<TRN_MAX_AUTOGUN_TRACK;i++)
		{
			LastTrackingList[i] = AG->TrackingList[i];
			AG->TrackingList[i] = NULL;
			Distances[i] = 100000000.0f;
			Priorities[i] = 100;
			Visible[i] = 0;
		}

		while (CurrentVehicle != NULL)
		{
			// country check
            WWII_SIDE aiside = wwiiSide(AG->Country);
            WWII_SIDE vehside = wwiiSide(CurrentVehicle->Country);

            // Force to fire on you (Ticket # 5376)
            if(!Online)
                aiside = MAX_SIDES;

            if((CurrentVehicle->isSTO() ||
               aiside == vehside ||
			   CurrentVehicle->VehicleType->Type == VEH_TYPE_FLOATER) &&
			   CurrentVehicle->Country != 0)
			{
				CurrentVehicle = CurrentVehicle->getNext();
				continue;
			}

			if (CurrentVehicle->VehicleObjectData == NULL)
			{
				CurrentVehicle = CurrentVehicle->getNext();
				continue;
			}

			// vehicle type check
			Found = 0;
			for (i=0;i<3;i++)
			{
				if (CurrentVehicle->VehicleType != NULL)
				{
					Spec = CurrentVehicle->VehicleType->WeaponSpec;

					for (j=0;j<_MAX_IN_A_GROUP;j++)
					{
						if (_AutoGunTargetGroups[AG->Type->EngageVehiclePriority[AG->Country][i]][j].wCategory != 0 &&
							_AutoGunTargetGroups[AG->Type->EngageVehiclePriority[AG->Country][i]][j].wClass != 0)
						{
							if (_AutoGunTargetGroups[AG->Type->EngageVehiclePriority[AG->Country][i]][j].wCategory == Spec.wCategory &&
								_AutoGunTargetGroups[AG->Type->EngageVehiclePriority[AG->Country][i]][j].wClass == Spec.wClass)
							{
								Found = 1;
								Priority = i;
								break;
							}
						}
					}
					if (Found != 0)
					{
						break;
					}
				}
			}

			if (Found == 0)
			{
				CurrentVehicle = CurrentVehicle->getNext();
				continue;
			}

			// alive check
			if (CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				// ignore dead guys

				if ((CurrentVehicle->VehicleObjectData->PlaneObject) &&
					(CurrentVehicle->VehicleObjectData->PlaneObject->child) &&
					(CurrentVehicle->VehicleObjectData->PlaneObject->child->flag2 & OBJECT_ISFPS))
				{
					if (fpFPSIsDead())
					{
						CurrentVehicle = CurrentVehicle->getNext();
						continue;
					}
				}
				else if (gr2AnimIsDead(CurrentVehicle->VehicleObjectData->PlaneObject,&IsDead) == TRUE)
				{
					if (IsDead != 0)
					{
						CurrentVehicle = CurrentVehicle->getNext();
						continue;
					}
				}
			}
			else
			{
				// ignore burning tanks
				if (CurrentVehicle->StateAnimation.TankOnFire == TRUE)
				{
					CurrentVehicle = CurrentVehicle->getNext();
					continue;
				}

				// ignore burning vehicles
				Break = 0;
				for (i=0;i<ML_MAX_FUELTANKS;i++)
				{
					if (CurrentVehicle->StateAnimation.FuelFireOn[i] != 0)
					{
						Break = 1;
						break;
					}
				}
				if (Break != 0)
				{
					CurrentVehicle = CurrentVehicle->getNext();
					continue;
				}
			}

			// LOS/distance check
			if (_IsVehicleInLineOfSight(AG,CurrentVehicle,&Dist,TRUE) != TRUE)
			{
				Found = 0;
				for (i=0;i<TRN_MAX_AUTOGUN_TRACK;i++)
				{
					if (LastTrackingList[i] == CurrentVehicle)
					{
						Found = 1;
						break;
					}
				}

				if (Found == 0 || Dist > AG->Type->MaxEngagementRange[AG->Country] * 1.5f)
				{
					CurrentVehicle = CurrentVehicle->getNext();
					continue;
				}
				else
				{
					VisLevel = 0;
				}
			}
			else
			{
				VisLevel = 1;
			}
			// ok, lets try to add to the list
			Position = TRN_MAX_AUTOGUN_TRACK - 1;
			while (Position >= 0 &&
					(Dist < Distances[Position] || Priority < Priorities[Position] ||
					 VisLevel > Visible[Position]))
			{
				if (Position == TRN_MAX_AUTOGUN_TRACK - 1)
				{
					AG->TrackingList[Position] = CurrentVehicle;
					Distances[Position] = Dist;
					Priorities[Position] = Priority;
					Visible[Position] = VisLevel;
				}
				else
				{
					TempVehicle = (vehVehicle*)AG->TrackingList[Position];
					AG->TrackingList[Position] = AG->TrackingList[Position + 1];
					AG->TrackingList[Position + 1] = TempVehicle;

					TempDist = Distances[Position];
					Distances[Position] = Distances[Position + 1];
					Distances[Position + 1] = TempDist;

					TempPriority = Priorities[Position];
					Priorities[Position] = Priorities[Position + 1];
					Priorities[Position + 1] = TempPriority;

					TempVis = Visible[Position];
					Visible[Position] = Visible[Position + 1];
					Visible[Position + 1] = TempVis;
				}
				Position--;
			}

			CurrentVehicle = CurrentVehicle->getNext();
		}

		AG->NextTrackingAdjustmentTime = frametime + _TRACKING_SELECTION_TIME + mathGetChaoticNumber() % 500;
	}


	// here are the target selection rules:
	//
	// first find a visible target, if found, shoot him
	// otherwise see if there's a no-vis target, if found, aim at him but don't shoot
	if (frametime - AG->NextTargetSelectTime > _TARGET_SELECTION_TIME)
	{
		AG->LastTarget = AG->Target;
		AG->Target = NULL;
		AG->FireAtTarget = FALSE;
		NoVis = NULL;
		for (i=0;i<TRN_MAX_AUTOGUN_TRACK;i++)
		{
			if (AG->TrackingList[i] != NULL)
			{
				if (_IsVehicleInLineOfSight(AG,(vehVehicle*)AG->TrackingList[i],NULL,FALSE) == TRUE)
				{
					AG->Target = AG->TrackingList[i];
					AG->FireAtTarget = TRUE;
					break;
				}
				else
				{
					NoVis = (vehVehicle*)AG->TrackingList[i];
				}
			}
		}
		if (AG->Target == NULL)
		{
			AG->Target = NoVis;
		}

		AG->NextTargetSelectTime = frametime + _TARGET_SELECTION_TIME + mathGetChaoticNumber() % 100;
	}

	if (AG->LastTarget != AG->Target)
	{
		AG->AimPntDisplacement.x = 0.0f;
		AG->AimPntDisplacement.y = 0.0f;
		AG->AimPntDisplacement.z = 0.0f;
	}

	return TRUE;
}

// aim gun at deflection point
static LOGICAL _DetermineOptimumAimPnt(trnAutoGun * AG, FLOAT64VECTOR * AimPnt)
{
	if (AG == NULL || AimPnt == NULL)
	{
		return FALSE;
	}

	// check for target
	vehVehicle* Target = (vehVehicle*)AG->Target;
	if (Target == NULL)
	{
		return TRUE;
	}

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	*AimPnt =
		(PlayerVehicle == Target && PlayerVehicle->ModelData != NULL)
			? Target->ModelData->RenderPos
			: Target->PosData.RenderPos
			;

	return TRUE;
}

static LOGICAL _IsVehicleInLineOfSight(trnAutoGun * AG,vehVehicle* Vehicle,FLOAT32* Dist32,
									   LOGICAL AddFudge)
{
    FLOAT64VECTOR Corners[9] = {0, 0, 0};
    FLOAT64VECTOR DistVec = {0, 0, 0};
    FLOAT64 Dist = 0;
    FLOAT64 TempNum = 0;
    FLOAT64 Angle = 0;
    FLOAT64VECTOR TempVec = {0, 0, 0};
    FLOAT64VECTOR WorldOriginOffset = {0, 0, 0};
    UINT32 NumCornersToCheck = 0;
    piVECTOR BBox1 = {0, 0, 0};
    piVECTOR BBox2 = {0, 0, 0};
    piMATRIX TempMat = IdentityMatrix;
    piVECTOR V1 = {0, 0, 0};
    piVECTOR V2 = {0, 0, 0};
    UINT32 i = 0;
    OBJECT*	pAnimationObject = NULL;


	if (AG == NULL || Vehicle == NULL)
	{
		return FALSE;
	}

	trnGetWorldOriginOffset(&WorldOriginOffset);

	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		if (Vehicle->VehicleObjectData != NULL)
		{
			if (Vehicle->VehicleObjectData->PlaneObject != NULL)
			{
				pAnimationObject = Vehicle->VehicleObjectData->PlaneObject;
				if (pAnimationObject == NULL)
				{
					Write_Debug("Error! Unable to find Animation Model in _CalculateActualAimPnt()");
					return FALSE;
				}
			}
		}
	}

	if (Vehicle->Flags & VEH_FLAGS_UPDATESYSTEM_CONTROLLED ||
		Vehicle->ModelData == NULL || Vehicle->VehicleObjectData == NULL)
	{
		NumCornersToCheck = 1;
		Corners[0] = Vehicle->PosData.Pos;

		if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		{
			V1 = pAnimationObject->pos;
			TempMat = pAnimationObject->mat;
			TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
			math3dVectorMultiply(&V1,&V1,&TempMat);

			Corners[0].x += V1.x;
			Corners[0].y += V1.y;
			Corners[0].z += -V1.z;
		}
	}
	else
	{
		NumCornersToCheck = 9;
		Corners[0] = Vehicle->ModelData->AdjustedForCGPos;

		if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		{

			V1 = pAnimationObject->pos;
			TempMat = pAnimationObject->mat;
			TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
			math3dVectorMultiply(&V1,&V1,&TempMat);

			Corners[0].x += V1.x;
			Corners[0].y += V1.y;
			Corners[0].z += -V1.z;

			BBox1 = pAnimationObject->bbox[0];
			BBox2 = pAnimationObject->bbox[1];

		}
		else
		{
			BBox1 = Vehicle->VehicleObjectData->BBox1;
			BBox2 = Vehicle->VehicleObjectData->BBox2;
		}

		Corners[1].x = BBox1.x;
		Corners[1].y = BBox1.y;
		Corners[1].z = BBox1.z;

		Corners[2].x = BBox1.x;
		Corners[2].y = BBox2.y;
		Corners[2].z = BBox1.z;

		Corners[3].x = BBox2.x;
		Corners[3].y = BBox2.y;
		Corners[3].z = BBox1.z;

		Corners[4].x = BBox2.x;
		Corners[4].y = BBox1.y;
		Corners[4].z = BBox1.z;

		Corners[5].x = BBox1.x;
		Corners[5].y = BBox1.y;
		Corners[5].z = BBox2.z;

		Corners[6].x = BBox1.x;
		Corners[6].y = BBox2.y;
		Corners[6].z = BBox2.z;

		Corners[7].x = BBox2.x;
		Corners[7].y = BBox2.y;
		Corners[7].z = BBox2.z;

		Corners[8].x = BBox2.x;
		Corners[8].y = BBox1.y;
		Corners[8].z = BBox2.z;

		for (i=1;i<9;i++)
		{
			mathAddVectors(&Corners[i],&Corners[i],&Corners[0]);
		}
	}

	for (i=0;i<NumCornersToCheck;i++)
	{
		mathAddVectors(&TempVec,&AG->EyePos,&AG->Pos);
		mathSubVectors(&DistVec,&Corners[i],&TempVec);

		Dist = mathExactVectorLength(&DistVec);
		if (Dist32 != NULL)
		{
			*Dist32 = (FLOAT32)Dist;
		}

		if (Dist > AG->Type->MaxEngagementRange[AG->Country])
		{
			return FALSE;
		}

		// first check arc of fire
		// vertically
		TempVec.x = TempVec.y = 0.0;
		TempVec.z = 1.0f;

		TempNum = mathVectDotProduct(&TempVec,&DistVec);

		Angle = asin(TempNum/Dist);

		if (AddFudge == TRUE)
		{
			if (Angle > AG->MaxElevate + AG->Type->VisAdditionalAngle[AG->Country])
			{
				continue;
			}
			if (Angle < AG->MinElevate - AG->Type->VisAdditionalAngle[AG->Country])
			{
				continue;
			}
		}
		else
		{
			if (Angle > AG->MaxElevate)
			{
				continue;
			}
			if (Angle < AG->MinElevate)
			{
				continue;
			}
		}

		// next horizontally
		TempVec.z = 0.0f;
		TempVec.x = sin(AG->CenterTraverse);
		TempVec.y = cos(AG->CenterTraverse);

		TempNum = mathVectDotProduct(&TempVec,&DistVec);

		Angle = acos(TempNum/Dist);

		if (AG->LeftRightAngle <= mlDEG_TO_RAD(179.0))
		{
			if (AddFudge == TRUE)
			{
				if (Angle > AG->LeftRightAngle + AG->Type->VisAdditionalAngle[AG->Country])
				{
					continue;
				}
				if (Angle < -AG->LeftRightAngle - AG->Type->VisAdditionalAngle[AG->Country])
				{
					continue;
				}
			}
			else
			{
				if (Angle > AG->LeftRightAngle)
				{
					continue;
				}
				if (Angle < -AG->LeftRightAngle)
				{
					continue;
				}
			}
		}
		if(Vehicle->VehicleType->Type != VEH_TYPE_AIR)
		{
			// ok we're in view field, lets see if LOS is blocked
			V1.x = (FLOAT32)(AG->Pos.x + AG->EyePos.x - WorldOriginOffset.x);
			V1.y = (FLOAT32)(AG->Pos.y + AG->EyePos.y - WorldOriginOffset.y);
			V1.z = -(FLOAT32)(AG->Pos.z + AG->EyePos.z + WorldOriginOffset.z);

			V2.x = (FLOAT32)(Corners[i].x - WorldOriginOffset.x);
			V2.y = (FLOAT32)(Corners[i].y - WorldOriginOffset.y);
			V2.z = -(FLOAT32)(Corners[i].z + WorldOriginOffset.z);

#ifdef BETA_RELEASE
draw3dLineDebug(&V1,&V2,colorBLUEALPHA);
#endif

			if (trncdCollideDownMediumList(&V1,&V2) == TRUE)
			{
				continue;
			}
		}

		return TRUE;
	}

	return FALSE;
}

LOGICAL trnAutoVehicleDisappearing(void*  Vehicle)
{
trnAutoGun* AG = NULL;
UINT32 i = 0;

	for (AG = _AutoGunHead.next;AG != NULL;AG = AG->next)
	{
		if (AG->Target == Vehicle)
		{
			AG->Target = NULL;
		}

		for (i=0;i<TRN_MAX_AUTOGUN_TRACK;i++)
		{
			if (AG->TrackingList[i] == Vehicle)
			{
				AG->TrackingList[i] = NULL;
			}
		}
	}

	return TRUE;
}

LOGICAL trnUpdateAutogunStatus(void*  InAG,SINT32 country,SINT32 vislevel)
{
trnAutoGun * AG = NULL;

	if (InAG == NULL)
	{
		return FALSE;
	}

	AG = (trnAutoGun*)InAG;

// country is set when autogun created
//	AG->Country = country;
	if (vislevel != 0)
	{
		AG->AGDeadOrHidden = 1;
	}
	else
	{
		AG->AGDeadOrHidden = 0;
	}

	return TRUE;
}

LOGICAL trnGetAutoGunHead(trnAutoGun ** AG)
{
	*AG = _AutoGunHead.next;

	return TRUE;
}


// the theory is this:
// we advance the position of the target and the instant bullet
// and observe the closest the two get to each other
// this is the vector that the round missed the target, and is used
// as a correction factor the next instant round

static LOGICAL _FireInstantRoundAndGetDisplacement(trnAutoGun* AG,piVECTOR * AimVec,FLOAT64VECTOR * AimPnt,piVECTOR * NewDisplacement)
{
    FLOAT64 Mach = 0;
    FLOAT64 BaseMach = 0;
    FLOAT64 Distance = 0;
    FLOAT64 LastDistance = 0;
    FLOAT64 Time,LastTime = 0;
    FLOAT64 Speed = 0;
    FLOAT64 TempNum = 0;
    FLOAT64 Cd = 0;
    FLOAT64 Delta = 0;
    FLOAT64 Density = 0;
    FLOAT64VECTOR Accel = {0, 0, 0};
    FLOAT64VECTOR TempVec2 = {0, 0, 0};
    FLOAT64 DeltaTime = 0;
    FLOAT64VECTOR LastPosition = {0, 0, 0};
    SINT32 Index1 = 0;
    SINT32 Index2 = 0;

    FLOAT64VECTOR TargetPos = {0, 0, 0};
    FLOAT64VECTOR LastTargetPos = {0, 0, 0};
    FLOAT64VECTOR TargetVel = {0, 0, 0};
    FLOAT64VECTOR LastTargetVel = {0, 0, 0};
    FLOAT64VECTOR TargetAcc = {0, 0, 0};
    FLOAT64VECTOR LastTargetAcc = {0, 0, 0};
    FLOAT64VECTOR ClosestPoint = {0, 0, 0};
    FLOAT64 ClosestPointLen = 0;

    FLOAT64ATT Att1 = {0, 0, 0};
    FLOAT64ATT Att2 = {0, 0, 0};
    FLOAT64 Len = 0;
    FLOAT64VECTOR TempVec1 = {0, 0, 0};

    piVECTOR TempVec = {0, 0, 0};
    weapAmmoType * AmmoType = NULL;
    vehVehicle* Vehicle = NULL;

    weapAmmo Ammo;

    memset(&Ammo, 0, sizeof(weapAmmo));

	DeltaTime = TIME_INCREMENT;

	Distance = LastDistance = 0.0;
	Time = LastTime = 0.0;


	AmmoType = ((weapWeapon*)AG->Weapon)->WeaponType->AmmoType[0];

    // Sanity check.
    if(AmmoType == NULL)
    {
        // Program should bail during init if there are bad ammo files...
        // but just in case...
        Write_Debug("[trnautogun] *** AmmoType is NULL! Need to validate ammo files ***");
        return FALSE;
    }

	memset(&Ammo,0,sizeof(weapAmmo));
	TempVec = *AimVec;
	math3dNormalize(&TempVec);
	math3dScaleVector(&TempVec,&TempVec,AmmoType->InitVelocity);
	Ammo.Velocity.x = TempVec.x;
	Ammo.Velocity.y = TempVec.y;
	Ammo.Velocity.z = TempVec.z;

	mathAddVectors(&Ammo.Position,&AG->Pos,&AG->EyePos);

	ClosestPointLen = 10000000000000000000.0;
	ClosestPoint.x = ClosestPoint.y = ClosestPoint.z = 0.0;

	TargetPos = *AimPnt;
	Vehicle = (vehVehicle*)(AG->Target);
	if (Vehicle->ModelData != NULL)
	{
		TargetVel = Vehicle->ModelData->VehicleRealVel;
		TargetAcc = Vehicle->ModelData->VehicleAcc;
		TargetAcc.x = -TargetAcc.x;
		TargetAcc.z = -TargetAcc.z;
	}
	else
	{
		TargetVel = Vehicle->PosData.Vel;
		TargetAcc = Vehicle->PosData.Acc;
		TargetAcc.x = -TargetAcc.x;
		TargetAcc.z = -TargetAcc.z;
	}

	while (Time < AmmoType->LifeTime)
	{
		LastDistance = Distance;
		LastTime = Time;

		Time += TIME_INCREMENT;

		mlGetAltSpeedOfSound(&BaseMach,Ammo.Position.z);
		mlGetAltDensity(&Density,Ammo.Position.z);

		Speed = mathExactVectorLength(&(Ammo.Velocity));
		Mach = Speed / BaseMach;

		Index1 = (UINT32)(floor(Mach / G1_CHART_MACH_DELTA));
		Index2 = Index1+1;

		if (Index2 >= G1_CHART_NUM_DIVISIONS)
		{
			Cd = G1MachChart[G1_CHART_NUM_DIVISIONS - 1];
		}
		else
		{
			Delta = (Mach / G1_CHART_MACH_DELTA) - Index1;

			mlInterpolate(&Cd,G1MachChart[Index1],G1MachChart[Index2],Delta);
		}

		// NOTE: the 1790 is a constant that makes the Cd table work with
		// G1 ballistic coefficients and this particular formula
		TempNum = Density * Cd * Speed / (AmmoType->BallisticCoefficient * 1790.0f);
		Accel.x = TempNum * Ammo.Velocity.x;
		Accel.y = TempNum * Ammo.Velocity.y;
		Accel.z = TempNum * Ammo.Velocity.z;

		TempVec2.x = (Accel.x + Ammo.LastAcc.x) * 0.5;
		TempVec2.y = (Accel.y + Ammo.LastAcc.y) * 0.5;
		TempVec2.z = (Accel.z + Ammo.LastAcc.z) * 0.5;

		Ammo.Velocity.x -= TempVec2.x * DeltaTime;
		Ammo.Velocity.y -= TempVec2.y * DeltaTime;
		Ammo.Velocity.z -= TempVec2.z * DeltaTime;

		Ammo.LastAcc = Accel;

		// add in gravity
		Ammo.Velocity.z -= 9.8 * DeltaTime;

		LastPosition = Ammo.Position;
		// now apply velocity vector
		if (fabs(Ammo.LastVel.x) > fabs(Ammo.Velocity.x) ||
			fabs(Ammo.LastVel.y) > fabs(Ammo.Velocity.y))
		{
			Ammo.Position.x += Ammo.LastVel.x * DeltaTime + 0.5 * TempVec2.x * DeltaTime * DeltaTime;
			Ammo.Position.y += Ammo.LastVel.y * DeltaTime + 0.5 * TempVec2.y * DeltaTime * DeltaTime;
			Ammo.Position.z += Ammo.LastVel.z * DeltaTime + 0.5 * TempVec2.z * DeltaTime * DeltaTime;
		}
		else
		{
			Ammo.Position.x += Ammo.Velocity.x * DeltaTime;
			Ammo.Position.y += Ammo.Velocity.y * DeltaTime;
			Ammo.Position.z += Ammo.Velocity.z * DeltaTime;
		}

		Ammo.LastVel = Ammo.Velocity;


		// now adjust predicted position of target
		LastTargetAcc = TargetAcc;
		LastTargetVel = TargetVel;
		LastTargetPos = TargetPos;

		// adjust velocity
		TargetVel.x += TargetAcc.x * DeltaTime;
		TargetVel.y += TargetAcc.y * DeltaTime;
		TargetVel.z += TargetVel.z * DeltaTime;

		// adjust acceleration

		mathGetExactAttFromVec(&Att2,&TargetVel);
		mathGetExactAttFromVec(&Att1,&LastTargetVel);

		Att2.Pitch -= Att1.Pitch;
		Att2.Roll -= Att1.Roll;
		Att2.Yaw -= Att1.Yaw;

        // Clamp small numbers, otherwise we get #DEN and matherr
        if(TargetAcc.x < 1e-16 || TargetAcc.x > -1e-16)
        {
            TargetAcc.x = 0;
        }

        if(TargetAcc.y < 1e-16 || TargetAcc.y > -1e-16)
        {
            TargetAcc.y = 0;
        }

        if(TargetAcc.z < 1e-16 || TargetAcc.z > -1e-16)
        {
            TargetAcc.z = 0;
        }

        mathGetExactAttFromVec(&Att1,&TargetAcc);

		Att1.Pitch += Att2.Pitch;
		Att1.Roll += Att2.Roll;
		Att1.Yaw += Att2.Yaw;

		TargetAcc.x = TargetAcc.y = 0.0;
		TargetAcc.z = 1.0;

		mathRotateVectorByAtt(&TargetAcc,&TargetAcc,&Att1);

		Len = mathExactVectorLength(&LastTargetAcc);

		// adjust position
		TargetPos.x += TargetVel.x * DeltaTime;
		TargetPos.y += TargetVel.y * DeltaTime;
		TargetPos.z += TargetVel.z * DeltaTime;

		// now find out if this is the closest point
		mathSubVectors(&TempVec1,&TargetPos,&Ammo.Position);

		Len = mathExactVectorLength(&TempVec1);
		if (Len < ClosestPointLen)
		{
			ClosestPointLen = Len;
			ClosestPoint = TempVec1;
		}

	}

	// now ensure that the displacement is in the plane perpendicular to the aim vector
	mathAddVectors(&ClosestPoint,NewDisplacement,&ClosestPoint);

	TempVec1.x = AimVec->x;
	TempVec1.y = AimVec->y;
	TempVec1.z = AimVec->z;
	Len = mathExactVectorLength(AimVec);
	TempVec1.x /= Len;
	TempVec1.y /= Len;
	TempVec1.z /= Len;
	Len = mathVectDotProduct(&ClosestPoint,&TempVec1);

	ClosestPoint.x = ClosestPoint.x - Len * TempVec1.x;
	ClosestPoint.y = ClosestPoint.y - Len * TempVec1.y;
	ClosestPoint.z = ClosestPoint.z - Len * TempVec1.z;

	NewDisplacement->x = (FLOAT32)ClosestPoint.x;
	NewDisplacement->y = (FLOAT32)ClosestPoint.y;
	NewDisplacement->z = (FLOAT32)ClosestPoint.z;

	AG->LastLeadCalcTime = frametime;

	return TRUE;
}
