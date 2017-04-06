#pragma once

#include "teul/teulConfigVar.h"

extern const ConfigPath    cv_pnPath;
extern ConfigPath	cv_dataDir;
extern ConfigPath	cv_importFile;
extern ConfigPath	cv_terrainSubdir;
extern ConfigPath	cv_originFile;
extern ConfigStr	cv_stratLanguage;
extern ConfigPath   cv_thblitzPath;
extern ConfigBool   cv_stratIgnoreErrors;
extern ConfigPath   cv_blitzworPath;
extern ConfigPath   cv_luaPath;
extern ConfigPath   cv_defaultLogPath;
extern ConfigPath   cv_luaRCFile;
extern ConfigPath   cv_luaStratlibFile;

extern ConfigAddr	cv_wwiiDBAddress;
extern ConfigStr	cv_wwiiDBDefaultSchema;
extern ConfigStr    cv_wwiiDBPassword;
extern ConfigStr    cv_wwiiDBUser;
extern ConfigStr    cv_wwiiDBPort;
extern ConfigAddr   cv_dbsePrimaryAddress;
extern ConfigUint   cv_dbseConnectionTimeout_ms;
extern ConfigBool   cv_mysqlCompression;

extern ConfigStr	cv_clusterName;
extern ConfigUint   cv_arenaID;
extern ConfigUint   cv_appID;
extern ConfigAddr	cv_ircServer;
extern ConfigStr	cv_ircPassword;
extern ConfigUint	cv_ircPort;
extern ConfigStr	cv_authErrorUrl;
extern ConfigAddr	cv_baseAddress;
extern ConfigAddr   cv_publicIP;
extern ConfigBool   cv_oneProcessMode;
extern ConfigPath	cv_teulWebPath;

extern ConfigAddr   cv_authHost;
extern ConfigAddr   cv_authdbAddress;
extern ConfigStr    cv_authdbPort;
extern ConfigStr    cv_authdbPassword;
extern ConfigStr    cv_authdbSchema;
extern ConfigStr    cv_authdbUser;

extern ConfigUint   cv_maxClusterUsers;
extern ConfigUint   cv_maxCellPlayers;
extern ConfigAddr   cv_baseUpdtAddress;
extern ConfigUint   cv_peerConnectionTimeout_ms;
extern ConfigStr    cv_defaultInterface;

extern ConfigStr    cv_manageProcesses;
