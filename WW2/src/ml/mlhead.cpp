#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"

#include <math.h>

#include "math/mathvect.h"
#include "ml/mlhead.h"
#include "ml/mlutility.h"
#include "ml/mltypes.h"
#include "math/math3d.h"
#include "pers/persmain.h"
#include "imap/ww2func.h"
#include "imaplib/imapTypes.h"
#include "fp/fpFPS.h"
#include "tut/TutorialManager.h"

/************************************************************************************************
	mlHead
*************************************************************************************************/

// 1g new lateral movements
#define _1G_FRONTBACK_MOVEMENT 0.04
#define _1G_LEFTRIGHT_MOVEMENT 0.04
#define _1G_TOPBOTTOM_MOVEMENT 0.004

#define _LEFTRIGHT_G_CAP 0.04
#define _FRONTBACK_G_CAP 0.04
#define _TOPBOTTOM_G_CAP 0.05

#define _HEAD_SHOCK_VAL 20.0

#define _NUM_TICS_PER_SEC 250
#define _TIC_S 1.0 / _NUM_TICS_PER_SEC
#define _TIC_MS 1000 / _NUM_TICS_PER_SEC
#define _DELTA_TIME 0.001

#define NUM_VIEW_POS (IMAP_LOOK_END - IMAP_LOOK_START)

#define YAW_LIMIT 			(3.0 / 4.0 * PI)
#define PITCH_DOWN_LIMIT	(-PI / 4.0)
#define PITCH_UP_LIMIT		(PI / 2.0)

struct mlHeadAttitude
{
	FLOAT64ATT head;
	FLOAT64ATT eye;
	UINT32 functionCode;
	SINT32 frontBack;
	SINT32 leftRight;
	SINT32 upDown;
	SINT32 enabled;
};

static SINT32 _curView = 0;
static LOGICAL _mouseLook;
static FLOAT64ATT _mouseLookAtt;
static LOGICAL _mouseLookAndTorsoUseSameKeymap;
static LOGICAL _absHeadPos;

static mlHeadAttitude _HeadAttitudes[NUM_VIEW_POS] =
{
	{{0,0,0},			{0,0,0},			IMAP_LOOK_FORWARD, 				 1, 0, 0,	0},
	{{0,0,-PI/2},		{0,0,-PI/4},		IMAP_LOOK_BACK,					-1, 0, 0,	0}, // actual view is back left

	{{0,PI/4,0},		{0,PI/4,0},			IMAP_LOOK_UP,					 0, 0, 1,	0},
	{{0,-PI/8,0},		{0,-PI/8,0},		IMAP_LOOK_DOWN,					 0, 0,-1,	0},

	{{0, PI/4,0},		{0,0,0},			IMAP_LOOK_FORWARD_UP,			 1, 0, 1,	0},
	{{0,-PI/8,0},		{0,-PI/8,0},		IMAP_LOOK_FORWARD_DOWN,			 1, 0,-1,	0},

	{{0,0,PI/2},		{0,0,0},			IMAP_LOOK_RIGHT,				 0, 1, 0,	0},
	{{0,0,-PI/2},		{0,0,0},			IMAP_LOOK_LEFT,					 0,-1, 0,	0},

	{{0,0, PI/4},		{0,0,0},			IMAP_LOOK_FORWARD_RIGHT,		 1, 1, 0,	0},
	{{0,0,-PI/4},		{0,0,0},			IMAP_LOOK_FORWARD_LEFT,			 1,-1, 0,	0},
	{{0,0, PI/2},		{0,0, PI/4},		IMAP_LOOK_BACK_RIGHT,			-1, 1, 0,	0},
	{{0,0,-PI/2},		{0,0,-PI/4},		IMAP_LOOK_BACK_LEFT,			-1,-1, 0,	0},

	{{0,-PI/8, PI/2},	{0,-PI/8,0},		IMAP_LOOK_RIGHT_DOWN,			 0, 1,-1,	0},
	{{0,-PI/8,-PI/2},	{0,-PI/8,0},		IMAP_LOOK_LEFT_DOWN,			 0,-1,-1,	0},

	{{0,PI/4, PI/2},	{0,0,0},			IMAP_LOOK_RIGHT_UP,				 0, 1, 1,	0},
	{{0,PI/4,-PI/2},	{0,0,0},			IMAP_LOOK_LEFT_UP,				 0,-1, 1,	0},

	{{0,-PI/8, PI/4},	{0,-PI/8,0},		IMAP_LOOK_FORWARD_RIGHT_DOWN,	 1, 1,-1,	0},
	{{0,-PI/8,-PI/4},	{0,-PI/8,0},		IMAP_LOOK_FORWARD_LEFT_DOWN,	 1,-1,-1,	0},
	{{0,-PI/8, PI/2},	{0,-PI/8, PI/4},	IMAP_LOOK_BACK_RIGHT_DOWN,		-1, 1,-1,	0},
	{{0,-PI/8,-PI/2},	{0,-PI/8,-PI/4},	IMAP_LOOK_BACK_LEFT_DOWN,		-1,-1,-1,	0},

	{{0,PI/4, PI/4},	{0,0,0},			IMAP_LOOK_FORWARD_RIGHT_UP,		 1, 1, 1,	0},
	{{0,PI/4,-PI/4},	{0,0,0},			IMAP_LOOK_FORWARD_LEFT_UP,		 1,-1, 1,	0},
	{{0,PI/4 ,PI/2},	{0,0, PI/4},		IMAP_LOOK_BACK_RIGHT_UP,		-1, 1, 1,	0},
	{{0,PI/4,-PI/2},	{0,0,-PI/4},		IMAP_LOOK_BACK_LEFT_UP,			-1,-1, 1,	0},

	{{0,-PI/8,-PI/2},	{0,-PI/8,-PI/4},	IMAP_LOOK_BACK_DOWN,			-1, 0,-1,	0}, // actual view is back left down
	{{0,PI/4,-PI/2},	{0,0,-PI/4},		IMAP_LOOK_BACK_UP,				-1, 0, 1,	0}, // actual view is back left up
};

