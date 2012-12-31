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
#else // Now for Windows.
# include <windows.h>
#endif

#include <stdexcept>
#include <string.h>

namespace IO
{

//////////////////////////////////////////////////////////////////////
// Populate filename with the combination of dirname and filename.

static void
_populateFilename(filename_char_t* const dest, const filename_char_t* const filename, const filename_char_t* const dirname)
{
	filename_char_t* const end = dest + PATH_MAX ;
	filename_char_t* ptr = dest ;
	if ( dirname != NULL && dirname[0] != 0 )
	{
		for ( const filename_char_t* src = dirname ; *src != 0 && ptr < end ; )
			*(ptr++) = *(src++) ;
		if ( *(ptr - 1) != PATH_SEP_CHAR )
			*(ptr++) = PATH_SEP_CHAR ;
	}

	for ( const filename_char_t* src = filename ; *src != 0 && ptr < end ; )
		*(ptr++) = *(src++) ;
	*ptr = 0 ;
}

//////////////////////////////////////////////////////////////////////
// Constructor (Posix + Windows versions combined)
// Note: Posix doesn't support wchar_t filenames.

MappedFile::MappedFile(const filename_char_t* const filename_, const filename_char_t* const dirname_)
	: m_basePtr(NULL)
	, m_endPtr(NULL)
	, m_currentPtr(NULL)
	, m_size(0)
	, m_lineNo(0)
#if IO_MAP_TYPE == _IMT_WINDOWS
	, m_handle(INVALID_HANDLE_VALUE)
	, m_mapping(INVALID_HANDLE_VALUE)
#else // Unix uses a file descriptor.
	, m_fd(-1)
#endif
{
	if ( !mapFile(filename_, dirname_) )
	{
#if IO_MAP_TYPE == _IMT_WINDOWS
		throw std::runtime_error(strerror(GetLastError())) ;
#else
		throw std::runtime_error(strerror(errno)) ;
#endif
	}
}

//////////////////////////////////////////////////////////////////////
// Destructor.

MappedFile::~MappedFile()
{
	unmapFile() ;
}

//////////////////////////////////////////////////////////////////////
// Attempt to open a file. Returns false on error.

bool
MappedFile::mapFile(const filename_char_t* const filename_, const filename_char_t* const dirname_)
{
#if IO_MAP_TYPE == _IMT_UNIX
	// At time of writing, Linux/BSD kernels don't provide widecharacter open()s.
	static_assert(sizeof(filename_char_t) == 1, "widecharacter filenames not supported on this platform") ;
#endif

	// Make sure we close any currently-open/mapped file.
	unmapFile() ;

	_populateFilename(m_filename, filename_, dirname_) ;

	// Check we can open the file and stat it for size.
#if IO_MAP_TYPE == _IMT_WINDOWS
	// Windows implementation.
	m_handle = CreateFile(m_filename, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL) ;
	if ( m_handle == INVALID_HANDLE_VALUE )
		return false ;

	// Determine file size.
	LARGE_INTEGER size ;
	if ( !GetFileSizeEx(m_handle, &size) )
		return unmapFile() ;

	m_size = (size_t)size.QuadPart ;
#else
	// Posix implementation.
	const int fd = OS_OPEN(m_filename, O_RDONLY) ;
	if ( fd < 0 )
		return unmapFile();
	m_fd = fd ;

	// Use fstat to quickly obtain the file size.
	struct stat stats ;
	const int sr = fstat(fd, &stats) ;
	if ( sr < 0 )
		return unmapFile() ;
	m_size = stats.st_size ;
#endif

	// Empty file? Don't try and mmap it then.
	if ( m_size == 0 )
		return true ;

	// Ask the OS to provide an in-memory view of the data; which is
	// basically saying "load this file into buffers like you would,
	// but then give us direct access to the buffer memory".
#if IO_MAP_TYPE == _IMT_WINDOWS
	// Windows implementation.
	m_mapping = CreateFileMapping(m_handle, NULL, PAGE_READONLY, 0, 0, NULL) ;
	if ( m_mapping == INVALID_HANDLE_VALUE )
		return unmapFile() ;

	LPVOID const ptr = MapViewOfFileEx(m_mapping, FILE_MAP_READ, 0, 0, 0, NULL) ;
	if ( ptr == NULL )
		return unmapFile() ;
#else
	// Posix implementation.
	static const int flags = 
			  MAP_FILE 		// Compatability flag, not really required.
			| MAP_SHARED	// Share buffers with any other mmappers of this file.
			;
	void* const ptr =
		mmap(NULL, m_size + 1, PROT_READ, flags, m_fd, 0) ;
	if ( ptr == MAP_FAILED )
		return unmapFile() ;
#endif

	// Both methods return a pointer to the beginning of the
	// OS'es internal buffers. There may not be any data there,
	// and the first access may result in a page fault (the
	// OS has to actually fetch data, akin to the first call
	// of read()).
	m_basePtr = (void*)ptr ;

	// currentPtr is our seek pointer.
	m_currentPtr = static_cast<const char*>(m_basePtr) ;

	// For convenience, pre-calculate where the end of the data is.
	m_endPtr = m_currentPtr + m_size ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Close and unmap the mapping.

bool
MappedFile::unmapFile()
{
	if ( m_basePtr != NULL )
	{
#if IO_MAP_TYPE == _IMT_WINDOWS
		UnmapViewOfFile(m_basePtr) ;
#else
		munmap(m_basePtr, m_size + 1) ;
#endif

		m_basePtr = NULL ;
		m_endPtr = m_currentPtr = NULL ;
		m_size = 0 ;
	}

#if IO_MAP_TYPE == _IMT_WINDOWS
	if ( m_mapping != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_mapping) ;
		m_mapping = INVALID_HANDLE_VALUE ;
	}
	if ( m_handle != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_handle) ;
		m_handle = INVALID_HANDLE_VALUE ;
	}
#else
	if ( m_filename[0] != 0 && m_fd >= 0 )
	{
		close(m_fd) ;
		m_fd = -1 ;
	}
#endif

