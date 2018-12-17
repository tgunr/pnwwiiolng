# Server project dependencies.
MARK_AS_ADVANCED(
    LIBEVENT_FOUND LIBEVENT_INCLUDE_DIR
    GIT_EXECUTABLE
    LIBIRCCLIENT_FOUND
)

# We need PkgConfig for finding some things on Linux
IF ( LINUX )
	FIND_PACKAGE( PkgConfig REQUIRED )
ENDIF()

# Pull in MySQL
FIND_PACKAGE( MySQL REQUIRED )
INCLUDE_DIRECTORIES( ${MYSQL_INCLUDE_DIR} )

# Make sure Git is available. While we don't use it further in the CMakeLists,
# build_info needs it (unless PN_NO_BUILD_INFO is set).
FIND_PROGRAM( GIT_EXECUTABLE NAMES git )
IF ( NOT GIT_EXECUTABLE AND NOT PN_NO_BUILD_INFO )
    MESSAGE( FATAL_ERROR "A working git executable is required" )
ENDIF()

# Eventually we'll go find Lua, but for now pull in our embedded (outdated) one
ADD_SUBDIRECTORY( ${LIBRARIES_DIR}/lua51 lua51 )
SET_TARGET_PROPERTIES( lua51 PROPERTIES FOLDER "Libs" )

# Ditto for tolua++ since we haven't gotten rid of it yet
ADD_SUBDIRECTORY( ${LIBRARIES_DIR}/tolua++ ${CMAKE_CURRENT_BINARY_DIR}/tolua++ )
SET_TARGET_PROPERTIES( libtolua++ PROPERTIES FOLDER "Libs" )
SET_TARGET_PROPERTIES( tolua++ PROPERTIES FOLDER "Tools" )

# On Windows, pull in our embedded libevent; elsewhere, search the system for it
IF ( WIN32 )
	ADD_SUBDIRECTORY( "${LIBRARIES_DIR}/libevent" "${CMAKE_CURRENT_BINARY_DIR}/libevent" )
	SET( LIBEVENT_INCLUDE_DIRS "${LIBRARIES_DIR}/libevent/include" )
	INCLUDE_DIRECTORIES( BEFORE SYSTEM ${LIBEVENT_INCLUDE_DIRS} )
	INCLUDE_DIRECTORIES( BEFORE SYSTEM ${LIBEVENT_INCLUDE_DIRS}/../WIN32-CODE )
	SET_TARGET_PROPERTIES( event PROPERTIES FOLDER "Libs" )
	SET(LIBEVENT_LIBRARIES event)
ELSE()
    FIND_PACKAGE( LibEvent REQUIRED )
	INCLUDE_DIRECTORIES( SYSTEM ${LIBEVENT_INCLUDE_DIRS} )
ENDIF()

IF ( LINUX )
	# Need libcrypto
	FIND_PACKAGE( OpenSSL REQUIRED )
	LINK_LIBRARIES( OpenSSL::Crypto )

	# Need a modern libbsd
	PKG_CHECK_MODULES( LibBsd REQUIRED IMPORTED_TARGET libbsd>=0.9.1 )
	LINK_LIBRARIES( PkgConfig::LibBsd )
ENDIF()

# Need Python for build_info.py
FIND_PACKAGE( PythonInterp 2.7 REQUIRED )

# Need libircclient
INCLUDE_DIRECTORIES( libircclient/include )

IF ( WIN32 )
	FIND_LIBRARY( LIBXML2_LIBRARIES
		NAMES libxml2 xml2
		PATHS "${PROJECT_DIR}/WW2/lib/win/Release/3rdParty"
	)
	ADD_LIBRARY( LibXml2::LibXml2 STATIC IMPORTED )
	SET_PROPERTY( TARGET LibXml2::LibXml2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LIBRARIES_DIR}/libxml2/include" )
	SET_PROPERTY( TARGET LibXml2::LibXml2 APPEND PROPERTY IMPORTED_LOCATION "${LIBXML2_LIBRARIES}" )
ELSE()
	FIND_PACKAGE( LibXml2 REQUIRED )
ENDIF()

INCLUDE_DIRECTORIES( ${LIBRARIES_DIR}/json/src )
