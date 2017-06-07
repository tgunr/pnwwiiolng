#include "syscfg.h" // Path is project specific.
//////////////////////////////////////////////////////////////////////
// stoeffects.cpp
//////////////////////////////////////////////////////////////////////
#include "pi/pi.h"

#include "fx/stoeffects.h"
#include "fx/fxPcl.h"
#include "fx/fxSnd.h"
#include "include/vars.h"
#include "math/mathnum.h"			// For mathGetFloat32ChaoticNumber
#include "math/matheul.h"			// For mathGetFloat32ChaoticNumber
#include "math/math3d.h"
#include "gr2/gr2Anim.h"

#define USE_FX2 1


//////////////////////////////////////////////////////////////////////
//                  Global
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// FUNCTION: _killstoSound
//////////////////////////////////////////////////////////////////////
void _killstoSound(void* data)
{
    SINT32* soundid = (SINT32*)data;

    if(soundid)
    {
         *soundid = 0;
    }
}

//////////////////////////////////////////////////////////////////////
// FUNCTION: _killParticle
//////////////////////////////////////////////////////////////////////
void _killParticle(void* data)
{
    SINT32* particleid = (SINT32*)data;

    if(particleid)
    {
         *particleid = 0;
    }
}

//////////////////////////////////////////////////////////////////////
//                  SmokeGrenadeSTO members
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// FUNCTION: SmokeGrenadeSTO::~SmokeGrenadeSTO
//////////////////////////////////////////////////////////////////////
SmokeGrenadeSTO::~SmokeGrenadeSTO()
{
    if(te)
    {
        te->kill();
        te = nullptr;
    }

    if(_stosoundpopID > 0)
    {
        fxSndSetKillCallback(_stosoundpopID, nullptr);
        fxSndStopSoundDelayed(_stosoundpopID, 1);
        _stosoundpopID = 0;
    }

    if(_stosoundfizzID > 0)
    {
        fxSndSetKillCallback(_stosoundfizzID, nullptr);
        fxSndStopSoundDelayed(_stosoundfizzID, 1);
        _stosoundfizzID = 0;
    }
}

//////////////////////////////////////////////////////////////////////
// FUNCTION: SmokeGrenadeSTO::activate
//////////////////////////////////////////////////////////////////////
bool SmokeGrenadeSTO::activate()
{
	if(activated())
    {
        return false;
    }

    createSmokeGrenade();

	return ServerTrackedObject::activate();
}

//////////////////////////////////////////////////////////////////////
// FUNCTION: SmokeGrenadeSTO::deactivate
//////////////////////////////////////////////////////////////////////
void SmokeGrenadeSTO::deactivate()
{
	if(!activated())
    {
        return;
    }

    if(te)
    {
        te->kill();
        te = nullptr;
    }

    if(_stosoundpopID > 0)
    {
        fxSndStopSoundDelayed(_stosoundpopID, 1);
    }

    if(_stosoundfizzID > 0)
    {
        fxSndStopSoundDelayed(_stosoundfizzID, 1);
    }

     ServerTrackedObject::deactivate();
}

//////////////////////////////////////////////////////////////////////
// FUNCTION: SmokeGrenadeSTO::createSmokeGrenade
//////////////////////////////////////////////////////////////////////
bool SmokeGrenadeSTO::createSmokeGrenade()
{
    if(!timeRemaining())
    {
        return false;
    }

	piMATRIX matrix = IdentityMatrix;
    FLOAT64VECTOR pos;
	piVECTOR att;

    pos.x = position().x();
    pos.y = position().y();
    pos.z = position().z();

    // &#*(@#!!!
    pos.z = -pos.z;

	AttitudeVector av = attitude();

	att.x = av.x();
	att.y = av.y();
	att.z = av.z();

    mathEul_ToMatrix(&matrix, EulOrdXYZs, &att);

    piVECTOR dir = {0, 0, 1};

    math3dVectorMultiply(&dir, &dir, &matrix);

    // Force smoke to rise a bit.
    //dir.z = -0.2f;

    mathReallyFastNormalize(dir);

    // assume direction out the Y
	dir.x = -dir.x;
	dir.y = -dir.y;
	dir.z = -dir.z;

    te = TransformEffect::AddEffect("smokegrenade", Vec3f(dir), timeRemaining(), nullptr);

    if(te)
    {
        te->enableemitters(true);
        te->setmatrix(Matrixd::translate(Vec3d(pos)));
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
    }

    if(age() < 200)
    {
        // create 'pop'
        _stosoundpopID = fxSndGetNewSound();

        if(_stosoundpopID > 0)
        {
            fxSndSetSoundName(_stosoundpopID, "sounds/pop.wav");
            fxSndSetPosition(_stosoundpopID, position().x(), position().y(), position().z());
            fxSndSetPriority(_stosoundpopID, IDE_TP_PRIORITY_LOW);
            fxSndSetSoundType(_stosoundpopID, TYPE_FINITE_EXTERNAL_SOUND);
            fxSndSetVolume(_stosoundpopID, 1.0f);
            fxSndSetPitch(_stosoundpopID, 1.0f);
            fxSndSetPlayType(_stosoundpopID, DEF_PLAYTYPE_WORLD);
            fxSndSetMaxDistance(_stosoundpopID, 100.0f);
            fxSndSetMinDistance(_stosoundpopID, 10.0f);
            fxSndPushFlags(_stosoundpopID, SOUND_PLAY_IMMEDIATE);
            fxSndSetKillCallback(_stosoundpopID, _killstoSound);
            fxSndSetKillData(_stosoundpopID, &_stosoundpopID);

            fxSndInitSound(_stosoundpopID);
        }
    }

    // create 'fizz'
    _stosoundfizzID = fxSndGetNewSound();

    if(_stosoundfizzID > 0)
    {
        fxSndSetSoundName(_stosoundfizzID, "sounds/fizz.wav");
        fxSndSetPosition(_stosoundfizzID, position().x(), position().y(), position().z());
        fxSndSetPriority(_stosoundfizzID, IDE_TP_PRIORITY_LOW);
        fxSndSetSoundType(_stosoundfizzID, TYPE_INFINITE_EXTERNAL_SOUND);
        fxSndSetVolume(_stosoundfizzID, 1.0f);
        fxSndSetPitch(_stosoundfizzID, 0.8f + 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f));
        fxSndSetPlayType(_stosoundfizzID, DEF_PLAYTYPE_WORLD);
        fxSndSetMaxDistance(_stosoundfizzID, 100.0f);
        fxSndSetMinDistance(_stosoundfizzID, 10.0f);
        fxSndStopSoundDelayed(_stosoundfizzID, timeRemaining());
        fxSndSetKillCallback(_stosoundfizzID, _killstoSound);
        fxSndSetKillData(_stosoundfizzID, &_stosoundfizzID);

        fxSndInitSound(_stosoundfizzID);
    }

	// create a visual model
	if(m_flags)
	{
		const char* modelPath = nullptr;

		if(m_flags & BRITISH)
			modelPath = "TPS\\meshes\\uk_gren_smk\\uk_gren_smk";
		else if(m_flags & FRENCH)
			modelPath = "TPS\\meshes\\fr_gren_smk\\fr_gren_smk";
		else if(m_flags & GERMAN)
			modelPath = "TPS\\meshes\\de_gren_smk\\de_gren_smk";

		if(modelPath)
		{
			pos.z = -pos.z;
			gr2AnimAddCorpse(modelPath, pos, matrix, 120000); // 2 minutes
		}
	}

    return true;
}
