#include <syscfg.h> // Path is project specific.
#pragma warning(disable:4786)

#include "spt/Tree.h"
#include "spt/Random.h"
#include "ld/lddds.h"
#include "include/vars.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "math/math3d.h"
#include "rn/rnlogo.h"
#include "ogl/oglGlobalStates.h"
#include "ld/ldtexture.h"
#include "math/Vec2d.h"
#include "ld/ldpix.h"
#include "math/mathUnitConversion.h"
#include "math/matheul.h"
#include "ld/ldtex.h"
#include "ogl/oglMaterial.h"
#include "ogl/oglTextureObject.h"
#include "ob/obdraw.h"
#include "shader/glslUniformBlock.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

#include <string>

#define FIND_NO_GEOMETRY    0
#define WANT_LEAF_NORMALS   1

int TextureLoad(std::string& name, unsigned int flags = 0);

#if CHRISTMAS
static bool xmas = false;
#endif

static int _dummy_detail = -1;

std::string CTree::treedir = "";
static ref_ptr<oglTextureObject> treetbo;

/*---------------------------------------------------------------------
Function: CTree
Description:
---------------------------------------------------------------------*/
CTree::CTree()
    : leafRockingState(true),
      hasHorizBillboardGeom(false),
      hasBranchGeometry(false),
      farLodDistanceSqr(0.0f),
      nearLodDistance(0.0f),
      nearLodDistanceSqr(0.0f),
      scale(1.0f),
      collidegeom(0),
#if CHRISTMAS
      christmas(false),
#endif
      wind(nullptr)

{
    branchmaterialON = nullptr;
    branchmaterialOFF = nullptr;
    branchmaterial = nullptr;

    leafmaterialON = nullptr;
    leafmaterialOFF = nullptr;
    leafmaterial = nullptr;

    billboardmaterial = nullptr;
    billboardmaterialON = nullptr;
    billboardmaterialOFF = nullptr;

    horizbillboardmaterial = nullptr;
    horizbillboardmaterialON = nullptr;
    horizbillboardmaterialOFF = nullptr;

    if(!ogl::UseDeprecatedPath())
    {
        if(!treetbo.valid())
        {
            treetbo = oglTextureObject::GetTextureObject("dynamicglobal");
}
    }
}

/*---------------------------------------------------------------------
Function: ~CTree
Description:
---------------------------------------------------------------------*/
CTree::~CTree()
{
}

/*---------------------------------------------------------------------
Function: SelectShader
Description:
---------------------------------------------------------------------*/
void CTree::SelectShader()
{
    if(ogl::UseDeprecatedPath())
    {
        if(obdrawIsRenderPass(RENDER_SHADOW_MAP | RENDER_SCENE_DEPTH))
        {
            branchmaterial = branchdepth;
            leafmaterial = leafdepth;
            frondmaterial = fronddepth;
        }
        else
        {
            branchmaterial = g_wantNormalMaps ? branchmaterialON : branchmaterialOFF;
            leafmaterial = g_wantNormalMaps ? leafmaterialON : leafmaterialOFF;
            frondmaterial = frondmaterialOFF;
        }
    }
    else
    {
        if(obdrawIsRenderPass(RENDER_SHADOW_MAP | RENDER_SCENE_DEPTH))
        {
            branchmaterial = branchdepth;
            leafmaterial = leafdepth;
            frondmaterial = fronddepth;
        }
        else
        {
            branchmaterial = branchmaterialON;
            leafmaterial = leafmaterialON;
            frondmaterial = frondmaterialOFF;
        }
    }

    if(ogl::UseDeprecatedPath())
    {
        billboardmaterial = g_wantNormalMaps ? billboardmaterialON : billboardmaterialOFF;
        horizbillboardmaterial = g_wantNormalMaps ? horizbillboardmaterialON : horizbillboardmaterialOFF;
    }
    else
    {
        billboardmaterial = billboardmaterialON;
        horizbillboardmaterial = horizbillboardmaterialON;
    }
}

/*---------------------------------------------------------------------
Function: GetBoundingBox
Description:
---------------------------------------------------------------------*/
void CTree::GetBoundingBox(float box[6])
{
    float b[6];
    CSpeedTreeRT::GetBoundingBox(b);

    // #@$!@# coordinate system!
    box[0] = -b[3];
    box[1] = -b[4];
    box[2] = -b[5];
    box[3] = -b[0];
    box[4] = -b[1];
    box[5] = -b[2];
}

/*---------------------------------------------------------------------
Function: LoadTree
Description:
---------------------------------------------------------------------*/
bool CTree::LoadSPTFile(const char* filename,
                        unsigned int seed,
                        float n,
                        float f,
                        CSpeedWind* w)
{
    wind = w;
    nearLodDistance = n;
    farLodDistance = f;
    nearLodDistanceSqr = n * n;
    farLodDistanceSqr = f * f;

#if CHRISTMAS
    xmas = false;

    if(strstr(filename, "Christmas"))
        xmas = true;

    christmas = xmas;
#endif

    CSpeedTreeRT::SetNumWindMatrices(wind->GetNumWindMatrices());
    CSpeedTreeRT::SetTextureFlip(false);

    treename = treedir + filename;

    size_t numbytes = fioFileSize(treename.c_str());

	if(numbytes == -1)
	{
		return false;
	}

    int fd = fioOpen(treename.c_str());

	if(fd == -1)
	{
		return false;
	}

	unsigned char* block = (unsigned char*)memAlloc(numbytes);

    fioRead(fd, block, numbytes);
	fioClose(fd);

    if(!CSpeedTreeRT::LoadTree(block, numbytes))
    {
        Write_Log(LOG_OLD, LOG_WARN, CSpeedTreeRT::GetCurrentError());
        return false;
    }

    memFree(block);

    SetBranchLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
    SetLeafLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
    SetFrondLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);

    SetBranchWindMethod(CSpeedTreeRT::WIND_GPU);
    SetLeafWindMethod(CSpeedTreeRT::WIND_GPU);
    SetFrondWindMethod(CSpeedTreeRT::WIND_GPU);

    scale = 1.0f;

#if SCALE_TREES
    float treesize, trash;
    GetTreeSize(treesize, trash);
    SetTreeSize(FT_TO_METER(treesize));
#endif

    if(!Compute(nullptr, seed))
    {
        Write_Log(LOG_OLD, LOG_WARN, CSpeedTreeRT::GetCurrentError());
        return false;
    }

    CSpeedTreeRT::SetLodLimits(n, f);
    SetLeafRockingState(true);
    SetDropToBillboard(true);

    float box[6];
    GetBoundingBox(box);

    float cx = (box[3] - box[0]);
    float cy = (box[4] - box[1]);
    float cz = (box[5] - box[2]);

    // Center point is at base of tree
    radius = std::max(cz, std::max(cx, cy));

    // Some collision trees have angled cylinders. Determine matrix once during init.
    for(int i = 0; i < GetNumCollisionObjects(); i++)
    {
        float eulerAngles[3];
        float pos[3];
        float dimension[3];

        CSpeedTreeRT::ECollisionObjectType type;

        GetCollisionObject(i, type, pos, dimension, eulerAngles);

        if(type != CSpeedTreeRT::CO_CAPSULE)
        {
            continue;
        }

        piMATRIX m = IdentityMatrix;
        piVECTOR v = {DEG_TO_RAD(eulerAngles[0]), DEG_TO_RAD(-eulerAngles[1]), DEG_TO_RAD(eulerAngles[2])};

        mathEul_ToMatrix(&m, EulOrdXYZs, &v);
        euler.push_back(Matrixf(m));
    }

    // scale ambient values
    GetLightShaderParams(lightParams);

#if USE_STATESET
    if(!branchdepth.valid())
    {
        branchdepth = new RenderStateSet;
        branchdepth->SetMode(GL_LIGHTING, 0);
        branchdepth->SetRenderState(new ColorMask(false, false, false, false));
        branchdepth->SetRenderState(new Program("BranchDepth"));
        branchdepth->SetMode(GL_FOG, 0);
    }

    if(!fronddepth.valid())
    {
        fronddepth = new RenderStateSet;
        fronddepth->SetRenderState(new Program("FrondDepth"));
        fronddepth->SetMode(GL_LIGHTING, 0);
        fronddepth->SetMode(GL_FOG, 0);
        fronddepth->SetRenderState(new ColorMask(false, false, false, false));
        fronddepth->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    }

    if(!leafdepth.valid())
    {
        leafdepth = new RenderStateSet;
        leafdepth->SetRenderState(new Program("LeafDepth"));
        leafdepth->SetMode(GL_LIGHTING, 0);
        leafdepth->SetMode(GL_FOG, 0);
        leafdepth->SetRenderState(new ColorMask(false, false, false, false));
        leafdepth->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    }
#else
    if(!branchdepth.valid())
    {
        branchdepth = new oglMaterial;
        branchdepth->SetShader("BranchDepth");
    }

    if(!fronddepth.valid())
    {
        fronddepth = new oglMaterial;
        fronddepth->SetShader("FrondDepth");
    }

    if(!leafdepth.valid())
    {
        leafdepth = new oglMaterial;
        leafdepth->SetShader("LeafDepth");
    }
#endif

#if USE_STATESET
    billboardmaterialON = new RenderStateSet;
    billboardmaterialON->SetMode(GL_LIGHTING, 0);
    billboardmaterialON->SetMode(GL_ALPHA_TEST, 1);

    horizbillboardmaterialON = new RenderStateSet;
    horizbillboardmaterialON->SetMode(GL_LIGHTING, 0);
    horizbillboardmaterialON->SetMode(GL_ALPHA_TEST, 1);
#else
    branchmaterialON = new oglMaterial;
    leafmaterialON = new oglMaterial;
    frondmaterialOFF = new oglMaterial;
    billboardmaterialON = new oglMaterial;
    horizbillboardmaterialON = new oglMaterial;
#endif

    if(ogl::UseDeprecatedPath())
    {
#if USE_STATESET
        billboardmaterialOFF = new RenderStateSet;
        horizbillboardmaterialOFF = new RenderStateSet;

        billboardmaterialOFF->SetMode(GL_LIGHTING, 0);
        billboardmaterialOFF->SetMode(GL_ALPHA_TEST, 1);

        horizbillboardmaterialOFF->SetMode(GL_LIGHTING, 0);
        horizbillboardmaterialOFF->SetMode(GL_ALPHA_TEST, 1);
#else
        branchmaterialOFF = new oglMaterial;
        leafmaterialOFF = new oglMaterial;
        billboardmaterialOFF = new oglMaterial;
        horizbillboardmaterialOFF = new oglMaterial;
#endif
    }

    return true;
}

