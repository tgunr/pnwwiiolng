#ifndef PHUI_CONSOLEVIEW_H
#define PHUI_CONSOLEVIEW_H

#include <streambuf>

#include "phui/TextView.h"

namespace phui
{
/************************************************************************************************
	ConsoleView
*************************************************************************************************/
/**
	A terminal console, also an honest to god stream buffer. You can set this to cout by:
	std::cout.rdbuf(terminal);
	
*/
class ConsoleView : public TextView
#ifdef CONSOLEVIEW_IS_COUT
	, public std::basic_streambuf<char>
#endif
{
public:
	ConsoleView();
	virtual ~ConsoleView();

	virtual bool KeyDown(int keycode, unsigned short unicode);
	virtual void AddText(const char* text);

	enum { MAX_ARGS = 128, MAX_MATCHES = 128 };
	typedef int (*cmd_callback)(const char* line, int argc, const char** argv, int refcon);
	typedef const char* (*cmd_completion_callback)(const char* line, const char* matchtext, int idx, int refcon);
	
	// would be cool to do as Message but a callback is probably more appropriate in this case.
	void setCmdCallback(cmd_callback cb, int refcon=0) { _cmdCallback = cb;	_cmdCallbackRefcon = refcon;}
	void setCmdCompletionCallback(cmd_completion_callback cb, int refcon=0) {_cmdCompletion = cb; _cmdCompletionRefcon = refcon;}

	std::wstring getHistory(unsigned int idx) const;
	void addToHistory(std::wstring const&);
	void executeCommandLine(std::wstring const&);

// stdio
#ifdef CONSOLEVIEW_IS_COUT
	int_type overflow(int_type c = traits_type::eof())	
	{
		if (c == traits_type::eof()) return traits_type::not_eof(c);
		_text += traits_type::to_char_type(c);
		_cur_pos++;
		return c;
	}
//	virtual pos_type seekpos(pos_type, int) { return pos_type(-1); }
#endif
protected:
	std::wstring commandLine() const;
	void doTabCommandCompletion(std::wstring const&);
	void setCurrentHistory(std::wstring const&);
	void replaceCommandLine(std::wstring const&);

private:
	class CmdHistory;
	CmdHistory* _cmdHistory;
	int _curhistory;
	
	int _cmdCallbackRefcon;
	cmd_callback _cmdCallback;
	int _cmdCompletionRefcon;
	cmd_completion_callback _cmdCompletion;
};

}
#endif // PHUI_CONSOLEVIEW_H
