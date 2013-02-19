CMAKE_MINIMUM_REQUIRED (VERSION 2.6.4)
PROJECT (GameHosts)

################################################################################
## Configuration.

IF ( UNIX AND NOT IS_DIRECTORY "/playnet/" )
	message(SEND_ERROR "No /playnet folder")
ENDIF ( )

ENABLE_TESTING()
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes")
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
SET(LIBRARIES_DIR "${PROJECT_DIR}/Libraries")

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

SET(AUTH_HOST_EXE "authHost")
SET(DBD_HOST_EXE "dbd")
SET(CHAT_HOST_EXE "chatd")
SET(STRAT_HOST_EXE "stratd")
SET(MAP_HOST_EXE "mmhostd")
SET(CELL_HOST_EXE "hostd")
SET(PING_HOST_EXE "pingHost")

SET(_LUA_HOST "stratd")

IF ( PN_BUILD_TYPE MATCHES "Auth" OR PN_CLUSTER MATCHES "Auth" )
	SET(PN_BUILD_TYPE "Auth")
	SET(PN_CLUSTER "Auth")
	SET(CMAKE_BUILD_TYPE "Debug")
	SET(PN_MEM_DEBUG ON)
	SET(PN_STACK_PROTECT ON)
	SET(PN_TRACER ON)
ENDIF ( )

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hostproj ${CMAKE_CURRENT_BINARY_DIR})

# Define and validate user-specifiable options.
include ( cmakeincludes/UserOptions.cmake )

# Process the PN_BUILD_TYPE setting.
include ( cmakeincludes/TargetEnv.cmake )

# Do Platform specific checks and build config.h
include ( cmakeincludes/PlatformChecks.cmake )

# Check various dependencies.
include ( cmakeincludes/Dependencies.cmake )

include ( cmakeincludes/PCHSupport.cmake )
SET(PCH_INCLUDE_HEADERS
	hostproj/syscfg.h
	include/platforms.h
	include/systypes.h
	include/defs.h
	dbug/dbug.h
	chat/chatHandle.h
	mem/mem.h
	hostproj/versionNo.h
	include/precompile_includes.h
	inst/instDescriptor.h
	wwii/wwiiPlayer.h
	server/serverIndexes.h
	wwii/wwiiIRC.h
	server/serverPeers.h
	wwii/wwiiServerTypeEnums.h
	../../Libraries/OpenThreads/Thread
	../../Libraries/OpenThreads/Mutex
	wwii/wwiiHash.h
	wwii/wwiiOwnershipEnums.h
	wwii/wwiiConfig.h
	server/serverServerInfo.h
	teul/teulTime.h
	teul/teulClosingEnums.h
	teul/teulConfigVar.h
	teul/teulPackers.h
	teul/teulPackEnums.h
	teul/teulKit.h
	teul/teulBasic.h
	dbug/dbugExtErrors.h
	wwii/wwiiPermissions.h
	inst/instTypes.h
	wwii/wwiiFullObjID.h
	teul/teulPacket.h
	weap/weapWeapon.h
	teul/teulEndpoint.h
)

######################################################################

message(STATUS "${PN_BUILD_TYPE}/${BUILDNAME} build. Opt:${OPTIMIZATION}, MEM:${PN_MEM_DEBUG}, STACK:${PN_STACK_PROTECT}")

######################################################################
#### Libraries.

add_library (ircclient
	libircclient/src/libircclient.cpp
)
set_property(TARGET ircclient PROPERTY COMPILE_DEFINITIONS IN_BUILDING_LIBIRC=1)
set_property(TARGET  ircclient PROPERTY FOLDER "Libs")
IF (WIN32 OR WIN64)
	SET_TARGET_PROPERTIES(ircclient PROPERTIES COMPILE_FLAGS "${CXX_COMPILE_FLAGS} /W1")
ENDIF ( )

