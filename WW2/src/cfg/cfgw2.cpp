#include "syscfg.h" // Path is project specific.

/*
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA
*/

#include <string.h>
#include <ctype.h>
#include <math.h>

#include "str/stringLib.h"
#include "cfg/cfgCodes.h"
#include "cfg/cfgData.h"
#include "cfg/cfgw2.h"

#include <sys/types.h>
#include <sys/stat.h>

#if platOS == OS_WIN32
#include <io.h>
#endif

static int FindIntValueInList(int Type, int Value)
{
int i;

	for (i = 0; i <= NumValues[Type]; ++i)
	{
		if (Value == ((int*) Defaults[Type])[i])
		{
			break;
		}
	}

	if (i == NumValues[Type])
	{
		return -1;
	}
	else
	{
		return i;
	}
}

static int FindStringValueInList(int Type, char* Value)
{
int i;

	for (i = 0; i < NumValues[Type]; ++i)
	{
		if (((char **) Defaults[Type])[i] != nullptr)
		{
			if (strcmp(Value, ((char **) Defaults[Type])[i]) == 0)
			{
				break;
			}
		}
	}

	if (i == NumValues[Type])
	{
		return -1;
	}
	else
	{
		return i;
	}
}

#ifdef UNUSED
static int FindFloatValueInList(int Type, float Value)
{
int i;

	for (i = 0; i <= NumValues[Type]; ++i)
	{
		if (Value == ((int*) Defaults[Type])[i])
		{
			break;
		}
	}

	if (i == NumValues[Type])
	{
		return -1;
	}
	else
	{
		return i;
	}
}
#endif

int cfgLoadW2Defaults(void** ppVariables)
{
int i, KeyType;

	for (i = 0; i < cfgNUM_TOTAL_PARAMS; ++i)
	{
		if (ppVariables[i] != nullptr)	//	_MASTER_VALUE_LIST
		{
			KeyType = cfgParameters[i].Type;
			switch (KeyType)
			{
			case cfgTYPE_INTEGER:
				*(int*) ppVariables[i] =
					((int **) Defaults)[KeyType][cfgParameters[i].DefaultValue];
				break;
			case cfgTYPE_FLOAT:
				*(float*) ppVariables[i] =
					((float **) Defaults)[KeyType][cfgParameters[i].DefaultValue];
				break;
			case cfgTYPE_STRING:
				strcpy((char*) ppVariables[i], ((char **) (Defaults[KeyType]))[cfgParameters[i].DefaultValue]);
				break;
			case cfgTYPE_SHADOW_SIZE_SELECT:
			case cfgTYPE_FSAA_SAMPLES:
				//*(int*) ppVariables[i] = ((int*) (Defaults[KeyType]))[cfgParameters[i].DefaultValue];
				//break;
			case cfgTYPE_TEXTURE_SHADE_MODE:
			case cfgTYPE_TEXTURE_MAG_FILTER:
			case cfgTYPE_TEXTURE_MIN_FILTER:
			case cfgTYPE_TEXTURE_MIP_FILTER:
			case cfgTYPE_TEXTURE_ANTIALIASING:
				*(int*) ppVariables[i] =  cfgParameters[i].DefaultValue;
				break;
			case cfgTYPE_TRUE_FALSE:
				if (strcmp((char*) (((char **) (Defaults[KeyType]))[cfgParameters[i].DefaultValue]), "true") == 0)
				{
					*(int*) ppVariables[i] = TRUE;
				}
				else
				{
					*(int*) ppVariables[i] = FALSE;
				}

				break;
			default:
				break;
			}
		}
	}

	return TRUE;
}