static SINT32 _FrontBackState;
static SINT32 _LeftRightState;
static SINT32 _UpDownState;

/************************************************************************************************
	mlheadGetHeadView
*************************************************************************************************/

UINT32 mlheadGetHeadView(FLOAT64ATT * HeadAtt, FLOAT64ATT * EyeAtt)
{
	if(_mouseLook)
	{
		FLOAT64 maxEyePitch = PI/4;

		EyeAtt->Roll = HeadAtt->Roll = 0;

		if(_mouseLookAtt.Yaw > PI/2)
		{
			HeadAtt->Yaw = PI/2;
			EyeAtt->Yaw = _mouseLookAtt.Yaw - PI/2;
			maxEyePitch = (PI/4 - EyeAtt->Yaw);
		}else if(_mouseLookAtt.Yaw < -PI/2)
		{
			HeadAtt->Yaw = -PI/2;
			EyeAtt->Yaw = _mouseLookAtt.Yaw + PI/2;
			maxEyePitch = (PI/4 + EyeAtt->Yaw);
		}else
		{
			HeadAtt->Yaw = _mouseLookAtt.Yaw;
			EyeAtt->Yaw = 0;
		}

		if(_mouseLookAtt.Pitch > PI/4)
		{
			HeadAtt->Pitch = PI/4;
			EyeAtt->Pitch = _mouseLookAtt.Pitch - PI/4;
		}else if(_mouseLookAtt.Pitch < -PI/8)
		{
			HeadAtt->Pitch = -PI/8;
			EyeAtt->Pitch = _mouseLookAtt.Pitch + PI/8;
		}else
		{
			HeadAtt->Pitch = _mouseLookAtt.Pitch;
			EyeAtt->Pitch = 0;
		}

		if(EyeAtt->Pitch > maxEyePitch)
			EyeAtt->Pitch = maxEyePitch;
	}else
	{
		*HeadAtt = _HeadAttitudes[_curView].head;
		*EyeAtt  = _HeadAttitudes[_curView].eye;
	}

	return 0;
}

/************************************************************************************************
	mlSetHeadViewCallback
*************************************************************************************************/

