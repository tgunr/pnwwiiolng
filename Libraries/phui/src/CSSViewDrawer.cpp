#include "phui-precompiled-header.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#define strcasecmp _stricmp
#else
#include <OpenGL/gl.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include "phui/GraphicsContext.h"
#include "phui/CSSViewDrawer.h"
#include "phui/View.h"
#include "phui/Button.h"
#include "phui/Viewport.h"
#include "phui/Caption.h"
#include "phui/Util.h"
#include "phui/Checkbox.h"
#include "phui/TabView.h"
#include "phui/UIXML.h"
#include "phui/Scrollbar.h"
#include "phui/RadioButton.h"
#include "phui/Slider.h"
//#include "phui/ShadowView.h"
#include "phui/UIXML.h"

using namespace phui;

#define STYLE_WIDTH					"width"
#define STYLE_HEIGHT				"height"
#define STYLE_FONTSIZE				"font-size"
#define STYLE_FONTFAMILY			"font-family"
#define STYLE_BGCOLOR				"background-color"
#define STYLE_COLOR					"color"
#define STYLE_BGIMAGE				"background-image"
#define STYLE_TEXALIGN				"text-align"
#define STYLE_UV					"uv"
#define STYLE_BORDER_LEFT			"border-left"
#define STYLE_BORDER_RIGHT			"border-right"
#define STYLE_BORDER_TOP			"border-top"
#define STYLE_BORDER_BOTTOM			"border-bottom"
#define STYLE_BORDER_TOPLEFT		"border-topleft"
#define STYLE_BORDER_TOPRIGHT		"border-topright"
#define STYLE_BORDER_BOTTOMLEFT		"border-bottomleft"
#define STYLE_BORDER_BOTTOMRIGHT 	"border-bottomright"
#define STYLE_BORDER_WIDTH			"border-width"
#define STYLE_BORDER_HEIGHT			"border-height"
#define STYLE_BORDER_COLOR			"border-color"
#define STYLE_INSET					"inset"
#define STYLE_MARGIN				"margin"
#define STYLE_CONTROL_ON			"control-on"
#define STYLE_CONTROL_OFF			"control-off"
#define STYLE_CONTROL_ON_BACK		"control-on-back"
#define STYLE_CONTROL_ON_CAP		"control-on-endcap"
#define STYLE_CONTROL_OFF_BACK		"control-off-back"
#define STYLE_CONTROL_OFF_CAP		"control-off-endcap"
#define STYLE_CONTROL_ON_COLOR		"control-on-color"
#define STYLE_BORDER_VERTICAL		"border-vertical"
#define STYLE_BORDER_HORIZONTAL		"border-horizontal"
#define STYLE_BORDER_CORNER			"border-corner"
#define STYLE_BORDER_SIDES			"border-sides"
#define STYLE_BORDER_SIZE_LEFT		"border-size-left"
#define STYLE_BORDER_SIZE_RIGHT		"border-size-right"
#define STYLE_BORDER_SIZE_TOP		"border-size-top"
#define STYLE_BORDER_SIZE_BOTTOM	"border-size-bottom"
#define STYLE_BORDER_TOP_CORNER		"border-top-corner"
#define STYLE_BORDER_BOTTOM_CORNER	"border-bottom-corner"
#define STYLE_CONTROL_DISABLED		"control-disabled"
#define STYLE_TOOLTIP				"tooltip"
#define STYLE_TOOLTIP_DELAY			"tooltip-delay"
#define STYLE_SLIDE_COLOR			"slide-color"
#define STYLE_THUMB_ON_COLOR		"thumb-on-color"
#define STYLE_THUMB_OFF_COLOR		"thumb-off-color"
#define STYLE_TABALIGN				"tab-align"
#define STYLE_SHADOW_TYPE			"shadow-type"
#define STYLE_SHADOW_COLOR			"shadow-color"
#define STYLE_SHADOW_BORDER			"shadow-border-color"

class CheckboxDrawer : public CSSViewDrawer
{
public:
	CheckboxDrawer(View* v, Viewport* vp, CSSTextureCallback c) : CSSViewDrawer(v, vp, c){ _checkbox = dynamic_cast<Checkbox*>(v); }
	virtual ~CheckboxDrawer() {}
	
	virtual void Draw();
	
	Checkbox* _checkbox;
};

class TabViewDrawer : public CSSViewDrawer
{
public:
	TabViewDrawer(View* v, Viewport* vp, CSSTextureCallback c) : CSSViewDrawer(v, vp, c){ _tabview = dynamic_cast<TabView*>(v); }
	virtual ~TabViewDrawer() {}
	
	virtual void Draw();
	virtual void DrawTab(GraphicsContext* ctx, TabView::Tab &tab, int font);
	
	TabView* _tabview;
};


class ButtonDrawer : public CSSViewDrawer
{
public:
	ButtonDrawer(View* v, Viewport* vp, CSSTextureCallback c) : CSSViewDrawer(v, vp, c){ _button = dynamic_cast<Button*>(v); }
	virtual ~ButtonDrawer() {}
	
