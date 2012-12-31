#include <syscfg.h> // Path is project specific.


#include "weap/weapmain.h"
#include "weap/weapammo.h"
#include "weap/weapons.h"


UINT32 weapInit(void)
{

logTimerBeginExtMacro(weapAmmoInit);
	weapAmmoInit();
logTimerEndExtMacro(weapAmmoInit);

logTimerBeginExtMacro(weaponsInit);
	weaponsInit();
logTimerEndExtMacro(weaponsInit);

	return 0;
}

UINT32 weapKill(void)
{

	weaponsKill();
	weapAmmoKill();

	return 0;
}
