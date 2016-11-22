#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: lightTool.cpp
// Description: Tool to view light values for sun/moon, etc.
// Invoked with ".lighttool" in BETA_RELEASE only
//////////////////////////////////////////////////////////////////////
#include "tools/lightTool.h"
#include "pi/pi.h"
#include "ld/ldtex.h"
#include "draw/draw2d.h"
#include "ui/uiMain.h"
#include "include/vars.h"
#include "trn/trntod.h"
#include "rn/rnmain.h"
#include "trn/trncloud.h"
#include "trn/trndata.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "fio/fio.h"

#if platOS == OS_WIN32
#include <io.h>
#else platOS == OS_MACOS
#include <unistd.h>
#define _access access
#endif

using namespace std;
using namespace phui;

static SINT32 xmlinputmatch(const SCHAR* name) {return 1;}

static void _saveLightColors(CMyButton* button);
static void _onfoglinear(MyRadioButton* button);
static void _onfogexp(MyRadioButton* button);
static void _onfogexp2(MyRadioButton* button);

static void _loadLogFile(SCHAR* log);

//////////////////////////////////////////////////////////////////////
// Function: MyRadioButton
//////////////////////////////////////////////////////////////////////
MyRadioButton::MyRadioButton()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~MyRadioButton
//////////////////////////////////////////////////////////////////////
MyRadioButton::~MyRadioButton()
{
}

//////////////////////////////////////////////////////////////////////
// Function: MouseUp
//////////////////////////////////////////////////////////////////////
bool MyRadioButton::MouseUp(float x, float y, int button)
{
    if(_callback)
    {
        _callback(this);
    }

    return RadioButton::MouseUp(x, y, button);
}

//////////////////////////////////////////////////////////////////////
// Function: CMyButton
//////////////////////////////////////////////////////////////////////
CMyButton::CMyButton()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~CMyButton
//////////////////////////////////////////////////////////////////////
CMyButton::~CMyButton()
{
}

//////////////////////////////////////////////////////////////////////
// Function: MouseUp
//////////////////////////////////////////////////////////////////////
bool CMyButton::MouseUp(float x, float y, int button)
{
    if(m_callback)
    {
        m_callback(this);
    }

    return Button::MouseUp(x, y, button);
}

//////////////////////////////////////////////////////////////////////
// Function: CMySlider
//////////////////////////////////////////////////////////////////////
CMySlider::CMySlider(float min, float max, int type) : Slider()
{
    m_min = min;
    m_max = max;
    m_type = type;
}

