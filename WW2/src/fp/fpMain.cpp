#include "syscfg.h" // Path is project specific.
// main first person module

/*
	 First Person System Core
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Jonathan Miller
*/

#include "mem/mem.h"

#include "gr2/gr2Anim.h"

#include "pers/persmain.h"
#include "ml/mltypes.h"
#include <math.h>
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/Matrixd.h"
#include "include/vars.h"
#include "trn/trnmain.h"
#include "ml/mlutility.h"
#include "draw/draw3d.h"
#include "weap/weapons.h"
#include "fp/fpTypes.h"
#include "fp/fpMain.h"
#include "veh/vehmain.h"
#include "fx/FXInterface.h"
#include "fio/fio.h"
#include "pers/persgunsight.h"
#include "fx/fxAmb.h"
#include "tut/TutorialManager.h"
#include "trn/trncd.h"
#include "ui/uiChat.h"
#include "ui/uiMain.h"
#include "cfg/cfgPrefs.h"
#include "trn/trnvars.h"
#include "scen/ScenarioManager.h"

#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "xml/XMLLoader2.h"

#define	TROOPER_PRONE_SHIFT_BACK_DISTANCE		0.55			// meters
#define	Z_ADJUST_FILE "vehicles\\ground\\groundman\\zadjust.xml"

/*
 TODO - what should be done with this??

// z-physics hackery data
static piVECTOR _CageEdgesStand[7] = {
{-0.125f,	 0.0f,	0.0f},
{-0.0625f,	 0.075f,	0.0f},
{ 0.0625f,	 0.075f,	0.0f},
{ 0.125f,	 0.0f,	0.0f},
{ 0.0625f,	-0.075f,	0.0f},
{-0.0625f,	-0.075f,	0.0f},
{0.0f, 0.0f, 0.0f}
};

static piVECTOR	_WaistOuterRing[6] = {
{-0.50f,	 0.0f,		0.99f},
{-0.25f,	 0.433f,	0.99f},
{ 0.25f,	 0.433f,	0.99f},
{ 0.50f,	 0.0f,		0.99f},
{ 0.25f,	-0.433f,	0.99f},
{-0.25f,	-0.433f,	0.99f}
};

static piVECTOR	_WaistInnerRing[6] = {
{ 0.0f,	 0.0f,	0.5f},
{ 0.0f,	 0.0f,	0.5f},
{ 0.0f,	 0.0f,	0.5f},
{ 0.0f,	 0.0f,	0.5f},
{ 0.0f,	 0.0f,	0.5f},
{ 0.0f,	 0.0f,	0.5f}
};

static piVECTOR	_CageEdgesProne[10] = {
{-0.50f,	 0.0f,		0.0f},
{-0.25f,	 0.433f,	0.0f},
{ 0.25f,	 0.433f,	0.0f},
{ 0.50f,	 0.0f,		0.2f},
{ 0.25f,	-0.433f,	0.2f},
{-0.25f,	-0.433f,	0.2f},
{0.0f, 0.0f, 0.0f},
{0.0f, 0.1f, 0.0f},
{0.0f, 0.2f, 0.0f},
{0.0f, 0.3f, 0.0f}
};
 */

#define fpFALL_DISTANCE_NORMAL_STUN_MIN	4
#define fpFALL_DISTANCE_NORMAL_STUN_MAX	16

#define fpFALL_DISTANCE_NORMAL_HURT_MIN	8
#define fpFALL_DISTANCE_NORMAL_HURT_MAX	16

#define fpFALL_DISTANCE_WATER_STUN_MIN	8
#define fpFALL_DISTANCE_WATER_STUN_MAX	20

#define fpFALL_DISTANCE_WATER_HURT_MIN	12
#define fpFALL_DISTANCE_WATER_HURT_MAX	20

using FPAdjustKey = std::pair<int, int>;
using FPAdjustments = std::map<FPAdjustKey, float>;

static FPAdjustments _fpsAdjustments;
static FPAdjustments _tpsAdjustments;

class fpAdjustLoader : public XMLLoader2<fpAdjustLoader>
{
public:

	fpAdjustLoader(const char* path);

	// tag handlers
	XML_TAG_HANDLER(DoTPS);
	XML_TAG_HANDLER(DoFPS);

protected:

	void addAdjustment(FPAdjustments& a, xmlNode* node);
	int getIndex(const char* type, const char* name);
};

fpAdjustLoader::fpAdjustLoader(const char* path) : XMLLoader2<fpAdjustLoader>(path)
{
	AddHandler("tps", &fpAdjustLoader::DoTPS, this);
	AddHandler("fps", &fpAdjustLoader::DoFPS, this);
}

bool fpAdjustLoader::DoTPS(xmlNode* node, bool first, void* userData)
{
	if(first) addAdjustment(_tpsAdjustments, node);
	return false;
}

bool fpAdjustLoader::DoFPS(xmlNode* node, bool first, void* userData)
{
	if(first) addAdjustment(_fpsAdjustments, node);
	return false;
}

int fpAdjustLoader::getIndex(const char* type, const char* name)
{
	// -1 means default
	// 0 and greater are real pose and move types
	// less than -1 are artificial stand ins
	if(!stricmp("pose", type) && !stricmp("dead", name))
	{
		return -2;
	}
	else if(!stricmp("pose", type) && !stricmp("emote", name))
	{
		return -3;
	}
	else
	{
		std::string key = "tps_";

		key += type;
		key += "type_";
		key += name;

		return fpTPSGetTypeIndexFromValue(type, key.c_str());
	}
}

void fpAdjustLoader::addAdjustment(FPAdjustments& a, xmlNode* node)
{
	int poseNo			= getIndex("pose", xmlGetStringFromNode(node, "pose"));
	int moveNo			= getIndex("move", xmlGetStringFromNode(node, "move"));
	float adjust		= 0.0f;

	xmlGetNodeValue(node, XML_DATA_FLOAT, &adjust, "adjust");

	if(adjust)
		a[std::make_pair(poseNo, moveNo)] = adjust;
}

LOGICAL UpdatePositionRotation(mlVehicleData* pVehicleData, FLOAT64 DeltaTime, FLOAT64VECTOR* pCurrentVelocity);

static Vec3d _vehicleCollision(vehVehicle* me, vehVehicle* cv);
static void ApplyFallEffects(OBJECT* pObject, fpPersonData* pPersonData, FLOAT64 Velocity);
FLOAT64 DetermineFallEffect(FLOAT64 Velocity, FLOAT64 MinDistance, FLOAT64 MaxDistance);
/*
LOGICAL fpCheckFallRing (mlVehicleData* pVehicleData, SINT32 numLines,// piVECTOR* testArray, FLOAT32 rayLength,
						 piVECTOR* startPt, piVECTOR* endPt,
						 OBJECT** ColObjPtr, piVECTOR* posPtr, vehVehicle**vehicleWalkPtr);
*/

