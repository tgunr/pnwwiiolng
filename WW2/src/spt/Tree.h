#pragma once
//////////////////////////////////////////////////////////////////////
//	SpeedTreeRTExample Class
//
//	(c) 2003 IDV, Inc.
//
//	This class is provided to illustrate one way to incorporate
//	SpeedTreeRT into an OpenGL application.  All of the SpeedTreeRT
//	calls that must be made on a per tree basis are done by this class.
//	Calls that apply to all trees (i.e. static SpeedTreeRT functions)
//	are made in the functions in main.cpp.
//
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com
//

//////////////////////////////////////////////////////////////////////
//	Include Files

#define SPEEDTREE_DATA_FORMAT_OPENGL
#include "spt/SpeedTreeRT.h"
#include "ogl/oglVertexBuffer.h"
#include "spt/sptmain.h"
#include "treeData.h"
#include "math/Matrixf.h"
#include "osgImport/ref_ptr.h"
#include "osgImport/Texture2D.h"
#include "shader/glsl.h"
#include "physics/colliderQuery.h"


#include <string>

#define MAX_LODS    4

class oglMaterial;

#if USE_STATESET
class RenderStateSet;
#endif

struct HorizontalBillboard
{
    float data[60];

    inline void update(float*& copy, Vec3f& p, Vec4f& rot, float fade)
    {
        /*--------------------------------
        Corner 1
        ---------------------------------*/
        // data[0] = object space x
        // data[1] = object space y
        // data[2] = object space z
        *(copy + 0) = data[0];
        *(copy + 1) = data[1];
        *(copy + 2) = data[2];
        *(copy + 3) = fade;

        //data[4] = tex coord
        //data[5] = tex coord
        *(copy + 4) = data[4];
        *(copy + 5) = data[5];

        //data[6] = lightscale.x
        //data[7] = lightscale.y
        //data[8] = lightscale.z
        *(copy + 6) = data[6];
        *(copy + 7) = data[7];
        *(copy + 8) = data[8];

        *(copy + 9) = rot.x();
        *(copy + 10) = rot.y();
        *(copy + 11) = rot.z();

        *(copy + 12) = p.x();
        *(copy + 13) = p.y();
        *(copy + 14) = p.z();
        /*--------------------------------
        Corner 2
        ---------------------------------*/
        // data[15] = object space x
        // data[16] = object space y
        // data[17] = object space z
        *(copy + 15) = data[15];
        *(copy + 16) = data[16];
        *(copy + 17) = data[17];
        *(copy + 18) = fade;

        //data[19] = tex coord
        //data[20] = tex coord
        *(copy + 19) = data[19];
        *(copy + 20) = data[20];

        //data[21] = lightscale.x
        //data[22] = lightscale.y
        //data[23] = lightscale.z
        *(copy + 21) = data[21];
        *(copy + 22) = data[22];
        *(copy + 23) = data[23];

        *(copy + 24) = rot.x();
        *(copy + 25) = rot.y();
        *(copy + 26) = rot.z();

        *(copy + 27) = p.x();
        *(copy + 28) = p.y();
        *(copy + 29) = p.z();
        /*--------------------------------
        Corner 3
        ---------------------------------*/
        // data[30] = object space x
        // data[31] = object space y
        // data[32] = object space z
        *(copy + 30) = data[30];
        *(copy + 31) = data[31];
        *(copy + 32) = data[32];
        *(copy + 33) = fade;

        //data[34] = tex coord
        //data[35] = tex coord
        *(copy + 34) = data[34];
        *(copy + 35) = data[35];

        //data[36] = lightscale.x
        //data[37] = lightscale.y
        //data[38] = lightscale.z
        *(copy + 36) = data[36];
        *(copy + 37) = data[37];
        *(copy + 38) = data[38];

        *(copy + 39) = rot.x();
        *(copy + 40) = rot.y();
        *(copy + 41) = rot.z();

        *(copy + 42) = p.x();
        *(copy + 43) = p.y();
        *(copy + 44) = p.z();
        /*--------------------------------
        Corner 4
        ---------------------------------*/
        // data[45] = object space x
        // data[46] = object space y
        // data[47] = object space z
        *(copy + 45) = data[45];
        *(copy + 46) = data[46];
        *(copy + 47) = data[47];
        *(copy + 48) = fade;

        //data[49] = tex coord
        //data[50] = tex coord
        *(copy + 49) = data[49];
        *(copy + 50) = data[50];

        //data[51] = lightscale.x
        //data[52] = lightscale.y
        //data[53] = lightscale.z
        *(copy + 51) = data[51];
        *(copy + 52) = data[52];
        *(copy + 53) = data[53];

        *(copy + 54) = rot.x();
        *(copy + 55) = rot.y();
        *(copy + 56) = rot.z();

        *(copy + 57) = p.x();
        *(copy + 58) = p.y();
        *(copy + 59) = p.z();
    }
};

