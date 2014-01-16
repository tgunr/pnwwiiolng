# Server project dependencies.

MARK_AS_ADVANCED(CNL LIBEVENT_FOUND LIBEVENT_INCLUDE_DIR)

FIND_LIBRARY(CNL NAMES ncurses)
IF ( UNIX AND NOT CNL )
 MESSAGE(SEND_ERROR "ncurses library not found")
ENDIF ()

# Include our macros etc
find_package(MySQL REQUIRED)

# Find SubVersion
if ( 0 )
 find_package(Subversion REQUIRED)
 IF ( NOT Subversion_FOUND )
	 message(SEND_ERROR "Unable to find subversion executables")
 ENDIF ( NOT Subversion_FOUND )
 # Ask Subversion for info on this working copy.
 Subversion_WC_INFO(${CMAKE_CURRENT_SOURCE_DIR} REPO)
else ( )
# include ( cmakeincludes/FindMercurial.cmake )
# if ( NOT MERCURIAL_FOUND )
#  message(SEND_ERROR "Unable to find mercurial executables")
# endif ( )
# MERCURIAL_HG_INFO(${CMAKE_CURRENT_SOURCE_DIR} REPO)
endif ( )

# Find Lua 5.1
IF ( WIN32 )
	SET(LUA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/lua")
	SET(LUA_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/../lib/win/3rdparty/lua51.lib")
ENDIF ( )
find_package(Lua51 REQUIRED)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
find_package(Toluapp REQUIRED)
INCLUDE_DIRECTORIES(${TOLUAPP_INCLUDE_DIR})

IF ( UNIX )
	find_program(CTAGS ctags)
	add_custom_target(ctags
		COMMAND ${CTAGS} -R ${CMAKE_CURRENT_SOURCE_DIR}/*/*.[ch]*
		)
	MARK_AS_ADVANCED(CTAGS)
ENDIF ( )

IF ( WIN OR WIN32 OR WIN64 )
	ADD_SUBDIRECTORY("${LIBRARIES_DIR}/libevent" "${CMAKE_CURRENT_BINARY_DIR}/libevent")
	SET(LIBEVENT_INCLUDE_DIR "${LIBRARIES_DIR}/libevent/include")
	INCLUDE_DIRECTORIES(${LIBEVENT_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${LIBEVENT_INCLUDE_DIR}/../WIN32-CODE)
	SET_TARGET_PROPERTIES(event PROPERTIES FOLDER "Libs")
ELSE ( )
	FIND_LIBRARY(LIBEVENT_FOUND
			NAMES	event-2.0-5 event-2.0 event2 event
				libevent-2.0-5 libevent-2.0 libevent2 libevent
			)
	FIND_PATH(LIBEVENT_INCLUDE_DIR event2/event.h
			HINTS /usr/include /usr/local/include)
	IF (NOT LIBEVENT_FOUND OR NOT LIBEVENT_INCLUDE_DIR)
		MESSAGE(FATAL_ERROR "libevent not found")
	ENDIF ( )
ENDIF ( )
