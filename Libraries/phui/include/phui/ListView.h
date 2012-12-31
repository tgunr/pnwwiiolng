#ifndef PHUI_LISTVIEW_H
#define PHUI_LISTVIEW_H

#include <string>
#include <list>
#include <vector>

#include "phui/View.h"
#include "phui/Scalar.h"
#include "phui/Referenced.h"

namespace phui
{

/************************************************************************************************
	ListView
*************************************************************************************************/

class ListView : public View
{
public:
	ListView();
	virtual ~ListView(); 
		
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseScroll(float x, float y, int dir);
	virtual void ScrollBy(float,float);
	virtual void ScrollTo(float,float);
	
	void AddRow(View*, Referenced* data=0, bool selectable=true);
	void ClearRows();
	void RemoveRow(unsigned int);
	void ScrollByRowIndex(int);
	
	void SetRowData(unsigned int, Referenced*);
	Referenced* GetRowData(unsigned int);
	View* GetRowView(unsigned int);

	View* SelectedRow() const;
	int SelectedIndex();
	bool IsSelectable(int i) const;
	unsigned int NumRows() const { return _rows.size(); }
	
	struct Row
	{
		Row() : view(0), selectable(true) {}
		View* view;
		referenced_ptr data;
		bool selectable;
	};
	
protected:
	View* _contentview;	
	long _lastselected;
	unsigned int _lastselectedtime;
	std::vector<Row> _rows;

};

}
#endif // PHUI_LISTVIEW_H
