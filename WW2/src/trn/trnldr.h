#pragma once

#include "trn/trnmain.h"

extern LOGICAL trnldrInit(void);
extern void trnldrKill(void);
extern LOGICAL trnldrUpdate(void);
extern LOGICAL trnldrFinishUpdate(void);
extern LOGICAL trnldrFinishUpdateRelocate(void);
extern LOGICAL trnldrStartLoadingAll(SINT32 sc, SINT16 scx, SINT16 scy);
extern LOGICAL trnldrStartLoadingFirstHalf(SINT32 sc, SINT16 scx, SINT16 scy);
extern LOGICAL trnldrStartLoadingSecondHalf(SINT32 sc, SINT16 scx, SINT16 scy);
extern LOGICAL trnldrStartUnloadingAll(SINT32 sc);
extern LOGICAL trnldrStartUnloadingFirstHalf(SINT32 sc);
extern LOGICAL trnldrStartUnloadingSecondHalf(SINT32 sc);
extern LOGICAL trnldrReleaseSupercell(SINT32 sc);
extern LOGICAL trnldrSetTextureRotation(struct OBJECT* lpobj, SINT32 tuv, SINT32 octet);
extern LOGICAL trnldrReadHdrFile(const char* filename, struct DEMHDR* hdr);
extern LOGICAL trnldrLoadSupercell(SINT32 index);
extern LOGICAL trnldrModifySupercell(SINT32 index);
extern LOGICAL trnldrSaveSupercell(struct TERRAIN_DETAIL* td);
extern LOGICAL trnldrLoadDTL(struct TERRAIN_DETAIL* td);
extern LOGICAL trnldrLoadNormals(struct TERRAIN_DETAIL* td);
extern LOGICAL trnldrLoadPoints(struct TERRAIN_DETAIL* td);
extern LOGICAL trnldrReleaseTerrainObject(struct OBJECT* lpobj, struct TERRAIN_DETAIL* td);
extern LOGICAL trnldrFindStratObjectPosition(SINT32 SupercellX, SINT32 SupercellY, SINT32 LocId, struct FLOAT64VECTOR* pos);
extern SINT32 trnldrGetTLOGsForSupercell(SINT16 scx, SINT16 scy);
extern LOGICAL trnldrRLEToData(short inlen, short* indata, short* outlen, short* outdata);

extern void trnldrReduceEditList(struct TERRAIN_DETAIL* td);

LOGICAL trnldrLoadNormals(OBJECT* tt, short normals[SUPERCELL_SIZE_XY]);
LOGICAL trnldrLoadPoints(OBJECT* tt, short heightmap[SUPERCELL_SIZE_XY]);
void trnldrInitializeSuperCellVertices(OBJECT* tt, int scx, int scy);
