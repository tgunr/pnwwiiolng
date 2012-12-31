#pragma once
// IO::MappedFile -- Based on SMUGL::cl_mappedfile
// Author: Oliver "kfsone" Smith 2012 <oliver@kfs.org>
// Redistribution and re-use fully permitted contingent on inclusion of these 3 lines in copied- or derived- works.

////////////////////////////////////////////////////////////////////// Doxygen documentation.
//! @class IO::MappedFile
//! @brief Provide maximum performance to a file by exposing it as in-memory data.
//!
//! @detail IO::MappedFile provides an abstraction layer over the
//! Windows/Linux memory-mapped file concepts.
//!
//! Essentially, eliminates the "read()" step from file IO access
//! by giving you access to the buffers used by read.
//!
//! Normally, read() has to have the OS load data from disk into
//! private buffers and then has to copy that data into user-space
//! memory (the buffers you supply). 
//!
//! Mapped files gives you protected access to those private buffers
//! in a way that provides on-demand loading of the file (presumably
//! use the system's MMU).
//!
//! Benefits:
//!  - Reduces memory pressure (no copies),
//!  - Increases cache coherency (file data in a single location),
//!  - Reduces memory usage/management overhead,
//!  - Works with the OSes disk-buffer management rather than against.
//!
//! Also implements several utility functions for dealing with text files.


////////////////////////////////////////////////////////////////////// Platform configuration.
// IO_MAP_TYPE
//	Whether to use CreateMappedFile (_IMT_WINDOWS) or mmap (_IMT_UNIX)
// filename_char_t
//	Character type to use for filenames: Windows supports widechar
//  filenames, Linux/Mac use ascii-encoding.
// PATH_SEP_CHAR
//	Because under DOS '/' was the argument separator

#define _IMT_WINDOWS	0
#define _IMT_UNIX		1

#if defined(WIN32) || defined(WIN64) || defined(_MSCVER) || defined(_MSC_VER)

	// Windows

# define IO_MAP_TYPE _IMT_WINDOWS
# if !defined(PATH_MAX)
#  define PATH_MAX 4096		// Tut, tut.
# endif

# if defined(UNICODE)
#  include <wchar.h>
#  define filename_char_t wchar_t
#  define PATH_SEP_CHAR L'\\'
# else
#  define filename_char_t char
#  define PATH_SEP_CHAR '\\'
# endif

#elif defined(__APPLE__)

	// MacOS X

# define IO_MAP_TYPE _IMT_UNIX
# define filename_char_t char
# define PATH_SEP_CHAR '/'

#elif defined(__GNUC__)

	// Unix / Linux

# define IO_MAP_TYPE _IMT_UNIX
# define filename_char_t char
# define PATH_SEP_CHAR '/'

#endif

#include <stdlib.h>
#include <limits.h>

namespace IO
{

class MappedFile
{
public:
	MappedFile()
		: m_basePtr(NULL)
		, m_endPtr(NULL)
		, m_currentPtr(NULL)
		, m_size(0)
		, m_lineNo(0)
#if IO_MAP_TYPE == _IMT_WINDOWS
        , m_handle(INVALID_HANDLE_VALUE)
        , m_mapping(INVALID_HANDLE_VALUE)
#else
        , m_fd(-1)
#endif
		{
			m_filename[0] = 0 ;
		}

	//! Create an in-memory view of a file from disk,
	//! while trying to avoid loading it from disk.
	//! @param[in] filename_ the file/sub-path of the file to open.
	//! @param[in] dirname_ [optional] prefix path.
	MappedFile(const filename_char_t* const filename_, const filename_char_t* const dirname_=NULL) ;

	//! Create a viewport onto a portion of a file.
	MappedFile(const char* start_, const char* end_, size_t lineNo_)
		: m_basePtr(NULL)
		, m_endPtr(end_)
		, m_currentPtr(start_)
		, m_size(end_ - start_)
		, m_lineNo(lineNo_)
#if IO_MAP_TYPE == _IMT_WINDOWS
        , m_handle(INVALID_HANDLE_VALUE)
        , m_mapping(INVALID_HANDLE_VALUE)
#else
        , m_fd(-1)
#endif
		{
			m_filename[0] = 0 ;
		}

	virtual ~MappedFile() ;						// Destructor.

private:
	MappedFile(const MappedFile& rhs_) ;		//! @internal Do not use!

public:
	// Copying mapped files is only allowed for sub-views.
	MappedFile& operator = (const MappedFile& rhs_) ;

public:
	//! Determine if we have read all the data.
	bool atEof() const { return m_endPtr < m_currentPtr ; }

	//! Determine if the file is mapped to something.
#if IO_MAP_TYPE == _IMT_WINDOWS
	bool isOpen() const { return m_mapping != INVALID_HANDLE_VALUE ; }
#else
	bool isOpen() const { return m_fd >= 0 ; }
#endif

	//! Open a new file (closes any currently open file first).
	//! @param[in] filename_ the file/sub-path of the file to open.
	//! @param[in] dirname_ [optional] prefix path.
	//! @return true if the file was opened, false otherwise.
	bool mapFile(const filename_char_t* const filename_, const filename_char_t* const dirname_=NULL) ;
	//! Close and unmap the file.
	//! @return Always returns false.
	bool unmapFile() ;

	//! Look at the next character.
	char nextCharacter() const { return atEof() ? 0 : *m_currentPtr ; }

	//! Advance to the end of the current line.
	bool advanceToEndOfLine() ;

	//! Advance to the start of the next line.
	bool advanceToNextLine() ;

	//! Find where this paragraph ends.
	bool advanceToEndOfParagraph() ;

	//! Skip current whitespace.
	//! @return true if we found a non-whitespace character,
	//! false if we reached eof.
	bool advanceToNonWhitespace() ;

	//! Advance to the next whitespace.
	bool advancePastCurrentWord() ;

	//! Skip ahead to first non-comment content.
	bool advanceToNextContent()
	{
		do
		{
			if ( advanceToNonWhitespace() )
				return false ;
			if ( *m_currentPtr != ';' )
				return true ;
		}
		while ( advanceToNextLine() ) ;

		return false ;
	}

public:
	// Accesors.

	//! Get the object's file name.
	const filename_char_t* filename() const { return m_filename ; }
	//! Get a pointer to the first byte of the file.
	const void* basePtr() const { return m_basePtr ; }
	//! Get a pointer to the current byte in the file.
	const char* currentPtr() const { return m_currentPtr ; }
	//! Get a pre-calculated pointer to the end of the file (last byte + 1).
	const char* endPtr() const { return m_endPtr ; }
	//! Get the number of bytes in the file at the time it was opened.
	size_t sizeBytes() const { return m_size ; }
	//! Get the number of bytes remaining between the current position and the end pointer.
	size_t remainingBytes() const { return (m_endPtr - m_currentPtr) ; }
	//! For a text file, return the current line number.
	size_t lineNo() const { return m_lineNo ; }

protected:
	//! Name of the file.
	filename_char_t	m_filename[PATH_MAX + 1] ;
	//! Where the file contents actually starts.
	void*			m_basePtr ;
	//! For convenience, where the file would end.
	const char*		m_endPtr ;
	//! Seek pointer (the next character).
	const char*		m_currentPtr ;

	//! Size (in bytes) of the entire file.k
	size_t			m_size ;

	//! Current line number.
	size_t			m_lineNo ;

#if IO_MAP_TYPE == _IMT_WINDOWS
	// For windows:
	HANDLE			m_handle ;
	HANDLE			m_mapping ;
#else
	int				m_fd ;
#endif
} ;

}

