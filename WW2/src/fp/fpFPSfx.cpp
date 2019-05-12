#include "syscfg.h" // Path is project specific.
#include "fx/fxMgr.h"
#include "fx/fxSnd.h"
#include "fx/fxPcl.h"
#include "fp/fptypes.h"
#include "pers/persmain.h"
#include "ob/obpool.h"
#include "ogrp/ogrptype.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "spt/Random.h"

#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "fp/fpTPSdata.h"
#include "fx/fxUtl.h"
#include "include/vars.h"
#include "ob/obMgr.h"
#include "trn/trnvars.h"
#include "fx/TransformEffect.h"

#include <deque>

#define MAX_SHELLS  32
//#define FPSFX_DEBUG

static struct OBJECT* fpFPSfxGetFreeShell();
#if(0)
static void fpFPSfxShellKillCallback(void* pData);
#endif

//static FLOAT64VECTOR	fpFPSSoundPositionZero;
static SINT32 fpFPSfxPlaySound(const char* soundName, SINT32 soundPriority, SINT32 soundType, SINT32 playType, FLOAT64VECTOR* soundPosition, OBJECT* soundObject, UINT32 flags, FLOAT32 fMin, FLOAT32 fMax);

static struct OBJECT* _shells[MAX_SHELLS];
static std::deque<struct OBJECT*> _freeshells;

static FLOAT32 _piover180 = PI / 180.0f;

LOGICAL fpFPSfxInit()
{
    SINT32 i;

    OBJECT* o = nullptr;

    for(i = 0; i < MAX_SHELLS; i++)
    {
        if(!obmgrCreateModelInstance("shell"))
            return FALSE;

        o = obmainFindNamedObject("shell");

        if(!o)
            return FALSE;

        _shells[i] = o;

        while(o->flags & OBJECT_NOTOBJECT)
        {
            o = o->child;
        }

        obmainMoveToNotObjectTree(o);

        _freeshells.push_back(o);
    }

    return TRUE;
}

void fpFPSfxKill()
{
    SINT32 i;

    for(i = 0; i < MAX_SHELLS; i++)
    {
        if(_shells[i])
        {
            ogrpFreeObjectTree(_shells[i]);
        }
    }

    _freeshells.clear();
}

bool _updateShell(OBJECT* o)
{
    // Only display for 3 seconds
    // Position below trooper feet
    if((o->loc.z > -o->mat._43) || (frametime - o->createtime > 3000.0f))
    {
        FLOAT64VECTOR pos;
        pos.x = o->mat._41 + WorldOriginOffset.x;
        pos.y = o->mat._42 + WorldOriginOffset.y;
        pos.z = -o->mat._43;

        fxSndAddFiniteWorldSound("sounds/casing.wav", 5, 50, IDE_TP_PRIORITY_LOW, pos);

        o->createtime = 0.0f;
        o->updatetime = 0.0f;
        o->updatecallback = nullptr; // Removes from obmainUpdate
        fxUtlSetMatrixInTree(o, &IdentityMatrix);
        obmainMoveToNotObjectTree(o);

        _freeshells.push_back(o);

        return true;
    }

    // Tree id used for time update
    int timediff = frametime - o->updatetime;
    float time = (float)timediff * 0.001f;
    static float drag = 0.0f;

    fxUtlApplyDrag(&o->vel, &drag, &o->updatetime);

    o->updatetime = frametime;

    piMATRIX matRotX = IdentityMatrix;
    piMATRIX matRotY = IdentityMatrix;
    piMATRIX matRotZ = IdentityMatrix;
    piMATRIX matObj = IdentityMatrix;

    matObj = o->mat;

    // Translate to origin
    matObj._41 = 0.0f;
    matObj._42 = 0.0f;
    matObj._43 = 0.0f;

    // Rotate around X
    matRotX._22 = (FLOAT32)cos(o->rot.x);
    matRotX._23 = (FLOAT32)sin(o->rot.x);
    matRotX._32 = -matRotX._23;
    matRotX._33 =  matRotX._22;

    math3dMatrixMultiply(&matObj, &matObj, &matRotX);

    // Rotate around Y
    matRotY._11 = (FLOAT32)cos(o->rot.y);
    matRotY._13 = (FLOAT32)-sin(o->rot.y);
	matRotY._31 = -matRotY._13;
	matRotY._33 =  matRotY._11;

    math3dMatrixMultiply(&matObj, &matObj, &matRotY);

    // Rotate around Z
    matRotZ._11 = (FLOAT32)cos(o->rot.z);
    matRotZ._12 = (FLOAT32)sin(o->rot.z);
	matRotZ._21 = -matRotZ._12;
	matRotZ._22 =  matRotZ._11;

    math3dMatrixMultiply(&matObj, &matObj, &matRotZ);

    matObj._41 = o->mat._41 + time * o->vel.x;
    matObj._42 = o->mat._42 + time * o->vel.y;
    matObj._43 = o->mat._43 + time * o->vel.z;

    o->vel.z += 9.8f * time;

    fxUtlSetMatrixInTree(o, &matObj);

    return true;
}

