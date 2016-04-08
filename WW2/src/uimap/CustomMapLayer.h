#pragma once

#include <map>
#include "uimap/SuperCellMapView.h"

/************************************************************************************************
	CustomMapLayer
*************************************************************************************************/

class CustomMapLayer : public SuperCellMapView::Renderer
{
public:
    CustomMapLayer(SuperCellMapView* mv);
	~CustomMapLayer(){};

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) { return 0; }

private:

	SuperCellMapView* _mv;
	bool _firstRun;
	int _texid;
};
