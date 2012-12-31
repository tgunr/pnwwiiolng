#include <syscfg.h> // Path is project specific.

#include "str/striws.h"
#include <ctype.h>

/************************************************************************************************
	strcmpiws
*************************************************************************************************/
// (iws) Ignore White Space and case insensitive

int strcmpiws(const char* str1, const char* str2)
{
	char c1, c2;
	int i, j;

	for(i = 0, j = 0;;)
	{
		for(;str1[i]; i++)
		{
			if(!isspace(str1[i])) break;
		}

		for(;str2[j]; j++)
		{
			if(!isspace(str2[j])) break;
		}

		c1 = tolower(str1[i++]);
		c2 = tolower(str2[j++]);

		if(c1 != c2) break;
		if (!c1 || !c2) return 0;
	}
	return(c1 - c2);
}

/************************************************************************************************
	strstriws
*************************************************************************************************/

char* strstriws(const char* str1, const char* str2)
{
	char c1, c2;
	int i, j;

	for(i = 0, j = 0;;)
	{
		for(;str1[i]; i++)
		{
			if(!isspace(str1[i])) break;
		}

		for(;str2[j]; j++)
		{
			if(!isspace(str2[j])) break;
		}

		c1 = tolower(str1[i++]);
		c2 = tolower(str2[j++]);

		if (!c2)
		{
			return (i > 0) ?(char*)&str1[i-1] : (char*)str1;
		}

		if((c1 != c2) || !c1) break;
	}

	return NULL;
}
