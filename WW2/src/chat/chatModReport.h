#pragma once
// Created: Tue Jul 26 16:33:41 CDT 2005
//
// Transport for notifying chat that someone has modified their
// client or tampered with the data stream
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct MemoryRequest;

namespace Chat
{
	enum
	{
		ModUnknown, ModUpdateSeq, ModUpdatePrime, ModTamper, ModResupply, ModDamned, ModHack, ModMax
	};
	using ModReport = unsigned int;
	void SendModReport(const ChatHandle handle, const unsigned int report);
	// TODO: Change the header name since this is more generic anti-hack stuff now.
	void SendMemoryRequestResult(playerid_t player, MemoryRequest& req);
}
