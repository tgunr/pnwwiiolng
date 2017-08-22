#include "syscfg.h" // Path is project specific.
// Date: Fri Nov  9 12:42:29 CST 2007
//
// SHARED: Client/host functions
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapAWS.h"				// Our header file
#include "teul/teulPackers.h"

TEUL_CALLBACK* AWS::callback;

//////////////////////////////////////////////////////////////////////
// Constructor from a network action object.

AWS::Request::Request(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer

SINT32
AWS::Request::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_row, 10);

	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_startCol, 12);

	UINT16 dummy = 0;
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &dummy, 4);

	a->packer(mode, m_newView);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Constructor for the network action object

AWS::Grid::Grid(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer

SINT32
AWS::Grid::packer(PackMode mode, ACTION_OBJECT* a)
{
	// Definition of the grid
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_row, 10);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_startCol, 11);

	// Is any data contained in it?
	a->packer(mode, m_hasData);
	if ( m_hasData == false )
		return a->offset;

	for ( UINT32 y = 0; y < AWS::MAX_COLUMNS; ++y )
	{
		// Does this row have data in it?
		a->packer(mode, m_rowHasData[y]);
		if ( m_rowHasData[y] )
		{
			for ( UINT32 x = 0; x < AWS::MAX_COLUMNS; ++x )
			{
				// Bit saving feature: we only need to second 2 bits if
				// there is a presence. Essentially bit 1 is "data/nodata"
				// and bit 2 is "low/high".
				bool low = false, high = false;
				if ( mode == TEUL_PACK )
				{
					if ( m_data[y][x] > NO_PRESENCE )
						low = true;
					if ( m_data[y][x] >= HEAVY_PRESENCE )
						high = true;
				}
				a->packer(mode, low);
				if ( low )
					a->packer(mode, high);
				if ( mode == TEUL_UNPACK )
				{
					m_data[y][x] = high ? HEAVY_PRESENCE : (low ? LIGHT_PRESENCE : NO_PRESENCE);
				}
			}
		}
	}

	return a->offset;
}
