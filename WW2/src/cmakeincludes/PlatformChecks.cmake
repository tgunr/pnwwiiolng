# Generate the config.h and tell the source code that it's there to be used
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/config.h.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )

# Windows needs an additional library search path
IF ( WIN32 )
	LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/../lib/win" )
ENDIF()
