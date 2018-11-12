#include "syscfg.h" // Path is project specific.

#include <math.h>
#include <queue>
#include "pi/pi.h"
#include "include/vars.h"
#include "trn/trnmain.h"
#include "trn/trnclutter.h"
#include "trn/trncd.h"
#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "draw/draw3d.h"
#include "ld/ldpix.h"
#include "fx/FXInterface.h"
#include "ob/obdlite.h"
#include "ob/obpool.h"
#include "rn/rnvis.h"
#include "math/matheul.h"
#include "ob/obMgr.h"
#include "math/mathnum.h"
#include "trn/speedgrass.h"
#include "ogl/ogl.h"
#include "ui/uiMain.h"
#include "spt/Random.h"
#include "wwii/wwiiHash.h"
#include "scen/ScenarioManager.h"
#include "sdm/sdmDefines.h"
#include "sdm/sdmReplacementOctedData.h"

#define USE_SPEEDTREE_FOR_CLUTTER
//#undef USE_SPEEDTREE_FOR_CLUTTER

#ifdef USE_SPEEDTREE_FOR_CLUTTER
#include "spt/sptmain.h"
#endif

struct CLUTTER
{
    bool kill;

	FLOAT64VECTOR Pos;
	piVECTOR Norm;
	SINT32 Type;
	UINT32 FlipFlop;

	UINT32	draw3d;
    UINT32 treeID;

	FLOAT32 Width;
	FLOAT32 Height;

	FLOAT32 Rotation;

	piMATRIX ObjMatrix;

    char grassid[64];
};

#include <algorithm>

unsigned int _id = 1;

using ClutterHash = std::unordered_set<std::string>;
static ClutterHash keys;
#define max_clutter_per_octet 64

using ClutterMap = std::multimap<std::string, CLUTTER*>;
static ClutterMap cluttermap;

static SINT32 _GetRandomNumberFromSeed(SINT32* Seed);

static UINT32 _AddClutterToFreeList(CLUTTER * Clutter);
#if(0)
static UINT32 _FreeClutter(CLUTTER * Clutter);
#endif
static CLUTTER * _GetNewClutter();
#if(0)
static void FreeClutter();
#endif

static UINT32 _RepopulateArray();

static bool _CollideAndSetupClutter(CLUTTER * Clutter,SINT32 PosX,SINT32 PosY, SINT32* ClutterDensity);

static Covert<bool> _drawclutter = true;

#define MAX_FREE_OBJECTS 1024

static CLUTTER * _GetNewClutter()
{
    CLUTTER* c = (CLUTTER*)memAlloc(sizeof(CLUTTER));
    return c;
}

// puts clutter onto free list
#if(0)
static UINT32 _FreeClutter(CLUTTER * Clutter)
{
	return 0;
}
#endif

static UINT32 _AddClutterToFreeList(CLUTTER* c)
{
    memFree(c);
    return 0;
}

UINT32 trnClutterInit()
{
	return 0;
}

UINT32 trnClutterKill()
{
    trnClutterReset();
    ClutterMap::iterator itr = cluttermap.begin();

    for(; itr != cluttermap.end(); ++itr)
    {
        memFree(itr->second);
    }

    cluttermap.clear();

	return 0;
}

#if(0)
static void FreeClutter()
{
}
#endif

UINT32 trnClutterUpdate()
{
    if(!uiInGame())
        return 0;

    if(gCustomTerrain)
        return 0;

	_RepopulateArray();

	return 0;
}

void trnClutterFreeAttributeIfNecessary(size_t key)
{
    // Grass collision failed somewhere
    // Just leave it on the list. When octets shift, it will be cleared.
}

