#include <syscfg.h> // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"
#include "math/mathvect.h"
#include "ml/mltypes.h"
#include "pers/persmain.h"
#include "pers/persmsg.h"
#include "draw/draw3d.h"
#include "miss/missionUtils.h"

#include "trn/trnmain.h"
#include "trn/trncd.h"

#include "veh/vehmain.h"
#include "fp/fpMain.h"
#include "fp/fpTPS.h"

#include <math.h>
#include "math/math3d.h"
#include "fp/fpFPS.h"
#include "gr2/gr2Anim.h"
#include "tut/TutorialManager.h"
#include "imap/ww2func.h"
#include "veh/vehupdate.h"

//  copied straight from stratData.c on the host
//static WWII_SIDE _sideArray[] = {NEUTRAL, ALLIED, ALLIED, ALLIED, AXIS, AXIS, AXIS, ALLIED, ALLIED, ALLIED};

#define _MS_PER_TIC 20.0 // 50 cycles per second model loop

// trooper radius is 0.5 at the widest point!

// Stand Collider Info
static piVECTOR _FPSCollideCenterPntStand = {0.0f,0.0f,0.0f};
#define			_FPS_NUM_COLLIDE_LINES_STAND 24
static piVECTOR _FPSCollideLinesStand[_FPS_NUM_COLLIDE_LINES_STAND][2] = {
// Bottom Ring
{{-0.125f,	 0.0f,		0.5f},	{-0.0625f,	 0.075f,	0.5f}},
{{-0.0625f,	 0.075f,	0.5f},	{ 0.0625f,	 0.075f,	0.5f}},
{{ 0.0625f,	 0.075f,	0.5f},	{ 0.125f,	 0.0f,		0.5f}},
{{ 0.125f,	 0.0f,		0.5f},	{ 0.0625f,	-0.075f,	0.5f}},
{{ 0.0625f,	-0.075f,	0.5f},	{-0.0625f,	-0.075f,	0.5f}},
{{-0.0625f,	-0.075f,	0.5f},	{-0.125f,	 0.0f,		0.5f}},

// Connect Bottom to Middle Ring
{{-0.125f,	 0.0f,		0.5f},	{-0.50f,	 0.0f,		0.99f}},
{{-0.0625f,	 0.075f,	0.5f},	{-0.25f,	 0.433f,	0.99f}},
{{ 0.0625f,	 0.075f,	0.5f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.125f,	 0.0f,		0.5f},	{ 0.50f,	-0.0f,		0.99f}},
{{ 0.0625f,	-0.075f,	0.5f},	{ 0.25f,	-0.433f,	0.99f}},
{{-0.0625f,	-0.075f,	0.5f},	{-0.25f,	-0.433f,	0.99f}},

// Middle Ring
{{-0.50f,	 0.0f,		0.99f},	{-0.25f,	 0.433f,	0.99f}},
{{-0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.50f,	 0.0f,		0.99f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.25f,	-0.433f,	0.99f}},
{{ 0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	0.99f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.50f,	 0.0f,		0.99f}},

// Connect Top to Middle Rings

{{-0.50f,	 0.0f,		0.99f},	{-0.50f,	 0.0f,		1.8f}},
{{-0.25f,	 0.433f,	0.99f},	{-0.25f,	 0.433f,	1.8f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	1.8f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.50f,	 0.0f,		1.8f}},
{{ 0.25f,	-0.433f,	0.99f},	{ 0.25f,	-0.433f,	1.8f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	1.8f}},
};

// Kneel Collider Info
static piVECTOR	_FPSCollideCenterPntKneel = {0.0f,0.0f,0.0f};
#define			_FPS_NUM_COLLIDE_LINES_KNEEL 24
static piVECTOR	_FPSCollideLinesKneel[_FPS_NUM_COLLIDE_LINES_KNEEL][2] = {
// Middle Ring
{{-0.125f,	 0.0f,		0.5f},	{-0.0625f,	 0.075f,	0.5f}},
{{-0.0625f,	 0.075f,	0.5f},	{ 0.0625f,	 0.075f,	0.5f}},
{{ 0.0625f,	 0.075f,	0.5f},	{ 0.125f,	 0.0f,		0.5f}},
{{ 0.125f,	 0.0f,		0.5f},	{ 0.0625f,	-0.075f,		0.5f}},
{{ 0.0625f,	-0.075f,		0.5f},	{-0.0625f,	-0.075f,		0.5f}},
{{-0.0625f,	-0.075f,		0.5f},	{-0.125f,	 0.0f,		0.5f}},

// Connect Bottom to Middle Ring
{{-0.125f,	 0.0f,		0.5f},	{-0.50f,		0.0f,		0.99f}},
{{-0.0625f,	 0.075f,	0.5f},	{-0.25f,		0.433f,		0.99f}},
{{ 0.0625f,	 0.075f,	0.5f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.125f,	 0.0f,		0.5f},	{ 0.50f,	-0.0f,		0.99f}},
{{ 0.0625f,	-0.075f,		0.5f},	{ 0.25f,	-0.433f,		0.99f}},
{{-0.0625f,	-0.075f,		0.5f},	{-0.25f,		-0.433f,		0.99f}},

// Middle Ring
{{-0.50f,	 0.0f,		0.99f},	{-0.25f,		0.433f,		0.99f}},
{{-0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.50f,	 0.0f,		0.99f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.25f,	-0.433f,		0.99f}},
{{ 0.25f,	-0.433f,		0.99f},	{-0.25f,		-0.433f,	0.99f}},
{{-0.25f,	-0.433f,		0.99f},	{-0.50f,		0.0f,		0.99f}},

// Connect Top to Middle Rings

{{-0.50f,	 0.0f,		0.99f},	{-0.50f,	 0.0f,		1.3f}},
{{-0.25f,	 0.433f,	0.99f},	{-0.25f,	 0.433f,	1.3f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	1.3f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.50f,	 0.0f,		1.3f}},
{{ 0.25f,	-0.433f,	0.99f},	{ 0.25f,	-0.433f,	1.3f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	1.3f}},
};

