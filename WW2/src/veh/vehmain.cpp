#include <syscfg.h> // Path is project specific.
#include <math.h>
#include <queue>
#include <list>

#include "cfg/cfgPrefs.h"
#include "chat/chatClientMessages.h"
#include "chat/chatHandle.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatWaypointExchg.h"
#include "client/clientSTO.h"
#include "client/GameClient.h"
#include "comm/commClientMessages.h"
#include "ez/EZManager.h"
#include "fio/fio.h"
#include "fp/fpFPS.h"
#include "fp/fpMain.h"
#include "fp/fpPPO.h"
#include "fp/fpTPSPhysics.h"
#include "fp/fptypes.h"
#include "fx/fxMgr.h"
#include "fx/fxSnd.h"
#include "fx/TransformEffect.h"
#include "gr2/gr2Anim.h"
#include "gr2/gr2Physics.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "imap/ww2imap.h"
#include "include/vars.h"
#include "ld/ldtex.h"
#include "locl/loclLang.h"
#include "map/mapArenaDescription.h"
#include "map/mapMissionList.h"
#include "map/mapPersonaSummary.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "math/mathUnitConversion.h"
#include "math/mathvect.h"
#include "math/Matrixd.h"
#include "math/Vec3d.h"
#include "mem/mem.h"
#include "miss/missionDetail.h"
#include "ml/mlCompon.h"
#include "ml/mlhead.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "ob/obdraw.h"
#include "ob/obmain.h"
#include "ob/obMgr.h"
#include "ogrp/ogrptype.h"
#include "pers/persbombsight.h"
#include "pers/persgunsight.h"
#include "pers/pershud.h"
#include "pers/persmain.h"
#include "physics/colliderMain.h"
#include "pi/pi.h"
#include "rn/rnmain.h"
#include "rn/rnvis.h"
#include "scen/ScenarioManager.h"
#include "sdm/StratDB.h"
#include "sn/sndsSoundCache.h"
#include "spt/Random.h"
#include "trn/trnautogun.h"
#include "trn/trncd.h"
#include "trn/trnfx.h"
#include "trn/trnldr.h"
#include "trn/trnmain.h"
#include "trn/trnop.h"
#include "trn/trnvars.h"
#include "tut/TutorialManager.h"
#include "ui/uiChat.h"
#include "ui/uiDotCommands.h"
#include "ui/uiGameEvents.h"
#include "ui/uiMain.h"
#include "updt/updtVehicleHit.h"
#include "vars.h"
#include "veh/vehAnimEnum.h"
#include "veh/vehanim.h"
#include "veh/vehcntrl.h"
#include "veh/vehdecal.h"
#include "veh/vehfx.h"
#include "veh/vehIcon.h"
#include "veh/vehload.h"
#include "veh/vehmain.h"
#include "veh/vehupdate.h"
#include "veh/vehVisDamage.h"
#include "weap/weapammo.h"
#include "weap/weapList.h"
#include "weap/weapons.h"
#include "wwii/wwiiPPOLoader.h"

// WWIIOL specific.
#include "client/clientStrtQueue.h"
#include "miss/missionUtils.h"
#include "strt/strtCaptureState.h"
#include "strt/strtClientMessages.h"
#include "strt/strtCPFlags.h"

#include "wwii/wwiiOhYeahCodes.h"

#define REPORT_ENEMIES

#define CLIENT_VEHICLE_CACHING
//#define CLIENT_VEHICLE_CACHING_DEBUG
//#define CLIENT_VEHICLE_CACHING_REPORT

#ifdef CLIENT_VEHICLE_CACHING_REPORT
static UINT32 _vCacheHits = 0;
static UINT32 _vCacheTries = 0;
#endif

#ifdef CLIENT_VEHICLE_CACHING
using VEHICLE_READY_MAP = std::map<ChatHandle, vehVehicle*>;

// map of players we just popped off the vis list
// we let them linger in case they pop back on quick enough
static VEHICLE_READY_MAP _readyMap;
#endif

#ifdef REPORT_ENEMIES
using ENEMY_REPORTS = std::map<SINT32, FLOAT32>;
#endif

//static vehVehicle _VehicleHead; // head of linked list

// used offline and ignored online
static std::queue<vehVehicle*> g_freeVeh;

using HULK_SET = std::set<VehicleSTO*>;
static HULK_SET _hulkqueue;

// used offline and ignored online
static vehVehicle g_vehicles[MAX_POSSIBLE_VIS_VEHICLES];

static VEHICLE_REPLY _vehicletospawn;
static bool _spawnvehicle;
static StratDB::StratObject* _lastEntered = 0;
static trnCDTerrainObject* _lastEnteredTO = 0;
static ChatHandle _lastInfantryDamaged = 0;

// bailout stuff
static BailoutSTO* _bailout = nullptr;
static SINT32 _bailoutTime = 0;
static SINT32 _bailoutPos = 0;
static FLOAT32 _bailoutLife = 0;

static void vehmainSpawn();

using VEHICLE_HITS = std::list<Vehicle::Hit>;
using VEHICLE_HIT_LIST = std::vector<Vehicle::Hit>;

static VEHICLE_HITS _DeferredHitMessages;

// this really should be a pointer inside the mlModelData structure, but since we have a lot
// of c code, and this is a stl thing, it's a static to this file currently
static VEHICLE_HIT_LIST g_MyHitMessages;

extern Covert<bool> ProtectedOnlineFlag;

#define IS_OFFLINE (!ProtectedOnlineFlag)

#ifndef _DEATHCAM_TIME
#define _DEATHCAM_TIME 4000
#endif

#define WANT_OFFLINE_RAGDOLL 1

// 5.5 seconds
#define MOBILE_SPAWN_UPDATE_TIMER   5500
// 5 seconds
#define MOBILE_DEPLOYING_TIMEOUT    5000
// 60 seconds
#define MOBILE_DEPLOYED_TIMEOUT     60000

#define MAX_MED_LODS 4
#define MAX_HI_LODS 1

static vehVehicle& getVehicleHead();

static UINT32 _GetNewVehicle(vehVehicle** VehiclePtr);
static UINT32 _AddToFreeListOrDelete(vehVehicle* Vehicle);
static UINT32 _AddToListHead(vehVehicle* Vehicle);
static void _CleanVehicle(vehVehicle* v);
extern bool _AirplaneBailout(piVECTOR* pos, piVECTOR* aim);

#ifdef CLIENT_VEHICLE_CACHING
static bool _UpdateReadyList(bool force = false, bool report = false);
static UINT32 _AddToReadyList(vehVehicle* Vehicle);
static vehVehicle* _RemoveFromReady(ChatHandle ch, bool freeVehicle);
#endif

static UINT32 _UpdateVehicleObject(vehVehicle* );
static void _ResetVehicleLODs(vehVehicle* v, bool resetLOD = false);

static UINT32 _CheckForDamage(vehVehicle* CurrentVehicle);
static UINT32 _SendDamageUpdate(vehVehicle* CurrentVehicle, vehDamageQue * DQ);
static void _sendSTODamage(vehVehicle*veh, vehDamageQue* que);
static void	_LoadInfantryVehicle(vehVehicle* Vehicle);

UINT32 _UpdateVehicleBoundingBox(mlObjectData* ObjectData,OBJECT * PlaneObject);
static UINT32 _UpdateVehicleBoundingBox1(mlObjectData* ObjectData,OBJECT * PlaneObject,UINT32* FirstBBox);

static UINT32 _CheckAndUpdateHitmap(vehVehicle* CurrentVehicle);
static UINT32 _ClearAllVehicles();

#if(0)
static LOGICAL _CanPlayerExit();
#endif

#if(0)
static void _FetchSupercell (int x, int y, TERRAIN_DETAIL** td);
#endif

static void _reportEnemyAircraft(vehVehicle* CurrentVehicle);
static void vehApplyVehicleHit(Vehicle::Hit const& hit);

static SINT32 _NumVehicles = 0;

static bool _findRTBFRU(ServerTrackedObject& sto, void* cookie);

#ifdef GM_RELEASE
static SINT32	disableVehLODs;
#endif

static SINT32 _uniqueSTOid = -1000;

static void _FindNearbyFacilities(vehVehicle* vehicle);

static bool _initSTOHulkIfNecessary();

static void _setVehicleState(vehVehicle* v, bool start);

/*----------------------------------------------------------*/
/* MOBILE SPAWN STUFF                                       */
static const FLOAT64 TFR_RANGE_2 = (FLOAT64)(1350*1350);
static const FLOAT64 EFR_RANGE_2 = (FLOAT64)(400*400);

static SINT32 g_mobileUpdateTimer = 0;

static MSPAWN_RESULT _canWeDeploy(vehVehicle* veh, bool ignoreMovement);
static SINT32 _checkCurrentDeployState(vehVehicle* veh);

#if(0)
static void _updateMobileSpawn();
#endif

static void _setVehicleDeployedState(DEPLOYEDSTATE enumState);
static void _resetBombBayDoors();

static MSPAWN_RESULT _checkFacilityRange(StratDB::StratObject* so, int side);
static LOGICAL _isDeployingTimeout();
static LOGICAL _isDeployedTimeout();

#ifdef BETA_RELEASE

extern SINT32 _CurrentLOD;
extern SINT32 _CurrentHitmapDisplay;
extern SINT32 _CurrentHitmapObjectDisplay;

#endif // BETA_RELEASE

extern std::vector<PPOSpec *> activePPOList;
extern UINT16 activePPOIndex;

static bool g_canDeployMobileSpawn = false;

extern void vehloadKill();
extern SINT32 globalMSCoolDownEnd;

static Covert<vehVehicle*> _VehicleHead = new vehVehicle();

/*----------------------------------------------------------*/

/****************************************************************************
	_getVehicleSpawnObjectDof
	Return pointer to the vehicle's spawn object dof or null
 ****************************************************************************/

static inline OBJECTDOF* _getVehicleSpawnObjectDof(vehVehicle* veh)
{
	if(
		veh == nullptr
		|| veh->VehicleObjectData == nullptr
		|| veh->VehicleObjectData->spawnObject == nullptr
		)
		return nullptr;
	return veh->VehicleObjectData->spawnObject->dof;
}

/************************************************************************************************
	_ReadComponentFiles
*************************************************************************************************/
// sets up important damage array by reading its component file
// also sets total mass for each vehicle (hack for concussion damage)

static void _ReadComponentFiles()
{
	int i, k;
	vehType* veh;
	mlComponentList* compList;
	mlComponent* comp;
	char fileName[256];
	char vehName[64];
	mlComponentListIterator it;
	logTimerBeginMacro();


	for(i = 0;; i++)
	{
		veh = (vehType*)vehtypeGetByIndex(i);
		if(!veh) break;

		compList = nullptr;

		// bit of a hack as the  actual component file name is specificed in the .veh file
		// which we don't have access to at this point (since it's only read by user controlled
		// vehicles, as is this component data), so we assume it's <vehname>.cmp
		strncpy(vehName, (char*)veh->ModelFile, 63); strtok(vehName, ".");
		snprintf(fileName, 255,"vehicles\\%s\\%s.cmp", veh->ModelDirectory, vehName);

// go ahead and fill this out while we're here for weap logging
#ifdef EVENT_LOG
		if(veh->Type == VEH_TYPE_PERSON)
		{
//			strcpy((char*)veh->DamageObjects[0], "head");
//			strcpy((char*)veh->DamageObjects[1], "torso");
//			strcpy((char*)veh->DamageObjects[2], "arms");
//			strcpy((char*)veh->DamageObjects[3], "legs");
			strcpy((char*)veh->DamageObjects[0], "Bip01 Pelvis");
			strcpy((char*)veh->DamageObjects[1], "Bip01 Spine");
			strcpy((char*)veh->DamageObjects[2], "Bip01 Spine1");
			strcpy((char*)veh->DamageObjects[3], "Bip01 Spine2");
			strcpy((char*)veh->DamageObjects[4], "Bip01 Spine3");
			strcpy((char*)veh->DamageObjects[5], "Bip01 Neck");
			strcpy((char*)veh->DamageObjects[6], "Bip01 Head");
			strcpy((char*)veh->DamageObjects[7], "Dummy21");
			strcpy((char*)veh->DamageObjects[8], "Bip01 L Clavicle");
			strcpy((char*)veh->DamageObjects[9], "Bip01 L Upperarm");
			strcpy((char*)veh->DamageObjects[10], "Bip01 L Forearm");
			strcpy((char*)veh->DamageObjects[11], "Bip01 L Finger0");
			strcpy((char*)veh->DamageObjects[12], "Bip01 L Finger01");
			strcpy((char*)veh->DamageObjects[13], "Dummy06");
			strcpy((char*)veh->DamageObjects[14], "Bip01 L Hand");
			strcpy((char*)veh->DamageObjects[15], "Bip01 L Finger1");
			strcpy((char*)veh->DamageObjects[16], "Bip01 L Finger11");
			strcpy((char*)veh->DamageObjects[17], "Dummy07");
			strcpy((char*)veh->DamageObjects[18], "Bip01 R Clavicle");
			strcpy((char*)veh->DamageObjects[19], "Bip01 R Upperarm");
			strcpy((char*)veh->DamageObjects[20], "Bip01 R Forearm");
			strcpy((char*)veh->DamageObjects[21], "Bip01 R Finger0");
			strcpy((char*)veh->DamageObjects[22], "Bip01 R Finger01");
			strcpy((char*)veh->DamageObjects[23], "Dummy01");
			strcpy((char*)veh->DamageObjects[24], "Bip01 R Hand");
			strcpy((char*)veh->DamageObjects[25], "Bip01 R Finger1");
			strcpy((char*)veh->DamageObjects[26], "Bip01 R Finger11");
			strcpy((char*)veh->DamageObjects[27], "Dummy02");
			strcpy((char*)veh->DamageObjects[28], "Bip01 L Thigh");
			strcpy((char*)veh->DamageObjects[29], "Bip01 L Calf");
			strcpy((char*)veh->DamageObjects[30], "Bip01 L Foot");
			strcpy((char*)veh->DamageObjects[31], "Bip01 L Toe0");
			strcpy((char*)veh->DamageObjects[32], "Dummy16");
			strcpy((char*)veh->DamageObjects[33], "Bip01 R Thigh");
			strcpy((char*)veh->DamageObjects[34], "Bip01 R Calf");
			strcpy((char*)veh->DamageObjects[35], "Bip01 R Foot");
			strcpy((char*)veh->DamageObjects[36], "Bip01 R Toe0");
			strcpy((char*)veh->DamageObjects[37], "Dummy11");
		}
#endif

		if(mlcomponReadDamageComponents(&compList, (UCHAR*)fileName, (UCHAR*)"", i, veh) != 0)
		{
			if(veh->Type != VEH_TYPE_PERSON && veh->Type != VEH_TYPE_FLOATER)
			{
				char str[256];
				snprintf(str, 255, "_ReadComponentFiles: Can't open component file: %s", fileName);
				Write_Debug(str);
			}

			continue;
		}

		mlComponentListBegin(compList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			comp = it.comp;

			switch (comp->Type)
			{
				case ML_COMPONENT_AIRFOIL:
				case ML_COMPONENT_ENGINE:
				case ML_COMPONENT_PITOT:
				case ML_COMPONENT_GEAR:
				case ML_COMPONENT_WEAPONMOUNT:
					for(k = 0; k < 4; k++)
						veh->DamageImportant[comp->DamageComponentNum[k]] = 1;
				break;
				case ML_COMPONENT_CREW:
                {
                    int counttosearch = comp->isPak38_6lb_57mm ? 10 : 4;
					for(k = 0; k < counttosearch; k++)
						veh->DamageImportant[comp->DamageComponentNum[k]] = 1;
                    break;
                }
			}
		}

		mlcomponFreeComponentList(compList);
	}
logTimerEndMacro();
}

UINT32 vehInit()
{
	if(vehtypeInit() !=0 ) return 1;

	if(!LowMemorySystem && !NoBlitzWorPreload)
		_ReadComponentFiles();

	_NumVehicles = 0;

    // add all the vehicles on the vehicle list to the free list
    for(SINT32 i = 0; i < MAX_POSSIBLE_VIS_VEHICLES; i++)
    {
        g_freeVeh.push(&g_vehicles[i]);  // VEHSTUFF
    }

    if (!IS_OFFLINE)
    {
        size_t cnt = sizeof(g_vehicles);
        memset( reinterpret_cast<void *>(&g_vehicles[0]), cnt, 0xff ); // damn cheaters hopefully choke on this
    }

	vehAnimInit();
	vehCntrlInit();
	vehIconInit();

    if(!vehfxInit())
        return 1;

    if(vehdecalInit() != 0)
        return 1;

	g_MyHitMessages.clear();

	return 0;
}

UINT32 vehKill()
{
vehVehicle*CurrentVehicle, *NextVehicle;


	for (CurrentVehicle = getVehicleHead().getNext();
		 CurrentVehicle != nullptr; )
	{
		NextVehicle = CurrentVehicle->getNext();
		vehKillVehicle(CurrentVehicle);
		CurrentVehicle = NextVehicle;
	}

	vehtypeKill();
	vehIconKill();
	vehAnimKill();
    vehfxKill();
    vehdecalKill();
    vehloadKill();

	g_MyHitMessages.clear();

    vehVehicle* v = _VehicleHead.Get();
    delete v;
    _VehicleHead.Set(nullptr);

	return 0;
}

/*
static LOGICAL _CenterVisualDamage(OBJECT* obj, UINT32 user)
{
	// each object found is a visual damage object
	// center it's child to the object's origin
	OBJECT* visualObject = obj->child;

	if(visualObject && !visualObject->child)
	{
		// create an object to hold our position offset from vehicle center
		visualObject->child = obmainGetFreeObject();

		// give it a common name
		strncpy(visualObject->child->name, "offset", 32);

		// save our pos and loc members so the visual damage system can make use of them later
		visualObject->child->pos = visualObject->pos;
		visualObject->child->loc = visualObject->loc;

		// reset the pos and loc members for our visual damage object
		visualObject->pos.x = visualObject->pos.y = visualObject->pos.z = 0;
		visualObject->loc.x = visualObject->loc.y = visualObject->loc.z = 0;

		// now let's reset the vertex positions to the vehicle's origin
		piVECTOR offset = visualObject->child->loc;

		piVectorPtr pverts = visualObject->MutablePositions();

		for(int i = 0; i < visualObject->numverts; i++)
		{
			// move it back to the origin
			pverts->x -= offset.x;
			pverts->y -= offset.y;
			pverts->z -= offset.z;

			pverts++;
		}
	}

	return TRUE;
}

void vehmainInitModelInstance(OBJECT* root)
{
	return;
	// find the root of the visual damage group
	OBJECT* crapGroup = obmainFindChildFromObject(root, "crap");

	if(crapGroup)
		obmainFindCommentFirstPartCallback(crapGroup, "visdamage", 0, _CenterVisualDamage);
}
*/

UINT32 vehCreateNewVehicle(vehVehicle**VehiclePtr, UINT32 HostIndex, SINT32 VehicleType, ChatHandle ch)
{
	vehVehicle* Vehicle;

#ifdef CLIENT_VEHICLE_CACHING_DEBUG
	const wchar_t* callsign = chatGetWideCallsign(ch);
#endif

	*VehiclePtr = nullptr;
	Vehicle = vehFindVehicle(HostIndex);

	if (Vehicle != nullptr)
		return 0; // we're already loaded

	if (VehicleType == -1)
		return 0; // vehicle type unknown


#ifdef CLIENT_VEHICLE_CACHING
	// check ready list
	Vehicle = _RemoveFromReady(ch, false);

#ifdef CLIENT_VEHICLE_CACHING_REPORT
	++_vCacheTries;
#endif

	if(Vehicle != nullptr)
	{
		bool sameType		= (Vehicle->VehicleType->VehicleTypeNum == VehicleType);
		bool sameSortie	= (Vehicle->VehicleHostIndex == HostIndex);
		bool isTrooper		= (Vehicle->VehicleType->Type == VEH_TYPE_PERSON);

#ifdef CLIENT_VEHICLE_CACHING_DEBUG
		uiChatPrintLine(L"CACHE: Parameters for %s are same type = %s, same sortie = %s, is trooper = %s",
									callsign,
									(sameType) ? L"true": L"false",
									(sameSortie) ? L"true": L"false",
									(isTrooper) ? L"true": L"false");
#endif

		if(sameType && (isTrooper || sameSortie))
		{
#ifdef CLIENT_VEHICLE_CACHING_DEBUG
			uiChatPrintLine(L"CACHE: Using cached vehicle(%p) %ls id: %d -> %d vt: %ls",
									Vehicle,
									callsign,
									Vehicle->VehicleHostIndex,
									HostIndex,
									UTF16(Vehicle->VehicleType->Id));
#endif // CLIENT_VEHICLE_CACHING_DEBUG

			// add it back to the vehicle list
			_AddToListHead(Vehicle);

			// make sure our vehicle space is enabled
			if(Vehicle->spaceID)
				dGeomEnable((dGeomID)Vehicle->spaceID);

			//if(!sameSortie) // only troops will fall into here
			//{
				// reset states
				//_setVehicleState(Vehicle, true);

				// we have a new vehicle index, reset the ode colliders
				//if(Vehicle->VehicleObjectData->VisDamageHulkLOD)
				//	physicsRecurseAndToggleObjectCollider(Vehicle->VehicleObjectData->VisDamageHulkLOD, false);

				//if(Vehicle->VehicleObjectData->HitMap)
				//	physicsRecurseAndToggleObjectCollider(Vehicle->VehicleObjectData->HitMap, true);
			//}

			// make sure we attach this vehicle to the new host index (might be the same)
			Vehicle->VehicleHostIndex = HostIndex;

			// return it
			*VehiclePtr = Vehicle;

#ifdef CLIENT_VEHICLE_CACHING_REPORT
			++_vCacheHits;
#endif

			// we found a good vehicle
			return 2;
		}
		else
		{
#ifdef CLIENT_VEHICLE_CACHING_DEBUG
			uiChatPrintLine(L"CACHE: Freeing cached vehicle(%p) %ls id: %d vt: %ls",
									Vehicle,
									callsign,
									Vehicle->VehicleHostIndex,
									UTF16(Vehicle->VehicleType->Id));
#endif // CLIENT_VEHICLE_CACHING_DEBUG

			// add it back to the free list for future use

			// cleanup resources
			_CleanVehicle(Vehicle);
			_AddToFreeListOrDelete(Vehicle);

			// this will let it fall through and a new vehicle will be created
			Vehicle = nullptr;
		}
	}
#endif

	vehType const* vehtype = vehtypeGetByIndex(VehicleType);
	if(vehtype == nullptr)
	{
		Write_Debug("Attempted to create a vehicle with invalid id number %d",VehicleType);
		return 0;
	}

	_GetNewVehicle(&Vehicle);

	if (Vehicle == nullptr)
	{
#ifdef CLIENT_VEHICLE_CACHING
		// let's try to clear a vehicle off the ready list and get a new one again
		if(_UpdateReadyList(true))
			_GetNewVehicle(&Vehicle);

		if (Vehicle == nullptr)
#endif // CLIENT_VEHICLE_CACHING
			return 0; // no vehicle slots left!!
	}

	// got a new one, init it
	Vehicle->VehicleInitTime = frametime;
	Vehicle->VehicleHostIndex = HostIndex;
	Vehicle->InUse = TRUE;
	Vehicle->VehicleType = (vehType*)vehtype; // grrr should be const

	vehInitVehicleAnimation(Vehicle);

	*VehiclePtr = Vehicle;

    vehfxAttachVehicle(Vehicle, &Vehicle->iFXVehicleID);

#ifdef CLIENT_VEHICLE_CACHING_DEBUG
	uiChatPrintLine(L"CACHE: Using new vehicle(%p) %ls id: %d -> %d vt: %ls",
									Vehicle,
									callsign,
									Vehicle->VehicleHostIndex,
									HostIndex,
									UTF16(Vehicle->VehicleType->Id));
#endif

	return 1;
}


UINT32 vehSetVehicleToUserControlled(vehVehicle* Vehicle,SINT32 MultiCrewPos)
{
UCHAR DirName[64];
UINT32 RetValue;

	if (Vehicle == nullptr)
	{
		return 1;
	}

	weapAmmoSetFuzeInterval(0);

	Vehicle->Flags |= VEH_FLAGS_USERCONTROLLED;

	if (Vehicle->ModelData == nullptr)
	{
		strcpy((char*)DirName,(char*)"vehicles\\");
		strcat((char*)DirName,(char*)Vehicle->VehicleType->ModelDirectory);

		Vehicle->ModelData = (mlVehicleData*)memCalloc(1, sizeof(mlVehicleData));
		if (Vehicle->ModelData == nullptr)
		{
			return 1;
		}
		mlInitVehicleData(Vehicle->ModelData);
		Vehicle->ModelData->VehicleMasterPtr = Vehicle;
		RetValue = mlReadVehicleFile(Vehicle->ModelData,
							Vehicle->VehicleType->ModelFile,
							DirName,
							Vehicle->VehicleType->VehicleTypeNum);
		if (RetValue != 0)
		{
			return 1;
		}

		if (Vehicle->ModelData == nullptr)
		{
			return 1;
		}

		if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		{
			if (!fpInit(Vehicle))
			{
				return 1;
			}
		}
	}

	Vehicle->ModelData->MultiCrewPos = MultiCrewPos;

	if (Vehicle->VehicleObjectData != nullptr)
	{
		if (Vehicle->VehicleObjectData->LineCollideObject != nullptr)
		{
			Vehicle->VehicleObjectData->LineCollideObject->flag2 &=
				~(OBJECT_DONT_UPDATE);
		}
		vehmainCreateODECollider(Vehicle);
	}

	return 0;
}

static void _ResetVehicleLODs(vehVehicle* v, bool resetLOD)
{
	if(!v) return;

	OBJECT* Object = nullptr;
	piVECTOR temp;


	//by default all vehicles are non-visible
	// unless otherwise specified
	for (int i=0;i<ML_MAX_LODS;i++)
	{
		Object = v->VehicleObjectData->LODObjects[i];

		if (Object != nullptr)
		{
			while (Object->flags & OBJECT_NOTOBJECT)
			{
				Object = Object->child;

				if (Object == nullptr)
					break;
			}

			if (Object != nullptr)
			{
				Object->minlod = 0.0f;
				Object->maxlod = 0.0f;

				// update the LOD
				if(resetLOD)
					obdrawProcessLODCallback(Object, 0, &temp, FALSE, 0, nullptr);
			}
		}
	}

	v->MinVehicleLOD = -1;
}

static UINT32 _UpdateVehicleLODs()

{
	if (ft500mscount == 0) return 0;

	FLOAT64VECTOR WorldOriginOffset;

	trnGetWorldOriginOffset(&WorldOriginOffset);
	WorldOriginOffset.z = -WorldOriginOffset.z;

	vehVehicle*CurrentVehicle = persGetPersonaVehiclePtr();

	// get player's vehicle position;
	if (CurrentVehicle == nullptr || CurrentVehicle->VehicleObjectData == nullptr || CurrentVehicle->VehicleObjectData->PlaneObject == nullptr)
		return 0;

	static vehVehicle*LODVehicles[MAX_POSSIBLE_VIS_VEHICLES+1];
	static FLOAT32 LODVehiclesDistSqr[MAX_POSSIBLE_VIS_VEHICLES+1];
	vehVehicle*TempVehicle;
	FLOAT32 DistSqr;
	vehVehicle*PlayerVehicle;
	OBJECT * Object;
	UINT32 i,j;
	UINT32 Position;
	piVECTOR PlayPos,VehPos,Delta;

	for(i = 0; i <= MAX_POSSIBLE_VIS_VEHICLES; i++)
	{
		LODVehiclesDistSqr[i] = 1000000000000.0f;
		LODVehicles[i] = nullptr;
	}

	PlayPos.x = CurrentVehicle->VehicleObjectData->PlaneObject->mat._41;
	PlayPos.y = CurrentVehicle->VehicleObjectData->PlaneObject->mat._42;
	PlayPos.z = CurrentVehicle->VehicleObjectData->PlaneObject->mat._43;
	PlayerVehicle = CurrentVehicle;

	for (CurrentVehicle = getVehicleHead().getNext(); CurrentVehicle != nullptr; CurrentVehicle = CurrentVehicle->getNext())
	{
		if (CurrentVehicle->VehicleObjectData == nullptr || CurrentVehicle->VehicleObjectData->PlaneObject == nullptr)
			continue;


		if (CurrentVehicle->VehicleObjectData->PlaneObject->flag2 & OBJECT_USERVEHICLE)
			continue;

		if (CurrentVehicle == PlayerVehicle)
			continue;

		_ResetVehicleLODs(CurrentVehicle);

		if (Online == 1)
		{
			if (CurrentVehicle->HideVehicle == 1 || CurrentVehicle->BumpedOff == 1)
				continue;

			// make buzzards invisible (only rats for now)
			if(CurrentVehicle->VehicleType->Type == VEH_TYPE_FLOATER && !CurrentVehicle->StateAnimation.GearSetting &&
				PlayerVehicle->VehicleType->Type != VEH_TYPE_FLOATER)// && CurrentVehicle->Rank >= 100)
			{
				continue;
			}
		}
        else
        {
            // For offline hulk testing.
            if(CurrentVehicle->HideVehicle == 1)
            {
                continue;
            }
        }

		// now add object to sort list if applicable;
		Position = MAX_POSSIBLE_VIS_VEHICLES;
		LODVehicles[Position] = CurrentVehicle;
		VehPos.x = (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - WorldOriginOffset.x);
		VehPos.y = (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - WorldOriginOffset.y);
		VehPos.z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - WorldOriginOffset.z);

#ifdef BETA_RELEASE
		// make sure it doesn't interfere with other object analysis stuff
		if(VehicleObjectAnalysis && (_CurrentLOD != 0 || _CurrentHitmapDisplay !=0 || _CurrentHitmapObjectDisplay != 0) )
			continue;
#endif

		mathSubVectors(&Delta,&VehPos,&PlayPos);
		DistSqr = Delta.x * Delta.x + Delta.y * Delta.y + Delta.z * Delta.z;
		LODVehiclesDistSqr[Position] = DistSqr;

		while (Position > 0 && LODVehiclesDistSqr[Position - 1] > LODVehiclesDistSqr[Position])
		{
			DistSqr = LODVehiclesDistSqr[Position - 1];
			LODVehiclesDistSqr[Position - 1] = LODVehiclesDistSqr[Position];
			LODVehiclesDistSqr[Position] = DistSqr;

			TempVehicle = LODVehicles[Position - 1];
			LODVehicles[Position - 1] = LODVehicles[Position];
			LODVehicles[Position] = TempVehicle;

			Position--;
		}
	}
