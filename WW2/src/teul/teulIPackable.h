#pragma once
// Created: Tue Apr 26 13:34:06 CDT 2011
//
// Interface for packable objects.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct ACTION_OBJECT;

namespace Teul
{

	//! Packable entity interface.
	struct IPackable
	{
		enum Direction
		{
			Receive			// Equivalent to TEUL_UNPACK
		,	Send			// Equivalent to TEUL_PACK
		};

		// Abstract class, so the compiler knows we don't need a vtable.
		virtual ~IPackable() = 0;

		// Packer routine.
		virtual void packer(Direction, ACTION_OBJECT* a) = 0;
	};

} // namespace Teul