static bool _InitObjectParams(OBJECT* o, piVECTOR& dir, float scale, FLOAT64VECTOR& pos)
{
    piMATRIX matOrient = IdentityMatrix;

    float dot = math3dDotProduct(&dir, &dir);

    if(dot > -0.0001f && dot < 0.0001f)
    {
        return false;
    }

    if(_isnan(dot) || !_finite(dot))
    {
        return false;
    }

    piVECTOR vecDir = dir;

    vecDir.z = -vecDir.z;

    mathVectorToYAxis(&vecDir, &matOrient);
    math3dInvertMatrix(&matOrient, &matOrient);

    piMATRIX matScale = IdentityMatrix;

    // In case user has rotated object prior to this call
    piMATRIX matObj = IdentityMatrix;

    if(o)
    {
        matObj = o->mat;
    }

    matObj._41 = 0.0f;
    matObj._42 = 0.0f;
    matObj._43 = 0.0f;

    matScale._11 = scale;
    matScale._22 = scale;
    matScale._33 = scale;

    math3dMatrixMultiply(&matOrient, &matOrient, &matScale);
    math3dMatrixMultiply(&matObj, &matObj, &matOrient);

    matObj._41 = float(pos.x - WorldOriginOffset.x);
    matObj._42 = float(pos.y - WorldOriginOffset.y);
    matObj._43 = float(-pos.z);

    if(o)
    {
        fxUtlSetMatrixInTree(o, &matObj);
    }

    return true;
}

/*-----------------------------------------------------------------------------------------
                                        SHELLS
------------------------------------------------------------------------------------------*/
UINT32 fpFPSfxEjectShell(piVECTOR& gundir,
                         piVECTOR& ejectdir,
                         piVECTOR& troopspeed,
                         FLOAT64VECTOR& shellpos,
                         FLOAT32 shellspeed,
                         LOGICAL firstperson,
                         FLOAT32 scale,
                         FLOAT64VECTOR& trooppos)
{
    mathReallyFastNormalize(gundir);
    mathReallyFastNormalize(ejectdir);

    // 3rd person gets the sprite. 1st person gets the 3d bullet shell.
    if(firstperson)
    {
        struct OBJECT* fx = fpFPSfxGetFreeShell();

        if(!fx)
        {
            return 1;
        }

        fx->updatecallback = _updateShell;
        obmainAddForUpdateCallback(fx);

        fx->createtime = frametime;
        fx->updatetime = frametime;

        // Add to normal render system for proper sorting
        obmainMoveToObjectTree(fx);

        // Particle velocity
        fx->vel.x = ejectdir.x * shellspeed;
        fx->vel.y = ejectdir.y * shellspeed;
        fx->vel.z = ejectdir.z * shellspeed;

        // Add troop speed
        fx->vel.x += troopspeed.x;
        fx->vel.y += troopspeed.y;
        fx->vel.z += troopspeed.z;

        fx->vel.z = -fx->vel.z;

        // Store rotation in unused loc
        fx->rot.x = (float)(mathGetChaoticNumber() % 25) * _piover180;
        fx->rot.y = (float)(mathGetChaoticNumber() % 25) * _piover180;
        fx->rot.z = (float)(mathGetChaoticNumber() % 25) * _piover180;

        fx->loc.x = float(trooppos.x - WorldOriginOffset.x);
        fx->loc.y = float(trooppos.y - WorldOriginOffset.y);
        fx->loc.z = float(trooppos.z);

        _InitObjectParams(fx, gundir, scale, shellpos);
    }
    else
    {
        shellspeed += GetRandom(-0.2f, 0.2f);

        Vec3f n = Vec3f(ejectdir) * shellspeed;

        TransformEffect* te = TransformEffect::AddEffect("shellcasing", n, 0, nullptr);

        if(te)
        {
            Vec3d pos(shellpos);
            pos.z() = -pos.z();

            te->enableemitters(true);;
            te->setmatrix(Matrixf::translate(pos));
            te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        }
    }

    return 0;
}