// Prone Collider Info
static piVECTOR	_FPSCollideCenterPntProne = {0.0f,0.0f,0.0f};
#define			_FPS_NUM_COLLIDE_LINES_PRONE 18
#define			_PRONE_MP 1.75f
static piVECTOR	_FPSCollideLinesProne[_FPS_NUM_COLLIDE_LINES_PRONE][2] = {
// Connect Bottom to Middle Ring
{{-0.125f * _PRONE_MP,	 0.0f * _PRONE_MP,		0.25f},	{-0.50f,	 0.0f,		0.99f}},
{{-0.0625f * _PRONE_MP,	 0.075f * _PRONE_MP,	0.25f},	{-0.25f,	 0.433f,	0.99f}},
{{ 0.0625f * _PRONE_MP,	 0.075f * _PRONE_MP,	0.25f},	{ 0.25f, 0.433f,	0.99f}},
{{ 0.125f * _PRONE_MP,	 0.0f * _PRONE_MP,		0.25f},	{ 0.50f,-0.0f,		0.99f}},
{{ 0.0625f * _PRONE_MP,	-0.075f * _PRONE_MP,		0.25f},	{ 0.25f,-0.433f,		0.99f}},
{{-0.0625f * _PRONE_MP,	-0.075f * _PRONE_MP,		0.25f},	{-0.25f,	-0.433f,		0.99f}},

// Middle Ring
{{-0.50f,	 0.0f,		0.99f},	{-0.25f,	 0.433f,	0.99f}},
{{-0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.50f,	 0.0f,		0.99f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.25f,	-0.433f,	0.99f}},
{{ 0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	0.99f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.50f,	 0.0f,		0.99f}},

// Connect Top to Middle Rings

{{-0.50f,	 0.0f,		0.99f},	{-0.50f,	 0.0f,		1.3f}},
{{-0.25f,	 0.433f,	0.99f},	{-0.25f,	 0.433f,	1.3f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	1.3f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.50f,	 0.0f,		1.3f}},
{{ 0.25f,	-0.433f,	0.99f},	{ 0.25f,	-0.433f,	1.3f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	1.3f}},
};

// Transition Collider Info
static piVECTOR _FPSCollideCenterPntTransition = {0.0f,0.0f,0.0f};
#define			_FPS_NUM_COLLIDE_LINES_TRANSITION 42
static piVECTOR _FPSCollideLinesTransition[_FPS_NUM_COLLIDE_LINES_TRANSITION][2] = {
	// stand cage
// Bottom Ring
{{-0.125f,	 0.0f,		0.5f},	{-0.0625f,	 0.075f,	0.5f}},
{{-0.0625f,	 0.075f,	0.5f},	{ 0.0625f,	 0.075f,	0.5f}},
{{ 0.0625f,	 0.075f,	0.5f},	{ 0.125f,	 0.0f,		0.5f}},
{{ 0.125f,	 0.0f,		0.5f},	{ 0.0625f,	-0.075f,	0.5f}},
{{ 0.0625f,	-0.075f,	0.5f},	{-0.0625f,	-0.075f,	0.5f}},
{{-0.0625f,	-0.075f,	0.5f},	{-0.125f,	 0.0f,		0.5f}},

// Connect Bottom to Middle Ring
{{-0.125f,	 0.0f,		0.5f},	{-0.50f,	 0.0f,		0.99f}},
{{-0.0625f,	 0.075f,	0.5f},	{-0.25f,	 0.433f,	0.99f}},
{{ 0.0625f,	 0.075f,	0.5f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.125f,	 0.0f,		0.5f},	{ 0.50f,	-0.0f,		0.99f}},
{{ 0.0625f,	-0.075f,	0.5f},	{ 0.25f,	-0.433f,	0.99f}},
{{-0.0625f,	-0.075f,	0.5f},	{-0.25f,	-0.433f,	0.99f}},

// Middle Ring
{{-0.50f,	 0.0f,		0.99f},	{-0.25f,	 0.433f,	0.99f}},
{{-0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	0.99f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.50f,	 0.0f,		0.99f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.25f,	-0.433f,	0.99f}},
{{ 0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	0.99f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.50f,	 0.0f,		0.99f}},

// Connect Top to Middle Rings

{{-0.50f,	 0.0f,		0.99f},	{-0.50f,	 0.0f,		1.8f}},
{{-0.25f,	 0.433f,	0.99f},	{-0.25f,	 0.433f,	1.8f}},
{{ 0.25f,	 0.433f,	0.99f},	{ 0.25f,	 0.433f,	1.8f}},
{{ 0.50f,	 0.0f,		0.99f},	{ 0.50f,	 0.0f,		1.8f}},
{{ 0.25f,	-0.433f,	0.99f},	{ 0.25f,	-0.433f,	1.8f}},
{{-0.25f,	-0.433f,	0.99f},	{-0.25f,	-0.433f,	1.8f}},
	// prone cage
// Bottom Ring
{{-0.50f,	 0.0f,		0.2f},	{-0.25f,	 0.433f,	0.2f}},
{{-0.25f,	 0.433f,	0.2f},	{ 0.25f,	 0.433f,	0.2f}},
{{ 0.25f,	 0.433f,	0.2f},	{ 0.50f,	 0.0f,		0.2f}},
{{ 0.50f,	 0.0f,		0.2f},	{ 0.25f,	-0.433f,	0.2f}},
{{ 0.25f,	-0.433f,	0.2f},	{-0.25f,	-0.433f,	0.2f}},
{{-0.25f,	-0.433f,	0.2f},	{-0.50f,	 0.0f,		0.2f}},

// Connect Bottom to Top Rings
{{-0.50f,	 0.0f,		0.2f},	{-0.50f,	 0.0f,		0.6f}},
{{-0.25f,	 0.433f,	0.2f},	{-0.25f,	 0.433f,	0.6f}},
{{ 0.25f,	 0.433f,	0.2f},	{ 0.25f,	 0.433f,	0.6f}},
{{ 0.50f,	 0.0f,		0.2f},	{ 0.50f,	 0.0f,		0.6f}},
{{ 0.25f,	-0.433f,	0.2f},	{ 0.25f,	-0.433f,	0.6f}},
{{-0.25f,	-0.433f,	0.2f},	{-0.25f,	-0.433f,	0.6f}},

// Top Ring
{{-0.50f,	 0.0f,		0.6f},	{-0.25f,	 0.433f,	0.6f}},
{{-0.25f,	 0.433f,	0.6f},	{ 0.25f,	 0.433f,	0.6f}},
{{ 0.25f,	 0.433f,	0.6f},	{ 0.50f,	 0.0f,		0.6f}},
{{ 0.50f,	 0.0f,		0.6f},	{ 0.25f,	-0.433f,	0.6f}},
{{ 0.25f,	-0.433f,	0.6f},	{-0.25f,	-0.433f,	0.6f}},
{{-0.25f,	-0.433f,	0.6f},	{-0.50f,	 0.0f,		0.6f}},
};


