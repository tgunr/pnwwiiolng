CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(OMGgen)
SET(CMAKE_CXX_STANDARD 14)

SET(OMGGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

IF ( WIN32 )
	SET(BISON_EXE "${OMGGEN_DIR}/win_bison.exe")
	SET(FLEX_EXE "${OMGGEN_DIR}/win_flex.exe")
ELSE ( )
	SET(BISON_EXE "bison")
	SET(FLEX_EXE "flex")
ENDIF ( )

SET(OMGGEN_TAB_CPP "${CMAKE_CURRENT_BINARY_DIR}/omggen.tab.cpp")
SET(OMGGEN_TAB_H "${CMAKE_CURRENT_BINARY_DIR}/omggen.tab.h")
SET(OMGGEN_YY_CPP "${CMAKE_CURRENT_BINARY_DIR}/omggen.yy.cpp")
SET(OMGGEN_YY_H "${CMAKE_CURRENT_BINARY_DIR}/omggen.yy.h")

ADD_CUSTOM_COMMAND(OUTPUT ${OMGGEN_TAB_CPP} ${OMGGEN_TAB_H}
    PRE_BUILD
    COMMAND ${BISON_EXE}
    ARGS -o ${OMGGEN_TAB_CPP} --defines=${OMGGEN_TAB_H} --report=all omggen.y
    DEPENDS "${OMGGEN_DIR}/omggen.y"
    MAIN_DEPENDENCY "${OMGGEN_DIR}/omggen.y"
    COMMENT "[BISON] Building OMGGen"
    WORKING_DIRECTORY ${OMGGEN_DIR}
)
ADD_CUSTOM_COMMAND(OUTPUT ${OMGGEN_YY_CPP} ${OMGGEN_YY_H}
    PRE_BUILD
    COMMAND ${FLEX_EXE}
    ARGS -o ${OMGGEN_YY_CPP} -B --nounistd --header-file=${OMGGEN_YY_H} omggen.l
    DEPENDS ${OMGGEN_DIR}/omggen.l ${OMGGEN_TAB_H}
    MAIN_DEPENDENCY "${OMGGEN_DIR}/omggen.l"
    COMMENT "[FLEX] Building OMGGen"
    WORKING_DIRECTORY ${OMGGEN_DIR}
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} .)

ADD_EXECUTABLE(
	OMGgen
		omggen.l
		omggen.y
		${OMGGEN_TAB_CPP} ${OMGGEN_TAB_H}
		${OMGGEN_YY_CPP} ${OMGGEN_YY_H}
		omggen.cpp omggen.h
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
	SET(CMAKE_CXX_FLAGS "-std=c++14 -O3 -march=native -mtune=native -Wno-error -Wno-all -Wno-everything -Wno-format-security -Wno-sign-compare -Wno-unused-function -Wno-unreachable-code -Wno-unreachable-code-break")
ENDIF()
