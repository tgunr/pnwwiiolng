# Default assumption that we aren't a China build.
SET( CHINA 0 )

# PN_BUILD_TYPE describes whether you are building binaries for use
# with a beta server, training server, live server, etc, etc.
# Use the CMAKE_BUILD_TYPE variable to control whether you are
# building a Debug vs Public release.
SET( PN_BUILD_TYPE "Beta" CACHE STRING "Select Release type: Dev/Beta/RC/Live/China/ChinaBeta" )

# The default is BETA.
IF ( NOT PN_BUILD_TYPE )
	message(STATUS "PN_BUILD_TYPE unspecified, assuming Dev")
	set(PN_BUILD_TYPE "Dev")
ENDIF ( NOT PN_BUILD_TYPE )

set(BUILDNAME ${PN_BUILD_TYPE})
IF ( PN_BUILD_TYPE STREQUAL "Dev" )
	# Developer build for internal use.
	set(OPTIMIZATION 0)
	add_definitions(-DDEV_BUILD)
ELSEIF ( PN_BUILD_TYPE STREQUAL "Beta" OR PN_BUILD_TYPE STREQUAL "Auth" )
	# Closed beta build.
	set(OPTIMIZATION ${PN_ENABLE_OPTS})
	add_definitions(-DCLOSED_BETA)
ELSEIF ( PN_BUILD_TYPE STREQUAL "RC" )
	# Usually a release-candidate beta build.
	set(OPTIMIZATION 4)
	add_definitions(-DTEST_RELEASE)
ELSEIF ( PN_BUILD_TYPE STREQUAL "Live" )
	# Public release for Playnet.
	set(OPTIMIZATION 4)
	add_definitions(-DPUBLIC_RELEASE)
ELSEIF ( PN_BUILD_TYPE STREQUAL "China" )
	# Public release for Happyoo.
	set(BUILDNAME "Zhongguo")
	set(OPTIMIZATION 4)
	set(CHINA 1)
	add_definitions(-DHAPPYOO=1 -DCHINA=1 -DPUBLIC_RELEASE)
ELSEIF ( PN_BUILD_TYPE STREQUAL "ChinaBeta" )
	# Testing build for Happyoo.
	set(BUILDNAME "Zhongguo-Beta")
	set(OPTIMIZATION 2)
	set(CHINA 1)
	add_definitions(-DHAPPYOO=1 -DCHINA=1 -DCLOSED_BETA)
ELSE ( )
	message(FATAL_ERROR "Unrecognized PN_BUILD_TYPE type '${PN_BUILD_TYPE}'")
ENDIF ( )

# We put the host stuff in a different folder for China.
IF ( NOT CHINA )
	set(CMAKE_INSTALL_PREFIX "${PN_INSTALL_PATH}/wwiiol" CACHE FILEPATH "Installation directory")
ELSE ( )
	set(CMAKE_INSTALL_PREFIX "/playnet/builds/china/binary/wwiiol" CACHE FILEPATH "Installation directory")
ENDIF ( )