/*------------------------------------------------------------------------
Function: TextureLoad
Description:
------------------------------------------------------------------------*/
int TextureLoad(std::string& name, unsigned int flags)
{
    if(name.empty())
    {
        return -1;
    }

    size_t postga = name.find(".tga");

    if(postga == -1)
    {
        postga = name.find(".TGA");
    }

    if(postga != -1)
    {
#if CHRISTMAS
        if(!xmas)
            name.replace(postga, 4, ".dds");
#endif
    }

    std::string path = CTree::GetTreeDirectory();

    size_t pos = name.find_last_of("\\");

    if(pos == -1)
    {
        pos = name.find_last_of("/");
    }

    if(pos != -1)
    {
        pos++;

#if CHRISTMAS
        if(xmas)
            path += "Christmas_Tree/";
#endif

        path += name.substr(pos, name.length() - pos);
    }
    else
    {
#if CHRISTMAS
        if(xmas)
            path += "Christmas_Tree/";
#endif
        path += name;
    }

    return ldTextureLoad(path, flags);
}

#if USE_STATESET
/*------------------------------------------------------------------------
Function: LoadLeafFrondTexture
Description:
------------------------------------------------------------------------*/
bool CTree::LoadLeafFrondTexture()
{
    if(!HasLeafGeometry() && !HasFrondGeometry())
    {
        return true;
    }

    CSpeedTreeRT::SMapBank mapbank;
    GetMapBank(mapbank);

    std::string name;

#if CHRISTMAS
    if(christmas)
        name = "terrain/trees4.2/Christmas_Tree/CompositeMap_Diffuse.dds";
    else
#endif
    name = mapbank.m_pCompositeMaps[CSpeedTreeRT::TL_DIFFUSE];

    LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE] = TextureLoad(name, LDPIX_FLIP);

    if(LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE] > -1)
    {
        Texture2D* tex = ldTextureGet2D(LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);

        if(tex)
        {
            tex->setWrap(Texture2D::WRAP_S, Texture2D::REPEAT);
            tex->setWrap(Texture2D::WRAP_T, Texture2D::REPEAT);

            leafmaterialON->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]));

            if(leafmaterialOFF.valid())
            {
                leafmaterialOFF->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]));
                leafdepth->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]));
            }

            if(frondmaterialOFF.valid())
            {
                frondmaterialOFF->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]));
                fronddepth->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]));
            }
        }
    }

    name = mapbank.m_pCompositeMaps[CSpeedTreeRT::TL_NORMAL];

    LeafFrondMaps[CSpeedTreeRT::TL_NORMAL] = TextureLoad(name, LDPIX_FLIP);

    if(LeafFrondMaps[CSpeedTreeRT::TL_NORMAL] > -1)
    {
        Texture2D* tex = ldTextureGet2D(LeafFrondMaps[CSpeedTreeRT::TL_NORMAL]);

        if(tex)
        {
            tex->setWrap(Texture2D::WRAP_S, Texture2D::REPEAT);
            tex->setWrap(Texture2D::WRAP_T, Texture2D::REPEAT);

            leafmaterialON->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_NORMAL, LeafFrondMaps[CSpeedTreeRT::TL_NORMAL]));
        }
    }

    return true;
}
#else
/*------------------------------------------------------------------------
Function: LoadLeafFrondTexture
Description:
------------------------------------------------------------------------*/
bool CTree::LoadLeafFrondTexture()
{
    if(!HasLeafGeometry() && !HasFrondGeometry())
    {
        return true;
    }

    CSpeedTreeRT::SMapBank mapbank;
    GetMapBank(mapbank);

    std::string name;

#if CHRISTMAS
    if(christmas)
        name = "terrain/trees4.2/Christmas_Tree/CompositeMap_Diffuse.dds";
    else
#endif
    name = mapbank.m_pCompositeMaps[CSpeedTreeRT::TL_DIFFUSE];

    LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE] = TextureLoad(name, LDPIX_FLIP);

    if(LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE] > -1)
    {
        Texture2D* tex = ldTextureGet2D(LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);

        if(tex)
        {
            tex->setWrap(Texture2D::WRAP_S, Texture2D::REPEAT);
            tex->setWrap(Texture2D::WRAP_T, Texture2D::REPEAT);

            leafmaterialON->AddTexture(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);

            if(leafmaterialOFF.valid())
            {
                leafmaterialOFF->AddTexture(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);
                leafdepth->AddTexture(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);
            }

            if(frondmaterialOFF.valid())
            {
                frondmaterialOFF->AddTexture(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);
                fronddepth->AddTexture(CSpeedTreeRT::TL_DIFFUSE, LeafFrondMaps[CSpeedTreeRT::TL_DIFFUSE]);
            }
        }
    }

#if CHRISTMAS
    if(christmas)
        name = "terrain/trees4.2/Christmas_Tree/CompositeMap_Normal.dds";
    else
#endif
    name = mapbank.m_pCompositeMaps[CSpeedTreeRT::TL_NORMAL];

    LeafFrondMaps[CSpeedTreeRT::TL_NORMAL] = TextureLoad(name, LDPIX_FLIP);

    if(LeafFrondMaps[CSpeedTreeRT::TL_NORMAL] > -1)
    {
        Texture2D* tex = ldTextureGet2D(LeafFrondMaps[CSpeedTreeRT::TL_NORMAL]);

        if(tex)
        {
            tex->setWrap(Texture2D::WRAP_S, Texture2D::REPEAT);
            tex->setWrap(Texture2D::WRAP_T, Texture2D::REPEAT);

            leafmaterialON->AddTexture(CSpeedTreeRT::TL_NORMAL, LeafFrondMaps[CSpeedTreeRT::TL_NORMAL]);
        }
    }

    leafmaterialON->AddTexture(CSpeedTreeRT::TL_DETAIL, -1);

    if(leafmaterialOFF.valid())
        leafmaterialOFF->AddTexture(CSpeedTreeRT::TL_DETAIL, -1);

    return true;
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: LoadBillboardTexture
Description:
------------------------------------------------------------------------*/
bool CTree::LoadBillboardTexture()
{
    CSpeedTreeRT::SMapBank mapbank;
    GetMapBank(mapbank);

    for(int i = CSpeedTreeRT::TL_DIFFUSE; i <= CSpeedTreeRT::TL_NORMAL; i++)
    {
        std::string name = mapbank.m_pBillboardMaps[i];

        if(name.empty())
        {
            continue;
        }

        BillboardMaps[i] = TextureLoad(name, LDPIX_FLIP);

        if(BillboardMaps[i] > -1)
        {
            Texture2D* tex = ldTextureGet2D(BillboardMaps[i]);

            if(tex)
            {
                tex->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP);
                tex->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP);

                billboardmaterialON->SetRenderState(new TextureUnit(i, BillboardMaps[i]));
                horizbillboardmaterialON->SetRenderState(new TextureUnit(i, BillboardMaps[i]));

                if(i == CSpeedTreeRT::TL_DIFFUSE)
                {
                    if(billboardmaterialOFF.valid())
                        billboardmaterialOFF->SetRenderState(new TextureUnit(i, BillboardMaps[i]));

                    if(horizbillboardmaterialOFF.valid())
                        horizbillboardmaterialOFF->SetRenderState(new TextureUnit(i, BillboardMaps[i]));
                }
            }
        }
    }

    return true;
}
#else
/*------------------------------------------------------------------------
Function: LoadBillboardTexture
Description:
------------------------------------------------------------------------*/
bool CTree::LoadBillboardTexture()
{
    CSpeedTreeRT::SMapBank mapbank;
    GetMapBank(mapbank);

    for(int i = CSpeedTreeRT::TL_DIFFUSE; i <= CSpeedTreeRT::TL_NORMAL; i++)
    {
        std::string name = mapbank.m_pBillboardMaps[i];

        if(name.empty())
        {
            continue;
        }

        BillboardMaps[i] = TextureLoad(name, LDPIX_FLIP);

        if(BillboardMaps[i] > -1)
        {
            Texture2D* tex = ldTextureGet2D(BillboardMaps[i]);

            if(tex)
            {
                tex->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP);
                tex->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP);

                billboardmaterialON->AddTexture(i, BillboardMaps[i]);
                horizbillboardmaterialON->AddTexture(i, BillboardMaps[i]);

                if(i == CSpeedTreeRT::TL_DIFFUSE)
                {
                    if(billboardmaterialOFF.valid())
                        billboardmaterialOFF->AddTexture(i, BillboardMaps[i]);

                    if(horizbillboardmaterialOFF.valid())
                        horizbillboardmaterialOFF->AddTexture(i, BillboardMaps[i]);
                }
            }
        }
    }

    return true;
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: LoadBranchTexture
Description:
------------------------------------------------------------------------*/
bool CTree::LoadBranchTexture()
{
    if(!HasBranchGeometry())
    {
        return true;
    }

    if(!oglIsTexture(_dummy_detail, 0))
    {
        _dummy_detail = oglCreateRawTexture(-1, 0, 4, 4, 24, 4, CRT_FLAG_NONE);
        ldtexFillTexture(_dummy_detail, 0, colorBLACK);
    }

    CSpeedTreeRT::SMapBank mapbank;
    GetMapBank(mapbank);

    for(int i = CSpeedTreeRT::TL_DIFFUSE; i <= CSpeedTreeRT::TL_NORMAL; i++)
    {
        std::string name = mapbank.m_pBranchMaps[i];

        BranchMaps[i] = TextureLoad(name, LDPIX_FLIP);

        if(BranchMaps[i] > -1)
        {
            Texture2D* tex = ldTextureGet2D(BranchMaps[i]);

            if(tex)
            {
                tex->setWrap(Texture2D::WRAP_S, Texture2D::REPEAT);
                tex->setWrap(Texture2D::WRAP_T, Texture2D::REPEAT);

                branchmaterialON->SetRenderState(new TextureUnit(i, BranchMaps[i]));

                if(i != CSpeedTreeRT::TL_NORMAL && branchmaterialOFF.valid())
                    branchmaterialOFF->SetRenderState(new TextureUnit(i, BranchMaps[i]));
            }
        }
        else
        {
            if(i == CSpeedTreeRT::TL_DETAIL)
            {
                BranchMaps[i] = _dummy_detail;

                branchmaterialON->SetRenderState(new TextureUnit(i, _dummy_detail));

                if(branchmaterialOFF.valid())
                    branchmaterialOFF->SetRenderState(new TextureUnit(i, _dummy_detail));
            }
        }
    }

    return true;
}
#else
/*------------------------------------------------------------------------
Function: LoadBranchTexture
Description:
------------------------------------------------------------------------*/
bool CTree::LoadBranchTexture()
{
    if(!HasBranchGeometry())
    {
        return true;
    }

    if(!oglIsTexture(_dummy_detail, 0))
    {
        _dummy_detail = oglCreateRawTexture(-1, 0, 4, 4, 24, 4, CRT_FLAG_NONE);
        ldtexFillTexture(_dummy_detail, 0, colorBLACK);
    }

    CSpeedTreeRT::SMapBank mapbank;
    GetMapBank(mapbank);

    for(int i = CSpeedTreeRT::TL_DIFFUSE; i <= CSpeedTreeRT::TL_NORMAL; i++)
    {
        std::string name = mapbank.m_pBranchMaps[i];

        BranchMaps[i] = TextureLoad(name, LDPIX_FLIP);

        if(BranchMaps[i] > -1)
        {
            Texture2D* tex = ldTextureGet2D(BranchMaps[i]);

            if(tex)
            {
                tex->setWrap(Texture2D::WRAP_S, Texture2D::REPEAT);
                tex->setWrap(Texture2D::WRAP_T, Texture2D::REPEAT);

                branchmaterialON->AddTexture(i, BranchMaps[i]);

                if(i != CSpeedTreeRT::TL_NORMAL && branchmaterialOFF.valid())
                    branchmaterialOFF->AddTexture(i, BranchMaps[i]);
            }
        }
        else
        {
            if(i == CSpeedTreeRT::TL_DETAIL)
            {
                BranchMaps[i] = _dummy_detail;

                branchmaterialON->AddTexture(i, _dummy_detail);

                if(branchmaterialOFF.valid())
                    branchmaterialOFF->AddTexture(i, _dummy_detail);
            }
        }
    }

    return true;
}
#endif