LOGICAL fpInit(vehVehicle*vehicle)
{
mlVehicleData*	VehicleData;
fpPersonData*	PersonData;
vehVehicle*		Vehicle;
int				JMMFileHandle;
char			JMMFileName[256], JMMString[64], AmmoLine[50];
int				MountNumber = 0;
int				expendable;

	VehicleData = vehicle->ModelData;

	VehicleData->PersonData = (fpPersonData*)memCalloc(1, sizeof(fpPersonData));
	if (VehicleData->PersonData == nullptr)
	{
		return FALSE;
	}

	PersonData = (fpPersonData*)VehicleData->PersonData;

	if (VehicleData != nullptr)
	{
		Vehicle = (vehVehicle*)VehicleData->VehicleMasterPtr;
		if (Vehicle != nullptr)
		{
			if (Vehicle->VehicleType != nullptr)
			{
				strcpy(JMMFileName, "vehicles/");
				strcat(JMMFileName, (char*)Vehicle->VehicleType->ModelDirectory);
				strcat(JMMFileName, "/");
				strcat(JMMFileName, (char*)Vehicle->VehicleType->Shape);
				strcat(JMMFileName, ".jmm");

				JMMFileHandle = fioOpen(JMMFileName);

				if (JMMFileHandle != -1)
				{
					while (!fioEOF(JMMFileHandle) && (MountNumber < 16))
					{
						weapInitializeWeapon(&Vehicle->Weapons[MountNumber][0],
								Vehicle->VehicleType->Loadout[0].WeaponTypes[MountNumber][0]);

						fioGetString(JMMFileHandle, JMMString, 64);

						// skip remarks
						if(JMMString[0] == '#')
							continue;

						if(sscanf(JMMString, "%s %d", AmmoLine, &expendable) == 2)
						{
							char* ammo = strtok(AmmoLine, ",");
							int ammoIndex = 0;

							while(ammo != nullptr && ammoIndex < 4)
							{
								int ammoAmount = atoi(ammo);
								Vehicle->Weapons[MountNumber][0].NumRoundsAmmoLeft[ammoIndex] = ammoAmount;
								Vehicle->Weapons[MountNumber][0].NumTotalRounds[ammoIndex] = (ammoAmount >= 0) ? ammoAmount: 0;
								ammo = strtok(nullptr, ",");
								++ammoIndex;
							}

							PersonData->Expendable[MountNumber] = expendable;
						}

						MountNumber++;
					}

					fioClose(JMMFileHandle);
				}
				else
				{
					WriteLog(LOG_FPS, LOG_INFO, "ERROR: Unable to find JMM file: %s", JMMFileName);
				}
			}
		}
	}

	if (!fpFPSLoadModels(vehicle) )
	{
		// bad bad bad
		WriteLog (LOG_FPS, LOG_INFO, "**************FPS Load Models failed!");
		WriteLog (LOG_FPS, LOG_INFO, "**************FPS Load Models failed!");
		WriteLog (LOG_FPS, LOG_INFO, "**************FPS Load Models failed!");
		WriteLog (LOG_FPS, LOG_INFO, "**************FPS Load Models failed!");
		WriteLog (LOG_FPS, LOG_INFO, "**************FPS Load Models failed!");
		WriteLog (LOG_FPS, LOG_INFO, "**************FPS Load Models failed!");
		return FALSE;
	}

	return TRUE;
}

void fpKill(mlVehicleData& VehicleData)	///TODO: Convert to destructor?
{
	if(VehicleData.PersonData != nullptr)
	{
		memFree(VehicleData.PersonData);
		VehicleData.PersonData = nullptr;
	}

	fpFPSKill();
}

static FLOAT64 _doGravityScenario(mlVehicleData* vd, vehVehicle** walkOn, FLOAT64VECTOR& LastPos, FLOAT64 DeltaTime)
{
	static bool fallShortSoundPlayed, fallFarSoundPlayed;

	// Initialize
	fpPersonData* pd = (fpPersonData*)vd->PersonData;

	FLOAT64 Velocity = 0.0;

	// calculated in fpFPS.cpp > fpFPSAreaChecks()
	FPSColliderData& collider = fpFPSGetCollider();

	// it hasn't been initialized yet
	if(mathExactVectorLength2(&collider.groundHit) == 0)
		return 0.0f;

	// for sound effects
	if (collider.objectHit)
	{
		fpFPSSetStandMaterial(collider.objectHit->matid);

		// set the condition to true to allow any vehicle surface to be used (troops standing on tanks and truck beds
		// as well as planes...anything); extra code is needed to make troops fall off moving planes or disallow
		// deploying on ground vehicles or whatever...a later patch perhaps?
		UINT32 walkMask = (false) ? COLLIDER_DRIVABLE_MASK: COLLIDER_WALK;

		// use the vehicle CollideWalkObject only
		bool walkable = (collider.objectHit->collider) ? colliderIsType(collider.objectHit->collider->getCategoryMask(), walkMask): false;

		if(walkable)
		{
			*walkOn = colliderSpaceData<vehVehicle>((dGeomID)collider.objectHit->collider->getSpace());

			if(!*walkOn || !vehVehicleInList(*walkOn))
				*walkOn = nullptr;
		}
	}

	FLOAT64 agl = vd->VehiclePos.z - collider.groundHit.z;
	FLOAT64 fagl = fabs(agl);

	const FLOAT64 fallThreshold = .15f;
	const FLOAT64 groundThreshold = .05f;

	/*
	if(agl < 0.0f && fagl > groundThreshold)
	{
		// we're too far below the ground, adjust us
		vd->VehiclePos.z = collider.groundHit.z;
		agl = 0.0f;
		fagl = 0.0f;
	}
	*/

	if(fpsTrooper.lastUnrideTime && (frametime - fpsTrooper.lastUnrideTime) <= 1000 && fagl < 2.0f)
		fpsTrooper.teleportToGround = TRUE;

	if(agl > groundThreshold && !fpsTrooper.teleportToGround)
	{
		// we need to fall
		pd->heightAboveGround = agl;

		// at this point, we're falling, so figure out what to do about it
		if(agl > fallThreshold)
			fxAmbDisable();

		if(pd->StartFallTime10ms == 0)
		{
			pd->StartFallTime10ms = ft10mscount;
			pd->StartFallHeight = vd->VehiclePos.z;

			if(agl > fallThreshold)
			{
				pd->Flags |= fpFLAGS_FALLING;
				fallShortSoundPlayed = fallFarSoundPlayed = false;
			}
		}

		if(ft10mscount - pd->StartFallTime10ms > 10) // i.e. 10 * 10ms == 0.1 second
			Velocity = 9.8 * (ft10mscount - pd->StartFallTime10ms - 10) / 100.0;

		if(fpsTrooper.parachuteOpen)
		{
			Velocity = fpFPSGetParachuteFallSpeed();
		}
		else if(fpsTrooper.parachuteWorn && fpsTrooper.parachuteOKToOpen && pd->heightAboveGround > 25.0f)
		{
			if(fpFPSIsTrooperFalling())
				fpFPSetHeadFixed(TRUE);

			if((ft10mscount - pd->StartFallTime10ms) > (fpsTrooper.parachuteDeployTime * 100.0))
			{
				fpFPSParachuteOpened();
			}
		}
		else if(((ft10mscount - pd->StartFallTime10ms) > (fpsTrooper.fallShortTime * 100.0)) && (!fallShortSoundPlayed))
		{
			if(agl > fallThreshold)
			{
				fpFPSPlaySound(fpsTrooper.fallShortSound);
				fallShortSoundPlayed = true;
			}
		}
		else if(((ft10mscount - pd->StartFallTime10ms) > (fpsTrooper.fallFarTime * 100.0)) && (!fallFarSoundPlayed))
		{
			if(agl > fallThreshold)
			{
				fpFPSPlaySound(fpsTrooper.fallFarSound);
				fallFarSoundPlayed = true;
			}
		}

		// WriteLog (LOG_FPS, LOG_INFO, "Falling at %4.2f m/s", Velocity);

		// adjust our z down
		vd->VehiclePos.z -= (Velocity * (ft10mscount - pd->LastFallTime10ms) / 100.0f);

		pd->LastFallTime10ms = ft10mscount;

		// add some lag
		if(agl > fallThreshold)
			fpFPSAddWeaponLag(AROUND_X, -3.0f);
	}
	else
	{
		pd->heightAboveGround = 0.0f;

		if(pd->Flags & fpFLAGS_FALLING)
		{
			fxAmbEnable();

#ifdef BETA_RELEASE
			if(!SuperTroop)
				ApplyFallEffects(vd->VehicleObjectData->PlaneObject, pd, pd->StartFallHeight - vd->VehiclePos.z);
#else
			ApplyFallEffects(vd->VehicleObjectData->PlaneObject, pd, pd->StartFallHeight - vd->VehiclePos.z);
#endif // BETA_RELEASE

			pd->StartFallTime10ms = 0;
			pd->Flags &= ~fpFLAGS_FALLING;

			if((fpsTrooper.parachuteOpen) || (fpsTrooper.parachuteOKToOpen))
				fpFPSParachuteLanded();
		}

		pd->StartFallTime10ms = 0;

		if(agl < -groundThreshold || fpsTrooper.teleportToGround)
		{
			pd->heightAboveGround = 0.0f;
			vd->VehiclePos.z = collider.groundHit.z;
			Velocity = 0.0f;
			agl = 0.0f;
			fagl = 0.0f;
		}
	}

/*
	// put an "x" at collision point
#ifdef VEHICLE_OBJECT_ANALYSIS
	{
		piVECTOR V1,V2,V3;

		// outline collide triangle
		V1 = ColObj->RenderPositions()[ColObj->ptris[CollisionList[0].PolyNum].v1];
		V2 = ColObj->RenderPositions()[ColObj->ptris[CollisionList[0].PolyNum].v2];
		V3 = ColObj->RenderPositions()[ColObj->ptris[CollisionList[0].PolyNum].v3];

		math3dVectorMultiply(&V1,&V1,&ColObj->mat);
		math3dVectorMultiply(&V2,&V2,&ColObj->mat);
		math3dVectorMultiply(&V3,&V3,&ColObj->mat);

		draw3dLine((piVECTOR*)&V1, (piVECTOR*)&V2, colorGREENALPHA);
		draw3dLine((piVECTOR*)&V2, (piVECTOR*)&V3, colorGREENALPHA);
		draw3dLine((piVECTOR*)&V3, (piVECTOR*)&V1, colorGREENALPHA);

		V1 = CollisionList[0].CollidePnt;
		math3dVectorMultiply(&V1,&V1,&ColObj->mat);

		V2 = V3 = Pos;
		V2.x -= 1.0f;
		V3.x += 1.0f;
		draw3dLine((piVECTOR*)&V2, (piVECTOR*)&V3, colorYELLOWALPHA);
		V2 = V3 = Pos;
		V2.y -= 1.0f;
		V3.y += 1.0f;
		draw3dLine((piVECTOR*)&V2, (piVECTOR*)&V3, colorYELLOWALPHA);

	}
#endif
*/

	/*
	// make sure we're always above or on the ground
	if(vd->VehiclePos.z < collider.groundHit.z)
	{
		vd->VehiclePos.z = collider.groundHit.z;
		Velocity = 0.0f;
		pd->heightAboveGround = 0.0f;
	}
	*/

	return -Velocity;
}

