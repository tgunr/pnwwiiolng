#pragma once

LOGICAL trnInitAutogunSystem();
LOGICAL trnKillAutogunSystem();
LOGICAL trnKillAutogun(struct trnAutoGun ** AutoGun);
LOGICAL trnAddAutoGun(trnCDTerrainObject * TO);
LOGICAL trnUpdateAutoGuns();
LOGICAL trnAutoVehicleDisappearing(void*  Vehicle);
LOGICAL trnUpdateAutogunStatus(void*  InAG,SINT32 country,SINT32 vislevel);
LOGICAL trnGetAutoGunHead(struct trnAutoGun ** AG);

void trnautogunToggleAI();
