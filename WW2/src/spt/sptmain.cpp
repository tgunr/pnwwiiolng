#include "syscfg.h" // Path is project specific.
/*---------------------------------------------------------------------
Module: sptmain.cpp
Description:
---------------------------------------------------------------------*/
#include "SpeedTreeRT.h"
#include "spt/sptmain.h"
#include "treeData.h"
#include "fio/fio.h"
#include "shader/glsl.h"
#include "shader/glslProgram.h"
#include "ob/obcd.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "trn/trnvars.h"
#include "trn/trncd.h"
#include "Tree.h"
#include "rn/rnvis.h"
#include "trn/trnsky.h"
#include "math/Matrixf.h"
#include "math/Matrixd.h"
#include "math/Vec2f.h"
#include "math/mathUnitConversion.h"
#include "spt/Random.h"
#include "ogl/oglGlobalStates.h"
#include "math/mathCollide.h"
#include "ui/uiChat.h"
#include "osgImport/Polytope.h"
#include "ob/obdlite.h"
#include "ob/obdraw.h"
#include "ogl/States.h"
#include "ui/uiDotCommands.h"
#include "shader/glslWater.h"
#include "pers/persmain.h"
#include "math/matheul.h"
#include "sdm/sdmDefines.h"
#include "wwii/wwiiThemes.h"

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

#include "wwii/wwiiHash.h"

#define MAX_TREE_PRIMITIVES  1000

#ifdef BETA_RELEASE
#define DEBUG_SPEEDTREE 0
#define DEBUG_CULLING 0
#else
#define DEBUG_SPEEDTREE 0
#define DEBUG_CULLING 0
#endif

#if DEBUG_SPEEDTREE
static Vec3f _oderay;
static Vec3f _odepos;
#if DEBUG_CULLING
static bool drawsupercellsandoctets = false;
struct rc
{
    int r;
    int c;
};

static std::map<int, std::vector<rc> > rowcolumn;
#endif // DEBUG_CULLING
#endif // DEBUG_SPEEDTREE

#define HORIZONTAL_INCREMENT_SIZE   60
#define VERTICAL_INCREMENT_SIZE     56
#define HALLOC_SIZE                 HORIZONTAL_INCREMENT_SIZE * 1000
#define VALLOC_SIZE                 VERTICAL_INCREMENT_SIZE * 1000
#define INDICE_SIZE                 6
#define IALLOC_SIZE                 INDICE_SIZE * 1000

// Used for billboards since std::vector resize does a mem copy
template<typename Type>
class FakeVector
{
private:
	enum { GrowthRate = 128 } ;	// Grow in units of 128. MUST BE POWER OF 2.
	enum { GrowthMask = GrowthRate - 1 };
public:
    FakeVector() : _data(0), _size(0), _capacity(0)
	{
		reserve(1024);
	}
    ~FakeVector()
    {
		FreeData();
    }

    unsigned int capacity() const {return _capacity;}
    unsigned int size() const {return _size;}

    bool empty() const {return _size == 0;}

	void FreeData()
	{
        if(_data)
        {
            memFree(_data);
            _data = nullptr;
        }
	}

    void reserve(unsigned int newcap)
    {
		newcap = (newcap + GrowthMask) & (~GrowthMask);
        if(!_data)
        {
            _data = (Type*)memAlloc(sizeof(Type) * newcap);
        }
        // realloc fails in debug
        else
        {
            void* old = _data;
            _data = (Type*)memAlloc(sizeof(Type) * newcap);
            memcpy(_data, old, sizeof(Type) * _size);
            memFree(old);
        }

        _capacity = newcap;
    }

    void resize(unsigned int newsize) {_size = newsize;}
    const Type& operator [](unsigned int position) const {return _data[position];}
    Type& operator [](unsigned int position) {return _data[position];}
    void clear() {_size = 0;}

private:
    Type* _data;
    unsigned int _size;
    unsigned int _capacity;
};

#define VisLimit    3200.0f

static std::map<int, float *> windMatrices;      // vector of float pointers storing wind matrices for each matrix defined by SpeedWind
using TreeMap = std::multimap<OBJECT*, Tree*>;
static TreeMap objectTreeMap;

static std::vector<float> windRustleAngles;     // table of leaf rustle angles shared by all base trees
static std::vector<float> windRockAngles;       // table of leaf rock angles shared by all base trees
static std::vector<float> leafAngleUploadTable; // used as a repository to batch shader constant uploads

#define MAX_TREE_TYPES 32

#include "osgImport/ShadowMap.h"
#include "osgImport/Polytope.h"
#include "scen/ScenarioManager.h"
static Polytope reflectionPolytope;
static std::vector<Tree*> reflectionTrees[MAX_TREE_TYPES];
static std::vector<Tree*> shadowTrees[MAX_TREE_TYPES];

static std::vector<Tree*> threeD[MAX_TREE_TYPES];
static std::vector<Tree*> bblist[MAX_TREE_TYPES];
static std::vector<int> dotcommands;

static colliderQuery* TreeCollider = nullptr;

static FakeVector<float> billboards[MAX_TREE_TYPES];
static FakeVector<float> horizbillboards[MAX_TREE_TYPES];
static FakeVector<unsigned short> billboardindices[MAX_TREE_TYPES];
static FakeVector<unsigned short> horizbillboardindices[MAX_TREE_TYPES];

static int billboardindiceoffset[MAX_TREE_TYPES] = {0};
static int horizbillboardindiceoffset[MAX_TREE_TYPES] = {0};

static int numbillboards = 0;
static int numhorizbillboards = 0;
static int number3dtrees = 0;
static int totaltriangles = 0;
static int horizindex = 0;

static float fovscale = 0.0f;
static float lodscale = 0.0f;

static float vislimit2;

static bool drawbranches = true;
static bool drawleaves = true;
static bool drawfronds = true;
static bool drawbillboards = true;
static bool drawhorizbillboards = true;
static bool drawcylinders = false;
static bool drawboundbox = false;
static bool drawtrees = true;
static bool nocull = false;
static bool cullnecessary = false;
static bool firsttime = true;

static CSpeedWind* speedwind = nullptr;

//---------------------------------------------------------------------

void Tree::attach(OBJECT& o) {
	detach();
	this->o = &o;

#if DEBUG_WW2
	// Make sure we're not adding trees to the treemap twice.
	auto rng = objectTreeMap.equal_range(&o);
	for ( auto it = rng.first ; it != rng.second ; ++it ) {
		assert ( it->second != this );
	}
#endif

	objectTreeMap.insert(std::make_pair(&o, this));
}


void Tree::detach() {
	if ( ! o )
		return;

	// Make sure we're not removing trees to the treemap twice.
	auto rng = objectTreeMap.equal_range(o);
	for ( auto it = rng.first ; it != rng.second ; ++it ) {
		if ( it->second != this ) continue;

		objectTreeMap.erase(it);

		if ( o->treeID == (UINT32)this )
			o->treeID = (UINT32)nullptr;
		return;
	}

	// If we didn't return during the loop,
	// we didn't find this tree in the map.
	assert ( false );
}

/*---------------------------------------------------------------------
Function: initializeWind
Description:
---------------------------------------------------------------------*/
static bool initializeWind(const std::string& path)
{
    std::string filepath = path + "SpeedWind.ini";

    size_t size = fioFileSize(filepath.c_str());

    if(size == 0)
    {
        return false;
    }

    int fd = fioOpen(filepath.c_str());

    if(fd < 0)
    {
        return false;
    }

    char* data = new char[size];
    fioRead(fd, data, size);
    fioClose(fd);

    if(!speedwind)
    {
        speedwind = new CSpeedWind;
    }

	bool result = speedwind->Load(data, size);
	delete[] data;
	if ( ! result )
		return false;

    windRustleAngles.resize(speedwind->GetNumLeafAngleMatrices());
    windRockAngles.resize(speedwind->GetNumLeafAngleMatrices());

    speedwind->SetWindStrengthAndDirection(0.05f, 1.0f, 0.0f, 0.0f);

    for(unsigned int i = 0; i < speedwind->GetNumWindMatrices(); i++)
    {
        windMatrices[i] = (float*)speedwind->GetWindMatrix(i);
    }

    return true;
}

/*---------------------------------------------------------------------
Function: InitBillboardUniforms
Description:
---------------------------------------------------------------------*/
static void InitBillboardUniforms(glslProgram* pgm)
{
    // Set once
    const std::vector<float>& texcoords = treeDataGetBillboardTexCoords();
    std::vector<float>::const_iterator itr = texcoords.begin();

    int element = 0;
    glslUniform* uniform = nullptr;

    uniform = pgm->GetUniform("table1");

    for(unsigned int i = 0; i < texcoords.size(); i += 4)
    {
        uniform->SetElement(element, Vec4f(itr[i+0], itr[i+1], itr[i+2], itr[i+3]));
        element++;
    }

    uniform = pgm->GetUniform("scales");

    uniform->SetElement(0, Vec4f(0, 0, 0, 0));
    uniform->SetElement(1, Vec4f(1, 0, 0, 0));
    uniform->SetElement(2, Vec4f(1, 1, 0, 0));
    uniform->SetElement(3, Vec4f(0, 1, 0, 0));

    uniform = pgm->GetUniform("unitSquare");

    uniform->SetElement(0, Vec4f(0,  0.5, 1.0, 0));
    uniform->SetElement(1, Vec4f(0, -0.5, 1.0, 0));
    uniform->SetElement(2, Vec4f(0, -0.5, 0.0, 0));
    uniform->SetElement(3, Vec4f(0,  0.5, 0.0, 0));
}

/*---------------------------------------------------------------------
Function: CreateBillboardUniformsOn
Description:
---------------------------------------------------------------------*/
static void CreateBillboardUniformsOn(int pass)
{
    glslProgram* pgm = GLSL.getProgram("BillboardShaderNormal");

    // Compile to get "table1" location. Cards differ on
    // naming.
    pgm->Apply();

   // Share uniforms where possible.
#define NUMBER_TABLE_ELEMENTS   84

    pgm->GetOrCreateUniform("table1", glslUniform::FLOAT_VEC4, NUMBER_TABLE_ELEMENTS);
    pgm->GetOrCreateUniform("scales", glslUniform::FLOAT_VEC4, 4);
    pgm->GetOrCreateUniform("unitSquare", glslUniform::FLOAT_VEC4, 4);

    // Set once
    const std::vector<float>& texcoords = treeDataGetBillboardTexCoords();
    std::vector<float>::const_iterator itr = texcoords.begin();

    int element = 0;
    glslUniform* uniform = nullptr;

    uniform = pgm->GetUniform("table1");
    uniform->SetOneTimeApply(true);

    for(unsigned int i = 0; i < texcoords.size(); i += 4)
    {
        uniform->SetElement(element, Vec4f(itr[i+0], itr[i+1], itr[i+2], itr[i+3]));
        element++;
    }

    uniform = pgm->GetUniform("scales");
    uniform->SetOneTimeApply(true);

    uniform->SetElement(0, Vec4f(0, 0, 0, 0));
    uniform->SetElement(1, Vec4f(1, 0, 0, 0));
    uniform->SetElement(2, Vec4f(1, 1, 0, 0));
    uniform->SetElement(3, Vec4f(0, 1, 0, 0));

    uniform = pgm->GetUniform("unitSquare");
    uniform->SetOneTimeApply(true);

    uniform->SetElement(0, Vec4f(0,  0.5, 1.0, 0));
    uniform->SetElement(1, Vec4f(0, -0.5, 1.0, 0));
    uniform->SetElement(2, Vec4f(0, -0.5, 0.0, 0));
    uniform->SetElement(3, Vec4f(0,  0.5, 0.0, 0));

    pgm->Apply();
    pgm->ApplyUniformMap();
    ProgramState::Bind(0);
}

/*---------------------------------------------------------------------
Function: initShaders
Description:
---------------------------------------------------------------------*/
static void initShaders()
{
    CreateBillboardUniformsOn(0);
}

/*---------------------------------------------------------------------
Function: initShadersDeprecated
Description:
---------------------------------------------------------------------*/
static void initShadersDeprecated()
{
    InitBillboardUniforms(GLSL.getProgram("BillboardShader"));
    InitBillboardUniforms(GLSL.getProgram("BillboardShaderNormal"));
}

/*---------------------------------------------------------------------
Function: sptmainInit
Description:
---------------------------------------------------------------------*/
bool sptmainInit()
{
    vislimit2 = VisLimit * VisLimit;

    sptExitApp();

    initializeWind("terrain/trees4.2/");
    treeDataInit("terrain/trees4.2/", speedwind);

    CSpeedTreeRT::SetNumWindMatrices(4);

    unsigned int size = treeDataGetBaseSize();

    for(unsigned int base = 0; base < size; base++)
    {
        treeDataGetBaseTree(base)->SelectShader();
    }

    firsttime = true;

    return true;
}

