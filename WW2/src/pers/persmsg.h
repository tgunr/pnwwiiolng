#pragma once

#include "wwii/wwiiOwnershipEnums.h"

extern LOGICAL persmsgBuilderMessage(struct FULL_OBJ_ID* pID, LOGICAL bOn);
extern LOGICAL persmsgDraw();
extern LOGICAL persmsgInit();
extern LOGICAL persmsgKill();

extern bool persmsgAddMessage(const char* messagefile, struct OBJECT* o);

extern void persmsgSetCanSpawn(LOGICAL bSpawn);
extern void persmsgSetTableOwner(WWII_COUNTRY wwiiCountry);

extern char* persmsgTableFacility();