//#warning "don't check in"
//return 0;
	// now set the appropriate vehicles to allow med/high LODs
	for (i = 0; i < MAX_POSSIBLE_VIS_VEHICLES; i++)
	{
		CurrentVehicle = LODVehicles[i];
		if (LODVehicles[i] == nullptr)
			continue;

#if(1)
		// set the minimum LOD number
		if (i < MAX_HI_LODS)
			CurrentVehicle->MinVehicleLOD = 0;
		else if (i < MAX_MED_LODS)
			CurrentVehicle->MinVehicleLOD = 1;
		else
			CurrentVehicle->MinVehicleLOD = 2;
#endif

#ifdef GM_RELEASE
		// FIXME - not convinced MinVehicleLOD is used!
		if (disableVehLODs)
		{
			CurrentVehicle->MinVehicleLOD = 0;
		}
#endif

#if(1)
		// now set the LOD ranges
		for (j = 0; j < ML_MAX_LODS; j++)
		{
			Object = CurrentVehicle->VehicleObjectData->LODObjects[j];
			if (Object != nullptr)
			{
				while (Object->flags & OBJECT_NOTOBJECT)
				{
					Object = Object->child;
					if (Object == nullptr)
					{
						break;
					}
				}
				if (Object != nullptr)
				{
#ifdef GM_RELEASE
					if (disableVehLODs)
					{
						Object->minlod = 0.0f;
						Object->maxlod = (j == 0) ? 10000.0f : 0.0f;
					}
					// we're a valid lod object
					else
#endif
                    {
                        Object->minlod = CurrentVehicle->VehicleObjectData->SavedMinLODRange[j];
                        Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
                    }
#if(0)
                    if (j == 0)
					{
						if (i < MAX_HI_LODS)
						{
							Object->minlod = CurrentVehicle->VehicleObjectData->SavedMinLODRange[j];
							Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
						}
						// else we're disabled
					}
					else if (j == 1)
					{
						if (i < MAX_HI_LODS)
						{
							Object->minlod = CurrentVehicle->VehicleObjectData->SavedMinLODRange[j];
							Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
						}
						else if (i < MAX_MED_LODS)
						{
							Object->minlod = 0.0f;
							Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
						}
						// else we're disabled
					}
					else if (j == 2)
					{
						if (i < MAX_MED_LODS)
						{
							Object->minlod = CurrentVehicle->VehicleObjectData->SavedMinLODRange[j];
							Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
						}
						else
						{
							Object->minlod = 0.0f;
							Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
						}
					}
					else
					{
						Object->minlod = CurrentVehicle->VehicleObjectData->SavedMinLODRange[j];
						Object->maxlod = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[j];
					}
#endif
				}

			}
		}
#endif
	}

	// vehicle you're riding on always has highest LOD
	if(PlayerVehicle->RideVehicleHostIndex > 0)
	{
		vehVehicle* RideVehicle = vehFindVehicle(PlayerVehicle->RideVehicleHostIndex);
		if(RideVehicle && !RideVehicle->isHulked())
		{
			RideVehicle->MinVehicleLOD = 0;

			for (j = 0; j < ML_MAX_LODS; j++)
			{
				Object = nullptr;
				if(RideVehicle->VehicleObjectData)
					Object = RideVehicle->VehicleObjectData->LODObjects[j];

				if(Object)
				{
					while (Object->flags & OBJECT_NOTOBJECT)
					{
						Object = Object->child;
						if (Object == nullptr) break;
					}
					if(Object)
					{
						Object->minlod = 0.0f;
						Object->maxlod = (j == 0) ? 10000.0f : 0.0f;
					}
				}
			}
		}
	}

	return 0;
}

/************************************************************************************************
	_isInterior
*************************************************************************************************/
static void _isInterior(vehVehicle*v)
{
#ifdef BETA_RELEASE
    static int a = 0;

    if(a) return;
#endif

    if(!v || !v->VehicleObjectData)
    {
        return;
    }

    OBJECT* o = v->VehicleObjectData->PlaneObject;
	if(!o)
	{
		return;
	}

    if(Online)
    {
        if(o->flags & OBJECT_SKIPDRAWING ||
           o->flag2 & OBJECT_HIDDEN)
        {
            return;
        }
    }

    bool objectIsInside = false;
	StratDB::StratObject* strat = nullptr;
	trnCDTerrainObject* TO = 0;

	Vec3d start = (v->ModelData) ? Vec3d(v->ModelData->VehiclePos) : Vec3d(v->PosData.Pos);
	static Vec3d dir = Vec3d(0.0f, 0.0f, -1.0f);

	// bring it into the physical world
	start = physicsGameToPhysical(start);

	// move it up slightly and then we'll shoot down
	start.z() += .5f;

	// do the query
	const colliderQuery::CollisionList& hits = collideRayList(COLLIDER_TRN_BUILDING, 2.0f, start, dir);

	for(int i = 0; i < (int)hits.size(); i++)
	{
		const colliderQuery::Collision& hit = hits.at(i);
		OBJECT* obHit = colliderData<OBJECT>(hit.g2);

		if(!obHit)
			continue;

		// On a vehicle
        if(v->VehicleType->Type == VEH_TYPE_PERSON && v->RideVehicleHostIndex)
        {
            // If any in the list has an interior material, set
            // it to light 'special'.
            if(piIsMaterialInterior(obHit->matid))
            {
                if(!(o->flag3 & OBJECT_IS_INTERIOR))
                    obmainSetFlag3InThisTree(o, OBJECT_IS_INTERIOR);

				TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);
				if(TO) strat = TO->StratObj();

                objectIsInside = true;
                break;
            }
        }
        else
        {
            if(piIsMaterialInterior(obHit->matid))
            {
                if(!(o->flag3 & OBJECT_IS_INTERIOR))
                    obmainSetFlag3InThisTree(o, OBJECT_IS_INTERIOR);

				TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);
				if(TO) strat = TO->StratObj();

                objectIsInside = true;
            }

			// only check the first hit
			break;
        }
	}

    v->isInside = objectIsInside;

	if(v->IsMe())
	{
		if(objectIsInside)
		{
			if(!_lastEntered && strat)
			{
				_lastEntered = strat;
				_lastEnteredTO = TO;
				TUT_MGR->DoEvent(TUT_EVENT_ENTER, _lastEntered);
				uiRunScript("game.onenterbuilding()");
			}
			else if(_lastEntered && strat && strat != _lastEntered)
			{
				TUT_MGR->DoEvent(TUT_EVENT_LEAVE, _lastEntered);
				uiRunScript("game.onleavebuilding()");
				PlayerClient()->GetCaptureState().m_valid = false; // invalidate current state
				_lastEntered = strat;
				_lastEnteredTO = TO;
				TUT_MGR->DoEvent(TUT_EVENT_ENTER, _lastEntered);
				uiRunScript("game.onenterbuilding()");
			}
		}
		else
		{
			if(o->flag3 & OBJECT_IS_INTERIOR)
			{
				obmainClearFlag3InThisTree(o, OBJECT_IS_INTERIOR);
			}

			if(_lastEntered)
			{
				TUT_MGR->DoEvent(TUT_EVENT_LEAVE, _lastEntered);
				_lastEntered = nullptr;
				_lastEnteredTO = nullptr;
				uiRunScript("game.onleavebuilding()");
				PlayerClient()->GetCaptureState().m_valid = false; // invalidate current state
			}
		}
	}
	else if(!v->isInside && o->flag3 & OBJECT_IS_INTERIOR)
	{
		obmainClearFlag3InThisTree(o, OBJECT_IS_INTERIOR);
	}
}

/************************************************************************************************
	vehUpdate1stPersonVehicle
*************************************************************************************************/

static SINT32 vehUpdate1stPersonVehicle(vehVehicle* CurrentVehicle)
{
	if (CurrentVehicle->VehicleDeadTime != 0)
	{
		if (frametime - CurrentVehicle->VehicleDeadTime > _DEATHCAM_TIME)
		{
			if(SCEN_MGR->IsActive() && SCEN_MGR->HasFlag(ScenarioManager::RESPAWN_ME) && !uiFindView("respawn"))
			{
				uiLoadView("respawn");
				return 0; // give this a frame to load out
			}

			if(_bailout)
				_bailout->setExpired();

			vehKillVehicle(CurrentVehicle);
			persClearBombsight();
			_ClearAllVehicles();
			persSetInternalCamera(1,0,0);
			persResetHealths();
            fxMgrFreeAll();
            weapAmmoKillAllAmmo();
			trncdProcessODEQueue(true);
			uiGameToUI();

			return -1;
		}
	}

#if(0)
	static Vec3d myLastPos;

	Vec3d myPos = Vec3d(CurrentVehicle->ModelData->VehiclePos);
	Vec3d myPosDiff = myPos - myLastPos;
	double distance = myPosDiff.length();

	myLastPos = myPos;
#endif

	mlUpdateVehicle(CurrentVehicle->ModelData);

	if (CurrentVehicle->ModelData->MultiCrewPos >= 2)
	{
		vehAnimUpdateHostControlledVehicleAnimations(CurrentVehicle);
		vehupdateUpdateVehicle(CurrentVehicle);
	}
	else if (CurrentVehicle->ModelData->MultiCrewPos == 1 || (!Online && persGetCameraMode() == 1))
	{
		vehUpdateWeapons(CurrentVehicle);
	}

	_UpdateVehicleObject(CurrentVehicle);
	_CheckForDamage(CurrentVehicle); // check if any self-inflicted damage needs to be sent

	vehAnimUpdateVehicleStateAnimations(CurrentVehicle);
	vehIconUpdate(CurrentVehicle);
	_CheckAndUpdateHitmap(CurrentVehicle);

    if(frametime - CurrentVehicle->interiorCheck > 500)
    {
        // Check if vehicle is inside building for 'special' lighting
        _isInterior(CurrentVehicle);
        CurrentVehicle->interiorCheck = frametime;
    }

	// update any current ppo
	if(fpPPOUpdate())
		fpPPOSet(nullptr);

	return 0;
}

/************************************************************************************************
	vehUpdate3rdPersonVehicle
*************************************************************************************************/

static SINT32 vehUpdate3rdPersonVehicle(vehVehicle* CurrentVehicle)
{
	if(!(CurrentVehicle->Flags & VEH_FLAGS_ODE_CONTROLLED))
		vehupdateUpdateVehicle(CurrentVehicle);

	vehupdateDisplayVehicle(CurrentVehicle);
	_CheckForDamage(CurrentVehicle); // check if any damage needs to be sent
	vehAnimUpdateHostControlledVehicleAnimations(CurrentVehicle);

	vehAnimUpdateVehicleStateAnimations(CurrentVehicle);
	vehIconUpdate(CurrentVehicle);
	_CheckAndUpdateHitmap(CurrentVehicle);

    if(frametime - CurrentVehicle->interiorCheck > 500)
    {
        // Check if vehicle is inside building for 'special' lighting
        _isInterior(CurrentVehicle);
        CurrentVehicle->interiorCheck = frametime;
    }

    if(CurrentVehicle->hulksto)
    {
        if(vehVisDamageHulkVehicle(CurrentVehicle, FALSE) == 0)
        {
            CurrentVehicle->hulksto = false;
            CurrentVehicle->HideVehicle = 1;
        }
    }

	return 0;
}

/* no longer used apparently
static void _HitMessageUpdate()
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	if(!PlayerVehicle)
	{
		if(!_DeferredHitMessages.empty())
			_DeferredHitMessages.clear();

		return;
	}

	VEHICLE_HITS::iterator it = _DeferredHitMessages.begin();

	for(; it != _DeferredHitMessages.end();)
	{
		Vehicle::Hit& hit = *it;
		VEHICLE_HITS::iterator curIt = it;
		++it;
		vehVehicle* ShooterVehicle = vehFindVehicle(hit.m_shooter);

		if(!ShooterVehicle)
		{
			vehApplyVehicleHit(hit);
			_DeferredHitMessages.erase(curIt);
		}
		else
		{
			///TODO: Troy: Vehicle::Hit::m_timeStamp is a teulClock64, not a frametime,
			///and is currently transferred over the network with an age. Not sure if
			///we need that or not?
			vehInterpolator::Update const& cur = ShooterVehicle->interpolator.Interpolate(frametime);

			TeulDeltaMS delta = (TeulDeltaMS)(cur.timestamp - hit.m_timestamp);
			if(delta > 0 || delta < -1000)
			{
				vehApplyVehicleHit(hit);
				_DeferredHitMessages.erase(curIt);
			}
		}
	}
}
*/

/************************************************************************************************
	vehUpdate
*************************************************************************************************/

UINT32 vehUpdate()
{
	if(_spawnvehicle)
    {
		vehmainSpawn();
        // Ticket 4892: Spawning upside down fix. Something fubar with
        // components on first spawn with specific vehicles. Don't
        // update immediately.
        return 0;
    }

	vehVehicle* CurrentVehicle;

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	logTimerBeginMacro();

	persResetVehicleG();
	_UpdateVehicleLODs();

	static int then = 0;
	static int enemyCheck = 0;
	static int enemyDuration = -1;

	if(enemyDuration == -1)
	{
		// report every 5 minutes
		enemyDuration = 1000 * 60 * 5;

		// check for an arena var
		ARENA_VARS::iterator itr = g_arenaDescription.find("hqreports");

		if(itr != g_arenaDescription.end())
			enemyDuration = atoi(itr->second.c_str()) * 1000;
	}


    if(frametime - then > 500)
    {
        then = frametime;
        _initSTOHulkIfNecessary();
    }

#ifdef REPORT_ENEMIES
	static ENEMY_REPORTS reports;
	static ENEMY_REPORTS::iterator rit;

    reports.clear();
#endif

	if ( PlayerVehicle && PlayerVehicle->ModelData && PlayerVehicle->RideVehicleHostIndex == 0 )
		vehUpdate1stPersonVehicle(PlayerVehicle);

	// all vehicles that aren't riding on anything
	for (CurrentVehicle = getVehicleHead().getNext(); CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if (CurrentVehicle->LoadVehicle)
		{
			if(PlayerVehicle &&	PlayerVehicle->VehicleType->Type == VEH_TYPE_SEA &&
				CurrentVehicle->VehicleType->Type == VEH_TYPE_AIR &&
				CurrentVehicle->Side != PlayerVehicle->Side )
			{
				_reportEnemyAircraft(CurrentVehicle);
			}

			if(CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON) // infantry load immediately
				_LoadInfantryVehicle(CurrentVehicle);
			else
				vehloadCreateVehicleInstance(CurrentVehicle, FALSE);
		}

		if(CurrentVehicle->ModelData == nullptr && CurrentVehicle->RideVehicleHostIndex == 0)
		{
			if(!CurrentVehicle->VehicleObjectData)
				continue;

			vehUpdate3rdPersonVehicle(CurrentVehicle);

#ifdef REPORT_ENEMIES
			if(enemyDuration > 0 &&
			   frametime >= enemyCheck &&
			   PlayerVehicle &&
			   PlayerVehicle->InUse &&
			   CurrentVehicle->InUse &&
			   CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON &&
			   PlayerVehicle->VehicleType->Type == VEH_TYPE_PERSON &&
			   CurrentVehicle->Side != PlayerVehicle->Side)
			{
				FLOAT64VECTOR& p = CurrentVehicle->PosData.Pos;
				FLOAT32 distance	= CurrentVehicle->VehicleObjectData->PlaneObject->distsqr;
				SINT32 vindex		= -1;
				FLOAT32 score	= 0;

				FLOAT64VECTOR dir;
				mathSubVectors(&dir, &p, &PlayerVehicle->ModelData->VehiclePos);

				mathNormalize(&dir);

				double d = RAD_TO_DEG(atan2(dir.x, dir.y));

				if((d < 0 && d > -22.5f) || (d >= 0 && d <= 22.5f))
					vindex = 0; // n
				else if(d > 22.5f && d <= 67.5f)
					vindex = 1; // ne
				else if(d > 67.5f && d <= 112.5f)
					vindex = 2; // e
				else if(d > 112.5f && d <= 157.0f)
					vindex = 3; // se
				else if(d > 157.0f || d <= -157.5f)
					vindex = 4; // s
				else if(d <= -112.5f && d > -157.5f)
					vindex = 5; // sw
				else if(d <= -67.5f && d > -112.5f)
					vindex = 6; // w
				else if(d <= -22.5f && d > -67.5f)
					vindex = 7; // nw

				score = (distance < 200.0f) ? 1.0f: 1.0f - FLOAT32(distance / (FLOAT32)500);

				if(score < .1f)
					score = .1f;

				rit = reports.find(vindex);

				if(rit != reports.end())
				{
					reports[vindex] += score;
				}
				else
				{
					reports[vindex] = score;
				}
			}
#endif // REPORT_ENEMIES
		}
	}

	// all vehicles that are riding on something
	for (CurrentVehicle = getVehicleHead().getNext(); CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(CurrentVehicle->RideVehicleHostIndex > 0)
		{
			if (CurrentVehicle->ModelData)
			{
				if(vehUpdate1stPersonVehicle(CurrentVehicle) < 0)
					break;
			}else
			{
				vehUpdate3rdPersonVehicle(CurrentVehicle);
			}
		}
	}

	vehVisDamageUpdateCrap();

	if(PlayerVehicle && PlayerVehicle->VehicleDeadTime == 0)
	{
		_FindNearbyFacilities(PlayerVehicle);
	}

	/*
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(VehicleData)
	{
		if(VehicleData->MultiCrewPos < 2)
			_updateMobileSpawn();
	}
	*/

    vehfxUpdate();

//	_HitMessageUpdate();

	// clean up cached vehicles as needed
#ifdef CLIENT_VEHICLE_CACHING
	_UpdateReadyList();
#endif

#ifdef REPORT_ENEMIES
	if(enemyDuration > 0 && frametime >= enemyCheck && (!SCEN_MGR->IsActive() || SCEN_MGR->HasFlag(ScenarioManager::ENEMY_INTEL)))
	{
		if(!reports.empty())
		{
			SINT32 bestIndex			= -1;
			FLOAT32 bestScore		= 0;
			FLOAT32 currentScore	= 0;

			for(rit = reports.begin(); rit != reports.end(); ++rit)
			{
				// -1 is near me
				currentScore = rit->second;

				if(currentScore > bestScore)
				{
					bestIndex	= rit->first;
					bestScore	= currentScore;
				}
			}

			//const char* strength[]		= {"Light", "Moderate", "Heavy"};
			//const char* cardinals[]		= {"north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"};

			wchar_t message[255+1];
			char keyBuffer[30+1];

			// calculate strength
			SINT32 sindex = (bestScore > 9.0f) ? 2 : (bestScore > 3.0f) ? 1 : 0;

			snprintf(keyBuffer, 30, "hud.hqintel.weight.%d", sindex);
			std::wstring strength = uiLocString(keyBuffer);

			if(bestIndex < 0)
			{
				std::wstring intel = uiLocString("hud.hqintel.close");

				swprintf(message, 128, intel.c_str(), strength.c_str());
			}
			else
			{
				snprintf(keyBuffer, 30, "hud.hqintel.dir.%d", bestIndex);

				std::wstring intel = uiLocString("hud.hqintel.far");
				std::wstring dir = uiLocString(keyBuffer);

				swprintf(message, 128, intel.c_str(), strength.c_str(), dir.c_str());
			}

			// add game alert
			uiGameEvent(11, 1, message);

			// add chat event
			uiChatPrintLineColor(colorORANGEALPHA, message);
		}

		enemyCheck = frametime + enemyDuration;
	}
#endif // REPORT_ENEMIES

	logTimerEndMacro();

	return 0;
}

static UINT32 _CheckAndUpdateHitmap(vehVehicle* CurrentVehicle)
{
FLOAT64VECTOR WorldOriginOffset;
piVECTOR TempVec;
UINT32 i;
FLOAT32 MaxDist;
FLOAT32 Ratio;
FLOAT32 Dist;

	if (CurrentVehicle == nullptr)
	{
		return 0;
	}
	// it's kinda dumb we do this separately should just be a part of the normal object update

	// next check ensures that hitmap isn't adjusted every frame unless
	// there are bullet holes on that vehicle
	/*if (CurrentVehicle->VehicleHasBeenHit == 0 &&
		CurrentVehicle->ModelData == nullptr)
	{
		return 0;
	}*/

	if (CurrentVehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}

	if (CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		return 0;
	}

	trnGetWorldOriginOffset(&WorldOriginOffset);

	if (CurrentVehicle->ModelData != nullptr)
	{
		TempVec.x = (FLOAT32)(CurrentVehicle->ModelData->VehiclePos.x - WorldOriginOffset.x);
		TempVec.y = (FLOAT32)(CurrentVehicle->ModelData->VehiclePos.y - WorldOriginOffset.y);
		TempVec.z = (FLOAT32)(-CurrentVehicle->ModelData->VehiclePos.z - WorldOriginOffset.z);
	}
	else
	{
		TempVec.x = (FLOAT32)(CurrentVehicle->PosData.Pos.x - WorldOriginOffset.x);
		TempVec.y = (FLOAT32)(CurrentVehicle->PosData.Pos.y - WorldOriginOffset.y);
		TempVec.z = (FLOAT32)(-CurrentVehicle->PosData.Pos.z - WorldOriginOffset.z);
	}

	mathAddVectors(&TempVec,&TempVec,&playpos);

	MaxDist = 0.0f;
	for (i=0;i<ML_MAX_LODS;i++)
	{
		if (CurrentVehicle->VehicleObjectData->SavedMaxLODRange[i] > MaxDist)
		{
			MaxDist = CurrentVehicle->VehicleObjectData->SavedMaxLODRange[i];
		}
	}
	Dist = TempVec.x * TempVec.x + TempVec.y * TempVec.y + 0.25f * TempVec.z * TempVec.z;
	Ratio = FieldOfView / 90.0f;
	Dist *= Ratio;
	if (Dist < MaxDist)
	{
		weapUpdateVehicleHitmap(CurrentVehicle);
	}

	return 0;
}

extern UINT32 _CameraMode;

UINT32 _UpdateVehicleObject(vehVehicle* Vehicle)
{
	OBJECT * PlaneObject = nullptr;
	mlVehicleData * VehicleData = nullptr;
    FLOAT64VECTOR TempVec = {0, 0, 0};
    FLOAT64ATT TempAtt = {0, 0, 0};

	if (Vehicle->ModelData == nullptr || Vehicle->VehicleObjectData == nullptr || Vehicle->VehicleObjectData->PlaneObject == nullptr)
		return 0;

	PlaneObject = Vehicle->VehicleObjectData->PlaneObject;
	VehicleData = Vehicle->ModelData;

	if (VehicleData->MultiCrewPos < 2 || Vehicle->PosData.LastReceivedTime == 0)
	{
		if(VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0)
		{
			TempAtt = VehicleData->RenderAtt = Vehicle->PosData.RenderAtt = VehicleData->VehicleRealAtt;
			TempVec = VehicleData->RenderPos = Vehicle->PosData.RenderPos = VehicleData->AdjustedForCGPos;
		}else
		{
			// hack on hacks on hacks...
			if(!VehicleObjectAnalysis && (_CameraMode == 0))
			{
                FLOAT64VECTOR vel = VehicleData->VehicleRealVel;

                // Reverse x/z velocities for troopers to avoid 'walking twitch bug'
                if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
                {
                    vel.x = -vel.x;
                    vel.z = -vel.z;
                }

				mlForwardPredict(&TempVec,
								&TempAtt,
								&VehicleData->AdjustedForCGPos,
								&VehicleData->VehicleRealAtt,
								&vel,
								&VehicleData->VehicleRealAttVel,
								VehicleData->LastVehicleUpdateTime);

				// mlForwardPredict supposedly works against the fps system, causing jitters
				// Previously this used the RenderPos, but that wasn't initialized on the
				// first frame, causing bad juju.
				if ( persIsPersonaVehicleType(VEH_TYPE_PERSON) )
					TempVec = VehicleData->VehiclePos;

				VehicleData->RenderAtt = TempAtt;
				VehicleData->RenderPos = TempVec;

				Vehicle->PosData.RenderAtt = TempAtt;
				Vehicle->PosData.RenderPos = TempVec;

				// HACK to prevent joiner in multicrew from warping to the ocean center.
				Vehicle->PosData.Pos = TempVec;
			}else
			{
				TempAtt = VehicleData->RenderAtt = Vehicle->PosData.RenderAtt = VehicleData->VehicleRealAtt;
				TempVec = VehicleData->RenderPos = Vehicle->PosData.RenderPos = VehicleData->AdjustedForCGPos;
			}
		}
	} else
	{
		TempAtt = VehicleData->RenderAtt = Vehicle->PosData.RenderAtt;
		TempVec = VehicleData->RenderPos = Vehicle->PosData.RenderPos;

		VehicleData->AdjustedForCGPos = TempVec;
		VehicleData->VehiclePos = VehicleData->VehicleLastPos = TempVec;
		VehicleData->VehicleAttMat = Vehicle->PosData.AttMat;
		math3dInvertMatrix(&VehicleData->VehicleAttInvMat,&VehicleData->VehicleAttMat);
		VehicleData->VehicleRealVel = Vehicle->PosData.Vel;
		VehicleData->VehicleVel.x = -VehicleData->VehicleRealVel.x;
		VehicleData->VehicleVel.y = VehicleData->VehicleRealVel.y;
		VehicleData->VehicleVel.z = -VehicleData->VehicleRealVel.z;

		VehicleData->VehicleRealAttVel = Vehicle->PosData.AttVel;
		VehicleData->VehicleAttVel.Pitch = VehicleData->VehicleRealAttVel.Pitch;
		VehicleData->VehicleAttVel.Roll = -VehicleData->VehicleRealAttVel.Roll;
		VehicleData->VehicleAttVel.Yaw = -VehicleData->VehicleRealAttVel.Yaw;

		VehicleData->VehicleRealAtt = Vehicle->PosData.Att;
		VehicleData->VehicleAtt.Pitch = -VehicleData->VehicleRealAtt.Pitch;
		VehicleData->VehicleAtt.Roll = VehicleData->VehicleRealAtt.Roll;
		VehicleData->VehicleAtt.Yaw = -VehicleData->VehicleRealAtt.Yaw;

		VehicleData->VehicleAcc = Vehicle->PosData.Acc;
	}

    mlMoveAndRotateObject(PlaneObject,&TempVec,&TempAtt);

	if (Vehicle->VehicleObjectData->HitMap != nullptr)
	{
		mlApplyVelocityToObject(Vehicle->VehicleObjectData->HitMap,&VehicleData->VehicleRealVel);
	}
	else
	{
		mlApplyVelocityToObject(PlaneObject,&VehicleData->VehicleRealVel);
	}

	vehUpdateVehicleAnimation(Vehicle);

	if (PlaneObject != nullptr)
	{
		_UpdateVehicleBoundingBox(Vehicle->VehicleObjectData,PlaneObject->child);
	}

	return 0;
}

static UINT32 _GetNewVehicle(vehVehicle** VehiclePtr) // VEHSTUFF
{

    if (VehiclePtr == nullptr) return 1;

	vehVehicle* Vehicle = nullptr;

    if ( IS_OFFLINE )
    {
        if(g_freeVeh.empty()) return 1;
        Vehicle = g_freeVeh.front();
        g_freeVeh.pop();
    }
    else
    {
        Vehicle = new vehVehicle();
    }

	Vehicle->clear();

	if (_AddToListHead(Vehicle) == 1)
	{
		return 1;
	}

    *VehiclePtr = Vehicle;
	_NumVehicles++;

	return 0;
}

// -----------------------------------------

#define xDELETE_IMMEDIATELY
#define DELETE_VEHICLES_DELAY_TIME 10000

struct TempSave
{
    vehVehicle* vehicle;
    SINT32 timeToRemove;

    TempSave(vehVehicle* v, SINT32 t)
    {
        vehicle = v;
        timeToRemove = t + DELETE_VEHICLES_DELAY_TIME;
    }
    TempSave()
    {
        vehicle = nullptr;
        timeToRemove = 0;
    }
};

static std::map<vehVehicle*,TempSave> TempSaveVehicles;

// -----------------------------------------

static UINT32 _AddToFreeListOrDelete(vehVehicle* Vehicle) // This routine must be called last as Vehicle is removed here
{
    if(!Vehicle) return 1;

    weapAmmoClearVehicle(Vehicle); // make sure we don't use vehicle after its dead

    if ( IS_OFFLINE )
    {
        if(Vehicle->InUse == 0) return 1;

        Vehicle->InUse = 0;

        g_freeVeh.push(Vehicle);
    }
    else
    {
        Vehicle->InUse = 0;
        assert ( Vehicle->getNext() == nullptr );

#ifdef DELETE_IMMEDIATELY
        delete Vehicle;
#else
        if ( TempSaveVehicles.find(Vehicle) != TempSaveVehicles.end() )
        {
            assert(TempSaveVehicles.find(Vehicle) == TempSaveVehicles.end());
        }
        else
        {
            std::map<vehVehicle*,TempSave>::iterator it = TempSaveVehicles.begin();
            while (it != TempSaveVehicles.end())
            {
                if ((*it).second.timeToRemove < frametime )
                {
                    //printf("removing %p\n", (*it).second.vehicle );
                    delete (*it).second.vehicle;
                    TempSaveVehicles.erase(it++);
                }
                else
                {
                    it++;
                }
            }
            TempSaveVehicles[Vehicle]=TempSave(Vehicle,frametime);
            //printf("temp saving %p\n", Vehicle );

        }

#endif
    }

	return 0;
}