void fpFPSfxShellKillCallback(void* data)
{
	if(data == nullptr) return
    fxUtlSetMatrixInTree((struct OBJECT*)data, &IdentityMatrix);
    _freeshells.push_back((struct OBJECT*)data);
}

struct OBJECT* fpFPSfxGetFreeShell()
{
    if(_freeshells.empty())
    {
        return nullptr;
    }

    struct OBJECT* o = _freeshells.front();
    _freeshells.pop_front();

    return o;
}

/*-----------------------------------------------------------------------------------------
                                        SOUNDS
------------------------------------------------------------------------------------------*/

// external use interface calls .. simplified calls so outsiders don't have to know as much to get sounds to play
SINT32 fpFPSfxPlayFireSound (char* soundName, SINT32 soundPriority, SINT32 soundType, FLOAT64VECTOR* soundPosition, OBJECT* soundObject)
{
	return fpFPSfxPlaySound(soundName, soundPriority, soundType, DEF_PLAYTYPE_HEAD, soundPosition, soundObject, DEF_SOUND_STOPS_AMBIENCE, 0, 0);
}

SINT32 fpFPSfxPlayNormalSound (const char* soundName, SINT32 soundPriority, SINT32 soundType, FLOAT64VECTOR* soundPosition, OBJECT* soundObject)
{
	return fpFPSfxPlaySound(soundName, soundPriority, soundType, DEF_PLAYTYPE_HEAD, soundPosition, soundObject, 0, 0, 0);
}

SINT32 fpFPSfxPlayTPSMovementSound (char* soundName, OBJECT* soundObject, FLOAT32 fMin, FLOAT32 fMax, void* model, void (*pFn)(void* data))
{
	SINT32	soundID;

	soundID = fpFPSfxPlaySound(soundName, IDE_TP_PRIORITY_LOW, TYPE_INFINITE_EXTERNAL_SOUND, DEF_PLAYTYPE_OBJECT, nullptr, soundObject, 0, fMin, fMax);

	if (soundID > 0)
	{
		fxSndSetKillCallback(soundID, pFn);
		fxSndSetKillData(soundID, model);
		return soundID;
	}
	else
		return soundID;
}

SINT32 fpFPSfxPlayTPSSingleSound (const char* soundName, OBJECT* soundObject, FLOAT32 fMin, FLOAT32 fMax)
{
	SINT32	soundID;

	soundID = fpFPSfxPlaySound(soundName, IDE_TP_PRIORITY_LOW, TYPE_FINITE_EXTERNAL_SOUND, DEF_PLAYTYPE_OBJECT, nullptr, soundObject, 0, fMin, fMax);
	return soundID;
}

