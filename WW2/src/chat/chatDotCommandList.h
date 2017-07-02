#pragma once
// Created: Fri Sep 30 16:13:36 CDT 2011
// Originating Programmer: Oliver "kfsone" Smith
// Copyright: PlayNet Inc, 2011 -- All rights reserved.
// Describes all of the chat commands known by the chat host.

#include <vector>
#include "teul/teulExtract.h"

namespace Chat
{
	using cmdcount_t = unsigned short;

	struct HostCommand
	{
		UCHAR			playnetOnly;	// 0 for normal users, 1 for playnet only
		std::string 	command;		// The command itself.
		SINT32			usage;			// usage help (lstr id)
		SINT32			detail;			// more detailed help (lstr id)
	};

	using HostCommands = std::vector<HostCommand>;

	static inline HostCommands GetHostCommandsFromStream(const UCHAR* source)
	{
		HostCommands cmdList;

		// Starts with a command count.
		const cmdcount_t& numCommands = *(const cmdcount_t*)source;
		source += sizeof(numCommands);

		cmdList.resize(numCommands);

		for ( size_t i = 0; i < numCommands; ++i )
		{
			HostCommand& cur = cmdList[i];
			cur.playnetOnly = Teul::ExtractAndAdvance<const UCHAR>(source);

			cur.command = reinterpret_cast<const char*>(source);
			// Size (bytes) plus zero.
			source += cur.command.size() + 1;

			cur.usage = Teul::ExtractAndAdvance<SINT16>(source);
			cur.detail = Teul::ExtractAndAdvance<SINT16>(source);
		}

		return cmdList;
	}

	extern HostCommands g_hostDotCommands;
}
