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
	// @brief Describes a persona exiting a server/the cluster.

	class PersonaExit : public PersonaXfer
	{
		public: PersonaExit(UINT32 _exitCode, const PersonaXferData& _data)
			: PersonaXfer(XferReason::Exit, _data.dispatch, _data.auxSize, _data.auxData)
			, exitCode(_exitCode)
			{}
		public: PersonaExit() : PersonaXfer(XferReason::Exit), exitCode(0) {}

		public: UINT32 exitCode;

		public: virtual SINT32 onReceive(SINT32 teulIndex) override;
		public: virtual void packer(PackMode mode, ACTION_OBJECT* const a) override
		{
			PersonaXfer::packer(mode, a);
			a->packer(mode, exitCode);
		}
	};

}
