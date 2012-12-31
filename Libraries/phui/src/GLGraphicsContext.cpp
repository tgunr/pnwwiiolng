#include "phui-precompiled-header.h"

#include <list>
#include <string>
#include <map>
#include <vector>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
template <typename _Type>
_Type inline round(const _Type& x) { return floor(x + (_Type)0.5); }
//#define round(x) floor(x+.5)
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#include "macgl.h"
#endif

#include "phui/FTFontReplacement.h"

#include "phui/GLGraphicsContext.h"
#include "phui/Util.h"


#ifndef M_PI
#define M_PI 3.1415926535898
#endif

using namespace phui;

typedef std::pair<std::string,int> FontKey;
typedef std::map<FontKey, FTFontAdaptor*> FontMap;

class GLGraphicsContext::FontMgr
{
public:
	FontMap fontmap;
	std::vector<FTFontAdaptor*> font;
	std::vector<std::string> fontnames;
	int defaultfont;
	
	FTFontAdaptor* FontAt(unsigned int i)
	{
		if(i == 0) i = defaultfont;
		
		if(i > 0 && i <= font.size())
			return font[i - 1];
		else
			return NULL;
	}
	
	std::string FontName(unsigned int i)
	{
		if(i == 0) i = defaultfont;
		
		if(i > 0 && i <= font.size())
			return fontnames[i - 1];
		else
			return "";
	}
	
	FontMgr() { defaultfont = 0;}
};

/************************************************************************************************
	GLGraphicsContext
*************************************************************************************************/

GLGraphicsContext::GLGraphicsContext()
{
	_fonts = new FontMgr;
	_io = &phui::fdIOCallbacks;
}

GLGraphicsContext::~GLGraphicsContext()
{
	delete _fonts;
	// NO need to delete all fonts allocated as well (and memory buffer for read?)
}

/************************************************************************************************
	MakeCurrentContext
*************************************************************************************************/

void GLGraphicsContext::MakeCurrentContext()
{
}

/************************************************************************************************
	PushState
*************************************************************************************************/

void GLGraphicsContext::PushState()
{	
	//glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	// | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT GL_VIEWPORT_BIT | GL_TEXTURE_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
}

/************************************************************************************************
	PushState
*************************************************************************************************/

void GLGraphicsContext::PopState()
{
	glPopMatrix();
	//glPopAttrib();
}

/************************************************************************************************
	EnableTexture
*************************************************************************************************/

void GLGraphicsContext::EnableTexture()
{
	glEnable(GL_TEXTURE_2D);
}

/************************************************************************************************
	SetTexture
*************************************************************************************************/

void GLGraphicsContext::SetTexture(int texid)
{
	glBindTexture(GL_TEXTURE_2D, texid);
}

/************************************************************************************************
	DisableTexture
*************************************************************************************************/

void GLGraphicsContext::DisableTexture()
{
	glDisable(GL_TEXTURE_2D);
}

/************************************************************************************************
	Clip
*************************************************************************************************/

void GLGraphicsContext::Clip(float left, float bottom, float right, float top)
{	

	double clipl[4] = {1,0,0,-(round(left)-.1f)};
	double clipr[4] = {-1,0,0,round(right)+.1f};
	double clipt[4] = {0,-1,0,round(top)+.1f};
	double clipb[4] = {0,1,0,-(round(bottom)-.1f)};
	
	glClipPlane(GL_CLIP_PLANE0, clipl);
	glClipPlane(GL_CLIP_PLANE1, clipr);
	glClipPlane(GL_CLIP_PLANE2, clipt);
	glClipPlane(GL_CLIP_PLANE3, clipb);
}

/************************************************************************************************
	SetColor
*************************************************************************************************/

void GLGraphicsContext::SetColor(float r, float g, float b, float a)
{
	glColor4f(r,g,b,a);
}

/************************************************************************************************
	MovePenPosition
*************************************************************************************************/

