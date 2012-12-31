#pragma once

#include "sdm/sdmDefines.h"
#include "trn/trntypes.h"
#include "sdm/sdmObjInfo.h"
#include "sdm/sdmTerrainInfo.h"

struct sdm_replacement_octet_data
{
	int numtex;
	char texname[SDM_MAX_TERRAIN_TEXTURES][32];

	int numtrninfo;
	sdm_terrain_info    trninfo[SDM_TERRAIN_TRIANGLES];

	int numobjinfo;
	SDM_ObjInfo     objinfo[SDM_SUPERCELL_OCTETS_XY * MAX_TRNOBJ_OCTET];
};