#define			_FPS_MAX_COLLIDE_LINES _FPS_NUM_COLLIDE_LINES_TRANSITION

UINT32 mlDetermineInfantryCollisions(mlVehicleData*	VehicleData,
									 FLOAT64VECTOR*	Vel,
									 UINT32			poseToCheck,
									 UINT32			vehicleCollideCheck,
									 UINT32			Flags,
									vehVehicle**	collideVehicle)
{
	static const UINT32 _MAX_COLLISIONS = 32;
	static trnCDTerrainObject* _lastCollision = nullptr;

	UINT32				i,j;

	// Line Collider Info
	UINT32				NumLines = 0;
	piVECTOR			(*pCollideLines)[2];
	piVECTOR*			pCollideCenterPnt = nullptr;

	static FASTCOLLIDE			CollisionList[_MAX_COLLISIONS];
	FASTCOLLIDE*			CollideListPtr = nullptr;
	UINT32				TempNum = 0;
	piVECTOR			OldPos = {0, 0, 0};
    piVECTOR            NewPos = {0, 0, 0};
	FLOAT64VECTOR		WorldOffsetOrigin;
	FLOAT64				Len;
	UINT32				NumCollisions = 0;
    piVECTOR			Norm = {0, 0, 0};
    piVECTOR			TempVec = {0, 0, 0};
	piMATRIX			Mat = IdentityMatrix;
	UINT32				RetVal = 0;
	UINT32				Alpha = 0;
	LOGICAL				Found = 0;
    piVECTOR			trooperPos = {0, 0, 0};
	trnCDTerrainObject*	TO1 = nullptr;
	vehVehicle*			Vehicle = nullptr;
    vehVehicle*          RideVehicle = nullptr;
    piVECTOR			collidePoint = {0, 0, 0};
    piVECTOR            collideVector = {0, 0, 0};


//static SINT32 _LastNukeSendTime; // the "nuke" is the damage done to capture object when collided

// PARANOID_COLLIDER changes how the fudge is applied, and checks to make sure we're not
// both fudging and correcting in conflicting directions
//
// right now, it's a bit too sensitive, it prevents folks from sliding along walls
//

#ifdef PARANOID_COLLIDER
	piVECTOR			cumulativeDisplacementVelocity;
	piVECTOR			cumulativeFudgeVelocity;
	UINT32				xConflictFlag, yConflictFlag;
#endif

#ifdef BETA_RELEASE
	FLOAT64VECTOR		originalVelocity;
	static UINT32		debugCounter;
#endif

	if (VehicleData == nullptr)
	{
		return 0;
	}
	if (VehicleData->VehicleObjectData == nullptr)
	{
		return 0;
	}
	if (VehicleData->VehicleObjectData->PlaneObject == nullptr)
	{
		return 0;
	}

	// don't do vehicle collisions if riding vehicle
	if(VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0)
	{
		if(VehicleData->VehicleMasterPtr->RideSeatNum != VEH_TROOPER_ON_DECK_RIDE_SEAT)
			return 0;

		RideVehicle = vehFindVehicle(VehicleData->VehicleMasterPtr->RideVehicleHostIndex);
	}

#ifdef BETA_RELEASE
	originalVelocity = *Vel;
	debugCounter++;
#endif

#ifdef PARANOID_COLLIDER
	cumulativeDisplacementVelocity.x = 0;
	cumulativeDisplacementVelocity.y = 0;
	cumulativeDisplacementVelocity.z = 0;
	cumulativeFudgeVelocity.x = 0;
	cumulativeFudgeVelocity.y = 0;
	cumulativeFudgeVelocity.z = 0;
	xConflictFlag = yConflictFlag = 0;
#endif

	switch (poseToCheck)
	{
	case (UINT32)POSE_STAND:
		NumLines =			 _FPS_NUM_COLLIDE_LINES_STAND;
		pCollideLines =		 _FPSCollideLinesStand;
		pCollideCenterPnt =	&_FPSCollideCenterPntStand;
		break;
	case (UINT32)POSE_CROUCH:
		NumLines =			 _FPS_NUM_COLLIDE_LINES_KNEEL;
		pCollideLines =		 _FPSCollideLinesKneel;
		pCollideCenterPnt = &_FPSCollideCenterPntKneel;
		break;
	case (UINT32)POSE_PRONE:
		NumLines =			 _FPS_NUM_COLLIDE_LINES_PRONE;
		pCollideLines =		 _FPSCollideLinesProne;
		pCollideCenterPnt = &_FPSCollideCenterPntProne;
		break;
	case (UINT32)POSE_TRANSITION:
		NumLines =			 _FPS_NUM_COLLIDE_LINES_TRANSITION;
		pCollideLines =		 _FPSCollideLinesTransition;
		pCollideCenterPnt = &_FPSCollideCenterPntTransition;
		break;
	default:
		// Not a position we know anything about
		return 0;
	}

	static FASTCOLLIDE collisionsSaveList[_FPS_MAX_COLLIDE_LINES];

	memset (collisionsSaveList, 0, sizeof(FASTCOLLIDE) * NumLines);

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	trooperPos.x = (FLOAT32)(VehicleData->VehiclePos.x - WorldOffsetOrigin.x);
	trooperPos.y = (FLOAT32)(VehicleData->VehiclePos.y - WorldOffsetOrigin.y);
	trooperPos.z = -(FLOAT32)(VehicleData->VehiclePos.z - WorldOffsetOrigin.z);


#ifdef BETA_RELEASE
	{
		piVECTOR V1;

		V1.x = trooperPos.x;
		V1.y = trooperPos.y;
		V1.z = trooperPos.z;

		draw3dDebugMarkPoint (&V1, colorWHITEALPHA, 0.15f);
	}
#endif

    // TODO: Check if bounding box collides with terrain objects bounding box here.
    // If collides, then do the NumLines check.

	//UINT32 category = 0; //COLLIDER_MANMADE_MASK|COLLIDER_TRN_TREE|COLLIDER_VEH_HULK;

	for (i = 0; i < NumLines; ++i)
	{
		Mat = VehicleData->VehicleObjectData->PlaneObject->mat;
		Mat._41 = Mat._42 = Mat._43 = 0.0f;

		if(fpsTrooper.riverExit)
		{
			piVECTOR v0 = pCollideLines[i][0];
			piVECTOR v1 = pCollideLines[i][1];

			static FLOAT32 move = .5f;

			v0.z += move;
			v1.z += move;

			math3dVectorMultiply(&OldPos,&v0,&Mat);
			math3dVectorMultiply(&NewPos,&v1,&Mat);
		}
		else
		{
			math3dVectorMultiply(&OldPos,&pCollideLines[i][0],&Mat);
			math3dVectorMultiply(&NewPos,&pCollideLines[i][1],&Mat);
		}

		OldPos.x = (FLOAT32)(trooperPos.x + OldPos.x);
		OldPos.y = (FLOAT32)(trooperPos.y + OldPos.y);
		OldPos.z = -(FLOAT32)((VehicleData->VehiclePos.z - WorldOffsetOrigin.z) + OldPos.z);

		NewPos.x = (FLOAT32)(trooperPos.x + NewPos.x);
		NewPos.y = (FLOAT32)(trooperPos.y + NewPos.y);
		NewPos.z = -(FLOAT32)((VehicleData->VehiclePos.z - WorldOffsetOrigin.z) + NewPos.z);

#ifdef BETA_RELEASE
		draw3dLineDebug(&NewPos, &OldPos, colorBLUEALPHA);
#endif

		TempNum = _MAX_COLLISIONS;

		CollideListPtr = CollisionList;

		if(!Flags || (Flags & COLLIDER_TERRAIN_MASK))
		{
			trncdCollideWithListOfTerrainObjects(&CollideListPtr,
												 &TempNum,
												 VehicleData->ShortTrnList,
												 ML_NUM_SHORT_LIST_ELEMENTS,
												 &OldPos,
												 &NewPos,
												 1,
												 Flags);
		}

		NumCollisions = _MAX_COLLISIONS - TempNum;

		for (j=0;j<NumCollisions;j++)
		{
			CollisionList[j].Flags = 0;
		}

		if ((!Flags || (Flags & COLLIDER_DRIVABLE_MASK)) && NumCollisions < _MAX_COLLISIONS)
		{
			weapCollideRoundWithVehicles(&CollideListPtr,
								 &TempNum,
								 &OldPos,
								 &NewPos,
								 VehicleData->LastVehicleUpdateTime / 1000.0,
								 VehicleData->LastVehicleUpdateTime / 1000.0 + _MS_PER_TIC / 1000.0,
								 0,
								 0,
								 &WorldOffsetOrigin,
								 2,
								 VehicleData->CollisionSide);
		}

		vehVehicle* cv = nullptr;

		for (j=NumCollisions;j<_MAX_COLLISIONS;j++)
		{
			CollisionList[j].Flags = 1;

			if (j < _MAX_COLLISIONS - TempNum)
			{
				Vehicle = (vehVehicle*)CollisionList[j].TopLevelObject;
				if (Vehicle != nullptr)
				{
					if( VehicleData->VehicleMasterPtr->Side != Vehicle->Side)
					{
						if (Vehicle->VehicleType != nullptr)
						{
							// only vehicles can run over infantry
							if (Vehicle->VehicleType->Type != VEH_TYPE_PERSON && Vehicle != RideVehicle)
							{
								if (vehicleCollideCheck)
								{
									math3dVectorMultiply(&collidePoint,
														 &(CollisionList[j].CollidePnt),
														 &(CollisionList[j].CollideObject->mat));

									mathSubVectors(&collideVector, &collidePoint, &trooperPos);

									// shift by the collide center point
									// enable this code when all of the centers are non zero
#if 0
//									math3dMultiplyRotation(&centerPointVector,
//														   pCollideCenterPnt,
//														   &(VehicleData->VehicleObjectData->PlaneObject->mat));

//									mathSubVectors(&collideVector, &centerPointVector, &collideVector);
#endif

									// at this point, collideVector is a vector from the collide center point
									// to where the collision occurred
									//
									// since we don't care about the actual length, just that it's less than a
									// certain value, we don't call mathExactVectorLength
									//
									// since the trooper's widest radius is 0.5 .. we'll compare to 0.45
									// since everything is squared, we'll compare to 0.2025 :)
									//
									// also, insure the vehicle is moving at all

									WriteDebug ("CollideVector %f", collideVector.x * collideVector.x +
										collideVector.y * collideVector.y);

									if ((collideVector.x * collideVector.x +
										collideVector.y * collideVector.y) < 0.2025)
									{
										if ((Vehicle->PosData.Vel.x * Vehicle->PosData.Vel.x +
											 Vehicle->PosData.Vel.y * Vehicle->PosData.Vel.y +
											 Vehicle->PosData.Vel.z * Vehicle->PosData.Vel.z) > 0.0)
										{
//											WriteDebug ("DIE!DIE!DIE!");
											persKillCrew();
										}
									}
								}
								else
								{
									fpFPSMarkForVehicleCollideCheck(TRUE);
								}
							}
						}
					}

					// return the vehicle we collided with
					if(!cv) cv = Vehicle;
				}
			}
		}

		if(collideVehicle && cv) *collideVehicle = cv;

		// Note that weapCollideRoundWithVehicles may have changed* tempnum*
		NumCollisions = _MAX_COLLISIONS - TempNum;

		if (TempNum > 0)
		{
			fpCollideRoundWithVehicleWalkObject(&CollideListPtr, &TempNum, &OldPos, &NewPos);
			NumCollisions = _MAX_COLLISIONS - TempNum;
		}

		for (j=0;j<NumCollisions;j++)
		{
			if (CollisionList[j].Alpha > 0.5f)
			{
				collisionsSaveList[i] = CollisionList[j];

				Found = TRUE;
				TO1 = nullptr;

				if ((frametime - fpFPSGetLastUnrideTime()) > 2500)
				{
					if (CollisionList[j].TopLevelObject != nullptr && CollisionList[j].Flags != 1 && !(CollisionList[j].Flags & _FP_WALK_COLLIDE_FLAG))
					{
						if ((CollisionList[j].TopLevelObject != nullptr) && (CollisionList[j].TopLevelObject->child != nullptr))
						{
							if (CollisionList[j].TopLevelObject->child->lpspare != nullptr)
							{
								TO1 = (trnCDTerrainObject*)(CollisionList[j].TopLevelObject->child->lpspare);

								/* // we no longer capture facilities through bumping
								// make sure we're bumping a capture table (building, capture) and that it's got a radio on it
								// (damagelevel 0)
								if (TO1->ObjId.objClass == CLASS_BUILDING &&
									TO1->ObjId.objType == BUILDING_CAPTURE ) // &&
	//								TO1->DamageLevel == 0)
								{
	//								if (_sideArray[TO1->Country] != ((vehVehicle*)VehicleData->VehicleMasterPtr)->Side)
	//								{
									FULL_OBJ_ID fid = nullObjID;
									populateFullObjID(&fid, TO1->StratSupercellX, TO1->StratSupercellY, TO1->ObjId);
									fpFPSHitCaptureObject(StratDB::FindStratObject(fid));
	//								}
								}
								*/

								if(!IN_COMBAT(30000)) // if we haven't been in combat for at least 30 seconds
								{
									if(TO1->uFlags & TRN_OBJECT_BUILDER)
									{
										FULL_OBJ_ID fid = nullObjID;
										populateFullObjID(&fid, TO1->StratSupercellX, TO1->StratSupercellY, TO1->ObjId);
										fpFPSHitBuilderObject(StratDB::FindStratObject(fid));
									}
									else if(TO1->TerrainObject)
									{
										if(TO1->TerrainObject->flag3 & OBJECT_HAS_POPUP)
										{
											char* p = strstr(TO1->TerrainObject->comment, "popup=");

											if(p)
											{
												p += 6;
												persmsgAddMessage(p, TO1->TerrainObject);
											}
										}
									}
								}

								if(!_lastCollision || _lastCollision != TO1)
								{
									_lastCollision = TO1;
									// alert the manage we hit something
									TUT_MGR->DoEvent(TUT_EVENT_COLLISION, _lastCollision);
								}
							}
						}
					}
				}

				if(!TO1)
					_lastCollision = nullptr;
			}
		}

		if (Found == FALSE)
		{
			if (NumCollisions > 0)
			{
				collisionsSaveList[i] = CollisionList[0];
				Alpha = 1;
			}
		}

	}

	for (i = 0; i < NumLines;i++)
	{
		if(collisionsSaveList[i].Flags == 1 && (vehVehicle*)collisionsSaveList[i].TopLevelObject == RideVehicle)
			continue;

		if (collisionsSaveList[i].Alpha > 0.5f && collisionsSaveList[i].CollideObject)
		{
			// note, Norm has nothing to do with normals .. it's really the vector
			// from the trooper's origin to the collide point
			math3dVectorMultiply(&Norm,&collisionsSaveList[i].CollidePnt,&collisionsSaveList[i].CollideObject->mat);

			mathSubVectors(&Norm,&Norm,&trooperPos);

			// shift by the collide center point
			math3dMultiplyRotation(&TempVec,pCollideCenterPnt,&VehicleData->VehicleObjectData->PlaneObject->mat);

			mathSubVectors(&Norm,&TempVec,&Norm);

			Norm.x = -Norm.x;
			Norm.y = -Norm.y;
			Norm.z = 0.0f;

#ifdef BETA_RELEASE
			{
				piVECTOR	V1, V2;
				if (collisionsSaveList[i].CollideObject->flags & OBJECT_USEMATRIX)
				{
					math3dVectorMultiply(&V1,&collisionsSaveList[i].CollidePnt,&(collisionsSaveList[i].CollideObject->mat));
				}
				else
					V1 = collisionsSaveList[i].CollidePnt;

				V2.x = V1.x - Norm.x;
				V2.y = V1.y - Norm.y;
				V2.z = V1.z;

				draw3dLineDebug(&V1, &V2, colorYELLOWALPHA);
			}
#endif
			math3dNormalize(&Norm);

			Len = mathVectDotProduct(Vel,&Norm);
			if (Len >= 0.0f)
			{
#ifndef PARANOID_COLLIDER
				Vel->x -= Norm.x * Len;
				Vel->y -= Norm.y * Len;
				Vel->z -= Norm.z * Len;

				Vel->x -= Norm.x * 0.1;
				Vel->y -= Norm.y * 0.1;
				Vel->z -= Norm.z * 0.1;
			}

			RetVal = 1;
		}
	}

#else //#ifdef PARANOID_COLLIDER
				float		fudgeFactor = 0.1f;

/*
#ifdef VEHICLE_OBJECT_ANALYSIS
				piVECTOR	correctionImpulse;
				piVECTOR	V1, V2;

				V1 = collisionsSaveList[i].CollidePnt;
				if (collisionsSaveList[i].CollideObject->flags & OBJECT_USEMATRIX)
				{
					math3dVectorMultiply(&V1,&V1,&(collisionsSaveList[i].CollideObject->mat));
				}

				correctionImpulse.x = Norm.x * ((float)Len + fudgeFactor);
				correctionImpulse.y = Norm.y * ((float)Len + fudgeFactor);

				V2.x = V1.x - correctionImpulse.x;
				V2.y = V1.y - correctionImpulse.y;
				V2.z = V1.z;

//				draw3dLine(&V1, &V2, colorGREENALPHA);
				V2.x = trooperPos.x;
				V2.y = trooperPos.y;
				V2.z = trooperPos.z;

				mathSubVectors(&V2,&V2,&TempVec);
				draw3dLine(&V1, &V2, colorREDALPHA);
#endif
*/

				// check to see if the correction we're applying is counter to any previous
				// correction we've applied .. if it is, set the conflict flag
				// if the result of 2 multiplied numbers is positive, they have the same sign
				// if not, they have opposite signs

				if (((Norm.x * Len * -1.0) * cumulativeDisplacementVelocity.x) < 0.0f)
				{
					xConflictFlag = 1;
				}
				if (((Norm.y * Len * -1.0) * cumulativeDisplacementVelocity.y) < 0.0f)
				{
					yConflictFlag = 1;
				}

				cumulativeDisplacementVelocity.x -= Norm.x * (FLOAT32)Len;
				cumulativeDisplacementVelocity.y -= Norm.y * (FLOAT32)Len;

				cumulativeFudgeVelocity.x -= Norm.x * fudgeFactor;
				cumulativeFudgeVelocity.y -= Norm.y * fudgeFactor;
			}

			RetVal = 1;