void GLGraphicsContext::MovePenPosition(float x, float y)
{
    // Floor gives incorrect offsets
    x = float(int(x));
    y = float(int(y));

	glTranslatef(x, y, 0);
}

/************************************************************************************************
	DrawPolygon
*************************************************************************************************/

void GLGraphicsContext::DrawPolygon(int count, float const* coords)
{
	glBegin(GL_POLYGON);
		for(int i = 0; i < count;i++)
			glVertex2f(round(coords[2*i]), round(coords[2*i+1]));
    glEnd();

	//glVertexPointer(count, GL_FLOAT, 0, coords);
	//glDrawArrays(GL_POLYGON, 0, count);
}

/************************************************************************************************
	DrawQuad
*************************************************************************************************/

void GLGraphicsContext::DrawQuad(const float* coords, const float* uv)
{
/*
    glBegin(GL_QUADS);
		if(uv) glTexCoord2f(uv[0],uv[1]);
        glVertex2f(round(coords[0]),round(coords[1]));

       if(uv) glTexCoord2f(uv[2],uv[3]);
        glVertex2f(round(coords[2]),round(coords[3]));
		
        if(uv) glTexCoord2f(uv[4],uv[5]);
		glVertex2f(round(coords[4]),round(coords[5]));
        
        if(uv) glTexCoord2f(uv[6],uv[7]);
        glVertex2f(round(coords[6]),round(coords[7]));
    glEnd();
*/
    
    glBegin(GL_TRIANGLES);
    
    // 0,1,2
    if(uv) glTexCoord2f(uv[0],uv[1]);
    glVertex2f(round(coords[0]),round(coords[1]));
    
    if(uv) glTexCoord2f(uv[2],uv[3]);
    glVertex2f(round(coords[2]),round(coords[3]));
    
    if(uv) glTexCoord2f(uv[4],uv[5]);
    glVertex2f(round(coords[4]),round(coords[5]));
    
    // 0,2,3
    if(uv) glTexCoord2f(uv[0],uv[1]);
    glVertex2f(round(coords[0]),round(coords[1]));
    
    if(uv) glTexCoord2f(uv[4],uv[5]);
    glVertex2f(round(coords[4]),round(coords[5]));
    
    if(uv) glTexCoord2f(uv[6],uv[7]);
    glVertex2f(round(coords[6]),round(coords[7]));
    
    glEnd();
    
}

/************************************************************************************************
	DrawLineStrip
*************************************************************************************************/

void GLGraphicsContext::DrawLineStrip(int count, float const* coords, bool closed)
{
	glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
		for(int i = 0; i < count;i++)
			glVertex2f(round(coords[2*i]), round(coords[2*i+1]));
	glEnd();
}

/************************************************************************************************
	DrawLineStrip - with width
*************************************************************************************************/

void GLGraphicsContext::DrawLineStrip(int count, float const* coords, bool closed, float width)
{
	GLfloat current;
	
	glGetFloatv(GL_LINE_WIDTH, &current);
	
	glLineWidth((GLfloat)width);
	DrawLineStrip(count, coords, closed);
	glLineWidth(current);
}

/************************************************************************************************
	DrawCircle
*************************************************************************************************/

void GLGraphicsContext::DrawCircle(float r, bool filled)
{
	int count = 19;
	float div = (float)(count + 1);
	glBegin(filled ? GL_POLYGON : GL_LINE_LOOP);
		for(int i = 0; i < count;i++)
		{
			float x = r*(float)cos(2.0 * M_PI * i / div);
			float y = r*(float)sin(2.0 * M_PI * i / div);
			glVertex2f(x,y);
		}
	glEnd();
}

/************************************************************************************************
	DrawString
*************************************************************************************************/

