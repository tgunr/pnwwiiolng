PROJECT ( Lua51 )
CMAKE_MINIMUM_REQUIRED ( VERSION 3.12.0 )

INCLUDE_DIRECTORIES ( include )

# Sources required for building the core liblua
SET ( LUA_CORE_SRCS
    src/lapi.c src/lcode.c src/ldebug.c src/ldo.c src/ldump.c src/lfunc.c
    src/lgc.c src/llex.c src/lmem.c src/lobject.c  src/lopcodes.c
    src/lparser.c src/lstate.c src/lstring.c src/ltable.c src/ltm.c
    src/lundump.c src/lvm.c src/lzio.c )

# Additional sources required to include Lua's builtin libs
SET ( LUA_LIB_SRCS
    src/lauxlib.c src/lbaselib.c src/ldblib.c src/liolib.c src/lmathlib.c
    src/loslib.c src/ltablib.c src/lstrlib.c src/loadlib.c src/linit.c )

# Additional sources required for building the lua executable
SET ( LUA_EXECUTABLE_SRCS
	src/lua.c
)

# Additional sources required for building the luac executable
SET ( LUAC_EXECUTABLE_SRCS
	src/luac.c src/print.c
)

####
## LibLua
###

# The liblua target
ADD_LIBRARY ( lua51 STATIC
	${LUA_CORE_SRCS}
	${LUA_LIB_SRCS}
)

# Include directories (for both liblua and its users)
TARGET_INCLUDE_DIRECTORIES ( lua51 PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Preprocessor definitions for both liblua and its users
TARGET_COMPILE_DEFINITIONS ( lua51 PUBLIC
	"$<$<PLATFORM_ID:Darwin>:LUA_USE_MACOSX>"
	"$<$<PLATFORM_ID:Linux>:LUA_USE_LINUX>"
	"$<$<PLATFORM_ID:Windows>:LUA_USE_WINDOWS>"
)

# Preprocessor definitions for just liblua
TARGET_COMPILE_DEFINITIONS ( lua51 PRIVATE
	"$<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>"
)

# Compilation settings for liblua
# Note that CMake handles configuring apropos debug and optimization settings
TARGET_COMPILE_OPTIONS ( lua51 PRIVATE
	# Always disable warnings
	"$<IF:$<PLATFORM_ID:Windows>,/W0,-w>"
)

####
## Lua executables
####

#ADD_EXECUTABLE ( lua
#	${LUA_EXECUTABLE_SRCS}
#)
#
#TARGET_LINK_LIBRAREIS ( lua PRIVATE
#	lua51
#)

#ADD_EXECUTABLE ( luac
#	#{LUAC_EXECUTABLE_SRCS}
#)
#
#TARGET_LINK_LIBRAREIS ( lua PRIVATE
#	lua51
#)

####
## Output
####

# The lua51 target already exports everything clients need to use it, as long as
# they know the target name. For historical reasons, provide variables to the
# parent scope anyway.
SET ( LUA_LIBRARIES lua51 PARENT_SCOPE )
SET ( LUA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE )

MARK_AS_ADVANCED ( LUA_LIBRARIES LUA_INCLUDE_DIR )