#ifdef BETA_RELEASE
			{
				piVECTOR			V1;
				if (collisionsSaveList[i].Alpha > 0.5f)
				{
					if (collisionsSaveList[i].CollideObject->flags & OBJECT_USEMATRIX)
					{
						math3dVectorMultiply(&V1,&collisionsSaveList[i].CollidePnt,&collisionsSaveList[i].CollideObject->mat);
					}
					else
						V1 = collisionsSaveList[i].CollidePnt;
					draw3dDebugMarkPoint (&V1, colorCYANALPHA, 0.25f);
				}
			}
#endif

		}
	}

	//
	// before we change Vel, use it to determine where we started
	//
	// since trooperPos already is the origin of our trooper, just reuse it
	// so trooperPos becomes the origin of our trooper before the velocity is applied
	//

	trooperPos.x -= (FLOAT32)Vel->x;
	trooperPos.y -= (FLOAT32)Vel->y;
	trooperPos.z -= (FLOAT32)Vel->z;

	// now apply the calculated velocity changes


	if (xConflictFlag)
	{
		Vel->x = 0;
		WriteDebug ("X Conflict, velocity Zeroed");
	}
	else
	{
		if ((cumulativeDisplacementVelocity.x * cumulativeDisplacementVelocity.x)
			 > (Vel->x * Vel->x))
		{
			Vel->x = 0;
		}
		else
		{
			Vel->x += cumulativeDisplacementVelocity.x;
		}

		if (cumulativeFudgeVelocity.x > 0.1f)
			cumulativeFudgeVelocity.x = 0.1f;
		else if (cumulativeFudgeVelocity.x < -0.1f)
			cumulativeFudgeVelocity.x = -0.1f;

		Vel->x += cumulativeFudgeVelocity.x;
	}
	if (yConflictFlag)
	{
		Vel->y = 0;
		WriteDebug ("Y Conflict, velocity Zeroed");
	}
	else
	{
		if ((cumulativeDisplacementVelocity.y * cumulativeDisplacementVelocity.y)
			 > (Vel->y * Vel->y))
		{
			Vel->y = 0;
		}
		else
		{
			Vel->y += cumulativeDisplacementVelocity.y;
		}
		if (cumulativeFudgeVelocity.y > 0.1f)
			cumulativeFudgeVelocity.y = 0.1f;
		else if (cumulativeFudgeVelocity.y < -0.1f)
			cumulativeFudgeVelocity.y = -0.1f;

		Vel->y += cumulativeFudgeVelocity.y;
	}


	// and now .. some sanity checking
	// for each collision point, get the collision triangle, and using
	// that triangle, make sure that the center point of our object didn't move through it
	//
	// yes I'm rewriting a basic ray/surface collider here
	//
	// not finished yet, may not be necessary
	//