IF ( WIN32 )
	add_library(OpenThreads
		${LIBRARIES_DIR}/OpenThreads/Atomic.cpp
		${LIBRARIES_DIR}/OpenThreads/Version.cpp
		${LIBRARIES_DIR}/OpenThreads/WIN32Condition.cpp
		${LIBRARIES_DIR}/OpenThreads/Win32Mutex.cpp
		${LIBRARIES_DIR}/OpenThreads/Win32Thread.cpp
		${LIBRARIES_DIR}/OpenThreads/Win32ThreadBarrier.cpp

		${LIBRARIES_DIR}/OpenThreads/HandleHolder.h
		${LIBRARIES_DIR}/OpenThreads/Win32BarrierPrivateData.h
		${LIBRARIES_DIR}/OpenThreads/Win32Condition.h
		${LIBRARIES_DIR}/OpenThreads/WIn32ConditionPrivateData.h
		${LIBRARIES_DIR}/OpenThreads/Win32MutexPrivateData.h
		${LIBRARIES_DIR}/OpenThreads/Win32ThreadPrivateData.h
	)
	set_property(TARGET OpenThreads PROPERTY COMPILE_DEFINITIONS OT_LIBRARY_STATIC)
	set_property(TARGET OpenThreads PROPERTY COMPILE_DEFINITIONS OpenThreads_EXPORTS)
	set_property(TARGET OpenThreads PROPERTY COMPILE_DEFINITIONS OPENTHREADS_EXPORTS)
	SET(_PCH "openthreads-precompile-pch.h")
	SET_TARGET_PROPERTIES(OpenThreads PROPERTIES COMPILE_FLAGS "/Yu\"${_PCH}\" /FI\"${_PCH}\" /Fp\"\$(IntDir)\$(TargetName).pch\"")
	SET_TARGET_PROPERTIES(OpenThreads PROPERTIES FOLDER "Libs")
	SET_SOURCE_FILES_PROPERTIES(${LIBRARIES_DIR}/OpenThreads/Atomic.cpp PROPERTIES COMPILE_FLAGS "/Yc\"${_PCH}\"")
ENDIF ( )

# TeulServer library - this is a baseline library that provides
# teulKit services and the cluster's "server" API.
add_library (teulserver
	auth/authDaemon.cpp
	list/listCL.cpp
	mem/memlinux.cpp
	server/server.cpp
	server/serverIfConf.cpp
	server/serverLuaInterface.cpp
	server/serverMessages.cpp
	server/serverPeers.cpp
	server/serverTransfer.cpp
	str/stringLib.cpp
	teul/teulCallback.cpp
	teul/teulDatagramPool.cpp
	teul/teulEndpoint.cpp
	teul/teulMicroWebServer.cpp
	teul/teulModCalls.cpp
	teul/teulModRecv.cpp
	teul/teulModSend.cpp
	teul/teulModule.cpp
	teul/teulNetTimeMS.cpp
	teul/teulPack.cpp
	teul/teulPacket.cpp
	teul/teulTransmission.cpp
	teul/teulTransport.cpp
	teul/teulUtils.cpp
	wwii/wwiiCallbacks.cpp
	auth/authPack.cpp
	auth/authGameMessages.cpp
	teul/teulAuth.cpp
	wwii/wwiiConfig.cpp

	${PCH_INCLUDE_HEADERS}
	dbug/dbugInternal.h
)
ADD_PCH(teulserver)
target_link_libraries(teulserver event)
SET_PROPERTY(TARGET teulserver PROPERTY FOLDER "Libs")

# Code for any host process that has to participate in awareness of instances
# (chat, map, cell do; strat doesn't)
add_library (instancing
	chat/chatLocalizedMessage.cpp
	inst/instCommon.cpp
	inst/instHost.cpp
	inst/instPackers.cpp
	wwii/wwiiPlayer.cpp
	chat/chatPackers.cpp

	${PCH_INCLUDE_HEADERS}
)
ADD_PCH(instancing)
SET_PROPERTY(TARGET instancing PROPERTY FOLDER "Libs")

