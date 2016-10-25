INCLUDE( CheckIncludeFiles )
INCLUDE( CheckFunctionExists )

# Many of these will fail on Windows; that's okay
CHECK_INCLUDE_FILES( signal.h HAVE_SIGNAL_H )
CHECK_INCLUDE_FILES( stdlib.h HAVE_STDLIB_H )
CHECK_INCLUDE_FILES( unistd.h HAVE_UNISTD_H )
CHECK_INCLUDE_FILES( sys/time.h HAVE_SYS_TIME_H )
CHECK_INCLUDE_FILES( syslog.h HAVE_SYSLOG_H )
CHECK_INCLUDE_FILES( arpa/inet.h HAVE_ARPA_INET_H )
CHECK_INCLUDE_FILES( sys/sendfile.h HAVE_SYS_SENDFILE_H )
CHECK_INCLUDE_FILES( sys/param.h HAVE_SYS_PARAM_H )

# Ditto for these
CHECK_FUNCTION_EXISTS( gethostbyname_r HAVE_GETHOSTBYNAME_R )
CHECK_FUNCTION_EXISTS( sigaction HAVE_SIGACTION )
CHECK_FUNCTION_EXISTS( strtoull HAVE_STRTOULL )

# Generate the config.h and tell the source code that it's there to be used
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/config.h.cmake.in
    ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/config.h
)
ADD_DEFINITIONS( "-DHAVE_CONFIG_H=1" )

# Windows needs an additional library search path
IF ( WIN32 OR WIN64 )
	LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/../lib/win" )
ENDIF()
