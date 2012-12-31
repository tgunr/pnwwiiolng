#pragma once

#include "uimap/SuperCellMapView.h"
#include "sdm/sdm.h"

/************************************************************************************************
	TerrainObjectRenderer
*************************************************************************************************/

class TerrainObjectRenderer : public SuperCellMapView::Renderer
{
public:
    TerrainObjectRenderer();
	~TerrainObjectRenderer() {};

	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy);

	int (*GetTerrainSDMData)(int sx, int sy, sdm_replacement_octet_data*);
	int (*GetTerrainTextureData)(struct SDM_ObjInfo const*, int* texid, int* x, int* y, int* width, int* height, float* uv);

class SuperCell;
};
