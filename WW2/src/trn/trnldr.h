#pragma once

#include "trn/trnmain.h"

LOGICAL trnldrInit();
void trnldrKill();
LOGICAL trnldrUpdate();
LOGICAL trnldrFinishUpdate();
LOGICAL trnldrFinishUpdateRelocate();
LOGICAL trnldrStartLoadingAll(SINT32 sc, SINT16 scx, SINT16 scy);
LOGICAL trnldrStartLoadingFirstHalf(SINT32 sc, SINT16 scx, SINT16 scy);
LOGICAL trnldrStartLoadingSecondHalf(SINT32 sc, SINT16 scx, SINT16 scy);
LOGICAL trnldrStartUnloadingAll(SINT32 sc);
LOGICAL trnldrStartUnloadingFirstHalf(SINT32 sc);
LOGICAL trnldrStartUnloadingSecondHalf(SINT32 sc);
LOGICAL trnldrReleaseSupercell(SINT32 sc);
LOGICAL trnldrSetTextureRotation(struct OBJECT* lpobj, SINT32 tuv, SINT32 octet);
LOGICAL trnldrReadHdrFile(const char* filename, struct DEMHDR* hdr);
LOGICAL trnldrLoadSupercell(SINT32 index);
LOGICAL trnldrModifySupercell(SINT32 index);
LOGICAL trnldrSaveSupercell(struct TERRAIN_DETAIL* td);
LOGICAL trnldrLoadDTL(struct TERRAIN_DETAIL* td);
LOGICAL trnldrLoadNormals(struct TERRAIN_DETAIL* td);
LOGICAL trnldrLoadPoints(struct TERRAIN_DETAIL* td);
LOGICAL trnldrReleaseTerrainObject(struct OBJECT* lpobj, struct TERRAIN_DETAIL* td);
LOGICAL trnldrFindStratObjectPosition(SINT32 SupercellX, SINT32 SupercellY, SINT32 LocId, struct FLOAT64VECTOR* pos);
SINT32 trnldrGetTLOGsForSupercell(SINT16 scx, SINT16 scy);
LOGICAL trnldrRLEToData(short inlen, short* indata, short* outlen, short* outdata);

void trnldrReduceEditList(struct TERRAIN_DETAIL* td);

LOGICAL trnldrLoadNormals(OBJECT* tt, short normals[SUPERCELL_SIZE_XY]);
LOGICAL trnldrLoadPoints(OBJECT* tt, short heightmap[SUPERCELL_SIZE_XY]);
void trnldrInitializeSuperCellVertices(OBJECT* tt, int scx, int scy);
