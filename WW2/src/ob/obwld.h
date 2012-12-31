#pragma once

#include "ob/obdshad.h"

#define NEW_PARSE

#ifdef NEW_PARSE

#include "parse/parseWor.h"
	struct CONSTANT_VALUE
	{
		char	flagname[32];
		UINT32	flagval;

	} ;

#else

	#define MAX_ARGS	10

	enum ARG_TYPE
	{
		ARG_HEX,
		ARG_NUMBER,
		ARG_FLOAT,
		ARG_STRING,
		ARG_QUOTED,
		MAX_ARG_TYPES

	} ;

	struct CONSTANT_VALUE
	{
		char	flagname[32];
		UINT32	flagval;

	} ;

  struct FUNCARGS
  {
		char	svalue[128];
    UINT32 dvalue;
    float fvalue;

  } ;

#include "parse/parseIO.h"
#endif

extern LOGICAL obwldInit(char* levelfile);
extern void obwldKill(void);
extern LOGICAL obwldUpdate(void);

#ifdef USE_SHADOWS
	extern LOGICAL obwldPredrawUpdate(void);
#endif
