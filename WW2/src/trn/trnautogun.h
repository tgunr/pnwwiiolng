#pragma once

extern LOGICAL trnInitAutogunSystem(void);
extern LOGICAL trnKillAutogunSystem(void);
extern LOGICAL trnKillAutogun(struct trnAutoGun ** AutoGun);
extern LOGICAL trnAddAutoGun(trnCDTerrainObject * TO);
extern LOGICAL trnUpdateAutoGuns(void);
extern LOGICAL trnAutoVehicleDisappearing(void*  Vehicle);
extern LOGICAL trnUpdateAutogunStatus(void*  InAG,SINT32 country,SINT32 vislevel);
extern LOGICAL trnGetAutoGunHead(struct trnAutoGun ** AG);

extern void trnautogunToggleAI();
