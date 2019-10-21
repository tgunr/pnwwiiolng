#.rst:
# FindLibEvent
# -----------
#
# Try to find the libevent library
#
# Once done this will define
#
# ::
#
#   LIBEVENT_FOUND - System has libevent
#   LIBEVENT_INCLUDE_DIR - The libevent include directory
#   LIBEVENT_LIBRARIES - The libraries needed to use libevent

FIND_PATH( LIBEVENT_INCLUDE_DIR
    NAMES event2/event.h
    PATH_SUFFIXES event2
)

FIND_LIBRARY( LIBEVENT_LIBRARY
    NAMES event-2.0-5 event-2.0 event2 event libevent-2.0-5 libevent-2.0 libevent2 libevent
)

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( LibEvent
    REQUIRED_VARS LIBEVENT_INCLUDE_DIR LIBEVENT_LIBRARY
)

MARK_AS_ADVANCED( LIBEVENT_INCLUDE_DIR LIBEVENT_LIBRARY )

SET( LIBEVENT_LIBRARIES "${LIBEVENT_LIBRARY}" )
SET( LIBEVENT_INCLUDE_DIRS "${LIBEVENT_INCLUDE_DIR}" )