void static InitColliderIfNecessary()
{
    if(!TreeCollider)
    {
        TreeCollider = new colliderQuery(0, false);

        TreeCollider->addGeomToCollideWith((dGeomID)colliderTreeSpace());
        TreeCollider->setMaxContactsPerHit(1);

        dGeomID ray = dCreateRay(TreeCollider->space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        TreeCollider->addGeom(ray, 0, COLLIDER_TRN_TREE);
    }
}

/*---------------------------------------------------------------------
Function: sptIdle
Description:
---------------------------------------------------------------------*/
inline static void sptIdle()
{
    float camera[3];
	float direction[3];

    camera[0] = -playpos.x;
    camera[1] = -playpos.y;
    camera[2] = -playpos.z;

    if(obdrawIsRenderPass(RENDER_REFLECTION))
    {
        const Matrixf& cam = oglGetWaterReflectionCamera();

        direction[0] = -cam(0, 2);
        direction[1] = cam(1, 2);
        direction[2] = -cam(2, 2);
    }
    else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
    {
        if(gCustomTerrain)
        {
            const Matrixf& cam = ShadowMap::GetInstance().GetShadowMapCamera();

            direction[0] = -cam(0, 2);
            direction[1] = cam(1, 2);
            direction[2] = -cam(2, 2);
        }
    }
    else
    {
        direction[0] = -playdir.x;
        direction[1] =  playdir.y;
        direction[2] = -playdir.z;
    }

    double ref_time = (double)frametime * 0.001;

    if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
    {
        fovscale = 1.0f;
        CSpeedTreeRT::SetTime((float)ref_time);
    	CSpeedTreeRT::SetCamera(camera, direction);

        return;
    }

    fovscale = FieldOfView / 90.0f;
    lodscale = 45.0f / FieldOfView;
    lodscale = lodscale < 1.0f ? 1.0f : lodscale;

    CSpeedTreeRT::SetTime((float)ref_time);

    speedwind->Advance((float)ref_time, true, false, -playdir.x, -playdir.y, -playdir.z);

    for(unsigned int i = 0; i < speedwind->GetNumWindMatrices(); i++)
    {
        windMatrices[i] = (float*)speedwind->GetWindMatrix(i);
    }

    speedwind->GetRockAngles(&windRockAngles[0]);
    speedwind->GetRustleAngles(&windRustleAngles[0]);

 	// Camera information must be updated when the camera moves
	CSpeedTreeRT::SetCamera(camera, direction);
}

/*---------------------------------------------------------------------
Function: sptClearDrawList
Description:
---------------------------------------------------------------------*/
static void sptClearDrawList()
{
    logTimerBeginMacro();

    unsigned int size = treeDataGetBaseSize();

    for(unsigned int key = 0; key < size; key++)
    {
        if(gCustomTerrain)
        {
            if(g_wantWaterReflections)
                reflectionTrees[key].clear();

            if(ShadowMapFlags != 0)
                shadowTrees[key].clear();
        }

        threeD[key].clear();
        billboards[key].clear();
        horizbillboards[key].clear();
        billboardindices[key].clear();
        horizbillboardindices[key].clear();
        billboardindiceoffset[key] = 0;
        horizbillboardindiceoffset[key] = 0;
    }

    numbillboards = 0;
    numhorizbillboards = 0;
    number3dtrees = 0;
    horizindex = 0;

    logTimerEndMacro();
}

/*---------------------------------------------------------------------
Function: Cellcull
Description:
---------------------------------------------------------------------*/
inline static bool Cellcull(TERRAIN_DETAIL* t)
{
	if(!t->terrainorig)
		return true;

    piVECTOR pos;

    // Supercell begins at left botton corner. Cull supercell from the
    // center.
    pos.set((float)t->scx * SDM_SUPERCELL_SIZE + SDM_SUPERCELL_SIZE_2,
            (float)t->scy * SDM_SUPERCELL_SIZE + SDM_SUPERCELL_SIZE_2,
            0.0f);

    pos.z = t->terrainorig->pos.z;

    // Translate to object space
    pos.x -= (float)WorldOriginOffset.x;
    pos.y -= (float)WorldOriginOffset.y;

    // a^2 + b^2 = 6400^2 + 6400^2 = c^2 ~= 9051
    return !rnvisVectorFOVCheck(&pos, 9051.0f);
}

/*---------------------------------------------------------------------
Function: Octetcull
Description:
---------------------------------------------------------------------*/
inline static bool Octetcull(TERRAIN_DETAIL* t, int r, int c, unsigned int& mask)
{
    if(t->m_trees == nullptr)
    {
        return true;
    }

	if(!t->terrainorig)
		return true;

    TDTL_TREES& octet = t->m_trees[TRNINDEX(r, c)];

    if(octet.empty())
    {
		return true;
    }

    piVECTOR pos;

    pos.set((float)t->scx * SDM_SUPERCELL_SIZE,
            (float)t->scy * SDM_SUPERCELL_SIZE,
            0.0f);

    // Place pos at center of octet
    pos.x += (SDM_OCTET_SIZE * c + SDM_OCTET_SIZE_2);
    pos.y += (SDM_OCTET_SIZE * r + SDM_OCTET_SIZE_2);

    pos.x -= (float)WorldOriginOffset.x;
    pos.y -= (float)WorldOriginOffset.y;

    piVECTOR dis;
    dis.set(pos.x + playpos.x, pos.y + playpos.y, 0.0f);

    float dist2 = dis.x * dis.x + dis.y * dis.y;

    if(dist2 > vislimit2)
    {
        return true;
    }

#if(0)
    return rnvisSphereCull(mask, pos, 566.0f);
#else
    return !rnvisVectorFOVCheck(&pos, 800.0f);
#endif
}

/*---------------------------------------------------------------------
Function: fovCheck
Description:
---------------------------------------------------------------------*/
inline static bool fovCheck(Tree* tree, const Vec3f& relativepos)
{
    float x, y, z, zz;

    float radius = tree->radius;

    float tx = relativepos.x();
    float ty = relativepos.y();
    float tz = relativepos.z();

    z = tree->distance;

    if(z + radius < 0 || z - radius > BackClip)
    {
        return true;
    }

    y = (tx * playup.x +
         ty * playup.y +
         tz * playup.z +
         playtrans.y) * TopPlaneY;

    zz = z * TopPlaneZ;

    if(zz + y > radius || zz - y > radius)
    {
        return true;
    }

    x = (tx * playright.x +
        ty * playright.y +
        tz * playright.z +
        playtrans.x) * LeftPlaneX;

    zz = z * LeftPlaneZ;

    if(zz + x > radius || zz - x > radius)
    {
        return true;
    }

    return false;
}

/*---------------------------------------------------------------------
Function: AddToDrawList
Description:
---------------------------------------------------------------------*/
static inline void AddToDrawList(Tree* tree, bool reflection = false, bool shadow = false)
{
    float nearlod = 0.0f;
    float farlod = 0.0f;

    int base = GET_BASE(tree->isbush_isclutter_base);
    CTree* basetree = treeDataGetBaseTree(base);

    float scale = 1.0f;

    basetree->GetLodLimits(nearlod, farlod);

    if(lodscale > 1.0f)
    {
        farlod *= lodscale;
        nearlod *= lodscale;
    }

    Vec3f relativepos;
    relativepos = tree->worldpos - WorldOriginOffset;

    basetree->SetTreePosition(relativepos.x(), relativepos.y(), relativepos.z());

    float lod = 1.0f - (tree->distance - nearlod) / (farlod - nearlod);
    lod = lod > 1.0f ? 1.0f : lod < 0.0f ? 0.0f : lod;
    basetree->SetLodLevel(lod);

    CSpeedTreeRT::SLodValues lodvalues;

    memset(&lodvalues, 0, sizeof(CSpeedTreeRT::SLodValues));
    basetree->GetLodValues(lodvalues);

    tree->branchalpha = (unsigned char)lodvalues.m_fBranchAlphaTestValue;
    tree->branchlod = (char)lodvalues.m_nBranchActiveLod;
    tree->frondalpha = (unsigned char)lodvalues.m_fFrondAlphaTestValue;
    tree->frondlod = (char)lodvalues.m_nFrondActiveLod;
    tree->leafalphas[0] = (unsigned char)lodvalues.m_afLeafAlphaTestValues[0];
    tree->leafalphas[1] = (unsigned char)lodvalues.m_afLeafAlphaTestValues[1];
    tree->leaflods[0] = lodvalues.m_anLeafActiveLods[0] > -1;
    tree->leaflods[1] = lodvalues.m_anLeafActiveLods[1] > -1;

    if(tree->distance < farlod)
    {
        if(gCustomTerrain)
        {
            if(reflection)
            {
                if(tree->distance < 200.0f)
                    reflectionTrees[GET_BASE(tree->isbush_isclutter_base)].push_back(tree);

                number3dtrees++;

                return;
            }
            else if(shadow)
            {
                shadowTrees[GET_BASE(tree->isbush_isclutter_base)].push_back(tree);
                number3dtrees++;
                return;
            }
            else
                threeD[GET_BASE(tree->isbush_isclutter_base)].push_back(tree);
        }
        else
            threeD[GET_BASE(tree->isbush_isclutter_base)].push_back(tree);

        number3dtrees++;
    }

    float fadeout = lodvalues.m_fBillboardFadeOut;

    if(tree->distance > nearlod && fadeout > 0.01f)
    {
        float altitude = trnskyGetAltitude() * fovscale;

        if(IS_BUSH(tree->isbush_isclutter_base))
        {
            if(altitude > 1000.0f)
            {
                return;
            }
        }

        Vec3f dv = Vec3f(-playpos.x, -playpos.y, -playpos.z) - relativepos;
        dv.fastnormalize();

        float d = dv.z() < 0.0f ? -dv.z() : dv.z();

		// Don't use "vertexdata[size]" because that may
		// reference an out-of-bound element.
        if(d <= 0.866f)
        {
            if(billboards[base].size() + VERTICAL_INCREMENT_SIZE >= billboards[base].capacity())
            {
			    // Increase memory allocation size by one codepage.
                billboards[base].reserve(billboards[base].capacity() + VALLOC_SIZE);
            }

            if(billboardindices[base].size() + INDICE_SIZE >= billboardindices[base].capacity())
            {
                billboardindices[base].reserve(billboardindices[base].capacity() + IALLOC_SIZE);
            }

            // Vertical Billboard
			// Increase in-use count by the number of vertexes we're about to add.
			const unsigned int bbdata = billboards[base].size();

			billboards[base].resize(bbdata + VERTICAL_INCREMENT_SIZE);
            billboardindices[base].resize(billboardindices[base].size() + INDICE_SIZE);

            float* p = &billboards[base][bbdata];

            Billboard& billboard = basetree->GetBillboard();
            billboard.update(p, relativepos, DEG_TO_RAD((float)tree->rot_angle), fadeout, scale);

            int idx = billboardindices[base].size() - 6;

            billboardindices[base][idx++] = billboardindiceoffset[base] + 0;
            billboardindices[base][idx++] = billboardindiceoffset[base] + 1;
            billboardindices[base][idx++] = billboardindiceoffset[base] + 2;
            billboardindices[base][idx++] = billboardindiceoffset[base] + 0;
            billboardindices[base][idx++] = billboardindiceoffset[base] + 2;
            billboardindices[base][idx++] = billboardindiceoffset[base] + 3;

            billboardindiceoffset[base] += 4;

            numbillboards++;
        }

        if(basetree->HasHorizBillboardGeometry() && d >= 0.5f)
        {
            if(horizbillboards[base].size() + HORIZONTAL_INCREMENT_SIZE >= horizbillboards[base].capacity())
            {
			    // Increase memory allocation size by one codepage.
                horizbillboards[base].reserve(horizbillboards[base].capacity() + HALLOC_SIZE);
            }

            if(horizbillboardindices[base].size() + INDICE_SIZE >= horizbillboardindices[base].capacity())
            {
                horizbillboardindices[base].reserve(horizbillboardindices[base].capacity() + IALLOC_SIZE);
            }

            // Horizontal billboards
			// Increase in-use count by the number of vertexes we're about to add.
			const unsigned int hbbdata = horizbillboards[base].size();
			horizbillboards[base].resize(hbbdata + HORIZONTAL_INCREMENT_SIZE);
            horizbillboardindices[base].resize(horizbillboardindices[base].size() + INDICE_SIZE);

			float* p = &horizbillboards[base][hbbdata];

            HorizontalBillboard& billboard = basetree->GetHorizontalBillboard();
            billboard.update(p, relativepos, tree->rotation, 1.0f - fadeout);

            int idx = horizbillboardindices[base].size() - 6;

            horizbillboardindices[base][idx++] = horizbillboardindiceoffset[base] + 0;
            horizbillboardindices[base][idx++] = horizbillboardindiceoffset[base] + 1;
            horizbillboardindices[base][idx++] = horizbillboardindiceoffset[base] + 2;
            horizbillboardindices[base][idx++] = horizbillboardindiceoffset[base] + 0;
            horizbillboardindices[base][idx++] = horizbillboardindiceoffset[base] + 2;
            horizbillboardindices[base][idx++] = horizbillboardindiceoffset[base] + 3;

            horizbillboardindiceoffset[base] += 4;
            numhorizbillboards++;
        }
    }
}

/*---------------------------------------------------------------------
Function: Treecull
Description:
---------------------------------------------------------------------*/
inline static void Treecull(TERRAIN_DETAIL* dtl, int r, int c, unsigned int mask)
{
    if(!dtl->m_trees)
        return;

    TDTL_TREES& octet = dtl->m_trees[TRNINDEX(r, c)];
    TDTL_TREES::iterator itr = octet.begin();

    unsigned int address = 0;

    Vec3d world;
    world.set(WorldOriginOffset);

    Vec2f dis;
    Vec3f relativepos;

    for(; itr != octet.end(); ++itr)
    {
        if((*itr) == 0)
        {
            continue;
        }

        address = (*itr);

        Tree* t = (Tree*)address;

        if(!t) continue;

        if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
        {
            // Ignore bushes on shadows, since the sun is too high to provide
            // any significant shadow
            if(IS_BUSH(t->isbush_isclutter_base))
            {
                continue;
            }
        }

        relativepos = t->worldpos - world;

        // Addition since playpos is reversed.
        dis.x() = relativepos.x() + playpos.x;
        dis.y() = relativepos.y() + playpos.y;

        if(dis.length2() > vislimit2)
        {
            continue;
        }

        t->distance = relativepos * Vec3f(playdir) + playtrans.z;

        if(gCustomTerrain)
        {
            if(g_wantWaterReflections && glslWaterCanReflect())
            {
                static BoundingSphere bs;

                bs.set(relativepos, t->radius);

                if(reflectionPolytope.contains(bs))
                {
                    AddToDrawList(t, true);
                }
            }

            if(ShadowMap::GetInstance().Enabled())
            {
                static BoundingSphere bs;

                bs.set(relativepos, t->radius);

                if(!ShadowMap::GetInstance().cull(bs))
                {
                    AddToDrawList(t, false, true);
                }
            }
        }

        if(fovCheck(t, relativepos))
        {
            continue;
        }

        AddToDrawList(t);
    }
}

/*---------------------------------------------------------------------
Function: frontToBack
Description:
---------------------------------------------------------------------*/
int frontToBack(Tree* l, Tree* r)
{
    return l->distance < r->distance;
}

/*---------------------------------------------------------------------
Function: sptSortTrees
Description:
---------------------------------------------------------------------*/
inline void sptSortTrees()
{
    unsigned int basesize = treeDataGetBaseSize();

    // Sort for optimal rendering. As per documentation

    for(unsigned int i = 0; i < basesize; i++)
    {
         if(threeD[i].empty())
            continue;

         sort(threeD[i].begin(), threeD[i].end(), frontToBack);
    }
}

/*---------------------------------------------------------------------
Function: CullTrees
Description:
---------------------------------------------------------------------*/
static void CullTrees()
{
    if(nocull)
    {
        return;
    }

    if(gCustomTerrain)
    {
        if(g_wantWaterReflections && glslWaterCanReflect())
        {
            static float zf = 100.0f;
            static float zn = 1.0f;

            reflectionPolytope.setToUnitFrustum();

            Matrixf proj;

            proj(0, 0) = 1.0f / tan(FieldOfView / 2.0f);
            proj(1, 1) = 1.0f / tan(FieldOfView / 2.0f);
            proj(2, 2) = (zf + zn) / (zf - zn);
            proj(2, 3) = 1.0f;
            proj(3, 2) = -2 * zf * zn / (zf - zn);
            proj(3, 3) = 0.0f;

            const Matrixf& mv = oglGetWaterReflectionCamera();
            Matrixf mvp = mv * proj;

            reflectionPolytope.transformProvidingInverse(mvp);
        }
    }

    sptClearDrawList();

    int r, c;

#if DEBUG_CULLING
    std::vector<struct rc> rrcc;
    rowcolumn.clear();
#endif

    // Find in-view supercells
    for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
    {
        if(!activeTerrainDetail[i])
            continue;

        if(trndtl[i].terrain == nullptr)
            continue;

        if(!(trndtl[i].terrain->flags & OBJECT_VISIBLE))
            continue;

        if(Cellcull(&trndtl[i]))
        {
            // Failed cull test
            continue;
        }

        unsigned int mask = CLIP_ALL_PLANES;

        for(r = 0; r < SUPERCELL_OCTETS_Y; r++)
        {
            for(c = 0; c < SUPERCELL_OCTETS_X; c++)
            {
                mask = CLIP_ALL_PLANES;

                if(Octetcull(&trndtl[i], r, c, mask))
                {
                    // Octet failed
                    continue;
                }

#if DEBUG_CULLING
                struct rc RC;
                RC.r = r;
                RC.c = c;
                rrcc.push_back(RC);
#endif
                Treecull(&trndtl[i], r, c, mask);
            }
        }

#if DEBUG_CULLING
        if(!rrcc.empty())
        {
            rowcolumn[i] = rrcc;
        }
#endif
    }
}

/*--------------------------------------------------------------
Function: UpdateBillboardData
Description:
---------------------------------------------------------------*/
inline static void UpdateBillboardData(int pass)
{
    glslProgram* pgm;
    pgm = GLSL.getProgram("BillboardShaderNormal");

    // query camera angles
    float azimuth, pitch;
    CSpeedTreeRT::GetCameraAngles(azimuth, pitch);

    // adjust azimuth from range [-180,180] to [0,360]
    azimuth = DEG_TO_RAD(azimuth);
    azimuth = azimuth < 0.0f ? azimuth + PI2 : azimuth;

    Vec3f camdir, campos;
    CSpeedTreeRT::GetCamera(campos.ptr(), camdir.ptr());
    camdir = -camdir;

    float bbazimuth = atan2f(camdir.y(), camdir.x());
    bbazimuth = bbazimuth < 0.0f ? bbazimuth + PI2 : bbazimuth >= PI2 ? fmod(bbazimuth, PI2) : bbazimuth;

    pgm->GetUniform("rotation")->Set(Vec4f(sinf(-azimuth),
                                           cosf(-azimuth),
                                          -sinf(-azimuth),
                                           0.0f));

    pgm->GetUniform("billboardAzimuth")->Set(bbazimuth);

    CSpeedTreeRT::SGeometry::S360Billboard geo;
    CSpeedTreeRT::UpdateBillboardLighting(geo);

    const float* n = geo.m_pNormals;

    // Adjust for f'd up coordinate system (-n[1])
    pgm->GetUniform("normal")->Set(Vec3f(n[0], -n[1], n[2]));

    const float* b = geo.m_pBinormals;

    // Adjust for f'd up coordinate system (-b[2])
    pgm->GetUniform("binormal")->Set(Vec3f(b[0], b[1],  -b[2]));

    const float* t = geo.m_pTangents;

    pgm->GetUniform("tangent")->Set(Vec3f(t[0], t[1],  t[2]));

    // Set view to be from light source
    static LIGHT* sun = nullptr;
    static LIGHT* moon = nullptr;

    if(!sun)
        sun = obdliteFindDynaLight("sun");

    if(!moon)
        moon = obdliteFindDynaLight("moon");

    Vec3f lightdir(sun->flags & LITE_ON /*sic*/ ? (piVECTOR&)sun->light.dvDirection : (piVECTOR&)moon->light.dvDirection);

    Vec3f lightxy(lightdir.x(), lightdir.y(), 0.0f);
    Vec3f camxy(camdir[0], camdir[1], 0.0f);

    lightxy.normalize();
    camxy.normalize();

    float adjust = (lightxy * camxy + 1.0f) * 0.5f;

    pgm->GetUniform("lightadjust")->Set(adjust);
}

/*--------------------------------------------------------------
Function: UpdateBillboardDataDeprecated
Description:
---------------------------------------------------------------*/
inline static void UpdateBillboardDataDeprecated(int pass)
{
    glslProgram* pgm;

    if(g_wantNormalMaps)
    {
        pgm = GLSL.getProgram("BillboardShaderNormal");
    }
    else
    {
        pgm = GLSL.getProgram("BillboardShader");
    }

    // query camera angles
    float azimuth, pitch;
    CSpeedTreeRT::GetCameraAngles(azimuth, pitch);

    // adjust azimuth from range [-180,180] to [0,360]
    azimuth = DEG_TO_RAD(azimuth);
    azimuth = azimuth < 0.0f ? azimuth + PI2 : azimuth;

    Vec3f camdir, campos;
    CSpeedTreeRT::GetCamera(campos.ptr(), camdir.ptr());
    camdir = -camdir;

    float bbazimuth = atan2f(camdir.y(), camdir.x());
    bbazimuth = bbazimuth < 0.0f ? bbazimuth + PI2 : bbazimuth >= PI2 ? fmod(bbazimuth, PI2) : bbazimuth;

    pgm->GetUniform("rotation")->Set(Vec4f(sinf(-azimuth),
                                           cosf(-azimuth),
                                          -sinf(-azimuth),
                                           0.0f));

    pgm->GetUniform("billboardAzimuth")->Set(bbazimuth);

    CSpeedTreeRT::SGeometry::S360Billboard geo;
    CSpeedTreeRT::UpdateBillboardLighting(geo);

    if(g_wantNormalMaps)
    {
        const float* n = geo.m_pNormals;

        // Adjust for f'd up coordinate system (-n[1])
        pgm->GetUniform("normal")->Set(Vec3f(n[0], -n[1], n[2]));

        const float* b = geo.m_pBinormals;

        // Adjust for f'd up coordinate system (-b[2])
        pgm->GetUniform("binormal")->Set(Vec3f(b[0], b[1],  -b[2]));

        const float* t = geo.m_pTangents;

        pgm->GetUniform("tangent")->Set(Vec3f(t[0], t[1],  t[2]));
    }

    // Set view to be from light source
    static LIGHT* sun = nullptr;
    static LIGHT* moon = nullptr;

    if(!sun)
        sun = obdliteFindDynaLight("sun");

    if(!moon)
        moon = obdliteFindDynaLight("moon");

    Vec3f lightdir(sun->flags & LITE_ON /*sic*/ ? (piVECTOR&)sun->light.dvDirection : (piVECTOR&)moon->light.dvDirection);

    Vec3f lightxy(lightdir.x(), lightdir.y(), 0.0f);
    Vec3f camxy(camdir[0], camdir[1], 0.0f);

    lightxy.normalize();
    camxy.normalize();

    float adjust = (lightxy * camxy + 1.0f) * 0.5f;

    pgm->GetUniform("lightadjust")->Set(adjust);
}

/*---------------------------------------------------------------------
Function: sptDrawBillboards
Description:
---------------------------------------------------------------------*/
inline static void sptDrawBillboards(int pass)
{
    logTimerBeginMacro();

    // Future speedtree may have billboards per tree, not composite
    if(numbillboards > 0)
    {
        static int stride = 56;

        for(unsigned int i = 0; i < treeDataGetBaseSize(); i++)
        {
            if(billboards[i].empty())
            {
                continue;
            }

            CTree* tree = treeDataGetBaseTree(i);
            // Billboards may be on separate textures in the future.
            tree->StartBillboardType();

            unsigned int size = 4 * billboards[i].size() / stride;

            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 4, GL_FLOAT, 0, stride, &billboards[i][0]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 4, GL_FLOAT, 0, stride, &billboards[i][4]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 3, GL_FLOAT, 0, stride, &billboards[i][8]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 3, GL_FLOAT, 0, stride, &billboards[i][11]);

            ogl::DrawArrays(GL_QUADS, 0, size);

            tree->EndBillboardType();
        }

        totaltriangles += numbillboards * 2;
    }

    logTimerEndMacro();
}

