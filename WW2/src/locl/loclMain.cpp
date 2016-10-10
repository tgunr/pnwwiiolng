#include <syscfg.h> // Path is project specific.
#include <string.h>
#include "fio/fio.h"
#include "locl/loclMain.h"
#include "locl/loclLang.h"
#include "mem/mem.h"
#include "ui/uiMain.h"

#define MAX_LOCALIZATION_STRING_LENGTH	128

static SCHAR**	_MessageTable;
static SINT32	_LanguageInUse;
static SINT32	HighestStringNumber;
static SCHAR	_LanguageName[30];

static const char*		LocMissingFile = {"Missing localization file"};
//If you change the LocLanguageFiles, be SURE to also change
//the loclLocalizationLanguage enums in the loclMain.h file.
static const char*		LocLanguageFiles[] =
{
	"English",
};


// loclLoadLocalizationStringTable
// This opens a file (using fio subsystem) to load in a group of
// message strings. The message strings are placed in a global
// array with size dynamically allocated. This call will erase
// all previously loaded messages in the array. Each line of the
// localization file has the following syntax: <ID>,<STRING>
// where ID is a unique 32-bit number, no greater than
// HighestStringNumber, and STRING is terminated by a newline
// character. The files will be named "<language>.loc"
// This function returns TRUE if the file is found and loads
// properly, and returns FALSE if the file is not found or cannot
// be loaded. The function locl_Message_Table_Kill is the "Kill
// Switch" used to free the allocated memory.
LOGICAL loclLoadLocalizationStringTable(SINT32 LanguageID)
{
SINT32	i,
		FileID,
		MessageIndex,
		CountLanguageStrings = 0;

char	FileNameBuffer[MAX_PATH],
		HoldMessageString[MAX_LOCALIZATION_STRING_LENGTH],
		*Token;

	locl_Message_Table_Kill();

    //////////////////////////////////////////
    // New for independent localization files.
    //////////////////////////////////////////
    if(!loclLangInit((ENUM_LANGUAGES)gLanguage))
        return FALSE;

	if(LanguageID < loclLastLanguage)
	{
		_LanguageInUse = LanguageID;
		strcpy(_LanguageName, LocLanguageFiles[LanguageID]);
	}

	strcpy(FileNameBuffer, LocLanguageFiles[LanguageID]);
	strcat(FileNameBuffer, ".loc");
	FileID = fioOpen(FileNameBuffer);
	if(FileID > -1)
	{
		memset(HoldMessageString,0,MAX_LOCALIZATION_STRING_LENGTH);
		for(i = 0; !fioEOF(FileID); i++)
		{
			fioGetString(FileID, HoldMessageString, MAX_LOCALIZATION_STRING_LENGTH);
			Token = strtok(HoldMessageString, ",");
			if (Token == nullptr)
			{
				continue;
			}

			MessageIndex = atoi(Token);
			if(HighestStringNumber < MessageIndex)
			{
				HighestStringNumber = MessageIndex;
			}

			CountLanguageStrings++;
		}

		if(_MessageTable != nullptr)
		{
			return FALSE;
		}
		else
		{
			_MessageTable = (char **)memAlloc((HighestStringNumber + 1) * sizeof(char*));
		}

		fioSeek(FileID, 0, SEEK_SET);

		for(i = 0;; )
		{
			if (i >= CountLanguageStrings)
			{
				break;
			}

			fioGetString(FileID, HoldMessageString, MAX_LOCALIZATION_STRING_LENGTH);
			Token = strtok(HoldMessageString, ",");
			if (Token == nullptr)
			{
				continue;
			}

			MessageIndex = atoi(Token);
			Token = strtok(nullptr, "\n");
			_MessageTable[MessageIndex] = (SCHAR*)memAlloc(strlen(Token)+1);
			strcpy(_MessageTable[MessageIndex], Token);

			i++;
		}

		fioClose(FileID);
		return TRUE;
	}
	else
	{
		HighestStringNumber = 0;
		_MessageTable = (char **)memAlloc(sizeof(char*));
		_MessageTable[0] = (SCHAR*)memAlloc(strlen(LocMissingFile)+1);
		strcpy(_MessageTable[0], LocMissingFile);
		return FALSE;
	}
}

// loclGetLocalizationStringByID
// This function returns a signed character string from the localization
// table. If there is no string for a given ID (that is to say, an invalid
// ID is passed), the default message (message 0) will be returned.

const WCHAR* loclGetLocalizationStringByID(SINT32 MessageID)
{
	return uiLocString(IDE_LANG_CLIENT, MessageID).c_str();

	/*
	if (_MessageTable == nullptr)
	{
#pragma message("****** FIXME: Ramp fix this (localMain.cpp line 143)")
		return "Get Ramp to fix this (localMain.cpp line 143)!!!";
	}

	if((MessageID > 0) && (MessageID <= HighestStringNumber))
	{
		return _MessageTable[MessageID];
	}
	else
	{
		return _MessageTable[0];
	}
	*/
}

// loclGetLanguageID
// This function returns an SINT32 for the currently installed
// language identifier.
SINT32 loclGetLanguageID()
{
	return _LanguageInUse;
}

// loclGetLanguageName
// This function returns an SCHAR* for the currently installed
// language name.
SCHAR* loclGetLanguageName()
{
	return _LanguageName;
}

void locl_Message_Table_Kill()
{
    loclLangKill();

	if(_MessageTable != nullptr)
	{
		for(SINT32 i = 0; i <= HighestStringNumber; i++)
		{
			if(_MessageTable[i] != nullptr)
			{
				memFree(_MessageTable[i]);
			}
		}

		HighestStringNumber = 0;
		memFree(_MessageTable);
		_MessageTable = nullptr;
	}
}
