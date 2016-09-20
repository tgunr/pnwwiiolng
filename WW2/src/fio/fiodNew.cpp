#include <syscfg.h> // Path is project specific.

#include "wwii/wwiiHash.h" // does this work on linux?
# include "dbug/dbug.h"
#include <map>
#include <set>

#if platOS != OS_LINUX && platBuild != BUILD_SERVER
# include "pi/pi.h"
# include "include/vars.h"
#endif

#include "mem/mem.h"
#include "fio/fiod.h"
#include "fio/fio.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(WIN32) || defined(WIN64)
# include <io.h>
# include <Windows.h>	// WIN32_FIND_DATA, FindFirstFile, FindNextFile
# include <direct.h>
# if defined(lstrcat)
#  undef lstrcat
#  undef lstrcpy
# endif
#else
# include <dirent.h>
# include <unistd.h>
# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif

#if platOS == OS_WIN32 || platOS == OS_LINUX // Does Mac need these?
# define lstrcpy strcpy
# define lstrcat strcat
#endif

#if platOS == OS_WIN32
# if platBuild == BUILD_SERVER
char DataPath[MAX_PATH] = "c:\\playnet\\data\\" ;
# endif
#endif

#ifndef LIMITED_FIO
 static char* os_file_path(char* path);
 static wchar_t* os_file_path(wchar_t* path);
#else
# define os_file_path(x) (x)
#endif

#if platOS == OS_LINUX
static const char* DataPath = "./data";
#endif

#ifndef _POSIX_
# define _POSIX_
#endif
#include <limits.h>
#include <string>

#include "wwii/wwiiUTF.h"

#ifndef PATH_MAX
# define PATH_MAX 1024
#endif

#if platOS == OS_WIN32
#define CHANGE_SEP 	'/'
#define OS_SEP		'\\'
#define WCHANGE_SEP 	L'/'
#define WOS_SEP		L'\\'
#else
#define CHANGE_SEP 	'\\'
#define OS_SEP		'/'
#define WCHANGE_SEP 	L'\\'
#define WOS_SEP		L'/'
#endif

#ifdef RECORD_FILES
static std::set<std::string> ReferencedFiles;

void dumpFileRefs()
{
	printf("\n\n");
	for ( std::set<std::string>::iterator it = ReferencedFiles.begin(); it!= ReferencedFiles.end(); it++)
	{
		printf("%s\n",it->c_str());
	}
}

static void recordRef( std::string ref )
{
	ReferencedFiles.insert(ref);
}

#else
#define recordRef(x)
#endif

//////////////////////////////////////////////////////////////////////
// Get me the name and size of a given file.

#if platOS == OS_WIN32
static off_t _getFilePathAndSize(const char* name, bool userdir, wchar_t* widePath, size_t wsize)
#else
static off_t _getFilePathAndSize(const char* name, bool /*userdir*/, wchar_t* widePath, size_t wsize)
#endif
{
	SINT32 retval = 0 ;

	char buffer[PATH_MAX + 1];

#if platBuild == BUILD_CLIENT && platOS == OS_WIN32
	if(userdir)
	{
		// no bounds checking here, nice
		strcpy(buffer, UserDataPath);
		strcat(buffer, (char*)name);

		if(WWII::ConvertChars(buffer, widePath, wsize))
		{
			struct _stat64i32 wStatBuffer;

			retval = _wstat(os_file_path(widePath), &wStatBuffer);

			if (retval == 0)
			{
				return wStatBuffer.st_size ; // Return what the OS thinks it is
			}
		}
	// Else fall through normally
	}
#endif

	// think this works as char* because when we get to here we're talking about relative paths which are all ascii?
	strcpy(buffer, DataPath) ;
	strcat(buffer, name) ;

	struct stat statBuffer ;
	retval = stat(os_file_path(buffer), &statBuffer);

#if platOS != OS_WIN32
	///TODO: OGS: I think this works under Windows too...
	if ( retval < 0 || !S_ISREG(statBuffer.st_mode) )
	{
		return -1 ;
	}
#else
	///TODO: OGS: See above: This allows Windows to [try to] open directories like files.
	if ( retval < 0 )
	{
		return -1 ;
	}
#endif

	// make sure and update our passed in widePath to the found path
	if(!WWII::ConvertChars(buffer, widePath, wsize))
		return -1;

	return statBuffer.st_size ; // Return what the OS thinks it is
}