static void _CleanVehicle(vehVehicle* v)
{
	//uiChatPrintLine("Cleaning Vehicle for vehicle %d", v->VehicleHostIndex);

	if (v->VehicleObjectData != nullptr)
	{
		//uiChatPrintLine("Cleaning Vehicle Object Data!");
		mlobjectKillVehicleObject(&v->VehicleObjectData);
		v->VehicleObjectData = nullptr;
	}

	if(v->spaceID)
	{
		dSpaceDestroy(v->spaceID);
		v->spaceID = 0;
	}

	if(v->collider)
	{
		delete v->collider;
		v->collider = nullptr;
	}

	v->VehicleType = nullptr;
	v->VehicleNum = 0;
	v->Flags = 0;
	v->InUse = FALSE;
	v->VehicleHostIndex = 0;

	// reset our vehicle
	v->clear();
}

#ifdef CLIENT_VEHICLE_CACHING

static void _processvehicleobject(OBJECT& o, void* userdata)
{
	if(userdata)
		obmainMoveToObjectTree(&o);
	else
		obmainMoveToNotObjectTree(&o);
}

static UINT32 _AddToReadyList(vehVehicle* Vehicle)
{
	if(gVCache == 0) return 0; // disabled
    if(Vehicle == nullptr) return 0;
    if(Vehicle->InUse == 0) return 0;
	if(Vehicle->VehicleObjectData == nullptr) return 0;
	if(Vehicle->isHulked()) return 0;
	if(Vehicle->VehicleType->Type < VEH_TYPE_PERSON) return 0; // must be a vehicle
	if(gVCache == 1 && Vehicle->VehicleType->Type != VEH_TYPE_PERSON) return 0; // troops only mode and we're not a troop
	if(Online && Vehicle->isSTO()) return 0;

	VEHICLE_READY_MAP::iterator found = _readyMap.find(Online ? Vehicle->Chat_Handle : 0);

#ifdef CLIENT_VEHICLE_CACHING_DEBUG
	const wchar_t* callsign = chatGetWideCallsign(Vehicle->Chat_Handle);

	uiChatPrintLine(L"CACHE: Adding %ls to ready list with id: %d vt: %ls",
							callsign,
							Vehicle->VehicleHostIndex,
							UTF16(Vehicle->VehicleType->Id));
#endif // CLIENT_VEHICLE_CACHING_DEBUG

	// check to see if this player already has an entry and remove it if so
	if(found != _readyMap.end() && found->second != Vehicle)
	{
#ifdef CLIENT_VEHICLE_CACHING_DEBUG
		vehVehicle* cached = found->second;

		uiChatPrintLine(L"CACHE: %ls was already in the ready list with id: %d vt: %ls",
			callsign,
			cached->VehicleHostIndex,
			UTF16(cached->VehicleType->Id));
#endif // CLIENT_VEHICLE_CACHING_DEBUG

		_RemoveFromReady(found->first, true);
	}

	// its not already in the ready map
	_readyMap[Online ? Vehicle->Chat_Handle : 0] = Vehicle;

	// set active flags
	Vehicle->InUse = 0;
	Vehicle->HideVehicle = 1;

	_setVehicleState(Vehicle, false);

	obmainFindCallback(Vehicle->VehicleObjectData->PlaneObject, _processvehicleobject, nullptr);

	if(Vehicle->spaceID)
	{
		// disable our vehicle's space
		dGeomDisable((dGeomID)Vehicle->spaceID);

		// disable all geoms for this vehicle
		//physicsRecurseAndToggleObjectCollider(Vehicle->VehicleObjectData->PlaneObject, false);
	}

	return 1;
}

static vehVehicle* _RemoveFromReady(ChatHandle ch, bool freeVehicle)
{
	vehVehicle* Vehicle = nullptr;
	VEHICLE_READY_MAP::iterator found = _readyMap.find(Online ? ch : 0);

#ifdef CLIENT_VEHICLE_CACHING_DEBUG
	const wchar_t* callsign = chatGetWideCallsign(ch);

	/*
	if(freeVehicle)
		uiChatPrintLine(L"CACHE: Freeing %ls from ready list!", callsign);
	else
		uiChatPrintLine(L"CACHE: Getting %ls from ready list!", callsign);
	*/
#endif // CLIENT_VEHICLE_CACHING_DEBUG

	if(found != _readyMap.end())
	{
		Vehicle = found->second;

		Vehicle->InUse = 1;
		Vehicle->HideVehicle = 0;

		if(Vehicle->VehicleObjectData)
			obmainFindCallback(Vehicle->VehicleObjectData->PlaneObject, _processvehicleobject, &Vehicle);
		else
			freeVehicle = true;

#ifdef CLIENT_VEHICLE_CACHING_DEBUG
		uiChatPrintLine(L"CACHE: Found cached vehicle(%p) %ls in ready list with id: %d vt: %ls",
							Vehicle,
							callsign,
							Vehicle->VehicleHostIndex,
							UTF16(Vehicle->VehicleType->Id));
#endif // CLIENT_VEHICLE_CACHING_DEBUG

		_readyMap.erase(found);

		if(freeVehicle)
		{
			// add it back to the free list for future use

			// cleanup resources
			_CleanVehicle(Vehicle);
			_AddToFreeListOrDelete(Vehicle);

			// we're free!
			Vehicle = nullptr;
		}
		else
		{
			_setVehicleState(Vehicle, true);
		}
	}

	return Vehicle;
}

static bool _UpdateReadyList(bool force, bool report)
{
#ifdef CLIENT_VEHICLE_CACHING_REPORT
	static SINT32 _nextCacheReport = 0;

	if(frametime >= _nextCacheReport || report)
	{
		FLOAT32 vCacheAvg = (_vCacheTries) ? (FLOAT32)_vCacheHits / (FLOAT32)_vCacheTries: 0.0f;
		uiChatPrintLine(L"CACHE: hits = %u, tries = %u, success = %d%% (%d vehicles in the cache)", _vCacheHits, _vCacheTries, (int)(100 * vCacheAvg), (int)_readyMap.size());
		_nextCacheReport = frametime + 30000; // every 30 seconds

		if(frametime > 120000)
			uiDotCommand("/vcount");
	}
#endif

	if(_readyMap.empty())
		return false;

	static SINT32 _nextCheck = 0;

	if(!force && frametime < _nextCheck) // !force
		return false;

	VEHICLE_READY_MAP::iterator itr = _readyMap.begin();
	bool removed = false;

	for(; itr != _readyMap.end(); ++itr)
	{
		vehVehicle* v = itr->second;

		if(Online && (force || (frametime - v->LastUpdate) >= 300000)) //old than 5 minutes, kill it force ||
		{
#ifdef CLIENT_VEHICLE_CACHING_DEBUG
			const wchar_t* callsign = chatGetWideCallsign(v->Chat_Handle);
			uiChatPrintLine(L"CACHE: Ready list update, removing cached vehicle(%p) - %ls", callsign);
#endif // CLIENT_VEHICLE_CACHING_DEBUG
			_RemoveFromReady(v->Chat_Handle, true);
			removed = true;
			break;
		}
	}

	_nextCheck = frametime + 500;

	return removed; // did we remove one?
}
#endif

static vehVehicle& getVehicleHead()
{
    return *_VehicleHead;
}

UINT32 vehGetVehicleHead(vehVehicle** Vehicle)
{
	*Vehicle = getVehicleHead().getNext();

	return 0;
}

static UINT32 _RemoveFromList(vehVehicle* Vehicle)
{
	if (Vehicle == nullptr)
	{
		return 1;
	}


    vehVehicle*p, *prev;
    vehVehicle& head = getVehicleHead();
    prev = &head;
    p = head.getNext();
    while ( p != nullptr )
    {
        if ( p == Vehicle )
        {
            prev->setNext( p->getNext());
            p->setNext( nullptr );
            break;
        }
        prev = p;
        p = p->getNext();
    }

    return 0;

/*

	if (Vehicle->Next != nullptr)
	{
		Vehicle->Next->Prev = Vehicle->Prev;
	}

	if (Vehicle->Prev != nullptr)
	{
		Vehicle->Prev->Next = Vehicle->Next;
	}

	Vehicle->Prev = Vehicle->Next = nullptr;

	return 0;
*/
}

static UINT32 _AddToListHead(vehVehicle* Vehicle)
{
	if (Vehicle == nullptr)
	{
		return 1;
	}

    Vehicle->setNext(getVehicleHead().getNext());
	getVehicleHead().setNext(Vehicle);
	return 0;
/*
	Vehicle->Prev = &getVehicleHead();
	Vehicle->Next = getVehicleHead().Next;
	if (getVehicleHead().Next != nullptr)
	{
		getVehicleHead().Next->Prev = Vehicle;
	}
	getVehicleHead().Next = Vehicle;

	return 0;
*/
}
/*
bool _ReadyAirplaneBailout(piVECTOR* pos, piVECTOR* aim)
{
	static SINT32 bail = 0;

	if(!pos)
	{
		persSetExternalCameraPos(0, 20, 15);
		//persSetExternalCameraAdjuster(nullptr);
		//persSetExternalCameraTarget(1000.0f, 0, 0, 30.0f); // rotate around the horizontal plane for 30 seconds if we're slow or not moving

		bail = frametime + 1000;
	}

	if(frametime >= bail)
	{
		// set our bail camera
		persSetExternalCameraAdjuster(_AirplaneBailout);

		// init it
		_AirplaneBailout(nullptr, nullptr);
	}
	else if(frametime >= bail - 750)
	{
		vehVehicle* player = persGetPersonaVehiclePtr();

		// force all canopies to come off when bailing!
		for(int i = 0; i < VEH_MAX_STATE_BITS; i++)
		{
			vehanimVehicleStateType& state = player->VehicleType->StateType[i];

			if(state.StateType == VehAnim::STATE_TYPE_VISDAMAGE && strstr((const char*)state.Name, "canopy"))
			{
				// let others know
				vehAnimChangeVehicleStateByNumber(player, i, TRUE);

				// update my vehicle
				vehVisDamageGroupDestroyed(player, state.Name, FALSE, player->StateAnimation.FirstUpdateProcessed);
			}
		}
	}

	return true;
}
*/

bool _AirplaneBailout(piVECTOR* pos, piVECTOR* aim)
{
	static SINT32 stage = 0;
	static SINT32 nextStage = 0;
	static Vec3d _camPos;
	static Vec3d _lookAt;
	//static SINT32 _expires = 0;
	static SINT32 _lastUpdate = 0;
	static SINT32 _firstUpdate = 0;
	static SINT32 _nextBailout = 0; // next time we will try to make another crew member jump!
	static int _bailoutCount = 1; // always count me
	static bool _exited = false;

	if(!pos)
	{
		stage = -1;
		nextStage = frametime;
	}

	vehVehicle* v					= persGetPersonaVehiclePtr();
	bool myPilot					= (_bailout && _bailout->isMyPilot()) ? true: false;
	Vec3d camOffset					= Vec3d(5.0, -30.0f, 0.0f);

	if(frametime >= nextStage && stage < 2)
	{
		++stage;

		// switch the change so we can do some inits
		switch(stage)
		{
			case 0: // show the plane
			{
				persSetExternalCameraPos(0, 20, 25);
				nextStage = frametime + 1000;
				_exited = false;
			}
			break;
			case 1: // pop canopy if we have one
			{
				vehVehicle* player = persGetPersonaVehiclePtr();
				bool foundCanopy = false;

				// force all canopies to come off when bailing!
				for(int i = 0; i < VEH_MAX_STATE_BITS; i++)
				{
					vehanimVehicleStateType& state = player->VehicleType->StateType[i];

					if(state.StateType == VehAnim::STATE_TYPE_VISDAMAGE && strstr((const char*)state.Name, "canopy"))
					{
						// let others know
						vehAnimChangeVehicleStateByNumber(player, i, TRUE);

						// update my vehicle
						vehVisDamageGroupDestroyed(player, state.Name, FALSE, player->StateAnimation.FirstUpdateProcessed);

						foundCanopy = true;
					}
				}

				nextStage = (foundCanopy) ? frametime + 2000: frametime;
			}
			break;
			case 2: // init our bailout
			{
				if(v)
				{
					FLOAT64VECTOR& vpos = v->ModelData->VehiclePos;
					FLOAT64ATT& vatt = v->ModelData->VehicleAtt;

					if(!myPilot)
					{
						WorldPosition_m position(vpos.x, vpos.y, vpos.z + GetRandom(-2.0f, -.5f));
						AttitudeVector attitude(0, 0, (FLOAT32)vatt.Yaw);

						const vehType* vt = BailoutSTO::VTYPE(v);
						if(!vt) return false;

						// this is our focal point
						_bailout = new BailoutSTO(0, vt, position, attitude);

						// make us the owner; for tracking purposes
						_bailout->setOwner(v->Chat_Handle);

						// add us to the sto index
						ServerTrackedObject::addToIndex(_bailout);
					}

					piMATRIX transform = IdentityMatrix;

					piVECTOR attVec;
					attVec.x = 0;
					attVec.y = 0;
					attVec.z = (float)vatt.Yaw; // just interested in the yaw

					mathEul_ToMatrix(&transform, EulOrdXYZr, &attVec);

					Matrixd att = Matrixd(transform);

					// temporary vector for calculating various things

					// rotate it towards our velocity vector
					camOffset = camOffset * att;

					_camPos.set(vpos);
					_camPos += camOffset;

					_lookAt.set(vpos);

					// calculated in vehmainBailout()
					if(!myPilot)
						_bailout->setLifeSpan((unsigned int)_bailoutLife);

					_firstUpdate = frametime;
					_lastUpdate = frametime;
					_nextBailout = frametime + GetRandom(1000,2000);
					_bailoutCount = 1;

					// put us into an external cam
					//persSetExternalCamera(1, 0, 0);

					return true;
				}
			}
			break;
		}
	}

	if(stage >= 2)
	{
		//if(v && v->VehicleDeadTime)
		//	return true;

		if(!_nextBailout)
			_nextBailout = frametime + GetRandom(0,1000);

		vehVehicle* bv = (_bailout) ? _bailout->getVehicle(): nullptr;

		if(v && bv)
		{
			FLOAT64VECTOR& vpos = v->ModelData->VehiclePos;
			FLOAT64ATT& vatt = v->ModelData->VehicleAtt;
			Matrixd att = Matrixd(bv->PosData.AttMat);

			/*
			piMATRIX transform = IdentityMatrix;

			piVECTOR attVec;
			attVec.x = 0;
			attVec.y = 0;
			attVec.z = _bailout->getVehicle()->PosData.AttMat; // just interested in the yaw

			mathEul_ToMatrix(&transform, EulOrdXYZr, &attVec);

			att.set(transform);
			*/

			_lookAt.set(vpos);

			Vec3d bpos = Vec3d(bv->PosData.Pos);
			Vec3d to = _lookAt - bpos;

			// get rid of z
			//to.z() = 0;

			// normalize
			to.normalize();

			Vec3d forward = Vec3d(0,1.0f,0);

			forward = forward * att;
			camOffset = camOffset * att;

			//Matrixd rotate;
			//rotate.makeRotate(forward, to);
			att.makeRotate(forward, to);

			//camOffset = camOffset * rotate;
			camOffset = camOffset * att;

			_camPos = bpos;
			_camPos += camOffset;

			//_camPos.z() -= BailoutSTO::FALL_RATE() * (FLOAT64(frametime - _lastUpdate) / (FLOAT64)1000.0f);

			// bail out other crew members if needed
			if(!myPilot &&
				frametime >= _nextBailout &&
				v->VehicleType->VehicleIsMultiCrew &&
				_bailoutPos < ML_MAX_CREW_POSITIONS &&
				_bailoutCount < 3)
			{
				// foreach position
				UINT32* mpos	= v->VehicleType->MultiCrewPos;
				Vec3d bpos		= Vec3d(vpos);

				// skip pilot
				for(UINT32 i = _bailoutPos; i < ML_MAX_CREW_POSITIONS; i++)
				{
					// valid position and alive?
					if(mpos[i] == 0 || persGetCrewIsDead(i))
						continue;

					_bailoutPos = i + 1;

					bpos.x() = vpos.x + GetRandom(-5.0f, 5.0f);
					bpos.y() = vpos.y + GetRandom(-5.0f, 5.0f);
					bpos.z() = vpos.z + GetRandom(-5.0f, -1.0f);

					WorldPosition_m position(bpos.x(), bpos.y(), bpos.z());
					AttitudeVector attitude(0, 0, (FLOAT32)vatt.Yaw);

					const vehType* vt = BailoutSTO::VTYPE(v);

					if(vt)
					{
						BailoutSTO* sto = new BailoutSTO(0, vt, position, attitude);

						sto->setLifeSpan((unsigned int)(_bailoutLife - (frametime - _bailoutTime)));

						if(Online)
						{
							sto->request(PlayerClient()->ChatConnection());
							delete sto;
						}
						else
						{
							ServerTrackedObject::addToIndex(sto);
						}

						++_bailoutCount;
					}

					// process the next bailout in 1 to 3 seconds
					_nextBailout = frametime + GetRandom(1000,2000);

					// only do one at a time
					break;
				}
			}
		}

		if(!_exited)
		{
			if((frametime - _firstUpdate) >= 30000)
				_exited = true;
			else if(!_bailout || _bailout->expired())
				_exited = true;

			if(_exited)
				vehExitVehicle(1, 0, 0);
		}

		if(pos)
		{
			pos->x = -float(_camPos.x() - WorldOriginOffset.x);
			pos->y = -float(_camPos.y() - WorldOriginOffset.y);
			pos->z = float(_camPos.z());
		}

		if(aim)
		{
			aim->x = -float(_lookAt.x() - WorldOriginOffset.x);
			aim->y = -float(_lookAt.y() - WorldOriginOffset.y);
			aim->z = float(_lookAt.z());
		}

		_lastUpdate = frametime;
	}

	return true;
}

vehVehicle*vehFindVehicle(UINT32 HostIndex)
{
vehVehicle*cur;

	for (cur = getVehicleHead().getNext()
		; cur != nullptr; cur = cur->getNext())
	{
		if (HostIndex == cur->VehicleHostIndex)
			return cur;
	}

	return nullptr;
}

LOGICAL vehValid(vehVehicle* Vehicle)
{
vehVehicle*cur;

	for (cur = getVehicleHead().getNext()
		; cur != nullptr; cur = cur->getNext())
	{
		if (Vehicle == cur)
			return TRUE;
	}

	return FALSE;
}

vehVehicle*vehFindVehicleByChatHandle(const ChatHandle chathandle)
{
	for ( vehVehicle* cur = getVehicleHead().getNext(); cur != nullptr; cur = cur->getNext())
	{
		if(chatGetLowercaseHandle(cur->Chat_Handle) == chatGetLowercaseHandle(chathandle))
			return cur;
	}

	return nullptr;
}

// kills the vehicle, cleans up the vehicle, and puts it back on the free list
UINT32 vehKillVehicle(vehVehicle* Vehicle)
{
	if(Vehicle == nullptr) return 0;
    if(Vehicle->InUse == 0) return 0;

	// process flags one more time to catch dead guys that didn't ragdoll
	if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		fpProcessVehicleAnimationFlags(Vehicle);

	if(cfgPrefsGetBool("app.deathicons", true) &&
		!Vehicle->ModelData &&
		Vehicle->VehicleType)
	{
		bool addIcon = false;

		if(Vehicle->VehicleType->Type == VEH_TYPE_GROUND && Vehicle->isHulked())
		{
			addIcon = true;
		}
		else if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON &&
				Vehicle->VehicleObjectData &&
				Vehicle->VehicleObjectData->PlaneObject &&
				Vehicle->VehicleObjectData->PlaneObject->animModel &&
				Vehicle->VehicleObjectData->PlaneObject->animModel->deathState)
		{
			addIcon = true;
		}

		if(addIcon)
		{
			vehVehicle* player = persGetPersonaVehiclePtr();

			if(player && (Vehicle->Side == player->Side || SCEN_MGR->HasFlag(ScenarioManager::ENEMY_DEATHS))) // && !Vehicle->isSTO())
			{
				ChatHandle& handle = Vehicle->Chat_Handle;
				Vehicle->PosData.Pos.z += 1.0f;

				PersHUDManager::HUDDeath* deathIcon = new PersHUDManager::HUDDeath(handle, Vehicle->PosData.Pos);

				if(Vehicle->Side != player->Side)
					deathIcon->color = gIconEnemyColor;

				HUD_MGR->AddObject(deathIcon);
			}
		}
	}

    vehfxDetachVehicle(Vehicle->iFXVehicleID);
	Vehicle->iFXVehicleID = 0;

	trnAutoVehicleDisappearing(Vehicle);

	// first detach the player's persona if necessary
	persDetachFromVehicle(Vehicle);

	vehKillVehicleAnimation(Vehicle);
	vehVisDamageRemoveAllCrap(Vehicle);

#ifdef CLIENT_VEHICLE_CACHING
	_RemoveFromList(Vehicle);

	if(Vehicle->ModelData == nullptr)
	{
		// cache tps vehicles for a period in case they pop back on our list
		if(!_AddToReadyList(Vehicle))
		{
#ifdef CLIENT_VEHICLE_CACHING_DEBUG
			const wchar_t* callsign = chatGetWideCallsign(Vehicle->Chat_Handle);
			uiChatPrintLine(L"CACHE: Failed to add %ls to ready list with id: %d vt: %ls",
							callsign,
							Vehicle->VehicleHostIndex,
							UTF16(Vehicle->VehicleType->Id));
#endif // CLIENT_VEHICLE_CACHING_DEBUG
			_CleanVehicle(Vehicle);
			_AddToFreeListOrDelete(Vehicle);
		}
	}
	else
	{
		// first person vehicles are always killed immediately

		if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		{
			if (Vehicle->ModelData)
				fpKill(*Vehicle->ModelData);
		}

		mlKillVehicleData(Vehicle->ModelData);
		memFree(Vehicle->ModelData);
		Vehicle->ModelData = nullptr;

		_CleanVehicle(Vehicle);
		_AddToFreeListOrDelete(Vehicle);
	}

#else
	_RemoveFromList(Vehicle);

	if (Vehicle->ModelData != nullptr)
	{
		if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		{
			if (Vehicle->ModelData)
				fpKill(*Vehicle->ModelData);
		}

		mlKillVehicleData(Vehicle->ModelData);
		memFree(Vehicle->ModelData);
		Vehicle->ModelData = nullptr;
	}

	_CleanVehicle(Vehicle);
    _AddToFreeListOrDelete(Vehicle);
#endif // CLIENT_VEHICLE_CACHING

	_NumVehicles--;

	return 0;
}

UINT32 vehGetHitmapComponentThickness(FLOAT64 * Thickness,vehVehicle* Vehicle,UINT32 PartNum)
{
vehType * VehicleType;

	if (Thickness == nullptr || Vehicle == nullptr)
	{
		return 0;
	}

	if(Vehicle->isHulked())
	{
		*Thickness = 1000000;
		return 0;
	}

	if (PartNum == 0)
	{
		*Thickness = 0.0;
		return 0;
	}

	VehicleType = Vehicle->VehicleType;
	if (PartNum > VehicleType->NumDamageObjects)
	{
		*Thickness = 0.0;
		return 0;
	}

	*Thickness = VehicleType->DamageArmorThickness[PartNum-1];

	return 0;
}

static UINT32 _CheckForDamage(vehVehicle* CurrentVehicle)
{
	vehDamageQue * DQ;
	UINT32 TempNum;
	UINT32 i;

	if (CurrentVehicle == nullptr)
	{
		return 0;
	}

	DQ = &CurrentVehicle->DamageQue;

	DQ->LastCheckTime = frametime;

    // Server Tracked Objects (STO) send total joules. Denoted by negative STO ID
    if(CurrentVehicle->isSTO())
    {
        _sendSTODamage(CurrentVehicle, DQ);
    }
    else
    {
	    // ok we're sending out a damage update
		_SendDamageUpdate(CurrentVehicle, DQ);
    }

	// reset que
//	memset(DQ->DamageAccrued,0,sizeof(FLOAT32)*VEH_MAX_DAMAGE_OBJECTS);
//	memset(DQ->NumRoundsHit,0,sizeof(FLOAT32)*VEH_MAX_DAMAGE_OBJECTS);

	DQ->LastSendTime = frametime;
	DQ->SendMore = 0;
	TempNum = 0;

	for (i=1;i<VEH_MAX_DAMAGE_OBJECTS;i++)
	{
		if (DQ->NumRoundsHit[i] > 0)
		{
			DQ->SendMore = 1;
			break;
		}
	}
	if (DQ->SendMore == 0)
	{
		DQ->FirstRoundTimer = 0;
	}

	return 0;
}

static void _sendSTODamage(vehVehicle*veh, vehDamageQue* que)
{
    // VehicleNum is the STO -uid().
    ServerTrackedObject* sto = ServerTrackedObject::lookup(-veh->VehicleNum);

    if(!sto)
    {
        Write_Log(LOG_OLD, LOG_INFO, "[_sendSTODamage] FAILED to find STO for %d", -veh->VehicleNum);
        return;
    }

    for(SINT32 i = 1; i < VEH_MAX_DAMAGE_OBJECTS; i++)
    {
        if(que->NumRoundsHit[i] > 0)
        {
			// If the damage accrued would wrap a SIN64,
			// apply joules will send multiple packets
			const SINT64 joules = (SINT64)que->DamageAccrued[i];
            sto->applyJoules(joules);

            que->NumRoundsHit[i] = 0;
            que->DamageAccrued[i] = 0.0f;
        }
    }

    sto = nullptr;
}

static UINT32 _SendDamageUpdate(vehVehicle* CurrentVehicle, vehDamageQue * DQ)
{
	Vehicle::Hit hit(CurrentVehicle->VehicleHostIndex);
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	UINT32& Count = (UINT32&)hit.m_components._M_size;
	Count = 0;

	for (UINT32 i = 1; Count < VEH_MAX_OBJ_HIT - 1 && i < VEH_MAX_DAMAGE_OBJECTS; ++i)
	{
		if (DQ->NumRoundsHit[i] <= 0)
			continue;

		Write_Debug("WEAP HIT component %s, num individual hits: %d, damage: %f",
							CurrentVehicle->VehicleType->DamageObjects[i-1],
							DQ->NumRoundsHit[i],
							DQ->DamageAccrued[i]);

		Vehicle::Hit::Component& comp = hit.m_components[Count];

		comp.compID = i;
		comp.isImportant = DQ->isImportant[i];
		comp.NumHits = DQ->NumRoundsHit[i];
		comp.m_damage = (UINT64)DQ->DamageAccrued[i];
		DQ->NumRoundsHit[i] = 0;
		DQ->DamageAccrued[i] = 0.0;

		Count++;
	}

	if(Count == 0)
		return 0;

	Write_Log(LOG_COMM, LOG_INFO, "WEAP COMM Sending hit packet for vehicle, index = %u, ident = %u, handle = %s, hits = %u",
					CurrentVehicle->VehicleHostIndex,
					CurrentVehicle->VehicleNum,
					CurrentVehicle->Handle,
					Count);

	if (CurrentVehicle->VehicleHostIndex == PlayerVehicle->VehicleHostIndex &&
		CurrentVehicle->VehicleNum == PlayerVehicle->VehicleNum &&
		(PlayerVehicle->ModelData == nullptr ||
		 PlayerVehicle->ModelData->MultiCrewPos == 0))
	{
		// hitting self, don't send to host
		vehProcessVehicleHit(hit);
	}
	else
	{
		teulSend(PlayerClient()->CellConnection(), commCLIENT_HIT, &hit);
	}

	return 0;
}

static void vehApplyVehicleHit(const Vehicle::Hit& hit)
{
	mlVehicleData* ModelData;
	vehVehicle* Vehicle;
	OBJECT * Object;

	Vehicle = vehFindVehicle(hit.m_target);

	if (!Vehicle || !Vehicle->ModelData)
		return;

	ModelData = Vehicle->ModelData;

	const size_t numComponents = hit.m_components.size();

	for (UINT32 i=0; i < numComponents;i++)
	{
		const Vehicle::Hit::Component& comp = hit.m_components[i];
		const UINT16& compID = comp.compID;

		if ( compID >= VEH_MAX_DAMAGE_OBJECTS )
		{
			Write_Debug("Received bad hit damage component number");
			continue;
		}

		///TODO: Troy: This was always being set to true before,
		///presumably because the host would do things to the flag
		///mask being used.
		Vehicle->DamageQue.isImportant[compID] = true; //comp.isImportant;

		///TODO: Troy: Because the is important was always being
		///set TRUE before, the code here was always being executed.
		///Does it need to be?
		if (Vehicle->DamageQue.isImportant[compID] )
		{
			// got multiple hit messages at once
			if (ModelData->NewDamage == 1)
			{
				ModelData->DamageAccrued[compID] += comp.m_damage;
				ModelData->NumRoundsHit[compID] += comp.NumHits;
			}
			else
			{
                //printf( "applying damage to compID %d\n",compID );
				ModelData->DamageAccrued[compID] = (float)comp.m_damage;
				ModelData->NumRoundsHit[compID] = comp.NumHits;
			}
		}
	}

	ModelData->NewDamage = 1;

	Write_Log(LOG_COMM, LOG_INFO, "WEAP COMM Received hit message from host, vehicle index = %d",
				hit.m_target);

	g_MyHitMessages.push_back(hit);

	for (size_t i = 0; i < numComponents; i++)
	{
		const Vehicle::Hit::Component& comp = hit.m_components[i];

		if (comp.NumHits > 0)
		{
			Write_Log(LOG_COMM, LOG_INFO, "WEAP COMM Hit part %s, damage approx %lf/hit, hits = %d",
						Vehicle->VehicleType->DamageObjects[comp.compID-1],
						ModelData->DamageAccrued[comp.compID] / comp.NumHits,
						comp.NumHits);
		}
	}

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	if (PlayerVehicle == Vehicle && PlayerVehicle != nullptr)
	{
		// play hit sounds
		if (PlayerVehicle->VehicleObjectData != nullptr)
		{
			if (PlayerVehicle->VehicleObjectData->PlaneObject != nullptr)
			{
				for (size_t i=0;i<numComponents;i++)
				{
					const Vehicle::Hit::Component& comp = hit.m_components[i];
					const UINT16& compID = comp.compID;

					if (compID < VEH_MAX_DAMAGE_OBJECTS)
					{
						Object = nullptr;
						if (PlayerVehicle->VehicleObjectData->HitMap != nullptr)
						{
							ogrpFindSubObjectByName(&Object,
												PlayerVehicle->VehicleObjectData->HitMap,
												(SCHAR*)PlayerVehicle->VehicleType->DamageObjects[compID]);
						}
						if (Object == nullptr)
						{
							Object = PlayerVehicle->VehicleObjectData->PlaneObject;
						}

						persAddHitSoundsForHitComponent(Object, comp.NumHits, (float)comp.m_damage);
					}
				}
			}
		}
	}
	else
	{
		Write_Debug("WEAP Hit message DOES NOT applies to player's vehicle (error condition)");
	}

#ifdef BETA_RELEASE
	if(SuperTroop)
	{
	   // for (size_t i=0;i<numComponents;i++) Ticket # 5376
		for(size_t i = 0; i < VEH_MAX_DAMAGE_OBJECTS; i++)
		{
			ModelData->DamageAccrued[i] = 0;
			ModelData->NumRoundsHit[i] = 0;
		}
	}
#endif //BETA_RELEASE
}

