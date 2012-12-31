#include <syscfg.h> // Path is project specific.

#include <string>
#include <set>
#include "mem/mem.h"
#include "str/strpool.h"
#include "pi/pi.h"

#if platOS == OS_WIN32
#include <hash_set>
#endif

/************************************************************************************************
	strpool
*************************************************************************************************/

static char EMPTY_STR[] = "";

#define NEW_STRPOOL

#if platOS == OS_WIN32
typedef stdext::hash_set<std::string> StringSet;
#else
typedef std::set<std::string> StringSet;
#endif

static StringSet strings;
static size_t callcount = 0;
static size_t foundcount = 0;

#ifdef NEW_STRPOOL
const char* strpool(const char* s)
{
	if(*s == '\0') return EMPTY_STR;

  callcount++;

  std::pair<StringSet::iterator,bool> p = strings.insert(s); // returns either new string if inserted or existing

  if ( ! p.second )
  {
    foundcount++;
  }

  int setsize = strings.size();

  const char* result = (*p.first).c_str();

  return result;
}

void strpoolKill()
{
  // not called
}

#else

#define MAX_POOL_STRS 16384

static char* strings[MAX_POOL_STRS];
static int numStr = 0;

/************************************************************************************************
	strpool
*************************************************************************************************/

/*	Might redo this with a more clever optimization scheme.
 Right now is optimized for the case when identical strings
 are all added around the same time, hence the search starts
 at the end of the list. */

const char* strpool(const char* s)
  {
	int i;
	char* found = NULL;
	char** sl = (numStr > 0) ? strings + numStr - 1 : strings;

	if(*s == '\0') return EMPTY_STR;

	// start search at the* end* of the list and work backwards.
	for(i = numStr; i > 0; i--, sl--)
	{
		if(**sl == *s && !strcmp(*sl, s))
		{
			found = *sl;
			break;
		}
	}

	if(!found && numStr < MAX_POOL_STRS)
	{
		found = (char*)memAlloc(strlen(s) + 1);
		if(found)
		{
			strcpy(found, s);
			strings[numStr] = found;
			numStr++;
		}
	}

	if(!found)
		piExitProgram(__FILE__, __LINE__, "strpool exhausted");

	return found;
}

/************************************************************************************************
	strpoolKill
*************************************************************************************************/

void strpoolKill()
{
	int i;

	for(i = 0; i < numStr; i++)
	{
		if(strings[i])
		{
			memFree(strings[i]);
			strings[i] = NULL;
		}
	}
	numStr = 0;
}
#endif
