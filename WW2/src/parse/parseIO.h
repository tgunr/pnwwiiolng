#pragma once
// Created: Thu Mar 05 15:50:16 2001
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "parse/parse.h"

#define MAX_SVALUE_SIZE 128

#define MAX_SECT_NAME_SIZE 64

#define MAX_FUNC_NAME 32

#if platOS == OS_WIN32
	typedef bool (__cdecl* PARSE_FUNCTION)(SINT32 numargs, struct FUNCARGS* args);
#elif platOS == OS_MACOS || platOS == OS_LINUX
	typedef bool (*PARSE_FUNCTION)(int numargs, struct FUNCARGS* args);
#else
	#error "define this"
#endif

struct PARSE_CALLBACK
{
	char            type[32];
	PARSE_FUNCTION  function;
} ;

// maximum number of load module args
//#define MAX_LM_ARG	3

extern bool parseFile(char* pathName, PARSE_CALLBACK* sectionCallbacks, PARSE_CALLBACK* userCallbacks, UINT32* lineCounter);
