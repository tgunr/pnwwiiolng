# Configures the compiler for building the host given the target environment etc.

# Specify include directories.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/hostproj
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/mem
	${CMAKE_CURRENT_SOURCE_DIR}/sys
	${MYSQL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libircclient/include
	${CMAKE_CURRENT_SOURCE_DIR}/libevent/include
)

# Platform specifics.
if ( MSVC )

	include ( cmakeincludes/ConfigVisualStudio.cmake )
	include_directories(
			${CMAKE_CURRENT_SOURCE_DIR}/../../Libraries
			${CMAKE_CURRENT_SOURCE_DIR}/libevent/WIN32-Code
			)

else ( )
	include ( cmakeincludes/ConfigGCC.cmake )

	# Raknet is in /playnet/wiiol/lib.
	link_directories(${CMAKE_CURRENT_SOURCE_DIR_BINARY_DIR} /playnet/wwiiol/lib)

endif ( )

if ( _DEBUG_FLAGS AND (CMAKE_BUILD_TYPE STREQUAL "DEBUG" OR CMAKE_BUILD_TYPE STREQUAL "Debug") )
 set(_DEBUG_FLAG "${_DEBUG_FLAG} ${_DEBUG_FLAGS}")
endif ( )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_OPTIMIZATION_FLAG} ${_OPTIMIZATION_FLAGS} ${_LINKTIME_OPTIMIZATON} ${ARCHITECTURE} ${_DEBUG_FLAG} ${_CXX_FLAGS} ${_LANGUAGE_FLAGS} ${_DIAGNOSTIC_FLAGS} ${_HEAVY_OPTIMIZATIONS} ${PN_CXX_FLAGS}")
if ( PN_BUILD_TYPE MATCHES "Dev" )	# Apply to Dev and ChinaDev
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_DEV_FLAGS}")
endif ( )
#if ( _FORCE_INCLUDE_FLAG )
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_FORCE_INCLUDE_FLAG}hostproj/syscfg.h")
#endif ( )

SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_LINKER_FLAGS} ${_LINKTIME_OPTIMIZATON}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_LINKER_FLAGS} ${_LINKTIME_OPTIMIZATON}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_LINKER_FLAGS} ${_LINKTIME_OPTIMIZATON}")

if ( PN_MEM_DEBUG )
  add_definitions("-DMEMORY_DEBUG=1")
endif ( )
if ( PN_HOST_TIMERS )
  add_definitions("-DENABLE_HOST_TIMERS=1")
endif ( )

if (WIN32)
  add_definitions("-DplatOS=OS_WIN32")
else ( )
  add_definitions("-DplatOS=OS_LINUX")
endif ( )

add_definitions("-DAS_SERVER=1")

add_definitions("-DAPP_ID=${PN_APP_ID}")
