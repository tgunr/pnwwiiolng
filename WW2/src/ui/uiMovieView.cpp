#include "syscfg.h" // Path is project specific.
#include "libxml/tree.h"
#include "phui/MessageLoop.h"
#include "include/vars.h"
#include "pi/pi.h"
#include "ui/uiMain.h"

#include "ui/uiMovieView.h"

/************************************************************************************************
	uiMovieView
*************************************************************************************************/

uiMovieView::uiMovieView() : _movie(0)
{
	x1 = y1 = x2 = y2 = 0;
}

uiMovieView::~uiMovieView()
{
	if (_movie )
		delete _movie;
}

void uiMovieView::DrawSelf()
{
	if(!_movie) return;

	_movie->UpdateFrame(ParentLoop()->FrameTime());
	_movie->DrawFrame(x1, y1, x2, y2);
}

void uiMovieView::SetMovie(sysMovie* m)
{
	if(_movie == m) return;
	delete _movie;
	_movie = m;

	x1 = 0;
	y1 = 0;
	x2 = Width();
	y2 = Height();

// need to figure out how to get rid of these ugly ifdef's
// and hide all this nonsense in the implementation code
#if platOS == OS_WIN32
	// movies are positioned absolutely
	// lets get and save the screen x and y for the underlying view
	// we'll use this when drawing, this makes the movie
	// appear where the view is set in the uixml layer
	// I do this here because it only needs to be once in most cases

	LocalToScreen(&x1, &y1);

	x2 = x1 + Width();
	y2 = y1 + Height();

	_movie->DrawFrame(x1, y1, x2, y2);
	_movie->Stop();
#endif
}


void uiMovieView::Show()
{
	if(_movie)
		_movie->Enable(TRUE);

	View::Show();
}

void uiMovieView::Hide()
{
	if(_movie)
		_movie->Enable(FALSE);

	View::Hide();
}

/************************************************************************************************
	jsMovieView
*************************************************************************************************/

JSClass jsMovieView::jsclass = JS_CLASS_MACRO("movie");

JSFunctionSpec jsMovieView::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsMovieView, play, 0),
	JS_FUNCTION_SPEC(jsMovieView, rewind, 0),
	JS_FUNCTION_SPEC(jsMovieView, load, 1),
	{0}
};

JSPropertySpec jsMovieView::jsproperties[] =
{
	{"playing",		jsMovieView::PLAYING_PROP,		JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsMovieView
*************************************************************************************************/
jsMovieView::jsMovieView(uiMovieView* v, phui::DOM::ElementDOM* w) : ElementDOM(v,w, &jsMovieView::jsclass)
{
	DefineFunctions(jsMovieView::jsfunctions);
	DefineProperties(jsMovieView::jsproperties);

	_movieview = v;
};

/************************************************************************************************
	~jsMovieView - destructor
*************************************************************************************************/

jsMovieView::~jsMovieView()
{
}

/************************************************************************************************
	play
*************************************************************************************************/

JS_FUNCTION_DEF(jsMovieView, play)
{
	jsMovieView* dom = phui::js::Cast<jsMovieView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	sysMovie* smv = dom->_movieview->Movie();

	if(smv)
	{
		if(smv->Playing())
			smv->Pause();
		else
			smv->Start(frametime);
	}

	return JS_TRUE;
}

/************************************************************************************************
	stop - actually rewinds
*************************************************************************************************/

JS_FUNCTION_DEF(jsMovieView, rewind)
{
	jsMovieView* dom = phui::js::Cast<jsMovieView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	sysMovie* smv = dom->_movieview->Movie();

	if(smv)
		smv->Stop();

	return JS_TRUE;
}

/************************************************************************************************
	load
*************************************************************************************************/

JS_FUNCTION_DEF(jsMovieView, load)
{
	jsMovieView* dom = phui::js::Cast<jsMovieView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	if(argc < 1) return JS_FALSE;

	const char* filename = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	sysMovie* movie = new sysMovie;

	if(movie->Load((char*)filename))
		dom->_movieview->SetMovie(movie);
	else
		delete movie;

	return JS_TRUE;
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool jsMovieView::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case PLAYING_PROP:
		{
			sysMovie* smv = _movieview->Movie();

			if(smv && smv->Playing())
				*val = BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				*val = BOOLEAN_TO_JSVAL(JS_FALSE);
		}
		break;
	}

	return JS_TRUE;
}

JSBool jsMovieView::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	return JS_TRUE;
}

/************************************************************************************************
	uiMovieViewCreate
*************************************************************************************************/

phui::View* uiMovieViewCreate(_xmlNode* xmlnode, phui::View* parent)
{
	uiMovieView* mv = new uiMovieView;
	if(xmlnode)
	{
		xmlChar* val;

		val = xmlGetProp(xmlnode, (xmlChar*)"src");
		if(val)
		{
			sysMovie* movie = new sysMovie;

			if(movie->Load((char*)val))
				mv->SetMovie(movie);
			else
				delete movie;

			xmlFree(val);
		}
	}

	return mv;
}

/************************************************************************************************
	uiMovieCreateDOM
*************************************************************************************************/

phui::DOM::ElementDOM* uiMovieCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent)
{
	uiMovieView* mv = dynamic_cast<uiMovieView*>(view);

	if(mv)
		return new jsMovieView(mv, parent);

	return 0;
}
