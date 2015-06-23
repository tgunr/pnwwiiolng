#pragma once
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

namespace World
{
	namespace Meters
	{
		// Top right of game world
		namespace TopRight { enum { X = 10011200, Y = 2675200 }; }
		// Bottom left
		namespace BottomLeft { enum { X = -332800,	Y = 921600 }; }

		enum { Width = TopRight::X - BottomLeft::X };
		enum { Height = TopRight::Y - BottomLeft::Y };

		enum { OctetSize = 800, GridCellSize = 1600 };
	}

	namespace Cells
	{
		enum { Width = Meters::Width / Meters::GridCellSize };
		enum { Height = Meters::Height / Meters::GridCellSize };
	}
}

#include "host/hostGridCell.h"