//#define COLLISION_SANITY_CHECK
#undef COLLISION_SANITY_CHECK
#if COLLISION_SANITY_CHECK
	{
		// variables I'm only using here
		FLOAT64VECTOR	newVelocity;
		piVECTOR		startPoint;	// point we know to be good
		piVECTOR		endPoint;	// where our velocity will take us
		FLOAT32			dot1, dot2;	// dot products
		piVECTOR		ray1, ray2;	// rays from trianglePoint to startPoint and endPoint
		piVECTOR		surfaceNormal;
		piVECTOR*		trianglePoint;	// one of the vertices of the triangle
		OBJECT*			collideObj;

		startPoint = trooperPos;

		endPoint.x = startPoint.x + (FLOAT32)Vel->x;
		endPoint.y = startPoint.y + (FLOAT32)Vel->y;
		endPoint.z = startPoint.z + (FLOAT32)Vel->z;

		newVelocity = *Vel;

		for (i = 0; i < NumLines;i++)
		{
			if (collisionsSaveList[i].Alpha > 0.5f)
			{
				collideObj = collisionsSaveList[i].CollideObject;
				surfaceNormal = collideObj->psnorms[collisionsSaveList[i].PolyNum];
				trianglePoint = (piVECTOR*)&(collideObj->pdlverts[(collideObj->ptris[collisionsSaveList[i].PolyNum]).v1]);
				// generate ray from start point to triangle point
				mathSubVectors(&ray1,&startPoint,trianglePoint);
				// dot product tells us "which way"
				dot1 = math3dDotProduct(&ray1, &surfaceNormal);
				// sanity check
				if (dot1 < 0.001f && dot1 > -0.001f)
				{
					continue;
				}
				// do the same for the end point
				mathSubVectors(&ray2,&endPoint,trianglePoint);
				dot2 = math3dDotProduct(&ray2, &surfaceNormal);
				// if the signs of the dot products are the same, we're on the same side .. so multiply them
				// and if they come up negative, we collided
				if (dot1 * dot2 < 0)
				{

				}
				// no collision, continue down list
			}
		}

		Vel->x = endPoint.x - startPoint.x;
		Vel->y = endPoint.y - startPoint.y;
	}
