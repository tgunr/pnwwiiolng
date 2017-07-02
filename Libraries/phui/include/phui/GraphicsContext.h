#pragma once

#include <string>

namespace phui
{

struct IOCallbacks;

/************************************************************************************************
	GraphicsContext
*************************************************************************************************/
/**
	Functions that need to be implemented in whatever graphics API you want to use with the library.
	Really just a very thin wrapper around the default OpenGL/FTGL implementation.
*/

class GraphicsContext
{
public:
	struct TextureInfo
	{
		TextureInfo() : texid(-1)
		{
			uv[0]=0;uv[1]=0;uv[2]=1;uv[3]=0;uv[4]=1;uv[5]=1;uv[6]=0;uv[7]=1;
		}

		int texid;
		float uv[8];
	};
	
	GraphicsContext() : _io(0), _localTexture(false) {}
	virtual ~GraphicsContext();
	
	virtual void MakeCurrentContext() = 0;
	
	virtual void PushState() = 0;
	virtual void PopState() = 0;
	
	virtual void SetLocalTexture(bool l){ _localTexture = l; }
	virtual void SetTexture(int texid) = 0;
	virtual void EnableTexture() = 0;
	virtual void DisableTexture() = 0;
	
	virtual void Clip(float left, float bottom, float right, float top) = 0;
	
	virtual void SetColor(float r, float g, float b, float a = 1.0) = 0;
	virtual void MovePenPosition(float x, float y) = 0;
	
	virtual void DrawPolygon(int count, float const* coords) = 0;
	virtual void DrawQuad(const float* coords, const float* uv = 0) = 0;
	virtual void DrawLineStrip(int count, const float* coords, bool closed = false) = 0;
	virtual void DrawLineStrip(int count, const float* coords, bool closed, float width) = 0;
	virtual void DrawCircle(float radius, bool filled = false) = 0;
	virtual void DrawString(unsigned int font, const char*) = 0;
	virtual void DrawString(unsigned int font, const wchar_t*) = 0;
	
	virtual float FontHeight(unsigned int font) = 0;
	virtual float FontAdvance(unsigned int font, const char*) = 0;
	virtual float FontAdvance(unsigned int font, const wchar_t*) = 0;
	virtual bool SetDefaultFont(const char* name, unsigned int size) = 0;	// Used whenever 0 is passed as font number to font functions.
	virtual bool SetDefaultFont(unsigned int size) = 0;						// Used whenever 0 is passed as font number to font functions.
	virtual int GetDefaultFont() = 0;
	virtual unsigned int FindFont(const char* fontname, int size) = 0;
	virtual std::string FontName(unsigned int font) = 0;
	virtual int FontSize(unsigned int font) = 0;
	
	virtual void DrawTexture(TextureInfo const&, float left, float bottom, float right, float top) = 0;
	virtual void DrawFlippedTexture(TextureInfo const&, float left, float bottom, float right, float top, bool fliph, bool flipv) = 0;
	virtual void DrawRotatedTexture(TextureInfo const&, float left, float bottom, float right, float top, int orient) = 0;

	void SetIO(IOCallbacks const* io) { _io = io; }
	
protected:
	IOCallbacks const* _io;
	bool _localTexture;
};

}
