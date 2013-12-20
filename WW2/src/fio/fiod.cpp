#include <syscfg.h> // Path is project specific.

#include "wwii/wwiiHash.h" // does this work on linux?
# include "dbug/dbug.h"
#include <map>
#include <set>

#if platOS != OS_LINUX && platBuild != TEUL_SERVER
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
#else
# include <dirent.h>
# include <unistd.h>
# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif

#ifndef LIMITED_FIO
 static char* os_file_path(char* path);
 static wchar_t* os_file_path(wchar_t* path);
#else
# define os_file_path(x) (x)
#endif

#if platBuild == TEUL_SERVER
#include "wwii/wwiiConfigVars.h"
# define DataPath cv_dataDir.GetString()
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
	SINT32 retval = 0;

	char buffer[PATH_MAX + 1];

#if platBuild == TEUL_CLIENT && platOS == OS_WIN32
	if(userdir)
	{
		// no bounds checking here, nice
		pathprintf(buffer, UserDataPath, name);

		if(WWII::ConvertChars(buffer, widePath, wsize))
		{
			struct _stat64i32 wStatBuffer;

			retval = _wstat(os_file_path(widePath), &wStatBuffer);

			if (retval == 0)
			{
				return wStatBuffer.st_size; // Return what the OS thinks it is
			}
		}
	// Else fall through normally
	}
#endif

	// think this works as char* because when we get to here we're talking about relative paths which are all ascii?
	size_t pathLen = pathprintf(buffer, DataPath, name);
	for ( size_t i = 0; i < pathLen; ++i )
	{
		if ( buffer[i] == CHANGE_SEP )
			buffer[i] = PATH_SEPARATOR;
	}

	struct stat statBuffer;
	retval = stat(os_file_path(buffer), &statBuffer);

#if platOS != OS_WIN32
	///TODO: OGS: I think this works under Windows too...
	if ( retval < 0 || !S_ISREG(statBuffer.st_mode) )
	{
		return -1;
	}
#else
	///TODO: OGS: See above: This allows Windows to [try to] open directories like files.
	if ( retval < 0 )
	{
		return -1;
	}
#endif

	// make sure and update our passed in widePath to the found path
	if(!WWII::ConvertChars(buffer, widePath, wsize))
		return -1;

	return statBuffer.st_size; // Return what the OS thinks it is
}

//////////////////////////////////////////////////////////////////////
// Load an entire file into memory and return a pointer to the memory

