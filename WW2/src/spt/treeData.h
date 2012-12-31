#pragma once

#include "math/Vec3d.h"
#include "math/Vec4d.h"
#include "math/Matrixd.h"
#include "math/Matrixf.h"
#include "ogl/oglVertexBuffer.h"
#include "physics/colliderMain.h"
#include "physics/physicsTypes.h"
#include "math/mathUnitConversion.h"

#include <string>
#include <vector>

#define SCALE_TREES 0

#define SET_IS_BUSH(x, y) x |= (y << 15)
#define SET_IS_CLUTTER(x, y) x |= (y << 14)
#define SET_BASE(x, y) x |= y
#define IS_BUSH(x) (x >> 15) & 0x1
#define IS_CLUTTER(x) (x >> 14) & 0x1
#define GET_BASE(x) x & 0x001F

// Smart Heap should handle the 'free list' memory for us
#define USE_PREALLOCATED_TREES 0

class CTree;

typedef struct TreeData
{
    std::string name;

    Vec3d position;

    CTree* base;

    float nearlod;
    float farlod;

    TreeData()
    {
        base = NULL;
        nearlod = 0;
        farlod = 0;
    }

} TreeData;

struct Tree
{
#if USE_PREALLOCATED_TREES
    struct Tree* next;
    struct Tree* prev;
#endif

    unsigned short isbush_isclutter_base;
    unsigned char rot_angle;

    TDTL_TREES* octet;

    float distance;

    Vec3f worldpos;
    Vec4f rotation;

    // In case multiple collision objects
    std::vector<Quat> quat;

    OBJECT* o;

    char branchlod;
    char frondlod;
    bool leaflods[2];
    unsigned char frondalpha;
    unsigned char branchalpha;
    unsigned char leafalphas[2];

    Tree()
    {
        octet = NULL;
        o = NULL;
        rot_angle = 0;
        distance = 0.0f;
        octet = 0;
        isbush_isclutter_base = 0;
    }

    ~Tree()
    {
    }
};

extern void treeDataInit(const char* path, CSpeedWind* wind);
extern void treeDataFreeTree(Tree* t);
extern void treeDataFreeTree(unsigned int id);
extern void treeDataKill();
extern void treeClampBaseIfNecessary(int& base);

extern unsigned int treeDataGetBaseSize();
extern int treeDataGetTotalTrees();
extern CTree* treeDataGetBaseTree(int base);
extern Tree* treeDataGetFreeTree();
extern const std::vector<float>& treeDataGetBillboardTexCoords();
extern oglVertexBuffer* treeDataGetVBO();
extern bool treeDataIsValid(unsigned int address);
