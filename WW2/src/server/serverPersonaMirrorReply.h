#pragma once
// Created: Mon Apr 25 15:29:46 CDT 2011
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Oliver "kfsone" Smith

#ifndef EINVAL
#include <errno.h>
#endif

namespace Server
{
	//////////////////////////////////////////////////////////////////////
	// Reply to a player mirror.

	class PersonaMirrorReply : public Transfer
	{
		public: PersonaMirrorReply(const Transfer& rhs, const SINT32 resultCode_=-EINVAL) : Transfer(rhs), resultCode(resultCode_) {}
		virtual ~PersonaMirrorReply() {}
		public: virtual void packer(LOGICAL mode, ACTION_OBJECT* const a) OVERRIDE_FN
		{
			Transfer::packer(mode, a) ;
			a->packer(mode, resultCode) ;
		}
		public: PersonaMirrorReply() : Transfer(XferReason::Mirror), resultCode(-EINVAL) {}
		public: virtual SINT32 onReceive(SINT32 teulIndex) OVERRIDE_FN ;
		public: virtual SINT32 onComplete(SINT32 /*resultCode*/) const OVERRIDE_FN { return -EINVAL ; }
		public: SINT32 resultCode ;
	} ;
}

