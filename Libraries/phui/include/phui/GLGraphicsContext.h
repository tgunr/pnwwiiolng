#ifndef PHUI_GLGRAPHICS_CONTEXT_H
#define PHUI_GLGRAPHICS_CONTEXT_H

#include "phui/GraphicsContext.h"

#define MAX_FONTNAME 80

namespace phui
{

/************************************************************************************************
	GLGraphicsContext
*************************************************************************************************/
/**
	Functions that need to be implemented in whatever graphics API you want to use with the library.
	Really just a very thin wrapper around the default OpenGL/FTGL implementation.
*/

class GLGraphicsContext : public GraphicsContext
{
public:
	GLGraphicsContext();
	virtual ~GLGraphicsContext();
	
	virtual void MakeCurrentContext();
	
	virtual void PushState();
	virtual void PopState();
	
	virtual void SetTexture(int texid);
	virtual void EnableTexture();
	virtual void DisableTexture();
	
	virtual void Clip(float left, float bottom, float right, float top);
	
	virtual void SetColor(float r, float g, float b, float a = 1.0);
	virtual void MovePenPosition(float x, float y);
	
	virtual void DrawPolygon(int count, float const* coords);
	virtual void DrawQuad(const float* coords, const float* uv = 0);
	virtual void DrawLineStrip(int count, const float* coords, bool closed = false);
	virtual void DrawLineStrip(int count, const float* coords, bool closed, float width);
	virtual void DrawCircle(float radius, bool filled = false);
	virtual void DrawString(unsigned int font, const char*);
	virtual void DrawString(unsigned int font, const wchar_t*);
	
	virtual float FontHeight(unsigned int font);
	virtual float FontAdvance(unsigned int font, const char*);
	virtual float FontAdvance(unsigned int font, const wchar_t*);
	virtual bool SetDefaultFont(const char* name, unsigned int size);
	virtual bool SetDefaultFont(unsigned int size);
	virtual int GetDefaultFont();
	virtual unsigned int FindFont(const char* fontname, int size);
	virtual std::string FontName(unsigned int font);
	virtual int FontSize(unsigned int font);

	virtual void DrawTexture(TextureInfo const&, float left, float bottom, float right, float top);
	virtual void DrawFlippedTexture(TextureInfo const&, float left, float bottom, float right, float top, bool fliph, bool flipv);
	virtual void DrawRotatedTexture(TextureInfo const&, float left, float bottom, float right, float top, int orient);

private:
class FontMgr;
	FontMgr* _fonts;
};
}
#endif // PHUI_GLGRAPHICS_CONTEXT_H