#endif
/*
#ifdef VEHICLE_OBJECT_ANALYSIS
	if (((originalVelocity.x != 0.0) || (originalVelocity.y != 0.0) || (Vel->x != 0.0) || (Vel->y != 0.0))
		&& ((originalVelocity.x != Vel->x) || (originalVelocity.y != Vel->y))
		)
	WriteDebug ("Eye (%lf, %lf) .. Initial (%lf, %lf) .. End (%lf, %lf) .. Fudge (%lf, %lf)",
		eyePoint.x, eyePoint.y,
		originalVelocity.x, originalVelocity.y,
		Vel->x, Vel->y,
		cumulativeFudgeVelocity.x, cumulativeFudgeVelocity.y);
#endif
*/
#endif // PARANOID_COLLIDER

	return RetVal;
}

void mlInfantryCheckLeanLines (mlVehicleData* VehicleData)
{
	// Line Collider Info
	piVECTOR			startPos, endPos;
	FLOAT64VECTOR		WorldOffsetOrigin;
	piMATRIX			troopMat;
	piVECTOR			trooperPos;

	if (VehicleData == nullptr) return;
	if (VehicleData->VehicleObjectData == nullptr) return;
	if (VehicleData->VehicleObjectData->PlaneObject == nullptr) return;

	if (!fpFPSGetLeanPoints(&startPos, &endPos))
		return;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	trooperPos.x = (FLOAT32)(VehicleData->VehiclePos.x - WorldOffsetOrigin.x);
	trooperPos.y = (FLOAT32)(VehicleData->VehiclePos.y - WorldOffsetOrigin.y);
	trooperPos.z = -(FLOAT32)(VehicleData->VehiclePos.z - WorldOffsetOrigin.z);

	troopMat = VehicleData->VehicleObjectData->PlaneObject->mat;
	troopMat._41 = troopMat._42 = troopMat._43 = 0.0f;

	math3dVectorMultiply(&startPos,&startPos,&troopMat);
	startPos.x = (FLOAT32)(trooperPos.x + startPos.x);
	startPos.y = (FLOAT32)(trooperPos.y + startPos.y);
	startPos.z = -(FLOAT32)((VehicleData->VehiclePos.z - WorldOffsetOrigin.z) + startPos.z);

	math3dVectorMultiply(&endPos,&endPos,&troopMat);
	endPos.x = (FLOAT32)(trooperPos.x + endPos.x);
	endPos.y = (FLOAT32)(trooperPos.y + endPos.y);
	endPos.z = -(FLOAT32)((VehicleData->VehiclePos.z - WorldOffsetOrigin.z) + endPos.z);

#ifdef BETA_RELEASE
	draw3dLineDebug(&startPos, &endPos, colorMAGENTAALPHA);
#endif

	if (trncdCollideWithVehiclesAndTerrain(&startPos, &endPos, nullptr, nullptr, nullptr, nullptr, nullptr, FALSE, FALSE, TRUE) == TRUE)
	{
		fpFPSLeanOff();
	}
}

