######################################################################

SET(PCHSourceFile "syscfg.h")
SET(PCHSourceDir  "${CMAKE_CURRENT_SOURCE_DIR}/hostproj")
SET(SourcePCH ${PCHSourceDir}/${PCHSourceFile})

# Do all of the platform-specific configuration stuff for building.
include ( cmakeincludes/HostConfiguration.cmake )

IF ( NOT PN_CLUSTER STREQUAL "Auth" )
	# Add a rule to build the Lua glue file
	SET(_LUA_CC "${CMAKE_CURRENT_BINARY_DIR}/${_LUA_HOST}Lua.cc")
	ADD_CUSTOM_COMMAND(OUTPUT ${_LUA_CC}
		COMMAND ${TOLUAPP_EXECUTABLE}
			-o  ${_LUA_CC}
			-n  game
			-D
			-C
			-E  project=ww2ol
			hostproj/${_LUA_HOST}.pkg
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		MAIN_DEPENDENCY hostproj/${_LUA_HOST}.pkg
		DEPENDS
            ${TOLUAPP_EXECUTABLE}
            ${TOLUAPP_LIBRARIES}
			${PCHSourceDir}/${PCHSourceFile}
			hostproj/${_LUA_HOST}.pkg
			hostproj/hostversion.h
			include/platforms.h
			inst/instDescriptor.h
			wwii/wwiiPlayer.h
			server/serverIndexes.h
			wwii/wwiiIRC.h
			server/serverPeers.h
			wwii/wwiiServerTypeEnums.h
			teul/teulConfigVar.h
			include/precompile_includes.h
			sys/sysWinISO.h
			include/systypes.h
			include/defs.h
			dbug/dbug.h
			dbug/dbugExtErrors.h
			wwii/wwiiHash.h
			chat/chatHandle.h
			wwii/wwiiOwnershipEnums.h
			wwii/wwiiTypes.h
			wwii/wwiiBranchEnums.h
			teul/teulTime.h
			teul/teulClosingEnums.h
			teul/teulKit.h
			teul/teulBasic.h
			teul/teulPackers.h
			wwii/wwiiFullObjID.h
			weap/weapSpec.h
			wwii/wwiiPermissions.h
			inst/instTypes.h
			teul/teulPacket.h
			weap/weapWeapon.h
			teul/teulEndpoint.h
			chat/chatLStringIDs.h
			str/stringLib.h
	)
ENDIF( )

CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/revision.h.in ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/revision.h )

IF (NOT MSVC)
  IF (IS_CLANG OR PN_PCH)
	SET(BinaryPCH ${CMAKE_CURRENT_BINARY_DIR}/${PCHSourceFile}.gch)

	STRING(TOUPPER CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} _flags_name)
	SET(_pchFlags ${CMAKE_CXX_FLAGS})
	LIST(APPEND _pchFlags ${${_flags_name}})

	GET_DIRECTORY_PROPERTY(_pchIncludePaths INCLUDE_DIRECTORIES)
	FOREACH (_inc ${_pchIncludePaths})
		LIST(APPEND _pchFlags "-I" ${_inc})
	ENDFOREACH()

	GET_DIRECTORY_PROPERTY(_pchDefines DEFINITIONS)
	LIST(APPEND _pchFlags ${_pchDefines})
	SEPARATE_ARGUMENTS(_pchFlags)
	ADD_CUSTOM_COMMAND(
			PRE_BUILD
			OUTPUT ${BinaryPCH}
			COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} ${_pchFlags} -o ${BinaryPCH} ${SourcePCH}
			MAIN_DEPENDENCY ${SourcePCH}
			IMPLICIT_DEPENDS CXX ${SourcePCH}
			DEPENDS ${SourcePCH}
	)
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${BinaryPCH})

	ADD_CUSTOM_TARGET(precompiled_header DEPENDS ${BinaryPCH})

	MACRO(ADD_PCH _TARGET)
	 ADD_DEPENDENCIES(${_TARGET} precompiled_header)
	ENDMACRO()
  ELSE ( )
	MACRO(ADD_PCH _TARGET)
  	ENDMACRO()
  ENDIF ( )
ELSE ( )
	MACRO(ADD_PCH _TARGET)
	  GET_TARGET_PROPERTY(_SOURCES ${_TARGET} SOURCES)
	  LIST(GET _SOURCES 0 _FIRST_SOURCE)
	  SET(BinaryPCH "\$(IntDir)\$(TargetName)-syscfg.pch")
	  SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES COMPILE_FLAGS "/Yu\"${SourcePCH}\" /FI\"${SourcePCH}\" /Fp\"${BinaryPCH}\"")
	  SET_SOURCE_FILES_PROPERTIES(${_FIRST_SOURCE} PROPERTIES COMPILE_FLAGS "/Yc\"${SourcePCH}\"")
	ENDMACRO ()
ENDIF ( )