//////////////////////////////////////////////////////////////////////
// Load an entire file into memory and return a pointer to the memory

bool
fiodLoad(const char* filename, char*& outPtr, unsigned int* outSize)
{
	wchar_t	widePath[PATH_MAX + 1] = L"" ;
	char	asciiPath[PATH_MAX + 1] = "" ;

	outPtr = NULL ;

	off_t fileSize = _getFilePathAndSize(filename, false, widePath, sizeof(widePath)) ;
	if ( fileSize < 0 )
	{
		DEBUG_ERR("Error performing stat on '%s'.", filename) ;
		return false ;
	}

	if ( fileSize == 0 )
		return true ;

	WWIIConvert(widePath, asciiPath);

#if platOS == OS_WIN32
	// already been converted and path seperators fixed
	int fd = OS_WOPEN(widePath, O_RDONLY | O_BINARY);
	recordRef(widePath);
#else
	int fd = OS_OPEN(os_file_path(asciiPath), O_RDONLY | O_BINARY);
	recordRef(os_file_path(asciiPath));
#endif
	if ( fd < 0 )
	{
		DEBUG_ERR("Unable to open '%s'.", asciiPath) ;
		return false ;
	}

	static const size_t _PAGE_SIZE = 4096 ;		// *MUST* be a power of two
	const size_t allocSize = ((size_t)fileSize + 1 + _PAGE_SIZE) & ~_PAGE_SIZE ;

	char* mem = (char*)memAlloc(allocSize) ;
	if ( mem == NULL )
	{
		OS_CLOSE(fd) ;
		DEBUG_FATAL("Out of memory loading '%s' (%" PRSizeT " bytes).", filename, allocSize) ;
		return false ;
	}

	int bytesRead = OS_READ(fd, mem, fileSize) ;
	OS_CLOSE(fd) ;

	if ( bytesRead < 0 )
	{
		DEBUG_ERR("Error reading data from %s (fd:%d, size:%u)", asciiPath, fd, (unsigned int)fileSize) ;
		memFree(mem) ;
		return false ;
	}

	if ( bytesRead != fileSize )
	{
		DEBUG_ERR("Read error reading %s, got %d bytes instead of %u.", filename, bytesRead, (unsigned int)fileSize) ;
	}

	mem[bytesRead] = 0 ;

	if ( outSize != NULL )
		*outSize = (unsigned int)fileSize ;

	outPtr = mem ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/* Original fiod code would byte-by-byte read files;
 * this is a replacement which reads the entire file
 * into memory and uses fio as a simplified stdio
 * replacement */
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#include "include/io_mapped_file.h"
typedef std::map<int, IO::MappedFile*> IOMAPPED_FILES;
static IOMAPPED_FILES s_filemap;

//////////////////////////////////////////////////////////////////////
// Lookup a Fiod::FileState from a descriptor

static int lastFd = -1 ;
static IO::MappedFile* lastResult = NULL ;

static IO::MappedFile*
_lookupFileState(int fd)
{
	if ( fd != lastFd )
	{
		IOMAPPED_FILES::iterator it = s_filemap.find(fd) ;
		if ( it != s_filemap.end() )
		{
			lastFd = fd ;
			lastResult = it->second ;
		}
		else
		{
			lastFd = -1 ;
			lastResult = NULL ;
		}
	}

	return lastResult ;
}

//////////////////////////////////////////////////////////////////////
// Get me the name and size of a given file.

static bool _mapFile(IO::MappedFile& ioMap, const char* name, bool userdir, wchar_t* widePath, size_t wsize)
{
	char buffer[PATH_MAX + 1];

#if platBuild == BUILD_CLIENT && platOS == OS_WIN32
	if(userdir)
	{
		// no bounds checking here, nice
		strcpy(buffer, UserDataPath);
		strcat(buffer, (char*)name);

		if(WWII::ConvertChars(buffer, widePath, wsize))
		{
			if (ioMap.mapFile(widePath))
				return true;
		}
	// Else fall through normally
	}
#else
	(void)userdir;
#endif

	// think this works as char* because when we get to here we're talking about relative paths which are all ascii?
	strcpy(buffer, DataPath) ;
	strcat(buffer, name) ;

#if platOS == OS_WIN32
	for ( size_t i = 0; i < sizeof(buffer) && buffer[i]; ++i )
	{
		if ( buffer[i] == '/' )
			buffer[i] = PATH_SEPARATOR;
	}
#endif

	if(!WWII::ConvertChars(buffer, widePath, wsize))
		return false;

#if platOS == OS_WIN32
	return ioMap.mapFile(widePath);
#else
	return ioMap.mapFile(buffer);
#endif
}

//////////////////////////////////////////////////////////////////////
// cross-platform 'open' wrapper.

int fiodOpen(const char* filename, bool userdir, int prereadSize)
{
	wchar_t widePath[PATH_MAX + 1] = L"";
	char asciiPath[PATH_MAX + 1] = "";

	const int fd = s_filemap.empty() ? FIOD_FD_START : (s_filemap.rbegin()->first + 1);
	ABORT_IF( s_filemap.find(fd) != s_filemap.end() );
	IO::MappedFile* ioMap = new IO::MappedFile();
	s_filemap[fd] = ioMap;
	if ( !_mapFile(*ioMap, filename, userdir, widePath, sizeof(widePath)) )
	{
		delete ioMap;
		s_filemap.erase(fd);
		return -1;
	}

	WWIIConvert(widePath, asciiPath);
	recordRef(asciiPath);

	return fd ;
}

//////////////////////////////////////////////////////////////////////
// Provide access to the IO Mapped File behind an fd.

IO::MappedFile* fiodGetMappedFile(int fd)
{
	return fd >= 0 ? _lookupFileState(fd) : nullptr;
}

//////////////////////////////////////////////////////////////////////
// cross-platform 'read' wrapper

size_t fiodRead(int fd, void* dptr, size_t length)
{
	IO::MappedFile* state = _lookupFileState(fd) ;
	if ( state != NULL )
		return state->read(dptr, length);
	else
		return (int)OS_READ(fd, dptr, length) ;
}

//////////////////////////////////////////////////////////////////////
// cross-platform 'seek' wrapper

size_t fiodSeek(int fd, size_t offset, int from)
{
	IO::MappedFile* state = _lookupFileState(fd) ;
	if ( state != NULL )
	{
		switch (from)
		{
		case fioSEEK_CUR:
			return state->seekCur(offset);
		case fioSEEK_SET:
			return state->seekSet(offset);
		case fioSEEK_END:
			return state->seekEnd(offset);
		default:
			return (size_t)-1;
		}
	}
	else
		return OS_LSEEK(fd, offset, from) ;
}

//////////////////////////////////////////////////////////////////////
// returns 1 (true) if the file is at or beyond eof, 0 (false) otherwise

int fiodEOF(int fd)
{
	IO::MappedFile* state = _lookupFileState(fd) ;
	if ( state != NULL )
		return state->atEof() ? 1 : 0;
	else
		return -1 ;
}

//////////////////////////////////////////////////////////////////////
// Release a file resource

void fiodClose(int fd)
{
	IO::MappedFile* state = _lookupFileState(fd);
	if ( state != NULL )
	{
		delete state;
		s_filemap.erase(fd);
		lastFd = -1;
		lastResult = NULL;
	}
	else
		OS_CLOSE(fd) ;
}

//////////////////////////////////////////////////////////////////////
// External wrapper for getFilePathAndSize

size_t fiodFileSize(const char* name, bool userdir)
{
	wchar_t widePath[PATH_MAX + 1] ;
	return _getFilePathAndSize(name, userdir, widePath, sizeof(widePath)) ;
}

//////////////////////////////////////////////////////////////////////
// Returns a pointer to the path prefix

const char* fiodGetPrepath(void)
{
	return(DataPath);
}

//////////////////////////////////////////////////////////////////////
// Changes the path prefix

#ifndef LIMITED_FIO
void fiodSetPrepath(char* newpath)
{
	if(newpath != NULL)
		lstrcpy(DataPath, newpath);
}
#endif

//////////////////////////////////////////////////////////////////////
// Create a directory within prefix space
//
#if platOS != OS_LINUX
void fiodmkdir(const char* path)
{
	char fullPath[PATH_MAX];

#if platOS == OS_MACOS
	snprintf(fullPath, PATH_MAX-1, "%s%s", macPreferencesPath(), path);
	OS_MKDIR(os_file_path(fullPath), S_IRWXU);
#elif platOS == OS_WIN32

	lstrcpy(fullPath, DataPath);
	lstrcat(fullPath, path);

	wchar_t widePath[PATH_MAX+1];

	if(WWIIConvert(fullPath, widePath))
		OS_WMKDIR(os_file_path(widePath));

#endif
}
#endif

// ------------------------------

#if platOS == OS_WIN32
// WIN versions

LOGICAL fiodAccess(const char* filename, bool userdir)
{
	char	fullpath[PATH_MAX];

#if platBuild == BUILD_CLIENT && platOS == OS_WIN32
	if(userdir)
	{
		lstrcpy(fullpath, UserDataPath);
		lstrcat(fullpath, filename);

		bool success = false;

		wchar_t widePath[PATH_MAX+1];

		if(WWIIConvert(fullpath, widePath))
			success = OS_WACCESS(os_file_path(widePath), 0) == 0;

		if(success)
		{
			return TRUE;
		}

		// Else fall through normally
	}
#endif

	lstrcpy(fullpath, DataPath);
	lstrcat(fullpath, filename);

	return (OS_ACCESS(os_file_path(fullpath), 0) == 0);
}

LOGICAL fiodModified(SINT32* modified, const char* filename, bool userdir)
{
	char	fullpath[PATH_MAX];
	SINT32	retval;
	struct	stat	buffer;

#if platOS == OS_WIN32 && platBuild != BUILD_SERVER
	if(userdir)
	{
		lstrcpy(fullpath, UserDataPath);
		lstrcat(fullpath, filename);

		wchar_t widePath[PATH_MAX+1];

		struct _stat64i32 wbuffer;

		if(WWIIConvert(fullpath, widePath))
			retval = _wstat(os_file_path(widePath), &wbuffer);

		if(retval == 0)
		{
			*modified = (SINT32)wbuffer.st_mtime;
			return TRUE;
		}
		// Else fall through normally
	}
#endif

	lstrcpy(fullpath, DataPath);
	lstrcat(fullpath, filename);
	retval = stat(os_file_path(fullpath), &buffer);

	if (retval == 0)
	{
		*modified = (SINT32)buffer.st_mtime;
	}

	return(retval == 0);
}

#else

// Mac versions

LOGICAL fiodAccess(const char* filename, bool /* userdir */)
{
  char fullpath[1024];
  strcpy(fullpath, DataPath);
  strcat(fullpath, filename);
  return(OS_ACCESS(os_file_path(fullpath), 0) == 0);
}

LOGICAL fiodModified(SINT32* modified, const char* filename, bool /* userdir */)
{
  char fullpath[1024];
  SINT32 retval;
  struct stat buffer;
  strcpy(fullpath, DataPath);
  strcat(fullpath, filename);
  retval = stat(os_file_path(fullpath), &buffer);

  if (retval == 0)
  {
	*modified = (SINT32)buffer.st_mtime;
  }
  return(retval == 0);
}
#endif

// ------------------------------

size_t fiodWrite(int hfile, const void* dptr, size_t length)
{
  return OS_WRITE(hfile, dptr, length);
}

#if platOS == OS_WIN32
int fiodOpenWrite(const char* filename, bool userdir )
#else
int fiodOpenWrite(const char* filename, bool /* userdir */)
#endif
{
  char  fullpath[PATH_MAX];

#if platOS == OS_WIN32 && platBuild != BUILD_SERVER
  if(userdir)
  {
		// Make sure directorys are created since some filenames contain directory paths.
		lstrcpy(fullpath, UserDataPath);
		lstrcat(fullpath, filename);

		wchar_t widePath[PATH_MAX+1];

		WWIIConvert(fullpath, widePath);

	   // Make sure directorys are created since some filenames contain directory paths.
		std::wstring wpath = widePath;

		for(unsigned int i = 0; i < wpath.length(); i++)
		{
			if(wpath[i] == L'/') wpath[i] = L'\\';
		}

		size_t pos = wpath.find(L'\\');
		std::wstring path;

		size_t start = 0;

		// Need to create directories
		while(pos != std::wstring::npos)
		{
			path += wpath.substr(start, pos - start);
			int error = OS_WMKDIR(path.c_str());

			start = pos;
			pos = wpath.find(L'\\', pos + 1);
		}

		return OS_WOPEN(wpath.c_str(), O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);
  }
  else
  {
	lstrcpy(fullpath, DataPath);
	lstrcat(fullpath, filename);
  }
#else
  lstrcpy(fullpath, DataPath);
  lstrcat(fullpath, filename);
#endif

	return OS_OPEN(os_file_path(fullpath), O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);
}

// The host uses "LIMITED_FIO"
#ifdef LIMITED_FIO
size_t fiodGetString(int fd, char* string, size_t length)
{
	IO::MappedFile* state = _lookupFileState(fd) ;
	if ( state != NULL )
	{
		return state->readline(string, length);
	}
	else
	{
		string[0] = 0 ;
		///TODO: OGS: Audit callers and make them take
		/// -1 as a return value.
		return 0 ;
	}
}
#endif

#if platOS == OS_WIN32

void fiodGetDirFileList(SCHAR* dirToSearch, SCHAR*** fileList, SINT32* numFilesInDir)
{	UINT32 numFiles;
	UINT32 longestFileName;
	UINT32 count;
	SCHAR  curWorkingDir[MAX_PATH];
	SCHAR  newWorkingDir[MAX_PATH];
	SCHAR**  list;

	WIN32_FIND_DATAA findData;
	HANDLE fileHandle;


	fileHandle = INVALID_HANDLE_VALUE;

	if(numFilesInDir == NULL || fileList == NULL || fileList == NULL)
	{
		return;
	}

	GetCurrentDirectoryA(MAX_PATH, curWorkingDir);

	if(dirToSearch == NULL)	// searching within the root of the DataPath
	{
		sprintf(newWorkingDir, ".\\");
	}
	else
	{
		sprintf(newWorkingDir, "%s", dirToSearch);
	}

	SetCurrentDirectoryA(newWorkingDir);

	fileHandle = FindFirstFileA("*.*", &findData);
	numFiles = 0;
	longestFileName = 0;

	if(fileHandle != INVALID_HANDLE_VALUE)
	{
		while(FindNextFileA(fileHandle, &findData))
		{
			if(!(findData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
											  FILE_ATTRIBUTE_HIDDEN |
											  FILE_ATTRIBUTE_SYSTEM |
											  FILE_ATTRIBUTE_OFFLINE |
											  FILE_ATTRIBUTE_TEMPORARY)))
			{
				numFiles++;

				if(strlen(findData.cFileName) > longestFileName)
				{
					longestFileName = strlen(findData.cFileName);
				}
			}
		}
	}

	if(numFiles < 1)
	{
		return;
	}

	list = (SCHAR **)memAlloc(sizeof(SCHAR*) * numFiles);

	if(list == NULL)
	{
		SetCurrentDirectoryA(curWorkingDir);
		return;
	}

	for(count = 0; count < numFiles; count++)
	{
		list[count] = (SCHAR*)memAlloc(sizeof(SCHAR) * longestFileName + 1); // dont forget the NULL terminator, duh

		if(list[count] == NULL)
		{
			SetCurrentDirectoryA(curWorkingDir);
			return;
		}
	}

	fileHandle = FindFirstFileA("*.*", &findData);
	numFiles = 0;

	if(fileHandle != INVALID_HANDLE_VALUE)
	{
		while(FindNextFileA(fileHandle, &findData))
		{
			if(!(findData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
											  FILE_ATTRIBUTE_HIDDEN |
											  FILE_ATTRIBUTE_SYSTEM |
											  FILE_ATTRIBUTE_OFFLINE |
											  FILE_ATTRIBUTE_TEMPORARY)))
			{
				strcpy(list[numFiles], findData.cFileName);
				numFiles++;
			}
		}
	}


	*fileList = list;
	*numFilesInDir = numFiles;
	SetCurrentDirectoryA(curWorkingDir);
}
#endif // platOS == OS_WIN32

