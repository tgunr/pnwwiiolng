#include "phui-precompiled-header.h"

#include <deque>

#include "phui/KeyCodes.h"
#include "phui/ConsoleView.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"

using namespace phui;

#define LINE_SPACING 5

class ConsoleView::CmdHistory : public std::deque<std::wstring> {};

/************************************************************************************************
	ConsoleView
*************************************************************************************************/

ConsoleView::ConsoleView() : TextView()
{
	_cmdHistory = new CmdHistory;
	_cmdCallback = NULL;
	_cmdCompletion = NULL;
	_curhistory = 0;
	_cmdCallbackRefcon = _cmdCompletionRefcon = 0;
}

ConsoleView::~ConsoleView()
{
	delete _cmdHistory;
}

/************************************************************************************************
	KeyDown
*************************************************************************************************/

bool ConsoleView::KeyDown(int keycode, unsigned short unicode)
{
	bool handled = true;
	
	if(keycode == phui::KEYCODE_TAB && _cmdCompletion)
		doTabCommandCompletion(commandLine());

	if(keycode != phui::KEYCODE_UP && keycode != phui::KEYCODE_DOWN)
		setCurrentHistory(commandLine());
	
	switch(keycode)
	{		
		case phui::KEYCODE_UP: // history
			_curhistory++;
	
			if(_curhistory >= (int)_cmdHistory->size())
				_curhistory = (int)_cmdHistory->size() - 1;
	
			replaceCommandLine(getHistory(_curhistory));
		break;
		
		case phui::KEYCODE_DOWN: // history
			_curhistory--;
			
			if(_curhistory < 0)
				_curhistory = 0;
		
			replaceCommandLine(getHistory(_curhistory));
		break;
		
		case phui::KEYCODE_RETURN: // execute command
		{
			if(_cmdHistory->size() > 0 && !_cmdHistory->front().empty())		
				addToHistory(std::wstring()); // always have empty string in history

			_curhistory = 0;
			_cursor = (int)_text.size();
			std::wstring line(commandLine());

			TextView::KeyDown(keycode, unicode);

			executeCommandLine(line);
		}
		break;
		
		default:
			handled = false;
		break;
	}
	
	if(!handled)
		return TextView::KeyDown(keycode, unicode);
		
	return true;
}


void ConsoleView::AddText(const char* text)
{
	float current = _contentheight;
	TextView::AddText(text);
	ScrollBy(0, _contentheight - current);
		/*
//	int len = (int)strlen(text);

	// insert at the beginning, pushing everything down
	_text.insert(0, utf8_to_utf16((const unsigned char*)text));

	//for(int i = 0; i < len; i++)
	//	_text += text[i];
	
	UpdateLines();
	*/
}

/************************************************************************************************
	getHistory
*************************************************************************************************/

std::wstring ConsoleView::commandLine() const
{
	unsigned int n = (unsigned int)_text.find_last_of('\n');
	
	if(n != std::wstring::npos)
	{
		if(n == _text.size() - 1)
			return std::wstring();
		else
			return _text.substr(n+1);
	}else
		return _text;
}


/************************************************************************************************
	getHistory
*************************************************************************************************/

std::wstring ConsoleView::getHistory(unsigned int idx) const
{
	CmdHistory::const_iterator it = _cmdHistory->begin();
	for(unsigned int i = 0; it != _cmdHistory->end(); ++it, ++i)
	{
		if(i == idx || i == _cmdHistory->size() - 1)
		{
			return *it;
		}
	}
	
	return std::wstring();
}

/************************************************************************************************
	addToHistory
*************************************************************************************************/

void ConsoleView::addToHistory(std::wstring const& str)
{
	_cmdHistory->push_front(str);
}

/************************************************************************************************
	executeCommandLine
*************************************************************************************************/

void ConsoleView::executeCommandLine(std::wstring const& in_line)
{
	char argstr[256];
	unsigned int argc, pos;
	const char* argv[MAX_ARGS];
	std::string ascii;

	int len = (int)in_line.size();	
	for(int i = 0; i < len; i++) ascii += (int)in_line[i];
	strncpy(argstr, ascii.c_str(), 255);
	
	if(_cmdCallback == NULL || ascii.empty()) return;
	
	pos = (unsigned int)ascii.find_first_not_of(' ');
	
	for(argc = 0; argc < MAX_ARGS && pos != std::wstring::npos && pos < 256;)
	{
		argv[argc++] = &argstr[pos];

		pos = (unsigned int)ascii.find(' ', pos);
		
		if(pos == (unsigned int)std::wstring::npos)		
			break;

		argstr[pos] = '\0';
		pos = (unsigned int)ascii.find_first_not_of(' ', pos + 1);
	}
	
	if(argc > 0)
		_cmdCallback(ascii.c_str(), argc, argv, _cmdCallbackRefcon);
}

/************************************************************************************************
	doTabCommandCompletion
*************************************************************************************************/

void ConsoleView::doTabCommandCompletion(std::wstring const& line)
{
	std::wstring matchText, newText, matches[MAX_MATCHES];
	const char* matchcstr;
	unsigned int i, nummatches;
	int pos;
	
	if(_cmdCompletion == NULL) return;

	pos = (int)line.rfind(' ');
	if(pos == (int)std::wstring::npos) pos = -1;
	
	matchText = line.substr(pos + 1);
	
	for(i = 0; i < MAX_MATCHES; i++)
	{
		matchcstr = NULL;// fix _cmdCompletion(line.c_str(), matchText.c_str(), i, _cmdCompletionRefcon);
		
		if(matchcstr == NULL)
			break;
			
//		matches[i] = matchcstr;
	}
	
	nummatches = i;
	if(nummatches > 1)
	{
	/*	if(_out)
		{
			for(i = 0; i < nummatches; i++)
			{
				(*_out) << matches[i];
				if(i < nummatches - 1) (*_out) << " ";
			}   
			(*_out) << endl;
		}*/
		
		// match as much a possible to get replaceText
		for(pos = 0; pos < (int)matches[0].size(); pos++)
		{
			bool nomatch = false;
			char ch = (char)matches[0][pos];
			for(i = 1; i < nummatches; i++)
			{
				if(pos >= (int)matches[i].size())
				{
					nomatch = true;
					break;
				}
				
				if(matches[i][pos] != ch)
				{
					nomatch = true;
					break;
				}
			}		
			
			if(nomatch)
				break;
			
			if(pos < (int)matchText.size() && matchText[pos] != ch) 
				break; // just in case they sent us a strings that don't match

			newText += ch;			
		}
	}else if(nummatches == 1)
	{
		newText = matches[0];
	}

	if(!newText.empty())
	{
		replaceCommandLine(newText);
		setCurrentHistory(commandLine());
	}
}

/************************************************************************************************
	setCurrentHistory
*************************************************************************************************/

void ConsoleView::setCurrentHistory(std::wstring const& str)
{	
	if(_cmdHistory->size() > 0) 
		_cmdHistory->pop_front();
	
	_cmdHistory->push_front(str);
	
	if(0)//_cmdHistory->size() > 1)
	{ 
		if(getHistory(0) == getHistory(1))
			_cmdHistory->pop_front();
	}
}

/************************************************************************************************
	replaceCommandLine
*************************************************************************************************/

void ConsoleView::replaceCommandLine(std::wstring const& str)
{
	unsigned int n = (unsigned int)_text.find_last_of('\n');
	
	if(n == std::wstring::npos)
		n = 0;
	
	if(!_text.empty() && n < _text.size() - 1)	
		_text.erase(n+1);
		
	_text += str;
	_cursor = (int)_text.size();
}