# Common-to-all WWII library, provides all the basics like ChatHandle functions,
# weapon list, database operations, wwii_config, etc.
add_library (libwwii 
	chat/chatHandle.cpp
	chat/chatPackers.cpp
	chat/chatTimeInterval.cpp
	dbug/dbug.cpp
	fio/fiod.cpp
	inst/instCommon.cpp
	inst/instPackers.cpp
	parse/countryCodes.cpp
	parse/parse.cpp
	parse/parseIO.cpp
	weap/weapList.cpp
	weap/weapLoadIndexes.cpp
	weap/weapParser.cpp
	weap/weapSpec.cpp
	tinyxml/tinystr.cpp
	tinyxml/tinyxml.cpp
	tinyxml/tinyxmlerror.cpp
	tinyxml/tinyxmlparser.cpp
	tut/tutGroupProgress.cpp
	wwii/wwiiCommandLineArgs.cpp
	wwii/wwiiIRC.cpp
	wwii/wwiiRank.cpp
	wwii/wwiiString.cpp
	wwii/wwiiThreads.cpp
	wwii/wwiiThreadText.cpp
	wwii/wwiiUTF.cpp
	auth/authDBConnection.cpp
	score/scoreActor.cpp
	src/io_mapped_file.cpp
	src/mydbQuery.cpp
	src/myDbseWrapper.cpp
	wwii/wwiiDBConfig.cpp
	wwii/wwiiDBThreads.cpp
	wwii/wwiiSerialNumber.cpp
	wwii/wwiiSortieManager.cpp
	wwii/wwiiSyncVar.cpp
	sdm/sdmHash.cpp
	sdm/sdmParse.cpp
	src/wthrTime.cpp
	strt/strtSupercell.cpp
	wwii/wwiiCallbacks.cpp
	wwii/wwiiHCUnitsCommon.cpp
	dbse/dbseClient.cpp
	dbse/dbseClientMessages.cpp
	dbse/dbsePackers.cpp
	wwii/wwiiClusterConfig.cpp
	src/configVars.cpp

	sdm/sdmConfigVars.h
	server/serverConfigVars.h
	wwii/wwiidbConfigVars.h

	${PCH_INCLUDE_HEADERS}
	include/io_mapped_file.h
	str/strdupptr.h
	wwii/wwiiClusterConfig.h
	wwii/wwiiNetPorts.h
	wwii/wwiiCallbacks.h
	wwii/wwiiThreads.h
)
ADD_PCH(libwwii)
target_link_libraries(libwwii OpenThreads)
SET_PROPERTY(TARGET libwwii PROPERTY FOLDER "Libs")

######################################################################
#### Client/Utility command line applications.


IF ( NOT PN_CLUSTER STREQUAL "Auth")
	IF ( NOT WIN32 )
		# Command-line based access to the Chat Host (chatClient)
		add_executable(chatClient
			chat/chatLocFile.cpp
			chat/chatManagementClientMessages.cpp
			chat/chatManagementPack.cpp
			chat/chatRoster.cpp
			chat/chatRosterClient.cpp
		)
		ADD_PCH(chatClient)
		target_link_libraries(chatClient libwwii teulserver)
		SET_PROPERTY(TARGET chatClient PROPERTY FOLDER "Tools")

		# Provide access to the LuaVMs.
		add_executable(luaClient
			src/luaClient.cpp
		)
		target_link_libraries(luaClient libwwii teulserver readline ncurses)
		SET_PROPERTY(TARGET luaClient PROPERTY FOLDER "Tools")
	ENDIF ( )

	# Tool for validating strings as possible gamenames.
	# (China was using non-standard characters in their game names)
	add_executable(checkGameName
		src/checkGameName.cpp
	)
	target_link_libraries(checkGameName libwwii)
	SET_PROPERTY(TARGET checkGameName PROPERTY FOLDER "Tools")

	# In theory, SDM pre-parser. In practice? Deletes SDM files that don't
	# contain any strat data.
	# Not part of stratValidator because the original coders couldn't figure
	# out why it leaked so much memory.
	add_executable(sdmpp
		sdm/sdmpp.cpp

		${PCH_INCLUDE_HEADERS}
		sdm/sdmConfigVars.h
	)
	target_link_libraries(sdmpp libwwii teulserver)
	SET_PROPERTY(TARGET sdmpp PROPERTY FOLDER "Tools")

	# Analyze SDM files

	add_executable(sdmDump
		sdm/sdmDump.cpp
		${PCH_INCLUDE_HEADERS}
	)
	target_link_libraries(sdmDump libwwii teulserver)
	SET_PROPERTY(TARGET sdmDump PROPERTY FOLDER "Tools")

	# Reads SDM files and imports portions of them into the game database.
	# Which is a shame, because the hosts wind up reading the SDM files too
	# to get the missing data. *Sigh*
	add_executable(stratLoader
		src/loader.cpp
		sdm/sdmLoad.cpp
		${PCH_INCLUDE_HEADERS}
	)
	target_link_libraries(stratLoader libwwii teulserver ircclient ${MYSQL_LIBRARIES})
	SET_PROPERTY(TARGET stratLoader PROPERTY FOLDER "Tools")

	# Reads SDM and blitzWor data through the server's strat routines,
	# without accessing the database, to validate a given terrain for
	# its ability to run on the host.
	# This tool is used by whomever edits the terrain by logging into
	# the "fedex" box as "terrain" and running the "validate" script.
	add_executable(stratValidator
		parse/parseWor.cpp
		strt/strtCPStruct.cpp
		strt/strtData.cpp
		strt/strtFacilityStruct.cpp
		strt/strtObjectStruct.cpp
		strt/strtIO.cpp
		strt/strtValidator.cpp
		${PCH_INCLUDE_HEADERS}
	)
	ADD_PCH(stratValidator)
	set_property(TARGET stratValidator PROPERTY COMPILE_DEFINITIONS CHECK_SUPPLY=1)
	target_link_libraries(stratValidator libwwii teulserver event)
	SET_PROPERTY(TARGET stratValidator PROPERTY FOLDER "Tools")
