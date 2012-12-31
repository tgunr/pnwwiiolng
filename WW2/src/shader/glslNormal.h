#pragma once

#include "ob/obmain.h"

extern bool glslNormalInit();
extern void glslNormalSetup(struct OBJECT* o);
extern void glslNormalFpsSetup(struct gr2MeshData* data);
extern void glslNormalDrawBin();
extern void glslNormalClearBin();
extern void glslNormalDrawBinDeprecated();
