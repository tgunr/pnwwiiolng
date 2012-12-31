#pragma once

#include <map>
#include "uimap/SuperCellMapView.h"
#include "wwii/wwiiHCUnits.h"
#include "uimap/AWSManager.h"

/************************************************************************************************
	AWSLayer
*************************************************************************************************/

class AWSLayer : public SuperCellMapView::Renderer
{
public:
    AWSLayer();
	~AWSLayer() {};

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) { return 0; }

private:

	void _calcGridColor(int level, float* r, float* g, float* b, float* a) const;

	AWSManager* _gridMgr;

	// list of grids currently in view
	GridView _gridView;

	// whats the max scale we'll draw labels at
	UINT32 _maxLabelScale;

	// when to request an update in ms
	int _refreshTime;

	// how between refreshes in ms
	int _refreshInterval;

	float _lastMapX;
	float _lastMapY;
};
