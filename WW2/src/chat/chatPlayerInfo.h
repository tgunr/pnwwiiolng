#pragma once
// Created: Mon Aug 22 17:33:25 CDT 2011
//
// Describes the Chat::PlayerInfo structure.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct ACTION_OBJECT;

namespace Chat
{
	struct PlayerInfo
	{
		UINT32			playerID;
		UINT32			otp;
		UINT32			squadID;
		ChatHandle		m_handle;
		PermissionMask	m_permissionMask;

		SINT32 packer(PackMode mode, ACTION_OBJECT* a);
	};

} // namespace Chat

