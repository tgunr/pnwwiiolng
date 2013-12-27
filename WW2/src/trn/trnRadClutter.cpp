#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////////////
// Module: trnRadClutter.cpp
// Description: Radial clutter.
/////////////////////////////////////////////////////////
#include "trn/trnRadClutter.h"

#include "mem/mem.h"
#include "include/vars.h"
#include "pers/persmain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "spt/Random.h"
#include "ml/mltypes.h"
#include "trn/speedgrass.h"
#include "trn/trnvars.h"
#include "math/Vec2f.h"
#include "trn/trnmain.h"
#include "physics/colliderMain.h"
#include "physics/physicsmain.h"
#include "math/mathnum.h"
#include "scen/ScenarioManager.h"
#include "math/SuperFastHash.h"
#include "sdm/sdmDefines.h"

#define max_attributes          4096
#define max_movement            2
#define min_density             1
#define min_radius              60.0f

#define max_flora_1               28
#define max_flora_2               12
#define max_flora_3               8

#define max_width_1               120.0f
#define max_width_2               150.0f
#define max_width_3               180.0f

static float _maxmove2 = max_movement * max_movement;

static float distclamp2_1 = 0;
static float distclamp2_2 = 0;
static float distclamp2_3 = 0;
static float radial_radius_1 = 0;
static float radial_radius_2 = 0;
static float radial_radius_3 = 0;
static float rad_width_1 = 0;
static float rad_width_2 = 0;
static float rad_width_3 = 0;
static float olddensity = 0.0f;
static float oldradius = 0.0f;

static bool _enabled = false;
static bool _freeclutter = false;
static bool _dirty = false;

static int _density_1 = 0;
static int _density_2 = 0;
static int _density_3 = 0;
static int _id = 1;

static std::queue<Attributes*> _free;

static Attributes* _attributes;
static std::set<std::string> radcluttermap_1;
static std::set<std::string> radcluttermap_2;
static std::set<std::string> radcluttermap_3;

typedef std::map<size_t, Attributes*> AttributeMap;
static AttributeMap active;

static void CheckMovement_1();
static void CheckMovement_2();
static void CheckMovement_3();
static void FreeNode(Attributes* free);
static void PlaceClutter_1(const Vec3d& pos);
static void PlaceClutter_2(const Vec3d& pos);
static void PlaceClutter_3(const Vec3d& pos);
static void RemoveClutterIfNecessary();

static Attributes* GetFreeAttribute();

#define DEBUG_GRASS 0

#if DEBUG_GRASS
static int changed[9];
static FLOAT64VECTOR points[9];
#endif

/////////////////////////////////////////////////////////
// Function: radclutterDirty
// Description:
/////////////////////////////////////////////////////////
void radclutterDirty()
{
    _dirty = true;
}

/////////////////////////////////////////////////////////
// Function: SetToKill
// Description:
/////////////////////////////////////////////////////////
void static SetToKill(Attributes* a)
{
    _freeclutter = true;
    a->kill = true;
}

/////////////////////////////////////////////////////////
// Function: radclutterRemoveAll
// Description:
/////////////////////////////////////////////////////////
void radclutterRemoveAll()
{
    AttributeMap::iterator itr = active.begin();

    for(; itr != active.end(); ++itr)
    {
        SetToKill(itr->second);
    }

    radcluttermap_1.clear();
    radcluttermap_2.clear();
    radcluttermap_3.clear();
}

/////////////////////////////////////////////////////////
// Function: radclutterEnable
// Description:
/////////////////////////////////////////////////////////
void radclutterEnable(bool enable)
{
    _enabled = enable;
}

/////////////////////////////////////////////////////////
// Function: radclutterKill
// Description:
/////////////////////////////////////////////////////////
void radclutterKill()
{
    while(!_free.empty())
    {
        _free.pop();
    }

    memFree(_attributes);

    Write_Log(LOG_OLD, LOG_INFO, "[CRadClutter::Kill]");
}

