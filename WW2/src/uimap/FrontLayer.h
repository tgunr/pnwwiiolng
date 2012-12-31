#ifndef PN_UIMAP_FRONTLAYER_H
#define PN_UIMAP_FRONTLAYER_H 1

#include <map>
#include "uimap/SuperCellMapView.h"
#include "wwii/wwiiHCUnits.h"
#include "list/list.h"
#include "sdm/StratDB.h"

/************************************************************************************************
	StratLayer
*************************************************************************************************/

namespace StratDB{ class StratObject; }

class FrontLayer : public SuperCellMapView::Renderer
{
public:

	StratDB::StratObject* _cp;
	StratDB::StratObjList _list;
	StratDB::StratObjList _alliedCPs, _axisCPs;


	struct FrontSegment
	{
		GLfloat* points;
	};

    FrontLayer() {};
	~FrontLayer() {};

	bool FrontLayer::isCPOnFront(StratDB::StratObject* cp);

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) { return 0; }

	void CalculateFrontLine();

protected:

	std::vector<FrontSegment> _segments;
};

#endif // PN_UIMAP_FRONTLAYER_H
