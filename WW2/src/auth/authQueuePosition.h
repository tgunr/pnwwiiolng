#pragma once
// Created: Sun Jan 24 01:17:56 CST 2010
//
// Simple structure for telling a playgate it is in a login queue
// at position X with an expected wait of N seconds.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Auth
{
	struct QueuePosition
	{
		QueuePosition(UINT32 position, UINT32 waitTime)
			: m_position(position), m_waitTime(waitTime)
			{}
		QueuePosition(ACTION_OBJECT* const a)
			: m_position(0), m_waitTime(0)
			{
				packer(TEUL_UNPACK, a);
			}
		SINT32 packer(PackMode mode, ACTION_OBJECT* a)
		{
			a->packer(mode, &m_position);
			a->packer(mode, &m_waitTime);
			return a->offset;
		}

		UINT32 m_position, m_waitTime;
	};
}

