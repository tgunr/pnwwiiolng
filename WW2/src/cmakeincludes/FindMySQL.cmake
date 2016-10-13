## Begin original module leader and license:
# - Find MySQL
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
## End original module leader and license

SET( MYSQL_FOUND 0 )

# OS X has no embedded prebuilts; always use system version there
IF ( PN_USE_SYSTEM_MYSQL OR APPLE )
    # Original module's search-the-system logic goes until the end of this block
	IF (WIN32)
		STRING(REGEX REPLACE "\\+" "/" SystemDrive "$ENV{SystemDrive}")
		STRING(REGEX REPLACE "\\+" "/" ProgramFiles "$ENV{ProgramFiles}")
		IF (CMAKE_BITNESS EQUAL 64)
			# At time of writing: Windows cmake is built as a 32-bit binary,
			# so it sees "Program Files (x86)" even when building 64-bit.
			STRING(REGEX REPLACE " *\\(x86\\)" "" ProgramFiles "${ProgramFiles}")
		ENDIF ()

		FIND_PATH(
			MYSQL_INCLUDE_DIR
			NAMES "mysql.h"
			PATHS
					"${ProgramFiles}/MySQL/MySQL Connector C*/include"
					"${ProgramFiles}/MySQL/MySQL Server 5.6/include"
					"${ProgramFiles}/MySQL/MySQL Server 5.5/include"
					"${ProgramFiles}/MySQL/MySQL Server 5.1/include"
					"${ProgramFiles}/MySQL/MySQL Server 5.0/include"
					"${ProgramFiles}/MySQL/MySQL Server 5/include"
					"${ProgramFiles}/MySQL/include"
					"${SystemDrive}/MySQL/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.6;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.5;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.1;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.0;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.6;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.5;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.1;Location]/include"
					"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.0;Location]/include"
					"${ProgramFiles}/MySQL/*/include"
					"${SystemDrive}/MySQL/*/include"
			)

		FIND_LIBRARY(
			MYSQL_LIBRARIES
			NAMES "libmysql" "mysqlclient"
			PATHS
				"${ProgramFiles}/MySQL/MySQL Connector C*/lib/opt"
				"${MYSQL_INCLUDE_DIR}/../lib"
				"${MYSQL_INCLUDE_DIR}/../lib/opt"
				"${ProgramFiles}/MySQL/MySQL Server 5.6/lib/opt"
				"${ProgramFiles}/MySQL/MySQL Server 5.6/lib"
				"${ProgramFiles}/MySQL/MySQL Server 5.5/lib/opt"
				"${ProgramFiles}/MySQL/MySQL Server 5.5/lib"
				"${ProgramFiles}/MySQL/MySQL Server 5.1/lib/opt"
				"${ProgramFiles}/MySQL/MySQL Server 5.1/lib"
				"${ProgramFiles}/MySQL/MySQL Server 5.0/lib/opt"
				"${ProgramFiles}/MySQL/MySQL Server 5.0/lib"
				"${ProgramFiles}/MySQL/MySQL Server 5/lib/opt"
				"${ProgramFiles}/MySQL/MySQL Server 5/lib"
				"${ProgramFiles}/MySQL/lib/opt"
				"${ProgramFiles}/MySQL/lib"
				"${SystemDrive}/MySQL/lib/opt"
				"${SystemDrive}/MySQL/lib/debug"
				"${SystemDrive}/MySQL/lib/opt"
				"${SystemDrive}/MySQL/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.6;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.6;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.5;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.5;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.1;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.1;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.0;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\MySQL AB\\MySQL Server 5.0;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.6;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.6;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.5;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.5;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.1;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.1;Location]/lib/opt"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.0;Location]/lib"
				"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\MySQL AB\\MySQL Server 5.0;Location]/lib/opt"
				"${ProgramFiles}/MySQL/* Server */lib/opt"
				"${SystemDrive}/MySQL/* Server */lib/opt"
				"${ProgramFiles}/MySQL/* Server */lib/"
				"${SystemDrive}/MySQL/* Server */lib/"
				"${ProgramFiles}/MySQL/* Server */lib/debug"
				"${SystemDrive}/MySQL/* Server */lib/debug"
				"c:/msys/local/include"
		)
	ELSE ()
		FIND_PATH(
			MYSQL_INCLUDE_DIR
			NAMES "mysql.h"
			PATHS
					/usr/include
					/usr/include/mysql
					/usr/local/include
					/usr/local/include/mysql
					/usr/local/mysql/include
		)
		FIND_LIBRARY(
			MYSQL_LIBRARIES
			NAMES mysqlclient_r mysqlclient libmysqlclient_r libmysqlclient
			HINTS
				/usr/lib
				/usr/lib/mysql
				/usr/local/lib
				/usr/local/lib/mysql
				/usr/local/mysql/lib
				/opt/lib
				/usr/lib64
			PATH_SUFFIXES
				mysql
				mysql5.6
				mysql5-6
				mysql-5.6
				mysql5.5
				mysql5-5
				mysql-5.5
				mysql5.1
				mysql5-1
				mysql-5.1
				mysql5
				mysql-5
		)
		SET(MYSQL_LIBRARIES ${MYSQL_LIBRARIES} dl)
	ENDIF ()
ELSE()
    # Our use-embedded-library logic
	FIND_PATH( MYSQL_INCLUDE_DIR NAMES "mysql.h" PATHS ${LIBRARIES_DIR}/mysql/include )
	IF ( WIN32 )
		FIND_LIBRARY( MYSQL_LIBRARIES
			NAMES libmysql mysqlclient
			HINTS ${LIBRARIES_DIR}/mysql/lib/win${BUILD_BITNESS}
		)
	ELSE()
		FIND_LIBRARY( MYSQL_LIBRARIES
			NAMES mysqlclient_r libmysqlclient_r
			HINTS ${LIBRARIES_DIR}/mysql/lib/lin${BUILD_BITNESS}
		)
		SET( MYSQL_LIBRARIES ${MYSQL_LIBRARIES} dl )
	ENDIF()
ENDIF()

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( MySQL DEFAULT_MSG MYSQL_LIBRARIES MYSQL_INCLUDE_DIR )

MARK_AS_ADVANCED( MYSQL_INCLUDE_DIR MYSQL_LIBRARIES )