/*------------------------------------------------------------------------
Function: AddBranchDraw
Description:
------------------------------------------------------------------------*/
void CTree::AddBranchDraw(const Tree* tree, const Vec4f& pos, const Vec4f& rot)
{
    logTimerBeginMacro();

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_BranchGeometry);

    const CSpeedTreeRT::SGeometry::SIndexed& b = geo.m_sBranches;

    if(tree->branchlod < b.m_nNumLods)
    {
        if(b.m_pNumStrips[static_cast<int>(tree->branchlod)] > 0)
        {
            if(tree->branchlod < MAX_LODS)
            {
                if(data[static_cast<int>(tree->branchlod)].empty())
                    data[static_cast<int>(tree->branchlod)].resize(2048);

                if(BranchCount[static_cast<int>(tree->branchlod)] >= 1024)
                {
                    PushBranches();
                }

                int idx = BranchCount[static_cast<int>(tree->branchlod)] * 2;

                data[static_cast<int>(tree->branchlod)][idx + 0] = rot;
                data[static_cast<int>(tree->branchlod)][idx + 1] = pos;

                BranchCount[static_cast<int>(tree->branchlod)]++;
            }
        }
    }

    logTimerEndMacro();
}

/*------------------------------------------------------------------------
Function: AddFrondDraw
Description:
------------------------------------------------------------------------*/
void CTree::AddFrondDraw(const Tree* tree, const Vec4f& pos, const Vec4f& rot)
{
    logTimerBeginMacro();

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_FrondGeometry);

    const CSpeedTreeRT::SGeometry::SIndexed& f = geo.m_sFronds;

    if(tree->frondlod < f.m_nNumLods)
    {
        if(f.m_pNumStrips[static_cast<int>(tree->frondlod)] > 0)
        {
            if(tree->frondlod < MAX_LODS)
            {
                if(data[static_cast<int>(tree->frondlod)].empty())
                    data[static_cast<int>(tree->frondlod)].resize(2048);

                if(FrondCount[static_cast<int>(tree->frondlod)] >= 1024)
                {
                    PushFronds();
            }

                int idx = FrondCount[static_cast<int>(tree->frondlod)] * 2;

                data[static_cast<int>(tree->frondlod)][idx + 0] = rot;
                data[static_cast<int>(tree->frondlod)][idx + 1] = pos;

                FrondCount[static_cast<int>(tree->frondlod)]++;
        }
    }
}

    logTimerEndMacro();
}

/*------------------------------------------------------------------------
Function: AddLeafDraw
Description:
------------------------------------------------------------------------*/
void CTree::AddLeafDraw(const Tree* tree, const Vec4f& pos, const Vec4f& rot, int whichlod)
{
    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_LeafGeometry);

    if(whichlod >= geo.m_nNumLeafLods)
    {
        return;
    }

    logTimerBeginMacro();

    int lod = tree->leaflods[whichlod];

    if(data[lod].empty())
        data[lod].resize(2048);

    if(LeafCount[lod] >= 1024)
    {
        PushLeaves();
    }

    int idx = LeafCount[lod] * 2;

    data[lod][idx + 0] = rot;
    data[lod][idx + 1] = pos;

    LeafCount[lod]++;

    logTimerEndMacro();
}

/*------------------------------------------------------------------------
Function: PushLeaves
Description:
------------------------------------------------------------------------*/
int CTree::PushLeaves()
{
    int tris = 0;

    logTimerBeginMacro();

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_LeafGeometry);

    oglVertexBuffer* vbo = treeDataGetVBO();

    for(int lod = 0; lod < MAX_LODS; ++lod)
    {
        if(LeafCount[lod])
        {
            treetbo->UpdateBuffer(LeafCount[lod] * 2 * sizeof(Vec4f), &data[lod][0], 0);

            if(LeafCount[lod] > 1)
                vbo->DrawInstancedPrimitive(LeafCount[lod], 0, leafvboid[lod]);
            else
                vbo->DrawPrimitive(0, leafvboid[lod]);

            const SGeometry::SLeaf& leaves = geo.m_pLeaves[lod];

            tris += (leaves.m_nNumLeaves * 2) * LeafCount[lod];

            LeafCount[lod] = 0;
        }
    }

    logTimerEndMacro();

    return tris;
}

/*------------------------------------------------------------------------
Function: PushBranches
Description:
------------------------------------------------------------------------*/
int CTree::PushBranches()
{
    int tris = 0;

    logTimerBeginMacro();

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_BranchGeometry);

    const CSpeedTreeRT::SGeometry::SIndexed& b = geo.m_sBranches;

    oglVertexBuffer* vbo = treeDataGetVBO();

    for(int lod = 0; lod < MAX_LODS; ++lod)
    {
        if(BranchCount[lod])
        {
            treetbo->UpdateBuffer(BranchCount[lod] * 2 * sizeof(Vec4f), &data[lod][0], 0);

            if(BranchCount[lod] > 1)
                vbo->DrawInstancedPrimitive(BranchCount[lod], lod, branchvboid);
            else
                vbo->DrawPrimitive(lod, branchvboid);

            tris += (b.m_pStripLengths[lod][0] - 2) * BranchCount[lod];

            BranchCount[lod] = 0;
        }
    }

    logTimerEndMacro();

    return tris;
}

/*------------------------------------------------------------------------
Function: PushFronds
Description:
------------------------------------------------------------------------*/
int CTree::PushFronds()
{
    int tris = 0;

    logTimerBeginMacro();

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_FrondGeometry);

    const CSpeedTreeRT::SGeometry::SIndexed& f = geo.m_sFronds;

    oglVertexBuffer* vbo = treeDataGetVBO();

    for(int lod = 0; lod < MAX_LODS; ++lod)
    {
        if(FrondCount[lod])
        {
            treetbo->UpdateBuffer(FrondCount[lod] * 2 * sizeof(Vec4f), &data[lod][0], 0);

            if(FrondCount[lod] > 1)
                vbo->DrawInstancedPrimitive(FrondCount[lod], lod, frondvboid);
            else
                vbo->DrawPrimitive(lod, frondvboid);

            tris += (f.m_pStripLengths[lod][0] - 2) * FrondCount[lod];

            FrondCount[lod] = 0;
        }
    }

    logTimerEndMacro();

    return tris;
}

/*------------------------------------------------------------------------
Function: DrawBranchesDeprecated
Description:
------------------------------------------------------------------------*/
int CTree::DrawBranchesDeprecated(Tree* tree)
{
    if(tree->branchlod < CSpeedTreeRT::GetNumBranchLodLevels())
    {
        oglVertexBuffer* vbo = treeDataGetVBO();
        vbo->DrawPrimitive(tree->branchlod, branchvboid);
    }

    return 0;
}

/*------------------------------------------------------------------------
Function: DrawFrondsDeprecated
Description:
------------------------------------------------------------------------*/
int CTree::DrawFrondsDeprecated(Tree* tree)
{
    if(tree->frondlod < CSpeedTreeRT::GetNumFrondLodLevels())
    {
        oglVertexBuffer* vbo = treeDataGetVBO();
        vbo->DrawPrimitive(tree->frondlod, frondvboid);
    }

    return 0;
}

/*------------------------------------------------------------------------
Function: DrawLeavesDeprecated
Description:
------------------------------------------------------------------------*/
int CTree::DrawLeavesDeprecated(Tree* tree, int whichlod)
{
    if(whichlod < CSpeedTreeRT::GetNumLeafLodLevels())
    {
        oglVertexBuffer* vbo = treeDataGetVBO();
        vbo->DrawPrimitive(0, leafvboid[whichlod]);
    }

    return 0;
}

/*------------------------------------------------------------------------
Function: EndBranchType
Description:
------------------------------------------------------------------------*/
void CTree::EndBranchType()
{
    if(obdrawIsRenderPass(RENDER_SCENE_DEPTH))
    {
        oglVertexBuffer* vbo = treeDataGetVBO();

        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_NORMAL, true);
        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_TEXCOORD0, true);
        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_TEXCOORD1, true);
        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_TEXCOORD2, true);
    }
}

/*------------------------------------------------------------------------
Function: StartBranchType
Description:
------------------------------------------------------------------------*/
void CTree::StartBranchType()
{
    memset(BranchCount, 0, sizeof(int) * MAX_LODS);

    if(branchmaterial.valid())
        branchmaterial->Apply();

    if(obdrawIsRenderPass(RENDER_SCENE_DEPTH))
    {
        oglVertexBuffer* vbo = treeDataGetVBO();

        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_NORMAL, false);
        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_TEXCOORD0, false);
        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_TEXCOORD1, false);
        vbo->SetActiveAttribute(branchvboid, oglVertexBuffer::OVB_TEXCOORD2, false);
    }
}

/*------------------------------------------------------------------------
Function: StartLeafType
Description:
------------------------------------------------------------------------*/
void CTree::StartLeafType()
{
    if(leafmaterial.valid())
        leafmaterial->Apply();

    memset(LeafCount, 0, sizeof(int) * MAX_LODS);

    if(obdrawIsRenderPass(RENDER_SCENE_DEPTH))
    {
        for(int i = 0; i < CSpeedTreeRT::GetNumLeafLodLevels(); i++)
        {
            oglVertexBuffer* vbo = treeDataGetVBO();

            vbo->SetActiveAttribute(leafvboid[i], oglVertexBuffer::OVB_NORMAL, false);

          //  vbo->SetActiveAttribute(leafvboid[i], oglVertexBuffer::OVB_TEXCOORD2, false);
          //  vbo->SetActiveAttribute(leafvboid[i], oglVertexBuffer::OVB_TEXCOORD3, false);
        }
    }
}

/*------------------------------------------------------------------------
Function: StartBillboardType
Description:
------------------------------------------------------------------------*/
void CTree::StartBillboardType()
{
    if(billboardmaterial.valid())
        billboardmaterial->Apply();
}

/*------------------------------------------------------------------------
Function: EndBillboardType
Description:
------------------------------------------------------------------------*/
void CTree::EndBillboardType()
{
}

