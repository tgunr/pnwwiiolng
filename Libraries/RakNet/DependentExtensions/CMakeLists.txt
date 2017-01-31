cmake_minimum_required(VERSION 2.6)

include(ExternalProject)

option( RAKNET_ENABLE_EXTENSIONS 		"Enable extension projects that might need specific dependencies." TRUE )
option( RAKNET_EXTENSION_USEMYSQL 		"Enable extensions using MySQL" 		FALSE )
option( RAKNET_EXTENSION_USEPOSTGRESQL		"Enable extensions using PostgreSQL" 		FALSE )
option( RAKNET_EXTENSION_USEBOOST		"Enable extensions using Boost" 		FALSE )
option( RAKNET_EXTENSION_USESPEEX		"Enable extensions using Speex" 		FALSE )
option( RAKNET_EXTENSION_USEIRRLICHT		"Enable extensions using Irrlicht" 		FALSE )
option( RAKNET_EXTENSION_USEIRRKLANG		"Enable extensions using Irkklang" 		FALSE )
option( RAKNET_EXTENSION_USEOGRE3D		"Enable extensions using Ogre3D" 		FALSE )
option( RAKNET_EXTENSION_Autopatcher		"Enable Autopatcher extension" 			FALSE )
option( RAKNET_EXTENSION_CATID			"Enable CatID lib extension"			TRUE )
option( RAKNET_EXTENSION_BZIP			"Enable BZIP lib extension"			FALSE )
option( RAKNET_EXTENSION_XML			"Enable XML lib extension"			FALSE )
option( RAKNET_EXTENSION_PORTAUDIO		"Enable Port Audio extension"			FALSE )


if( RAKNET_ENABLE_EXTENSIONS ) 
	if( RAKNET_EXTENSION_CATID )
		add_subdirectory( "${RakNet_SOURCE_DIR}/DependentExtensions/cat/build" )
	endif()

	if( RAKNET_EXTENSION_BZIP )
	        add_subdirectory("bzip2-1.0.6")
	endif()

	if( RAKNET_EXTENSION_XML )
		add_subdirectory("XML")
	endif()
endif()

if( RAKNET_ENABLE_EXTENSIONS )
	IF(RAKNET_EXTENSION_USEMYSQL ) 
		add_subdirectory(MySQLInterface)
	ENDIF() 

	IF(RAKNET_EXTENSION_USEPOSTGRESQL) 
		add_subdirectory(PostgreSQLInterface)
	ENDIF() 

endif()

if( RAKNET_ENABLE_EXTENSIONS ) 
	if( RAKNET_EXTENSION_Autopatcher )
		add_subdirectory(Autopatcher)
	endif()
endif()

if( RAKNET_ENABLE_EXTENSIONS ) 
	IF (WIN32 AND NOT UNIX)
		IF(NOT ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")
			IF(RAKNET_EXTENSION_PORTAUDIO)
				add_subdirectory(portaudio_v18_1)
			ENDIF()
		ENDIF(NOT ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")
		IF (RAKNET_EXTENSION_USESPEEX)
			add_subdirectory(speex-1.1.12)
		ENDIF()
	ENDIF(WIN32 AND NOT UNIX)
endif()

if( RAKNET_ENABLE_EXTENSIONS ) 

	IF(RAKNET_EXTENSION_USEBOOST) 
		add_subdirectory(RPC3)
	ENDIF() 

endif()

IF(NOT WIN32 AND UNIX)
	IF(RAKNET_EXTENSION_USESPEEX AND RAKNET_ENABLE_EXTENSIONS) 
		FINDSPEEX()
		include_directories(${RAKNETHEADERFILES} ${SPEEX_INCLUDE_DIRS})
		add_library(LibRakVoice STATIC RakVoice.h RakVoice.cpp)
		target_link_libraries(LibRakVoice ${RAKNET_COMMON_LIBS} ${SPEEX_LIBRARIES})
	ENDIF() 
ENDIF()

if( RAKNET_ENABLE_EXTENSIONS )

	IF( RAKNET_EXTENSION_USEIRRLICHT AND RAKNET_EXTENSION_USEIRRKLANG ) 
		add_subdirectory(IrrlichtDemo)
	ENDIF()

	IF( RAKNET_EXTENSION_USEOGRE3D ) 
		add_subdirectory(Ogre3DInterpDemo)
	ENDIF()

endif()

