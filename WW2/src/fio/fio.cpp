#include <syscfg.h> // Path is project specific.
#include "VersionNo.h"
#include "fio/fio.h"
#include "fio/fiod.h"
#include "include/vars.h"
#include "ui/uiUTF.h"
#include "mem/mem.h"

#if platOS == OS_WIN32
#include <direct.h> // _mkdir
#include <shlobj.h>
#include <io.h>
#include "sys/sysreg.h"
#endif

#ifndef FIO_NO_ARCHIVE
#include "fio/fioa.h"
#endif

#define MAX_FIO_STACK 64

static SINT32 fio_switch;

#include "dbug/dbug.h"

/************************************************************************************************
	fioInit - sets up the user data directory on Windows
*************************************************************************************************/

void fioInit()
{
	*UserDataPath = 0;

#if platOS == OS_WIN32

	TCHAR path[_MAX_PATH];
	const TCHAR* base = NULL;

	// default to this directory
	ustrcpy(path, _T("."));

#if GREEN_INSTALL != 1
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, path);

	base = sysregGetInstallValue(_T("Version"));
#endif // GREEN_INSTALL != 1

#ifdef _UNICODE
	// setup path
	strcpy(UserDataPath, UTF8(path));
	strcat(UserDataPath, "\\");

	if(base)
	{
		strcat(UserDataPath, UTF8(base));
		strcat(UserDataPath, "\\");
	}

#if GREEN_INSTALL != 1
	// make the directory
	OS_WMKDIR(UTF16(UserDataPath));
#endif // GREEN_INSTALL != 1
#else

	// UserDataPath is an ascii string
	strcpy(UserDataPath, path);
	strcat(UserDataPath, "\\");

	if(base)
	{
		strcat(UserDataPath, base);
		strcat(UserDataPath, "\\");
	}

	// make the directory
    OS_MKDIR(UserDataPath);
#endif // _UNICODE

#endif // OS_WIN32
}


/************************************************************************************************
	char* fioLoad(const char* filename, char*& outPtr, unsigned int* outSize)
		Opens the file, allocates memory for it, returns a pointer to the memory.
		outSize can be null if you don't care about the size.
		Returns false if there is an error loading the file, otherwise true.
		NOTE: outPtr may be NULL if the file was empty.
*************************************************************************************************/

bool fioLoad(const char* filename, char*& outPtr, unsigned int* outSize)
{
	// fiod has its own custom fiodLoad definition.
	if ( fio_switch == ISDIRECTORY )
		return fiodLoad(filename, outPtr, outSize);

	outPtr = NULL;

	// Assess the file size, assume fioFileSize will
	// report any errors locating the file etc.
	SINT32 fileSize = fioFileSize(filename);
	if ( fileSize < 0 )
	{
		return false;
	}
	if ( fileSize == 0 )
	{
		return true;
	}

	// Open the file, again assume fioOpen will err for us.
	int fd = fioOpen(filename);
	if ( fd < 0 )
	{
		return false;
	}

	// Allocate the memory.
	char* data = (char*)memAlloc(fileSize + 1);
	if ( data == NULL )
	{
		DEBUG_ERR("Unable to allocate %u bytes of memory for loading %s.", fileSize + 1);
		fiodClose(fd);
		return false;
	}

	size_t bytesRead = fioRead(fd, data, fileSize);
	fioClose(fd);

	if ( bytesRead < 0 )
	{
		DEBUG_ERR("Read error reading %u bytes from %s.", bytesRead, filename);
		memFree(data);
		return false;
	}

	if ( bytesRead != fileSize )
	{
		DEBUG_ERR("Read error reading %s, got %u bytes instead of %u.", filename, bytesRead, fileSize);
	}

	data[bytesRead] = 0;

	if ( outSize != NULL )
		*outSize = (unsigned int)fileSize;

	outPtr = data;

	return true;
}

/************************************************************************************************
	fioOpen
*************************************************************************************************/

#if platOS == OS_MACOS
static void myReplace(std::string& str, const std::string& old, const std::string& nw)
{
  size_t pos = 0;
  while((pos = str.find(old, pos)) != std::string::npos)
  {
    str.replace(pos, old.length(), nw);
    pos += nw.length();
  }
}

static void fixer(const char*filename, char*temp)
{
  std::string str(filename);
  myReplace( str, "%5c", "/" );
  myReplace( str, "%5C", "/" );
  strcpy(temp, str.c_str());
}