/////////////////////////////////////////////////////////
// Function: SetupParameters
// Description:
/////////////////////////////////////////////////////////
static void SetupParameters()
{
    rad_width_1 = (float)FloatToInt(min_radius + g_fRadialRadius * (max_width_1 - min_radius));
    rad_width_2 = (float)FloatToInt(min_radius + g_fRadialRadius * (max_width_2 - min_radius));
    rad_width_3 = (float)FloatToInt(min_radius + g_fRadialRadius * (max_width_3 - min_radius));

    radial_radius_1 = rad_width_1 * 0.5f;
    radial_radius_2 = rad_width_2 * 0.5f;
    radial_radius_3 = rad_width_3 * 0.5f;

    // Make density related to distance. Using similar triangles makes it too dense.
    _density_1 = FloatToInt(min_density + g_fRadialDensity * (max_flora_1 - min_density));
    _density_2 = FloatToInt(min_density + g_fRadialDensity * (max_flora_2 - min_density));
    _density_3 = FloatToInt(min_density + g_fRadialDensity * (max_flora_3 - min_density));

    _density_1 = std::max(min_density, FloatToInt(g_fRadialRadius * (float)_density_1));
    _density_2 = std::max(min_density, FloatToInt(g_fRadialRadius * (float)_density_2));
    _density_3 = std::max(min_density, FloatToInt(g_fRadialRadius * (float)_density_3));

    distclamp2_1 = radial_radius_1 * radial_radius_1 + radial_radius_1 * radial_radius_1;
    distclamp2_2 = radial_radius_2 * radial_radius_2 + radial_radius_2 * radial_radius_2;
    distclamp2_3 = radial_radius_3 * radial_radius_3 + radial_radius_3 * radial_radius_3;

    olddensity = g_fRadialDensity;
    oldradius = g_fRadialRadius;
}

/////////////////////////////////////////////////////////
// Function: radclutterInit
// Description:
/////////////////////////////////////////////////////////
void radclutterInit()
{
    _attributes = (Attributes*)memAlloc(sizeof(Attributes) * max_attributes);

    if(_attributes == NULL)
    {
        g_bRadialClutter = FALSE;
        return;
    }

    for(int i = 0; i < max_attributes; i++)
    {
        _free.push(&_attributes[i]);
    }

    SetupParameters();
}

/////////////////////////////////////////////////////////
// Function: FreeClutter
// Description:
/////////////////////////////////////////////////////////
static void FreeClutter()
{
    // Only loop if necessary
    if(!_freeclutter)
        return;

    _freeclutter = false;

    AttributeMap::iterator itr = active.begin();
    AttributeMap::iterator free;

    while(itr != active.end())
    {
        if(itr->second->kill)
        {
            free = itr;
            ++itr;

            FreeNode(free->second);
            active.erase(free);

            continue;
        }

        ++itr;
    }
}

/////////////////////////////////////////////////////////
// Function: radclutterUpdate
// Description:
/////////////////////////////////////////////////////////
void radclutterUpdate()
{
    if(gCustomTerrain)
        return;

    if(!_enabled)
    {
        radclutterRemoveAll();
        FreeClutter();
        return;
    }

    if(_dirty)
    {
        // Enabled set in gmlpmain each frame
        if(olddensity != g_fRadialDensity ||
           oldradius != g_fRadialRadius)
        {
            radclutterRemoveAll();
            SetupParameters();
        }

        _dirty = false;
    }

    RemoveClutterIfNecessary();

    CheckMovement_3();
    CheckMovement_2();
    CheckMovement_1();

    FreeClutter();
}

/////////////////////////////////////////////////////////
// Function: RemoveClutterIfNecessary
// Description:
/////////////////////////////////////////////////////////
void RemoveClutterIfNecessary()
{
    vehVehicle* veh = persGetPersonaVehiclePtr();

    if(veh == NULL)
        return;

    if(veh->ModelData == NULL)
        return;

    Vec2f p1, p2;

    p1.x() = float(veh->ModelData->VehiclePos.x - WorldOriginOffset.x);
    p1.y() = float(veh->ModelData->VehiclePos.y - WorldOriginOffset.y);

    int count = 0;

    AttributeMap::iterator itr = active.begin();

    for(; itr != active.end(); ++itr)
    {
        p2.x() = (float)(itr->second->pos.x - WorldOriginOffset.x);
        p2.y() = (float)(itr->second->pos.y - WorldOriginOffset.y);

        float dis2 = (p2 - p1).length2();

        if(dis2 > itr->second->maxdist)
        {
            SetToKill(itr->second);
        }
    }
}

