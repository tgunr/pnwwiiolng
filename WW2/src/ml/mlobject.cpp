#include "syscfg.h" // Path is project specific.

#include "pi/pi.h"
#include "include/vars.h"

#include <string.h>
#include <math.h>

#include "fio/fio.h"
#include "veh/vehmain.h"

#include "gr2/gr2Anim.h"

#include "ml/mltypes.h"
#include "ml/mlobject.h"
#include "ml/mlgauges.h"
#include "ml/mlgear.h"
#include "ml/mlweapons.h"
#include "ml/mlCompon.h"
#include "ml/mlutility.h"
#include "ob/obmain.h"
#include "ob/obpool.h"
#include "ld/ldmdl.h"
#include "ogrp/ogrptype.h"
#include "mem/mem.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "veh/vehcntrl.h"
#include "trn/trnmain.h"
#include "pers/persmain.h"
#include "rn/rnmain.h"
#include "ld/ldpix.h"
#include "ob/obdshad.h"
#include "ob/obdraw.h"
#include "locl/loclMain.h"
#include "veh/vehanim.h"
#include "ob/obMgr.h"
#include "ogl/ogl.h"
#include "ogl/oglMaterial.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

struct ReflectorSight
{
    bool flip;
	LOGICAL inited;
	FLOAT32 width;
	piVECTOR up, right,front;
	piVECTOR center;
	OBJECT* gviewObj;
};

ReflectorSight _ReflectorSights[ML_MAX_CREW_POSITIONS];

static UINT32 _ProcessObjectWithAirfoilComponent(mlComponent * Component,OBJECT * Object);
static UINT32 _ProcessObjectWithEngineComponent(mlComponent * Component,OBJECT * Object);
static UINT32 _ProcessObjectWithWeaponComponent(mlComponent * Component,OBJECT * Object);
static UINT32 _ProcessObjectWithGearComponent(mlComponent * Component,OBJECT * Object);
static UINT32 _FindAllContactPoints(OBJECT ** ObjectPtrArray,UINT32* Index,UINT32 * NumLeft,UINT32 GearNum,OBJECT * Object);
static UINT32 _RelocateObject(OBJECT * Object,piMATRIX * Mat);
static UINT32 _SetupVehicleTracks(mlObjectTrackSection ** TrackSectionList,
								  UINT32 * Count,
								  mlObjectTrackAnimation ** TrackAnimationList,
								  UINT32 * AnimCount,
								  OBJECT * Object);
static UINT32 _CountTrackSections(UINT32 * Count, OBJECT * Object);
static UINT32 _CountTrackAnimations(UINT32 * Count, OBJECT * Object);
static UINT32 _SetupTracks(mlObjectTrackSection ** TrackSection,
						   OBJECT * Object,
						   OBJECT * OriginalObject);
static UINT32 _SetupTrackAnimations(mlObjectTrackAnimation ** TrackAnimation,
									OBJECT * Object,
									OBJECT * OriginalObject);
static UINT32 _SetupTrackSection(mlObjectTrackSection ** TrackSection,
								 UCHAR * LinkObject,
								 OBJECT * Object,
								 OBJECT * OriginalObject);
static UINT32 _FindAndSetupFPSSeats(mlObjectData * ObjectData,
									OBJECT * CurrentObject,
									LOGICAL ScanSiblings);
static UINT32 _FindAndSetupVehicleSeats(mlObjectData * ObjectData,
									OBJECT * CurrentObject,
									LOGICAL ScanSiblings);

static UINT32 _CalculateObjectTreeRadius(FLOAT32 * Radius,OBJECT * Object, UINT32 IncludeSibs);

static UINT32 _CountLineCollideSubObjects(UINT32 * Count, OBJECT * Object);
static UINT32 _SetupCollisionObjects(mlObjectCollidePnt ** CurrentCollidePnt,OBJECT * Object);
static UINT32 _FindAndSetupReflectorSightObjects(OBJECT * Object);
static UINT32 _FindAndSetupReflectorSightSubObjects(OBJECT * Object);
static bool _DrawGunsightReticle(OBJECT * Object);
static bool _OldDrawGunsightReticle(OBJECT * Object);
static UINT32 _FindLODs(mlObjectData * ObjectData);
static UINT32 _FindAllWheelRadius(OBJECT * lpobj,mlObjectData * ObjectData);
static UINT32 _FindAllWheelRadiusInSubTree(OBJECT * lpobj,mlObjectData * ObjectData);
static UINT32 _FindSubObjectInSubTree(OBJECT * Object, OBJECT * SearchObject);
static UINT32 _ShowTreeFromOneObjectToAnother(OBJECT * CurrentObject, OBJECT * SearchObject);
static UINT32 _HideAllGunsights(OBJECT * Object);
static UINT32 _ImportAndSetupWeapons(mlObjectData*, vehType const* VehicleType);
static UINT32 _IncrementObjectLayer(OBJECT * Object);
static UINT32 _IncrementSubObjectLayer(OBJECT * Object);

static UINT32 _CountAndSetupCageLines(UINT32 * NumCageLines,
									  piVECTOR * CageLineV1,
									  piVECTOR * CageLineV2,
									  OBJECT ** CageLineObject,
									  OBJECT * Object,
									  UINT32 IncludeSibs);
static UINT32 _FindAndSetupHideUnhideObjects(mlObjectData * ObjectData,
											 OBJECT * Object);
static UINT32 mlProcessHoistContactPointObject(mlObjectData* ObjectData);

static UINT32 _DisplayObjectTree(OBJECT * Object,UINT32 IncludeSibs);
static UINT32 _FindLowestHighestPoints(OBJECT* Object,FLOAT32* LowestPoint, FLOAT32* HighestPoint);
static UINT32 _WeaponCount;

#ifdef USE_SHADOWS
static LOGICAL _CreateVehicleShadow(OBJECT* lpgrp)
{
OBJECT*	lpobj;

	lpobj = obmainFindChildFromObject(lpgrp, "shadow");
	if (lpobj == nullptr)
	{
		WriteLog(LOG_WEAPON, LOG_DEBUG,"Shadow object not found in model %s", lpgrp->name);
		// no shadow, no error
		return(TRUE);
	}

	if (!obdshadCreate(lpgrp, lpobj))
	{
		return(FALSE);
	}

	return(TRUE);
}
#endif

static LOGICAL _MissingCallback(OBJECT* obj, UINT32 user)
{
	ogrpHideObjectAndSubtree(obj);
	obmainSetFlag2InThisTree(obj,  OBJECT_MISSING);

	return TRUE;
}

UINT32 mlobjectLoadVehicleObject(mlObjectData ** ObjectData,
								 UCHAR * ObjectName,
								 UINT32 NumDamageObjects,
								 vehVehicle*Vehicle,
								 UINT32 LoadoutNum,
								 UCHAR * DamageObjectList,
								 LOGICAL LoadCockpit)
{
OBJECT * Object;
SINT8* ptr, LocalObjectName[256];
static UINT32 Counter = 0;


	if (ObjectData == nullptr || ObjectName == nullptr)
	{
		return 1;
	}

	if (strlen((char*)ObjectName) == 0)
	{
		return 1;
	}

	Object = nullptr;

	lstrcpy(LocalObjectName, (char*)ObjectName);
	ptr = strrchr(LocalObjectName, '\\');
	if (ptr != nullptr)
	{
		ptr += 1;
		lstrcpy(LocalObjectName, ptr);
	}

	// if we're a person, create a placeholder object, don't go to the pool
	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		Object = gr2AnimCreateObject(Vehicle, Vehicle->VehicleType->VehicleTypeNum);
	}
	else
	{
		if(!obmgrCreateModelInstance((char*)ObjectName))
		{
			return 1;
		}

		lstrcpy(LocalObjectName, (char*)ObjectName);
		ptr = strrchr(LocalObjectName, '\\');
		if (ptr != nullptr)
		{
			ptr += 1;
			lstrcpy(LocalObjectName, ptr);
		}

		Object = obmainFindNamedObject((char*)LocalObjectName);

		if (Object == nullptr)
		{
			return 1;
		}
	}


	// Animated objects are a special case and need their shadows loaded
	// seperately.
	if (Object->flag2 & OBJECT_ANIMATION)
	{
		if (!gr2AnimInitShadow(Object))
		{
			return 1;
		}
	}
#ifdef USE_SHADOWS
	else
	{
		if (!_CreateVehicleShadow(Object))
		{
			obpoolFreeObjectCallback(Object);
			return 1;
		}
	}
#endif


	// rename object
	snprintf(Object->name, 32, "%s%dml",LocalObjectName,Counter++);
	(*ObjectData) = mlObjectData::Get();

    if (*ObjectData == nullptr)
	{
        // This looks bad...will probably cause a crash somewhere
		obpoolFreeObjectCallback(Object);
	}
    else
    {
        (*ObjectData)->PlaneObject = Object;
    }

	mlInitVehicleObject(ObjectData, Vehicle->VehicleType, LoadCockpit);

	vehAnimResetVehicleStateAnimations(Vehicle);


	return 0;
}