/************************************************************************************************
	mlInfantryUpdate
*************************************************************************************************/

UINT32 mlInfantryUpdate(mlVehicleData* VehicleData, SINT32 TicTime)
{
	UINT32 Hit = 0;
	SINT32 j;

	for (j=1; j < VEH_MAX_DAMAGE_OBJECTS; j++)
	{
		if (VehicleData->NumRoundsHit[j] > 0)
		{
			SINT32 location = gr2AnimHitMapLocation(j-1);

			if(location >= 0 && location < FP_HIT_LOCATION_NUM)
			{
				// -1 to hit location because it's STUPID and is 1 based
				persApplyDamage(VehicleData->DamageAccrued[j], 0, location, false);
				Hit = 1;
				// turn off lean on damage
				fpFPSLeanOff();
			}
		}
	}

	fpUpdatePerson(VehicleData,TicTime / 1000.0);

	// clear out damage que list
	if (VehicleData->NewDamage == 1)
	{
		for (j=0;j<VEH_MAX_DAMAGE_OBJECTS;j++)
		{
			VehicleData->DamageAccrued[j] = 0.0;
			VehicleData->NumRoundsHit[j] = 0;
		}
		VehicleData->NewDamage = 0;
	}

	if (VehicleData->KillCrew == TRUE || VehicleData->LastKillCrew == TRUE)
	{
		vehApplyDamageToVehicleComponent(VehicleData->VehicleMasterPtr,
								  1,
								  10000,
								  frametime);
	}


	// check for dead guy
	if (!VehicleData->VehicleIsDead && persGetCrewDeadForKillScore(0) == 1)
	{
		VehicleData->VehicleIsDead = 1;
		vehupdateUpdate(TRUE); // send it right away
	}

return 0;
}

