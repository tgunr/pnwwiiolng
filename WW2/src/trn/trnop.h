#pragma once

struct FindTree
{
    OBJECT* tree;

    FindTree()
    {
        tree = nullptr;
    }

    void Find(OBJECT* o)
    {
        if(o && !tree)
        {
            if(o->flag2 & OBJECT_SPEEDTREE)
            {
                if(o->name[0] && !strncmp(o->name, "speedtree", 9))
                {
                    tree = o;
                    return;
                }
            }

            Find(o->child);
            Find(o->sibling);
        }
    }
};

extern LOGICAL trnopCollideDown(piVECTOR* retpos, SINT32* poly, piVECTOR* pos);
extern LOGICAL trnopCollideDownWithDTL(piVECTOR* retpos, piVECTOR* pos, SINT32* poly, SINT32 iDTLIndex);
extern LOGICAL trnopCollide(piVECTOR const* oldpos, piVECTOR const* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly);
extern LOGICAL trnopConformBase(piVECTOR* rot, OBJECT* lpobj, SINT32 poly);
extern LOGICAL trnopOrientSubobjects(OBJECT* lpobj, piVECTOR* pos, bool nonPlanarBase);
extern void trnopRecalcBoundsAndNormals(OBJECT* Object);
extern SINT32 trnopHandleSpeedTree(OBJECT* group);
extern bool trnopDynamicBase(const FLOAT64VECTOR& worldPos,
										 piMATRIX worldAtt,
										 FLOAT32 voffset,
										 FLOAT32 hoffset,
										 FLOAT64VECTOR& obPos,
										 piVECTOR& obAtt);

extern void trnopLoadTrees(LOGICAL bLoad);