	virtual void Draw();
	
	Button* _button;
};

class ScrollbarDrawer : public CSSViewDrawer
{
public:
	ScrollbarDrawer(View* v, Viewport* vp, CSSTextureCallback c) : CSSViewDrawer(v, vp, c){ _scrollbar = dynamic_cast<Scrollbar*>(v); }
	virtual ~ScrollbarDrawer() {}
	
	virtual void Draw();
	
	Scrollbar* _scrollbar;
};

class RadioButtonDrawer : public CSSViewDrawer
{
public:
	RadioButtonDrawer(View* v, Viewport* vp, CSSTextureCallback c) : CSSViewDrawer(v, vp, c){ _radiobutton = dynamic_cast<RadioButton*>(v); }
	virtual ~RadioButtonDrawer() {}
	
	virtual void Draw();
	
	RadioButton* _radiobutton;
};

// this class doesn't draw the slide, but merely provides an interface for handling custom
// css calls for the slider: slide color, thumb on/off color
class SliderDrawer : public CSSViewDrawer
{
public:
	SliderDrawer(View* v, Viewport* vp, CSSTextureCallback c) : CSSViewDrawer(v, vp, c)
	{
		_slider = dynamic_cast<Slider*>(v);
		_enabled = false;
	}
	virtual ~SliderDrawer() {}
	
	virtual void SetStyleProperty(const char* key, const char* val);
	
	Slider* _slider;
};

/************************************************************************************************
	CSSViewDrawerFactory
*************************************************************************************************/

CSSViewDrawerFactory::CSSViewDrawerFactory(Viewport* vp, CSSTextureCallback c) 
					: _viewport(vp), _gettexture(c)
{
}

CSSViewDrawerFactory::~CSSViewDrawerFactory()
{
}

/************************************************************************************************
	CreateDrawer
*************************************************************************************************/

ViewDrawer* CSSViewDrawerFactory::CreateDrawer(View* v)
{
	ViewDrawer* drawer = 0;
	
	if(dynamic_cast<Checkbox*>(v))
		drawer = new CheckboxDrawer(v, _viewport, _gettexture);
	else if(dynamic_cast<Button*>(v))
		drawer = new ButtonDrawer(v, _viewport, _gettexture);
	else if(dynamic_cast<TabView*>(v))
		drawer = new TabViewDrawer(v, _viewport, _gettexture);
	else if(dynamic_cast<Scrollbar*>(v))
		drawer = new ScrollbarDrawer(v, _viewport, _gettexture);
	else if(dynamic_cast<RadioButton*>(v))
		drawer = new RadioButtonDrawer(v, _viewport, _gettexture);
	else if(dynamic_cast<Slider*>(v))
		drawer = new SliderDrawer(v, _viewport, _gettexture);
	else
		drawer = new CSSViewDrawer(v, _viewport, _gettexture);
	
	return drawer;
}

/************************************************************************************************
	CSSViewDrawer
*************************************************************************************************/

CSSViewDrawer::CSSViewDrawer(View* v, Viewport* vp, CSSTextureCallback c) 
			: ViewDrawer(v), _gettexture(c)
{
	_viewport = vp;
	_style_width = _style_height = 0;
	_bguv_top = _bguv_left = 0; _bguv_bottom = _bguv_right = 1;
	_bordersizeleft = _bordersizeright = _bordersizetop = _bordersizebottom = 0;
	_controloncolor[0] = _controloncolor[1] = _controloncolor[2] = _controloncolor[3] = 0;
	_shadowcolor[0] = _shadowcolor[1] = _shadowcolor[2] = _shadowcolor[3] = 0;
	_shadowbordercolor[0] = _shadowbordercolor[1] = _shadowbordercolor[2] = _shadowbordercolor[3] = 0;
	_shadowtype = CST_INVALID;
	_shadowx = _shadowy = 0;
}

CSSViewDrawer::~CSSViewDrawer()
{
}

/************************************************************************************************
	Draw
*************************************************************************************************/

void CSSViewDrawer::Draw()
{		
	float height = _view->Height();
	float width = _view->Width();

	if(_shadowcolor[3] && _shadowtype > CST_INVALID && _shadowtype < CST_NUM_TYPES)
		DrawShadow();

	DrawBackground(width, height);
	DrawBorders(width, height);
}

/************************************************************************************************
	SetStyleProperty
*************************************************************************************************/