/*------------------------------------------------------------------------
Function: StartHorizBillboardType
Description:
------------------------------------------------------------------------*/
void CTree::StartHorizBillboardType()
{
    horizbillboardmaterial->Apply();
}

/*------------------------------------------------------------------------
Function: EndHorizBillboardType
Description:
------------------------------------------------------------------------*/
void CTree::EndHorizBillboardType()
{
}

/*------------------------------------------------------------------------
Function: EndLeafType
Description:
------------------------------------------------------------------------*/
void CTree::EndLeafType()
{
    if(obdrawIsRenderPass(RENDER_SCENE_DEPTH))
    {
        for(int i = 0; i < CSpeedTreeRT::GetNumLeafLodLevels(); i++)
        {
            oglVertexBuffer* vbo = treeDataGetVBO();

            vbo->SetActiveAttribute(leafvboid[i], oglVertexBuffer::OVB_NORMAL, true);
           // vbo->SetActiveAttribute(leafvboid[i], oglVertexBuffer::OVB_TEXCOORD2, true);
          //  vbo->SetActiveAttribute(leafvboid[i], oglVertexBuffer::OVB_TEXCOORD3, true);
        }
    }
}

/*------------------------------------------------------------------------
Function: EndFrondType
Description:
------------------------------------------------------------------------*/
void CTree::EndFrondType()
{
    if(obdrawIsRenderPass(RENDER_SCENE_DEPTH))
    {
        oglVertexBuffer* vbo = treeDataGetVBO();
        vbo->SetActiveAttribute(frondvboid, oglVertexBuffer::OVB_NORMAL, true);
    }
}

/*------------------------------------------------------------------------
Function: StartFrondType
Description:
------------------------------------------------------------------------*/
void CTree::StartFrondType()
{
    if(frondmaterial.valid())
        frondmaterial->Apply();

    memset(FrondCount, 0, sizeof(int) * MAX_LODS);

    if(obdrawIsRenderPass(RENDER_SCENE_DEPTH))
    {
        oglVertexBuffer* vbo = treeDataGetVBO();
        vbo->SetActiveAttribute(frondvboid, oglVertexBuffer::OVB_NORMAL, false);
    }
}

/*------------------------------------------------------------------------
Function: SetupFrondGeometryDeprecated
Description:
------------------------------------------------------------------------*/
void CTree::SetupFrondGeometryDeprecated()
{
    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_FrondGeometry);

    const SGeometry::SIndexed& fronds = geo.m_sFronds;

    if(fronds.m_nNumVertices == 0)
    {
        hasFrondGeometry = false;
        return;
    }

    if(fronds.m_pNumStrips[0] == 0)
    {
        hasFrondGeometry = false;
        return;
    }

#if FIND_NO_GEOMETRY
    for(int i = 0; i < fronds.m_nNumLods; i++)
    {
        if(fronds.m_pNumStrips[i] == 0)
        {
            Enable_Log(LOG_OLD, 1);
            Write_Log(LOG_OLD, LOG_INFO, "[FRONDS] TREE = %s LOD = %d", treename.c_str(), i);
            Enable_Log(LOG_OLD, 0);
        }
    }
#endif

    hasFrondGeometry = true;

    std::vector<float> fronddata;

    // Interleave data
    for(int i = 0; i < fronds.m_nNumVertices; i++)
    {
        fronddata.push_back(-fronds.m_pCoords[i * 3 + 0]); //0
        fronddata.push_back(fronds.m_pCoords[i * 3 + 1]); //4
        fronddata.push_back(-fronds.m_pCoords[i * 3 + 2]); //8

        fronddata.push_back(-fronds.m_pNormals[i * 3 + 0]); //12
        fronddata.push_back(fronds.m_pNormals[i * 3 + 1]); //16
        fronddata.push_back(-fronds.m_pNormals[i * 3 + 2]); //20

        const float* uv = &fronds.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE][i * 2];
        fronddata.push_back(uv[0]); //24
        fronddata.push_back(uv[1]); //28

#if(0)
        float s = 0.0f;
        float t = 0.0f;

        float index1 = float(int(fronds.m_pWindMatrixIndices[0][i] * 10.0f / wind->GetNumWindMatrices()));
        float index2 = float(int(fronds.m_pWindMatrixIndices[1][i] * 10.0f / wind->GetNumWindMatrices()));
        float weight1 = fronds.m_pWindWeights[0][i];
        float weight2 = fronds.m_pWindWeights[1][i];

        fronddata.push_back(index1 + weight1); //32
        fronddata.push_back(index2 + weight2); //36
        fronddata.push_back(s); //40
        fronddata.push_back(t); //44
#endif
    }

    static unsigned int stride = 32;

    oglVertexBuffer* vbo = treeDataGetVBO();

    frondvboid = vbo->AddBuffer(&fronddata[0], fronddata.size() * sizeof(float), true);

    vbo->SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, 0, frondvboid);
    vbo->SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, 12, frondvboid);
    vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, stride, 24, frondvboid);
#if(0)
    frondbuffer->SetStride(oglVertexBuffer::OVB_TEXCOORD1, 4, GL_FLOAT, stride, 32);
#endif

    for(int i = 0; i < fronds.m_nNumLods; i++)
    {
        if(fronds.m_pNumStrips[i] == 0 || fronds.m_pStripLengths[i][0] == 0)
            continue;

        if(fronds.m_pNumStrips[i] == 1)
        {
            vbo->AddDrawRangeElementsPrimitive(GL_TRIANGLE_STRIP,
                                               fronds.m_pStripLengths[i][0],
                                               fronds.m_pStrips[i][0],
                                               frondvboid);
        }
    }

    const float* m = CSpeedTreeRT::GetFrondMaterial();

    float scale = lightParams.m_fAmbientScalar;

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(m[0], m[1], m[2], 1.0f);
    material.ambient.set(m[3] * scale, m[4] * scale, m[5] * scale, 1.0f);
    material.specular.set(m[6], m[7], m[8], 1.0f);
    material.emissive.set(m[9], m[10], m[11], 1.0f);

    int id = oglCreateMaterial((void*)&material);

#if USE_STATESET
    frondmaterialOFF = new RenderStateSet;

    frondmaterialOFF->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    frondmaterialOFF->SetRenderState(new Material(id));
    frondmaterialOFF->SetRenderState(new Program("FrondShader"));
    frondmaterialOFF->SetMode(GL_LIGHTING, 0);
    frondmaterialOFF->SetMode(GL_ALPHA_TEST, 1);
    //frondmaterialOFF->SetRenderState(new DepthFunc(GL_EQUAL));
#else
    frondmaterialOFF->SetMaterial(id);
    frondmaterialOFF->SetShader("FrondShader");
#endif
}

/*------------------------------------------------------------------------
Function: SetupFrondGeometry
Description:
------------------------------------------------------------------------*/
void CTree::SetupFrondGeometry()
{
    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_FrondGeometry);

    const SGeometry::SIndexed& fronds = geo.m_sFronds;

    if(fronds.m_nNumVertices == 0)
    {
        hasFrondGeometry = false;
        return;
    }

    if(fronds.m_pNumStrips[0] == 0)
    {
        hasFrondGeometry = false;
        return;
    }

#if FIND_NO_GEOMETRY
    for(int i = 0; i < fronds.m_nNumLods; i++)
    {
        if(fronds.m_pNumStrips[i] == 0)
        {
            Enable_Log(LOG_OLD, 1);
            Write_Log(LOG_OLD, LOG_INFO, "[FRONDS] TREE = %s LOD = %d", treename.c_str(), i);
            Enable_Log(LOG_OLD, 0);
        }
    }
#endif

    hasFrondGeometry = true;

    std::vector<float> fronddata;

    // Interleave data
    for(int i = 0; i < fronds.m_nNumVertices; i++)
    {
        fronddata.push_back(-fronds.m_pCoords[i * 3 + 0]); //0
        fronddata.push_back(fronds.m_pCoords[i * 3 + 1]); //4
        fronddata.push_back(-fronds.m_pCoords[i * 3 + 2]); //8

        fronddata.push_back(-fronds.m_pNormals[i * 3 + 0]); //12
        fronddata.push_back(fronds.m_pNormals[i * 3 + 1]); //16
        fronddata.push_back(-fronds.m_pNormals[i * 3 + 2]); //20

        const float* uv = &fronds.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE][i * 2];
        fronddata.push_back(uv[0]); //24
        fronddata.push_back(uv[1]); //28

#if(0)
        float s = 0.0f;
        float t = 0.0f;

        float index1 = float(int(fronds.m_pWindMatrixIndices[0][i] * 10.0f / wind->GetNumWindMatrices()));
        float index2 = float(int(fronds.m_pWindMatrixIndices[1][i] * 10.0f / wind->GetNumWindMatrices()));
        float weight1 = fronds.m_pWindWeights[0][i];
        float weight2 = fronds.m_pWindWeights[1][i];

        fronddata.push_back(index1 + weight1); //32
        fronddata.push_back(index2 + weight2); //36
        fronddata.push_back(s); //40
        fronddata.push_back(t); //44
#endif
    }

    static unsigned int stride = 32;

    oglVertexBuffer* vbo = treeDataGetVBO();

    frondvboid = vbo->AddBuffer(&fronddata[0], fronddata.size() * sizeof(float), true, oglVertexBuffer::MODE_ARB_VAO);

    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, stride, 0, frondvboid);
    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, stride, 12, frondvboid);
    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, stride, 24, frondvboid);

    for(int i = 0; i < fronds.m_nNumLods; i++)
    {
        if(fronds.m_pNumStrips[i] == 0 || fronds.m_pStripLengths[i][0] == 0)
            continue;

        if(fronds.m_pNumStrips[i] == 1)
        {
            vbo->AddDrawElementsPrimitive(GL_TRIANGLE_STRIP,
                                          fronds.m_pStripLengths[i][0],
                                          fronds.m_pStrips[i][0],
                                          frondvboid);
        }
    }

    const float* m = CSpeedTreeRT::GetFrondMaterial();

    float scale = lightParams.m_fAmbientScalar;

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(m[0], m[1], m[2], 1.0f);
    material.ambient.set(m[3] * scale, m[4] * scale, m[5] * scale, 1.0f);
    material.specular.set(m[6], m[7], m[8], 1.0f);
    material.emissive.set(m[9], m[10], m[11], 1.0f);

    int id = oglCreateMaterial((void*)&material);

#if USE_STATESET
    frondmaterialOFF->SetRenderState(new Material(id));
    frondmaterialOFF->SetRenderState(new Program("FrondShader"));
    frondmaterialOFF->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_USER1, GL_TEXTURE_BUFFER, treetbo->GetTextureObjectName()));
    frondmaterialOFF->SetMode(GL_ALPHA_TEST, 1);
#else
    frondmaterialOFF->SetMaterial(id);
    frondmaterialOFF->SetShader("FrondShader");
    frondmaterialOFF->AddTexture(CSpeedTreeRT::TL_USER1, GL_TEXTURE_BUFFER, treetbo->GetTextureObjectName());