void GLGraphicsContext::DrawString(unsigned int i, const char* str)
{
	FTFontAdaptor* font = _fonts->FontAt(i);
	if(font)
	{
		MovePenPosition(0, -font->Descender());
#ifdef _WIN32
		font->Render(""); // stupid nVidia workaround
#endif
		font->Render(str);
		MovePenPosition(0, font->Descender());
	}
}

/************************************************************************************************
	DrawString
*************************************************************************************************/

void GLGraphicsContext::DrawString(unsigned int i, const wchar_t* str)
{
	FTFontAdaptor* font = _fonts->FontAt(i);
	if(font)
	{
		MovePenPosition(0, -font->Descender());
#ifdef _WIN32
		font->Render(""); // stupid nVidia workaround
#endif
		font->Render(str);
		MovePenPosition(0, font->Descender());
	}
}

/************************************************************************************************
	FontHeight
*************************************************************************************************/

float GLGraphicsContext::FontHeight(unsigned int i)
{
	FTFontAdaptor* font = _fonts->FontAt(i);
	if(font)
		return font->Ascender() - font->Descender();
	
	return 0;
}

/************************************************************************************************
	FontAdvance
*************************************************************************************************/

float GLGraphicsContext::FontAdvance(unsigned int i, const char* str)
{
	FTFontAdaptor* font = _fonts->FontAt(i);
	if(font && str && *str)
		return font->Advance(str);
	
	return 0;
}

/************************************************************************************************
	FontAdvance
*************************************************************************************************/

float GLGraphicsContext::FontAdvance(unsigned int i, const wchar_t* str)
{
	FTFontAdaptor* font = _fonts->FontAt(i);
	if(font && str && *str)
		return font->Advance(str);

	return 0;
}

/************************************************************************************************
	SetDefaultFont
*************************************************************************************************/

bool GLGraphicsContext::SetDefaultFont(const char* name, unsigned int size)
{
	int found = FindFont(name, size);

	if(found)
		_fonts->defaultfont = found;
	
	return found != 0;
}

/************************************************************************************************
	SetDefaultFont
*************************************************************************************************/

bool GLGraphicsContext::SetDefaultFont(unsigned int size)
{
	return SetDefaultFont(_fonts->FontName(_fonts->defaultfont).c_str(), size);
}

/************************************************************************************************
	SetDefaultFont
*************************************************************************************************/

int GLGraphicsContext::GetDefaultFont()
{
	return _fonts->defaultfont;
}

/************************************************************************************************
	FindFont
*************************************************************************************************/

static unsigned char* FontBuff = NULL;
static size_t FontBuffLen = 0;

extern unsigned int gLanguage;

unsigned int GLGraphicsContext::FindFont(const char* fontname, int size)
{
	if(!_io) return 0;
    
	FTFontAdaptor* font = NULL;
	FontKey key = FontKey(fontname,size);
	FontMap::iterator it = _fonts->fontmap.find(key);
	int fontnum = 0;
	
	if(it == _fonts->fontmap.end())
	{
        if ( gLanguage != 5 ) // not IDE_LANGUAGE_CHINESE
        {
            font = new FTFontWestern();
        }
        else
        {
            if ( FontBuff == NULL )
            {
                int fd = _io->openread(fontname);
                if(fd > -1)
                {
                    FontBuffLen = _io->size(fontname);
                    FontBuff = (unsigned char*)malloc(FontBuffLen);
                    
                    _io->read(fd, FontBuff, (int)FontBuffLen);
                    _io->close(fd);	
                }
            }
            font = new FTFontAdapted(new FTGLTextureFont(FontBuff, FontBuffLen));
        }
        
        if(font->Error() == 0)
        {
            font->FaceSize(size);
            
            _fonts->font.push_back(font);
            _fonts->fontnames.push_back(fontname);
            fontnum = (int)_fonts->font.size();
        }
        else
        {
            delete font; 
            font = NULL;
        }
		
		_fonts->fontmap[key] = font;
	}
    else
	{
		font = it->second;
        
		for(unsigned int i = 0; i < _fonts->font.size(); i++)
		{
			if(_fonts->font[i] == font)
			{
				fontnum = i+1;
				break;
			}
		}
	}
	
	return fontnum;
}  

