#pragma once

#include "phui/View.h"
#include "phui/UIXML.h"
#include "phui/DOM.h"

#include "sysMovie.h"

/************************************************************************************************
	uiMovieView
*************************************************************************************************/

class uiMovieView : public phui::View
{
public:
	uiMovieView();
	~uiMovieView();

	virtual void DrawSelf();
	virtual void Show();
	virtual void Hide();

	void SetMovie(sysMovie*);
	sysMovie* Movie() { return _movie; }

protected:
	sysMovie* _movie;
private:
	float x1, y1, x2, y2;
};

class jsMovieView : public phui::DOM::ElementDOM
{
public:
	jsMovieView(uiMovieView*, phui::DOM::ElementDOM*);

	JS_CLASS_DATA;
	JS_FUNCTION(play);
	JS_FUNCTION(rewind);
	JS_FUNCTION(load);

	enum{ PLAYING_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	uiMovieView* _movieview;

protected:
	~jsMovieView();

};

extern phui::View* uiMovieViewCreate(_xmlNode*, phui::View* parent);
extern phui::DOM::ElementDOM* uiMovieCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);
