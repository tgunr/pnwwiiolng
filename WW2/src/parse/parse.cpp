#include <syscfg.h> // Path is project specific.
// Created: Tue Mar 5 10:40:54 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "parse/parse.h"

#define DBUG_INVALID_INPUT			20021

const BASE_ARG_TYPES intAllow[] = {T_INT, T_ZERO};
const BASE_ARG_TYPES floatAllow[] = {T_FLOAT, T_INT, T_ZERO};
const BASE_ARG_TYPES charAllow[] = {T_CHAR, T_ZERO};
const BASE_ARG_TYPES stringAllow[] = {T_STRING, T_CHAR, T_ZERO, T_INT};
const BASE_ARG_TYPES emptyAllow[] = {T_EMPTY};

const BASE_ARG_TYPES* baseAllow[] = {nullptr
									,intAllow
									,floatAllow
									,charAllow
									,stringAllow
									,emptyAllow
									,nullptr};

ARG_KEY* parseGetArgKey(UCHAR* strIn)
{
	static UINT32 tmpUint32;
	UINT16	tmpUint16;
	UINT8	tmpUint8;
	SINT32	tmpSint32;
	SINT16	tmpSint16;
	SINT8	tmpSint8;

	FLOAT32	tmpFloat32;
	SCHAR*	str = (SCHAR*) strIn;
	SCHAR*	endPoint;
	SINT8	isNegative = 0;
	static	ARG_KEY tt = {nullptr,0,0,0,0,0,0};

	dbugSetErrno(0);

	if(str==nullptr)
	{
		dbugSetErrno(EINVAL);
		return (nullptr);
	}

	tt.svalue = str;
	tt.character = str[0];

	if ( *str == '-')
		tt.isNegative = TRUE;

	tmpUint32 = (UINT32)strtoul(str, &endPoint,10);

	if (dbugGetErrno() != 0 || endPoint == str || *endPoint != 0)
	{
		tmpSint32 = 0;

		tmpFloat32 = (FLOAT32)strtod(str, &endPoint);
		if (dbugGetErrno() != 0 || endPoint == str || *endPoint != 0)
		{

			if ( (tt.numBytes = (SINT16)strlen(str)) > 1)
				tt.baseType = T_STRING;
			else
				tt.baseType = T_CHAR;

			tmpFloat32 = 0;

		}
		else
		{
			tt.baseType = T_FLOAT;
			tt.numBytes = 4;
		}

	}
	else
	{
		tt.baseType = T_INT;

		tmpSint32 = (SINT32)tmpUint32;

		if ( isNegative )
		{
			tmpSint8 = (SINT8) tmpSint32;
			tmpSint16 = (SINT16) tmpSint32;

			if (tmpSint32 == (SINT32)tmpSint8)
				tt.numBytes =1;
			else if (tmpSint32 == (SINT32)tmpSint16)
				tt.numBytes =2;
			else
				tt.numBytes = 4;

		}
		else
		{
			tmpUint8 = (UINT8) tmpUint32;
			tmpUint16 = (UINT16) tmpUint32;
			if (tmpUint32 == (UINT32)tmpUint8)
				tt.numBytes =1;

			else if (tmpUint32 == (UINT32)tmpUint16)
				tt.numBytes =2;
			else
				tt.numBytes = 4;
		}

		tmpFloat32 = (FLOAT32)strtod(str, &endPoint);
	}

	tt.fvalue = tmpFloat32;
	tt.dvalue = tmpSint32;

	if (str[0] == 0)
		tt.baseType = T_EMPTY;

	return (&tt);
}

static inline SINT32 _fillParamInt(UCHAR* /*arg*/, UCHAR* target, EXT_ARG_TYPES thisType, ARG_KEY* argSpec)
{
SINT32 tvalSint32, offset = 0;

	tvalSint32 = argSpec->dvalue;

	switch (thisType)
	{

	case T_SINT32:
	case T_UINT32:
		*((SINT32*) target)= tvalSint32;
		offset = sizeof(SINT32);
		break;
	case T_SINT16:
	case T_UINT16:
		if ( argSpec->numBytes > 2 )
			return -DBUG_INVALID_INPUT;

		*((SINT16*) target)= tvalSint32;
		offset = sizeof(SINT16);
		break;
	case T_SINT8:
	case T_UINT8:
		if ( argSpec->numBytes > 1 )
			return -DBUG_INVALID_INPUT;

		*((SINT8*) target)= tvalSint32;
		 offset = sizeof(SINT8);
		break;

	case T_SINT64:
	case T_UINT64:
	default:
		return -DBUG_INVALID_INPUT;
	}

	return offset;
}

static inline SINT32 _fillParamFloat(UCHAR* /*arg*/, UCHAR* target, EXT_ARG_TYPES thisType, ARG_KEY* argSpec)
{
//FLOAT64 tvalFloat64;
SINT32 offset = 0;

//	tvalFloat64 = argSpec->float64;

	switch (thisType)
	{
/*
	case T_FLOAT64:
		*((FLOAT64*) target)= tvalFloat64;
		 offset = sizeof(FLOAT64);
		break;
*/
	case T_FLOAT32:
		if ( argSpec->numBytes > 4 )
			return -DBUG_INVALID_INPUT;

		*((FLOAT32*) target)= argSpec->fvalue;
		offset = sizeof(FLOAT32);
		break;
	default:
		return -DBUG_INVALID_INPUT;
	}

	return offset;

}