/////////////////////////////////////////////////////////
// Function: GetFreeAttribute
// Description:
/////////////////////////////////////////////////////////
Attributes* GetFreeAttribute()
{
    if(_free.empty())
        return NULL;

    Attributes* attr = _free.front();
    _free.pop();

    attr->kill = false;
    attr->key[0] = 0;
    attr->maxdist = 0;
    attr->pos.x = 0;
    attr->pos.y = 0;
    attr->pos.z = 0;

    return attr;
}

/////////////////////////////////////////////////////////
// Function: FreeNode
// Description:
/////////////////////////////////////////////////////////
void FreeNode(Attributes* free)
{
    if(free->key[0])
    {
        speedgrassRemove(free->key);
    }

    _free.push(free);
}

/////////////////////////////////////////////////////////
// Function: radclutterFreeAttributeIfNecessary
// Description: Need this to free attributes from speedgrass
// that have been queued up and failed for whatever reason.
/////////////////////////////////////////////////////////
void radclutterFreeAttributeIfNecessary(size_t key)
{
    AttributeMap::iterator itr = active.find(key);

    if(itr != active.end())
    {
        SetToKill(itr->second);
    }
}

/////////////////////////////////////////////////////////
// Function: PlaceClutter_1
// Description:
/////////////////////////////////////////////////////////
void PlaceClutter_1(const Vec3d& pos)
{
    float half = (rad_width_1 / 3.0f) * 0.5f;

    for(int i = 0; i < _density_1; i++)
    {
        Attributes* attr = GetFreeAttribute();

        if(attr)
        {
            const char* type = "radclutter";
            attr->pos.x = pos.x() + PNRandom::random(-half, half);
            attr->pos.y = pos.y() + PNRandom::random(-half, half);
            attr->pos.z = pos.z();
            attr->maxdist = distclamp2_1;

            sprintf(attr->key, "radclutter%d", _id++);
            size_t key = SuperFastHash(attr->key, sizeof(char) * strlen(attr->key));

            active[key] = attr;

            if(!speedgrassAddAndCollide(Vec3d(attr->pos), type, key, max_width_1 / 2.0f))
            {
                SetToKill(attr);
            }

            if(_id > 32768)
                _id = 0;
        }
    }
}

/////////////////////////////////////////////////////////
// Function: PlaceClutter_2
// Description:
/////////////////////////////////////////////////////////
void PlaceClutter_2(const Vec3d& pos)
{
    float half = (rad_width_2 / 3.0f) * 0.5f;

    for(int i = 0; i < _density_2; i++)
    {
        Attributes* attr = GetFreeAttribute();

        if(attr)
        {
            const char* type = speedgrassGetRandomRadialType();
            attr->pos.x = pos.x() + PNRandom::random(-half, half);
            attr->pos.y = pos.y() + PNRandom::random(-half, half);
            attr->pos.z = pos.z();
            attr->maxdist = distclamp2_2;

            sprintf(attr->key, "radclutter%d", _id++);
            size_t key = SuperFastHash(attr->key, sizeof(char) * strlen(attr->key));

            active[key] = attr;

            if(!speedgrassAddAndCollide(Vec3d(attr->pos), type, key, max_width_2 / 2.0f))
            {
                SetToKill(attr);
            }

            if(_id > 32768)
                _id = 0;
        }
    }
}

/////////////////////////////////////////////////////////
// Function: PlaceClutter_3
// Description:
/////////////////////////////////////////////////////////
void PlaceClutter_3(const Vec3d& pos)
{
    float half = (rad_width_3 / 3.0f) * 0.5f;

    for(int i = 0; i < _density_3; i++)
    {
        Attributes* attr = GetFreeAttribute();

        if(attr)
        {
            const char* type = speedgrassGetRandomRadialType();
            attr->pos.x = pos.x() + PNRandom::random(-half, half);
            attr->pos.y = pos.y() + PNRandom::random(-half, half);
            attr->pos.z = pos.z();
            attr->maxdist = distclamp2_3;

            sprintf(attr->key, "radclutter%d", _id++);
            size_t key = SuperFastHash(attr->key, sizeof(char) * strlen(attr->key));

            active[key] = attr;

            if(!speedgrassAddAndCollide(Vec3d(attr->pos), type, key, max_width_3 / 2.0f))
            {
                SetToKill(attr);
            }

            if(_id > 32768)
                _id = 0;
        }
    }
}

