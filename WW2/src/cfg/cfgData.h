#pragma once
//	 Copyright 2008 Playnet Inc, All rights reserved
//	 Not to be distributed in API form except as part of a compiled library
//	 under License or NDA

#include "cfg/cfgCodes.h"

#define cfgSTRING_LENGTH	128

// Struct for storing name strings for cfgs
struct cfgPARAM_SETUP
{
	char	Name[cfgSTRING_LENGTH];
	int		Type;
	int		DefaultValue;

};

// Struct for storing values strings for cfgs
struct cfgPARAM_VALUE_STRINGS
{
	int  id;
	char value[cfgSTRING_LENGTH];

};

extern cfgPARAM_SETUP cfgParameters[cfgNUM_TOTAL_PARAMS];
extern const char* cfgTextureShadeModeValueStrings[cfgNUM_TEXTURE_SHADE_MODE_VALUES];
extern const char* cfgTextureMagFilterValueStrings[cfgNUM_TEXTURE_MAG_FILTER_VALUES];
extern const char* cfgTextureMinFilterValueStrings[cfgNUM_TEXTURE_MIN_FILTER_VALUES];
extern const char* cfgTextureMipFilterValueStrings[cfgNUM_TEXTURE_MIP_FILTER_VALUES];
extern const char* cfgTextureAntialiasingValueStrings[cfgNUM_TEXTURE_ANTIALIASING_VALUES];
extern const char* cfgTrueFalseValueStrings[cfgNUM_TRUE_FALSE_VALUES];
extern int cfgShadowSizeSelectValues[cfgNUM_SPECIAL_FX_SHADOW_SIZE_SELECT_VALUES];
extern void* Values[cfgNUM_TYPE_VALUES];
extern int NumValues[cfgNUM_TYPE_VALUES];
extern const char* cfgDefaultStrings[cfgNUM_DEFAULT_STRINGS];
extern int cfgDefaultIntegers[cfgNUM_DEFAULT_INTEGERS];
extern float cfgDefaultFloats[cfgNUM_DEFAULT_FLOATS];
extern void* Defaults[cfgNUM_TYPE_VALUES];