/*---------------------------------------------------------------------
Function: sptDrawBillboardsDeprecated
Description:
---------------------------------------------------------------------*/
inline static void sptDrawBillboardsDeprecated(int pass)
{
    logTimerBeginMacro();

    // Future speedtree may have billboards per tree, not composite
    if(numbillboards > 0)
    {
        static int stride = 56;

        for(unsigned int i = 0; i < treeDataGetBaseSize(); i++)
        {
            if(billboards[i].empty())
            {
                continue;
            }

            CTree* tree = treeDataGetBaseTree(i);
            // Billboards may be on separate textures in the future.
            tree->StartBillboardType();

            unsigned int size = 4 * billboards[i].size() / stride;

            ogl::SetVertexPointer(4, GL_FLOAT, stride, &billboards[i][0]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 4, GL_FLOAT, stride, &billboards[i][4]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 3, GL_FLOAT, stride, &billboards[i][8]);
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE2, 3, GL_FLOAT, stride, &billboards[i][11]);

            ogl::DrawElements(GL_TRIANGLES, billboardindices[i].size(), GL_UNSIGNED_SHORT, &billboardindices[i][0]);

            tree->EndBillboardType();
        }

        totaltriangles += numbillboards * 2;
    }

    logTimerEndMacro();
}

/*---------------------------------------------------------------------
Function: sptDrawHorizBillboards
Description:
---------------------------------------------------------------------*/
inline static void sptDrawHorizBillboards()
{
    logTimerBeginMacro();

    if(numhorizbillboards > 0)
    {
        static int stride = 60;

        for(unsigned int i = 0; i < treeDataGetBaseSize(); i++)
        {
            if(horizbillboards[i].empty())
            {
                continue;
            }

            CTree* tree = treeDataGetBaseTree(i);
            tree->StartHorizBillboardType();

            unsigned int size = 4 * horizbillboards[i].size() / stride;

            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 4, GL_FLOAT, 0, stride, &horizbillboards[i][0]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, stride, &horizbillboards[i][4]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 3, GL_FLOAT, 0, stride, &horizbillboards[i][6]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 3, GL_FLOAT, 0, stride, &horizbillboards[i][9]);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD3, 3, GL_FLOAT, 0, stride, &horizbillboards[i][12]);

            ogl::DrawArrays(GL_QUADS, 0, size);

            tree->EndHorizBillboardType();
        }

        totaltriangles += numhorizbillboards * 2;
   }

    logTimerEndMacro();
}

/*---------------------------------------------------------------------
Function: sptDrawHorizBillboardsDeprecated
Description:
---------------------------------------------------------------------*/
inline static void sptDrawHorizBillboardsDeprecated()
{
    logTimerBeginMacro();

    if(numhorizbillboards > 0)
    {
        static int stride = 60;

        for(unsigned int i = 0; i < treeDataGetBaseSize(); i++)
        {
            if(horizbillboards[i].empty())
            {
                continue;
            }

            CTree* tree = treeDataGetBaseTree(i);
            tree->StartHorizBillboardType();

            unsigned int size = 4 * horizbillboards[i].size() / stride;

            // Position and fade
            ogl::SetVertexPointer(4, GL_FLOAT, stride, &horizbillboards[i][0]);

            // Tex coords
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, &horizbillboards[i][4]);

            // LightAdjusts
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 3, GL_FLOAT, stride, &horizbillboards[i][6]);

            // Rotations
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE2, 3, GL_FLOAT, stride, &horizbillboards[i][9]);

            // World positions
            ogl::SetTextureUnitCoordPointer(GL_TEXTURE3, 3, GL_FLOAT, stride, &horizbillboards[i][12]);

            ogl::DrawElements(GL_TRIANGLES, horizbillboardindices[i].size(), GL_UNSIGNED_SHORT, &horizbillboardindices[i][0]);

            tree->EndHorizBillboardType();
        }

        totaltriangles += numhorizbillboards * 2;
   }

    logTimerEndMacro();
}