// the actual "make it happen" sound call
// returns the soundID
SINT32 fpFPSfxPlaySound(const char* soundName, SINT32 soundPriority, SINT32 soundType, SINT32 playType, FLOAT64VECTOR* soundPosition, OBJECT* soundObject, UINT32 flags, FLOAT32 fMin, FLOAT32 fMax)
{
	if (soundName)
	{
#ifdef FPSFX_DEBUG
		WriteLog (LOG_FPS, LOG_INFO, "Playing sound %s", soundName);
#endif
    }
	else
	{
#ifdef FPSFX_DEBUG
        WriteLog (LOG_FPS, LOG_INFO, "nullptr sound");
#endif
        return -1;
	}

	SINT32 iID;

	iID = fxSndGetNewSound();

	if(iID == -1)
			return -1;


	fxSndSetSoundName(iID, soundName);
	fxSndSetPriority(iID, (ENUM_PRIORITY)soundPriority);
	fxSndSetSoundType(iID, soundType);
	fxSndSetPitch(iID, 1.0f);
	fxSndSetVolume(iID, 1.0f);
	fxSndPushFlags(iID, flags);

    // Set min and max distances
	if (fMin)
	{
		fxSndSetMinDistance(iID, fMin);
	}
	if (fMax)
	{
		fxSndSetMaxDistance(iID, fMax);
	}

	if (soundPosition != nullptr)
	{
		fxSndSetPosition(iID, soundPosition->x, soundPosition->y, soundPosition->z);
	}

	fxSndSetEmitterObject(iID, soundObject);
	fxSndSetPlayType(iID, playType);

    // A failure does not necessarily imply an error. For example, if a sound
    // is beyond its max distance, this function will fail.
    if(!fxSndInitSound(iID))
        return -1;

    return iID;
}

