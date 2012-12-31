#include "phui-precompiled-header.h"

#ifdef _WIN32
#include <windows.h>
#include <Gl/gl.h>
#else
#include <OpenGL/gl.h>
#endif


#include "phui/GraphicsContext.h"
#include "phui/StencilView.h"

using namespace phui;

/************************************************************************************************
	StencilView
*************************************************************************************************/

StencilView::StencilView()
{
	_acceptsmouseclick = false;
}

StencilView::~StencilView()
{
}
	
/************************************************************************************************
	DrawChildren
*************************************************************************************************/

void StencilView::DrawChildren()
{
	glEnable(GL_STENCIL_TEST);
	glClearStencil(0x0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 0x1, 0x1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GraphicsContext* ctx = GetGraphicsContext();

	ctx->EnableTexture();
	ctx->DrawTexture(_texid, 0, 0, _width, _height);

	glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	View::DrawChildren();
	
	glDisable(GL_STENCIL_TEST);
}