void CSSViewDrawer::SetStyleProperty(const char* key, const char* val)
{
	_properties[key] = val;
	
	float fontsize = 0;
	float relative = 0;
	
	GraphicsContext* ctx = _viewport->GetGraphicsContext();
	std::string fontname = ctx->FontName(0);
	
	float lmargin,rmargin,bmargin,tmargin;
	_view->GetMargins(&lmargin, &rmargin, &tmargin, &bmargin);
	
	if(!strcmp(key,STYLE_WIDTH))
	{
		_style_width = _view->ScaleValue(View::SCALE_WIDTH, (float)atof(val));
	}
	else if(!strcmp(key,STYLE_HEIGHT))
	{
		_style_height = _view->ScaleValue(View::SCALE_HEIGHT, (float)atof(val));
	}
	else if(!strcmp(key,STYLE_FONTSIZE))
	{
		fontsize = _view->ScaleValue(View::SCALE_HEIGHT, (float)atof(val));
		
		// is this a relative font size
		if(val[0] == '-' || val[0] == '+')
		{
			relative = fontsize;
			fontsize = ctx->FontSize(ctx->GetDefaultFont()) + relative;
		}
	}
	else if(!strcmp(key, STYLE_FONTFAMILY))
	{
	
	}
	else if(!strcmp(key, STYLE_BGCOLOR))
	{
		float r,g,b,a;
		UIXMLColor((unsigned char*)val,&r,&g,&b,&a);
		_view->SetBackgroundColor(r,g,b,a);
	}else if(!strcmp(key, STYLE_BORDER_COLOR))
	{
		float r,g,b,a;
		UIXMLColor((unsigned char*)val,&r,&g,&b,&a);
		_view->SetBorderColor(r,g,b,a);
	}else if (!strcmp(key, STYLE_COLOR))
	{
		float r,g,b,a;
		UIXMLColor((unsigned char*)val,&r,&g,&b,&a);
		_view->SetTextColor(r,g,b,a);
	}else if (!strcmp(key, STYLE_CONTROL_ON_COLOR))
	{
		UIXMLColor((unsigned char*)val,&_controloncolor[0],&_controloncolor[1],&_controloncolor[2],&_controloncolor[3]);
	}else if(!strcmp(key, STYLE_UV))
	{
		const char* s = key;
		
		_bguv_left = (float)atof(s);
		
		s = strchr(s, ',');
		if(s)
		{
			_bguv_bottom = (float)atof(++s);
		
			s = strchr(s, ',');
			if(s)
			{
				_bguv_right = (float)atof(++s);
			
				s = strchr(s, ',');
				if(s)
					_bguv_top = (float)atof(++s);
			}
		}
	}else if(!strcmp(key, STYLE_TEXALIGN))
	{
		Caption* caption = dynamic_cast<Caption*>(_view);
		if(caption)
		{
			if(!strcmp("left", val))
				caption->SetTextAlignment(Caption::ALIGN_LEFT);
			else if(!strcmp("right", val))
				caption->SetTextAlignment(Caption::ALIGN_RIGHT);
			else if(!strcmp("center", val))
				caption->SetTextAlignment(Caption::ALIGN_CENTER);
		}
	}else if(!strcmp(key, STYLE_BORDER_WIDTH))
	{
		lmargin = rmargin = _bordersizeleft = _bordersizeright = _view->ScaleValue(View::SCALE_WIDTH, (float)atof(val));
	}else if(!strcmp(key, STYLE_BORDER_HEIGHT))
	{
		tmargin = bmargin = _bordersizetop = _bordersizebottom = _view->ScaleValue(View::SCALE_HEIGHT, (float)atof(val));
	}else if(!strcmp(key, STYLE_BORDER_SIZE_LEFT))
	{
		lmargin = _bordersizeleft = _view->ScaleValue(View::SCALE_X, (float)atof(val));
	}else if(!strcmp(key, STYLE_BORDER_SIZE_RIGHT))
	{
		rmargin = _bordersizeright = _view->ScaleValue(View::SCALE_X, (float)atof(val));
	}else if(!strcmp(key, STYLE_BORDER_SIZE_TOP))
	{
		tmargin = _bordersizetop = _view->ScaleValue(View::SCALE_Y, (float)atof(val));
	}else if(!strcmp(key, STYLE_BORDER_SIZE_BOTTOM))
	{
		bmargin = _bordersizebottom = _view->ScaleValue(View::SCALE_Y, (float)atof(val));
	}else if(!strcmp(key, STYLE_TOOLTIP))
	{
		_view->SetToolTipPath(val);
	}else if(!strcmp(key, STYLE_TOOLTIP_DELAY))
	{
		_view->SetToolTipDelay(atoi(val));
	}else if(!strcmp(key, STYLE_TABALIGN))
	{
		TabView* tabview = dynamic_cast<TabView*>(_view);
		if(tabview)
		{
			if(!strcmp("left", val))
				tabview->SetTabAlignment(TabView::ALIGN_LEFT);
			else if(!strcmp("right", val))
				tabview->SetTabAlignment(TabView::ALIGN_RIGHT);
			else if(!strcmp("bottom", val))
				tabview->SetTabAlignment(TabView::ALIGN_BOTTOM);
		}
	}else if(!strcmp(key, STYLE_SHADOW_TYPE))
	{
		if(!strcmp("point", val))
			_shadowtype = CST_POINT;
		else if(!strcmp("shade", val))
			_shadowtype = CST_SHADE;
	}else if(!strcmp(key, STYLE_SHADOW_COLOR))
	{
		UIXMLColor((unsigned char*)val,&_shadowcolor[0],&_shadowcolor[1],&_shadowcolor[2],&_shadowcolor[3]);
	}else if(!strcmp(key, STYLE_SHADOW_BORDER))
	{
		UIXMLColor((unsigned char*)val,&_shadowbordercolor[0],&_shadowbordercolor[1],&_shadowbordercolor[2],&_shadowbordercolor[3]);
	}
	
	if(_gettexture)
	{
		if(!strcmp(key, STYLE_BGIMAGE))
			_bgimage = _gettexture(val,false);
		else if(!strcmp(key, STYLE_BORDER_LEFT))
			_borderleft = _gettexture(val,true);
		else if(!strcmp(key, STYLE_BORDER_RIGHT))
			_borderright = _gettexture(val, true);
		else if(!strcmp(key, STYLE_BORDER_TOP))
			_bordertop = _gettexture(val,true);
		else if(!strcmp(key, STYLE_BORDER_BOTTOM))
			_borderbottom = _gettexture(val,true);
		else if(!strcmp(key, STYLE_BORDER_TOPLEFT))
			_bordertopleft = _gettexture(val,false);
		else if(!strcmp(key, STYLE_BORDER_TOPRIGHT))
			_bordertopright = _gettexture(val,false);
		else if(!strcmp(key, STYLE_BORDER_BOTTOMLEFT))
			_borderbottomleft = _gettexture(val,false);
		else if(!strcmp(key, STYLE_BORDER_BOTTOMRIGHT))
			_borderbottomright = _gettexture(val,false);
		else if(!strcmp(key, STYLE_CONTROL_ON))
			_controlon = _gettexture(val,false);
		else if(!strcmp(key, STYLE_CONTROL_OFF))
			_controloff = _gettexture(val,false);
		else if(!strcmp(key, STYLE_BORDER_VERTICAL))
			_bordervertical = _gettexture(val, true);
		else if(!strcmp(key, STYLE_BORDER_HORIZONTAL))
			_borderhorizontal = _gettexture(val, true);
		else if(!strcmp(key, STYLE_BORDER_CORNER))
			_bordercorner = _gettexture(val,false);
		else if(!strcmp(key, STYLE_BORDER_SIDES))
			_bordersides = _gettexture(val, true);
		else if(!strcmp(key, STYLE_CONTROL_ON_BACK))
			_controlonback = _gettexture(val,false);
		else if(!strcmp(key, STYLE_CONTROL_ON_CAP))
			_controloncap = _gettexture(val,false);
		else if(!strcmp(key, STYLE_CONTROL_OFF_BACK))
			_controloffback = _gettexture(val,false);
		else if(!strcmp(key, STYLE_CONTROL_OFF_CAP))
			_controloffcap = _gettexture(val,false);	
		else if(!strcmp(key, STYLE_BORDER_TOP_CORNER))
			_bordertopcorner = _gettexture(val,false);	
		else if(!strcmp(key, STYLE_BORDER_BOTTOM_CORNER))
			_borderbottomcorner = _gettexture(val,false);	
		else if(!strcmp(key, STYLE_CONTROL_DISABLED))
			_controldisabled = _gettexture(val,false);	
	}
	_enabled = (_bgimage.texid > -1 ||  _bordertopleft.texid  > -1 || _bordertopright.texid  > -1 || _bordertop.texid  > -1 ||
		_borderleft.texid  > -1 || _borderright.texid  > -1 ||
		_borderbottomleft.texid  > -1 || _borderbottom.texid  > -1 || _borderbottomright.texid  > -1 ||
		_controlon.texid  > -1 || _controloff.texid  > -1 ||
		_controlonback.texid  > -1 || _controloncap.texid  > -1 ||
		_controloffback.texid  > -1 || _controloffcap.texid  > -1 ||
		_bordervertical.texid  > -1 || _borderhorizontal.texid  > -1 || _bordercorner.texid  > -1 || _bordersides.texid  > -1 ||
		_bordertopcorner.texid  > -1 || _borderbottomcorner.texid  > -1 ||
		_controldisabled .texid > -1);

	if(!_enabled && (_shadowcolor[3] || _shadowbordercolor[3]))
		_enabled = true;
	
	if(_style_width > 0 || _style_height > 0)		
	{
		_view->SetPreferredSize(_style_width,_style_height);
		_view->SetMinSize(_style_width,_style_height);
	}
	
	if(fontsize > 0)
	{
		_view->SetFont(ctx->FindFont(fontname.c_str(), (int)fontsize), (int)relative);
	}

	_view->SetMargins(lmargin, rmargin, tmargin, bmargin);
}

