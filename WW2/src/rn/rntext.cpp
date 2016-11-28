#include <syscfg.h>
#include <math.h>
#include <wchar.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ld/ldpix.h"
#include "ld/ldtex.h"
#include "rn/rntext.h"
#include "fio/fio.h"
#include "ld/ldtexture.h"
#include "draw/fontadaptor.h"

static std::unique_ptr<FontAdaptor> iconfontAdaptor;
static std::unique_ptr<FontAdaptor> gunsightfontAdaptor;

int rntextInit()
{
    std::unique_ptr<FTFontWestern> iconfont = std::make_unique<FTFontWestern>();

    iconfont->FaceSize(26);
    iconfontAdaptor = std::make_unique<FontAdaptor>(std::move(iconfont));

    std::unique_ptr<FTFontWestern> gunsightfont = std::make_unique<FTFontWestern>();
    gunsightfont->FaceSize(18);
    gunsightfontAdaptor = std::make_unique<FontAdaptor>(std::move(gunsightfont));

    return 1;
}

void rntextKill(void)
{
    iconfontAdaptor.reset();
    gunsightfontAdaptor.reset();
}

static std::unique_ptr<FontAdaptor> &rntextAdaptorForFont(int iFont)
{
    return iFont == 1 ? gunsightfontAdaptor : iconfontAdaptor;
}

int rntextGetHeight(int Font)
{
    return rntextAdaptorForFont(Font)->getHeight();
}

SINT32 rntextGetStringWidth(const WCHAR*s, SINT32 n)
{
    return rntextAdaptorForFont(n)->getStringWidth(s);
}

LOGICAL rntext3DString(const WCHAR* pszText, UINT32 uColor, SINT32 iFont, piLVERTEX* plVertices, SINT32 iNumVertices)
{
    return rntextAdaptorForFont(iFont)->get3DString(pszText, uColor, plVertices, iNumVertices);
}

SINT32 rntextGetTexId(SINT32 iFont)
{
    return rntextAdaptorForFont(iFont)->getTextureId();
}
