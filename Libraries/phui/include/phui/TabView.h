#ifndef PHUI_TABVIEW_H
#define PHUI_TABVIEW_H

#include <string>

#include "phui/Control.h"

namespace phui
{

/************************************************************************************************
	TabView
*************************************************************************************************/
/**
	TabView
*/

class TabView : public Control
{
public:
	TabView();
	virtual ~TabView(); 
	
	void AddTab(const char* name, int alignment = ALIGN_LEFT);
	void AddTab(const wchar_t* name, int alignment = ALIGN_LEFT);

	void SetTabAlignment(int a){ _tabalign = a; }
	int GetTabAlignment(){ return _tabalign; }

	struct Tab;
	void AddTab(Tab tab);
	
	virtual int AddChild(View* v, View* before=nullptr);
	
	virtual void DrawSelf();

	virtual void PreferredSize(float* w, float* h) { MinSize(w,h); }
	virtual void MinSize(float* w, float* h);
	virtual void FontResized(unsigned int size);

	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );
	
	virtual void SetValue(const Scalar& v);
	
	virtual void Show();
	
	int NumTabs() const;
	bool ValidTab(int i);

	TabView::Tab& GetTab(int);
	TabView::Tab* GetTab(View* child);
	
	/*
	std::wstring GetTabName(int) const;
	int GetTabAlignment(int);
	float GetTabWidth(int);
	*/

	bool TabPressed() const;
	int TabMouseDown() const;
	
	enum
	{
		ALIGN_LEFT = 0,
		ALIGN_RIGHT,
		ALIGN_TOP,
		ALIGN_BOTTOM
	};
	
	struct Tab
	{
		Tab();
		~Tab();
		
		std::wstring name;
		std::wstring tooltip;
		int alignment, index;
		float x, y, width, height;
		bool visible;
	};

protected:
	void InitTabs();
	int FindTabHit(float x, float y);
	
	friend class ViewDrawer;
	bool _pressed, _initialized;
	int _mousedown, _lasthit, _tabalign;
	
	class TabList;
	TabList* _tabs;
};

}
#endif // PHUI_TABVIEW_H
