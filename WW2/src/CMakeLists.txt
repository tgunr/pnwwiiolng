## Declare the CMake version requirement
CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)

## Declare the project
PROJECT(GameHosts)


SET(CMAKE_CXX_STANDARD 14)

# This will determine bitness, amongst other things.
INCLUDE( cmakeincludes/Platform.cmake )

################################################################################
## Configuration.

ENABLE_TESTING()
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes")
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
SET(LIBRARIES_DIR "${PROJECT_DIR}/Libraries")

SET(AUTH_HOST_EXE "authHost")
SET(CONTROLLER_HOST_EXE "controller")
SET(DBD_HOST_EXE "dbd")
SET(CHAT_HOST_EXE "chatd")
SET(CELL_HOST_EXE "hostd")
SET(MAP_HOST_EXE "mmhostd")
SET(STRAT_HOST_EXE "stratd")

SET(_LUA_HOST "stratd")

IF( PN_BUILD_TYPE MATCHES "Auth" OR PN_CLUSTER MATCHES "Auth" )
    SET(PN_BUILD_TYPE "Auth")
    SET(PN_CLUSTER "Auth")
    SET(CMAKE_BUILD_TYPE "Debug")
    SET(PN_MEM_DEBUG ON)
    SET(PN_STACK_PROTECT ON)
    SET(PN_TRACER ON)
ENDIF( )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hostproj ${CMAKE_CURRENT_BINARY_DIR})

# Define and validate the default top-level for this build.
INCLUDE( cmakeincludes/PlaynetPath.cmake )

# Define and validate user-specifiable options.
INCLUDE( cmakeincludes/UserOptions.cmake )

# Process the PN_BUILD_TYPE setting.
INCLUDE( cmakeincludes/TargetEnv.cmake )

# Do Platform specific checks and build config.h
INCLUDE( cmakeincludes/PlatformChecks.cmake )

# Check various dependencies.
INCLUDE( cmakeincludes/Dependencies.cmake )

include( cmakeincludes/PCHSupport.cmake )
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
    wwii/wwiiHash.h
    wwii/wwiiOwnershipEnums.h
    wwii/wwiiConfig.h
    server/serverServerInfo.h
    teul/teulTime.h
    teul/teulClosingEnums.h
    teul/teulConfigVar.h
    teul/teulPackers.h
    teul/teulKit.h
    teul/teulBasic.h
    dbug/dbugExtErrors.h
    wwii/wwiiPermissions.h
    inst/instTypes.h
    wwii/wwiiFullObjID.h
    teul/teulPacket.h
    weap/weapWeapon.h
    teul/teulEndpoint.h
    chat/chatLStringIDs.h
)

######################################################################

MESSAGE(STATUS "${PN_BUILD_TYPE}/${BUILDNAME} build. Opt:${OPTIMIZATION}, MEM:${PN_MEM_DEBUG}, STACK:${PN_STACK_PROTECT}")

######################################################################
#### Code Generators.

