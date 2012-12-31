#include <syscfg.h> // Path is project specific.
#include <string>

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"

#include "gl/glplat.h"

#include "uimap/FrontLayer.h"
#include "sdm/StratDB.h"
#include "strt/strtCPFlags.h"

/************************************************************************************************
	FrontLayer
*************************************************************************************************/


/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/
void FrontLayer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	CalculateFrontLine();

	/*
	//glClearColor(0.0, 0.0, 0.0, 0.0);
	//glShadeModel(GL_FLAT);


	glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrlpoints[0][0]);
	glEnable(GL_MAP1_VERTEX_3);

	// draw color
	glColor3f(1.0, 0, 0);

	// draw line size


	// draw line
	glBegin(GL_LINE_STRIP);
	  for (int i = 0; i <= 30; i++)
		 glEvalCoord1f((GLfloat) i/30.0);
	glEnd();
	*/


	glClearColor(0.0, 0.0, 0.0, 0.0);
	ctx->SetColor(0.0f,1.0f,1.0f,0.9f);
	glShadeModel(GL_SMOOTH);
	glLoadIdentity();
	static float x = 0.0f;
	static float y = 500.0f;
	glLineWidth((GLfloat)5.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(x/SDM_SUPERCELL_SIZE, y/SDM_SUPERCELL_SIZE);
	glEnd();

}

bool FrontLayer::isCPOnFront(StratDB::StratObject* cp)
{
	unsigned int cpFlags = cp->StratFlags();
	return (cpFlags & CP_STAT_ENEMY_LINKED);
}

void FrontLayer::CalculateFrontLine()
{
	StratDB::AllCPsInWorld(_list); // All CPs
	StratDB::StratObjList::iterator cit = _list.begin();

	// Grab every CP on frontline and not a bridge
	for(; cit != _list.end(); cit++)
	{
		StratDB::StratObject* cp = *cit;
		if((cp->StratType() == SDM_CP_BRIDGE) && !isCPOnFront(cp))
			cit = _list.erase(cit);
	}

	StratDB::StratObjList::iterator it = _list.begin();

	// Separate into Axis and Allied CP lists
	for(; it != _list.end(); it++)
	{
		StratDB::StratObject* cp = *it;
		if(cp->Side()== ALLIED)
			_alliedCPs.push_back(cp);
		else if(cp->Side() == AXIS)
			_axisCPs.push_back(cp);
	}


}