#endif
}

/*------------------------------------------------------------------------
Function: SetupBranchGeometry
Description:
------------------------------------------------------------------------*/
void CTree::SetupBranchGeometry()
{
    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_BranchGeometry);

    const SGeometry::SIndexed& branches = geo.m_sBranches;

    if(branches.m_nNumVertices == 0)
    {
        hasBranchGeometry = false;
        return;
    }

    if(branches.m_pNumStrips[0] == 0)
    {
        hasBranchGeometry = false;
        return;
    }

#if FIND_NO_GEOMETRY
    for(int i = 0; i < branches.m_nNumLods; i++)
    {
        if(branches.m_pNumStrips[i] == 0)
        {
            Enable_Log(LOG_OLD, 1);
            Write_Log(LOG_OLD, LOG_INFO, "[BRANCHES] TREE = %s LOD = %d", treename.c_str(), i);
            Enable_Log(LOG_OLD, 0);
        }
    }
#endif

    hasBranchGeometry = true;

    std::vector<float> branchdata;

    // Interleave data
    for(int i = 0; i < branches.m_nNumVertices; i++)
    {
        // Positions
        branchdata.push_back(-branches.m_pCoords[i * 3 + 0]); //0
        branchdata.push_back(branches.m_pCoords[i * 3 + 1]); //4
        branchdata.push_back(-branches.m_pCoords[i * 3 + 2]); //8

        // Normals
        branchdata.push_back(-branches.m_pNormals[i * 3 + 0]); //12
        branchdata.push_back(branches.m_pNormals[i * 3 + 1]); //16
        branchdata.push_back(-branches.m_pNormals[i * 3 + 2]); //20

        // Tex coord 0
        const float* uv = &branches.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE][i * 2];
        branchdata.push_back(uv[0]); //24
        branchdata.push_back(uv[1]); //28

        // Normal map tex coords (unit 1) xy
        const float* normal = &branches.m_pTexCoords[CSpeedTreeRT::TL_NORMAL][i * 2];
        branchdata.push_back(normal[0]); //32
        branchdata.push_back(normal[1]); //36

        // Detail map tex coords (unit 1) zw
        const float* detail = &branches.m_pTexCoords[CSpeedTreeRT::TL_DETAIL][i * 2];
        branchdata.push_back(detail[0]); //40
        branchdata.push_back(detail[1]); //44

        // Derive binormals in the shader
        if(branches.m_pTangents)
        {
            branchdata.push_back(-branches.m_pTangents[i * 3 + 0]); //48
            branchdata.push_back(branches.m_pTangents[i * 3 + 1]); //52
            branchdata.push_back(-branches.m_pTangents[i * 3 + 2]); //56
        }
        else
        {
            branchdata.push_back(0); //48
            branchdata.push_back(0); //52
            branchdata.push_back(0); //56
        }
    }

    static unsigned int stride = 60;

    oglVertexBuffer* vbo = treeDataGetVBO();

    branchvboid = vbo->AddBuffer(&branchdata[0], branchdata.size() * sizeof(float), true, oglVertexBuffer::MODE_ARB_VAO);

    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, stride, 0, branchvboid);
    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, stride, 12, branchvboid);
    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, stride, 24, branchvboid);
    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 4, GL_FLOAT, stride, 32, branchvboid);
    vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, stride, 48, branchvboid);

    for(int i = 0; i < branches.m_nNumLods; i++)
    {
        if(branches.m_pStripLengths[i][0] == 0)
            continue;

        if(branches.m_pNumStrips[i] == 1)
        {
            vbo->AddDrawElementsPrimitive(GL_TRIANGLE_STRIP,
                                          branches.m_pStripLengths[i][0],
                                          branches.m_pStrips[i][0],
                                          branchvboid);
        }
    }

    const float* m = CSpeedTreeRT::GetBranchMaterial();

    float scale = lightParams.m_fAmbientScalar;

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(m[0], m[1], m[2], 1.0f);
    material.ambient.set(m[3] * scale, m[4] * scale, m[5] * scale, 1.0f);
    material.specular.set(m[6], m[7], m[8], 1.0f);
    material.emissive.set(m[9], m[10], m[11], 1.0f);

    int id = oglCreateMaterial((void*)&material);

#if USE_STATESET
    branchmaterialON->SetRenderState(new Material(id));
    branchmaterialON->SetRenderState(new Program("BranchShaderNormal"));
    branchmaterialON->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_USER1, GL_TEXTURE_BUFFER, treetbo->GetTextureObjectName()));
    branchmaterialON->SetMode(GL_ALPHA_TEST, 1);
#else
    branchmaterialON->SetMaterial(id);
    branchmaterialON->SetShader("BranchShaderNormal");
    branchmaterialON->AddTexture(CSpeedTreeRT::TL_USER1, GL_TEXTURE_BUFFER, treetbo->GetTextureObjectName());
#endif
}

/*------------------------------------------------------------------------
Function: SetupBranchGeometryDeprecated
Description:
------------------------------------------------------------------------*/
void CTree::SetupBranchGeometryDeprecated()
{
    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_BranchGeometry);

    const SGeometry::SIndexed& branches = geo.m_sBranches;

    if(branches.m_nNumVertices == 0)
    {
        hasBranchGeometry = false;
        return;
    }

    if(branches.m_pNumStrips[0] == 0)
    {
        hasBranchGeometry = false;
        return;
    }

#if FIND_NO_GEOMETRY
    for(int i = 0; i < branches.m_nNumLods; i++)
    {
        if(branches.m_pNumStrips[i] == 0)
        {
            Enable_Log(LOG_OLD, 1);
            Write_Log(LOG_OLD, LOG_INFO, "[BRANCHES] TREE = %s LOD = %d", treename.c_str(), i);
            Enable_Log(LOG_OLD, 0);
        }
    }
#endif

    hasBranchGeometry = true;

    std::vector<float> branchdata;

    // Interleave data
    for(int i = 0; i < branches.m_nNumVertices; i++)
    {
        // Positions
        branchdata.push_back(-branches.m_pCoords[i * 3 + 0]); //0
        branchdata.push_back(branches.m_pCoords[i * 3 + 1]); //4
        branchdata.push_back(-branches.m_pCoords[i * 3 + 2]); //8

        // Normals
        branchdata.push_back(-branches.m_pNormals[i * 3 + 0]); //12
        branchdata.push_back(branches.m_pNormals[i * 3 + 1]); //16
        branchdata.push_back(-branches.m_pNormals[i * 3 + 2]); //20

        // Tex coord 0
        const float* uv = &branches.m_pTexCoords[CSpeedTreeRT::TL_DIFFUSE][i * 2];
        branchdata.push_back(uv[0]); //24
        branchdata.push_back(uv[1]); //28

        // Normal map tex coords (unit 1) xy
        const float* normal = &branches.m_pTexCoords[CSpeedTreeRT::TL_NORMAL][i * 2];
        branchdata.push_back(normal[0]); //32
        branchdata.push_back(normal[1]); //36

        // Detail map tex coords (unit 1) zw
        const float* detail = &branches.m_pTexCoords[CSpeedTreeRT::TL_DETAIL][i * 2];
        branchdata.push_back(detail[0]); //40
        branchdata.push_back(detail[1]); //44

        // Derive binormals in the shader
        if(branches.m_pTangents)
        {
            branchdata.push_back(-branches.m_pTangents[i * 3 + 0]); //48
            branchdata.push_back(branches.m_pTangents[i * 3 + 1]); //52
            branchdata.push_back(-branches.m_pTangents[i * 3 + 2]); //56
        }
        else
        {
            branchdata.push_back(0); //48
            branchdata.push_back(0); //52
            branchdata.push_back(0); //56
        }
    }

    static unsigned int stride = 60;

    oglVertexBuffer* vbo = treeDataGetVBO();

    branchvboid = vbo->AddBuffer(&branchdata[0], branchdata.size() * sizeof(float), true);

    vbo->SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, 0, branchvboid);
    vbo->SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, 12, branchvboid);
    vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, stride, 24, branchvboid);
    vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD1, 4, GL_FLOAT, stride, 32, branchvboid);
#if(0)
    branchbuffer->SetStride(oglVertexBuffer::OVB_TEXCOORD2, 4, GL_FLOAT, stride, 48);
    branchbuffer->SetStride(oglVertexBuffer::VERTEX_TEXCOORD3, 3, GL_FLOAT, stride, 64);
#else
    vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD2, 3, GL_FLOAT, stride, 48, branchvboid);
#endif

    for(int i = 0; i < branches.m_nNumLods; i++)
    {
        if(branches.m_pStripLengths[i][0] == 0)
            continue;

        if(branches.m_pNumStrips[i] == 1)
        {
            vbo->AddDrawRangeElementsPrimitive(GL_TRIANGLE_STRIP,
                                               branches.m_pStripLengths[i][0],
                                               branches.m_pStrips[i][0],
                                               branchvboid);
        }
    }

    const float* m = CSpeedTreeRT::GetBranchMaterial();

    float scale = lightParams.m_fAmbientScalar;

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(m[0], m[1], m[2], 1.0f);
    material.ambient.set(m[3] * scale, m[4] * scale, m[5] * scale, 1.0f);
    material.specular.set(m[6], m[7], m[8], 1.0f);
    material.emissive.set(m[9], m[10], m[11], 1.0f);

    int id = oglCreateMaterial((void*)&material);

#if USE_STATESET
    branchmaterialON = new RenderStateSet;

    branchmaterialON->SetRenderState(new Material(id));
    branchmaterialON->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    branchmaterialON->SetRenderState(new Program("BranchShaderNormal"));
    branchmaterialON->SetMode(GL_LIGHTING, 0);
   // branchmaterialON->SetRenderState(new DepthFunc(GL_EQUAL));

    branchmaterialOFF = new RenderStateSet;

    branchmaterialOFF->SetRenderState(new Material(id));
    branchmaterialOFF->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    branchmaterialOFF->SetRenderState(new Program("BranchShader"));
    branchmaterialOFF->SetMode(GL_LIGHTING, 0);
  //  branchmaterialOFF->SetRenderState(new DepthFunc(GL_EQUAL));
#else
    branchmaterialON->SetMaterial(id);
    branchmaterialON->SetShader("BranchShaderNormal");

    branchmaterialOFF->SetMaterial(id);
    branchmaterialOFF->SetShader("BranchShader");
#endif
}

