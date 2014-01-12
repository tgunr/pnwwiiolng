#pragma once
// Created: Mon Apr 25 15:29:46 CDT 2011
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Oliver "kfsone" Smith

#include "server/serverPersonaXferData.h"
#ifndef EINVAL
#include <errno.h>
#endif

namespace Server
{
	//////////////////////////////////////////////////////////////////////
	// @brief Class for describing a player transfer or copy between
	// servers.

	class PersonaXfer : public Transfer
	{
	public:
		PersonaXfer(XferReason _reason=XferReason::Login) : Transfer(_reason), data() {}

	public:
		PersonaXfer(XferReason _reason, const Auth::Dispatch& _dispatch, SINT32 _size=0, void* _auxData=nullptr, UINT64 _source=0)
			: Transfer(_reason)
			, data(_dispatch, _size, _auxData)
			{
				m_source = _source ;
			}

		virtual ~PersonaXfer() {}

		PersonaXfer(XferReason _reason, const PersonaXferData& _data, Server::ServerInfo* const _destination)
			: Transfer(_reason, _destination)
			, data(_data)
			{}

		Auth::Dispatch& dispatch() { return data.dispatch ; }
		const Auth::Dispatch& dispatch() const { return data.dispatch ; }

		public: virtual SINT32 onComplete(SINT32 /* result */) const OVERRIDE_FN { return -EINVAL ; }
		public: virtual void packer(LOGICAL mode, ACTION_OBJECT* const a) OVERRIDE_FN
		{
			Transfer::packer(mode, a) ;
			data.packer(mode, a) ;
		}

	public:
		PersonaXferData		data ;
	} ;
}

