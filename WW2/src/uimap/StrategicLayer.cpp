#include <syscfg.h> // Path is project specific.
#include <string>

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"

#include "gl/glplat.h"

#include "uimap/StrategicLayer.h"
#include "sdm/StratDB.h"

/************************************************************************************************
	StrategicLayer
*************************************************************************************************/

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void StrategicLayer::SetDivision(unsigned int id)
{
	HCLinksMapObject* links = AddHCLinksMapObject();

	if(links)
		links->SetDivision(id);
}

void StrategicLayer::SetCP(StratDB::StratObject* cp)
{
	HCLinksMapObject* links = AddHCLinksMapObject();

	if(links)
		links->SetCP(cp);
}

HCLinksMapObject* StrategicLayer::AddHCLinksMapObject()
{
	HCLinksMapObject* links = dynamic_cast<HCLinksMapObject*>(_strategic[STRATEGIC_HCLINKS].ptr());

	if(!links)
	{
		links = new HCLinksMapObject(this);
		_strategic[STRATEGIC_HCLINKS] = links;
		MapObjects()->push_back(links);
	}

	return links;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

 HCLinksMapObject::HCLinksMapObject(StrategicLayer* layer)
			: SuperCellMapView::MapObject()
			, _division(NULL)
			, _strategiclayer(layer),
			_x1(0),
			_y1(0),
			_x2(0),
			_y2(0)
{
	_visible = true;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void HCLinksMapObject::DrawSelf(SuperCellMapView* mv)
{
	if(!_units.size())
		return;

	HCUnitMap::iterator it = _units.begin();

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	ctx->DisableTexture();

    bool enabled = false;
    if(GLEW_ARB_multisample)
    {
        enabled = glIsEnabled(GL_MULTISAMPLE_ARB);
        glDisable(GL_MULTISAMPLE_ARB);
    }

    ctx->DisableTexture();

	ctx->SetColor(0,0,1,.5f);

	GLfloat current;
	//GLint factor = 10;			// Stippling factor
	//GLushort pattern = 0x5555;	// Stippling pattern

	glGetFloatv(GL_LINE_WIDTH, &current);
	glLineWidth((GLfloat)5.0f);
	//glEnable(GL_LINE_STIPPLE);

	for(; it != _units.end(); ++it)
	{
		HCUnit* division = GetDivision(HCUnit::Lookup(it->second->unitID()));

		if(!division || !it->second->isBrigade() || (_division && division->unitID() != _division->unitID()))
			continue;

		StratDB::StratObject* divHQ		= StratDB::FindStratObject(division->homeCPID());
		StratDB::StratObject* brigHQ	= StratDB::FindStratObject(it->second->homeCPID());

		if(divHQ && brigHQ)
		{
			/*
			bool inSupply = false;

			StratDB::StratObject::ChildList children		= divHQ->ChildStratObjects();
			StratDB::StratObject::ChildList::iterator cit	= children.begin();

			for(; cit != children.end(); ++cit)
			{
				StratDB::StratObject* linkedDepot = (*cit)->GetLinkedDepot();

				if(linkedDepot && linkedDepot->ParentStratObject() == brigHQ)
				{
					inSupply = true;
					break;
				}
			}

			if(inSupply)
			{
				glDisable(GL_LINE_STIPPLE);
			}
			else
			{
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(factor, pattern);
			}
			*/

			if(division->country() == 4)
				ctx->SetColor(1,0,0,.5f);
			else
				ctx->SetColor(0,0,1,.5f);

			glBegin(GL_LINE_STRIP);
			glVertex2f(brigHQ->PosX()/SDM_SUPERCELL_SIZE, brigHQ->PosY()/SDM_SUPERCELL_SIZE);
			glVertex2f(divHQ->PosX()/SDM_SUPERCELL_SIZE, divHQ->PosY()/SDM_SUPERCELL_SIZE);
			glEnd();
		}
	}

	//glDisable(GL_LINE_STIPPLE);
	glLineWidth(current);
	ctx->SetColor(1,1,1,1);

    if(GLEW_ARB_multisample)
    {
        if(enabled)
            glEnable(GL_MULTISAMPLE_ARB);
    }
}

void HCLinksMapObject::SetDivision(unsigned int id)
{
	_division = GetDivision(HCUnit::Lookup(id));
	_cp = (_division) ? StratDB::FindStratObject(_division->homeCPID()): NULL;
	SetUnits();
}

void HCLinksMapObject::SetCP(StratDB::StratObject* cp)
{
	_cp = cp;
	_division = NULL;
	SetUnits();
}

void HCLinksMapObject::SetUnits()
{
	_units.clear();
	_x1 = _y1 = _x2 = _y2 = 0;

	if(_cp)
	{
		const StratDB::StratObject::UnitList& units = _cp->GetUnits();
		StratDB::StratObject::UnitList::const_iterator uit = units.begin();

		for(; uit != units.end(); ++uit)
		{
			HCUnit* division = GetDivision(HCUnit::Lookup(*uit));

			if(division)
			{
				SetBounds(division);

				HCUNIT_INDEX::const_iterator it = hcUnits.begin();

				for(; it != hcUnits.end(); ++it)
				{
					if(it->second->parentID() == division->unitID())
					{
						HCUnit* brigade = it->second;
						_units[brigade->unitID()] = brigade;
						SetBounds(brigade);
					}
				}
			}
		}
	}
}

void HCLinksMapObject::SetBounds(const HCUnit* unit)
{
	if(unit)
	{
		StratDB::StratObject* home = StratDB::FindStratObject(unit->homeCPID());

		if(home)
		{
			if(_x1)
			{
				// push the x coords
				_x1 = std::min(_x1, home->PosX());
				_x2 = std::max(_x2, home->PosX());
				_y1 = std::min(_y1, home->PosY());
				_y2 = std::max(_y2, home->PosY());
			}
			else
			{
				// initial point
				_x1 = _x2 = home->PosX();
				_y1 = _y2 = home->PosY();
			}
		}
	}
}

HCUnit* HCLinksMapObject::GetDivision(HCUnit* unit)
{
	if(unit)
	{
		switch(unit->type())
		{
			case HCU_BRIGADE:
				return unit->parent();
				break;
			case HCU_DIVISION:
				return unit;
				break;
		}
	}

	return NULL;
}
