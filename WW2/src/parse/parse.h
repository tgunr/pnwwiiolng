#pragma once
// Created: Thu Mar 05 15:50:16 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#define EMPTY_LINE 0
#define MAX_ARGS	20

enum BASE_ARG_TYPES
{
	T_ZERO = 0,
	T_INT = 1,
	T_FLOAT,
	T_CHAR,
	T_STRING,
	T_EMPTY,
	T_MAX_BASE_TYPE

} ;

enum EXT_ARG_TYPES
{
	T_SINT64 = 1,
	T_SINT32,
	T_SINT16,
	T_SINT8,
	T_UINT64,
	T_UINT32,
	T_UINT16,
	T_UINT8,
	T_FLOAT64,
	T_FLOAT32,
	T_SCHAR,
	T_UCHAR
} ;

struct ARG_LIST
{
	SINT32 numArgs;
	SCHAR* line;
	SCHAR* arg[MAX_ARGS +1];
} ;

struct ERROR_TOKEN
{
	SINT16	location;
	SINT16	error;
} ;

struct ARG_KEY
{
	const char* svalue;//[MAX_SVALUE_SIZE];
	UINT32	dvalue;
	FLOAT32	fvalue;
	SINT16	numBytes;
	SINT8	baseType;
	SINT8	isNegative;
	UCHAR	character;

} ;

#define FUNCARGS ARG_KEY

struct ARG_KEY_HEAD
{
	SINT32 numKeys;
	ARG_KEY*	keys;
} ;

struct ARG_SPEC
{
	BASE_ARG_TYPES	baseType;
	SINT32		maxArgBytes;
} ;

extern LOGICAL parseValidArgs(ARG_SPEC* argSpec, FUNCARGS* args, SINT32 numArgs, const char* section, SINT32 lineNum);

extern SINT32 parseFillKeys(ARG_KEY_HEAD* keyList, ARG_KEY_HEAD* defaultList, ARG_LIST* argList, SINT32 offset);

extern ERROR_TOKEN getParams(ARG_LIST* list, UCHAR* paramString
							, void* destination, SINT32 minParams);

extern bool parseIsObjectIgnoringError(const char* objectName, const char* error) ;

