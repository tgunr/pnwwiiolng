#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"

#include "include/vars.h"
#include "trn/trnmain.h"
#include "trn/trncloud.h"
#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "spt/Random.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "fx/FXInterface.h"
#include "ob/obpool.h"
#include "rn/rnvis.h"
#include "trn/trnsky.h"
#include "fx/fxWeather.h"

#include "trn/trndata.h"

#define _CLOUD_HAZE_START 50 // 300 meters from cloud

static bool _displayclouds = true;

static UINT32				AddCloudWisps();
static std::set<OBJECT*>	cloudInstances;

void trncloudToggle(bool toggle)
{
    _displayclouds = toggle;
}

void trnCloudRegisterCloudInstance(OBJECT* topNode)
{
    if(topNode->name[0] == 'r')
    {
        obmainSetFlag3InThisTree(topNode, OBJECT_RAINCLOUDS);
    }
    else
    {
        obmainSetFlag3InThisTree(topNode, OBJECT_CLOUDS);
    }

    // Need this for sun to be visible...since we had to draw sorted objects incorrectly (with z buffer writes enabled)
    obmainSetFlagsInThisTree(topNode, OBJECT_ZWRITEOFF);

	cloudInstances.insert(topNode);
}


void trnCloudDeRegisterCloudInstance(OBJECT* topNode)
{
	cloudInstances.erase(topNode);
}


UINT32 trnCloudUpdate()
{
	AddCloudWisps();
	return 0;
}


static void getWispIntesity(const OBJECT* o, const FLOAT64VECTOR& Pos, float* Intensity)
{
	if(!(o->flags & OBJECT_NOTOBJECT))
	{
		piVECTOR	bBoxWorld[2];

        mathAddVectors(&bBoxWorld[0], &o->bbox[0], &o->pos);
        mathAddVectors(&bBoxWorld[1], &o->bbox[1], &o->pos);
		math3dVectorMultiply(&bBoxWorld[0], &bBoxWorld[0], &o->mat);
		math3dVectorMultiply(&bBoxWorld[1], &bBoxWorld[1], &o->mat);

		if (Pos.x > (bBoxWorld[0].x - (float)_CLOUD_HAZE_START) &&
		    Pos.y > (bBoxWorld[0].y - (float)_CLOUD_HAZE_START) &&
		    Pos.z > (bBoxWorld[0].z - (float)_CLOUD_HAZE_START) &&
		    Pos.x < (bBoxWorld[1].x + (float)_CLOUD_HAZE_START) &&
		    Pos.y < (bBoxWorld[1].y + (float)_CLOUD_HAZE_START) &&
		    Pos.z < (bBoxWorld[1].z + (float)_CLOUD_HAZE_START))
	    {
			double distance = 0.0f;

			if(Pos.x < (bBoxWorld[0].x)) distance += fabs(bBoxWorld[0].x - Pos.x);
			if(Pos.x > (bBoxWorld[1].x)) distance += fabs(bBoxWorld[1].x - Pos.x);
			if(Pos.y < (bBoxWorld[0].y)) distance += fabs(bBoxWorld[0].y - Pos.y);
			if(Pos.y > (bBoxWorld[1].y)) distance += fabs(bBoxWorld[1].y - Pos.y);
			if(Pos.z < (bBoxWorld[0].z)) distance += fabs(bBoxWorld[0].z - Pos.z);
			if(Pos.z > (bBoxWorld[1].z)) distance += fabs(bBoxWorld[1].z - Pos.z);

			double thisIntensity	= 1.0f - pow(distance / (double)_CLOUD_HAZE_START, 2);
			if(thisIntensity < 0.0f) *Intensity = 0.0f;

			*Intensity += (float)thisIntensity;
		}
	}

	if(o->attached) getWispIntesity(o->attached, Pos, Intensity);
	if(o->child) getWispIntesity(o->child, Pos, Intensity);
	if(o->sibling) getWispIntesity(o->sibling, Pos, Intensity);

	return;
}



#define _WISP_INTERVAL 100
#define _WISP_TIME 0.5f
#define _WISP_TIME_INTEGER 1000
#define _WISP_SIZE 75.0f
#define _WISP_DISPERSION 15.0f

