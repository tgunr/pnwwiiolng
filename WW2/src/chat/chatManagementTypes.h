#pragma once
// Created: Thu Jul  5 11:59:59 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

namespace Chat
{
	struct ManagementLogin2
	{
		ManagementLogin2() : m_version(1), m_wantHandle(0ULL) {}

		UINT16		m_version;
		// Desired username.
		ChatHandle	m_wantHandle;

		// Server-side login function
		bool Login(SINT32 teulIndex);

		// Network packer
		SINT32 packer(PackMode, ACTION_OBJECT*);
	};
}
