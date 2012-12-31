#pragma once

extern LOGICAL obcPlayerInfo(struct piVECTOR* lppos, struct piVECTOR* lpdir);
extern LOGICAL obcLight(struct LIGHT* lplight);
extern LOGICAL obcMaterial(struct MATERIAL* material);
extern struct OBJECT* obcObject(struct OBJECT* attached, OBJECT* tv);
extern struct OBJECT* obcSphere(const char* name, const char* texname, int MeshSize, struct piVECTOR* Location, float Scale, UINT32 color);
extern struct OBJECT* obcCylinder(const char* name, const char* texname, int MeshSize, struct piVECTOR* Location, float Scale, float Ellipse, UINT32 Color);
extern struct OBJECT* obcHemiSphere(const char* name, const char* texname, int MeshSize, struct piVECTOR* Location, float Scale, float Ellipse, UINT32 Color);
extern struct OBJECT* obcSphereVoxel(const char* name, const char* texname, int MeshSize, struct piVECTOR* Location, float Scale, float Depth, UINT32 Color);
extern void obcTextureFixup(struct OBJECT* lpobj);

extern LOGICAL obcInit();
#ifdef MAP_MATS
    extern LOGICAL materialsMapInit();
#endif

extern void obcUpdate();