static FLOAT64 _doGravityCampaign(mlVehicleData* vd, vehVehicle** walkOn, FLOAT64VECTOR& LastPos, FLOAT64 DeltaTime)
{
	static bool fallShortSoundPlayed, fallFarSoundPlayed;

	// Initialize
	fpPersonData* pd = (fpPersonData*)vd->PersonData;

	FLOAT64 Velocity = 0.0;

	// calculated in fpFPS.cpp > fpFPSAreaChecks()
	FPSColliderData& collider = fpFPSGetCollider();

	// it hasn't been initialized yet
	if(mathExactVectorLength2(&collider.groundHit) == 0)
		return 0.0f;

	// save our current z
	LastPos.z = vd->VehiclePos.z;

	// how high are we above ground?
	FLOAT64 agl = LastPos.z - collider.groundHit.z;

	////////////////////////////////////
	pd->heightAboveGround = (agl > 0) ? agl: 0.0f;

	//uiChatPrintLine("Height Above Ground is %.2f", pPersonData->heightAboveGround);

	// teleport us to the ground hit z (we might move from this)
	vd->VehiclePos.z = collider.groundHit.z;

	// for sound effects
	if (collider.objectHit)
	{
		fpFPSSetStandMaterial (collider.objectHit->matid);

		// set the condition to true to allow any vehicle surface to be used (troops standing on tanks and truck beds
		// as well as planes...anything); extra code is needed to make troops fall off moving planes or disallow
		// deploying on ground vehicles or whatever...a later patch perhaps?
		UINT32 walkMask = (false) ? COLLIDER_DRIVABLE_MASK: COLLIDER_WALK;

		// use the vehicle CollideWalkObject only
		bool walkable = (collider.objectHit->collider) ? colliderIsType(collider.objectHit->collider->getCategoryMask(), walkMask): false;

		if(walkable)
		{
			*walkOn = colliderSpaceData<vehVehicle>((dGeomID)collider.objectHit->collider->getSpace());

			if(!*walkOn || !vehVehicleInList(*walkOn))
				*walkOn = nullptr;
		}
	}

	FLOAT64 zDiff = LastPos.z - vd->VehiclePos.z;

	if(fpsTrooper.lastUnrideTime && (frametime - fpsTrooper.lastUnrideTime) <= 1000 && fabs(zDiff) < 2.0f)
		fpsTrooper.teleportToGround = TRUE;

	// final test for falling based on the resulting position
	if ((zDiff > 0.15) && (!fpsTrooper.teleportToGround))
	{
		// at this point, we're falling, so figure out what to do about it
		fxAmbDisable();

		if (pd->StartFallTime10ms == 0)
		{
			pd->StartFallTime10ms = ft10mscount;
			pd->StartFallHeight = LastPos.z;
			pd->Flags |= fpFLAGS_FALLING;
			fallShortSoundPlayed = fallFarSoundPlayed = false;
			Velocity = 0.0f;
		}

		if (ft10mscount - pd->StartFallTime10ms > 10) // i.e. 10 * 10ms == 0.1 second
		{
			Velocity = 9.8 * (ft10mscount - pd->StartFallTime10ms - 10) / 100.0;
		}
		else
		{
			vd->VehiclePos.z = LastPos.z;
		}

		if (fpsTrooper.parachuteOpen)
		{
			Velocity = fpFPSGetParachuteFallSpeed ();
		}
		else if (fpsTrooper.parachuteWorn && fpsTrooper.parachuteOKToOpen && pd->heightAboveGround > 25.0f)
		{
			if(fpFPSIsTrooperFalling())
				fpFPSetHeadFixed(TRUE);

			if ((ft10mscount - pd->StartFallTime10ms) > (fpsTrooper.parachuteDeployTime * 100.0))
			{
				fpFPSParachuteOpened();
			}
		}
		else if (((ft10mscount - pd->StartFallTime10ms) > (fpsTrooper.fallShortTime * 100.0)) && (!fallShortSoundPlayed))
		{
			fpFPSPlaySound(fpsTrooper.fallShortSound);
			fallShortSoundPlayed = 1;
		}
		else if (((ft10mscount - pd->StartFallTime10ms) > (fpsTrooper.fallFarTime * 100.0)) && (!fallFarSoundPlayed))
		{
			fpFPSPlaySound(fpsTrooper.fallFarSound);
			fallFarSoundPlayed = true;
		}

//				WriteLog (LOG_FPS, LOG_INFO, "Falling at %4.2f m/s", Velocity);

		LastPos.z = LastPos.z - Velocity * (ft10mscount - pd->LastFallTime10ms) / 100.0f;

		if(LastPos.z < collider.groundHit.z)
			LastPos.z = collider.groundHit.z;

		if (LastPos.z >= vd->VehiclePos.z)
			vd->VehiclePos.z = LastPos.z;

		pd->LastFallTime10ms = ft10mscount;
		fpFPSAddWeaponLag(AROUND_X, -3.0f);
	}
	else
	{
		if (pd->Flags & fpFLAGS_FALLING)
		{
			fxAmbEnable();
#ifdef BETA_RELEASE
			if(!SuperTroop)
				ApplyFallEffects(vd->VehicleObjectData->PlaneObject, pd, pd->StartFallHeight - vd->VehiclePos.z);
#else
			ApplyFallEffects(vd->VehicleObjectData->PlaneObject, pd, pd->StartFallHeight - vd->VehiclePos.z);
#endif // BETA_RELEASE

			pd->StartFallTime10ms = 0;
			pd->Flags &= ~fpFLAGS_FALLING;
			if ((fpsTrooper.parachuteOpen) || (fpsTrooper.parachuteOKToOpen))
				fpFPSParachuteLanded();
		}

		/*
		static FLOAT64 lastZDiff = 0;

		if((zDiff > 0 && lastZDiff < 0) || (zDiff < 0 && lastZDiff > 0))
			uiChatPrintLine("Z Direction Change is %.4f", zDiff);

		lastZDiff = zDiff;
		*/

		// z smoothing here?
		if(false && zDiff != 0.0f && !SuperTroop)
		{
			static FLOAT64 zRate = 1.0f;

			// make going up positive, going down negative
			// for clarity
			zDiff = -zDiff;

			//Velocity = zRate * DeltaTime;

			//uiChatPrintLine("Z Diff is %.4f", zDiff);

			// move us vertically consistently over time
			vd->VehiclePos.z = LastPos.z + (zRate * DeltaTime * zDiff);

			if(zDiff < 0 && vd->VehiclePos.z < collider.groundHit.z) // downhill overshoot
				vd->VehiclePos.z = collider.groundHit.z;
			else if(zDiff > 0 && vd->VehiclePos.z > collider.groundHit.z) // uphill overshoot
				vd->VehiclePos.z = collider.groundHit.z;
		}
		// else snap us to the ground z

		pd->StartFallTime10ms = 0;

		// snap us to the ground
		vd->VehiclePos.z = collider.groundHit.z;
		LastPos.z = collider.groundHit.z;
		Velocity = 0.0f;
		pd->heightAboveGround = 0.0f;
	}

/*
	// put an "x" at collision point
#ifdef VEHICLE_OBJECT_ANALYSIS
	{
		piVECTOR V1,V2,V3;

		// outline collide triangle
		V1 = ColObj->RenderPositions()[ColObj->ptris[CollisionList[0].PolyNum].v1];
		V2 = ColObj->RenderPositions()[ColObj->ptris[CollisionList[0].PolyNum].v2];
		V3 = ColObj->RenderPositions()[ColObj->ptris[CollisionList[0].PolyNum].v3];

		math3dVectorMultiply(&V1,&V1,&ColObj->mat);
		math3dVectorMultiply(&V2,&V2,&ColObj->mat);
		math3dVectorMultiply(&V3,&V3,&ColObj->mat);

		draw3dLine((piVECTOR*)&V1, (piVECTOR*)&V2, colorGREENALPHA);
		draw3dLine((piVECTOR*)&V2, (piVECTOR*)&V3, colorGREENALPHA);
		draw3dLine((piVECTOR*)&V3, (piVECTOR*)&V1, colorGREENALPHA);

		V1 = CollisionList[0].CollidePnt;
		math3dVectorMultiply(&V1,&V1,&ColObj->mat);

		V2 = V3 = Pos;
		V2.x -= 1.0f;
		V3.x += 1.0f;
		draw3dLine((piVECTOR*)&V2, (piVECTOR*)&V3, colorYELLOWALPHA);
		V2 = V3 = Pos;
		V2.y -= 1.0f;
		V3.y += 1.0f;
		draw3dLine((piVECTOR*)&V2, (piVECTOR*)&V3, colorYELLOWALPHA);

	}
#endif
*/

	if(vd->VehiclePos.z < collider.groundHit.z)
		vd->VehiclePos.z = collider.groundHit.z;

	return -Velocity;
}