//////////////////////////////////////////////////////////////////////
// Function: _setskycolor
//////////////////////////////////////////////////////////////////////
void _setskycolor(int type, float color)
{
    TERRAINSKY* s = trndataGetSkyData(0);

    switch(type)
    {
    case IDE_SKY_COLOR_RED:                 s->_skycolor.r = color; break;
    case IDE_SKY_COLOR_GREEN:               s->_skycolor.g = color; break;
    case IDE_SKY_COLOR_BLUE:                s->_skycolor.b = color; break;
    case IDE_SKY_COLOR_NIGHT_RED:           s->_skycolornight.r = color; break;
    case IDE_SKY_COLOR_NIGHT_GREEN:         s->_skycolornight.g = color; break;
    case IDE_SKY_COLOR_NIGHT_BLUE:          s->_skycolornight.b = color; break;
    case IDE_SKY_CLEAR_AMBIENT_RED:         s->_skyclearambient.r = color; break;
    case IDE_SKY_CLEAR_AMBIENT_GREEN:       s->_skyclearambient.g = color; break;
    case IDE_SKY_CLEAR_AMBIENT_BLUE:        s->_skyclearambient.b = color; break;
    case IDE_SKY_NIGHT_AMBIENT_RED:         s->_skynightambient.r = color; break;
    case IDE_SKY_NIGHT_AMBIENT_GREEN:       s->_skynightambient.g = color; break;
    case IDE_SKY_NIGHT_AMBIENT_BLUE:        s->_skynightambient.b = color; break;
    case IDE_SUN_NOON_RED_COLOR:            s->_sunnoon.r = color; break;
    case IDE_SUN_NOON_GREEN_COLOR:          s->_sunnoon.g = color; break;
    case IDE_SUN_NOON_BLUE_COLOR:           s->_sunnoon.b = color; break;
    case IDE_SUN_ITSELF_NOON_RED_COLOR:     s->_sunitselfnoon.r = color; break;
    case IDE_SUN_ITSELF_NOON_GREEN_COLOR:   s->_sunitselfnoon.g = color; break;
    case IDE_SUN_ITSELF_NOON_BLUE_COLOR:    s->_sunitselfnoon.b = color; break;
    case IDE_SUN_NIGHT_RED_COLOR:           s->_sunnight.r = color; break;
    case IDE_SUN_NIGHT_GREEN_COLOR:         s->_sunnight.g = color; break;
    case IDE_SUN_NIGHT_BLUE_COLOR:          s->_sunnight.b = color; break;
    case IDE_SUNSET_RED_COLOR:              s->_sunset.r = color; break;
    case IDE_SUNSET_GREEN_COLOR:            s->_sunset.g = color; break;
    case IDE_SUNSET_BLUE_COLOR:             s->_sunset.b = color; break;
    case IDE_HORIZ_COLOR_RED:               s->_horizon.r = color; break;
    case IDE_HORIZ_COLOR_GREEN:             s->_horizon.g = color; break;
    case IDE_HORIZ_COLOR_BLUE:              s->_horizon.b = color; break;
    case IDE_MOON_RED_COLOR:                s->_moon.r = color; break;
    case IDE_MOON_GREEN_COLOR:              s->_moon.g = color; break;
    case IDE_MOON_BLUE_COLOR:               s->_moon.b = color; break;
    case IDE_MOON_DAY_RED_COLOR:            s->_moonday.r = color; break;
    case IDE_MOON_DAY_GREEN_COLOR:          s->_moonday.g = color; break;
    case IDE_MOON_DAY_BLUE_COLOR:           s->_moonday.b = color; break;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: MouseDrag
//////////////////////////////////////////////////////////////////////
bool CMySlider::MouseDrag(float x, float y, int button, bool start)
{
    Slider::MouseDrag(x, y, button, start);

//  TERRAINSKY* sky =
	trndataGetSkyData(0);

    {
        m_value = _value.Float();

        char s[32];

        switch(m_type)
        {
        case type_skylight:
            {
                strprintf(s, "%f", m_value * m_max);
                c->SetValue(s);

                _setskycolor(m_color, m_value * m_max);
                break;
            }
        case type_fogstart:
            {
                strprintf(s, "%f", m_value * m_max);
                c->SetValue(s);
                FogStart = piFloatLikeLong(m_value * m_max);
                rnmainSetDefaultFogStart(m_value * m_max);
                break;
            }
        case type_fogend:
            {
                strprintf(s, "%f", m_value * m_max);
                c->SetValue(s);
                rnmainSetDefaultFogEnd(m_value * m_max);
                break;
            }
        case type_fogcolorR:
            {
                strprintf(s, "%f", m_value * m_max);
                c->SetValue(s);

                UINT32 color = rnmainGetDefaultFogColor();
                FLOAT32 r = m_value * m_max;
                FLOAT32 g = piRGBA_GETGREEN(color) * colorONEOVER255;
                FLOAT32 b = piRGBA_GETBLUE(color) * colorONEOVER255;

                color = piRGBA(r, g, b, 1.0f);
                rnmainSetDefaultFogColor(color);
                break;
           }
        case type_fogcolorG:
            {
                strprintf(s, "%f", m_value * m_max);
                c->SetValue(s);

                UINT32 color = rnmainGetDefaultFogColor();
                FLOAT32 g = m_value * m_max;
                FLOAT32 r = piRGBA_GETRED(color) * colorONEOVER255;
                FLOAT32 b = piRGBA_GETBLUE(color) * colorONEOVER255;

                color = piRGBA(r, g, b, 1.0f);
                rnmainSetDefaultFogColor(color);
                break;
            }
        case type_fogcolorB:
            {
                strprintf(s, "%f", m_value * m_max);
                c->SetValue(s);

                UINT32 color = rnmainGetDefaultFogColor();
                FLOAT32 b = m_value * m_max;
                FLOAT32 r = piRGBA_GETRED(color) * colorONEOVER255;
                FLOAT32 g = piRGBA_GETGREEN(color) * colorONEOVER255;

                color = piRGBA(r, g, b, 1.0f);
                rnmainSetDefaultFogColor(color);
                break;
           }
       }
    }

    trnskyChangeSetting(trnskyGetSetting(), trnskyGetFogOption(), true);

    return true;
}

//////////////////////////////////////////////////////////////////////
// Function: setSlider
//////////////////////////////////////////////////////////////////////
void CMySlider::setSlider(float value)
{
    m_value = value;

    SetValue(m_value / m_max);
}

//////////////////////////////////////////////////////////////////////
// Function: CLightTool
//////////////////////////////////////////////////////////////////////
CLightTool::CLightTool()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~CLightTool
//////////////////////////////////////////////////////////////////////
CLightTool::~CLightTool()
{
    Kill();
}

//////////////////////////////////////////////////////////////////////
// Function: Kill
//////////////////////////////////////////////////////////////////////
void CLightTool::Kill()
{
    vector<CMySlider *>::iterator sB = m_vSliders.begin();
    vector<CMySlider *>::iterator sFREE;

    m_viewport = uiViewport();

    while(sB != m_vSliders.end())
    {
        sFREE = sB;
        ++sB;

        m_viewport->RemoveChild((*sFREE)->c);
        m_viewport->RemoveChild(*sFREE);

        delete (*sFREE)->c;
        delete (*sFREE);
    }

    vector<Caption *>::iterator cB = m_vCaptions.begin();
    vector<Caption *>::iterator cFREE;

    while(cB !=  m_vCaptions.end())
    {
        cFREE = cB;
        ++cB;

        m_viewport->RemoveChild(*cFREE);

        delete (*cFREE);
    }

    vector<CMyButton *>::iterator bB = m_vButtons.begin();
    vector<CMyButton *>::iterator bFREE;

    while(bB !=  m_vButtons.end())
    {
        bFREE = bB;
        ++bB;

        m_viewport->RemoveChild(*bFREE);

        delete (*bFREE);
    }

    vector<MyRadioButton *>::iterator b = _radioButtons.begin();
    vector<MyRadioButton *>::iterator free;

    while(b !=  _radioButtons.end())
    {
        free = b;
        ++b;

        m_viewport->RemoveChild(*free);

        delete (*free);
    }

    _radioButtons.clear();
    m_vSliders.clear();
    m_vCaptions.clear();
    m_vButtons.clear();
}

//////////////////////////////////////////////////////////////////////
// Function: createButton
//////////////////////////////////////////////////////////////////////
void CLightTool::createButton(int x, int y, void (*callback)(CMyButton*), char* caption)
{
    CMyButton* b = new CMyButton;

    m_viewport = uiViewport();
	m_viewport->AddChild(b);

    b->MoveTo((float)x, (float)y);
    b->SetLabel(caption);
    b->setCallback(callback);

    b->ResizeTo(64, 32);

    b->Show();

    m_vButtons.push_back(b);
}

//////////////////////////////////////////////////////////////////////
// Function: createButton
//////////////////////////////////////////////////////////////////////
void CLightTool::createRadioButton(int x, int y, void (*callback)(MyRadioButton*), char* caption)
{
    MyRadioButton* b = new MyRadioButton;

    m_viewport = uiViewport();
	m_viewport->AddChild(b);

    b->MoveTo((float)x, (float)y);
    b->SetLabel(caption);
    b->setCallback(callback);

    GLint param[1];

    glGetIntegerv(GL_FOG_MODE, param);

    switch(param[0])
    {
    case GL_LINEAR:
        {
            if(!strncmp(caption, "LINEAR", 6))
            {
                _buttonenabled = 1;
            }

            break;
        }
    case GL_EXP:
        {
            if(!strncmp(caption, "GL_EXP", 6))
            {
                _buttonenabled = 2;
            }

            break;
        }
    case GL_EXP2:
        {
            if(!strncmp(caption, "GL_EXP2", 7))
            {
                _buttonenabled = 3;
            }

            break;
        }
    }

    b->ResizeTo(16, 16);

    b->Show();

    _radioButtons.push_back(b);
}

//////////////////////////////////////////////////////////////////////
// Function: refresh
//////////////////////////////////////////////////////////////////////
void CLightTool::refresh()
{
#if(0)
    vector<CMySlider *>::iterator sB = m_vSliders.begin();

    int i = 0;
    char str[32];

    while(sB != m_vSliders.end())
    {
        switch((*sB)->getType())
        {
        case CMySlider::type_skylight:
            {
                (*sB)->setSlider(trnskyGetColor(i));

                strprintf(str, "%f", trnskyGetColor(i++));

                (*sB)->c->SetValue(str);

                break;
            }
        }

        ++sB;
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: createSlider
//////////////////////////////////////////////////////////////////////
void CLightTool::createSlider(int x,
                              int y,
                              float min,
                              float max,
                              ENUM_TERRAIN_COLOR enumColor)
{
    CMySlider* s = new CMySlider(min, max, CMySlider::type_skylight);

    m_viewport = uiViewport();
	m_viewport->AddChild(s);

    s->Show();
    s->ResizeTo(100, 16);

    s->SetBackgroundColor(0, 0, 0, 0);
    s->MoveTo((float)x, (float)y);
    s->SetMinValue(0);
    s->SetMaxValue(1);
    s->setSlider(trnskyGetColor(enumColor));

    s->setEnumColor(enumColor);

    Caption* c = new Caption();

    s->c = c;

    c->SetTextColor(1, 1, 1, 1);

    c->ResizeTo(32, 16);
    c->SetBackgroundColor(0, 0, 0, 0);
    c->MoveTo((float)x + 100, (float)y);

    char str[32];
    strprintf(str, "%f", trnskyGetColor(enumColor));

    c->SetValue(str);
    c->SetTextAlignment(Caption::ALIGN_LEFT);
    c->Show();

    m_viewport->AddChild(c);

    m_vSliders.push_back(s);
}

//////////////////////////////////////////////////////////////////////
// Function: createSlider2
//////////////////////////////////////////////////////////////////////
void CLightTool::createSlider2(int x,
                               int y,
                               int shaderparam,
                               float min,
                               float max)
{
}

//////////////////////////////////////////////////////////////////////
// Function: createFogSlider
//////////////////////////////////////////////////////////////////////
void CLightTool::createFogSlider(int x,
                                 int y,
                                 float min,
                                 float max,
                                 int type)
{
    CMySlider* s = new CMySlider(min, max, type);

    m_viewport = uiViewport();
	m_viewport->AddChild(s);

    s->Show();
    s->ResizeTo(100, 16);

    s->SetBackgroundColor(0, 0, 0, 0);
    s->MoveTo((float)x, (float)y);

    char str[32];

    switch(type)
    {
    case CMySlider::type_fogstartdensity:
        {
            float start;
            float end;
            trnskyGetFogDensity(start, end);
            s->SetMinValue(0);
            s->SetMaxValue(0.0005f);
            s->setSlider(start);
            strprintf(str, "%f", start);
            break;
        }
    case CMySlider::type_fogenddensity:
        {
            float start;
            float end;
            trnskyGetFogDensity(start, end);
            s->SetMinValue(0);
            s->SetMaxValue(0.0005f);
            s->setSlider(end);
            strprintf(str, "%f", end);
            break;
        }
    case CMySlider::type_fogstart:
        {
            float value = rnmainGetDefaultFogStart();
            s->SetMinValue(0);
            s->SetMaxValue(1.0f);
            s->setSlider(value);
            strprintf(str, "%f", value);
            break;
        }
    case CMySlider::type_fogend:
        {
            float value = rnmainGetDefaultFogEnd();
            s->SetMinValue(0);
            s->SetMaxValue(1.0f);
            s->setSlider(value);
            strprintf(str, "%f", value);
            break;
       }
    case CMySlider::type_fogcolorR:
        {
            float value = piRGBA_GETRED(rnmainGetDefaultFogColor()) * colorONEOVER255;
            s->setSlider(value);
            strprintf(str, "%f", value);
            break;
        }
    case CMySlider::type_fogcolorG:
        {
            float value = piRGBA_GETGREEN(rnmainGetDefaultFogColor()) * colorONEOVER255;
            s->setSlider(value);
            strprintf(str, "%f", value);
            break;
        }
    case CMySlider::type_fogcolorB:
        {
            float value = piRGBA_GETBLUE(rnmainGetDefaultFogColor()) * colorONEOVER255;
            s->setSlider(value);
            strprintf(str, "%f", value);
            break;
        }
    }

    Caption* c = new Caption();

    s->c = c;

    c->SetTextColor(1, 1, 1, 1);

    c->ResizeTo(32, 16);
    c->SetBackgroundColor(0, 0, 0, 0);
    c->MoveTo((float)x + 100, (float)y);

    c->SetValue(str);
    c->SetTextAlignment(Caption::ALIGN_LEFT);
    c->Show();

    m_viewport->AddChild(c);

    m_vSliders.push_back(s);
}

//////////////////////////////////////////////////////////////////////
// Function: setCaption
//////////////////////////////////////////////////////////////////////
void CLightTool::setCaption(char* szCaption, int x, int y, float r, float g, float b)
{
    Caption* c = new Caption();

    m_viewport = uiViewport();
	m_viewport->AddChild(c);

    c->Show();
    c->SetTextColor(r, g, b);

    c->ResizeTo(120, 16);
    c->SetBackgroundColor(0, 0, 0, 0);
    c->MoveTo((float)x, (float)y);
    c->SetValue(szCaption);
    c->SetTextAlignment(Caption::ALIGN_LEFT);

    m_vCaptions.push_back(c);
}

//////////////////////////////////////////////////////////////////////
// Function: lightToolCreate
//////////////////////////////////////////////////////////////////////
void lightToolCreate(bool refresh)
{
    static LOGICAL toggle = FALSE;
    static CLightTool* lt = NULL;

    if(refresh)
    {
        if(lt)
            lt->refresh();

        return;
    }

    toggle = !toggle;

    if(toggle)
    {
        if(lt)
        {
            delete lt;
        }

        lt = new CLightTool();

        _loadLogFile("lightlog");
        trnskyChangeSetting(trnskyGetSetting(), trnskyGetFogOption(), true);

        SINT32 h = piGetHeight() - 64;
        SINT32 x = 64;
        SINT32 xoffset = 142;

        lt->setCaption("SKY COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_COLOR_RED);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_COLOR_GREEN);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_COLOR_BLUE);

        h = piGetHeight() - 64;
        x += xoffset;

        lt->setCaption("SKY COLOR NIGHT", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_COLOR_NIGHT_RED);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_COLOR_NIGHT_GREEN);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_COLOR_NIGHT_BLUE);

        h = piGetHeight() - 64;
        x += xoffset;

        lt->setCaption("SKY CLEAR AMBIENT", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_CLEAR_AMBIENT_RED);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_CLEAR_AMBIENT_GREEN);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_CLEAR_AMBIENT_BLUE);

        h = piGetHeight() - 64;
        x += xoffset;

        lt->setCaption("SKY NIGHT AMBIENT", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_NIGHT_AMBIENT_RED);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_NIGHT_AMBIENT_GREEN);
        lt->createSlider(x, h-=32, 0, 255, IDE_SKY_NIGHT_AMBIENT_BLUE);

        h = piGetHeight() - 196;
        x = 64;

        lt->setCaption("SUN NOON COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_NOON_RED_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_NOON_GREEN_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_NOON_BLUE_COLOR);

        h = piGetHeight() - 196;
        x += xoffset;

        lt->setCaption("SUN ITSELF COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_ITSELF_NOON_RED_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_ITSELF_NOON_GREEN_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_ITSELF_NOON_BLUE_COLOR);

        h = piGetHeight() - 196;
        x += xoffset;

        lt->setCaption("SUN NIGHT COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_NIGHT_RED_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_NIGHT_GREEN_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUN_NIGHT_BLUE_COLOR);

        h = piGetHeight() - 196;
        x += xoffset;

        lt->setCaption("SUNSET COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 1, IDE_SUNSET_RED_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUNSET_GREEN_COLOR);
        lt->createSlider(x, h-=32, 0, 1, IDE_SUNSET_BLUE_COLOR);

        h = piGetHeight() - 328;
        x = 64;

        lt->setCaption("HORIZON COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_HORIZ_COLOR_RED);
        lt->createSlider(x, h-=32, 0, 255, IDE_HORIZ_COLOR_GREEN);
        lt->createSlider(x, h-=32, 0, 255, IDE_HORIZ_COLOR_BLUE);

        h = piGetHeight() - 328;
        x += xoffset;

        lt->setCaption("MOON COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_MOON_RED_COLOR);
        lt->createSlider(x, h-=32, 0, 255, IDE_MOON_GREEN_COLOR);
        lt->createSlider(x, h-=32, 0, 255, IDE_MOON_BLUE_COLOR);

        h = piGetHeight() - 328;
        x += xoffset;

        lt->setCaption("MOON DAY COLOR", x, h, 1, 1, 0);

        lt->createSlider(x, h-=32, 0, 255, IDE_MOON_DAY_RED_COLOR);
        lt->createSlider(x, h-=32, 0, 255, IDE_MOON_DAY_GREEN_COLOR);
        lt->createSlider(x, h-=32, 0, 255, IDE_MOON_DAY_BLUE_COLOR);

        h = piGetHeight() - 64;
        x += xoffset;
        x += xoffset;

        lt->createRadioButton(x + 0,  h, _onfoglinear, "LINEAR");
        lt->createRadioButton(x + 32, h, _onfogexp, "EXP");
        lt->createRadioButton(x + 64, h, _onfogexp2, "EXP2");

        h = piGetHeight() - 96;

        lt->setCaption("FOG START DENSITY", x, h, 1, 1, 0);
        lt->createFogSlider(x, h-=24, 0, 0.05f, CMySlider::type_fogstartdensity);

        lt->setCaption("FOG END DENSITY", x, h-=32, 1, 1, 0);
        lt->createFogSlider(x, h-=24, 0, 0.05f, CMySlider::type_fogenddensity);

        h = piGetHeight() - 224;
        lt->setCaption("FOG START", x, h, 1, 1, 0);
        lt->createFogSlider(x, h-=32, 0, 10000, CMySlider::type_fogstart);

        h = piGetHeight() - 288;
        lt->setCaption("FOG END", x, h, 1, 1, 0);
        lt->createFogSlider(x, h-=32, 0, 10000, CMySlider::type_fogend);

        x = 256;
        h = piGetHeight() - 32;

        lt->createButton(x, h, _saveLightColors, "SAVE");
    }
    else
    {
        if(lt)
        {
            delete lt;
            lt = NULL;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _saveLightColors
//////////////////////////////////////////////////////////////////////
void _saveLightColors(CMyButton* button)
{
    TERRAINSKY* s = trndataGetSkyData(0);

    SCHAR fileName[64] = "lightlog.xml";
    SCHAR* p = "lightlog";
    SINT32 count = 1;
    SINT32 max = 64;

    while(max && _access(fileName, 0) == 0)
    {
        strprintf(fileName, "%s%d.xml", p, count++);
        max--;
    }

    FILE* fp = fopen(fileName, "w");

    if(!fp)
    {
        return;
    }

    fprintf(fp, "%s\n", "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>");
    fprintf(fp, "%s\n", "<!-- ==================================================== -->");
    fprintf(fp, "%s\n", "<!-- CUT AND PASTE BELOW INTO terrain/xml/light_cloud.xml -->");
    fprintf(fp, "%s\n", "<!-- ==================================================== -->");
    fprintf(fp, "%s\n", "<TerrainSky>");
    fprintf(fp, "    %s\n", "<sky_color>");
    fprintf(fp, "        <red>%f</red>\n", s->_skycolor.r);
    fprintf(fp, "        <green>%f</green>\n", s->_skycolor.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_skycolor.b);
    fprintf(fp, "    %s\n", "</sky_color>");
    fprintf(fp, "    %s\n", "<sky_color_night>");
    fprintf(fp, "        <red>%f</red>\n", s->_skycolornight.r);
    fprintf(fp, "        <green>%f</green>\n", s->_skycolornight.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_skycolornight.b);
    fprintf(fp, "    %s\n", "</sky_color_night>");
    fprintf(fp, "    %s\n", "<sky_clear_ambient>");
    fprintf(fp, "        <red>%f</red>\n", s->_skyclearambient.r);
    fprintf(fp, "        <green>%f</green>\n", s->_skyclearambient.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_skyclearambient.b);
    fprintf(fp, "    %s\n", "</sky_clear_ambient>");
    fprintf(fp, "    %s\n", "<sky_night_ambient>");
    fprintf(fp, "        <red>%f</red>\n", s->_skynightambient.r);
    fprintf(fp, "        <green>%f</green>\n", s->_skynightambient.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_skynightambient.b);
    fprintf(fp, "    %s\n", "</sky_night_ambient>");
    fprintf(fp, "    %s\n", "<sun_noon>");
    fprintf(fp, "        <red>%f</red>\n", s->_sunnoon.r);
    fprintf(fp, "        <green>%f</green>\n", s->_sunnoon.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_sunnoon.b);
    fprintf(fp, "    %s\n", "</sun_noon>");
    fprintf(fp, "    %s\n", "<sun_itself_noon>");
    fprintf(fp, "        <red>%f</red>\n", s->_sunitselfnoon.r);
    fprintf(fp, "        <green>%f</green>\n", s->_sunitselfnoon.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_sunitselfnoon.b);
    fprintf(fp, "    %s\n", "</sun_itself_noon>");
    fprintf(fp, "    %s\n", "<sun_night>");
    fprintf(fp, "        <red>%f</red>\n", s->_sunnight.r);
    fprintf(fp, "        <green>%f</green>\n", s->_sunnight.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_sunnight.b);
    fprintf(fp, "    %s\n", "</sun_night>");
    fprintf(fp, "    %s\n", "<sunset>");
    fprintf(fp, "        <red>%f</red>\n", s->_sunset.r);
    fprintf(fp, "        <green>%f</green>\n", s->_sunset.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_sunset.b);
    fprintf(fp, "    %s\n", "</sunset>");
    fprintf(fp, "    %s\n", "<horizon>");
    fprintf(fp, "        <red>%f</red>\n", s->_horizon.r);
    fprintf(fp, "        <green>%f</green>\n", s->_horizon.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_horizon.b);
    fprintf(fp, "    %s\n", "</horizon>");
    fprintf(fp, "    %s\n", "<moon>");
    fprintf(fp, "        <red>%f</red>\n", s->_moon.r);
    fprintf(fp, "        <green>%f</green>\n", s->_moon.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_moon.b);
    fprintf(fp, "    %s\n", "</moon>");
    fprintf(fp, "    %s\n", "<moon_day>");
    fprintf(fp, "        <red>%f</red>\n", s->_moonday.r);
    fprintf(fp, "        <green>%f</green>\n", s->_moonday.g);
    fprintf(fp, "        <blue>%f</blue>\n", s->_moonday.b);
    fprintf(fp, "    %s\n", "</moon_day>");
    fprintf(fp, "%s\n", "</TerrainSky>");

    fclose(fp);
}

//////////////////////////////////////////////////////////////////////
// Function: _setcolor
// Description:
//////////////////////////////////////////////////////////////////////
void _setcolor(xmlNodePtr root, piCOLORVALUE* color)
{
    xmlNodePtr node = root;

    for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"red", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                color->r = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }
        }
        else if(!xmlStrcmp((xmlChar*)"green", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                color->g = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }
        }
        else if(!xmlStrcmp((xmlChar*)"blue", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                color->b = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _skyelements
// Description:
//////////////////////////////////////////////////////////////////////
void _skyelements(xmlNodePtr root)
{
    xmlNodePtr node = root;

    TERRAINSKY* sky = trndataGetSkyData(0);

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_color", node->name))
        {
            _setcolor(node->children, &sky->_skycolor);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_color_night", node->name))
        {
            _setcolor(node->children, &sky->_skycolornight);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_clear_ambient", node->name))
        {
            _setcolor(node->children, &sky->_skyclearambient);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_night_ambient", node->name))
        {
            _setcolor(node->children, &sky->_skynightambient);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sun_noon", node->name))
        {
            _setcolor(node->children, &sky->_sunnoon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sun_itself_noon", node->name))
        {
            _setcolor(node->children, &sky->_sunitselfnoon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sun_night", node->name))
        {
            _setcolor(node->children, &sky->_sunnight);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sunset", node->name))
        {
            _setcolor(node->children, &sky->_sunset);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"horizon", node->name))
        {
            _setcolor(node->children, &sky->_horizon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"moon", node->name))
        {
            _setcolor(node->children, &sky->_moon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"moon_day", node->name))
        {
            _setcolor(node->children, &sky->_moonday);
            continue;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _loadLogFile
//////////////////////////////////////////////////////////////////////
void _loadLogFile(SCHAR* log)
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    std::string path = DataPath;
    DataPath[0] = 0;

    SCHAR* name = "lightlog.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    strncpy(DataPath, path.c_str(), path.length());

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return;
        }

        if(!xmlStrcmp((xmlChar*)"TerrainSky", root->name))
        {
            _skyelements(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

//////////////////////////////////////////////////////////////////////
// Function: _onfoglinear
//////////////////////////////////////////////////////////////////////
void _onfoglinear(MyRadioButton* button)
{
    piSetRenderState(piRENDERSTATE_FOGTABLEMODE, piFOG_LINEAR);
}

//////////////////////////////////////////////////////////////////////
// Function: _onfogexp
//////////////////////////////////////////////////////////////////////
void _onfogexp(MyRadioButton* button)
{
    piSetRenderState(piRENDERSTATE_FOGTABLEMODE, piFOG_EXP);
}

//////////////////////////////////////////////////////////////////////
// Function: _onfogexp2
//////////////////////////////////////////////////////////////////////
void _onfogexp2(MyRadioButton* button)
{
    piSetRenderState(piRENDERSTATE_FOGTABLEMODE, piFOG_EXP2);
}
