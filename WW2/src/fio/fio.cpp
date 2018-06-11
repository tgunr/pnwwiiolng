#include "syscfg.h" // Path is project specific.
#include <codecvt>
#include "fio/fio.h"
#include "fio/fiod.h"
#include "fio/fioa.h"
#include "include/vars.h"
#include "ui/uiUTF.h"
#include "mem/mem.h"

#if platOS == OS_WIN32
#include <direct.h> // _mkdir
#include <shlobj.h>
#include <io.h>
#include "sys/sysreg.h"
#endif

#define MAX_FIO_STACK 64

static SINT32 fio_switch;

/************************************************************************************************
	fioInit - sets up the user data directory on clients
*************************************************************************************************/

void fioInit()
{
    LOGICAL resultValue = FALSE;
    
    // Default to current directory
#if platOS == OS_WIN32
    
    //  \User -> My Documents -> WWIIOnLine
    resultValue = fioStructureInit();
    resultValue = SetCurrentDirectoryW(WW2_userConfigPath.c_str());

#elif platOS == OS_MACOS
    
    //  /Users -> "currentuser" -> Documents -> WWIIOnline
    resultValue = fioStructureInit();
    //  resultValue = fioSetCurrentWorkingPath(WW2_userConfigPath);

#endif
}


/************************************************************************************************
	char* fioLoad(const char* filename, char*& outPtr, unsigned int* outSize)
		Opens the file, allocates memory for it, returns a pointer to the memory.
		outSize can be null if you don't care about the size.
		Returns false if there is an error loading the file, otherwise true.
		NOTE: outPtr may be nullptr if the file was empty.
*************************************************************************************************/

bool fioLoad(const char* filename, char*& outPtr, unsigned int* outSize)
{
	// fiod has its own custom fiodLoad definition.
	if ( fio_switch == ISDIRECTORY )
		return fiodLoad(filename, outPtr, outSize);

	outPtr = nullptr;

	// Assess the file size, assume fioFileSize will
	// report any errors locating the file etc.
	UINT32 fileSize = fioFileSize(filename);
	if ( PN_UNLIKELY(fileSize < 0) )
	{
		return false;
	}
	if ( PN_UNLIKELY(fileSize == 0) )
	{
		return true;
	}

	// Open the file, again assume fioOpen will err for us.
	int fd = fioOpen(filename);
	if ( PN_UNLIKELY(fd < 0) )
	{
		return false;
	}

	// Allocate the memory.
	char* data = (char*)memAlloc(fileSize + 1);
	if ( PN_UNLIKELY(data == nullptr) )
	{
		DEBUG_ERR("Unable to allocate %u bytes of memory for loading %s.", fileSize + 1, filename);
		fiodClose(fd);
		return false;
	}

	size_t bytesRead = fioRead(fd, data, fileSize);
	fioClose(fd);

	if ( PN_UNLIKELY(bytesRead < 0) )
	{
		DEBUG_ERR("Read error reading %zu bytes from %s.", bytesRead, filename);
		memFree(data);
		return false;
	}

	if ( PN_UNLIKELY(bytesRead != fileSize) )
	{
		DEBUG_ERR("Read error reading %s, got %zu bytes instead of %u.", filename, bytesRead, fileSize);
	}

	data[bytesRead] = 0;

	if ( outSize != nullptr )
		*outSize = (unsigned int)fileSize;

	outPtr = data;

	return true;
}

/************************************************************************************************
	fioOpen
*************************************************************************************************/

int fioOpen(const char* const filename)
{
    int fd = -1;
    const char* fn = filename;

	switch(fio_switch)
    {
        //  local "user" directory.
		case ISUSERDIRECTORY:
            WITH ( UsingUserdir() )
				fd = fiodOpen(fn);
		break;
            
        //  local directory.
        case ISDIRECTORY:
       	default:
            fd = fiodOpen(fn);
       	break;
            
        //  playarc virtual directory (playarc).
        case ISARCHIVE:
            fd = fioaOpen(fn);
       	break;
    }

    return fd;
}

/************************************************************************************************
	fioUserFile
*************************************************************************************************/

FILE* fioUserFile(const char* filename, const char* mode)
{
	fullpath_t path[PN_MAX_PATH];

	wchar_t tmpFilename[PN_MAX_PATH];
	wwiiolpath_string tmpPath = WW2_exeWorkingPath;

	mbstowcs(tmpFilename, filename, PN_MAX_PATH);
//	tmpPath += L"\\";
//	tmpPath += tmpFilename;
	
#if platOS == OS_WIN32 && defined(_UNICODE)
    tmpPath += L"\\";
    tmpPath += tmpFilename;

    return _wfopen(tmpPath.c_str(), UTF16(mode));
#else
    tmpPath += L"/";
    tmpPath += tmpFilename;

    Wstring2Char(tmpPath, path, PN_MAX_PATH);
	return fopen(path, mode);
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
			WITH ( UsingUserdir() )
				fd = fiodOpenWrite(filename);
		break;
		case ISDIRECTORY:
			fd = fiodOpenWrite(filename);
		break;
		case ISARCHIVE:
		    fd = fioaOpenWrite(filename);
		break;
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
        case ISARCHIVE:
            fioaClose(fd);
		break;
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
			WITH ( UsingUserdir() )
				return fiodModified(modified,filename);
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
        case ISARCHIVE:
            return fioaEOF(fd);
		break;
    }
}

// Short-cut function to test file for existance with options
bool fioFileExists(std::string base, std::string suffix, std::string extension)
{
	std::string filename = base + suffix + extension;
	return fioFileExists(filename.c_str());
}

/************************************************************************************************
 fioFileExists
 *************************************************************************************************/

bool fioFileExists(const char* name)
{
	switch(fio_switch)
	{
		case -1:
			return false;
			break;
		case ISUSERDIRECTORY:
			WITH ( UsingUserdir() )
			return fiodFileExists(name);
			break;
		case ISDIRECTORY:
		default:
			return fiodFileExists(name);
			break;
		case ISARCHIVE:
			return fioaFileExists(name);
			break;
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
			WITH ( UsingUserdir() )
				return fiodFileSize(name);
		break;
        case ISDIRECTORY:
        default:
            return(fiodFileSize(name));
		break;
        case ISARCHIVE:
            return fioaFileSize(name);
		break;
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
        case ISARCHIVE:
            return fioaSeek(fd, offset,from);
      	break;
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
        case ISARCHIVE:
            return fioaRead(fd, dptr,length);
		break;
    }
}

size_t fioMap(int fd, const void*& data)
{
	SINT32 fdtype = fioDescriptorType(fd);

	switch (fdtype)
	{
		case - 1:
			data = nullptr;
			return 0;
		break;
		case ISUSERDIRECTORY:
		case ISDIRECTORY:
		default:
			return fiodMap(fd, data);
		break;
		case ISARCHIVE:
			return fioaMap(fd, data);
		break;
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
        case ISARCHIVE:
            return fioaWrite(fd, dptr,length);
		break;
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
	// Return a FILE* to the unified log file
	FILE* OpenLogFile()
	{
		return fioUserFile(Fio::LogFileName(), "a");
	}

	void DeleteLogFile()
	{
		fullpath_t path[PN_MAX_PATH];
		WITH ( UsingUserdir() )
			GetFullPath(path, Fio::LogFileName());
#if platOS == OS_WIN32
		_wunlink(path);
#else
		unlink(path);
#endif
	}

};	// end namespace Fio