#endif
int fioOpen(const char* const filename)
{
    int fd = -1;
    const char* fn = filename;

#if platOS == OS_MACOS
  char temp[PATH_MAX]; // remove the damn %5C that libxml2 puts into paths in OS 10.5 (sometimes)
  fixer(filename,temp);
    fn = temp;
#endif

	switch(fio_switch)
    {
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			fd = fiodOpen(fn);
			fiodSetPrepath(PREPATH);
		}
		break;
        case ISDIRECTORY:
       	default:
			fd = fiodOpen(fn);
       	break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
			fd = fioaOpen(fn);
       	break;
#endif
    }

    return fd;
}

/************************************************************************************************
	fioUserFile
*************************************************************************************************/

FILE* fioUserFile(const char* filename, const char* mode)
{
	std::string path = UserDataPath;
    path += filename;

#if platOS == OS_WIN32 && defined(_UNICODE)
    return _wfopen(UTF16(path.c_str()), UTF16(mode));
#else
	return fopen(path.c_str(), mode);
#endif
}

/************************************************************************************************
	fioOpenWrite
*************************************************************************************************/

int fioOpenWrite(const char* filename)
{
	int fd = -1;

	switch(fio_switch)
	{
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			fd = fiodOpenWrite(filename);
			fiodSetPrepath(PREPATH);
		}
		break;
		case ISDIRECTORY:
			fd = fiodOpenWrite(filename);
		break;
#ifndef FIO_NO_ARCHIVE
		case ISARCHIVE:
		    fd = fioaOpenWrite(filename);
		break;
#endif
	}

    return fd;
}

/************************************************************************************************
	fioClose
*************************************************************************************************/

void fioClose(int fd)
{
    SINT32 fdtype = fioDescriptorType(fd);

    switch(fdtype)
    {
		case -1:
		break;
		case ISUSERDIRECTORY:
        case ISDIRECTORY:
        default:
            fiodClose(fd);
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            fioaClose(fd);
		break;
#endif
    }
}

/************************************************************************************************
	fioGetRegionPath
*************************************************************************************************/

const char* fioGetRegionalPath(const char* p)
{
	// chinese specific for now; someday we may have other regions
	// these ids correlate to the language ids in loclApp.h but I didn't
	// want to include that here
	if(gRegion == 5 && p && *p)
	{
		// lets see if we have a localized version
		static std::string path;

		const char* extension = strstr(p, ".");
		path = p;

		if(extension)
		{
			// strip out extension
			path = path.substr(0, path.size() - 4);
		}

		switch(gRegion)
		{
			/*
			case 1:	path += "_it";		break;
			case 2:	path += "_fr";		break;
			case 3:	path += "_de";		break;
			case 4:	path += "_es";		break;
			*/
			case 5:	path += "_zh";		break; // chinese
		}

		if(extension)
			path += extension;

		// DATA/Terrain/BLD/uk-1.dds
		// becomes
		// DATA/Terrain/BLD/uk-1_zh.dds
		// if found, its returned

		if(fioAccess(path.c_str()))
			return path.c_str();
	}

	// return the original path;
	return p;
}

/************************************************************************************************
	fioAccess
*************************************************************************************************/

LOGICAL fioAccess(const char* filename)
{
    switch(fio_switch)
    {
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			LOGICAL result = fiodAccess(filename);
			fiodSetPrepath(PREPATH);
			return result;
		}
		break;
        case ISDIRECTORY:
        default:
            return(fiodAccess(filename));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaAccess(filename);
		break;
#endif
    }
}

/************************************************************************************************
	fioModified
*************************************************************************************************/

LOGICAL fioModified(SINT32* modified, const char* filename)
{
    switch(fio_switch)
    {
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			LOGICAL result = fiodModified(modified,filename);
			fiodSetPrepath(PREPATH);
			return result;
		}
		break;
        case ISDIRECTORY:
		default:
            return(fiodModified(modified,filename));
		break;
        case ISARCHIVE:
           *modified = 0;
           return 0;
		break;
    }
}

/************************************************************************************************
	fioEOF
*************************************************************************************************/

int fioEOF(int fd)
{
    SINT32 fdtype = fioDescriptorType(fd);

    switch(fdtype)
   	{
		case -1:
			return -1;
		break;
		case ISUSERDIRECTORY:
        case ISDIRECTORY:
        default:
            return(fiodEOF(fd));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaEOF(fd);
		break;
#endif
    }
}

