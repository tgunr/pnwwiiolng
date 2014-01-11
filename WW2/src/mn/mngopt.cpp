#include <syscfg.h> // Path is project specific.

#include <ctype.h>
#include <string.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mn/mngopt.h"

// Used to build fully-qualified domain names, primarily auth.
char RegionSuffix[512] = ".playnet.com";

enum key_TYPE
{
	ENULL,
	ELOGICAL,
	EINT,
	EUINT,
	ELONG,
	EDWORD,
	EFLOAT,
	EDOUBLE,
	ESTRING

};

struct KEY
{
	char*		key;
	key_TYPE	ktype;

};

//  lowercase ONLY!
static KEY	vars[]={{"help", ENULL},
                    {"width", EINT},
                    {"height", EINT},
                    {"bpp", EINT},
                    {"alpha", EINT},
                    {"hdw", ELOGICAL},
                    {"stencil", ELOGICAL},
                    {"window", ELOGICAL},
					{"staccato", ELOGICAL},
					{"ticket", ESTRING},
					{"bypass", ESTRING},
					{"serverip", ESTRING},
					{"serverport", ESTRING},
					{"serverotp", ESTRING},
					{"apid", ESTRING},
					{"serverprotocol", ESTRING},
					{"log", ELOGICAL},
                    {"side", EINT},
                    {"data", ELOGICAL},
                   	{"nowor", ENULL},
                   	{"noinf", ENULL},
                    {"build", ENULL},
                    {"logcomm", ENULL},
                    {"logfps", ENULL},
                    {"logweap", ENULL},
                   	{"screen", ESTRING},
                   	{"nospawn", ENULL},
                    {"console", ENULL},
					{"region", ESTRING},
					{"ragdolls", ELOGICAL},
                    {"buildterrain", ENULL},
    {"o", ESTRING},
    {"altfont", ENULL},
					{nullptr, ENULL}};

static void*	pvars[]={&WantedHelp,
					&WantedWidth,
					&WantedHeight,
					&WantedBpp,
					&WantedAlpha,
					&WantedHdw,
					&WantedStencil,
					&WantedWindow,
					&UseStaccato,
					&TicketFile,
					&BypassString,
					&ServerIP,
					&ServerPort,
					&ServerOTP,
					&ServerApID,
					&ServerProtocol,
					&WantedUserLogEnabled,
                    &WantedSide,
                    &WantDataDir,
                    &NoBlitzWorPreload,
                    &NoInfPreload,
                    &g_bBuildMDLS,
					&g_log_comm,
					&g_log_fps,
					&g_log_weap,
					&WantedVideoCard,
					&OverrideSpawner,
					&OpenConsole,
					&RegionSuffix,
					&gRagdolls,
					&g_buildTerrain,
    &HackID,
    &AltFont,
					nullptr};

static char* _skipFilename(const char* ptr)
{
	if ( *ptr == '"' )
	{
		while ( *ptr == '"' )
			++ptr;
		ptr = strchr(ptr, '"');
		while ( ptr && *ptr == '"' )
			++ptr;
	}
	else
		ptr = strchr(ptr, ' ');

	if ( ptr == nullptr )
		return nullptr;

	while ( *ptr == ' ' )
		++ptr;

	return (char*)ptr;
}

LOGICAL mngoptParse(char* InputLine)
{
	char*	ptr, sep[2]="-", CommandLine[512];
	char	cmd[256], value[256];
	int		i;

#if platOS == OS_WIN32
	ptr = _skipFilename(GetCommandLineA());
#else
	ptr = _skipFilename(GetCommandLine());
#endif

	if ( ptr == nullptr )
		return TRUE;
	if ( *ptr != '-' && *ptr != '/' )
		return TRUE;

	strncpy(CommandLine, ptr, 511);

	ptr = CommandLine;
    sep[0] = *ptr;

    ptr = strtok(ptr, sep);

	while ( ptr != nullptr )
	{
		// find end of command
		for (i=0; ptr[i]; i++)
		{
			if (isspace(ptr[i]) ||
				ptr[i] == ':' ||
				ptr[i] == '=')
			{
				break;
			}

			cmd[i] = tolower(ptr[i]);
		}

		cmd[i] = 0;

		// trim tail of command
		while ( isspace(ptr[i]) )
			++i;

		// move to head of line
		ptr = &ptr[i];

		// find end of value
		if (ptr[0] == '"')
		{
			ptr += 1;
			for (i=0; ptr[i]; i++)
			{
				if (ptr[i] == '"')
				{
					break;
				}

				value[i] = tolower(ptr[i]);
			}
		}
		else
		{
			for (i=0; ptr[i]; i++)
			{
				if (isspace(ptr[i]))
				{
					break;
				}

				value[i] = tolower(ptr[i]);
			}
		}

		value[i] = 0;

		// search for cmd in known commands
		for (i=0; vars[i].key; i++)
		{
			if (0 == lstrcmp(vars[i].key, cmd))
			{
				break;
			}
		}

		ptr = strtok(nullptr, sep);

		// continue if command not known
		if (nullptr == vars[i].key)
		{
			Write_Debug("Unknown switch: '%s', '%s'", cmd, value);
			continue;
		}

		switch (vars[i].ktype)
		{
			case ENULL:
			{
					*(LOGICAL*)pvars[i] = TRUE;
				break;
			}
			case ELOGICAL:
			{
				if (0 == lstrcmp(value, "false") ||
						0 == lstrcmp(value, "0"))
				{
					*(LOGICAL*)pvars[i] = FALSE;
				}
				else
				if (0 == lstrcmp(value, "true") ||
						0 == lstrcmp(value, "1"))
				{
					*(LOGICAL*)pvars[i] = TRUE;
				}
				break;
			}
			case EINT:
			{
				*(int*)pvars[i] = (int)strtol(value, nullptr, 0);
				break;
			}
			case EUINT:
			{
				*(unsigned int*)pvars[i] = (unsigned int)strtol(value, nullptr, 0);
				break;
			}
			case ELONG:
			{
				*(long*)pvars[i] = (long)strtol(value, nullptr, 0);
				break;
			}
			case EDWORD:
			{
				*(unsigned long*)pvars[i] = (unsigned long)strtoul(value, nullptr, 0);
				break;
			}
			case EFLOAT:
			{
				*(float*)pvars[i] = (float)strtod(value, nullptr);
				break;
			}
			case EDOUBLE:
			{
				*(double*)pvars[i] = (double)strtod(value, nullptr);
				break;
			}
			case ESTRING:
			{
				strcpy((char*)pvars[i], value);
				break;
			}
			default:
			{
				// never happen
				continue;
			}
		}
	}
	while (ptr != nullptr);

	return(TRUE);
}
