#pragma once
// Created: Mon Apr 25 15:29:46 CDT 2011
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Oliver "kfsone" Smith

#include "server/serverTransfer.h"

namespace Server
{
	// Data for a persona transfer
	// + auth dispatch
	// + auxiliary data
	struct PersonaXferData
	{
		//! @brief Default ctor.
		PersonaXferData() : dispatch(), auxSize(0), auxData(nullptr) {}
		//! @brief Initializing ctor.
		PersonaXferData(const Auth::Dispatch& _dispatch, SINT32 _auxSize=0, void* const _auxData=nullptr)
			: dispatch(_dispatch), auxSize(_auxSize), auxData(_auxData)
			{}

		virtual ~PersonaXferData() {}

		public:	Auth::Dispatch	dispatch;

		public:	SINT32			auxSize;
		public:	void*			auxData;

		virtual void packer(LOGICAL mode, ACTION_OBJECT* const a)
		{
			dispatch.packer(mode, a);
			a->packer(mode, auxSize);
			if ( auxSize > 0 )
			{
				assert( auxData != nullptr );
				a->packBytes(mode, auxData, auxSize);
			}
		}
	};
}