UINT32 mlSetHeadViewCallback(SINT32 keyState, UINT32 functionNum, UINT32 unused)
{
	if(vehmainHasBailedOut())
		return 1;

	int i;
	if(functionNum >= IMAP_LOOK_START && functionNum < IMAP_LOOK_END)
	{
		_curView = 0;
		_FrontBackState = _LeftRightState = _UpDownState = 0;

		// enable, disable attitude
		for(i = 0; i < NUM_VIEW_POS; i++)
		{
			if(_HeadAttitudes[i].functionCode == functionNum)
			{
				_HeadAttitudes[i].enabled = keyState;
				break;
			}
		}

		// get states from all enabled attitudes
		// we do this so we can combine functions to get diagonal and up/down views from combinations of 4 way functions
		// i.e. pressing forward + right keys equivalent to forward + right function
		// even if no key mapped specifically to forward + right function
		for(i = 0; i < NUM_VIEW_POS; i++)
		{
			if(_HeadAttitudes[i].enabled)
			{
				if(_HeadAttitudes[i].frontBack)
					_FrontBackState	= _HeadAttitudes[i].frontBack;

				if(_HeadAttitudes[i].leftRight)
					_LeftRightState	= _HeadAttitudes[i].leftRight;

				if(_HeadAttitudes[i].upDown)
					_UpDownState 	= _HeadAttitudes[i].upDown;
			}
		}


		// third times a charm. see which view matches.
		for(i = 0; i < NUM_VIEW_POS; i++)
		{
			if( _HeadAttitudes[i].frontBack == _FrontBackState &&
				_HeadAttitudes[i].leftRight == _LeftRightState &&
				_HeadAttitudes[i].upDown 	== _UpDownState)
			{
				_curView = i;
				break;
			}
		}
		persSetInternalCamera(1,0,0);

		// let the tutorial system know about head movements
		TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionNum, &keyState);
	}

	return 1;
}

/************************************************************************************************
	mlGetHeadPos
*************************************************************************************************/
// Displacement is relative to the world, this is different from before.