#if platOS == OS_MACOS
void fiodGetDirFileList(SCHAR* dirToSearch, SCHAR*** fileList, SINT32* numFilesInDir)
{
	DIR* dir;
	struct dirent* ent;
	int i;
	SCHAR* fileName;
	SCHAR** list;
	int numFiles = 0;
	SCHAR newWorkingDir[MAX_PATH];
	SCHAR filePath[MAX_PATH];
	struct stat st;

	if(numFilesInDir == NULL || fileList == NULL) return;

	if(dirToSearch == NULL)
		sprintf(newWorkingDir, "%s", ".\\");
	else
		sprintf(newWorkingDir, "%s", dirToSearch);
	dir = opendir(os_file_path(newWorkingDir));
	if(dir == NULL) return;

// Get number of files in directory
	while((ent = readdir(dir)) != NULL)
	{
		sprintf(filePath, "%s%s%s",newWorkingDir, macFileSeparator(), ent->d_name);
		if(!stat(os_file_path(filePath), &st) && S_ISREG(st.st_mode))
			numFiles++;
	}
	rewinddir(dir);

	if(numFiles == 0) return;

	list = (SCHAR **)memAlloc(sizeof(SCHAR*) * numFiles);

	// If no memory then stop and return
	//  Rick, Do you want to do more?
	if(list == NULL)
	{
		return;
	}

// Add filenames
	for(i = 0; i < numFiles;)
	{
		ent = readdir(dir);
		if(ent == NULL) break;

		sprintf(filePath, "%s%s%s",newWorkingDir, macFileSeparator(), ent->d_name);
		if(!stat(os_file_path(filePath), &st) && S_ISREG(st.st_mode))
		{
			fileName = (SCHAR*)memAlloc(strlen(ent->d_name) + 1);
			// If no memory then stop and return
			//  Rick, Do you want to do more?
			if(fileName == NULL)
			{
				return;
			}
			strcpy(fileName, ent->d_name);
			list[i] = fileName;
			i++;
		}
	}

	closedir(dir);

	*fileList = list;
	*numFilesInDir = numFiles;
}
#endif