/////////////////////////////////////////////////////////
// Function: GetPositionKey
// Description:
/////////////////////////////////////////////////////////
bool GetPositionKey(FLOAT64VECTOR& pos, char buff[128], Vec3d& center, float w)
{
    // Get supercell
    int sx = (int)(pos.x / SDM_SUPERCELL_SIZE);
    int sy = (int)(pos.y / SDM_SUPERCELL_SIZE);

    sx = sx < 0 ? sx - 1 : sx;
    sy = sy < 0 ? sy - 1 : sy;

    piVECTOR v;

    // Get position within a supercell
    v.x = float(pos.x - sx * SDM_SUPERCELL_SIZE);
    v.y = float(pos.y - sy * SDM_SUPERCELL_SIZE);

    // Determine row/column in the supercell to octet level
    int c = (int)(v.x / SDM_OCTET_SIZE);
    int r = (int)(v.y / SDM_OCTET_SIZE);

    // Pos relative to octet
    v.x = v.x - c * SDM_OCTET_SIZE;
    v.y = v.y - r * SDM_OCTET_SIZE;

    float size = w / 3.0f;

    int cc = (int)(v.x / size);
    int rr = (int)(v.y / size);

    sprintf(buff, "rclutter%d%d%d%d%d%d", sx, sy, r, c, rr, cc);

    center.x() = sx * SDM_SUPERCELL_SIZE + c * SDM_OCTET_SIZE + cc * size;
    center.y() = sy * SDM_SUPERCELL_SIZE + r * SDM_OCTET_SIZE + rr * size;

    // Move to center of box
    center.x() += size * 0.5f;
    center.y() += size * 0.5f;
    center.z() = pos.z;

    return true;
}

/////////////////////////////////////////////////////////
// Function: CheckMovement_1
// Description:
/////////////////////////////////////////////////////////
void CheckMovement_1()
{
    if(!_enabled)
        return;

    vehVehicle* veh = persGetPersonaVehiclePtr();

    if(veh == NULL)
        return;

    if(veh->ModelData == NULL)
        return;

    Vec2f dis;
    static piVECTOR oldpos = {0, 0, 0};

    dis.x() = -playpos.x - oldpos.x;
    dis.y() = -playpos.y - oldpos.y;

    float dis2 = dis.length2();

    if(dis2 < _maxmove2)
    {
        return;
    }

    oldpos.set(-playpos.x, -playpos.y, 0.0f);

    FLOAT64VECTOR pos = veh->ModelData->VehiclePos;
    pos.z = -pos.z;

    static float offsets[9][2];
    float size = (rad_width_1 / 3.0f);

    char key[128] = {0};
    bool change = false;
    Vec3d center;

    offsets[0][0] = -size;
    offsets[0][1] = -size;

    offsets[1][0] = 0.0f;
    offsets[1][1] = -size;

    offsets[2][0] = size;
    offsets[2][1] = -size;

    offsets[3][0] = -size;
    offsets[3][1] = 0.0f;

    offsets[4][0] = 0.0f;
    offsets[4][1] = 0.0f;

    offsets[5][0] = size;
    offsets[5][1] = 0.0f;

    offsets[6][0] = -size;
    offsets[6][1] =  size;

    offsets[7][0] = 0.0f;
    offsets[7][1] = size;

    offsets[8][0] = size;
    offsets[8][1] = size;

    std::map<int, std::string> grid;

    bool ok = false;

#if DEBUG_GRASS
    int a[9];
#endif

    for(int i = 0; i < 9; i++)
    {
        pos = veh->ModelData->VehiclePos;
        pos.z = -pos.z;

        pos.x += offsets[i][0];
        pos.y += offsets[i][1];

        ok = GetPositionKey(pos, key, center, rad_width_1);

        if(!ok) // on water?
            continue;

#if DEBUG_GRASS
        a[i] = 0;
        center.get(points[i]);
#endif
        grid[i] = key;

        if(radcluttermap_1.find(key) == radcluttermap_1.end())
        {
            change = true;
#if DEBUG_GRASS
            a[i] = 1;
#endif
            PlaceClutter_1(center);
        }
    }

#if DEBUG_GRASS
    memcpy(changed, a, sizeof(int) * 9);
#endif

    if(change)
    {
        radcluttermap_1.clear();

        std::map<int, std::string>::iterator itr = grid.begin();

        for(; itr != grid.end(); ++itr)
        {
            radcluttermap_1.insert(itr->second);
        }
    }
}