/*---------------------------------------------------------------------
Function: sptDrawBranches
Description:
---------------------------------------------------------------------*/
inline static void sptDrawBranches()
{
    static unsigned int size = treeDataGetBaseSize();

    static CTree* basetree;
    Vec3f relativepos;

    int primcount = 0;

    std::vector<Tree*>* trees;

    for(unsigned int base = 0; base < size; base++)
    {
        if(gCustomTerrain)
        {
            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                trees = &reflectionTrees[base];
            }
            else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                trees = &shadowTrees[base];
            }
            else
            {
                trees = &threeD[base];
            }
        }
        else
        {
            trees = &threeD[base];
        }

        if(trees->empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(!basetree->HasBranchGeometry())
        {
            continue;
        }

        basetree->StartBranchType();
        basetree->SetBranchDiffuseScale();

        primcount = 0;

        for(unsigned int j = 0; j < trees->size(); j++)
        {
            Tree* t = (*trees)[j];

            if(t->branchlod < 0)
            {
                continue;
            }

            relativepos = t->worldpos - WorldOriginOffset;

            basetree->AddBranchDraw(t,
                                    Vec4f(relativepos, t->branchalpha * colorONEOVER255),
                                    t->rotation);

            primcount++;

            if(primcount >= MAX_TREE_PRIMITIVES)
            {
                totaltriangles += basetree->PushBranches();
                primcount = 0;
            }
        }

        if(primcount > 0)
        {
            totaltriangles += basetree->PushBranches();
            primcount = 0;
        }
    }
}

/*---------------------------------------------------------------------
Function: sptDrawFronds
Description:
---------------------------------------------------------------------*/
inline static void sptDrawFronds()
{
    static unsigned int size = treeDataGetBaseSize();

    static CTree* basetree;
    Vec3f relativepos;

    int primcount = 0;

    std::vector<Tree*>* trees;

    for(unsigned int base = 0; base < size; base++)
    {
        if(gCustomTerrain)
        {
            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                trees = &reflectionTrees[base];
            }
            else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                trees = &shadowTrees[base];
            }
            else
            {
                trees = &threeD[base];
            }
        }
        else
        {
            trees = &threeD[base];
        }

        if(trees->empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(!basetree->HasFrondGeometry())
        {
            continue;
        }

        // These are bushes and don't need fronds
        if(basetree->GetNumCollisionObjects() == 0)
        {
            continue;
        }

        basetree->StartFrondType();
        basetree->SetFrondDiffuseScale();

        for(unsigned int j = 0; j < trees->size(); j++)
        {
            Tree* t = (*trees)[j];

            if(t->frondlod < 0)
            {
                continue;
            }

            // Probably can't see lod2 fronds.
            if(t->frondlod >= 1)
            {
                continue;
            }

            relativepos = t->worldpos - WorldOriginOffset;

            basetree->AddFrondDraw(t,
                                   Vec4f(relativepos, t->frondalpha * colorONEOVER255),
                                   t->rotation);

            primcount++;

            if(primcount >= MAX_TREE_PRIMITIVES)
            {
                totaltriangles += basetree->PushFronds();
                primcount = 0;
            }
        }

        if(primcount > 0)
        {
            totaltriangles += basetree->PushFronds();
            primcount = 0;
        }
    }
}

/*---------------------------------------------------------------------
Function: sptDrawLeaves
Description:
---------------------------------------------------------------------*/
inline static void sptDrawLeaves()
{
    float azimuth, pitch;
    CSpeedTreeRT::GetCameraAngles(azimuth, pitch);

    pitch = DEG_TO_RAD(pitch);
    azimuth = DEG_TO_RAD(azimuth);

    if(azimuth < 0.0)
    {
        azimuth += PI * 2.0f;
    }

    static CTree* basetree;

    int primcount = 0;
    float alpha;
    Vec3f relativepos;

    unsigned int size = treeDataGetBaseSize();

    std::vector<Tree*>* trees;

    for(unsigned int base = 0; base < size; base++)
    {
        if(gCustomTerrain)
        {
            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                trees = &reflectionTrees[base];
            }
            else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                trees = &shadowTrees[base];
            }
            else
            {
                trees = &threeD[base];
            }
        }
        else
        {
            trees = &threeD[base];
        }

        if(trees->empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(!basetree->HasLeafGeometry())
        {
            continue;
        }

        basetree->StartLeafType();

        basetree->SetLeafCameraAngle(azimuth, pitch);
        basetree->SetLeafDiffuseScale();

        for(unsigned int j = 0; j < trees->size(); j++)
        {
            Tree* t = (*trees)[j];

            if(!t->leaflods[0] && !t->leaflods[1])
            {
                continue;
            }

            relativepos = t->worldpos - WorldOriginOffset;

            if(t->leaflods[0])
            {
                if(IS_BUSH(t->isbush_isclutter_base))
                    alpha = 84.0f * colorONEOVER255;
                else
                    alpha = t->leafalphas[0] * colorONEOVER255;

                basetree->AddLeafDraw(t,
                                      Vec4f(relativepos, alpha),
                                      t->rotation,
                                      0);

                primcount++;

                if(primcount >= MAX_TREE_PRIMITIVES)
                {
                    totaltriangles += basetree->PushLeaves();
                    primcount = 0;
                }
            }

            if(t->leaflods[1])
            {
                if(IS_BUSH(t->isbush_isclutter_base))
                    alpha = 84.0f * colorONEOVER255;
                else
                    alpha = t->leafalphas[1] * colorONEOVER255;

                basetree->AddLeafDraw(t,
                                      Vec4f(relativepos, alpha),
                                      t->rotation,
                                      1);

                primcount++;

                if(primcount >= MAX_TREE_PRIMITIVES)
                {
                    totaltriangles += basetree->PushLeaves();
                    primcount = 0;
                }
            }
        }

        if(primcount > 0)
        {
            totaltriangles += basetree->PushLeaves();
            primcount = 0;
        }
    }
}

/*---------------------------------------------------------------------
Function: sptDrawLeavesDeprecated
Description:
---------------------------------------------------------------------*/
inline static void sptDrawLeavesDeprecated()
{
    float azimuth, pitch;
    CSpeedTreeRT::GetCameraAngles(azimuth, pitch);

    pitch = DEG_TO_RAD(pitch);
    azimuth = DEG_TO_RAD(azimuth);

    Vec3f relativepos;

    if(azimuth < 0.0)
    {
        azimuth += PI * 2.0f;
    }

    static CTree* basetree;

    unsigned int size = treeDataGetBaseSize();

    std::vector<Tree*>* trees;

    for(unsigned int base = 0; base < size; base++)
    {
        if(gCustomTerrain)
        {
            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                trees = &reflectionTrees[base];
            }
            else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                trees = &shadowTrees[base];
            }
            else
            {
                trees = &threeD[base];
            }
        }
        else
        {
            trees = &threeD[base];
        }

        if(trees->empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(!basetree->HasLeafGeometry())
        {
            continue;
        }

        basetree->StartLeafType();
        basetree->SetLeafCameraAngle(azimuth, pitch);
        basetree->SetLeafDiffuseScale();

        for(unsigned int j = 0; j < trees->size(); j++)
        {
            Tree* t = (*trees)[j];

            if(!t->leaflods[0] && !t->leaflods[1])
            {
                continue;
            }

            relativepos = t->worldpos - WorldOriginOffset;

            basetree->SetLeafParamBlock(relativepos, t->rotation);

            if(t->leaflods[0])
            {
#if USE_STATESET
                basetree->SetAlphaRef(IS_BUSH(t->isbush_isclutter_base) ? 84.0f : t->leafalphas[0], CTree::TREE_LEAVES);
#else
                oglSetRenderState(oglRENDERSTATE_ALPHAREF, IS_BUSH(t->isbush_isclutter_base) ? 84 : FloatToInt(t->leafalphas[0]));
#endif
                totaltriangles += basetree->DrawLeavesDeprecated(t, 0);
            }

            if(t->leaflods[1])
            {
#if USE_STATESET
                basetree->SetAlphaRef(IS_BUSH(t->isbush_isclutter_base) ? 84.0f : t->leafalphas[1], CTree::TREE_LEAVES);
#else
                oglSetRenderState(oglRENDERSTATE_ALPHAREF, IS_BUSH(t->isbush_isclutter_base) ? 84 : FloatToInt(t->leafalphas[1]));
#endif
                totaltriangles += basetree->DrawLeavesDeprecated(t, 1);
            }
        }

        basetree->EndLeafType();
    }
}

/*---------------------------------------------------------------------
Function: sptDrawFrondsDeprecated
Description:
---------------------------------------------------------------------*/
inline static void sptDrawFrondsDeprecated()
{
   unsigned int size = treeDataGetBaseSize();

    static CTree* basetree;
    Vec3f relativepos;

    std::vector<Tree*>* trees;

    for(unsigned int base = 0; base < size; base++)
    {
        if(gCustomTerrain)
        {
            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                trees = &reflectionTrees[base];
            }
            else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                trees = &shadowTrees[base];
            }
            else
            {
                trees = &threeD[base];
            }
        }
        else
        {
            trees = &threeD[base];
        }

        if(trees->empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(!basetree->HasFrondGeometry())
        {
            continue;
        }

        // These are bushes and don't need fronds
        if(basetree->GetNumCollisionObjects() == 0)
        {
            continue;
        }

        basetree->StartFrondType();
        basetree->SetFrondDiffuseScale();

        for(unsigned int j = 0; j < trees->size(); j++)
        {
            Tree* t = (*trees)[j];

            if(t->frondlod < 0)
            {
                continue;
            }

            // Probably can't see lod2 fronds.
            if(t->frondlod >= 1)
            {
                continue;
            }

            relativepos = t->worldpos - WorldOriginOffset;
            basetree->SetFrondParamBlock(relativepos, t->rotation);

#if USE_STATESET
            basetree->SetAlphaRef(t->frondalpha, CTree::TREE_FRONDS);
#else
            oglSetRenderState(oglRENDERSTATE_ALPHAREF, FloatToInt(t->frondalpha));
#endif

            totaltriangles += basetree->DrawFrondsDeprecated(t);
        }

        basetree->EndFrondType();
    }
}

/*---------------------------------------------------------------------
Function: sptDrawBranchesDeprecated
Description:
---------------------------------------------------------------------*/
inline static void sptDrawBranchesDeprecated()
{
    unsigned int size = treeDataGetBaseSize();

    static CTree* basetree;
    Vec3f relativepos;

    std::vector<Tree*>* trees;

    for(unsigned int base = 0; base < size; base++)
    {
        if(gCustomTerrain)
        {
            if(obdrawIsRenderPass(RENDER_REFLECTION))
            {
                trees = &reflectionTrees[base];
            }
            else if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                trees = &shadowTrees[base];
            }
            else
            {
                trees = &threeD[base];
            }
        }
        else
        {
            trees = &threeD[base];
        }

        if(trees->empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(!basetree->HasBranchGeometry())
        {
            continue;
        }

        basetree->StartBranchType();
        basetree->SetBranchDiffuseScale();

        for(unsigned int j = 0; j < trees->size(); j++)
        {
            Tree* t = (*trees)[j];

            if(t->branchlod < 0)
            {
                continue;
            }

            relativepos = t->worldpos - WorldOriginOffset;


#if USE_STATESET
            basetree->SetBranchParamBlock(relativepos, t->rotation);
            basetree->SetAlphaRef(t->branchalpha, CTree::TREE_BRANCHES);
#else
            oglSetRenderState(oglRENDERSTATE_ALPHAREF, FloatToInt(t->branchalpha));
            basetree->SetBranchParamBlock(relativepos, t->rotation);
#endif

            totaltriangles += basetree->DrawBranchesDeprecated(t);
        }

        basetree->EndBranchType();
    }
}

/*---------------------------------------------------------------------
Function: NeedCulling
Description:
---------------------------------------------------------------------*/
inline bool NeedCulling()
{
    static float oldFOV = 0;
    static int oldFogScale = 0;
    static Vec3f oldPlayPos;
    static Vec3f oldDir;
    static unsigned int oldcrewpos = 99999;

    if(cullnecessary)
    {
        cullnecessary = false;
        return true;
    }

    // Check if we need to cull trees by FOV change
    if(oldFOV != FieldOfView)
    {
        oldFOV = FieldOfView;
        return true;
    }

    // Fog distance has changed
    if(oldFogScale != FogScaleSelector)
    {
        oldFogScale = FogScaleSelector;
        return true;
    }

    // Player has moved in distance
    Vec3f player(-playpos.x, -playpos.y, -playpos.z);

    if((oldPlayPos - player).length2() > 1.0f)
    {
        oldPlayPos = player;
        return true;
    }

    // Player has turned
    Vec3f dir(playdir.x, playdir.y, playdir.z);
    float len = (dir - oldDir).length2();

    if(len > 0.0005f * fovscale)
    {
        oldDir = dir;
        return true;
    }

    unsigned int crewpos = persGetPersonaPosition();

    if(oldcrewpos != crewpos)
    {
        oldcrewpos = crewpos;
        return true;
    }

    return false;
}