	return false ;
}

//////////////////////////////////////////////////////////////////////
// Non-platform specific functions.
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Find the last interesting character of the current line.

bool
MappedFile::advanceToEndOfLine()
{
	const char* ptr = m_currentPtr ;
	while ( ptr < m_endPtr )
	{
		const char cur = *(ptr++) ;
		if ( cur == ';' || cur == '\r' || cur == '\n' )
			return true ;
		// Quoted comments ... skip them.
		if ( cur == '\\' && ptr < m_endPtr && *ptr == ';' )
			++ptr ;
		if ( ! isspace(cur) )
			m_currentPtr = ptr ;
	}
	return false ;
}

//////////////////////////////////////////////////////////////////////
// Skip to the start of the next line.

bool
MappedFile::advanceToNextLine()
{
	while ( !atEof() )
	{
		const char cur = *m_currentPtr ;
		// We support both \r and \n so that files work
		// across platforms too. If this isn't one of
		// them, then we're still on the line.
		if ( cur != '\r' && cur != '\n')
		{
			++m_currentPtr ;
			continue ;
		}

		// Now we've reached an end-of-line character.
		++m_lineNo ;
		++m_currentPtr ;

		// Did that put us at EOF?
		if ( atEof() )
			return false ;

		// But what if it put us at a different EOL,
		// as happens on systems with \r\n?
		if ( *m_currentPtr != cur
			&& (*m_currentPtr == '\r' || *m_currentPtr == '\n' ) )
		{
			++m_currentPtr ;
			return !atEof() ;
		}

		// We reached the next line.
		return true ;
	}

	return false ;
}

//////////////////////////////////////////////////////////////////////
// Skip to the end of the current paragraph.

bool
MappedFile::advanceToEndOfParagraph()
{
	while ( !atEof() )
	{
		// Seek to the start of the next line.
		if ( !advanceToNextLine() )
			return false ;

		if ( *m_currentPtr == '\r' || *m_currentPtr == '\n' )
			return true ;
	}

	return false ;
}

//////////////////////////////////////////////////////////////////////
// Advance to the next non-whitespace character, with linecounts.

bool
MappedFile::advanceToNonWhitespace()
{
	while ( !atEof() )
	{
		if ( !isspace(*m_currentPtr) )
			return true ;
		if ( *m_currentPtr == ';' )
		{
			if ( !advanceToNextLine() )
				return false ;
		}
		++m_currentPtr ;
	}
	return false ;
}

//////////////////////////////////////////////////////////////////////
// Advance past the current word.

bool
MappedFile::advancePastCurrentWord()
{
	while ( m_currentPtr < m_endPtr )
	{
		if ( isspace(*m_currentPtr) )
			return true ;
		++m_currentPtr ;
	}

	return false ;
}

//////////////////////////////////////////////////////////////////////
// Allow partial copying of mapped files; this creates a sub-view that
// doesn't own the file mapping itself.

MappedFile&
MappedFile::operator = (const MappedFile& rhs_)
{
	if ( rhs_.m_filename[0] == 0 || rhs_.m_basePtr != NULL )
		throw std::runtime_error("illegal mapped-file operation") ;
	if ( m_filename[0] != 0 || m_basePtr != NULL )
		throw std::runtime_error("illegal mapped-file overwrite operation") ;

	m_filename[0] = (filename_char_t)0 ;
	m_basePtr = NULL ;
	m_endPtr = rhs_.m_endPtr ;
	m_currentPtr = rhs_.m_currentPtr ;
	m_size = m_endPtr - m_currentPtr ;
	m_lineNo = rhs_.m_lineNo ;

	return *this ;
}

}
