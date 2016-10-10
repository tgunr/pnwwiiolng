#pragma once

UINT32 trnClutterInit();
UINT32 trnClutterKill();
UINT32 trnClutterUpdate();
UINT32 trnClutterReset();
void trnClutterSetDrawFlag(bool draw);
void trnClutterFreeAttributeIfNecessary(size_t key);
