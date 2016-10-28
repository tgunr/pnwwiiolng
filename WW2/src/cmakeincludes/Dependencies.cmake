# Server project dependencies.
MARK_AS_ADVANCED(
    LIBEVENT_FOUND LIBEVENT_INCLUDE_DIR
    LUA_LIBRARIES LUA_INCLUDE_DIR
    GIT_EXECUTABLE
    LIBIRCCLIENT_FOUND
	LIBB64_DIR
)

# We need ncurses and readline for the lua client, but don't care on Windows
IF ( NOT WIN32 )
	FIND_PACKAGE( Readline 6.0 )
	IF ( NOT READLINE_FOUND )
		MESSAGE( FATAL_ERROR "readline library is required" )
	ENDIF()

	FIND_PROGRAM(CTAGS ctags)
	IF ( CTAGS )
		ADD_CUSTOM_TARGET(ctags COMMAND ${CTAGS} -R ${CMAKE_CURRENT_SOURCE_DIR}/*/*.[ch]*)
	ENDIF()
	MARK_AS_ADVANCED(CTAGS)
ENDIF()

# Pull in MySQL
FIND_PACKAGE( MySQL REQUIRED )
INCLUDE_DIRECTORIES( ${MYSQL_INCLUDE_DIR} )

# Make sure Git is available
FIND_PROGRAM( GIT_EXECUTABLE NAMES git )
IF ( NOT GIT_EXECUTABLE )
    MESSAGE( FATAL_ERROR "A working git executable is required" )
ENDIF()

# Find Lua 5.1- but for now, pull in our embedded (outdated) one
ADD_SUBDIRECTORY( ${LIBRARIES_DIR}/lua51 ${CMAKE_CURRENT_BINARY_DIR}/lua51 )
INCLUDE_DIRECTORIES( ${LUA_INCLUDE_DIR} )
SET_TARGET_PROPERTIES( lua51 PROPERTIES FOLDER "Libs" )

# Ditto for tolua++ since we haven't gotten rid of it yet
ADD_SUBDIRECTORY( ${LIBRARIES_DIR}/tolua++ ${CMAKE_CURRENT_BINARY_DIR}/tolua++ )
SET_TARGET_PROPERTIES( libtolua++ PROPERTIES FOLDER "Libs" )
SET_TARGET_PROPERTIES( tolua++ PROPERTIES FOLDER "Tools" )

# On Windows, pull in our embedded libevent; elsewhere, search the system for it
IF ( WIN32 )
	ADD_SUBDIRECTORY( "${LIBRARIES_DIR}/libevent" "${CMAKE_CURRENT_BINARY_DIR}/libevent" )
	SET( LIBEVENT_INCLUDE_DIR "${LIBRARIES_DIR}/libevent/include" )
	INCLUDE_DIRECTORIES( ${LIBEVENT_INCLUDE_DIR} ${LIBEVENT_INCLUDE_DIR}/../WIN32-CODE )
	SET_TARGET_PROPERTIES( event PROPERTIES FOLDER "Libs" )
ELSE()
    FIND_PACKAGE( LibEvent REQUIRED )
ENDIF()

# Need Python for revno.py
FIND_PACKAGE( PythonInterp 2.7 REQUIRED )

# Need Bison and Flex for OMGgen; use embedded versions on Windows
IF ( WIN32 )
    SET( BISON_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/omggen/win_bison.exe" )
    SET( FLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/omggen/win_flex.exe" )
ENDIF()
# Setting the executables first on Windows allows these to work normally
FIND_PACKAGE( BISON REQUIRED )
FIND_PACKAGE( FLEX REQUIRED )

# Need libircclient
INCLUDE_DIRECTORIES( libircclient/include )

# Embedded libb64
SET(LIBB64_DIR "${LIBRARIES_DIR}/libb64/libb64-1.2")
ADD_LIBRARY( libb64
	${LIBB64_DIR}/src/cdecode.c
    	${LIBB64_DIR}/include/b64/cdecode.h
    		${LIBB64_DIR}/include/b64/decode.h
    ${LIBB64_DIR}/src/cencode.c
    	${LIBB64_DIR}/include/b64/cencode.h
    		${LIBB64_DIR}/include/b64/encode.h
)
INCLUDE_DIRECTORIES( ${LIBB64_DIR}/include )
SET_PROPERTY( TARGET libb64 PROPERTY COMPILE_DEFINITIONS )
SET_PROPERTY( TARGET libb64 PROPERTY FOLDER "Libs" )


IF ( PN_CLUSTER STREQUAL "rPI" )
	SET(LIBXML2_LIBRARIES "${PROJECT_DIR}/WW2/lib/rPI/${CMAKE_BUILD_TYPE}/3rdParty/libxml2.a")
	SET(LIBZ_LIBRARIES "/usr/lib/arm-linux-gnueabihf/libz.a")
	INCLUDE_DIRECTORIES( "${PROJECT_DIR}/WW2/include" )
	MESSAGE(STATUS "rPI : Libs: ${LIBXML2_LIBRARIES} ${LIBZ_LIBRARIES}")
ELSEIF ( WIN OR WIN32 OR WIN64 )
	FIND_LIBRARY(LIBXML2_LIBRARIES
			NAMES	libxml2 xml2
			HINTS "${PROJECT_DIR}/WW2/lib/win/${CMAKE_BUILD_TYPE}/3rdParty")
	INCLUDE_DIRECTORIES( "${PROJECT_DIR}/WW2/include" )
ELSE()
	FIND_PACKAGE( LibXml2 REQUIRED )
	INCLUDE_DIRECTORIES( ${LIBXML2_INCLUDE_DIR} )
ENDIF()
