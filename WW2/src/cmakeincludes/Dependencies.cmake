# Server project dependencies.
MARK_AS_ADVANCED(
    LIBEVENT_FOUND LIBEVENT_INCLUDE_DIR
    GIT_EXECUTABLE
    LIBIRCCLIENT_FOUND
)

# We need PkgConfig for finding some things
FIND_PACKAGE( PkgConfig REQUIRED )

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
#INCLUDE_DIRECTORIES( BEFORE SYSTEM ${LUA_INCLUDE_DIR} )
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
	if ( WIN32 )
		INCLUDE_DIRECTORIES( BEFORE SYSTEM ${LIBEVENT_INCLUDE_DIRS}/../WIN32-CODE )
	ENDIF()
	SET_TARGET_PROPERTIES( event PROPERTIES FOLDER "Libs" )
	SET(LIBEVENT_LIBRARIES event)
ELSE()
    FIND_PACKAGE( LibEvent REQUIRED )
	INCLUDE_DIRECTORIES( SYSTEM ${LIBEVENT_INCLUDE_DIRS} )
ENDIF()

# On Linux, make sure libcrypto is handy
IF ( UNIX AND NOT MSVC AND NOT APPLE )
	FIND_PACKAGE( OpenSSL REQUIRED )
	INCLUDE_DIRECTORIES( SYSTEM ${OPENSSL_INCLUDE_DIR} )
	LINK_LIBRARIES( ${OPENSSL_LIBRARIES} )
ENDIF()

# On Linux, we need a modern libbsd
IF ( UNIX AND NOT MSVC AND NOT APPLE )
	PKG_CHECK_MODULES( LIBBSD REQUIRED libbsd>=0.9.1 )
	LINK_LIBRARIES( ${LIBBSD_LDFLAGS} )
ENDIF()

# Need Python for build_info.py
FIND_PACKAGE( PythonInterp 2.7 REQUIRED )

# Need libircclient
INCLUDE_DIRECTORIES( libircclient/include )

SET(BUILDING_OWN_LIBXML2 OFF)
IF ( NOT CMAKE_BITNESS EQUAL BUILD_BITNESS AND NOT APPLE )
	SET(BUILDING_OWN_LIBXML2 ON)
ENDIF()
SET(LIBXML2_SOURCE_DIR ${LIBRARIES_DIR}/libxml2)
IF ( BUILDING_OWN_LIBXML2 )
	MESSAGE(STATUS "Building own libxml2")
	INCLUDE(ExternalProject)
	SET(LIBXML2_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libxml2)
	SET(LIBXML2_LIBRARIES ${LIBXML2_INSTALL_DIR}/lib/libxml2.a)
	IF ( DEBUG )
		SET(LIBXML2_CFLAGS "-m${BUILD_BITNESS} -Og -g3")
	ELSE()
		SET(LIBXML2_CFLAGS "-m${BUILD_BITNESS} -O3 -g")
	ENDIF()
	SET(LIBXML2_LDFLAGS "-m${BUILD_BITNESS}")
	ExternalProject_Add(
		libxml2
		SOURCE_DIR ${LIBXML2_SOURCE_DIR}
		CONFIGURE_COMMAND ${LIBXML2_SOURCE_DIR}/autogen.sh
			"CFLAGS=${LIBXML2_CFLAGS}" "LDFLAGS=${LIBXML2_LDFLAGS}"
			--prefix=${LIBXML2_INSTALL_DIR}
			--disable-shared --enable-static --disable-ipv6
			--without-python --with-modules
			--without-lzma --without-zlib
			--with-iconv --without-icu --with-iso8859x
			--without-catalog
			--without-debug
		PREFIX ${LIBXML2_INSTALL_DIR}
		BUILD_COMMAND make -j5 -ks
		BUILD_IN_SOURCE 1
		BUILD_BYPRODUCTS ${LIBXML2_LIBRARIES}
	)
	# Need to make sure we include the installed versions rather than system
	# versions, but we should fall back to the source version for any other headers.
	INCLUDE_DIRECTORIES( BEFORE SYSTEM ${LIBXML2_INSTALL_DIR}/include )
	SET(LIBXML2_INCLUDE_DIR "${LIBXML2_SOURCE_DIR}/include")
ELSEIF ( PN_CLUSTER STREQUAL "rPI" )
	SET(LIBXML2_LIBRARIES "${PROJECT_DIR}/WW2/lib/rPI/${CMAKE_BUILD_TYPE}/3rdParty/libxml2.a")
	SET(LIBZ_LIBRARIES "/usr/lib/arm-linux-gnueabihf/libz.a")
	SET(LIBXML2_INCLUDE_DIR "${LIBXML2_SOURCE_DIR}/include")
	MESSAGE(STATUS "rPI : Libs: ${LIBXML2_LIBRARIES} ${LIBZ_LIBRARIES}")
ELSEIF ( WIN OR WIN32 OR WIN64 )
	FIND_LIBRARY(LIBXML2_LIBRARIES
			NAMES	libxml2 xml2
			HINTS "${PROJECT_DIR}/WW2/lib/win/${CMAKE_BUILD_TYPE}/3rdParty")
	SET(LIBXML2_INCLUDE_DIR "${LIBXML2_SOURCE_DIR}/include")
ELSE()
	FIND_PACKAGE( LibXml2 REQUIRED )
ENDIF()

INCLUDE_DIRECTORIES( ${LIBXML2_INCLUDE_DIR} )

INCLUDE_DIRECTORIES( ${LIBRARIES_DIR}/json/src )
