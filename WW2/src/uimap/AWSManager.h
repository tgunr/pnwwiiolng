#pragma once

#include <map>
#include "map/mapAWS.h"

// ----------- defines

#define GRID_LEVELS 3
#define GRID_SIZE 16000
#define AWS_ROWS 60
#define AWS_COLS 100
#define AWS_ORIGINX (-21 * GRID_SIZE)
#define AWS_ORIGINY (168 * GRID_SIZE)
//#define AWS_ORIGINX (-26 * SDM_SUPERCELL_SIZE)
//#define AWS_ORIGINY (210 * SDM_SUPERCELL_SIZE)
#define AWS_MAX_VIEW 16
#define AWS_WIDTH (GRID_SIZE * AWS_COLS)
#define AWS_HEIGHT (GRID_SIZE * AWS_ROWS)
#define AWS_GRID_INDEX(c,r) ((r*AWS_COLS)+c)
#define AWS_INITIAL_DELAY 60000
#define AWS_DEFAULT_REFRESH 60000
#define AWS_MAX_REFRESH 5000
#define AWS_CHANGE_DELAY 5000
#define AWS_ACCESS_DELAY 10000

// ----------- types

struct AWSGrid
{
	UINT32 index;
	SINT32 level;
	SINT32 expires;
};

typedef std::map<UINT32,AWSGrid> GridMap;
typedef std::map<UINT32,AWSGrid*> GridView;

/************************************************************************************************
	AWSManager
*************************************************************************************************/

class AWSManager
{
public:

    AWSManager();
	~AWSManager();

	void Init();

	// performs grid updates
	void Update();

	// allows client maps to pull levels about their view of the map
	bool GetView(GridView& view, int scx1, int scy1, int scx2, int scy2);

	AWSGrid* GetGrid(UINT32 index);
	UINT32 GetRows() const;
	UINT32 GetColumns() const;
	UINT32 GetLevels() const;
	UINT32 GetGridSize() const;
	UINT32 GetGridIndex(UINT32 column, UINT32 row) const;
	FLOAT32 GetRefreshCompletion() const;
	void GetSCPosition(SINT32 scx, SINT32 scy, UINT32* column, UINT32* row) const;
	void GetTheaterOffset(float* xoffset, float* yoffset) const;
	void SetGridLevel(UINT32 index, SINT32 level);
	void SetGridExpires(UINT32 index);
	void ResetRow(UINT32 r);

	// receive a row of data from the host
	void ReceiveRow(const AWS::Row& row);
	void ReceiveResponse();

	// gets the master instance of the manager
	static AWSManager* GetInstance();

private:

	// all grids on the map
	GridMap _gridMap;

	// size of AWS grid (defaults to 16x16km)
	UINT32 _gridSize;

	// number of rows (0 to x)
	UINT32 _rows;

	// number of columns (A to X)
	UINT32 _columns;

	// my current updateview
	UINT32 _colMin;
	UINT32 _colMax;
	UINT32 _rowMin;
	UINT32 _rowMax;

	// which row I'm currently updating
	UINT32 _row;

	// how many levels of activity do we show (grid square color)
	UINT32 _levels;

	// when to request an update in ms
	int _refreshTime;

	// how long between refreshes in ms
	int _refreshInterval;

	// last time a layer requested data
	int _lastAccess;

	// has the view changed since we last requested
	bool _modified;

	// is a request pending?
	bool _pending;

	// theater dimensions in supercells
	FLOAT64 _theaterX;
	FLOAT64 _theaterY;
	FLOAT64 _theaterW;
	FLOAT64 _theaterH;
};
