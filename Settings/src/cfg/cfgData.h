// cfgData.h

#ifndef cfgdata_HEADER

  #define cfgdata_HEADER


/*
	 What am i?
	 Copyright 2001 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Did You Write This?
	Maintaining Programmer : Have You Taken it Over?
	Supporting Programmer  : Have you had Someone Elses Messy Code frobbed off on you?
	
	Rev 1  c2001
*/

#include "cfgCodes.h"


#define cfgSTRING_LENGTH	128

// Struct for storing name strings for cfgs
typedef struct _cfgPARAM_SETUP
{
	char	Name[cfgSTRING_LENGTH];
	int		Type;
	int		DefaultValue;

} cfgPARAM_SETUP;

// Struct for storing values strings for cfgs
typedef struct _cfgPARAM_VALUE_STRINGS
{
	int  id;
	char value[cfgSTRING_LENGTH];

} cfgPARAM_VALUE_STRINGS;

extern cfgPARAM_SETUP cfgParameters[cfgNUM_TOTAL_PARAMS];
extern char *cfgTextureShadeModeValueStrings[cfgNUM_TEXTURE_SHADE_MODE_VALUES]; 
extern char *cfgTextureMagFilterValueStrings[cfgNUM_TEXTURE_MAG_FILTER_VALUES]; 
extern char *cfgTextureMinFilterValueStrings[cfgNUM_TEXTURE_MIN_FILTER_VALUES]; 
extern char *cfgTextureMipFilterValueStrings[cfgNUM_TEXTURE_MIP_FILTER_VALUES];
extern char *cfgTextureAntialiasingValueStrings[cfgNUM_TEXTURE_ANTIALIASING_VALUES];
extern char *cfgTrueFalseValueStrings[cfgNUM_TRUE_FALSE_VALUES];
extern int cfgShadowSizeSelectValues[cfgNUM_SPECIAL_FX_SHADOW_SIZE_SELECT_VALUES];
extern void *Values[cfgNUM_TYPE_VALUES];
extern int NumValues[cfgNUM_TYPE_VALUES];
extern char *cfgDefaultStrings[cfgNUM_DEFAULT_STRINGS];
extern int cfgDefaultIntegers[cfgNUM_DEFAULT_INTEGERS];
extern float cfgDefaultFloats[cfgNUM_DEFAULT_FLOATS];
extern void *Defaults[cfgNUM_TYPE_VALUES];

#endif

