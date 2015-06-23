#pragma once
// Created: Mon Apr 25 15:29:46 CDT 2011
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Oliver "kfsone" Smith

#include "server/serverPersonaXfer.h"

namespace Server
{
	//////////////////////////////////////////////////////////////////////
	// @brief Describes a player logging in.

	class PersonaLogin : public PersonaXfer
	{
		public: PersonaLogin(const Auth::Dispatch& _dispatch)
			: PersonaXfer(XferReason::Login, _dispatch, 0, nullptr, 0)
			{}

		virtual ~PersonaLogin() {}
	};
}

