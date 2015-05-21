# Checks for platform functions/features.

include(CheckIncludeFiles)
include(CheckFunctionExists)

CHECK_INCLUDE_FILES(signal.h HAVE_SIGNAL_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)

IF ( NOT MSVC )
	# These generally don't exist under Windows, if you have a version of them,
	# move the check out of the if.
	CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
	CHECK_INCLUDE_FILES(sys/time.h HAVE_SYS_TIME_H)
	CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
	CHECK_INCLUDE_FILES(arpa/inet.h HAVE_ARPA_INET_H)
	CHECK_INCLUDE_FILES(sys/sendfile.h HAVE_SYS_SENDFILE_H)
	CHECK_INCLUDE_FILES(sys/param.h HAVE_SYS_PARAM_H)

	CHECK_FUNCTION_EXISTS(gethostbyname_r HAVE_GETHOSTBYNAME_R)
	SET(HAVE_SIGACTION 1)
ENDIF ( )

IF ( UNIX )
 CHECK_FUNCTION_EXISTS(strtoull HAVE_STRTOULL)
 IF ( NOT HAVE_STRTOULL )
   MESSAGE(FATAL_ERROR "No strtoull function")
 ENDIF ( )
ENDIF ( )

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/hostproj/config.h.cmake.in ${CMAKE_CURRENT_SOURCE_DIR}/hostproj/config.h)

add_definitions("-DHAVE_CONFIG_H=1")

IF (WIN32 OR WIN64)
	LINK_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../lib/win")
ENDIF ( )