static UINT32 _FindAndSetupHideUnhideObjects(mlObjectData* ObjectData, OBJECT* Object)
{
	OBJECT* ObjSib;
	SINT8* CommentStr;
	SINT32 idx;

	if (Object == nullptr || ObjectData == nullptr) return 0;

	for (ObjSib = Object; ObjSib; ObjSib = ObjSib->sibling)
	{
		if (ObjSib->flags & OBJECT_GROUP)
		{
			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"gearup") && ObjectData->GearUpCount < ML_MAX_GEARUPDOWN_GROUPS)
			{
				ObjectData->GearUpGroup[ObjectData->GearUpCount] = ObjSib;
				ObjectData->GearUpCount++;
			}

			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"geardn") && ObjectData->GearDownCount < ML_MAX_GEARUPDOWN_GROUPS)
			{
				ObjectData->GearDownGroup[ObjectData->GearDownCount] = ObjSib;
				ObjectData->GearDownCount++;
			}

			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"flapup") && ObjectData->FlapsUpCount < ML_MAX_FLAPSUPDOWN_GROUPS)
			{
				ObjectData->FlapsUpGroup[ObjectData->FlapsUpCount] = ObjSib;
				ObjectData->FlapsUpCount++;
			}

			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"flapdn") && ObjectData->FlapsDownCount < ML_MAX_FLAPSUPDOWN_GROUPS)
			{
				ObjectData->FlapsDownGroup[ObjectData->FlapsDownCount] = ObjSib;
				ObjectData->FlapsDownCount++;
			}

			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"enginefast"))
			{
				idx = atoi(CommentStr) - 1;
				if (idx >= 0)
				{
					if (ObjectData->EngineFastCount[idx] < ML_MAX_ENGINEFASTSLOW_GROUPS)
					{
						ObjectData->EngineFastGroup[idx][ObjectData->EngineFastCount[idx]] = ObjSib;
						ObjectData->EngineFastCount[idx]++;
					}
				}else
				{
					WriteLog(LOG_WEAPON, LOG_DEBUG,"enginefast comment for object %s with invalid engine number", ObjSib->name);
				}
			}

			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"engineslow"))
			{
				idx = atoi(CommentStr) - 1;
				if (idx >= 0)
				{
					if (ObjectData->EngineSlowCount[idx] < ML_MAX_ENGINEFASTSLOW_GROUPS)
					{
						ObjectData->EngineSlowGroup[idx][ObjectData->EngineSlowCount[idx]] = ObjSib;
						ObjectData->EngineSlowCount[idx]++;
					}
				}else
				{
					WriteLog(LOG_WEAPON, LOG_DEBUG,"enginefast comment for object %s with invalid engine number", ObjSib->name);
				}
			}
		}

		if (ObjSib->child)
			_FindAndSetupHideUnhideObjects(ObjectData,ObjSib->child);
	}


	return 0;
}

