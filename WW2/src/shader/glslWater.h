#pragma once

#include "ob/obmain.h"

extern bool glslWaterInit();
extern bool glslWaterCanReflect();
extern bool glslWaterDraw();

extern void glslOceanSetup(OBJECT* o);
extern void glslCoastSetup(OBJECT* o);
extern void glslScenarioCoastSetup(OBJECT* o, const char* shader);
extern void glslScenarioRiverSetup(OBJECT* o, const char* shader);
extern void glslScenarioOceanSetup(OBJECT* o, const char* shader);
extern void glslRiverSetup(OBJECT* o);
extern void glslWaterUpdate();
extern void glslWaterDrawBin();
extern void glslWaterClearBin();
extern void glslWaterReset();
extern void glslInitWaterTexture();
extern void glslWaterDrawBinDeprecated(bool fbodraw = false);

extern float glslWaterGetClosestWaterHeight();
