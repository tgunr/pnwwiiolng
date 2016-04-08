#pragma once

#include "trn/trnmain.h"
#include "trn/trnldr.h"

// Declared in trnmain.c
extern LOGICAL           TerrainLightingUpdate, TerrainStable;
extern LOGICAL           loading;
extern SINT8             mapname[64];
extern SINT32            deftexid, trnlod;
extern FLOAT32           xstep, ystep, xmult, horizon;
extern TERRAIN_DETAIL    trndtl[NUM_TERRAIN_RECTANGLES];
extern INT32VECTOR       WorldOriginOffset; // the current offset from the absolute origin
extern DEMHDR            hdr;
extern TRN_OBJECT_NOTIFY creation;
extern TRN_OBJECT_NOTIFY deletion;
extern LOGICAL activeTerrainDetail[NUM_TERRAIN_RECTANGLES];