void CSSViewDrawer::GetShadowColor(float *r, float *g, float *b, float *a)
{
	*r = _shadowcolor[0];
	*g = _shadowcolor[1];
	*b = _shadowcolor[2];
	*a = _shadowcolor[3];
}

void CSSViewDrawer::GetShadowBorderColor(float *r, float *g, float *b, float *a)
{
	*r = _shadowbordercolor[0];
	*g = _shadowbordercolor[1];
	*b = _shadowbordercolor[2];
	*a = _shadowbordercolor[3];
}

/************************************************************************************************
	Draw
*************************************************************************************************/

void CheckboxDrawer::Draw()
{
	if(!_checkbox) return;
	
	float lmargin,rmargin,tmargin,bmargin;
	_view->GetMargins(&lmargin,&rmargin,&tmargin,&bmargin);
	GraphicsContext* ctx = _view->GetGraphicsContext();
	float height = _checkbox->Height();
//	float width = _checkbox->Width();
	unsigned int font = _checkbox->GetFont();
	float fontheight = ctx->FontHeight(font);
	float x = _checkbox->Height() + (lmargin+rmargin);
	float y = (height - fontheight) / 2.0f;
	float r,g,b,a;
	_checkbox->GetTextColor(&r, &g, &b, &a);
		
	ctx->SetColor(r, g, b, a);
	ctx->EnableTexture();
	ctx->MovePenPosition(x, y);
	ctx->DrawString(font, _checkbox->WideLabel());
	ctx->MovePenPosition(-x, -y);

	x = lmargin;
	y = (height - _checkbox->Height()) / 2.0f;	

	ctx->EnableTexture();
	ctx->SetColor(1,1,1);
	ctx->DrawTexture(_checkbox->GetValue().Bool() ? _controlon : _controloff,
						 x, y, _checkbox->Height(), _checkbox->Height());	
}

