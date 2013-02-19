#include <syscfg.h> // Path is project specific.
// sdm data related configuration variables.
#include "teul/teulConfigVar.h"

#if platOS != OS_WIN32
CVAR_STRING(dataDir, "/playnet/data", "DATA_DIR");
#else
CVAR_STRING(dataDir, "c:\\playnet\\data", "DATA_DIR");
#endif
CVAR_STRING(importFile, "importlist.txt", "IMPORT_FILE");
CVAR_STRING(terrainSubdir, "terrain", "TERRAIN_SUBDIR");

