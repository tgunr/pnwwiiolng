#include "syscfg.h" // Path is project specific.

#if platOS == OS_MACOS
#include "sysmac/sysMacCompat.h"
#endif

#include "wwii/wwiiHash.h"
#include <map>
#include <set>
#include <string.h>

#if platBuild != BUILD_SERVER
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
# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif

# define FILE_URL_DESIGNATOR    "file://"

#if platBuild == BUILD_SERVER
#include "include/configVars.h"
# define DataPath cv_dataDir.GetString()
#endif

#include "wwii/wwiiUTF.h"

#include "str/stringLib.h"

#ifdef RECORD_FILES
static std::set<std::wstring> ReferencedFiles;

void dumpFileRefs()
{
	printf("\n\nAll referenced files:");
	for ( auto it = ReferencedFiles.begin(), end = ReferencedFiles.end(); it != end; ++it)
	{
		printf("%ls\n", it->c_str());
	}
}

static void recordRef( std::wstring ref )
{
	ReferencedFiles.insert(ref);
}

#else
#define recordRef(x)
#endif

#if platBuild == BUILD_CLIENT
	#if platOS == OS_WIN32
		#define CHANGE_SEP  '/'
		#define OS_SEP      '\\'
		#define WCHANGE_SEP L'/'
		#define WOS_SEP     L'\\'
	#else
		#define CHANGE_SEP  '\\'
		#define OS_SEP      '/'
		#define WCHANGE_SEP L'\\'
		#define WOS_SEP     L'/'
	#endif

	char* os_file_path(char* path) {
		for ( char* s = path ; *s; ++s ) {
			if ( *s == CHANGE_SEP )
				*s = OS_SEP;
		}
		return path;
	}

	wchar_t* os_file_path(wchar_t* path) {
		for ( wchar_t* s = path ; *s; ++s ) {
			if ( *s == WCHANGE_SEP )
				*s = WOS_SEP;
		}
		return path;
	}
#else
	#define os_file_path(x) (x)
#endif

//--------------

UINT32 GetFullPath_unsafe(fullpath_t *path, const char* rawname) {
#if platOS == OS_WIN32
	fullpath_t name[PN_MAX_PATH];
	WWII::ConvertChars(rawname, name, array_size(name));
	name[array_size(name)-1] = L'\0';
#else
	const char* name = rawname
#endif
	return pathprintf(path, PN_MAX_PATH, fiodGetPrepath(), os_file_path(name));
}

//==============================================================================

#if defined(WIN32) || defined(WIN64)
	#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
	#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

namespace {
#if platOS != OS_WIN32
	typedef struct stat stat_t;
	int GetStat(stat_t& s, const fullpath_t* path) {
		return stat(path, &s);
	};
#else
	typedef struct _stat64i32 stat_t;
	int GetStat(stat_t& s, const fullpath_t* path) {
		return _wstat(path, &s);
	};
#endif

	bool GetFileStat(stat_t& s, const char* name, bool userdir, fullpath_t* outpath = nullptr) {
		fullpath_t locpath[PN_MAX_PATH];
		fullpath_t* path = (outpath ? outpath : locpath);

		WITH ( UsingUserdir(userdir) )
			GetFullPath_unsafe(path, name);
		int result = GetStat(s, path);
		if ( ! (result < 0 || ! S_ISREG(s.st_mode)) )
			return true;
		else if ( userdir ) // Look in the default dir as a backup
			return GetFileStat(s, name, false, outpath);
		else
			return false;
	}

	template <size_t N>
	bool GetFileStat(stat_t& s, const char* name, bool userdir, fullpath_t (&outpath)[N]) {
		static_assert ( N == PN_MAX_PATH , "All fio paths must be properly sized" );
		return GetFileStat(s, name, userdir, (fullpath_t*)outpath);
	}
}

//////////////////////////////////////////////////////////////////////
// Load an entire file into memory and return a pointer to the memory
// TODO: Lots of overlap with fiodOpen(), should probably merge some stuff

