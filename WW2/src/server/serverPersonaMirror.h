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
	// Describes a player mirror (isn't this a spawn?)

	class PersonaMirror : public PersonaXfer
	{
		public: PersonaMirror(const PersonaXferData& _data, class Server::ServerInfo* const _destination)
			: PersonaXfer(XferReason::Mirror, _data, _destination)
			{}
		virtual ~PersonaMirror() {}
		public: PersonaMirror(void* auxBuffer_=nullptr) : PersonaXfer(Transfer::XferReason::Mirror)
		{
			data.auxSize = 0;
			data.auxData = auxBuffer_;
		}
		public: virtual SINT32 onReceive(SINT32 teulIndex) override;
		public: virtual SINT32 onComplete(SINT32 resultCode) const override;
		public: virtual void packer(PackMode mode, ACTION_OBJECT* const a) override { PersonaXfer::packer(mode, a); }
		public: SINT32 send(const playerid_t playerID, class Server::ServerInfo* dest);
	};
}

