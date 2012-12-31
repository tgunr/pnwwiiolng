#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: waterTool.cpp
// Description: Tool to adjust water parameters
//////////////////////////////////////////////////////////////////////
#include "tools/waterTool.h"
#include "ui/uiMain.h"
#include "include/vars.h"
#if platOS == OS_WIN32
#include <io.h>
#else platOS == OS_MACOS
#include <unistd.h>
#define _access access
#endif

using namespace std;
using namespace phui;

static void _saveWaterParams();
static void _loadWaterLogFile();

static SCHAR saveNames[MAX_WATER_PARAMS][64] =
{
    "static FLOAT32 g_hdrMultiplier",
    "static FLOAT32 g_fresnelBias",
    "static FLOAT32 g_fresnelScale",
    "static FLOAT32 g_fresnelPower",
    "static FLOAT32 g_bumpScale",
    "static FLOAT32 g_waterAmount",
    "static FLOAT32 g_reflectionAmount",
    "static FLOAT32 g_bumpSpeed[2]",
    "static FLOAT32 g_textureScale",
    "static UINT32 g_shallowColor",
    "static UINT32 g_deepColor",
    "static UINT32 g_reflectionColor"
};

//////////////////////////////////////////////////////////////////////
// Function: CWaterButton
//////////////////////////////////////////////////////////////////////
CWaterButton::CWaterButton()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~CWaterButton
//////////////////////////////////////////////////////////////////////
CWaterButton::~CWaterButton()
{
}

//////////////////////////////////////////////////////////////////////
// Function: MouseUp
//////////////////////////////////////////////////////////////////////
bool CWaterButton::MouseUp(float x, float y, int button)
{
    if(m_callback)
    {
        m_callback();
    }

    return Button::MouseUp(x, y, button);
}

//////////////////////////////////////////////////////////////////////
// Function: CWaterSlider
//////////////////////////////////////////////////////////////////////
CWaterSlider::CWaterSlider(float min, float max, int ticks) : Slider()
{
    SetMinValue(min);
    SetMaxValue(max);
    SetTicks(ticks);

    m_min = min;
    m_max = max;
}