static FLOAT64 _doGravity(mlVehicleData* vd, vehVehicle** walkOn, FLOAT64VECTOR& LastPos, FLOAT64 DeltaTime)
{
	if(gCustomTerrain)
		return _doGravityScenario(vd, walkOn, LastPos, DeltaTime);
	else
		return _doGravityCampaign(vd, walkOn, LastPos, DeltaTime);
}

static bool _updatePersonScenario(mlVehicleData* pVehicleData, FLOAT64 DeltaTime)
{
	if (!fpFPSUpdatePerson(pVehicleData, DeltaTime))
		return false;

	fpFPSSetStandingOnVehicle(FALSE);

	vehVehicle* FPSRideVehicle		= nullptr;
	vehVehicle* VehicleWalkingOn	= nullptr;
	vehVehicle* Vehicle				= pVehicleData->VehicleMasterPtr;

	// do some riding stuff
	if(Vehicle->RideVehicleHostIndex > 0)
		FPSRideVehicle = vehFindVehicle(Vehicle->RideVehicleHostIndex);

	if (FPSRideVehicle != nullptr && FPSRideVehicle->RideVehicleHostIndex == 0)
	{
		if(Vehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT && pVehicleData->LastRideVehicle == Vehicle->RideVehicleHostIndex)
			fpMoveToRelRidePos(FPSRideVehicle, Vehicle);	// allows trooper to "stick" to the deck

		if (FPSRideVehicle->StateAnimation.PlayCrash == 1) // check for plane blowup .. if so kill you
		{
			// bye bye!
			persApplyDamage(450.0, 0, PERS_HITLOCATION_HEAD, false);
		}

		if (FPSRideVehicle->StateAnimation.EjectRiders == 1) // check for ejection
		{
			// kick rider off of vehicle
			FPSRideVehicle = nullptr;
			fpFPSRideUnrideVehicle();
		}
	}
	else if(fpFPSGetPlayerPose() == POSE_SIT) // ahh, disappeared from under our feet
	{
		FPSRideVehicle = nullptr;
		fpFPSRideUnrideVehicle();
	}

	// if riding a vehicle, vehicle attitude and position override all
	if (FPSRideVehicle != nullptr && Vehicle->RideSeatNum != VEH_TROOPER_ON_DECK_RIDE_SEAT)
	{
		vehAttachToRidePoint(Vehicle);
		fpFPSSetInheritedVelocity (&FPSRideVehicle->PosData.Vel);
//		WriteLog (LOG_FPS, LOG_INFO, "Vehicle Yaw = %f", mlRAD_TO_DEG(FPSRideVehicle->PosData.Att.Yaw));
		fpFPSDoRidingShake();
	}
	else
	{
		// get our current velocity
		FLOAT64VECTOR CurrentVelocity;
		fpFPSGetVelocity(&CurrentVelocity);

		// this is our last good position
		FLOAT64VECTOR lpos = pVehicleData->VehiclePos;

		FPSColliderData& collider = fpFPSGetCollider();

		// move us on the xy plane
		UpdatePositionRotation(pVehicleData, DeltaTime, &CurrentVelocity);

		// do the ground collisions
		fpFPSAreaChecks(&CurrentVelocity, DeltaTime);

		// see if we hit our head
		if(collider.blocked)
			pVehicleData->VehiclePos = lpos;

		// do falling code once our pos has been reset
		if(Vehicle->ModelData->LastResetTime && SCEN_MGR->IsTerrainLoaded() && fpsTrooper.doGravity)
			CurrentVelocity.z = _doGravity(pVehicleData, &VehicleWalkingOn, lpos, DeltaTime); //CurrentVelocity.z =
		//else
		//	CurrentVelocity.z = 0.0f;

		pVehicleData->RenderPos = pVehicleData->VehiclePos;
		pVehicleData->LastYaw = pVehicleData->VehicleAtt.Yaw;

		fpsTrooper.teleportToGround = FALSE;

		pVehicleData->VehicleVel = CurrentVelocity;

		// Set VechicleAttMat
		pVehicleData->VehicleAtt.Pitch = pVehicleData->VehicleAtt.Roll = 0.0;

		piVECTOR TempPiVec;
		TempPiVec.x =  (FLOAT32)pVehicleData->VehicleAtt.Pitch;
		TempPiVec.y =  (FLOAT32)pVehicleData->VehicleAtt.Roll;
		TempPiVec.z = -(FLOAT32)pVehicleData->VehicleAtt.Yaw;

		mathEul_ToMatrix(&pVehicleData->VehicleAttMat,EulOrdXYZs,&TempPiVec);
	}

	pVehicleData->VehicleAttVel.Pitch = 0;
	pVehicleData->VehicleAttVel.Roll = 0;
	pVehicleData->VehicleAttVel.Yaw = 0;

	if(VehicleWalkingOn && !VehicleWalkingOn->isHulked())
	{
		//fpFPSStartSwimming (pVehicleData, FALSE);
		Vehicle->RideVehicleHostIndex = VehicleWalkingOn->VehicleHostIndex;
		Vehicle->RideSeatNum = VEH_TROOPER_ON_DECK_RIDE_SEAT;

		// save relative position
		fpSaveRelRidePos(VehicleWalkingOn, Vehicle);
	}
	else
	{
		// we were on the deck now we're not
		if(Vehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
		{
			Vehicle->RideVehicleHostIndex = 0;
			Vehicle->RideSeatNum = 0;
		}
	}

	return true;
}

static bool _updatePersonCampaign(mlVehicleData* pVehicleData, FLOAT64 DeltaTime)
{
	vehVehicle* Vehicle = pVehicleData->VehicleMasterPtr;

	vehVehicle* FPSRideVehicle		= nullptr;
	vehVehicle* VehicleWalkingOn	= nullptr;

	// Initialize
	if (!fpFPSUpdatePerson (pVehicleData, DeltaTime))
		return false;

	trncdCheckTreeBox(TRUE);

	fpFPSSetStandingOnVehicle(FALSE);

	if(Vehicle->RideVehicleHostIndex > 0)
		FPSRideVehicle = vehFindVehicle(Vehicle->RideVehicleHostIndex);

	if (FPSRideVehicle != nullptr && FPSRideVehicle->RideVehicleHostIndex == 0)
	{
		if(Vehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT && pVehicleData->LastRideVehicle == Vehicle->RideVehicleHostIndex)
			fpMoveToRelRidePos(FPSRideVehicle, Vehicle);	// allows trooper to "stick" to the deck

		if (FPSRideVehicle->StateAnimation.PlayCrash == 1) // check for plane blowup .. if so kill you
		{
			// bye bye!
			persApplyDamage(450.0, 0, PERS_HITLOCATION_HEAD, false);
		}

		if (FPSRideVehicle->StateAnimation.EjectRiders == 1) // check for ejection
		{
			// kick rider off of vehicle
			FPSRideVehicle = nullptr;
			fpFPSRideUnrideVehicle();
		}
	}
	else if(fpFPSGetPlayerPose() == POSE_SIT) // ahh, disappeared from under our feet
	{
		FPSRideVehicle = nullptr;
		fpFPSRideUnrideVehicle();
	}

	// if riding a vehicle, vehicle attitude and position override all
	if (FPSRideVehicle != nullptr && Vehicle->RideSeatNum != VEH_TROOPER_ON_DECK_RIDE_SEAT)
	{
		vehAttachToRidePoint(Vehicle);
		fpFPSSetInheritedVelocity (&FPSRideVehicle->PosData.Vel);
//		WriteLog (LOG_FPS, LOG_INFO, "Vehicle Yaw = %f", mlRAD_TO_DEG(FPSRideVehicle->PosData.Att.Yaw));
		fpFPSDoRidingShake();
	}
	else
	{
		FLOAT64VECTOR	CurrentVelocity;
		fpFPSGetVelocity (&CurrentVelocity);

		FLOAT64VECTOR	LastPos;

		// Update Positional Info
		LastPos.x = pVehicleData->VehiclePos.x;
		LastPos.y = pVehicleData->VehiclePos.y;
		LastPos.z = pVehicleData->VehiclePos.z;

		// move us on the xy plane
		UpdatePositionRotation(pVehicleData, DeltaTime, &CurrentVelocity);

		// do the ground collisions
		fpFPSAreaChecks(&CurrentVelocity, DeltaTime);

		// do falling code once our pos has been reset
		if(Vehicle->ModelData->LastResetTime)
			CurrentVelocity.z = _doGravity(pVehicleData, &VehicleWalkingOn, LastPos, DeltaTime);
		//else
		//	CurrentVelocity.z = 0.0f;

		// move us in the z
		//pVehicleData->VehiclePos.z += CurrentVelocity.z * DeltaTime;

		// /////////////////
		// Collision Code //
		////////////////////

		FLOAT64 SavedYaw  = pVehicleData->VehicleAtt.Yaw;

		// collider works
		// move forward
		// collide back
		// if collision, move back, then forward to collide point
		// could be .. more efficient
		// checks only for models
		vehVehicle* collideVehicle = nullptr;

		if (mlDetermineInfantryCollisions(pVehicleData, &CurrentVelocity, (UINT32)(fpFPSGetPlayerPose()), FALSE, 0, &collideVehicle))
		{
			pVehicleData->VehiclePos.x = LastPos.x;
			pVehicleData->VehiclePos.y = LastPos.y;
			//pVehicleData->VehiclePos.z = LastPos.z;

			pVehicleData->VehiclePos.x += CurrentVelocity.x * DeltaTime;
			pVehicleData->VehiclePos.y += CurrentVelocity.y * DeltaTime;
			//pVehicleData->VehiclePos.z += CurrentVelocity.z * DeltaTime;

			if(collideVehicle && collideVehicle->Side == Vehicle->Side && collideVehicle->VehicleType->Type == VEH_TYPE_GROUND)
			{
				// forward push
				Vec3d fpush = _vehicleCollision(Vehicle, collideVehicle);

				pVehicleData->VehiclePos.x += fpush.x() * DeltaTime;
				pVehicleData->VehiclePos.y += fpush.y() * DeltaTime;

				// side push
				Vec3d spush = fpush;
				spush.z() = 0;
				spush.normalize();

				Vec3d up = Vec3d(0, 0, 1.0f);

				// get side push
				spush = spush ^ up;

				// scale it down some
				spush *= .75f;

				pVehicleData->VehiclePos.x += spush.x() * DeltaTime;
				pVehicleData->VehiclePos.y += spush.y() * DeltaTime;
			}
		}

		pVehicleData->RenderPos = pVehicleData->VehiclePos;
		pVehicleData->LastYaw = SavedYaw;

		if (fpFPSVehicleCollideCheckNeeded() && (!fpsTrooper.lastUnrideTime || (frametime - fpsTrooper.lastUnrideTime) > 1000))
			mlDetermineInfantryCollisions(pVehicleData, &CurrentVelocity, (UINT32)(fpFPSGetPlayerPose()), TRUE);

		fpFPSMarkForVehicleCollideCheck (FALSE);

		fpsTrooper.teleportToGround = FALSE;

		pVehicleData->VehicleVel = CurrentVelocity;

		// Set VechicleAttMat
		pVehicleData->VehicleAtt.Pitch = pVehicleData->VehicleAtt.Roll = 0.0;

		piVECTOR TempPiVec;
		TempPiVec.x =  (FLOAT32)pVehicleData->VehicleAtt.Pitch;
		TempPiVec.y =  (FLOAT32)pVehicleData->VehicleAtt.Roll;
		TempPiVec.z = -(FLOAT32)pVehicleData->VehicleAtt.Yaw;

		mathEul_ToMatrix(&pVehicleData->VehicleAttMat,EulOrdXYZs,&TempPiVec);
	}
	////////////////////////
	// End Collision Code //
	////////////////////////

	pVehicleData->VehicleAttVel.Pitch = 0;
	pVehicleData->VehicleAttVel.Roll = 0;
	pVehicleData->VehicleAttVel.Yaw = 0;

	if(VehicleWalkingOn && !VehicleWalkingOn->isHulked())
	{
		fpFPSStartSwimming (pVehicleData, FALSE);
		Vehicle->RideVehicleHostIndex = VehicleWalkingOn->VehicleHostIndex;
		Vehicle->RideSeatNum = VEH_TROOPER_ON_DECK_RIDE_SEAT;

		// save relative position
		fpSaveRelRidePos(VehicleWalkingOn, Vehicle);
	}
	else
	{
		LOGICAL	overWater;

		// we were on the deck now we're not
		if(Vehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
		{
			Vehicle->RideVehicleHostIndex = 0;
			Vehicle->RideSeatNum = 0;
		}

		if (mlIsFPSOnWater(pVehicleData, &overWater))
		{
			fpFPSStartSwimming (pVehicleData, overWater);
		}
	}

	//mlInfantryCheckLeanLines(pVehicleData);

	return true;
}

bool fpUpdatePerson(mlVehicleData* pVehicleData, FLOAT64 DeltaTime)
{
	// Sanity Checks
	if (pVehicleData == nullptr) return 0;
	if (pVehicleData->VehicleObjectData == nullptr) return 0;
	if (pVehicleData->VehicleObjectData->PlaneObject == nullptr) return 0;
	if (pVehicleData->LastResetTime == 0) return 0;
	if (gCustomTerrain && !SCEN_MGR->IsTerrainLoaded()) return 0;

	/*
	static bool first = true;

	if(first)
	{
		uiConsoleprintf("SCENARIO: First fp update called!");
		first = false;
	}
	*/

	bool result;
	if(gCustomTerrain)
		result = _updatePersonScenario(pVehicleData, DeltaTime);
	else
		result = _updatePersonCampaign(pVehicleData, DeltaTime);

	OBJECT* pObject = pVehicleData->VehicleObjectData->PlaneObject;

	// Sound Update
	fpFPSUpdateSounds(pObject);

	// Set the AnimationFlags
	fpFPSSetAnimationFlags();

	// update our tps
	gr2AnimProcessAnimationFlags(pObject, fpGetFPPersonDataPtr()->AnimationFlags);

	if(!Online)
		fpFPSUpdateSent();

	trncdCheckTreeBox(FALSE);

	return result;
}

LOGICAL _doUpdatePosRotCampaign(mlVehicleData* pVehicleData, FLOAT64 DeltaTime, FLOAT64VECTOR* pCurrentVelocity)
{
	FLOAT64			CosValue, SinValue;
	FLOAT64VECTOR	Velocity;

	// rotate vector along yaw axis (which is along the Z axis, counterclockwise)
	CosValue = cos((pVehicleData->VehicleAtt.Yaw + PI));
	SinValue = sin((pVehicleData->VehicleAtt.Yaw + PI));

	// Update Position
	Velocity.y = CosValue * pCurrentVelocity->y + SinValue * pCurrentVelocity->x;
	Velocity.x = -SinValue * pCurrentVelocity->y + CosValue * pCurrentVelocity->x;

	pVehicleData->VehiclePos.x += Velocity.x * DeltaTime;
	pVehicleData->VehiclePos.y += Velocity.y * DeltaTime;

	fpFPSUpdateOverSupply(pVehicleData);

	pCurrentVelocity->y = Velocity.y;
	pCurrentVelocity->x = Velocity.x;

	return TRUE;
}


LOGICAL _doUpdatePosRotScenario(mlVehicleData* pVehicleData, FLOAT64 DeltaTime, FLOAT64VECTOR* pCurrentVelocity)
{
	Vec3d velocity;
	Vec3d down = Vec3d(0, 0, -1.0f);

	Matrixd rotate;
	rotate.makeRotate(pVehicleData->VehicleAtt.Yaw + PI, 0, 0, 1.0f);

	fpFPSSetSteepness(0.0f);

	velocity.set(*pCurrentVelocity);

	double speed = velocity.length();

	if(speed)
	{
		velocity = velocity * rotate;

		// scale velocity times deltatime
		velocity *= DeltaTime;

		// find out the normal in front of us
		Vec3d position = Vec3d(pVehicleData->VehiclePos);

		position = physicsGameToPhysical(position);

		Vec3d start = position;
		start.z() += 2.0f;

		// move our temp position forward
		start += velocity;

		const UINT32 cflags = COLLIDER_HARDTERRAIN_MASK|COLLIDER_VEHICLE_MASK|COLLIDER_WALK|COLLIDER_TILE;
		const colliderQuery::CollisionList& hits = collideRayList(cflags, 2.0f, start, down, true);

		// do we have a hit
		if(!hits.empty())
		{
			// maximum allow z to use
			const double maxZ = pVehicleData->LastVehiclePos.z + MAX_FPS_STEP;

			const Vec3d up = Vec3d(0, 0, 1.0f);

			// we hit something, do we need to adjust our velocity up and down a hill?
			for(size_t i = 0; i < hits.size(); i++)
			{
				const colliderQuery::Collision& hit = hits.at(i);

				if(gCustomTerrain && colliderIsType(hit.g2, COLLIDER_TRN_WATER))
					continue;

				if(hit.pos.z() > maxZ)
					continue;

				if((up * hit.normal) < 0)
					continue;

				Vec3d from = velocity;
				from.normalize();

				// only adjust velocity if we're going up
				if(!colliderIsType(hit.g2, COLLIDER_MANMADE_MASK))//hit.pos.z() > pos.z())
				{
					// if we are going up
					if(hit.pos.z() > position.z())
					{
						Vec3d dir = -velocity;
						dir.normalize();

						fpFPSSetSteepness((float)(dir * hit.normal));
					}

					Vec3d to = hit.pos - position;
					to.normalize();

					//Matrixd rotate;
					rotate.makeRotate(from, to);

					// rotate our normal into the same plane as our direction
					velocity = velocity * rotate;

					break;
				}
			}
		}

		// now we update our position
		pVehicleData->VehiclePos.x += velocity.x();
		pVehicleData->VehiclePos.y += velocity.y();
		pVehicleData->VehiclePos.z += velocity.z();

		velocity.normalize();
		velocity *= speed;

		// and our velocity
		pCurrentVelocity->y = velocity.y();
		pCurrentVelocity->x = velocity.x();
		pCurrentVelocity->z = velocity.z();

		// update oversupply state
		fpFPSUpdateOverSupply(pVehicleData);
	}

	return TRUE;
}

LOGICAL UpdatePositionRotation(mlVehicleData* pVehicleData, FLOAT64 DeltaTime, FLOAT64VECTOR* pCurrentVelocity)
{
	if(gCustomTerrain)
		return _doUpdatePosRotScenario(pVehicleData, DeltaTime, pCurrentVelocity);
	else
		return _doUpdatePosRotCampaign(pVehicleData, DeltaTime, pCurrentVelocity);
}

static void ApplyFallEffects(OBJECT* pObject, fpPersonData* pPersonData, FLOAT64 Distance)
{
	SINT32			fallType = FPS_FALL_NORMAL;
	FLOAT64			Damage;
	bool			ATPOnly = false;
	FLOAT64			speed, fallTime;

	if ((fpsTrooper.parachuteOpen) || (fpsTrooper.parachuteOKToOpen))
	{
		Damage = 0.5;
		ATPOnly = true;

		speed = mathExactVectorLength(&fpsTrooper.inheritedVehicleVelocity);

		// use this to determine if it's just a jump off or a JUMP off
		if (speed > fpsTrooper.maxSafeVehicleSpeed)
		{
			fallTime = (ft10mscount - pPersonData->StartFallTime10ms)/100.0f;

			if (fallTime < fpsTrooper.parachuteDecelerationTime)
			{
				// splat
				Damage = 1.0;
                // fallType = FPS_FALL_NORMAL_HURT; // See DEV-781
			}
		}
	}
	else if (fpFPSIsSwimming())
	{
		Damage = DetermineFallEffect(Distance, fpFALL_DISTANCE_WATER_HURT_MIN, fpFALL_DISTANCE_WATER_HURT_MAX);
		if (Damage == 0.0)
		{
			Damage   = DetermineFallEffect(Distance, fpFALL_DISTANCE_WATER_STUN_MIN, fpFALL_DISTANCE_WATER_STUN_MAX);
			fallType = FPS_FALL_WATER;
			ATPOnly  = true;
		}
		else
		{
			fallType = FPS_FALL_WATER_HURT;
		}
	}
	else
	{
		Damage = DetermineFallEffect(Distance, fpFALL_DISTANCE_NORMAL_HURT_MIN, fpFALL_DISTANCE_NORMAL_HURT_MAX);
		if (Damage == 0.0)
		{
			Damage   = DetermineFallEffect(Distance, fpFALL_DISTANCE_NORMAL_STUN_MIN, fpFALL_DISTANCE_NORMAL_STUN_MAX);
			if (Damage > 0.0)
			{
				fallType = FPS_FALL_NORMAL_HURT;
			}

			ATPOnly  = true;
		}
		else
		{
			fallType = FPS_FALL_NORMAL_HURT;
		}
	}

	Damage *= 450.0; // 400.0 joules == death;

	if (Damage > 400.0)
	{
		persApplyDamage(Damage, 0, PERS_HITLOCATION_HEAD, false);
	}
	else
	{
		persApplyDamage(Damage, 0, PERS_HITLOCATION_LEGS, ATPOnly);
		fpFPSAddWeaponLag(AROUND_X, 20.0f, true);
	}

	fpFPSPlayFallSound(pObject, fallType);
}

FLOAT64 DetermineFallEffect(FLOAT64 Distance, FLOAT64 MinDistance, FLOAT64 MaxDistance)
{
	if (Distance > MinDistance)
	{
		// Determine Loss
		if (Distance >= MaxDistance)
		{
			return 1.0;
		}
		else
		{
			return (Distance - MinDistance) / (MaxDistance - MinDistance);
		}
	}

	return 0.0;
}

LOGICAL fpProcessVehicleAnimationFlags(vehVehicle*Vehicle)
{
	if(!Vehicle->VehicleObjectData || !Vehicle->VehicleObjectData->PlaneObject)
		return FALSE;

	OBJECT* pObject = Vehicle->VehicleObjectData->PlaneObject;

	if (pObject && pObject->flag2 & OBJECT_ANIMATION)
		gr2AnimProcessAnimationFlags(pObject, Vehicle->AnimationBits);

	return TRUE;
}

LOGICAL fpHandleProneTrooperPosition(OBJECT* pObject, vehVehiclePositionData * PosData)
{
	if (false && gr2AnimIsTrooperProne(pObject))
	{
		if ((!(pObject->flag2 & OBJECT_ISFPS)))
		{
			if (PosData)
			{
				FLOAT64VECTOR	shiftVec;
				FLOAT64			cosValue, sinValue;

				cosValue = cos(PosData->Att.Yaw + PI);
				sinValue = sin(PosData->Att.Yaw + PI);

				shiftVec.x = sinValue * -TROOPER_PRONE_SHIFT_BACK_DISTANCE;
				shiftVec.y = cosValue * -TROOPER_PRONE_SHIFT_BACK_DISTANCE;
				shiftVec.z = 0;

				mathAddVectors (&(PosData->Pos), &(PosData->Pos), &shiftVec);
			}
		}
	}
	return TRUE;
}


LOGICAL fpSetExternal(mlVehicleData * ModelData)
{
fpPersonData* PersonData;

	if (ModelData == nullptr)
	{
		return FALSE;
	}

	PersonData = (fpPersonData*) ModelData->PersonData;
	if (PersonData == nullptr)
	{
		return FALSE;
	}

	PersonData->Flags |= fpFLAGS_FORCE_EXTERNAL;

	return TRUE;
}

LOGICAL fpSetInternal(mlVehicleData * ModelData)
{
fpPersonData* PersonData;

	if (ModelData == nullptr)
	{
		return FALSE;
	}

	PersonData = (fpPersonData*) ModelData->PersonData;
	if (PersonData == nullptr)
	{
		return FALSE;
	}

	PersonData->Flags &= ~fpFLAGS_FORCE_EXTERNAL;

	return TRUE;
}

LOGICAL fpGetEyePoint(mlVehicleData* pVehicleData, piMATRIX* pMatrix, LOGICAL allowGunSight)
{
	fpFPSGetEyeMatrix (pMatrix);

	return TRUE;
}

// type 0 = fps, type 1 = tps
//FLOAT32 fpGetZAdjust(int pose, int move, int point, int type)
FLOAT32 fpGetZAdjust(int pose, int move, int type)
{
	//WriteLog (LOG_FPS, LOG_INFO, "Z ADJUST: pose = %d, move = %d", pose, move);

	// load adjustments if needed
	if(_fpsAdjustments.empty())
	{
		fpAdjustLoader loader = fpAdjustLoader(Z_ADJUST_FILE);
		loader.Load();
	}

	//UINT64 id = fpTPSCreateBlendID(pose, move, TPS_WEAPONTYPE_ANY, point);

	FPAdjustments::iterator found;
	FPAdjustments& adjusts = (type == 1) ? _tpsAdjustments: _fpsAdjustments;

	found = adjusts.find(std::make_pair(pose, move));

	if(found != adjusts.end())
		return found->second;

	if(move > -1)
	{
		found = adjusts.find(std::make_pair(pose, (int)-1));

		if(found != adjusts.end())
			return found->second;

		if(pose > -1)
		{
			found = adjusts.find(std::make_pair((int)-1, (int)-1));

			if(found != adjusts.end())
				return found->second;
		}
	}

	return (type == 1) ? 0.0f: 1.602f;
}

UINT32 fpCollideRoundWithVehicleWalkObject(FASTCOLLIDE ** CollideListPtr,
										 UINT32* NumCollisionSlotsLeft,
										 piVECTOR const* OldPos,
										 piVECTOR const* NewPos)
{
	vehVehicle*		CurrentVehicle;
	piVECTOR		SphereCenter;
	UINT32			Collision;
//	SINT32			collisionOccurred = 0;
	FLOAT64VECTOR 	WorldOriginOffset;
	OBJECT*			colObject;

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	vehGetVehicleHead(&CurrentVehicle);
	trnGetWorldOriginOffset(&WorldOriginOffset);
	WorldOriginOffset.z = -WorldOriginOffset.z;

	for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if (CurrentVehicle == PlayerVehicle)
			continue;

		if (CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON)
			continue;

		if (CurrentVehicle->VehicleObjectData == nullptr)
			continue;

		if (!CurrentVehicle->InUse)
			continue;

		if (CurrentVehicle->VehicleType->Type == VEH_TYPE_AIR)
			continue;

		if (CurrentVehicle->VehicleObjectData->CollideWalkObject)
			colObject = CurrentVehicle->VehicleObjectData->CollideWalkObject;
		else
		{
			// ignore friendly vehicles w/o walk objects
			if (CurrentVehicle->Side == PlayerVehicle->Side)
				continue;

			colObject = CurrentVehicle->VehicleObjectData->CollideExternObject;
		}

		if (colObject == nullptr)
			continue;

		//CurrentVehicle->VehicleObjectData->CollideExternObject
		//CurrentVehicle->VehicleObjectData->HitBox
		SphereCenter.x =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - WorldOriginOffset.x);
		SphereCenter.y =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - WorldOriginOffset.y);
		SphereCenter.z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - WorldOriginOffset.z);

		mlUtilityCheckLineSphereCollide(&Collision, &SphereCenter, OldPos,
										NewPos, CurrentVehicle->VehicleObjectData->Radius);

		if (Collision != 0)
		{
            // Prevent boats from taking infantry off of bridges.
            if(CurrentVehicle->VehicleType->Type == VEH_TYPE_SEA)
            {
                // Check if I'm on a bridge
                if(fpFPSOnBridge())
                {
                    continue;
                }
            }

 			UINT32 TempNum = *NumCollisionSlotsLeft;
			FASTCOLLIDE* TempCollideListPtr = *CollideListPtr;

			weapUpdateVehicleHitmap(CurrentVehicle);
			weapCollideWithHitmapObject(CollideListPtr,
										 NumCollisionSlotsLeft,
										 colObject,
//										 CurrentVehicle->VehicleObjectData->CollideWalkObject,
										 (OBJECT*)CurrentVehicle,
										 OldPos,
										 NewPos,
										 1,
										 1);
			TempNum -= *NumCollisionSlotsLeft;
			if ((CurrentVehicle->VehicleObjectData->CollideWalkObject) && (CurrentVehicle->Side == PlayerVehicle->Side))
				for (UINT32 i=0;i<TempNum;i++)
					TempCollideListPtr[i].Flags |= _FP_WALK_COLLIDE_FLAG;

#if(0) // Allow deploy on ships by not setting this
			fpFPSSetStandingOnVehicle(TRUE);
#endif
		}
	}

	return 0;
}

