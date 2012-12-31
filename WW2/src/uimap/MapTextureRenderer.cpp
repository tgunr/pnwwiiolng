#include <syscfg.h> // Path is project specific.
#include <vector>
#include <math.h>

#include "phui/GraphicsContext.h"
#include "phui/Util.h"

#include "uimap/MapTextureRenderer.h"

class MapTextureRenderer::SuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	SuperCell(SuperCellMapView::Renderer* r, int col, int row);

	virtual void Draw(phui::GraphicsContext*);
	virtual void Load();

	int texid;
	int drawcount;
	int col,row;
};

class MapTextureRenderer::CellMap : public std::vector<std::vector<MapTextureRenderer::SuperCell*> > {};

/************************************************************************************************
	MapTextureRenderer
*************************************************************************************************/

MapTextureRenderer::MapTextureRenderer()
{
	_bottom = _left = 0;
	_numcols = _numrows = 0;
	_cellwidth = _cellheight = 0;
	_gettex = 0;
	_cells = 0;
	drawcount = 0;
}

MapTextureRenderer::~MapTextureRenderer()
{
	delete _cells;
}

void MapTextureRenderer::SetMapOrigin(double left, double bottom)
{
	_bottom = bottom; _left = left;
}

void MapTextureRenderer::SetNumColsRows(int numcols, int numrows)
{
	_numcols = numcols; _numrows = numrows;
	delete _cells;
	_cells = 0;
}

void MapTextureRenderer::SetCellDimensions(double width, double height)
{
	_cellwidth = width;
	_cellheight = height;
}

void MapTextureRenderer::SetTextureCallback(int (*gettex)(int,int))
{
	_gettex = gettex;
}

void MapTextureRenderer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
// optimize just iterate through _cells instead
	drawcount++;
	SuperCellMapView::Renderer::RenderSuperCells(mv, scx1, scy1, scx2, scy2);
}

SuperCellMapView::Renderer::SuperCell* MapTextureRenderer::NewSuperCell(Renderer* r, int scx, int scy)
{
	if(!_cells)
	{
		_cells = new CellMap;
		std::vector<SuperCell*> col(_numrows);
		_cells->resize(_numcols,col);
	}

	int x = (int)((scx-_left) / _cellwidth);
	int y = (int)((scy-_bottom) / _cellheight);

	if(x < 0 || x >= _numcols || y < 0 || y >= _numrows) return 0;

	MapTextureRenderer::SuperCell* sc = (*_cells)[x][y];
	if(!sc)
	{
		sc = new MapTextureRenderer::SuperCell(this, x, y);
		(*_cells)[x][y] = sc;
	}

	return sc;
}
void MapTextureRenderer::SuperCell::Draw(phui::GraphicsContext* ctx)
{
	if(!loaded)
		return;

	MapTextureRenderer* mtr = (MapTextureRenderer*)renderer;

	if(drawcount == mtr->drawcount) return;
	drawcount = mtr->drawcount;

	float uv[8] = {0,1-1,1,1-1,1,1-0,0,1-0};
	phui::BoxVerts box;

	phui::box_verts_with_pos(box, (float)scx, (float)scy, (float)mtr->_cellwidth, (float)mtr->_cellheight);

	ctx->EnableTexture();
	ctx->SetTexture(texid);
	phui::draw_gl_quad(box.verts, uv);
}

void MapTextureRenderer::SuperCell::Load()
{
	if(loaded) return;

	texid = ((MapTextureRenderer*)renderer)->_gettex(col, row);

	loaded = true;
	loading = false;
}

MapTextureRenderer::SuperCell::SuperCell(SuperCellMapView::Renderer* r, int cl, int rw) : SuperCellMapView::Renderer::SuperCell(r,0,0), col(cl), row(rw)
{
	texid = -1;
	drawcount = 0;

	MapTextureRenderer* mtr = (MapTextureRenderer*)renderer;

	scx = (int)(mtr->_left + mtr->_cellwidth * col);
	scy = (int)(mtr->_bottom + mtr->_cellheight * row);
}
