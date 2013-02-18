#include <syscfg.h>	// Path is project specific.
#include "io_mapped_file.h"
// IO::MappedFile -- Based on SMUGL::cl_mappedfile
// Author: Oliver "kfsone" Smith 2012 <oliver@kfs.org>
// Redistribution and re-use fully permitted contingent on inclusion of these 3 lines in copied- or derived- works.
// Revision 3: Added open and close functions so you can re-use a mapped file option.
// Revision 2: Fixed some bad '#if's for Windows,
// Revision 1: Original version.

#if IO_MAP_TYPE == _IMT_UNIX
# include <sys/mman.h>	// Some systems use mman.h instead.
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <fcntl.h>

#define OsALLOCA alloca
#define OsStrlen strlen
#define OsStrcpy strcpy
#else // Now for Windows.
# include <windows.h>

#define OsALLOCA _alloca
#define OsStrlen _tcslen
#define OsStrcpy _tcscpy
#endif

#define ALLOC_STACK_CHARS(x) static_cast<filename_char_t*>(OsALLOCA((x + 1) * sizeof(filename_char_t)))

#include <stdexcept>
#include <string.h>

namespace IO
{

//////////////////////////////////////////////////////////////////////
// Constructor (Posix + Windows versions combined)
// Note: Posix doesn't support wchar_t filenames.

MappedFile::MappedFile(const filename_char_t* const filename_, const filename_char_t* const dirname_)
		: m_basePtr(nullptr)
		, m_startPtr(nullptr)
		, m_endPtr(nullptr)
		, m_currentPtr(nullptr)
		, m_size(0)
#if IO_MAP_TYPE == _IMT_WINDOWS
		, m_handle(INVALID_HANDLE_VALUE)
		, m_mapping(INVALID_HANDLE_VALUE)
#else
		, m_fd(-1)
#endif
{
	if ( !mapFile(filename_, dirname_) )
	{
#if IO_MAP_TYPE == _IMT_WINDOWS
		throw std::runtime_error(strerror(GetLastError()));
#else
		throw std::runtime_error(strerror(errno));
#endif
	}
}

//////////////////////////////////////////////////////////////////////
// Destructor.

MappedFile::~MappedFile()
{
	unmapFile();
}

//////////////////////////////////////////////////////////////////////
// Attempt to open a file. Returns false on error.

bool
MappedFile::mapFile(const filename_char_t* const filename_, const filename_char_t* const dirname_)
{
#if IO_MAP_TYPE == _IMT_UNIX
	// At time of writing, Linux/BSD kernels don't provide widecharacter open()s.
	static_assert(sizeof(filename_char_t) == 1, "widecharacter filenames not supported on this platform");
#endif

	// Make sure we close any currently-open/mapped file.
	unmapFile();

	const filename_char_t* filename = nullptr;
	if (dirname_ && dirname_[0])
	{
		// We have to combine both strings and join them with a filename sep.
		// Figure out how much storage we want and allocate it on the stack.
		const size_t filenameLen = OsStrlen(filename_);
		const size_t dirnameLen = dirname_ ? OsStrlen(dirname_) : 0;
		const size_t storageSize = (filenameLen + 1 + dirnameLen + 1);
		filename_char_t* filenameAlloc = ALLOC_STACK_CHARS(storageSize);
		OsStrcpy(filenameAlloc, dirname_);
		filenameAlloc[dirnameLen] = PATH_SEP_CHAR;
		OsStrcpy(filenameAlloc + dirnameLen + 1, filename_);
		filename = filenameAlloc;
	}
	else
		filename = filename_;

	// Check we can open the file and stat it for size.
#if IO_MAP_TYPE == _IMT_WINDOWS
	// Windows implementation.
	m_handle = CreateFile(filename, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if ( m_handle == INVALID_HANDLE_VALUE )
		return false;

	// Determine file size.
	LARGE_INTEGER size;
	if ( !GetFileSizeEx(m_handle, &size) )
		return unmapFile();

	m_size = (size_t)size.QuadPart;
#else
	// Posix implementation.
	const int fd = OS_OPEN(filename, O_RDONLY);
	if ( fd < 0 )
		return unmapFile();
	m_fd = fd;

	// Use fstat to quickly obtain the file size.
	struct stat stats;
	const int sr = fstat(fd, &stats);
	if ( sr < 0 )
		return unmapFile();
	m_size = stats.st_size;
#endif

	// Empty file? Don't try and mmap it then.
	if ( m_size == 0 )
		return true;

	// Ask the OS to provide an in-memory view of the data; which is
	// basically saying "load this file into buffers like you would,
	// but then give us direct access to the buffer memory".
#if IO_MAP_TYPE == _IMT_WINDOWS
	// Windows implementation.
	m_mapping = CreateFileMapping(m_handle, NULL, PAGE_READONLY, 0, 0, NULL);
	if ( m_mapping == INVALID_HANDLE_VALUE )
		return unmapFile();

	LPVOID const ptr = MapViewOfFileEx(m_mapping, FILE_MAP_READ, 0, 0, 0, NULL);
	if ( ptr == NULL )
		return unmapFile();
#else
	// Posix implementation.
	static const int flags = 
			  MAP_FILE 		// Compatability flag, not really required.
			| MAP_SHARED	// Share buffers with any other mmappers of this file.
			;
	void* const ptr =
		mmap(NULL, m_size + 1, PROT_READ, flags, m_fd, 0);
	if ( ptr == MAP_FAILED )
		return unmapFile();
#endif

	// Both methods return a pointer to the beginning of the
	// OS'es internal buffers. There may not be any data there,
	// and the first access may result in a page fault (the
	// OS has to actually fetch data, akin to the first call
	// of read()).
	m_basePtr = static_cast<void*>(ptr);
	m_startPtr = static_cast<const char*>(ptr);

	// currentPtr is our seek pointer.
	m_currentPtr = m_startPtr;

	// For convenience, pre-calculate where the end of the data is.
	m_endPtr = m_currentPtr + m_size;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Close and unmap the mapping.

bool
MappedFile::unmapFile()
{
	if ( m_basePtr != NULL )
	{
#if IO_MAP_TYPE == _IMT_WINDOWS
		UnmapViewOfFile(m_basePtr);
#else
		munmap(m_basePtr, m_size + 1);
#endif

		m_basePtr = NULL;
		m_startPtr = m_endPtr = m_currentPtr = NULL;
		m_size = 0;
	}

#if IO_MAP_TYPE == _IMT_WINDOWS
	if ( m_mapping != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_mapping);
		m_mapping = INVALID_HANDLE_VALUE;
	}
	if ( m_handle != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}
#else
	if ( m_fd >= 0 )
	{
		close(m_fd);
		m_fd = -1;
	}
#endif

	return false;
}

//////////////////////////////////////////////////////////////////////
// Non-platform specific functions.
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Allow partial copying of mapped files; this creates a sub-view that
// doesn't own the file mapping itself.

MappedFile&
MappedFile::operator = (const MappedFile& rhs_)
{
	if ( rhs_.m_basePtr != NULL )
		throw std::runtime_error("illegal mapped-file operation");
	if ( m_basePtr != NULL )
		throw std::runtime_error("illegal mapped-file overwrite operation");

	m_basePtr = NULL;
	m_startPtr = rhs_.m_startPtr;
	m_endPtr = rhs_.m_endPtr;
	m_currentPtr = rhs_.m_currentPtr;
	m_size = m_endPtr - m_currentPtr;

	return *this;
}

//////////////////////////////////////////////////////////////////////
// Read the next line into a buffer.

size_t
MappedFile::readline(char* into, size_t maxReadLength)
{
	const char* const start = currentPtr();
	size_t bytesToRead = remainingBytes();
	if ( maxReadLength < bytesToRead )
		bytesToRead = maxReadLength;
	const char* const stopPtr = start + bytesToRead;
	while ( currentPtr() < stopPtr )
	{
		if ( atEof() )
			break;
		*into = *(m_currentPtr++);
		if (*into != '\n')
			++into;
		else
			break;
	}
	*into = 0;
	return static_cast<size_t>(currentPtr() - start);
}

}
