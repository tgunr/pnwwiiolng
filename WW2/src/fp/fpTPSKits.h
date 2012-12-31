#pragma once

// forward declares
struct gr2AnimTrooper;

// prototypes
extern void fpTPSKitsInit(bool force = false);
extern void fpTPSKitsKill();

// clears any kit related attachments
extern void fpTPSKitsReset(gr2AnimTrooper* t);

// sets a trooper kit
extern void fpTPSKitsSetByVehicle(gr2AnimTrooper* anim, SINT32 index = -1);
extern void fpTPSKitsSetByName(gr2AnimTrooper* anim, const char* kitName);
