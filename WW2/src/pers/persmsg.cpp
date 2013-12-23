#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////
// Module: persmsg.cpp
// Description: Handle capture message and builder statue
// messages.
//////////////////////////////////////////////////////////
#include <math.h>

#include "ob/obmain.h"
#include "pers/persmsg.h"
//#include "rn/rntextloc.h"
#include "fio/fio.h"
#include "sdm/StratDB.h"
#include "js/jsGame.h"
#include "ui/uiMain.h"
#include "wwii/wwiiFullObjID.h"
#include "ml/mltypes.h"
#include "trn/trnmain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "pers/persmain.h"
#include "ui/uiGameEvents.h"

#include <map>

static LOGICAL g_bCanSpawn = FALSE;

static bool _drawBuilderMessage = false;

static WWII_COUNTRY g_wwiiTableOwner = MAX_COUNTRIES;
static char g_wwiiTableFacility[128];

static SINT32 _messageLength = 0;

typedef struct _popupmessage
{
    bool isdrawn;
    bool dead;

    FLOAT64VECTOR playpos;

    std::string file;

    _popupmessage()
    {
        isdrawn = false;
        dead = false;
    }

} PopUpMessage;

static std::map<OBJECT*, PopUpMessage*> _messages;

//////////////////////////////////////////////////////////
// Function: persmsgInit
// Description:
//////////////////////////////////////////////////////////
LOGICAL persmsgInit()
{
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: persmsgKill
// Description:
//////////////////////////////////////////////////////////
LOGICAL persmsgKill()
{
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: persmsgBuilderMessage
// Description:
//////////////////////////////////////////////////////////
LOGICAL persmsgBuilderMessage(FULL_OBJ_ID* pID, LOGICAL on)
{
    // Already displaying
    if(_drawBuilderMessage)
    {
        _drawBuilderMessage = on;

		// hide the popup if not on
        if(!on) uiGameEvent(GAMEEVENT_POPUP_BUILDER);

        return TRUE;
    }

    if(on && pID)
    {
     	StratDB::StratObject* stratobj =  StratDB::FindStratObject(*(sdm_full_obj_id*)pID);
      	const wchar_t* buildingname = NULL;

      	if(stratobj)
      		buildingname = stratobj->DisplayName();

        if(buildingname == NULL)
        {
            _drawBuilderMessage = false;
            return FALSE;
        }

		// show the popup
		uiGameEvent(GAMEEVENT_POPUP_BUILDER, 1, buildingname);
    }

    _drawBuilderMessage = on;

    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: persmsgPopUpMessage
// Description:
//////////////////////////////////////////////////////////
bool persmsgAddMessage(const char* messagefile, OBJECT* key)
{
    if(!messagefile || !key)
    {
        return false;
    }

    if(_messages.find(key) != _messages.end())
    {
        return true;
    }

    PopUpMessage* popup = new PopUpMessage;

    popup->file = messagefile;

	mlVehicleData* vd = persGetPersonaModelDataPtr();
    popup->playpos = vd->VehiclePos;

    _messages[key] = popup;

    return true;
}

//////////////////////////////////////////////////////////
// Function: _verifyMessage
// Description:
//////////////////////////////////////////////////////////
inline bool _verifyMessage(OBJECT* o, FLOAT64VECTOR* pos)
{
    if(!obmainValidateObject(o))
    {
        return false;
    }

    mlVehicleData* vd = persGetPersonaModelDataPtr();

    FLOAT64VECTOR diff;
    mathSubVectors(&diff, pos, &vd->VehiclePos);

    double dist = mathVectDotProduct(&diff, &diff);

    if(dist > 2.0)
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////
// Function: _drawPopUpMessage
// Description:
//////////////////////////////////////////////////////////
bool _drawPopUpMessage()
{
    if(_messages.empty())
    {
        return true;
    }

    std::map<OBJECT*, PopUpMessage*>::iterator itr;
    std::map<OBJECT*, PopUpMessage*>::iterator free;

    itr = _messages.begin();

    for(; itr != _messages.end(); ++itr)
    {
        if(!_verifyMessage(itr->first, &itr->second->playpos))
        {
            itr->second->dead = true;
			uiGameEvent(GAMEEVENT_POPUP_CONTENT);
        }
        else if(!itr->second->isdrawn)
        {
			uiGameEvent(GAMEEVENT_POPUP_CONTENT, "'%s'", itr->second->file.c_str());
            itr->second->isdrawn = true;
       }
    }

    itr = _messages.begin();

    while(itr != _messages.end())
    {
        if(itr->second->dead)
        {
            free = itr;
            ++itr;

            delete free->second;
            _messages.erase(free);
        }
        else
        {
            ++itr;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////
// Function: persmsgDraw
// Description:
//////////////////////////////////////////////////////////
LOGICAL persmsgDraw()
{
    _drawPopUpMessage();
    return TRUE;
}


//////////////////////////////////////////////////////////
// Function: persmsgCanSpawn
// Description:
//////////////////////////////////////////////////////////
void persmsgSetCanSpawn(LOGICAL bSpawn)
{
    g_bCanSpawn = bSpawn;
}

//////////////////////////////////////////////////////////
// Function: persmsgCanSpawn
// Description:
//////////////////////////////////////////////////////////
void persmsgSetTableOwner(WWII_COUNTRY wwiiCountry)
{
    g_wwiiTableOwner = wwiiCountry;
}

char* persmsgTableFacility()
{
    return g_wwiiTableFacility;
}