#ifdef BETA_RELEASE
#include "draw/draw3d.h"
/*---------------------------------------------------------------------
Function: drawTreeBoundingBoxes
Description:
---------------------------------------------------------------------*/
void drawTreeBoundingBoxes()
{
    unsigned int size = treeDataGetBaseSize();

    static CTree* basetree;

    float box[6];
    Vec3f relativepos;

    for(unsigned int base = 0; base < size; base++)
    {
        // No trees for this base
        if(threeD[base].empty())
        {
            continue;
        }

        // Use this for drawing different bounding boxes if need be.
        treeDataGetBaseTree(base)->GetBoundingBox(box);

        for(unsigned int i = 0; i < threeD[base].size(); i++)
        {
            Tree* t = threeD[base][i];
            float radius = t->radius;

            // Use these for drawing different bounding boxes (not with radius) if need be.
            float x1 = box[0];
            float y1 = box[1];
            float z1 = box[2];

            float x2 = box[3];
            float y2 = box[4];
            float z2 = box[5];

            relativepos = t->worldpos - WorldOriginOffset;

            // Face 1
            piVECTOR a1 = {relativepos.x() - radius, relativepos.y() - radius, relativepos.z()};
            piVECTOR a2 = {relativepos.x() - radius, relativepos.y() + radius, relativepos.z()};
            piVECTOR a3 = {relativepos.x() + radius, relativepos.y() + radius, relativepos.z()};
            piVECTOR a4 = {relativepos.x() + radius, relativepos.y() - radius, relativepos.z()};

            // Face 2
            piVECTOR b1 = {relativepos.x() - radius, relativepos.y() - radius, relativepos.z() - radius};
            piVECTOR b2 = {relativepos.x() - radius, relativepos.y() + radius, relativepos.z() - radius};
            piVECTOR b3 = {relativepos.x() + radius, relativepos.y() + radius, relativepos.z() - radius};
            piVECTOR b4 = {relativepos.x() + radius, relativepos.y() - radius, relativepos.z() - radius};

            // Box is already positioned in the world.
            piSetWorldMatrix(&IdentityMatrix);

            draw3dLine(&a1, &a2, colorWHITEALPHA);
            draw3dLine(&a2, &a3, colorWHITEALPHA);
            draw3dLine(&a3, &a4, colorWHITEALPHA);
            draw3dLine(&a4, &a1, colorWHITEALPHA);

            draw3dLine(&b1, &b2, colorWHITEALPHA);
            draw3dLine(&b2, &b3, colorWHITEALPHA);
            draw3dLine(&b3, &b4, colorWHITEALPHA);
            draw3dLine(&b4, &b1, colorWHITEALPHA);

            draw3dLine(&a1, &b1, colorBLUEALPHA);
            draw3dLine(&a2, &b2, colorBLUEALPHA);
            draw3dLine(&a3, &b3, colorBLUEALPHA);
            draw3dLine(&a4, &b4, colorBLUEALPHA);
        }
    }
}
#endif

#ifdef BETA_RELEASE
/*---------------------------------------------------------------------
Function: drawCollisionCylinders
Description: Draw collision cylinder objects for debugging
---------------------------------------------------------------------*/
void drawCollisionCylinders()
{
    CTree* basetree = nullptr;
    unsigned int size = treeDataGetBaseSize();
    GLUquadric* obj = gluNewQuadric();

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    oglDisableTexturesAboveAndIncluding(0);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 1, 0.5));

    for(unsigned int base = 0; base < size; base++)
    {
        // No trees for this base
        if(threeD[base].empty())
            continue;

        basetree = treeDataGetBaseTree(base);

        if(basetree->GetNumCollisionObjects() == 0)
            continue;

        Vec3f pos;
        float p[3];
        float dimensions[3];
        float euler[3];
        CSpeedTreeRT::ECollisionObjectType type;

        Vec3f relativepos;

        for(unsigned int j = 0; j < threeD[base].size(); j++)
        {
            Tree* t = threeD[base][j];

            Matrixf rot;
            rot.makeRotate(DEG_TO_RAD(t->rot_angle), Vec3f(0, 0, -1));

            for(int i = 0; i < treeDataGetBaseTree(base)->GetNumCollisionObjects(); i++)
            {
                basetree->GetCollisionObject(i, type, p, dimensions, euler);

                if(type != CSpeedTreeRT::CO_CAPSULE)
                    continue;

                if(t->collidemat.empty())
                    continue;

                // ww2 coordinate system bs.
                p[0] = -p[0];
                p[2] = -p[2];

                piMATRIX m = IdentityMatrix;
                piVECTOR v = {DEG_TO_RAD(-euler[0]), DEG_TO_RAD(-euler[1]), DEG_TO_RAD(-euler[2])};

                mathEul_ToMatrix(&m, EulOrdXYZs, &v);

                Vec3f pp = Vec3f(p[0], p[1], p[2]) * rot;

                Matrixf r;
                // Flip to our coordinate system
                r.makeRotate(PI, Vec3f(0, 1, 0));

                (Matrixf(m) * r).get(m);
                (Matrixf(m) * rot).get(m);

                float radius = dimensions[0];
                float height = dimensions[1];

                relativepos = t->worldpos - WorldOriginOffset + pp;

                m._41 = relativepos.x();
                m._42 = relativepos.y();
                m._43 = relativepos.z();

                oglSetWorldMatrix(&m);

                gluCylinder(obj, radius, radius, height, 16, 16);
            }
        }
    }

    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    gluDeleteQuadric(obj);
    oglSetWorldMatrix(&IdentityMatrix);
}

/*---------------------------------------------------------------------
Function: drawBoundBox
Description:
---------------------------------------------------------------------*/
void drawBoundBox()
{
    CTree* basetree = nullptr;
    unsigned int size = treeDataGetBaseSize();

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    piDisableTexture();
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 1, 1));

    for(unsigned int base = 0; base < size; base++)
    {
        // No trees for this base
        if(threeD[base].empty())
        {
            continue;
        }

        for(unsigned int j = 0; j < threeD[base].size(); j++)
        {
            Tree* t = threeD[base][j];

            if(!t->o)
                continue;

            obdrawObjectBoundingBox(t->o, colorBLUEALPHA);

            piVECTOR p;
            math3dVectorMultiply(&p, &t->o->pos, &t->o->mat);
            piVECTOR p2 = p;
            p2.z -= 50.0f;

            draw3dLine(&p, &p2, colorYELLOWALPHA);
        }
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#endif

/*---------------------------------------------------------------------
Function: sptUpdate
Description:
---------------------------------------------------------------------*/
void sptUpdate()
{
    logTimerBeginMacro();

    if(firsttime)
    {
        firsttime = false;

        InitColliderIfNecessary();

        if(ogl::UseDeprecatedPath())
            initShadersDeprecated();
        else
            initShaders();
    }

    logTimerEndMacro();
}

#if DEBUG_CULLING
/*---------------------------------------------------------------------
Function: drawCulledSuperCellsAndOctets
Description:
---------------------------------------------------------------------*/
static void drawCulledSuperCellsAndOctets()
{
    std::map<int, std::vector<struct rc> >::iterator itr = rowcolumn.begin();

    static float z = -255.0f; // Adjust base on terrain location

    for(; itr != rowcolumn.end(); ++itr)
    {
        std::vector<struct rc> v = itr->second;

        for(unsigned int i = 0; i < v.size(); i++)
        {
            struct rc RC = v[i];

            float sx = trndtl[itr->first].scx * 12800.0f;
            float sy = trndtl[itr->first].scy * 12800.0f;

            sx += RC.c * 800.0f;
            sy += RC.r * 800.0f;

            sx += 400.0f;
            sy += 400.0f;

            sx -= WorldOriginOffset.x;
            sy -= WorldOriginOffset.y;

            piVECTOR p1, p2;
            p1.z = p2.z = z;

            p1.x = sx - 400.0f;
            p1.y = sy + 400.0f;
            p2 = p1;
            p2.x += 800.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1 = p2;
            p2.y -= 800.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1 = p2;
            p2.x -= 800.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1 = p2;
            p2.y += 800.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);
        }
    }

    itr = rowcolumn.begin();

    for(; itr != rowcolumn.end(); ++itr)
    {
        float sx = trndtl[itr->first].scx * 12800.0f;
        float sy = trndtl[itr->first].scy * 12800.0f;

        sx += 6400.0f;
        sy += 6400.0f;

        sx -= WorldOriginOffset.x;
        sy -= WorldOriginOffset.y;

        piVECTOR p1, p2;
        p1.z = p2.z = z - 2.0f;

        p1.x = sx - 6400.0f;
        p1.y = sy + 6400.0f;
        p2 = p1;
        p2.x += 12800;

        draw3dLine(&p1, &p2, colorWHITEALPHA);

        p1 = p2;
        p2.y -= 12800;

        draw3dLine(&p1, &p2, colorWHITEALPHA);

        p1 = p2;
        p2.x -= 12800;

        draw3dLine(&p1, &p2, colorWHITEALPHA);

        p1 = p2;
        p2.y += 12800;

        draw3dLine(&p1, &p2, colorWHITEALPHA);
    }
}
#endif

#if USE_STATESET
/*---------------------------------------------------------------------
Function: sptDisplayDeprecated
Description:
---------------------------------------------------------------------*/
void sptDisplayDeprecated()
{
    oglSetWorldMatrix(&IdentityMatrix);

#ifdef BETA_RELEASE
    if(!drawtrees)
    {
        return;
    }
#endif

    totaltriangles = 0;
    sptIdle();

    if(!gCustomTerrain)
    {
        if(NeedCulling())
        {
            CullTrees();
        }
    }

    if(number3dtrees == 0 && numbillboards == 0 && numhorizbillboards == 0)
    {
        return;
    }

    const size_t size = treeDataGetBaseSize();

    for(size_t base = 0; base < size; base++)
    {
        treeDataGetBaseTree(base)->SelectShader();
    }

    if(drawbranches)
    {
        logTimerCallVoid(sptDrawBranchesDeprecated);
    }

    if(drawfronds && obdrawIsRenderPass(RENDER_SCENE | RENDER_SCENE_DEPTH))
    {
        logTimerCallVoid(sptDrawFrondsDeprecated);
    }

    if(drawleaves)
    {
        logTimerCallVoid(sptDrawLeavesDeprecated);
    }

    if(!obdrawIsRenderPass(RENDER_REFLECTION | RENDER_SHADOW_MAP))
    {
        if(numbillboards > 0 || numhorizbillboards > 0)
        {
            if(numbillboards > 0 && drawbillboards)
            {
                UpdateBillboardDataDeprecated(0);
                sptDrawBillboardsDeprecated(0);
            }

            if(numhorizbillboards > 0 && drawhorizbillboards)
            {
                logTimerCallVoid(sptDrawHorizBillboardsDeprecated);
            }
        }
    }

#if DEBUG_SPEEDTREE
#if DEBUG_CULLING
    if(drawsupercellsandoctets)
    {
        drawCulledSuperCellsAndOctets();
    }
#endif // DEBUG_CULLING

    if(drawcylinders)
    {
        drawCollisionCylinders();
    }

    if(drawboundbox)
    {
        drawBoundBox();
    }

    piVECTOR p1, p2;

    Vec3d pp = _odepos - Vec3d(WorldOriginOffset);
    pp.get(p1);
    pp = pp + _oderay;
    pp.get(p2);

    draw3dLine(&p1, &p2, colorWHITEALPHA);
#endif
}
#else
/*---------------------------------------------------------------------
Function: sptDisplayDeprecated
Description:
---------------------------------------------------------------------*/
void sptDisplayDeprecated()
{
    oglSetWorldMatrix(&IdentityMatrix);

#ifdef BETA_RELEASE
    if(!drawtrees)
    {
        return;
    }
#endif

    totaltriangles = 0;
    sptIdle();

    if(!gCustomTerrain)
    {
        if(NeedCulling())
        {
            CullTrees();
        }
    }

    if(number3dtrees == 0 && numbillboards == 0 && numhorizbillboards == 0)
    {
        return;
    }

    unsigned int size = treeDataGetBaseSize();

    for(unsigned int base = 0; base < size; base++)
    {
        treeDataGetBaseTree(base)->SelectShader();
    }

    oglDisableTexturesAboveAndIncluding(0);
    oglEnableTexture();

    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT); // Reverse Z crap
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, TRUE);
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);
    oglSetRenderStageState(0, oglTSS_ADDRESSU, piTADDRESS_WRAP);
	oglSetRenderStageState(0, oglTSS_ADDRESSV, piTADDRESS_WRAP);

#if(0)
    UploadUtilityData();
    UploadLeafAngles();
#endif

    if(drawbranches)
    {
        logTimerCallVoid(sptDrawBranchesDeprecated);
    }

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    if(drawfronds && obdrawIsRenderPass(RENDER_SCENE))
    {
        logTimerCallVoid(sptDrawFrondsDeprecated);
    }

    if(drawleaves)
    {
        logTimerCallVoid(sptDrawLeavesDeprecated);
    }

    ogl::Disable();

    if(!obdrawIsRenderPass(RENDER_REFLECTION | RENDER_SHADOW_MAP))
    {
        if(numbillboards > 0 || numhorizbillboards > 0)
        {
            oglSetRenderState(oglRENDERSTATE_ALPHAREF, 84);

            if(numbillboards > 0 && drawbillboards)
            {
                UpdateBillboardDataDeprecated(0);
                sptDrawBillboardsDeprecated(0);
            }

            if(numhorizbillboards > 0 && drawhorizbillboards)
            {
                logTimerCallVoid(sptDrawHorizBillboardsDeprecated);
            }
        }
    }

    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
	oglSetRenderState(oglRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);

#if DEBUG_SPEEDTREE
#if DEBUG_CULLING
    if(drawsupercellsandoctets)
    {
        drawCulledSuperCellsAndOctets();
    }