/************************************************************************************************
	fioFileSize
*************************************************************************************************/

size_t fioFileSize(const char* name)
{
	switch(fio_switch)
   	{
		case -1:
			return -1;
		break;
        case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			size_t size = fiodFileSize(name);
			fiodSetPrepath(PREPATH);
			return size;
		}
		break;
        case ISDIRECTORY:
        default:
            return(fiodFileSize(name));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaFileSize(name);
		break;
#endif
    }
}

/************************************************************************************************
	fioSet
*************************************************************************************************/

void fioSet(SINT32 fio_s)
{
	fio_switch = fio_s;
}

/************************************************************************************************
	fioQuery
*************************************************************************************************/

SINT32 fioQuery()
{
    return fio_switch;
}

/************************************************************************************************
	fioDescriptorType
*************************************************************************************************/

SINT32 fioDescriptorType(int fd)
{
	static_assert(FIOA_FD_START > FIOD_FD_START, "FIOA fds must be bigger than FIOD fds.");
	if(fd < 0) return -1;
	if(fd >= FIOA_FD_START)
		return ISARCHIVE;

	return ISDIRECTORY;
}

/************************************************************************************************
	fioSeek
*************************************************************************************************/

size_t fioSeek(int fd, size_t offset, int from)
{
	SINT32 fdtype = fioDescriptorType(fd);

    switch(fdtype)
    {
		case -1:
			return -1;
		break;
		case ISUSERDIRECTORY:
        case ISDIRECTORY:
        default:
            return(fiodSeek(fd, offset,from));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaSeek(fd, offset,from);
      	break;
#endif
    }
}

/************************************************************************************************
	fioRead
*************************************************************************************************/

size_t fioRead(int fd, void* dptr, size_t length)
{
	SINT32 fdtype = fioDescriptorType(fd);

    switch(fdtype)
    {
		case -1:
			*(char*)dptr = 0;
			return -1;
		break;
		case ISUSERDIRECTORY:
        case ISDIRECTORY:
        default:
            return(fiodRead(fd, dptr,length));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaRead(fd, dptr,length);
		break;
#endif
    }
}

/************************************************************************************************
	fioWrite
*************************************************************************************************/

size_t fioWrite(int fd, const void* dptr, size_t length)
{
	SINT32 fdtype = fioDescriptorType(fd);

	switch(fdtype)
    {
		case -1:
			return -1;
		break;
		case ISUSERDIRECTORY:
        case ISDIRECTORY:
        default:
            return(fiodWrite(fd, dptr,length));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaWrite(fd, dptr,length);
		break;
#endif
    }
}

/************************************************************************************************
	fioGetLine
*************************************************************************************************/

size_t fioGetLine(int fd, char* string, size_t length)
{
	size_t   i;

	if(fd < 0) return -1;

  for (i=0; i < length - 1; i++)
  {
    if (fioRead(fd, &string[i], 1) != 1)
	{
		break;
	}

    if (string[i] == '\n')
    {
      if (i > 0 && string[i-1] == '\r')
      {
        i -= 1;
      }

      string[i] = 0;
      return(i+1);
    }

#if platOS == OS_MACOS
// mac newlines, ugh
   	if(string[i] == '\r'){
    	char c = '\0';
	  	if(fioRead(fd, &c, 1) == 1 ){
	  		fioSeek(fd,-1,SEEK_CUR);
	  		if(c != '\n'){
	  			string[i] = '\0';
	  			return (i+1);
	  		}
	  	}
	}
#endif
  }

  if (i < length)
  {
	string[i] = 0;
  }

  return(i);
}

/************************************************************************************************
	fioGetc
*************************************************************************************************/

char fioGetc(int fd)
{
    char c = 0;

	fioRead(fd, &c, 1);

    return c;
}

/************************************************************************************************
	fioGets
*************************************************************************************************/

short fioGets(int fd)
{
	short s = 0;

  	fioRead(fd, &s, 2);

	return s;
}

/************************************************************************************************
	fioGetl
*************************************************************************************************/

long fioGetl(int fd)
{
    long l = 0;

 	fioRead(fd, &l, 4);

    return l;
}

/************************************************************************************************
	fioGetf
*************************************************************************************************/

float fioGetf(int fd)
{
    float f = 0;

	fioRead(fd, &f, 4);

    return f;
}

