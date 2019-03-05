INCLUDE( CheckIncludeFiles )
INCLUDE( CheckFunctionExists )

# Generate the config.h and tell the source code that it's there to be used
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/config.h.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)
ADD_DEFINITIONS( "-DHAVE_CONFIG_H=1" )
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )
