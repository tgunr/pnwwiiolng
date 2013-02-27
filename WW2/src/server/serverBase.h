#pragma once

#include "wwii/wwiiServerTypeEnums.h"	// For SERVER_TYPE
struct TEUL_CALLBACK;

class ServerBase
{
public:
	ServerBase(SERVER_TYPE serverType_, const char* serverName_);
	virtual ~ServerBase()
	{
	}

	virtual void OnError(SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/) {}

	SERVER_TYPE ServerType() const { return m_serverType; }
	const char* ServerName() const { return m_serverName; }

private:
	const SERVER_TYPE	m_serverType;
	const char* const	m_serverName;

	static void s_ErrorDelegate(SINT32 teulIndex_, const UCHAR* const data_, SINT32 size_, const TEUL_CALLBACK* const object_, SINT32 error_);
};

extern ServerBase* s_serverBase;
extern ServerBase& GetServerBase() { return *s_serverBase; }
