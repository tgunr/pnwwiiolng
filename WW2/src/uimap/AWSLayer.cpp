#include "syscfg.h" // Path is project specific.
#include <string>

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"
#include "phui/Util.h"
#include "include/vars.h"
#include "sdm/sdm.h"
#include "cfg/cfgPrefs.h"

#include "gl/glplat.h"

#include "uimap/AWSLayer.h"

/************************************************************************************************
	AWSLayer
*************************************************************************************************/

AWSLayer::AWSLayer() :
	_gridMgr(AWSManager::GetInstance()),
	_refreshTime(0),
	_refreshInterval(1000)
{
	enabled = false;
}

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void AWSLayer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	// request an update of the current grid view
	if(frametime > _refreshTime)
	{
		_gridMgr->GetView(_gridView, scx1, scy1, scx2, scy2);
		_refreshTime = frametime + _refreshInterval;
	}

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	ctx->DisableTexture();

	phui::BoxVerts coords;
	float color[4];
	float posx			= 0;
	float posy			= 0;
	UINT32 rowMin		= 0;
	UINT32 rowMax		= 0;
	UINT32 colMin		= 0;
	UINT32 colMax		= 0;
	UINT32 gridSize		= _gridMgr->GetGridSize();
	float mapGridSize	= gridSize/SDM_SUPERCELL_SIZE;

	_gridMgr->GetSCPosition(scx1, scy1, &colMin, &rowMin);
	_gridMgr->GetSCPosition(scx2, scy2, &colMax, &rowMax);

	float xOffset = 0;
	float yOffset = 0;

	_gridMgr->GetTheaterOffset(&xOffset, &yOffset);

	// draw colored squares

	for(UINT32 c = colMin; c <= colMax; c++)
	{
		for(UINT32 r = rowMin; r <= rowMax; r++)
		{
			AWSGrid* grid = _gridView[_gridMgr->GetGridIndex(c, r)];
			int level = (grid && frametime < grid->expires) ? grid->level: 0;

			if(level)
			{
				_calcGridColor(level, &color[0], &color[1], &color[2], &color[3]);

				if(color[3])
				{
					posx = (c * mapGridSize) + xOffset;
					posy = (r * mapGridSize) + yOffset;

					phui::box_verts_with_pos(coords, posx, posy, mapGridSize, mapGridSize);

					ctx->SetColor(color[0], color[1], color[2], color[3]);

					glBegin(GL_QUADS);
						glVertex2f(coords.verts[0],coords.verts[1]);
						glVertex2f(coords.verts[2],coords.verts[3]);
						glVertex2f(coords.verts[4],coords.verts[5]);
						glVertex2f(coords.verts[6],coords.verts[7]);
					glEnd();
				}
			}
		}
	}

	/***************** draw grid lines **************/

	ctx->SetColor(0,0,0,.9f);

	///////////// draw vertical lines

	// start and end y is the same for each column
	coords.verts[1] = (rowMin * mapGridSize) + yOffset;
	coords.verts[3] = (rowMax * mapGridSize) + mapGridSize + yOffset;

	for(UINT32 c = colMin; c <= colMax; c++)
	{
		// set the start and end x
		coords.verts[0] = coords.verts[2] = (c * mapGridSize) + xOffset;

		// draw the line
		glBegin(GL_LINE_STRIP);
		glVertex2f(coords.verts[0], coords.verts[1]);
		glVertex2f(coords.verts[2], coords.verts[3]);
		glEnd();
	}

	// cap the right side
	if(colMax == _gridMgr->GetColumns())
	{
		coords.verts[0] = coords.verts[2] += mapGridSize;

		// draw the line
		glBegin(GL_LINE_STRIP);
		glVertex2f(coords.verts[0], coords.verts[1]);
		glVertex2f(coords.verts[2], coords.verts[3]);
		glEnd();
	}

	//////////// draw horizontal lines

	// start and end x is the same for each row
	coords.verts[0] = (colMin * mapGridSize)  + xOffset;
	coords.verts[2] = (colMax * mapGridSize) + mapGridSize + xOffset;

	for(UINT32 r = rowMin; r <= rowMax; r++)
	{
		// set the start and end y
		coords.verts[1] = coords.verts[3] = (r * mapGridSize) + yOffset;

		// draw the line
		glBegin(GL_LINE_STRIP);
		glVertex2f(coords.verts[0], coords.verts[1]);
		glVertex2f(coords.verts[2], coords.verts[3]);
		glEnd();
	}

	// cap the top side
	if(rowMax == _gridMgr->GetRows())
	{
		coords.verts[1] = coords.verts[3] += mapGridSize;

		// draw the line
		glBegin(GL_LINE_STRIP);
		glVertex2f(coords.verts[0], coords.verts[1]);
		glVertex2f(coords.verts[2], coords.verts[3]);
		glEnd();
	}

	/****************** setup scaled elements like labels and the radar sweep **********************/

	ctx->DisableTexture();
	ctx->SetColor(0,0,0,.7f);

	//phui::phuiDrawString drawString;

	char label[10];
	label[0]			= 0;

	float scale			= mv->Width() / ((mv->Scale() > 8) ? 8.0f: mv->Scale());
	int font			= mv->GetFont();
