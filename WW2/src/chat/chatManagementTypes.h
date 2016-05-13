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
	struct ManagementLogin3
	{
		ManagementLogin3() : m_version(1), m_wantHandle(0ULL), m_country(0) {}

		UINT16		m_version;
		// Desired username.
		ChatHandle	m_wantHandle;
		// Desired persona (0 for none).
		UINT16		m_country;

		// Server-side login function
		bool Login(SINT32 teulIndex);

		// Network packer
		SINT32 packer(LOGICAL, ACTION_OBJECT*);
	};
}
