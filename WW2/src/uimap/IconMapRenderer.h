#pragma once

#include "sdm/sdm.h"
#include "uimap/SuperCellMapView.h"

/************************************************************************************************
	IconMapRenderer
*************************************************************************************************/
/// Class for rendering sdm data using icons for each octet

class IconMapRenderer : public SuperCellMapView::Renderer
{
public:
	IconMapRenderer() : GetOctetTexture(0), GetSDMData(0) {};
	~IconMapRenderer() {};

	virtual SuperCell* NewSuperCell(Renderer*, int scx, int scy);
	virtual void RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2);

	int (*GetOctetTexture)(IconMapRenderer*, const char* texname, float uv[4]);
	int (*GetSDMData)(int scx, int scy, sdm_dtl_data*, sdm_replacement_octet_data*);

	class RenderBucket;
	class SuperCell;
};