/////////////////////////////////////////////////////////
// Function: CheckMovement_2
// Description:
/////////////////////////////////////////////////////////
void CheckMovement_2()
{
    if(!_enabled)
        return;

    vehVehicle* veh = persGetPersonaVehiclePtr();

    if(veh == NULL)
        return;

    if(veh->ModelData == NULL)
        return;

    Vec2f dis;
    static piVECTOR oldpos = {0, 0, 0};

    dis.x() = -playpos.x - oldpos.x;
    dis.y() = -playpos.y - oldpos.y;

    float dis2 = dis.length2();

    if(dis2 < _maxmove2)
    {
        return;
    }

    oldpos.set(-playpos.x, -playpos.y, 0.0f);

    FLOAT64VECTOR pos = veh->ModelData->VehiclePos;
    pos.z = -pos.z;

    static float offsets[9][2];
    float size = (rad_width_2 / 3.0f);

    char key[128] = {0};
    bool change = false;
    Vec3d center;

    offsets[0][0] = -size;
    offsets[0][1] = -size;

    offsets[1][0] = 0.0f;
    offsets[1][1] = -size;

    offsets[2][0] = size;
    offsets[2][1] = -size;

    offsets[3][0] = -size;
    offsets[3][1] = 0.0f;

    offsets[4][0] = 0.0f;
    offsets[4][1] = 0.0f;

    offsets[5][0] = size;
    offsets[5][1] = 0.0f;

    offsets[6][0] = -size;
    offsets[6][1] =  size;

    offsets[7][0] = 0.0f;
    offsets[7][1] = size;

    offsets[8][0] = size;
    offsets[8][1] = size;

    std::map<int, std::string> grid;

    bool ok = false;

#if DEBUG_GRASS
    int a[9];
#endif

    for(int i = 0; i < 9; i++)
    {
        pos = veh->ModelData->VehiclePos;
        pos.z = -pos.z;

        pos.x += offsets[i][0];
        pos.y += offsets[i][1];

        ok = GetPositionKey(pos, key, center, rad_width_2);

        if(!ok) // on water?
            continue;

#if DEBUG_GRASS
        a[i] = 0;
        center.get(points[i]);
#endif
        grid[i] = key;

        if(radcluttermap_2.find(key) == radcluttermap_2.end())
        {
            change = true;
#if DEBUG_GRASS
            a[i] = 1;
#endif
            PlaceClutter_2(center);
        }
    }

#if DEBUG_GRASS
    memcpy(changed, a, sizeof(int) * 9);
#endif

    if(change)
    {
        radcluttermap_2.clear();

        std::map<int, std::string>::iterator itr = grid.begin();

        for(; itr != grid.end(); ++itr)
        {
            radcluttermap_2.insert(itr->second);
        }
    }
}

