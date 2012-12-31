#pragma once
// Created: Mon Mar 14 11:52:39 CST 2005
//
// Defines standard wwii exceptions
//
// Copyright (c) 2005 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdexcept>

namespace std
{

class request_failed : public logic_error
{
public:

	explicit request_failed(UINT32 withReturnCode, const char* withMessage)
		: logic_error(withMessage), m_returnCode(withReturnCode) {}
	UINT32 returnCode() const { return m_returnCode ; }

protected:
	UINT32 m_returnCode ;
} ;

}

