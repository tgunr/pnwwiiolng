PROJECT ( Lua51 )
CMAKE_MINIMUM_REQUIRED ( VERSION 2.8.3 )
INCLUDE_DIRECTORIES( include )

IF ( NOT MSVC AND PN_BUILD_32BIT AND NOT PN_CLUSTER STREQUAL "rPI" )
	SET(CMAKE_CXX_FLAGS "-m32")
	SET(CMAKE_C_FLAGS "-m32")
ENDIF ( )

SET ( LUA_CORE_SRCS
    src/lapi.c src/lcode.c src/ldebug.c src/ldo.c src/ldump.c src/lfunc.c
    src/lgc.c src/llex.c src/lmem.c src/lobject.c  src/lopcodes.c
    src/lparser.c src/lstate.c src/lstring.c src/ltable.c src/ltm.c
    src/lundump.c src/lvm.c src/lzio.c )
SET ( LUA_LIB_SRCS
    src/lauxlib.c src/lbaselib.c src/ldblib.c src/liolib.c src/lmathlib.c 
    src/loslib.c src/ltablib.c src/lstrlib.c src/loadlib.c src/linit.c )

ADD_LIBRARY ( lua51 STATIC ${LUA_CORE_SRCS} ${LUA_LIB_SRCS} )
IF(APPLE)
    SET_TARGET_PROPERTIES ( lua51
        PROPERTIES COMPILE_DEFINITIONS LUA_USE_MACOSX )
ELSEIF(MSVC)
    SET_TARGET_PROPERTIES ( lua51
        PROPERTIES COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS;LUA_USE_WINDOWS")
ELSE()
    SET_TARGET_PROPERTIES ( lua51
        PROPERTIES COMPILE_DEFINITIONS LUA_USE_LINUX )
	SET_TARGET_PROPERTIES ( lua51
		PROPERTIES COMPILE_FLAGS "-Wno-all -Wno-everything" )
	IF ( CMAKE_BUILD_TYPE MATCHES Debug )
		ADD_COMPILE_OPTIONS( "-Og" "-g3" )
	ELSE()
		ADD_COMPILE_OPTIONS( "-Os" )
	ENDIF()
ENDIF()

#ADD_EXECUTABLE ( lua lua.c lua51 )
#ADD_EXECUTABLE ( luac luac.c print.c lua51 )

IF ( MSVC )
	SET ( LUA_LIBRARIES lua51 PARENT_SCOPE )
ELSE ()
	SET ( LUA_LIBRARIES lua51 m dl PARENT_SCOPE )
ENDIF ()
SET ( LUA_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include PARENT_SCOPE )
