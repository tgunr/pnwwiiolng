#include <syscfg.h> // Path is project specific.

#include <string>
#include "str/strpool.h"

#include <unordered_set>

/************************************************************************************************
	strpool
*************************************************************************************************/

static char EMPTY_STR[] = "";

typedef std::unordered_set<std::string> StringSet;

static StringSet strings;
#ifdef STRPOOL_DEBUG
static size_t callcount = 0;
static size_t foundcount = 0;
#endif

const char* strpool(const char* s)
{
	if(PN_UNLIKELY(*s == '\0'))
		return EMPTY_STR;

#ifdef STRPOOL_DEBUG
	callcount++;
#endif

	std::pair<StringSet::iterator,bool> p = strings.insert(s); // returns either new string if inserted or existing

#ifdef STRPOOL_DEBUG
	if ( ! p.second )
		foundcount++;
#endif

	return (*p.first).c_str();
}

void strpoolKill()
{
#ifdef STRPOOL_DEBUG
	dbugLog(_FLD, "strpool: %" PRSizeT " strings, %" PRSizeT " calls, %" PRSizeT " matches\n", strings.size(), callcount, foundcount);
#endif
}
