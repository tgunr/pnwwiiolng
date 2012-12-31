#include <syscfg.h> // Path is project specific.
// Functions for Auth::ClientData
// Copyright (C) PlayNet Inc 2010

#include "auth/authClientData.h"
#include "auth/authQueue.h"
#include "auth/authTypes.h"
#include <assert.h>

#define USE_LOGIN_QUEUES

//////////////////////////////////////////////////////////////////////
// Destructor for ClientData -- free the launch request if it was used.

Auth::ClientData::~ClientData()
{
	if ( isQueued )
		RemoveFromQueue(false) ;

	if ( m_launchRequest != NULL )
	{
		delete m_launchRequest ;
		m_launchRequest = NULL ;
	}
}

//////////////////////////////////////////////////////////////////////
// Add a player to the login queue for an arena.

void
Auth::ClientData::AddToQueue(const APP_LAUNCH_REQ& request)
{
#if defined(USE_LOGIN_QUEUES)
	// Make sure we don't get double-queued.
	RemoveFromQueue(false) ;

	// Add us into the queue.
	Auth::Queue::AddPlayer((UINT32)request.arenaID, this, isPremium) ;

	// Mark us as being queued
	isQueued = true ;

	// Take a copy of the request
	if ( m_launchRequest == NULL )
		m_launchRequest = new APP_LAUNCH_REQ(request) ;
	else
		*m_launchRequest = request ;
#endif // USE_LOGIN_QUEUES
}

//////////////////////////////////////////////////////////////////////
// Remove a player from the login queue for their arena.

void
Auth::ClientData::RemoveFromQueue(bool enteredGame)
{
#if defined(USE_LOGIN_QUEUES)
	if ( isQueued == false )
		return ;

	assert( m_launchRequest != NULL ) ;

	Auth::Queue::RemovePlayer((UINT32)m_launchRequest->arenaID, this, isPremium, enteredGame) ;

	isQueued = false ;
#endif // USE_LOGIN_QUEUES
}

