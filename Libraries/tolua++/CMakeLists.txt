# Original attribution:
## Copyright (C) 2007-2012 LuaDist.
## Created by Peter Kapec
## Redistribution and use of this file is allowed according to the terms of the MIT license.
## For details see the COPYRIGHT file distributed with LuaDist.
## Please note that the package source code is licensed under its own license.
CMAKE_MINIMUM_REQUIRED ( VERSION 3.12.0 )
PROJECT ( toluapp C )

# Build lib
FILE( GLOB SRC_LIBTOLUAPP src/lib/*.c )
ADD_LIBRARY( libtolua++ STATIC ${SRC_LIBTOLUAPP} )
TARGET_INCLUDE_DIRECTORIES( libtolua++ PUBLIC include PRIVATE src/lib ${LUA_INCLUDE_DIR} )
TARGET_LINK_LIBRARIES( libtolua++ PUBLIC ${LUA_LIBRARIES} )
IF ( MSVC )
	TARGET_COMPILE_DEFINITIONS( libtolua++ PRIVATE "-D_CRT_SECURE_NO_WARNINGS" )
ENDIF()

# Build app
ADD_EXECUTABLE( tolua++ src/bin/tolua.c src/bin/toluabind.c )
TARGET_INCLUDE_DIRECTORIES( tolua++ PRIVATE src/bin )
TARGET_LINK_LIBRARIES( tolua++ libtolua++ )

# Export names to parent scope
SET( TOLUAPP_EXECUTABLE tolua++ PARENT_SCOPE )
SET( TOLUAPP_LIBRARIES libtolua++ PARENT_SCOPE )
SET( TOLUAPP_INCLUDE_DIR "" PARENT_SCOPE )