/*------------------------------------------------------------------------
Function: SetupLeafCardGeometry
Description:
------------------------------------------------------------------------*/
void CTree::SetupLeafCardGeometry()
{
    unsigned int lodcount = CSpeedTreeRT::GetNumLeafLodLevels();

    if(lodcount == 0)
    {
        return;
    }

    std::vector<float> leafdata;

    hasLeafGeometry = false;

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_LeafGeometry);

    int corner = 0;

    for(unsigned int lod = 0; lod < lodcount; lod++)
    {
        const SGeometry::SLeaf& leaves = geo.m_pLeaves[lod];
        const int numleaves = leaves.m_nNumLeaves;

        if(numleaves == 0)
        {
            continue;
        }

        if(lod == 0)
        {
            leafRockScalar = leaves.m_fLeafRockScalar;
            leafRustleScalar = leaves.m_fLeafRustleScalar;
        }

        const int numSpeedWindAngles = wind->GetNumLeafAngleMatrices();

        Matrixf unit(0.0f, -0.5f, -0.5f, 0.0f,
                     0.0f,  0.5f, -0.5f, 0.0f,
                     0.0f,  0.5f,  0.5f, 0.0f,
                     0.0f, -0.5f,  0.5f, 0.0f);

        for(int leaf = 0; leaf < numleaves; leaf++)
        {
            const CSpeedTreeRT::SGeometry::SLeaf::SCard* card = leaves.m_pCards + leaves.m_pLeafCardIndices[leaf];

            if(!card)
            {
                continue;
            }

            hasLeafGeometry = true;

            for(corner = 0; corner < 4; corner++)
            {
                Vec3f pivotpt = Vec3f(0.0f, unit(corner, 1), unit(corner, 2));
                pivotpt.y() += card->m_afPivotPoint[0] - 0.5f;
                pivotpt.z() += card->m_afPivotPoint[1] - 0.5f;

                Vec3f cornerpt;
                cornerpt.x() = pivotpt.x() * card->m_fWidth;
                cornerpt.y() = pivotpt.y() * card->m_fWidth;
                cornerpt.z() = pivotpt.z() * card->m_fHeight;

                leafdata.push_back(cornerpt.x()); //0
                leafdata.push_back(cornerpt.y()); //4
                leafdata.push_back(cornerpt.z()); //8

                float x = -leaves.m_pNormals[leaf * 12 + corner * 3 + 0];
                float y = leaves.m_pNormals[leaf * 12 + corner * 3 + 1];
                float z = -leaves.m_pNormals[leaf * 12 + corner * 3 + 2];

                leafdata.push_back(x); //12
                leafdata.push_back(y); //16
                leafdata.push_back(z); //20

                float index1 = float(int(leaves.m_pWindMatrixIndices[0][leaf] * 10.0f / wind->GetNumWindMatrices()));
                float index2 = float(int(leaves.m_pWindMatrixIndices[1][leaf] * 10.0f / wind->GetNumWindMatrices()));

                float weight1 = leaves.m_pWindWeights[0][leaf];
                float weight2 = leaves.m_pWindWeights[1][leaf];

                leafdata.push_back(card->m_pTexCoords[corner * 2 + 0]); //24
                leafdata.push_back(card->m_pTexCoords[corner * 2 + 1]); //28

                leafdata.push_back(index1 + weight1); //32
                leafdata.push_back(index2 + weight2); //36

                x = -leaves.m_pCenterCoords[leaf * 3 + 0];
                y = leaves.m_pCenterCoords[leaf * 3 + 1];
                z = -leaves.m_pCenterCoords[leaf * 3 + 2];

                leafdata.push_back(x); //40
                leafdata.push_back(y); //44
                leafdata.push_back(z); //48

                leafdata.push_back(DEG_TO_RAD(card->m_afAngleOffsets[0])); //52
                leafdata.push_back(DEG_TO_RAD(card->m_afAngleOffsets[1])); //56
                leafdata.push_back(float(leaf % numSpeedWindAngles)); //60
                leafdata.push_back(leaves.m_pDimming[leaf]); //64

#if WANT_LEAF_NORMALS
                if(leaves.m_pTangents)
                {
                    leafdata.push_back(-leaves.m_pTangents[12 * leaf + 3 * corner + 0]); //68
                    leafdata.push_back(leaves.m_pTangents[12 * leaf + 3 * corner + 1]); //72
                    leafdata.push_back(-leaves.m_pTangents[12 * leaf + 3 * corner + 2]); //76
                }
                else
                {
                    leafdata.push_back(0.0f);
                    leafdata.push_back(0.0f);
                    leafdata.push_back(0.0f);
                }
#endif
            }
        }

#if WANT_LEAF_NORMALS
        static unsigned int stride = 80;
#else
        static unsigned int stride = 68;
#endif
        oglVertexBuffer* vbo = treeDataGetVBO();

        leafvboid[lod] = vbo->AddBuffer(&leafdata[0], leafdata.size() * sizeof(float), true, oglVertexBuffer::MODE_ARB_VAO);

        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, stride, 0, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, stride, 12, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, stride, 24, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 3, GL_FLOAT, stride, 40, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 4, GL_FLOAT, stride, 52, leafvboid[lod]);
#if WANT_LEAF_NORMALS
        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, stride, 68, leafvboid[lod]);
#endif

        vbo->AddDrawArraysPrimitive(GL_QUADS, 0, leaves.m_nNumLeaves * 4, leafvboid[lod]);
        leafdata.clear();
    }

    const float* m = CSpeedTreeRT::GetLeafMaterial();

    float scale = lightParams.m_fAmbientScalar;

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(m[0], m[1], m[2], 1.0f);
    material.ambient.set(m[3] * scale, m[4] * scale, m[5] * scale, 1.0f);
    material.specular.set(m[6], m[7], m[8], 1.0f);
    material.emissive.set(m[9], m[10], m[11], 1.0f);

    int id = oglCreateMaterial((void*)&material);

#if USE_STATESET
    leafmaterialON = new RenderStateSet;

    leafmaterialON->SetRenderState(new Material(id));
    leafmaterialON->SetRenderState(new Program("LeafShaderNormal"));
    leafmaterialON->SetRenderState(new TextureUnit(CSpeedTreeRT::TL_USER1, GL_TEXTURE_BUFFER, treetbo->GetTextureObjectName()));
    //leafmaterialON->SetRenderState(new DepthFunc(GL_EQUAL));
#else
    leafmaterialON->SetMaterial(id);
    leafmaterialON->SetShader("LeafShaderNormal");
    leafmaterialON->AddTexture(CSpeedTreeRT::TL_USER1, GL_TEXTURE_BUFFER, treetbo->GetTextureObjectName());
#endif
}

/*------------------------------------------------------------------------
Function: SetupLeafCardGeometryDeprecated
Description:
------------------------------------------------------------------------*/
void CTree::SetupLeafCardGeometryDeprecated()
{
    unsigned int lodcount = CSpeedTreeRT::GetNumLeafLodLevels();

    if(lodcount == 0)
    {
        return;
    }

    std::vector<float> leafdata;

    hasLeafGeometry = false;

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_LeafGeometry);

    int corner = 0;
    std::vector<short> indices;

    for(unsigned int lod = 0; lod < lodcount; lod++)
    {
        const SGeometry::SLeaf& leaves = geo.m_pLeaves[lod];
        const int numleaves = leaves.m_nNumLeaves;

        if(numleaves == 0)
        {
            continue;
        }

        if(lod == 0)
        {
            leafRockScalar = leaves.m_fLeafRockScalar;
            leafRustleScalar = leaves.m_fLeafRustleScalar;
        }

        const int numSpeedWindAngles = wind->GetNumLeafAngleMatrices();

        Matrixf unit(0.0f, -0.5f, -0.5f, 0.0f,
                     0.0f,  0.5f, -0.5f, 0.0f,
                     0.0f,  0.5f,  0.5f, 0.0f,
                     0.0f, -0.5f,  0.5f, 0.0f);

        for(int leaf = 0; leaf < numleaves; leaf++)
        {
            const CSpeedTreeRT::SGeometry::SLeaf::SCard* card = leaves.m_pCards + leaves.m_pLeafCardIndices[leaf];

            if(!card)
            {
                continue;
            }

            hasLeafGeometry = true;

            for(corner = 0; corner < 4; corner++)
            {
                Vec3f pivotpt = Vec3f(0.0f, unit(corner, 1), unit(corner, 2));
                pivotpt.y() += card->m_afPivotPoint[0] - 0.5f;
                pivotpt.z() += card->m_afPivotPoint[1] - 0.5f;

                Vec3f cornerpt;
                cornerpt.x() = pivotpt.x() * card->m_fWidth;
                cornerpt.y() = pivotpt.y() * card->m_fWidth;
                cornerpt.z() = pivotpt.z() * card->m_fHeight;

                leafdata.push_back(cornerpt.x()); //0
                leafdata.push_back(cornerpt.y()); //4
                leafdata.push_back(cornerpt.z()); //8

                float x = -leaves.m_pNormals[leaf * 12 + corner * 3 + 0];
                float y = leaves.m_pNormals[leaf * 12 + corner * 3 + 1];
                float z = -leaves.m_pNormals[leaf * 12 + corner * 3 + 2];

                leafdata.push_back(x); //12
                leafdata.push_back(y); //16
                leafdata.push_back(z); //20

                float index1 = float(int(leaves.m_pWindMatrixIndices[0][leaf] * 10.0f / wind->GetNumWindMatrices()));
                float index2 = float(int(leaves.m_pWindMatrixIndices[1][leaf] * 10.0f / wind->GetNumWindMatrices()));

                float weight1 = leaves.m_pWindWeights[0][leaf];
                float weight2 = leaves.m_pWindWeights[1][leaf];

                leafdata.push_back(card->m_pTexCoords[corner * 2 + 0]); //24
                leafdata.push_back(card->m_pTexCoords[corner * 2 + 1]); //28

                leafdata.push_back(index1 + weight1); //32
                leafdata.push_back(index2 + weight2); //36

                x = -leaves.m_pCenterCoords[leaf * 3 + 0];
                y = leaves.m_pCenterCoords[leaf * 3 + 1];
                z = -leaves.m_pCenterCoords[leaf * 3 + 2];

                leafdata.push_back(x); //40
                leafdata.push_back(y); //44
                leafdata.push_back(z); //48

                leafdata.push_back(DEG_TO_RAD(card->m_afAngleOffsets[0])); //52
                leafdata.push_back(DEG_TO_RAD(card->m_afAngleOffsets[1])); //56
                leafdata.push_back(float(leaf % numSpeedWindAngles)); //60
                leafdata.push_back(leaves.m_pDimming[leaf]); //64

#if WANT_LEAF_NORMALS
                if(leaves.m_pTangents)
                {
                    leafdata.push_back(-leaves.m_pTangents[12 * leaf + 3 * corner + 0]); //68
                    leafdata.push_back(leaves.m_pTangents[12 * leaf + 3 * corner + 1]); //72
                    leafdata.push_back(-leaves.m_pTangents[12 * leaf + 3 * corner + 2]); //76
                }
                else
                {
                    leafdata.push_back(0.0f);
                    leafdata.push_back(0.0f);
                    leafdata.push_back(0.0f);
                }
#endif
            }
        }

#if WANT_LEAF_NORMALS
        static unsigned int stride = 80;
#else
        static unsigned int stride = 68;
#endif
        oglVertexBuffer* vbo = treeDataGetVBO();

        leafvboid[lod] = vbo->AddBuffer(&leafdata[0], leafdata.size() * sizeof(float), true);

        vbo->SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, 0, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, 12, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD0, 4, GL_FLOAT, stride, 24, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD1, 3, GL_FLOAT, stride, 40, leafvboid[lod]);
        vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD2, 4, GL_FLOAT, stride, 52, leafvboid[lod]);