UINT32 vehProcessVehicleHit(const Vehicle::Hit& hit)
{
/*	vehVehicle* TargetVehicle = vehFindVehicle(vehHit->target);
	if (!TargetVehicle || !TargetVehicle->ModelData) return 1;
	vehVehicle* ShooterVehicle = vehFindVehicle(vehHit->shooter);
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	if(ShooterVehicle && TargetVehicle == PlayerVehicle && ShooterVehicle != PlayerVehicle
	&& ShooterVehicle->VehicleType->Type == VEH_TYPE_PERSON
	&& TargetVehicle->VehicleType->Type  == VEH_TYPE_PERSON)
	{
		vehHit->timeStamp = (UINT16)vehHit->timeStamp;// fucking short timestamp bullshit
		_DeferredHitMessages.push_back(*vehHit);
	}else*/
	{
		vehApplyVehicleHit(hit);
	}
	return 0;
}

UINT32 vehFindHitmapObjectNumByName(UINT32 * ComponentNum,
									UCHAR * ComponentName,
									UINT32 VehicleNum)
{
	vehType const* VehicleType;
	UINT32 i;

	VehicleType = vehtypeGetByIndex(VehicleNum);
	if(VehicleType == nullptr) return 0;

	if (strlen((char*)ComponentName) == 0)
	{
		return 0;
	}

	*ComponentNum = 0;
	for (i=0;i<VEH_MAX_DAMAGE_OBJECTS;i++)
	{
		if (strcmp((char*)ComponentName,(char*)VehicleType->DamageObjects[i]) == 0)
		{
			*ComponentNum = i+1;
            //printf("Component %s has ComponentNum = %d\n",ComponentName,i+1);

			break;
		}
	}

	return 0;
}

UINT32 _UpdateVehicleBoundingBox(mlObjectData* ObjectData,OBJECT * PlaneObject)
{
	UINT32 FirstBBox = 0;

	_UpdateVehicleBoundingBox1(ObjectData,PlaneObject,&FirstBBox);

	return 0;
}

