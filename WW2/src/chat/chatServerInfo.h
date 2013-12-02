#pragma once
// Created: Sun Jan 13 15:43:59 CST 2008
//
// CHAT_SERVER_INFO structure
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct ChatServerInfo
{
	//Message sent down to the front end apprising it of where
	//	to log into the chat server
	ChatServerInfo(UINT32 otp, UINT32 ip, UINT16 port) : m_otp(otp), m_ipAddress(ip), m_port(port) {}
	ChatServerInfo() : m_otp(0), m_ipAddress(0), m_port(0) {}

	UINT32	m_otp;			// One Time Password to use during connect
	UINT32	m_ipAddress;	// IP address of the chat server
	UINT16	m_port;			// Port of the chat server

	// Member functions
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);
};
