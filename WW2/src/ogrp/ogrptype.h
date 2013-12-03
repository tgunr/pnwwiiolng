#pragma once
// object group handling functions
//
// creator - hoof
// 2nd programmer - kango

#include "ob/obmain.h"

extern UINT32 ogrpMoveAndRotateObjectGroup(OBJECT * ObjectPtr,piVECTOR * Pos,piVECTOR * Att);
extern UINT32 ogrpMoveAndRotateSubObject(OBJECT * ObjectPtr,
									 piMATRIX * CurrentMatrix,
									 piMATRIX * OriginalMatrix);
extern UINT32 ogrpFreeObjectGroup(OBJECT * ObjectPtr);
extern UINT32 ogrpSetDOFVariable(OBJECT * ObjectPtr, FLOAT32 VarContents, UINT32 CycleVarContents, UINT32 IgnoreCurrent);
extern UINT32 ogrpParseDOFParameters(OBJECT * InObjectPtr,UCHAR* VarList, UINT32 NumVars, UINT32 MaxVarnameLength);
extern UINT32 ogrpSetObjectAndSubtreeToNotDraw(OBJECT * ObjectPtr);
extern UINT32 ogrpSetObjectAndSubtreeToDraw(OBJECT * ObjectPtr);
extern UINT32 ogrpFindSubObjectByName(OBJECT ** ReturnObject,OBJECT * ObjectPtr, const char * Name);
extern UINT32 ogrpFindSubObjectByDOFVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum);
extern UINT32 ogrpFindSubObjectByDOFVarAndSubVar(OBJECT ** ReturnObject,OBJECT * ObjectPtr,UINT32 VarNum,UINT32 SubVarNum);
extern UINT32 ogrpFreeObjectTree(OBJECT * Object); // frees an object and its subtree
extern UINT32 ogrpShowObjectAndSubtree(OBJECT * ObjectPtr);
extern UINT32 ogrpHideObjectAndSubtree(OBJECT * ObjectPtr);
extern UINT32 ogrpMakeObjectAChildOfAnotherObject(OBJECT * Parent,OBJECT * NewChild);
extern UINT32 ogrpSetObjectAndSubtreeAsCockpit(OBJECT * Object);