/////////////////////////////////////////////////////////
// Function: CheckMovement_3
// Description:
/////////////////////////////////////////////////////////
void CheckMovement_3()
{
    if(!_enabled)
        return;

    vehVehicle* veh = persGetPersonaVehiclePtr();

    if(veh == NULL)
        return;

    if(veh->ModelData == NULL)
        return;

    Vec2f dis;

    static piVECTOR oldpos = {0, 0, 0};

    dis.x() = -playpos.x - oldpos.x;
    dis.y() = -playpos.y - oldpos.y;

    float dis2 = dis.length2();

    if(dis2 < _maxmove2)
    {
        return;
    }

    oldpos.set(-playpos.x, -playpos.y, 0.0f);

    FLOAT64VECTOR pos = veh->ModelData->VehiclePos;
    pos.z = -pos.z;

    static float offsets[9][2];
    float size = (rad_width_3 / 3.0f);

    char key[128] = {0};
    bool change = false;
    Vec3d center;

    offsets[0][0] = -size;
    offsets[0][1] = -size;

    offsets[1][0] = 0.0f;
    offsets[1][1] = -size;

    offsets[2][0] = size;
    offsets[2][1] = -size;

    offsets[3][0] = -size;
    offsets[3][1] = 0.0f;

    offsets[4][0] = 0.0f;
    offsets[4][1] = 0.0f;

    offsets[5][0] = size;
    offsets[5][1] = 0.0f;

    offsets[6][0] = -size;
    offsets[6][1] =  size;

    offsets[7][0] = 0.0f;
    offsets[7][1] = size;

    offsets[8][0] = size;
    offsets[8][1] = size;

    std::map<int, std::string> grid;

    bool ok = false;

#if DEBUG_GRASS
    int a[9];
#endif

    for(int i = 0; i < 9; i++)
    {
        pos = veh->ModelData->VehiclePos;
        pos.z = -pos.z;

        pos.x += offsets[i][0];
        pos.y += offsets[i][1];

        ok = GetPositionKey(pos, key, center, rad_width_3);

        if(!ok) // on water?
            continue;

#if DEBUG_GRASS
        a[i] = 0;
        center.get(points[i]);
#endif
        grid[i] = key;

        if(radcluttermap_3.find(key) == radcluttermap_3.end())
        {
            change = true;
#if DEBUG_GRASS
            a[i] = 1;
#endif
            PlaceClutter_3(center);
        }
    }

#if DEBUG_GRASS
    memcpy(changed, a, sizeof(int) * 9);
#endif

    if(change)
    {
        radcluttermap_3.clear();

        std::map<int, std::string>::iterator itr = grid.begin();

        for(; itr != grid.end(); ++itr)
        {
            radcluttermap_3.insert(itr->second);
        }
    }
}

#if DEBUG_GRASS
#include "draw/draw3d.h"
void DrawSquare(FLOAT64VECTOR& pos, float radius, unsigned int color)
{
    float x = float(pos.x - WorldOriginOffset.x);
    float y = float(pos.y - WorldOriginOffset.y);
    float z = -485.0f;//float(pos.z - 0.05);

    piVECTOR p1, p2;

    p1.set(x - radius, y + radius, z);
    p2.set(x + radius, y + radius, z);
    draw3dLine(&p1, &p2, color);

    p1 = p2;
    p2.set(x + radius, y - radius, z);
    draw3dLine(&p1, &p2, color);

    p1 = p2;
    p2.set(x - radius, y - radius, z);
    draw3dLine(&p1, &p2, color);

    p1 = p2;
    p2.set(x - radius, y + radius, z);
    draw3dLine(&p1, &p2, color);
}

void DrawLine(FLOAT64VECTOR& pos, unsigned int color)
{
    float x = float(pos.x - WorldOriginOffset.x);
    float y = float(pos.y - WorldOriginOffset.y);
    float z = -485.0f;//float(pos.z - 0.05);

    piVECTOR p1, p2;

    p1.set(x, y, z);
    p2.set(x, y, z - 100.0f);
    draw3dLine(&p1, &p2, color);
}
#endif

void trnclutterDrawDebug()
{
#if DEBUG_GRASS
    float radius = (rad_width / 3.0f) * 0.5f;

    DrawSquare(points[0], radius, colorORANGEALPHA);
    DrawSquare(points[1], radius, colorYELLOWALPHA);
    DrawSquare(points[2], radius, colorWHITEALPHA);
    DrawSquare(points[3], radius, colorREDALPHA);
    DrawSquare(points[4], radius, colorBLUEALPHA);
    DrawSquare(points[5], radius, colorYELLOWALPHA);
    DrawSquare(points[6], radius, colorWHITEALPHA);
    DrawSquare(points[7], radius, colorREDALPHA);
    DrawSquare(points[8], radius, colorBLUEALPHA);

    for(int i = 0; i < 9; i++)
    {
        if(changed[i])
            DrawLine(points[i], colorYELLOWALPHA);
        else
            DrawLine(points[i], colorBLUEALPHA);
    }
#endif
}
