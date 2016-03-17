#pragma once

#include "trn/trnmain.h"
#include "trn/trnldr.h"

// Declared in trnmain.c
#undef EXTERN
#ifndef  GLOBALS
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN LOGICAL           TerrainLightingUpdate, TerrainStable;
EXTERN LOGICAL           loading;
EXTERN SINT8             mapname[64];
EXTERN SINT32            deftexid, trnlod;
EXTERN FLOAT32           xstep, ystep, xmult, horizon;
EXTERN TERRAIN_DETAIL    trndtl[NUM_TERRAIN_RECTANGLES];
EXTERN INT32VECTOR       WorldOriginOffset; // the current offset from the absolute origin
EXTERN DEMHDR            hdr;
EXTERN TRN_OBJECT_NOTIFY creation;
EXTERN TRN_OBJECT_NOTIFY deletion;

#ifdef GLOBALS
	EXTERN LOGICAL activeTerrainDetail[NUM_TERRAIN_RECTANGLES] = { FALSE, FALSE, FALSE, FALSE, FALSE,
	                                                               FALSE,  TRUE,  TRUE,  TRUE, FALSE,
	                                                               FALSE,  TRUE,  TRUE,  TRUE, FALSE,
	                                                               FALSE,  TRUE,  TRUE,  TRUE, FALSE,
	                                                               FALSE, FALSE, FALSE, FALSE, FALSE };
#else
	EXTERN LOGICAL activeTerrainDetail[NUM_TERRAIN_RECTANGLES];
#endif