UINT32 mlGetHeadPos(FLOAT64VECTOR* Displacement, FLOAT64ATT* AttChange,
					mlVehicleData* VehicleData, FLOAT64VECTOR* NeckPos,  piMATRIX* NeckAttMat)
{
	FLOAT64VECTOR Gravity;
	FLOAT64VECTOR Acc;

	FLOAT64VECTOR TempAcc;
	FLOAT64ATT DeltaAtt;
	FLOAT64VECTOR Pos1,Pos2;
	FLOAT64VECTOR Velocity;
	FLOAT64VECTOR TempVec;
	FLOAT64 Pos;
	FLOAT64 Vel;
	FLOAT64 LastVel;
	FLOAT64 Length;
	FLOAT64 TempNum;
	FLOAT64VECTOR AccTotal;

	FLOAT64VECTOR SpringCo;

	FLOAT64 DeltaTime;
	UINT32 NumTics;
	piMATRIX InvNeckAttMat;
	UINT32 i;

	FLOAT32 Strength;
	LOGICAL DummyLogical;

//	static FLOAT64VECTOR _LastAcc;
	static FLOAT64VECTOR _LastPos;
	static FLOAT64VECTOR _LastVel;
	static FLOAT64 _LastTime;

	if (VehicleData == nullptr)
	{
		return 0;
	}

	math3dInvertMatrix(&InvNeckAttMat,NeckAttMat);

	Displacement->x = Displacement->y = Displacement->z = 0.0;
	AttChange->Pitch = AttChange->Roll = AttChange->Yaw = 0.0;

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis) return 0;
#endif

	// compute gravity force
	Gravity.x = Gravity.y = 0.0;
	Gravity.z = -9.8;

	// compute straight acceleration
	if (VehicleData->AccCount > 0)
	{
		Acc.x = VehicleData->SavedAcc.x / VehicleData->AccCount;
		Acc.y = VehicleData->SavedAcc.y / VehicleData->AccCount;
		Acc.z = VehicleData->SavedAcc.z / VehicleData->AccCount;
	}
	else
	{
		Acc = VehicleData->VehicleAcc;
	}

	VehicleData->AccCount = 0;
	VehicleData->SavedAcc.x = VehicleData->SavedAcc.y = VehicleData->SavedAcc.z = 0.0;

	Acc.x = -Acc.x;
	Acc.z = -Acc.z;

	AccTotal = Acc;

	// compute centrifugal acceleration
	Pos1 = Pos2 = *NeckPos;
	Length = mathExactVectorLength(&Pos1);
	DeltaAtt.Pitch = VehicleData->VehicleAttVel.Pitch / 100.0;
	DeltaAtt.Roll = VehicleData->VehicleAttVel.Roll / 100.0;
	DeltaAtt.Yaw = VehicleData->VehicleAttVel.Yaw / 100.0;
	mathRotateVectorByAtt(&Pos2,&Pos2,&DeltaAtt);
	mathSubVectors(&Velocity,&Pos2,&Pos1);
	Velocity.x *= 100.0;
	Velocity.y *= 100.0;
	Velocity.z *= 100.0;
	TempVec.x = NeckPos->x / Length;
	TempVec.y = NeckPos->y / Length;
	TempVec.z = NeckPos->z / Length;
	mathVectDoCrossProduct(&TempAcc,&Velocity,&TempVec);
	TempNum = mathExactVectorLength(&TempAcc);

	Acc.x = NeckPos->x * TempNum * TempNum / Length;
	Acc.y = NeckPos->y * TempNum * TempNum / Length;
	Acc.z = NeckPos->z * TempNum * TempNum / Length;
	mathVectorMultiply64(&Acc,&Acc,&VehicleData->VehicleAttMat);

	mathAddVectors(&AccTotal,&AccTotal,&Acc);

	// attitude acceleration
	Pos1 = Pos2 = *NeckPos;
	Length = mathExactVectorLength(&Pos1);
	DeltaAtt.Pitch = VehicleData->VehicleAttAcc.Pitch / 100.0;
	DeltaAtt.Roll = VehicleData->VehicleAttAcc.Roll / 100.0;
	DeltaAtt.Yaw = VehicleData->VehicleAttAcc.Yaw / 100.0;
	mathRotateVectorByAtt(&Pos2,&Pos2,&DeltaAtt);
	mathSubVectors(&Velocity,&Pos2,&Pos1);
	Velocity.x *= 100.0;
	Velocity.y *= 100.0;
	Velocity.z *= 100.0;

	Acc.x = Velocity.x * Length;
	Acc.y = -Velocity.y * Length;
	Acc.z = Velocity.z * Length;

	mathVectorMultiply64(&Acc,&Acc,&VehicleData->VehicleAttMat);

	mathAddVectors(&AccTotal,&AccTotal,&Acc);

	// remove gravity
	AccTotal.z += 9.8;

	// convert to body DOF frame of reference
	mathVectorMultiply64(&AccTotal,&AccTotal,&VehicleData->VehicleAttInvMat);
	mathVectorMultiply64(&AccTotal,&AccTotal,&InvNeckAttMat);

	// ok, we have the acceleration vector, now convert to movement
	persGetControlStatus(&Strength,&DummyLogical);
	Strength = 0.3f + 0.7f * Strength;;
	SpringCo.x = 1.0 / _1G_LEFTRIGHT_MOVEMENT * 9.8 * (0.1f + 0.9f * Strength);
	SpringCo.y = 1.0 / _1G_FRONTBACK_MOVEMENT * 9.8 * (0.1f + 0.9f * Strength);
	SpringCo.z = 1.0 / _1G_TOPBOTTOM_MOVEMENT * 9.8;

	DeltaTime = frametime / 1000.0 - _LastTime;
	if (DeltaTime > 10.0 || DeltaTime < 0.0)
	{
		_LastTime = frametime / 1000.0;
		DeltaTime = 0.0;
	}

	NumTics = (UINT32)(DeltaTime / _DELTA_TIME);

	for (i=0;i<NumTics;i++)
	{
		// left-right
		Pos = _LastPos.x;
		Vel = _LastVel.x;

		LastVel = Vel;
		Vel += AccTotal.x * _DELTA_TIME;
		Vel -= ((Pos + _LastPos.x) * 0.5) * SpringCo.x * _DELTA_TIME;
		Vel -= LastVel * _HEAD_SHOCK_VAL * _DELTA_TIME * Strength;
		Pos += (Vel + LastVel) * 0.5 * _DELTA_TIME + AccTotal.x * _DELTA_TIME * _DELTA_TIME * 0.5;

		_LastPos.x = Pos;
		_LastVel.x = Vel;

		// front-back
		Pos = _LastPos.y;
		Vel = _LastVel.y;

		LastVel = Vel;
		Vel += AccTotal.y * _DELTA_TIME;
		Vel -= ((Pos + _LastPos.y) * 0.5) * SpringCo.y * _DELTA_TIME;
		Vel -= LastVel * _HEAD_SHOCK_VAL * _DELTA_TIME * Strength;
		Pos += (Vel + LastVel) * 0.5 * _DELTA_TIME + AccTotal.y * _DELTA_TIME * _DELTA_TIME * 0.5;

		_LastPos.y = Pos;
		_LastVel.y = Vel;

		// top-bottom
		Pos = _LastPos.z;
		Vel = _LastVel.z;

		LastVel = Vel;
		Vel += AccTotal.z * _DELTA_TIME;
		Vel -= ((Pos + _LastPos.z) * 0.5) * SpringCo.z * _DELTA_TIME;
		Vel -= LastVel * _HEAD_SHOCK_VAL * _DELTA_TIME;
		Pos += (Vel + LastVel) * 0.5 * _DELTA_TIME + AccTotal.z * _DELTA_TIME * _DELTA_TIME * 0.5;

		_LastPos.z = Pos;
		_LastVel.z = Vel;

		_LastTime += _DELTA_TIME;
	}


	Displacement->x = -_LastPos.x;
	Displacement->y = _LastPos.y;
	Displacement->z = -_LastPos.z;

	if (Displacement->z < 0.0)
	{
		Displacement->z = -_TOPBOTTOM_G_CAP * sqrt(fabs(Displacement->z) / _TOPBOTTOM_G_CAP);
	}
	else
	{
		Displacement->z = _TOPBOTTOM_G_CAP * sqrt(fabs(Displacement->z) / _TOPBOTTOM_G_CAP);
	}

	Displacement->z += sqrt(_1G_TOPBOTTOM_MOVEMENT / _TOPBOTTOM_G_CAP) * _TOPBOTTOM_G_CAP;

	if (Displacement->x > _LEFTRIGHT_G_CAP)
	{
		Displacement->x = _LEFTRIGHT_G_CAP;
	}
	if (Displacement->x < -_LEFTRIGHT_G_CAP)
	{
		Displacement->x = -_LEFTRIGHT_G_CAP;
	}
	if (Displacement->y > _FRONTBACK_G_CAP)
	{
		Displacement->y = _FRONTBACK_G_CAP;
	}
	if (Displacement->y < -_FRONTBACK_G_CAP)
	{
		Displacement->y = -_FRONTBACK_G_CAP;
	}
	if (Displacement->z > _TOPBOTTOM_G_CAP)
	{
		Displacement->z = _TOPBOTTOM_G_CAP;
	}
	if (Displacement->z < -_TOPBOTTOM_G_CAP)
	{
		Displacement->z = -_TOPBOTTOM_G_CAP;
	}

	AttChange->Roll = Displacement->x * mlDEG_TO_RAD(15.0) / _LEFTRIGHT_G_CAP;
	AttChange->Pitch = Displacement->y * mlDEG_TO_RAD(15.0) / _FRONTBACK_G_CAP;

	return 0;
}