LOGICAL fpFPSUpdateSounds(OBJECT* pObject)
{
	return TRUE;

	LOGICAL			soundChanged = FALSE;
	fpFPSSoundType	newSound;
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	fpPersonData*	personData = (fpPersonData*) vehicleData->PersonData;

	if (personData->Flags & fpFLAGS_FALLING)
	{
		if ((fpsTrooper.parachuteOKToOpen) && (!fpsTrooper.parachuteOpen))
		{
			if (ft10mscount - personData->StartFallTime10ms < 80)
				newSound = FPS_SOUND_NONE;
			else
				newSound = FPS_SOUND_FREEFALL;
		}
		else if (fpsTrooper.parachuteOpen)
			newSound = FPS_SOUND_PARACHUTE_FALL;
		else
			newSound = FPS_SOUND_NONE;

		if (fpsTrooper.movementSound != newSound)
		{
			soundChanged = TRUE;
			fpsTrooper.movementSound = newSound;
		}
	}
    else if (fpFPSIsSwimming() || fpFPSIsWading())
	{
		if (fpsTrooper.movementSound != FPS_SOUND_WATER)
		{
			soundChanged = TRUE;
			fpsTrooper.movementSound = FPS_SOUND_WATER;
		}
	}
	else
	{
		if (fpFPSIsSilent())
		{
			if (fpsTrooper.movementSound != FPS_SOUND_NONE)
			{
				soundChanged = TRUE;
				fpsTrooper.movementSound = FPS_SOUND_NONE;
			}
		}
		// assume movement here
		else if (fpFPSIsProne())
		{
			if (fpsTrooper.movementSound != FPS_SOUND_CRAWL)
			{
				soundChanged = TRUE;
				fpsTrooper.movementSound = FPS_SOUND_CRAWL;
			}
		}
		else if (fpFPSIsCrouch())
		{
			if (fpFPSIsAim())
			{
				if (fpsTrooper.movementSound != FPS_SOUND_CREEP_AIM)
				{
					soundChanged = TRUE;
					fpsTrooper.movementSound = FPS_SOUND_CREEP_AIM;
				}
			}
			else
			{
				if (fpsTrooper.movementSound != FPS_SOUND_CREEP)
				{
					soundChanged = TRUE;
					fpsTrooper.movementSound = FPS_SOUND_CREEP;
				}
			}
		} // assume standing here
		else if (fpFPSIsFast())
		{
			if (fpsTrooper.movementSound != FPS_SOUND_SPRINT)
			{
				soundChanged = TRUE;
				fpsTrooper.movementSound = FPS_SOUND_SPRINT;
			}
		} // assume slow here
		else if (fpFPSIsAim() || fpFPSIsWalking())
		{
			if (fpsTrooper.movementSound != FPS_SOUND_TROT_AIM)
			{
				soundChanged = TRUE;
				fpsTrooper.movementSound = FPS_SOUND_TROT_AIM;
			}
		}
		else // assume stand, slow, not aim.  i.e. trot
		{
			if (fpsTrooper.movementSound != FPS_SOUND_TROT)
			{
				soundChanged = TRUE;
				fpsTrooper.movementSound = FPS_SOUND_TROT;
			}
		}
	}

	if (soundChanged == TRUE)
	{
		fxSndStopSound(fpsTrooper.movementSoundID);
		fpsTrooper.movementSoundID = 0;

		if (fpsTrooper.movementSound != FPS_SOUND_NONE)
		{
			if ((fpsTrooper.movementSound == FPS_SOUND_WATER) || (fpsTrooper.movementSound == FPS_SOUND_FREEFALL) || (fpsTrooper.movementSound == FPS_SOUND_PARACHUTE_FALL))
				fpsTrooper.movementSoundID = fpFPSfxPlayNormalSound (fpsTrooper.moveSounds[fpsTrooper.movementSound], IDE_FP_PRIORITY_MANDATORY, TYPE_INFINITE_EXTERNAL_SOUND, nullptr, pObject);
			else
			{
				const SCHAR* soundName;
				switch (fpsTrooper.movementSound)
				{
					case FPS_SOUND_CRAWL:
						soundName = fpTPSSurfaces[fpsTrooper.currentSurfaceID].fpsCrawlSound;
						break;
					case FPS_SOUND_CREEP_AIM:
						soundName = fpTPSSurfaces[fpsTrooper.currentSurfaceID].fpsCreepAimSound;
						break;
					case FPS_SOUND_CREEP:
						soundName = fpTPSSurfaces[fpsTrooper.currentSurfaceID].fpsCreepSound;
						break;
					case FPS_SOUND_TROT_AIM:
						soundName = fpTPSSurfaces[fpsTrooper.currentSurfaceID].fpsTrotAimSound;
						break;
					case FPS_SOUND_TROT:
						soundName = fpTPSSurfaces[fpsTrooper.currentSurfaceID].fpsTrotSound;
						break;
					case FPS_SOUND_SPRINT:
						soundName = fpTPSSurfaces[fpsTrooper.currentSurfaceID].fpsSprintSound;
						break;
					default:
						soundName = "";
						break;
				}

				fpsTrooper.movementSoundID = fpFPSfxPlayNormalSound ( (char*)soundName, IDE_FP_PRIORITY_MANDATORY, TYPE_INFINITE_EXTERNAL_SOUND, nullptr, pObject);
			}
		}
	}

	return TRUE;
}

LOGICAL fpFPSPlayFallSound(OBJECT* pObject, SINT32 fallType)
{
	UINT32 SoundNameIndex = FPS_SOUND_FALL_NORMAL;

	switch (fallType)
	{
	case FPS_FALL_NORMAL:
		SoundNameIndex = FPS_SOUND_FALL_NORMAL;
		break;
	case FPS_FALL_NORMAL_HURT:
		SoundNameIndex = FPS_SOUND_FALL_NORMAL_HURT;
		break;
	case FPS_FALL_WATER:
		SoundNameIndex = FPS_SOUND_FALL_WATER;
		break;
	case FPS_FALL_WATER_HURT:
		SoundNameIndex = FPS_SOUND_FALL_WATER_HURT;
		break;
	}

	fpFPSfxPlayNormalSound (fpsTrooper.moveSounds[SoundNameIndex], IDE_FP_PRIORITY_MANDATORY, TYPE_FINITE_SOUND, nullptr, pObject);

	return TRUE;
}
