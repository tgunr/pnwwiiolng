#pragma once
// Date: Mon Jun 27 2005
//
// Describes an exception class for Network Errors
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Borrowed from HTTP/1.1
enum
{
	FB_SEND_AC_FAIL	=	400
,	FB_CONN_FAIL	=	404
,	FB_CONN_BAD		=	406
,	FB_CONN_GONE	=	410
};

class ClientNetError
{
public:
	ClientNetError(const char* what) : m_error(-1), m_what(what) {}
	ClientNetError(int error, const char* what) : m_error(error), m_what(what) {}

	int Error() const { return m_error; }
	const char* What() const { return m_what; }

private:
	int m_error;
	const char* m_what;
};
