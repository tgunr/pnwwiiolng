#pragma once
//	 Copyright 2008 Playnet Inc, All rights reserved
//	 Not to be distributed in API form except as part of a compiled library
//	 under License or NDA

extern LOGICAL draw3dPoint(piVECTOR* point, UINT32 Color);
extern LOGICAL draw3dLine(const piVECTOR* point1, const piVECTOR* point2, UINT32 Color);
extern LOGICAL draw3dColoredLine(piVECTOR* point1, UINT32 Color1, piVECTOR* point2, UINT32 Color2);

extern LOGICAL draw3dTexturedTriangle(piVECTOR* point1, piVECTOR* point2, piVECTOR* point3,
														int upperleft, UINT32 Color);
extern LOGICAL draw3dTriangle(piVECTOR* point1, piVECTOR* point2, piVECTOR* point3,
					UINT32 Color1, UINT32 Color2, UINT32 Color3);

extern LOGICAL draw3dTexturedQuad(piVECTOR* point1, piVECTOR* point2, piVECTOR* point3, piVECTOR* point4,
						UINT32 Color);

extern LOGICAL draw3dTexture(SINT32 iTexID,
					  piUV uvCoords[2],
					  piVECTOR vecPts[4],
					  UINT32 uColor);

extern LOGICAL draw3dDebugMarkPoint (piVECTOR* point, UINT32 color, FLOAT32 size);

extern LOGICAL draw3dBox(piVECTOR const* corner1, piVECTOR const* corner2, UINT32 color);
extern void draw3dBoxTop(piVECTOR const* corner1, piVECTOR const* corner2, UINT32 color);

extern LOGICAL draw3dLineDebug(const piVECTOR* point1, const piVECTOR* point2, UINT32 Color);

// Actually useful debug line function
extern LOGICAL draw3dGizmoLine(FLOAT64VECTOR const* v1, FLOAT64VECTOR const* v2, piMATRIX const* Mat, const FLOAT64VECTOR* Pos, UINT32 Color);

// functions for putting persistent debug lines and text into the world
// co-ordinates are* not* relative to WorldOriginOffset (hence the FLOAT64VECTOR)
extern void draw3dDebugLineUpdate(LOGICAL drawTopObjects);

extern int  draw3dSetWorldDebugLine(FLOAT64VECTOR const* v1, FLOAT64VECTOR const* v2, const char* name, UINT32 Color);
extern int  draw3dSetWorldDebugText(FLOAT64VECTOR const* pos, const char* text, const char* name, UINT32 Color);

extern int  draw3dSetRelativeDebugLine(FLOAT64VECTOR const* v1, FLOAT64VECTOR const* v2, piMATRIX const* mat, const FLOAT64VECTOR* pos,
								const char* name, UINT32 Color);

extern int  draw3dSetDebugObjectFlipZ(int objID, LOGICAL flipZ); // default false
extern int  draw3dSetDebugObjectAlwaysOnTop(int objID, LOGICAL alwaysOnTop); // default true
extern int  draw3dSetDebugObjectViewRange(int objID, FLOAT64 viewRange); // default infinite (-1)
extern int  draw3dSetDebugObjectRotateToFace(int objID, LOGICAL rotateToFace); // default false
extern int  draw3dSetDebugObjectColor(int objID, UINT32 Color);
extern int  draw3dDebugObjectName(int objID, const char* name, int n);

extern int  draw3dFindDebugObject(const char* name);

extern void draw3dClearDebugLines();
extern void draw3dRemoveDebugObject(int);
extern void draw3dRemoveAllDebugObjects(void);
extern void draw3dInit();

#ifdef BETA_RELEASE
extern void draw3dAddDebugLine(const piVECTOR& p1, const piVECTOR& p2, UINT32 color);
extern void draw3dUpdateDebugPositions(const piVECTOR&);
extern void draw3dDrawDebug();
extern void draw3dSetLineDrawing(bool l);
#endif // #ifdef BETA_RELEASE
