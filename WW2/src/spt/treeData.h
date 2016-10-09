#pragma once

#include "math/Vec3d.h"
#include "math/Vec4d.h"
#include "math/Matrixd.h"
#include "math/Matrixf.h"
#include "ogl/oglVertexBuffer.h"
#include "physics/colliderMain.h"

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

struct TreeData
{
    std::string name;

    Vec3d position;

    CTree* base;

    float nearlod;
    float farlod;

    TreeData()
    {
        base = nullptr;
        nearlod = 0;
        farlod = 0;
    }
};

struct Tree
{
#if USE_PREALLOCATED_TREES
    struct Tree* next;
    struct Tree* prev;
#endif

    unsigned short isbush_isclutter_base;
    unsigned short rot_angle;

    TDTL_TREES* octet;

    float distance;
    float radius;

    Vec3d worldpos;
    Vec4f rotation;

    // In case multiple collision objects
    std::vector<Matrixf> collidemat;
    std::vector<Matrixf> collidematinv;

    OBJECT* o;

    char branchlod;
    char frondlod;
    bool leaflods[2];
    unsigned char frondalpha;
    unsigned char branchalpha;
    unsigned char leafalphas[2];

    std::vector<dGeomID> transform;

#ifdef SPATIAL_DB
    void* sdbInfo;
    float smallradius;
#endif

    Tree()
    {
        octet = nullptr;
        o = nullptr;
        rot_angle = 0;
        radius = 0.0f;
        distance = 0.0f;
        octet = 0;
        isbush_isclutter_base = 0;
#ifdef SPATIAL_DB
        sdbInfo = nullptr;
        smallradius=0.0f;
#endif
    }

    ~Tree()
    {
        if(!transform.empty())
        {
            for(unsigned int i = 0; i < transform.size(); i++)
            {
                dGeomDestroy(transform[i]);
            }
            
            transform.clear();
        }
		detach();
    }

	void attach(OBJECT&);
	void detach(void);
};

void treeDataInit(const char* path, CSpeedWind* wind);
void treeDataFreeTree(Tree* t);
void treeDataFreeTree(unsigned int id);
void treeDataKill();
void treeClampBaseIfNecessary(int& base);

unsigned int treeDataGetBaseSize();
int treeDataGetTotalTrees();
CTree* treeDataGetBaseTree(int base);
Tree* treeDataGetFreeTree();
const std::vector<float>& treeDataGetBillboardTexCoords();
oglVertexBuffer* treeDataGetVBO();
bool treeDataIsValid(unsigned int address);
