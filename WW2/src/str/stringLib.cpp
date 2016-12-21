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

std::string string_replace(const std::string &haystack, const std::string &needle, const std::string &replacement)
{
    size_t pos = 0;
    std::string result = haystack;

    while((pos = result.find(needle, pos)) != std::string::npos)
    {
        result.replace(pos, needle.length(), replacement);
        pos += replacement.length();
    }
    return result;
}

std::vector<std::string> split_string(const std::string &str, const std::string &delimiter, size_t max_split)
{
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos && strings.size() < (max_split == 0 ? 0 : max_split - 1))
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }
    strings.push_back(str.substr(prev));
    return strings;
}


bool has_prefix(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::string strtolower(const std::string &str)
{
    std::string result = str;
    for (char &c : result) { c = static_cast<char>(tolower(c)); }
    return result;
}

std::string strtrim(const std::string &str)
{
    size_t snwspos = str.find_first_not_of(" \t\n\v\f\r");

    if (snwspos == std::string::npos) {
        return "";
    }

    size_t enwspos = str.find_last_not_of(" \t\n\v\f\r");

    if (enwspos == std::string::npos) {
        return "";
    }

    return str.substr(snwspos, (enwspos + 1) - snwspos);
}
