#pragma once

#include "sdm/sdm.h"
#include "uimap/SuperCellMapView.h"

/************************************************************************************************
	TopoMapRenderer
*************************************************************************************************/

class TopoMapRenderer : public SuperCellMapView::Renderer
{
public:
	TopoMapRenderer();
	virtual ~TopoMapRenderer();

	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy);

	void SetRange(float start, float end) { _altstart = start; _altend = end; }
	void SetSpacing(float spacing) { _altspacing = spacing; }

	typedef int (*datafn)(int scx, int scy, sdm_dtl_data*, sdm_replacement_octet_data*);
	void SetDataCallback(datafn d) { _sdmdata = d;}

	float _altstart, _altend, _altspacing;
	datafn _sdmdata;

class ContourSuperCell;
class ShadedSuperCell;
};
