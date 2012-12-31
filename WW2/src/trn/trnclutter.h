#pragma once

extern UINT32 trnClutterInit(void);
extern UINT32 trnClutterKill(void);
extern UINT32 trnClutterUpdate(void);
extern UINT32 trnClutterReset(void);
extern void trnClutterSetDrawFlag(bool draw);
extern void trnClutterFreeAttributeIfNecessary(size_t key);
