#pragma once
// Date: Sat Mar 17 19:14:12 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#define fioSEEK_SET   SEEK_SET
#define fioSEEK_CUR   SEEK_CUR
#define fioSEEK_END   SEEK_END

enum
{
	FIOD_FD_START = (1 << 16),
	FIOA_FD_START = (1 << 17),
};

#if platBuild != TEUL_SERVER
namespace Fio
{
	// Name of our log file
	extern const char* LogFileName();

	// Open the standard logging file or return stderr.
	// (guaranteed not to return nullptr)
	extern FILE* OpenLogFile( void );
	extern void DeleteLogFile();
}
#endif

#if ! defined(LIMITED_FIO) || LIMITED_FIO == 0
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

	extern bool fioLoad(const char* filename, char*& outPtr, unsigned int* outSize=nullptr);
	extern int  fioOpen(const char* const filename);
	extern int  fioOpenWrite(const char*filename);

	extern FILE* fioUserFile(const char* filename, const char* mode);

	extern void fioClose(int fd);

	extern const char* fioGetRegionalPath(const char* p);
	extern LOGICAL fioAccess(const char* filename);
	extern LOGICAL fioModified(SINT32* modified, const char* filename);
	extern int 	fioEOF(int fd);
	extern size_t 	fioFileSize(const char* name);

	extern void fioSet(SINT32);
	extern SINT32 fioQuery();
	extern SINT32 fioDescriptorType(int fd);

	extern size_t fioSeek(int fd, size_t offset, int from);
	extern size_t fioRead(int fd, void* dptr, size_t length);
	extern size_t fioWrite(int fd, const void* dptr, size_t length);

	extern size_t fioMap(int fd, const void*& data);

	extern size_t    fioGetLine(int fd, char* string, size_t length);
	extern char   fioGetc(int fd);
	extern short  fioGets(int fd);
	extern long   fioGetl(int fd);
	extern float  fioGetf(int fd);
	extern double fioGetd(int fd);

	extern size_t fioReads(int fd, SINT16* sp, int numshorts);
	extern size_t fioReadl(int fd, SINT32* lp, int numlongs);
	extern size_t fioReadf(int fd, FLOAT32* fp, int numfloats);
	extern size_t fioReadd(int fd, FLOAT64* dp, int numdoubles);

	extern void fioPutc(int fd, SCHAR c);
	extern void fioPuts(int fd, SINT16 s);
	extern void fioPutl(int fd, SINT32 l);
	extern void fioPutf(int fd, FLOAT32 f);
	extern void fioPutd(int fd, FLOAT64 d);

	extern void fioWrites(int fd, const SINT16* sp, int numshorts);
	extern void fioWritel(int fd, const SINT32* lp, int numlongs);
	extern void fioWritef(int fd, const FLOAT32* fp, int numfloats);
	extern void fioWrited(int fd, const FLOAT64* dp, int numdoubles);

	extern void fioXmlRegisterInputCallbacks();
#endif
