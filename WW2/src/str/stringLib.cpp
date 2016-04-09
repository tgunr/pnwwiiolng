#include <syscfg.h> // Path is project specific.
// Created: Fri Mar 31 16:26:10 2000
//
// The following file is a trimmed down version of several files
// I got from:
//
// http://chesworth.com/pv/resources/snippets.htm#section1group5
//
// All functions used were declared to be public domain and therefore free
// for my bogarting
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <ctype.h>
#include "str/stringLib.h"
#include <string.h>

#if platOS != OS_WIN32

//Turns a string into lowercase
char* strlwr(char* string)
{
      char* s;

      if (string)
      {
            for (s = string; *s; ++s)
                  *s = tolower(*s);
      }
      return string;
}

//Turns a string into lowercase
char* strupr(char* string)
{
      char* s;

      if (string)
      {
            for (s = string; *s; ++s)
                  *s = toupper(*s);
      }
      return string;
}
#endif

//Removes leading whitespace from a string
//Removes leading whitespace from a string
char* rmlead(char* str)
{

  if (str)
  {
	while( isspace(*str))
	{
		str++;
	}
  }

  return str;
}

//Removes trailing whitespace from a string
char* rmtrail(char* str)
{
	if(!str || *str == 0)
		return str;

	// Find the last character of the string (not terminator).
	char* endStr = str + strlen(str) - 1;

	// Find the last non-space character but don't overshoot the start of the string.
	char* p = endStr;
	while(p > str && isspace(*p))
		--p;

	// If p was decremented, replace the prior character (a whitespace) with terminator.
	if ( p < endStr )
		*(p+1) = 0;

	return str;
}

char* strescape(char* d,const char* s, char c)
{
	char* curr = d;
	while (*s != '\0')
	{
		if (*s == c)
			*(curr++) = '\\';
		*(curr++) = *(s++);
	}
	*curr = '\0';
	return d;
}

