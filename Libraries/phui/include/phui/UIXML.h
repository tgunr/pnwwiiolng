#ifndef PHUI_UIXML_H
#define PHUI_UIXML_H

#include <vector>
#include "phui/View.h"
#include "phui/GraphicsContext.h"

/************************************************************************************************
	UIXML
*************************************************************************************************/
// "native" XML format for phui widgets

struct _xmlNode;

namespace phui
{

class View;
struct IOCallbacks;
class CSSViewDrawerFactory;

typedef View* (*ViewFromUIXML)(_xmlNode*, View* parent);
typedef GraphicsContext::TextureInfo (*UIXMLTextureCallback) (const char*,bool); // path, ?
typedef GraphicsContext::TextureInfo (*UIXMLCreateTextureCallback) (int,int,int); // width, height, bpp
typedef void (*UIXMLDestroyTextureCallback) (int); // texid
typedef const wchar_t* (*UIXMLClipboardCallback) (const wchar_t*); // data
typedef const wchar_t* (*UIXMLLocalizationCallbackA)(const char*);
typedef const wchar_t* (*UIXMLLocalizationCallbackW)(const wchar_t*);

void RegisterTagWithUIXML(const char* tag, ViewFromUIXML);
View* CreateViewFromUIXML(const char* classname, _xmlNode*, View* parent);

void SetUIXMLIOCallbacks(IOCallbacks const*);
void SetUIXMLTextureCallback(UIXMLTextureCallback);
void SetUIXMLCreateTextureCallback(UIXMLCreateTextureCallback);
void SetUIXMLDestroyTextureCallback(UIXMLDestroyTextureCallback);
void SetUIXMLDrawerFactory(CSSViewDrawerFactory*);
void SetUIXMLSetViewportSize(float width, float height, float xwRatio = 3.0f); // extreme width ratio (triplehead)
void SetUIXMLClipboardCallback(UIXMLClipboardCallback);
void SetUIXMLLocalizationCallback(UIXMLLocalizationCallbackA);
void SetUIXMLLocalizationCallback(UIXMLLocalizationCallbackW);

View* LoadUIXML(const char* name, IOCallbacks const *io = nullptr, View* parent = nullptr);
float ScaleUIXMLValue(int type, float val, unsigned int options = View::ASPECT_SCALE, float scaler = 1.0f, float limiter = 1.0f);
float NormUIXMLValue(int type, float val, unsigned int options = View::ASPECT_SCALE, float scaler = 1.0f, float limiter = 1.0f);

bool UIXMLbool(const unsigned char*);
void UIXMLColor(const unsigned char* val, float* r, float* g, float* b, float* a=0);

GraphicsContext::TextureInfo UIXMLCreateTexture(int, int, int);
void UIXMLDestroyTexture(int);
const wchar_t* UIXMLClipboard(const wchar_t*);
const wchar_t* UIXMLLocalize(const wchar_t* text);
const wchar_t* UIXMLLocalize(const char* text);

}
#endif // PHUI_UIXML_H
