#include <syscfg.h> // Path is project specific.

#include <libxml/tree.h>

#include "mech/MechTarget.h"
#include "phui/UIXML.h"
#include "phui/GraphicsContext.h"

phui::View* uiMechTargetViewCreate(_xmlNode* node, phui::View* parent)
{
	MechTargetView* tv = new MechTargetView;

	if(tv)
	{
		if(node)
		{
			xmlChar* val;

			val = xmlGetProp(node, (xmlChar*)"linelength");
			if(val)
			{
				tv->SetLineLength((float)atof((const char*)val));
				// do something
				xmlFree(val);
			}
		}
	}

	return tv;
}

void MechTargetView::DrawSelf()
{
	phui::View::DrawSelf();

	if(!_textcolor[3]) return;

	phui::GraphicsContext* ctx = GetGraphicsContext();
	int numcoords = 2; // we should always be at least 4 points in size
	float coords[4];
	float length = Width() * lineLength;

	ctx->DisableTexture();
	ctx->SetColor(_textcolor[0], _textcolor[1], _textcolor[2], _textcolor[3]);

	// bottom left corner
	coords[0] = 0;
	coords[1] = 0;
	coords[2] = length;
	coords[3] = 0;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	coords[0] = 0;
	coords[1] = 0;
	coords[2] = 0;
	coords[3] = length;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	// bottom right corner
	coords[0] = _width;
	coords[1] = 0;
	coords[2] = _width - length;
	coords[3] = 0;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	coords[0] = _width;
	coords[1] = 0;
	coords[2] = _width;
	coords[3] = length;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	// top right corner
	coords[0] = _width;
	coords[1] = _height;
	coords[2] = _width;
	coords[3] = _height - length;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	coords[0] = _width;
	coords[1] = _height;
	coords[2] = _width - length;
	coords[3] = _height;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	// top left corner
	coords[0] = 0;
	coords[1] = _height;
	coords[2] = length;
	coords[3] = _height;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	coords[0] = 0;
	coords[1] = _height;
	coords[2] = 0;
	coords[3] = _height - length;
	ctx->DrawLineStrip(numcoords, coords, true, 2.0f);

	ctx->SetColor(1,1,1,1);
}