#endif // DEBUG_CULLING

    if(drawcylinders)
    {
        drawCollisionCylinders();
    }

    if(drawboundbox)
    {
        drawBoundBox();
    }

    piVECTOR p1, p2;

    Vec3d pp = _odepos - Vec3d(WorldOriginOffset);
    pp.get(p1);
    pp = pp + _oderay;
    pp.get(p2);

    draw3dLine(&p1, &p2, colorWHITEALPHA);
#endif
}
#endif

/*---------------------------------------------------------------------
Function: sptCull
Description:
---------------------------------------------------------------------*/
void sptCull()
{
    if(NeedCulling())
    {
        CullTrees();
    }
}

/*---------------------------------------------------------------------
Function: sptDraw
Description:
---------------------------------------------------------------------*/
void sptDraw()
{
    oglSetWorldMatrix(&IdentityMatrix);

#ifdef BETA_RELEASE
    if(!drawtrees)
    {
        return;
    }
#endif

    totaltriangles = 0;
    sptIdle();

    if(number3dtrees == 0 && numbillboards == 0 && numhorizbillboards == 0)
    {
        return;
    }

    oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT); // Reverse Z crap
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);
    oglSetRenderStageState(0, oglTSS_ADDRESSU, piTADDRESS_WRAP);
	oglSetRenderStageState(0, oglTSS_ADDRESSV, piTADDRESS_WRAP);

    if(drawbranches)
    {
        logTimerCallVoid(sptDrawBranches);
    }

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    if(drawfronds)
    {
        logTimerCallVoid(sptDrawFronds);
    }

    if(drawleaves)
    {
        logTimerCallVoid(sptDrawLeaves);
    }

    if(!obdrawIsRenderPass(RENDER_REFLECTION))
    {
        if((numbillboards > 0 || numhorizbillboards > 0))
        {
        if(numbillboards > 0 && drawbillboards)
        {
            UpdateBillboardData(0);
            sptDrawBillboards(0);
        }

        if(numhorizbillboards > 0 && drawhorizbillboards)
        {
            logTimerCallVoid(sptDrawHorizBillboards);
        }
    }
    }

    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

#if DEBUG_SPEEDTREE
#if DEBUG_CULLING
    if(drawsupercellsandoctets)
    {
        drawCulledSuperCellsAndOctets();
    }
#endif // DEBUG_CULLING

    if(drawcylinders)
    {
        drawCollisionCylinders();
    }

    if(drawboundbox)
    {
        drawBoundBox();
    }
#endif
}

/*---------------------------------------------------------------------
Function: sptGetStats
Description:
---------------------------------------------------------------------*/
void sptGetStats(char stats[])
{
    unsigned int size = treeDataGetBaseSize();

    sprintf(stats,
            "TREES: Total %d Drawn %d 3D = %d 2D = %d Triangles %d",
            treeDataGetTotalTrees(),
            number3dtrees + numbillboards + numhorizbillboards,
            number3dtrees,
            numbillboards + numhorizbillboards,
            totaltriangles);
}

/*---------------------------------------------------------------------
Function: sptClearTrees
Description:
---------------------------------------------------------------------*/
void sptClearTrees()
{
}

/*---------------------------------------------------------------------
Function: sptExitApp
Description:
---------------------------------------------------------------------*/
void sptExitApp()
{
    // Find in-view supercells
    for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
    {
        if(!activeTerrainDetail[i])
            continue;

        if(trndtl[i].terrain == nullptr)
            continue;

        if(!(trndtl[i].terrain->flags & OBJECT_VISIBLE))
            continue;

        if ( trndtl[i].m_trees != nullptr )
        {
            sptFreeTrees(trndtl[i].m_trees);
        }
    }

    dotcommands.clear();
    treeDataKill();
}

/*---------------------------------------------------------------------
Function: sptCollideWithTree
Description:
---------------------------------------------------------------------*/
bool sptCollideWithTree(unsigned int address,
                        piVECTOR const* pt1,
                        piVECTOR const* pt2,
                        piVECTOR* collidePt,
                        float* time)
{
    Tree* tree = (Tree*)address;//treeDataGetTree(id);

    if(!tree)
        return false;

    if(tree->o == nullptr)
        return false;

    if(IS_BUSH(tree->isbush_isclutter_base))
        return false;

    CTree* basetree = treeDataGetBaseTree(GET_BASE(tree->isbush_isclutter_base));

    if(!basetree) return false;

    piVECTOR pos;
    float dimension[3], euler[3];
    CSpeedTreeRT::ECollisionObjectType type;

    piVECTOR raystart, rayend;

    unsigned int count = basetree->GetNumCollisionObjects();

    piMATRIX rot = IdentityMatrix;

    // Rotate euler collision capsule to the tree rotation
    rot._11 = tree->rotation.y();
    rot._12 = tree->rotation.z();
    rot._21 = tree->rotation.x();
    rot._22 = tree->rotation.y();

    Vec3f relativepos;

    if ( count != tree->collidematinv.size() )
    {
        Write_Debug("Base tree collision objects count != tree collidematinv matrix count!");
        return false;
    }

    for(unsigned int i = 0; i < count; i++)
    {
        basetree->GetCollisionObject(i, type, (float*)&pos, dimension, euler);

        if(type != CSpeedTreeRT::CO_CAPSULE)
        {
            continue;
        }

#if SCALE_TREES
        dimension[0] = FT_TO_METER(dimension[0]);
        dimension[1] = FT_TO_METER(dimension[1]);
#endif

        pos.x = -pos.x;
        pos.z = -pos.z;

#if SCALE_TREES
        pos.x() = FT_TO_METER(pos.x());
        pos.y() = FT_TO_METER(pos.y());
        pos.z() = FT_TO_METER(pos.z());
#endif

        // Rotate point to match tree rotation
        math3dMultiplyRotation(&pos, &pos, &rot);

        relativepos = tree->worldpos - WorldOriginOffset;

        // Move point to world
        pos.x += relativepos.x();
        pos.y += relativepos.y();
        pos.z += relativepos.z();

        // Translate ray to object space
        raystart.set(pt1->x - pos.x, pt1->y - pos.y, pt1->z - pos.z);
        rayend.set(pt2->x - pos.x, pt2->y - pos.y, pt2->z - pos.z);

        // Point is at origin. Some trees are angled or 'slanted'
        // Orient ray along the z axis. This matrix has the euler angles oriented
        // to the tree rotation
        piMATRIX m;
        tree->collidematinv[i].get(m);

        math3dMultiplyRotation(&raystart, &raystart, &m);
        math3dMultiplyRotation(&rayend, &rayend, &m);

        // Now ray is oriented correctly to the capsule as if the capsule is straight up the z axis
        // Translate back
        mathAddVectors(&raystart, &raystart, &pos);
        mathAddVectors(&rayend, &rayend, &pos);

        int collide = -1;

        // Now do the actual collision
        collide = mathRayToCylinder(&raystart,    // start point
                                    &rayend,      // end point
                                    &pos,         // cylinder pos
                                    dimension[0], // radius
                                    dimension[1], // height
                                    TRUE,         // check caps
                                    time,         // collide time
                                    collidePt);   // collide point

        if(collide == 0)
        {
            // Collision on cylinder orient along z axis. Rotate this point to proper position.
            // Bring to origin
            mathSubVectors(&raystart, collidePt, &pos);

            tree->collidemat[i].get(m);
            // Rotate
            math3dMultiplyRotation(&raystart, &raystart, &m);
            // Translate back
            mathAddVectors(&raystart, &raystart, &pos);

            collidePt->set(raystart.x, raystart.y, raystart.z);

            // Collision point is in relative world position.
            // We now have to hack it to make it fit into the FASTCOLLIDE object.
            math3dVectorMultiply(collidePt, collidePt, &tree->o->matint);

            return true;
        }
    }

    return false;
}

/*---------------------------------------------------------------------
Function: sptCollide
Description:
---------------------------------------------------------------------*/
bool sptCollide(const piVECTOR& pt1, const piVECTOR& pt2, FASTCOLLIDE** list, unsigned int& NumSlotsLeft)
{
    FASTCOLLIDE* collidelist = *list;

    colliderQuery::dGeomList& ray = TreeCollider->geoms();

    Vec3d p1(pt1);
    Vec3d p2(pt2);

    p1 += WorldOriginOffset;
    p2 += WorldOriginOffset;

    p1 = physicsGameToPhysical(p1);
    p2 = physicsGameToPhysical(p2);

    // 'F-me'
    p1.z() = -p1.z();
    p2.z() = -p2.z();

    Vec3d dir = p2 - p1;
    dGeomRaySetLength(ray[0], dir.length());

    dir.normalize();

    dGeomRaySet(ray[0], p1.x(), p1.y(), p1.z(), dir.x(), dir.y(), dir.z());

    TreeCollider->setDirty(true);

    if(TreeCollider->findCollisions(true))
    {
        colliderQuery::CollisionList& collisions = TreeCollider->getCollisions();

        collidelist->CollideObject = nullptr;
        collidelist->CollideTime = (float)collisions[0].depth;
        collidelist->Alpha = 1.0f;
        collidelist->Flags = ODE_TREE_COLLISION;

        Vec3d p = physicsPhysicalToGame(collisions[0].pos);

        p -= WorldOriginOffset;

        collidelist->CollidePnt.set((float)p.x(), (float)p.y(), (float)-p.z());

        collidelist++;
        NumSlotsLeft--;

        *list = collidelist;

        return true;
    }

    return false;
}

/*---------------------------------------------------------------------
Function: sptCollide
Description:
---------------------------------------------------------------------*/
bool sptCollide(const piVECTOR& pt1, const piVECTOR& pt2, piVECTOR& collidept, float& time)
{
    colliderQuery::dGeomList& ray = TreeCollider->geoms();

    Vec3d p1(pt1);
    Vec3d p2(pt2);

    p1 += WorldOriginOffset;
    p2 += WorldOriginOffset;

    p1 = physicsGameToPhysical(p1);
    p2 = physicsGameToPhysical(p2);

    // 'F-me'
    p1.z() = -p1.z();
    p2.z() = -p2.z();

    Vec3d dir = p2 - p1;
    dGeomRaySetLength(ray[0], dir.length());

    dir.normalize();

    dGeomRaySet(ray[0], p1.x(), p1.y(), p1.z(), dir.x(), dir.y(), dir.z());

    TreeCollider->setDirty(true);

    if(TreeCollider->findCollisions(true))
    {
        colliderQuery::CollisionList& collisions = TreeCollider->getCollisions();
        Vec3d p = physicsPhysicalToGame(collisions[0].pos);
        collidept.set((float)p.x(), (float)p.y(), (float)-p.z());

        return true;
    }

    return false;
}

/*---------------------------------------------------------------------
Function: sptCollideWithCluster
Description:
---------------------------------------------------------------------*/
bool sptCollideWithCluster(OBJECT* o,
                           const piVECTOR* p1,
                           const piVECTOR* p2,
                           piVECTOR* collisionpoint,
                           float* t,
                           int* tri)
{
    int counter = 0;
    int collides = 0;

    TreeMap::iterator find = objectTreeMap.find(o);

    if(find == objectTreeMap.end())
    {
        return false;
    }

//    printf("treemap size == %ld\n",objectTreeMap.size());

    piVECTOR dist;
    Vec3f relativepos;

    while(find != objectTreeMap.end())
    {
        if(o != find->first)
        {
            break;
        }

        Tree* tree = find->second;

        if(IS_BUSH(tree->isbush_isclutter_base))
        {
            ++find;
            continue;
        }

        counter++;

        relativepos = tree->worldpos - WorldOriginOffset;

        dist.x = relativepos.x() - p1->x;
        dist.y = relativepos.y() - p1->y;

        float r1 = dist.x * dist.x + dist.y * dist.y;

        dist.x = relativepos.x() - p2->x;
        dist.y = relativepos.y() - p2->y;

        float r2 = dist.x * dist.x + dist.y * dist.y;

        float radius2 = r1 < r2 ? r1 : r2;

        if(math3dFastSqrt(radius2) > (tree->radius + 1.0f))
        {
            ++find;
            continue;
        }

        collides++;

        if(sptCollideWithTree((unsigned int)tree, p1, p2, collisionpoint, t))
        {
//            printf("Found after %d checks with %d collides\n",counter,collides);
            return true;
        }

        (*tri)++;

        ++find;
    }

//    printf("None found after %d checks with %d collides\n",counter,collides);
    return false;
}

