# Hide these options from the default CMake gui view.
mark_as_advanced(ARCHITECTURE BUILDNAME OPTIMIZATION PN_APP_ID CMAKE_INSTALL_PREFIX PN_BETA_APPID PN_RELEASE_APPID)

IF (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
	SET(IS_CLANG TRUE)
ELSE ()
	SET(IS_CLANG FALSE)
ENDIF ()

IF ( NOT CMAKE_BUILD_TYPE )
	message(STATUS "No CMAKE_BUILD_TYPE specified: Assuming DEBUG")
	SET(CMAKE_BUILD_TYPE DEBUG)
ENDIF ()

# User-settable options for enabling various SSE variants.
SET   ( PN_CLUSTER "Dev"		CACHE STRING "Target hardware on a specific cluster" )

OPTION( PN_MEM_DEBUG		"Enable debugging of memory usage" ON )

OPTION( PN_DISABLE_AUTH		"Use auth-bypass only" OFF )
OPTION( PN_DIFFERENT_PORTS	"Use different public-facing ports for game servers." ON )

IF ( HAVE_EXPERIMENTAL_STUFF )
	OPTION( PN_EXPERIMENTAL		"Enable expirimental files" OFF )
	IF ( NOT MSVC )
		OPTION( PN_HEAVY_OPTS       "Enable additional heavy-hitting optimizations" OFF )
	ELSE ()
		SET(PN_HEAVY_OPTS "OFF")
		MARK_AS_ADVANCED(PN_HEAVY_OPTS)
	ENDIF ()
ELSE ()
	SET(PN_EXPERIMENTAL "OFF")
	SET(PN_HEAVY_OPTS "OFF")
	MARK_AS_ADVANCED(PN_EXPERIMENTAL PN_HEAVY_OPTS)
ENDIF ()

IF ( NOT MSVC )
	OPTION( PN_HOST_TIMERS		"Enable host performance timers" OFF )
ENDIF ()
IF ( NOT IS_CLANG AND NOT MSVC )
	OPTION( PN_GCC_STATS		"Enable output of GCC compilation stats" OFF )
ELSE ()
	SET(PN_GCC_STATS 0 CACHE STRING "Enable GCC compilation stats")
	MARK_AS_ADVANCED(PN_GCC_STATS)
ENDIF ()

IF ( IS_CLANG OR NOT MSVC )
	OPTION( PN_OMIT_FRAME_POINTER "Enable omit-frame pointer" ON )
ENDIF ()

IF ( IS_CLANG )
	OPTION( PN_NICE_STACKS		 "Disable tail call elimination" OFF )
	SET( PN_CLANG_SANITIZER  "" CACHE STRING "Enable specific clang sanitizer(s)" )
	OPTION( PN_STATIC_ANALYZER	 "Static Analyzer" OFF )
ELSE ()
	mark_as_advanced(PN_NICE_STACKS PN_CLANG_SANITIZER PN_STATIC_ANALYZER)
ENDIF ()

IF ( !WIN32 )
	OPTION( PN_BUILD_32BIT		"Enable 32-bit build" OFF )
ELSE ()
	SET(PN_BUILD_32BIT "OFF")
	MARK_AS_ADVANCED(PN_BUILD_32BIT)
ENDIF ()

# User-settable option for enabling optimizations.
IF ( PN_BUILD_TYPE MATCHES "Dev" OR CMAKE_BUILD_TYPE MATCHES "Debug" )
	SET ( PN_ENABLE_OPTS	0 CACHE STRING "Enable optimizations" )
ELSEIF ( PN_BUILD_TYPE MATCHES "Beta" )
	SET ( PN_ENABLE_OPTS	1 CACHE STRING "Enable optimizations" )
ELSE ()
	SET ( PN_ENABLE_OPTS	3 CACHE STRING "Enable optimizations" )
ENDIF ()

# Control which game APP_ID the executables will target.
SET ( PN_BETA_APPID "12" CACHE STRING "Beta AppID" )
SET ( PN_RELEASE_APPID "12" CACHE STRING "Release AppID" )
#SET ( PN_RELEASE_APPID "1" CACHE STRING "Release AppID" )
SET ( PN_APP_ID "${PN_BETA_APPID}" CACHE STRING "AppID for this build (${PN_BETA_APPID}=Beta, ${PN_RELEASE_APPID}=Release)" )
