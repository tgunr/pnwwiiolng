#include "phui-precompiled-header.h"

#include <list>

#include "phui/ListView.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"
#include "phui/MessageLoop.h"

using namespace phui;

/************************************************************************************************
	ListView
*************************************************************************************************/

ListView::ListView(): View()
{
	_acceptsFocus = false;
	_contentview = 0;
	_lastselected = -1;
	_lastselectedtime = 0;
}

ListView::~ListView()
{
}

/************************************************************************************************
	AddRow
*************************************************************************************************/

void ListView::AddRow(View* view, Referenced* data, bool selectable)
{
	if(!_contentview)
	{
		_contentview = new View;
		_contentview->ResizeTo(_width, _height);
		_contentview->SetResizeMode(FOLLOW_ALL_SIDES);
		AddChild(_contentview);
		_contentview->Show();
	}

	view->SetResizeMode(FOLLOW_LEFT_RIGHT | FOLLOW_TOP);
	
	float y = 0;
	unsigned int rownum = (unsigned int)_rows.size();
	y = (rownum > 0) ? _rows[rownum - 1].view->Bottom() : Height();
	
	view->MoveTo(0, y - view->Height());
	
	Row row;
	row.view = view;
	row.data = data;
	row.selectable = selectable;
	
	_rows.push_back(row);

	_contentview->AddChild(view);
	view->Show();
	
	_contentheight += view->Height();
}

/************************************************************************************************
	ClearRows
*************************************************************************************************/

void ListView::ClearRows()
{
	ScrollTo(0,0);
	SetValue(-1);
	
	_lastselected = -1;
	_lastselectedtime = 0;
	
	for(int i = 0; i < (int)_rows.size(); i++)
	{
		_contentview->RemoveChild(_rows[i].view);
		delete _rows[i].view;
	}
	_rows.clear();
	_contentheight = 0;
}

/************************************************************************************************
	RemoveRow
*************************************************************************************************/

void ListView::RemoveRow(unsigned int rownum)
{
	if(rownum >= _rows.size()) return;
	
	Row& row = _rows[rownum];
	float height = row.view->Height();
	_contentheight -= height;
	
	_contentview->RemoveChild(row.view);
	delete row.view;
	
	_rows.erase(_rows.begin() + rownum);
	
	for(int i = rownum; i < (int)_rows.size(); i++)
	{
		_rows[i].view->MoveBy(0, height);
	}
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool ListView::MouseDown(float x, float y, int button)
{
	int i = 0;
		
	for(std::vector<Row>::iterator it = _rows.begin(); it != _rows.end(); ++it,i++)
	{
		Row& row = *it;
		if(row.selectable && y >= row.view->Bottom() && y < row.view->Top())
		{
			SetValue(i);
			phui::Message msg(SELECT_MSG);
			
			unsigned int t = ParentLoop()->FrameTime();
			
			if(_lastselected == i && (t - _lastselectedtime) < 250)
				msg.what = DBLSELECT_MSG;
			
			ParentLoop()->PostMessage(&msg, this);
	
			_lastselectedtime = t;
			_lastselected = i;

			break;
		}
	}
				
	return true;
}

/************************************************************************************************
	SelectedRow
*************************************************************************************************/

View* ListView::SelectedRow() const
{
	int val = _value.Long();

	if(val >= 0 && val < (int)_rows.size())
		return _rows[val].view;
	
	return nullptr;
}

/************************************************************************************************
	SelectedIndex
*************************************************************************************************/

int ListView::SelectedIndex()
{
	return _value.Long();
}

/************************************************************************************************
	IsSelectable
*************************************************************************************************/

bool ListView::IsSelectable(int i) const
{
	if(i >= 0 && i < (int)_rows.size())
		return _rows[i].selectable;

	return false;
}

/************************************************************************************************
	MouseScroll
*************************************************************************************************/

bool ListView::MouseScroll(float x, float y, int dir)
{
	float rowheight = 10;
	if(_rows.size() > 0)
		rowheight = _rows[0].view->Height();
		
	float sy =  ScrollY() - rowheight * dir;
	float maxsy = _contentheight - _height;
	if(maxsy < 0) maxsy = 0;
	
	if(sy < 0) sy = 0;
	if(sy > maxsy) sy = maxsy;
	
	ScrollTo(ScrollX(), sy);

	return true;
}

/************************************************************************************************
	SetRowData
*************************************************************************************************/

void ListView::SetRowData(unsigned int row, Referenced* data)
{
	if(row >= _rows.size()) return;
	_rows[row].data = data;
}

/************************************************************************************************
	GetRowData
*************************************************************************************************/

Referenced* ListView::GetRowData(unsigned int row)
{
	if(row >= _rows.size()) return nullptr;
	return _rows[row].data.ptr();
}

/************************************************************************************************
	GetRowView
*************************************************************************************************/

View* ListView::GetRowView(unsigned int row)
{
	if(row >= _rows.size()) return nullptr;
	return _rows[row].view;
}

/************************************************************************************************
	ScrollBy
*************************************************************************************************/

void ListView::ScrollBy(float dx, float dy)
{
	if(_contentview)
	{
		unsigned int n = _contentview->ChildCount();
		for(unsigned int i = 0; i < n ; i++)
		{
			View* v = _contentview->ChildAt(i);
			v->MoveBy(dx, dy);
		}
	}
	View::ScrollBy(dx, dy);
}

/************************************************************************************************
	ScrollTo
*************************************************************************************************/

void ListView::ScrollTo(float x, float y)
{
	float dx = x - _scroll_x;
	float dy = y - _scroll_y;
	
	ScrollBy(dx, dy);
}

/************************************************************************************************
	ScrollByRowIndex
*************************************************************************************************/

void ListView::ScrollByRowIndex(int i)
{
	if(_contentview)
	{
		unsigned int count = _contentview->ChildCount();
		int index = (i < 0 || i >= (int)count) ? (int)count - 1: i;
		
		View *child = _contentview->ChildAt(index);
		
		if(child)
			ScrollTo(ScrollX(), child->Y());
	}
}
