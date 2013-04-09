# Create the basic directory structure needed by the game in
# the specified top-level path.

IF ( UNIX OR APPLE )
	SET( PN_PLAYNET_PATH "/playnet/"	CACHE STRING "Default top-level runtime path for this build to use." )
ELSEIF ( WIN32 )
	SET( PN_PLAYNET_PATH "C:/playnet/" CACHE STRING "Default top-level runtime path for this build to use." )
ELSE ( )
	# This could be perfectly fine, but I haven't planned for it.
	MESSAGE(FATAL_ERROR "Unrecognized build configuration, please review CMakeLists.txt before proceeding.")
ENDIF ( )
FILE(TO_CMAKE_PATH "${PN_PLAYNET_PATH}" PN_PLAYNET_PATH)

FILE(TO_NATIVE_PATH "${PN_PLAYNET_PATH}" _PLAYNET)
IF ( WIN32 )
	# Make an escaped version for the config file.
	STRING(REGEX REPLACE "\\\\" "\\\\\\\\" _PLAYNET_PATH_VAR "${_PLAYNET}") 
ENDIF ( )

# The top level path must exist before we'll continue.
IF ( NOT IS_DIRECTORY "${_PLAYNET}" )
	MESSAGE(FATAL_ERROR "${_PLAYNET} does not exist or PN_PLAYNET_PATH is not configured.")
ENDIF ( )

# History:
# The /playnet folder existed on the ".1" machine and was nfs readonly
# mounted to the rest of the cluster.
# Non-game stuff, like cluster management scripts etc, went into the
# /playnet/bin and so on, game related stuff went into /playnet/wwiiol,
# with teulKit stuff going into /playnet/teulKit. teulkit is no-longer
# separate.
FOREACH ( SUB_PATH
			"bin"
			"etc"							
			"wwiiol"
			"wwiiol/bin"
			"wwiiol/lua"
			"wwiiol/web/debug"	# must NEVER expose to players.
			"wwiiol/data"
			"wwiiol/data/terrain"
			"wwiiol/data/th_blitz"
			"wwiiol/data/th_blitz/weapon"
			"wwiiol/data/xml"
			)
	SET(_DIRECTORY "${_PLAYNET}/${SUB_PATH}")
	FILE(TO_NATIVE_PATH "${_DIRECTORY}" _NATIVE_DIRECTORY)
	IF ( NOT IS_DIRECTORY "${_NATIVE_DIRECTORY}" )
		MESSAGE(STATUS "Creating ${_NATIVE_DIRECTORY}.")
		FILE(MAKE_DIRECTORY "${_NATIVE_DIRECTORY}")
	ENDIF ( )
ENDFOREACH ( )
