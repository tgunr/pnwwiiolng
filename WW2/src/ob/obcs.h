#pragma once

struct SORTHEADER
{
	struct SORTHEADER*  prev;
	struct SORTHEADER*  next;
	struct OBJECT*    part;
	float       distsqr;
	short       layer;
};

extern LOGICAL obcsInit();
extern void obcsKill();
extern void obcsSetIgnoreAdsOnCull(LOGICAL ignore);
extern void obcsDoCull(LOGICAL bOn);

extern int obcsCullAndSortObjects(int numobjects, struct OBJECT* lpobjorig);
extern int obcsCullAndSortShadowObjects(int numobjects, OBJECT* lpobjorig, piVECTOR* lpdir);

extern LOGICAL obcsBubbleSortObjects(int numobjects, struct OBJECT* lpobjorig, piVECTOR* lpdir);
extern LOGICAL obcsSortObjectsByTexture(struct OBJECT* lpobjorig);
extern LOGICAL obcsSortObjectTriangles(struct OBJECT* lpobj);
extern void obcsAddToUnsortedList(SORTHEADER* lpsort);
extern SORTHEADER* obcsGetNextSortItem();
extern void obcsToggleAdCount();
extern void obcsResetSortedList();
extern void obcsResetUnsortedList();
extern void obcsResetSortedCockpitList();
extern void obcsResetUnsortedCockpitList();
extern void obcsResetWater();
extern void obcsResetMultiTexture();

extern SORTHEADER* obcsGetSortedList();
extern SORTHEADER* obcsGetUnsortedList();
extern SORTHEADER* obcsGetSortedCockpitList();
extern SORTHEADER* obcsGetUnsortedCockpitList();
extern SORTHEADER* obcsGetWaterList();
extern SORTHEADER* obcsGetMultiTextureList();

extern SORTHEADER* obcsGetNextOnList(SORTHEADER* list);
extern SORTHEADER* obcsGetPrevOnList(SORTHEADER* list);
extern OBJECT* obcsGetListPart(SORTHEADER* list);
extern LOGICAL obcsGetObjectCullStats(SINT32 * CullObjectCnt,SINT32 * DisplayedObjectCnt);