/************************************************************************************************
	os_file_path
*************************************************************************************************/

#ifndef LIMITED_FIO
static char* os_file_path(char* path)
{
	char* s = path;
	for(;*s;s++)
	{
		if(*s == CHANGE_SEP)
			*s = OS_SEP;
	}

	return path;
}

static wchar_t* os_file_path(wchar_t* path)
{
#if platOS == OS_WIN32
	wchar_t* s = path;
	for(;*s;s++)
	{
		if(*s == WCHANGE_SEP)
			*s = WOS_SEP;
	}
#endif

	return path;
}
#endif

#if platOS == OS_MACOS

size_t fiocfgRead(int fd, void* dptr, size_t length)
{
  return OS_READ(fd, dptr, length);
}

void fiocfgMkDir(const char* path, bool)
{
	fiodmkdir(path);
}

LOGICAL fiocfgAccess(const char* filename, bool)
{
	char	fullpath[_MAX_PATH];
	snprintf(fullpath,_MAX_PATH-1,"%s%s", macPreferencesPath(), filename);

	return(OS_ACCESS(os_file_path(fullpath), 0) == 0);
}

LOGICAL fiocfgModified(SINT32* modified, const char* filename)
{
	char	fullpath[_MAX_PATH];
	snprintf(fullpath,_MAX_PATH-1,"%s%s", macPreferencesPath(), filename);

	struct	stat	buffer;
	int retval = stat(os_file_path(fullpath), &buffer);
	if (retval == 0)
		*modified = buffer.st_mtime;

	return (retval == 0);
}

