# Heavily based on CMake's vendor-supplied FindPostgreSQL.cmake. Original was
# distributed under the OSI-approved BSD 3-Clause License.  See
# https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindMySQL
--------------

Find the MySQL installation.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``MySQL::MySQL``
if MySQL has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``MySQL_FOUND``
  True if MySQL is found.
``MySQL_LIBRARIES``
  the MySQL libraries needed for linking
``MySQL_INCLUDE_DIRS``
  the directories of the MySQL headers
``MySQL_LIBRARY_DIRS``
  the link directories for MySQL libraries
``MySQL_VERSION_STRING``
  the version of MySQL found
#]=======================================================================]

# ----------------------------------------------------------------------------
# History:
# This module is derived from the official CMake module, which was in turn
# derived from the module originally found in the VTK source tree.
#
# ----------------------------------------------------------------------------
# Note:
# MySQL_ADDITIONAL_VERSIONS is a variable that can be used to set the version
# number of the implementation of MySQL.
# In Windows the default installation of MySQL uses that as part of the path.
# E.g C:\Program Files\MySQL\MySQL Server 5.7.
# Currently, the following version numbers are known to this module:
# "8.0" "5.7" "5.6" "5.5" "5.1" "5.0" "5" "4.1"
#
# To use this variable just do something like this:
# set(MySQL_ADDITIONAL_VERSIONS "5.7" "8.0")
# before calling find_package(MySQL) in your CMakeLists.txt file.
# This will mean that the versions you set here will be found first in the order
# specified before the default ones are searched.
#
# ----------------------------------------------------------------------------
# Note:
# You may set the MySQL_FORCE_STATIC_LINK variable to ON to force use of the
# static MySQL client library even when a shared library is available. On UNIX
# systems, this causes the search mechanism to ignore .dylib or .so files in
# favor of .a files when searching for the client library. On Windows, the
# search will look only for mysqlclient.lib instead of searching for
# libmysql.dll first. If no static library is found when this variable is on,
# MySQL is treated as not found, even if a dynamic version is available.
#
# ----------------------------------------------------------------------------
# You may need to manually set:
#  MySQL_INCLUDE_DIR  - the path to where the MySQL include files are.
#  MySQL_LIBRARY_DIR  - The path to where the MySQL library files are.
# If FindMySQL.cmake cannot find the include files or the library files.
#
# ----------------------------------------------------------------------------
# The following variables are set if MySQL is found:
#  MySQL_FOUND         - Set to true when MySQL is found.
#  MySQL_INCLUDE_DIRS  - Include directories for MySQL
#  MySQL_LIBRARY_DIRS  - Link directories for MySQL libraries
#  MySQL_LIBRARIES     - The MySQL libraries.
#
# The ``MySQL::MySQL`` imported target is also created.
#
# ----------------------------------------------------------------------------
# If you have installed MySQL in a non-standard location.
# (Please note that in the following comments, it is assumed that <Your Path>
# points to the root directory of the include directory of MySQL.)
# Then you have three options.
# 1) After CMake runs, set MySQL_INCLUDE_DIR to <Your Path>/include and
#    MySQL_LIBRARY_DIR to wherever the library libmysqlclient (or libmysql in Windows) is
# 2) Use CMAKE_INCLUDE_PATH to set a path to <Your Path>/MySQL<-version>. This will allow find_path()
#    to locate MySQL_INCLUDE_DIR by utilizing the PATH_SUFFIXES option. e.g. In your CMakeLists.txt file
#    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "<Your Path>/include")
# 3) Set an environment variable called ${MySQL_ROOT} that points to the root of where you have
#    installed MySQL, e.g. <Your Path>.
#
# ----------------------------------------------------------------------------

set(MySQL_INCLUDE_PATH_DESCRIPTION "top-level directory containing the MySQL include directories. E.g /usr/local/include/mysql or C:/Program Files/MySQL/MySQL Server 5.7/include")
set(MySQL_INCLUDE_DIR_MESSAGE "Set the MySQL_INCLUDE_DIR cmake cache entry to the ${MySQL_INCLUDE_PATH_DESCRIPTION}")
set(MySQL_LIBRARY_PATH_DESCRIPTION "top-level directory containing the MySQL libraries.")
set(MySQL_LIBRARY_DIR_MESSAGE "Set the MySQL_LIBRARY_DIR cmake cache entry to the ${MySQL_LIBRARY_PATH_DESCRIPTION}")
set(MySQL_ROOT_DIR_MESSAGE "Set the MySQL_ROOT system variable to where MySQL is found on the machine E.g C:/Program Files/MySQL/MySQL Server 5.7")