void RadioButtonDrawer::Draw()
{
	if(!_radiobutton) return;
	
	float lmargin,rmargin,tmargin,bmargin;
	_view->GetMargins(&lmargin,&rmargin,&tmargin,&bmargin);
	GraphicsContext* ctx = _view->GetGraphicsContext();
	float height = _radiobutton->Height();
//	float width = _radiobutton->Width();
	unsigned int font = _radiobutton->GetFont();
	float fontheight = ctx->FontHeight(font);
	float x = _radiobutton->Height() + (lmargin+rmargin);
	float y = (height - fontheight) / 2.0f;
	float r,g,b,a;
	_radiobutton->GetTextColor(&r, &g, &b, &a);
		
	ctx->SetColor(r, g, b, a);
	ctx->EnableTexture();
	ctx->MovePenPosition(x, y);
	ctx->DrawString(font, _radiobutton->WideLabel());
	ctx->MovePenPosition(-x, -y);

	x = lmargin;
	y = (height - _radiobutton->Height()) / 2.0f;	

	ctx->EnableTexture();

	if(_radiobutton->Enabled())
		ctx->SetColor(1,1,1);
	else
		ctx->SetColor(.8f,.8f,.8f);

	ctx->DrawTexture(_radiobutton->GetValue().Bool() ? _controlon : _controloff,
						 x, y, _radiobutton->Height(), _radiobutton->Height());	
}

/************************************************************************************************
	SetStyleProperty
*************************************************************************************************/

void SliderDrawer::SetStyleProperty(const char* key, const char* val)
{
	if(!_slider) return;
	
	CSSViewDrawer::SetStyleProperty(key, val);

	float r,g,b,a;
	UIXMLColor((unsigned char*)val,&r,&g,&b,&a);
	
	if(strcasecmp(key, STYLE_SLIDE_COLOR) == 0)
		_slider->SetSlideColor(r,g,b,a);
	else if(strcasecmp(key, STYLE_THUMB_ON_COLOR) == 0)
		_slider->SetThumbOnColor(r,g,b,a);
	else if(strcasecmp(key, STYLE_THUMB_OFF_COLOR) == 0)
		_slider->SetThumbOffColor(r,g,b,a);
}

/************************************************************************************************
	TabViewDrawer
*************************************************************************************************/

#define TABXMARGIN 5
#define TABYMARGIN 2

