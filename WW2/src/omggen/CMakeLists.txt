CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(OMGgen)

#SET( CMAKE_CXX_STANDARD_REQUIRED ON )
#SET( CMAKE_CXX_STANDARD 14 )
# Unfortunately, the above doesn't work until CMake 3.1
#ADD_COMPILE_OPTIONS( "-std=gnu++14" )
# And unfortunately, that doesn't work either because it breaks pure-C files
IF ( NOT MSVC )
	SET( CMAKE_CXX_FLAGS "-std=gnu++14" CACHE STRING "" FORCE )
	SET( CMAKE_C_FLAGS "-std=gnu11" CACHE STRING "" FORCE )
ENDIF()

# Need Bison and Flex for OMGgen; use embedded versions on Windows and macOS
IF ( WIN32 )
    SET( BISON_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/win_bison.exe" )
    SET( FLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/win_flex.exe" )
ELSEIF( APPLE )
    SET( BISON_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/mac_bison" )
    SET( FLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/mac_flex" )
ENDIF()
# Setting the executables first on Windows allows these to work normally
FIND_PACKAGE( BISON REQUIRED )
FIND_PACKAGE( FLEX REQUIRED )

BISON_TARGET( OMGParser
    "${CMAKE_CURRENT_SOURCE_DIR}/omggen.y" "${CMAKE_CURRENT_BINARY_DIR}/omggen.tab.cpp"
    COMPILE_FLAGS "--report=all"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/omggen.tab.h"
)
FLEX_TARGET( OMGLexer
    "${CMAKE_CURRENT_SOURCE_DIR}/omggen.l" "${CMAKE_CURRENT_BINARY_DIR}/omggen.yy.cpp"
    COMPILE_FLAGS "-B --nounistd"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/omggen.yy.h"
)
ADD_FLEX_BISON_DEPENDENCY( OMGLexer OMGParser )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} .)

ADD_EXECUTABLE(OMGgen
    omggen.l omggen.y
    omggen.cpp omggen.h
    ${CMAKE_CURRENT_BINARY_DIR}/omggen.tab.cpp ${CMAKE_CURRENT_BINARY_DIR}/omggen.tab.h
    ${CMAKE_CURRENT_BINARY_DIR}/omggen.yy.cpp ${CMAKE_CURRENT_BINARY_DIR}/omggen.yy.h
)
ADD_DEFINITIONS("-DYY_NO_UNPUT")
ADD_DEFINITIONS("-DYY_NO_INPUT")

# Unique Snowflake Alert: We don't want ANY pre-existing compile flags,
# we just want the minimum required to build an optimized binary. This is a
# compiler tool, not a part of the system itself per-se.
IF ( MSVC )
	SET(CMAKE_CXX_FLAGS "/Ox /MP /MT /EHsc")
    SET(CMAKE_CXX_FLAGS_DEBUG "/Ox /MP /MT /EHsc")
    SET_PROPERTY(TARGET OMGgen PROPERTY COMPILE_DEFINITIONS __STDC_VERSION__=199901L)
ELSE()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-error -Wno-all -Wno-everything -Wno-format-security -Wno-sign-compare -Wno-unused-function -Wno-unreachable-code -Wno-unreachable-code-break")
ENDIF()