struct Billboard
{
    float data[56];

    inline void update(float*& copy, Vec3f& p, float azimuth, float fade, float scale)
    {
        /*--------------------------------
        Corner 1
        ---------------------------------*/
        *(copy + 0) = p.x();
        *(copy + 1) = p.y();
        *(copy + 2) = p.z();
        *(copy + 3) = data[3]; // billboard corner

        //--------------------------tex0-----------------------*/
        *(copy + 4) = data[4]; // width
        *(copy + 5) = data[5]; // height
        *(copy + 6) = azimuth;
        *(copy + 7) = fade;

         //--------------------------tex1-----------------------*/
        *(copy + 8) = data[8]; // texcoord offset
        *(copy + 9) = data[9]; // num images
        *(copy + 10) = data[10]; // transition percent

        //--------------------------tex2-----------------------*/
        *(copy + 11) = data[11]; // bright scalar * global light scalar
        *(copy + 12) = data[12]; // dark scalar * global light scalar
        *(copy + 13) = data[13]; // ambient scalar

        /*--------------------------------
        Corner 2
        ---------------------------------*/
        *(copy + 14) = p.x();
        *(copy + 15) = p.y();
        *(copy + 16) = p.z();
        *(copy + 17) = data[17]; // corner

        //--------------------------tex0-----------------------*/
        *(copy + 18) = data[18]; // width
        *(copy + 19) = data[19]; // height
        *(copy + 20) = azimuth;
        *(copy + 21) = fade;

         //--------------------------tex1-----------------------*/
        *(copy + 22) = data[22]; // texcoord offset
        *(copy + 23) = data[23]; // num images
        *(copy + 24) = data[24]; // transition percent

        //--------------------------tex2-----------------------*/
        *(copy + 25) = data[25]; // bright scalar * global light scalar
        *(copy + 26) = data[26]; // dark scalar * global light scalar
        *(copy + 27) = data[27]; // ambient scalar

        /*--------------------------------
        Corner 3
        ---------------------------------*/
        *(copy + 28) = p.x();
        *(copy + 29) = p.y();
        *(copy + 30) = p.z();
        *(copy + 31) = data[31]; // corner

        //--------------------------tex0-----------------------*/
        *(copy + 32) = data[32]; // width
        *(copy + 33) = data[33]; // height
        *(copy + 34) = azimuth;
        *(copy + 35) = fade;

         //--------------------------tex1-----------------------*/
        *(copy + 36) = data[36]; // texcoord offset
        *(copy + 37) = data[37]; // num images
        *(copy + 38) = data[38]; // transition percent

        //--------------------------tex2-----------------------*/
        *(copy + 39) = data[39]; // bright scalar * global light scalar
        *(copy + 40) = data[40]; // dark scalar * global light scalar
        *(copy + 41) = data[41]; // ambient scalar

        /*--------------------------------
        Corner 4
        ---------------------------------*/
        *(copy + 42) = p.x();
        *(copy + 43) = p.y();
        *(copy + 44) = p.z();
        *(copy + 45) = data[45]; // corner

        //--------------------------tex0-----------------------*/
        *(copy + 46) = data[46]; // width
        *(copy + 47) = data[47]; // height
        *(copy + 48) = azimuth;
        *(copy + 49) = fade;

         //--------------------------tex1-----------------------*/
        *(copy + 50) = data[50]; // texcoord offset
        *(copy + 51) = data[51]; // num images
        *(copy + 52) = data[52]; // transition percent

        //--------------------------tex2-----------------------*/
        *(copy + 53) = data[53]; // bright scalar * global light scalar
        *(copy + 54) = data[54]; // dark scalar * global light scalar
        *(copy + 55) = data[55]; // ambient scalar
    }
};

