#pragma once

#include "phui/View.h"
#include "phui/Invoker.h"

namespace phui
{

class Message;

/************************************************************************************************
	Control
*************************************************************************************************/
/**
	The base control View
*/
class Control : public View, public Invoker
{
public:
	Control() : View(), Invoker(), _label()
	{
		_enabled = true;
	}
	virtual ~Control() {}
	
	virtual int Invoke(Message* msg=NULL); ///< adds "when" and "source" fields
	
	void SetLabel(const char* const label) { _label = label; }
	void SetLabel(const wchar_t* const label) { _label = label; }
	const char* Label() const { return _label.String(); }
	const wchar_t* WideLabel() const { return _label.WideString(); }
	
protected:
	Scalar _label;
};

}
