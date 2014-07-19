######################################################################

SET(SourcePCH "${CMAKE_CURRENT_SOURCE_DIR}/hostproj/syscfg.h")

# Do all of the platform-specific configuration stuff for building.
include ( cmakeincludes/HostConfiguration.cmake )

IF ( NOT PN_CLUSTER STREQUAL "Auth" )
	# Add a rule to build the Lua glue file
	add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${_LUA_HOST}Lua.cc
		COMMAND ${TOLUAPP_EXECUTABLE}
			-o  ${CMAKE_CURRENT_SOURCE_DIR}/${_LUA_HOST}Lua.cc
			-n  game
			-D
			-C
			-E  project=ww2ol
			hostproj/${_LUA_HOST}.pkg
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		MAIN_DEPENDENCY hostproj/${_LUA_HOST}.pkg
		DEPENDS
			${SourcePCH}
			hostproj/${_LUA_HOST}.pkg
			include/platforms.h
			include/systypes.h
			include/luaGlue.h
			wwii/wwiiThreads.h
			wwii/wwiiOwnershipEnums.h
			inst/instBasicStatus.h
			wwii/wwiiConfig.h
			chat/chatHandle.h
			include/precompile_includes.h
			weap/weapSpec.h
	)
ENDIF( )

CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/revision.h.in ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/revision.h )

IF (NOT MSVC)
	SET(BinaryPCH "${SourcePCH}.gch")
	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${BinaryPCH}")

	STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_name)
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

	ADD_CUSTOM_TARGET(generate_precompiled DEPENDS "${BinaryPCH}")

	MACRO(ADD_PCH _TARGET)
	 ADD_DEPENDENCIES(${_TARGET} ${BinaryPCH} generate_precompiled)
	ENDMACRO()
ELSE ( )
	MACRO(ADD_PCH _TARGET)
	  GET_TARGET_PROPERTY(_SOURCES ${_TARGET} SOURCES)
	  LIST(GET _SOURCES 0 _FIRST_SOURCE)
	  SET(BinaryPCH "\$(IntDir)\$(TargetName)-syscfg.pch")
	  SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES COMPILE_FLAGS "/Yu\"${SourcePCH}\" /FI\"${SourcePCH}\" /Fp\"${BinaryPCH}\"")
	  SET_SOURCE_FILES_PROPERTIES(${_FIRST_SOURCE} PROPERTIES COMPILE_FLAGS "/Yc\"${SourcePCH}\"")
	ENDMACRO ()
ENDIF ( )