/************************************************************************************************
	fioGetd
*************************************************************************************************/

double fioGetd(int fd)
{
 	double d = 0;

   	fioRead(fd, &d, 8);

 	return d;
}

/************************************************************************************************
	fioReads
*************************************************************************************************/

size_t fioReads(int fd, SINT16* sp, int numshorts)
{
	const size_t nb = fioRead(fd, sp, numshorts * sizeof(SINT16));
    return nb;
}

/************************************************************************************************
	fioReadl
*************************************************************************************************/

size_t fioReadl(int fd, SINT32* lp, int numlongs)
{
	const size_t nb = fioRead(fd, lp, numlongs * sizeof(SINT32));
    return nb;
}

/************************************************************************************************
	fioReadf
*************************************************************************************************/

size_t fioReadf(int fd, FLOAT32* fp, int numfloats)
{
	const size_t nb = fioRead(fd, fp, numfloats * sizeof(FLOAT32));
    return nb;
}

/************************************************************************************************
	fioReadd
*************************************************************************************************/

size_t fioReadd(int fd, FLOAT64* dp, int numdoubles)
{
	const size_t nb = fioRead(fd, dp, numdoubles * sizeof(FLOAT64));
    return nb;
}

/************************************************************************************************
	fioPutc
*************************************************************************************************/

void fioPutc(int fd, SCHAR c)
{
	fioWrite(fd, &c, 1);
}

/************************************************************************************************
	fioPuts
*************************************************************************************************/

void fioPuts(int fd, SINT16 s)
{
	fioWrite(fd, &s, 2);
}

/************************************************************************************************
	fioPutl
*************************************************************************************************/

void fioPutl(int fd, SINT32 l)
{
	fioWrite(fd, &l, 4);
}

/************************************************************************************************
	fioPutf
*************************************************************************************************/

void fioPutf(int fd, FLOAT32 f)
{
	fioWrite(fd, &f, 4);
}

/************************************************************************************************
	fioPutd
*************************************************************************************************/

void fioPutd(int fd, FLOAT64 d)
{
	fioWrite(fd, &d, 8);
}

/************************************************************************************************
	fioWrites
*************************************************************************************************/

void fioWrites(int fd, const SINT16* sp, int numshorts)
{
	fioWrite(fd, (void*)sp, numshorts * sizeof(SINT16));
}

/************************************************************************************************
	fioWritel
*************************************************************************************************/

void fioWritel(int fd, const SINT32* lp, int numlongs)
{
	fioWrite(fd, (void*)lp, numlongs * sizeof(SINT32));
}

/************************************************************************************************
	fioWritef
*************************************************************************************************/

void fioWritef(int fd, const FLOAT32* fp, int numfloats)
{
	fioWrite(fd, (void*)fp, numfloats * sizeof(FLOAT32));
}

/************************************************************************************************
	fioWrited
*************************************************************************************************/

void fioWrited(int fd, const FLOAT64* dp, int numdoubles)
{
	fioWrite(fd, (void*)dp, numdoubles * sizeof(FLOAT64));
}

#include "libxml/xmlIO.h"

static int xmlinputmatch(const char* filename) { return 1; }

void fioXmlRegisterInputCallbacks()
{
	xmlRegisterInputCallbacks(xmlinputmatch, (xmlInputOpenCallback)fioOpen, (xmlInputReadCallback)fioRead, (xmlInputCloseCallback)fioClose);
}

namespace Fio
{

	//////////////////////////////////////////////////////////////////////
	// Return the name of the standard log file.

   const char* LogFileName() { return "WW2_Log.txt"; }

	//////////////////////////////////////////////////////////////////////
	// Return a FILE* to either the unified log file or, if unable to
	// open it, stderr (since much of our code assumes this function
	// returns a valid FILE handle)

	FILE* OpenLogFile(void)
	{
		FILE* fp = fioUserFile(Fio::LogFileName(), "a");
		// If that fails, return stderr
		if ( fp == NULL )
			return OS_FDOPEN(OS_DUP(OS_FILENO(stderr)), "a");
		return fp;
	}

	void DeleteLogFile()
	{
		std::string path = UserDataPath;
		path += Fio::LogFileName();
#if platOS == OS_WIN32
		_unlink(path.c_str());
#else
		unlink(path.c_str());
#endif
	}

};	// end namespace Fio
