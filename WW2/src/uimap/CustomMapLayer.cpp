#include <syscfg.h> // Path is project specific.
#include <string>

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"
#include "phui/Util.h"
#include "include/vars.h"
#include "sdm/sdm.h"
#include "cfg/cfgPrefs.h"
#include "ui/uiMain.h"
#include "scen/ScenarioManager.h"

#include "gl/glplat.h"

#include "uimap/CustomMapLayer.h"

/************************************************************************************************
	CustomMapLayer
*************************************************************************************************/

CustomMapLayer::CustomMapLayer(SuperCellMapView* mv) :
    _mv(mv),
	_firstRun(true),
    _texid(-1)
{
	enabled = true;
}

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void CustomMapLayer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	if(_firstRun)
	{
		_texid = uiGetTexture(SCEN_MGR->MapPath("map.bmp"));

	    if(!piIsTexture(_texid, 0))
		{
			enabled = false;
		    return;
		}

		// need to go through and disable a few layers
		SuperCellMapView::RendererList renderers = _mv->RenderList();

		for(size_t i = 0; i < renderers.size(); i++)
		{
			phui::ref_ptr<Renderer>& renderer = renderers.at(i);

			if(strcasecmp(renderer->name.c_str(), "texture") == 0)
				renderer->enabled = false;
			else if(strcasecmp(renderer->name.c_str(), "icon") == 0)
				renderer->enabled = false;
			else if(strcasecmp(renderer->name.c_str(), "topo") == 0)
				renderer->enabled = false;
		}

		_firstRun = false;
	}
	else if(!piIsTexture(_texid, 0))
        return;

	phui::GraphicsContext* ctx = _mv->GetGraphicsContext();

	glPushMatrix();

	float mapWidth = 855.0f;
	float mapHeight = 400.0f;

	float scale = _mv->Width() / _mv->Scale();
	if(_mv->Scale() > 8) scale = _mv->Width() / 8.0f;

	float mapSizeW = mapWidth / scale;
	float mapSizeH = mapHeight / scale;

	ctx->EnableTexture();
	ctx->SetTexture(_texid);
	ctx->SetColor(1,1,1,1);

	float mapx = 0.0f;
	float mapy = 0.0f;
	_mv->LocalToWorldCoordinates(&mapx, &mapy);

	mapx = (float)mapx/SDM_SUPERCELL_SIZE - mapSizeW/2.0f;
	mapy = (float)mapy/SDM_SUPERCELL_SIZE - mapSizeH/2.0f;

	phui::BoxVerts box;
	float uv[8] = {0,0, 1,0, 1,1, 0,1};

	//phui::box_verts_with_pos(box, x, y, width, height);
	//phui::box_verts_with_pos(box, 0, 0, _mv->Width(), _mv->Height());
	phui::box_verts_with_pos(box, mapx, mapy, mapSizeW, mapSizeH);

	ctx->DrawQuad(box.verts, uv);
	ctx->SetColor(1,1,1,1);

	glPopMatrix();
}
