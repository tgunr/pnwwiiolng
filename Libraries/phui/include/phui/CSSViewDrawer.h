#ifndef PHUI_CSSVIEWDRAWER_H
#define PHUI_CSSVIEWDRAWER_H

#include <map>
#include <string>

#include "phui/ViewDrawer.h"
#include "phui/GraphicsContext.h"

namespace phui
{
class Viewport;

/************************************************************************************************
	CSSViewDrawerFactory
*************************************************************************************************/
/**
	A Drawer implementation that uses style sheet properties.
*/

typedef GraphicsContext::TextureInfo (*CSSTextureCallback) (const char*, bool repeatingTexture);

class CSSViewDrawerFactory : public ViewDrawerFactory
{
public:
	CSSViewDrawerFactory(Viewport*, CSSTextureCallback);
	virtual ~CSSViewDrawerFactory();

	virtual ViewDrawer* CreateDrawer(View*);
	Viewport* Port() const { return _viewport;}
	
protected:
	Viewport* _viewport;
	CSSTextureCallback _gettexture;
};

class CSSViewDrawer : public ViewDrawer
{
public:
	CSSViewDrawer(View*, Viewport*, CSSTextureCallback);
	virtual ~CSSViewDrawer();
	
	virtual void Draw();
	virtual void SetStyleProperty(const char* key, const char* val);
	const char* GetStyleProperty(const char* key) { return _properties[key].c_str(); }

	void GetShadowColor(float *r, float *g, float *b, float *a);
	void GetShadowBorderColor(float *r, float *g, float *b, float *a);
	
	virtual void DrawBackground(float width, float height);
	virtual void DrawBorders(float width, float height);
	virtual void DrawShadow();

	enum CSS_SHADOW_TYPE
	{
		CST_INVALID = 0,
		CST_POINT,
		CST_SHADE,
		CST_NUM_TYPES
	};

	float GetShadowX(){ return _shadowx; }
	float GetShadowY(){ return _shadowy; }
	void SetShadowX(float x);
	void SetShadowY(float y);
	
protected:
	float _style_width, _style_height;

	GraphicsContext::TextureInfo _bgimage;
	float _bguv_top, _bguv_left, _bguv_bottom, _bguv_right;
	
	GraphicsContext::TextureInfo _bordertopleft, _bordertopright, _bordertop;
	GraphicsContext::TextureInfo _bordertopcorner, _borderbottomcorner;
	GraphicsContext::TextureInfo _borderleft, _borderright;
	GraphicsContext::TextureInfo _borderbottomleft, _borderbottom, _borderbottomright;
	GraphicsContext::TextureInfo _controlon, _controloff;
	GraphicsContext::TextureInfo _controlonback, _controloncap;
	GraphicsContext::TextureInfo _controloffback, _controloffcap;
	GraphicsContext::TextureInfo _bordervertical, _borderhorizontal, _bordercorner, _bordersides;
	GraphicsContext::TextureInfo _controldisabled;
	
	float _bordersizeleft, _bordersizeright, _bordersizetop, _bordersizebottom;
	float _controloncolor[4];
	float _shadowcolor[4];
	float _shadowbordercolor[4];

	Viewport* _viewport;
	CSSTextureCallback _gettexture;
	
	std::map<std::string, std::string> _properties;

	float _shadowx, _shadowy;
	CSS_SHADOW_TYPE _shadowtype;
};

}
#endif // PHUI_CSSVIEWDRAWER_H