/*---------------------------------------------------------------------
Function: sptAddCluster
Description:
---------------------------------------------------------------------*/
void sptAddCluster(OBJECT* o, int* types/*[256]*/)
{
    int numberoftypes = types[0];

    if(!numberoftypes)
    {
        return;
    }

    logTimerBeginMacro();

    double x, y;

    piVECTOR& pp = o->pos;

    // First collide pos of object for matrix.
    if(o->flag2 & OBJECT_IS_ROTATED)
    {
        x = pp.x * o->mat._11 +
            pp.y * o->mat._21 +
            pp.z * o->mat._31 +
            o->mat._41;

        y = pp.x * o->mat._12 +
            pp.y * o->mat._22 +
            pp.z * o->mat._32 +
            o->mat._42;
    }
    else
    {
        x = pp.x + o->mat._41;
        y = pp.y + o->mat._42;
    }

    x += WorldOriginOffset.x;
    y += WorldOriginOffset.y;

    o->mat._43 = trnmainHeightAt(&trndtl[o->trnum], x, y);

    float highz = 10000.0f;
    float lowz = -10000.0f;
    unsigned int seed1;
	int seed2;
	FLOAT64VECTOR pos64;

    piVECTOR objpos;
    math3dVectorMultiply(&objpos, &pp, &o->mat);

    BoundingBox bb;

    piVectorConstPtr pos = o->ImmutablePositions();

    for(int i = 0; i < o->numverts; pos++, i++)
    {
        if(o->flag2 & OBJECT_IS_ROTATED)
        {
            x = pos->x * o->mat._11 +
                pos->y * o->mat._21 +
                pos->z * o->mat._31 +
                o->mat._41;

            y = pos->x * o->mat._12 +
                pos->y * o->mat._22 +
                pos->z * o->mat._32 +
                o->mat._42;
        }
        else
        {
            x = pos->x + o->mat._41;
            y = pos->y + o->mat._42;
        }

        x += WorldOriginOffset.x;
        y += WorldOriginOffset.y;

        TERRAIN_DETAIL* dtl = nullptr;

        // First find the supercell and then determine the row/colum of our octet
        int sx = (int)(x / SDM_SUPERCELL_SIZE);
        int sy = (int)(y / SDM_SUPERCELL_SIZE);

        sx = sx < 0 ? sx - 1 : sx;
        sy = sy < 0 ? sy - 1 : sy;

        for(int q = 0; q < NUM_TERRAIN_RECTANGLES; q++)
	    {
            if(activeTerrainDetail[q] && trndtl[q].scx == sx && trndtl[q].scy == sy)
            {
                dtl = &trndtl[q];
                break;
            }
	    }

        if(!dtl || !dtl->terrainorig)
            continue;

        PLANE* plane = trnmainGetPlaneAt(dtl, x, y);

        // Move to original position before the stupid world origin offset(s).
        float xx = (float)x;
        float yy = (float)y;

        xx -= dtl->terrainorig->mat._41;
        yy -= dtl->terrainorig->mat._42;

        xx -= (float)WorldOriginOffset.x;
        yy -= (float)WorldOriginOffset.y;

        // Get our plane height at this point
        float z = 0.0f;

        if(plane)
            z = plane->Z(xx, yy);

        bool changecluster = false;

        float angle1 = 0.0f;
        float angle2 = 0.0f;

        if(plane && plane->c != 0.0f)
        {
            angle1 = plane->a / plane->c;
            angle2 = plane->b / plane->c;

            if(angle1 != 0.0f)
                angle1 = RAD_TO_DEG(atan(angle1));

            if(angle2 != 0.0f)
                angle2 = RAD_TO_DEG(atan(angle2));

            if(angle1 > 7.0f || angle1 < -7.0f || angle2 > 7.0f || angle2 < -7.0f)
                changecluster = true;
        }

        // Use pos here and not c.newpos, since our friggen world offset
        // does not guarantee the world matrix is the same each time you arrive
        // at a specific location in the world.
        seed1 = (int)((SINT64)pos->x * (SINT64)113510315245LL *
                      (SINT64)pos->y * (SINT64)103515051525LL);
        seed2 = (int)seed1;

        // Get index for treetypes
        int ii = abs(mathGetRandomNumberFromSeed(&seed2) % numberoftypes) + 1;

        pos64.x = x;
        pos64.y = y;

		if (WWII::Theme::theme() == WWII::THEME::THEME_HOLIDAY_SEASON)
		{
			pos64.z = z;

			if(types[ii] - 1 != 2)
				// Sink a little to avoid floating roots
				pos64.z += 0.5f;
		}

        if(angle1 == 0.0f && angle2 == 0.0f)
            pos64.z = z;
        else
            // Sink a little to avoid floating roots
            pos64.z = z + 0.5f;

        if(z < highz)
        {
            highz = z;
        }

        if(z > lowz)
        {
            lowz = z;
        }

        unsigned int address = sptAddTree(o, types[ii] - 1, &trndtl[o->trnum], pos64, false, changecluster);

        if(address == 0)
        {
            continue;
        }

        Tree* t = (Tree*)address;

        if(!IS_BUSH(t->isbush_isclutter_base))
        {
            bb.expandBy(BoundingSphere(Vec3f(pos->x, pos->y, z), t->radius));
        }
    }

    if(bb.valid())
    {
        o->bbox[0].x = bb.xMin();
        o->bbox[0].y = bb.yMin();
        o->bbox[0].z = bb.zMin();
        o->bbox[1].x = bb.xMax();
        o->bbox[1].y = bb.yMax();
        o->bbox[1].z = bb.zMax();

        if(o->bbox[0].x > o->bbox[1].x)
        {
            std::swap(o->bbox[0].x, o->bbox[1].x);
        }

        if(o->bbox[0].y > o->bbox[1].y)
        {
            std::swap(o->bbox[0].y, o->bbox[1].y);
        }

        if(o->bbox[0].z > o->bbox[1].z)
        {
            std::swap(o->bbox[0].z, o->bbox[1].z);
        }

        o->pos.x -= (o->bbox[0].x + o->bbox[1].x) * 0.5f;
        o->pos.y -= (o->bbox[0].y + o->bbox[1].y) * 0.5f;
        o->pos.z -= (o->bbox[0].z + o->bbox[1].z) * 0.5f;

        // Adjust radius for engine culling
        o->maxrad = math3dDotProduct(&o->bbox[1], &o->bbox[1]);
        o->minrad = math3dDotProduct(&o->bbox[0], &o->bbox[0]);

        if(o->minrad > o->maxrad)
        {
            std::swap(o->minrad, o->maxrad);
        }
    }

    // Remove from standard rendering system
    obmainRemoveObjectTree(o);

    logTimerEndMacro();
}

/*---------------------------------------------------------------------
Function: releaseFromOctet
Description:
---------------------------------------------------------------------*/
static void releaseFromOctet(unsigned int address)
{
    Tree* tree = (Tree*)address;

    if(tree->octet && !tree->octet->empty())
    {
        TDTL_TREES::iterator find = tree->octet->find(address);

        if(find != tree->octet->end())
        {
            tree->octet->erase(find);
        }
    }
}

/*---------------------------------------------------------------------
Function: sptFreeTreeByID
Description:
---------------------------------------------------------------------*/
void sptFreeTreeByID(unsigned int address)
{
    if(treeDataIsValid(address))
    {
        cullnecessary = true;
        releaseFromOctet(address);
        treeDataFreeTree(address);
    }
}

/*---------------------------------------------------------------------
Function: sptFreeTreeByObject
Description:
---------------------------------------------------------------------*/
void sptFreeTreeByObject(OBJECT* key)
{
	if ( objectTreeMap.empty() )
		return;

	auto rng = objectTreeMap.equal_range(key);
	for ( auto it = rng.first ; it != rng.second ; ) {
		// Removing the tree will automatically remove the node in the tree map.
		auto free = it++ ;
		sptFreeTreeByID((unsigned int)free->second);
	}

	key->flag2 &= ~OBJECT_SPEEDTREE;
	key->flag3 &= ~OBJECT_TREE_CLUSTER;

	cullnecessary = true;
}

/*---------------------------------------------------------------------
Function: sptDrawOn
Description:
---------------------------------------------------------------------*/
void sptDrawOn(int type, bool draw)
{
    switch(type)
    {
    case 0:
        uiChatPrintLine("TREE drawing %s", draw ? "ON" : "OFF");
        drawtrees = draw;
        break;
    case 1:
        uiChatPrintLine("TREE branch drawing %s", draw ? "ON" : "OFF");
        drawbranches = draw;
        drawtrees = true;
        break;
    case 2:
        uiChatPrintLine("TREE frond drawing %s", draw ? "ON" : "OFF");
        drawfronds = draw;
        drawtrees = true;
        break;
    case 3:
        uiChatPrintLine("TREE leaf drawing %s", draw ? "ON" : "OFF");
        drawleaves = draw;
        drawtrees = true;
        break;
    case 4:
        uiChatPrintLine("TREE horizontal billboard drawing %s", draw ? "ON" : "OFF");
        drawhorizbillboards = draw;
        drawtrees = true;
        break;
    case 5:
        uiChatPrintLine("TREE vertical billboard drawing %s", draw ? "ON" : "OFF");
        drawbillboards = draw;
        drawtrees = true;
        break;
    case 6:
        uiChatPrintLine("TREE collision cylinder drawing %s", draw ? "ON" : "OFF");
        drawcylinders = draw;
        drawtrees = true;
        break;
    case 7:
        uiChatPrintLine("TREE bounding box drawing %s", draw ? "ON" : "OFF");
        drawboundbox = draw;
        drawtrees = true;
        break;
    default:
        uiChatPrintLine("Unrecognized tree command");
    }
}

/*---------------------------------------------------------------------
Function: sptToggleTreeCull
Description:
---------------------------------------------------------------------*/
void sptToggleTreeCull()
{
    nocull = !nocull;
    uiChatPrintLine(L"Tree culling %s", nocull ? L"DISABLED" : L"ENABLED");
}

/*---------------------------------------------------------------------
Function: sptIsTreeCollidable
Description:
---------------------------------------------------------------------*/
bool sptIsTreeCollidable(short base)
{
    CTree* tree = treeDataGetBaseTree(base);

	if(!tree)
	{
		Write_Log(LOG_OLD, LOG_INFO, "[sptIsTreeCollidable] ERROR: No Basetrees!");
		return false;
	}

    return tree->GetNumCollisionObjects() != 0;
}

/*---------------------------------------------------------------------
Function: determineOctet
Description:
---------------------------------------------------------------------*/
static bool determineOctet(Tree* t, TERRAIN_DETAIL* dtl)
{
    static int max_size = SUPERCELL_OCTETS_X * SUPERCELL_OCTETS_Y;

    // Find world location of tree object
    int r = 0;
    int c = 0;
    int sx = 0;
    int sy = 0;

    trnmainGetSupercellAndOctet(t->worldpos, sx, sy, r, c);

    // add tree or bush here

    unsigned char octet = (unsigned char)TRNINDEX(r, c);

    if(octet >= max_size)
    {
        return true;
    }

#ifdef SPATIAL_DB
    bool isbush = (IS_BUSH(t->isbush_isclutter_base)) || (IS_CLUTTER(t->isbush_isclutter_base));

    double xmin, xmax, ymin,ymax;
    xmin = t->worldpos.x() - t->smallradius;
    xmax = t->worldpos.x() + t->smallradius;
    ymin = t->worldpos.y() - t->smallradius;
    ymax = t->worldpos.y() + t->smallradius;

    SDB_AddTree( t, xmin, ymin, xmax, ymax, t->worldpos.z(), dtl, isbush );

#endif
	t->octet = &dtl->m_trees[octet];
	t->octet->insert((unsigned int)t);

    return true;
}

/*---------------------------------------------------------------------
Function: freeDotCommandTrees
Description:
---------------------------------------------------------------------*/
static void freeDotCommandTrees()
{
    for(unsigned int i = 0; i < dotcommands.size(); i++)
    {
        sptFreeTreeByID(dotcommands[i]);
    }

    dotcommands.clear();
}

/*---------------------------------------------------------------------
Function: sptAddTreeCommand
Description: Dot command for tree testing
---------------------------------------------------------------------*/
unsigned int sptAddTreeCommand(int base, const Vec3f& pos, bool FromHeightMap, float shadow)
{
    extern LOGICAL trnmainFindSupercell(SINT16* scx, SINT16* scy, piVECTOR* p);

    cullnecessary = true;

    if(base == -1)
    {
        freeDotCommandTrees();
        return 0;
    }

    CTree* tree = treeDataGetBaseTree(base);

    if(!tree)
        return 0;

    int id = 0;

    piVECTOR p;

	p.x = pos.x() + (float)WorldOriginOffset.x;
	p.y = pos.y() + (float)WorldOriginOffset.y;
	p.z = pos.z();

    int sx, sy, r, c;

    trnmainGetSupercellAndOctet(Vec3d(p), sx, sy, r, c);

    Vec3d worldpos;

    TERRAIN_DETAIL* dtl = nullptr;

    for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
        if(activeTerrainDetail[i] && trndtl[i].scx == sx && trndtl[i].scy == sy)
        {
            dtl = &trndtl[i];
            break;
        }
	}

    if(!FromHeightMap)
    {
        if(!dtl)
            return 0;

        FLOAT64VECTOR pp;

        if(gCustomTerrain)
        {
            Vec3f p1(-playpos.x, -playpos.y, playpos.z + 10.0f);
            Vec3f p2 = p1;
            p2.z() -= 200.0f;

            float z = collideRay(COLLIDER_TILE_MASK, 200.0f, p1.x(), p1.y(), p1.z());
            z = p1.z() + (z / 200.0f) * (p2.z() - p1.z());

	        pp.x = p1.x() + (float)WorldOriginOffset.x;
	        pp.y = p1.y() + (float)WorldOriginOffset.y;
            pp.z = -z;
        }
        else
        {
            COLLISION c;

            c.oldpos.x = pos.x();
            c.oldpos.y = pos.y();
            c.oldpos.z = -10000.0f;
            c.newpos = c.oldpos;

            c.newpos.z =  10000.0f;

            if(trncdCheckTerrainDownWithDTL(&c, dtl))
            {
	            pp.x = pos.x() + (float)WorldOriginOffset.x;
	            pp.y = pos.y() + (float)WorldOriginOffset.y;
                pp.z = c.colpos.z;
            }
        }

 	    dtl->terrain->flags |= OBJECT_VISIBLE;

        id = sptAddTree(nullptr, base, dtl, pp);

        dotcommands.push_back(id);

        worldpos.set(pp);
    }
    else
    {
        FLOAT64VECTOR pp;

        pp.x = pos.x() + WorldOriginOffset.x;
        pp.y = pos.y() + WorldOriginOffset.y;
        pp.z = pos.z();

        id = sptAddTree(nullptr, base, dtl, pp);

        worldpos.set(pp);
        dotcommands.push_back(id);
    }

	// Removed in new code?
    if(tree->IsCollidable())
    {
        float objpos[3], dimension[3], euler[3];
        CSpeedTreeRT::ECollisionObjectType type;

        Tree* t = (Tree*)(id);

        int numCollObjects = tree->GetNumCollisionObjects();

        Vec3d worldposition;

        for(int i = 0; i < numCollObjects; i++)
        {
            dGeomID transform = dCreateGeomTransform(colliderTreeSpace());
		    dGeomTransformSetCleanup(transform, 0);
		    dGeomTransformSetInfo(transform, 1);
            dGeomTransformSetGeom(transform, tree->GetCollideGeom());

            colliderSetBits(transform, COLLIDER_TRN_TREE, 0);

            worldposition = physicsGameToPhysical(worldpos);

            Matrixd roty, rotz;
            tree->GetCollisionObject(i, type, (float*)&objpos, dimension, euler);

            // Our stupid coordinate system bs
            roty.makeRotate(PI, Vec3f(0, 1, 0));

            rotz(0, 0) = t->rotation.y();
            rotz(0, 1) = t->rotation.z();
            rotz(1, 0) = t->rotation.x();
            rotz(1, 1) = t->rotation.y();

            // ODE has capsule pos is at the center. SpeedTree is at the end of the capsule
            // First, find offset position.
            Vec3f ray = Vec3f(0, 0, 1) * roty * t->collidemat[i];
            // Ray oriented along correct capsule rotation
            ray *= dimension[1];

#if DEBUG_SPEEDTREE
            _oderay = ray;
            _odepos = t->worldpos + Vec3f(-objpos[0], objpos[1], -objpos[2]) * rotz;
#endif
            // Half it for center pos
            ray *= 0.5f;
            // Rotate capsule object space pos to tree rotation
            ray += Vec3f(-objpos[0], objpos[1], -objpos[2]) * rotz;

            // Add to tree pos
            worldposition = worldposition + ray;

            dGeomSetPosition(transform, worldposition.x(), worldposition.y(), -worldposition.z());

            dMatrix3 rotation;
            // Stupid coordinate BS. Flip 180 degrees
            rotz.makeRotate(PI, Vec3f(0, 0, -1));

            t->collidemat[i] = t->collidemat[i] * rotz;
            MatrixTodMatrix(t->collidemat[i], rotation);

            dGeomSetRotation(transform, rotation);

            t->transform.push_back(transform);
        }
    }

    Tree* t = (Tree*)(id);
    t->rotation.w() = 1.0f;

    if(IS_BUSH(t->isbush_isclutter_base) || IS_CLUTTER(t->isbush_isclutter_base))
    {
        t->rotation.w() = shadow;
    }

    return id;
}