#if WANT_LEAF_NORMALS
        vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD3, 3, GL_FLOAT, stride, 68, leafvboid[lod]);
#endif
        int offset = 0;

        for(int i = 0; i < leaves.m_nNumLeaves; i++)
        {
            indices.push_back(offset + 0);
            indices.push_back(offset + 1);
            indices.push_back(offset + 2);
            indices.push_back(offset + 0);
            indices.push_back(offset + 2);
            indices.push_back(offset + 3);

            offset += 4;
        }

        vbo->AddDrawElementsPrimitive(GL_TRIANGLES, indices.size(), &indices[0], leafvboid[lod]);
        indices.clear();
        leafdata.clear();
    }

    const float* m = CSpeedTreeRT::GetLeafMaterial();

    float scale = lightParams.m_fAmbientScalar;

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(m[0], m[1], m[2], 1.0f);
    material.ambient.set(m[3] * scale, m[4] * scale, m[5] * scale, 1.0f);
    material.specular.set(m[6], m[7], m[8], 1.0f);
    material.emissive.set(m[9], m[10], m[11], 1.0f);

    int id = oglCreateMaterial((void*)&material);

#if USE_STATESET
    leafmaterialON = new RenderStateSet;
    leafmaterialON->SetRenderState(new Material(id));
    leafmaterialON->SetRenderState(new Program("LeafShaderNormal"));
    leafmaterialON->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    leafmaterialON->SetMode(GL_LIGHTING, 0);
    leafmaterialON->SetMode(GL_ALPHA_TEST, 1);
 //   leafmaterialON->SetRenderState(new DepthFunc(GL_EQUAL));

    leafmaterialOFF = new RenderStateSet;
    leafmaterialOFF->SetRenderState(new Material(id));
    leafmaterialOFF->SetRenderState(new Program("LeafShader"));
    leafmaterialOFF->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    leafmaterialOFF->SetMode(GL_LIGHTING, 0);
    leafmaterialOFF->SetMode(GL_ALPHA_TEST, 1);
 //   leafmaterialOFF->SetRenderState(new DepthFunc(GL_EQUAL));
#else
    leafmaterialON->SetMaterial(id);
    leafmaterialON->SetShader("LeafShaderNormal");

    if(leafmaterialOFF.valid())
    {
        leafmaterialOFF->SetMaterial(id);
        leafmaterialOFF->SetShader("LeafShader");
    }
