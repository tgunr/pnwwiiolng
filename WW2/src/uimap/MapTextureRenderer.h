#pragma once

#include "uimap/SuperCellMapView.h"

/************************************************************************************************
	MapTextureRenderer
*************************************************************************************************/

class MapTextureRenderer : public SuperCellMapView::Renderer
{
public:
    MapTextureRenderer();
	~MapTextureRenderer();

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy);

	void SetMapOrigin(double left, double bottom);
	void SetNumColsRows(int numcols, int numrows);
	void SetCellDimensions(double width, double height);
	void SetTextureCallback(int (*gettex)(int col, int row) );

	int drawcount;

class SuperCell;
class CellMap;

	double _left, _bottom;
	int _numcols, _numrows;
	double _cellwidth, _cellheight;
	int (*_gettex)(int,int);
	CellMap* _cells;
};
