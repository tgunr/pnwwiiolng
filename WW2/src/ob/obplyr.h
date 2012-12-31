#pragma once

#define PLAYER_BACK   -1.0f
#define PLAYER_FRONT   1.0f
#define PLAYER_LEFT   -2.0f
#define PLAYER_RIGHT   2.0f
#define PLAYER_TOP    -4.0f
#define PLAYER_BOTTOM  4.0f


extern LOGICAL obplyrUpdateThisObject(OBJECT* lpobj, piVECTOR* pos, piVECTOR* rot);
//	extern LOGICAL obplyrRotateRawObject(OBJECT* lpobj, piVECTOR* rot);
extern LOGICAL obplyrUpdateObject(OBJECT* lpobj, piVECTOR* pos, piVECTOR* rot);
//	extern LOGICAL obplyrUpdateNamedObject(char* name, piVECTOR* pos, piVECTOR* rot);
extern LOGICAL obplyrOffsetObject(OBJECT* lpobj, piVECTOR* loc);
extern LOGICAL obplyrApplyMatrixToObjectTree(OBJECT* lpobj, piMATRIX* mat);
extern LOGICAL obplyrTranslateObjectTree(OBJECT* lpobj, const piVECTOR& transVec);
extern LOGICAL obplyrRotateAndTranslateObjectTree(OBJECT* lpobj, const piMATRIX& rotMatrix, const piVECTOR& transVec);
extern void	obplyrApplyProjectionAndHeight(OBJECT* o, const piMATRIX& mat);
extern void	obplyrApplyProjectionAndHeightToBase(OBJECT* o, const piMATRIX* mat);
extern void	obplyrApplyBaseProjection(OBJECT* o, const piMATRIX& projectionMatrix, FLOAT32 minTerrainHeight, FLOAT32 maxTerrainHeight);
//	extern LOGICAL obplyrApplyMatrixToObjectNormals(OBJECT* lpmain, piMATRIX* mat);
extern LOGICAL obplyrApplyMatrixToObject(OBJECT* lpobj, piMATRIX* inmat);