bool
fiodLoad(const char* filename, char*& outPtr, unsigned int* outSize)
{
	wchar_t	widePath[PATH_MAX + 1];
	char	asciiPath[PATH_MAX + 1];
	widePath[0] = '\0';
	asciiPath[0] = '\0';

	outPtr = nullptr;

	off_t fileSize = _getFilePathAndSize(filename, false, widePath, sizeof(widePath));
	if ( PN_UNLIKELY(fileSize < 0) )
	{
		DEBUG_ERR("Error performing stat on '%s'.", filename);
		return false;
	}

	if ( PN_UNLIKELY(fileSize == 0) )
		return true;

	WWIIConvert(widePath, asciiPath);

#if platOS == OS_WIN32
	// already been converted and path seperators fixed
	int fd = OS_WOPEN(widePath, O_RDONLY | O_BINARY);
	recordRef(widePath);
#else
	int fd = OS_OPEN(os_file_path(asciiPath), O_RDONLY | O_BINARY);
	recordRef(os_file_path(asciiPath));
#endif
	if ( PN_UNLIKELY(fd < 0) )
	{
		DEBUG_ERR("Unable to open '%s'.", asciiPath);
		return false;
	}

	static const size_t _PAGE_SIZE = 4096;		// *MUST* be a power of two
	const size_t allocSize = ((size_t)fileSize + 1 + _PAGE_SIZE) & ~_PAGE_SIZE;

	char* mem = (char*)memAlloc(allocSize);
	if ( PN_UNLIKELY(mem == nullptr) )
	{
		OS_CLOSE(fd);
		DEBUG_FATAL("Out of memory loading '%s' (%" PRSizeT " bytes).", filename, allocSize);
		return false;
	}

	int bytesRead = OS_READ(fd, mem, fileSize);
	OS_CLOSE(fd);

	if ( bytesRead < 0 )
	{
		DEBUG_ERR("Error reading data from %s (fd:%d, size:%u)", asciiPath, fd, (unsigned int)fileSize);
		memFree(mem);
		return false;
	}

	if ( bytesRead != fileSize )
	{
		DEBUG_ERR("Read error reading %s, got %d bytes instead of %u.", filename, bytesRead, (unsigned int)fileSize);
	}

	mem[bytesRead] = 0;

	if ( outSize != nullptr )
		*outSize = (unsigned int)fileSize;

	outPtr = mem;

	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/* Original fiod code would byte-by-byte read files;
 * this is a replacement which reads the entire file
 * into memory and uses fio as a simplified stdio
 * replacement */
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

namespace Fiod
{
	struct FileState
	{
		FileState() : basePtr(nullptr), curPtr(nullptr), endPtr(nullptr), bytes(0) {}
		~FileState() { if ( basePtr != nullptr ) memFree(basePtr); }

		char* basePtr;
		char* curPtr;
		char* endPtr;
		size_t bytes;

		void Assign(char* base, size_t numBytes)
		{
			curPtr = basePtr = base;
			endPtr = basePtr + numBytes;
			bytes = numBytes;
		}

		size_t  BytesRemaining() const { return (size_t)(endPtr - curPtr); }
		bool IsAtEof() const { return curPtr >= endPtr; }

		size_t Read(void* dest, size_t length)
		{
			const size_t readLen = std::min(BytesRemaining(), length);
			if ( readLen > 0 )
			{
				memcpy(dest, curPtr, readLen);
				curPtr = curPtr + readLen;
			}
			return readLen;
		}

		size_t Seek(size_t offset, int from)
		{
			char* newPtr = nullptr;
			switch ( from )
			{
			case fioSEEK_SET:
				newPtr = basePtr + offset;
			break;
			case fioSEEK_CUR:
				newPtr = curPtr + offset;
			break;
			case fioSEEK_END:
				newPtr = endPtr + offset;
			break;
			}
			if ( newPtr < basePtr || newPtr > endPtr )
				return (size_t)-1;
			curPtr = newPtr;
			return (size_t)(curPtr - basePtr);
		}
	};
}

// We use a map of file descriptors to track memory allocations
// for fiod files.

#if platOS == OS_LINUX
typedef std::unordered_map<SINT32, Fiod::FileState> FILE_MAP;
#else
typedef stdext::hash_map<SINT32, Fiod::FileState> FILE_MAP;
#endif

static FILE_MAP s_fileMap;

//////////////////////////////////////////////////////////////////////
// Lookup a Fiod::FileState from a descriptor

static int lastFd = -1;
static Fiod::FileState* lastResult = nullptr;

static Fiod::FileState*
_lookupFileState(int fd)
{
	if ( fd != lastFd )
	{
		FILE_MAP::iterator it = s_fileMap.find(fd);
		if ( it != s_fileMap.end() )
		{
			lastFd = fd;
			lastResult = &it->second;
		}
		else
		{
			lastFd = -1;
			lastResult = nullptr;
		}
	}

	return lastResult;
}

//////////////////////////////////////////////////////////////////////
// cross-platform 'open' wrapper.

int fiodOpen(const char* filename, bool userdir, int prereadSize)
{
	wchar_t widePath[PATH_MAX + 1];
	char asciiPath[PATH_MAX + 1];
	widePath[0] = 0;
	asciiPath[0] = 0;

	off_t fileSize = _getFilePathAndSize(filename, userdir, widePath, sizeof(widePath));

	if ( fileSize < 0 )
	{
		return - 1;
	}

	if ( prereadSize > 0 )
	{
	fileSize = prereadSize; // if too big will be caught below
	}

	WWIIConvert(widePath, asciiPath);

	// Add one byte so I can make absolutely sure there
	// is a null byte at the end :)
	// Then round the size up to the next page size to
	// limit memory fragmentation.

	static const size_t _PAGE_SIZE = 4096;		// *MUST* be a power of two
	const size_t allocSize = ((size_t)fileSize + 1 + _PAGE_SIZE) & ~_PAGE_SIZE;

	char* mem = (char*)memAlloc(allocSize);
	if ( PN_UNLIKELY(mem == nullptr) )
	{
#if platBuild == TEUL_SERVER
		LogAbort("out of memory opening %s", asciiPath);
#else
		Write_Log(LOG_ALL, LOG_FATAL, "out of memory opening %s", asciiPath);
		piExitProgram(__FILE__, __LINE__, "out of memory");
		return -1;
#endif
	}

	mem[fileSize] = 0;

	int fd = -1;

	try
	{
#if platOS == OS_WIN32
		// already been converted and path separators fixed
		fd = OS_WOPEN(widePath, O_RDONLY | O_BINARY);
		recordRef(asciiPath);
#else
		fd = OS_OPEN(os_file_path(asciiPath), O_RDONLY | O_BINARY);
		recordRef(os_file_path(asciiPath));
#endif
		if ( fd < 0 )
			throw "open failed";

		// Sanity check: we don't already think we have this
		// descriptor open.
		assert( s_fileMap.find(fd) == s_fileMap.end() );

		// Try to read the file contents into the allocation
		off_t bytesRead = OS_READ(fd, mem, fileSize);
		if ( bytesRead < fileSize )
			throw "read failed/truncated";

		// Index this allocation into the file map
		auto& fileMap = s_fileMap[fd];
		fileMap.Assign(mem, fileSize);
		lastFd = fd;
		lastResult = &fileMap;
	}
	catch ( const char* cause )
	{
		// Report
#if platBuild == TEUL_SERVER
		dbugLog(_FLD, "fiod:%s:%s", cause, asciiPath);
#else
		Write_Log(LOG_OLD, LOG_INFO, "fiod:%s:%s", cause, asciiPath);
#endif

		// Relinquish the file descriptor
		if ( fd >= 0 )
			OS_CLOSE(fd);

		// Free the memory
		memFree(mem);

		return -1;
	}

	return fd;
}

//////////////////////////////////////////////////////////////////////
// cross-platform 'read' wrapper

size_t fiodRead(int fd, void* dptr, size_t length)
{
	Fiod::FileState* state = _lookupFileState(fd);

	if ( state == nullptr )
		return (int)OS_READ(fd, dptr, length);

	return state->Read(dptr, length);
}

//////////////////////////////////////////////////////////////////////
// cross-platform 'seek' wrapper

size_t fiodSeek(int fd, size_t offset, int from)
{
	Fiod::FileState* state = _lookupFileState(fd);

	if ( state == nullptr )
		return OS_LSEEK(fd, offset, from);

	return state->Seek(offset, from);
}

//////////////////////////////////////////////////////////////////////
// allow the caller to see the internal data buffer, like mmap.

size_t fiodMap(int fd, const void*& data)
{
	Fiod::FileState* state = _lookupFileState(fd);
	if (state != nullptr && state->curPtr < state->endPtr)
	{
		data = state->curPtr;
		return static_cast<size_t>(state->endPtr - state->curPtr);
	}
	data = nullptr;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// returns 1 (true) if the file is at or beyond eof, 0 (false) otherwise

int fiodEOF(int fd)
{
	Fiod::FileState* state = _lookupFileState(fd);

	if ( state == nullptr )
		return -1;

	return state->IsAtEof() ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////
// Release a file resource

void fiodClose(int fd)
{
	OS_CLOSE(fd);

	if ( lastFd == fd )
	{
		lastFd = -1;
		lastResult = nullptr;
	}

	FILE_MAP::iterator it = s_fileMap.find(fd);

	if ( it != s_fileMap.end() )
	{
		// Destructor will handle cleanup for us.
		s_fileMap.erase(it);
	}
}

//////////////////////////////////////////////////////////////////////
// External wrapper for getFilePathAndSize

size_t fiodFileSize(const char* name, bool userdir)
{
	wchar_t widePath[PATH_MAX + 1];
	return _getFilePathAndSize(name, userdir, widePath, sizeof(widePath));
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
	if (newpath == nullptr)
		return;
#if platBuild == TEUL_SERVER
		cv_dataDir.SetString(newPath);
#else
		strncpy(DataPath, newpath, sizeof(DataPath));
#endif
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

	pathprintf(fullPath, DataPath, path);

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

#if platBuild == TEUL_CLIENT && platOS == OS_WIN32
	if(userdir)
	{
		pathprintf(fullpath, UserDataPath, filename);

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

	pathprintf(fullpath, DataPath, filename);

	return (OS_ACCESS(os_file_path(fullpath), 0) == 0);
}

LOGICAL fiodModified(SINT32* modified, const char* filename, bool userdir)
{
	char	fullpath[PATH_MAX];
	SINT32	retval;
	struct	stat	buffer;

#if platOS == OS_WIN32 && platBuild != TEUL_SERVER
	if(userdir)
	{
		pathprintf(fullpath, UserDataPath, filename);

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

	pathprintf(fullpath, DataPath, filename);
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
  pathprintf(fullpath, DataPath, filename);
  return(OS_ACCESS(os_file_path(fullpath), 0) == 0);
}

LOGICAL fiodModified(SINT32* modified, const char* filename, bool /* userdir */)
{
  char fullpath[1024];
  SINT32 retval;
  struct stat buffer;
  pathprintf(fullpath, DataPath, filename);
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

#if platOS == OS_WIN32 && platBuild != TEUL_SERVER
  if(userdir)
  {
		// Make sure directorys are created since some filenames contain directory paths.
		pathprintf(fullpath, UserDataPath, filename);

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
#endif
	  pathprintf(fullpath, DataPath, filename);

	return OS_OPEN(os_file_path(fullpath), O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);
}

// The host uses "LIMITED_FIO"
#ifdef LIMITED_FIO
size_t fiodGetString(int fd, char* string, size_t length)
{
	FILE_MAP::iterator it = s_fileMap.find(fd);
	if ( it == s_fileMap.end() )
	{
		string[0] = 0;
		///TODO: OGS: Audit callers and make them take
		/// -1 as a return value.
		return 0;
	}

	Fiod::FileState& fs = it->second;
	const char* const endPtr = fs.curPtr + std::min((size_t)length, fs.BytesRemaining());
	char* writePtr = string;
	while ( fs.curPtr < endPtr && *fs.curPtr && *fs.curPtr != '\n' )
	{
		*(writePtr++) = *(fs.curPtr++);
	}
	*writePtr = 0;

	// Treat the \n as consumed.
	if ( *fs.curPtr == '\n' )
		++fs.curPtr;

	return (size_t)(writePtr - string);
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

	if (numFilesInDir == nullptr || fileList == nullptr || fileList == nullptr)
	{
		return;
	}

	GetCurrentDirectoryA(MAX_PATH, curWorkingDir);

	if (dirToSearch == nullptr)	// searching within the root of the DataPath
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

	if (PN_UNLIKELY(list == nullptr))
	{
		SetCurrentDirectoryA(curWorkingDir);
		return;
	}

	for(count = 0; count < numFiles; count++)
	{
		list[count] = (SCHAR*)memAlloc(sizeof(SCHAR) * longestFileName + 1); // dont forget the nullptr terminator, duh

		if (PN_UNLIKELY(list[count] == nullptr))
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

	if (numFilesInDir == nullptr || fileList == nullptr)
		return;

	if (dirToSearch == nullptr)
		sprintf(newWorkingDir, "%s", ".\\");
	else
		sprintf(newWorkingDir, "%s", dirToSearch);
	dir = opendir(os_file_path(newWorkingDir));
	if (dir == nullptr)
		return;

// Get number of files in directory
	while ((ent = readdir(dir)) != nullptr)
	{
		sprintf(filePath, "%s%s%s",newWorkingDir, macFileSeparator(), ent->d_name);
		if(!stat(os_file_path(filePath), &st) && S_ISREG(st.st_mode))
			numFiles++;
	}
	rewinddir(dir);

	if (numFiles == 0)
		return;

	list = (SCHAR **)memAlloc(sizeof(SCHAR*) * numFiles);

	// If no memory then stop and return
	//  Rick, Do you want to do more?
	if (PN_UNLIKELY(list == nullptr))
	{
		return;
	}

// Add filenames
	for(i = 0; i < numFiles;)
	{
		ent = readdir(dir);
		if (ent == nullptr) break;

		sprintf(filePath, "%s%s%s",newWorkingDir, macFileSeparator(), ent->d_name);
		if(!stat(os_file_path(filePath), &st) && S_ISREG(st.st_mode))
		{
			fileName = (SCHAR*)memAlloc(strlen(ent->d_name) + 1);
			// If no memory then stop and return
			//  Rick, Do you want to do more?
			if (PN_UNLIKELY(fileName == nullptr))
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

#elif platOS != OS_LINUX && platBuild != TEUL_SERVER

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
