#pragma once
// Created: Fri Nov  9 12:42:29 CST 2007
//
// SHARED: Client/Host definitions
//
// Air Warning System - A grid system to replace EWS with
// imprecise "presence levels" for aircraft across the map.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct TEUL_CALLBACK;

namespace AWS
{
	// Dimensions of the sides of each grid cell in meters.
	// Grid cells might not be square.
	static const UINT32 CELL_HEIGHT_M = 16000;	// 16km
	static const UINT32 CELL_WIDTH_M  = CELL_HEIGHT_M;

	// Determine the bottom left of the grid
	static const SINT16	GRID_OFFSET_LEFT = 21;	// Column 0 = 21 grids left
	static const SINT16 GRID_OFFSET_UP = 168;	// Row 0 = 168 grids up

	// How often the grid is updated (seconds)
	static const UINT32 UPDATE_INTERVAL_S = 60;

	// Maximum width you can* request*
	static const UINT32 MAX_COLUMNS = 16;

	// What the data means
	enum
	{
		NO_PRESENCE = 0
	,	LIGHT_PRESENCE = 1
	,	HEAVY_PRESENCE = 2
		// We're not using "3" yet.

	,	NO_AWS_DATA = 255		// Client only value, never sent

		// Number of bits used for sending data
	,	DATA_BITS = 2
	};

	// Client->Host Request
	class Request
	{
	public:
		// Ask the host for the specified row between (and
		// including) the specified x values
		Request(UINT16 row, UINT16 startCol, bool newView)
			: m_row(row), m_startCol(startCol), m_newView(newView)
			{}

		// Create me from a network packer
		Request(ACTION_OBJECT* const a);

		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

		// Send me to the chat host - returns false on failure.
		bool send(SINT32 chatConnection);

		//// Members

		// Row number -- essentially the world Y of the cell / CELL_HEIGHT_M
		UINT16		m_row;
		UINT16		m_startCol;		// The start column you want for this row

		// What cycle was the last data we received for this row stamped with,
		// or zero.
		bool		m_newView;
	};


	// Host->client response
	class Row
	{
	public:
		// Constructor
		Row(UINT16 row, bool hasData, UINT16 startCol)
			: m_row(row), m_hasData(hasData)
			, m_startCol(startCol)
			, m_data(nullptr)
			{}

		// Which row this is. It needs to be xfered because lag/loss of
		// connection could cause out-of-sequence delivery.
		UINT16		m_row;

		// Does the row have any data? If this is false, then the row
		// is empty. Note: this has precedence over m_hasChanged.
		bool		m_hasData;

		UINT16		m_startCol;		// Left most column

		// The actual data (m_data[0] to m_data[m_width-1])
		UCHAR*		m_data;
	};

	class Grid
	{
	public:
		Grid(UINT16 row, UINT16 startCol)
			: m_row(row), m_startCol(startCol), m_hasData(false)
			{
				for ( UINT16 y = 0; y < MAX_COLUMNS; ++y )
					m_rowHasData[y] = false;
			}
		Grid(ACTION_OBJECT* const a);

		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

		// Bottom left of the view
		UINT16		m_row, m_startCol;

		// Is there any data in this view?
		bool		m_hasData;

		// The actual data
		bool		m_rowHasData[MAX_COLUMNS];
		UCHAR		m_data[MAX_COLUMNS][MAX_COLUMNS]; // NOTE: m_data[row][column]
	};

	extern TEUL_CALLBACK* callback;

}

///TEMPORARY
#ifdef IS_CHAT_HOST
class Player;
extern void chatProcessAWSRequest(Player& player, const AWS::Request& request);
#endif