static void SetupClutter(const std::string& key, int sx, int sy, int r, int c)
{
    unsigned int seed = sx * 4919 + sy * 7433 + r * 2371 + c * 6043;

    PNRandom::seedrandom(seed);

    // Bounds for this octet
    int x1 = (int)(sx * SDM_SUPERCELL_SIZE + c * SDM_OCTET_SIZE);
    int y1 = (int)(sy * SDM_SUPERCELL_SIZE + r * SDM_OCTET_SIZE);
    int x2 = (int)(x1 + SDM_OCTET_SIZE);
    int y2 = (int)(y1 + SDM_OCTET_SIZE);

    int density = 1;

    for(int i = 0; i < max_clutter_per_octet; i++)
    {
        int x = PNRandom::random(x1, x2);
        int y = PNRandom::random(y1, y2);

        CLUTTER* c = _GetNewClutter();

        if(!c)
            return;

        if(!_CollideAndSetupClutter(c, x, y, &density))
        {
            _AddClutterToFreeList(c);
            continue;
        }

        cluttermap.insert(std::make_pair(key, c));
    }
}

static void ClearClutter(const std::string& key)
{
    ClutterMap::iterator itr = cluttermap.find(key);

    if(itr == cluttermap.end())
        return;

    std::vector<ClutterMap::iterator> remove;

    while(itr != cluttermap.end())
    {
        if(key != itr->first)
            break;

        CLUTTER* c = itr->second;

        if(c->grassid[0])
        {
            speedgrassRemove(c->grassid);
        }

        _AddClutterToFreeList(c);

        remove.push_back(itr);

        ++itr;
    }

    for(unsigned int i = 0; i < remove.size(); i++)
    {
        cluttermap.erase(remove[i]);
    }

#if DEBUG_WW2
#if(0)
    if(freeclutter.size() + cluttermap.size() != MAX_FREE_OBJECTS)
    {
        int stop = 0;
    }
#endif
#endif
}

UINT32 trnClutterReset()
{
    ClutterHash::iterator itr = keys.begin();

    for(; itr != keys.end(); ++itr)
    {
        ClearClutter((*itr));
    }

    keys.clear();

    return 0;
}

static void GetGridCenter(int grid, const Vec3d& pos, Vec3d& center)
{
    // [0 ][1 ][2 ][3 ]
    // [4 ][5 ][6 ][7 ]
    // [8 ][9 ][10][11]
    // [12][13][14][15]
    if(grid == 0)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
    }
    else if(grid == 1)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
    }
    else if(grid == 2)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
    }
    else if(grid == 3)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
    }
    else if(grid == 4)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE_2;
    }
    else if(grid == 5)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE_2;
    }
    else if(grid == 6)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE_2;
    }
    else if(grid == 7)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
        center.y() = pos.y() + SDM_OCTET_SIZE_2;
    }
    else if(grid == 8)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE_2;
    }
    else if(grid == 9)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE_2;
    }
    else if(grid == 10)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE_2;
    }
    else if(grid == 11)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE_2;
    }
    else if(grid == 12)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
    }
    else if(grid == 13)
    {
        center.x() = pos.x() - SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
    }
    else if(grid == 14)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
    }
    else if(grid == 15)
    {
        center.x() = pos.x() + SDM_OCTET_SIZE + SDM_OCTET_SIZE_2;
        center.y() = pos.y() - SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
    }
}

static UINT32 _RepopulateArray()
{
    if(!_drawclutter)
    {
        return 0;
    }

    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
    {
        if(SuppressPlaneClutter)
        {
            return 0;
        }
    }

    logTimerBeginMacro();

    const mlVehicleData* data = persGetPersonaModelDataPtr();

    if(data == nullptr)
        return 0;

    Vec3d pos(data->VehiclePos);
    Vec3d center;
    int sx, sy, r, c;

    static ClutterHash wantedkeys;
    wantedkeys.clear();

    char key[64];

    for(int i = 0; i < 16; i++)
    {
        GetGridCenter(i, pos, center);
        trnmainGetSupercellAndOctet(center, sx, sy, r, c);

        strprintf(key, "clutter%d%d%d%d", sx, sy, r, c);
        wantedkeys.insert(key);

        if(keys.find(key) == keys.end())
        {
            SetupClutter(key, sx, sy, r, c);
            keys.insert(key);
        }
    }

    ClutterHash::iterator itr = keys.begin();
    ClutterHash::iterator free;

    while(itr != keys.end())
    {
        if(wantedkeys.find((*itr)) == wantedkeys.end())
        {
            free = itr;
            ++itr;
            ClearClutter((*free));
            keys.erase(free);
            continue;
        }

        ++itr;
    }

    logTimerEndMacro();

	return 0;
}

