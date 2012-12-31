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

if(WIN32 AND MSVC)
 STRING(REGEX REPLACE "\\+" "/" ProgramFiles "$ENV{ProgramFiles}")
 STRING(REGEX REPLACE "\\+" "/" SystemDrive "$ENV{SystemDrive}")
  find_path(MYSQL_INCLUDE_DIR NAMES "mysql.h"
      PATHS
      "${ProgramFiles}/MySQL/*/include"
      "${ProgramFiles86}/MySQL/*/include"
      "${SystemDrive}/MySQL/*/include"
      )
  find_library(MYSQL_LIBRARIES NAMES "libmysql" "mysqlclient"
      PATHS
      "${ProgramFiles}/MySQL/lib"
      "${ProgramFiles}/MySQL/*/lib"
      "${ProgramFiles}/MySQL/lib/opt"
      "${ProgramFiles}/MySQL/*/lib/opt"
      "${SystemDrive}/MySQL/lib"
      "${SystemDrive}/MySQL/*/lib"
      "${SystemDrive}/MySQL/lib/opt"
      "${SystemDrive}/MySQL/*/lib/opt"
      )
else(WIN32 AND MSVC)
  find_path(MYSQL_INCLUDE_DIR NAMES "mysql.h"
	  HINTS
      "/usr/include"
      "/usr/local/include"
      PATH_SUFFIXES
      mysql mysql5 msql5.1 mysql-5 mysql-5.1 mysql5-1
      )
  find_library(MYSQL_LIBRARIES NAMES mysqlclient_r mysqlclient libmysqlclient_r libmysqlclient
      HINTS
      /usr/lib
      /usr/local/lib
      /opt/lib
      PATH_SUFFIXES
      mysql mysql5 msql5.1 mysql-5 mysql-5.1 mysql5-1
      )
  SET(MYSQL_LIBRARIES ${MYSQL_LIBRARIES} z)
endif(WIN32 AND MSVC)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MySQL DEFAULT_MSG MYSQL_LIBRARIES MYSQL_INCLUDE_DIR)

MARK_AS_ADVANCED(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)

