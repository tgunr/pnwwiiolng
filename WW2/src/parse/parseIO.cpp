#include <syscfg.h> // Path is project specific.
// Created: Tue Mar 5 10:40:54 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "mem/mem.h"
#include "include/defs.h"

#if defined(FRONT_END) && !defined(CMDLINE_CLIENT)
#include "include/vars.h"
#include "rn/rnlogo.h"
#endif

#include "parse/parse.h"
#include "parse/parseIO.h"
#include "str/stringLib.h"

#ifdef LIMITED_FIO
	#include "fio/fioDefs.h"
	#include "fio/fiod.h"
#else
	#include "fio/fio.h"
#endif

#if defined(FRONT_END) && !defined(CMDLINE_CLIENT)
extern SINT32	LoadingScreen;
#endif

static bool _setSection(int numargs, FUNCARGS* args);
static bool _setDefault(int numargs, FUNCARGS* args);
static bool _setIgnore(int numargs, FUNCARGS* args);
static bool _executeFunc(char* func, FUNCARGS* fargs, SINT32 nargs
						, SINT32 argOffset, PARSE_CALLBACK* callback, bool* funcFound, UINT32* lineCounter);

static PARSE_CALLBACK  _defCallbacks[]={ {"section",		_setSection}
										,{"default",		_setDefault}
										,{"ignore",		_setIgnore}
										,{"",				nullptr}	};

struct SECTION_HEAD
{
	SINT32	numargs;
	UCHAR	name[MAX_SECT_NAME_SIZE];
	UCHAR	header[MAX_ARGS][MAX_SECT_NAME_SIZE];
};

static SECTION_HEAD* _section;

static char _fileName[PN_MAX_PATH];
static ARG_KEY_HEAD _defaultKeys;

#include <set>
typedef std::set< std::pair<std::string, std::string> > IGNORE_LIST;
static IGNORE_LIST _ignoreList;

static ARG_LIST* paramGetArgList(char* src)
{
	static ARG_LIST list;

	list.numArgs = 0;
	list.line = src;

	do
	{
		list.arg[list.numArgs] = src;
		src = strchr(src, ',');

		if ( src != nullptr )
		{
			// Remove trailing white space and any quotes.
			for ( char* p = src - 1; p > list.arg[list.numArgs] && (isspace(*p) || *p == '\"'); --p )
			{
				*p = 0;
			}

			// Trim the comma
			*(src++) = 0;

			// Skip leading whitespace or any quotes on the next argument
			while ( *src && (isspace(*src) || *src == '\"') )
			{
				++src;
			}
		}

		++list.numArgs;
	}
	while ( src && *src && list.numArgs < MAX_ARGS );

	// if the last arg's last char is a quote, kill it here; above code doesn't catch it for some reason
	int lastArg = list.numArgs - 1;
	char* lastArgChar = (lastArg >= 0) ? list.arg[lastArg] + strlen(list.arg[lastArg]) - 1 : nullptr;

	if(lastArgChar && *lastArgChar == '\"')
		*lastArgChar = 0;

	// Trim empty arguments from the end.
	for ( int i = list.numArgs - 1; i >= 0 && list.arg[i] && list.arg[i][0] == 0; --i )
	{
		list.arg[i] = nullptr;
		--list.numArgs;
	}

	if ( list.numArgs == 0)
		return nullptr;

	return &list;
}


// Returns a pointer to the start of the next line
// or the null byte at the end of the data.
static char*
_getEndOfLine(char* src)
{
	// Find the end of this line.
	while ( *src )
	{
		char c = *src;
		if ( c == '\n' || c == '\r' )
		{
			*(src++) = 0;
			if ( src[0] != c && (src[0] == '\n' || src[0] == '\r' ) )
			{
				// Handle OSes that have \r\n and \n\r
				++src;
			}
			return src;
		}
		++src;
	}

	return src;
}

// Returns a pointer to the end of the current line,
// Updates 'src' to be the start of the line.
static char*
_getNextValidLine(char*& eol, UINT32& lineCounter)
{
	char* src = nullptr;
	bool inComment = false;
	while ( eol && *eol )
	{
		++lineCounter;		// We have a line.

		src = eol;
		eol = _getEndOfLine(src);

		while ( *src && isspace(*src) )
			++src;

		// Line contained nothing but spaces.
		if ( !*src )
			continue;

		if ( inComment )
		{
			if ( src[0] != '*' || src[1] != '/' )
				continue;
			inComment = false;
			continue;
		}

		if ( src[0] == '/' && src[1] == '*' )
		{
			if ( strstr(src + 2, "*/") == nullptr )
			{
				/* Not a comment like this */
				inComment = true;
			}
			continue;
		}

		// Comment line, skip.
		if ( *src == '#' || *src == ';' || *src == '/' || *src == '"' )
			continue;

		return src;
	}

	return nullptr;
}