class CTree : public CSpeedTreeRT
{
public:
    enum
    {
        TREE_BRANCHES,
        TREE_FRONDS,
        TREE_LEAVES
    };

    CTree();
    virtual	~CTree();

    void SetLeafTextureCoords(UINT32 nLeafMapIndex, const FLOAT32* pTexCoords);
    void SetNewFrame();

    bool LoadSPTFile(const char* filename,
                     unsigned int seed,
                     float n,
                     float f,
                     CSpeedWind* wind);

    bool LoadBranchTexture();
    bool LoadLeafFrondTexture();
    bool LoadBillboardTexture();

    int PushBranches();
    int PushFronds();
    int PushLeaves();
    int DrawBranchesDeprecated(Tree* tree);
    int DrawFrondsDeprecated(Tree* treee);
    int DrawLeavesDeprecated(Tree* tree, int whichlod);
    int DrawBillboard(Tree* tree);

    int WriteToVideoMemory(std::vector<FLOAT32>& v);

    float GetBranchDiffuseScale() {return lightParams.m_fBranchLightScalar * lightParams.m_fGlobalLightScalar;}
    float GetFrondDiffuseScale() {return lightParams.m_fFrondLightScalar * lightParams.m_fGlobalLightScalar;}
    float GetLeafDiffuseScale() {return lightParams.m_fLeafLightScalar * lightParams.m_fGlobalLightScalar;}
    float GetRockScalar() {return leafRockScalar;}
    float GetRustleScalar() {return leafRustleScalar;}
    float GetScale() {return scale;}

    void StartBillboardType();
    void EndBillboardType();

    void StartHorizBillboardType();
    void EndHorizBillboardType();

    void SetTreeParamBlock(const Vec3f& pos, const Vec4f& rot);
    void SetBranchDiffuseScale();

    void SetFrondDiffuseScale();

    void SetLeafDiffuseScale();
    void SetLeafCameraAngle(const float& pitch, const float& azimuth);

#if USE_STATESET
    void SetAlphaRef(float alpharef, int type);
#endif
    void SetBranchParamBlock(const Vec3f& position, const Vec4f& rotation);
    void SetLeafParamBlock(const Vec3f& position, const Vec4f& rotation);
    void SetFrondParamBlock(const Vec3f& position, const Vec4f& rotation);
    void AddCollisionGeometryObject(dGeomID geom)
    {
        collidegeom.push_back(geom);
    }

    void EndBranchType();
    void EndLeafType();
    void EndFrondType();
    void GetLeafTables(const FLOAT32*& pTable, UINT32& uEntryCount);
    void StartBranchType();
    void StartLeafType();
    void StartFrondType();
    void UpdateBillboardCoords();
    void SetupBillboardGeometry(int& texCoordsOffset, std::vector<float>& texcoords);
    void SetupBranchGeometryDeprecated();
    void SetupBranchGeometry();
    void SetupFrondGeometryDeprecated();
    void SetupFrondGeometry();
    void SetupLeafCardGeometryDeprecated();
    void SetupLeafCardGeometry();
    void GetBoundingBox(float box[6]);
    void SelectShader();
    void AddBranchDraw(const Tree* tree, const Vec4f& pos, const Vec4f& rot);
    void AddFrondDraw(const Tree* tree, const Vec4f& pos, const Vec4f& rot);
    void AddLeafDraw(const Tree* tree, const Vec4f& pos, const Vec4f& rot, int whichlod);

    Billboard& GetBillboard() {return billboard;}
    HorizontalBillboard& GetHorizontalBillboard() {return horizBillboard;}