bool fiodLoad(const char* filename, char*& outPtr, unsigned int* outSize) {
	outPtr = nullptr;
	if ( outSize ) *outSize = 0;

	fullpath_t path[PN_MAX_PATH];
	stat_t s;
	if ( ! GetFileStat(s, filename, false, path) ) {
		dbugLog(_FLA, "Unable to stat %" PRpathT, path);
		return false;
	}

	if ( PN_UNLIKELY(s.st_size == 0) )
		return true;


	int fd = 1;
	OS_OPEN(&fd, path, O_RDONLY | O_BINARY, 0);
	recordRef(widePath);

	if ( PN_UNLIKELY(fd < 0) ) {
		dbugLog(_FLA, "Unable to open %" PRpathT, path);
		return false;
	}

	const size_t page_size = 4096; // Must be a power of two
	const size_t alloc_size = ((size_t)s.st_size + 1 + page_size) & ~page_size;

	char* mem = (char*)memAlloc(alloc_size);
	if ( PN_UNLIKELY(mem == nullptr) ) {
		OS_CLOSE(fd);
		LogAbort("Out of memory loading '%" PRpathT "' (%" PRSizeT " bytes).", path, alloc_size);
		return false;
	}

	int bytes_read = OS_READ(fd, mem, s.st_size);
	OS_CLOSE(fd);

	if ( bytes_read != s.st_size ) {
		dbugLog(_FLA, "Error reading data from %" PRpathT " (fd:%d, size:%u, read:%d)",
		        path, fd, (unsigned int)s.st_size, bytes_read);
		memFree(mem);
		return false;
	}

	mem[bytes_read] = 0;

	outPtr = mem;
	if ( outSize ) *outSize = (unsigned int)bytes_read;

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
			case SEEK_SET:
				newPtr = basePtr + offset;
			break;
			case SEEK_CUR:
				newPtr = curPtr + offset;
			break;
			case SEEK_END:
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

using FILE_MAP = std::unordered_map<SINT32, Fiod::FileState>;
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

int fiodOpen(const char* filename, bool userdir, int prereadSize) {
	dbugLog(_FLD, "fiodOpen(%s, %d, %d)", filename, userdir, prereadSize);

	fullpath_t path[PN_MAX_PATH];
	stat_t s;
	if ( ! GetFileStat(s, filename, userdir, path) ) {
		dbugLog(_FLW, "Could not open file");
		return -1;
	}

	off_t read_size = s.st_size;
	if ( prereadSize > 0 && prereadSize > read_size ) {
		dbugLog(_FLD, "Altering read size to %d", prereadSize);
		read_size = prereadSize;
	}

	// Add one byte to make absolutely sure there is a null byte at the end
	// Then round the size up to the next page size to limit memory fragmentation
	const size_t page_size = 4096; // Must be a power of two
	const size_t alloc_size = ((size_t)read_size + 1 + page_size) & ~page_size;

	char* mem = (char*)memAlloc(alloc_size);
	if ( PN_UNLIKELY(mem == nullptr) ) {
#if platBuild == BUILD_SERVER
		LogAbort("out of memory opening %" PRpathT, path);
#else
		Write_Log(LOG_ALL, LOG_FATAL, "out of memory opening %" PRpathT, path);
		piExitProgram(_FL, "out of memory");
		return -1;
#endif
	} else {
		dbugLog(_FLD, "Alloc size for new file is %" PRSizeT "", alloc_size);
	}

	mem[read_size] = 0;
	int fd = -1;
	try {
		OS_OPEN(&fd, path, O_RDONLY | O_BINARY, 0);
		recordRef(path);

		if ( fd < 0 )
			throw "open failed";

		// Sanity check: We don't think we already have this discriptor open
		assert ( s_fileMap.find(fd) == s_fileMap.end() );

		dbugLog(_FLD, "About to read %" PRSsizeT " bytes from file", read_size);
		off_t bytes_read = OS_READ(fd, mem, read_size);
		if ( bytes_read < read_size )
			throw "read failed/truncated";

		auto& file_map = s_fileMap[fd];
		file_map.Assign(mem, read_size);
		lastFd = fd;
		lastResult = &file_map;

		return fd;
	} catch ( const char* cause ) {
		dbugLog(_FLW, "fiod:%s:%" PRpathT, cause, path);

		if ( fd >= 0 )
			OS_CLOSE(fd);
		memFree(mem);
		return -1;
	}
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

size_t fiodFileSize(const char* name, bool userdir) {
	stat_t s;
	if ( GetFileStat(s, name, userdir) )
		return s.st_size;
	else
		return -1;
}

//////////////////////////////////////////////////////////////////////
// Returns a pointer to the path prefix

#if platBuild == BUILD_CLIENT
namespace {
	bool use_userpath = false;
}

void fiodSetUserpath(bool userpath) {
	use_userpath = userpath;
}

const fullpath_t* fiodGetPrepath()
{
	return use_userpath ? UserDataPath : DataPath;
}
#else
const fullpath_t* fiodGetPrepath() {
	return DataPath;
}
#endif

//////////////////////////////////////////////////////////////////////
// Create a directory within prefix space
//
#if platBuild == BUILD_CLIENT
void fiodmkdir(const char* path)
{
#if platOS == OS_MACOS || platOS == OS_LINUX
	char fullPath[PN_MAX_PATH];
	strprintf(fullPath, "%s%s", macPreferencesPath(), path);
	OS_MKDIR(os_file_path(fullPath), S_IRWXU);
#elif platOS == OS_WIN32
	wchar_t fullPath[PN_MAX_PATH];
	GetFullPath(fullPath, path);
	OS_WMKDIR(os_file_path(fullPath));
#endif
}

// ------------------------------

LOGICAL fiodModified(SINT32* modified, const char* filename, bool userdir) {
	stat_t s;
	if ( GetFileStat(s, filename, userdir) ) {
		*modified = (SINT32)s.st_mtime;
		return true;
	} else {
		return false;
	}
}
#endif // BUILD_CLIENT

// ------------------------------

size_t fiodWrite(int hfile, const void* dptr, size_t length)
{
  return OS_WRITE(hfile, dptr, length);
}

int fiodOpenWrite(const char* filename, bool userdir) {
	fullpath_t fullpath[PN_MAX_PATH];
	WITH ( UsingUserdir(userdir) )
		GetFullPath(fullpath, filename);

	// TODO: Only Windows?
#if platOS == OS_WIN32 && platBuild != BUILD_SERVER
	// We may also need to create directories for userdir files
	if ( userdir ) {
		std::wstring str(fullpath);

		size_t start = 0;
		size_t pos = str.find(PATH_SEPARATOR_WIDE);

		std::basic_string<fullpath_t> path;
		while ( pos != std::basic_string<fullpath_t>::npos ) {
			path += str.substr(start, pos - start);
			OS_WMKDIR(path.c_str());
			start = pos;
			pos = str.find(PATH_SEPARATOR_WIDE, pos + 1);
		}
	}
#endif

	int fd = -1;
	OS_OPEN(&fd, fullpath, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);
	return fd;
}

#if platBuild == BUILD_SERVER
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
	SCHAR**  list;

	WIN32_FIND_DATAA findData;
	HANDLE fileHandle;


	fileHandle = INVALID_HANDLE_VALUE;

	if (numFilesInDir == nullptr || fileList == nullptr || fileList == nullptr)
	{
		return;
	}

	GetCurrentDirectoryA(MAX_PATH, curWorkingDir);

	const char* newWorkingDir = dirToSearch ? dirToSearch : ".\\";
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

#else

static int fiodGetDirListFilter(const struct dirent *ent)
{
    return ent->d_type == DT_REG;
}

void fiodGetDirFileList(SCHAR* dirToSearch, SCHAR*** fileList, SINT32* numFilesInDir)
{
	if (numFilesInDir == nullptr || fileList == nullptr) {
		return;
    }

    char *truepath = realpath(safe_os_file_path<char>(dirToSearch ?: ".").c_str(), nullptr);
    struct dirent **entries = nullptr;
    int nentries = 0;

    nentries = scandir(truepath, &entries, fiodGetDirListFilter, nullptr);
    if (nentries < 0) {
        *fileList = nullptr;
        *numFilesInDir = 0;
        free(truepath);
        return;
    }

    SCHAR **resultList = static_cast<SCHAR **>(calloc(static_cast<size_t>(nentries), sizeof(SCHAR *)));

    *numFilesInDir = nentries;
    while (nentries--) {
// Hack.
#if platOS == OS_WIN32
const char sep = '\\';
#else
const char sep = '/';
#endif
        if (asprintf(&resultList[nentries], "%s%c%s", truepath, sep, entries[nentries]->d_name) < 0) {
            *fileList = nullptr;
            *numFilesInDir = 0;
            free(truepath);
            return;
        }
        free(entries[nentries]);
    }
    free(entries);
    free(truepath);
    *fileList = resultList;
}

#endif

#if platBuild == BUILD_SERVER
#include "libxml/xmlIO.h"

struct Context
{
	Context(int fd) : m_fd(fd) {}
	int m_fd;
};

void fioXmlRegisterInputCallbacks()
{
	xmlRegisterInputCallbacks(
		// input match: always report willing to serve.
		[](const char* filename) { return 1; },
		// xmlInputOpenCallback: returns an opaque handle disguised as a void*
		/// HACK: fiodOpen returns -1 on error, libxml expects a 0 for error.
		/// so I'm bumping the file descriptor by one for all operations here.
		[](const char* filename) -> void* { int fd = fiodOpen(filename); return fd == -1 ? nullptr : new Context(fd); },
		// xmlInputReadCallback: passed the handle, buffer and len
		[](void* ctx, char* buffer, int len) -> int { return ctx ? fiodRead(static_cast<Context*>(ctx)->m_fd, buffer, len) : -1; },
		// xmlInputCloseCallback: close file
		[](void* ctx) { fiodClose(static_cast<Context*>(ctx)->m_fd); delete static_cast<Context*>(ctx); return 0; }
	);
}
#endif

#if platOS == OS_MACOS && platBuild == BUILD_CLIENT

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
	strprintf(fullpath,"%s%s", macPreferencesPath(), filename);
    int fd = -1;
    if (OS_OPEN(&fd, os_file_path(fullpath), O_RDONLY, 0) == 0) { // fix for 1083
        OS_CLOSE(fd);
        return TRUE;
    }
    return FALSE;
}

LOGICAL fiocfgModified(SINT32* modified, const char* filename)
{
	char	fullpath[_MAX_PATH];
	strprintf(fullpath,"%s%s", macPreferencesPath(), filename);

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

	strprintf(fullpath,"%s%s", macPreferencesPath(), filename);
	os_file_path(fullpath);

	stat(fullpath, &st);
	if(S_ISREG(st.st_mode))
		OS_OPEN(&hfile, fullpath, O_RDONLY | O_BINARY, 0);

	return(hfile);
}

size_t fiocfgOpenWrite(const char* filename, bool)
{
	char  fullpath[_MAX_PATH];
	int   hfile = -1;

	strprintf(fullpath,"%s%s", macPreferencesPath(), filename);
	os_file_path(fullpath);

	OS_OPEN(&hfile, fullpath, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);

	return(hfile);
}

SINT32 fiocfgFileSize(const char* filename, bool)
{
	char fullpath[_MAX_PATH];
	struct	stat	buffer;
	SINT32 retval;

	strprintf(fullpath,"%s%s", macPreferencesPath(), filename);
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

LOGICAL fiocfgAccess(const char* filename, bool userdir)
{
	WITH ( UsingUserdir(userdir) ) {
		int fd = fiodOpen(filename, true);
		if (fd != -1)
			fiodClose(fd);
		return fd != -1;
	}
}

void fiocfgMkDir(const char* path, bool userdir)
{
	WITH ( UsingUserdir(userdir) )
		fiodmkdir(path);
}

#endif
