# Original attribution:
## Copyright (C) 2007-2012 LuaDist.
## Created by Peter Kapec
## Redistribution and use of this file is allowed according to the terms of the MIT license.
## For details see the COPYRIGHT file distributed with LuaDist.
## Please note that the package source code is licensed under its own license.

PROJECT ( toluapp C )
CMAKE_MINIMUM_REQUIRED ( VERSION 2.8.3 )
IF (CMAKE_VERBOSE_MAKEFILE)
    INCLUDE ( cmake/dist.cmake )
ENDIF ( )

INCLUDE_DIRECTORIES ( include src/lib ${LUA_INCLUDE_DIR} )

# Build lib
FILE ( GLOB SRC_LIBTOLUAPP src/lib/*.c )

ADD_LIBRARY ( libtolua++ STATIC ${SRC_LIBTOLUAPP} )
TARGET_LINK_LIBRARIES ( libtolua++ ${LUA_LIBRARIES} )
SET_TARGET_PROPERTIES ( libtolua++ PROPERTIES CLEAN_DIRECT_OUTPUT 1 )
IF(MSVC)
    SET_TARGET_PROPERTIES ( libtolua++ PROPERTIES
        COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS )
ENDIF()

# Build app
INCLUDE_DIRECTORIES ( src/bin )
ADD_EXECUTABLE ( tolua++ src/bin/tolua.c src/bin/toluabind.c )
TARGET_LINK_LIBRARIES ( tolua++ libtolua++ ${LUA_LIBRARIES} )

SET ( TOLUAPP_EXECUTABLE tolua++ PARENT_SCOPE )
SET ( TOLUAPP_LIBRARIES libtolua++ PARENT_SCOPE )
SET ( TOLUAPP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include PARENT_SCOPE )
