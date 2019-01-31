####################################
## Declare CMake version requirement
CMAKE_MINIMUM_REQUIRED(VERSION 3.12.0)

####################################
## Simplify environment
IF( WIN32 OR APPLE )
	SET( LINUX OFF )
ELSEIF( UNIX )
	SET( LINUX ON )
ELSE()
	MESSAGE( FATAL_ERROR "Detected neither WIN32, APPLE, nor UNIX. What system is this?" )
ENDIF()

####################################
## Initial CMake config
LIST( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmakeincludes" )

SET( CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo )
IF ( NOT CMAKE_BUILD_TYPE AND CMAKE_GENERATOR MATCHES "Ninja|Makefile" )
	MESSAGE( FATAL_ERROR "CMAKE_BUILD_TYPE is required with ${CMAKE_GENERATOR}. Specify one of: ${CMAKE_CONFIGURATION_TYPES}" )
ELSEIF( CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST CMAKE_CONFIGURATION_TYPES )
	MESSAGE( FATAL_ERROR "Unrecognized CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}. Expected one of: ${CMAKE_CONFIGURATION_TYPES}" )
ENDIF()

####################################
## Declare the project (don't specify version here)
PROJECT( WW2Hosts
	DESCRIPTION "Hosts and utilities for World War II Online"
	HOMEPAGE_URL "https://wwiionline.com"
	LANGUAGES C CXX
)

####################################
## Require C++14
SET( CMAKE_CXX_STANDARD_REQUIRED ON )
SET( CMAKE_CXX_STANDARD 14 )

####################################
# RakNet
# Ridiculously, this has to be all the way up here, otherwise, RakNet will pick
# up a bunch of build settings not intended for it, which screws up the stupid
# thing a surprising amount. Oh wait. That's only on UNIX/macOS. On Windows this
# actually makes RakNet unable to link, because Windows is an infantile OS in 2018.
IF( NOT WIN32 )
	ADD_SUBDIRECTORY(../../Libraries/RakNet ./RakNet)
	SET(RAKNET_INCLUDE_DIRS "${RakNet_SOURCE_DIR}/Source;${RakNet_SOURCE_DIR}/DependentExtensions;${RakNet_SOURCE_DIR}/DependentExtensions/cat/include")
	INCLUDE_DIRECTORIES( SYSTEM ${RAKNET_INCLUDE_DIRS} )
ENDIF( NOT WIN32 )

####################################
## Determine bitness

IF ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	SET( CMAKE_BITNESS 32 )
ELSEIF ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	SET( CMAKE_BITNESS 64 )
ELSE ()
	MESSAGE( FATAL_ERROR "Unknown CMAKE_SIZEOF_VOID_P value ${CMAKE_SIZEOF_VOID_P}" )
ENDIF ()
MARK_AS_ADVANCED( CMAKE_BITNESS )

OPTION( PN_BUILD_32BIT "Enable 32-bit build" OFF )
IF ( WIN32 OR CMAKE_BITNESS EQUAL 32 )
	MARK_AS_ADVANCED( PN_BUILD_32BIT )
ENDIF()

IF ( CMAKE_BITNESS EQUAL 64 AND NOT PN_BUILD_32BIT )
	SET( BUILD_BITNESS 64 )
ELSE ( )
	SET( BUILD_BITNESS 32 )
ENDIF ( )
MARK_AS_ADVANCED( BUILD_BITNESS )

IF ( APPLE AND NOT BUILD_BITNESS EQUAL 64 )
	MESSAGE( FATAL_ERROR "It is no longer possible to build 32-bit hosts on macOS" )
ELSEIF ( LINUX AND NOT CMAKE_BITNESS EQUAL BUILD_BITNESS )
	MESSAGE( WARNING "You are building ${BUILD_BITNESS}-bit on a ${CMAKE_BITNESS}-bit machine. This is probably a bad idea." )
ELSEIF ( WIN32 AND NOT BUILD_BITNESS EQUAL 32 )
	MESSAGE( FATAL_ERROR
		"The 64-bit host build is temporarily unsupported on Win32 due to problems with libxml2 prebuilts." )
ENDIF()

MESSAGE( STATUS "CMake is ${CMAKE_BITNESS}-bit, build is ${BUILD_BITNESS}-bit" )

####################################
## Executable names
SET( AUTH_HOST_EXE "authHost" )
SET( CONTROLLER_HOST_EXE "controller" )
SET( CHAT_HOST_EXE "chatd" )
SET( CELL_HOST_EXE "hostd" )
SET( MAP_HOST_EXE "mmhostd" )
SET( STRAT_HOST_EXE "stratd" )

####################################
## Embedded library search path
SET( PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
SET( LIBRARIES_DIR "${PROJECT_DIR}/Libraries" )

####################################
## General search paths

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/hostproj
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/mem
	${CMAKE_CURRENT_SOURCE_DIR}/sys
    ${CMAKE_CURRENT_BINARY_DIR}
)

####################################
## Determine installation directory
# This is the wrong way around but it has to be :(
# CMAKE_INSTALL_PREFIX has an automatic-default value from CMake so we can't do a
# soft-override or supply our own default (uhm, duh, cmake guys).
SET( PN_PLAYNET_PATH "/playnet" CACHE PATH "Default top-level runtime and install path." )
SET( CMAKE_INSTALL_PREFIX "${PN_PLAYNET_PATH}" CACHE PATH "Installation path (auto-set to PN_PLAYNET_PATH)" FORCE )
FILE( TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}" _PLAYNET )

####################################
## Define and validate user-specifiable options.

# Enable per-host ports for running clusters on less than 4 hosts
OPTION( PN_DIFFERENT_PORTS	"Use different public-facing ports for game servers." ON )

# Disable building and use of auth host (for cluster-on-a-box/controller testing)
OPTION( PN_DISABLE_AUTH     "Build without use of/support for auth" OFF )

# Disable the build info generator (speeds up building during development)
# Only valid in debug builds
IF ( CMAKE_BUILD_TYPE MATCHES Debug )
	OPTION( PN_NO_BUILD_INFO    "Disable the build_info.cpp generator (Debug only)" OFF )
ELSE()
	SET( PN_NO_BUILD_INFO OFF CACHE BOOL "Disables generating build_info.cpp" FORCE )
ENDIF()

# If the user is building a Debug build, enable SANITIZE options.
IF ( NOT WIN32 )
	IF ( CMAKE_BUILD_TYPE MATCHES Debug )
		OPTION( PN_SANITIZE "Enable -fsanitize=address, -fsanitize=undefined and -fstack-protector-all" OFF )
	ELSE()
		SET( PN_SANITIZE OFF CACHE BOOL "Enables Address Sanitizer, etc. on Unix builds" FORCE )
		MARK_AS_ADVANCED( PN_SANITIZE )
	ENDIF()
ENDIF()

IF ( CMAKE_BUILD_TYPE MATCHES Debug )
    OPTION( PN_DEV_BUILD "Enable funny farm and more logging" OFF )
ELSE()
    SET( PN_DEV_BUILD OFF )
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

ADD_DEFINITIONS( "-DplatBuild=BUILD_SERVER" )

####################################
## Provide critical build type definitions
# For now, we always build PUBLIC_RELEASE
ADD_DEFINITIONS( "-DPUBLIC_RELEASE" )

IF ( PN_DEV_BUILD )
    ADD_DEFINITIONS( "-DDEV_BUILD" )
ENDIF()

IF ( PN_BETA_RELEASE )
    ADD_DEFINITIONS( "-DBETA_RELEASE" )
ENDIF()

####################################
## Generate config.h
# TODO THIS NEEDS TO BE MOVED DOWN AND APPLY ITSELF TO THE MAIN HOSTS CONFIG TARGET
INCLUDE( cmakeincludes/PlatformChecks.cmake )

####################################
## Set up the various compiler definitions for things
INCLUDE( cmakeincludes/HostConfiguration.cmake )

####################################
## Check various dependencies.
INCLUDE( cmakeincludes/Dependencies.cmake )

####################################
# RakNet
# Ridiculously, this has to be all the way down here, otherwise, RakNet will miss
# a bunch of build settings required for it to build correctly. Oh wait. That's
# only on Windows. On UNIX/macOS this makes RakNet pick up a bunch of garbage
# build setitings, because Windows is an infantile OS in 2018.
IF( WIN32 )
	ADD_SUBDIRECTORY(../../Libraries/RakNet ./RakNet)
	SET(RAKNET_INCLUDE_DIRS "${RakNet_SOURCE_DIR}/Source;${RakNet_SOURCE_DIR}/DependentExtensions;${RakNet_SOURCE_DIR}/DependentExtensions/cat/include")
	INCLUDE_DIRECTORIES( SYSTEM ${RAKNET_INCLUDE_DIRS} )
ENDIF( WIN32 )

####################################
## Run build_info.py to get build_info.cpp
# Use BYPRODUCTS for the real result and a symbolic (fake) OUTPUT so the script
# runs always; easiest way to avoid the run-when-needed default behaviors.
IF ( PN_NO_BUILD_INFO )
	# If bypassing build info generation was requested, substitute hardcoded info instead
	SET( BUILD_INFO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/build_info.generic.cpp )
ELSE()
	ADD_CUSTOM_COMMAND(
		OUTPUT build_info.stamp
		BYPRODUCTS build_info.cpp
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cpp.in
		COMMAND
			${PYTHON_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/build_info.py
			host
			${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating build information"
		VERBATIM
	)
	SET_SOURCE_FILES_PROPERTIES( build_info.stamp PROPERTIES SYMBOLIC TRUE )
	SET( BUILD_INFO_FILES
		${CMAKE_CURRENT_BINARY_DIR}/build_info.stamp
		${CMAKE_CURRENT_BINARY_DIR}/build_info.cpp
	)
ENDIF()

####################################
# maclib library, provides the Base64 implementation on macOS for the hosts
# This is needed because the Base64 implementation uses Objective-C and that
# needs a separate target
IF ( APPLE )
	FIND_LIBRARY( FOUNDATION Foundation )
	ADD_LIBRARY(maclib STATIC sysmac/sysMacCompat.mm)
	TARGET_LINK_LIBRARIES( maclib ${FOUNDATION} )
	LINK_LIBRARIES( maclib ${FOUNDATION} )
ENDIF()

####################################
# teulserver library, provides teul and the "server" API, for what they're worth
ADD_LIBRARY(teulserver STATIC
	auth/authGameMessages.cpp auth/authGameMessages.h
	auth/authPack.cpp auth/authPack.h
	auth/authServerTypes.h
	auth/authTypes.h
	include/platforms.h
	include/pnThread.h
	include/precompile_includes.h
	mem/memPool.h
	server/server.cpp server/server.h
	server/serverMessages.cpp server/serverMessages.h
	server/serverPeers.cpp server/serverPeers.h
	server/serverReleaseTypeEnum.h
	server/serverTransfer.cpp server/serverTransfer.h
	str/stringLib.cpp str/stringLib.h
	teul/raknetConn.cpp teul/raknetConn.h
	teul/teulAuth.cpp teul/teulAuth.h
	teul/teulAuthServer.h
	teul/teulBasic.h
	teul/teulBits.h
	teul/teulBlipRepeater.h
	teul/teulCallback.cpp teul/teulCallback.h
	teul/teulCallback.h
	teul/teulCallbackInit.h
	teul/teulCallbackStruct.h
	teul/teulConnection.h
	teul/teulConnectionInstruction.h
	teul/teulConstants.h
	teul/teulEndpoint.cpp teul/teulEndpoint.h
	teul/teulEndpointDefs.h
	teul/teulEndpointImpl.h
	teul/teulExtErrors.h
	teul/teulFuncTypes.h
	teul/teulKit.h
	teul/teulMicroWebServer.cpp teul/teulMicroWebServer.h
	teul/teulModCalls.cpp teul/teulModCalls.h
	teul/teulModRecv.cpp
	teul/teulModSend.cpp
	teul/teulModule.cpp teul/teulModule.h
	teul/teulNetTimeMS.cpp teul/teulNetTimeMS.h
	teul/teulNetTimeMS.h
	teul/teulNetTimeMSPacker.h
	teul/teulNetworkArray.h
	teul/teulNetworkMsg.h
	teul/teulNetworkObject.h
	teul/teulPack.cpp teul/teulPackers.h
	teul/teulPacket.cpp teul/teulPacket.h
	teul/teulTickSample.h
	teul/teulTime.h
	teul/teulTimeMS.h
	teul/teulTransmission.cpp teul/teulTransmission.h
	teul/teulTransport.cpp
	teul/teulUtils.cpp teul/teulUtils.h
	teul/teulVersion.h
	updt/loginTypes.h
	wwii/wwiiCallbacks.cpp wwii/wwiiCallbacks.h
	wwii/wwiiConfig.cpp wwii/wwiiConfig.h
	wwii/wwiiConfigMap.h
)
TARGET_LINK_LIBRARIES( teulserver LibXml2::LibXml2 MySQL::MySQL ${LIBEVENT_LIBRARIES} ${RAKNET_LIBRARY_LIBS} RakNetLibStatic libcattunnel libcatcrypt libcatmath libcatcommon )
SET_PROPERTY( TARGET teulserver PROPERTY FOLDER "Libs" )

####################################
## instancing library for hosts that deal with instances (all but strat)
ADD_LIBRARY( instancing STATIC
	auth/authDispatch.h
	chat/chatDefines.h
	chat/chatLocalizedMessage.cpp chat/chatLocalizedMessage.h
	inst/instDescriptor.h
	inst/instHost.cpp inst/instHost.h
	inst/instPackers.cpp
	miss/missFlags.h
	mm/mmEnums.h
	wwii/wwiiBranchEnums.h
	wwii/wwiiPlayer.cpp wwii/wwiiPlayer.h
)
TARGET_LINK_LIBRARIES( instancing MySQL::MySQL )
SET_PROPERTY( TARGET instancing PROPERTY FOLDER "Libs" )

####################################
## wwii library common to everything, when it isn't broken by target definitions
ADD_LIBRARY( wwiilib STATIC
	${BUILD_INFO_FILES}
	chat/chatHandle.cpp chat/chatHandle.h
	chat/chatName.h
	chat/chatPackers.cpp
	chat/chatTimeInterval.cpp chat/chatTimeInterval.h
	dbug/dbug.cpp dbug/dbug.h
	fio/fiod.cpp fio/fiod.h
	host/hostCommand.h
	include/configVars.h
	include/dbseWrapper.h
	include/io_mapped_file.h
	include/mydbQuery.h
	include/wthrTime.h
	inst/instCommon.cpp
	inst/instPackers.cpp
	libircclient/src/libircclient.cpp
	parse/countryCodes.cpp parse/countryCodes.h
	parse/parse.cpp parse/parse.h
	parse/parseIO.cpp parse/parseIO.h
	score/scoreActor.cpp score/scoreActor.h
	sdm/sdmHash.cpp sdm/sdmHash.h
	sdm/sdmParse.cpp sdm/sdmParse.h
	src/configVars.cpp
	src/io_mapped_file.cpp
	src/mydbQuery.cpp
	src/myDbseWrapper.cpp
	src/wthrTime.cpp
	str/strdupptr.h
	strt/strtDataErrorInfo.h
	strt/strtDataErrors.cpp strt/strtDataErrors.h
	strt/strtDTL.h
	strt/strtSupercell.cpp strt/strtSupercell.h
	strt/strtSupercells.h
	teul/teulCallback.h
	teul/teulCallbackInit.h
	teul/teulConfigVar.h
	tut/tutGroupProgress.cpp tut/tutGroupProgress.h
	weap/weapList.cpp weap/weapList.h
	weap/weapLoadIndexes.cpp weap/weapLoadIndexes.h
	weap/weapParser.cpp weap/weapParser.h
	weap/weapSerial.h
	weap/weapSpec.cpp weap/weapSpec.h
	wwii/wwiiCallbacks.cpp wwii/wwiiCallbacks.h
	wwii/wwiiCommandLineArgs.cpp wwii/wwiiCommandLineArgs.h
	wwii/wwiiDBConfig.cpp wwii/wwiiDBConfig.h
	wwii/wwiiDBThreads.cpp wwii/wwiiDBThreads.h
	wwii/wwiiHCUnitsCommon.cpp
	wwii/wwiiIRC.cpp wwii/wwiiIRC.h
	wwii/wwiiNetPorts.h
	wwii/wwiiRank.cpp wwii/wwiiRank.h
	wwii/wwiiSerialNumber.cpp wwii/wwiiSerialNumber.h
	wwii/wwiiSortieManager.cpp wwii/wwiiSortieManager.h
	wwii/wwiiSyncVar.cpp wwii/wwiiSyncVar.h
	wwii/wwiiThreads.cpp wwii/wwiiThreads.h
	wwii/wwiiThreadText.cpp wwii/wwiiThreadText.h
	wwii/wwiiThreadTextQuery.h
	wwii/wwiiThreadTypes.h
	wwii/wwiiTypes.h
	wwii/wwiiUTF.cpp wwii/wwiiUTF.h

	# These weren't previously in wwiilib but seem to be required in various places
	auth/authDB.cpp auth/authDB.h
	chat/omgChat.cpp chat/omgChat.h
	chat/omgMissionSortieData.cpp chat/omgMissionSortieData.h
	include/infoHostMessages.h
	include/infoPack.h
	src/infoHostMessages.cpp
	src/infoPack.cpp
	weap/weapSpawnXML.cpp weap/weapSpawnXML.h
	wwii/wwiiPPOLoader.cpp wwii/wwiiPPOLoader.h
	xml/XMLLoader2.cpp xml/XMLLoader2.h
)
SET_PROPERTY( TARGET wwiilib PROPERTY FOLDER "Libs" )
TARGET_LINK_LIBRARIES( wwiilib LibXml2::LibXml2 MySQL::MySQL )
MACRO( USE_WWIILIB_ON Name )
	TARGET_LINK_LIBRARIES( ${Name} wwiilib LibXml2::LibXml2 MySQL::MySQL )
ENDMACRO()

####################################
## Various commandline utility executables
IF ( NOT WIN32 )

    # Chat management client
    ADD_EXECUTABLE( chatClient
		chat/chatLocFile.cpp chat/chatLocFile.h
		chat/chatManagementClientMessages.cpp chat/chatManagementClientMessages.h
		chat/chatManagementPack.cpp
		chat/chatRoster.cpp
		chat/chatRosterClient.cpp
    )
	USE_WWIILIB_ON( chatClient )
    TARGET_LINK_LIBRARIES( chatClient teulserver ${LIBZ_LIBRARIES} )
    SET_PROPERTY( TARGET chatClient PROPERTY FOLDER "Tools" )

    INSTALL(
        TARGETS chatClient #luaClient
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT cmdclients
    )

ENDIF()

IF ( WIN32 )
	# kfsone: I'm currently using these because they're helpful for cluster-in-a-box,

	# Not currently generated:
	#  checkGameName: Check callsign validity, not all that useful
	#  sdmpp: Prunes terrain files lacking strat data, pretty unhelpful currently
	#  sdmDump: Analyzes terrain files, not helpful without terrain work happening
	#  stratLoader: Partially loads terrain into the database, which hosts do anyway
	#  stratValidator: Uses server routines to validate SDM/WOR data, terrain terrain terrain, sigh
	# See git revision history for build commands.

	# In theory, SDM pre-parser. In practice? Deletes SDM files that don't
	# contain any strat data.
	# Not part of stratValidator because the original coders couldn't figure
	# out why it leaked so much memory.
	ADD_EXECUTABLE(sdmpp
		include/configVars.h
		sdm/sdmpp.cpp sdm/sdmpp.h
	)
	USE_WWIILIB_ON(sdmpp)
	TARGET_LINK_LIBRARIES(sdmpp teulserver)
	SET_PROPERTY(TARGET sdmpp PROPERTY FOLDER "Tools")
ENDIF()

	ADD_EXECUTABLE(stratValidator
		include/configVars.h
		inst/instDescriptor.h
		mm/mmEnums.h
		parse/parse.h
		parse/parseIO.h
		parse/parseWor.cpp parse/parseWor.h
		strt/strtCPFlags.h
		strt/strtCPStruct.cpp strt/strtCPStruct.h
		strt/strtData.cpp strt/strtData.h
		strt/strtDTL.h
		strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h
		strt/strtIO.cpp strt/strtIO.h
		strt/strtObjectStruct.cpp strt/strtObjectStruct.h
		strt/strtProtos.h
		strt/strtValidator.cpp
		strt/strtWOR.h
		teul/teulConfigVar.h
		wwii/wwiiHCUnits.h
		${PCH_INCLUDE_HEADERS}
	)
	SET_PROPERTY(TARGET stratValidator PROPERTY COMPILE_DEFINITIONS CHECK_SUPPLY=1)
	USE_WWIILIB_ON(stratValidator)
	TARGET_LINK_LIBRARIES(stratValidator teulserver)
	SET_PROPERTY(TARGET stratValidator PROPERTY FOLDER "Tools")

# Reads SDM files and imports portions of them into the game database.
# Which is a shame, because the hosts wind up reading the SDM files too
# to get the missing data. *Sigh*
ADD_EXECUTABLE(stratLoader
	sdm/sdmLoad.cpp sdm/sdmLoad.h
	sdm/sdmParse.h
	src/loader.cpp
)
USE_WWIILIB_ON(stratLoader)
TARGET_LINK_LIBRARIES(stratLoader teulserver ${LIBZ_LIBRARIES} )
SET_PROPERTY(TARGET stratLoader PROPERTY FOLDER "Tools")


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
    IF ( HOST_TARGET_ARG_INSTANCING )
        TARGET_LINK_LIBRARIES( "${Name}" instancing )
    ENDIF()
    IF ( HOST_TARGET_ARG_LUA )
        TARGET_LINK_LIBRARIES( "${Name}" ${LUA_LIBRARIES} ${TOLUAPP_LIBRARIES} )
    ENDIF()
	USE_WWIILIB_ON( "${Name}" )
    TARGET_LINK_LIBRARIES( "${Name}" teulserver ${LIBZ_LIBRARIES} )
    IF ( NOT "${HOST_TARGET_ARG_EXTRA_DEFINITIONS}" STREQUAL "" )
        SET_PROPERTY( TARGET "${Name}" PROPERTY COMPILE_DEFINITIONS "${HOST_TARGET_ARG_EXTRA_DEFINITIONS}" )
    ENDIF()
    SET_PROPERTY( TARGET "${Name}" PROPERTY FOLDER "Hosts" )
	TARGET_COMPILE_OPTIONS( "${Name}" PRIVATE ${_EXTRA_HOST_COMPILE_OPTIONS} )
	TARGET_LINK_LIBRARIES( "${Name}" ${_EXTRA_HOST_LINK_OPTIONS} )
ENDMACRO( ADD_HOST_TARGET )

####################################
## Auth host
IF ( NOT PN_DISABLE_AUTH )
	ADD_HOST_TARGET( ${AUTH_HOST_EXE} SOURCES
		auth/authClientData.cpp auth/authClientData.h
		auth/authGame.h
		auth/authHost.cpp auth/authHost.h
		auth/authHostMessages.cpp auth/authHostMessages.h
		auth/authOTP.cpp auth/authOTP.h
		auth/authPack.cpp auth/authPack.h
		wwii/wwiiRunningAverage.h
		EXTRA_DEFINITIONS "IS_AUTH_HOST"
	)
	SET(AUTH_INSTALL ${AUTH_HOST_EXE})
ENDIF()

####################################
## Controller host
ADD_HOST_TARGET( ${CONTROLLER_HOST_EXE} SOURCES
	src/controller.cpp
	src/controllerGameProcess.cpp src/controllerGameProcess.h
    EXTRA_DEFINITIONS "IS_CONTROLLER_HOST"
)
TARGET_LINK_LIBRARIES( ${CONTROLLER_HOST_EXE} ${LIBEVENT_LIBRARIES} )

####################################
## Chat host

# Primarily communications-oriented server, the chat host.
# Sadly it does an awful lot of the "map/mission" server's job
# because the original design disconnected players from
# map/mission when they spawned.
ADD_HOST_TARGET( ${CHAT_HOST_EXE} SOURCES
	auth/authDispatch.h
	chat/chatBailSTO.h
	chat/chatBanPlayer.cpp chat/chatBanPlayer.h
	chat/chatBrigadeControl.cpp chat/chatBrigadeControl.h
	chat/chatBrigadeMove.cpp chat/chatBrigadeMove.h
	chat/chatBrigadeMoveResults.h
	chat/chatBrigadeVote.h
	chat/chatBroadcast.h
	chat/chatBroadcastHCPacket.h
	chat/chatBroadcastObject.h
	chat/chatChannelID.h
	chat/chatChannelList.cpp chat/chatChannelList.h
	chat/chatChannelNumber.cpp chat/chatChannelNumber.h
	chat/chatChannels.h
	chat/chatChannelTypes.h
	chat/chatClientMessage.h
	chat/chatConstants.h
	chat/chatData.h
	chat/chatDB.cpp chat/chatDB.h
	chat/chatDebugConditions.h
	chat/chatDecisions.cpp chat/chatDecisions.h
	chat/chatDefines.h
	chat/chatDotBetaCommands.cpp
	chat/chatDotCommand.cpp chat/chatDotCommand.h
	chat/chatDotCommands.cpp
	chat/chatDotGMCommands.cpp
	chat/chatDotHCCommands.cpp
	chat/chatDotPassthrus.cpp
	chat/chatDotSquadCommands.cpp
	chat/chatEditString.cpp chat/chatEditString.h
	chat/chatFriendList.cpp chat/chatFriendList.h
	chat/chatFriendLogin.cpp
	chat/chatGenericString.cpp chat/chatGenericString.h
	chat/chatGrid.cpp chat/chatGrid.h
	chat/chatGroupPermissions.cpp chat/chatGroupPermissions.h
	chat/chatGroupsType.h
	chat/chatHost.cpp chat/chatHost.h
	chat/chatHostBrigadeControl.cpp chat/chatHostBrigadeControl.h
	chat/chatHostMessages.cpp chat/chatHostMessages.h
	chat/chatHostMotds.cpp chat/chatHostMotds.h
	chat/chatHostPlayerIndex.h
	chat/chatHulkSTO.h
	chat/chatIRCPlayer.h
	chat/chatList.cpp chat/chatList.h
	chat/chatLocFile.cpp chat/chatLocFile.h
	chat/chatManagementHostMessages.cpp chat/chatManagementHostMessages.h
	chat/chatManagementIDs.cpp chat/chatManagementIDs.h
	chat/chatManagementPack.cpp
	chat/chatManagementPlayer.cpp chat/chatManagementPlayer.h
	chat/chatMissionInvite.cpp chat/chatMissionInvite.h
	chat/chatMissionMember.cpp chat/chatMissionMember.h
	chat/chatMissions.cpp chat/chatMissions.h
	chat/chatMissionTypes.h
	chat/chatMotds.cpp chat/chatMotds.h
	chat/chatMultiCrew.cpp chat/chatMultiCrew.h
	chat/chatOrdnanceSTO.h
	chat/chatPlayer.cpp chat/chatPlayer.h
	chat/chatPlayerInfo.h
	chat/chatPPOSTO.h
	chat/chatPrototypes.h
	chat/chatReports.cpp chat/chatReports.h
	chat/chatSaveChannels.cpp chat/chatSaveChannels.h
	chat/chatSay.cpp chat/chatSay.h
	chat/chatServerSTO.h
	chat/chatServerTrackedObjects.cpp chat/chatServerTrackedObjects.h
	chat/chatServerTypes.h
	chat/chatSpamCan.cpp chat/chatSpamCan.h
	chat/chatSquadControl.cpp chat/chatSquadControl.h
	chat/chatSquadControlResults.h
	chat/chatSquadControls.cpp
	chat/chatSquadCreate.cpp chat/chatSquadCreate.h
	chat/chatSquadList.cpp chat/chatSquadList.h
	chat/chatSquadListPurge.cpp chat/chatSquadListPurge.h
	chat/chatSquadVote.cpp chat/chatSquadVote.h
	chat/chatSTODamage.h
	chat/chatSTOJoules.h
	chat/chatStrings.cpp
	chat/chatThreads.cpp chat/chatThreads.h
	chat/chatTimeString.cpp chat/chatTimeString.h
	chat/chatTrainers.cpp chat/chatTrainers.h
	chat/chatUMSSTO.h
	chat/chatUnitMotds.cpp chat/chatUnitMotds.h
	chat/chatVehicleSTO.h
	chat/chatWaypoints.cpp chat/chatWaypoints.h
	chat/chatWho.cpp chat/chatWho.h
	chat/chatWhos.cpp chat/chatWhos.h
	host/hostPlayers.h
	inst/instDescriptor.h
	map/mapAWS.cpp map/mapAWS.h
	map/mapHCUnitLocations.cpp map/mapHCUnitLocations.h
	miss/missFlags.h
	mm/mmEnums.h
	strt/strtChatSupportMessages.cpp strt/strtChatSupportMessages.h
	strt/strtChatTypes.cpp strt/strtChatTypes.h
	wwii/wwiiCallbacks.h
	wwii/wwiiHCUnits.cpp wwii/wwiiHCUnits.h
	wwii/wwiiNetPorts.h
	wwii/wwiiPack.h
	wwii/wwiiPlayer.h
	wwii/wwiiServerTrackedObject.cpp wwii/wwiiServerTrackedObject.h
	wwii/wwiiServerTrackedObjectPacker.cpp wwii/wwiiServerTrackedObjectPacker.h
	wwii/wwiiSquads.cpp wwii/wwiiSquads.h

	INSTANCING
    EXTRA_DEFINITIONS "IS_CHAT_HOST"
)
TARGET_LINK_LIBRARIES( ${CHAT_HOST_EXE} ${LIBZ_LIBRARIES})

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
	comm/commHostMessages.cpp comm/commHostMessages.h
	comm/commPackers.cpp comm/commPackers.h
	comm/commTypes.h
	host/host.cpp host/host.h
	host/hostAutomation.cpp host/hostAutomation.h
	host/hostCellNet.cpp host/hostCellNet.h
	host/hostFraming.cpp host/hostFraming.h
	host/hostGrid.cpp
	host/hostGridCell.cpp host/hostGridCell.h
	host/hostPlayer.h
	host/hostPlayers.h
	host/hostScoringActor.cpp
	host/hostVehicle.cpp host/hostVehicle.h
	include/xyz_tuple.h
	inst/instDescriptor.h
	miss/missFlags.h
	miss/missionPack.cpp miss/missionPack.h
	miss/missionPacket.cpp miss/missionPacket.h
	mm/mmCellHostSupportMessages.cpp mm/mmCellHostSupportMessages.h
	mm/mmEnums.h
	mm/mmPack.cpp mm/mmPack.h
	parse/parse.h
	parse/parseIO.h
	parse/parseWor.cpp parse/parseWor.h
	src/installCallbacks.cpp
	strt/strtConstants.h
	strt/strtCPColliders.cpp strt/strtCPColliders.h
	strt/strtCPFlags.h
	strt/strtCPStruct.cpp strt/strtCPStruct.h
	strt/strtData.cpp strt/strtData.h
	strt/strtDTL.h
	strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h
	strt/strtHack.cpp strt/strtHack.h
	strt/strtIO.cpp strt/strtIO.h
	strt/strtObjectStruct.cpp strt/strtObjectStruct.h
	strt/strtPack.cpp strt/strtPack.h
	strt/strtProtos.h
	strt/strtSyncData.cpp strt/strtSyncData.h
	strt/strtSyncSupportMessages.cpp strt/strtSyncSupportMessages.h
	strt/strtTypes.h
	strt/strtUpdater.cpp strt/strtUpdater.h
	strt/strtUpdaterHostMessages.cpp strt/strtUpdaterHostMessages.h
	strt/strtUpdaterPack.cpp strt/strtUpdaterPack.h
	strt/strtUpdaterSupportMessages.cpp strt/strtUpdaterSupportMessages.h
	strt/strtWOR.h
	updt/updtEnums.h
	updt/updtFraming.h
	updt/updtInfDeath.h
	updt/updtPackers.cpp updt/updtPackers.h
	updt/updtTurrets.h
	updt/updtTypes.h
	updt/updtVehiclePosition.h
	weap/weapSerial.h
	weap/weapSpecPacker.h
	wwii/wwiiExitCodes.h
	wwii/wwiiFullObjIDPacker.h
	wwii/wwiiHCUnits.h
	wwii/wwiiHostMessages.cpp wwii/wwiiHostMessages.h
	wwii/wwiiRank.h
	wwii/wwiiSyncVar.h

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
	auth/authDispatch.h
	auth/authPlayerDetails.h
	chat/chatGroupPermissions.cpp chat/chatGroupPermissions.h
	chat/chatGroupsType.h
	chat/chatSupportMessages.cpp chat/chatSupportMessages.h
	chat/chatSupportRPCMessages.cpp chat/chatSupportRPCMessages.h
	host/hostPlayers.h
	include/mobileSpawnEnums.h
	inst/instBasicStatus.h
	inst/instDescriptor.h
	inst/instHostInstance.h
	inst/instHostInstanceImpl.h
	inst/instHostMessages.cpp inst/instHostMessages.h
	inst/instStateEnums.h
	map/mapArenaDescription.cpp map/mapArenaDescription.h
	map/mapBrigades.cpp map/mapBrigades.h
	map/mapBrigadeSummary.h
	map/mapChecksums.cpp map/mapChecksums.h
	map/mapData.cpp map/mapData.h
	map/mapDB.cpp map/mapDB.h
	map/mapHCUnitLocations.cpp map/mapHCUnitLocations.h
	map/mapHost.cpp
	map/mapHostMessages.cpp map/mapHostMessages.h
	map/mapHostSupercellData.cpp
	map/mapMissionList.h
	map/mapPack.cpp
	map/mapPersonaTypes.h
	map/mapPlayer.cpp map/mapPlayer.h
	map/mapPlayerTypes.h
	map/mapThreads.cpp map/mapThreads.h
	miss/missActiveMission.cpp miss/missActiveMission.h
	miss/missFlags.h
	miss/mission.cpp miss/mission.h
	miss/missionDetail.cpp miss/missionDetail.h
	miss/missionHostMessages.cpp miss/missionHostMessages.h
	miss/missionPack.cpp miss/missionPack.h
	miss/missionPacket.cpp miss/missionPacket.h
	miss/missionScoring.cpp
	miss/missionTypes.h
	miss/missionUtils.cpp miss/missionUtils.h
	miss/missMobileSpawn.cpp miss/missMobileSpawn.h
	mm/mmdbLazyFetchPersona.cpp mm/mmdbLazyFetchPersona.h
	mm/mmdbPersonaSelections.cpp mm/mmdbPersonaSelections.h
	mm/mmEnums.h
	mm/mmHostInstanceImpl.cpp mm/mmHostInstanceImpl.h
	mm/mmHostMessages.cpp mm/mmHostMessages.h
	mm/mmMain.cpp mm/mmMain.h
	mm/mmPack.cpp mm/mmPack.h
	mm/mmPolls.cpp mm/mmPolls.h
	mm/mmSpawn.cpp mm/mmSpawn.h
	mm/mmSpawnGrantResponse.cpp mm/mmSpawnGrantResponse.h
	mm/mmSpawnTypes.h
	parse/parse.h
	parse/parseIO.h
	parse/parseWor.cpp parse/parseWor.h
	score/scoreActor.h
	score/scoreCallbacks.cpp
	score/scoreDeath.cpp score/scoreDeath.h
	score/scoreEvent.h
	score/scoreSortie.cpp score/scoreSortie.h
	strt/strtConstants.h
	strt/strtCPFlags.h
	strt/strtCPStruct.cpp strt/strtCPStruct.h
	strt/strtData.cpp strt/strtData.h
	strt/strtDB.cpp strt/strtDB.h
	strt/strtDTL.h
	strt/strtEvents.cpp strt/strtEvents.h
	strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h
	strt/strtIO.cpp strt/strtIO.h
	strt/strtMapMessages.cpp strt/strtMapMessages.h
	strt/strtObjectStruct.cpp strt/strtObjectStruct.h
	strt/strtPack.cpp strt/strtPack.h
	strt/strtProtos.h
	strt/strtSyncData.cpp strt/strtSyncData.h
	strt/strtSyncSupportMessages.cpp strt/strtSyncSupportMessages.h
	strt/strtWOR.h
	teul/teulNetworkObject.h
	weap/weapSerial.h
	weap/weapSpecPacker.h
	wwii/wwiiFullObjIDPacker.h
	wwii/wwiiHCUnits.cpp wwii/wwiiHCUnits.h
	wwii/wwiiHostMessages.cpp wwii/wwiiHostMessages.h
	wwii/wwiiNetPorts.h
	wwii/wwiiOpinionPoll.cpp wwii/wwiiOpinionPoll.h
	wwii/wwiiPack.h
	wwii/wwiiPlayer.h
	wwii/wwiiRank.h
	wwii/wwiiRTBCodes.h
	wwii/wwiiSyncVar.h

	# Additional Files
	garrisons/garrisonCountryChangeRequest.cpp garrisons/garrisonCountryChangeRequest.h
	strt/strtLinkStruct.h
	map/mapPersonaSummary.h

    INSTANCING
	EXTRA_DEFINITIONS "IS_MAP_HOST"
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
		chat/chatHandle.h
		hostproj/${STRAT_HOST_EXE}.pkg
		hostproj/syscfg.h
		include/luaGlue.h
		include/luaPkg.h
		include/platforms.h
		include/precompile_includes.h
		include/systypes.h
		inst/instBasicStatus.h
		weap/weapSpec.h
		wwii/wwiiConfig.h
		wwii/wwiiOwnershipEnums.h
		wwii/wwiiThreads.h
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
	chat/chatBrigadeMoveResults.h
	chat/chatStratSupportMessages.cpp chat/chatStratSupportMessages.h
	chat/chatSupportRPCMessages.cpp chat/chatSupportRPCMessages.h
	cnvy/cnvyConvoyList.cpp cnvy/cnvyConvoyList.h
	cnvy/cnvyMain.cpp cnvy/cnvyMain.h
	garrisons/garrisonCountryChangeRequest.cpp garrisons/garrisonCountryChangeRequest.h
	include/intrusiveList.h
	inst/instDescriptor.h
	list/list.h
	list/listCL.cpp list/listCL.h
	mem/memPool.h
	miss/missionPack.cpp miss/missionPack.h
	miss/missionTypes.h
	mm/mmEnums.h
	mm/mmPack.cpp mm/mmPack.h
	mm/mmSpawnTypes.h
	parse/parse.h
	parse/parseIO.h
	parse/parseWor.cpp parse/parseWor.h
	src/luaGlue.cpp include/luaGlue.h
	strt/strtCaptureAction.cpp strt/strtCaptureAction.h
	strt/strtCaptureContribution.cpp strt/strtCaptureContribution.h
	strt/strtCaptureScore.h
	strt/strtChatTypes.cpp strt/strtChatTypes.h
	strt/strtConstants.h
	strt/strtCP.cpp strt/strtCP.h
	strt/strtCPClass.cpp strt/strtCPClass.h
	strt/strtCPFlags.h
	strt/strtCPStruct.cpp strt/strtCPStruct.h
	strt/strtData.cpp strt/strtData.h
	strt/strtDatabaseSchemas.h
	strt/strtDB.cpp strt/strtDB.h
	strt/strtdbSaveWorld.cpp strt/strtdbSaveWorld.h
	strt/strtDotCommands.cpp strt/strtDotCommands.h
	strt/strtDTL.h
	strt/strtEnemyReport.cpp strt/strtEnemyReport.h
	strt/strtEventQueue.h
	strt/strtFacilityStruct.cpp strt/strtFacilityStruct.h
	strt/strtFB.cpp strt/strtFB.h
	strt/strtHack.cpp strt/strtHack.h
	strt/strtHCUnits.cpp strt/strtHCUnits.h
	strt/strtHost.cpp
	strt/strtHostDB.cpp strt/strtHostDB.h
	strt/strtHostLua.cpp strt/strtHostLua.h
	strt/strtHostMessages.cpp strt/strtHostMessages.h
	strt/strtIO.cpp strt/strtIO.h
	strt/strtJoinList.h
	strt/strtJoinRequestList.cpp
	strt/strtMain.cpp strt/strtMain.h
	strt/strtMapTypes.h
	strt/strtMission.cpp strt/strtMission.h
	strt/strtObjectStruct.cpp strt/strtObjectStruct.h
	strt/strtPack.cpp strt/strtPack.h
	strt/strtPostedWeaponList.cpp strt/strtPostedWeaponList.h
	strt/strtProtos.h
	strt/strtServerTypes.h
	strt/strtSpawn.cpp strt/strtSpawn.h
	strt/strtSpawnWeaponList.cpp
	strt/strtSupply.cpp strt/strtSupply.h
	strt/strtSync.cpp strt/strtSync.h
	strt/strtSyncData.cpp strt/strtSyncData.h
	strt/strtTOE.cpp strt/strtTOE.h
	strt/strtTOETypes.h
	strt/strtTypes.h
	strt/strtWeaponEntry.cpp strt/strtWeaponEntry.h
	strt/strtWeaponsReturn.h
	strt/strtWOR.h
	teul/teulCLList.h
	weap/weapSerial.h
	wwii/wwiiHCUnits.cpp wwii/wwiiHCUnits.h
	wwii/wwiiRTBCodes.h

    ${STRAT_HOST_EXE}Lua.cc
    EXTRA_DEFINITIONS "IS_STRAT_HOST" "LUA_MODULE_OPEN=tolua_stratd_open" "CHECK_SUPPLY=1"
    INSTANCING LUA
)
IF ( WIN32 )
    SET_PROPERTY( SOURCE ${STRAT_HOST_EXE}Lua.cc PROPERTY COMPILE_FLAGS
                  "/W0" )
ELSE()
	SET_PROPERTY( SOURCE ${STRAT_HOST_EXE}Lua.cc PROPERTY COMPILE_FLAGS
                  "-Wno-unused-function -Wno-zero-as-null-pointer-constant" )
ENDIF()
TARGET_INCLUDE_DIRECTORIES( ${STRAT_HOST_EXE} PRIVATE ${TOLUAPP_INCLUDE_DIR})

INSTALL(
    TARGETS
        ${CONTROLLER_HOST_EXE} ${STRAT_HOST_EXE}
        ${CHAT_HOST_EXE} ${MAP_HOST_EXE} ${CELL_HOST_EXE} ${AUTH_INSTALL}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    COMPONENT hosts
)

####################################
## File installs

# Environment template files from the source tree
# Do not actually install this file, otherwise we could accidentally overwrite real config
#INSTALL( FILES environment.cfg DESTINATION bin COMPONENT config )

# Lua scripts
INSTALL( DIRECTORY hostproj/lua/ DESTINATION lua COMPONENT lua PATTERN ".git" EXCLUDE PATTERN "*.lua" )

# Embedded webserver root
INSTALL( DIRECTORY hostproj/web/ DESTINATION web/embedded COMPONENT web PATTERN ".git" EXCLUDE )

# Auth relay
IF ( NOT PN_DISABLE_AUTH )
    CONFIGURE_FILE( auth/authRelay.apache2.conf.in auth/authRelay.apache2.conf @ONLY )
    INSTALL(
        FILES auth/authRelay.php auth/authRelay.apache2.conf
        DESTINATION web/relay
        COMPONENT relay
    )
ENDIF()

####################################
## Tests

# Auth connection test
IF ( NOT PN_DISABLE_AUTH )
	ADD_EXECUTABLE( testAuthConnect tests/testAuthConnect.cpp )
	USE_WWIILIB_ON( testAuthConnect )
	TARGET_LINK_LIBRARIES( testAuthConnect teulserver )
	SET_PROPERTY( TARGET testAuthConnect PROPERTY FOLDER "Test" )
ENDIF()

# Builds a single executable that runs all of the unit tests;
# The install process will not execute unless all tests complete.
ADD_EXECUTABLE( testUnits
	auth/authClientData.cpp
	comm/commPackers.cpp
	host/hostGridCell.cpp
	tests/testChatHandle.cpp
	tests/testChatMissionStatus.cpp
	tests/testCommandWithArgs.cpp
	tests/testCovert.cpp
	tests/testCryptographer.cpp
	tests/testFullObjID.cpp
	tests/testHostGridCell.cpp
	tests/testInplaceVector.cpp
	tests/testLocalizedMessage.cpp
	tests/testPackers.cpp
	tests/testPersonaXfer.cpp
	tests/testSuite.h
	tests/testSupercellVislevelXfer.cpp
	tests/testTeulBits.cpp
	tests/testTeulPacket.cpp
	tests/testUnits.cpp tests/testUnits.h
	tests/testXYZTuple.cpp
	wwii/wwiiServerTrackedObjectPacker.cpp
)
SET_PROPERTY( TARGET testUnits PROPERTY COMPILE_DEFINITIONS "ENABLE_TESTS" )
USE_WWIILIB_ON( testUnits )
TARGET_COMPILE_OPTIONS( testUnits PRIVATE ${_EXTRA_HOST_COMPILE_OPTIONS} )
TARGET_LINK_LIBRARIES( testUnits ${_EXTRA_HOST_LINK_OPTIONS} )
TARGET_LINK_LIBRARIES( testUnits teulserver ${LIBZ_LIBRARIES} )
SET_PROPERTY( TARGET testUnits PROPERTY FOLDER "Test" )

ADD_TEST( testUnits testUnits )

# teul tests
ADD_EXECUTABLE(testTeul
	tests/testTeul.cpp
	tests/testTeulClient.h
	tests/testTeulServer.h
	tests/testTeulTest.h
)
USE_WWIILIB_ON( testTeul )
TARGET_COMPILE_OPTIONS( testTeul PRIVATE ${_EXTRA_HOST_COMPILE_OPTIONS} )
TARGET_LINK_LIBRARIES( testTeul ${_EXTRA_HOST_LINK_OPTIONS} )
TARGET_LINK_LIBRARIES( testTeul teulserver ${LIBZ_LIBRARIES} )
SET_PROPERTY( TARGET testTeul PROPERTY FOLDER "Test" )

