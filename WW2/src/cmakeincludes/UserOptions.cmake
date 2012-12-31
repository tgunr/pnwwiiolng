# Hide these options from the default CMake gui view.
mark_as_advanced(ARCHITECTURE BUILDNAME OPTIMIZATION PN_APP_ID CMAKE_INSTALL_PREFIX PN_BETA_APPID PN_RELEASE_APPID)

IF ( NOT CMAKE_BUILD_TYPE )
	message(STATUS "No CMAKE_BUILD_TYPE specified: Assuming DEBUG")
	SET(CMAKE_BUILD_TYPE DEBUG)
ENDIF ( )

# User-settable options for enabling various SSE variants.
SET   ( PN_CLUSTER "Dev"		CACHE STRING "Target hardware on a specific cluster" )

OPTION( PN_TRACER			"Enable GCC's -ftracer" ON )
OPTION( PN_STACK_PROTECT	"Enable GCC's stack protector option" OFF )

OPTION( PN_MEM_DEBUG		"Enable debugging of memory usage" ON )
OPTION( PN_HOST_TIMERS		"Enable host performance timers" OFF )

OPTION( PN_DISABLE_AUTH		"Use auth-bypass only" OFF )

OPTION( PN_HEAVY_OPTS       "Enable additional heavy-hitting optimizations" OFF )

OPTION( PN_EXPERIMENTAL		"Enable expirimental files" OFF )

# User-settable option for enabling optimizations.
IF ( PN_BUILD_TYPE MATCHES "Dev" OR CMAKE_BUILD_TYPE MATCHES "Debug" )
	SET ( PN_ENABLE_OPTS	0 CACHE STRING "Enable optimizations" )
ELSEIF ( PN_BUILD_TYPE MATCHES "Beta" )
	SET ( PN_ENABLE_OPTS	1 CACHE STRING "Enable optimizations" )
ELSE ( )
	SET ( PN_ENABLE_OPTS	3 CACHE STRING "Enable optimizations" )
ENDIF ( )

# Control which game APP_ID the executables will target.
SET ( PN_BETA_APPID "12" CACHE STRING "Beta AppID" )
SET ( PN_RELEASE_APPID "1" CACHE STRING "Release AppID" )
SET ( PN_APP_ID "${PN_BETA_APPID}" CACHE STRING "AppID for this build (${PN_BETA_APPID}=Beta, ${PN_RELEASE_APPID}=Release)" )