ENDIF ( )

######################################################################
#### Server Processes

# Authentication Server (auth host)
add_executable(${AUTH_HOST_EXE}
	auth/authClientData.cpp
	auth/authDB.cpp
	auth/authHost.cpp
	auth/authHostMessages.cpp
	auth/authOTP.cpp
	auth/authQueue.cpp
	src/infoHostMessages.cpp
	src/infoPack.cpp
	auth/authPack.cpp
	dbse/dbseDatabaseSchema.cpp
	${PCH_INCLUDE_HEADERS}
)
ADD_PCH(${AUTH_HOST_EXE})
target_link_libraries(${AUTH_HOST_EXE} teulserver libwwii ${MYSQL_LIBRARIES} ircclient)
SET_PROPERTY(TARGET ${AUTH_HOST_EXE} PROPERTY FOLDER "Hosts")

IF ( NOT PN_CLUSTER STREQUAL "Auth" )
	# Primarily communications-oriented server, the chat host.
	# Sadly it does an awful lot of the "map/mission" server's job
	# because the original design disconnected players from
	# map/mission when they spawned.
	add_executable(${CHAT_HOST_EXE}
		chat/chatBanPlayer.cpp
		chat/chatChannelList.cpp
		chat/chatChannelNumber.cpp
		chat/chatDB.cpp
		chat/chatDotBetaCommands.cpp
		chat/chatDotBuzzardSpawn.cpp
		chat/chatDotChatLog.cpp
		chat/chatDotCommand.cpp
		chat/chatDotCommands.cpp
		chat/chatDotFriends.cpp
		chat/chatDotGMCommands.cpp
		chat/chatDotIgnore.cpp
		chat/chatDotProfRes.cpp
		chat/chatEditString.cpp
		chat/chatFriendList.cpp
		chat/chatFriendLogin.cpp
		chat/chatGenericString.cpp
		chat/chatGrid.cpp
		chat/chatGroupPermissions.cpp
		chat/chatHost.cpp
		chat/chatHostMessages.cpp
		chat/chatList.cpp
		chat/chatLocFile.cpp
		chat/chatMakeTempString.cpp
		chat/chatManagementHostMessages.cpp
		chat/chatManagementIDs.cpp
		chat/chatManagementPack.cpp
		chat/chatManagementPlayer.cpp
		chat/chatPlayer.cpp
		chat/chatServerTrackedObjects.cpp
		chat/chatSpamCan.cpp
		chat/chatStrings.cpp
		chat/chatThreads.cpp
		chat/chatTimeString.cpp
		chat/chatTrainers.cpp
		chat/chatUnitMotds.cpp
		chat/chatWaypoints.cpp
		chat/chatWho.cpp
		chat/chatWhos.cpp
		wwii/wwiiServerTrackedObject.cpp
		wwii/wwiiServerTrackedObjectPacker.cpp

		chat/chatBrigadeControl.cpp
		chat/chatBrigadeMove.cpp
		chat/chatdbClientMessages.cpp
		chat/chatDecisions.cpp
		chat/chatDotHCCommands.cpp
		chat/chatDotListCOs.cpp
		chat/chatDotListSquad.cpp
		chat/chatDotPassthrus.cpp
		chat/chatDotRenameSquad.cpp
		chat/chatDotSquadCommands.cpp
		chat/chatHostBrigadeControl.cpp
		chat/chatHostMotds.cpp
		chat/chatMissionInvite.cpp
		chat/chatMissionMember.cpp
		chat/chatMissions.cpp
		chat/chatMissionStatusPacker.cpp
		chat/chatMotds.cpp
		chat/chatMultiCrew.cpp
		chat/chatReports.cpp
		chat/chatSaveChannels.cpp
		chat/chatSay.cpp
		chat/chatSquadControl.cpp
		chat/chatSquadControls.cpp
		chat/chatSquadCreate.cpp
		chat/chatSquadList.cpp
		chat/chatSquadListPurge.cpp
		chat/chatSquadVote.cpp
		map/mapAWS.cpp
		map/mapHCUnitLocations.cpp
		strt/strtChatTypes.cpp
		wwii/wwiiHCUnits.cpp
		wwii/wwiiSquads.cpp

		${PCH_INCLUDE_HEADERS}
	)
	ADD_PCH(${CHAT_HOST_EXE})
	set_property(TARGET ${CHAT_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_CHAT_HOST)
	target_link_libraries(${CHAT_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient)
	SET_PROPERTY(TARGET ${CHAT_HOST_EXE} PROPERTY FOLDER "Hosts")

	######################################################################
	# Game World Server aka Cell Host.
	# Only server of which multiple instances are intended to operate
	# in a single cluster. The work is distributed based on load, not
	# on any clever spatial distribution.
	# Currently, when a player spawns, they are actually transferred
	# (teul redirected) from the map host to this host. When they
	# despawn, they are redirected back to the map host.
	# Unfortunately, redirection is crudely implemented, and the player
	# actually /leaves/ the map host preventing the map host in dealing
	# with map/mission work for any spawned players. *SIGH*
	add_executable(${CELL_HOST_EXE}
		chat/chatCellHostSupportMessages.cpp
		chat/chatSupportRPCMessages.cpp
		comm/commHostMessages.cpp
		comm/commPackers.cpp
		host/host.cpp
		host/hostFraming.cpp
		host/hostGrid.cpp
		host/hostScoringActor.cpp
		host/hostVehicle.cpp
		src/installCallbacks.cpp
		miss/missionPack.cpp
		mm/mmCellHostSupportMessages.cpp
		mm/mmPack.cpp
		updt/updtPackers.cpp
		wwii/wwiiHostMessages.cpp

		host/hostAutomation.cpp
		host/hostCellNet.cpp
		host/hostGridCell.cpp
		miss/missionPacket.cpp
		parse/parseWor.cpp
		strt/strtCPColliders.cpp
		strt/strtCPStruct.cpp
		strt/strtData.cpp
		strt/strtFacilityStruct.cpp
		strt/strtHack.cpp
		strt/strtIO.cpp
		strt/strtObjectStruct.cpp
		strt/strtPack.cpp
		strt/strtSyncData.cpp
		strt/strtSyncSupportMessages.cpp
		strt/strtUpdater.cpp
		strt/strtUpdaterHostMessages.cpp
		strt/strtUpdaterPack.cpp
		strt/strtUpdaterSupportMessages.cpp

		${PCH_INCLUDE_HEADERS}
	)
	ADD_PCH(${CELL_HOST_EXE})
	set_property(TARGET ${CELL_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_CELL_HOST)
	target_link_libraries(${CELL_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient)
	SET_PROPERTY(TARGET ${CELL_HOST_EXE} PROPERTY FOLDER "Hosts")

	######################################################################
	# Map/Mission Host, probably better described as the "Game Login Server".
	# This is the host to which players are forwarded from auth/playgate.
	# See the description of the cell host above for details on host spawning
	# works and why the chat host has wound up doing so much of map host's job.
	add_executable(${MAP_HOST_EXE}
		chat/chatGroupPermissions.cpp
		chat/chatSupportMessages.cpp
		chat/chatSupportRPCMessages.cpp
		inst/instHostMessages.cpp
		map/mapArenaDescription.cpp
		map/mapChecksums.cpp
		map/mapDB.cpp
		map/mapHost.cpp
		map/mapHostMessages.cpp
		map/mapMinuteBalance.cpp
		map/mapPack.cpp
		map/mapPlayer.cpp
		map/mapThreads.cpp
		miss/missActiveMission.cpp
		miss/mission.cpp
		miss/missionDetail.cpp
		miss/missionHostMessages.cpp
		miss/missionPack.cpp
		miss/missMobileSpawn.cpp
		mm/mmAntiAddiction.cpp
		mm/mmHostInstanceImpl.cpp
		mm/mmHostMessages.cpp
		mm/mmMain.cpp
		mm/mmPack.cpp
		mm/mmPolls.cpp
		mm/mmSpawn.cpp
		mm/mmSpawnGrantResponse.cpp
		score/scoreCallbacks.cpp
		score/scoreDeath.cpp
		score/scoreSortie.cpp
		wwii/wwiiHostMessages.cpp
		wwii/wwiiOpinionPoll.cpp

		map/mapBrigades.cpp
		map/mapData.cpp
		map/mapHCUnitLocations.cpp
		map/mapHostSupercellData.cpp
		miss/missionPacket.cpp
		miss/missionScoring.cpp
		miss/missionUtils.cpp
		mm/mmdbClientMessages.cpp
		mm/mmdbPack.cpp
		mm/mmdbPersonaSelections.cpp
		parse/parseWor.cpp
		strt/strtCPStruct.cpp
		strt/strtData.cpp
		strt/strtDB.cpp
		strt/strtEvents.cpp
		strt/strtFacilityStruct.cpp
		strt/strtIO.cpp
		strt/strtMapMessages.cpp
		strt/strtObjectStruct.cpp
		strt/strtPack.cpp
		strt/strtSyncData.cpp
		strt/strtSyncSupportMessages.cpp
		wwii/wwiiHCUnits.cpp

		${PCH_INCLUDE_HEADERS}
	)
	ADD_PCH(${MAP_HOST_EXE})
	set_property(TARGET ${MAP_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_MAP_HOST)
	target_link_libraries(${MAP_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient)
	SET_PROPERTY(TARGET ${MAP_HOST_EXE} PROPERTY FOLDER "Hosts")

	######################################################################
	# Playnet's solution to an Autonomous System Number: Have multiple
	# network routes and make the client figure out which one to use.
	# Pinghostd listens on all available interfaces and clients connect
	# to it during startup and see which interface they liked the best.
	# Selection is based on speed, which means that in many cases they
	# pick the one with the highest speed and most variability/least
	# reliability :(
	add_executable(${PING_HOST_EXE} src/pinghost.cpp)
	set_property(TARGET ${PING_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_PING_HOST)
	target_link_libraries(${PING_HOST_EXE} libwwii teulserver)
	SET_PROPERTY(TARGET ${PING_HOST_EXE} PROPERTY FOLDER "Hosts")

	######################################################################
	# Database Proxy Daemon [deprecated].
	# Because the original host implementation was
	# entirely single-threaded, this host process was created to offload
	# expensive synchronous database queries.
	# You should really use DB::Threads instead.
	add_executable(${DBD_HOST_EXE}
		chat/chatdbHostMessages.cpp
		chat/chatdbParser.cpp
		dbse/dbseHost.cpp
		dbse/dbseDaemon.cpp
		dbse/dbseHostMessages.cpp
		miss/missionPacket.cpp
		mm/mmdbHostMessages.cpp
		mm/mmdbPack.cpp
		mm/mmdbParser.cpp
		wwii/wwiiSquads.cpp

		${PCH_INCLUDE_HEADERS}
	)
	ADD_PCH(${DBD_HOST_EXE})
	set_property(TARGET ${DBD_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_DBD_HOST)
	target_link_libraries(${DBD_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient)
	SET_PROPERTY(TARGET ${DBD_HOST_EXE} PROPERTY FOLDER "Hosts")

	######################################################################
	# Strat host: Most of the other hosts maintain copies of the strat
	# data, but this is the center of authority.
	# Because this host is generally doing very little, it typically sits
	# on the same machine as the MySQL database, and it often gets
	# offloaded with more convoluted gameplay management stuff, which
	# turns out to be a real pain in the ass because you wind up writing
	# a metric ton of network routines for communicating with it...
	# It also hosts a Lua scripting environment.
	add_executable(${STRAT_HOST_EXE}
		chat/chatStratSupportMessages.cpp
		chat/chatSupportRPCMessages.cpp
		cnvy/cnvyConvoyList.cpp
		cnvy/cnvyMain.cpp
		dbse/dbseDatabaseSchema.cpp
		src/luaGlue.cpp
		miss/missionPack.cpp
		mm/mmPack.cpp
		parse/parseWor.cpp
		strt/strtChatTypes.cpp
		strt/strtCPClass.cpp
		strt/strtCP.cpp
		strt/strtCPStruct.cpp
		strt/strtData.cpp
		strt/strtDB.cpp
		strt/strtdbSaveWorld.cpp
		strt/strtCaptureAction.cpp
		strt/strtCaptureContribution.cpp
		strt/strtDotCommands.cpp
		strt/strtEnemyReport.cpp
		strt/strtFacilityStruct.cpp
		strt/strtFB.cpp
		strt/strtHack.cpp
		strt/strtHCUnits.cpp
		strt/strtHost.cpp
		strt/strtHostDB.cpp
		strt/strtHostMessages.cpp
		strt/strtIO.cpp
		strt/strtJoinRequestList.cpp
		strt/strtHostLua.cpp
		strt/strtMain.cpp
		strt/strtMission.cpp
		strt/strtObjectStruct.cpp
		strt/strtPack.cpp
		strt/strtPostedWeaponList.cpp
		strt/strtSpawn.cpp
		strt/strtSpawnWeaponList.cpp
		strt/strtSupply.cpp
		strt/strtSync.cpp
		strt/strtSyncData.cpp
		strt/strtTOE.cpp
		strt/strtUpdateList.cpp
		strt/strtWeaponEntry.cpp
		weap/weapSpawnXML.cpp
		wwii/wwiiHCUnits.cpp

		${_LUA_HOST}Lua.cc

		${PCH_INCLUDE_HEADERS}
	)
	ADD_PCH(${STRAT_HOST_EXE})
	set_property(TARGET ${STRAT_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_STRAT_HOST LUA_MODULE_OPEN=tolua_stratd_open CHECK_SUPPLY=1)
	set_property(TARGET ${_LUA_HOST} PROPERTY DEPENDS ${_LUA_HOST}Lua.cc)
	IF ( UNIX )
		set_property(SOURCE ${_LUA_HOST}Lua.cc PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS} -Wno-unused -Wno-unused-function")
	ELSE ( )
		set_property(SOURCE ${_LUA_HOST}Lua.cc PROPERTY COMPILE_FLAGS "/W0")
		set_property(SOURCE ${_LUA_HOST}Lua.cc PROPERTY COMPILE_FLAGS "/wd4480;4800;4305;4505")
	ENDIF ( )
	set_property(SOURCE ${_LUA_HOST}Lua.cc PROPERTY GENERATED true)
	target_link_libraries(${STRAT_HOST_EXE} libwwii ${MYSQL_LIBRARIES} teulserver ircclient ${LUA_LIBRARIES} ${TOLUAPP_LIBRARIES} )
	SET_PROPERTY(TARGET ${STRAT_HOST_EXE} PROPERTY FOLDER "Hosts")
ENDIF()

######################################################################
#### Test Module(s)

add_executable(testAuthConnect
	tests/testAuthConnect.cpp

	${PCH_INCLUDE_HEADERS}
)
target_link_libraries(testAuthConnect libwwii teulserver)
SET_PROPERTY(TARGET testAuthConnect PROPERTY FOLDER "Test")

IF ( NOT PN_CLUSTER STREQUAL "Auth")
	# Builds a single executable that runs all of the unit tests;
	# The install process will not execute unless all tests complete.
	add_executable(testUnits
		tests/testUnits.cpp
		tests/testAuthQueue.cpp auth/authClientData.cpp
		tests/testChatHandle.cpp
		tests/testFullObjID.cpp
		tests/testCommandWithArgs.cpp
		tests/testCovert.cpp
		tests/testLocalizedMessage.cpp
		tests/testInplaceVector.cpp
		tests/testPackers.cpp	comm/commPackers.cpp
		tests/testChatMissionStatus.cpp	chat/chatMissionStatusPacker.cpp wwii/wwiiServerTrackedObjectPacker.cpp
		tests/testHostGridCell.cpp host/hostGridCell.cpp
		tests/testSupercellVislevelXfer.cpp
		tests/testPersonaXfer.cpp
		tests/testCryptographer.cpp
		tests/testXYZTuple.cpp
		tests/testTeulPacket.cpp

		${PCH_INCLUDE_HEADERS}
		tests/testSuite.h
		tests/testUnits.h
	)
	ADD_PCH(testUnits)
	set_property(TARGET testUnits PROPERTY COMPILE_DEFINITIONS ENABLE_TESTS)
	target_link_libraries(testUnits libwwii teulserver)
	SET_PROPERTY(TARGET testUnits PROPERTY FOLDER "Test")

	add_test(testUnits ${EXECUTABLE_OUTPUT_PATH}/testUnits)
ENDIF()

######################################################################
#### Module for testing teulKit functionality

IF ( NOT PN_CLUSTER STREQUAL "Auth")
	add_executable(testTeul
		tests/testTeul.cpp
		
		${PCH_INCLUDE_HEADERS}
		teul/teulExceptions.h
		teul/teulKit.h
		teul/teulEndpoint.h
		teul/teulEndpointImpl.h
		teul/teulPacket.h
		teul/teulPacketHeader.h
		wwii/wwiiCallbacks.h
		tests/testTeulTest.h
		tests/testTeulClient.h
		tests/testTeulServer.h
		teul/teulVersion.h
		teul/teulConnectionInstruction.h
	)
	target_link_libraries(testTeul teulserver libwwii)
	SET_PROPERTY(TARGET testTeul PROPERTY FOLDER "Test")
ENDIF ( )

######################################################################
#### Install rule.

###TODO: Should really be dependent on testUnits as described above. Duh.
IF ( NOT PN_CLUSTER STREQUAL "Auth")
	IF ( NOT WIN32 )
		INSTALL(TARGETS	chatClient luaClient
			ARCHIVE DESTINATION lib
			LIBRARY DESTINATION lib
			RUNTIME DESTINATION bin
			COMPONENT cmdclients
		)
	ENDIF ( )
	INSTALL(TARGETS checkGameName stratLoader sdmpp stratValidator
			ARCHIVE DESTINATION lib
			LIBRARY DESTINATION lib
			RUNTIME DESTINATION bin
			COMPONENT tools
	)
	INSTALL(TARGETS ${GOVERNOR_HOST_EXE} ${STRAT_HOST_EXE} ${DBD_HOST_EXE} ${PING_HOST_EXE}
					${CHAT_HOST_EXE} ${MAP_HOST_EXE} ${CELL_HOST_EXE}
			ARCHIVE DESTINATION lib
			LIBRARY DESTINATION lib
			RUNTIME DESTINATION bin
			COMPONENT game
	)
ENDIF ( )
INSTALL(TARGETS
			${AUTH_HOST_EXE}
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
		COMPONENT auth
)
IF ( NOT PN_CLUSTER STREQUAL "Auth")
	INSTALL(DIRECTORY hostproj/lua/ DESTINATION lua COMPONENT lua PATTERN ".svn" EXCLUDE PATTERN ".lua")
	INSTALL(DIRECTORY hostproj/web/ DESTINATION web COMPONENT web PATTERN ".svn" EXCLUDE)
ENDIF ( )

######################################################################
#### Package building.

SET(CPACK_COMPONENTS_ALL game lua web auth)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_INSTALL_PREFIX "/playnet/wwiiol")
SET(CPACK_SET_DESTDIR ON)
SET(CPACK_PACKAGING_INSTALL_PREFIX "/playnet/wwiiol")
SET(OptLevel "NoOptimization")
IF ( PN_ENABLE_OPTS AND OPTIMIZATION GREATER 0 )
 SET(OptLevel "OptLevel${OPTIMIZATION}")
ENDIF ( )
SET(CPACK_PACKAGE_VERSION "${REPO_WC_REVISION}.${CMAKE_BUILD_TYPE}.${BUILDNAME}.${OptLevel}")
SET(CPACK_OUTPUT_FILE_PREFIX "/playnet/packages")
SET(CPACK_PACKAGE_VENDOR "Cornered Rat Software")
SET(CPACK_PACKAGE_NAME "wwiiol")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WWII Online game servers")
SET(CPACK_PACKAGE_DESCRIPTION "'BattleGround Europe: WWII Online' game servers, auth host and tools.")
IF ( PN_CLUSTER STREQUAL "Auth" )
 SET(CPACK_PACKAGE_NAME "wwiiol-auth")
 SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WWIIOL Auth Host")
 SET(CPACK_PACKAGE_DESCRIPTION "'BattleGround Europe: WWII Online' stand-alone auth host.")
ENDIF ( )
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Oliver Smith")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libopenthreads-dev (>= 2.8), lua5.1 (>= 5.1.4-5), libmysqlclient16 (>= 5.1.49), bash (>= 4.1), gdb (>= 7.1), nfs-common, mysql-server (>= 5.1)")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "WWIIOL Game Servers")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Network")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "Important")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "vim, mysql-client (>= 5.1), python (>= 2.6.5), bzip2")
SET(CPACK_DEBIAN_PACKAGE_SUGGESTS "perl (>= 5.1), etckeeper, byobu")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes/preinst;${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes/postinst;")

INCLUDE (CPack)		# For making installers

SET(_PREV_BUILD_TYPE "${PN_BUILD_TYPE}" CACHE INTERNAL "Value of PN_BUILD_TYPE last time CMake Ran" FORCE)