size_t fiocfgOpen(const char* filename, bool)
{
	char	fullpath[_MAX_PATH];
	int		hfile = -1;
	struct stat st;

	snprintf(fullpath,_MAX_PATH-1,"%s%s", macPreferencesPath(), filename);
	os_file_path(fullpath);

	stat(fullpath, &st);
	if(S_ISREG(st.st_mode))
		hfile = OS_OPEN(fullpath, O_RDONLY | O_BINARY);

	return(hfile);
}

size_t fiocfgOpenWrite(const char* filename, bool)
{
	char  fullpath[_MAX_PATH];
	int   hfile = -1;

	snprintf(fullpath,_MAX_PATH-1,"%s%s", macPreferencesPath(), filename);
	os_file_path(fullpath);

	hfile = OS_OPEN(fullpath, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);

	return(hfile);
}

SINT32 fiocfgFileSize(const char* filename, bool)
{
	char fullpath[_MAX_PATH];
	struct	stat	buffer;
	SINT32 retval;

	snprintf(fullpath,_MAX_PATH-1,"%s%s", macPreferencesPath(), filename);
	os_file_path(fullpath);

	retval = stat(fullpath, &buffer);

	if (retval == 0)
	{
		// st_size is type off_t
		return((SINT32)buffer.st_size); // Return what the OS thinks it is
	}

	return(-1);
}

#elif platOS != OS_LINUX && platBuild != BUILD_SERVER

void fiocfgMkDir(const char* path, bool userdir)
{
	if(userdir)
	{
		fiodSetPrepath(UserDataPath);
	}

	fiodmkdir(path);

	if(userdir)
	{
		fiodSetPrepath(PREPATH);
	}
}

#endif
