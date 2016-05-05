#.rst:
# FindReadline
# -----------
#
# Try to find the GNU Readline library
#
# Requires and will search for ncurses via FindCurses
#
# Once done this will define
#
# ::
#
#   READLINE_FOUND - System has Readline
#   READLINE_INCLUDE_DIR - The Readline include directory
#   READLINE_LIBRARIES - The libraries needed to use Readline
#   READLINE_VERSION_STRING - the version of Readline found

SET ( CURSES_NEED_NCURSES TRUE )
FIND_PACKAGE( Curses REQUIRED )

FIND_PATH( READLINE_INCLUDE_DIR
    NAMES readline/readline.h
    PATH_SUFFIXES readline
)

FIND_LIBRARY( READLINE_LIBRARY
    NAMES readline libreadline
)

# This is a bit silly.
FILE( STRINGS "${READLINE_INCLUDE_DIR}/readline/readline.h" readline_version_str
    REGEX "^#define[\t ]+RL_READLINE_VERSION[\t ]+(0x[0-9A-Fa-f]+)"
)
STRING( REGEX REPLACE
    "^#define[\t ]+RL_READLINE_VERSION[\t ]+(0x[0-9A-Fa-f]+)(.*)$" "\\1"
    readline_version_str "${readline_version_str}"
)
STRING( REGEX MATCHALL "[0-9A-Fa-f][0-9A-Fa-f]"
    readline_version_nums "${readline_version_str}"
)
SET( readline_version_result "" )
FOREACH( readline_version_num IN LISTS readline_version_nums )
    STRING( REGEX REPLACE "^0+" "" readline_version_num "${readline_version_num}" )
    STRING( APPEND readline_version_result ".${readline_version_num}" )
ENDFOREACH()
STRING( SUBSTRING "${readline_version_result}" 1 -1 READLINE_VERSION_STRING )
# End the silly

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( Readline
    REQUIRED_VARS READLINE_INCLUDE_DIR READLINE_LIBRARY
    VERSION_VAR READLINE_VERSION_STRING
)

MARK_AS_ADVANCED( READLINE_INCLUDE_DIR READLINE_LIBRARY )

SET( READLINE_LIBRARIES "${READLINE_LIBRARY}" )
SET( READLINE_INCLUDE_DIRS "${READLINE_INCLUDE_DIR}" )