UINT32 mlToggleHeadLook(SINT32 keyState, UINT32 functionNum, UINT32 unused)
{
    if(keyState) {
        _mouseLook = !_mouseLook;
        mlCenterMouseLook();
        TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionNum, &keyState);
    }
    return 0;
}

UINT32 mlHeadLook(SINT32 param, UINT32 functionNum, UINT32 data){

	Write_Debug("mlHeadLook Unimplemented");
	return -1;
}

/************************************************************************************************
	mlToggleMouseLookadPos
*************************************************************************************************/

UINT32 mlToggleMouseLook(SINT32 keyState, UINT32 functionNum, UINT32 unused)
{
	if(keyState)
	{
		_mouseLook = !_mouseLook;
		mlCenterMouseLook();
		TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionNum, &keyState);
	}

	return 0;
}

/************************************************************************************************
	mlMouseLook
*************************************************************************************************/

UINT32 mlMouseLook(SINT32 param, UINT32 functionNum, UINT32 data)
{
	imap_callback_ext_data* extdata;
	vehVehicle*	pVehicle = persGetPersonaVehiclePtr();

    if(!pVehicle) return 0;

	if (pVehicle && pVehicle->VehicleType->Type == VEH_TYPE_PERSON)
		return fpFPSControlInput (param, functionNum, data);

	if(!_mouseLook) return 0;

	extdata = (imap_callback_ext_data*)data;
	if(extdata)
		_absHeadPos = (extdata->axisType != IMAP_DELTA_AXIS);

	if(functionNum == IMAP_MOUSE_LOOK_X)
	{
		if(extdata && extdata->axisType != IMAP_DELTA_AXIS)
		{
			_mouseLookAtt.Yaw = PI * (param / 1000.0f);
		}else
		{
			_mouseLookAtt.Yaw += param * .025f / PI;
		}

		if(_mouseLookAtt.Yaw < -YAW_LIMIT)
			_mouseLookAtt.Yaw = -YAW_LIMIT;
		else if(_mouseLookAtt.Yaw > YAW_LIMIT)
			_mouseLookAtt.Yaw = YAW_LIMIT;
	}else if(functionNum == IMAP_MOUSE_LOOK_Y)
	{
		if(extdata && extdata->axisType != IMAP_DELTA_AXIS)
		{
			_mouseLookAtt.Pitch = PI/2.0f * (param / 1000.0f);
		}else
		{
			_mouseLookAtt.Pitch += -param * .025f / PI;
		}

		if(_mouseLookAtt.Pitch < PITCH_DOWN_LIMIT)
			_mouseLookAtt.Pitch = PITCH_DOWN_LIMIT;
		else if(_mouseLookAtt.Pitch > PITCH_UP_LIMIT)
			_mouseLookAtt.Pitch = PITCH_UP_LIMIT;
	}

	if(pVehicle->VehicleType->Type == VEH_TYPE_FLOATER)
	{
		mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

		if(functionNum == IMAP_MOUSE_LOOK_X)
			VehicleData->VehicleRealAtt.Yaw   += param * .025f / PI;
		else if(functionNum == IMAP_MOUSE_LOOK_Y)
		 	VehicleData->VehicleRealAtt.Pitch += param * .025f / PI;

		pVehicle->ModelData->RenderPos = VehicleData->VehiclePos;
		pVehicle->ModelData->RenderAtt = VehicleData->VehicleRealAtt;

		pVehicle->PosData.RenderPos = VehicleData->VehiclePos;
		pVehicle->PosData.RenderAtt = VehicleData->VehicleRealAtt;

		pVehicle->PosData.Pos = VehicleData->VehiclePos;
		pVehicle->PosData.Att = VehicleData->VehicleRealAtt;

		VehicleData->VehicleAtt   = VehicleData->VehicleRealAtt;
	}

	return 0;
}

/************************************************************************************************
	mlCenterMouseLook
*************************************************************************************************/

void mlCenterMouseLook()
{
	if(!_absHeadPos)
	{
		_mouseLookAtt.Pitch = _mouseLookAtt.Roll = _mouseLookAtt.Yaw = 0;
	}
}


/************************************************************************************************
	mlSetMouseLookAndTorsoKeymapFlag
*************************************************************************************************/

void mlSetMouseLookAndTorsoKeymapFlag(LOGICAL used)
{
	_mouseLookAndTorsoUseSameKeymap = used;
}

/************************************************************************************************
	mlMouseLookEnabled
*************************************************************************************************/

LOGICAL mlMouseLookEnabled()
{
	return _mouseLook;
}

/************************************************************************************************
	mlSetMouseLookEnabled
*************************************************************************************************/

void mlSetMouseLookEnabled(LOGICAL enable)
{
	_mouseLook = enable;
}

/************************************************************************************************
	mlMouseLookAndTorsoUseSameKeymap
*************************************************************************************************/

LOGICAL mlMouseLookAndTorsoUseSameKeymap()
{
	return _mouseLookAndTorsoUseSameKeymap;
}