set(MySQL_KNOWN_VERSIONS ${MySQL_ADDITIONAL_VERSIONS}
    "8.0" "5.7" "5.6" "5.5" "5.1" "5.0" "5" "4.1")

# Define additional search paths for root directories.
set( MySQL_ROOT_DIRECTORIES
   ENV MySQL_ROOT
   ${MySQL_ROOT}
)
foreach(suffix ${MySQL_KNOWN_VERSIONS})
  if(WIN32)
    list(APPEND MySQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
        "MySQL/MySQL Server ${suffix}/lib")
    list(APPEND MySQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
        "MySQL/MySQL Server ${suffix}/include")
  endif()
  if(UNIX)
    list(APPEND MySQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
        "mysql-${suffix}/lib")
    list(APPEND MySQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
        "mysql-${suffix}/include")
  endif()
endforeach()

#
# Look for an installation.
#
find_path(MySQL_INCLUDE_DIR
  NAMES mysql.h
  PATHS
   # Look in other places.
   ${MySQL_ROOT_DIRECTORIES}
  PATH_SUFFIXES
    mysql
    include
    ${MySQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES}
  # Help the user find it if we cannot.
  DOC "The ${MySQL_INCLUDE_DIR_MESSAGE}"
)

# The MySQL library.
if (WIN32)
    set (MySQL_LIBRARIES_TO_FIND mysqlclient.lib)
    if( NOT MySQL_FORCE_STATIC_LINK )
        list(INSERT MySQL_LIBRARIES_TO_FIND 0 libmysql)
    endif()
else()
    list (APPEND MySQL_LIBRARIES_TO_FIND libmysqlclient_r.a libmysqlclient.a)
    if( NOT MySQL_FORCE_STATIC_LINK )
        list(INSERT MySQL_LIBRARIES_TO_FIND 0 mysqlclient_r mysqlclient)
    endif()
endif()

find_library(MySQL_LIBRARY
 NAMES ${MySQL_LIBRARIES_TO_FIND}
 PATHS
   ${MySQL_ROOT_DIRECTORIES}
 PATH_SUFFIXES
   lib
   ${MySQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES}
 # Help the user find it if we cannot.
 DOC "The ${MySQL_LIBRARY_DIR_MESSAGE}"
)
get_filename_component(MySQL_LIBRARY_DIR ${MySQL_LIBRARY} PATH)

if (MySQL_INCLUDE_DIR)
  file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.cxx"
       "#include <mysql.h>\n#include <stdio.h>\nint main() { printf(\"%s\\n\", LIBMYSQL_VERSION); }")
  try_run(_MySQL_VERSION_RUN_RESULT _MySQL_VERSION_COMPILE_RESULT
          ${CMAKE_BINARY_DIR}
          ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.cxx
          CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${MySQL_INCLUDE_DIR}
          RUN_OUTPUT_VARIABLE _MySQL_VERSION_STRING)
  if (_MySQL_VERSION_COMPILE_RESULT AND NOT _MySQL_VERSION_RUN_RESULT STREQUAL FAILED_TO_RUN)
    string(STRIP "${_MySQL_VERSION_STRING}" MySQL_VERSION_STRING)
  endif()
endif()

# Did we find anything?
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
                                  REQUIRED_VARS MySQL_LIBRARY MySQL_INCLUDE_DIR
                                  VERSION_VAR MySQL_VERSION_STRING)
set(MySQL_FOUND  ${MYSQL_FOUND})

# Now try to get the include and library path.
if(MySQL_FOUND)
  if (NOT TARGET MySQL::MySQL)
    add_library(MySQL::MySQL UNKNOWN IMPORTED)
    set_target_properties(MySQL::MySQL PROPERTIES
      IMPORTED_LOCATION "${MySQL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${MySQL_INCLUDE_DIR}")
  endif ()
  set(MySQL_INCLUDE_DIRS ${MySQL_INCLUDE_DIR} )
  set(MySQL_LIBRARY_DIRS ${MySQL_LIBRARY_DIR} )
  set(MySQL_LIBRARIES ${MySQL_LIBRARY})
endif()

mark_as_advanced(MySQL_INCLUDE_DIR MySQL_LIBRARY )