#ifdef STOP_FIRING_INSIDE

//
// this just checks the bounding box of all enemy vehicles to see if our eyepoint is inside of one of them
//
// this is used to supress firing of weapons if inside an enemy vehicle .. i.e. clipping
//
SINT32 mlInfantryInsideVehicleCheck (mlVehicleData* pVehicleData, piVECTOR* eyePoint)
{
	if (!pVehicleData)
		return FALSE;

	piVECTOR		trooperPos;
	FLOAT64VECTOR	WorldOffsetOrigin;
	vehVehicle*		CurrentVehicle;
	vehVehicle*		PlayerVehicle = persGetPersonaVehiclePtr();
//	OBJECT*			colObject;
	FLOAT32			minx, miny, minz, maxx, maxy, maxz;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	trooperPos.x = (FLOAT32)(pVehicleData->VehiclePos.x - WorldOffsetOrigin.x) + eyePoint->x;
	trooperPos.y = (FLOAT32)(pVehicleData->VehiclePos.y - WorldOffsetOrigin.y) + eyePoint->y;
	trooperPos.z = -(FLOAT32)(pVehicleData->VehiclePos.z - WorldOffsetOrigin.z) + eyePoint->z;

	vehGetVehicleHead(&CurrentVehicle);

	for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->Next)
	{
		if (CurrentVehicle == PlayerVehicle) continue;
		if (CurrentVehicle->VehicleObjectData == nullptr) continue;
		if ((pVehicleData->CollisionSide > 0) && (pVehicleData->CollisionSide == CurrentVehicle->Side)) continue;
		if (CurrentVehicle->VehicleType->Type != VEH_TYPE_GROUND) continue;
/*
		if (CurrentVehicle->VehicleObjectData->HitBox != nullptr)
			colObject = CurrentVehicle->VehicleObjectData->HitBox;
		else if (CurrentVehicle->VehicleObjectData->CollideExternObject)
			colObject = CurrentVehicle->VehicleObjectData->CollideExternObject;
		else if (CurrentVehicle->VehicleObjectData->CollideInternObject)
			colObject = CurrentVehicle->VehicleObjectData->CollideInternObject;
		else
		{
			WriteLog (LOG_FPS, LOG_INFO, "No Bounding Box found for collision!!");
			continue;
		}

		minx = colObject->bbox[0].x;
		miny = colObject->bbox[0].y;
		minz = colObject->bbox[0].z;

		maxx = colObject->bbox[1].x;
		maxy = colObject->bbox[1].y;
		maxz = colObject->bbox[1].z;
*/
		minx = CurrentVehicle->VehicleObjectData->BBox1.x;
		miny = CurrentVehicle->VehicleObjectData->BBox1.y;
		minz = CurrentVehicle->VehicleObjectData->BBox1.z;

		maxx = CurrentVehicle->VehicleObjectData->BBox2.x;
		maxy = CurrentVehicle->VehicleObjectData->BBox2.y;
		maxz = CurrentVehicle->VehicleObjectData->BBox2.z;

		if ((trooperPos.x > minx) && (trooperPos.x < maxx))
			return TRUE;
		if ((trooperPos.y > miny) && (trooperPos.y < maxy))
			return TRUE;
		if ((trooperPos.z > minz) && (trooperPos.z < maxz))
			return TRUE;
	}

	return FALSE;
}

#endif
