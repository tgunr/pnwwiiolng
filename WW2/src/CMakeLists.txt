####################################
## Declare CMake version requirement
CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)

####################################
## Initial CMake config
ENABLE_TESTING()
SET( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes;${CMAKE_MODULE_PATH}" )
IF ( NOT UNIX AND NOT APPLE AND NOT WIN32 )
    MESSAGE( FATAL_ERROR "Unknown build environment, not continuing" )
ENDIF()

IF ( NOT CMAKE_BUILD_TYPE )
	MESSAGE( WARNING "No CMAKE_BUILD_TYPE specified. Assuming Debug." )
	SET( CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE )
ENDIF()

####################################
## Declare the project (don't specify version here)
PROJECT( WW2GameHosts LANGUAGES C CXX )

####################################
## Require C++14
SET( CMAKE_CXX_STANDARD_REQUIRED ON )
SET( CMAKE_CXX_STANDARD 14 )

####################################
## Determine bitness
MARK_AS_ADVANCED( CMAKE_BITNESS )

IF ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	SET( CMAKE_BITNESS 32 )
ELSEIF ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	SET( CMAKE_BITNESS 64 )
ELSE ()
	MESSAGE( FATAL_ERROR "Unknown CMAKE_SIZEOF_VOID_P value ${CMAKE_SIZEOF_VOID_P}" )
ENDIF ()

MESSAGE( STATUS "Build is ${CMAKE_BITNESS}-bit" )

####################################
## Executable names
SET( AUTH_HOST_EXE "authHost" )
SET( CONTROLLER_HOST_EXE "controller" )
SET( DBD_HOST_EXE "dbd" )
SET( CHAT_HOST_EXE "chatd" )
SET( CELL_HOST_EXE "hostd" )
SET( MAP_HOST_EXE "mmhostd" )
SET( STRAT_HOST_EXE "stratd" )

####################################
## General search paths

# /usr/local is explicitly added because many systems don't search it by
# default despite it being an obvious place to look.
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../../Libraries
	${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/hostproj
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/mem
	${CMAKE_CURRENT_SOURCE_DIR}/sys
    ${CMAKE_CURRENT_BINARY_DIR}
)

####################################
## Embedded library search path
SET( LIBRARIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Libraries" )

####################################
## Determine installation directory
SET( CMAKE_INSTALL_PREFIX "/playnet" CACHE PATH "Install path prefix" )
SET( PN_PLAYNET_PATH "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Default top-level runtime path." )
FILE( TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" _PLAYNET )

####################################
## Define and validate user-specifiable options.

# Enable per-host ports for running clusters on less than 4 hosts
OPTION( PN_DIFFERENT_PORTS	"Use different public-facing ports for game servers." ON )

# Default to 64-bit on Windows for now
IF ( !WIN32 )
	OPTION( PN_BUILD_32BIT		"Enable 32-bit build" ON )
ELSE()
	OPTION( PN_BUILD_32BIT		"Enable 32-bit build" OFF )
	MARK_AS_ADVANCED( PN_BUILD_32BIT )
ENDIF()

####################################
## Provide platform and server defines
IF(WIN32)
    ADD_DEFINITIONS( "-DplatOS=OS_WIN32" )
ELSEIF(APPLE)
    ADD_DEFINITIONS( "-DplatOS=OS_MACOS" )
ELSE()
    ADD_DEFINITIONS( "-DplatOS=OS_LINUX" )
ENDIF()

ADD_DEFINITIONS ( "-DAS_SERVER=1" )

####################################
## Status output
MESSAGE( STATUS "${CMAKE_BUILD_TYPE} build, ${CMAKE_BITNESS}-bit." )

####################################
## Generate config.h
INCLUDE( cmakeincludes/PlatformChecks.cmake )

####################################
## Check various dependencies.
INCLUDE( cmakeincludes/Dependencies.cmake )

####################################
## Set up the various compiler definitions for things
INCLUDE( cmakeincludes/HostConfiguration.cmake )

####################################
## Set up the OMG parser commands
INCLUDE( cmakeincludes/BuildOMGgen.cmake )

# And the actual OMG files we generate
ADD_OMG_TARGET( omg/omgMissionSortieData )
ADD_OMG_TARGET( omg/omgChat )

####################################
## Run revno.py to get build_info.cpp
# Use BYPRODUCTS for the real result and a symbolic (fake) OUTPUT so the script
# runs always; easiest way to avoid the run-when-needed default behaviors.
ADD_CUSTOM_COMMAND(
    OUTPUT build_info.stamp
    BYPRODUCTS build_info.cpp
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cpp.in
    COMMAND
        ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Tools/revno.py
        host
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating build information"
    VERBATIM
)
SET_SOURCE_FILES_PROPERTIES( build_info.stamp PROPERTIES SYMBOLIC TRUE )

####################################
# teulserver library, provides teul and the "server" API, for what they're worth
ADD_LIBRARY(teulserver STATIC
    server/server.cpp server/server.h
    server/serverReleaseTypeEnum.h
    server/serverLuaInterface.cpp
    server/serverMessages.cpp server/serverMessages.h
    server/serverPeers.cpp server/serverPeers.h
    server/serverTransfer.cpp server/serverTransfer.h
    str/stringLib.cpp str/stringLib.h
    teul/teulCallback.cpp teul/teulCallback.h
    teul/teulEndpoint.cpp teul/teulEndpoint.h teul/teulEndpointImpl.h
    teul/teulMicroWebServer.cpp teul/teulMicroWebServer.h include/pnThread.h
    teul/teulModCalls.cpp teul/teulModCalls.h
    teul/teulModRecv.cpp teul/teulModSend.cpp teul/teulModule.cpp teul/teulModule.h
    teul/teulNetTimeMS.cpp teul/teulNetTimeMS.h teul/teulNetTimeMSPacker.h teul/teulNetTimeMS.h
    teul/teulBits.h teul/teulPack.cpp teul/teulPacket.cpp teul/teulPacket.h
    teul/teulTransmission.cpp teul/teulTransmission.h
    teul/teulTransport.cpp
    teul/teulUtils.cpp teul/teulUtils.h
    wwii/wwiiCallbacks.cpp wwii/wwiiCallbacks.h teul/teulCallback.h teul/teulCallbackInit.h
    auth/authPack.cpp auth/authPack.h
    auth/authGameMessages.cpp auth/authGameMessages.h auth/authTypes.h auth/authServerTypes.h
    teul/teulAuth.cpp teul/teulAuth.h updt/loginTypes.h
    wwii/wwiiConfig.cpp wwii/wwiiConfig.h wwii/wwiiConfigMap.h
    dbug/dbugInternal.h mem/memPool.h
)
TARGET_INCLUDE_DIRECTORIES( teulserver PRIVATE ${LIBEVENT_INCLUDE_DIRS} )
TARGET_LINK_LIBRARIES( teulserver ${LIBEVENT_LIBRARIES} )
SET_PROPERTY( TARGET teulserver PROPERTY FOLDER "Libs" )

####################################
## instancing library for hosts that deal with instances (all but strat)
ADD_LIBRARY( instancing STATIC
    chat/chatLocalizedMessage.cpp chat/chatLocalizedMessage.h chat/chatDefines.h
    inst/instHost.cpp inst/instHost.h inst/instDescriptor.h
    wwii/wwiiPlayer.cpp wwii/wwiiPlayer.h
    mm/mmEnums.h auth/authDispatch.h
    miss/missFlags.h wwii/wwiiBranchEnums.h
)
SET_PROPERTY( TARGET instancing PROPERTY FOLDER "Libs" )

####################################
## wwii library common to everything, when it isn't broken by target definitions
ADD_LIBRARY( wwiilib STATIC
    build_info.cpp build_info.stamp
    chat/chatHandle.cpp chat/chatHandle.h chat/chatName.h
    chat/chatPackers.cpp chat/chatTimeInterval.cpp chat/chatTimeInterval.h
    dbug/dbug.cpp dbug/dbug.h
    fio/fiod.cpp fio/fiod.h
    inst/instCommon.cpp inst/instPackers.cpp
    parse/countryCodes.cpp parse/countryCodes.h parse/parse.cpp parse/parse.h
    parse/parseIO.cpp parse/parseIO.h parse/parse.h parse/countryCodes.h
    weap/weapList.cpp weap/weapList.h weap/weapSerial.h
    weap/weapLoadIndexes.cpp weap/weapLoadIndexes.h weap/weapParser.cpp weap/weapParser.h
    weap/weapSpec.cpp weap/weapSpec.h
    tinyxml/tinystr.cpp tinyxml/tinystr.h tinyxml/tinyxml.cpp tinyxml/tinyxml.h
    tinyxml/tinyxmlerror.cpp tinyxml/tinyxmlparser.cpp
    tut/tutGroupProgress.cpp tut/tutGroupProgress.h
    wwii/wwiiCommandLineArgs.cpp wwii/wwiiCommandLineArgs.h
    wwii/wwiiIRC.cpp wwii/wwiiIRC.h host/hostCommand.h
    wwii/wwiiRank.cpp wwii/wwiiRank.h
    wwii/wwiiString.cpp wwii/wwiiString.h
    wwii/wwiiThreads.cpp wwii/wwiiThreads.h wwii/wwiiThreadTypes.h
    wwii/wwiiThreadText.cpp wwii/wwiiThreadText.h wwii/wwiiThreadTextQuery.h
    wwii/wwiiUTF.cpp wwii/wwiiUTF.h
    score/scoreActor.cpp score/scoreActor.h
    src/io_mapped_file.cpp include/io_mapped_file.h
    src/mydbQuery.cpp include/mydbQuery.h src/myDbseWrapper.cpp
    wwii/wwiiDBConfig.cpp wwii/wwiiDBConfig.h wwii/wwiiDBThreads.cpp wwii/wwiiDBThreads.h
    wwii/wwiiSerialNumber.cpp wwii/wwiiSerialNumber.h
    wwii/wwiiSortieManager.cpp wwii/wwiiSortieManager.h
    wwii/wwiiSyncVar.cpp wwii/wwiiSyncVar.h
    sdm/sdmHash.cpp sdm/sdmHash.h sdm/sdmParse.cpp sdm/sdmParse.h strt/strtDTL.h wwii/wwiiTypes.h
    src/wthrTime.cpp include/wthrTime.h
    strt/strtSupercell.cpp strt/strtSupercell.h strt/strtSupercells.h
    wwii/wwiiCallbacks.cpp wwii/wwiiCallbacks.h teul/teulCallback.h teul/teulCallbackInit.h
    wwii/wwiiHCUnitsCommon.cpp
    src/configVars.cpp sdm/sdmConfigVars.h server/serverConfigVars.h wwii/wwiidbConfigVars.h
    str/strdupptr.h
    wwii/wwiiNetPorts.h wwii/wwiiCallbacks.h
    # These weren't previously in wwiilib but seem to be required in various places
    auth/authDB.cpp auth/authDB.h src/infoHostMessages.cpp include/infoHostMessages.h
    src/infoPack.cpp include/infoPack.h dbse/dbseDatabaseSchema.cpp dbse/dbseDatabaseSchema.h
)
SET_PROPERTY( TARGET wwiilib PROPERTY FOLDER "Libs" )

####################################
## Various commandline utility executables
IF ( NOT WIN32 )

    # Chat management client
    ADD_EXECUTABLE( chatClient
        chat/chatLocFile.cpp chat/chatLocFile.h
        chat/chatManagementClientMessages.cpp chat/chatManagementClientMessages.h
        chat/chatManagementPack.cpp chat/chatRoster.cpp chat/chatRosterClient.cpp
    )
    TARGET_LINK_LIBRARIES( chatClient wwiilib teulserver ${MYSQL_LIBRARIES} )
    SET_PROPERTY( TARGET chatClient PROPERTY FOLDER "Tools" )
    
    ADD_EXECUTABLE( luaClient src/luaClient.cpp )
    TARGET_INCLUDE_DIRECTORIES( luaClient
        PRIVATE ${READLINE_INCLUDE_DIRS}
        PRIVATE ${NCURSES_INCLUDE_DIRS}
    )
    TARGET_LINK_LIBRARIES( luaClient
        wwiilib teulserver ${READLINE_LIBRARIES} ${NCURSES_LIBRARIES} ${MYSQL_LIBRARIES}
    )
    SET_PROPERTY( TARGET luaClient PROPERTY FOLDER "Tools" )

    INSTALL(
        TARGETS chatClient luaClient
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT cmdclients
    )

    # Not currently generated:
    #  checkGameName: Check callsign validity, not all that useful
    #  sdmpp: Prunes terrain files lacking strat data, pretty unhelpful currently
    #  sdmDump: Analyzes terrain files, not helpful without terrain work happening
    #  stratLoader: Partially loads terrain into the database, which hosts do anyway
    #  stratValidator: Uses server routines to validate SDM/WOR data, terrain terrain terrain, sigh
    # See git revision history for build commands.

ENDIF()

####################################
## Macro for building hosts
MACRO( ADD_HOST_TARGET Name )
    SET( HOST_TARGET_Usage "ADD_HOST_TARGET( <Name> [INSTANCING] [LUA] SOURCES ...)" )
    
    CMAKE_PARSE_ARGUMENTS( HOST_TARGET_ARG "INSTANCING;LUA" "" "SOURCES;EXTRA_DEFINITIONS" ${ARGN} )
    IF ( NOT "${HOST_TARGET_ARG_UNPARSED_ARGUMENTS}" STREQUAL "" )
        MESSAGE( FATAL_ERROR "0 ${HOST_TARGET_Usage} ${HOST_TARGET_ARG_UNPARSED_ARGUMENTS}" )
    ENDIF()
    IF ( "${HOST_TARGET_ARG_SOURCES}" STREQUAL "" )
        MESSAGE( FATAL_ERROR "1 ${HOST_TARGET_Usage}" )
    ENDIF()
    ADD_EXECUTABLE( "${Name}" ${HOST_TARGET_ARG_SOURCES} )
    TARGET_LINK_LIBRARIES( "${Name}" teulserver wwiilib ircclient ${MYSQL_LIBRARIES} )
    IF ( HOST_TARGET_ARG_INSTANCING )
        TARGET_LINK_LIBRARIES( "${Name}" instancing )
    ENDIF()
    IF ( HOST_TARGET_ARG_LUA )
        TARGET_LINK_LIBRARIES( "${Name}" ${LUA_LIBRARIES} ${TOLUAPP_LIBRARIES} )
    ENDIF()
    IF ( NOT "${HOST_TARGET_ARG_EXTRA_DEFINITIONS}" STREQUAL "" )
        SET_PROPERTY( TARGET "${Name}" PROPERTY COMPILE_DEFINITIONS "${HOST_TARGET_ARG_EXTRA_DEFINITIONS}" )
    ENDIF()
    SET_PROPERTY( TARGET "${Name}" PROPERTY FOLDER "Hosts" )
ENDMACRO( ADD_HOST_TARGET )

####################################
## Auth host
ADD_HOST_TARGET( ${AUTH_HOST_EXE} SOURCES
    auth/authClientData.cpp auth/authClientData.h
    auth/authDatabaseSchemas.h
    auth/authGame.h
    auth/authHost.cpp auth/authHost.h auth/authHostMessages.cpp auth/authHostMessages.h
    auth/authQueue.cpp auth/authQueue.h auth/authQueuePosition.h wwii/wwiiRunningAverage.h
    auth/authOTP.cpp auth/authOTP.h
    auth/authPack.cpp auth/authPack.h
)

####################################
## Controller host
ADD_HOST_TARGET( ${CONTROLLER_HOST_EXE} SOURCES
    src/controller.cpp src/controllerGameProcess.cpp src/controllerGameProcess.h
    EXTRA_DEFINITIONS "IS_CONTROLLER_HOST"
)
TARGET_INCLUDE_DIRECTORIES( ${CONTROLLER_HOST_EXE} PRIVATE ${LIBEVENT_INCLUDE_DIRS} )
TARGET_LINK_LIBRARIES( ${CONTROLLER_HOST_EXE} ${LIBEVENT_LIBRARIES} )

####################################
## Chat host

# Primarily communications-oriented server, the chat host.
# Sadly it does an awful lot of the "map/mission" server's job
# because the original design disconnected players from
# map/mission when they spawned.
ADD_HOST_TARGET( ${CHAT_HOST_EXE} SOURCES
    chat/chatDefines.h chat/chatData.h
    chat/chatBanPlayer.cpp chat/chatBanPlayer.h
    chat/chatChannels.h
    chat/chatChannelList.cpp chat/chatChannelList.h chat/chatChannelNumber.cpp chat/chatChannelNumber.h
    chat/chatDB.cpp chat/chatDB.h
    chat/chatClientMessage.h chat/chatGroupsType.h
    chat/chatDotBetaCommands.cpp chat/chatDotCommand.cpp
    chat/chatDotCommand.h chat/chatDotCommands.cpp chat/chatDotGMCommands.cpp
    chat/chatDotHCCommands.cpp chat/chatDotPassthrus.cpp chat/chatDotSquadCommands.cpp
    chat/chatEditString.cpp chat/chatEditString.h
    chat/chatFriendList.cpp chat/chatFriendList.h chat/chatFriendLogin.cpp
    chat/chatGenericString.cpp chat/chatGenericString.h
    chat/chatGrid.cpp chat/chatGrid.h
    chat/chatGroupPermissions.cpp chat/chatGroupPermissions.h
    chat/chatHost.cpp chat/chatHost.h
    chat/chatChannelTypes.h chat/chatPrototypes.h chat/chatChannelID.h
    chat/chatDebugConditions.h
    chat/chatBroadcast.h chat/chatBroadcastHCPacket.h chat/chatBroadcastObject.h
    chat/chatHostMessages.cpp chat/chatHostMessages.h
    chat/chatList.cpp chat/chatList.h
    host/hostPlayers.h
    chat/chatHostPlayerIndex.h
    chat/chatLocFile.cpp chat/chatLocFile.h
    chat/chatManagementHostMessages.cpp chat/chatManagementHostMessages.h
    chat/chatManagementIDs.cpp chat/chatManagementIDs.h
    chat/chatManagementPack.cpp chat/chatManagementPlayer.cpp chat/chatManagementPlayer.h
    chat/chatIRCPlayer.h chat/chatPlayer.h wwii/wwiiPlayer.h
    auth/authDispatch.h mm/mmEnums.h
    chat/chatServerTypes.h chat/chatPlayerInfo.h
    inst/instDescriptor.h miss/missFlags.h
    chat/chatPlayer.cpp chat/chatPlayer.h
    chat/chatServerTrackedObjects.cpp chat/chatServerTrackedObjects.h
    chat/chatBailSTO.h chat/chatHulkSTO.h chat/chatOrdnanceSTO.h chat/chatPPOSTO.h chat/chatServerSTO.h
    chat/chatSTODamage.h chat/chatSTOJoules.h chat/chatUMSSTO.h chat/chatVehicleSTO.h
    chat/chatSpamCan.cpp chat/chatSpamCan.h
    chat/chatStrings.cpp
    chat/chatThreads.cpp chat/chatThreads.h
    chat/chatTimeString.cpp chat/chatTimeString.h
    chat/chatTrainers.cpp chat/chatTrainers.h
    chat/chatUnitMotds.cpp chat/chatUnitMotds.h
    chat/chatWaypoints.cpp chat/chatWaypoints.h
    chat/chatWho.cpp chat/chatWho.h chat/chatWhos.cpp chat/chatWhos.h
    wwii/wwiiServerTrackedObject.cpp wwii/wwiiServerTrackedObject.h
    wwii/wwiiServerTrackedObjectPacker.cpp wwii/wwiiServerTrackedObjectPacker.h
    chat/chatBrigadeControl.cpp chat/chatBrigadeControl.h
    chat/chatBrigadeMove.cpp chat/chatBrigadeMove.h chat/chatBrigadeMoveResults.h
    teul/teulNetworkMsg.h wwii/wwiiNetPorts.h
    chat/chatdbClientMessages.cpp chat/chatdbClientMessages.h
    chat/chatDecisions.cpp chat/chatDecisions.h
    chat/chatHostBrigadeControl.cpp chat/chatHostBrigadeControl.h chat/chatBrigadeVote.h
    chat/chatHostMotds.cpp chat/chatHostMotds.h chat/chatMotds.cpp chat/chatMotds.h
    chat/chatMissionInvite.cpp chat/chatMissionInvite.h
    chat/chatMissionMember.cpp chat/chatMissionMember.h chat/chatMissions.cpp chat/chatMissions.h
    omg/omgChat.cxx omg/omgChat.hxx chat/chatConstants.h
    chat/chatMultiCrew.cpp chat/chatMultiCrew.h
    teul/teulNetworkArray.h teul/teulNetworkObject.h wwii/wwiiPack.h
    chat/chatReports.cpp chat/chatReports.h
    chat/chatSaveChannels.cpp chat/chatSaveChannels.h
    chat/chatSay.cpp chat/chatSay.h
    chat/chatSquadControl.cpp chat/chatSquadControl.h chat/chatSquadControlResults.h
    chat/chatSquadControls.cpp chat/chatSquadCreate.cpp chat/chatSquadCreate.h
    chat/chatSquadList.cpp chat/chatSquadList.h chat/chatSquadListPurge.cpp chat/chatSquadListPurge.h
    chat/chatSquadVote.cpp chat/chatSquadVote.h wwii/wwiiSquads.cpp wwii/wwiiSquads.h
    map/mapAWS.cpp map/mapAWS.h
    map/mapHCUnitLocations.cpp map/mapHCUnitLocations.h
    strt/strtChatTypes.cpp strt/strtChatTypes.h
    wwii/wwiiHCUnits.cpp wwii/wwiiHCUnits.h
    dbse/dbseClient.cpp dbse/dbseClient.h dbse/dbseTypes.h dbse/dbseConstants.h
    dbse/dbseClientMessages.cpp dbse/dbseClientMessages.h
    teul/teulBasic.h wwii/wwiiCallbacks.h
    INSTANCING
    EXTRA_DEFINITIONS "IS_CHAT_HOST"
)

####################################
## Cell host

# Only server of which multiple instances are intended to operate
# in a single cluster. The work is distributed based on load, not
# on any clever spatial distribution.
# Currently, when a player spawns, they are actually transferred
# (teul redirected) from the map host to this host. When they
# despawn, they are redirected back to the map host.
# Unfortunately, redirection is crudely implemented, and the player
# actually /leaves/ the map host, preventing the map host from dealing
# with map/mission work for any spawned players.
# Current implementation only works for a single cell host.
ADD_HOST_TARGET( ${CELL_HOST_EXE} SOURCES
    chat/chatCellHostSupportMessages.cpp chat/chatCellHostSupportMessages.h
    chat/chatSupportRPCMessages.cpp chat/chatSupportRPCMessages.h
    comm/commHostMessages.cpp comm/commHostMessages.h comm/commTypes.h
    updt/updtEnums.h updt/updtInfDeath.h
    wwii/wwiiExitCodes.h weap/weapSerial.h wwii/wwiiRank.h
    comm/commPackers.cpp comm/commPackers.h
    host/host.cpp host/host.h
    miss/missionPacket.h mm/mmEnums.h
    host/host.h host/hostPlayers.h
    omg/omgMissionSortieData.cxx omg/omgMissionSortieData.hxx
    miss/missFlags.h wwii/wwiiSyncVar.h
    teul/teulNetTimeMS.h teul/teulNetTimeMSPacker.h
    weap/weapSpecPacker.h wwii/wwiiFullObjIDPacker.h
    host/hostPlayer.h host/hostFraming.cpp host/hostFraming.h host/hostGrid.cpp
    host/hostScoringActor.cpp
    host/hostVehicle.cpp host/hostVehicle.h inst/instDescriptor.h
    updt/updtTypes.h updt/updtTurrets.h
    updt/updtFraming.h updt/updtVehiclePosition.h
    strt/strtTypes.h include/xyz_tuple.h
    src/installCallbacks.cpp miss/missionPack.cpp miss/missionPack.h
    mm/mmCellHostSupportMessages.cpp mm/mmCellHostSupportMessages.h
    mm/mmPack.cpp mm/mmPack.h updt/updtPackers.cpp updt/updtPackers.h
    wwii/wwiiHostMessages.cpp wwii/wwiiHostMessages.h
    host/hostAutomation.cpp host/hostAutomation.h
    host/hostCellNet.cpp host/hostCellNet.h host/hostGridCell.cpp host/hostGridCell.h
    miss/missionPacket.cpp miss/missionPacket.h
    parse/parseWor.cpp parse/parseWor.h parse/parseIO.h strt/strtWOR.h parse/parse.h strt/strtDTL.h
    strt/strtCPColliders.cpp strt/strtCPColliders.h strt/strtCPStruct.cpp strt/strtCPStruct.h
    strt/strtProtos.h wwii/wwiiHCUnits.h
    strt/strtCPFlags.h strt/strtData.cpp strt/strtData.h
    strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h
    strt/strtHack.cpp strt/strtHack.h strt/strtIO.cpp strt/strtIO.h
    strt/strtObjectStruct.cpp strt/strtObjectStruct.h
    strt/strtPack.cpp strt/strtPack.h
    strt/strtSyncData.cpp strt/strtSyncData.h strt/strtConstants.h
    strt/strtSyncSupportMessages.cpp strt/strtSyncSupportMessages.h
    strt/strtUpdater.cpp strt/strtUpdater.h
    strt/strtUpdaterHostMessages.cpp strt/strtUpdaterHostMessages.h
    strt/strtUpdaterPack.cpp strt/strtUpdaterPack.h
    strt/strtUpdaterSupportMessages.cpp strt/strtUpdaterSupportMessages.h
    INSTANCING
    EXTRA_DEFINITIONS "IS_CELL_HOST"
)

####################################
## Map/Mission Host

# Probably better described as the "Game Login Server".
# This is the host to which players are forwarded from auth/playgate.
# See the description of the cell host above for details on host spawning
# works and why the chat host has wound up doing so much of map host's job.
ADD_HOST_TARGET( ${MAP_HOST_EXE} SOURCES
    chat/chatGroupPermissions.cpp chat/chatGroupPermissions.h chat/chatGroupsType.h
    chat/chatSupportMessages.cpp chat/chatSupportMessages.h
    chat/chatSupportRPCMessages.cpp chat/chatSupportRPCMessages.h
    inst/instHostMessages.cpp inst/instHostMessages.h
    map/mapArenaDescription.cpp map/mapArenaDescription.h
    map/mapChecksums.cpp map/mapChecksums.h
    map/mapDB.cpp map/mapDB.h
    auth/authPlayerDetails.h
    map/mapHost.cpp map/mapHostMessages.cpp map/mapHostMessages.h
    map/mapMinuteBalance.cpp map/mapMinuteBalance.h
    map/mapPack.cpp
    map/mapPlayer.cpp map/mapPlayer.h map/mapPlayerTypes.h
    miss/missionTypes.h miss/missionPacket.h miss/missFlags.h
    wwii/wwiiPlayer.h mm/mmEnums.h wwii/wwiiRank.h weap/weapSerial.h
    omg/omgMissionSortieData.cxx omg/omgMissionSortieData.hxx
    wwii/wwiiSyncVar.h teul/teulNetTimeMS.h teul/teulNetTimeMSPacker.h
    weap/weapSpecPacker.h wwii/wwiiFullObjIDPacker.h
    inst/instDescriptor.h auth/authDispatch.h
    map/mapThreads.cpp map/mapThreads.h
    miss/missActiveMission.cpp miss/missActiveMission.h miss/mission.cpp miss/mission.h
    miss/missionDetail.cpp miss/missionDetail.h map/mapMissionList.h miss/missActiveMission.h
    teul/teulNetworkMsg.h wwii/wwiiNetPorts.h
    miss/missionHostMessages.cpp miss/missionHostMessages.h
    miss/missionPack.cpp miss/missionPack.h
    miss/missMobileSpawn.cpp miss/missMobileSpawn.h include/mobileSpawnEnums.h
    mm/mmAntiAddiction.cpp mm/mmAntiAddiction.h wwii/wwiiAntiAddiction.h
    mm/mmHostInstanceImpl.cpp mm/mmHostInstanceImpl.h
    inst/instHostInstanceImpl.h inst/instHostInstance.h inst/instBasicStatus.h
    host/hostPlayers.h inst/instStateEnums.h
    mm/mmHostMessages.cpp mm/mmHostMessages.h mm/mmSpawnTypes.h
    mm/mmMain.cpp mm/mmMain.h mm/mmPack.cpp mm/mmPack.h
    mm/mmPolls.cpp mm/mmPolls.h
    mm/mmSpawn.cpp mm/mmSpawn.h
    mm/mmSpawnGrantResponse.cpp mm/mmSpawnGrantResponse.h
    score/scoreCallbacks.cpp score/scoreDeath.cpp score/scoreDeath.h score/scoreEvent.h
    score/scoreActor.h score/scoreSortie.cpp score/scoreSortie.h
    wwii/wwiiRTBCodes.h wwii/wwiiHostMessages.cpp wwii/wwiiHostMessages.h
    wwii/wwiiOpinionPoll.cpp wwii/wwiiOpinionPoll.h
    teul/teulNetworkObject.h
    map/mapBrigades.cpp map/mapBrigades.h map/mapBrigadeSummary.h map/mapPersonaTypes.h
    wwii/wwiiHCUnits.h
    map/mapData.cpp map/mapData.h
    map/mapHCUnitLocations.cpp map/mapHCUnitLocations.h
    teul/teulNetworkArray.h wwii/wwiiPack.h
    map/mapHostSupercellData.cpp
    miss/missionPacket.cpp miss/missionPacket.h
    miss/missionScoring.cpp miss/missionUtils.cpp miss/missionUtils.h
    mm/mmdbPersonaSelections.cpp mm/mmdbPersonaSelections.h
    parse/parseWor.cpp parse/parseWor.h parse/parseIO.h strt/strtWOR.h parse/parse.h strt/strtDTL.h
    strt/strtCPStruct.cpp strt/strtCPStruct.h strt/strtProtos.h strt/strtCPFlags.h
    strt/strtData.cpp strt/strtData.h strt/strtDB.cpp strt/strtDB.h
    strt/strtEvents.cpp strt/strtEvents.h
    strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h
    strt/strtIO.cpp strt/strtIO.h
    strt/strtMapMessages.cpp strt/strtMapMessages.h
    strt/strtObjectStruct.cpp strt/strtObjectStruct.h
    strt/strtPack.cpp strt/strtPack.h
    strt/strtSyncData.cpp strt/strtSyncData.h
    strt/strtConstants.h strt/strtSyncSupportMessages.cpp strt/strtSyncSupportMessages.h
    wwii/wwiiHCUnits.cpp wwii/wwiiHCUnits.h
    EXTRA_DEFINITIONS "IS_MAP_HOST"
    INSTANCING
)

####################################
## Database Proxy Daemon [deprecated]

# Because the original host implementation was
# entirely single-threaded, this host process was created to offload
# expensive synchronous database queries.
# You should really use DB::Threads instead.
ADD_HOST_TARGET( ${DBD_HOST_EXE} SOURCES
    chat/chatdbHostMessages.cpp chat/chatdbParser.cpp chat/chatdbParser.h
    dbse/dbseHost.cpp dbse/dbseHostMessages.cpp dbse/dbseConstants.h dbse/dbseTypes.h
    wwii/wwiiCallbacks.h teul/teulBasic.h server/serverConfigVars.h
    wwii/wwiiSquads.cpp wwii/wwiiSquads.h
    EXTRA_DEFINITIONS "IS_DBD_HOST"
    INSTANCING
)

####################################
## Strat host

# Build the Lua glue file
ADD_CUSTOM_COMMAND(
    OUTPUT ${STRAT_HOST_EXE}Lua.cc
    COMMAND ${TOLUAPP_EXECUTABLE}
        -o ${CMAKE_CURRENT_BINARY_DIR}/${STRAT_HOST_EXE}Lua.cc
        -n game -D -C -E project=ww2ol
        hostproj/${STRAT_HOST_EXE}.pkg
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    MAIN_DEPENDENCY hostproj/${STRAT_HOST_EXE}.pkg
    DEPENDS
        ${TOLUAPP_EXECUTABLE} ${TOLUAPP_LIBRARIES}
        hostproj/syscfg.h hostproj/${STRAT_HOST_EXE}.pkg
        include/platforms.h include/systypes.h include/luaGlue.h include/precompile_includes.h
        wwii/wwiiThreads.h wwii/wwiiOwnershipEnums.h inst/instBasicStatus.h wwii/wwiiConfig.h
        chat/chatHandle.h weap/weapSpec.h
)
SET_PROPERTY( SOURCE ${STRAT_HOST_EXE}Lua.cc PROPERTY DEPENDS ${TOLUAPP_EXECUTABLE} ${TOLUAPP_LIBRARIES} )

# Most of the other hosts maintain copies of the strat data, but this is the
# center of authority. Because this host is generally doing very little, it
# typically sits on the same machine as the MySQL database, and it often gets
# offloaded with more convoluted gameplay management stuff, which
# turns out to be a real pain in the ass because you wind up writing
# a metric ton of network routines for communicating with it...
# It also hosts a Lua scripting environment.
ADD_HOST_TARGET( ${STRAT_HOST_EXE} SOURCES
    strt/strtChatTypes.h
    chat/chatStratSupportMessages.cpp chat/chatStratSupportMessages.h
    chat/chatSupportRPCMessages.cpp chat/chatSupportRPCMessages.h
    cnvy/cnvyConvoyList.cpp cnvy/cnvyConvoyList.h
    cnvy/cnvyMain.cpp cnvy/cnvyMain.h
    strt/strtProtos.h strt/strtEventQueue.h strt/strtTypes.h strt/strtServerTypes.h
    mem/memPool.h teul/teulCLList.h list/listCL.h include/intrusiveList.h
    strt/strtDatabaseSchemas.h
    src/luaGlue.cpp include/luaGlue.h
    miss/missionPack.cpp miss/missionPack.h
    mm/mmPack.cpp mm/mmPack.h
    parse/parseWor.cpp parse/parseWor.h parse/parseIO.h parse/parse.h
    strt/strtWOR.h strt/strtDTL.h
    strt/strtChatTypes.cpp strt/strtChatTypes.h
    strt/strtCPClass.cpp strt/strtCPClass.h
    strt/strtCP.cpp strt/strtCP.h strt/strtCPStruct.cpp strt/strtCPStruct.h strt/strtCPFlags.h
    wwii/wwiiHCUnits.h inst/instDescriptor.h mm/mmEnums.h
    strt/strtData.cpp strt/strtData.h strt/strtDB.cpp strt/strtDB.h
    strt/strtdbSaveWorld.cpp strt/strtdbSaveWorld.h
    strt/strtCaptureAction.cpp strt/strtCaptureAction.h
    strt/strtCaptureContribution.cpp strt/strtCaptureContribution.h strt/strtCaptureScore.h
    strt/strtDotCommands.cpp strt/strtDotCommands.h
    strt/strtEnemyReport.cpp strt/strtEnemyReport.h
    strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h strt/strtFB.cpp strt/strtFB.h
    strt/strtHack.cpp strt/strtHack.h
    strt/strtHCUnits.cpp strt/strtHCUnits.h chat/chatBrigadeMoveResults.h
    strt/strtTOETypes.h
    strt/strtHost.cpp strt/strtHostDB.cpp strt/strtHostDB.h
    strt/strtHostMessages.cpp strt/strtHostMessages.h
    strt/strtMapTypes.h mm/mmSpawnTypes.h miss/missionTypes.h wwii/wwiiRTBCodes.h weap/weapSerial.h
    strt/strtIO.cpp strt/strtIO.h
    strt/strtJoinRequestList.cpp
    strt/strtHostLua.cpp strt/strtHostLua.h
    strt/strtMain.cpp strt/strtMain.h
    strt/strtMission.cpp strt/strtMission.h
    strt/strtObjectStruct.cpp strt/strtObjectStruct.h
    strt/strtPack.cpp strt/strtPack.h
    strt/strtPostedWeaponList.cpp strt/strtPostedWeaponList.h
    strt/strtJoinList.h
    strt/strtSpawn.cpp strt/strtSpawn.h strt/strtWeaponsReturn.h strt/strtSpawnWeaponList.cpp
    strt/strtSupply.cpp strt/strtSupply.h
    strt/strtSync.cpp strt/strtSync.h strt/strtSyncData.cpp strt/strtSyncData.h strt/strtConstants.h
    strt/strtTOE.cpp strt/strtTOE.h
    strt/strtWeaponEntry.cpp strt/strtWeaponEntry.h
    strt/strtTypes.h strt/strtObjectStruct.h
    weap/weapSpawnXML.cpp weap/weapSpawnXML.h
    wwii/wwiiHCUnits.cpp wwii/wwiiHCUnits.h
    list/listCL.cpp list/listCL.h list/list.h
    ${STRAT_HOST_EXE}Lua.cc
    EXTRA_DEFINITIONS "IS_STRAT_HOST" "LUA_MODULE_OPEN=tolua_stratd_open" "CHECK_SUPPLY=1"
    INSTANCING LUA
)
IF ( MSVC )
    SET_PROPERTY( SOURCE ${STRAT_HOST_EXE}Lua.cc PROPERTY COMPILE_FLAGS
                  "/W0" "/wd4480" "/wd4800" "/wd4305" "/wd4505" )
ENDIF()
TARGET_INCLUDE_DIRECTORIES( ${STRAT_HOST_EXE} PRIVATE ${TOLUAPP_INCLUDE_DIR} )

INSTALL(
    TARGETS
        ${CONTROLLER_HOST_EXE} ${STRAT_HOST_EXE} ${DBD_HOST_EXE}
        ${CHAT_HOST_EXE} ${MAP_HOST_EXE} ${CELL_HOST_EXE} ${AUTH_HOST_EXE}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    COMPONENT hosts
)

####################################
## File installs

# Environment template files from the source tree
INSTALL( FILES environment.cfg environment.win.cfg DESTINATION bin COMPONENT config )

# Lua scripts
INSTALL( DIRECTORY hostproj/lua/ DESTINATION lua COMPONENT lua PATTERN ".git" EXCLUDE PATTERN "*.lua" )

# Embedded webserver root
INSTALL( DIRECTORY hostproj/web/ DESTINATION web COMPONENT web PATTERN ".git" EXCLUDE )

####################################
## Tests

# Auth connection test
ADD_EXECUTABLE( testAuthConnect tests/testAuthConnect.cpp )
TARGET_LINK_LIBRARIES( testAuthConnect wwiilib teulserver ${MYSQL_LIBRARIES} )
SET_PROPERTY( TARGET testAuthConnect PROPERTY FOLDER "Test" )

# Builds a single executable that runs all of the unit tests;
# The install process will not execute unless all tests complete.
ADD_EXECUTABLE( testUnits
    tests/testUnits.cpp tests/testUnits.h tests/testCovert.cpp tests/testLocalizedMessage.cpp
    tests/testTeulBits.cpp tests/testAuthQueue.cpp auth/authClientData.cpp
    tests/testChatHandle.cpp tests/testFullObjID.cpp tests/testCommandWithArgs.cpp
    tests/testInplaceVector.cpp tests/testPackers.cpp comm/commPackers.cpp
    tests/testChatMissionStatus.cpp tests/testHostGridCell.cpp host/hostGridCell.cpp
    tests/testSupercellVislevelXfer.cpp tests/testPersonaXfer.cpp tests/testCryptographer.cpp
    tests/testXYZTuple.cpp tests/testTeulPacket.cpp
    wwii/wwiiServerTrackedObjectPacker.cpp omg/omgChat.cxx omg/omgChat.hxx
    tests/testSuite.h tests/testUnits.h
)
SET_PROPERTY( TARGET testUnits PROPERTY COMPILE_DEFINITIONS "ENABLE_TESTS" )
TARGET_LINK_LIBRARIES( testUnits wwiilib teulserver ${MYSQL_LIBRARIES} )
SET_PROPERTY( TARGET testUnits PROPERTY FOLDER "Test" )

ADD_TEST( testUnits testUnits )

# teul tests
ADD_EXECUTABLE(testTeul
    tests/testTeul.cpp teul/teulExceptions.h teul/teulKit.h teul/teulEndpoint.h
    teul/teulEndpointImpl.h teul/teulPacket.h teul/teulPacketHeader.h
    wwii/wwiiCallbacks.h tests/testTeulTest.h tests/testTeulClient.h tests/testTeulServer.h
    teul/teulVersion.h teul/teulConnectionInstruction.h
)
TARGET_LINK_LIBRARIES( testTeul teulserver wwiilib ${MYSQL_LIBRARIES} )
SET_PROPERTY( TARGET testTeul PROPERTY FOLDER "Test" )

######################################################################
#### Package building.

SET(CPACK_COMPONENTS_ALL game lua web auth)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_INSTALL_PREFIX "${_PLAYNET}/wwiiol")
SET(CPACK_SET_DESTDIR ON)
SET(CPACK_PACKAGING_INSTALL_PREFIX "${_PLAYNET}/wwiiol")
SET(OptLevel "NoOptimization")
IF( PN_ENABLE_OPTS AND OPTIMIZATION GREATER 0 )
 SET(OptLevel "OptLevel${OPTIMIZATION}")
ENDIF( )
SET(CPACK_PACKAGE_VERSION "${REPO_WC_REVISION}.${CMAKE_BUILD_TYPE}.${BUILDNAME}.${OptLevel}")
SET(CPACK_OUTPUT_FILE_PREFIX "${_PLAYNET}/packages")
SET(CPACK_PACKAGE_VENDOR "Cornered Rat Software")
SET(CPACK_PACKAGE_NAME "wwiiol")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WWII Online game servers")
SET(CPACK_PACKAGE_DESCRIPTION "'BattleGround Europe: WWII Online' game servers, auth host and tools.")
IF( PN_CLUSTER STREQUAL "Auth" )
 SET(CPACK_PACKAGE_NAME "wwiiol-auth")
 SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WWIIOL Auth Host")
 SET(CPACK_PACKAGE_DESCRIPTION "'BattleGround Europe: WWII Online' stand-alone auth host.")
ENDIF( )
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Oliver Smith")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libopenthreads-dev(>= 2.8), lua5.1(>= 5.1.4-5), libmysqlclient16(>= 5.5.0), bash(>= 4.1), gdb(>= 7.1), nfs-common, mysql-server(>= 5.5)")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "WWIIOL Game Servers")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Network")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "Important")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "vim, mysql-client(>= 5.5), python(>= 2.6.5), bzip2")
SET(CPACK_DEBIAN_PACKAGE_SUGGESTS "perl(>= 5.1), etckeeper, byobu")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes/preinst;${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes/postinst;")

INCLUDE(CPack)      # For making installers

SET(_PREV_BUILD_TYPE "${PN_BUILD_TYPE}" CACHE INTERNAL "Value of PN_BUILD_TYPE last time CMake Ran" FORCE)
