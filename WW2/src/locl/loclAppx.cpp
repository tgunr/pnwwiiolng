#include "syscfg.h" // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: loclApp.c
// Description: Module to house application specific localization structures.
//////////////////////////////////////////////////////////////////////
#include "locl/loclApp.h"

LANGTABLE g_LangTable[NUMBER_OF_CATEGORIES];

//////////////////////////////////////////////////////////////////////
// Any separate localization files go here. Need to update the
// enumerated ENUM_CATEGORIES in loclApp.h
//////////////////////////////////////////////////////////////////////
SCHAR g_szFiles[NUMBER_OF_CATEGORIES][64] =
{
    "ui",
	"iac",
	"function",
	"host",
    "mspawn",
	"english"
};

//////////////////////////////////////////////////////////////////////
// Language paths go here. Need to update the enumerated
// ENUM_LANGUAGES in loclApp.h
//////////////////////////////////////////////////////////////////////
SCHAR g_szPaths[NUMBER_OF_LANGUAGES][64] =
{
    "languages\\english\\",
	"languages\\italian\\",
	"languages\\french\\",
    "languages\\german\\",
    "languages\\spanish\\",
	"languages\\chinese\\",
	"languages\\russian\\"
};

const SCHAR* g_szLanguages[NUMBER_OF_LANGUAGES] =
{
	"english",
    "italian",
    "french",
    "german", 
	"spanish", 
	"chinese", 
	"russian"}; 
