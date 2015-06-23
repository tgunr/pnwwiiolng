#pragma once

#include "wwii/wwiiServerTrackedObject.h"
#include "fx/TransformEffect.h"

enum STO_EFFECT_SUBTYPES
{
    _STO_NONE,
    _STO_SMOKE_GRENADE,
    //Add here
    _STO_MAX_EFFECTS
};

class SmokeGrenadeSTO : public ServerTrackedObject
{
public:

	enum COLORS
	{
		BRITISH		= 1<<0,
		FRENCH		= 1<<1,
		GERMAN		= 1<<2
	};

    SmokeGrenadeSTO(const WorldPosition_m& worldpos, const AttitudeVector& attitude, UINT32 mslifetime, UINT32 flags)
                    : ServerTrackedObject(0, _EFFECT, _STO_SMOKE_GRENADE, worldpos, attitude, flags, mslifetime)
					, te(nullptr)
					, _stosoundpopID(0)
					, _stosoundfizzID(0)
    {
    }

    virtual ~SmokeGrenadeSTO();

    SmokeGrenadeSTO(ACTION_OBJECT* const a)
    : ServerTrackedObject(a), te(nullptr), _stosoundpopID(0), _stosoundfizzID(0)
    {
    }

    bool createSmokeGrenade();

protected:
	// Interface for activating and deactivating
	virtual bool activate();
	virtual void deactivate();

private:
    TransformEffect* te;

    SINT32 _stosoundpopID;
    SINT32 _stosoundfizzID;
};