static UINT32 _UpdateVehicleBoundingBox1(mlObjectData* ObjectData,OBJECT * PlaneObject,UINT32* FirstBBox)
{
	static piVECTOR BBox[8];
	static piVECTOR bbox[2];
	OBJECT* ObjSib = PlaneObject;
	OBJECT* ObjAtt;
	UINT32 i;

	do
	{
		if (ObjSib->flags & OBJECT_VISIBLE && !(ObjSib->flags & OBJECT_SKIPDRAWING)
			&& !(ObjSib->flag2 & OBJECT_HIDDEN))
		{
			ObjAtt = ObjSib;

			do
			{
				if(!(ObjSib->flags & OBJECT_NOTOBJECT))
				{
	//				memset(&BBox[0],0,sizeof(piVECTOR) * 8);

					mathAddVectors(&bbox[0],&ObjSib->bbox[0],&ObjSib->pos);
					mathAddVectors(&bbox[1],&ObjSib->bbox[1],&ObjSib->pos);

					// ok, this bounding box is in object space and we need it in world space
					// generate the 8 points of the bounding box
					BBox[0].x = bbox[0].x;
					BBox[0].y = bbox[0].y;
					BBox[0].z = bbox[0].z;
					BBox[1].x = bbox[0].x;
					BBox[1].y = bbox[1].y;
					BBox[1].z = bbox[0].z;
					BBox[2].x = bbox[1].x;
					BBox[2].y = bbox[1].y;
					BBox[2].z = bbox[0].z;
					BBox[3].x = bbox[1].x;
					BBox[3].y = bbox[0].y;
					BBox[3].z = bbox[0].z;
					BBox[4].x = bbox[0].x;
					BBox[4].y = bbox[0].y;
					BBox[4].z = bbox[1].z;
					BBox[5].x = bbox[0].x;
					BBox[5].y = bbox[1].y;
					BBox[5].z = bbox[1].z;
					BBox[6].x = bbox[1].x;
					BBox[6].y = bbox[1].y;
					BBox[6].z = bbox[1].z;
					BBox[7].x = bbox[1].x;
					BBox[7].y = bbox[0].y;
					BBox[7].z = bbox[1].z;

					// now rotate these 8 points
					for (i=0;i<8;i++)
					{
						math3dVectorMultiply(&BBox[i],&BBox[i],&(ObjAtt->matint));
					}

					bbox[0] = BBox[0];
					bbox[1] = BBox[0];

					for (i=1;i<8;i++)
					{
						if (bbox[0].x > BBox[i].x)
						{
							bbox[0].x = BBox[i].x;
						}
						if (bbox[0].y > BBox[i].y)
						{
							bbox[0].y = BBox[i].y;
						}
						if (bbox[0].z > BBox[i].z)
						{
							bbox[0].z = BBox[i].z;
						}

						if (bbox[1].x < BBox[i].x)
						{
							bbox[1].x = BBox[i].x;
						}
						if (bbox[1].y < BBox[i].y)
						{
							bbox[1].y = BBox[i].y;
						}
						if (bbox[1].z < BBox[i].z)
						{
							bbox[1].z = BBox[i].z;
						}
					}
					// check mins
					if (*FirstBBox != 0)
					{
						*FirstBBox = 0;
						ObjectData->BBox1 = bbox[0];
						ObjectData->BBox2 = bbox[1];
					}
					else
					{
						if (ObjectData->BBox1.x > bbox[0].x)
						{
							ObjectData->BBox1.x = bbox[0].x;
						}
						if (ObjectData->BBox1.y > bbox[0].y)
						{
							ObjectData->BBox1.y = bbox[0].y;
						}
						if (ObjectData->BBox1.z > bbox[0].z)
						{
							ObjectData->BBox1.z = bbox[0].z;
						}

						// check maxs
						if (ObjectData->BBox2.x < bbox[1].x)
						{
							ObjectData->BBox2.x = bbox[1].x;
						}
						if (ObjectData->BBox2.y < bbox[1].y)
						{
							ObjectData->BBox2.y = bbox[1].y;
						}
						if (ObjectData->BBox2.z < bbox[1].z)
						{
							ObjectData->BBox2.z = bbox[1].z;
						}
					}
				}
				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
		{
			_UpdateVehicleBoundingBox1(ObjectData,ObjSib->child,FirstBBox);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);


	return 0;
}

UINT32 vehApplyDamageToObject(const FLOAT64& Joules, trnCDTerrainObject* TO, vehVehicle* InShooterVehicle, weapAmmo* Ammo)
{
	if(!TO)
		return 0;

	// check for terrain sto first
	if(TO->IsSTO())
	{
		TerrainSTO* sto = static_cast<TerrainSTO*>(TO->TerrainMasterObject->lpspare);
		const SINT64 joules = (SINT64)Joules;
		if(sto) sto->applyJoules(joules);

		return 0;
	}

	if(TO->StratId == 0)
		return 0;

	vehVehicle* ShooterVehicle = (InShooterVehicle) ? InShooterVehicle : persGetPersonaVehiclePtr();
	if ( ShooterVehicle == nullptr ) return 0;

	SINT32 SuperCellX = TO->StratSupercellX;
	SINT32 SuperCellY = TO->StratSupercellY;
	UINT32 Id = TO->StratId;
	UINT32 Country = TO->Country;
	SINT32 DamThreshold = TO->DamThreshold;

	if ((Joules < DamThreshold) && (Joules>= 0))
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP insufficient energy %.0g J to damage %s, threshold %d J, scx = %d, scy = %d, id = %d",
					Joules, TO->TerrainMasterObject->name, DamThreshold, SuperCellX, SuperCellY, Id);
		return 0;
	}

	UINT32 i;
	LOGICAL Found;
	UINT32 SlotNum = 0;
	SINT32 Side;
	FLOAT64 Energy = Joules;

	// special handling for bridges
	// set side to -1, i.e. an invalid side, for bridges to allow them to be damaged by everyone
	if (TO->ObjId.objClass == CLASS_CP && TO->ObjId.objType == CP_BRIDGE)
	{
		Side = -1;
		if (ShooterVehicle && ShooterVehicle->VehicleType->Type == VEH_TYPE_PERSON && Energy >= 0)
		{
			if (Ammo && Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_PLACED)
			{
				const FLOAT64 energyOverride = fpFPSGetBridgeDamage();
				if (energyOverride >= 1.0f )
				{
					Energy = energyOverride;
					Write_Log(LOG_WEAP, LOG_INFO, "WEAP Trooper bridge damage override from fpsData.txt, damage set to %g", Energy);
				}
			}
		}
	}
	else
		Side = wwiiSide(Country);

	if (TO->ObjId.objClass == CLASS_BUILDING)// && TO->ObjId.objType == BUILDING_GENERIC)
	{
		TERRAIN_DETAIL*	td;
		BUILDING*		bld = nullptr;
		SINT32			j;

		td = trnGetTerainDetail((SINT16)TO->StratSupercellX, (SINT16)TO->StratSupercellY);
		if(td)
		{
			for(j = 0; j < td->numBuildings; j++)
			{
				if(td->pbld[j].oid.id == TO->ObjId.id)// && td->pbld[j].oid.objClass == CLASS_BUILDING && td->pbld[j].oid.objType == BUILDING_GENERIC)
				{
					bld = &td->pbld[j];
					break;
				}
			}

			if(bld /*&& bld->pid.objClass == CLASS_BUILDING*/ && bld->pid.objType == FACILITY_FIREBASE)
			{
				if (ShooterVehicle && ShooterVehicle->VehicleType->Type != VEH_TYPE_PERSON)
				{
					Write_Log(LOG_WEAP, LOG_INFO, "WEAP Non-trooper damage to firebase suppressed!");
					return 0;
				}
				Write_Log(LOG_WEAP, LOG_INFO, "WEAP Trooper damage to firebase allowed!");
			}
		}
	}

	if(TO->ObjId.objClass == CLASS_FACILITY && TO->ObjId.objType == FACILITY_FIREBASE)
	{
		if (ShooterVehicle->Side == Side) 	// can't damage own firebases
			return 0;

		if (ShooterVehicle && ShooterVehicle->VehicleType->Type != VEH_TYPE_PERSON)
		{
			Write_Log(LOG_WEAP, LOG_INFO, "WEAP Non-trooper damage to firebase suppressed!");
			return 0;
		}
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP Trooper damage to firebase allowed!");
	}
	else
	{
		///FRIENDLY-FIRE
#ifdef CANT_DAMAGE_OWN_BUILDINGS
		if (ShooterVehicle->Side == Side) 	// can't damage own buildings
			return 0;
#endif
	}

	Found = FALSE;

	// search for building in list
	for (i = 0;i<ShooterVehicle->DamageQue.Count;i++)
	{
		if (ShooterVehicle->DamageQue.SuperCellX[i] == SuperCellX &&
			ShooterVehicle->DamageQue.SuperCellY[i] == SuperCellY &&
			ShooterVehicle->DamageQue.Id[i] == Id)
		{
			Found = TRUE;
			SlotNum = i;
			break;
		}
	}

	if (ShooterVehicle->DamageQue.Count <= VEH_MAX_BUILDINGS_HIT)
	{
		if (Found == FALSE)
		{
			SlotNum = ShooterVehicle->DamageQue.Count++;
			ShooterVehicle->DamageQue.SuperCellX[SlotNum] = SuperCellX;
			ShooterVehicle->DamageQue.SuperCellY[SlotNum] = SuperCellY;
			ShooterVehicle->DamageQue.Id[SlotNum] = Id;
		}

		ShooterVehicle->DamageQue.Damage[SlotNum] += Energy;

		Write_Log(LOG_WEAP, LOG_INFO, "WEAP*** ACTUALLY APPLYING DAMAGE*** %.1f J to %s (%d, %d, %d)",
					Energy, TO->TerrainMasterObject->name,
					TO->StratSupercellX, TO->StratSupercellY, TO->StratSupercellX, TO->ObjId.id);
	}

	return 0;
}

UINT32 vehExitVehicleTrigger(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	if (Param != 1 || vehmainHasBailedOut())
		return 0;

	vehVehicle* player = persGetPersonaVehiclePtr();

	/*
	if(player && !player->ModelData->DeclaredKilled)
	{
		// if we hit exit again, kill the popup
		phui::View* exitView = uiViewport()->FindView("exitvehicle");

		if(exitView != nullptr)
		{
			uiRunScript(exitView, "doDespawn(false, false);");
			return 0;
		}
	}
	*/

	// handle bailing cases; only for planes right now
	if(player->ModelData && player->VehicleType->Type == VEH_TYPE_AIR)
	{
		// are we already bailing?
		if(_bailoutTime)
		{
			// if less than 30 seconds old, just return
			if(_bailout && _bailout->age() <= 30000)
				return 0;

			// otherwise fall down below and let them despawn
		}
		else
		{
			FLOAT64VECTOR& vpos		= player->ModelData->VehiclePos;
			Vec3d velocity			= Vec3d(player->ModelData->VehicleVel);
			FLOAT32 altitude		= collideRay(COLLIDER_TILE_MASK, 10000.0f, vpos.x, vpos.y, vpos.z + 3.0f);
			FLOAT32 speed			= (FLOAT32)velocity.length();
			altitude				= (altitude) ? fabs(altitude - 3.0f) : 0.0f;

			bool doBail = true;

			// now handle cases where we will just allow them to despawn
			if(persGetCrewIsDead(0) ||						// pilot is dead
			   player->ModelData->MultiCrewPos > 1 ||		// you are the crew in a multicrew vehicle
			   player->isHulked() ||						// your vehicle is destroyed
			   speed <= 3.0f ||								// moving slowly
			   (altitude && altitude <= 6.0f))				// you are on the ground
			{
				doBail = false;
			}

			if(doBail)
			{
				if(altitude && altitude < BailoutSTO::SAFE_DISTANCE())
				{
					uiGameEvent(GAMEEVENT_NOBAILOUT, 1, uiLocString("hud.alerts.30.minalt").c_str());
					return 0;
				}

				// ok, we can bail!
				phui::View* bailout = uiFindView("bailout");
				if(!bailout) uiLoadView("bailout");

				return 0;
			}
		}
	}

	/* new mobile spawn builder
	if(!player->ModelData->DeclaredKilled && vehmainIsMobileSpawn())
	{
		// don't let them despawn until the timer is up
		return 0;
	}
	*/

/*#ifndef ALWAYS_CAN_EXIT
	if (Online == 0 ||
	(persCanVehicleExit() || _CanPlayerExit()))
#endif*/
	{
		/*
        if(persIsInDeathCam())
        {
            return 0;
        }
		*/

		uiImapCursorCallback(0,0,0);

		bool exitReady = true;

		/*
		if(player->VehicleType->Type == VEH_TYPE_GROUND || player->VehicleType->Type == VEH_TYPE_AIR)
		{
			player->ModelData->HulkOnDespawn = TRUE;
			player->ModelData->DeclaredKilled = TRUE;
		}
		*/

		if((player->VehicleType->Type == VEH_TYPE_GROUND || player->VehicleType->Type == VEH_TYPE_AIR) &&
			player->ModelData->DeclaredKilled &&
		    player->ModelData->HulkOnDespawn &&
			!player->isHulked())
		{
			bool hulkVehicle = true;

			// but not if you have rtb'ed as a plane
			if(player->VehicleType->Type == VEH_TYPE_AIR)
			{
				SINT32 ExitStatus = 0;
				FULL_OBJ_ID rtbFac;

				// calculate rtb code
				vehDetermineExitStatus(&ExitStatus, &rtbFac);

				if(ExitStatus == PEXIT_RTB || vehmainHasBailedOut())
					hulkVehicle = false;
			}

			if(hulkVehicle)
			{
				vehVisDamageHulkVehicle(player, TRUE);
				persSetExitTime(frametime + 10000); // 10 seconds from now
				exitReady = false;
			}
		}

        if(exitReady)
			persSetExitTime(frametime); // exit now

//		mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	}

	fpPPOKill();

	return 0;
}

LOGICAL vehmainWaitingOnLogoutTimer()
{
	return (uiViewport()->FindView("exitvehicle") != nullptr);
}

void vehmainBailout(BailoutSTO* pilot)
{
	vehVehicle* Vehicle = persGetPersonaVehiclePtr();

	if(!pilot)
	{
		FLOAT64VECTOR& pos = Vehicle->ModelData->VehiclePos;
		FLOAT64ATT& att = Vehicle->ModelData->VehicleAtt;

		// no bailing out if pilot is dead
		if(persGetCrewIsDead(0))
		{
			uiGameEvent(GAMEEVENT_NOBAILOUT, 1, uiLocString("hud.alerts.30.deadpilot").c_str());
			return;
		}

		const vehType* vt = BailoutSTO::VTYPE(Vehicle);

		if(!vt)
			return;

		// no longer than 1 minute
		_bailoutLife = 60000;

		// move this index to the next bailout position which will be handled by _AirplaneBailout later
		// we're position 0 and we're bailing out now
		_bailoutPos = 1;

		FLOAT32 alt = collideRay(COLLIDER_TILE, 10000.0f, pos.x, pos.y, pos.z + 1.0f);

		if(alt && alt > BailoutSTO::MIN_ALT())
		{
			// calculate how long before this thing expires; based on alt and fall rate
			_bailoutLife = ((alt - BailoutSTO::MIN_ALT()) / BailoutSTO::FALL_RATE()) * 1000;
		}

		if(Online)
		{
			// send out our pilot sto
			WorldPosition_m position(pos.x, pos.y, pos.z + GetRandom(-2.0f, -.5f));
			AttitudeVector attitude(0, 0, (FLOAT32)att.Yaw);

			BailoutSTO sto(0, vt, position, attitude);

			sto.setLifeSpan((unsigned int)_bailoutLife);
			sto.setOwner(Vehicle->Chat_Handle);
			sto.request(PlayerClient()->ChatConnection());
		}
	}
	else
	{
		_bailout = pilot;
	}

	// set external camera adjuster
	persSetExternalCameraAdjuster(_AirplaneBailout);

	// init our bailout callback
	_AirplaneBailout(nullptr, nullptr);

	// disable controls so we can fly the plane remotely
	vehcntrlBailout();

	// make sure we stop firing after bailing
	Vehicle->ModelData->PrimaryTriggerDown		= 0;
	Vehicle->ModelData->PicklePrimary			= 0;
	Vehicle->ModelData->SecondaryTriggerDown	= 0;
	Vehicle->ModelData->PickleSecondary			= 0;

	_bailoutTime = frametime;
}

LOGICAL vehmainHasBailedOut()
{
	return (_bailoutTime > 0) ? TRUE: FALSE;
}

LOGICAL _CanPlayerExit()
{
	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	if (Vehicle == nullptr || Vehicle->ModelData == nullptr) return FALSE;

	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON 	||
		Vehicle->VehicleType->Type == VEH_TYPE_FLOATER 	||
		Vehicle->RideVehicleHostIndex > 0)
	{
		return TRUE;
	}

	FLOAT64VECTOR WorldOriginOffset;
	trnGetWorldOriginOffset(&WorldOriginOffset);

	piVECTOR V1, V2;
	SINT32 DummyNum;

	V1.x = (FLOAT32)(Vehicle->ModelData->RenderPos.x - WorldOriginOffset.x);
	V1.y = (FLOAT32)(Vehicle->ModelData->RenderPos.y - WorldOriginOffset.y);
	V1.z = -(FLOAT32)(Vehicle->ModelData->RenderPos.z);

	if (trnopCollideDown(&V2,&DummyNum,&V1))
	{
		FLOAT32 Delta = (FLOAT32)(Vehicle->ModelData->RenderPos.z - (-V2.z));
		if (Delta < -5.0f)
		{
			// we're underground, let them bail
			return TRUE;
		}

		V2 = V1;
		V2.z += 10000;
		V1.z -= 10000;
		if (!trncdCollideDownMediumList(&V1,&V2))
		{
			return TRUE;
		}

		Delta = (FLOAT32)(Vehicle->ModelData->RenderPos.z - (-V2.z));

		if (Delta > 5.0f)
		{
			return FALSE;
		}
		if (mathExactVectorLength(&Vehicle->ModelData->VehicleVel) < 3.0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}

}

UINT32 vehExitVehicle(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if(Param != 1)
		return 0;

#ifdef GR2_BLEND_EDITOR
	if(!Online && uiViewport()->FindView("blendedit"))
		uiDotCommand("/blendeditor"); // toggle it off
#endif

	vehVehicle* Vehicle = persGetPersonaVehiclePtr();

	if (Vehicle == nullptr)
		return 0;

	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	SINT32 ExitStatus = 0;
	FULL_OBJ_ID rtbFacilityID;

	// clear our mobile spawn flag
	/* new mobile spawn functionality
	if(vehmainIsDeployed() && VehicleData->MultiCrewPos < 2)
    {
	    _setVehicleDeployedState(IDE_VEHICLE_UNDEPLOYED);
	    _resetBombBayDoors();
    }
	*/

	if(vehmainIsDeployed() && VehicleData->MultiCrewPos < 2)
	{
		// Request a server tracked object (STO) from the host
		WorldPosition_m position(Vehicle->PosData.Pos);

		vehVehiclePositionData* p = &Vehicle->PosData;
		AttitudeVector attitude((FLOAT32)p->Att.Roll, (FLOAT32)p->Att.Pitch, (FLOAT32)p->Att.Yaw);

		VehicleSTO vehicle(0, Vehicle->VehicleType, position, attitude);

		_setVehicleDeployedState(IDE_VEHICLE_UNDEPLOYED);
		_resetBombBayDoors();
		vehicle.request(PlayerClient()->ChatConnection());
	}
	else if(persIsPersonaVehicleType(VEH_TYPE_GROUND) && Vehicle->isHulked() && vehmainCanSTO())
	{
		if (Vehicle->VehicleDeadTime == 0) //vehExitVehicle was not called before, so request an STO was not send also
		{
		// Can hulk if ground, vehicle is actually hulked and meet same requirements
		// as STO for vehicles.

		// Request a server tracked object (STO) from the host
		WorldPosition_m position(Vehicle->PosData.Pos);

		vehVehiclePositionData* p = &Vehicle->PosData;
		AttitudeVector attitude((FLOAT32)p->Att.Roll, (FLOAT32)p->Att.Pitch, (FLOAT32)p->Att.Yaw);

		HulkSTO hulk(0, Vehicle->VehicleType, position, attitude);
		hulk.request(PlayerClient()->ChatConnection());
		}
	}

	// make sure this is off
	//PlayerClient()->EnableDespawnTimer(false);

	vehcntrlDisableAllControls();

	vehDetermineExitStatus(&ExitStatus, &rtbFacilityID);

	// send out an update to make sure they ragdoll our guy
	if(ExitStatus == PEXIT_DEAD && Vehicle->VehicleType->Type == VEH_TYPE_PERSON && !fpFPSIsDead())
	{
		fpFPSKillTrooper();
		vehupdateUpdate(TRUE);
	}

	Vehicle->VehicleDeadTime = frametime;

	// check for any critical damage so we can award a kill
	if(ExitStatus != PEXIT_RTB && Vehicle->VehicleType->Type == VEH_TYPE_AIR)
	{
		//uiChatPrintLine("Looking for plink kills!");
		bool update = false;

		if(!Vehicle->ModelData->DeclaredKilled || Vehicle->ModelData->DeclaredKilled == Vehicle->VehicleHostIndex)
		{
			UINT32 plinkKiller = vehmainDeterminePlinkKiller();

			if(plinkKiller)
				Vehicle->ModelData->DeclaredKilled = plinkKiller;

			// if(Vehicle->ModelData->DeclaredKilled)
			if(Vehicle->ModelData->DeclaredKilled && Vehicle->ModelData->DeclaredKilled != Vehicle->VehicleHostIndex)
			{
				Vehicle->ModelData->VehicleIsDead = TRUE;
				update = true;
			}
		}

		if(Vehicle->ModelData->DeclaredKilled)
		{
			//uiChatPrintLine("Declaring myself killed!");
			commSendKillNotice(PlayerClient()->CellConnection(),
							   Vehicle->VehicleHostIndex,
							   Vehicle->ModelData->DeclaredKilled);
		}

		if(update)
		{
			//uiChatPrintLine("Forcing !");
			vehupdateUpdate(TRUE);
		}
	}
	else if(Vehicle->ModelData->DeclaredKilled)
	{
		//uiChatPrintLine("Declaring myself killed!");
		commSendKillNotice(PlayerClient()->CellConnection(),
			   Vehicle->VehicleHostIndex,
			   Vehicle->ModelData->DeclaredKilled);

		// make sure pr_killed gets sent up
		vehupdateUpdate(TRUE);
	}

	PlayerClient()->SendPlayerExit(ExitStatus, rtbFacilityID);

	//Vehicle->VehicleDeadTime = frametime;

	imapRemoveControlCallbacks();

	// reset current ppo if we have one
	fpPPOKill();

	return 0;
}

inline bool _isdespawning()
{
    vehVehicle*vehicle = persGetPersonaVehiclePtr();

    if(vehicle && vehicle->VehicleDeadTime)
    {
        return true;
    }

    return false;
}

bool HulkSTO::activate()
{
    if(_isdespawning())
    {
        return false;
    }

    if(!VehicleSTO::activate())
    {
        return false;
    }

    vehVehicle*veh = vehFindVehicle((UINT32)m_uniqueSTOvehicleID);
    if(veh)
    {
        // Vehicle loading is threaded, so can't hulk until vehicle is loaded.
        veh->hulksto = true;
    }

	return true;
}

void HulkSTO::deactivate()
{
//	bool found = false;
	HULK_SET::iterator itr = _hulkqueue.find(this);

    if(itr != _hulkqueue.end())
    {
        _hulkqueue.erase(itr);
	    ServerTrackedObject::deactivate();
        return;
    }
    else
    {
        VehicleSTO::deactivate();
    }
}

bool _initSTOHulkIfNecessary()
{
    if(_hulkqueue.empty())
    {
        return false;
    }

    VehicleSTO* sto = *_hulkqueue.begin();
    _hulkqueue.erase(_hulkqueue.begin());

    sto->load();

    vehVehicle*veh = vehFindVehicle((UINT32)sto->getSTOVehicleID());

    if(veh)
    {
        // Vehicle loading is threaded, so can't hulk until vehicle is loaded.
        veh->hulksto = true;
    }

    return true;
}

bool VehicleSTO::activate()
{
	if(activated()) return false;

    m_uniqueSTOvehicleID = _uniqueSTOid;
	_uniqueSTOid--;

    if(type() == _HULK)
    {
        _hulkqueue.insert(this);
	    return ServerTrackedObject::activate();
    }

    load();

	return ServerTrackedObject::activate();
}

UINT32 VehicleSTO::getVehicleTypeNum()
{
	WEAPON_SPEC spec;
	spec.fromShort(subType());

	const vehType* vtype = vehTypeGetBySpec(spec);

	return (vtype) ? vtype->VehicleTypeNum: 0;
}

void VehicleSTO::load()
{
    vehVehicle*veh = nullptr;

    vehupdateCreateVehicle(&veh, getVehicleTypeNum(), -(SINT32)uid(), m_uniqueSTOvehicleID);

    if(!veh)
    {
        return;
    }

    veh->Side = wwiiSide(veh->VehicleType->WeaponSpec.wCountry);
    veh->Country = veh->VehicleType->WeaponSpec.wCountry;
	veh->persDecal = persDecal;
	veh->unitDecal = unitDecal;

    veh->PosData.Pos.x = position().x();
    veh->PosData.Pos.y = position().y();
    veh->PosData.Pos.z = position().z();

    veh->PosData.Att.Roll = attitude().x();
    veh->PosData.Att.Pitch = attitude().y();
    veh->PosData.Att.Yaw = attitude().z();

    veh->PosData.RenderPos = veh->PosData.Pos;
    veh->PosData.RenderAtt = veh->PosData.Att;

    if(type() == _VEHICLE)
    {
		snprintf((char*)veh->Handle, sizeof(veh->Handle)-1, "%s",icon.c_str());
	    veh->Chat_Handle = chatGetHandle((char*)veh->Handle);
    }

#ifdef BETA_RELEASE
    const UCHAR* name = vehtypeGetByIndex(getVehicleTypeNum())->Shape;

	char buf[150];
    strprintf(buf, "[activate] %s id:%d, veh:%s", type() == _HULK ? "HULK" : "STO", (int)uid(), name);
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_BETA, LSTR_GENERIC, uiUTF8toUTF16(buf));
#endif
}

void VehicleSTO::deactivate()
{
	if(!activated()) return;

#ifdef BETA_RELEASE
    const UCHAR* name = vehtypeGetByIndex(getVehicleTypeNum())->Shape;

	char buf[150];
    strprintf(buf, "[deactivate] %s id:%d, veh:%s", type() == _HULK ? "HULK" : "STO", (int)uid(), name);
	uiChatPrintLineMessageLoc(IDE_LANG_HOST, LSTR_BETA, LSTR_GENERIC, uiUTF8toUTF16(buf));
#endif

    vehVehicle*veh = nullptr;

    veh = vehFindVehicle((UINT32)m_uniqueSTOvehicleID);

    // If this vehicle is hulked, the hulksto should be false.
    // That flag is set to apply the hulk to the vehicle once it's loaded..
    // because it's threaded. Sometimes we get an immediate deactivate
    // before the vehicle has been fully loaded...this flag is still true
    // then. We also don't want to do this if we are despawning.
    if(type() == _HULK && !_isdespawning() && veh && !veh->hulksto)
    {
		veh->VisDamageNoStick = TRUE;
		// Sink vehicle 4 meters in 30 seconds
        vehfxSinkVehicle(veh->iFXVehicleID, 30000, 4.0f / 30000.0f);
    }
    else if(veh)
    {
		if(persIsInDeathCam() && !SCEN_MGR->IsActive())
        {
            // Do nothing. Leave vehicles to display while in deathcam. Vehicles removed with ClearAllVehicles
        }
        else
        {
            vehKillVehicle(veh);
        }
    }
    else
    {
        Write_Log(LOG_OLD, LOG_INFO, "[VehicleSTO::deactivate] Unable to free vehicle");
    }

	ServerTrackedObject::deactivate();
}

vehVehicle* VehicleSTO::getVehicle()
{
	return vehFindVehicle((UINT32)m_uniqueSTOvehicleID);
}

void MobileSpawnSTO::preProcess()
{
	PlayerPlacedSTO::preProcess();
}

void MobileSpawnSTO::postProcess()
{
	PlayerPlacedSTO::postProcess();
}

void PlayerPlacedSTO::preProcess()
{
    TerrainSTO::preProcess();

    if (type() == ServerTrackedObject::_UMS || subType() == PPOSpecLoader::_PPO_AFRU)
    {
        if(_to)
            trnFXSetCountryFlags(_to, (SINT32)flags());
    }
}

void PlayerPlacedSTO::postProcess()
{
    TerrainSTO::postProcess();

    if (type() == ServerTrackedObject::_UMS || subType() == PPOSpecLoader::_PPO_AFRU)
    {
        if(_to)
        {
            trnCDTerrainObject* TO = trncdFindTerrainObject(_to->child);

            // flags is used to pass the country for mobilespawn stos
            if(TO)
                TO->Country = (SINT32)flags();
        }
    }
}





BailoutSTO::~BailoutSTO()
{
	if(this == _bailout)
		_bailout = nullptr;
}

bool BailoutSTO::activate()
{
    if(!VehicleSTO::activate())
        return false;

    vehVehicle* veh = getVehicle();
    if(!veh) return false;

	gr2AnimBitPacker bp(veh->AnimationBits);
	bp.chuteOpen = 1;
	veh->AnimationBits = bp.pack();
	veh->Flags |= VEH_FLAGS_USE_OLD_INTERPOLATOR;

	vehUpdate();

	//veh->Flags |= VEH_FLAGS_IS_CLONE;

	//const FLOAT32 fallRate = 5.0f; // meters per second
	//const FLOAT32 minALT = 30.0f; // min allowed height above ground before expiring

	// add some negative velocity
	veh->PosData.Vel.z = -FALL_RATE();

	// see where the ground is
	FLOAT32 hit = collideRay(COLLIDER_TILE, 10000.0f, veh->PosData.Pos.x, veh->PosData.Pos.y, veh->PosData.Pos.z);

	if(!hit)
	{
		// nothing hit, expire us
		setExpired();
	}
	else
	{
		// otherwise calculate how far down to move this bailout based on age
		double fell = ((double)age() / (double)1000.0f) * FALL_RATE();

		if(hit - fell <= MIN_ALT())
		{
			setExpired();
		}
		else
		{
			// calculate how long before this thing expires; based on age and fall rate
			m_lifespan = (unsigned int)(frametime + (((hit - fell - MIN_ALT()) / FALL_RATE()) * 1000));

			// move us down to where our age would put us
			veh->PosData.Pos.z -= fell;

			// we're above the min alt, draw us
			return true;
		}
	}

	return false;
}

bool BailoutSTO::isMyPilot() const
{
	ChatHandle& commander = PlayerClient()->MultiCrewCommander();
	return (commander && commander == owner());
}

const vehType* BailoutSTO::VTYPE(vehVehicle* v)
{
	const vehType* vt = nullptr;

	if(v)
	{
		const char* vid = nullptr;

		switch(v->VehicleType->Type)
		{
			case VEH_TYPE_AIR:
			{
				switch(v->VehicleType->WeaponSpec.country())
				{
					case WWII_ENGLAND:
						vid = "uk-para-rfl";
						break;
					case WWII_USA:
						//vid = "us-para-rfl";
						vid = "uk-para-rfl";
						break;
					case WWII_FRANCE:
						vid = "fr-para-rfl";
						break;
					case WWII_GERMANY:
						vid = "de-para-rfl";
						break;
                    default:
                        // no other valid countries
                        break;
				}
			}
			break;
		}

		if(vid)
			vt = vehTypeGetById(vid);
	}

	return vt;
}

static UINT32 _ClearAllVehicles()
{
	bool scenario = SCEN_MGR->IsActive();

	if (getVehicleHead().getNext() != nullptr)
	{
		vehVehicle* CurrentVehicle;
		vehVehicle* NextVehicle;

		for (CurrentVehicle = getVehicleHead().getNext();
			 CurrentVehicle != nullptr;)
		{
			NextVehicle = CurrentVehicle->getNext();

			// remove by default
			bool remove = true;

			// otherwise normal processing
			if(!Online && CurrentVehicle->isSTO())
				remove = false;
			else if(scenario && CurrentVehicle->isSTO() && (SINT32)CurrentVehicle->VehicleHostIndex < 0) // these are removed by the scenario manager
				remove = false;

			if(remove)
				vehKillVehicle(CurrentVehicle);

			CurrentVehicle = NextVehicle;
		}
	}

	return 0;
}

static void _LoadInfantryVehicle(vehVehicle* Vehicle)
{
	if(!Vehicle || !Vehicle->LoadVehicle) return;

	vehupdateCreateVehicleLoadObjectPortion(Vehicle,0);
	fpProcessVehicleAnimationFlags(Vehicle);

	Vehicle->LoadVehicle = 0;
}

#define _ARMYBASE_DIST 500.0f
#define _AIRBASE_DIST 1000.0f
#define _NAVALBASE_DIST 5000.0f
#define	_DEPOT_PERIMITER	50.0f		// Have to be > 50m from an ab to RTB at a depot

// Macro function for determining whether ot not a facility is of a spawnable type
static inline LOGICAL _isSpawnableFacilityType(SINT8 objType)
{
	switch (objType)
	{
		case FACILITY_ARMYBASE:
		case FACILITY_NAVALBASE:
		case FACILITY_AIRBASE:
		case FACILITY_FIREBASE:
		case FACILITY_DEPOT:
			return TRUE;
		break;

		default:
		break;
	}

	return FALSE;
}

UINT32 vehDetermineExitStatus(SINT32* ExitStatus, FULL_OBJ_ID* id)
{
	*ExitStatus = (_bailoutTime) ? PEXIT_OVER_ENEMY_TERRITORY : PEXIT_DEAD;
	*id = nullObjID;

	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	if (Vehicle == nullptr || Vehicle->ModelData == nullptr)
		return 0;

	if (persIsCrewDead())
		return 0;

	trnCDTerrainObject* TO, *ClosestTO, *ResupplyTO;
	StratDB::StratObject* ClosestStrat;
	FLOAT64VECTOR Pos, TOPos, Dist, WorldOriginOffset;
	FLOAT64 Len, BaseLen, HomeBaseLen;
	UINT32 FriendlyTerritory, NearBase, OnGroundStopped;
	piVECTOR V1,V2;

	trnGetWorldOriginOffset(&WorldOriginOffset);
	Pos = Vehicle->ModelData->RenderPos;
	trncdGetTOHead(&TO);
	ClosestTO = nullptr;
	ClosestStrat = nullptr;
	ResupplyTO = nullptr;
	BaseLen = HomeBaseLen = 1000000000000000.0;

	NearBase = 0;
	FriendlyTerritory = 0;
	for ( ; TO != nullptr ; TO = TO->Next)
	{
		if (TO->StratId == 0 && TO->StratSupercellY == 0 && TO->StratSupercellX == 0) // If no strat id, it's not a strat object, thus ignore.
			continue;

		if( TO->ObjId.objClass == CLASS_FACILITY && TO->DamageLevel != FB_CLOSED_VISLEVEL
			&& _isSpawnableFacilityType(TO->ObjId.objType))
		{
			StratDB::StratObject* strat = TO->StratObj();
			if(!strat)
				continue;

			V1 = TO->Center;

			TOPos.x = V1.x + WorldOriginOffset.x;
			TOPos.y = V1.y + WorldOriginOffset.y;
			TOPos.z = -(V1.z + WorldOriginOffset.z);

			mathSubVectors(&Dist,&TOPos,&Pos);

			Len = sqrt(Dist.x * Dist.x + Dist.y * Dist.y); // ignore z
			if (Len < BaseLen)
			{
				ClosestTO = TO;
				ClosestStrat = strat;
				BaseLen = Len;
			}

			if (Vehicle->Side == wwiiSide((UINT32)strat->Country()))
			{
				switch(TO->ObjId.objType )
				{
					case FACILITY_AIRBASE:
						if (Len < _AIRBASE_DIST)
						{
							FriendlyTerritory = 1;

							if (Vehicle->VehicleType->Type == VEH_TYPE_AIR || Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
								NearBase = 1;

							if (Len < HomeBaseLen && // is it closer?
							   (Vehicle->VehicleType->Type == VEH_TYPE_AIR || // is it an aircraft?
							   (Vehicle->VehicleType->WeaponSpec.wCategory == 4 && Vehicle->VehicleType->WeaponSpec.wClass == 14))) // is it a para?
							{
								ResupplyTO = TO;
								HomeBaseLen = Len;
							}
						}
					break;

					case FACILITY_NAVALBASE:
						if (Len < _NAVALBASE_DIST)
						{
							FriendlyTerritory = 1;

							if (Vehicle->VehicleType->Type == VEH_TYPE_SEA || Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
								NearBase = 1;

							if (Vehicle->VehicleType->Type == VEH_TYPE_SEA && Len < HomeBaseLen)
							{
								ResupplyTO = TO;
								HomeBaseLen = Len;
							}
						}
					break;

					case FACILITY_ARMYBASE:
					case FACILITY_FIREBASE:
						if (Len < _ARMYBASE_DIST)
						{
							FriendlyTerritory = 1;

							if (Vehicle->VehicleType->Type == VEH_TYPE_GROUND || Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
							{
								NearBase = 1;
								if (Len < HomeBaseLen)
								{
									ResupplyTO = TO;
									HomeBaseLen = Len;
								}
							}
						}
					break;
					case FACILITY_DEPOT:
					{
						// infantry depot check is done below
						if (Len < _ARMYBASE_DIST && Vehicle->VehicleType->WeaponSpec.wCategory == 2)
						{
							UINT8 wClass = Vehicle->VehicleType->WeaponSpec.wClass;

							// truck or gun
							if(wClass == 6 || wClass == 7)
							{
								FriendlyTerritory = 1;
								NearBase = 1;

								if (Len < HomeBaseLen)
								{
									ResupplyTO = TO;
									HomeBaseLen = Len;
								}
							}
						}
					}
					break;
				}
			}
		}
	}

	if(!FriendlyTerritory)
	{
		if(ClosestStrat && (Vehicle->Side == wwiiSide((UINT32)ClosestStrat->Country())) )
			FriendlyTerritory = 1;
	}

	OnGroundStopped = 0;
	if (Vehicle->VehicleType->Type == VEH_TYPE_AIR)
	{ // Need to check if landed.
		SINT32 poly;

		V1.x = 	(FLOAT32)(Vehicle->ModelData->RenderPos.x - WorldOriginOffset.x);
		V1.y =  (FLOAT32)(Vehicle->ModelData->RenderPos.y - WorldOriginOffset.y);
		V1.z = -(FLOAT32)(Vehicle->ModelData->RenderPos.z);

		if (trnopCollideDown(&V2,&poly,&V1))
		{
			FLOAT32 height = (FLOAT32)(Vehicle->ModelData->RenderPos.z - (-V2.z));
			if(height < 25.0f && mathExactVectorLength(&Vehicle->ModelData->VehicleVel) < 25.0)
			{ // A generous definition of on the ground and stopped. :)
				OnGroundStopped = 1;
			}
		}else
		{
			OnGroundStopped = 1;
		}
	}else
	{
		OnGroundStopped = 1;

		if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON && Vehicle->RideVehicleHostIndex > 0)
		{
			vehVehicle* rideVehicle = vehFindVehicle(Vehicle->RideVehicleHostIndex);
			if(rideVehicle && rideVehicle->VehicleType->Type == VEH_TYPE_AIR)
			{
				OnGroundStopped = 0;
			}
		}
	}

	if (FriendlyTerritory)
	{
		// Are we closer to a Depot than we are a spawn facility?
		if (ClosestTO && ClosestStrat
			&& ClosestTO->ObjId.objType == FACILITY_DEPOT
			&& Vehicle->Side == wwiiSide((UINT32)ClosestStrat->Country())
			&& Vehicle->VehicleType->Type == VEH_TYPE_PERSON
			&& HomeBaseLen > _DEPOT_PERIMITER	// Army base is out of depot range
			&& BaseLen <= _ARMYBASE_DIST		// Not more than an armybase away
			&& BaseLen < HomeBaseLen			// And closer than the other facility
			)
		{
			NearBase = 1;						// We are, in fact, near a base
			ResupplyTO = ClosestTO;				// despawn-facility for host to trace supply for
			HomeBaseLen = BaseLen;				// Tidiness
		}

		if (NearBase && OnGroundStopped && !_bailoutTime)
			*ExitStatus = PEXIT_RTB;
		else
			*ExitStatus = PEXIT_OVER_FRIENDLY_TERRITORY;
	}
	else
	{
		*ExitStatus = PEXIT_OVER_ENEMY_TERRITORY;
	}

	// check to see if we're a troop and near a FRU
	if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON &&
	   (*ExitStatus == PEXIT_OVER_ENEMY_TERRITORY || *ExitStatus == PEXIT_OVER_FRIENDLY_TERRITORY))
	{
		if(wwiiForEachSTO(_findRTBFRU))
			*ExitStatus = PEXIT_RTB;
	}

	if(ResupplyTO && *ExitStatus == PEXIT_RTB)
	{
		id->cell.ox	= ResupplyTO->StratSupercellX << 4;
		id->cell.oy	= ResupplyTO->StratSupercellY << 4;
		id->id 		= ResupplyTO->ObjId.id;
		id->objClass= ResupplyTO->ObjId.objClass;
		id->objType	= ResupplyTO->ObjId.objType;
	}

	return 0;
}

UINT32 vehAdjustAnimationTime()
{
	if (getVehicleHead().getNext() != nullptr)
	{
		vehVehicle* CurrentVehicle;

		for (CurrentVehicle = getVehicleHead().getNext();
			 CurrentVehicle != nullptr;
			 CurrentVehicle = CurrentVehicle->getNext())
		{
			CurrentVehicle->StateAnimation.LastUpdateTime = frametime;

		}
	}

	return 0;
}

// does 4 things:
// - finds nearest enemy capture building
// - finds nearest enemy facility marker
// - finds nearest friendly facility marker
// - sends out an EWS alert under the right circumstances
static void _FindNearbyFacilities(vehVehicle* Vehicle)
{
	if(!Vehicle || !Vehicle->ModelData) return;

	static SINT32 lasttime = 0;
	if(abs(frametime-lasttime) < 5000 ) return;
	lasttime = frametime;

	memset(&Vehicle->ModelData->closestCp, 0, sizeof(FULL_OBJ_ID));
	memset(&Vehicle->ModelData->closestCapTO, 0, sizeof(FULL_OBJ_ID));
	memset(&Vehicle->ModelData->closestFriendlyFAC, 0, sizeof(FULL_OBJ_ID));
	memset(&Vehicle->ModelData->closestEnemyFAC, 0, sizeof(FULL_OBJ_ID));

	Vec3d pos = Vec3d(Vehicle->ModelData->VehiclePos.x, Vehicle->ModelData->VehiclePos.y, 0);

	const StratDB::StratObject* strat		= nullptr;
	const StratDB::StratObject* cp			= nullptr;
	const StratDB::StratObject* capBld		= nullptr;
	const StratDB::StratObject* friendlyFac	= nullptr;
	const StratDB::StratObject* enemyFac	= nullptr;

	Vec3d stratPos;
	Vec3d diff;
	Vec3d capBldPos;
	/*
	Vec3d cpPos;
	Vec3d friendlyFacPos;
	Vec3d enemyFacPos;
	*/

	//double diffDist = 0;
	double diffDist2 = 0;
	double cpDist2 = 0;
	double capBldDist2 = 0;
	double friendlyFacDist2 = 0;
	double enemyFacDist2 = 0;

	trnCDTerrainObject* capTO = nullptr;
	trnCDTerrainObject* TO = nullptr;
	trncdGetTOHead(&TO);

	for(; TO; TO = TO->Next)
	{
		if(TO->StratId == 0 && TO->StratSupercellY == 0 && TO->StratSupercellX == 0)
			continue;

		strat = TO->StratObj();
		if(!strat) continue;

		// our strat position
		stratPos.set((FLOAT64)strat->PosX(), (FLOAT64)strat->PosY(), 0);

		// range from us
		diff = pos - stratPos;
		diffDist2 = diff.length2();
		//diffDist = diff.length();

		switch(strat->StratClass())
		{
			case SDM_CLASS_CP:
			{
				// find closest cp
				if(strat->StratType() != SDM_CP_BRIDGE && (!cp || diffDist2 < cpDist2))
				{
					cp = strat;
					//cpPos = stratPos;
					cpDist2 = diffDist2;
				}
			}
			break;
			case SDM_CLASS_FACILITY:
			{
				if(strat->StratType() == SDM_FACILITY_FIREBASE && !(strat->StratFlags() & CP_STAT_OPEN))
					continue;

				// find closest friendly and enemy facilities
				if(strat->Side() == Vehicle->Side)
				{
					if(!friendlyFac || diffDist2 < friendlyFacDist2)
					{
						friendlyFac = strat;
						//friendlyFacPos = stratPos;
						friendlyFacDist2 = diffDist2;
					}
				}
				else
				{
					if(!enemyFac || diffDist2 < enemyFacDist2)
					{
						enemyFac = strat;
						//enemyFacPos = stratPos;
						enemyFacDist2 = diffDist2;
					}
				}

			}
			break;
			case SDM_CLASS_BUILDING:
			{
				// find closest enemy cap building
				if(strat->Side() == Vehicle->Side)
					continue;

				const StratDB::StratObject* parent = strat->ParentStratObject();

				if(parent && parent->StratClass() == SDM_CLASS_FACILITY && parent->StratType() == SDM_FACILITY_FIREBASE)
					continue;

				if(TO->ObjId.objType == BUILDING_AUTOGUNNER || TO->ObjId.objType == BUILDING_BRIDGE)
					continue;

				//if (strat->StratType() != SDM_BUILDING_CAPTURE)
				//	continue;

				if(!capBld || diffDist2 < capBldDist2)
				{
					capBld = strat;
					capBldPos = stratPos;
					capTO = TO;
					capBldDist2 = diffDist2;
				}
			}
			break;
		}
	}

	if(cp)
	{
		Vehicle->ModelData->closestCp = *(cp->FullObjID());
		//uiChatPrintLine("Closest CP: %s", cp->Name());

#if defined(CLIENT_SEND_LOCALITY_UPDATE)
		//FLOAT64VECTOR& Pos = Vehicle->ModelData->VehiclePos;
		//chatSendLocality(PlayerClient()->ChatConnection(), Vehicle->VehicleHostIndex, Pos.x, Pos.y, Pos.z, Vehicle->ModelData->closestCp);
#endif
	}

	if(friendlyFac)
	{
		Vehicle->ModelData->closestFriendlyFAC = *(friendlyFac->FullObjID());
		//uiChatPrintLine("Closest Friendly Fac: %s", friendlyFac->Name());
	}

	if(enemyFac)
	{
		Vehicle->ModelData->closestEnemyFAC = *(enemyFac->FullObjID());
		//uiChatPrintLine("Closest Enemy Fac: %s", enemyFac->Name());
	}

	if(!capBld)
		return;

	//uiChatPrintLine("Closest Cap Building: %s", capBld->Name());

	Vehicle->ModelData->closestCapTO = *(capBld->FullObjID());

	static UINT32 lastSerial = 0;
	static WEAPON* weapon = nullptr;
	static STRAT_ENEMY_REPORT report;

	bool skipEWScheck = false;

	// Lookup the WEAPON* when the weapon changes, and only then
	// because the lookup is relatively expensive
	if(weapon == nullptr || Vehicle->WeaponSerial.serial != lastSerial)
	{
		weapon = weapLookup(Vehicle->WeaponSerial.spec);
		lastSerial = Vehicle->WeaponSerial.serial;
	}

	// don't do ews check if the weapon doesn't require it
	if(!weapon || !weapon->alertDist)
		skipEWScheck = true;

	// make sure skull and buzz don't send EWS
	if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER)
		skipEWScheck = true;

	if(!skipEWScheck && capTO)
	{
		FLOAT64 ewsDist2 = std::max(EFR_RANGE_2, (double)weapon->alertDist2);

		diff = capBldPos - pos;

		if(diff.length2() > ewsDist2)
			return;

		// If we don't have to update EWS, it's not worth trying to do anything
		if(capTO->EnemyReportTime && ft1000mscount < capTO->EnemyReportTime + 60)
			return;

		capTO->EnemyReportTime = ft1000mscount;

		const StratDB::StratObject* parentFac = capBld->ParentStratObject();
		if(!parentFac) return;

		FULL_OBJ_ID id = *(parentFac->FullObjID());

		SINT32 dir = 0;

		// send report
		dir = (SINT32)(-playrot.z / PI * 180.0f);
		dir = (dir < 0) ? 360 + (dir % 360) : dir % 360;

		memset(&report, 0x0, sizeof(STRAT_ENEMY_REPORT));
		report.reportingFacility = id;
		report.enemyReported = Vehicle->WeaponSerial;
		report.distance = (UINT32)diff.length();
		report.heading = (UINT32)dir;
		report.altitude = (UINT32)((pos.z() > 0) ? pos.z() : 0.0);
		report.speed = (UINT32)mathExactVectorLength(&Vehicle->ModelData->VehicleVel);

        if (Online) {
            strtSendEnemyReport(PlayerClient()->CellConnection(), &report);
        }
	}
}

// simplified: check any enemy building not in an FB facility
/*
static void _FindNearbyFacilities2(vehVehicle* Vehicle)
{
	if(!Vehicle || !Vehicle->ModelData) return;

	static SINT32 lasttime = 0;
	if(!Online || abs(frametime-lasttime) < 5432 ) return;
	lasttime = frametime;

	trnCDTerrainObject* TO, *capTO = nullptr;
	FLOAT64VECTOR Pos, TOPos, Dist, WorldOriginOffset;
	FLOAT64 Len2 = 0, MinLen2 = 1000000000000000.0;
	piVECTOR V1;
	TERRAIN_DETAIL* td;
	SINT32 i;
	BUILDING* capbld, *bld;
	LOGICAL				skipEWScheck = FALSE;
	FLOAT64				closeTOdist2 = 1000000000000000.0;
	trnCDTerrainObject*	closeTO = nullptr;
	FLOAT64				minDist2 = EFR_RANGE_2;
	FACILITY* fac = nullptr;
	FULL_OBJ_ID id;
	SINT32 dir;
	static STRAT_ENEMY_REPORT report;

	static UINT32 lastSerial = 0;
	static WEAPON* weapon;
	if(weapon == nullptr || Vehicle->WeaponSerial.serial != lastSerial)
	{
		// Lookup the WEAPON* when the weapon changes, and only then
		// because the lookup is relatively expensive
		weapon = weapLookup(Vehicle->WeaponSerial.spec);
		lastSerial = Vehicle->WeaponSerial.serial;
	}

	if(!weapon || !weapon->alertDist) skipEWScheck = TRUE;
	if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER) skipEWScheck = TRUE; // make sure skull and buzz don't send EWS

	trnGetWorldOriginOffset(&WorldOriginOffset);
	trncdGetTOHead(&TO);
	Pos = Vehicle->ModelData->RenderPos;

	capbld = nullptr;

	// Buildings need to be within alert distance of EFR range for us to notice them
	if(!skipEWScheck)
		minDist2 = std::max(minDist2, (double)weapon->alertDist2);

	const StratDB::StratObject* capStrat = nullptr;
	const StratDB::StratObject* closeStrat = nullptr;

	for(; TO; TO = TO->Next)
	{
		if(TO->StratId == 0 && TO->StratSupercellY == 0 && TO->StratSupercellX == 0)
			continue;

		if(TO->ObjId.objClass != CLASS_BUILDING)
			continue;

		const StratDB::StratObject* stratObj = TO->StratObj();

		if(!stratObj)
			continue;

		// check for a firebase; skip if found
		const StratDB::StratObject* parentStrat = stratObj->ParentStratObject();

		if(!parentStrat || (parentStrat->StratClass() == CLASS_FACILITY && parentStrat->StratType() == FACILITY_FIREBASE))
			continue;

		V1 = TO->Center;

		TOPos.x = V1.x + WorldOriginOffset.x;
		TOPos.y = V1.y + WorldOriginOffset.y;
		TOPos.z = -(V1.z + WorldOriginOffset.z);

		mathSubVectors(&Dist,&TOPos,&Pos);
		Len2 = (Dist.x * Dist.x + Dist.y * Dist.y); // ignore z

		// if this is a valid TO to check for EWS, do more as needed here
		{
			if(TO->Country < 1 || TO->Country >= MAX_COUNTRIES || // uninitialized check
				Vehicle->Side == wwiiSide(TO->Country)) // only looking for enemy
				continue;

			if(Len2 > minDist2 || Len2 > MinLen2) continue;

			td = trnGetTerainDetail((SINT16)TO->StratSupercellX, (SINT16)TO->StratSupercellY);
			if(!td) continue;

			// find building object
			bld = nullptr;
			for(i = 0; i < td->numBuildings; i++)
			{
				if(td->pbld[i].oid.id == TO->ObjId.id && td->pbld[i].oid.objClass == CLASS_BUILDING && td->pbld[i].oid.objType == TO->ObjId.objType)
				{
					bld = &td->pbld[i];
					break;
				}
			}

			// see if we have the right parent
			// bug in editor? shouldn't parent class be facility?
			if(bld && bld->pid.objClass == CLASS_BUILDING && bld->pid.objType == parentStrat->StratType())
			{
				capbld = bld;
				capTO = TO;
				capStrat = stratObj;
				MinLen2 = Len2;
			}
		}

		// if this is a flag building, check to see if it's the closest
		// old tables are going away, need to update this code
		if (TO->ObjId.objType == BUILDING_CAPTURE)
		{
			if (Len2 > closeTOdist2)
				continue;

			closeTOdist2 = Len2;
			closeTO = TO;
			closeStrat = stratObj;
		}
	}

	if(!capbld || !capTO)
		return;

	populateFullObjID(&Vehicle->ModelData->closestCapTO, capTO->StratSupercellX, capTO->StratSupercellY, capbld->pid);

	if(skipEWScheck || MinLen2 > weapon->alertDist2)
		return;

	// If we don't have to update EWS, it's not worth trying to do anything
	if(capTO->EnemyReportTime && ft1000mscount < capTO->EnemyReportTime + 60)
		return;

	capTO->EnemyReportTime = ft1000mscount;

	// Now we need to locate the FACILITY this TO represents
	td = trnGetTerainDetail((SINT16)capTO->StratSupercellX, (SINT16)capTO->StratSupercellY);
	if(td == nullptr)
		return;

	// find facility
	for(SINT32 i = 0; i < td->numFacilities; i++)
	{
		// bug in editor? shouldn't parent class be facility?
		if(td->pfac[i].oid.id == capbld->pid.id)
		{
			fac = &td->pfac[i];
			break;
		}
	}

	if(fac == nullptr)
		return;

	id.cell.ox = (fac->posx / 800) | (capTO->StratSupercellX << 4);
	id.cell.oy = (fac->posy / 800) | (capTO->StratSupercellY << 4);
	id.id = fac->oid.id;
	id.objClass = fac->oid.objClass;
	id.objType = fac->oid.objType;

	// send report
	dir = (SINT32)(-playrot.z / PI * 180.0f);
	dir = (dir < 0) ? 360 + (dir % 360) : dir % 360;

	memset(&report, 0x0, sizeof(STRAT_ENEMY_REPORT));
	report.reportingFacility = id;
	report.enemyReported = Vehicle->WeaponSerial;
	report.distance = (UINT32)sqrt(Len2);
	report.heading = (UINT32)dir;
	report.altitude = (UINT32)((Pos.z > 0) ? Pos.z : 0.0);
	report.speed = (UINT32)mathExactVectorLength(&Vehicle->ModelData->VehicleVel);

	strtSendEnemyReport(PlayerClient()->CellConnection(), &report);
}
*/

/************************************************************************************************
	vehApplyDamageToVehicleComponent
*************************************************************************************************/

UINT32 vehApplyDamageToVehicleComponent(vehVehicle* Vehicle,
									  UINT32 ComponentNum,
									  FLOAT64 Energy,
									  UINT32 Counter,
									  weapAmmo* Ammo)
{
	if ( Vehicle == nullptr)
	{
		Write_Log(LOG_WEAP,LOG_WARN,"nullptr vehicle!!");
		return 0;
	}

	if (!vehValid(Vehicle) )
	{
		Write_Log(LOG_WEAP,LOG_WARN,"corrupt vehicle!!");
		return 0;
	}

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	///FRIENDLY-FIRE
	if(Online)
	{
		if (PlayerVehicle != nullptr && Vehicle != nullptr && Vehicle != PlayerVehicle)
		{
			if (Vehicle->Side == PlayerVehicle->Side)
			{
				// Allow damage to be applied to friendly STO hulks
				if(Vehicle->isSTO() && Vehicle->isHulked())
				{
					// Do nothing
				}
				else
				{
					// player is not shooting themselves, and
					// the hit target is of the same country
					return 0;
				}
			}
		}
	}

	if (ComponentNum == 0)
	{
		Write_Log(LOG_WEAP,LOG_WARN,"Damaged component zero!!");
		return 0;
	}

	// Cast to UINT32 to ensure it is never negative
	if ((UINT32)ComponentNum > VEH_MAX_DAMAGE_OBJECTS)
	{
		Write_Log(LOG_WEAP,LOG_WARN,"INVALID Component specified in vehApplyDamageToVehicleComponent! %d", ComponentNum);
		return 0;
	}

	Vehicle->VehicleHasBeenHit = 1;

	if(Vehicle->IsMe() && Ammo && Ammo->Flags & WEAP_AMMOFLAG_IS_AUTOGUNROUND)
		PlayerClient()->SetKilledByAI(true);

// Determine importance of damage
	Vehicle->DamageQue.isImportant[ComponentNum] = Vehicle->VehicleType->DamageImportant[ComponentNum-1];

	if(Vehicle->ModelData != nullptr)
		Vehicle->DamageQue.isImportant[ComponentNum] = 1; // damage to ourselves is always important

	if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		Vehicle->DamageQue.isImportant[ComponentNum] = 1; // as is infantry
		_lastInfantryDamaged = Vehicle->Chat_Handle;
	}
	else
	{
		_lastInfantryDamaged = 0;
	}

// If we're below the damage threshold, set energy and isImportant to 0.
	if(Energy < Vehicle->VehicleType->DamageThreshold[ComponentNum-1])
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP insufficient energy %f J to damage %s, threshold %f J",
					Energy, Vehicle->VehicleType->DamageObjects[ComponentNum-1], Vehicle->VehicleType->DamageThreshold[ComponentNum-1]);
		Energy = 0;
		Vehicle->DamageQue.isImportant[ComponentNum]  = 0;
	}

	Vehicle->DamageQue.DamageAccrued[ComponentNum] += (FLOAT32)Energy;

	if (Vehicle->DamageQue.Counter[ComponentNum] != Counter || Counter == 0)
	{
		Vehicle->DamageQue.NumRoundsHit[ComponentNum]++;
		Vehicle->DamageQue.Counter[ComponentNum] = Counter;

		// remember how many AI rounds hit
		//if(Ammo && Ammo->Flags & WEAP_AMMOFLAG_IS_AUTOGUNROUND)
		//	Vehicle->DamageQue.AIRoundsHit[ComponentNum]++;
	}

	if(Energy)
		TUT_MGR->DoEvent(TUT_EVENT_DAMAGE, &Energy, Vehicle);

	if (Vehicle->DamageQue.FirstRoundTimer == 0)
	{
		Vehicle->DamageQue.FirstRoundTimer = frametime;
	}

	if(Energy)
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP*** ACTUALLY APPLYING DAMAGE*** %f J to %s", Energy, Vehicle->VehicleType->DamageObjects[ComponentNum-1]);