bool parseFile(char* pathName, PARSE_CALLBACK* sectionCallbacks
				, PARSE_CALLBACK* userCallbacks
				, UINT32* lineCounter
				)
{
	char		func[MAX_SECT_NAME_SIZE];
	int			nargs;
	ARG_KEY		fargs[MAX_ARGS];
	ARG_KEY_HEAD keyList;// = {0,fargs};
	ARG_LIST*	argList;
	bool		funcFound, retval;

	keyList.numKeys = 0;
	keyList.keys = fargs;

	strncpy(_fileName, pathName, PN_MAX_PATH);

	char* data = nullptr;
	bool loaded = fioLoad(_fileName, data);
	if ( data == nullptr )
		return loaded;

	_section = (SECTION_HEAD*)memAlloc(sizeof(SECTION_HEAD));
	memset(_section, 0x0, sizeof(SECTION_HEAD));

	_defaultKeys.keys = (ARG_KEY*)memAlloc(sizeof(ARG_KEY)*(MAX_ARGS+1));
	memset(_defaultKeys.keys, 0x0, sizeof(ARG_KEY)*(MAX_ARGS+1));

	// should no lines return false?
	retval = true;

	*lineCounter = 0;

#if defined(FRONT_END) && !defined(CMDLINE_CLIENT)
	if ( LoadingScreen )
	{
		rnUpdateLoadingScreen();
	}
#endif

	char* eol = data;
	while ( eol && *eol )
	{
		char* p = _getNextValidLine(eol, *lineCounter);
		if ( p == nullptr || *p == 0 )
			continue;

		argList = paramGetArgList(p);

		if (argList == nullptr || argList->numArgs < 1)
			continue;

		// if we get an error and break retval its false
		retval = false;

		strncpy(func, argList->arg[0], MAX_SECT_NAME_SIZE);
		_strlwr(func);
		if (  ! strcmp(func,"section"))
			_defaultKeys.numKeys = 0;

		nargs = parseFillKeys(&keyList, &_defaultKeys, argList, 0);

		if (nargs > MAX_ARGS)
		{
			DEBUG_ERR("%s:%u: Too many arguments", _fileName, *lineCounter);
			break;
		}

		funcFound = false;

		if ( fargs[0].baseType == T_STRING)
		{
			strcpy(func,fargs[0].svalue);
			_strlwr(func);

			if (!_executeFunc(func, fargs, nargs, 1, _defCallbacks, &funcFound, lineCounter))
				break;

			// call the users section callback as well if defined
			if ( ! strcmp(func,"section"))
			{
				if (!_executeFunc(func, fargs, nargs, 1, sectionCallbacks, &funcFound, lineCounter))
					break;
			}

			if (! funcFound)
			{
				if (!_executeFunc(func, fargs, nargs, 1, userCallbacks, &funcFound, lineCounter))
					break;

				if (*_section->name != 0
					&& (funcFound || ! strcmp((char*)func, "define")) )
				{
					strcpy((char*)func,(char*)_section->name);
					_strlwr(func);
					if (!_executeFunc(func, fargs, nargs, 1, sectionCallbacks, &funcFound, lineCounter))
						break;
				}
			}
		}
		else if (fargs[0].baseType == T_INT && *_section->name != 0)
		{
			strcpy(func,(char*)_section->name);
			_strlwr(func);

			if (!_executeFunc(func, fargs, nargs, 0, userCallbacks, &funcFound, lineCounter))
				break;
		}

		if (!funcFound)
		{
			DEBUG_ERR("%s:%u: Function \"%s\" not found in", _fileName, *lineCounter, func);
			break;
		}

		// the only way to be true is to run out of lines
		retval = true;
	}

	memFree(_section);
	memFree(_defaultKeys.keys);

	memFree(data);

	return (retval);
}

static bool _executeFunc(char* func, FUNCARGS* fargs, SINT32 nargs
						, SINT32 argOffset, PARSE_CALLBACK* callback, bool* funcFound, UINT32* lineCounter)
{
SINT32 i,sendNum;

	if (callback == nullptr)
		return true;

	sendNum = nargs-argOffset;

	for (i=0; callback[i].function; i++)
	{
		if (strcmp(func, callback[i].type) == 0)
		{
			*funcFound = true;
			if (!callback[i].function(sendNum, &fargs[argOffset]))
			{
				DEBUG_ERR("%s:%u: Function failure, funct %s", _fileName, *lineCounter, callback[i].type);
				return false;
			}

			break;
		}
	}

	return true;
}

static bool _setSection(int numargs, FUNCARGS* args)
{
SINT32 i;

	strncpy((char*)_section->name,args[0].svalue,MAX_SECT_NAME_SIZE);
	_section->numargs = numargs;

	strcpy((char*)_section->header[0],"file/name");

	for( i=1; i < numargs; i++ )
		strncpy((char*)_section->header[i],args[i].svalue,MAX_SECT_NAME_SIZE);

	_defaultKeys.numKeys= 0;

	return true;
}

static bool _setIgnore(int numargs, FUNCARGS* args)
{
	if ( numargs < 2 )
		return true;

	for ( int i = 1; i < numargs; ++i )
	{
		_ignoreList.insert( std::make_pair<std::string,std::string>(args[0].svalue, args[i].svalue) );
	}

	return true;
}

static bool _setDefault(int numargs, FUNCARGS* args)
{
SINT32 i;

	for( i=0; i < numargs && i < MAX_ARGS; i++ )
		_defaultKeys.keys[i+1] = args[i];

	_defaultKeys.numKeys = numargs+1;

	return true;

}

bool
parseIsObjectIgnoringError(const char* objectName, const char* error)
{
	IGNORE_LIST::iterator it = _ignoreList.find( std::make_pair<std::string, std::string>(objectName, error) );
	return ( it != _ignoreList.end() );
}