void fpMoveToRelRidePos(vehVehicle* FPSRideVehicle, vehVehicle* Vehicle)
{
	FLOAT64VECTOR RelRidePosition;

	if(FPSRideVehicle->ModelData)
		mathVectorMultiply64(&RelRidePosition, &Vehicle->RelRidePosition, &FPSRideVehicle->ModelData->VehicleAttMat);
	else
		mathVectorMultiply64(&RelRidePosition, &Vehicle->RelRidePosition, &FPSRideVehicle->PosData.AttMat);

	if(Vehicle->ModelData)
	{
		mathAddVectors(&Vehicle->ModelData->VehiclePos, &FPSRideVehicle->PosData.RenderPos, &RelRidePosition);
		Vehicle->ModelData->AdjustedForCGPos = Vehicle->ModelData->VehiclePos;
		Vehicle->ModelData->VehicleAtt.Yaw = (-FPSRideVehicle->PosData.RenderAtt.Yaw) + Vehicle->RelRideYaw;
	}else
	{
		mathAddVectors(&Vehicle->PosData.RenderPos, &FPSRideVehicle->PosData.RenderPos, &RelRidePosition);
		Vehicle->PosData.LastUpdatePos = Vehicle->PosData.RenderPos;
		Vehicle->PosData.Pos =  Vehicle->PosData.RenderPos;

		Vehicle->PosData.Att.Yaw = FPSRideVehicle->PosData.Att.Yaw + Vehicle->RelRideYaw;
		Vehicle->PosData.RenderAtt.Yaw = FPSRideVehicle->PosData.RenderAtt.Yaw + Vehicle->RelRideYaw;
	}
}