//	float fontheight	= ctx->FontHeight(font);
	float scaled		= 1.5f/scale;
	float fontx			= 0;
	float fonty			= mv->Height() - 30;
	static float barbuffer = 30.0f;

	// draw elements
	{
		// draw radar sweep

		/* disabled the radar sweep for now; Gophur didn't like it
		float percent = _gridMgr->GetRefreshCompletion();

		if(percent) // turn off sweep for now
		{
			fontx = 0;
			fonty = mv->Height() * percent;

			mv->LocalToSuperCellCoordinates(&fontx, &fonty);

			glTranslatef(fontx, fonty, 0);
			glPushMatrix();
			glScalef(scaled, scaled, 1);

			coords.verts[0] = 0;
			coords.verts[2] = mv->Width();
			coords.verts[1] = coords.verts[3] = 0;

			glBegin(GL_QUADS);
				ctx->SetColor(.3f,.3f,.3f,0);
				glVertex2f(coords.verts[2],coords.verts[3] - 40.0f);
				glVertex2f(coords.verts[0],coords.verts[1] - 40.0f);
				ctx->SetColor(.3f,.3f,.3f,.6f);
				glVertex2f(coords.verts[0],coords.verts[1]);
				glVertex2f(coords.verts[2],coords.verts[3]);
			glEnd();

			glPopMatrix();
			glTranslatef(-fontx, -fonty, 0);
		}
		*/

		// setup horizontal bar
		//float barx = mv->Width();
		//float bary = mv->Height();

		fontx		= 0;
		fonty		= mv->Height() - 30;

		mv->LocalToSuperCellCoordinates(&fontx, &fonty);
		//mv->LocalToSuperCellCoordinates(&barx, &bary);

		glTranslatef(fontx, fonty, 0);
		glPushMatrix();
		glScalef(scaled, scaled, 1);

		// draw a horizontal background to help the labels stand out
		phui::box_verts_with_pos(coords, barbuffer, 0, mv->Width() - barbuffer, barbuffer);

		ctx->SetColor(0,0,0,.7f);
		ctx->DrawQuad(coords.verts);

		glPopMatrix();
		glTranslatef(-fontx, -fonty, 0);

		// setup vertical bar
		fontx	= 0;
		fonty	= 0;
		//barx	= barbuffer;
		//bary	= mv->Height();

		mv->LocalToSuperCellCoordinates(&fontx, &fonty);
		//mv->LocalToSuperCellCoordinates(&barx, &bary);

		glTranslatef(fontx, fonty, 0);
		glPushMatrix();
		glScalef(scaled, scaled, 1);

		// draw a horizontal background to help the labels stand out
		phui::box_verts_with_pos(coords, 0, 0, barbuffer, mv->Height());
		ctx->DrawQuad(coords.verts);

		glPopMatrix();
		glTranslatef(-fontx, -fonty, 0);
	}

	ctx->SetColor(.9f,.9f,.9f,1.0f);
	ctx->EnableTexture();

	fontx	= 10;
	fonty	= mv->Height() - 25;

	mv->LocalToSuperCellCoordinates(&fontx, &fonty);

	for(UINT32 c = colMin; c <= colMax; c++)
	{
		fontx = (c * mapGridSize) + (mapGridSize/2) - .08f + xOffset;

		label[0] = (char)(c/26) + 65;
		label[1] = (char)(c%26) + 65;
		label[2] = 0;

		// translate to the label position
		glTranslatef(fontx, fonty, 0);
		glPushMatrix();
		glScalef(scaled, scaled, 1);
		ctx->DrawString(font, label);
		glPopMatrix();
		glTranslatef(-fontx, -fonty, 0);
	}

	fontx	= 10;
	fonty	= mv->Height() - 25;

	mv->LocalToSuperCellCoordinates(&fontx, &fonty);

	for(UINT32 r = rowMin; r <= rowMax; r++)
	{
		fonty = (r * mapGridSize) + (mapGridSize/2) - .05f + yOffset;
		snprintf(label, 9, "%d",_gridMgr->GetRows() - r);

		// translate to the label position
		glTranslatef(fontx, fonty, 0);
		glPushMatrix();
		glScalef(scaled, scaled, 1);
		ctx->DrawString(font, label);
		glPopMatrix();
		glTranslatef(-fontx, -fonty, 0);
	}

	ctx->SetColor(1,1,1,1);
}

#define AWS_COLOR_MIN .025f
#define AWS_COLOR_MAX .6f
#define AWS_COLOR_DEF .5f
#define AWS_COLOR_RANGE (AWS_COLOR_MAX - AWS_COLOR_MIN)

void AWSLayer::_calcGridColor(int level, float* r, float* g, float* b, float* a) const
{
	UINT32 color = 0;

	*a = AWS_COLOR_MIN + (cfgPrefsGetFloat("app.awsalpha", AWS_COLOR_DEF) * AWS_COLOR_RANGE);

	switch(level)
	{
		case -1:
		{
			*r = *g = *b = 155/255;
		}
		break;
		case 1:
		{
			color = (UINT32)cfgPrefsGetInt("app.awslow", piRGB_MAKE(255,255,0));

			*r = (float)piRGBA_GETRED(color)/255.0f;
			*g = (float)piRGBA_GETGREEN(color)/255.0f;
			*b = (float)piRGBA_GETBLUE(color)/255.0f;
		}
		break;
		case 2:
		case 3:
		{
			color = (UINT32)cfgPrefsGetInt("app.awshigh", piRGB_MAKE(255,0,0));

			*r = (float)piRGBA_GETRED(color)/255.0f;
			*g = (float)piRGBA_GETGREEN(color)/255.0f;
			*b = (float)piRGBA_GETBLUE(color)/255.0f;
		}
		break;
		default:
		{
			*r = *g = *b = 1.0f;
			*a = 0;
		}
		break;
	}
}
