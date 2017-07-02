#include <syscfg.h> // Path is project specific.
// Functions for Auth::ClientData
// Copyright (C) PlayNet Inc 2010

#include "auth/authClientData.h"
#include "auth/authTypes.h"

//////////////////////////////////////////////////////////////////////
// Destructor for ClientData -- free the launch request if it was used.

Auth::ClientData::~ClientData()
{
	delete m_launchRequest;
	m_launchRequest = nullptr;
}
