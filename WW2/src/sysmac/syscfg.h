#ifndef PN_SYSCFG_H
#define PN_SYSCFG_H 1

#include <wchar.h>

#if !__cplusplus
#warning BUILDING AS PLAIN C!!!
#else
#include "include/platforms.h"

#include <cmath>
#include <libkern/OSByteOrder.h>

//inline float abs(float f) { return fabs(f); }

#define TEUL_CLIENT	0
#define TEUL_SERVER	1

#define platBuild TEUL_CLIENT

#define FRONT_END
#define FE

#define _UNICODE		// Because Ramp added dependencies on it

#ifndef RELEASE_CLIENT
#define RELEASE_CLIENT 1
#endif

#define OPENGL3D 1
#define INC_STRAT 1
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <wchar.h>

#if defined(__cplusplus)
# include <string>
# include <stdexcept>
# include <vector>
# include <map>
# include <set>
# include <queue>
# include <list>
# include <algorithm>
# include "dbug/dbug.h"
# include "mem/mem.h"
# include "math/mathvect.h"

# include "pi/pi.h"
# include "include/vars.h"

# include "wwii/wwiiOwnershipEnums.h"
# include "wwii/wwiiFullObjID.h"
# include "chat/chatHandle.h"
# include "wwii/wwiiCovert.h"
# include "wwii/wwiiString.h"
# include "str/stringLib.h"
# include "weap/weapDefs.h"
# include "weap/weapSpec.h"
# include "teul/teulKit.h"

# include "phui/Javascript.h"

# include "fio/fio.h"
# include "fio/fiod.h"

# include "ml/mltypes.h"
# include "veh/vehtype.h"
# include "client/GameClient.h"

# include "trn/trntypes.h"
# include "strt/strtDTL.h"
# include "sdm/sdmHash.h"

# include "gl/glew.h"
#include <OpenGL/OpenGL.h>

# include "pers/pershud.h"
//# include "fp/fpFPS.h"
# include "trn/trnmain.h"
# include "ui/uiMain.h"

#endif

#include "cfg/cfgPrefs.h"
#include "ogl/ogl.h"
#endif

/*
#ifdef __cplusplus

#include "math.h"
#include "math/mathvect.h"
#include "math/mathnum.h"

#include "wwii/wwiiOwnershipEnums.h"
#include "wwii/wwiiFullObjID.h"
#include "chat/chatHandle.h"
#include "wwii/wwiiCovert.h"
#include "wwii/wwiiString.h"
#include "str/stringLib.h"
#include "weap/weapDefs.h"
#include "weap/weapSpec.h"

#include "phui/Javascript.h"

#include "fio/fio.h"
#include "fio/fiod.h"

#include "ml/mltypes.h"
#include "veh/vehtype.h"
#include "client/GameClient.h"

#include "trn/trntypes.h"
#include "strt/strtDTL.h"
#include "sdm/sdmHash.h"

#include "gl/gl.h"
#include "gl/glew.h"

#include "pers/pershud.h"
#include "fp/fpFPS.h"
#include "trn/trnmain.h"

#include "weap/weapammo.h"
#include "ml/mlutility.h"

#include "fx/fxMgr.h"

#include "wwii/wwiiServerTrackedObject.h"
#include "wwii/wwiiSTOTypes.h"
#endif // __cplusplus
*/

#endif //PN_SYSCFG_H

