#include <syscfg.h> // Path is project specific.

#include "uimap/AWSManager.h"
#include "client/GameClient.h"
#include "sdm/sdm.h"
#include "include/vars.h"
#include "spt/Random.h"

// private variables

// singleton
static AWSManager _gridMgr;

/************************************************************************************************
	AWSLayer
*************************************************************************************************/

AWSManager::AWSManager() :
	_gridSize(GRID_SIZE),
	_rows(AWS_ROWS),
	_columns(AWS_COLS),
	_colMin(0),
	_colMax(0),
	_rowMin(0),
	_rowMax(0),
	_row(0),
	_levels(GRID_LEVELS),
	_refreshTime(frametime + AWS_INITIAL_DELAY),
	_refreshInterval((int)(AWS::UPDATE_INTERVAL_S * 1000)),
	_lastAccess(0),
	_modified(true),
	_pending(false),
	_theaterX(AWS_ORIGINX),
	_theaterY(AWS_ORIGINY),
	_theaterW(AWS_WIDTH),
	_theaterH(AWS_HEIGHT)
{
}

AWSManager::~AWSManager()
{
}

void AWSManager::Update()
{
	int rows = _rowMax - _rowMin;

	if(_lastAccess && (frametime - _lastAccess) < AWS_ACCESS_DELAY &&
	  !_pending && rows && frametime >= _refreshTime)
	{
		_refreshTime = frametime + _refreshInterval;

		UINT32 width	= _colMax - _colMin;
		UINT32 row		= _rowMin + (int)(rows/2) - (int)(AWS_MAX_VIEW/2);
		UINT32 col		= _colMin + (int)(width/2) - (int)(AWS_MAX_VIEW/2);

		// create a request
		AWS::Request request(row, col, _modified);

		// send the request
		request.send(PlayerClient()->ChatConnection());

		_pending	= true;
		_modified	= false;
	}
}

bool AWSManager::GetView(GridView& view, int scx1, int scy1, int scx2, int scy2)
{
	static int lastSCX1 = 0;
	static int lastSCY1 = 0;
	static int lastSCX2 = 0;
	static int lastSCY2 = 0;

	if(scx1 != lastSCX1 || scy1 != lastSCY1 || scx2 != lastSCX2 || scy2 != lastSCY2)
	{
		lastSCX1 = scx1;
		lastSCY1 = scy1;
		lastSCX2 = scx2;
		lastSCY2 = scy2;

		GetSCPosition(scx1, scy1, &_colMin, &_rowMin);
		GetSCPosition(scx2, scy2, &_colMax, &_rowMax);

		if(_colMax >= _columns)
			_colMax = _columns - 1;

		if(_rowMax >= _rows)
			_rowMax = _rows - 1;

		// add a buffer to the current view
		if(_colMin > 0) --_colMin;
		if(_rowMin > 0) --_rowMin;
		if(_colMax < _columns - 1) ++_colMax;
		if(_rowMax < _rows - 1) ++_rowMax;

		_row			= _rowMin;
		_modified		= true;
	}

	view.clear();

	for(UINT32 c = _colMin; c < _colMax; c++)
	{
		for(UINT32 r = _rowMin; r < _rowMax; r++)
		{
			AWSGrid* grid = GetGrid(GetGridIndex(c, r));
			if(grid) view[grid->index] = grid;
		}
	}

	// update the last time a layer accessed us
	_lastAccess = frametime;

	return true;
}

UINT32 AWSManager::GetRows() const
{
	return _rows;
}

UINT32 AWSManager::GetColumns() const
{
	return _columns;
}

UINT32 AWSManager::GetLevels() const
{
	return _levels;
}

UINT32 AWSManager::GetGridSize() const
{
	return _gridSize;
}

UINT32 AWSManager::GetGridIndex(UINT32 column, UINT32 row) const
{
	return (row * GetColumns()) + column;
}

FLOAT32 AWSManager::GetRefreshCompletion() const
{
	return (frametime <= _refreshTime) ? 1.0f - ((float)(_refreshTime - frametime)/_refreshInterval): 0;
}

void AWSManager::GetSCPosition(SINT32 scx, SINT32 scy, UINT32* column, UINT32* row) const
{
	FLOAT64 x = scx * SDM_SUPERCELL_SIZE;
	FLOAT64 y = scy * SDM_SUPERCELL_SIZE;

	*column = *row = 0;

	if(x < _theaterX)
		*column = 0;
	else if(x >= _theaterX + _theaterW)
		*column = (UINT32)(_theaterW/_gridSize);
	else
		*column = (UINT32)((x + -_theaterX)/_gridSize);

	if(y < _theaterY)
		*row = 0;
	else if(y >= _theaterY + _theaterH)
		*row = (UINT32)(_theaterH/_gridSize);
	else
		*row = (UINT32)((y + -_theaterY)/_gridSize);
}

void AWSManager::GetTheaterOffset(float* xoffset, float* yoffset) const
{
	*xoffset = (float)(_theaterX/SDM_SUPERCELL_SIZE);
	*yoffset = (float)(_theaterY/SDM_SUPERCELL_SIZE);
}

void AWSManager::SetGridLevel(UINT32 index, SINT32 level)
{
	AWSGrid* grid = GetGrid(index);

	if(grid)
	{
		grid->level		= level;
		grid->expires	= frametime + (_refreshInterval * 2);
	}
}

void AWSManager::SetGridExpires(UINT32 index)
{
	AWSGrid* grid = GetGrid(index);
	if(grid) grid->expires = frametime + (_refreshInterval * 2);
}

void AWSManager::ResetRow(UINT32 r)
{
	for(UINT32 c = 0; c < AWS_COLS; c++)
		SetGridLevel(GetGridIndex(c, r), 0);
}

void AWSManager::ReceiveRow(const AWS::Row& row)
{
	if(row.m_hasData)
	{
		// we have data, save it in the grid map
		for(UINT32 i = 0; i < AWS::MAX_COLUMNS; i++)
			SetGridLevel(GetGridIndex(i + row.m_startCol, row.m_row), row.m_data[i]);
	}
}

void AWSManager::ReceiveResponse()
{
	// we got a response
	_pending = false;
}

AWSGrid* AWSManager::GetGrid(UINT32 index)
{
	// index must be on the grid
	if(index > (AWS_COLS * AWS_ROWS))
		return nullptr;

	GridMap::iterator found = _gridMap.find(index);

	// if we don't find it, add it
	if(found == _gridMap.end())
	{
		AWSGrid grid;

		grid.index		= index;
		grid.level		= 0;
		grid.expires	= 0;

		_gridMap[index] = grid;
	}

	return &(_gridMap[index]);
}

AWSManager* AWSManager::GetInstance()
{
	return &_gridMgr;
}