#if WANT_OFFLINE_RAGDOLL
		if(!Online && Vehicle != PlayerVehicle && (frametime - Vehicle->VehicleInitTime) > 2000)
		{
			gr2AnimTrooper* anim = Vehicle->VehicleObjectData->PlaneObject->animModel;

			if(anim && anim->active)
			{
				gr2AnimBitPacker animFlags(anim->currentFlags);

				animFlags.deathState = 1;
				Vehicle->interpolator.SetFlags(animFlags.pack());
			}
		}
#endif
	}

	return 0;
}

/************************************************************************************************
	vehFindInertiaComponent
*************************************************************************************************/

vehInertiaComponent const* vehFindInertiaComponent(vehType* vehtype, const char* name)
{
	vehInertiaComponent* comp = nullptr;
	int i;

	for(i = 0; i < VEH_MAX_INERTIA_COMPONENTS; i++)
	{
		if(vehtype->Inertia[i].name[0] == '\0')
			break;

		if(!strcmp(vehtype->Inertia[i].name, name))
		{
			comp = &vehtype->Inertia[i];
			break;
		}
	}

	if(!comp)
	{
		static vehInertiaComponent defaultcomp = { "", 1, 1, {0,0,0}, {{1,0,0},{0,1,0},{0,0,1}} };
		comp = &defaultcomp;
	}
	return comp;
}

/************************************************************************************************
	vehFindStateByName
*************************************************************************************************/

vehanimVehicleStateType const* vehFindStateByName(vehType* vehtype, const char* name)
{
	vehanimVehicleStateType* stateType = nullptr;
	int i;

	for(i = 0; i < VEH_MAX_STATE_BITS; i++)
	{
		if(vehtype->StateType[i].Name[0] == '\0')
			break;

		if(!strcmp(vehtype->StateType[i].Name, name))
		{
			stateType = &vehtype->StateType[i];
			break;
		}
	}
	return stateType;
}

/************************************************************************************************
	vehAttachToRidePoint
*************************************************************************************************/

SINT32 vehAttachToRidePoint(vehVehicle* RiderVehicle)
{
    vehVehicle* RideeVehicle = nullptr;
	OBJECT* attachObj = nullptr;
	mlVehicleData* RiderModelData = nullptr;
	mlObjectData* RiderObjectData = nullptr;
	mlObjectData* RideeObjectData = nullptr;
	piVECTOR pos, att;
	piVECTOR relpos = {0,0,0};
	FLOAT64VECTOR WorldOriginOffset;
	FLOAT64VECTOR RidePos;

    if(RiderVehicle == nullptr) return -1;
	if(RiderVehicle->RideSeatNum == VEH_TOWED_VEHICLE) return 0; // might do something fancy later

	RideeVehicle = vehFindVehicle(RiderVehicle->RideVehicleHostIndex);

    if(!RideeVehicle)
	{
		RiderVehicle->PosData.Vel.z = -3000;
		return -1;
	}
	RiderModelData = RiderVehicle->ModelData;
	RiderObjectData = RiderVehicle->VehicleObjectData;
	RideeObjectData = RideeVehicle->VehicleObjectData;

	if(!RiderObjectData || !RideeObjectData)
	{
		RiderVehicle->PosData.Vel.z = -3000;
		return -2;
	}

	if(RiderVehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		// Cast to unsigned to overflow possible negatives.
		if((UINT32)RiderVehicle->RideSeatNum < ML_MAX_FPS_SEATS)
		{
			attachObj = RideeObjectData->FPSSeats[RiderVehicle->RideSeatNum];
		}else if(RiderVehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)// troopers walking on deck
		{
			fpMoveToRelRidePos(RideeVehicle, RiderVehicle);
			return 0;
		}else
		{
			RiderVehicle->PosData.Vel.z = -3000;
			return -99; // how'd we get here?
		}
	}else
	{
		if(RiderVehicle->RideSeatNum < VEH_HOIST_RIDE_SEAT) // riding
		{
			// Cast to unsigned to overflow negatives
			if((UINT32)RiderVehicle->RideSeatNum < ML_MAX_VEH_SEATS)
			{
				attachObj = RideeObjectData->VehSeats[RiderVehicle->RideSeatNum];
				relpos.z = RiderObjectData->LowestPoint;
			}
		} else // hoisting
		{
			// too fast, knock 'em off
			if(mathExactVectorLength2(&RideeVehicle->PosData.Vel) > 25.0)
				return -5;

			relpos.z = RiderObjectData->HighestPoint;
			attachObj = RideeObjectData->HoistObject;
		}
	}

	if(attachObj == nullptr) return -4;
	if(mlUtilityGetPosAndAttFromDOF(attachObj, &relpos, &pos, &att) != 0)
	{
		RiderVehicle->PosData.Vel.z = -3000;
		return -3;
	}
	trnGetWorldOriginOffset(&WorldOriginOffset);
	mathAddVectors(&RidePos, &pos, &WorldOriginOffset); RidePos.z = -RidePos.z;

	if(RiderModelData) // 1st person
	{
		FLOAT64VECTOR delta;
		mathSubVectors(&delta, &RidePos, &RiderModelData->AdjustedForCGPos);
		mathAddVectors(&RiderModelData->VehiclePos, &RiderModelData->VehiclePos, &delta);
		RiderModelData->AdjustedForCGPos = RidePos;

		if(RiderVehicle->VehicleType->Type != VEH_TYPE_PERSON)
		{
			RiderModelData->VehicleAtt.Pitch = att.x;
			RiderModelData->VehicleAtt.Roll = -att.y + PI; // usual insane sign flips
			RiderModelData->VehicleAtt.Yaw = -att.z;
		}else
		{
			if (RideeVehicle->VehicleType->Type == VEH_TYPE_AIR)
			{
				RiderModelData->VehicleAtt.Pitch = att.x;
				RiderModelData->VehicleAtt.Roll = -att.y + PI; // usual insane sign flips
				RiderModelData->VehicleAtt.Yaw = -att.z;
			}
			else
			{
				RiderModelData->VehicleAtt.Pitch = 0;
				RiderModelData->VehicleAtt.Roll = 0;
				RiderModelData->VehicleAtt.Yaw = -att.z + PI;
			}
		}
	} else // 3rd person
	{
		RiderVehicle->PosData.RenderPos = RidePos;
		RiderVehicle->PosData.Vel.x = 0;
		RiderVehicle->PosData.Vel.y = 0;
		RiderVehicle->PosData.Vel.z = 0;

		if(RiderVehicle->VehicleType->Type != VEH_TYPE_PERSON)
		{
			RiderVehicle->PosData.RenderAtt.Pitch = att.x;
			RiderVehicle->PosData.RenderAtt.Roll = -att.y + PI;
			RiderVehicle->PosData.RenderAtt.Yaw = att.z;
		}else
		{
			if (RideeVehicle->VehicleType->Type == VEH_TYPE_AIR)
			{
				RiderVehicle->PosData.RenderAtt.Pitch = -att.x;
				RiderVehicle->PosData.RenderAtt.Roll = -att.y + PI;
				RiderVehicle->PosData.RenderAtt.Yaw = att.z;
			}
			else
			{
				RiderVehicle->PosData.RenderAtt.Pitch = 0;
				RiderVehicle->PosData.RenderAtt.Roll = 0;
				RiderVehicle->PosData.RenderAtt.Yaw = att.z + PI;
			}
		}

		FLOAT64VECTOR delta;
		mathSubVectors(&delta, &RiderVehicle->PosData.RenderPos, &RideeVehicle->PosData.RenderPos);
		if(mathExactVectorLength(&delta) > RideeVehicle->VehicleObjectData->Radius)
			RiderVehicle->PosData.RenderPos.z = -3000;
	}

	return 0;
}

/************************************************************************************************
	vehFindRidingVehicle
*************************************************************************************************/

vehVehicle*	vehFindRidingVehicle(UINT32 RideVehicleHostIndex, UINT32 RideSeatNum, UINT32 VehicleType)
{
	vehVehicle* _VehicleHead, *CurrentVehicle;
	vehGetVehicleHead(&_VehicleHead);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(	CurrentVehicle->VehicleType->Type == VehicleType &&
			CurrentVehicle->RideVehicleHostIndex == RideVehicleHostIndex &&
			CurrentVehicle->RideSeatNum == RideSeatNum )
		{
			return CurrentVehicle;
		}
	}

	return nullptr;
}

/************************************************************************************************
	vehFindRidingVehicle
*************************************************************************************************/

LOGICAL vehVehicleInList(vehVehicle* Vehicle)
{
	vehVehicle* _VehicleHead, *CurrentVehicle;
	vehGetVehicleHead(&_VehicleHead);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(CurrentVehicle == Vehicle)
			return TRUE;
	}

	return FALSE;
}

/************************************************************************************************
	vehFindRidingVehicle - how many of this vehicle type is spawned
*************************************************************************************************/

int vehVehicleSpawned(const vehType* vtype, bool onlyPlayers)
{
	int count = 0;

	vehVehicle* _VehicleHead, *CurrentVehicle;
	vehGetVehicleHead(&_VehicleHead);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(onlyPlayers && CurrentVehicle->isSTO())
			continue;

		if(CurrentVehicle->VehicleType->WeaponSpec == vtype->WeaponSpec)
			++count;
	}

	return count;

}

#ifdef GM_RELEASE
/************************************************************************************************
	vehmainToggleLOD .. turns on and off LOD switches (off means all vehicles use LOD 0)
*************************************************************************************************/
void vehmainToggleLOD ();

void vehmainToggleLOD ()
{
	disableVehLODs = !disableVehLODs;
	if (disableVehLODs)
	{
		rnmainListText ("Vehicle LODs disabled");
	}
	else
	{
		rnmainListText ("Vehicle LODs enabled");
	}
}
#endif

/************************************************************************************************
	_getVehicleSpawnObjectPosition
*************************************************************************************************/
static SINT32 _getVehicleSpawnObjectPosition(vehVehicle* veh, piVECTOR* vOut)
{
	OBJECTDOF* dof = _getVehicleSpawnObjectDof(veh);
	if(!dof) return -1;

	piVECTOR v =
    {
        dof->dofmatrix2._41,
        dof->dofmatrix2._42,
        dof->dofmatrix2._43
    };

    math3dVectorMultiply(vOut, &v, &veh->ModelData->VehicleAttMat);

	vOut->x += (FLOAT32)veh->ModelData->RenderPos.x;
	vOut->y += (FLOAT32)veh->ModelData->RenderPos.y;
	vOut->z += (FLOAT32)veh->ModelData->RenderPos.z;

    return 0;
}

/************************************************************************************************
	_checkFacilityRange
*************************************************************************************************/
static MSPAWN_RESULT _checkFacilityRange(StratDB::StratObject* so, int side)
{
	if(so->StratType() == FACILITY_FIREBASE && !(so->StratFlags() & CP_STAT_OPEN))
		return MSDR_TARGET_DESTROYED;

    FLOAT64VECTOR playPos = {-playpos.x, -playpos.y, -playpos.z};

    SINT32 x = so->PosX();
    SINT32 y = so->PosY();

    FLOAT64VECTOR facPos, diff;

    // Facility world position
    facPos.x = (FLOAT64)x - WorldOriginOffset.x;
    facPos.y = (FLOAT64)y - WorldOriginOffset.y;
    facPos.z = playPos.z;

    // Find distance sqrd from player
    mathSubVectors(&diff, &facPos, &playPos);

    FLOAT64 facDis2 = mathExactVectorLength2(&diff);

	if(facDis2 > TFR_RANGE_2)
		return MSDR_TFR;

	// If it is an enemy facility, is it* too* close?
	if(so->Side() != side && facDis2 <= EFR_RANGE_2)
		return MSDR_EFR;

	return MSDR_GRANTED;
}

/************************************************************************************************
	_inTargetFacilityRange
*************************************************************************************************/
static MSPAWN_RESULT _inTargetFacilityRange(int side)
{
 	FULL_OBJ_ID foi = PlayerClient()->SelectedMission().m_target;
    StratDB::StratObject* so = StratDB::FindStratObject((sdm_full_obj_id&)foi);

	if(!so) return MSDR_TFR;

    if(so->StratClass() == SDM_CLASS_FACILITY)
    {
		return _checkFacilityRange(so,side);
    }

	if(so->StratClass() != SDM_CLASS_CP)
		return MSDR_TARGET_INVALID;

	if(so->StratType() == SDM_CP_BRIDGE)
		return MSDR_TARGET_INVALID;

	StratDB::StratObject::ChildList cl = so->ChildStratObjects();

	if(cl.empty())
		return MSDR_TARGET_INVALID;

	StratDB::StratObject::ChildList::iterator B = cl.begin();
	StratDB::StratObject::ChildList::iterator E = cl.end();

	bool foundFacilityInRange = false;
	for(; B != E; ++B)
	{
		// Find closest distance
		if((*B)->StratClass() == SDM_CLASS_FACILITY)
		{
			// Look for a friendly facility in range
			MSPAWN_RESULT attempt = _checkFacilityRange(*B,side);
			// If it's an enemy facility in range, bail out
			if(attempt == MSDR_EFR)
				return MSDR_EFR;
			// If this facility was "granted", we need to
			// continue incase we* do* find an MSDR_EFR
			if(attempt == MSDR_GRANTED)
				foundFacilityInRange = true;
        }
    }

	if(foundFacilityInRange)
		return MSDR_GRANTED;

    return MSDR_TFR;
}

/************************************************************************************************
	_nearEnemyFacility
*************************************************************************************************/
static MSPAWN_RESULT _nearEnemyFacility(FULL_OBJ_ID& foi, int side)
{
    FLOAT64VECTOR playPos = {-playpos.x, -playpos.y, -playpos.z};

    StratDB::StratObject* so =  StratDB::FindStratObject((sdm_full_obj_id&)foi);

	// If we don't know where it is, it won't prohibit you spawning
    if(!so) return MSDR_GRANTED;
	if(so->Side() == side) return MSDR_GRANTED;

    SINT32 x = so->PosX();
    SINT32 y = so->PosY();

    FLOAT64VECTOR facPos, diff;

    // Facility world position
    facPos.x = (FLOAT64)x - WorldOriginOffset.x;
    facPos.y = (FLOAT64)y - WorldOriginOffset.y;
    facPos.z = playPos.z;

    // Find distance sqrd from player
    mathSubVectors(&diff, &facPos, &playPos);

    FLOAT64 facDis2 = mathExactVectorLength2(&diff);

	if(facDis2 <= EFR_RANGE_2) return MSDR_EFR;

	return MSDR_GRANTED;
}

/************************************************************************************************
	vehmainFRUBuild
*************************************************************************************************/

PlayerPlacedObject* vehmainFRUBuild()
{
	vehVehicle* veh = persGetPersonaVehiclePtr();
	if(veh == nullptr) return nullptr;

	// do we already have an msp on this misison?
	bool hasMobileSpawn = PlayerClient()->MobileSpawnMission();
	bool isMissionLeader = PlayerClient()->IsMissionLeader();

	// now check if we're the mission leader
	if(hasMobileSpawn && !isMissionLeader)
	{
		fpPPOAlert("ppos.fru.notleader");
		return nullptr;
	}

	PlayerPlacedObject* fru = fpPPOCreate(activePPOList[activePPOIndex]->id);

	// set our callbacks
    if (fru != nullptr)
    {
        // do one update to check our current validity
        if(fpPPOUpdate())
        {
            fpPPOAlert();
            return nullptr;
        }
    }
    else
    {
        dbugLog(_FLW, "Failed to create fru - fpPPOCreate for index %d and id %d", activePPOIndex, activePPOList[activePPOIndex]->id);
    }

	return fru;
}

/************************************************************************************************
	vehHandleMobileSpawnUpdate
*************************************************************************************************/

void vehHandleMobileSpawnUpdate(MSPAWN_RESULT enumResult)
{
	vehVehicle* veh = persGetPersonaPilotedOnlineVehicle();

	bool isError;

	if ( enumResult == MSDR_GRANTED || enumResult == MSDR_UNDEPLOYED )
		isError = false;
	else
		isError = true;

	uiChatPrintLineFeedback(IDE_LANG_MOBILE, enumResult, isError);

    switch(enumResult)
    {
		case MSDR_GRANTED:
		{
			_setVehicleDeployedState(IDE_VEHICLE_DEPLOYED);

			if(!veh) return;

			// One more check
			MSPAWN_RESULT permission = _canWeDeploy(veh, false);

			if(permission != MSDR_GRANTED)
			{
				if(vehmainIsDeployed())
				{
					// Print the message
					vehHandleMobileSpawnUpdate(permission);
					// And undeploy the vehicle
					vehmainUndeployVehicleForMobileSpawn();
				}
				return;
			}

			veh->deployedTimeout = frametime;
			g_mobileUpdateTimer = frametime;

			break;
		}
		case MSDR_UNDEPLOYED:
		{
			// Make sure doors are shut
			_resetBombBayDoors();
			_setVehicleDeployedState(IDE_VEHICLE_UNDEPLOYED);
			break;
		}
		default:
		{
			// Make sure doors are shut
			_resetBombBayDoors();
			_setVehicleDeployedState(IDE_VEHICLE_UNDEPLOYED);
		}
    }
}

/************************************************************************************************
	_canWeDeploy
*************************************************************************************************/
static MSPAWN_RESULT _canWeDeploy(vehVehicle* veh, bool ignoreMovement)
{
	g_canDeployMobileSpawn = false;

    if(!veh || !veh->ModelData)
		return MSDR_FAILED;

	if(_getVehicleSpawnObjectDof(veh) == nullptr)
		return MSDR_VEHICLE;

	int side = veh->Side;

    // Leaving
    if(persIsInDeathCam() || veh->ModelData->VehicleIsDead)
		return MSDR_INVALID_STATE;

	// We are dead or vehicle is dead (not spawnable)
    if(persIsCrewDead())
		return MSDR_DEAD;

    // Ground vehicle and moving
    if(persIsPersonaVehicleType(VEH_TYPE_GROUND))
	{
		// Are you on a winch?
		if(veh->RideVehicleHostIndex > 0 && veh->RideSeatNum >= VEH_HOIST_RIDE_SEAT) return MSDR_MOVING;

		if ( !ignoreMovement )
		{
			// Moving?
			bool isMoving = mathExactVectorLength2(&veh->ModelData->VehicleVel) >= 1.0f;
			if(isMoving)
				return MSDR_MOVING;
		}
	}
	else if(persIsPersonaVehicleType(VEH_TYPE_AIR))
	{
	    // Moving?
		bool aboveMinSpeed = (mathExactVectorLength2(&veh->ModelData->VehicleVel) >= 10.0f);
		// Aircraft must be in-flight to deploy a mobile spawn
		if(!aboveMinSpeed)
			return MSDR_STATIONARY;
	}

    // Not on a mission (always on missions now right?)
 //   if(uiMgrGetMissionID() == 0)
//		return MSDR_NO_MISSION;

    // Near mission target?
	MSPAWN_RESULT attempt = _inTargetFacilityRange(side);
	if(attempt != MSDR_GRANTED)
		return attempt;

    // Close enough to an enemy facility?
	attempt = _nearEnemyFacility(veh->ModelData->closestCapTO, veh->Side);
	if(attempt != MSDR_GRANTED)
		return attempt;

	g_canDeployMobileSpawn = true;

    return MSDR_GRANTED;
}

/************************************************************************************************
	_checkMobileSpawnAbility()
*************************************************************************************************/

static void _checkMobileSpawnAbility()
{
	// Don't bother doing anything if we aren't incharge
	// of a vehicle online
	vehVehicle* veh = persGetPersonaPilotedOnlineVehicle();
	if(veh == nullptr)
		return;

	// Ignore vehicles that don't have a mobile spawn object
	if(_getVehicleSpawnObjectDof(veh) == nullptr)
		return;

	// Ignore vehicles that are deployed
	if(veh->StateAnimation.enumDeployedState > IDE_VEHICLE_UNDEPLOYED)
		return;

	// Are we currently allowed to mobile spawn?
	bool couldDeploy = g_canDeployMobileSpawn;
	_canWeDeploy(veh, true);

	// Has this changed since we last checked?
	if(couldDeploy == g_canDeployMobileSpawn)
		return;

	if(g_canDeployMobileSpawn)
		uiChatPrintLineFeedback(IDE_LANG_MOBILE, 504, false);
	else
		uiChatPrintLineFeedback(IDE_LANG_MOBILE, 505, false);
}

/************************************************************************************************
	vehmainDeployVehicleForMobileSpawn
*************************************************************************************************/

SINT32 vehmainDeployVehicleForMobileSpawn()
{
	vehVehicle* veh = persGetPersonaPilotedOnlineVehicle();
	if(veh == nullptr)
		return -1;

    if(_checkCurrentDeployState(veh) != 0)
        return -1;

	MSPAWN_RESULT permission = _canWeDeploy(veh, false);

	if(permission != MSDR_GRANTED)
	{
		vehHandleMobileSpawnUpdate(permission);
		return -1;
	}

    piVECTOR spawnPos;

    // Get position of the vehicle mobile spawn object
    if(_getVehicleSpawnObjectPosition(veh, &spawnPos) != 0)
        return -1;

    // Send position to host
///FIXME: This functionality is no-longer supported.
/*
    ClientMobileSpawnUpdate cmsu(MSST_DEPLOYING, spawnPos.x, spawnPos.y, spawnPos.z);

    cmsu.send(PlayerClient()->CellConnection());
*/

    _setVehicleDeployedState(IDE_VEHICLE_DEPLOYING);

    veh->deployingTimeout = frametime;

	// new mobile spawn functionality
	//uiGameEvent(GAMEEVENT_ALERT, 1, L"Building Unmanned Mobile Spawn Point!");

    return 0;
}

/************************************************************************************************
	_checkCurrentDeployState
*************************************************************************************************/
SINT32 _checkCurrentDeployState(vehVehicle* veh)
{
    switch(veh->StateAnimation.enumDeployedState)
    {
    case IDE_VEHICLE_DEPLOYED:
        {
            // if deployed, there is a timeout period where you cannot undeploy.
            if(_isDeployedTimeout())
            {
				/* new mobile spawn functionality
				FLOAT64VECTOR& vehPos	= (veh->ModelData) ? veh->ModelData->VehiclePos: veh->PosData.Pos;
				piMATRIX& vehAtt			= (veh->ModelData) ? veh->ModelData->VehicleAttMat: veh->PosData.AttMat;
				FLOAT64VECTOR umsPos	= {0, 0, 0};
				piVECTOR umsAtt				= {0, 0, 0};

				if(trnopDynamicBase(vehPos, vehAtt, 2.0f, 1.0f, umsPos, umsAtt))
				{
					// update our ums spawn position to the center of the ums object
					ClientMobileSpawnUpdate cmsu(MSST_DEPLOYED, vehPos.x, vehPos.y, umsPos.z + 1.0f);
					cmsu.send(PlayerClient()->CellConnection());

					// Request a server tracked object (STO) from the host
					WorldPosition_m position(umsPos);
					AttitudeVector attitude(umsAtt.x, umsAtt.y, umsAtt.z);

					MobileSpawnSTO ums(0, veh->VehicleType->VehicleTypeNum, position, attitude);
					ums.request(PlayerClient()->ChatConnection());

					_setVehicleDeployedState(IDE_VEHICLE_UNDEPLOYING);
				    _resetBombBayDoors();

					uiGameEvent(GAMEEVENT_ALERT, 1, L"Unmanned Mobile Spawn Point Built! Move Out!");
				}
				else
				{
					vehmainUndeployVehicleForMobileSpawn();
				}
				*/

				 vehmainUndeployVehicleForMobileSpawn();
            }
            else
            {
                SINT32 timeOut = MOBILE_DEPLOYED_TIMEOUT / 1000;
                SINT32 timeRemaining = timeOut - ((frametime - veh->deployedTimeout) / 1000);

				uiChatPrintLineFeedback(IDE_LANG_MOBILE, 503, false, timeRemaining);
           }

            return -1;
        }
    case IDE_VEHICLE_DEPLOYING:
        {
            // if deploying, there is a timeout where you cannot undeploy.
            if(_isDeployingTimeout())
            {
                vehmainUndeployVehicleForMobileSpawn();
            }
            else
            {
                // "Mobile spawn request in progress."
                uiChatPrintLineFeedback(IDE_LANG_MOBILE, 502, false);

				// new mobile spawn functionality
				//uiGameEvent(GAMEEVENT_ALERT, 1, L"Building Unmanned Mobile Spawn Point!");
            }

            return -1;
        }
    case IDE_VEHICLE_UNDEPLOYED:
    case IDE_VEHICLE_UNDEPLOYING:
        {
            // "Vehicle Deploy: Requesting mobile spawn..."
			uiChatPrintLineFeedback(IDE_LANG_MOBILE, 501, false);
            return 0;
        }
    case IDE_DEPLOYED_NOT_USED:
        {
             // "Vehicle Deploy: Requesting mobile spawn..."
			uiChatPrintLineFeedback(IDE_LANG_MOBILE, 501, false);
            return 0;
       }
    default:
        // undefined
        break;
    }
    return 0;
}

/************************************************************************************************
	vehmainUndeployVehicleForMobileSpawn
*************************************************************************************************/
void vehmainUndeployVehicleForMobileSpawn()
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	// Crew can't undeploy vehicles
	if(VehicleData && VehicleData->MultiCrewPos > 1)
		return;

    // Send position to host
///FIXME: This functionality is no-longer supported.
/*
    ClientMobileSpawnUpdate cmsu(MSST_UNDEPLOYED, 0.0f, 0.0f, 0.0f);
	cmsu.send(PlayerClient()->CellConnection());
*/

    _setVehicleDeployedState(IDE_VEHICLE_UNDEPLOYING);

    _resetBombBayDoors();
}

/************************************************************************************************
	vehmainIsMobileSpawn - returns true if you are the mobile spawn
*************************************************************************************************/

LOGICAL vehmainIsMobileSpawn()
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

    if(VehicleData &&
	   VehicleData->VehicleMasterPtr &&
	   VehicleData->VehicleMasterPtr->VehicleType &&
	   VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_GROUND)
    {
        // Case 1:
        // Deployed and not a rider
        if(vehmainIsDeployed() && VehicleData->BombBayDoor)
        {
            return TRUE;
        }

        // Case 2:
        // Deployed and a rider (truck on a ship, for example)
        if(vehmainIsDeployed() && VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0)
        {
            return TRUE;
        }
    }

	return FALSE;
}

/************************************************************************************************
	vehmainCanSTO -
*************************************************************************************************/

