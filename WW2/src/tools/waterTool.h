#ifndef PN_TOOLS_WATERTOOL_H
#define PN_TOOLS_WATERTOOL_H 1

#include "phui/slider.h"
#include "phui/caption.h"
#include "phui/button.h"

#include "pi/pi.h"
#include "trn/trnwtr.h"

#include <vector>

class CWaterButton : public phui::Button
{
public:
    CWaterButton();
    virtual ~CWaterButton();

    virtual bool MouseUp(float x, float y, int button);

    void setCallback(void (*p)()) {m_callback = p;}

private:
    void (*m_callback)();
};

class CWaterSlider : public phui::Slider
{
public:
    CWaterSlider(float m_min = 0, float m_max = 100, int ticks = 100);

    virtual bool MouseDrag(float x, float y, int button, bool start);

    void setMax(float m) {m_max = m;}
    void setMin(float m) {m_min = m;}
    void setSlider(float value);

    void setWaterParam(WATER_PARAMS p) {m_param = p;}
    void setIndex(int i) {m_index = i;}

    phui::Caption* c;

private:
    float m_max;
    float m_min;
    float m_value;
    int m_index;

    WATER_PARAMS m_param;
};

class CWaterTool
{
public:
    CWaterTool();
    virtual ~CWaterTool();

    void setCaption(char* szCaption, int cX, int cY, float r, float g, float b);
    void Kill();

    void createSlider(int x,
                      int y,
                      float min,
                      float max,
                      int ticks,
                      WATER_PARAMS param);

    void createSlider2(int x,
                       int y,
                       float min,
                       float max,
                       int ticks,
                       WATER_PARAMS param);

    void createSlider3(int x,
                       int y,
                       float min,
                       float max,
                       int ticks,
                       WATER_PARAMS param);

    void createButton(int x, int y, void (*callback)(), char* caption);
    void refresh();

private:
    std::vector<CWaterSlider *> m_vSliders;
    std::vector<phui::Caption *> m_vCaptions;
    std::vector<CWaterButton *> m_vButtons;

    phui::Viewport* m_viewport;
};

void waterToolCreate();

#endif // PN_TOOLS_WATERTOOL_H
