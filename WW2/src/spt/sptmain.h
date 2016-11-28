#pragma once
// Prototypes.

#include "ob/obmain.h"
#include "ob/obcd.h"
#include "trn/trnmain.h"
#include "math/Vec4f.h"

#define DEF_TREE_SHADER         "tree_v.cg"
#define DEFAULT_TREE_PATH       "terrain\\trees\\"
#define DEF_SHADER_PATH         "fx\\shaders\\"
#define SPEED_WIND_INI          "SpeedWind.ini"

#define DEF_USE_GPU_LEAF_PLACEMENT
//#undef DEF_USE_GPU_LEAF_PLACEMENT

// Bush Clutter
enum bushtypes
{
    Azalea = 9,
    AmericanBoxwood1 = 10,
    AmericanBoxwood2 = 11,
    AmericanBoxwood3 = 18,
    AmericanBoxwood4 = 19,
    AmericanBoxwood5 = 20,
    MaxBushes = 6
};

struct TreeInstance
{
    TreeInstance* next;
    TreeInstance* prev;

    SINT32 ID;
    SINT32 base;

    UCHAR active;
    UCHAR octet;

    TERRAIN_DETAIL* dtl;

    FLOAT32 lodlevel;
    FLOAT32 distance;
    FLOAT32 radius;
    FLOAT32 halfradius;

    bool isbush;

    FLOAT64VECTOR initialpos;
    piVECTOR relativepos;

    OBJECT* o;
};

enum ENUMTREEPARTS
{
    TREE_BRANCH,
    TREE_FRONDS,
    TREE_LEAVES,
    MAX_TREE_PARTS
};

extern void sptUpdate();
extern void sptUpdateWindStrengths();
extern void sptExitApp();
extern void sptDisplayDeprecated();
extern void sptDraw();
extern void sptGetStats(SCHAR szStats[]);
extern void sptSortTrees();
extern void sptDrawRemainingBillboards();
extern void sptDrawTransitionBillboards();
extern void sptSetupWindMatrices();
extern void sptCull();
extern void sptAddCluster(OBJECT* o, SINT32* types/*[256]*/);
extern void sptAddCluster(OBJECT* o, SINT32* types/*[256]*/);
#if(0)
extern void sptFreeCluster(OBJECT* o);
#endif

extern void sptDrawOn(int type, bool draw);

extern bool sptmainInit();
extern bool sptDrawTreeShadowCasters();

extern bool sptCollideWithTree(unsigned int address,
                        piVECTOR const* pt1,
                        piVECTOR const* pt2,
                        piVECTOR* collidePt,
                        float* time);

extern bool sptIsTreeCollidable(short base);
extern bool sptInTreeBBox(const piVECTOR& pos, FLOAT32 altRadius = 0);
extern bool sptIsRadiusInTreeBBox(const piVECTOR& pos, float radius);

extern bool sptCollideWithCluster(OBJECT* o,
                           const piVECTOR* p1,
                           const piVECTOR* p2,
                           piVECTOR* collisionpoint,
                           float* t,
                           int* tri);

extern unsigned int sptAddTree(OBJECT* o,
                        int base,
                        TERRAIN_DETAIL* dtl,
                        FLOAT64VECTOR& pos,
                        bool isclutter = false,
                        bool changecluster = false);

extern void sptFreeTreeByObject(OBJECT* o);
extern void sptFreeTreeByID(UINT32 ID);
extern unsigned int sptAddTreeCommand(int base, const Vec3f& pos, bool FromHeightMap = false, float shadow = 1.0f);
extern void sptFreeClutter(TDTL_TREES* trees);
extern void sptFreeTrees(TDTL_TREES* trees);
extern bool sptCollide(const piVECTOR& pt1, const piVECTOR& pt2, piVECTOR& collidept, float& time, int& tri);
extern bool sptCollide(const piVECTOR& pt1, const piVECTOR& pt2, FASTCOLLIDE** list, unsigned int& NumSlotsLeft);
