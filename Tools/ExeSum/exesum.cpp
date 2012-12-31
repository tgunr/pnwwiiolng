// This is the main project file for VC++ application project 
// generated using an Application Wizard.

//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#include <windows.h>

typedef unsigned long long int ULONG64 ;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <exception>
#include <malloc.h>
#include <memory.h>
#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <windows.h>

using namespace std;

#include <exception>
#include <vector>

#define	READ_BUF_SIZE	8192		// Read 4k at a time; even a celeron can cope with that
#define CHECKSUM_BIT_LEN 64

ULONG64
check64(const char* const filename)
{
	int		fd = -1 ;				// Level 2 file descriptor
	char*	memory = NULL ;			// Memory store for the data
	int		bytes = 0 ;				// Number of bytes successfully read
	int		i ;
	ULONG64	checksum = 0 ;

	try
	{
		memory = (char*)malloc(READ_BUF_SIZE) ;
		if ( memory == NULL )
			throw "No buffer memory" ;

		// Get a descriptor for reading the file in binary mode
		fd = open(filename, _O_RDONLY | _O_BINARY) ;
		if ( fd == -1 )
			throw "File missing" ;

		do
		{
			bytes = _read(fd, memory, READ_BUF_SIZE) ;
			if ( bytes < 0 )
				throw "Read failure" ;

			// Its possible we might read 0 bytes
			if ( bytes == 0 )
				break ;

			// Rotate 5 bits right and add the new value, this gives some weight to positional value
			// of each value.
			for ( i = 0 ; i < bytes ; i++ )
			{
				checksum = ((checksum & 0x1f) << (CHECKSUM_BIT_LEN-5)) | (checksum >> 5) ;
				checksum += memory[i] ;
			}
		}
		while ( bytes == READ_BUF_SIZE ) ;
	}
	catch (const char* e)
	{
		fprintf(stderr, "%s: %s\n", filename, e) ;
	}

	if ( fd != -1 )
		close(fd) ;

	if ( memory != NULL )
		free(memory) ;

	return checksum ;
}

int
main(int argc, const char* const argv[])
{
	FILE* const fp = fopen("exesum.txt", "wb") ;
	if ( fp == NULL )
	{
		MessageBox(NULL, "Unable to open exesum.txt!", "FAILED", MB_ICONERROR|MB_OK) ;
		exit(0) ;
	}

	std::vector<const char*> files ;
	files.push_back("ww2.exe") ;
	for ( auto it = files.begin() ; it != files.end() ; ++it )
	{
		const char* const filename = *it ;
		ULONG64 checksum = check64(filename) ;
		printf("%s %I64u\r\n", filename, checksum) ;
		fprintf(fp, "%s %I64u\r\n", filename, checksum) ;
	}

	fclose(fp) ;

	return 0 ;
}