void TabViewDrawer::Draw()
{
	float h = _view->Height();
	float w = _view->Width();
	int font = _view->GetFont();

	GraphicsContext* ctx = _view->GetGraphicsContext();

	float tabheight = ctx->FontHeight(font) + 2 * TABYMARGIN;
	float lmargin,rmargin,bmargin,tmargin;
	float r,g,b,a;

	_view->GetMargins(&lmargin, &rmargin, &tmargin, &bmargin);

	float penpos	= lmargin;
	bool tabsright	= false;

	if(_tabview->GetTabAlignment() == TabView::ALIGN_RIGHT)
	{
		DrawBackground(w - tabheight, h);
		DrawBorders(w - tabheight, h);

		glRotatef(-90,0,0,1);
		ctx->MovePenPosition(-h, w - tabheight);
		ctx->MovePenPosition(penpos, 0);
	}
	else
	{
		DrawBackground(w, h - tabheight);
		DrawBorders(w, h - tabheight);
		
		ctx->MovePenPosition(penpos, h - tabheight);
	}
	
	_tabview->GetTextColor(&r,&g,&b,&a);
	
	ctx->SetColor(r,g,b,a);
	ctx->EnableTexture();
	
	// lets draw the left side tabs
	for(int i = 0; i < _tabview->NumTabs() ; i++)
	{
		TabView::Tab &tab = _tabview->GetTab(i);
		
		if(tab.alignment == TabView::ALIGN_LEFT && tab.visible)
		{
			DrawTab(ctx, tab, font);
			ctx->MovePenPosition(tab.width, 0);
			penpos += tab.width;
			
			if(tab.index == _tabview->GetValue().Long())
				ctx->SetColor(r,g,b,a);
		}
		
		if(tab.alignment == TabView::ALIGN_RIGHT)
			tabsright = true;
	}
	
	if(tabsright)
	{
		if(_tabview->GetTabAlignment() == TabView::ALIGN_RIGHT)
			ctx->MovePenPosition((h - rmargin) - penpos, 0);
		else
			ctx->MovePenPosition((w - rmargin) - penpos, 0);
		
		// lets draw the right side tabs
		// we use a second loop because we want to keep the tab order in tact
		for(int i = _tabview->NumTabs() - 1; i >= 0 ; i--)
		{
			TabView::Tab &tab = _tabview->GetTab(i);
			
			if(tab.alignment == TabView::ALIGN_RIGHT && tab.visible)
			{
				ctx->MovePenPosition(-tab.width, 0);
				DrawTab(ctx, tab, font);
				penpos -= tab.width;
				
				if(tab.index == _tabview->GetValue().Long())
					ctx->SetColor(r,g,b,a);
			}
		}
	}
	
	if(_tabview->GetTabAlignment() == TabView::ALIGN_RIGHT)
	{
		ctx->MovePenPosition(-penpos, 0);
		ctx->MovePenPosition(h, -(w - tabheight));
		glRotatef(90,0,0,1);
	}
	else
	{
		ctx->MovePenPosition(-penpos, -(h - tabheight));
	}
	
	ctx->SetColor(1,1,1,1);
}

void TabViewDrawer::DrawTab(GraphicsContext* ctx, TabView::Tab &tab, int font)
{
	if(tab.index == _tabview->GetValue().Long())
	{
		ctx->DrawFlippedTexture(_controloncap,0,0,8,tab.height,false, false);
		ctx->DrawFlippedTexture(_controloncap,tab.width-8,0,tab.width,tab.height,true,false);
		ctx->DrawTexture(_controlonback,7,0,tab.width-7,tab.height);

		if(_controloncolor[3])
			ctx->SetColor(_controloncolor[0], _controloncolor[1], _controloncolor[2], _controloncolor[3]);
	}
	else
	{
		ctx->DrawFlippedTexture(_controloffcap,0,0,8,tab.height,false, false);
		ctx->DrawFlippedTexture(_controloffcap,tab.width-8,0,tab.width,tab.height,true,false);
		ctx->DrawTexture(_controloffback,7,0,tab.width-7,tab.height);
	}	
	
	ctx->MovePenPosition(TABXMARGIN, 2); //TABYMARGIN);
	ctx->DrawString(font, tab.name.c_str());
	ctx->MovePenPosition(-TABXMARGIN, -2); //-TABYMARGIN);
}

/************************************************************************************************
	ButtonDrawer
*************************************************************************************************/

void ButtonDrawer::Draw()
{
	if(!_button) return;
	
	GraphicsContext* ctx = _button->GetGraphicsContext();
	ctx->SetColor(1,1,1,1);

	float height = _button->Height();
	float width = _button->Width();


	ctx->EnableTexture();
	if(_button->Enabled())
		ctx->DrawTexture(_button->Pressed() || (_button->Toggles() && _button->GetValue().Bool()) ? _controlon : _controloff,0,0,width,height);	
	else if(_controldisabled.texid  > -1)	
		ctx->DrawTexture(_controldisabled,0,0,width,height);	

	int font = _button->GetFont();
	float fontheight = ctx->FontHeight(font);
	float x = (width - ctx->FontAdvance(font, _button->WideLabel())) / 2.0f;
	float y = (height - fontheight) / 2.0f;

	float r,g,b,a;
	_button->GetTextColor(&r, &g, &b, &a);
	ctx->SetColor(r,g,b,a);

	ctx->MovePenPosition(x, y);
	ctx->DrawString(font, _button->WideLabel());
	ctx->MovePenPosition(-x, -y);
	
	if(_controldisabled.texid < 0 && !_button->Enabled())
	{
		BoxVerts box;
		box_verts_with_pos(box, 0,0, width, height);
		ctx->DisableTexture();
		ctx->SetColor(1,1,1,.5);
		ctx->DrawQuad(box.verts);
	}
}

/************************************************************************************************
	ScrollbarDrawer
*************************************************************************************************/