LOGICAL parseCmpBaseType(BASE_ARG_TYPES spec, BASE_ARG_TYPES arg)
{
const BASE_ARG_TYPES* specAllow;
SINT32 i;

	if ( spec == T_STRING )
		return TRUE;

	specAllow = baseAllow[spec];

	for ( i = 0; specAllow[i] != 0; i++)
	{
		if (specAllow[i] == arg)
			return (TRUE);
	}

	return (FALSE);
}

bool parseValidArgs(ARG_SPEC* argSpec, FUNCARGS* args, SINT32 numArgs, const char* section, SINT32 lineNum)
{
	for (SINT32 i = 0; i < numArgs; i++)
	{
		if (! parseCmpBaseType(argSpec[i].baseType, (BASE_ARG_TYPES)args[i].baseType) )
		{
#if platBuild == BUILD_SERVER
			dbugLog(_FLA, "########## BLITZWOR IS INVALID ########## missing or bad parameter (%d) type %d, value (%s) to type %d, section %s, line %d"
						,i,args[i].baseType,args[i].svalue,argSpec[i].baseType,section,lineNum);
#else
			WriteDebug("missing or bad parmeter (%d) type %d, value (%s) to type %d, section %s"
						,i,args[i].baseType,args[i].svalue,argSpec[i].baseType,section);
#endif
			return false;
		}
		if (args[i].numBytes > argSpec[i].maxArgBytes)
		{
#if platBuild == BUILD_SERVER
			dbugLog(_FLA, "########## BLITZWOR IS INVALID ########## parameter (%d) too long type %d, value (%s) to type %d, section %s, line %d"
						,i,args[i].baseType,args[i].svalue,argSpec[i].baseType,section,lineNum);
#else
			WriteDebug("parameter too long in blitzWor");
#endif
			return false;
		}
	}

	return true;
}

SINT32 parseFillKeys(ARG_KEY_HEAD* keyList, ARG_KEY_HEAD* defaultList, ARG_LIST* argList, SINT32 offset)
{
SINT32 nargs, i;
ARG_KEY* argKey, *fargs, *defs;


	fargs = keyList->keys;
	defs = defaultList->keys;

	nargs = argList->numArgs - offset;

	// fill out the fargs array
	for (i=0; i < nargs; i++)
	{
		argKey = parseGetArgKey((UCHAR*)argList->arg[i+offset]);

		if (argKey->baseType == T_EMPTY
			&& defaultList->numKeys > 0
			&& defaultList->numKeys > i)
		{
			fargs[i] = defs[i];
		}
		else
			fargs[i]	= *argKey;

	}

	if (defaultList->numKeys > 0 && nargs < defaultList->numKeys)
	{
		for ( i=nargs; i < defaultList->numKeys; i++ )
			fargs[i] = defs[i];
	}

	nargs = (nargs > defaultList->numKeys) ? nargs : defaultList->numKeys;

	for (i=nargs; i < MAX_ARGS; i++)
	{
		fargs[i].svalue = "\0";
		fargs[i].dvalue = 0;
		fargs[i].fvalue = 0.0f;
		fargs[i].baseType = T_EMPTY;
		fargs[i].isNegative = 0;
		fargs[i].numBytes = 0;
	}

	return nargs;
}


ERROR_TOKEN getParams(ARG_LIST* list, UCHAR* paramString
					, void* destination, SINT32 minParams)
{
SINT32	thisArg, maxParams;
void* target;
ERROR_TOKEN et = {0,0};
UCHAR* arg;
UCHAR** temp;
ARG_KEY* argKey;
SINT32	len, retval;

	et.error = -EINVAL;

	if ( destination == nullptr
		|| paramString == nullptr
		|| minParams < 0
		|| minParams > (maxParams = (SINT32)strlen((char*)paramString)) )
		return (et);

	target = destination;

	et.error = EMPTY_LINE;
	if (list == nullptr)
		return (et);

	et.error = -EINVAL;
	if (	list->numArgs < minParams
		|| list->numArgs > maxParams)
		return (et);

	for ( thisArg = 0; thisArg <= list->numArgs; thisArg++)
	{
		arg = (UCHAR*)list->arg[thisArg];
		argKey = parseGetArgKey(arg);
		et.location = thisArg;
		et.error = -EINVAL;
		len = (SINT32)strlen((char*)arg);

		switch (paramString[thisArg])
		{
		case T_STRING:
			*((UCHAR **)target) = arg;
			//target = ((UCHAR **)target)++;  //this line got a "this line
			//might be undefined warning with gcc 3.0.2, so I changed it to
			//the following 3 lines
			temp=(UCHAR**)target;
			temp++;
			target=temp;
			break;
		case T_SCHAR:
		case T_UCHAR:
			if (len > 1)
				return (et);
			*((UCHAR * )target) = *arg;
			target = (UCHAR*)target + 1;
		default:
			if (argKey->baseType == T_INT)
			{
				if ( (retval = _fillParamInt(arg, (UCHAR*)target, (EXT_ARG_TYPES)paramString[thisArg], argKey)) < 0)
				{
					et.error = retval;
					return (et);
				}

				// I know it doesn't look as efficient as target += blah,
				// but the compiler will make it that way.
				target = (UCHAR*)target + retval;
			}
			else if (argKey->baseType == T_FLOAT)
			{
				if ( (retval = _fillParamFloat(arg, (UCHAR*)target, (EXT_ARG_TYPES)paramString[thisArg], argKey)) < 0)
				{
					et.error = retval;
					return (et);
				}

				// See above remark about optimization.
				target = (UCHAR*)target + retval;
			}
			break;
		}

	}

	et.error = 0;
	et.location = list->numArgs;

	return et;
}
