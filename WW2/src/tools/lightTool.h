#ifndef PN_TOOLS_LIGHTTOOL_H
#define PN_TOOLS_LIGHTTOOL_H 1

#include "phui/slider.h"
#include "phui/caption.h"
#include "phui/button.h"
#include "phui/radiobutton.h"

#include "pi/pi.h"
#include "trn/trnsky.h"
#include "ogl/oglfbo.h"

#include <vector>

class MyRadioButton : public phui::RadioButton
{
public:
    MyRadioButton();
    virtual ~MyRadioButton();

    virtual bool MouseUp(float x, float y, int button);

    void setCallback(void (*p)(MyRadioButton*)) {_callback = p;}

private:
    void (*_callback)(MyRadioButton*);
};

class CMyButton : public phui::Button
{
public:
    CMyButton();
    virtual ~CMyButton();

    virtual bool MouseUp(float x, float y, int button);

    void setCallback(void (*p)(CMyButton*)) {m_callback = p;}

private:
    void (*m_callback)(CMyButton*);
};

class CMySlider : public phui::Slider
{
public:
    CMySlider(float m_min = 0, float m_max = 100, int type = -1);

    virtual bool MouseDrag(float x, float y, int button, bool start);

    void setMax(float m) {m_max = m;}
    void setMin(float m) {m_min = m;}
    void setSlider(float value);
    void setshaderParam(int param) {m_shaderparam = param;}

    void setEnumColor(ENUM_TERRAIN_COLOR c) {m_color = c;}

    int getType() {return m_type;}
    int getShaderParam() {return m_shaderparam;}

    phui::Caption* c;

    enum
    {
        type_skylight,
        type_filter,
        type_fogstartdensity,
        type_fogenddensity,
        type_fogstart,
        type_fogend,
        type_fogcolorR,
        type_fogcolorG,
        type_fogcolorB
    };

private:
    float m_max;
    float m_min;
    float m_value;

    int m_type;
    int m_shaderparam;

    ENUM_TERRAIN_COLOR m_color;
};

class CLightTool
{
public:
    CLightTool();
    virtual ~CLightTool();

    void setCaption(char* szCaption, int cX, int cY, float r, float g, float b);
    void Kill();

    void createSlider(int x,
                      int y,
                      float min,
                      float max,
                      ENUM_TERRAIN_COLOR enumColor);

    void createSlider2(int x,
                       int y,
                       int shaderparam,
                       float min,
                       float max);

    void createFogSlider(int x,
                         int y,
                         float min,
                         float max,
                         int type);

    void createButton(int x, int y, void (*callback)(CMyButton*), char* caption);
    void createRadioButton(int x, int y, void (*callback)(MyRadioButton*), char* caption);
    void refresh();

private:
    std::vector<CMySlider *> m_vSliders;
    std::vector<phui::Caption *> m_vCaptions;
    std::vector<CMyButton *> m_vButtons;
    std::vector<MyRadioButton *> _radioButtons;

    phui::Viewport* m_viewport;

    SINT32 _buttonenabled;
};

void lightToolCreate(bool refresh);

#endif // PN_TOOLS_LIGHTTOOL_H