#include "fx/TransformEffect.h"
#include "ui/uiChat.h"
static UINT32 AddCloudWisps()
{
    static SINT32 _LastUpdateTime;
	if (frametime - _LastUpdateTime < _WISP_INTERVAL && _LastUpdateTime != 0)
	{
		return 0;
	}
	_LastUpdateTime = frametime;

    FLOAT32 Intensity;
    FLOAT64VECTOR	Pos;
    FLOAT64VECTOR	PPos;
    piVECTOR Velocity;
    piVECTOR TempVel;
    piVECTOR RotationRate;
    FLOAT64VECTOR WorldOriginOffset;
    mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData != nullptr)
	{
		Velocity.x = (FLOAT32)(VehicleData->VehicleRealVel.x);
		Velocity.y = (FLOAT32)(VehicleData->VehicleRealVel.y);
		Velocity.z = (FLOAT32)(VehicleData->VehicleRealVel.z);
	}
	else
	{
		Velocity.x = Velocity.y = Velocity.z = 0.0;
	}

	Intensity = 0.0f;

	Pos.x = -playpos.x;
	Pos.y = -playpos.y;
	Pos.z = -playpos.z;

  //  for(SINT32 r = 0; r < MAX_CLOUD_LAYERS; r++)
    {
		std::set<OBJECT*>::iterator it = cloudInstances.begin();
		for(; it != cloudInstances.end(); it++)
	    {
            if((*it)->flag3 & OBJECT_RAINCLOUDS)
            {
                if(!fxWeatherIsCloudy())
                    continue;
            }

			getWispIntesity(*it, Pos, &Intensity);
	    }
    }

	if (Intensity > 0.001f)
	{
		trnGetWorldOriginOffset(&WorldOriginOffset);
		Pos.x += WorldOriginOffset.x;
		Pos.y += WorldOriginOffset.y;
		Pos.z += WorldOriginOffset.z;
		Pos.z = -Pos.z;

		PPos = Pos;
		PPos.x += Velocity.x * _WISP_TIME;
		PPos.y += Velocity.y * _WISP_TIME;
		PPos.z += Velocity.z * _WISP_TIME;

		PPos.x += (GetRandom(0.0f, 1.0f) * 2.0f - 1.0f) * _WISP_DISPERSION;
		PPos.y += (GetRandom(0.0f, 1.0f) * 2.0f - 1.0f) * _WISP_DISPERSION;
		PPos.z += (GetRandom(0.0f, 1.0f) * 2.0f - 1.0f) * _WISP_DISPERSION;

		TempVel.x = TempVel.y = TempVel.z = 0.0f;

		RotationRate.x = GetRandom(0.0f, 1.0f) * 2.0f * PI;
		RotationRate.y = 0.0f;
		RotationRate.z = 0.0f;

#if(1)
        ParticleMgr* pm = TransformEffect::GetParticleMgr();
        Vec3d pos(PPos);
        Vec3f vel(TempVel);

        pos.z() = -pos.z();
        float alpha[4] = {0, Intensity * 0.5f, Intensity * 0.5f, 0};
        float size[4] = {_WISP_SIZE, _WISP_SIZE, _WISP_SIZE, _WISP_SIZE};
        int time[4] =
        {
            0,
            _WISP_TIME_INTEGER * 2 / 3,
            _WISP_TIME_INTEGER * 4 / 3,
            _WISP_TIME_INTEGER * 6 / 3,
        };

        unsigned int flags = 0;
        pm->createparticle(pos, vel, size, alpha, time, "old11", RotationRate.x, flags);
#else
		FXCreateNewParticle(&PPos,
							&TempVel,
							0.0f,
							nullptr,
							0.0f,
						   Intensity * 0.5f,
						   Intensity * 0.5f,
						   0.0f,
						   _WISP_SIZE,
						   _WISP_SIZE,
						   _WISP_SIZE,
						   _WISP_SIZE,
						   frametime,
						   frametime + _WISP_TIME_INTEGER * 2 / 3,
						   frametime + _WISP_TIME_INTEGER * 4 / 3,
						   frametime + _WISP_TIME_INTEGER * 6 / 3,
						   10,
						   &RotationRate,
						   PARTICLE_FLAG_USESROTATION,
						   nullptr);
#endif
	}

	return 0;
}
