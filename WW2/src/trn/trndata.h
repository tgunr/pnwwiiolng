#pragma once

struct TERRAINSKY
{
    piCOLORVALUE _skycolor;
    piCOLORVALUE _skycolornight;
    piCOLORVALUE _skyclearambient;
    piCOLORVALUE _skynightambient;
    piCOLORVALUE _sunnoon;
    piCOLORVALUE _sunitselfnoon;
    piCOLORVALUE _sunnight;
    piCOLORVALUE _sunset;
    piCOLORVALUE _horizon;
    piCOLORVALUE _moon;
    piCOLORVALUE _moonday;

    float scale;

    SINT32 setting;

    TERRAINSKY()
    {
        scale = 1.0f;
    }
} ;


struct FOGOPTION
{
    SINT32 fogoption;

    std::vector<SINT32> time;
    std::vector<FLOAT32> start;
    std::vector<FLOAT32> end;

} ;

// Each terrain setting may have several fog options
struct TERRAINFOG
{
    SINT32 setting;

    std::vector<FOGOPTION *> options;
} ;

extern void trndataInit();
extern void trndataKill();
extern void trndataUpdate();
extern void trndataPrint();

extern TERRAINSKY* trndataGetSkyData(SINT32 i);
extern FOGOPTION* trndataGetFogData(SINT32 i, SINT32 option);
