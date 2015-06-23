#pragma once

//////////////////////////////////////////////////////////////////////
// Throwable error classes for dot command functions

namespace Chat
{

class LStringErr
{
public:
	explicit LStringErr(const char* err);
	explicit LStringErr(const char* err, const char* arg);
	explicit LStringErr(LSTRING_ID lstring, const char* arg1="", const char* arg2="")
		: m_lstring(lstring), m_arg1(arg1), m_arg2(arg2) {}

	LSTRING_ID m_lstring;
	std::string m_arg1, m_arg2;
};

}
