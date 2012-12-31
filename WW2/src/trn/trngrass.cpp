#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: trngrass.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "trn/trngrass.h"
#include "trn/trnRadClutter.h"
#include "trn/speedgrass.h"
#include "ob/obMgr.h"
#include "trn/trnvars.h"
#include "math/math3d.h"

//////////////////////////////////////////////////////////////////////
// Function: trngrassFree
// Description:
//////////////////////////////////////////////////////////////////////
void trngrassFree(OBJECT* o)
{
    size_t code = (size_t)o;
    speedgrassRemove(code);
}

//////////////////////////////////////////////////////////////////////
// Function: trngrassAddGrass
// Description:
//////////////////////////////////////////////////////////////////////
void trngrassAddGrass(OBJECT* obj)
{
    if(!obj || !obj->modelInstance)
        return;

    const char* modelname = obj->modelInstance->GetModelName();

    OBJECT* o = obj;

    while(o)
    {
        if(!(o->flags & OBJECT_NOTOBJECT))
        {
            break;
        }

        o = o->child;
    }

    size_t key = (size_t)o;

    piVECTOR pos;
    math3dVectorMultiply(&pos, &o->pos, &o->mat);

    speedgrassAddAndCollide(Vec3d(pos) + WorldOriginOffset, modelname, key, 1000.0f, NULL, o);

    if(o && o->flag2 & OBJECT_IS_GRASS)
    {
        if(o->layer <= 0)
        {
            return;
        }

        obmainMoveToNotObjectTree(o);
        o->flag2 |= OBJECT_HIDDEN;
    }
}