    static void SetTreeDirectory(std::string path) {treedir = path;}
    static const std::string& GetTreeDirectory() {return treedir;}

    bool HasBranchGeometry() {return hasBranchGeometry;}
    bool HasFrondGeometry() {return hasFrondGeometry;}
    bool HasLeafGeometry() {return hasLeafGeometry;}
    bool HasHorizBillboardGeometry() {return hasHorizBillboardGeom;}

    bool IsCollidable() {return !collidegeom.empty();}

    dGeomID GetCollideGeom(int i = 0) {return collidegeom[i];}

    std::vector<Matrixf> euler;

protected:
    bool leafRockingState;
    bool hasHorizBillboardGeom;
    bool hasBranchGeometry;
    bool hasFrondGeometry;
    bool hasLeafGeometry;

    float farLodDistanceSqr;
    float farLodDistance;
    float nearLodDistance;
    float nearLodDistanceSqr;
    float leafRockScalar;
    float leafRustleScalar;
    float scale;

    int branchvboid;
    int frondvboid;
    int leafvboid[8];
    int BranchCount[MAX_LODS];
    int FrondCount[MAX_LODS];
    int LeafCount[MAX_LODS];

    std::vector<dGeomID> collidegeom;

    std::vector<Vec4f> data[MAX_LODS];

#if USE_STATESET
    ref_ptr<RenderStateSet> branchmaterialON;
    ref_ptr<RenderStateSet> branchmaterialOFF;
    ref_ptr<RenderStateSet> branchdepth;
    ref_ptr<RenderStateSet> frondmaterialOFF;
    ref_ptr<RenderStateSet> fronddepth;
    ref_ptr<RenderStateSet> leafmaterialON;
    ref_ptr<RenderStateSet> leafmaterialOFF;
    ref_ptr<RenderStateSet> leafdepth;
    ref_ptr<RenderStateSet> branchmaterial;
    ref_ptr<RenderStateSet> frondmaterial;
    ref_ptr<RenderStateSet> leafmaterial;
    ref_ptr<RenderStateSet> billboardmaterial;
    ref_ptr<RenderStateSet> billboardmaterialON;
    ref_ptr<RenderStateSet> billboardmaterialOFF;
    ref_ptr<RenderStateSet> horizbillboardmaterial;
    ref_ptr<RenderStateSet> horizbillboardmaterialON;
    ref_ptr<RenderStateSet> horizbillboardmaterialOFF;
#else
    ref_ptr<oglMaterial> branchmaterialON;
    ref_ptr<oglMaterial> branchmaterialOFF;
    ref_ptr<oglMaterial> branchdepth;
    ref_ptr<oglMaterial> frondmaterialOFF;
    ref_ptr<oglMaterial> fronddepth;
    ref_ptr<oglMaterial> leafmaterialON;
    ref_ptr<oglMaterial> leafmaterialOFF;
    ref_ptr<oglMaterial> leafdepth;
    ref_ptr<oglMaterial> branchmaterial;
    ref_ptr<oglMaterial> frondmaterial;
    ref_ptr<oglMaterial> leafmaterial;
    ref_ptr<oglMaterial> billboardmaterial;
    ref_ptr<oglMaterial> billboardmaterialON;
    ref_ptr<oglMaterial> billboardmaterialOFF;
    ref_ptr<oglMaterial> horizbillboardmaterial;
    ref_ptr<oglMaterial> horizbillboardmaterialON;
    ref_ptr<oglMaterial> horizbillboardmaterialOFF;
#endif


    static std::string treedir;

    std::string treename;
    CSpeedTreeRT::SLightShaderParams lightParams;

    CSpeedWind* wind;

    int BranchMaps[CSpeedTreeRT::TL_NUM_TEX_LAYERS]; // texture for each layer
    int BillboardMaps[CSpeedTreeRT::TL_NUM_TEX_LAYERS];
    int LeafFrondMaps[CSpeedTreeRT::TL_NUM_TEX_LAYERS];

    struct Billboard billboard;
    struct HorizontalBillboard horizBillboard;
};