#endif
}

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetupBillboardGeometry
Description:
------------------------------------------------------------------------*/
void CTree::SetupBillboardGeometry(int& texCoordsOffset, std::vector<float>& texcoords)
{
    SetLodLevel(0.0f);

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_BillboardGeometry);

    UpdateBillboardGeometry(geo);

    int i = 0;

    const CSpeedTreeRT::SLightShaderParams& scales = lightParams;

    for(i = 0; i < 4; i++)
    {
        billboard.data[14 * i + 0] = 0.0f;
        billboard.data[14 * i + 1] = 0.0f;
        billboard.data[14 * i + 2] = 0.0f;
        billboard.data[14 * i + 3] = (float)i;

#if(1)
        billboard.data[14 * i + 4] = geo.m_s360Billboard.m_fWidth;
        billboard.data[14 * i + 5] = -geo.m_s360Billboard.m_fHeight;
        billboard.data[14 * i + 6] = 0.0f;
        billboard.data[14 * i + 7] = 0.0f;
#else
        billboard.data[14 * i + 4] = geo.m_s360Billboard.m_fWidth;
        billboard.data[14 * i + 5] = geo.m_s360Billboard.m_fHeight;
        billboard.data[14 * i + 6] = 0.0f;
        billboard.data[14 * i + 7] = 0.0f;
#endif

        billboard.data[14 * i + 8] = (float)texCoordsOffset;
        billboard.data[14 * i + 9] = (float)geo.m_s360Billboard.m_nNumImages;
        billboard.data[14 * i + 10] = (float)geo.m_s360Billboard.m_fTransitionPercent;

        billboard.data[14 * i + 11] = scales.m_fBillboardBrightSideLightScalar * scales.m_fGlobalLightScalar;
        billboard.data[14 * i + 12] = scales.m_fBillboardDarkSideLightScalar * scales.m_fGlobalLightScalar;
        billboard.data[14 * i + 13] = scales.m_fBillboardAmbientScalar;
    }

    if(geo.m_sHorzBillboard.m_pCoords)
    {
        hasHorizBillboardGeom = true;
    }

    if(hasHorizBillboardGeom)
    {
        for(i = 0; i < 4; i++)
        {
            horizBillboard.data[15 * i + 0] = -geo.m_sHorzBillboard.m_pCoords[3 * i + 0];
            horizBillboard.data[15 * i + 1] = geo.m_sHorzBillboard.m_pCoords[3 * i + 1];
            horizBillboard.data[15 * i + 2] = -geo.m_sHorzBillboard.m_pCoords[3 * i + 2];
            horizBillboard.data[15 * i + 3] = 0.0f; // Fade

            horizBillboard.data[15 * i + 4] = geo.m_sHorzBillboard.m_afTexCoords[2 * i + 0];
            horizBillboard.data[15 * i + 5] = geo.m_sHorzBillboard.m_afTexCoords[2 * i + 1];

            horizBillboard.data[15 * i + 6] = scales.m_fBillboardBrightSideLightScalar * scales.m_fGlobalLightScalar;
            horizBillboard.data[15 * i + 7] = scales.m_fBillboardDarkSideLightScalar * scales.m_fGlobalLightScalar;
            horizBillboard.data[15 * i + 8] = scales.m_fBillboardAmbientScalar * 0.5f; // 0.5 hack.

            // Rotate x, y, z
            horizBillboard.data[15 * i + 9] = 0.0f;
            horizBillboard.data[15 * i + 10] = 0.0f;
            horizBillboard.data[15 * i + 11] = 0.0f;

            // Position x, y, z
            horizBillboard.data[15 * i + 12] = 0.0f;
            horizBillboard.data[15 * i + 13] = 0.0f;
            horizBillboard.data[15 * i + 14] = 0.0f;
        }
    }

    for(i = 0; i < geo.m_s360Billboard.m_nNumImages; i++)
    {
        // Max u, v and width u, v
        float x = geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 0];
        float y = geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 1];
        float z = x - geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 4];
        float w = y - geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 5];

        texcoords.push_back(x);
        texcoords.push_back(y);
        texcoords.push_back(z);
        texcoords.push_back(w);
    }

    texCoordsOffset += geo.m_s360Billboard.m_nNumImages;

    billboardmaterialON->SetRenderState(new Material(-1));
    billboardmaterialON->SetRenderState(new Program("BillboardShaderNormal"));
    billboardmaterialON->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    billboardmaterialON->SetRenderState(new FrontFace(GL_CW));

    if(billboardmaterialOFF.valid())
    {
        billboardmaterialOFF->SetRenderState(new Material(-1));
        billboardmaterialOFF->SetRenderState(new Program("BillboardShader"));
        billboardmaterialOFF->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
        billboardmaterialOFF->SetRenderState(new FrontFace(GL_CW));
    }

    horizbillboardmaterialON->SetRenderState(new Material(-1));
    horizbillboardmaterialON->SetRenderState(new Program("HorizShaderNormal"));
    horizbillboardmaterialON->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));

    if(horizbillboardmaterialOFF.valid())
    {
        horizbillboardmaterialOFF->SetRenderState(new Material(-1));
        horizbillboardmaterialOFF->SetRenderState(new Program("HorizShader"));
        horizbillboardmaterialOFF->SetRenderState(new AlphaFunc(GL_GREATER, 84.0f * colorONEOVER255));
    }
}
#else
/*------------------------------------------------------------------------
Function: SetupBillboardGeometry
Description:
------------------------------------------------------------------------*/
void CTree::SetupBillboardGeometry(int& texCoordsOffset, std::vector<float>& texcoords)
{
    SetLodLevel(0.0f);

    CSpeedTreeRT::SGeometry geo;
    GetGeometry(geo, SpeedTree_BillboardGeometry);

    UpdateBillboardGeometry(geo);

    int i = 0;

    const CSpeedTreeRT::SLightShaderParams& scales = lightParams;

    for(i = 0; i < 4; i++)
    {
        billboard.data[14 * i + 0] = 0.0f;
        billboard.data[14 * i + 1] = 0.0f;
        billboard.data[14 * i + 2] = 0.0f;
        billboard.data[14 * i + 3] = (float)i;

#if(1)
        billboard.data[14 * i + 4] = geo.m_s360Billboard.m_fWidth;
        billboard.data[14 * i + 5] = -geo.m_s360Billboard.m_fHeight;
        billboard.data[14 * i + 6] = 0.0f;
        billboard.data[14 * i + 7] = 0.0f;
#else
        billboard.data[14 * i + 4] = geo.m_s360Billboard.m_fWidth;
        billboard.data[14 * i + 5] = geo.m_s360Billboard.m_fHeight;
        billboard.data[14 * i + 6] = 0.0f;
        billboard.data[14 * i + 7] = 0.0f;
#endif

        billboard.data[14 * i + 8] = (float)texCoordsOffset;
        billboard.data[14 * i + 9] = (float)geo.m_s360Billboard.m_nNumImages;
        billboard.data[14 * i + 10] = (float)geo.m_s360Billboard.m_fTransitionPercent;

        billboard.data[14 * i + 11] = scales.m_fBillboardBrightSideLightScalar * scales.m_fGlobalLightScalar;
        billboard.data[14 * i + 12] = scales.m_fBillboardDarkSideLightScalar * scales.m_fGlobalLightScalar;
        billboard.data[14 * i + 13] = scales.m_fBillboardAmbientScalar;
    }

    if(geo.m_sHorzBillboard.m_pCoords)
    {
        hasHorizBillboardGeom = true;
    }

    if(hasHorizBillboardGeom)
    {
        for(i = 0; i < 4; i++)
        {
            horizBillboard.data[15 * i + 0] = -geo.m_sHorzBillboard.m_pCoords[3 * i + 0];
            horizBillboard.data[15 * i + 1] = geo.m_sHorzBillboard.m_pCoords[3 * i + 1];
            horizBillboard.data[15 * i + 2] = -geo.m_sHorzBillboard.m_pCoords[3 * i + 2];
            horizBillboard.data[15 * i + 3] = 0.0f; // Fade

            horizBillboard.data[15 * i + 4] = geo.m_sHorzBillboard.m_afTexCoords[2 * i + 0];
            horizBillboard.data[15 * i + 5] = geo.m_sHorzBillboard.m_afTexCoords[2 * i + 1];

            horizBillboard.data[15 * i + 6] = scales.m_fBillboardBrightSideLightScalar * scales.m_fGlobalLightScalar;
            horizBillboard.data[15 * i + 7] = scales.m_fBillboardDarkSideLightScalar * scales.m_fGlobalLightScalar;
            horizBillboard.data[15 * i + 8] = scales.m_fBillboardAmbientScalar * 0.5f; // 0.5 hack.

            // Rotate x, y, z
            horizBillboard.data[15 * i + 9] = 0.0f;
            horizBillboard.data[15 * i + 10] = 0.0f;
            horizBillboard.data[15 * i + 11] = 0.0f;

            // Position x, y, z
            horizBillboard.data[15 * i + 12] = 0.0f;
            horizBillboard.data[15 * i + 13] = 0.0f;
            horizBillboard.data[15 * i + 14] = 0.0f;
        }
    }

    for(i = 0; i < geo.m_s360Billboard.m_nNumImages; i++)
    {
        // Max u, v and width u, v
        float x = geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 0];
        float y = geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 1];
        float z = x - geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 4];
        float w = y - geo.m_s360Billboard.m_pTexCoordTable[i * 8 + 5];

        texcoords.push_back(x);
        texcoords.push_back(y);
        texcoords.push_back(z);
        texcoords.push_back(w);
    }

    texCoordsOffset += geo.m_s360Billboard.m_nNumImages;

    billboardmaterialON->SetMaterial(-1);
    billboardmaterialON->SetShader("BillboardShaderNormal");

    if(billboardmaterialOFF.valid())
    {
        billboardmaterialOFF->SetMaterial(-1);
        billboardmaterialOFF->SetShader("BillboardShader");
    }

    horizbillboardmaterialON->SetMaterial(-1);
    horizbillboardmaterialON->SetShader("HorizShaderNormal");

    if(horizbillboardmaterialOFF.valid())
    {
        horizbillboardmaterialOFF->SetMaterial(-1);
        horizbillboardmaterialOFF->SetShader("HorizShader");
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetBranchDiffuseScale
Description:
------------------------------------------------------------------------*/
void CTree::SetBranchDiffuseScale()
{
    Program* pgm = dynamic_cast<Program*>(branchmaterial->GetRenderState(RS_GLSLPROGRAM));

    if(pgm)
    {
        glslProgram* p = pgm->GetProgram();

        if(p)
        {
            glslUniform* u = p->GetUniform("diffuseScale");

            if(u)
            {
                u->Set(GetBranchDiffuseScale());
                p->Apply(*u);
            }
        }
    }
}
#else
/*------------------------------------------------------------------------
Function: SetBranchDiffuseScale
Description:
------------------------------------------------------------------------*/
void CTree::SetBranchDiffuseScale()
{
    glslUniform* u = branchmaterial->GetProgram()->GetUniform("diffuseScale");

    if(u)
    {
        u->Set(GetBranchDiffuseScale());
        branchmaterial->GetProgram()->Apply(*u);
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetFrondDiffuseScale
Description:
------------------------------------------------------------------------*/
void CTree::SetFrondDiffuseScale()
{
    Program* pgm = dynamic_cast<Program*>(frondmaterial->GetRenderState(RS_GLSLPROGRAM));
    glslProgram* p = pgm->GetProgram();

    glslUniform* u = p->GetUniform("diffuseScale");

    if(u)
    {
        u->Set(GetFrondDiffuseScale());
        p->Apply(*u);
    }
}
#else
/*------------------------------------------------------------------------
Function: SetFrondDiffuseScale
Description:
------------------------------------------------------------------------*/
void CTree::SetFrondDiffuseScale()
{
    glslUniform* u = frondmaterial->GetProgram()->GetUniform("diffuseScale");

    if(u)
    {
        u->Set(GetFrondDiffuseScale());
        frondmaterial->GetProgram()->Apply(*u);
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetLeafDiffuseScale
Description:
------------------------------------------------------------------------*/
void CTree::SetLeafDiffuseScale()
{
    Program* pgm = dynamic_cast<Program*>(leafmaterial->GetRenderState(RS_GLSLPROGRAM));
    glslProgram* p = pgm->GetProgram();

    glslUniform* u = p->GetUniform("diffuseScale");

    if(u)
    {
        u->Set(GetLeafDiffuseScale());
        p->Apply(*u);
    }
}
#else
/*------------------------------------------------------------------------
Function: SetLeafDiffuseScale
Description:
------------------------------------------------------------------------*/
void CTree::SetLeafDiffuseScale()
{
    glslUniform* u = leafmaterial->GetProgram()->GetUniform("diffuseScale");

    if(u)
    {
        u->Set(GetLeafDiffuseScale());
        leafmaterial->GetProgram()->Apply(*u);
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetLeafCameraAngle
Description:
------------------------------------------------------------------------*/
void CTree::SetLeafCameraAngle(const float& pitch, const float& azimuth)
{
    Program* pgm = dynamic_cast<Program*>(leafmaterial->GetRenderState(RS_GLSLPROGRAM));
    glslProgram* p = pgm->GetProgram();

    glslUniform* u = p->GetUniform("cameraangle");

    if(u)
    {
        u->Set(Vec2f(pitch, azimuth));
        p->Apply(*u);
    }
}
#else
/*------------------------------------------------------------------------
Function: SetLeafCameraAngle
Description:
------------------------------------------------------------------------*/
void CTree::SetLeafCameraAngle(const float& pitch, const float& azimuth)
{
    glslUniform* u = leafmaterial->GetProgram()->GetUniform("cameraangle");

    if(u)
    {
        u->Set(Vec2f(pitch, azimuth));
        leafmaterial->GetProgram()->Apply(*u);
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetBranchParamBlock
Description:
------------------------------------------------------------------------*/
void CTree::SetBranchParamBlock(const Vec3f& position, const Vec4f& rotation)
{
    Program* pgm = dynamic_cast<Program*>(branchmaterial->GetRenderState(RS_GLSLPROGRAM));

    if(pgm)
    {
        glslProgram* p = pgm->GetProgram();

        if(p)
        {
            glslUniform* u = p->GetUniform("treepos");

            if(u)
            {
                u->Set(position);
                p->Apply(*u);
            }

            u = p->GetUniform("rotation");

            if(u)
            {
                u->Set(rotation);
                p->Apply(*u);
            }
        }
    }
}
#else
/*------------------------------------------------------------------------
Function: SetBranchParamBlock
Description:
------------------------------------------------------------------------*/
void CTree::SetBranchParamBlock(const Vec3f& position, const Vec4f& rotation)
{
    glslUniform* u = branchmaterial->GetProgram()->GetUniform("treepos");

    if(u)
    {
        u->Set(position);
        branchmaterial->GetProgram()->Apply(*u);
    }

    u = branchmaterial->GetProgram()->GetUniform("rotation");

    if(u)
    {
        u->Set(rotation);
        branchmaterial->GetProgram()->Apply(*u);
    }
}
#endif

/*------------------------------------------------------------------------
Function: SetAlphaRef
Description:
------------------------------------------------------------------------*/
void CTree::SetAlphaRef(float alpharef, int type)
{
#if USE_STATESET
    AlphaFunc* af = nullptr;

    if(type == TREE_BRANCHES)
        af = dynamic_cast<AlphaFunc*>(branchmaterial->GetRenderState(RS_ALPHAFUNC));
    else if(type == TREE_FRONDS)
        af = dynamic_cast<AlphaFunc*>(frondmaterial->GetRenderState(RS_ALPHAFUNC));
    else
        af = dynamic_cast<AlphaFunc*>(leafmaterial->GetRenderState(RS_ALPHAFUNC));

    if(af)
    {
        af->SetRefValue(alpharef * colorONEOVER255);
        af->apply();
    }
#else

    glslProgram* pgm = nullptr;

    if(type == TREE_BRANCHES)
        pgm = branchmaterial->GetProgram();

    if(type == TREE_FRONDS)
        pgm = frondmaterial->GetProgram();

    if(type == TREE_LEAVES)
        pgm = leafmaterial->GetProgram();

    if(type == TREE_BILLBOARDS)
        pgm = billboardmaterial->GetProgram();

    if(type == HORIZ_BILLBOARDS)
        pgm = horizbillboardmaterial->GetProgram();

    if(pgm)
    {
        glslUniform* u = pgm->GetUniform("alphatest");

        if(u)
        {
            u->Set(alpharef * colorONEOVER255);
            pgm->Apply(*u);
        }
    }
#endif
}

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetBranchParamBlock
Description:
------------------------------------------------------------------------*/
void CTree::SetLeafParamBlock(const Vec3f& position, const Vec4f& rotation)
{
    Program* pgm = dynamic_cast<Program*>(leafmaterial->GetRenderState(RS_GLSLPROGRAM));
    glslProgram* p = pgm->GetProgram();

    glslUniform* u = p->GetUniform("treepos");

    if(u)
    {
        u->Set(position);
        p->Apply(*u);
    }

    u = p->GetUniform("rotation");

    if(u)
    {
        u->Set(rotation);
        p->Apply(*u);
    }
}
#else
/*------------------------------------------------------------------------
Function: SetBranchParamBlock
Description:
------------------------------------------------------------------------*/
void CTree::SetLeafParamBlock(const Vec3f& position, const Vec4f& rotation)
{
    glslUniform* u = leafmaterial->GetProgram()->GetUniform("treepos");

    if(u)
    {
        u->Set(position);
        leafmaterial->GetProgram()->Apply(*u);
    }

    u = leafmaterial->GetProgram()->GetUniform("rotation");

    if(u)
    {
        u->Set(rotation);
        leafmaterial->GetProgram()->Apply(*u);
    }
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------------
Function: SetFrondParamBlock
Description:
------------------------------------------------------------------------*/
void CTree::SetFrondParamBlock(const Vec3f& position, const Vec4f& rotation)
{
    Program* pgm = dynamic_cast<Program*>(frondmaterial->GetRenderState(RS_GLSLPROGRAM));
    glslProgram* p = pgm->GetProgram();

    glslUniform* u = p->GetUniform("treepos");

    if(u)
    {
        u->Set(position);
        p->Apply(*u);
    }

    u = p->GetUniform("rotation");

    if(u)
    {
        u->Set(rotation);
        p->Apply(*u);
    }
}
#else
/*------------------------------------------------------------------------
Function: SetFrondParamBlock
Description:
------------------------------------------------------------------------*/
void CTree::SetFrondParamBlock(const Vec3f& position, const Vec4f& rotation)
{
    glslUniform* u = frondmaterial->GetProgram()->GetUniform("treepos");

    if(u)
    {
        u->Set(position);
        frondmaterial->GetProgram()->Apply(*u);
    }

    u = frondmaterial->GetProgram()->GetUniform("rotation");

    if(u)
    {
        u->Set(rotation);
        frondmaterial->GetProgram()->Apply(*u);
    }
}
#endif