SET(OMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/omg")
SET(OMGGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/omggen")

# bison first, then flex.
IF(WIN32)
    SET(BISON_EXE "${OMGGEN_DIR}/win_bison.exe")
    SET(FLEX_EXE "${OMGGEN_DIR}/win_flex.exe")
ELSE( )
    SET(BISON_EXE "bison")
    SET(FLEX_EXE "flex")
ENDIF( )

ADD_CUSTOM_COMMAND(OUTPUT ${OMGGEN_DIR}/omggen.tab.cpp ${OMGGEN_DIR}/omggen.tab.h
    PRE_BUILD
    COMMAND ${BISON_EXE}
    ARGS -o omggen.tab.cpp --defines=omggen.tab.h --report=all omggen.y
    DEPENDS "${OMGGEN_DIR}/omggen.y"
    MAIN_DEPENDENCY "${OMGGEN_DIR}/omggen.y"
    COMMENT "[BISON] Building OMGGEN"
    WORKING_DIRECTORY ${OMGGEN_DIR}
)
ADD_CUSTOM_COMMAND(OUTPUT ${OMGGEN_DIR}/omggen.yy.cpp ${OMGGEN_DIR}/omggen.yy.h
    PRE_BUILD
    COMMAND ${FLEX_EXE}
    ARGS -o omggen.yy.cpp -B --nounistd --header-file=omggen.yy.h omggen.l
    DEPENDS ${OMGGEN_DIR}/omggen.l ${OMGGEN_DIR}/omggen.tab.h
    MAIN_DEPENDENCY "${OMGGEN_DIR}/omggen.l"
    COMMENT "[FLEX] Building OMGGEN"
    WORKING_DIRECTORY ${OMGGEN_DIR}
)
ADD_EXECUTABLE(OMGgen
    ${OMGGEN_DIR}/omggen.tab.cpp
        ${OMGGEN_DIR}/omggen.tab.h
    ${OMGGEN_DIR}/omggen.yy.cpp
        ${OMGGEN_DIR}/omggen.yy.h
    ${OMGGEN_DIR}/omggen.cpp
        ${OMGGEN_DIR}/omggen.h
    ${OMGGEN_DIR}/omggen.y
    ${OMGGEN_DIR}/omggen.l
)
IF(WIN32)
    SET_PROPERTY(TARGET OMGgen PROPERTY COMPILE_DEFINITIONS __STDC_VERSION__=199901L)
ENDIF()

MACRO(ADD_OMG_TARGET _TARGET)
    SET(_TARGET_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/${_TARGET}")
    GET_FILENAME_COMPONENT(_TARGET_PREFIX ${_TARGET_PREFIX} ABSOLUTE)
    GET_FILENAME_COMPONENT(_TARGET_PATH ${_TARGET_PREFIX}.omg DIRECTORY)
    GET_FILENAME_COMPONENT(_FILENAME ${_TARGET_PREFIX} NAME)
    ADD_CUSTOM_COMMAND(
        PRE_BUILD
        OUTPUT  ${_TARGET_PREFIX}.cxx ${_TARGET_PREFIX}.hxx
        DEPENDS OMGgen ${_TARGET_PATH}/typedefs.omg ${_TARGET_PREFIX}.omg
        MAIN_DEPENDENCY ${_TARGET_PREFIX}.omg
        COMMENT "[OMG] ${_FILENAME}.omg"
        COMMAND OMGgen
        ARGS -D${_TARGET_PATH} --cxx=${_FILENAME}.cxx --hxx=${_FILENAME}.hxx ${_FILENAME}.omg
        WORKING_DIRECTORY ${_TARGET_PATH}
    )
ENDMACRO()

ADD_OMG_TARGET(omg/omgMissionSortieData)
ADD_OMG_TARGET(omg/omgChat)

IF(NOT MSVC)
    SET_TARGET_PROPERTIES(OMGgen PROPERTIES COMPILE_FLAGS "-O3 -march=native -mtune=native -Wno-error -Wno-all -Wno-format-security") #
ENDIF( )
SET_PROPERTY(TARGET OMGgen PROPERTY FOLDER "Tools")

######################################################################
#### Libraries.
ADD_LIBRARY(libb64
    ${LIBRARIES_DIR}/libb64/libb64-1.2/src/cdecode.c
    	${LIBRARIES_DIR}/libb64/libb64-1.2/include/b64/cdecode.h
    		${LIBRARIES_DIR}/libb64/libb64-1.2/include/b64/decode.h
    ${LIBRARIES_DIR}/libb64/libb64-1.2/src/cencode.c
    	${LIBRARIES_DIR}/libb64/libb64-1.2/include/b64/cencode.h
    		${LIBRARIES_DIR}/libb64/libb64-1.2/include/b64/encode.h
)
SET_PROPERTY(TARGET libb64 PROPERTY COMPILE_DEFINITIONS IN_BUILDING_LIBIRC=1)
SET_PROPERTY(TARGET libb64 PROPERTY FOLDER "Libs")
IF(WIN32 OR WIN64)
    SET_TARGET_PROPERTIES(libb64 PROPERTIES COMPILE_FLAGS "${CXX_COMPILE_FLAGS} /W1")
ENDIF( )

ADD_LIBRARY(ircclient
    libircclient/src/libircclient.cpp
)
SET_PROPERTY(TARGET ircclient PROPERTY COMPILE_DEFINITIONS IN_BUILDING_LIBIRC=1)
SET_PROPERTY(TARGET  ircclient PROPERTY FOLDER "Libs")
IF(WIN32 OR WIN64)
    SET_TARGET_PROPERTIES(ircclient PROPERTIES COMPILE_FLAGS "${CXX_COMPILE_FLAGS} /W1")
ENDIF( )

# TeulServer library - this is a baseline library that provides
# teulKit services and the cluster's "server" API.
ADD_LIBRARY(teulserver
    server/server.cpp
        server/server.h
            server/serverReleaseTypeEnum.h
    server/serverLuaInterface.cpp
    server/serverMessages.cpp
        server/serverMessages.h
    server/serverPeers.cpp
        server/serverPeers.h
    server/serverTransfer.cpp
        server/serverTransfer.h
    str/stringLib.cpp
        str/stringLib.h
    teul/teulCallback.cpp
        teul/teulCallback.h
    teul/teulEndpoint.cpp
        teul/teulEndpoint.h
        teul/teulEndpointImpl.h
    teul/teulMicroWebServer.cpp
        teul/teulMicroWebServer.h
            include/pnThread.h
    teul/teulModCalls.cpp
        teul/teulModCalls.h
    teul/teulModRecv.cpp
    teul/teulModSend.cpp
    teul/teulModule.cpp
        teul/teulModule.h
    teul/teulNetTimeMS.cpp
        teul/teulNetTimeMS.h
            teul/teulNetTimeMSPacker.h
                teul/teulNetTimeMS.h
        teul/teulBits.h
    teul/teulPack.cpp
    teul/teulPacket.cpp
        teul/teulPacket.h
    teul/teulTransmission.cpp
        teul/teulTransmission.h
    teul/teulTransport.cpp
    teul/teulUtils.cpp
        teul/teulUtils.h
    wwii/wwiiCallbacks.cpp
        wwii/wwiiCallbacks.h
            teul/teulCallback.h
            teul/teulCallbackInit.h
    auth/authPack.cpp
        auth/authPack.h
    auth/authGameMessages.cpp
        auth/authGameMessages.h
            auth/authTypes.h
                auth/authServerTypes.h
    teul/teulAuth.cpp
        teul/teulAuth.h
            updt/loginTypes.h
    wwii/wwiiConfig.cpp
        wwii/wwiiConfig.h
        wwii/wwiiConfigMap.h

    ${PCH_INCLUDE_HEADERS}
    dbug/dbugInternal.h
    mem/memPool.h
)
ADD_PCH(teulserver)
TARGET_LINK_LIBRARIES(teulserver event)
SET_PROPERTY(TARGET teulserver PROPERTY FOLDER "Libs")

# Code for any host process that has to participate in awareness of instances
#(chat, map, cell do; strat doesn't)
ADD_LIBRARY(instancing
    chat/chatLocalizedMessage.cpp
        chat/chatLocalizedMessage.h
            chat/chatDefines.h
    inst/instCommon.cpp
    inst/instHost.cpp
        inst/instHost.h
    inst/instPackers.cpp
    wwii/wwiiPlayer.cpp
        wwii/wwiiPlayer.h
            mm/mmEnums.h
            inst/instDescriptor.h
            auth/authDispatch.h
            miss/missFlags.h
                wwii/wwiiBranchEnums.h
    chat/chatPackers.cpp

    ${PCH_INCLUDE_HEADERS}
)
ADD_PCH(instancing)
SET_PROPERTY(TARGET instancing PROPERTY FOLDER "Libs")

# Common-to-all WWII library, provides all the basics like ChatHandle functions,
# weapon list, database operations, wwii_config, etc.
ADD_LIBRARY(libwwii 
    chat/chatHandle.cpp
        chat/chatHandle.h
        chat/chatName.h
    chat/chatPackers.cpp
    chat/chatTimeInterval.cpp
        chat/chatTimeInterval.h
    dbug/dbug.cpp
        dbug/dbug.h
    fio/fiod.cpp
        fio/fiod.h
    inst/instCommon.cpp
    inst/instPackers.cpp
    parse/countryCodes.cpp
        parse/countryCodes.h
    parse/parse.cpp
        parse/parse.h
    parse/parseIO.cpp
        parse/parseIO.h
            parse/parse.h
    weap/weapList.cpp
        weap/weapList.h
            parse/countryCodes.h
            weap/weapSerial.h
    weap/weapLoadIndexes.cpp
        weap/weapLoadIndexes.h
    weap/weapParser.cpp
        weap/weapParser.h
    weap/weapSpec.cpp
        weap/weapSpec.h
    tut/tutGroupProgress.cpp
        tut/tutGroupProgress.h
    wwii/wwiiCommandLineArgs.cpp
        wwii/wwiiCommandLineArgs.h
    wwii/wwiiIRC.cpp
        wwii/wwiiIRC.h
            host/hostCommand.h
    wwii/wwiiRank.cpp
        wwii/wwiiRank.h
    wwii/wwiiString.cpp
        wwii/wwiiString.h
    wwii/wwiiThreads.cpp
        wwii/wwiiThreads.h
        wwii/wwiiThreadTypes.h
    wwii/wwiiThreadText.cpp
        wwii/wwiiThreadText.h
        wwii/wwiiThreadTextQuery.h
    wwii/wwiiUTF.cpp
        wwii/wwiiUTF.h
    score/scoreActor.cpp
        score/scoreActor.h
    src/io_mapped_file.cpp
        include/io_mapped_file.h
    src/mydbQuery.cpp
        include/mydbQuery.h
    src/myDbseWrapper.cpp
    wwii/wwiiDBConfig.cpp
        wwii/wwiiDBConfig.h
    wwii/wwiiDBThreads.cpp
        wwii/wwiiDBThreads.h
            wwii/wwiiThreads.h
    wwii/wwiiSerialNumber.cpp
        wwii/wwiiSerialNumber.h
    wwii/wwiiSortieManager.cpp
        wwii/wwiiSortieManager.h
    wwii/wwiiSyncVar.cpp
        wwii/wwiiSyncVar.h
    sdm/sdmHash.cpp
        sdm/sdmHash.h
    sdm/sdmParse.cpp
        sdm/sdmParse.h
            strt/strtDTL.h
                wwii/wwiiTypes.h
    src/wthrTime.cpp
        include/wthrTime.h
    strt/strtSupercell.cpp
        strt/strtSupercell.h
        strt/strtSupercells.h
    wwii/wwiiCallbacks.cpp
        wwii/wwiiCallbacks.h
            teul/teulCallback.h
            teul/teulCallbackInit.h
    wwii/wwiiHCUnitsCommon.cpp
    src/configVars.cpp

    sdm/sdmConfigVars.h
    server/serverConfigVars.h
    wwii/wwiidbConfigVars.h

    ${PCH_INCLUDE_HEADERS}
    include/io_mapped_file.h
    str/strdupptr.h
    wwii/wwiiNetPorts.h
    wwii/wwiiCallbacks.h
    wwii/wwiiThreads.h
)
ADD_PCH(libwwii)
TARGET_LINK_LIBRARIES(libwwii)
SET_PROPERTY(TARGET libwwii PROPERTY FOLDER "Libs")

######################################################################
#### Client/Utility command line applications.

IF( NOT PN_CLUSTER STREQUAL "Auth")
    IF( NOT WIN32 )
        # Command-line based access to the Chat Host(chatClient)
        ADD_EXECUTABLE(chatClient
            chat/chatLocFile.cpp
                chat/chatLocFile.h
            chat/chatManagementClientMessages.cpp
                chat/chatManagementClientMessages.h
            chat/chatManagementPack.cpp
            chat/chatRoster.cpp
            chat/chatRosterClient.cpp
        )
        ADD_PCH(chatClient)
        TARGET_LINK_LIBRARIES(chatClient libwwii teulserver ${MYSQL_LIBRARIES})
        SET_PROPERTY(TARGET chatClient PROPERTY FOLDER "Tools")

        # Provide access to the LuaVMs.
        ADD_EXECUTABLE(luaClient
            src/luaClient.cpp
        )
        TARGET_LINK_LIBRARIES(luaClient libwwii teulserver readline ncurses ${MYSQL_LIBRARIES})
        SET_PROPERTY(TARGET luaClient PROPERTY FOLDER "Tools")
    ENDIF( )

    # Tool for validating strings as possible gamenames.
    #(China was using non-standard characters in their game names)
    ADD_EXECUTABLE(checkGameName
        src/checkGameName.cpp
    )
    TARGET_LINK_LIBRARIES(checkGameName libwwii teulserver ${MYSQL_LIBRARIES})
    SET_PROPERTY(TARGET checkGameName PROPERTY FOLDER "Tools")

    # In theory, SDM pre-parser. In practice? Deletes SDM files that don't
    # contain any strat data.
    # Not part of stratValidator because the original coders couldn't figure
    # out why it leaked so much memory.
    ADD_EXECUTABLE(sdmpp
        sdm/sdmpp.cpp
            sdm/sdmpp.h

        ${PCH_INCLUDE_HEADERS}
        sdm/sdmConfigVars.h
    )
    TARGET_LINK_LIBRARIES(sdmpp libwwii teulserver ${MYSQL_LIBRARIES})
    SET_PROPERTY(TARGET sdmpp PROPERTY FOLDER "Tools")

    # Analyze SDM files

    ADD_EXECUTABLE(sdmDump
        sdm/sdmDump.cpp
        ${PCH_INCLUDE_HEADERS}
    )
    TARGET_LINK_LIBRARIES(sdmDump libwwii teulserver ${MYSQL_LIBRARIES})
    SET_PROPERTY(TARGET sdmDump PROPERTY FOLDER "Tools")

    # Reads SDM files and imports portions of them into the game database.
    # Which is a shame, because the hosts wind up reading the SDM files too
    # to get the missing data. *Sigh*
    ADD_EXECUTABLE(stratLoader
        src/loader.cpp
        sdm/sdmLoad.cpp
            sdm/sdmLoad.h
            sdm/sdmParse.h
        ${PCH_INCLUDE_HEADERS}
    )
    TARGET_LINK_LIBRARIES(stratLoader libwwii teulserver ircclient ${MYSQL_LIBRARIES})
    SET_PROPERTY(TARGET stratLoader PROPERTY FOLDER "Tools")

    # Reads SDM and blitzWor data through the server's strat routines,
    # without accessing the database, to validate a given terrain for
    # its ability to run on the host.
    # This tool is used by whomever edits the terrain by logging into
    # the "fedex" box as "terrain" and running the "validate" script.
    ADD_EXECUTABLE(stratValidator
        parse/parseWor.cpp
            parse/parseWor.h
                parse/parseIO.h
                strt/strtWOR.h
                    parse/parse.h
                    strt/strtDTL.h
        strt/strtCPStruct.cpp
            strt/strtCPStruct.h
                strt/strtProtos.h
                wwii/wwiiHCUnits.h
                strt/strtCPFlags.h
                inst/instDescriptor.h
                    mm/mmEnums.h
        strt/strtData.cpp
            strt/strtData.h
        strt/strtFacilityStruct.cpp
            strt/strtFacilityStruct.h
        strt/strtObjectStruct.cpp
            strt/strtObjectStruct.h
        strt/strtIO.cpp
            strt/strtIO.h
        strt/strtValidator.cpp
        ${PCH_INCLUDE_HEADERS}
    )
    ADD_PCH(stratValidator)
    SET_PROPERTY(TARGET stratValidator PROPERTY COMPILE_DEFINITIONS CHECK_SUPPLY=1)
    TARGET_LINK_LIBRARIES(stratValidator libwwii teulserver event ${MYSQL_LIBRARIES})
    SET_PROPERTY(TARGET stratValidator PROPERTY FOLDER "Tools")
ENDIF( )

######################################################################
#### Server Processes

# Authentication Server(auth host)
ADD_EXECUTABLE(${AUTH_HOST_EXE}
    auth/authClientData.cpp
        auth/authClientData.h
    auth/authDB.cpp
        auth/authDB.h
        auth/authDatabaseSchemas.h
    auth/authHost.cpp
        auth/authHost.h
        auth/authGame.h
    auth/authHostMessages.cpp
        auth/authHostMessages.h
        auth/authQueuePosition.h
    auth/authOTP.cpp
        auth/authOTP.h
    auth/authQueue.cpp
        auth/authQueue.h
        wwii/wwiiRunningAverage.h
    src/infoHostMessages.cpp
        include/infoHostMessages.h
    src/infoPack.cpp
        include/infoPack.h
    auth/authPack.cpp
        auth/authPack.h
    dbse/dbseDatabaseSchema.cpp
        dbse/dbseDatabaseSchema.h
    ${PCH_INCLUDE_HEADERS}
)
ADD_PCH(${AUTH_HOST_EXE})
TARGET_LINK_LIBRARIES(${AUTH_HOST_EXE} teulserver libwwii ${MYSQL_LIBRARIES} ircclient)
SET_PROPERTY(TARGET ${AUTH_HOST_EXE} PROPERTY FOLDER "Hosts")

IF( NOT PN_CLUSTER STREQUAL "Auth" )
    ADD_EXECUTABLE(${CONTROLLER_HOST_EXE}
        src/controller.cpp
        src/controllerGameProcess.cpp
            src/controllerGameProcess.h
    )
    #ADD_PCH(${CONTROLLER_HOST_EXE})
    SET_PROPERTY(TARGET ${CONTROLLER_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_CONTROLLER_HOST)
    TARGET_LINK_LIBRARIES(${CONTROLLER_HOST_EXE} libwwii teulserver ${MYSQL_LIBRARIES})
    SET_PROPERTY(TARGET ${CONTROLLER_HOST_EXE} PROPERTY FOLDER "Hosts")

    # Primarily communications-oriented server, the chat host.
    # Sadly it does an awful lot of the "map/mission" server's job
    # because the original design disconnected players from
    # map/mission when they spawned.
    ADD_EXECUTABLE(${CHAT_HOST_EXE}
        chat/chatBanPlayer.cpp
            chat/chatBanPlayer.h
        chat/chatChannelList.cpp
            chat/chatChannelList.h
        chat/chatChannelNumber.cpp
            chat/chatChannelNumber.h
                chat/chatDefines.h
        chat/chatDB.cpp
            chat/chatDB.h
        chat/chatDotBetaCommands.cpp
        chat/chatDotCommand.cpp
            chat/chatDotCommand.h
                chat/chatClientMessage.h
                chat/chatGroupsType.h
        chat/chatDotCommands.cpp
        chat/chatDotGMCommands.cpp
        chat/chatEditString.cpp
            chat/chatEditString.h
        chat/chatFriendList.cpp
            chat/chatFriendList.h
        chat/chatFriendLogin.cpp
        chat/chatGenericString.cpp
            chat/chatGenericString.h
                chat/chatEditString.h
        chat/chatGrid.cpp
            chat/chatGrid.h
        chat/chatGroupPermissions.cpp
            chat/chatGroupPermissions.h
        chat/chatHost.cpp
            chat/chatHost.h
                chat/chatChannelTypes.h
                chat/chatPrototypes.h
                chat/chatDebugConditions.h
            chat/chatData.h
            chat/chatBroadcast.h
            chat/chatBroadcastHCPacket.h
            chat/chatBroadcastObject.h
        chat/chatHostMessages.cpp
            chat/chatHostMessages.h
                chat/chatChannelID.h
        chat/chatList.cpp
            chat/chatList.h
                host/hostPlayers.h
                chat/chatHostPlayerIndex.h
        chat/chatLocFile.cpp
            chat/chatLocFile.h
        chat/chatManagementHostMessages.cpp
            chat/chatManagementHostMessages.h
        chat/chatManagementIDs.cpp
            chat/chatManagementIDs.h
        chat/chatManagementPack.cpp
        chat/chatManagementPlayer.cpp
            chat/chatManagementPlayer.h
                chat/chatIRCPlayer.h
                chat/chatPlayer.h
                    auth/authDispatch.h
                    mm/mmEnums.h
                    chat/chatChannels.h
                    wwii/wwiiPlayer.h
                        chat/chatServerTypes.h
                            chat/chatPlayerInfo.h
                        inst/instDescriptor.h
                        miss/missFlags.h
        chat/chatPlayer.cpp
            chat/chatPlayer.h
        chat/chatServerTrackedObjects.cpp
            chat/chatServerTrackedObjects.h
                chat/chatBailSTO.h
                chat/chatHulkSTO.h
                chat/chatOrdnanceSTO.h
                chat/chatPPOSTO.h
                chat/chatServerSTO.h
                chat/chatSTODamage.h
                chat/chatSTOJoules.h
                chat/chatUMSSTO.h
                chat/chatVehicleSTO.h
        chat/chatSpamCan.cpp
            chat/chatSpamCan.h
        chat/chatStrings.cpp
        chat/chatThreads.cpp
            chat/chatThreads.h
        chat/chatTimeString.cpp
            chat/chatTimeString.h
        chat/chatTrainers.cpp
            chat/chatTrainers.h
        chat/chatUnitMotds.cpp
            chat/chatUnitMotds.h
        chat/chatWaypoints.cpp
            chat/chatWaypoints.h
        chat/chatWho.cpp
            chat/chatWho.h
        chat/chatWhos.cpp
            chat/chatWhos.h
        wwii/wwiiServerTrackedObject.cpp
            wwii/wwiiServerTrackedObject.h
        wwii/wwiiServerTrackedObjectPacker.cpp
            wwii/wwiiServerTrackedObjectPacker.h
                wwii/wwiiServerTrackedObject.h

        chat/chatBrigadeControl.cpp
            chat/chatBrigadeControl.h
        chat/chatBrigadeMove.cpp
            chat/chatBrigadeMove.h
                chat/chatBrigadeMoveResults.h
                teul/teulNetworkMsg.h
                    wwii/wwiiNetPorts.h
        chat/chatdbClientMessages.cpp
            chat/chatdbClientMessages.h
        chat/chatDecisions.cpp
            chat/chatDecisions.h
        chat/chatDotHCCommands.cpp
        chat/chatDotPassthrus.cpp
        chat/chatDotSquadCommands.cpp
        chat/chatHostBrigadeControl.cpp
            chat/chatHostBrigadeControl.h
            chat/chatBrigadeVote.h
        chat/chatHostMotds.cpp
            chat/chatHostMotds.h
        chat/chatMissionInvite.cpp
            chat/chatMissionInvite.h
        chat/chatMissionMember.cpp
            chat/chatMissionMember.h
        chat/chatMissions.cpp
            chat/chatMissions.h
            omg/omgChat.hxx
                chat/chatConstants.h
        chat/chatMotds.cpp
            chat/chatMotds.h
        chat/chatMultiCrew.cpp
            chat/chatMultiCrew.h
                teul/teulNetworkArray.h
                    wwii/wwiiPack.h
        chat/chatReports.cpp
            chat/chatReports.h
                teul/teulNetworkObject.h
        chat/chatSaveChannels.cpp
            chat/chatSaveChannels.h
        chat/chatSay.cpp
            chat/chatSay.h
        chat/chatSquadControl.cpp
            chat/chatSquadControl.h
                chat/chatSquadControlResults.h
        chat/chatSquadControls.cpp
        chat/chatSquadCreate.cpp
            chat/chatSquadCreate.h
        chat/chatSquadList.cpp
            chat/chatSquadList.h
        chat/chatSquadListPurge.cpp
            chat/chatSquadListPurge.h
        chat/chatSquadVote.cpp
            chat/chatSquadVote.h
        map/mapAWS.cpp
            map/mapAWS.h
        map/mapHCUnitLocations.cpp
            map/mapHCUnitLocations.h
        strt/strtChatTypes.cpp
            strt/strtChatTypes.h
        wwii/wwiiHCUnits.cpp
            wwii/wwiiHCUnits.h
        wwii/wwiiSquads.cpp
            wwii/wwiiSquads.h

        dbse/dbseClient.cpp
            dbse/dbseClient.h
                dbse/dbseTypes.h
        dbse/dbseClientMessages.cpp
            dbse/dbseClientMessages.h
            dbse/dbseConstants.h
            teul/teulBasic.h
            wwii/wwiiCallbacks.h
            dbse/dbseClient.h

        omg/omgChat.cxx

        ${PCH_INCLUDE_HEADERS}
    )
    ADD_PCH(${CHAT_HOST_EXE})
    SET_PROPERTY(TARGET ${CHAT_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_CHAT_HOST)
    TARGET_LINK_LIBRARIES(${CHAT_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient libb64 libxml2)
    SET_PROPERTY(TARGET ${CHAT_HOST_EXE} PROPERTY FOLDER "Hosts")

    ######################################################################
    # Game World Server aka Cell Host.
    # Only server of which multiple instances are intended to operate
    # in a single cluster. The work is distributed based on load, not
    # on any clever spatial distribution.
    # Currently, when a player spawns, they are actually transferred
    #(teul redirected) from the map host to this host. When they
    # despawn, they are redirected back to the map host.
    # Unfortunately, redirection is crudely implemented, and the player
    # actually /leaves/ the map host preventing the map host in dealing
    # with map/mission work for any spawned players. *SIGH*
    ADD_EXECUTABLE(${CELL_HOST_EXE}
        chat/chatCellHostSupportMessages.cpp
            chat/chatCellHostSupportMessages.h
        chat/chatSupportRPCMessages.cpp
            chat/chatSupportRPCMessages.h
        comm/commHostMessages.cpp
            comm/commHostMessages.h
                comm/commTypes.h
                    updt/updtEnums.h
                    wwii/wwiiExitCodes.h
                    weap/weapSerial.h
            updt/updtInfDeath.h
        comm/commPackers.cpp
            comm/commPackers.h
        host/host.cpp
            host/host.h
                miss/missionPacket.h
                host/hostPlayers.h
                    mm/mmEnums.h
                    wwii/wwiiRank.h
                    omg/omgMissionSortieData.hxx
                    miss/missFlags.h
                        wwii/wwiiSyncVar.h
                        teul/teulNetTimeMS.h
                        weap/weapSpecPacker.h
                        wwii/wwiiFullObjIDPacker.h
                            teul/teulNetTimeMSPacker.h
            host/hostPlayer.h
        host/hostFraming.cpp
            host/hostFraming.h
        host/hostGrid.cpp
        host/hostScoringActor.cpp
        host/hostVehicle.cpp
            host/hostVehicle.h
                inst/instDescriptor.h
                host/host.h
                updt/updtTypes.h
                    updt/updtTurrets.h
                    strt/strtTypes.h
                    updt/updtFraming.h
                    updt/updtVehiclePosition.h
                        include/xyz_tuple.h
        src/installCallbacks.cpp
        miss/missionPack.cpp
            miss/missionPack.h
        mm/mmCellHostSupportMessages.cpp
            mm/mmCellHostSupportMessages.h
        mm/mmPack.cpp
            mm/mmPack.h
        updt/updtPackers.cpp
            updt/updtPackers.h
        wwii/wwiiHostMessages.cpp
            wwii/wwiiHostMessages.h

        host/hostAutomation.cpp
            host/hostAutomation.h
        host/hostCellNet.cpp
            host/hostCellNet.h
        host/hostGridCell.cpp
            host/hostGridCell.h
        miss/missionPacket.cpp
            miss/missionPacket.h
        parse/parseWor.cpp
            parse/parseWor.h
                parse/parseIO.h
                strt/strtWOR.h
                    parse/parse.h
                    strt/strtDTL.h
        strt/strtCPColliders.cpp
            strt/strtCPColliders.h
        strt/strtCPStruct.cpp
            strt/strtCPStruct.h
                strt/strtProtos.h
                wwii/wwiiHCUnits.h
                strt/strtCPFlags.h
        strt/strtData.cpp
            strt/strtData.h
        strt/strtFacilityStruct.cpp
            strt/strtFacilityStruct.h
        strt/strtHack.cpp
            strt/strtHack.h
        strt/strtIO.cpp
            strt/strtIO.h
        strt/strtObjectStruct.cpp
            strt/strtObjectStruct.h
        strt/strtPack.cpp
            strt/strtPack.h
        strt/strtSyncData.cpp
            strt/strtSyncData.h
                strt/strtConstants.h
        strt/strtSyncSupportMessages.cpp
            strt/strtSyncSupportMessages.h
        strt/strtUpdater.cpp
            strt/strtUpdater.h
        strt/strtUpdaterHostMessages.cpp
            strt/strtUpdaterHostMessages.h
        strt/strtUpdaterPack.cpp
            strt/strtUpdaterPack.h
        strt/strtUpdaterSupportMessages.cpp
            strt/strtUpdaterSupportMessages.h

        omg/omgMissionSortieData.cxx

        ${PCH_INCLUDE_HEADERS}
    )
    ADD_PCH(${CELL_HOST_EXE})
    SET_PROPERTY(TARGET ${CELL_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_CELL_HOST)
    TARGET_LINK_LIBRARIES(${CELL_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient)
    SET_PROPERTY(TARGET ${CELL_HOST_EXE} PROPERTY FOLDER "Hosts")

    ######################################################################
    # Map/Mission Host, probably better described as the "Game Login Server".
    # This is the host to which players are forwarded from auth/playgate.
    # See the description of the cell host above for details on host spawning
    # works and why the chat host has wound up doing so much of map host's job.
    ADD_EXECUTABLE(${MAP_HOST_EXE}
        chat/chatGroupPermissions.cpp
            chat/chatGroupPermissions.h
                chat/chatGroupsType.h
        chat/chatSupportMessages.cpp
            chat/chatSupportMessages.h
        chat/chatSupportRPCMessages.cpp
            chat/chatSupportRPCMessages.h
        inst/instHostMessages.cpp
            inst/instHostMessages.h
        map/mapArenaDescription.cpp
            map/mapArenaDescription.h
        map/mapChecksums.cpp
            map/mapChecksums.h
        map/mapDB.cpp
            map/mapDB.h
            auth/authPlayerDetails.h
        map/mapHost.cpp
        map/mapHostMessages.cpp
            map/mapHostMessages.h
        map/mapMinuteBalance.cpp
            map/mapMinuteBalance.h
        map/mapPack.cpp
        map/mapPlayer.cpp
            map/mapPlayer.h
                miss/missionTypes.h
                miss/missionPacket.h
                map/mapPlayerTypes.h
                wwii/wwiiPlayer.h
                    mm/mmEnums.h
                    wwii/wwiiRank.h
                    weap/weapSerial.h
                    omg/omgMissionSortieData.hxx
                    miss/missFlags.h
                        wwii/wwiiSyncVar.h
                        teul/teulNetTimeMS.h
                        weap/weapSpecPacker.h
                        wwii/wwiiFullObjIDPacker.h
                            teul/teulNetTimeMSPacker.h
                    inst/instDescriptor.h
                    auth/authDispatch.h
        omg/omgMissionSortieData.cxx
        map/mapThreads.cpp
            map/mapThreads.h
        miss/missActiveMission.cpp
            miss/missActiveMission.h
        miss/mission.cpp
            miss/mission.h
        miss/missionDetail.cpp
            miss/missionDetail.h
                map/mapMissionList.h
                teul/teulNetworkMsg.h
                    miss/missActiveMission.h
                    wwii/wwiiNetPorts.h
        miss/missionHostMessages.cpp
            miss/missionHostMessages.h
        miss/missionPack.cpp
            miss/missionPack.h
        miss/missMobileSpawn.cpp
            miss/missMobileSpawn.h
                include/mobileSpawnEnums.h
        mm/mmAntiAddiction.cpp
            mm/mmAntiAddiction.h
            wwii/wwiiAntiAddiction.h
        mm/mmHostInstanceImpl.cpp
            mm/mmHostInstanceImpl.h
                inst/instHostInstanceImpl.h
                    inst/instHostInstance.h
                        inst/instBasicStatus.h
                        host/hostPlayers.h
                            inst/instStateEnums.h
        mm/mmHostMessages.cpp
            mm/mmHostMessages.h
                mm/mmSpawnTypes.h
        mm/mmMain.cpp
            mm/mmMain.h
        mm/mmPack.cpp
            mm/mmPack.h
        mm/mmPolls.cpp
            mm/mmPolls.h
        mm/mmSpawn.cpp
            mm/mmSpawn.h
        mm/mmSpawnGrantResponse.cpp
            mm/mmSpawnGrantResponse.h
        score/scoreCallbacks.cpp
        score/scoreDeath.cpp
            score/scoreDeath.h
                score/scoreEvent.h
                    score/scoreActor.h
        score/scoreSortie.cpp
            score/scoreSortie.h
                wwii/wwiiRTBCodes.h
        wwii/wwiiHostMessages.cpp
            wwii/wwiiHostMessages.h
        wwii/wwiiOpinionPoll.cpp
            wwii/wwiiOpinionPoll.h
                teul/teulNetworkObject.h

        map/mapBrigades.cpp
            map/mapBrigades.h
                map/mapBrigadeSummary.h
                map/mapPersonaTypes.h
                    wwii/wwiiHCUnits.h
        map/mapData.cpp
            map/mapData.h
        map/mapHCUnitLocations.cpp
            map/mapHCUnitLocations.h
                teul/teulNetworkArray.h
                    wwii/wwiiPack.h
        map/mapHostSupercellData.cpp
        miss/missionPacket.cpp
            miss/missionPacket.h
        miss/missionScoring.cpp
        miss/missionUtils.cpp
            miss/missionUtils.h
        mm/mmdbPersonaSelections.cpp
            mm/mmdbPersonaSelections.h
        parse/parseWor.cpp
            parse/parseWor.h
                parse/parseIO.h
                strt/strtWOR.h
                    parse/parse.h
                    strt/strtDTL.h
        strt/strtCPStruct.cpp
            strt/strtCPStruct.h
                strt/strtProtos.h
                strt/strtCPFlags.h
        strt/strtData.cpp
            strt/strtData.h
        strt/strtDB.cpp
            strt/strtDB.h
        strt/strtEvents.cpp
            strt/strtEvents.h
        strt/strtFacilityStruct.cpp
            strt/strtFacilityStruct.h
        strt/strtIO.cpp
            strt/strtIO.h
        strt/strtMapMessages.cpp
            strt/strtMapMessages.h
        strt/strtObjectStruct.cpp
            strt/strtObjectStruct.h
        strt/strtPack.cpp
            strt/strtPack.h
        strt/strtSyncData.cpp
            strt/strtSyncData.h
                strt/strtConstants.h
        strt/strtSyncSupportMessages.cpp
            strt/strtSyncSupportMessages.h
        wwii/wwiiHCUnits.cpp
            wwii/wwiiHCUnits.h

        ${PCH_INCLUDE_HEADERS}
    )
    ADD_PCH(${MAP_HOST_EXE})
    SET_PROPERTY(TARGET ${MAP_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_MAP_HOST)
    TARGET_LINK_LIBRARIES(${MAP_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient libxml2)
    SET_PROPERTY(TARGET ${MAP_HOST_EXE} PROPERTY FOLDER "Hosts")

    ######################################################################
    # Database Proxy Daemon [deprecated].
    # Because the original host implementation was
    # entirely single-threaded, this host process was created to offload
    # expensive synchronous database queries.
    # You should really use DB::Threads instead.
    ADD_EXECUTABLE(${DBD_HOST_EXE}
        chat/chatdbHostMessages.cpp
        chat/chatdbParser.cpp
            chat/chatdbParser.h
        dbse/dbseHost.cpp
        dbse/dbseHostMessages.cpp
            dbse/dbseConstants.h
            dbse/dbseTypes.h
            wwii/wwiiCallbacks.h
            teul/teulBasic.h
            server/serverConfigVars.h
        wwii/wwiiSquads.cpp
            wwii/wwiiSquads.h

        ${PCH_INCLUDE_HEADERS}
    )
    ADD_PCH(${DBD_HOST_EXE})
    SET_PROPERTY(TARGET ${DBD_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_DBD_HOST)
    TARGET_LINK_LIBRARIES(${DBD_HOST_EXE} libwwii teulserver instancing ${MYSQL_LIBRARIES} ircclient)
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
    ADD_EXECUTABLE(${STRAT_HOST_EXE}
        chat/chatStratSupportMessages.cpp
            chat/chatStratSupportMessages.h
                strt/strtChatTypes.h
        chat/chatSupportRPCMessages.cpp
            chat/chatSupportRPCMessages.h
        cnvy/cnvyConvoyList.cpp
            cnvy/cnvyConvoyList.h
        cnvy/cnvyMain.cpp
            cnvy/cnvyMain.h
                strt/strtProtos.h
            strt/strtEventQueue.h
            strt/strtTypes.h
            strt/strtServerTypes.h
            mem/memPool.h
            teul/teulCLList.h
                list/listCL.h
            include/intrusiveList.h
        dbse/dbseDatabaseSchema.cpp
            dbse/dbseDatabaseSchema.h
            strt/strtDatabaseSchemas.h
        src/luaGlue.cpp
            include/luaGlue.h
        miss/missionPack.cpp
            miss/missionPack.h
        mm/mmPack.cpp
            mm/mmPack.h
        parse/parseWor.cpp
            parse/parseWor.h
                parse/parseIO.h
                strt/strtWOR.h
                    parse/parse.h
                    strt/strtDTL.h
        strt/strtChatTypes.cpp
            strt/strtChatTypes.h
        strt/strtCPClass.cpp
            strt/strtCPClass.h
        strt/strtCP.cpp
            strt/strtCP.h
        strt/strtCPStruct.cpp
            strt/strtCPStruct.h
                wwii/wwiiHCUnits.h
                strt/strtCPFlags.h
                inst/instDescriptor.h
                    mm/mmEnums.h
        strt/strtData.cpp
            strt/strtData.h
        strt/strtDB.cpp
            strt/strtDB.h
        strt/strtdbSaveWorld.cpp
            strt/strtdbSaveWorld.h
        strt/strtCaptureAction.cpp
            strt/strtCaptureAction.h
        strt/strtCaptureContribution.cpp
            strt/strtCaptureContribution.h
                strt/strtCaptureScore.h
        strt/strtDotCommands.cpp
            strt/strtDotCommands.h
        strt/strtEnemyReport.cpp
            strt/strtEnemyReport.h
        strt/strtFacilityStruct.cpp
            strt/strtFacilityStruct.h
        strt/strtFB.cpp
            strt/strtFB.h
        strt/strtHack.cpp
            strt/strtHack.h
        strt/strtHCUnits.cpp
            strt/strtHCUnits.h
                chat/chatBrigadeMoveResults.h
                strt/strtTOETypes.h
        strt/strtHost.cpp
        strt/strtHostDB.cpp
            strt/strtHostDB.h
        strt/strtHostMessages.cpp
            strt/strtHostMessages.h
                strt/strtMapTypes.h
                    mm/mmSpawnTypes.h
                    miss/missionTypes.h
                    wwii/wwiiRTBCodes.h
                        weap/weapSerial.h
        strt/strtIO.cpp
            strt/strtIO.h
        strt/strtJoinRequestList.cpp
        strt/strtHostLua.cpp
            strt/strtHostLua.h
        strt/strtMain.cpp
            strt/strtMain.h
        strt/strtMission.cpp
            strt/strtMission.h
        strt/strtObjectStruct.cpp
            strt/strtObjectStruct.h
        strt/strtPack.cpp
            strt/strtPack.h
        strt/strtPostedWeaponList.cpp
            strt/strtPostedWeaponList.h
                strt/strtJoinList.h
        strt/strtSpawn.cpp
            strt/strtSpawn.h
            strt/strtWeaponsReturn.h
        strt/strtSpawnWeaponList.cpp
        strt/strtSupply.cpp
            strt/strtSupply.h
        strt/strtSync.cpp
            strt/strtSync.h
        strt/strtSyncData.cpp
            strt/strtSyncData.h
                strt/strtConstants.h
        strt/strtTOE.cpp
            strt/strtTOE.h
        strt/strtWeaponEntry.cpp
            strt/strtWeaponEntry.h
                strt/strtTypes.h
                strt/strtObjectStruct.h
        weap/weapSpawnXML.cpp
            weap/weapSpawnXML.h
        wwii/wwiiHCUnits.cpp
            wwii/wwiiHCUnits.h
        list/listCL.cpp
            list/listCL.h
                list/list.h

        ${_LUA_HOST}Lua.cc

        ${PCH_INCLUDE_HEADERS}
    )
    ADD_PCH(${STRAT_HOST_EXE})
    SET_PROPERTY(TARGET ${STRAT_HOST_EXE} PROPERTY COMPILE_DEFINITIONS AS_SERVER IS_STRAT_HOST LUA_MODULE_OPEN=tolua_stratd_open CHECK_SUPPLY=1)
    SET_PROPERTY(TARGET ${_LUA_HOST} PROPERTY DEPENDS ${_LUA_HOST}Lua.cc)
    IF( NOT MSVC )
        SET_PROPERTY(SOURCE ${_LUA_HOST}Lua.cc PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS} -Wno-unused -Wno-unused-function")
    ELSE( )
        SET_PROPERTY(SOURCE ${_LUA_HOST}Lua.cc PROPERTY COMPILE_FLAGS /W0)
        SET_PROPERTY(SOURCE ${_LUA_HOST}Lua.cc PROPERTY COMPILE_FLAGS "/wd4480 /wd4800 /wd4305 /wd4505")
    ENDIF( )
    SET_PROPERTY(SOURCE ${_LUA_HOST}Lua.cc PROPERTY DEPENDS ${TOLUAPP_EXECUTABLE} ${TOLUAPP_LIBRARIES})
    SET_PROPERTY(SOURCE ${_LUA_HOST}Lua.cc PROPERTY GENERATED true)
    TARGET_LINK_LIBRARIES(${STRAT_HOST_EXE} libwwii ${MYSQL_LIBRARIES} teulserver ircclient ${LUA_LIBRARIES} ${TOLUAPP_LIBRARIES} libxml2)
    SET_PROPERTY(TARGET ${STRAT_HOST_EXE} PROPERTY FOLDER "Hosts")

    TARGET_INCLUDE_DIRECTORIES(${_LUA_HOST} PRIVATE ${TOLUAPP_INCLUDE_DIR})
ENDIF( )

######################################################################
#### Test Module(s)

#ADD_EXECUTABLE(testAuthConnect
#    tests/testAuthConnect.cpp
#
#    ${PCH_INCLUDE_HEADERS}
#)
#TARGET_LINK_LIBRARIES(testAuthConnect libwwii teulserver)
#SET_PROPERTY(TARGET testAuthConnect PROPERTY FOLDER "Test")
#
#IF( NOT PN_CLUSTER STREQUAL "Auth")
    # Builds a single executable that runs all of the unit tests;
    # The install process will not execute unless all tests complete.
#    ADD_EXECUTABLE(testUnits
#        tests/testUnits.cpp
#        tests/testUnits.h
#        tests/testTeulBits.cpp
#        tests/testAuthQueue.cpp auth/authClientData.cpp
#        tests/testChatHandle.cpp
#        tests/testFullObjID.cpp
#        tests/testCommandWithArgs.cpp
#        tests/testCovert.cpp
#        tests/testLocalizedMessage.cpp
#        tests/testInplaceVector.cpp
#        tests/testPackers.cpp   comm/commPackers.cpp
#        tests/testChatMissionStatus.cpp wwii/wwiiServerTrackedObjectPacker.cpp omg/omgChat.cxx
#        tests/testHostGridCell.cpp host/hostGridCell.cpp
#        tests/testSupercellVislevelXfer.cpp
#        tests/testPersonaXfer.cpp
#        tests/testCryptographer.cpp
#        tests/testXYZTuple.cpp
#        tests/testTeulPacket.cpp
#
#        ${PCH_INCLUDE_HEADERS}
#        tests/testSuite.h
#        tests/testUnits.h
#    )
#    ADD_PCH(testUnits)
#    SET_PROPERTY(TARGET testUnits PROPERTY COMPILE_DEFINITIONS ENABLE_TESTS)
#    SET_PROPERTY(TARGET testUnits PROPERTY FOLDER "Test")
#    TARGET_LINK_LIBRARIES(testUnits libwwii teulserver)
#
#    ADD_TEST(testUnits ${EXECUTABLE_OUTPUT_PATH}/testUnits)
#ENDIF( )

######################################################################
#### Module for testing teulKit functionality

#IF( NOT PN_CLUSTER STREQUAL "Auth")
#    ADD_EXECUTABLE(testTeul
#        tests/testTeul.cpp
#        
#        ${PCH_INCLUDE_HEADERS}
#        teul/teulExceptions.h
#        teul/teulKit.h
#        teul/teulEndpoint.h
#        teul/teulEndpointImpl.h
#        teul/teulPacket.h
#        teul/teulPacketHeader.h
#        wwii/wwiiCallbacks.h
#        tests/testTeulTest.h
#        tests/testTeulClient.h
#        tests/testTeulServer.h
#        teul/teulVersion.h
#        teul/teulConnectionInstruction.h
#    )
#    TARGET_LINK_LIBRARIES(testTeul teulserver libwwii)
#    SET_PROPERTY(TARGET testTeul PROPERTY FOLDER "Test")
#ENDIF( )

######################################################################
#### Build files.

ADD_CUSTOM_TARGET(envconfig
    COMMAND ${CMAKE_COMMAND} -E
        copy ${PROJECT_DIR}/environment.cfg ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E
        copy ${PROJECT_DIR}/environment.win.cfg ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
        ${PROJECT_DIR}/environment.cfg
        ${PROJECT_DIR}/environment.win.cfg
    )
SET_TARGET_PROPERTIES(envconfig PROPERTIES FOLDER "Tools")

######################################################################
#### Install rule.

###TODO: Should really be dependent on testUnits as described above. Duh.
IF( NOT PN_CLUSTER STREQUAL "Auth")
    IF( NOT WIN32 )
        INSTALL(TARGETS chatClient luaClient
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            COMPONENT cmdclients
        )
    ENDIF( )
    INSTALL(TARGETS checkGameName stratLoader sdmpp stratValidator
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            COMPONENT tools
    )
    INSTALL(TARGETS ${CONTROLLER_HOST_EXE} ${STRAT_HOST_EXE} ${DBD_HOST_EXE}
                    ${CHAT_HOST_EXE} ${MAP_HOST_EXE} ${CELL_HOST_EXE}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            COMPONENT game
    )
ENDIF( )
INSTALL(TARGETS
            ${AUTH_HOST_EXE}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT auth
)
INSTALL(FILES
            environment.cfg
            environment.win.cfg
        DESTINATION bin
        COMPONENT config
        )
IF( NOT PN_CLUSTER STREQUAL "Auth")
    INSTALL(DIRECTORY hostproj/lua/ DESTINATION lua COMPONENT lua PATTERN ".svn" EXCLUDE PATTERN ".lua")
    INSTALL(DIRECTORY hostproj/web/ DESTINATION web COMPONENT web PATTERN ".svn" EXCLUDE)
ENDIF( )

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
