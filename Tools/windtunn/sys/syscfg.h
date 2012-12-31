// syscfg.h
//

#ifndef syscfg_HEADER

	#define syscfg_HEADER


	// Windows defines
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include "systypes.h"

#ifdef __cplusplus
extern "C"
{
#endif



#define WIND_TUNNEL

#define SYS_TITLE				"WINDTUNN"
#define SYS_COMPANY_NAME		"Playnet"

#undef PREPATH
#define PREPATH ".\\Data\\"

	#define MINFRAMESECOND  30
	#define MAXFRAMESECOND  30
	#define MAXENUMMODES    32
	#define MAXENUMDEVICES  32
	#define MAXENUMDRIVERS  32
	#define MAXENUMTEXTURES 32
	#define MINTEXTURES     64
	#define MAXMIPLEVELS    9
	#define MINMATERIALS    16
	#define MAXLIGHTS       128
	#define MINOBJECTS      2048
	#define MINSOUNDS       32
	#define MINDYNALITES    16
	#define MAXRENDERSTATES 128
	#define MAXLIGHTSTATES  32
	#define MAXTEXTURESTAGES	9
	#define MAXTEXTURESTATES	32

#ifdef EVENT_LOG
	BOOL        Open_Debug(char *filename);
	void        Close_Debug(void);
	void        __cdecl Write_Debug(char *str, ...);
#else
	#define     Open_Debug
	#define     Close_Debug()
	#define     Write_Debug
#endif

#ifdef __cplusplus
}
#endif

#endif
