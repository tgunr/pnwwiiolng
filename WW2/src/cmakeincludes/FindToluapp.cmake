# - Find Tolua++
# This module defines
#  TOLUAPP_EXECUTABLE
#  TOLUAPP_INCLUDE_DIR
#  TOLUAPP_LIBRARIES
#  TOLUAPP_FOUND
# Author: Oliver "kfsone" Smith <osmith@playnet.com>
# Redistribution permission granted.

IF ( WIN32 )
	SET (TOLUAPP_EXECUTABLE_EXT .exe)
ELSE ( WIN32 )
	SET (TOLUAPP_EXECUTABLE_EXT)
ENDIF ( WIN32 )

IF (NOT TOLUAPP_EXECUTABLE)
	FIND_PROGRAM(TOLUAPP_EXECUTABLE
					NAMES
						tolua++-5.1${TOLUAPP_EXECUTABLE_EXT}
						tolua++-51${TOLUAPP_EXECUTABLE_EXT}
						tolua++5.1${TOLUAPP_EXECUTABLE_EXT}
						tolua++-5-1${TOLUAPP_EXECUTABLE_EXT}
						tolua++51${TOLUAPP_EXECUTABLE_EXT}
						tolua++-5${TOLUAPP_EXECUTABLE_EXT}
						tolua++5${TOLUAPP_EXECUTABLE_EXT}
						tolua++${TOLUAPP_EXECUTABLE_EXT}
					PATHS
						/usr/bin
						/usr/local/bin
						$ENV{ProgramFiles}/tolua*
						$ENV{ProgramFiles}/tolua*/bin
						$ENV{ProgramFilesX86}/tolua*
						$ENV{ProgramFilesX86}/tolua*/bin
						$ENV{SystemDrive}/tolua*
						$ENV{SystemDrive}/tolua*/bin
						../tools/tolua
	)
ENDIF (NOT TOLUAPP_EXECUTABLE)

IF (NOT TOLUAPP_INCLUDE_DIR)
	find_path(TOLUAPP_INCLUDE_DIR NAMES tolua++.h
					PATHS
					/usr/include
					/usr/include/tolua++
					/usr/local/include
					/usr/local/include/tolua++
					$ENV{ProgramFiles}/tolua*/include
					$ENV{ProgramFilesX86}/tolua*/include
					$ENV{SystemDrive}/tolua*/include
					../include/lua
					../proj/win
	)
ENDIF (NOT TOLUAPP_INCLUDE_DIR)

IF (NOT TOLUAPP_LIBRARIES)
	IF (APPLE)
		find_library(TOLUAPP_LIBRARIES
						NAMES	libtolua++ tolua++ tolua++-5.1 tolua++51 tolua++5.1
						PATHS
							../lib/mac
							/usr/lib
							/usr/local/lib
		)
	ELSEIF (WIN32)
		find_library(TOLUAPP_LIBRARIES
						NAMES	tolua++ tolua++.lib libtolua++ libtolua++.lib
						PATHS
							../lib/win/3rdParty
							../lib/win/TestRelease
							../lib/win/Release
							$ENV{ProgramFiles}/tolua*/lib
							${PROGRAMFILESX86}/tolua*/lib
							$ENV{SystemDrive}/tolua*/lib
		)
	ELSE (WIN32)
		find_library(TOLUAPP_LIBRARIES
						NAMES
								tolua++-5.1 libtolua++-5.1
								tolua++-51 libtolua++-51
								tolua++5.1 libtolua++5.1
								tolua++51 libtolua++51
								tolua++ libtolua++
						PATHS /playnet/wwiiol/lib /usr/lib /usr/local/lib
		)
	ENDIF (APPLE)
ENDIF (NOT TOLUAPP_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(tolua++ DEFAULT_MSG TOLUAPP_LIBRARIES TOLUAPP_INCLUDE_DIR)

MARK_AS_ADVANCED(TOLUAPP_INCLUDE_DIR TOLUAPP_LIBRARIES TOLUAPP_EXECUTABLE)