/*---------------------------------------------------------------------
Function: determineCollisionMatrix
Description: Create a matrix that takes a ray and orients it to the
collision cylinder and the random tree rotation. Collision cylinders
now can be angled in the direction of the tree branch. Invert this
matrix to place any collision rays in the same space as the cylinder
is when oriented along the z axis.
---------------------------------------------------------------------*/
void determineCollisionMatrix(Tree* tree)
{
    CTree* basetree = treeDataGetBaseTree(GET_BASE(tree->isbush_isclutter_base));

    if(!basetree->HasBranchGeometry())
    {
        return;
    }

    if(basetree->GetNumCollisionObjects() == 0)
    {
        return;
    }

    float pos[3], dimension[3], euler[3];
    CSpeedTreeRT::ECollisionObjectType type;

    Matrixf rot;

    // Rotate euler collision capsule to the tree rotation
    rot(0, 0) = tree->rotation.y();
    rot(0, 1) = tree->rotation.z();
    rot(1, 0) = tree->rotation.x();
    rot(1, 1) = tree->rotation.y();

    int numCollObjects = basetree->GetNumCollisionObjects();
    int numCapsules = basetree->GetNumCapsuleCollisionObjects();

    tree->collidemat.reserve(numCapsules); // mucho faster to reserve
    tree->collidematinv.reserve(numCapsules);

    for(int i = 0; i < numCollObjects; i++)
    {
        basetree->GetCollisionObject(i, type, pos, dimension, euler);

        if(type != CSpeedTreeRT::CO_CAPSULE)
        {
            continue;
        }

        tree->collidemat.push_back(basetree->euler[i] * rot);

        piMATRIX inv;

        tree->collidemat[i].get(inv);
        math3dInvertMatrix(&inv, &inv);
        tree->collidematinv.push_back(Matrixf(inv));
    }
}

/*---------------------------------------------------------------------
Function: sptAddTree
Description:
---------------------------------------------------------------------*/

const int numRotations = 12;
static int rotations[numRotations] = { 0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330 };

unsigned int sptAddTree(OBJECT* o,
                        int base,
                        TERRAIN_DETAIL* dtl,
                        FLOAT64VECTOR& pos,
                        bool isclutter,
                        bool changecluster)
{
    Tree* tree = treeDataGetFreeTree();

    if(tree == nullptr || !dtl)
        return 0;

    treeClampBaseIfNecessary(base);

    float box[6];
    treeDataGetBaseTree(base)->GetBoundingBox(box);

	if ( o )
		tree->attach(*o);

    if(!gCustomTerrain)
    {
        // HACK to sink cluster trees into ground, otherwise they float
        if(base == 4 || base == 5 || base == 21)
        {
            if(changecluster)
                base = 0;
            else
                pos.z += 3.0f;
        }
    }

    SET_BASE(tree->isbush_isclutter_base, base);

    if(treeDataGetBaseTree(base)->GetNumCollisionObjects() == 0)
        SET_IS_BUSH(tree->isbush_isclutter_base, 0x1);

    if(isclutter)
        SET_IS_CLUTTER(tree->isbush_isclutter_base, 0x1);

    float cx = (box[3] - box[0]);
    float cy = (box[4] - box[1]);
    float cz = (box[5] - box[2]);

    // Center point is at base of tree
    tree->radius = std::max(cz, std::max(cx, cy));

#ifdef SPATIAL_DB
    if(base == 4 || base == 5 || base == 21)
    {
        tree->smallradius = std::max(cx, cy);
    }
    else if ( IS_BUSH(tree->isbush_isclutter_base ) )
    {
        tree->smallradius = std::max(cx, cy) * 0.5;
    }
    else if ( IS_CLUTTER(tree->isbush_isclutter_base ) )
    {
        tree->smallradius = std::max(cx, cy);
    }
    else
    {
        tree->smallradius = std::max(cx, cy) * 0.3;
    }
#endif

    // 'Random'/seed rotation based on position
    //tree->rot_angle = (unsigned int)(7159 * (pos.x - (int)pos.x) + 15137 * (pos.y - (int)pos.y)) % 360;

    unsigned int randm = (unsigned int)((int)pos.x * 7159 + (int)pos.y * 15137);
    tree->rot_angle = rotations[randm % numRotations];

    // For billboards
    float radians = DEG_TO_RAD((float)tree->rot_angle);

    tree->rotation.x() = sin(radians);
    tree->rotation.y() = cos(radians);
    tree->rotation.z() = -tree->rotation.x();
    tree->rotation.w() = 1.0f;

#if(0)
    tree->windOffset = float(int(GetRandom(0.0f, 1.0f) * 10.0f));
#endif

    // Need object if this tree is collidable
    if(o)
    {
        if(!(o->flag3 & OBJECT_TREE_CLUSTER))
        {
            o->bbox[0].x = box[0];
            o->bbox[0].y = box[1];
            o->bbox[0].z = box[2];
            o->bbox[1].x = box[3];
            o->bbox[1].y = box[4];
            o->bbox[1].z = box[5];

            o->pos.x -= (o->bbox[0].x + o->bbox[1].x) * 0.5f;
            o->pos.y -= (o->bbox[0].y + o->bbox[1].y) * 0.5f;

            o->maxrad = tree->radius;
            o->treeID = (unsigned int)tree;
        }
    }

    tree->worldpos.set(pos);

    if(!determineOctet(tree, dtl))
    {
        treeDataFreeTree(tree);
        return 0;
    }

    determineCollisionMatrix(tree);

    return (unsigned int)tree;
}

/*---------------------------------------------------------------------
Function: sptDrawTreeShadowCasters
Description:
---------------------------------------------------------------------*/
bool sptDrawTreeShadowCasters()
{
    if(!drawhorizbillboards)
    {
        return false;
    }

    oglSetWorldMatrix(&IdentityMatrix);

    sptIdle();

    if(NeedCulling())
    {
        logTimerCallVoid(CullTrees);
    }

    if(numhorizbillboards == 0)
    {
        return false;
    }

    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, TRUE);
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_ALPHAREF, 84);

    sptDrawHorizBillboardsDeprecated();
    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);

    numhorizbillboards = 0;

    return true;
}

/*---------------------------------------------------------------------
Function: sptInTreeBBox
Description: Check 3D draw list
---------------------------------------------------------------------*/
bool sptInTreeBBox(const piVECTOR& pos, FLOAT32 altRadius)
{
    unsigned int size = treeDataGetBaseSize();
    piVECTOR p;
    Vec3f relativepos;

    for(unsigned int base = 0; base < size; base++)
    {
        // No trees for this base
        if(threeD[base].empty())
        {
            continue;
        }

        CTree* basetree = treeDataGetBaseTree(base);

        for(unsigned int j = 0; j < threeD[base].size(); j++)
        {
            Tree* t = threeD[base][j];

            if(IS_BUSH(t->isbush_isclutter_base))
                continue;

            relativepos = t->worldpos - WorldOriginOffset;

            // Bring to origin
            p.set(pos.x, pos.y, relativepos.z());

            mathSubVectors(&p, &p, (piVECTOR*)relativepos.ptr());

			// if we passed in an alternate radius, use that otherwise use the tree's radius
			FLOAT32 radius = (altRadius) ? altRadius : t->radius;

            if(math3dFastSqrt(math3dDotProduct(&p, &p)) < radius)
            {
                return true;
            }
        }
    }

    return false;
}

/*---------------------------------------------------------------------
Function: sptIsRadiusInTreeBBox
Description: Check if point and radius overlap tree radius (not the
same as t->radius in this case. t->radius used for culling)
---------------------------------------------------------------------*/
bool sptIsRadiusInTreeBBox(const piVECTOR& pos, float radius)
{
    const unsigned int size = treeDataGetBaseSize();
    piVECTOR p;
    Vec3f center;
    float dimensions[3]; // dimensions[0] = radius dimensions[1] = height
    float euler[3];
    CSpeedTreeRT::ECollisionObjectType type;

    for(unsigned int base = 0; base < size; base++)
    {
        // No trees for this base
        if(threeD[base].empty())
        {
            continue;
        }

        CTree* basetree = treeDataGetBaseTree(base);
        unsigned int count = basetree->GetNumCollisionObjects();

        Vec3f relativepos;

        for(unsigned int j = 0; j < threeD[base].size(); j++)
        {
            Tree* t = threeD[base][j];

            if(IS_BUSH(t->isbush_isclutter_base))
                continue;

            relativepos = t->worldpos - WorldOriginOffset;

            for(unsigned int q = 0; q < count; q++)
            {
                // Get tree pos within cluster
                basetree->GetCollisionObject(q, type, (float*)&center, dimensions, euler);

                if(type == CSpeedTreeRT::CO_CAPSULE)
                {
                    // Convert position to our f'd up coordinate system
                    center.x() = -center.x();
                    center.z() = -center.z();

                    piMATRIX rot = IdentityMatrix;

                    // Rotate if necessary
                    rot._11 = t->rotation.y();
                    rot._12 = t->rotation.z();
                    rot._21 = t->rotation.x();
                    rot._22 = t->rotation.y();

                    math3dVectorMultiply((piVECTOR*)center.ptr(), (piVECTOR*)center.ptr(), &rot);

                    // Translate to tree pos
                    center += relativepos;

                    // Bring pos to origin
                    p.set(pos.x, pos.y, pos.z);
                    // Match z's since some trees are sunk into the ground
                    center.z() = pos.z;

                    // Pos to origin, then tree to origin
                    mathSubVectors(&p, (piVECTOR*)center.ptr(), &p);

                    // Compare distance to tree radius
                    float distfromorigin = math3dFastSqrt(math3dDotProduct(&p, &p));

                    // Some radii from Speedtree is negative (WTF?)
                    if(distfromorigin - radius < fabs(dimensions[0]))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

/*---------------------------------------------------------------------
Function: sptFreeClutter
Description: Free octet clutter
---------------------------------------------------------------------*/
void sptFreeClutter(TDTL_TREES* trees)
{
    std::vector<unsigned int> remove;

    for(int i = SDM_SUPERCELL_OCTETS_XY; i != 0; --i)
    {
        TDTL_TREES& octet = trees[i - 1];

        if(!octet.empty())
        {
            TDTL_TREES::iterator itr = octet.begin();

            for(; itr != octet.end(); ++itr)
            {
                Tree* t = (Tree*)(*itr);

                if(IS_CLUTTER(t->isbush_isclutter_base))
                {
                    remove.push_back(*itr);
                }
            }
        }
    }

    for(unsigned int i = remove.size(); i != 0; --i)
    {
        sptFreeTreeByID(remove[i - 1]);
    }
}

/*---------------------------------------------------------------------
Function: sptFreeTrees
Description: Free octet trees
---------------------------------------------------------------------*/
void sptFreeTrees(TDTL_TREES* trees)
{
    std::vector<unsigned int> remove;

    for(int i = SDM_SUPERCELL_OCTETS_XY; i != 0; --i)
    {
        TDTL_TREES& octet = trees[i - 1];

        if(!octet.empty())
        {
            TDTL_TREES::iterator itr = octet.begin();

            for(; itr != octet.end(); ++itr)
            {
                Tree* t = (Tree*)(*itr);
                remove.push_back(*itr);
            }

            octet.clear();
        }
    }

    for(unsigned int i = remove.size(); i != 0; --i)
    {
        sptFreeTreeByID(remove[i - 1]);
    }
}
