#pragma once
// fio.h
//
// Date: Sat Mar 17 19:14:12 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a 
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall
// Maintaining Programmer: Eric "Meat" Hall
// Supporting Programmer:  Marty "Vandal" Puolin

#define fioSEEK_SET   SEEK_SET
#define fioSEEK_CUR   SEEK_CUR
#define fioSEEK_END   SEEK_END

#ifdef LIMITED_FIO
	#include "fioDefs.h"
	#include "fiod.h"
#else
	#include <stdio.h>
	#include "systypes.h"

	#define FIOD_FD_START 0
	#define FIOA_FD_START 1024

	enum
	{
		ISDIRECTORY,
		ISARCHIVE,
		ISUSERDIRECTORY
	};

	#define fioGetString fioGetLine

	/************************************************************************************************
	fio
	*************************************************************************************************/

	// sets up the user data directory on Windows
	extern void fioInit();

	extern int  fioOpen(const char*filename);
	extern int  fioOpenWrite(const char*filename);
	extern int  fioOpenAppend(const char *filename);

	extern void fioClose(int fd);

	extern LOGICAL fioAccess(const char *filename);
	extern LOGICAL fioModified(SINT32 *modified, const char *filename);
	extern int 	fioEOF(int fd);
	extern SINT32 	fioFileSize(const char *name);

	extern void fioSet(SINT32);
	extern SINT32 fioQuery();
	extern SINT32 fioDescriptorType(int fd);

	extern int fioSeek(int fd, int offset, int from);
	extern int fioRead(int fd, void *dptr, int length);
	extern int fioWrite(int fd, const void *dptr, int length);

	extern int    fioGetLine(int fd, char *string, int length);
	extern char   fioGetc(int fd);
	extern short  fioGets(int fd);
	extern long   fioGetl(int fd);
	extern float  fioGetf(int fd);
	extern double fioGetd(int fd);

	extern int fioReads(int fd, SINT16 *sp, int numshorts);
	extern int fioReadl(int fd, SINT32 *lp, int numlongs);
	extern int fioReadf(int fd, FLOAT32 *fp, int numfloats);
	extern int fioReadd(int fd, FLOAT64 *dp, int numdoubles);

	extern void fioPutc(int fd, SCHAR c);
	extern void fioPuts(int fd, SINT16 s);
	extern void fioPutl(int fd, SINT32 l);
	extern void fioPutf(int fd, FLOAT32 f);
	extern void fioPutd(int fd, FLOAT64 d);

	extern void fioWrites(int fd, const SINT16 *sp, int numshorts);
	extern void fioWritel(int fd, const SINT32 *lp, int numlongs);
	extern void fioWritef(int fd, const FLOAT32 *fp, int numfloats);
	extern void fioWrited(int fd, const FLOAT64 *dp, int numdoubles);

	extern void fioXmlRegisterInputCallbacks();
#endif
