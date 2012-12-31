#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Can't use a pragma guard, because there are multiple syscfgs.

#pragma cpp_extensions on

#include "include/platforms.h"

#include <cmath>
#include <libkern/OSByteOrder.h>

#define platBuild TEUL_CLIENT

#define FRONT_END
#define FE

#define _UNICODE		// Because Ramp added dependencies on it

#ifndef RELEASE_CLIENT
#define RELEASE_CLIENT 1
#endif

#define OPENGL3D 1
#define USE_3DAPI_LIGHTING
#define DEF_USE_MAC_CURSOR 1
#define NDEBUG 1

#define MAXMIPLEVELS	12

#include "sysmac/sysMacCompat.h"

#define PREPATH getPrePath()

#define TERRAIN_PREPATH "Terrain\\"
#define GR2DATA_PREPATH "gr2Data\\"

#define USE_SPEED_TREE_RT 1
#define USE_PHUI
#define USE_VEHLOAD
#define dDOUBLE

#include "log/log.h"

#define USING_EXT_NAMESPACE using namespace __gnu_cxx

#if defined(__cplusplus) && !defined(GLOBALS)
# include <queue>
# include <deque>
# include "math/mathvect.h"
# include "math/mathnum.h"
# include "math/Matrixd.h"
# include "math/Matrixf.h"

# include "include/vars.h"

# include "wwii/wwiiOwnershipEnums.h"
# include "wwii/wwiiFullObjID.h"
# include "wwii/wwiiCovert.h"
# include "wwii/wwiiString.h"
# include "str/stringLib.h"
# include "weap/weapDefs.h"
# include "weap/weapSpec.h"

# include "phui/Javascript.h"

# include "pi/pi.h"

# include "fio/fio.h"
# include "fio/fiod.h"

# include "ml/mltypes.h"
# include "veh/vehtype.h"
# include "client/GameClient.h"

# include "trn/trntypes.h"
# include "strt/strtDTL.h"
# include "sdm/sdmHash.h"

# include "gl/glew.h"
# include <OpenGL/OpenGL.h>

# include "pers/pershud.h"
//# include "fp/fpFPS.h"
# include "trn/trnmain.h"
# include "ui/uiMain.h"

# include "inst/instDescriptor.h"
#endif

#include "cfg/cfgPrefs.h"
#include "ogl/ogl.h"


#endif //PN_SYSCFG_H