LOGICAL vehmainCanSTO()
{
    mlVehicleData* data = persGetPersonaModelDataPtr();
    if(!data)
    {
        return FALSE;

    }
    FASTCOLLIDE list[32];

    memset(list, 0, sizeof(FASTCOLLIDE) * 32);

    UINT32 numcollides = 32;

    COLLISION c;

    memset(&c, 0, sizeof(COLLISION));

    c.oldpos.x = (FLOAT32)(data->VehiclePos.x - WorldOriginOffset.x);
    c.oldpos.y = (FLOAT32)(data->VehiclePos.y - WorldOriginOffset.y);
    c.oldpos.z = (FLOAT32)(data->VehiclePos.z);

    c.oldpos.z = -10000.0f;

    c.newpos = c.oldpos;
    c.newpos.z = 10000.0f;

    bool collided = false;

    // Cannot STO if on a bridge
    if(trncdCollideDownShortList(c, list, numcollides))
    {
        collided = true;

        // List is sorted
        if(list[0].TO->ObjId.objClass == CLASS_CP &&
           list[0].TO->ObjId.objType == CP_BRIDGE)
        {
            return FALSE;
        }
    }

    // Cannot STO if riding on a ship
    if(data->VehicleMasterPtr->RideVehicleHostIndex > 0)
    {
        vehVehicle*v = nullptr;

        v = vehFindVehicle(data->VehicleMasterPtr->RideVehicleHostIndex);

        if(v)
        {
            if(v->VehicleType->Type == VEH_TYPE_SEA)
            {
                return FALSE;
            }
        }
    }

    // Cannot HULK if inside a building // Ref ticket #2023
    if(data->VehicleMasterPtr->isHulked() && collided)
    {
        if(list[0].TO->ObjId.objClass == CLASS_BUILDING)
        {
            return FALSE;
        }
    }

    return TRUE;
}

UINT32 vehmainAddSTO(const char* type, double x, double y, double z, float att, bool relative, const char* icon)
{
	return vehmainAddSTO(vehTypeGetById(type), x, y, z, att, relative, icon);
}

UINT32 vehmainAddSTO(const vehType* type, double x, double y, double z, float att, bool relative, const char* icon)
{
	if(!type) return 0;

    FLOAT64VECTOR pos = {x, y, z};

	if(relative)
	{
		mlVehicleData* modelData = persGetPersonaModelDataPtr();

		if(modelData)
		{
			pos.x += modelData->VehiclePos.x;
			pos.y += modelData->VehiclePos.y;
			pos.z += modelData->VehiclePos.z;
		}
	}

	// adjust position by the world origin offset

    WorldPosition_m position(pos.x, pos.y, pos.z);
    AttitudeVector attitude(0, 0, att);

    VehicleSTO* vehicle = new VehicleSTO(0, type, position, attitude);
	vehicle->setIcon(icon);

    ServerTrackedObject::addToIndex(vehicle);

	// add to the vehicles list
    return vehicle->uid();
}

bool vehmainRemoveSTO(UINT32 id)
{
    ServerTrackedObject* sto = ServerTrackedObject::lookup(id);

    if(sto)
    {
        sto->deactivate();
		wwiiRemoveSTO(id);

		/*
	    obcsResetSortedList();
		obcsResetUnsortedList();
		obcsResetMultiTexture();
		obcsResetSortedCockpitList();
		obcsResetUnsortedCockpitList();
		*/

		return true;
    }

	return false;
}

/************************************************************************************************
	_updateMobileSpawn
*************************************************************************************************/

void _updateMobileSpawn()
{
	// If we are deploying and have not received a GRANTED from the host within a
	// timeout period, tell host to undeploy (took too long)
	if(_isDeployingTimeout())
	{
		vehHandleMobileSpawnUpdate(MSDR_LAPSED);
		vehmainUndeployVehicleForMobileSpawn();
		return;
	}

	if(!vehmainIsDeployed())
	{
		_checkMobileSpawnAbility();
		return;
	}

	// Send updates to the host periodically
	if(frametime - g_mobileUpdateTimer < MOBILE_SPAWN_UPDATE_TIMER)
	{
		return;
	}

	vehVehicle* veh = persGetPersonaPilotedOnlineVehicle();

	g_mobileUpdateTimer = frametime;

	MSPAWN_RESULT permission = _canWeDeploy(veh, false);
	if(permission != MSDR_GRANTED)
	{
		vehHandleMobileSpawnUpdate(permission);
		vehmainUndeployVehicleForMobileSpawn();

		return;
	}

	piVECTOR spawnPos;

	// Get position of the vehicle mobile spawn object
	if(_getVehicleSpawnObjectPosition(veh, &spawnPos) != 0)
		return;

}

/************************************************************************************************
	_setVehicleDeployState
*************************************************************************************************/
void _setVehicleDeployedState(DEPLOYEDSTATE enumState)
{
	vehVehicle*veh = persGetPersonaVehiclePtr();
	if(!veh) return;

	veh->StateAnimation.enumDeployedState = enumState;
}

/************************************************************************************************
	_isDeployingTimeout
*************************************************************************************************/
LOGICAL _isDeployingTimeout()
{
    vehVehicle*veh = persGetPersonaVehiclePtr();

    if(!veh) return FALSE;

    if(!vehmainIsDeploying())
        return FALSE;

    return  frametime - veh->deployingTimeout > MOBILE_DEPLOYING_TIMEOUT;
}

/************************************************************************************************
	_isDeployedTimeout
*************************************************************************************************/
LOGICAL _isDeployedTimeout()
{
    vehVehicle*veh = persGetPersonaVehiclePtr();

    if(!veh) return FALSE;

    if(!vehmainIsDeployed())
        return FALSE;

    return  frametime - veh->deployedTimeout >= MOBILE_DEPLOYED_TIMEOUT;
}

/************************************************************************************************
	vehmainIsDeploying
*************************************************************************************************/
LOGICAL vehmainIsDeploying()
{
    vehVehicle*veh = persGetPersonaVehiclePtr();

    if(!veh) return FALSE;

    return  veh->StateAnimation.enumDeployedState == IDE_VEHICLE_DEPLOYING;
}

/************************************************************************************************
	vehmainReportCache
*************************************************************************************************/
void vehmainReportCache(int& fcount, int &rcount, bool update)  // VEHSTUFF
{
	if(update)
		_UpdateReadyList(false, true);

    if ( IS_OFFLINE )
	{
        fcount = (int)g_freeVeh.size();
    }
    else
    {
        fcount = 0; // no such thing
    }


	rcount = (int)_readyMap.size();
}

/************************************************************************************************
	vehmainClearVehicleCache
*************************************************************************************************/
void vehmainClearVehicleCache()
{
#ifdef CLIENT_VEHICLE_CACHING

	if(_readyMap.empty())
		return;

	VEHICLE_READY_MAP::iterator itr = _readyMap.begin();
	std::vector<ChatHandle> handles;

	for(; itr != _readyMap.end(); ++itr)
		handles.push_back(itr->first);

	for(unsigned int i = 0; i < handles.size(); i++)
		_RemoveFromReady(handles[i], true);

#ifdef CLIENT_VEHICLE_CACHING_REPORT
	_vCacheHits = _vCacheTries = 0;
#endif

	_readyMap.clear();
#endif // CLIENT_VEHICLE_CACHING
}

bool vehmainIsVehicleCached(ChatHandle ch)
{
	VEHICLE_READY_MAP::iterator itr = _readyMap.begin();

	for(; itr != _readyMap.end(); itr++)
	{
		if(itr->first == ch)
			return true;
	}

	return false;
}

/*
bool vehmainCachePrintDiff()
{
	std::vector<vehVehicle*> missing;

	for(int i = 0; i < MAX_POSSIBLE_VIS_VEHICLES; i++)
	{
		vehVehicle* v = &g_vehicles[i];

		bool visFound = false;
		bool readyFound = false;
		bool freeFound = false;

		vehVehicle*CurrentVehicle, *NextVehicle;

		for (CurrentVehicle = _VehicleHead.Next;
			 CurrentVehicle != nullptr; )
		{
			NextVehicle = CurrentVehicle->Next;

			if(v == CurrentVehicle)
			{
				visFound = true;
				break;
			}

			CurrentVehicle = NextVehicle;
		}

		VEHICLE_READY_MAP::iterator ritr = _readyMap.begin();

		for(; ritr != _readyMap.end(); ritr++)
		{
			if(v == ritr->second)
			{
				readyFound = true;
				break;
			}
		}

		std::stack<vehVehicle*> s;

		std::vector<vehVehicle*>::iterator qitr = g_freeVeh.begin();

		for(; qitr != g_freeVeh.end(); qitr++)
		{
			if(v == *qitr)
			{
				freeFound = true;
				break;
			}
		}

		if(visFound == false && readyFound == false && freeFound == false)
		{
			return true;
			//missing.push_back(v);
		}
	}

	for(int i = 0; i < missing.size(); i++)
	{
		vehVehicle* v = missing.at(i);

		uiChatPrintLine("# Missing Vehicle # callsign: %s index: %u type: %s used: %s hulk: %s sto: %s",
			(const char*)v->Handle,
			v->VehicleHostIndex,
			(v->VehicleType) ? (const char*)v->VehicleType->Id: "na",
			(v->InUse) ? "true": "false",
			(v->isHulked()) ? "true": "false",
			(v->isSTO()) ? "true": "false");
	}

	return false;
}
*/

/************************************************************************************************
	vehmainIsDeployed
*************************************************************************************************/
LOGICAL vehmainIsDeployed()
{
    vehVehicle*veh = persGetPersonaVehiclePtr();

    if(!veh) return FALSE;

    return  veh->StateAnimation.enumDeployedState == IDE_VEHICLE_DEPLOYED;
}

/************************************************************************************************
	vehmainIsUndeploying
*************************************************************************************************/
LOGICAL vehmainIsUndeploying()
{
    vehVehicle*veh = persGetPersonaVehiclePtr();

    if(!veh) return FALSE;

    return  veh->StateAnimation.enumDeployedState == IDE_VEHICLE_UNDEPLOYING;
}

/************************************************************************************************
	vehmainIsUndeployed
*************************************************************************************************/
LOGICAL vehmainIsUndeployed()
{
    vehVehicle*veh = persGetPersonaVehiclePtr();

    if(!veh) return FALSE;

    return  veh->StateAnimation.enumDeployedState == IDE_VEHICLE_UNDEPLOYED;
}

LOGICAL vehmainIsOverSupplied(vehVehicle* vehicle)
{
	if(vehicle == nullptr || vehicle->ModelData == nullptr)
		return 0;

	if(vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		// handle infantry
		for (int i = 0; i < VEH_MAX_MOUNTS; i++)
		{
			for (int j = 0; j < VEH_MAX_WEAPONS_PER_MOUNT; j++)
			{
				if(vehicle->Weapons[i][j].NumOverSupplyRounds[0] > 0 ||
				   vehicle->Weapons[i][j].NumOverSupplyRounds[1] > 0)
					return 1;
			}
		}
	}
	else
	{
		// handle vehicles
		mlComponent* Component;
		mlComponentListIterator it;

		mlComponentListBegin(vehicle->ModelData->ComponentList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			Component = it.comp;

			if(Component->Type == ML_COMPONENT_WEAPONMOUNT)
			{
				mlWeaponMount* Mount = (mlWeaponMount*)(Component->SpecialData);

				if(Mount)
				{
					for(int i = 0; i < ML_MAX_WEAPONS_PER_MOUNT; i++)
					{
						if(Mount->Weapons[i].NumOverSupplyRounds[0] > 0)
							return 1;
					}
				}
			}
		}
	}

	return 0;
}

SINT32 vehmainRequestAmmo(const char* input)
{
	static TeulTimeMS nextAmmoCommand = 0;

	if(teulClock64 >= nextAmmoCommand)
	{
		vehVehicle* veh = persGetPersonaVehiclePtr();

		if(veh && veh->VehicleType && (veh->VehicleType->Type == VEH_TYPE_GROUND || veh->VehicleType->Type == VEH_TYPE_PERSON))
		{
			bool isArmed = (veh->VehicleType->Type == VEH_TYPE_PERSON ||
						   (veh->ModelData && veh->ModelData->NumberOfWeaponMounts)) ? true: false;

			if(isArmed)
			{
				Waypoint waypoint(false, veh->Chat_Handle, veh->Chat_Handle, WPT_AMMO, 0, 0, 0, 0, "");
				WAYPOINT_XFER transfer(waypoint, WPXT_REQUEST);
				transfer.send(PlayerClient()->ChatConnection());
				uiChatPrintLine(uiLocString("request.ammo.ok").c_str());
			}
			else
			{
				uiChatPrintLine(uiLocString("request.ammo.na").c_str());
			}
		}
		else
		{
			uiChatPrintLine(uiLocString("request.ammo.onlyground").c_str());
		}

		// only let them send off the command every 30 seconds
		nextAmmoCommand = teulClock64 + 30000;
	}
	else
	{
		uiChatPrintLine(uiLocString("request.ammo.toosoon").c_str());
	}

	return 0;
}

/************************************************************************************************
	_resetBombBayDoors
*************************************************************************************************/

void _resetBombBayDoors()
{
    mlVehicleData* vd = persGetPersonaModelDataPtr();
	if (vd && vd->BombBayDoor == 1) {
			mlToggleBombBayDoor(1, 0, 0);
	}
}

/************************************************************************************************
	_FetchSupercell
*************************************************************************************************/

#if(0)
static void _FetchSupercell (int x, int y, TERRAIN_DETAIL** td)
{
	int i;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if ((trndtl[i].scx == x) && (trndtl[i].scy == y) && (trndtl[i].terrain))
		{
			*td = &trndtl[i];
			return;
		}
	}

	*td = nullptr;
}
#endif

/************************************************************************************************
	vehmainHandleVehicleKilled - based on the component, find out what kill message killed it, and
	   from that, determine the killer, and notify the host

	returns TRUE if it assigned kill credit, FALSE if it didn't
*************************************************************************************************/

UINT32 vehmainDetermineComponentKiller(mlComponent* killedComponent)
{
	if(PN_UNLIKELY(!killedComponent))
		return 0;

	// using a reverse iterator, scan the list of hit messages until we find one
	// that is damaging the component that was killed

	vehVehicle* player = persGetPersonaVehiclePtr();
	VEHICLE_HIT_LIST::reverse_iterator vehHitIt = g_MyHitMessages.rbegin();

	for (; vehHitIt != g_MyHitMessages.rend(); ++vehHitIt)
	{
		Vehicle::Hit& hit = *vehHitIt;

		for (Vehicle::Hit::Components::iterator it = hit.m_components.begin(); it != hit.m_components.end(); ++it)
		{
			Vehicle::Hit::Component& comp = *it;

			if (comp.compID >= VEH_MAX_DAMAGE_OBJECTS)
				continue;

			// several vehicles have more components than this so they are missed in this check..nice
			// check out the comment by the declaration of DamageComponentNum in mlComponent
			for (size_t j=0; j < 4; j++)
			{
				if (comp.compID == killedComponent->DamageComponentNum[j])
				{
					UINT32 shooter = hit.m_shooter;

					//if(hit.m_shooter == 0 && comp.AIHits > 0)
					//	shooter = ~0;

					// send the kill notice right away?
					/*
					if(player->VehicleType->Type != VEH_TYPE_AIR)
					{
						commSendKillNotice(PlayerClient()->CellConnection(),
										   player->VehicleHostIndex,
										   shooter);
					}
					*/

#ifdef BETA_RELEASE
					static sortieid_t mySortie = 0;

					if(Online && mySortie != player->VehicleHostIndex)
					{
						uiChatPrintLine("Damage from %u killed us", hit.m_shooter);
						mySortie = player->VehicleHostIndex;
					}
#endif

					Write_Log(LOG_WEAP, LOG_INFO, "WEAP Determined KilledBy due to damage to hitmap %d .. shooter = %u", comp.compID, hit.m_shooter);
					Write_Log(LOG_COMM, LOG_INFO, "COMM Sending KilledBy message to host .. vehicle index = %u", hit.m_shooter);

					return shooter;
				}
			}
		}
	}

	return (player) ? player->VehicleHostIndex : 0; // me or none
}

UINT32 vehmainDeterminePlinkKiller()
{
	if(g_MyHitMessages.empty())
		return 0;

	vehVehicle* player = persGetPersonaVehiclePtr();
	VEHICLE_HIT_LIST::reverse_iterator vehHitIt = g_MyHitMessages.rbegin();

	//uiChatPrintLine("We have some hit messages!");
	for (; vehHitIt != g_MyHitMessages.rend(); ++vehHitIt)
	{
		Vehicle::Hit& lastHit = *vehHitIt;

		SINT32 delta = (SINT32)(teulClock64 - lastHit.m_timestamp);

		if(lastHit.m_shooter && lastHit.m_shooter != player->VehicleHostIndex && delta <= 60000) // 30 seconds to get away
		{
#ifdef BETA_RELEASE
			uiChatPrintLine("Plink kill from %u killed us with %d ms into window", lastHit.m_shooter, delta);
#endif

			Write_Log(LOG_WEAP, LOG_INFO, "Bailoutkill Determined by last hit from %u %ums ago", lastHit.m_shooter, delta);

			return lastHit.m_shooter;
		}
	}

	// no-one plink killed me
	return 0;
}

void vehmainSpawnVehicle(VEHICLE_REPLY const* reply)
{
	_spawnvehicle = true;
	_vehicletospawn = *reply;
}

#define BINOCS_RANK 1

#ifdef ANTIHACK
#include "tools/diddle.h"
#endif

static void vehmainSpawn()
{
#ifdef ANTIHACK
	UCHAR yeahCode = Online ? (HackID[0] != 0 ? OH_YEAH_DAMNED : 0) : 0;
	bool checksumOK = true;
#endif

	// reset any shakes
	persShakeCamera(-1);

	_spawnvehicle = false;

	_bailoutPos = 0;
	_bailoutTime = 0;
	_bailoutLife = 0;

	// fixes an init issue with ppo materials
	fpPPOCreateMaterial();

	vehVehicle*pVehicle = nullptr;
    WEAPON* pWeapon = nullptr;

	g_MyHitMessages.clear();

    pWeapon = weapLookup(_vehicletospawn.spec);

	ww2imapSetKeymap(pWeapon->localIndex());
	uiUIToGameStart();

#ifdef ANTIHACK
	extern Hideaway* checksum;
	extern void* Imagebase;

	if ( Online )
	{
		checksumOK = checksum->compare(diddle_crc((const unsigned char*)(
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->ImageBase +
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->BaseOfCode),
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->SizeOfCode));
	}
#endif

	// this needs to be before vehupdateCreatePlayerControlledVehicle .. that's where
	// fpFPSInit is called

	int binocsRank = BINOCS_RANK;

	ARENA_VARS::iterator ait = g_arenaDescription.find("binocs_rank");

	if(ait != g_arenaDescription.end())
		sscanf("%d", ait->second.c_str(), &binocsRank);

	fpFPSEnableBinoculars (_vehicletospawn.rankLevel >= (UINT16)binocsRank && PlayerClient()->IsPermitted(USE_BINOCS));
	mlSetMouseLookEnabled(FALSE);

	vehupdateCreatePlayerControlledVehicle(pWeapon->localIndex()
											,0
											, _vehicletospawn.hostIndex
											, _vehicletospawn.persVehicleIndex
											, _vehicletospawn.station);

	pVehicle = vehFindVehicle(_vehicletospawn.hostIndex);

    if(pVehicle == nullptr)
        piExitProgram(_FL,"[vehcommProcessVehicleRequestResponse] pVehicle == nullptr");

	pVehicle->WeaponSerial.source = _vehicletospawn.source;
	pVehicle->WeaponSerial.serial = _vehicletospawn.hostIndex;
	pVehicle->WeaponSerial.spec = _vehicletospawn.spec;

	if(!gEZMode || !EZManager::CreateManager(pVehicle))
		EZManager::FreeManager();

	persAttachPlayerToVehicle(pVehicle, 0);

	// now ensure they're in the correct position to start with
	persSetPersonaPosition(0);

	// reset any old vehicle settings as needed
	persResetPersonaSettings();

	pVehicle->Country = _vehicletospawn.spec.wCountry;
	pVehicle->Side = wwiiSide(pVehicle->Country);
	pVehicle->Rank = _vehicletospawn.rankLevel;

	if(!Online)
	{
		strncpy((char*)pVehicle->Handle, " ", MAX_HANDLE_LEN);
	}
	else
	{
		pVehicle->Chat_Handle = PlayerClient()->Personas().m_callsign;
		strncpy((char*)pVehicle->Handle,chatGetCallsign(pVehicle->Chat_Handle), MAX_HANDLE_LEN);
	}

    if(pVehicle->VehicleType->Type != VEH_TYPE_PERSON)
    {
        vehdecalSetDecals(pVehicle);
    }

#ifdef ANTIHACK
	if ( Online && yeahCode == 0 && !checksumOK )
	{
		yeahCode = OH_YEAH_CHECKS;
	}
#endif

    vehupdateInitVehicle(pVehicle);

	g_canDeployMobileSpawn = false;
	PlayerClient()->SetKilledByAI(false);

#ifdef ANTIHACK
	if ( Online )
	{
		if ( yeahCode )
		{
			wwiiOhYeah(yeahCode);
		}
	}
#endif

#ifndef GM_RELEASE
    // Check to make absolutely sure we aren't spawning as a floater
	if(pVehicle && pVehicle->VehicleType && pVehicle->VehicleType->Type == VEH_TYPE_FLOATER)
	{
  		piExitProgram(_FL, "");
	}
#endif

	if(EZ_MGR)
		EZ_MGR->GetPosition().Load();

	// Load convergence setting if it exists, or get the global
	UINT16 key = pVehicle->VehicleType->WeaponSpec.asUINT16();

	char buff[256] = {'\0'};
	strprintf(buff, "convergence.%d", (SINT32)key);

	// set our vehicle's convergence
	persGunsightSetConvergenceRange((FLOAT32)cfgPrefsGetInt(buff, (SINT32)Convergence));
}

static void _reportEnemyAircraft(vehVehicle* CurrentVehicle)
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	int icons[CHATICON_MAX] = {-1,-1,IDE_ICON_CAUTION};

	FLOAT64 playerdir = PlayerVehicle->PosData.RenderAtt.Yaw;

	FLOAT64VECTOR planepos = CurrentVehicle->PosData.Pos;
	planepos.z = 0.0;

	FLOAT64VECTOR playerpos = PlayerVehicle->PosData.RenderPos;
	playerpos.z = 0.0;

	FLOAT64VECTOR toplane;
	mathSubVectors(&toplane, &planepos, &playerpos);

	FLOAT64 toplanedir = atan2(toplane.x, toplane.y);
	FLOAT64 relangle = toplanedir - playerdir;

	int clockangle = (int)(relangle / PI2 * 12);
	if(clockangle < 1) clockangle = 12+clockangle;

    char buf[64];
    strprintf(buf, "Enemy aircraft spotted at %d o'clock.", clockangle);
	uiChatPrintLineIcon(icons, colorREDALPHA, uiUTF8toUTF16(buf));

    if(g_wantMessageAlert)
        fxSndAddFiniteHeadSound("sounds/ping.wav", IDE_FP_PRIORITY_LOW, 1.0f, 1.0f);
}

bool vehVehicle::isHulked()
{
	// check the flag first
	if(Flags & VEH_FLAGS_HULKED)
		return true;

    if(VehicleObjectData)
    {
        if(obmainValidateObject(VehicleObjectData->VisDamageHulkLOD))
        {
            if(VehicleObjectData->VisDamageHulkLOD->flags & OBJECT_VISIBLE)
            {
                return true;
            }
        }
    }

    return false;
}

double vehVehicle::DistanceTo(double x, double y, double z)
{
	Vec3d start;
	Vec3d end(x, y, z);

	if(ModelData)
		start.set(ModelData->VehiclePos);
	else
		start.set(PosData.Pos);

	Vec3d diff = end - start;

	return diff.length();
}

bool vehVehicle::InRange(double x, double y, double z, double distance)
{
	Vec3d start;
	Vec3d end(x, y, z);

	if(ModelData)
		start.set(ModelData->VehiclePos);
	else
		start.set(PosData.Pos);

	Vec3d diff = end - start;

	return (diff.length2() <= (distance * distance));
}

gr2AnimTrooper* vehVehicle::GetAnim()
{
	return (VehicleObjectData && VehicleObjectData->PlaneObject) ? VehicleObjectData->PlaneObject->animModel : nullptr;
}

bool vehVehicle::IsFiring(SINT32 index) const
{
	if(index >= 0 && index < VEH_MAX_MOUNTS)
		return (ModelData) ? ModelData->MountIsFiring[index]: MountIsFiring[index];

	return false;
}

double vehVehicle::GetYaw() const
{
	double yaw = (ModelData) ? ModelData->VehicleAtt.Yaw: -PosData.Att.Yaw;
	return mlRAD_TO_DEG(yaw);
}

double vehVehicle::GetPos(int axis) const
{
	switch(axis)
	{
		case 1: return (ModelData) ? ModelData->VehiclePos.x: PosData.Pos.x;
		case 2: return (ModelData) ? ModelData->VehiclePos.y: PosData.Pos.y;
		case 3: return (ModelData) ? ModelData->VehiclePos.z: PosData.Pos.z;
	}

	return 0;
}

double vehVehicle::GetTraverse(int turret, bool world) const
{
	double traverse = 0;

	if(turret >= 0 && turret < MAX_TURRETS)
	{
		if(ModelData)
		{
			traverse = ModelData->TurretTraverse[turret];
		}
		else
		{
			traverse = StateAnimation.TurretTraverse[turret];
		}

		traverse *= PI2; // turrets are -1 to 1
	}

	traverse = -mlRAD_TO_DEG(traverse); // we negate to get world oriented yaw

	if(world) // world relative, otherwise vehicle relative
		traverse += GetYaw();

	while (traverse * traverse > 180.0 * 180.0)
	{
		if (traverse < 0)
			traverse += 360.0;
		else if (traverse >= 0)
			traverse -= 360.0;
	}

	return traverse; // -180 to 180 (degrees)
}

double vehVehicle::GetElevation(int turret, bool world) const
{
	double elevation = 0;

	if(turret >= 0 && turret < MAX_TURRETS)
	{
		if(ModelData)
		{
			elevation = ModelData->TurretElevate[turret];
		}
		else
		{
			elevation = StateAnimation.TurretElev[turret];
		}

		elevation *= PI2; // turrets are -1 to 1
	}

	elevation = mlRAD_TO_DEG(elevation);

	return elevation; // -180 to 180 (degrees)
}

void vehVehicle::clear()
{
    for(int i = 0; i < VEH_MAX_MOUNTS; i++)
    {
        MountIsFiring[i] = 0;

        for(int j = 0; j < VEH_MAX_WEAPONS_PER_MOUNT; j++)
        {
            Weapons[i][j].WeaponType = nullptr;
            Weapons[i][j].LastFireTime = 0.0f;
            Weapons[i][j].ReloadStartTime = 0.0f;
            Weapons[i][j].WeaponFireType = 0;
            Weapons[i][j].Displacement = 0;

            memset(Weapons[i][j].BeltSequence, 0, sizeof(Weapons[i][j].BeltSequence));
            memset(Weapons[i][j].BeltName, 0, sizeof(Weapons[i][j].BeltName));

            WeaponIsHidden[i][j] = 0;
        }
    }

    VehicleNum = 0;
    VehicleHostIndex = 0;
    persVehicleIndex = 0;
    VehicleType = nullptr;
    Flags = 0;
    ModelData = nullptr;
    VehicleObjectData = nullptr;
    ChunksOfCrap = nullptr;
    RideSeatNum = 0;
    RideVehicleHostIndex = 0;
	LastRideVehicleHostIndex = 0;
    InfantryTarget = 0;
    RelRideYaw = 0;

    memset(&RelRidePosition, 0, sizeof(FLOAT64VECTOR));
    memset(&WeaponSerial, 0, sizeof(WEAPON_SERIAL));
    memset(&PosData, 0, sizeof(vehVehiclePositionData));
    memset(&DamageQue, 0, sizeof(vehDamageQue));
    memset(&StateAnimation, 0, sizeof(vehanimVehicleStateAnimations));
    memset(&fpsBuildingID, 0, sizeof(REL_OBJ_ID));
    memset(Handle, 0, sizeof(UCHAR) * MAX_HANDLE_SIZE);
    memset(SquadHandle, 0, sizeof(UCHAR) * MAX_HANDLE_SIZE);

    VehicleHasBeenHit = 0;
	VehicleInitTime = 0;
    VehicleDeadTime = 0;
    MinVehicleLOD = 0;
    HideVehicle = 0;
    BumpedOff = 0;
    AnimationBits = 0;
    LoadVehicle = 0;
    LastUpdateType = 0;
    LastUpdateDelta = 0;
    LastUpdate = 0;
    LastHitmapUpdateTime = 0;
    LastFPSWeaponNum = 0;
    LastFireTime = 0;

    LoadoutNum = 0;
    EngineIdleVolume = 0;
    EnginePowerVolume = 0;
    IconTransparency = 0;
    IconRingSize = 0;
    LastVisTransparency = 0;
    Chat_Handle = 0;
    IconColor = 0;
    IconPulse = 0;
    LastVisChangeTime = 0;

    DrawIconHandle = false;
    DrawRangeCircle = false;
    DrawIconId = false;
    DrawIconRank = false;
    DrawIconSquad = false;

    VehicleIsSquaddie = false;
    IsCommander = false;
    IsBuilder = false;
    SameBrigade = false;
    SameMission = false;
    IsMissionLeader = false;
    IsMyMissionLeader = false;
    IsTrainer = false;

    hulksto = false;
    isInside = false;

    persDecal = 0;

	InUse = false;  			// is the vehicle struct in use?
	StickVehicleToGround = false;
 	VisDamageNoStick = false;

    unitDecal = 0;
	Rank = 0;
	Country = 0;
	Side = 0;

    iFXVehicleID = 0;

    deployingTimeout = 0;
    deployedTimeout = 0;
    interiorCheck = 0;

	spaceID = 0;
	collider = nullptr;

    interpolator = vehInterpolator(this);

}

#include "physics/physicsMain.h"