/************************************************************************************************
	FontName
*************************************************************************************************/

std::string GLGraphicsContext::FontName(unsigned int i)
{
	return _fonts->FontName(i);
}

/************************************************************************************************
	FontSize
*************************************************************************************************/

int GLGraphicsContext::FontSize(unsigned int i)
{
	FTFontAdaptor* font = _fonts->FontAt(i);
	if(font)
		return font->GetFaceSize();
	else
		return 0;
}

/************************************************************************************************
	DrawTexture
*************************************************************************************************/

void GLGraphicsContext::DrawTexture(TextureInfo const& texid, float left, float bottom, float right, float top)
{
	if(texid.texid < 0) return;

	if(_localTexture)
		glBindTexture(GL_TEXTURE_2D, texid.texid);
	else
		SetTexture(texid.texid);
	
	left = round(left);
	bottom = round(bottom);
	right = round(right);
	top = round(top);

	BoxVerts coords;
	phui::box_verts_with_bounds(coords, left, bottom, right, top);

	DrawQuad(coords.verts, texid.uv);
}

/************************************************************************************************
	DrawFlippedTexture
*************************************************************************************************/

void GLGraphicsContext::DrawFlippedTexture(TextureInfo const& info, float left, float bottom, float right, float top, bool fliph, bool flipv)
{
	if(info.texid < 0) return;

	SetTexture(info.texid);

	left = round(left);
	bottom = round(bottom);
	right = round(right);
	top = round(top);

	int idx = 0;
	if(flipv && fliph) idx = 2;
	else if(flipv) idx = 3;
	else if(fliph) idx = 1;
	
	BoxVerts coords;
	float uv[4][8] ={	{info.uv[0],info.uv[1], info.uv[2],info.uv[3], info.uv[4],info.uv[5], info.uv[6],info.uv[7]}
					,	{info.uv[2],info.uv[3], info.uv[0],info.uv[1], info.uv[6],info.uv[7], info.uv[4],info.uv[5]}
					,	{info.uv[4],info.uv[5], info.uv[6],info.uv[7], info.uv[0],info.uv[1], info.uv[2],info.uv[3]}
					,	{info.uv[6],info.uv[7], info.uv[4],info.uv[5], info.uv[2],info.uv[3], info.uv[0],info.uv[1]} 
					};
	phui::box_verts_with_bounds(coords, left, bottom, right, top);

	DrawQuad(coords.verts, uv[idx]);
}

/************************************************************************************************
	DrawRotatedTexture
*************************************************************************************************/

void GLGraphicsContext::DrawRotatedTexture(TextureInfo const& info, float left, float bottom, float right, float top, int orient)
{
	if(info.texid < 0) return;

	SetTexture(info.texid);
	
	left = round(left);
	bottom = round(bottom);
	right = round(right);
	top = round(top);

	if(orient < 0 || orient > 3) orient = 0;
	
	BoxVerts coords;
	float uv[4][8] = {	{info.uv[0],info.uv[1], info.uv[2],info.uv[3], info.uv[4],info.uv[5], info.uv[6],info.uv[7]}
					,	{info.uv[2],info.uv[3], info.uv[4],info.uv[5], info.uv[6],info.uv[7], info.uv[0],info.uv[1]}
					,	{info.uv[4],info.uv[5], info.uv[6],info.uv[7], info.uv[0],info.uv[1], info.uv[2],info.uv[3]}
					,	{info.uv[6],info.uv[7], info.uv[0],info.uv[1], info.uv[2],info.uv[3], info.uv[4],info.uv[5]} 
					};
	phui::box_verts_with_bounds(coords, left, bottom, right, top);

	DrawQuad(coords.verts, uv[orient]);
}