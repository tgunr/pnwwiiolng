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

# if defined(UNICODE)
#  define IO_MAP_WCHAR
#  include <wchar.h>
#  define filename_char_t wchar_t
#  define PATH_SEP_CHAR L'\\' 
# else
# pragma error
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
		{}

	//! Create an in-memory view of a file from disk,
	//! while trying to avoid loading it from disk.
	//! @param[in] filename_ the file/sub-path of the file to open.
	//! @param[in] dirname_ [optional] prefix path.
	MappedFile(const filename_char_t* const filename_, const filename_char_t* const dirname_=NULL);

	//! Create a viewport onto a portion of a file.
	MappedFile(const char* start_, const char* end_)
		: m_basePtr(nullptr)
		, m_startPtr(start_)
		, m_endPtr(end_)
		, m_currentPtr(start_)
		, m_size(static_cast<size_t>(end_ - start_))
#if IO_MAP_TYPE == _IMT_WINDOWS
		, m_handle(INVALID_HANDLE_VALUE)
		, m_mapping(INVALID_HANDLE_VALUE)
#else
		, m_fd(-1)
#endif
		{}

	virtual ~MappedFile();					// Destructor.

private:
	MappedFile(const MappedFile& rhs_);		//! @internal Do not use!

public:
	// Copying mapped files is only allowed for sub-views.
	MappedFile& operator = (const MappedFile& rhs_);

public:
	//! Determine if we have read all the data.
	bool atEof() const { return m_endPtr < m_currentPtr; }

	//! Determine if the file is mapped to something.
#if IO_MAP_TYPE == _IMT_WINDOWS
	bool isOpen() const { return m_mapping != INVALID_HANDLE_VALUE; }
#else
	bool isOpen() const { return m_fd >= 0; }
#endif

	//! Open a new file (closes any currently open file first).
	//! @param[in] filename_ the file/sub-path of the file to open.
	//! @param[in] dirname_ [optional] prefix path.
	//! @return true if the file was opened, false otherwise.
	bool mapFile(const filename_char_t* const filename_, const filename_char_t* const dirname_=NULL);
	//! Close and unmap the file.
	//! @return Always returns false.
	bool unmapFile();

	//! Adjust the cursor like seek().
	size_t seekSet(size_t offset)
	{
		if ( offset > sizeBytes() )
			return (size_t)-1;
		m_currentPtr = static_cast<const char*>(m_startPtr) + offset;
		return offset;
	}
	size_t seekCur(size_t offset)
	{
		if ( offset > remainingBytes() )
			return (size_t)-1;
		m_currentPtr += offset;
		return static_cast<size_t>(currentPtr() - m_startPtr);
	}
	size_t seekEnd(size_t offset)
	{
		if ( offset > sizeBytes() )
			return (size_t)-1;
		m_currentPtr = m_endPtr - offset;
		return sizeBytes() - offset;
	}

	//! Advance to the character after the next \n or the end of file.
	void skipline()
	{
		while ( !atEof() && *(m_currentPtr++) != '\n' )
			;
	}

	//! Copy bytes from the current position to a buffer.
	//! @note Try to avoid read() unless you absolutely need the data copied.
	size_t read(void* into, size_t bytes)
	{
		const size_t bytesToRead = bytes < remainingBytes() ? bytes : remainingBytes();
		memcpy(into, currentPtr(), bytesToRead);
		m_currentPtr += bytesToRead;
		return bytesToRead;
	}

	//! Copy characters from the file until we reach the next \n.
	//! @note More expensive than just taking a pointer and finding the \n yourself.
	size_t readline(char* into, size_t maxReadLength);

	//! Treats the data at the current position as being of type T and advances past it.
	//! @caution Only suitable for standard-layout objects.
public:
	template<class T>
	typename std::enable_if<std::is_standard_layout<T>::value>::type* read()
	{
		if ( remainingBytes() < sizeof(T) )
			return NULL;
		T* ptr = currentPtr();
		m_currentPtr += sizeof(T);
		return ptr;
	}

public:
	// Accesors.

	//! Get a pointer to the first byte of the file unless we are a view.
	const void* basePtr() const { return m_basePtr; }
	//! Pointer to the first byte of our data.
	const char* startPtr() const { return m_startPtr; }
	//! Get a pointer to the current byte in the file.
	const char* currentPtr() const { return m_currentPtr; }
	//! Get a pre-calculated pointer to the end of the file (last byte + 1).
	const char* endPtr() const { return m_endPtr; }
	//! Get the number of bytes in the file at the time it was opened.
	size_t sizeBytes() const { return m_size; }
	//! Get the number of bytes remaining between the current position and the end pointer.
	size_t remainingBytes() const { return (m_endPtr - m_currentPtr); }

protected:
	//! First byte of data we actually own.
	void*			m_basePtr;
	//! First byte of the data we are viewing.
	const char*		m_startPtr;
	//! For convenience, where the file would end.
	const char*		m_endPtr;
public:
	//! Seek pointer (the next character).
	const char*		m_currentPtr;

	//! Size (in bytes) of the entire file.k
	size_t			m_size;


#if IO_MAP_TYPE == _IMT_WINDOWS
	// For windows:
	HANDLE			m_handle;
	HANDLE			m_mapping;
#else
	int				m_fd;
#endif
};

}