void ScrollbarDrawer::Draw()
{
	if(!_scrollbar) return;

	View* parent = _scrollbar->Parent();
	
	if(parent)
	{
		GraphicsContext* ctx	= _scrollbar->GetGraphicsContext();
		float mx				= _scrollbar->X();
		float my				= _scrollbar->Y();
		float width				= _scrollbar->Width();
		float height			= _scrollbar->Height();
		float minbarsize		= _scrollbar->MinBarSize();
		
		ctx->MovePenPosition(mx,my);
		ctx->Clip(0,0,width,height);

		DrawBackground(width, height);

		ctx->EnableTexture();

		if(_scrollbar->Orientation() == Scrollbar::ORIENT_VERTICAL)
		{
			float ch = parent->ContentHeight();
			float ph = parent->Height();
			
			if(ch > ph)
			{
				float barheight = ph/ch*height;
				
				if(minbarsize > barheight)
					barheight = minbarsize;
				
				float y2 = height - barheight/2.0f;
				float y1 = barheight/2.0f;
				
				float y = y2 - (y2-y1) / (ch-ph) * parent->ScrollY();

				_scrollbar->SetValue(y);
				
				ctx->DrawTexture(_controlon, 0,y-barheight/2.0f, width, y+barheight/2.0f);
			}
		}

		ctx->MovePenPosition(-mx,-my);
	}
}

/************************************************************************************************
	DrawBackground
*************************************************************************************************/
	
void CSSViewDrawer::DrawBackground(float width, float height)
{
	GraphicsContext* ctx = _view->GetGraphicsContext();
	if(_bgimage.texid > -1)
	{
		ctx->EnableTexture();
	 	ctx->SetColor(1,1,1,1);
		ctx->DrawTexture(_bgimage, 0,0, width, height);	
		return;
	}

	float r,g,b,a;
	_view->GetBackgroundColor(&r,&g,&b,&a);
	if(a == 0) return;
	
 	View* parent = _view->Parent();
 	if(parent)
 	{
	 	float pr,pg,pb,pa;
		parent->GetBackgroundColor(&pr,&pg,&pb,&pa);
	
		if(pr == r && pg == g && pb == b && pa == a)
	 		return;	
 	}
 	
	BoxVerts box;
	phui::box_verts_with_bounds(box, _bordersizeleft, _bordersizebottom, width-_bordersizeright, height-_bordersizetop);
	ctx->DisableTexture();
	ctx->SetColor(r,g,b,a);
	ctx->DrawQuad(box.verts);
}

/************************************************************************************************
	DrawBorders
*************************************************************************************************/

