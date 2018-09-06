#ifndef PN_SYSCFG_H
#define PN_SYSCFG_H 1

#include <wchar.h>

#ifdef __cplusplus

#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYSLOG_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_SIGACTION 1
#include "include/platforms.h"
#include <libkern/OSByteOrder.h>

#define _UNICODE

#ifndef RELEASE_CLIENT
#define RELEASE_CLIENT 1
#endif

#define OPENGL3D 1
#define USE_3DAPI_LIGHTING

#define MAXMIPLEVELS	12

#include "sysmac/sysMacCompat.h"

#define PREPATH getPrePath()

#define SYS_TITLE        "WWIIOL"
#define SYS_COMPANY_NAME "Playnet"

#define TERRAIN_PREPATH "Terrain\\"
#define GR2DATA_PREPATH "gr2Data\\"

#define USE_SPEED_TREE_RT 1
#define USE_PHUI
#define USE_VEHLOAD
#define dDOUBLE

#include "log/log.h"

# include "mem/mem.h"
# include "math/mathvect.h"

# include "pi/pi.h"
# include "include/vars.h"
# include "ddmac/ddp.h"

# include "wwii/wwiiCovert.h"
# include "wwii/wwiiString.h"
# include "weap/weapDefs.h"
# include "weap/weapSpec.h"

# include "phui/Javascript.h"

# include "fio/fio.h"
# include "fio/fiod.h"

# include "ml/mltypes.h"
# include "veh/vehtype.h"
# include "client/GameClient.h"

# include "trn/trntypes.h"
# include "strt/strtDTL.h"
# include "sdm/sdmHash.h"

# include "GL/glew.h"
#include <OpenGL/OpenGL.h>

# include "pers/pershud.h"
# include "trn/trnmain.h"
# include "ui/uiMain.h"

#include "cfg/cfgPrefs.h"
#include "ogl/ogl.h"

#else
#warning BUILDING AS PLAIN C!!!
#endif

#endif //PN_SYSCFG_H