//////////////////////////////////////////////////////////////////////
// Function: MouseDrag
//////////////////////////////////////////////////////////////////////
bool CWaterSlider::MouseDrag(float x, float y, int button, bool start)
{
    Slider::MouseDrag(x, y, button, start);

    float m_value = GetValue();

    if(m_value >= m_min && m_value <= m_max)
    {
        char s[32];

        sprintf(s, "%f", m_value);
        c->SetValue(s);

        switch(m_param)
        {
        case HDR_MULTIPLIER:
        case FRESNEL_BIAS:
        case FRESNEL_SCALE:
        case FRESNEL_POWER:
        case BUMP_SCALE:
        case WATER_AMOUNT:
        case TEXTURE_SCALE:
        case REFLECTION_AMOUNT:
            {
                trnwtrSetShaderFloat(m_param, m_value);
                break;
            }
        case BUMP_SPEED:
            {
                trnwtrSetShaderFloatArray(m_param, m_index, m_value);
                break;
            }
        case DEEP_COLOR:
        case SHALLOW_COLOR:
        case REFLECTION_COLOR:
            {
                trnwtrSetShaderColor(m_param, m_index, m_value);
                break;
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// Function: setSlider
//////////////////////////////////////////////////////////////////////
void CWaterSlider::setSlider(float value)
{
    m_value = value;

    SetValue(value);
}

//////////////////////////////////////////////////////////////////////
// Function: CWaterTool
//////////////////////////////////////////////////////////////////////
CWaterTool::CWaterTool()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~CWaterTool
//////////////////////////////////////////////////////////////////////
CWaterTool::~CWaterTool()
{
    Kill();
}

//////////////////////////////////////////////////////////////////////
// Function: Kill
//////////////////////////////////////////////////////////////////////
void CWaterTool::Kill()
{
    vector<CWaterSlider *>::iterator sB = m_vSliders.begin();
    vector<CWaterSlider *>::iterator sFREE;

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

    vector<CWaterButton *>::iterator bB = m_vButtons.begin();
    vector<CWaterButton *>::iterator bFREE;

    while(bB !=  m_vButtons.end())
    {
        bFREE = bB;
        ++bB;

        m_viewport->RemoveChild(*bFREE);

        delete (*bFREE);
    }

    m_vSliders.clear();
    m_vCaptions.clear();
    m_vButtons.clear();
}

//////////////////////////////////////////////////////////////////////
// Function: createButton
//////////////////////////////////////////////////////////////////////
void CWaterTool::createButton(int x, int y, void (*callback)(), char* caption)
{
    CWaterButton* b = new CWaterButton;

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
// Function: refresh
//////////////////////////////////////////////////////////////////////
void CWaterTool::refresh()
{
}

//////////////////////////////////////////////////////////////////////
// Function: createSlider
//////////////////////////////////////////////////////////////////////
void CWaterTool::createSlider(int x,
                              int y,
                              float min,
                              float max,
                              int ticks,
                              WATER_PARAMS param)
{
    CWaterSlider* s = new CWaterSlider(min, max, ticks);

    m_viewport = uiViewport();
	m_viewport->AddChild(s);

    s->Show();
    s->ResizeTo(100, 16);

    s->SetBackgroundColor(0, 0, 0, 0);
    s->MoveTo((float)x, (float)y);

    s->setSlider(trnwtrGetShaderFloat(param));
    s->setWaterParam(param);

    Caption* c = new Caption();

    s->c = c;

    c->SetTextColor(1, 1, 1, 1);

    c->ResizeTo(64, 16);
    c->SetBackgroundColor(0, 0, 0, 0);
    c->MoveTo((float)x + 100, (float)y);

    char str[32];

    sprintf(str, "%3f", trnwtrGetShaderFloat(param));

    c->SetValue(str);
    c->SetTextAlignment(Caption::ALIGN_LEFT);
    c->Show();

    m_viewport->AddChild(c);

    m_vSliders.push_back(s);
}

//////////////////////////////////////////////////////////////////////
// Function: createSlider2
//////////////////////////////////////////////////////////////////////
void CWaterTool::createSlider2(int x,
                               int y,
                               float min,
                               float max,
                               int ticks,
                               WATER_PARAMS param)
{
    float* value = trnwtrGetShaderFloatArray(param);

    for(int i = 0; i < 2; i++)
    {
        CWaterSlider* s = new CWaterSlider(min, max, ticks);

        m_viewport = uiViewport();
	    m_viewport->AddChild(s);

        s->Show();
        s->ResizeTo(100, 16);

        s->SetBackgroundColor(0, 0, 0, 0);
        s->MoveTo((float)x, (float)y);

        s->setIndex(i);
        s->setSlider(value[i]);
        s->setWaterParam(param);

        Caption* c = new Caption();

        s->c = c;

        c->SetTextColor(1, 1, 1, 1);

        c->ResizeTo(64, 16);
        c->SetBackgroundColor(0, 0, 0, 0);
        c->MoveTo((float)x + 100, (float)y);

        char str[32];

        sprintf(str, "%3f", value[i]);

        c->SetValue(str);
        c->SetTextAlignment(Caption::ALIGN_LEFT);
        c->Show();

        m_viewport->AddChild(c);

        m_vSliders.push_back(s);

        y -= 32;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: createSlider2
//////////////////////////////////////////////////////////////////////
void CWaterTool::createSlider3(int x,
                               int y,
                               float min,
                               float max,
                               int ticks,
                               WATER_PARAMS param)
{
    UINT32 value = trnwtrGetShaderColor(param);

    float color[3];

    UINT32 r = piRGBA_GETRED(value);
    UINT32 g = piRGBA_GETGREEN(value);
    UINT32 b = piRGBA_GETBLUE(value);

    color[0] = (float)r * colorONEOVER255;
    color[1] = (float)g * colorONEOVER255;
    color[2] = (float)b * colorONEOVER255;

    for(int i = 0; i < 3; i++)
    {
        CWaterSlider* s = new CWaterSlider(min, max, ticks);

        m_viewport = uiViewport();
	    m_viewport->AddChild(s);

        s->Show();
        s->ResizeTo(100, 16);

        s->SetBackgroundColor(0, 0, 0, 0);
        s->MoveTo((float)x, (float)y);

        s->setIndex(i);
        s->setSlider(color[i]);
        s->setWaterParam(param);

        Caption* c = new Caption();

        s->c = c;

        if(i == 0)
            c->SetTextColor(1, 0, 0, 1);
        if(i == 1)
            c->SetTextColor(0, 1, 0, 1);
        if(i == 2)
            c->SetTextColor(0, 0, 1, 1);

        c->ResizeTo(64, 16);
        c->SetBackgroundColor(0, 0, 0, 0);
        c->MoveTo((float)x + 100, (float)y);

        char str[32];

        sprintf(str, "%3f", color[i]);

        c->SetValue(str);
        c->SetTextAlignment(Caption::ALIGN_LEFT);
        c->Show();

        m_viewport->AddChild(c);

        m_vSliders.push_back(s);

        y -= 32;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: setCaption
//////////////////////////////////////////////////////////////////////
void CWaterTool::setCaption(char* szCaption, int x, int y, float r, float g, float b)
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
// Function: waterToolCreate
//////////////////////////////////////////////////////////////////////
void waterToolCreate()
{
    static LOGICAL toggle = FALSE;
    static CWaterTool* wt = NULL;

    toggle = !toggle;

    if(toggle)
    {
        if(wt)
        {
            delete wt;
        }

        wt = new CWaterTool();

        _loadWaterLogFile();

        SINT32 h = piGetHeight() - 64;
        SINT32 x = 64;
        SINT32 xoffset = 160;

        wt->setCaption("HDR MULTIPLIER", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 100.0f, 0, HDR_MULTIPLIER);

        h = piGetHeight() - 128;

        wt->setCaption("FRESNEL BIAS", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 1.0f, 0, FRESNEL_BIAS);

        h = piGetHeight() - 196;

        wt->setCaption("FRESNEL SCALE", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 1.0f, 0, FRESNEL_SCALE);

        h = piGetHeight() - 260;

        wt->setCaption("FRESNEL POWER", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 10.0f, 0, FRESNEL_POWER);

        h = piGetHeight() - 324;

        wt->setCaption("BUMP SCALE", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 2.0f, 0, BUMP_SCALE);

        h = piGetHeight() - 388;

        wt->setCaption("WATER AMOUNT", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 2.0f, 0, WATER_AMOUNT);

        h = piGetHeight() - 452;

        wt->setCaption("REFLECTION AMOUNT", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 2.0f, 0, REFLECTION_AMOUNT);

        x += xoffset;
        h = piGetHeight() - 64;

        wt->setCaption("BUMP SPEED", x, h, 1, 1, 0);
        wt->createSlider2(x, h-=26, -1.0f, 1.0f, 0, BUMP_SPEED);

        h = piGetHeight() - 160;

        wt->setCaption("TEXTURE SCALE", x, h, 1, 1, 0);
        wt->createSlider(x, h-=26, 0.0f, 50.0f, 0, TEXTURE_SCALE);

        x += xoffset;
        h = piGetHeight() - 64;

        wt->setCaption("DEEP COLOR", x, h, 1, 1, 0);
        wt->createSlider3(x, h-=26, 0.0f, 1.0f, 0, DEEP_COLOR);

        h = piGetHeight() - 204;

        wt->setCaption("SHALLOW COLOR", x, h, 1, 1, 0);
        wt->createSlider3(x, h-=26, 0.0f, 1.0f, 0, SHALLOW_COLOR);

        h = piGetHeight() - 344;

        wt->setCaption("REFLECTION COLOR", x, h, 1, 1, 0);
        wt->createSlider3(x, h-=26, 0.0f, 1.0f, 0, REFLECTION_COLOR);

        x = (piGetWidth() / 2) - 32;
        h = piGetHeight() - 32;

        wt->createButton(x, h, _saveWaterParams, "SAVE");
    }
    else
    {
        if(wt)
        {
            delete wt;
            wt = NULL;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _saveWaterParams
//////////////////////////////////////////////////////////////////////
void _saveWaterParams()
{
    char fileName[64] = "waterlog.txt";
    char* p = "waterlog";
    int count = 1;

    while(_access(fileName, 0) == 0)
    {
        sprintf(fileName, "%s%d.txt", p, count++);
    }

    FILE* fp = fopen(fileName, "w");

    fprintf(fp, "%s\n\n", "/*** CUT AND PASTE BELOW TO REPLACE VARIABLES IN TRNWTR.CPP ***/");

    for(int i = 0; i < MAX_WATER_PARAMS; i++)
    {
        switch(i)
        {
        case HDR_MULTIPLIER:
        case FRESNEL_BIAS:
        case FRESNEL_SCALE:
        case FRESNEL_POWER:
        case BUMP_SCALE:
        case WATER_AMOUNT:
        case TEXTURE_SCALE:
        case REFLECTION_AMOUNT:
            {
                fprintf(fp, "%s = %ff;\n", saveNames[i], trnwtrGetShaderFloat((WATER_PARAMS)i));
                break;
            }
        case BUMP_SPEED:
            {
                float* value = trnwtrGetShaderFloatArray((WATER_PARAMS)i);
                fprintf(fp, "%s = {%ff, %ff};\n", saveNames[i], value[0], value[1]);
                break;
            }
        case DEEP_COLOR:
        case SHALLOW_COLOR:
        case REFLECTION_COLOR:
            {
                UINT32 color = trnwtrGetShaderColor((WATER_PARAMS)i);

                float r = (float)piRGBA_GETRED(color) * colorONEOVER255;
                float g = (float)piRGBA_GETGREEN(color) * colorONEOVER255;
                float b = (float)piRGBA_GETBLUE(color) * colorONEOVER255;

                fprintf(fp, "%s = piRGBA(%ff, %ff, %ff, 1.0f);\n", saveNames[i], r, g, b);
                break;
            }
        }
    }

    fclose(fp);
}

//////////////////////////////////////////////////////////////////////
// Function: _loadWaterLogFile
//////////////////////////////////////////////////////////////////////
void _loadWaterLogFile()
{
    FILE* fp = fopen("waterlog.txt", "r");

    if(!fp) return;

    char s[128];
    char* p1 = NULL;
    char* p2 = NULL;

    int index = -1;

    while(fgets(s, 127, fp) != NULL)
    {
        if(s[0] == '\n')
            continue;

        if(strlen(s) <= 1)
            continue;

        index = -1;

        for(int j = 0; j < MAX_WATER_PARAMS; j++)
        {
            if(p1 = strstr(s, saveNames[j]))
            {
                index = j;
                break;
            }
        }

        if(index < 0)
            continue;

        WATER_PARAMS param = (WATER_PARAMS)index;

        p2 = strchr(p1, '=');

        if(!p2) continue;
        p2 += 2;

        switch(param)
        {
        case HDR_MULTIPLIER:
        case FRESNEL_BIAS:
        case FRESNEL_SCALE:
        case FRESNEL_POWER:
        case BUMP_SCALE:
        case WATER_AMOUNT:
        case TEXTURE_SCALE:
        case REFLECTION_AMOUNT:
            {
                float value1;

                sscanf(p2, "%f", &value1);
                trnwtrSetShaderFloat(param, value1);
                break;
            }
        case BUMP_SPEED:
            {
                float value1, value2;

                p2++;

                sscanf(p2, "%f", &value1);

                p2 = strchr(p2, ',');
                p2 += 2;

                sscanf(p2, "%f", &value2);

                trnwtrSetShaderFloatArray(param, 0, value1);
                trnwtrSetShaderFloatArray(param, 1, value2);
                break;
            }
        case DEEP_COLOR:
        case SHALLOW_COLOR:
        case REFLECTION_COLOR:
            {
                p2 = strchr(p2, '(');

                if(!p2) continue;

                p2++;

                float r, g, b;

                sscanf(p2, "%f", &r);

                p2 = strchr(p2, ',');
                p2 += 2;

                sscanf(p2, "%f", &g);

                p2 = strchr(p2, ',');
                p2 += 2;

                sscanf(p2, "%f", &b);

                trnwtrSetShaderColor(param, 0, r);
                trnwtrSetShaderColor(param, 1, g);
                trnwtrSetShaderColor(param, 2, b);
                break;
            }
        }
    }

    fclose(fp);
}

//////////////////////////////////////////////////////////////////////
// Function: _restoreWaterColors
//////////////////////////////////////////////////////////////////////
void _restoreWaterColors()
{
}