static bool _CollideAndSetupClutter(CLUTTER * Clutter,SINT32 PosX, SINT32 PosY, SINT32* ClutterDensity)
{
	piVECTOR P1,P2;
	UINT32			NumCollides;
	FASTCOLLIDE		CollisionList[32];
	FASTCOLLIDE*		CollideListPtr;
	COLLISION		collision;
	piVECTOR TempVec;
    //piVECTOR TempVec1;
	SINT32 TexId;
	LOGICAL retval;
	piMATRIX TempMat;
	SINT32 i;
	SINT32 ClutterType[3];
	UINT32 ClutterNum;
	FLOAT32 Variance;
	const SINT8 * TexNamePtr = nullptr;
	//piMATRIX Mat1,Mat2;
	UINT32 Temp32;
	SINT32 TempS32;
	UINT32 Found;

	mlVehicleData*	VehicleData = persGetPersonaModelDataPtr();

	FLOAT64VECTOR WorldOriginOffset;
	trnGetWorldOriginOffset(&WorldOriginOffset);

	Clutter->Pos.x = PosX;
	Clutter->Pos.y = PosY;
	Clutter->Pos.z = 0.0;

	ClutterNum = 0;

	// determine random adjustments

	Temp32 = PosX * 1103515245;
	Temp32 *= PosY * 1103515245;

	// hash up the x and y a bit to make a semi-chaotic seed for the random number generator
	Temp32 = (SINT32)((SINT64)PosX * (SINT64)113510315245LL *
						(SINT64)PosY * (SINT64)103515051525LL);
	TempS32 = (SINT32)(Temp32);

    ClutterNum = _GetRandomNumberFromSeed(&TempS32) % 3;

	Clutter->Rotation = PI * (_GetRandomNumberFromSeed(&TempS32) / 32767.0f);

	Variance = (_GetRandomNumberFromSeed(&TempS32) / 32767.0f) * 0.2f + 0.8f;

	P1.x = (FLOAT32)(Clutter->Pos.x - WorldOriginOffset.x);
	P1.y = (FLOAT32)(Clutter->Pos.y - WorldOriginOffset.y);
	P1.z = -10000.0;
	P2 = P1;
	P2.z = 10000.0;

    trnCDTerrainObject* to = nullptr;

	TexId = -1;
	memset(&CollisionList[0],0,sizeof(FASTCOLLIDE)*32);
	if (VehicleData != nullptr)
	{
		NumCollides = 32;
		CollideListPtr = CollisionList;

        // Not necessary to collide with trees.
        trncdIgnoreTreeCollide(TRUE);
		trncdCollideWithListOfTerrainObjects(&CollideListPtr,
											 &NumCollides,
											 VehicleData->MedTrnList,
											 ML_NUM_MED_LIST_ELEMENTS,
											 &P1,
											 &P2,
											 0);
        trncdIgnoreTreeCollide(FALSE);

		NumCollides = 32 - NumCollides;

        // If this is greater than one, we have probably collided with a building.
        // This hopefully will prevent the random clutter in buildings.
        if(NumCollides > 1)
        {
            return false;
        }

		if (NumCollides == 0)
		{
			memset(&collision, 0, sizeof(collision));
			collision.object   = nullptr;
			collision.oldpos.x = P1.x;
			collision.oldpos.y = P1.y;
			collision.oldpos.z = P1.z;
			collision.newpos.x = P2.x;
			collision.newpos.y = P2.y;
			collision.newpos.z = P2.z;

			retval = trncdCheckTerrain(&collision);

			if (retval)
			{
				Clutter->Pos.z = -(collision.colpos.z + collision.colobj->mat._43);
				Clutter->Norm = collision.colobj->psnorms[collision.colpoly];

				TexId = collision.colobj->texid;
				TexNamePtr = collision.colobj->texname;

                to = trncdFindTerrainObject(collision.colobj);
			}
		}
		else
		{
			mlUtilitySortCollideList(&CollisionList[0],NumCollides);

			Found = 0;

			for (i=NumCollides-1;i>=0;i--)
			{
				if (CollisionList[i].Alpha > 0.5f)
				{
					Found = 1;
					break;
				}
			}

			if (Found == 1)
			{
				if (CollisionList[i].CollideObject != nullptr)
				{
                    to = CollisionList[i].TO;

					TexId = CollisionList[i].CollideObject->texid;
					TexNamePtr = CollisionList[i].CollideObject->texname;
					math3dVectorMultiply(&TempVec,&CollisionList[i].CollidePnt,&CollisionList[i].CollideObject->mat);
					Clutter->Pos.z = -TempVec.z;

					Clutter->Norm = CollisionList[i].CollideObject->psnorms[CollisionList[i].PolyNum];
					TempMat = CollisionList[i].CollideObject->mat;
					TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
					math3dVectorMultiply(&Clutter->Norm,&Clutter->Norm,&TempMat);
				}
			}
			else
			{
				memset(&collision, 0, sizeof(collision));
				collision.object   = nullptr;
				collision.oldpos.x = P1.x;
				collision.oldpos.y = P1.y;
				collision.oldpos.z = P1.z;
				collision.newpos.x = P2.x;
				collision.newpos.y = P2.y;
				collision.newpos.z = P2.z;

				retval = trncdCheckTerrain(&collision);

				if (retval)
				{
					Clutter->Pos.z = -(collision.colpos.z + collision.colobj->mat._43);
					Clutter->Norm = collision.colobj->psnorms[collision.colpoly];

					TexId = collision.colobj->texid;
					TexNamePtr = collision.colobj->texname;

                    to = trncdFindTerrainObject(collision.colobj);
				}
			}
		}
	}

	if (TexId != -1)
	{
		mlGetGroundClutterStuff(&ClutterType[0],
								&ClutterType[1],
								&ClutterType[2],
								&Clutter->Width,
								&Clutter->Height,
								ClutterDensity,
								TexNamePtr);

		Clutter->Type = ClutterType[ClutterNum];
	}

    if(Clutter->Type == 0)
        return false;

    // Replace old bush with speedtree bush
#ifdef USE_SPEEDTREE_FOR_CLUTTER

    switch(Clutter->Type)
    {
    case 9:
        {
            FLOAT64VECTOR pos;

            pos.x = Clutter->Pos.x;
            pos.y = Clutter->Pos.y;
            pos.z = -Clutter->Pos.z;

            strprintf(Clutter->grassid, "clutter%d", _id++);

            const char* type = speedgrassGetGrassByPos(pos);

#if USE_HEIGHT_AT
            if(trnmainIsReplacement(pos))
#else
            if(1)
#endif
            {
                if(!speedgrassAddAndCollide(pos, type, Clutter->grassid, 1000.0f, to))
                {
                    Clutter->grassid[0] = 0;
                    return false;
                }
            }
            else
            {
#if USE_HEIGHT_AT
                if(!speedgrassAddPatch(pos, type, Clutter->grassid, 1000.0f))
                {
                    Clutter->grassid[0] = 0;
                    return false;
                }
#endif
            }

            return true;
        }
    }

    return false;

#endif
}

// simple, but predictable random number generator
static SINT32 _GetRandomNumberFromSeed(SINT32* Seed)
{
    *Seed = *Seed * 1103515245 + 12345;
    return((*Seed >> 16) & 0x7FFF);
}

void trnClutterSetDrawFlag(bool draw)
{
    _drawclutter = draw;

    ClutterHash::iterator itr = keys.begin();

    for(; itr != keys.end(); ++itr)
    {
        ClearClutter((*itr));
    }

    keys.clear();
}
