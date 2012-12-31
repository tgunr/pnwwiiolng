#include "event2/event-config.h"
#include <winsock2.h>
#include <windows.h>
#include <io.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _EVENT_HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef _EVENT_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>
#include <errno.h>
#include <tchar.h>
#include <ctype.h>

#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/buffer_compat.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_compat.h"
#include "event2/bufferevent_struct.h"
#include "event2/thread.h"
#include "event2/util.h"

#include "bufferevent-internal.h"