void vehmainCreateODECollider(vehVehicle* veh)
{
	UINT32 cat = COLLIDER_VEHICLE_MASK;

	switch(veh->VehicleType->Type)
	{
		case VEH_TYPE_PERSON:	cat = COLLIDER_VEH_TROOP;		break;
		case VEH_TYPE_AIR:		cat = COLLIDER_VEH_AIR;			break;
		case VEH_TYPE_GROUND:	cat = COLLIDER_VEH_GROUND;	break;
		case VEH_TYPE_SEA:		cat = COLLIDER_VEH_SEA;			break;
	}

	if(veh->spaceID  == 0)
	{
		veh->spaceID = dSimpleSpaceCreate(colliderVehicleSpace());
		colliderSetBits((dGeomID)veh->spaceID, cat, 0);
		dGeomSetData((dGeomID)veh->spaceID, veh);
		dSpaceSetCleanup(veh->spaceID, 0);
	}

	if(veh->VehicleObjectData->CollideInternObject)
		physicsRecurseAndCreateObjectCollider(veh->VehicleObjectData->CollideInternObject, veh->spaceID,cat,0,false,false,false);

	if(veh->VehicleObjectData->CollideExternObject)
		physicsRecurseAndCreateObjectCollider(veh->VehicleObjectData->CollideExternObject, veh->spaceID,cat,0,false,false,false);

	if(veh->VehicleObjectData->CollideWalkObject)
		physicsRecurseAndCreateObjectCollider(veh->VehicleObjectData->CollideWalkObject, veh->spaceID,cat|COLLIDER_WALK,0,false,false,false);

	// setup a ode collider query if we're in an active scenario
	if(veh->VehicleType->Type == VEH_TYPE_GROUND && veh->IsMe() && SCEN_MGR->IsActive() && veh->VehicleObjectData->HitBox)
	{
		veh->collider = new colliderQuery;

		// this collider is used to detect ode collisions
		// currently we're only interested in vehicle blocks
		// while scenarios are running
		if(veh->collider)
		{
			// geoms will be destroyed when the object's collider is destroyed
			dSpaceSetCleanup(veh->collider->space(), 0);

			physicsRecurseAndCreateObjectCollider(veh->VehicleObjectData->HitBox,
												  veh->collider->space(),
												  cat,
												  COLLIDER_BLOCK,
												  false,
												  false,
												  false,
												  veh);
		}
	}
}

StratDB::StratObject* vehmainLastEntered(trnCDTerrainObject** TO)
{
	if(TO) *TO = _lastEnteredTO;
	return _lastEntered;
}

ChatHandle vehLastInfantryDamaged()
{
	return _lastInfantryDamaged;
}

#ifdef VEHICLES_GAME_INFO

static WORLD_UPDATE _lastWorldUpdate;

using UPDATES_PER_VEHICLE = std::map<UINT32 /*vehicleIndex*/, UINT32 /*numUpdates*/ >;
using UPDATES_HISTORY = std::map< UINT32 /* second */, UPDATES_PER_VEHICLE >;
static UPDATES_HISTORY s_updateHistory;

/* On processing an update for a given vehicle ... */
void vehRecordVehicleUpdate(const UINT32 vehicleIndex)
{
    s_updateHistory[teulTime()][vehicleIndex]++;
}

// Return a pointer to an n-byte, null-terminated, char array of characters
// describing the update history for a given vehicle.
// Each position represents one second,
// intoSize_bytes should include the null byte.
const char* vehGetHistoryStringFor(const UINT32 vehicleIndex, char* const into, size_t intoSize_bytes)
{
    // Eliminate expired entries.
    const UINT32 cutoff = teulTime() - intoSize_bytes;

    while ( s_updateHistory.empty() == false && s_updateHistory.begin()->first < cutoff )
    {
        s_updateHistory.erase(s_updateHistory.begin());
    }

    const size_t maxEntries = intoSize_bytes - 1;    // < Tells the compiler nothing will change this.
    into[maxEntries] = 0; // Terminating byte.

    for(size_t i = 0; i < maxEntries; ++i)
    {
        const UINT32 updatesReceived = s_updateHistory[teulTime() - i][vehicleIndex];    // Will set itself to 0 if none received.
        // Populate them backwards.
        char* const entry = &into[i];

        // Display a number 0-9 for updates or '*' for 10+.
        if(updatesReceived <= 0)
            *entry = '.';
        else if(updatesReceived >= 10)
            *entry = '+';
        else
            *entry = '0' + updatesReceived;
    }

    return into;
}

void vehSaveLastWorldUpdate(WORLD_UPDATE& wu)
{
	_lastWorldUpdate = wu;
}

std::string& vehVehiclesGameInfo()
{
	static std::string report;

	report.clear();

	if(_lastWorldUpdate.header.timerInfo)
	{
		const UpdateFraming& currentMode = framings[_lastWorldUpdate.header.framingMode];

		UINT32 numVehiclesDisplayed		= 0;
		const UINT32 maxVehiclesDisplayed		= currentMode.maxVehicles;
		const UINT32 numVehiclesWorldUpdate	= _lastWorldUpdate.header.numVehicles;
		const UINT32 numVehiclesInRange		= _lastWorldUpdate.header.actualVehiclesInRange;

		vehVehicle* cur;

		for (cur = getVehicleHead().getNext(); cur != nullptr; cur = cur->getNext())
		{
			if(cur->ModelData || !cur->InUse || cur->isSTO())
				continue;

			++numVehiclesDisplayed;
		}

		char buffer[256] = {0};

		strprintf(buffer, "Vehicles: %u of %u  Received: %u of %zu  Possible: %u\n",
			numVehiclesDisplayed,
			maxVehiclesDisplayed,
			numVehiclesWorldUpdate,
			currentMode.maxVehiclesPerFrame(),
			numVehiclesInRange);

		report = buffer;
	}

	return report;
}
#endif

#define COPY_VSMEMBER(m) { veh->StateAnimation.m = myVehicle->StateAnimation.m; }
#define COPY_VSARRAY(m, c) for(int i = 0; i < c; ++i){ veh->StateAnimation.m[i] = myVehicle->StateAnimation.m[i];  }

UINT32 vehmainCloneVehicle(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero)
{
	if(!keyState) return 0;
	if(Online) return 0;

	// look up current vehicle and vspawn it
	vehVehicle* myVehicle = persGetPersonaVehiclePtr();
	mlVehicleData* modelData = persGetPersonaModelDataPtr();

	/*
	if(myVehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		gr2AnimTrooper* anim = myVehicle->VehicleObjectData->PlaneObject->animModel;

		if(anim)
		{
			gr2Model* model = anim->skeleton->attachments.at(0);
			//gr2Model* model = anim->weapons[anim->currentWeapon].GetActive();
			//const char* path = weapon->GetPath();
			Matrixf transform;
			transform.set(model->GetBoneTransform(0));
			gr2RigidPhysicsModel* o = new gr2RigidPhysicsModel(model->GetPath(), myVehicle, transform, 0);
			return 0;
		}
	}
	*/

	/*
	if(vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{

	}
	else
	{

	}

   char name[64];
   */

	FLOAT64VECTOR vehpos = {0,0,0};
	if(modelData) vehpos = modelData->VehiclePos;

	for(int q = 0; q < (int)vehtypeCount(); q++)
	{
        vehType const* vehtype = nullptr;

        vehtype = vehtypeGetByIndex(q);

        if(!vehtype)
        {
            continue;
        }

		if(vehtype->WeaponSpec == myVehicle->VehicleType->WeaponSpec)
        {
            double x = vehpos.x;
            double y = vehpos.y;
            double z = vehpos.z;

            WorldPosition_m position(x, y, z);
			AttitudeVector attitude((FLOAT32)myVehicle->ModelData->VehicleAtt.Roll, (FLOAT32)myVehicle->ModelData->VehicleAtt.Pitch, (FLOAT32)myVehicle->ModelData->VehicleAtt.Yaw);

            VehicleSTO* vehicle = new VehicleSTO(0, vehtype, position, attitude);

			// setup our name and decal for personalization reasons
			char handle[11];
			snprintf((char*)handle, 10, "C%d%d", (int)vehicle->uid(), GetRandom(1000, 1000000));
			//veh->Chat_Handle = chatGetHandle((const char*)handle);

			vehicle->setIcon(handle);
#ifdef BETA_RELEASE
			vehicle->setDecals((SINT32)gPersDecalOverride, (SINT32)gUnitDecalOverride);
#endif
            ServerTrackedObject::addToIndex(vehicle);

			// so we can manage them via dot command
            uiDotCommandAddClone((int)vehicle->uid());

			vehUpdate();

			vehVehicle*veh = vehFindVehicle(vehicle->getSTOVehicleID());

            veh->Flags |= VEH_FLAGS_IS_CLONE;

			//veh->StateAnimation = myVehicle->StateAnimation;

			// reset our name so we can easily id which vehicle index we're using
			snprintf((char*)veh->Handle, 10, "C%d", (int)vehicle->uid());
			veh->Chat_Handle = chatGetHandle((const char*)veh->Handle);

			if(myVehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				gr2AnimTrooper* newAnimModel = veh->VehicleObjectData->PlaneObject->animModel;
				gr2AnimTrooper* myAnimModel = myVehicle->VehicleObjectData->PlaneObject->animModel;

				veh->AnimationBits = ((fpPersonData*)myVehicle->ModelData->PersonData)->AnimationFlags;
				//veh->AnimationBits = myAnimModel->currentFlags;
				newAnimModel->twist.vehicleYaw = myAnimModel->twist.vehicleYaw;
				//veh->TPSYaw = myVehicle->interpolator.LastUpdateYaw();
				veh->interpolator.Copy(myVehicle, veh->AnimationBits);


				newAnimModel->currentDownWeight = myAnimModel->currentDownWeight;
				newAnimModel->currentMiddleWeight = myAnimModel->currentMiddleWeight;
				newAnimModel->currentUpWeight = myAnimModel->currentUpWeight;
				//newAnimModel->currentMotion = myAnimModel->currentMotion;

				newAnimModel->twist.SetTwists(newAnimModel->skeleton);
				newAnimModel->twist.Copy(myAnimModel->twist);

				//gr2ModelSetTwists(myAnimModel->skeleton, newAnimModel->skeleton);

				//if(myAnimModel->currentPose == TPS_POSETYPE_PRONE)
				//	gr2ModelSetModelRotation(newAnimModel->skeleton, -90, 100.0f);

				//newAnimModel->movingYaw = myAnimModel->movingYaw;
				//newAnimModel->headYaw = myAnimModel->headYaw;
				//newAnimModel->torsoYaw = myAnimModel->torsoYaw;
				//newAnimModel->legsYaw = myAnimModel->legsYaw;

				if(false && myAnimModel->currentPose == TPS_POSETYPE_PRONE)
				{
					static FLOAT64 addYaw = 90.0f;

					FLOAT64 yaw = myAnimModel->myVehicle->PosData.Att.Yaw;

					yaw = -yaw;
					yaw = mlRAD_TO_DEG(yaw);
					yaw += addYaw;

					gr2NormalizeAngle(yaw);

					yaw = mlDEG_TO_RAD(yaw);
					yaw = -yaw;

					newAnimModel->myVehicle->PosData.Att.Yaw = yaw;


					/*
					FLOAT64 rotation = -90.0f;

					newAnimModel->headYaw += rotation;
					newAnimModel->torsoYaw += rotation;
					newAnimModel->legsYaw += rotation;

					gr2NormalizeAngle(newAnimModel->headYaw);
					gr2NormalizeAngle(newAnimModel->torsoYaw);
					gr2NormalizeAngle(newAnimModel->legsYaw);
					*/

					/*
					piMATRIX rotation;
					piMATRIX* t = &newAnimModel->myVehicle->VehicleObjectData->PlaneObject->mat;

					generateRotationMatrix (AROUND_Z, 90.0f, &rotation);
					math3dMatrixMultiply(t, &rotation, t);
					*/
				}

				/*
				newAnimModel->currentMotion = myAnimModel->currentMotion;
				newAnimModel->currentMove = myAnimModel->currentMove;
				newAnimModel->currentPoint = myAnimModel->currentPoint;
				newAnimModel->currentPoint = myAnimModel->currentPoint;
				*/

				veh->PosData.Att = myVehicle->ModelData->VehicleAtt;
				veh->PosData.RenderAtt = myVehicle->ModelData->VehicleAtt;
				veh->VehicleObjectData->PlaneObject->mat = myVehicle->VehicleObjectData->PlaneObject->mat;

				--newAnimModel->lastUpdateTime;

				// this is so each clone get's a different face
				//snprintf((char*)veh->Handle, 10, "C%d%d", (int)vehicle->uid(), GetRandom(1000, 1000000));
				//newAnimModel->model[0]->currentFaceID = -1;
				//newAnimModel->model[1]->currentFaceID = -1;
				//newAnimModel->model[2]->currentFaceID = -1;

				fpProcessVehicleAnimationFlags(veh);
				//gr2AnimPersonalize(veh->VehicleObjectData->PlaneObject, (char*)veh->Handle);
				gr2AnimUpdateModel(veh->VehicleObjectData->PlaneObject, FALSE);
				gr2UpdateModel(newAnimModel->skeleton, true);

				++newAnimModel->lastUpdateTime;
			}
			else
			{
				// do something for vehicles
				veh->AnimationBits = myVehicle->AnimationBits;
				veh->PosData = myVehicle->PosData;
				veh->PosData.AttVel.Pitch = veh->PosData.AttVel.Roll = veh->PosData.AttVel.Yaw = 0;
				veh->PosData.AttVelSum.Pitch = veh->PosData.AttVelSum.Roll = veh->PosData.AttVelSum.Yaw = 0;
				veh->PosData.LastUpdateAcc.x = veh->PosData.LastUpdateAcc.y = veh->PosData.LastUpdateAcc.z = 0;
				veh->PosData.Acc.x = veh->PosData.Acc.y = veh->PosData.Acc.z = 0;
				veh->PosData.LastUpdateAttVel.Pitch = veh->PosData.LastUpdateAttVel.Roll = veh->PosData.LastUpdateAttVel.Yaw = 0;
				veh->PosData.Vel.x = veh->PosData.Vel.y = veh->PosData.Vel.z = 0;

				veh->VehicleObjectData->PlaneObject->mat = myVehicle->VehicleObjectData->PlaneObject->mat;
				veh->VehicleObjectData->PlaneObject->pos = myVehicle->VehicleObjectData->PlaneObject->pos;

				// setup state animations
				COPY_VSMEMBER(LastUpdateTime);
				COPY_VSMEMBER(GearSetting);
				COPY_VSMEMBER(FlapSetting);
				COPY_VSARRAY(CanopySetting,ML_MAX_CREW_POSITIONS);
				COPY_VSARRAY(HatchSetting,ML_MAX_CREW_POSITIONS);

				COPY_VSMEMBER(BombBayDoorSetting);
				COPY_VSMEMBER(DiveBrakeSetting);

				COPY_VSMEMBER(GearMoveTime);
				COPY_VSMEMBER(FlapMoveTime);

				COPY_VSARRAY(CanopyMoveTime,ML_MAX_CREW_POSITIONS);
				COPY_VSARRAY(HatchMoveTime,ML_MAX_CREW_POSITIONS);

				COPY_VSMEMBER(BombBayDoorMoveTime);
				COPY_VSMEMBER(DiveBrakeMoveTime);

				COPY_VSARRAY(PropIsDiscSetting,ML_MAX_ENGINES);
				//COPY_VSARRAY(PropIsDisc,ML_MAX_ENGINES);

				COPY_VSARRAY(RecoilStartTime,VEH_MAX_MOUNTS);
				COPY_VSARRAY(RecoilPosition,VEH_MAX_MOUNTS);
				COPY_VSARRAY(RecoilTime,VEH_MAX_MOUNTS);

				//COPY_VSMEMBER(GearPos);
				//COPY_VSMEMBER(FlapPos);

				//COPY_VSARRAY(CanopyPos,ML_MAX_CREW_POSITIONS);
				//COPY_VSARRAY(HatchPos,ML_MAX_CREW_POSITIONS);

				//COPY_VSMEMBER(BombBayDoorPos);
				//COPY_VSMEMBER(DiveBrakePos);

				//COPY_VSARRAY(CanopyDOFVar,ML_MAX_CREW_POSITIONS);
				//COPY_VSARRAY(HatchDOFVar,ML_MAX_CREW_POSITIONS);

				//COPY_VSMEMBER(GearDOFVar);
				//COPY_VSMEMBER(FlapDOFVar);
				//COPY_VSMEMBER(BombBayDoorDOFVar);
				//COPY_VSMEMBER(DiveBrakeDOFVar);

				//COPY_VSARRAY(PropDiscSpinRpm,ML_MAX_ENGINES);
				//COPY_VSARRAY(PropSpinPosDOF,ML_MAX_ENGINES);

				COPY_VSMEMBER(UseCalculatedTrackAndWheel);
				COPY_VSARRAY(TrackPos,2);
				COPY_VSARRAY(WheelPos,ML_MAX_GEARNUM);
				COPY_VSMEMBER(VehicleSpeed);

				COPY_VSMEMBER(AileronDeflection);
				COPY_VSMEMBER(ElevatorDeflection);
				COPY_VSMEMBER(RudderDeflection);
				COPY_VSMEMBER(LastAileronDeflection);
				COPY_VSMEMBER(LastElevatorDeflection);
				COPY_VSMEMBER(LastRudderDeflection);

				COPY_VSMEMBER(LastAnimationVehicleSpeed);
				COPY_VSMEMBER(LastTankFireDeltaTime);
				COPY_VSMEMBER(LastMainGunSmokeLevel);
				COPY_VSMEMBER(MainGunSmokeLevel);
				COPY_VSMEMBER(CockpitSmokeLevel);

				/*
				uiChatPrintLine("Adding STO vehicle at %.2f,%.2f,%.2f from my vehicle at %.2f,%.2f,%.2f.",
					veh->PosData.Pos.x,
					veh->PosData.Pos.y,
					veh->PosData.Pos.z,
					myVehicle->PosData.Pos.x,
					myVehicle->PosData.Pos.y,
					myVehicle->PosData.Pos.z);
				*/
			}

			for(int i = 0; i < 16; i++)
			{
				veh->StateAnimation.TurretTraverse[i] = (float)myVehicle->ModelData->TurretTraverse[i];
				veh->StateAnimation.TurretElev[i] = (float)myVehicle->ModelData->TurretElevate[i];
				veh->StateAnimation.LastTurretElev[i] = (float)myVehicle->ModelData->TurretElevate[i];
				veh->StateAnimation.LastTurretTraverse[i] = (float)myVehicle->ModelData->TurretTraverse[i];
				veh->StateAnimation.LastTravUpdateTime[i] = frametime;
				veh->StateAnimation.LastElevUpdateTime[i] = frametime;
			}

			for(int i = 0; i < VEH_MAX_MOUNTS; i++)
			{
				veh->MountIsFiring[i] = (LOGICAL)(myVehicle->ModelData->PrimaryTriggerDown || myVehicle->ModelData->MountIsFiring[i]);
			}

			veh->StateAnimation.LastUpdateTime = frametime;
			veh->LastUpdate = frametime;
			veh->LastFireTime = myVehicle->LastFireTime;
			veh->PosData.LastUpdateTime = frametime;
			veh->StickVehicleToGround = false;
			veh->RideVehicleHostIndex = 0;

            break;
        }
    }

    return 0;
}

// set vehicle states for vehicles that were cached
static void _setVehicleState(vehVehicle* v, bool start)
{
	bool isTrooper = (v->VehicleType->Type == VEH_TYPE_PERSON);

	if(start) // only do this work if we're pushing the vehicle back out for use
	{
		if(isTrooper && v->VehicleObjectData->PlaneObject->animModel)
			gr2AnimReset(v->VehicleObjectData->PlaneObject->animModel);

		// reset the interpolator
		v->interpolator = vehInterpolator(v);

		// flag it so we can bypass other inits on down the line if need be
		// for example, don't re-add tranform effects
		v->Flags |= VEH_FLAGS_CACHED;
	}
	else // do this work when you are pushing the vehicle onto the cache
	{
		// kill all visible or audible states
		for (int i=0;i<VEH_MAX_STATE_BITS;i++)
		{
			switch(v->VehicleType->StateType[i].StateType)
			{
				case VehAnim::STATE_TYPE_SMOKE:
				case VehAnim::STATE_TYPE_SOUND:
				case VehAnim::STATE_TYPE_EFFECT:
					vehAnimChangeVehicleStateByNumber(v, i, FALSE);
				break;
			}
		}

		// reset vehicle generic stuff
		v->RideSeatNum = 0; // special values VEH_HOIST_RIDE_SEAT, VEH_TROOPER_ON_DECK_RIDE_SEAT, VEH_TOWED_VEHICLE
		v->InfantryTarget = 0;
		v->VehicleHasBeenHit = 0;
		v->VehicleDeadTime = 0;
		v->AnimationBits = 0;

		memset(&v->DamageQue, 0x0, sizeof(vehDamageQue));
		memset(&v->MountIsFiring, 0x0, sizeof(v->MountIsFiring));
		memset(&v->WeaponIsHidden, 0x0, sizeof(v->WeaponIsHidden));

		v->StateAnimation.FirstUpdateProcessed = false;
		v->StateAnimation.bVehicleDestroyed = false;
		v->StateAnimation.TankOnFire = FALSE;
		v->StateAnimation.PlayCrash = 0;
		v->StateAnimation.CrashHasPlayed = 0;
		v->StateAnimation.EjectRiders = 0;
		v->StateAnimation.MainGunLastUpdateTime = 0;

		//if(v->StateAnimation.enumDeployedState != IDE_DEPLOYED_NOT_USED)
		//	v->StateAnimation.enumDeployedState = IDE_VEHICLE_UNDEPLOYED;

		vehAnimUpdateVehicleStateAnimations(v);
	}

	/*
	// only vehicles (not troopers) on the same sortie get vcached, so don't bother
	// doing any of this since all of the states will remain for the current sortie
	if(!isTrooper)
		return;

	// reset vehicle generic stuff
	v->Flags = 0;
	v->RideSeatNum = 0; // special values VEH_HOIST_RIDE_SEAT, VEH_TROOPER_ON_DECK_RIDE_SEAT, VEH_TOWED_VEHICLE
	v->InfantryTarget = 0;
	v->VehicleHasBeenHit = 0;
	v->VehicleDeadTime = 0;
	v->AnimationBits = 0;

	if(start)
	{
		vehfxAttachVehicle(v, &v->iFXVehicleID);
		v->interpolator = vehInterpolator(v);
	}
	else
	{
		vehfxDetachVehicle(v->iFXVehicleID);
		v->iFXVehicleID = 0;

		memset(&v->DamageQue, 0x0, sizeof(vehDamageQue));
		memset(&v->MountIsFiring, 0x0, sizeof(v->MountIsFiring));
		memset(&v->WeaponIsHidden, 0x0, sizeof(v->WeaponIsHidden));

		// reset animation states
		memset(&v->StateAnimation.FuelFireOn, 0, sizeof(v->StateAnimation.FuelFireOn));
		memset(&v->StateAnimation.WingtipVortexSmokeOn, 0, sizeof(v->StateAnimation.WingtipVortexSmokeOn));
		memset(&v->StateAnimation.GlycolLeakSmokeOn, 0, sizeof(v->StateAnimation.GlycolLeakSmokeOn));
		memset(&v->StateAnimation.OilLeakSmokeOn, 0, sizeof(v->StateAnimation.OilLeakSmokeOn));
		memset(&v->StateAnimation.ExhaustSmokeOn, 0, sizeof(v->StateAnimation.ExhaustSmokeOn));
		memset(&v->StateAnimation.FuelLeakSmokeOn, 0, sizeof(v->StateAnimation.FuelLeakSmokeOn));
		memset(&v->StateAnimation.EngineFireOn, 0, sizeof(v->StateAnimation.EngineFireOn));

		int num = vehAnimFindVehicleStateNumber(v, "tankfire");
		if(num >= 0) v->PosData.States[num] = 0;

		memset(&v->StateAnimation.TreadDustOn, 0, sizeof(v->StateAnimation.TreadDustOn));
		memset(&v->StateAnimation.DebrisOn, 0, sizeof(v->StateAnimation.DebrisOn));

		// set all vis damage states
		for (int i=0;i<VEH_MAX_STATE_BITS;i++)
			vehAnimChangeVehicleStateByNumber(v, i, FALSE);

		v->StateAnimation.FirstUpdateProcessed = true;
		v->StateAnimation.bVehicleDestroyed = false;
		v->StateAnimation.TankOnFire = FALSE;
		v->StateAnimation.PlayCrash = 0;
		v->StateAnimation.CrashHasPlayed = 0;
		v->StateAnimation.EjectRiders = 0;
		v->StateAnimation.MainGunLastUpdateTime = 0;

		if(v->StateAnimation.enumDeployedState != IDE_DEPLOYED_NOT_USED)
			v->StateAnimation.enumDeployedState = IDE_VEHICLE_UNDEPLOYED;

		vehAnimUpdateVehicleStateAnimations(v);

		if(v->VehicleType->Type != VEH_TYPE_PERSON)
		{
			// temp variable
			OBJECT* Object = nullptr;

			// remove any crap
			if(v->ChunksOfCrap)
				vehVisDamageRemoveAllCrap(v);

			// clear all missing flags
			obmainClearFlag2InThisTree(v->VehicleObjectData->PlaneObject, OBJECT_MISSING);

			// cleaned up damaged objects
			obmainClearFlagsInThisTree(v->VehicleObjectData->VisDamageStubGroup, OBJECT_VISIBLE);
			obmainSetFlag2InThisTree(v->VehicleObjectData->VisDamageStubGroup, OBJECT_HIDDEN);

			// fix up hitmap
			obmainClearFlag2InThisTree(v->VehicleObjectData->HitMap, OBJECT_DONT_UPDATE|OBJECT_HIDDEN);
			//obmainClearFlag2InThisTree(v->VehicleObjectData->HitMap, OBJECT_HIDDEN);

			// set dont update for inside and outside
			//obmainSetFlag2InThisTree(v->VehicleObjectData->HitMap, OBJECT_DONT_UPDATE);

			Object = nullptr;
			ogrpFindSubObjectByName(&Object,v->VehicleObjectData->PlaneObject,"hulk");
			if(Object) Object->flag2 |= OBJECT_DONT_UPDATE;

			if(v->VehicleObjectData->VisDamageCrapGroup)
				v->VehicleObjectData->VisDamageCrapGroup->flag2 |= OBJECT_DONT_UPDATE;

			// if we're hulked, do a few more things, fix up colliders
			if(v->isHulked())
			{
				if(v->VehicleObjectData->LoadoutLODObject)
					ogrpHideObjectAndSubtree(v->VehicleObjectData->LoadoutLODObject);

				// disable ode colliders
				//physicsRecurseAndToggleObjectCollider(v->VehicleObjectData->VisDamageHulkLOD, false);

				// update hulk flags
				obmainClearFlag2InThisTree(v->VehicleObjectData->VisDamageHulkLOD, OBJECT_DONT_UPDATE);
				obmainClearFlagsInThisTree(v->VehicleObjectData->VisDamageHulkLOD, OBJECT_VISIBLE);

				// update crap flags
				obmainClearFlag2InThisTree(v->VehicleObjectData->VisDamageCrapGroup, OBJECT_DONT_UPDATE|OBJECT_HIDDEN);
				obmainClearFlagsInThisTree(v->VehicleObjectData->VisDamageCrapGroup, OBJECT_VISIBLE);
				//obmainClearFlag2InThisTree(v->VehicleObjectData->VisDamageCrapGroup, OBJECT_HIDDEN);

				// update stub flags
				obmainClearFlag2InThisTree(v->VehicleObjectData->VisDamageStubGroup, OBJECT_DONT_UPDATE);

				if (v->VehicleObjectData->HitMap)
				{
					//physicsRecurseAndToggleObjectCollider(v->VehicleObjectData->HitMap, true);

					Object = nullptr;
					ogrpFindSubObjectByName(&Object,v->VehicleObjectData->HitMap,"inside");
					v->VehicleObjectData->CollideInternObject = Object;

					Object = nullptr;
					ogrpFindSubObjectByName(&Object,v->VehicleObjectData->HitMap,"outside");
					v->VehicleObjectData->CollideExternObject = Object;

					Object = nullptr;
					ogrpFindSubObjectByName(&Object,v->VehicleObjectData->HitMap,"walk");
					v->VehicleObjectData->CollideWalkObject = Object;

					v->VehicleObjectData->Radius = 0;

					mlCalculateObjectTreeRadius(&(v->VehicleObjectData->Radius),v->VehicleObjectData->HitMap,0);
					v->VehicleObjectData->Radius = (FLOAT32)(sqrt(v->VehicleObjectData->Radius) + 1.0); // margin of error
				}
				else
				{
					v->VehicleObjectData->CollideInternObject = nullptr;
					v->VehicleObjectData->CollideExternObject = nullptr;
					v->VehicleObjectData->Radius = 0.0;

					if (v->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
						v->VehicleObjectData->Radius = 3.0;
				}
			}

			if(v->VehicleObjectData->HitMap)
			{
				Object = nullptr;
				ogrpFindSubObjectByName(&Object,v->VehicleObjectData->HitMap,"inside");
				if(Object) Object->flag2 |= OBJECT_DONT_UPDATE;

				Object = nullptr;
				ogrpFindSubObjectByName(&Object,v->VehicleObjectData->HitMap,"outside");
				if(Object) Object->flag2 |= OBJECT_DONT_UPDATE;
			}

			// now fixup lods
			for(int i = 0; i < ML_MAX_LODS; ++i)
			{
				if(v->VehicleObjectData->LODObjects[i])
				{
					OBJECT* lod = v->VehicleObjectData->LODObjects[i];

					//if(i == 0)
					//	obmainClearFlagsInThisTree(lod, OBJECT_SKIPDRAWING);

					// start updating our objects again
					obmainClearFlag2InThisTree(lod, OBJECT_DONT_UPDATE|OBJECT_HIDDEN);
					//obmainClearFlag2InThisTree(lod, OBJECT_HIDDEN);

					// for anything that is an ob, set vis
					obmainFindCallback(lod, _resetObject, nullptr);
				}
			}
		}
	}
	*/
}

bool _findRTBFRU(ServerTrackedObject& sto, void* cookie)
{
	vehVehicle* player = persGetPersonaVehiclePtr();

	if(sto.type() == ServerTrackedObject::_UMS && wwiiSide(sto.flags()) == player->Side)
	{
		const WorldPosition_m& pos = sto.position();

		Vec3d sPos = Vec3d(pos.x(), pos.y(), pos.z());
		Vec3d pPos = Vec3d(player->ModelData->VehiclePos);

		Vec3d diff = sPos - pPos;

		if(diff.length2() <= (20.0f * 20.0f))
			return true;
	}

	return false;
}
