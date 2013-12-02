#pragma once

#include "sdm/sdm.h"
#include "uimap/SuperCellMapView.h"

/************************************************************************************************
	ScenarioMapRenderer
*************************************************************************************************/
/// Class for rendering sdm data using icons for each octet

class ScenarioMapRenderer : public SuperCellMapView::Renderer
{
public:
	ScenarioMapRenderer() {};
	~ScenarioMapRenderer() {};

	virtual SuperCell* NewSuperCell(Renderer*, int scx, int scy);
	virtual void RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2);

	class RenderBucket;
	class SuperCell;
};
