#include <syscfg.h> // Path is project specific.
#include <string>
#include <string.h>

#include "gl/glplat.h"

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"

#include "uimap/ScenarioMapRenderer.h"
#include "ogl/GLCounter.h"
#include "scen/ScenarioManager.h"
#include "ui/uiMain.h"
#include "ld/ldtexture.h"
#include "ld/ldpix.h"

class ScenarioMapRenderer::SuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	SuperCell(Renderer*, int scx, int scy);
	virtual ~SuperCell();

	int texid;
	unsigned int lastdrawtime;

    virtual void Draw(phui::GraphicsContext*);
	virtual void Load();
};

/************************************************************************************************
	ScenarioMapRenderer
*************************************************************************************************/

/************************************************************************************************
	NewSuperCell
*************************************************************************************************/

SuperCellMapView::Renderer::SuperCell* ScenarioMapRenderer::NewSuperCell(Renderer* r, int scx, int scy)
{
	// scenario maps currently only occupy the area around 0,0,0
	if(scx == 1 && scy == 1)
		return new ScenarioMapRenderer::SuperCell(r, scx, scy);

	return NULL;
}

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void ScenarioMapRenderer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	SuperCellMapView::Renderer::RenderSuperCells(mv, -1, -1, 1, 1);

	unsigned int t = (unsigned int)frametime; //mv->ParentLoop()->FrameTime();

	SuperCellMap::iterator it = _scmap.begin();
	for(; it != _scmap.end(); )
	{
		SuperCellMap::iterator eraseit = it++;
		ScenarioMapRenderer::SuperCell* sc = (ScenarioMapRenderer::SuperCell*)eraseit->second.ptr();

		// quick hack to unload cells, nuke after 30 sec
		if(sc && sc->loaded && t - sc->lastdrawtime > 30*1000)
			_scmap.erase(eraseit);
	}
}

/************************************************************************************************
	IconMapRenderer::SuperCell
*************************************************************************************************/

ScenarioMapRenderer::SuperCell::SuperCell(Renderer* r, int x, int y)
							: SuperCellMapView::Renderer::SuperCell(r, x, y)
{
	texid = ldTextureLoad(SCEN_MGR->MapPath("map0.dds"), LDPIX_FLIP);
	//texid = uiGetTexture(SCEN_MGR->MapPath("map01.dds"));
	loaded = true;
	loading = false;
};

ScenarioMapRenderer::SuperCell::~SuperCell()
{
};

/************************************************************************************************
	Load
*************************************************************************************************/

void ScenarioMapRenderer::SuperCell::Load()
{
}

/************************************************************************************************
	IconMapRenderer::SuperCell::Draw
*************************************************************************************************/

void ScenarioMapRenderer::SuperCell::Draw(phui::GraphicsContext* ctx)
{
	if(texid == -1)
		return;

	float uv[8] = {0,1-1,1,1-1,1,1-0,0,1-0};
	phui::BoxVerts box;

	phui::box_verts_with_pos(box, 0, 0, (float)64.0f, (float)64.0f);

	ctx->EnableTexture();
	ctx->SetTexture(texid);

	FLOAT32 move = (1.0f / 16.0f) * .5f;

	glTranslatef(-move, -move, 0);

	FLOAT32 scale = 1.0f / 1024.0f; // / (float)renderer->_mv->Scale();

	glPushMatrix();
	glScalef(scale, scale, 1);
	phui::draw_gl_quad(box.verts, uv);
	glPopMatrix();

	glTranslatef(move, move, 0);

	lastdrawtime = frametime;
}