static UINT32 _FindAndSetupFPSSeats(mlObjectData * ObjectData,
									OBJECT * CurrentObject,
									LOGICAL ScanSiblings)
{
OBJECT * ObjSib;

	if (CurrentObject == nullptr)
	{
		return 0;
	}

	if (ObjectData->NumFPSSeats >= ML_MAX_FPS_SEATS)
	{
		return 0;
	}

	ObjSib = CurrentObject;

	do
	{
		if (ObjSib->flags & OBJECT_DOF)
		{
			if (ObjSib->dof->dofvarnum == ML_DOF_FPS_SEAT)
			{
				ObjectData->FPSSeats[ObjectData->NumFPSSeats++] = ObjSib;

				if (ObjectData->NumFPSSeats >= ML_MAX_FPS_SEATS)
				{
					return 0;
				}
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindAndSetupFPSSeats(ObjectData,
									ObjSib->child,
									TRUE);

			if (ObjectData->NumFPSSeats >= ML_MAX_FPS_SEATS)
			{
				return 0;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && ScanSiblings == TRUE);

	return 0;
}

static UINT32 _FindAndSetupVehicleSeats(mlObjectData * ObjectData,
									OBJECT * CurrentObject,
									LOGICAL ScanSiblings)
{
OBJECT * ObjSib;

	if (CurrentObject == nullptr)
	{
		return 0;
	}

	if (ObjectData->NumVehSeats >= ML_MAX_VEH_SEATS)
	{
		return 0;
	}

	ObjSib = CurrentObject;

	do
	{
		if (ObjSib->flags & OBJECT_DOF)
		{
			if (ObjSib->dof->dofvarnum == ML_DOF_VEH_SEAT)
			{
				ObjectData->VehSeats[ObjectData->NumVehSeats++] = ObjSib;

				if (ObjectData->NumVehSeats >= ML_MAX_VEH_SEATS)
				{
					return 0;
				}
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindAndSetupVehicleSeats(ObjectData,
									ObjSib->child,
									TRUE);

			if (ObjectData->NumVehSeats >= ML_MAX_VEH_SEATS)
			{
				return 0;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && ScanSiblings == TRUE);

	return 0;
}


UINT32 mlForEachSubObject(OBJECT* Object, mlForEachCallback callback, void* data, LOGICAL IncludeSib)
{
	OBJECT* ObjSib;

	if(Object == nullptr) return 0;

	ObjSib = Object;

	do
	{
		callback(ObjSib, data);

		if (ObjSib->child != nullptr)
			mlForEachSubObject(ObjSib->child, callback, data, 1);

		ObjSib = ObjSib->sibling;
	} while(IncludeSib && ObjSib);

	return 0;
}

static UINT32 _FindAllWheelRadius(OBJECT * lpobj,mlObjectData * ObjectData)
{
SINT8* CommentStr;
FLOAT32 TempNum;

	if (lpobj == nullptr)
	{
		return 0;
	}

	if (ObjectData == nullptr)
	{
		return 0;
	}

	if (lpobj->flags & OBJECT_DOF)
	{
		if (obmainFindCommentName((SINT8 **)&CommentStr,lpobj->comment,"radius") == TRUE)
		{
			sscanf((char*)CommentStr," %f",&TempNum);
			ObjectData->WheelRadius[lpobj->dof->dofvarsubnum] = TempNum;
		}
	}

	if (lpobj->child != nullptr)
	{
		_FindAllWheelRadiusInSubTree(lpobj->child,ObjectData);

	}

	return 0;
}

static UINT32 _FindAllWheelRadiusInSubTree(OBJECT * lpobj,mlObjectData * ObjectData)
{
OBJECT * ObjSib;
SINT8* CommentStr;
FLOAT32 TempNum;

	if (lpobj == nullptr)
	{
		return 0;
	}

	if (ObjectData == nullptr)
	{
		return 0;
	}

	ObjSib = lpobj;

	do
	{
		if (ObjSib->flags & OBJECT_DOF)
		{
			if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"radius") == TRUE)
			{
				sscanf((char*)CommentStr," %f",&TempNum);
				ObjectData->WheelRadius[ObjSib->dof->dofvarsubnum] = TempNum;
			}
		}

		if (ObjSib->child != nullptr)
		{
			_FindAllWheelRadiusInSubTree(ObjSib->child,ObjectData);

		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _FindLODs(mlObjectData * ObjectData)
{
UINT32 i;
OBJECT * ObjectPtr;

UCHAR s[32];

	if (ObjectData == nullptr)
	{
		return 1;
	}

	if (ObjectData->PlaneObject == nullptr)
	{
		return 1;
	}

	for (i=0;i<ML_MAX_LODS;i++)
	{
		sprintf((char*)s,"lod%d",i+1);

		ObjectPtr = nullptr;
		ogrpFindSubObjectByName(&ObjectPtr,ObjectData->PlaneObject,(char*)s);

		ObjectData->LODObjects[i] = ObjectPtr;
		if (ObjectPtr != nullptr)
		{
			while (ObjectPtr->flags & OBJECT_NOTOBJECT)
			{
				if (ObjectPtr->child == nullptr)
				{
					WriteLog(LOG_WEAPON, LOG_DEBUG,"No object under %s in vehicle object %s",s,ObjectData->PlaneObject->name);
					break;
				}
				ObjectPtr = ObjectPtr->child;
			}
			ObjectData->SavedMinLODRange[i] = ObjectPtr->minlod;
			ObjectData->SavedMaxLODRange[i] = ObjectPtr->maxlod;
		}
	}

	// vis damage
	ogrpFindSubObjectByName(&ObjectData->VisDamageGroup, ObjectData->PlaneObject, "visdamage");
	if(ObjectData->VisDamageGroup)
	{
		// crap
		ogrpFindSubObjectByName(&ObjectData->VisDamageCrapGroup, ObjectData->VisDamageGroup, "crap");
		if(ObjectData->VisDamageCrapGroup)
			ObjectData->VisDamageCrapGroup->flag2 |= OBJECT_DONT_UPDATE;

		// stub
		ogrpFindSubObjectByName(&ObjectData->VisDamageStubGroup, ObjectData->VisDamageGroup, "stub");
		if(ObjectData->VisDamageStubGroup)
			obmainSetFlag2InThisTree(ObjectData->VisDamageStubGroup, OBJECT_HIDDEN);

		// lod hulk
		ogrpFindSubObjectByName(&ObjectData->VisDamageHulkLOD, ObjectData->VisDamageGroup, "lodhulk");
		if(ObjectData->VisDamageHulkLOD)
		{
			ObjectData->VisDamageHulkLOD->flag2 |= OBJECT_DONT_UPDATE;
			for(OBJECT* obj = ObjectData->VisDamageHulkLOD->child; obj; obj=obj->sibling)
			{
				for(OBJECT* TempObject = obj;TempObject;TempObject = TempObject->attached)
					TempObject->trnum = 1; // using trnum for damage ID number so we can damage it
			}
		}
	}

	// find weapon loadout lod
	ObjectPtr = nullptr;
	ogrpFindSubObjectByName(&ObjectPtr,ObjectData->PlaneObject,"lodloadout");

	ObjectData->LoadoutLODObject = ObjectPtr;
	if (ObjectPtr != nullptr)
	{
		while (ObjectPtr->flags & OBJECT_NOTOBJECT)
		{
			if (ObjectPtr->child == nullptr)
			{
				WriteLog(LOG_WEAPON, LOG_DEBUG,"No object under lodloadout in vehicle object %s",ObjectData->PlaneObject->name);
				break;
			}
			ObjectPtr = ObjectPtr->child;
		}
		ObjectData->SavedMinLODLoadoutRange = ObjectPtr->minlod;
		ObjectData->SavedMaxLODLoadoutRange = ObjectPtr->maxlod;
	}

	return 0;
}

UINT32 mlMoveAndRotateObject(OBJECT * Object,FLOAT64VECTOR * InPos,FLOAT64ATT * InAtt)
{
	// if this object is under physics control, don't update it
	if(Object->flag3 & OBJECT_PHYSICAL)
		return 0;

	piVECTOR Pos;
	piVECTOR Att;
	FLOAT64VECTOR TempPos;
	FLOAT64VECTOR WorldOffsetOrigin;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	mathSubVectors(&TempPos,InPos,&WorldOffsetOrigin);

	Pos.x = (float)(TempPos.x) * ScaleFactor;
	Pos.y = (float)(TempPos.y) * ScaleFactor;
	Pos.z = -(float)(TempPos.z) * ScaleFactor;

	if (InAtt != nullptr)
	{
		Att.x = -(float)InAtt->Pitch;
		Att.y = -(float)InAtt->Roll;
		Att.z = (float)InAtt->Yaw;
	}
	else
	{
		Att.x = Att.y = Att.z = 0;
	}

	mlConvertModelAttitudeToWorldAttitude(&Att,&Att);
	ogrpMoveAndRotateObjectGroup(Object,&Pos,&Att);

	return 0;
}

#include "fx/fxMgr.h"

UINT32 mlobjectKillVehicleObject(mlObjectData ** ObjectData)
{
	if ((*ObjectData)->PlaneObject)
		fxMgrDetach((*ObjectData)->PlaneObject);

	if ((*ObjectData)->HitMap)
		fxMgrDetach((*ObjectData)->HitMap);

	if ((*ObjectData)->WheelRadius != nullptr)
	{
		memFree((*ObjectData)->WheelRadius);
		(*ObjectData)->WheelRadius = nullptr;
	}

	if ((*ObjectData)->PlaneObject != nullptr)
	{
		ogrpFreeObjectTree((*ObjectData)->PlaneObject);
		(*ObjectData)->PlaneObject = nullptr;
	}

	if ((*ObjectData)->TrackSection != nullptr)
	{
		memFree((*ObjectData)->TrackSection);
		(*ObjectData)->TrackSection = nullptr;
	}

	if ((*ObjectData)->TrackAnimation != nullptr)
	{
		memFree((*ObjectData)->TrackAnimation);
		(*ObjectData)->TrackAnimation = nullptr;
	}

	if ((*ObjectData)->CollidePntList != nullptr)
	{
		memFree((*ObjectData)->CollidePntList);
		(*ObjectData)->CollidePntList = nullptr;
	}

	if ((*ObjectData)->HoistCollidePntList != nullptr)
	{
		memFree((*ObjectData)->HoistCollidePntList);
		(*ObjectData)->HoistCollidePntList = nullptr;
	}

	if ((*ObjectData) != nullptr)
	{
		mlObjectData::Release(*ObjectData);
		*ObjectData = nullptr;
	}

	return 0;
}

UINT32 mlObjectSetInplaneObject(OBJECT * BaseObject,UINT32 Position)
{
OBJECT * Object;
OBJECT * Object1;
OBJECT * Object2;
UCHAR s[64];

	// hide external object
	ogrpFindSubObjectByName(&Object,BaseObject,"extern");
	if (Object != nullptr)
	{
		ogrpHideObjectAndSubtree(Object);
	}

	// now show the in-plane object
	ogrpFindSubObjectByName(&Object,BaseObject,"vcpit");
	ogrpHideObjectAndSubtree(Object);

	if (Object != nullptr)
	{
		sprintf((char*)s,"posit%d",Position);
		ogrpFindSubObjectByName(&Object1,Object,(char*)s);
		if (Object1 == nullptr || Object->child == nullptr)
		{
			ogrpShowObjectAndSubtree(Object);
		}
		else
		{
			for (Object2 = Object->child;Object2 != nullptr;)
			{
				_ShowTreeFromOneObjectToAnother(Object2,Object1);
				Object2 = Object2->sibling;
			}
		}
		_HideAllGunsights(Object->child);
	}

	// just to be sure
//	ogrpFindSubObjectByName(&Object,BaseObject,"both");

//	if (Object != nullptr)
//	{
//		ogrpSetObjectAndSubtreeToDraw(Object);
//	}

	return 0;
}

static UINT32 _ShowTreeFromOneObjectToAnother(OBJECT * CurrentObject, OBJECT * SearchObject)
{
OBJECT* ObjSib;

	if (CurrentObject == nullptr)
	{
		return 0;
	}
	if (CurrentObject->child == nullptr)
	{
		return 0;
	}

	ObjSib = CurrentObject->child;

	do
	{
		if (ObjSib->child != nullptr)
		{
			if(_FindSubObjectInSubTree(ObjSib->child,SearchObject) == 1)
			{
				ogrpShowObjectAndSubtree(ObjSib);
				return 0;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _FindSubObjectInSubTree(OBJECT * Object, OBJECT * SearchObject)
{
OBJECT * ObjSib;

	ObjSib = Object;

	do
	{
		if (ObjSib == SearchObject)
		{
			return 1;
		}

		if (ObjSib->child != nullptr)
		{
			if (_FindSubObjectInSubTree(ObjSib->child,SearchObject) == 1)
			{
				return 1;
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

UINT32 mlObjectSetExternObject(OBJECT * BaseObject)
{
	OBJECT * Object;

	// show external object
	ogrpFindSubObjectByName(&Object,BaseObject,"extern");
	if (Object != nullptr)
		ogrpShowObjectAndSubtree(Object);

	// now hide the in-plane object
	ogrpFindSubObjectByName(&Object,BaseObject,"vcpit");
	if (Object != nullptr)
		ogrpHideObjectAndSubtree(Object);

	return 0;
}

static LOGICAL _TurretConvergenceCallback(OBJECT* obj, UINT32 user)
{
	mlObjectData* VehicleObjectData = (mlObjectData*)user;
	char* ptr = nullptr;
	int turretnum = 0, turretsubnum = 0;

	obmainFindCrackedCommentFirstPart(&ptr, obj->comment, "ttraverseconverge");
	if(ptr)
	{
		sscanf(ptr, "ttraverseconverge%d_%d", &turretnum, &turretsubnum);
		if( turretnum > 0 && turretnum <= ML_MAX_CREW_POSITIONS &&
			turretsubnum > 0 && turretsubnum <= VEH_MAX_MOUNTS && obj->child)
		{
			// child should be elevate object which is the position we want for convergence calculations
			VehicleObjectData->TurretConvObj[turretnum-1][turretsubnum-1] = obj->child;
			ogrpFindSubObjectByDOFVarAndSubVar(&VehicleObjectData->FirePoint[turretnum-1][turretsubnum-1],obj,ML_DOF_CONNECT,0);
			return TRUE;
		}
	}

	return FALSE;
}

static LOGICAL _FireDirectorCallback(OBJECT* obj, UINT32 user)
{
	mlObjectData* VehicleObjectData = (mlObjectData*)user;
	char* ptr = nullptr;
	int turretnum = 0;

	obmainFindCrackedCommentFirstPart(&ptr, obj->comment, "firedirector");
	if(ptr)
	{
		sscanf(ptr, "firedirector%d", &turretnum);

		if(turretnum > 0 && turretnum <= ML_MAX_CREW_POSITIONS)
		{
			VehicleObjectData->FireDirectorObj[turretnum-1] = obj;
			return TRUE;
		}
	}

	return FALSE;
}

#define MAX_CAGE_LINES 128
UINT32 mlSetModelSpecialPoints(void*  InVehicleData,mlObjectData * VehicleObjectData)
{
	mlComponent* Component;
	UINT32 i,j;
	OBJECT* Object;
	mlVehicleData* VehicleData;
	UINT32 NumCageLines;
	piVECTOR CageLineV1[MAX_CAGE_LINES];
	piVECTOR CageLineV2[MAX_CAGE_LINES];
	OBJECT*  CageLineObject[MAX_CAGE_LINES];
	UINT32 Count, Same;
	mlComponentListIterator it;

	VehicleData = (mlVehicleData*)InVehicleData;
	if (VehicleData == nullptr) return 0;

	VehicleData->VehicleObjectData = nullptr;
	if (VehicleObjectData == nullptr) return 0;

	Object = VehicleObjectData->PlaneObject;
	if (Object == nullptr) return 0;

	VehicleData->VehicleObjectData = VehicleObjectData;
	if (VehicleData->ComponentList == nullptr) return 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		switch (Component->Type)
		{
			case ML_COMPONENT_AIRFOIL:
				_ProcessObjectWithAirfoilComponent(Component,Object);
				break;
			case ML_COMPONENT_ENGINE:
				_ProcessObjectWithEngineComponent(Component,Object);
				break;
			case ML_COMPONENT_GEAR:
				_ProcessObjectWithGearComponent(Component,Object);
				break;
			case ML_COMPONENT_WEAPONMOUNT:
				_ProcessObjectWithWeaponComponent(Component,Object);
				break;

			default:
				break;

		}
	}

	// handle cage collider stuff
	if (VehicleObjectData->HitBox != nullptr)
	{
		NumCageLines = 0;
		memset(CageLineObject,0,sizeof(OBJECT*) * MAX_CAGE_LINES);
		_CountAndSetupCageLines(&NumCageLines,CageLineV1,CageLineV2,CageLineObject,
								VehicleObjectData->HitBox,0);

		for (i=0;i<NumCageLines;i++)
		{
			for (j=i+1;j<NumCageLines;j++)
			{
				if (CageLineObject[j] == nullptr)
				{
					continue;
				}

				Same = 0;
				if (fabs(CageLineV1[i].x - CageLineV1[j].x) < 0.001f &&
					fabs(CageLineV1[i].y - CageLineV1[j].y) < 0.001f &&
					fabs(CageLineV1[i].z - CageLineV1[j].z) < 0.001f &&
					fabs(CageLineV2[i].x - CageLineV2[j].x) < 0.001f &&
					fabs(CageLineV2[i].y - CageLineV2[j].y) < 0.001f &&
					fabs(CageLineV2[i].z - CageLineV2[j].z) < 0.001f)
				{
					Same = 1;
				}

				if (fabs(CageLineV1[i].x - CageLineV2[j].x) < 0.001f &&
					fabs(CageLineV1[i].y - CageLineV2[j].y) < 0.001f &&
					fabs(CageLineV1[i].z - CageLineV2[j].z) < 0.001f &&
					fabs(CageLineV2[i].x - CageLineV1[j].x) < 0.001f &&
					fabs(CageLineV2[i].y - CageLineV1[j].y) < 0.001f &&
					fabs(CageLineV2[i].z - CageLineV1[j].z) < 0.001f)
				{
					Same = 1;
				}

				if (Same == 1)
				{
					CageLineObject[j] = nullptr;
				}
			}
		}
		Count = 0;

		for (i=0;i<NumCageLines;i++)
		{
			if (CageLineObject[i] != nullptr)
			{
				Count++;
			}
		}
		VehicleData->NumCageCollideLines = Count;

		VehicleData->CageLineV1 = (piVECTOR*)memAlloc(Count * sizeof(piVECTOR));
		VehicleData->CageLineV2 = (piVECTOR*)memAlloc(Count * sizeof(piVECTOR));
		VehicleData->CageLineObject = (OBJECT**)memAlloc(Count * sizeof(OBJECT*));

		Count = 0;
		for (i=0;i<NumCageLines;i++)
		{
			if (CageLineObject[i] != nullptr)
			{
				VehicleData->CageLineV1[Count] = CageLineV1[i];
				VehicleData->CageLineV2[Count] = CageLineV2[i];
				VehicleData->CageLineObject[Count] = CageLineObject[i];

				Count++;
			}
		}
	}

	// player's props are drawn specially
	for (j=0;j<ML_MAX_ENGINES;j++)
	{
		for (i=0;i<VehicleObjectData->EngineFastCount[j];i++)
		{
			obmainSetFlag2InThisTree(VehicleObjectData->EngineFastGroup[j][i],OBJECT_HIDDEN);
		}
	}

	// setup turret convergence
	if(VehicleObjectData->LoadoutLODObject)
	{
		LOGICAL foundDirector = obmainFindCommentFirstPartCallback(VehicleObjectData->LoadoutLODObject->child, "firedirector", (UINT32) VehicleObjectData, _FireDirectorCallback);

		if(foundDirector)
		{
			obmainFindCommentFirstPartCallback(VehicleObjectData->LoadoutLODObject->child, "ttraverseconverge", (UINT32)VehicleObjectData, _TurretConvergenceCallback);

			VehicleData->HasFireDirector = TRUE;

			// DOF min/max is setup screwy for limited traverse turrets
			for(i = 0; i < ML_MAX_CREW_POSITIONS; i++)
			{
				OBJECT* TurretTraverseDOF;
				ogrpFindSubObjectByDOFVarAndSubVar(&TurretTraverseDOF, VehicleObjectData->LoadoutLODObject, ML_DOF_ELEMENT_TTRAVERSE, i);
				if(TurretTraverseDOF && TurretTraverseDOF->dof && TurretTraverseDOF->dof->dofmaxval < TurretTraverseDOF->dof->dofminval)
					VehicleObjectData->TraverseReversed[i] = TRUE;
			}
		}
	}

	return 0;
}

static UINT32 _CountAndSetupCageLines(UINT32 * NumCageLines,
									  piVECTOR * CageLineV1,
									  piVECTOR * CageLineV2,
									  OBJECT ** CageLineObject,
									  OBJECT * Object,
									  UINT32 IncludeSibs)
{
OBJECT * ObjSib;
OBJECT * ObjAtt;
UINT32 i;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				piVectorConstPtr pos = ObjAtt->RenderPositions();

				for (i=0;i<ObjAtt->numtris;i++)
				{
					if (*NumCageLines < MAX_CAGE_LINES - 3)
					{
						CageLineV1[*NumCageLines] = pos[ObjAtt->ptris[i].v1];
						CageLineV2[*NumCageLines] = pos[ObjAtt->ptris[i].v2];
						CageLineObject[*NumCageLines] = ObjAtt;
						(*NumCageLines)++;

						CageLineV1[*NumCageLines] = pos[ObjAtt->ptris[i].v2];
						CageLineV2[*NumCageLines] = pos[ObjAtt->ptris[i].v3];
						CageLineObject[*NumCageLines] = ObjAtt;
						(*NumCageLines)++;

						CageLineV1[*NumCageLines] = pos[ObjAtt->ptris[i].v3];
						CageLineV2[*NumCageLines] = pos[ObjAtt->ptris[i].v1];
						CageLineObject[*NumCageLines] = ObjAtt;
						(*NumCageLines)++;
					}
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}


		if (ObjSib->child != nullptr)
		{
			_CountAndSetupCageLines(NumCageLines,
									CageLineV1,
									CageLineV2,
									CageLineObject,
									ObjSib->child,
									1);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && IncludeSibs != 0);

	return 0;
}


static UINT32 _ProcessObjectWithAirfoilComponent(mlComponent * Component,OBJECT * Object)
{

	return 0;
}

static UINT32 _ProcessObjectWithEngineComponent(mlComponent * Component,OBJECT * Object)
{

	return 0;
}

static UINT32 _ProcessObjectWithWeaponComponent(mlComponent * Component,OBJECT * Object)
{
mlWeaponMount* Mount;
UINT32 i;
//OBJECT * Object1;
OBJECT * AttachPnt;

	Mount = (mlWeaponMount*)(Component->SpecialData);
//	hackUnlockBlock(Mount);

	// first find the mount itself, bail if we can't find it
	ogrpFindSubObjectByName(&Mount->MountObject,Object,(char*)Mount->MountName);
	if (Mount->MountObject == nullptr)
	{
		WriteLog(LOG_WEAPON, LOG_DEBUG,"Unable to find weapon mount %s",Mount->MountName);
//		hackLockBlock(Mount);
		return 0;
	}

	// ok now let's import some weapons
	for (i=0;i<Mount->NumberOfWeaponsInMount;i++)
	{
		// first priority is to locate a connection point, no connection point, no weapon
		ogrpFindSubObjectByDOFVarAndSubVar(&AttachPnt,Mount->MountObject,ML_DOF_CONNECT,i);

		Mount->WeaponConnectPoints[i] = AttachPnt;

		// now save fire point
		ogrpFindSubObjectByDOFVar(&Mount->WeaponFirePoints[i],AttachPnt,ML_DOF_FIREPOINT);

		// now save crap point
		ogrpFindSubObjectByDOFVar(&Mount->WeaponCrapPoints[i],AttachPnt,ML_DOF_CRAPPOINT);

		// now hide fire and crap
		if(Mount->WeaponFirePoints[i])
			ogrpHideObjectAndSubtree(Mount->WeaponFirePoints[i]);

		if(Mount->WeaponCrapPoints[i])
			ogrpHideObjectAndSubtree(Mount->WeaponCrapPoints[i]);
	}

//	hackLockBlock(Mount);

	return 0;
}

static UINT32 _ProcessObjectWithGearComponent(mlComponent * Component,OBJECT * Object)
{
mlGearComponentData * GearData;
UINT32 NumLeft;
UINT32 GearNum;
UINT32 Index;
OBJECT * LODObject;

	GearData = (mlGearComponentData*)(Component->SpecialData);

	// find contact point data
	GearNum = GearData->GearNum;

	NumLeft = ML_MAX_GEAR_CONTACTPOINTS;

	Index = 0;
	_FindAllContactPoints(&(GearData->ContactPoint),&Index,&NumLeft,GearNum,Object);

	GearData->NumContactPoints = ML_MAX_GEAR_CONTACTPOINTS - NumLeft;

	// find the shock DOF, if it exists
	ogrpFindSubObjectByName(&LODObject,Object,"lod1");
	if (LODObject == nullptr)
	{
		LODObject = Object;
	}
	ogrpFindSubObjectByDOFVarAndSubVar(&(GearData->ShockDOF),LODObject,ML_DOF_WHEEL_STRUT,GearNum-1);
	ogrpFindSubObjectByDOFVarAndSubVar(&(GearData->CastorDOF),LODObject,ML_DOF_WHEEL_CASTOR,GearNum-1);



	return 0;
}

static UINT32 _ImportAndSetupWeapons(mlObjectData* VehicleObjectData, vehType const* VehicleType)
{
UINT32 i,j;
OBJECT * MountObject;
OBJECT * AttachPnt;
OBJECT * Object1;
OBJECT* MasterObj = VehicleObjectData->PlaneObject;
vehLoadout const* Loadout;
piMATRIX Mat;
UINT32 LoadoutNum = 0;

	if (VehicleType == nullptr)
	{
		WriteLog(LOG_WEAPON, LOG_DEBUG,"Vehicle exists with no vehicletype!");

		return 0;
	}

	Loadout = &(VehicleType->Loadout[LoadoutNum]);

	for (i=0;i<VEH_MAX_MOUNTS;i++)
	{
		if (Loadout->WeaponTypes[i][LoadoutNum] != nullptr)
		{
			ogrpFindSubObjectByName(&MountObject,MasterObj,
				(char*)Loadout->MountObject[i]);

			if (MountObject == nullptr)
			{
				WriteLog(LOG_WEAPON, LOG_DEBUG,"Unable to find %s in %s",
							Loadout->MountObject[i],
							MasterObj->name);
			}
			else
			{
				for (j=0;j<VEH_MAX_WEAPONS_PER_MOUNT;j++)
				{
					if (Loadout->WeaponTypes[i][j] != nullptr)
					{
						// first priority is to locate a connection point, no connection point, no weapon
						ogrpFindSubObjectByDOFVarAndSubVar(&AttachPnt,MountObject,ML_DOF_CONNECT,j);
						if (AttachPnt == nullptr)
						{
							WriteLog(LOG_WEAPON, LOG_DEBUG,"Connection point %d for mount %s doesn't exist",
								j+1,
								Loadout->MountObject[i]);
							continue;
						}

						Object1 = obmgrCreateModelInstance((char*)Loadout->WeaponObject[i][j]);
						if (!Object1)
						{
							WriteLog(LOG_WEAPON, LOG_DEBUG,"Unable to load object %s for mount %s",
								Loadout->WeaponObject[i][j],Loadout->MountObject[i]);
							continue;
						}
						strprintf(Object1->name, "%sn%d", Loadout->WeaponObject[i][j],_WeaponCount++);

						// insert object at connection point
						ogrpMakeObjectAChildOfAnotherObject(AttachPnt,Object1);

						// ensure object's DOFs are active
						ogrpParseDOFParameters(Object1,(UCHAR*)(&mlDOFVarNameTable[0][0]),ML_DOF_MAXNUM,32);

						// now move weapon's vertices to new position
						// use the attachment point matrix for this operation
						Mat = IdentityMatrix;
						Mat._11 = -1;
						math3dMatrixMultiply(&Mat,&Mat,&AttachPnt->dof->dofmatrix2);
						_RelocateObject(Object1,&Mat);

						// now save the fire point
						VehicleObjectData->WeaponConnectPoints[i][j] = AttachPnt;
						ogrpFindSubObjectByDOFVar(&VehicleObjectData->WeaponFirePoints[i][j],AttachPnt,ML_DOF_FIREPOINT);
					}
				}
			}
		}
	}

	return 0;
}


static UINT32 _SetupVehicleTracks(mlObjectTrackSection ** TrackSectionList,
								  UINT32 * Count,
								  mlObjectTrackAnimation ** TrackAnimationList,
								  UINT32 * AnimCount,
								  OBJECT * Object)
{
mlObjectTrackSection * TrackSection;
mlObjectTrackAnimation * TrackAnimation;


	// moving track part
	_CountTrackSections(Count,Object);


	if (*Count == 0)
	{
		// no track sections
		*TrackSectionList = nullptr;
	}
	else
	{

		*TrackSectionList = (mlObjectTrackSection*)memAlloc(sizeof(mlObjectTrackSection) * *Count);

		TrackSection = *TrackSectionList;
		_SetupTracks(&TrackSection,Object,Object);
	}

	// texture animation part
	_CountTrackAnimations(AnimCount,Object);
	if (*AnimCount == 0)
	{
		*TrackAnimationList = nullptr;
	}
	else
	{
		*TrackAnimationList = (mlObjectTrackAnimation*)memAlloc(sizeof(mlObjectTrackAnimation) * *AnimCount);

		TrackAnimation = *TrackAnimationList;
		_SetupTrackAnimations(&TrackAnimation,Object,Object);
	}

	return 0;
}

static UINT32 _CountTrackAnimations(UINT32 * Count, OBJECT * Object)
{
UCHAR* CommentStr;
OBJECT* ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"trackmove") == TRUE)
		{
			(*Count)++;
		}

		if (ObjSib->child != nullptr)
		{
			_CountTrackAnimations(Count,ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _CountTrackSections(UINT32 * Count, OBJECT * Object)
{
UCHAR* CommentStr;
OBJECT* ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"connected") == TRUE)
		{
			(*Count)++;
		}

		if (ObjSib->child != nullptr)
		{
			_CountTrackSections(Count,ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _SetupTrackAnimations(mlObjectTrackAnimation ** TrackAnimation,
									OBJECT * Object,
									OBJECT * OriginalObject)
{
UCHAR* CommentStr;
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{

		if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"trackmove") == TRUE)
		{

			sscanf((char*)CommentStr," %d %f",&((*TrackAnimation)->TrackNum),&((*TrackAnimation)->TrackMoveRate));
			(*TrackAnimation)->TrackMoveRate = 1.0f / (*TrackAnimation)->TrackMoveRate;

			(*TrackAnimation)->TrackAnimationObject = ObjSib;

			(*TrackAnimation)++;
		}

		if (ObjSib->child != nullptr)
		{
			_SetupTrackAnimations(TrackAnimation,ObjSib->child,OriginalObject);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _SetupTracks(mlObjectTrackSection ** TrackSection,
						   OBJECT * Object,
						   OBJECT * OriginalObject)
{
UCHAR* CommentStr;
UCHAR LinkObject[32];
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{

		if (obmainFindCommentName((SINT8 **)&CommentStr,ObjSib->comment,"connected") == TRUE)
		{
			LinkObject[0] = '\0';

			sscanf((char*)CommentStr," %s",LinkObject);

			if (strlen((char*)LinkObject) > 0)
			{
				_SetupTrackSection(TrackSection,LinkObject,ObjSib,OriginalObject);
				(*TrackSection)++;
			}
		}

		if (ObjSib->child != nullptr)
		{
			_SetupTracks(TrackSection,ObjSib->child,OriginalObject);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _SetupTrackSection(mlObjectTrackSection ** TrackSection,
								 UCHAR * LinkObject,
								 OBJECT * Object,
								 OBJECT * OriginalObject)
{
OBJECT * LinkObjectPtr;
UINT32 VertexCount;
UINT32 i,j;
FLOAT32 Threshold;
piVECTOR Vertex1,Vertex2;
piVECTOR TempVec;

	if (strlen((char*)LinkObject) == 0)
	{
		return 0;
	}

	// first find the link object
	ogrpFindSubObjectByName(&LinkObjectPtr,OriginalObject,(char*)LinkObject);

	if (LinkObjectPtr == nullptr)
	{
		return 0;
	}

	// ok we have the link object
	(*TrackSection)->TrackSegment = Object;
	(*TrackSection)->TrackSegmentObjectToLinkTo = LinkObjectPtr;

	// find all vertices that "share" space
	VertexCount = 0;
	Threshold = (FLOAT32)(0.01 * ScaleFactor); // 1cm accuracy

	piVectorConstPtr pos = Object->ImmutablePositions();
	piVectorConstPtr linkPos = LinkObjectPtr->ImmutablePositions();

	for (i=0;i<Object->numverts;i++)
	{
		if (VertexCount >= ML_MAX_TRACK_VERTICES)
		{
			break;
		}

		Vertex1 = pos[i];

		for (j=0;j<LinkObjectPtr->numverts;j++)
		{
			if (VertexCount >= ML_MAX_TRACK_VERTICES)
			{
				break;
			}

			// see if vertices are within 1cm
			Vertex2 = linkPos[j];

			mathSubVectors(&TempVec,&Vertex1,&Vertex2);
			if (TempVec.x > -Threshold && TempVec.x < Threshold &&
				TempVec.y > -Threshold && TempVec.y < Threshold &&
				TempVec.z > -Threshold && TempVec.z < Threshold)
			{
				(*TrackSection)->LinkToVertex[VertexCount][0] = i;
				(*TrackSection)->LinkToVertex[VertexCount][1] = j;
				VertexCount++;
				break;
			}

		}

	}
	(*TrackSection)->NumVertices = VertexCount;

	return 0;
}
// scans thru all siblings and children until NumLeft is zero or out of objects
static UINT32 _FindAllContactPoints(OBJECT ** ObjectPtrArray,UINT32* Index,UINT32 * NumLeft,UINT32 GearNum,OBJECT * Object)
{

	if (Object == nullptr)
	{
		return 0;
	}

	if (*NumLeft <= 0)
	{
		return 0;
	}

	if (Object->flags & OBJECT_DOF)
	{
		if (Object->dof->dofvarnum == ML_DOF_WHEEL_CONTACTPOINT && Object->dof->dofvarsubnum == GearNum - 1)
		{
			ObjectPtrArray[*Index] = Object;
			(*Index)++;
			(*NumLeft)--;
		}
	}

	if (Object->child != nullptr)
	{
		_FindAllContactPoints(ObjectPtrArray,Index,NumLeft,GearNum,Object->child);
	}

	if (Object->sibling != nullptr)
	{
		_FindAllContactPoints(ObjectPtrArray,Index,NumLeft,GearNum,Object->sibling);
	}

	return 0;
}


static UINT32 _RelocateObject(OBJECT * Object,piMATRIX * Mat)
{
UINT32 j;
OBJECT * Object1;

	if (Object == nullptr)
	{
		return 0;
	}

	if (Object->flags & OBJECT_DOF)
	{
	piMATRIX InvMat;

		// adjust dof matrices
		math3dInvertMatrix(&InvMat,Mat);

		if (Object->dof != nullptr)
		{
			math3dMatrixMultiply(&Object->dof->dofmatrix1,&InvMat,&Object->dof->dofmatrix1);
			math3dMatrixMultiply(&Object->dof->dofmatrix2,&Object->dof->dofmatrix2,Mat);
		}
	}
	else if(!(Object->flags & OBJECT_NOTOBJECT))
	{
		// adjust object's vertecies
		for (Object1 = Object;Object1 != nullptr;Object1 = Object1->attached)
		{
			piVectorPtr pos = Object1->MutablePositions();

			for (j=0;j<Object1->numverts;j++, ++pos)
			{
				math3dVectorMultiply(pos.ptr,pos.ptr,Mat);
			}
			//ldmdlCalcSurfaceNormals(Object1);
		}
	}

	if (Object->child != nullptr)
	{
		_RelocateObject(Object->child,Mat);
	}
	if (Object->sibling != nullptr)
	{
		_RelocateObject(Object->sibling,Mat);
	}

	return 0;
}

// "IncludeSibs" is used to allow selection of a top-level object that has sibs,
// but allowing the algorithm to look at only that object's subtree
//
// NOTE: Radius is actually radius squared!
static UINT32 _CalculateObjectTreeRadius(FLOAT32 * Radius,OBJECT * Object, UINT32 IncludeSibs)
{
OBJECT * ObjectSib,*ObjectAtt;
UINT32 i;
FLOAT32 DistSqr;
piVectorConstPtr Vert;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjectSib = Object;

	do
	{
		ObjectAtt = ObjectSib;

		do
		{
			if (!(ObjectAtt->flags & OBJECT_NOTOBJECT))
			{
				Vert = ObjectAtt->RenderPositions();
				for (i=0;i<ObjectAtt->numverts;i++)
				{
					DistSqr = Vert->x * Vert->x + Vert->y * Vert->y + Vert->z * Vert->z;

					if (*Radius < DistSqr)
					{
						*Radius = DistSqr;
					}
					++Vert;
				}

			}


			ObjectAtt = ObjectAtt->attached;
		}
		while (ObjectAtt != nullptr);


		if (ObjectSib->child != nullptr)
		{
			_CalculateObjectTreeRadius(Radius,ObjectSib->child,1);
		}

		ObjectSib = ObjectSib->sibling;
	}
	while (ObjectSib != nullptr && IncludeSibs != 0);


	return 0;
}

UINT32 mlApplyVelocityToObject(OBJECT * PlaneObject,FLOAT64VECTOR * Vel)
{
OBJECT * UseObject;

	if (PlaneObject == nullptr)
	{
		return 0;
	}

	UseObject = PlaneObject;

	while (UseObject->child != nullptr)
	{
		UseObject = UseObject->child;
	}

	UseObject->vel.x = (FLOAT32)(Vel->x);
	UseObject->vel.y = (FLOAT32)(Vel->y);
	UseObject->vel.z = (FLOAT32)(Vel->z);

	return 0;
}

UINT32 mlProcessContactPointObject(mlObjectData* ObjectData)
{
	UINT32 NumCollideObjects;
	mlObjectCollidePnt* CurrentCollidePnt;
	OBJECT* LineCollideObject;

	if (ObjectData == nullptr) return 1;

	LineCollideObject = ObjectData->LineCollideObject;
	if (LineCollideObject == nullptr) return 0;

	NumCollideObjects = 0;
	_CountLineCollideSubObjects(&NumCollideObjects,LineCollideObject->child);

	if(ObjectData->CollidePntList)
		memFree(ObjectData->CollidePntList);

	ObjectData->NumCollideObjects = NumCollideObjects;
	ObjectData->CollidePntList = (mlObjectCollidePnt*)memAlloc(sizeof(mlObjectCollidePnt) * NumCollideObjects);

	CurrentCollidePnt = ObjectData->CollidePntList;
	_SetupCollisionObjects(&CurrentCollidePnt,LineCollideObject->child);

	ObjectData->ProcessContactPointObject = FALSE;

	return 0;
}

static UINT32 mlProcessHoistContactPointObject(mlObjectData* ObjectData)
{
	UINT32 NumCollideObjects;
	mlObjectCollidePnt* CurrentCollidePnt;
	OBJECT* LineCollideObject;

	LineCollideObject = ObjectData->HoistLineCollideObject;

	NumCollideObjects = 0;
	_CountLineCollideSubObjects(&NumCollideObjects,LineCollideObject->child);

	ObjectData->NumHoistCollideObjects = NumCollideObjects;
	ObjectData->HoistCollidePntList = (mlObjectCollidePnt*)memAlloc(sizeof(mlObjectCollidePnt) * NumCollideObjects);

	CurrentCollidePnt = ObjectData->HoistCollidePntList;
	_SetupCollisionObjects(&CurrentCollidePnt,LineCollideObject->child);

	return 0;
}


static UINT32 _CountLineCollideSubObjects(UINT32 * Count, OBJECT * Object)
{
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}
	if (Count == nullptr)
	{
		return 1;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flag2 & OBJECT_MISSING) && ObjSib->dof &&
			(ObjSib->dof->dofvarnum == ML_DOF_POINTCOLLIDE || ObjSib->dof->dofvarnum == ML_DOF_WHEEL_CONTACTPOINT) )
		{
			(*Count)++;
		}

		if (ObjSib->child != nullptr)
		{
			_CountLineCollideSubObjects(Count,ObjSib->child);
		}

		ObjSib = ObjSib->sibling;

	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _SetupCollisionObjects(mlObjectCollidePnt ** CurrentCollidePnt,OBJECT * Object)
{
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	if (CurrentCollidePnt == nullptr)
	{
		return 1;
	}

	ObjSib = Object;

	do
	{
		if(!(ObjSib->flag2 & OBJECT_MISSING) && ObjSib->dof &&
			(ObjSib->dof->dofvarnum == ML_DOF_POINTCOLLIDE ||
			 ObjSib->dof->dofvarnum == ML_DOF_WHEEL_CONTACTPOINT) )
		{
			(*CurrentCollidePnt)->CollideObject = ObjSib;

			char* str = nullptr;

			if(obmainFindCommentFirstPartName(&str, ObjSib->comment, "visdamage=") && str)
			{
				str = strchr(str, '=');
				if(str)
				{
					str++;
					strncpy((*CurrentCollidePnt)->VisDamage, str, 31);
				}
			}

			if(obmainFindCommentName(&str, ObjSib->comment, "spike") && str)
			{
				(*CurrentCollidePnt)->IsSpike = TRUE;
			}
			(*CurrentCollidePnt)++;
		}

		if (ObjSib->child)
			_SetupCollisionObjects(CurrentCollidePnt,ObjSib->child);

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _FindAndSetupReflectorSightObjects(OBJECT * Object)
{
OBJECT * ObjAtt;
SCHAR * RetPtr;

	if (Object == nullptr)
	{
		return 0;
	}

	for(int i = 0; i < ML_MAX_CREW_POSITIONS; i++)
    {
        _ReflectorSights[i].flip = false;
		_ReflectorSights[i].inited = FALSE;
    }

	ObjAtt = Object;

	do
	{
		if (obmainFindCommentName(&RetPtr,ObjAtt->comment,"reflectorsight"))
		{
			ObjAtt->flags |= OBJECT_USERDRAW;
			ObjAtt->drawcallback = _OldDrawGunsightReticle;
		}else if (obmainFindCommentFirstPartName(&RetPtr,ObjAtt->comment,"reflectorsight"))
		{
			ObjAtt->flags |= OBJECT_USERDRAW;
			ObjAtt->drawcallback = _DrawGunsightReticle;
		}
		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != nullptr);

	if (Object->child != nullptr)
	{
		_FindAndSetupReflectorSightSubObjects(Object->child);
	}

	return 0;
}

static UINT32 _FindAndSetupReflectorSightSubObjects(OBJECT * Object)
{
OBJECT * ObjAtt,*ObjSib;
SCHAR * RetPtr;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		do
		{
			if (obmainFindCommentName(&RetPtr,ObjAtt->comment,"reflectorsight"))
			{
				ObjAtt->flags |= OBJECT_USERDRAW;
				ObjAtt->drawcallback = _OldDrawGunsightReticle;
			}else if (obmainFindCommentFirstPartName(&RetPtr,ObjAtt->comment,"reflectorsight"))
			{
				ObjAtt->flags |= OBJECT_USERDRAW;
				ObjAtt->drawcallback = _DrawGunsightReticle;
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != nullptr);

		if (ObjSib->child != nullptr)
		{
			_FindAndSetupReflectorSightSubObjects(ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static void _InitReflectorSight(OBJECT* Object, int crewpos)
{
	OBJECT* PlaneObject = persGetPersonaVehicleObjectDataPtr();

	OBJECT* gviewObj = nullptr;
	char name[32];
	snprintf(name, 31, "gview%d", crewpos+1);
	ogrpFindSubObjectByName(&gviewObj,PlaneObject,name);

	piMATRIX mat = gviewObj->dof->dofmatrix2;
	piVECTOR up = {0,0,1};
	piVECTOR right = {1,0,0};
	piVECTOR front = {0,1,0};
	piVECTOR center;
	FLOAT32 width = -1;

	center.x = mat._41; center.y = mat._42; center.z = mat._43;
	mat._41 = mat._42 = mat._43 = 0;

	math3dVectorMultiply(&up, &up, &mat);
	math3dVectorMultiply(&right, &right, &mat);
	math3dVectorMultiply(&front, &front, &mat);

	piVectorConstPtr pos 	= Object->RenderPositions();
	for(int i = 0; i < Object->numverts; i++, ++pos)
	{
		piVECTOR r;
		mathSubVectors(&r, pos.ptr, &center);
		float x = fabs(math3dDotProduct(&r, &right));
		if(x > width)
			width = 2*x;
	}

    TEXTURE* t = piGetTextureData(Object->texid);

    oglSetActiveTexture(0, Object->texid);

    piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_CLAMP);
    piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_CLAMP);
    piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);

    oglDisableTexture();

    if(t && t->isdds())
    {
        _ReflectorSights[crewpos].flip = true;
    }

	_ReflectorSights[crewpos].up = up;
	_ReflectorSights[crewpos].right = right;
	_ReflectorSights[crewpos].front = front;
	_ReflectorSights[crewpos].center = center;
	_ReflectorSights[crewpos].width = width;
	_ReflectorSights[crewpos].gviewObj = gviewObj;
	_ReflectorSights[crewpos].inited = TRUE;
}

static bool _DrawGunsightReticle(OBJECT * Object)
{
	char* s;

	obmainFindCommentFirstPartName(&s,Object->comment,"reflectorsight");
	s += sizeof("reflectorsight") - 1;

	int crewpos = atoi(s) - 1;
	if(crewpos < 0 || crewpos >= ML_MAX_CREW_POSITIONS)
		crewpos = 0;

	ReflectorSight* sight =	&_ReflectorSights[crewpos];

	if(!sight->inited)
		_InitReflectorSight(Object, crewpos);

	piVECTOR up, right, r;
	up = _ReflectorSights[crewpos].up;
	right = _ReflectorSights[crewpos].right;

	piVECTOR pp;
	pp.x = -playpos.x; pp.y = -playpos.y; pp.z = -playpos.z;

	piMATRIX mat = Object->mat;
	math3dInvertMatrix(&mat, &mat);
	math3dVectorMultiply(&pp, &pp, &mat);

	piVectorConstPtr pos   = Object->RenderPositions();
	piTexCoordPtr mtex     = Object->MutableTexCoords();

	for(int i = 0; i < Object->numverts; i++, ++mtex, ++pos)
	{
		mathSubVectors(&r, pos.ptr, &pp);

		mtex->u = math3dDotProduct(&r, &sight->right) / sight->width + .5f;
		mtex->v = math3dDotProduct(&r, &sight->up) / sight->width + .5f;
        mtex->v = sight->flip ? 1.0f - mtex->v : mtex->v;
	}

    obdrawDrawSingleObject(Object);

	return TRUE;
}

static bool _OldDrawGunsightReticle(OBJECT * Object)
{
UINT32 i;
FLOAT32 u,v;
piVECTOR pv,pp;
piVECTOR const* norm;
piVECTOR Up;
piVECTOR tempVec;
piMATRIX InvMat;
UINT32 Color;
FLOAT32 Len;

	pp.x = -playpos.x;
	pp.y = -playpos.y;
	pp.z = -playpos.z;

	Up.x = 0.0f;
	Up.y = 0.0f;
	Up.z = 1.0f;

	math3dInvertMatrix(&InvMat,&Object->mat);
	norm = &Object->psnorms[0];

	piVectorPtr pos 	= Object->MutablePositions();
	piTexCoordPtr tex 	= Object->MutableTexCoords();
	piVertexColorPtr clr= Object->MutableColors();

    TEXTURE* t = piGetTextureData(Object->texid);

	for (i=0;i<Object->numverts;i++, ++pos, ++tex, ++clr)
	{
		// calculate vector to vertex
		tempVec = *pos;
		math3dVectorMultiply(&pv,&pp,&InvMat);
		mathSubVectors(&pv,&pv,pos.ptr);
		Len = (FLOAT32)mathExactVectorLength(&pv);
		math3dScaleVector(&pv,&pv,1.0f/Len);

		Len = math3dDotProduct(&pv,norm);
		tempVec = *norm;
		math3dScaleVector(&tempVec,&tempVec,Len);

		mathSubVectors(&tempVec,&tempVec,&pv);
		math3dScaleVector(&tempVec,&tempVec,2.0f);
		mathAddVectors(&tempVec,&pv,&tempVec);

		math3dScaleVector(&tempVec,&tempVec,7.0f);

		// ok tempVec is now pointing along the reflected axis

		// next we compute the u/v coordinates needed
		u = tempVec.x + 0.5f;
		v = -tempVec.y + 0.5f;

		tex->u = u;
        tex->v = (t && t->isdds()) ? 1.0f - v : v;

		Color = clr->color;
		Color = piRGBA_SETALPHA(Color,127);
		clr->color = Color;
	}

    Object->material->Apply();

	piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_CLAMP);
	piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_CLAMP);
	piSetRenderStageState(0, piTSS_MINFILTER, piTFN_POINT);
    piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);
    piSetRenderStageState(1, piTSS_MIPFILTER, piTFP_NONE);

    oglSetWorldMatrix(&Object->mat);
    obdrawDrawMesh(Object);

    piSetRenderStageState(1, piTSS_MIPFILTER, TextureMipFilter);
    piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
	piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
	piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_WRAP);
	piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_WRAP);

	return TRUE;
}

static UINT32 _HideAllGunsights(OBJECT * Object)
{
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (strncmp(ObjSib->name,"gview",5) == 0 ||
			strncmp(ObjSib->name,"iview",5) == 0)
		{
			ogrpHideObjectAndSubtree(ObjSib);
			ObjSib = ObjSib->sibling;
			continue;
		}

		if (ObjSib->child != nullptr)
		{
			_HideAllGunsights(ObjSib->child);
		}
		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _IncrementObjectLayer(OBJECT * Object)
{
OBJECT * ObjAtt;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjAtt = Object;

	if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
	{
		do
		{
			if (ObjAtt->layer < 15)
			{
				ObjAtt->layer++;
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != nullptr);
	}

	if (Object->child != nullptr)
	{
		_IncrementSubObjectLayer(Object->child);
	}

	return 0;
}

static UINT32 _IncrementSubObjectLayer(OBJECT * Object)
{
OBJECT * ObjAtt;
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
		{
			do
			{
				if (ObjAtt->layer < 15)
				{
					ObjAtt->layer++;
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
		{
			_IncrementSubObjectLayer(ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

#include "trn/trnsky.h"
UINT32 mlDrawProps()
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	if (PlayerVehicle == nullptr ||
		PlayerVehicle->VehicleObjectData == nullptr ||
        PlayerVehicle->VehicleType->Type != VEH_TYPE_AIR)
	{
		return 0;
	}

	UINT32 i,j;
	OBJECT * Object;

	for (i=0;i<ML_MAX_ENGINES;i++)
	{
		for (j=0;j<PlayerVehicle->VehicleObjectData->EngineFastCount[i];j++)
		{
			Object = PlayerVehicle->VehicleObjectData->EngineFastGroup[i][j];

			_DisplayObjectTree(Object,0);
		}
	}

    oglSetWorldMatrix(&IdentityMatrix);

    return 0;
}

static UINT32 _DisplayObjectTree(OBJECT * Object,UINT32 IncludeSibs)
{
OBJECT * ObjAtt;
OBJECT * ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				if (ObjAtt->flags & OBJECT_VISIBLE &&
					!(ObjAtt->flags & OBJECT_SKIPDRAWING))
				{
                    if(!ogl::UseDeprecatedPath())
                    {
                        oglSetWorldMatrix(&ObjAtt->mat);
                        ObjAtt->material->Apply();
                    }

                    obdrawDrawObject(ObjAtt);
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
		{
			_DisplayObjectTree(ObjSib->child,1);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && IncludeSibs != 0);


	return 0;
}

static UINT32 _FindLowestHighestPoints(OBJECT* Object,FLOAT32* LowestPoint, FLOAT32* HighestPoint)
{
OBJECT * ObjSib, *ObjAtt;
UINT32 i;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				if (!(ObjAtt->flag2 & OBJECT_HIDDEN))
				{
					piVectorConstPtr pos = ObjAtt->RenderPositions();

					for (i=0;i<ObjAtt->numverts;i++, ++pos)
					{
						if (pos->z > *LowestPoint)
							*LowestPoint = pos->z;
						if (pos->z < *HighestPoint)
							*HighestPoint = pos->z;

					}
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);


		}

		if (ObjSib->child != nullptr)
		{
			_FindLowestHighestPoints(ObjSib->child,LowestPoint,HighestPoint);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);



	return 0;
}

UINT32 mlInitVehicleObject(mlObjectData ** ObjectData, vehType const* VehicleType, LOGICAL LoadCockpit)
{

OBJECT * Object, *Object1, *Object2;
SINT8* ptr, LocalObjectName[256];
UCHAR s[32];
UCHAR s1[32];
UINT32 i;
UINT32 Found;
UCHAR * DamageObjectListPtr;
OBJECT * TempObject;
static UINT32 Counter = 0;
UCHAR const* ObjectName = VehicleType->Shape;
UINT32 NumDamageObjects = VehicleType->NumDamageObjects;
//UINT32 LoadoutNum = 0;
UCHAR * DamageObjectList = (UCHAR*)&VehicleType->DamageObjects[0][0];

	// adjust external object layer value, to make external shapes show up better vs terrain
	_IncrementObjectLayer((*ObjectData)->PlaneObject);

	// find lowest point in the non-cockpit areas

	Object = nullptr;
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"lod1");
	if (Object == nullptr)
	{
		WriteLog(LOG_WEAPON, LOG_DEBUG,"Cannot find LOD1, using entire object for lowest point check");
		Object = (*ObjectData)->PlaneObject;
	}

	if(VehicleType->Type == VEH_TYPE_PERSON)
	{
		(*ObjectData)->LowestPoint = 0.0f;
		(*ObjectData)->HighestPoint = 2.0f;
	}
	else
	{
		(*ObjectData)->LowestPoint = -10000.0f; // z's are ass backwards
		(*ObjectData)->HighestPoint = 10000.0f; // in geometry
		_FindLowestHighestPoints(Object->child,&((*ObjectData)->LowestPoint),&((*ObjectData)->HighestPoint));
		(*ObjectData)->LowestPoint = -(*ObjectData)->LowestPoint; 	// *now* change to correct sign
		(*ObjectData)->HighestPoint = -(*ObjectData)->HighestPoint;
	}

	// now hide the in-plane object (default setting)
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"vcpit");


	if (Object != nullptr && LoadCockpit == TRUE)
	{
		// import cockpit, if necessary and desired
		strcpy((char*)s,(char*)ObjectName);
		strcat((char*)s,"_vcpit");

		if(obmgrCreateModelInstance((char*)s))
		{
			lstrcpy(LocalObjectName, (char*)s);
			ptr = strrchr(LocalObjectName, '\\');
			if (ptr != nullptr)
			{
				ptr += 1;
				lstrcpy(LocalObjectName, ptr);
			}

			Object1 = obmainFindNamedObject(LocalObjectName);
			if (Object1 != nullptr)
			{
				sprintf((char*)s1,"%s%d",LocalObjectName,Counter++);
				strcpy(Object1->name,(char*)s1);

				if (Object->child == nullptr)
				{
					Object->child = Object1;
				}
				else
				{
					Object2 = Object->child;

					while (Object2->sibling != nullptr)
					{
						Object2 = Object2->sibling;
					}

					Object2->sibling = Object1;
				}

			}
		}

		ogrpSetObjectAndSubtreeAsCockpit(Object);
		ogrpHideObjectAndSubtree(Object);
		_HideAllGunsights(Object->child);
	}
	Object = (*ObjectData)->PlaneObject;


	// now set up degree of freedom stuff
	ogrpParseDOFParameters(Object,(UCHAR*)(&mlDOFVarNameTable[0][0]),ML_DOF_MAXNUM,32);

    // Find mobile spawn object, if one exists
    obmainFindComment(&(*ObjectData)->spawnObject,
                       (*ObjectData)->PlaneObject,
                       "spawnobject");

	// read in track information for tracked vehicles
	_SetupVehicleTracks(&((*ObjectData)->TrackSection),
						&((*ObjectData)->TrackCount),
						&((*ObjectData)->TrackAnimation),
						&((*ObjectData)->TrackAnimationCount),
						Object);

	// find the collision objects
	Object = nullptr;
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"linecollider");
	(*ObjectData)->LineCollideObject = Object;

	if((*ObjectData)->LineCollideObject)
		obmainFindCommentFirstPartCallback((*ObjectData)->LineCollideObject, "visdamageon", 0, _MissingCallback);

	mlProcessContactPointObject(*ObjectData);

	Object = nullptr;
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"hitbox");
	(*ObjectData)->HitBox = Object;

	Object = nullptr;
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"hitmap");
	(*ObjectData)->HitMap = Object;
	if (Object != nullptr)
	{
		Object1 = nullptr;
		ogrpFindSubObjectByName(&Object1,Object,"inside");
		(*ObjectData)->CollideInternObject = Object1;

		Object1 = nullptr;
		ogrpFindSubObjectByName(&Object1,Object,"outside");
		(*ObjectData)->CollideExternObject = Object1;

		Object1 = nullptr;
		ogrpFindSubObjectByName(&Object1,Object,"walk");
		{
			(*ObjectData)->CollideWalkObject = Object1;
			obmainSetFlag3InThisTree((*ObjectData)->CollideWalkObject, OBJECT_WALK);
		}

		(*ObjectData)->Radius = 0;

		_CalculateObjectTreeRadius(&((*ObjectData)->Radius),Object,0);
		(*ObjectData)->Radius = (FLOAT32)(sqrt((*ObjectData)->Radius) + 1.0); // margin of error
	}
	else
	{
		(*ObjectData)->CollideInternObject = nullptr;
		(*ObjectData)->CollideExternObject = nullptr;
		(*ObjectData)->Radius = 0.0;
		if ((*ObjectData)->PlaneObject->flag2 & OBJECT_ANIMATION)
		{
			(*ObjectData)->Radius = 3.0;
		}

	}


	Object = nullptr;
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"efx");
	if (Object != nullptr)
	{
		(*ObjectData)->EffectsObject = Object;
	}
	else
	{
		(*ObjectData)->EffectsObject = nullptr;
	}

	DamageObjectListPtr = DamageObjectList;
	for (i=1;i<=NumDamageObjects;i++)
	{
		Found = 0;
		if ((*ObjectData)->CollideExternObject != nullptr)
		{
			ogrpFindSubObjectByName(&Object,
								(*ObjectData)->CollideExternObject,
								(char*)DamageObjectListPtr);

			if (Object != nullptr)
			{
				Found = 1;
			}
		}
		if (Found == 0 && (*ObjectData)->CollideInternObject != nullptr)
		{
			ogrpFindSubObjectByName(&Object,
								(*ObjectData)->CollideInternObject,
								(char*)DamageObjectListPtr);

			if (Object != nullptr)
			{
				Found = 1;
			}
		}
		if (Found == 1)
		{
			for (TempObject = Object;TempObject != nullptr;TempObject = TempObject->attached)
			{
				TempObject->trnum = i; // using trnum for damage ID number
			}
		}
		else
		{
			WriteLog(LOG_WEAPON, LOG_DEBUG,"Unable to find damage object %s in object %s",
							DamageObjectListPtr,
							(*ObjectData)->PlaneObject->name);
		}

		DamageObjectListPtr += 32;
	}

	// we hide all the non-visible objects to ensure they don't get sorted
	ogrpFindSubObjectByName(&Object,(*ObjectData)->PlaneObject,"lodc");

	if (Object != nullptr)
	{
		ogrpHideObjectAndSubtree(Object);

		ogrpFindSubObjectByName(&(*ObjectData)->MassObject, Object, "mass");
		if((*ObjectData)->MassObject)
			(*ObjectData)->MassObject->flag2 |= OBJECT_DONT_UPDATE;
	}

#ifdef BETA_RELEASE
if(!VehicleObjectAnalysis)
{
#endif
	if ((*ObjectData)->CollideInternObject != nullptr)
		(*ObjectData)->CollideInternObject->flag2 |= OBJECT_DONT_UPDATE;

	if ((*ObjectData)->CollideExternObject != nullptr)
		(*ObjectData)->CollideExternObject->flag2 |= OBJECT_DONT_UPDATE;

#ifdef BETA_RELEASE
}
#endif

	if ((*ObjectData)->CollideWalkObject != nullptr)
		(*ObjectData)->CollideWalkObject->flag2 |= OBJECT_DONT_UPDATE;

	if ((*ObjectData)->LineCollideObject != nullptr)
		(*ObjectData)->LineCollideObject->flag2 |= OBJECT_DONT_UPDATE;

	// set up all reflector gunsight objects
	if (LoadCockpit == TRUE)
	{
		_FindAndSetupReflectorSightObjects((*ObjectData)->PlaneObject);
	}

	_FindLODs(*ObjectData);


	(*ObjectData)->WheelRadius = (FLOAT32*)memAlloc(sizeof(FLOAT32)*ML_MAX_GEARNUM);
	_FindAllWheelRadius((*ObjectData)->PlaneObject,(*ObjectData));

	if ((*ObjectData)->LoadoutLODObject != nullptr)
	{
		ogrpShowObjectAndSubtree((*ObjectData)->LoadoutLODObject);

		// flag this LOD so it's always moved and rotated
		(*ObjectData)->LoadoutLODObject->flag2 |= OBJECT_SPECIAL_LOD;
	}

	// towing stuff
	ogrpFindSubObjectByDOFVar(&((*ObjectData)->TowObject),(*ObjectData)->PlaneObject,ML_DOF_TOWPOINT_HEAVY);
	if ((*ObjectData)->TowObject == nullptr)
	{
		ogrpFindSubObjectByDOFVar(&((*ObjectData)->TowObject),(*ObjectData)->PlaneObject,ML_DOF_TOWPOINT_MEDIUM);
		if ((*ObjectData)->TowObject == nullptr)
		{
			ogrpFindSubObjectByDOFVar(&((*ObjectData)->TowObject),(*ObjectData)->PlaneObject,ML_DOF_TOWPOINT_LIGHT);
			if ((*ObjectData)->TowObject != nullptr)
			{
				(*ObjectData)->TowType = ML_TOWTYPE_LIGHT;
			}
		}
		else
		{
			(*ObjectData)->TowType = ML_TOWTYPE_MEDIUM;
		}
	}
	else
	{
		(*ObjectData)->TowType = ML_TOWTYPE_HEAVY;
	}

	// fps seats onboard vehicles stuff
	_FindAndSetupFPSSeats(*ObjectData,
						(*ObjectData)->PlaneObject, FALSE);
	_FindAndSetupVehicleSeats(*ObjectData,
						(*ObjectData)->PlaneObject, FALSE);
	_FindAndSetupHideUnhideObjects(*ObjectData,
							(*ObjectData)->PlaneObject);

	// label all objects as vehicle objects
	obmainSetFlag2InThisTree((*ObjectData)->PlaneObject,OBJECT_IS_PARTOF_VEHICLE);

	(*ObjectData)->ShadowObject = nullptr;
	(*ObjectData)->ShadowObject = obmainFindChildFromObject((*ObjectData)->PlaneObject, "shadow");

	// hoist
	if((*ObjectData)->LoadoutLODObject)
		ogrpFindSubObjectByDOFVar(&((*ObjectData)->HoistObject),(*ObjectData)->LoadoutLODObject,ML_DOF_HOIST); // just support one for now

	if((*ObjectData)->HoistObject)
	{
		ogrpFindSubObjectByName(&(*ObjectData)->HoistLineCollideObject, (*ObjectData)->PlaneObject, "hoistcollider");
		ogrpFindSubObjectByName(&(*ObjectData)->HoistHitBox, (*ObjectData)->HitMap, "hoistbox");
		if((*ObjectData)->HoistLineCollideObject && (*ObjectData)->HoistHitBox)
			mlProcessHoistContactPointObject(*ObjectData);
	}

	_ImportAndSetupWeapons(*ObjectData, VehicleType);

	return 0;

}

UINT32 mlCalculateObjectTreeRadius(FLOAT32 * Radius,OBJECT * Object, UINT32 IncludeSibs)
{
	return _CalculateObjectTreeRadius(Radius, Object, IncludeSibs);
}