void fpSaveRelRidePos(vehVehicle* FPSRideVehicle, vehVehicle* Vehicle)
{
	piMATRIX invmat;
	math3dInvert2(&invmat, &FPSRideVehicle->PosData.AttMat);
	mathSubVectors(&Vehicle->RelRidePosition, &Vehicle->ModelData->VehiclePos, &FPSRideVehicle->PosData.RenderPos);
	mathVectorMultiply64(&Vehicle->RelRidePosition, &Vehicle->RelRidePosition, &invmat);
	Vehicle->RelRideYaw = Vehicle->ModelData->VehicleAtt.Yaw - (-FPSRideVehicle->PosData.RenderAtt.Yaw); // reversed of course
}

// needs to return a collide object, a position, and a vehicle walking on
// returns TRUE if we've gotten a new object to collide with, FALSE if not
/* not used
LOGICAL fpCheckFallRing (mlVehicleData* pVehicleData, SINT32 numLines,// piVECTOR* testArray, FLOAT32 rayLength,
						 piVECTOR* startPt, piVECTOR* endPt,
						 OBJECT** ColObjPtr, piVECTOR* posPtr, vehVehicle**vehicleWalkPtr)
{
	FLOAT64			CosValue, SinValue;
	SINT32			i;
	FLOAT64VECTOR	WorldOriginOffset;
	OBJECT*			keeperColObj = nullptr;
	piVECTOR		keeperPos;
	piVECTOR		rayStart, rayEnd;
	UINT32			TempUint;
	UINT32			NumCollides;
	UINT32			CollideNum;
	FASTCOLLIDE		CollisionList[32];
	FASTCOLLIDE*		CollideListPtr;
	UINT32			RetVal;
	// return data
	piVECTOR		colPos;
	FLOAT32			colTime, keeperColTime;
	OBJECT*			ColObj;
	vehVehicle*		VehicleWalkingOn = nullptr;

	CosValue = cos((pVehicleData->VehicleAtt.Yaw));
	SinValue = sin((pVehicleData->VehicleAtt.Yaw));
	trnGetWorldOriginOffset(&WorldOriginOffset);
	memset(CollisionList, 0x0, sizeof(CollisionList));

	for (i=0; i < numLines; i++)
	{
		rayStart.x = (FLOAT32)(pVehicleData->VehiclePos.x - WorldOriginOffset.x);
		rayStart.y = (FLOAT32)(pVehicleData->VehiclePos.y - WorldOriginOffset.y);
		rayStart.z = -(FLOAT32)(pVehicleData->VehiclePos.z + WorldOriginOffset.z);

		rayEnd = rayStart;

		rayStart.x += (FLOAT32)(CosValue * startPt[i].x + -SinValue * startPt[i].y);
		rayStart.y += (FLOAT32)(CosValue * startPt[i].y + SinValue * startPt[i].x);
		rayStart.z -= startPt[i].z;

		rayEnd.x += (FLOAT32)(CosValue * endPt[i].x + -SinValue * endPt[i].y);
		rayEnd.y += (FLOAT32)(CosValue * endPt[i].y + SinValue * endPt[i].x);
		rayEnd.z -= endPt[i].z;

//		draw3dLine(&rayStart, &rayEnd, colorGREENALPHA);

		TempUint = 32;
		CollideListPtr = CollisionList;
		trncdCollideWithListOfTerrainObjects(&CollideListPtr,
												&TempUint,
												pVehicleData->ShortTrnList,
												ML_NUM_SHORT_LIST_ELEMENTS,
												&rayStart,
												&rayEnd,
												0);


		NumCollides = 32 - TempUint;

		if (NumCollides < 32)
		{
			TempUint = 32 - NumCollides;
			fpCollideRoundWithVehicleWalkObject(&CollideListPtr, &TempUint, &rayStart, &rayEnd);
			NumCollides = 32 - TempUint;
		}

		if (NumCollides < 32)
		{

			RetVal = trncdCollideWithPolyList(
									&rayStart,
									&rayEnd,
									&CollisionList[NumCollides].CollidePnt,
									&CollisionList[NumCollides].CollideObject,
									&CollisionList[NumCollides].PolyNum,
									pVehicleData->TerrainAGLObjectList,
									pVehicleData->TerrainAGLPolyList,
									pVehicleData->NumTerrainAGL);

			if (RetVal != 0)
			{
				math3dVectorMultiply(&CollisionList[NumCollides].CollidePnt,
										&CollisionList[NumCollides].CollidePnt,
										&CollisionList[NumCollides].CollideObject->mat);


				CollisionList[NumCollides].CollideTime =
						(FLOAT32)((CollisionList[NumCollides].CollidePnt.z - rayStart.z) /
									(rayEnd.z - rayStart.z));
				CollisionList[NumCollides].CollideTime =
						(FLOAT32)(1.0 - CollisionList[NumCollides].CollideTime);

				CollisionList[NumCollides].Alpha = 1.0;
				NumCollides++;
			}
		}

		CollideNum = 0;

		mlUtilitySortCollideList(CollisionList,NumCollides);

		while (CollideNum < NumCollides && CollisionList[CollideNum].Alpha < 0.5)
		{
			CollideNum++;
		}

		if (CollideNum != NumCollides)
		{
			ColObj	= CollisionList[CollideNum].CollideObject;

			if(CollisionList[CollideNum].Flags & _FP_WALK_COLLIDE_FLAG) // walk object
				VehicleWalkingOn = (vehVehicle*)CollisionList[CollideNum].TopLevelObject;

			if(VehicleWalkingOn && !vehVehicleInList(VehicleWalkingOn))
			{
				VehicleWalkingOn = nullptr;
				ColObj = nullptr;
			}
			else
			{
				colPos = CollisionList[CollideNum].CollidePnt;

				if (ColObj && ColObj->flags & OBJECT_USEMATRIX)
					math3dVectorMultiply(&colPos,&colPos,&ColObj->mat);

				colTime = CollisionList[CollideNum].CollideTime;
			}
		}
		else
		{
			ColObj = nullptr;
		}

		if (ColObj)
		{
			if (!keeperColObj)
			{
				keeperColObj = ColObj;
				keeperPos = colPos;
				keeperColTime = colTime;
			}
			else
			{
				// negative z is up, and we're looking for higher ground, so whichever
				// has the LESSER value is the higher and we want to keep that one
				if (colPos.z < keeperPos.z)
				{
					keeperColObj = ColObj;
					keeperPos = colPos;
					keeperColTime = colTime;
				}
			}
		}

		ColObj	= keeperColObj;
		colPos	= keeperPos;
	}

	if (ColObj)
	{
		// since we're returning the position the trooper needs to be in to not penetrate (actually, only the z value) we
		// modify the return pos ..
		colPos.z += startPt[0].z * (1.0f - keeperColTime) + endPt[0].z * keeperColTime;
		*ColObjPtr = ColObj;
		*posPtr = colPos;

		if(vehicleWalkPtr)
			*vehicleWalkPtr = VehicleWalkingOn;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
*/

Vec3d _vehicleCollision(vehVehicle* me, vehVehicle* cv)
{
	Vec3d push;

	if(cv)
	{
		push = cv->PosData.Vel;
		push.z() = 0;

		if(push.length2())
		{
			Vec3d vpos = cv->PosData.Pos;
			Vec3d ppos = me->ModelData->VehiclePos;

			vpos.z() = 0;
			ppos.z() = 0;

			Vec3d toMe = ppos - vpos;
			Vec3d dir = push;

			toMe.normalize();
			dir.normalize();

			// must be coming towards me
			if((dir * toMe) < .7f)
				push.set(0,0,0);
		}
	}

	return push;
}
