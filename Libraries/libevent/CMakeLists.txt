CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (LibEvent)

IF ( WIN32 OR WIN64 )
	INCLUDE_DIRECTORIES(${LibEvent_SOURCE_DIR}/WIN32-Code)
	SET(LibEvent_C_FLAGS "/TP /W1 /wd4127 /wd4100 /wd4706")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LibEvent_C_FLAGS}")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LibEvent_C_FLAGS}")
ELSE ( )
	ADD_DEFINITIONS(-DHAVE_CONFIG_H)
ENDIF ( )

INCLUDE_DIRECTORIES(
	${LibEvent_SOURCE_DIR}/compat
	${LibEvent_SOURCE_DIR}/include
	${LibEvent_SOURCE_DIR}
	)

SET(LIBEVENT_SOURCES
		buffer.c
		bufferevent.c
		bufferevent_filter.c
		bufferevent_pair.c
		bufferevent_ratelim.c
		bufferevent_sock.c
		evdns.c
		event.c
		event_tagging.c
		evmap.c
		evthread.c
		evutil.c
		evutil_rand.c
		listener.c
		log.c
		signal.c
		)

SET(LIBEVENT_HEADERS
		event.h
		evdns.h
		evutil.h
		bufferevent-internal.h
		evbuffer-internal.h
		event-internal.h
		evmap-internal.h
		util-internal.h
		log-internal.h
		evthread-internal.h
		strlcpy-internal.h

		include/event2/buffer.h
		include/event2/buffer_compat.h
		include/event2/bufferevent.h
		include/event2/dns.h
		include/event2/util.h
		)

IF (WIN32 OR WIN64)
	SET(LIBEVENT_WIN_SOURCES
			${LibEvent_SOURCE_DIR}/WIN32-Code/event2/event-config.h
			bufferevent_async.c
			buffer_iocp.c
			event_iocp.c
			evthread_win32.c
			win32select.c
			strlcpy.c
			)
ENDIF ()

INCLUDE( CheckIncludeFiles )
INCLUDE( CheckFunctionExists )
CHECK_FUNCTION_EXISTS( select _EVENT_HAVE_SELECT )
IF (_EVENT_HAVE_SELECT)
	SET(LIBEVENT_SELECT_SOURCES select.c)
ENDIF ()

CHECK_INCLUDE_FILES( poll.h _EVENT_HAVE_POLL_H )
IF (_EVENT_HAVE_POLL_H)
	SET(LIBEVENT_POLL_SOURCES poll.c)
ENDIF ()

CHECK_INCLUDE_FILES( sys/epoll.h _EVENT_HAVE_SYS_EPOLL_H )
IF (_EVENT_HAVE_SYS_EPOLL_H)
	SET(LIBEVENT_EPOLL_SOURCES epoll.c epoll_sub.c)
ENDIF ()

CHECK_INCLUDE_FILES( sys/devpoll.h _EVENT_HAVE_SYS_DEVPOLL_H )
IF (_EVENT_HAVE_SYS_DEVPOLL_H)
	SET(LIBEVENT_DEVPOLL_SOURCES devpoll.c)
ENDIF ()

CHECK_INCLUDE_FILES( signal.h _EVENT_HAVE_SIGNAL_H )
IF (_EVENT_HAVE_SIGNAL_H)
	SET(LIBEVENT_SIGNAL_SOURCES signal.c)
ENDIF ()

IF (_EVENT_HAVE_EVENT_PORTS)
	SET(LIBEVENT_EVPORT_SOURCES evport.c)
ENDIF ()

IF (_EVENT_HAVE_WORKING_KQUEUE)
	SET(LIBEVENT_KQUEUE_SOURCES kqueue.c)
ENDIF ()

CHECK_FUNCTION_EXISTS( strlcpy _EVENT_HAVE_STRLCPY )
IF (NOT _EVENT_HAVE_STRLCPY)
	SET(LIBEVENT_STRLCPY_SOURCES strlcpy.c)
ENDIF ()

ADD_LIBRARY(event ${LIBEVENT_SOURCES} ${LIBEVENT_WIN_SOURCES} ${LIBEVENT_SELECT_SOURCES} ${LIBEVENT_POLL_SOURCES} ${LIBEVENT_EPOLL_SOURCES} ${LIBEVENT_DEVPOLL_SOURCES} ${LIBEVENT_SIGNAL_SOURCES} ${LIBEVENT_EVPORT_SOURCES} ${LIBEVENT_KQUEUE_SOURCES} ${LIBEVENT_STLRCPY_SOURCES} ${LIBEVENT_HEADERS} ${LIBEVENT_WIN_HEADERS} ${LIBEVENT_STRLCPY_SOURCES})
SET_TARGET_PROPERTIES(event PROPERTIES COMPILER_LANGUAGE CXX LINKER_LANGUAGE CXX LANGUAGE CXX)
