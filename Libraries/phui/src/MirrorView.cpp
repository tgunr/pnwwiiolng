#include "phui-precompiled-header.h"

#include "phui/GraphicsContext.h"
#include "phui/Viewport.h"
#include "phui/MirrorView.h"
#include "phui/UIXML.h"
#include "phui/Util.h"
#include "phui/CSSViewDrawer.h"

#ifdef _WIN32
#include <windows.h>
template <typename _Type>
_Type inline round(const _Type& x) { return floor(x + (_Type)0.5); }
//#define round(x) floor(x+.5)
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

using namespace phui;

/************************************************************************************************
	MirrorView
*************************************************************************************************/

MirrorView::MirrorView() :
	_sx(0), _sy(0), _sw(0), _sh(0), _capture(false)
{
}

MirrorView::~MirrorView()
{
	if(_texInfo.texid > 0)
		UIXMLDestroyTexture(_texInfo.texid);
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void MirrorView::DrawSelf()
{
	//View::DrawSelf();

	if(_capture)
		capture();
	
	if(_texInfo.texid > 0)
	{
		GraphicsContext* ctx = GetGraphicsContext();
		ctx->SetColor(1,1,1,1);
		ctx->EnableTexture();
		ctx->DrawTexture(_texInfo, 0, 0, _width, _height);

		// draw borders
		if(_bordercolor[3])
		{
			float size = 1.0f;

			ctx->DisableTexture();
			ctx->SetColor(_bordercolor[0], _bordercolor[1], _bordercolor[2], _bordercolor[3]);
			
			CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(GetDrawer());

			if(drawer)
			{
				const char* s = drawer->GetStyleProperty("border-size");
				if(s && *s) size = (float)atof(s);
			}
			
			BoxVerts box;
			box_verts_with_pos(box, 0, 0, Width(), Height());
			ctx->DrawLineStrip(4, box.verts, true, size);
		}
	}
}

void MirrorView::SnapShot(float x, float y, float w, float h)
{
	_sx			= x;
	_sy			= y;
	_sw			= w;
	_sh			= h;
	_capture	= true;
}

void MirrorView::capture()
{
	_capture = false;

	if(_texInfo.texid > 0)
		UIXMLDestroyTexture(_texInfo.texid);
	
	_texInfo.texid = -1;
	
	/*
	float sx, sy, sw, sh;
	sx = sy = sw = sh = 0;
	
	LocalToScreen(&sx, &sy);
	sw = Width();
	sh = Height();
	*/
	
	_texInfo = UIXMLCreateTexture((int)_sw, (int)_sh, 24);
	
	if(_texInfo.texid > 0)
	{
		GraphicsContext* ctx = GetGraphicsContext();
		ctx->EnableTexture();
		ctx->SetTexture(_texInfo.texid);

		//glEnable(GL_TEXTURE_2D);
		//glGenTextures(1, (GLuint*)&_texInfo.texid);
		//glBindTexture(GL_TEXTURE_2D, _texInfo.texid);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		
		//glPixelStorei(GL_PACK_ALIGNMENT, 1);
		//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		// create a new image
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		// copy a region of the back buffer
		glCopyTexSubImage2D(GL_TEXTURE_2D, (GLint)0, (GLint)0, (GLint)0, (GLint)_sx, (GLint)_sy, (GLsizei)_sw, (GLsizei)_sh);
		
		// cleanup
		ctx->DisableTexture();
		//glBindTexture(GL_TEXTURE_2D, 0);
		//glDisable(GL_TEXTURE_2D);
		
		//ResizeTo(sw, sh);
	}
}