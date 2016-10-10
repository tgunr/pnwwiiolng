#pragma once

struct SORTHEADER
{
	struct SORTHEADER*  prev;
	struct SORTHEADER*  next;
	struct OBJECT*    part;
	float       distsqr;
	short       layer;
};

extern LOGICAL obcsInit(void);
extern void obcsKill(void);
extern void obcsDoCull(bool bOn);

extern int obcsCullAndSortObjects(int numobjects, struct OBJECT* lpobjorig);
extern int obcsCullAndSortShadowObjects(int numobjects, OBJECT* lpobjorig, piVECTOR* lpdir);

extern LOGICAL obcsBubbleSortObjects(int numobjects, struct OBJECT* lpobjorig, piVECTOR* lpdir);
extern LOGICAL obcsSortObjectsByTexture(struct OBJECT* lpobjorig);
extern LOGICAL obcsSortObjectTriangles(struct OBJECT* lpobj);
extern void obcsAddToUnsortedList(SORTHEADER* lpsort);
extern SORTHEADER* obcsGetNextSortItem(void);
extern void obcsToggleAdCount();
extern void obcsResetSortedList(void);
extern void obcsResetUnsortedList(void);
extern void obcsResetSortedCockpitList(void);
extern void obcsResetUnsortedCockpitList(void);
extern void obcsResetWater();
extern void obcsResetMultiTexture();

extern SORTHEADER* obcsGetSortedList(void);
extern SORTHEADER* obcsGetUnsortedList(void);
extern SORTHEADER* obcsGetSortedCockpitList(void);
extern SORTHEADER* obcsGetUnsortedCockpitList(void);
extern SORTHEADER* obcsGetWaterList();
extern SORTHEADER* obcsGetMultiTextureList();

extern SORTHEADER* obcsGetNextOnList(SORTHEADER* list);
extern SORTHEADER* obcsGetPrevOnList(SORTHEADER* list);
extern OBJECT* obcsGetListPart(SORTHEADER* list);
extern LOGICAL obcsGetObjectCullStats(SINT32 * CullObjectCnt,SINT32 * DisplayedObjectCnt);