int cfgLoadW2(const char* ConfigName, void** ppVariables)
{
char	NewName[256], Key[256], Value[256], Line[256];
int		i, KeyType, KeyIndex, IntValue, Length, ValueIndex;
FILE*	ConfigFile;

	cfgLoadW2Defaults(ppVariables);

	// Open File
	ConfigFile = fopen(ConfigName, "rt");

	if (nullptr == ConfigFile)
	{
		// try up a directory if not found
#if platOS != OS_WIN32
		strlcpy(NewName, "../", 256);   // grrrrrrrrrr!
		strlcat(NewName, ConfigName, 256);
#else
		strcpy_s(NewName, "..\\");
		strcat_s(NewName, ConfigName);
#endif
		ConfigFile = fopen(NewName, "rt");

		if (nullptr == ConfigFile)
		{
			return TRUE; // Can't find the file, but that doesn't phase us! We are the
		}	             // the config parser of peace, love, and happiness

	}


	while (nullptr != fgets(Line,256,ConfigFile))
	{
		// Trim white-space off of begining
		for (i = 0; (Line[i] != 0) && (isspace(Line[i])); ++i)
			;

		strcpy(Line, &(Line[i]));

		// Other than any possible whitespace the line was empty
		if (strlen(Line) == 0)
		{
			continue;
		}

		// Copy Key
		for (i = 0; (Line[i] != 0) && (!isspace(Line[i])); ++i)
			;
		strncpy(Key, Line, i + 1);
		Key[i] = 0;

		// Lower case Key
		_strlwr(Key);

		strcpy(Line, &(Line[i]));


		// "//" Denotes a quote
		if (strncmp(Key, "//", 2) == 0)
		{
			continue;
		}

		// Trim white-space between pair
		for (i = 0; (Line[i] != 0) && (isspace(Line[i])); ++i)
			;

		strcpy(Line, &(Line[i]));

		// Copy Value
		for (i = 0; Line[i] != 0; ++i)
			;
		strncpy(Value, Line, i + 1);
		Value[i] = 0;

		// Lower case Value
		strlwr(Value);

		// Fill the proper global with the proper infoz
		// Match keystring with the proper KeyType
		for (i = 0; i < cfgNUM_TOTAL_PARAMS; ++i)
		{
			if (strcmp(cfgParameters[i].Name, Key) == 0)
			{
				KeyIndex = i;
				KeyType = cfgParameters[KeyIndex].Type;
				break;
			}
		}

		if (i == cfgNUM_TOTAL_PARAMS)
		{
			continue;
		}

		// Store Values
		if (ppVariables[KeyIndex] != nullptr)
		{
			switch (KeyType)
			{
			case cfgTYPE_INTEGER:
				*(int*) ppVariables[KeyIndex] = (int) atoi(Value);
				break;
			case cfgTYPE_FLOAT:
				*(float*) ppVariables[KeyIndex] = (float) atof(Value);
				break;
			case cfgTYPE_STRING:
				// Strip \n
				Length = strlen(Value);
				Value[Length - 1] = 0;
				strcpy((char*) ppVariables[KeyIndex], Value);
				break;
			case cfgTYPE_SHADOW_SIZE_SELECT:
			case cfgTYPE_FSAA_SAMPLES:
				IntValue = atoi(Value);

				ValueIndex = FindIntValueInList(KeyType, IntValue);
				if (ValueIndex != -1)
				{
					*(int*) ppVariables[KeyIndex] = ValueIndex;
				}
				break;
			case cfgTYPE_TEXTURE_SHADE_MODE:
			case cfgTYPE_TEXTURE_MAG_FILTER:
			case cfgTYPE_TEXTURE_MIN_FILTER:
			case cfgTYPE_TEXTURE_MIP_FILTER:
			case cfgTYPE_TEXTURE_ANTIALIASING:
			case cfgTYPE_TRUE_FALSE:
				// Strip \n
				Length = strlen(Value);
				Value[Length - 1] = 0;

				IntValue = FindStringValueInList(KeyType, Value);

				if (IntValue != -1)
				{
					*(int*) ppVariables[KeyIndex] = IntValue;
				}
				break;
			default:
				break;
			}
		}
	}

	fclose(ConfigFile);

	return(TRUE);
}


void cfgSetListItemValue(void** ppVariables, int index, void* value)
{
int KeyType;

	if(index >= 0 && index < cfgNUM_TOTAL_PARAMS)
	{
		KeyType = cfgParameters[index].Type;
		switch (KeyType)
		{
		case cfgTYPE_INTEGER:
			*(int*) ppVariables[index] = *(int*) value;
			break;
		case cfgTYPE_FLOAT:
			*(float*) ppVariables[index] = *(float*) value;
			break;
		case cfgTYPE_STRING:
			strcpy((char*) ppVariables[index], (char*) value);
			break;
		case cfgTYPE_TEXTURE_SHADE_MODE:
		case cfgTYPE_TEXTURE_MAG_FILTER:
		case cfgTYPE_TEXTURE_MIN_FILTER:
		case cfgTYPE_TEXTURE_MIP_FILTER:
		case cfgTYPE_TEXTURE_ANTIALIASING:
		case cfgTYPE_SHADOW_SIZE_SELECT:
		case cfgTYPE_FSAA_SAMPLES:
		case cfgTYPE_TRUE_FALSE:
			*(int*) ppVariables[index] = *(int*) value;
			break;
		default:
			break;
		}
	}
}

void* cfgGetListItemValue(void** list, int index)
{
	void* rval;


	rval = nullptr;

	if(index >= 0 && index < cfgNUM_TOTAL_PARAMS)
	{
		rval = list[index];
	}
	else
	{
// No printfs, a better debug statement should have been used
// Caveman
//		printf("blah");
	}

	return rval;
}

int cfgGetListItemType(void** list, int index)
{
	int rval;

	rval = -1;

	if(index >= 0 && index < cfgNUM_TOTAL_PARAMS)
	{
		rval = cfgParameters[index].Type;
	}

	return rval;
}