void CSSViewDrawer::DrawBorders(float width, float height)
{
	GraphicsContext* ctx = _view->GetGraphicsContext();
	
	float r, g, b, a;
	_view->GetBorderColor(&r, &g, &b, &a);
	
	if(a) // lets draw the view's borders if any
	{
		ctx->DisableTexture();
		ctx->SetColor(r, g, b, a);
		
		BoxVerts box;
		box_verts_with_pos(box, 0, 0, _view->Width(), _view->Height());
		ctx->DrawLineStrip(4, box.verts, true);
	}
	
	bool drawn[3][3] = {{false,false,false},{false,false,false},{false,false,false}};

	ctx->EnableTexture();
	ctx->SetColor(1,1,1,1);

	if(_bordertopleft.texid > -1)
	{
		if(!drawn[0][0])
		{
			ctx->DrawTexture(_bordertopleft, 0, height-_bordersizetop, _bordersizeleft, height);	
			drawn[0][0] = true;
		}
	}
	
	if(_bordertopright.texid > -1)
	{
		if(!drawn[0][2])
		{
			ctx->DrawTexture(_bordertopright, width-_bordersizeright, height-_bordersizetop, width, height);	
			drawn[0][2] = true;
		}
	}
	
	if(_borderbottomright.texid > -1)
	{
		if(!drawn[2][2])
		{
			ctx->DrawTexture(_borderbottomright, width-_bordersizeright, 0, width, _bordersizebottom);	
			drawn[2][2] = true;
		}
	}

	if(_borderbottomleft.texid > -1)
	{
		if(!drawn[2][0])
		{
			ctx->DrawTexture(_borderbottomleft, 0, 0, _bordersizeleft, _bordersizebottom);	
			drawn[2][0] = true;
		}
	}
	if(_bordertopcorner.texid > -1)
	{
		if(!drawn[0][0])
		{
			ctx->DrawFlippedTexture(_bordertopcorner, 0, height-_bordersizetop, _bordersizeleft, height,false,false);	
			drawn[0][0] = true;
		}
		
		if(!drawn[0][2])
		{
			ctx->DrawFlippedTexture(_bordertopcorner, width-_bordersizeright, height-_bordersizetop, width, height,true,false);	
			drawn[0][2] = true;
		}
	}
	
	if(_borderbottomcorner.texid > -1)
	{
		if(!drawn[2][0])
		{
			ctx->DrawFlippedTexture(_borderbottomcorner, width-_bordersizeright, 0, width, _bordersizebottom,false,false);		
			drawn[2][0] = true;
		}
		
		if(!drawn[2][2])
		{
			ctx->DrawFlippedTexture(_borderbottomcorner, 0, 0, _bordersizeleft, _bordersizebottom,true,false);	
			drawn[2][2] = true;
		}
	}
	
	if(_bordercorner.texid > -1)
	{
		if(!drawn[0][0])
		{
			ctx->DrawRotatedTexture(_bordercorner, 0, height-_bordersizetop, _bordersizeleft, height,0);	
			drawn[0][0] = true;
		}
		
		if(!drawn[0][2])
		{
			ctx->DrawRotatedTexture(_bordercorner, width-_bordersizeright, height-_bordersizetop, width, height,1);	
			drawn[0][2] = true;
		}
		
		if(!drawn[2][2])
		{
			ctx->DrawRotatedTexture(_bordercorner, width-_bordersizeright, 0, width, _bordersizebottom,2);	
			drawn[2][2] = true;
		}
		
		if(!drawn[2][0])
		{
			ctx->DrawRotatedTexture(_bordercorner, 0, 0, _bordersizeleft, _bordersizebottom,3);	
			drawn[2][0] = true;
		}
	}
			
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if(_borderleft.texid > -1)
	{
		if(!drawn[1][0])
		{
			ctx->DrawTexture(_borderleft, 0, _bordersizebottom, _bordersizeleft, height-_bordersizetop);		
			drawn[1][0] = true;
		}
	}
	if(_borderright.texid > -1)
	{
		if(!drawn[1][2])
		{
			ctx->DrawTexture(_borderright, width-_bordersizeright, _bordersizebottom, width, height-_bordersizetop);	
			drawn[1][2] = true;
		}
	}
	if(_bordertop.texid > -1)
	{
		if(!drawn[0][1])
		{
			ctx->DrawTexture(_bordertop, _bordersizeleft, height-_bordersizetop, width-_bordersizeright, height);	
			drawn[0][1] = true;
		}
	}
	
	if(_borderbottom.texid > -1)
	{
		if(!drawn[2][1])
		{
			ctx->DrawTexture(_borderbottom, _bordersizeleft, 0, width-_bordersizeright, _bordersizebottom);	
			drawn[2][1] = true;
		}
	}
	
	if(_bordervertical.texid > -1)
	{
		if(!drawn[1][0])
		{
			ctx->DrawRotatedTexture(_bordervertical, 0, _bordersizebottom, _bordersizeleft, height-_bordersizetop,0);		
			drawn[1][0] = true;
		}
		
		if(!drawn[1][2])
		{
			ctx->DrawRotatedTexture(_bordervertical, width-_bordersizeright, _bordersizebottom, width, height-_bordersizetop,2);	
			drawn[1][2] = true;
		}
	}
	
	if(_borderhorizontal.texid > -1)
	{
		if(!drawn[0][1])
		{
			ctx->DrawRotatedTexture(_borderhorizontal, _bordersizeleft, height-_bordersizetop, width-_bordersizeright, height,0);	
			drawn[0][1] = true;
		}
		
		if(!drawn[2][1])
		{
			ctx->DrawRotatedTexture(_borderhorizontal, _bordersizeleft, 0, width-_bordersizeright, _bordersizebottom,2);	
			drawn[2][1] = true;
		}
	}
	
	if(_bordersides.texid > -1)
	{
		
		if(!drawn[1][0])
		{
			ctx->DrawRotatedTexture(_bordersides, 0, _bordersizebottom, _bordersizeleft, height-_bordersizetop,0);		
			drawn[1][0] = true;
		}
		
		if(!drawn[0][1])
		{
			ctx->DrawRotatedTexture(_bordersides, _bordersizeleft, height-_bordersizetop, width-_bordersizeright, height,1);	
			drawn[0][1] = true;
		}
		
		if(!drawn[1][2])
		{
			ctx->DrawRotatedTexture(_bordersides, width-_bordersizeright, _bordersizebottom, width, height-_bordersizetop,2);	
			drawn[1][2] = true;
		}
		
		if(!drawn[2][1])
		{
			ctx->DrawRotatedTexture(_bordersides, _bordersizeleft, 0, width-_bordersizeright, _bordersizebottom,3);	
			drawn[2][1] = true;
		}
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	
}

/************************************************************************************************
	DrawShadow
*************************************************************************************************/

void CSSViewDrawer::DrawShadow()
{
	View* parent = _view->Parent();
	GraphicsContext* ctx = _view->GetGraphicsContext();

	// parents do the actual shadow drawing,
	// otherwise the shadow gets clipped
	if(!parent || !ctx)	return;
	
	ctx->MovePenPosition(-_view->X(), -_view->Y());
	parent->Clip();

	switch(_shadowtype)
	{
		case CST_POINT:
			parent->DrawPointShadow(_view);
		break;
		case CST_SHADE:
			parent->DrawShadeShadow(_view);
		break;
		default:
			// do nothing
		break;
	}

	ctx->MovePenPosition(_view->X(), _view->Y());
	_view->Clip();
}

void CSSViewDrawer::SetShadowX(float x)
{
	_shadowx = x;
}

void CSSViewDrawer::SetShadowY(float y)
{
	_shadowy = y;
}