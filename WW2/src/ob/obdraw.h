#pragma once

#define RENDER_SCENE        0x0001
#define RENDER_REFLECTION   0x0002
#define RENDER_SHADOW_MAP   0x0004
#define RENDER_FINAL_SHADOW 0x0008
#define RENDER_SCENE_DEPTH  0x0010

#define OBDRAW_UNSORTED_EXTERNAL_DRAW		1
#define OBDRAW_SORTED_EXTERNAL_DRAW			2
#define OBDRAW_UNSORTED_INTERNAL_DRAW		3
#define OBDRAW_SORTED_INTERNAL_DRAW			4

class Matrixf;

using USERDRAWFUNC = LOGICAL(*)(LOGICAL IsBeforeStage);

extern void obdrawWireframeObject(OBJECT* lpobj, UINT32 color);
extern void obdrawObjectBoundingBox(OBJECT* lpobj, UINT32 color);
extern LOGICAL obdrawDrawSingleObject(OBJECT* lpobj);
extern LOGICAL obdrawInit();
extern void obdrawKill();
extern void obdrawObjects();
extern LOGICAL obdrawObjectsDeprecated();
extern LOGICAL obdrawUpdateObjectLOD();
extern LOGICAL obdrawStraightAhead(piVECTOR* pos, piVECTOR* rot, float dist, UINT32 color);
extern LOGICAL obdrawProcessLODCallback(OBJECT* lplod, UINT32 user,piVECTOR* InPos,
								 LOGICAL ReLight,FLOAT32 Radius,LOGICAL * LODChanged);
extern UINT32 obdrawDrawTree(OBJECT*);

extern UINT32 obdrawDrawTreeSiblings(OBJECT*);
extern LOGICAL obdrawInstallDrawCallback(USERDRAWFUNC FuncPtr,UINT32 FuncNum);
extern LOGICAL obdrawUpdateTerrainObjectLighting(LOGICAL Override);

extern void obdrawSetDraw(SINT32 iType, LOGICAL bOn);
extern void obdrawObjectTriangles(LOGICAL draw);
extern void obdrawDrawBoundBox(LOGICAL draw);
extern void obdrawDrawTOBoundBox(LOGICAL draw);
extern void obdrawObjectNormals(LOGICAL draw);
extern void obdrawCollisionNormals(LOGICAL draw);
extern void obdrawSetOptimize(LOGICAL optimize);
extern void obdrawDrawWaterReflection();
extern void obdrawDrawWaterRefraction();

extern void obdrawDrawObject(const OBJECT* o, int units = 0);
extern void obdrawDrawObjectInstanced(const OBJECT* o, int units = 0, int primcount = 1);
extern void obdrawDrawAdvertisements();
extern void obdrawDrawUnsorted();
extern void obdrawDrawSorted();
extern void obdrawDrawSortedCockpit();
extern void obdrawDrawUnsortedCockpit();
extern void obdrawDrawMultitextured();
extern void obdrawDrawWater();
extern void obdrawSetRenderPass(unsigned int type);
extern void obdrawDrawUnsortedExternalCallback(LOGICAL isbeforestage);
extern void obdrawDrawSortedExternalCallback(LOGICAL isbeforestage);
extern void obdrawDrawUnsortedInternalCallback(LOGICAL isbeforestage);
extern void obdrawDrawSortedInternalCallback(LOGICAL isbeforestage);
extern void obdrawDrawMesh(OBJECT* o);
extern void obdrawToggleUnsortedDraw(bool draw);
extern void obdrawToggleSortedDraw(bool draw);
extern void obdrawToggleSortedCockpitDraw(bool draw);
extern void obdrawToggleUnsortedCockpitDraw(bool draw);
extern void obdrawToggleMultitextureDraw(bool draw);

extern bool obdrawDrawBuildingShadowCasters(int sx,
                                     int sy,
                                     int c,
                                     int r,
                                     const piVECTOR& offset);
extern bool obdrawIsRenderPass(unsigned int pass);

#define MAX_MULTITEXTURES		256
#define MAX_MULTITEXNAMESIZE	64

enum MT_OPCODES
{
	MT_NORMAL=0,		// Normal texturing
	MT_MULTIPASS,		// Multipass multitexturing
	MT_SINGLEPASS,		// This and below opcodes require hardware multitexturing
	MT_BUMPMAP,			// Bump mapping effects
	MT_ENVMAP,			// Environmental texture mapping

	NUM_MT_OPCODES

};

struct MULTITEXTURE
{
	SINT8						BaseTextureName[MAX_MULTITEXNAMESIZE];
	SINT8						MultiTextureName[MAX_MULTITEXNAMESIZE];
	SINT32						BaseTextureId;
	SINT32						MultiTextureId;
	SINT32						MultiMaterialId;
	piMATRIX					MultiTextureMatrix;
	FLOAT32						MultiMaxDistance;
	SINT32						MultiTextureOpcode;

};

extern LOGICAL obdrawSetMultiTexture(const char* BaseTexture, const char* MultiTexture, SINT32 TextureOpcode, FLOAT32 MaxDistance, FLOAT32 TextureMultiplierX, FLOAT32 TextureMultiplierY);
extern LOGICAL obdrawFindMultiTextureByName(SINT8* BaseTexture, SINT32* MTOffset);
extern LOGICAL obdrawFindMultiTextureById(SINT32 BaseId, SINT32* MTOffset);
extern SINT32 obdrawGetMultiTextureOpcode(SINT32 BaseId, SINT32* MTOffset);
extern LOGICAL obdrawGetMultiTextureInfo(MULTITEXTURE** retMultiTextureInfo, SINT32* retNumMultiTextures);
extern LOGICAL obdrawApplyMultiTexture(OBJECT* lpobj);
extern LOGICAL obdrawApplyMultiTextureStage(OBJECT* lpobj);
extern LOGICAL obdrawApplyMultiTextureStage2(OBJECT* lpobj);
extern LOGICAL obdrawUpdateDOT3ColorPoint(OBJECT* lpobj);
extern LOGICAL obdrawUpdateDOT3ColorDirectional(OBJECT* lpobj);
extern LOGICAL obdrawApplyBumpmapTextureStage(OBJECT* lpobj);
extern LOGICAL obdrawReleaseMultiTextureStage(OBJECT* lpobj);
extern LOGICAL obdrawReleaseBumpmapTextureStage(OBJECT* lpobj);
extern UINT32	obdrawGetMultiTextureCount(SINT32 texId);

extern LOGICAL obAdjustObjectTreeVisListStatus(OBJECT * Object, UINT32 IncludeSibs);
extern int obdrawGetMultiTextureID(int offset);
extern void obdrawCull();
extern void obdrawDrawShadowCasters();
extern void obdrawGetMultiTextureMatrix(int offset, Matrixf& matrix);

extern float GetDistance(OBJECT * lplod, bool multiply);
