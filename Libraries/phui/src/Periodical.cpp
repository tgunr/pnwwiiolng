#include "phui-precompiled-header.h"

#include <list>
#include <algorithm>
#include "phui/Periodical.h"

using namespace phui;

typedef std::list<Periodical*> PeriodicalList;
static PeriodicalList* _periodicals;

/************************************************************************************************
	Periodical
*************************************************************************************************/

void Periodical::Update(unsigned int t)
{
	if(!_periodicals) _periodicals = new PeriodicalList;
	
	for(PeriodicalList::iterator it = _periodicals->begin(); it != _periodicals->end();)
	{
		PeriodicalList::iterator next = it; // so it's safe for periodical to delete itself from within the tick
		next++;
		if((*it)->_executing)
		{
			if(!(*it)->_timeinited)
			{
				(*it)->_lasttime = t;
				(*it)->_timeinited = true;
			}
			(*it)->Tick(t);					
		}
			
		it = next;
	}
}

/************************************************************************************************
	Periodical
*************************************************************************************************/

Periodical::Periodical(callback cb, void* data) : _callback(cb), _data(data)
{
	_executing = false;
	_lasttime = 0;
	_interval = 0;
	_timeinited = false;
	
	if(!_periodicals) _periodicals = new PeriodicalList;
	_periodicals->push_back(this);
}

Periodical::~Periodical()
{
	PeriodicalList::iterator it = find(_periodicals->begin(), _periodicals->end(), this);
	if(it != _periodicals->end())
		_periodicals->erase(it);
}

/************************************************************************************************
	Tick
*************************************************************************************************/

void Periodical::Tick(unsigned int t)
{
	if(t - _lasttime > _interval)
	{
		Execute(t);
		_lasttime = t;
	}
}


