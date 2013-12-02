#include <syscfg.h> // Path is project specific.

#include "fio/fioa.h"
#include "fio/fio.h"
#include "pa/parchive.h"

#include "mem/mem.h"
#include "systhread.h"

/************************************************************************************************
	fioa
*************************************************************************************************/

#define FIOA_MAX_FILDES 128

struct fioaFD
{
	int marker;
	int datalen;
	unsigned char* data;
};

struct fioaObj
{
	fioaFD file[FIOA_MAX_FILDES];
	paFILE* pf;
};

static fioaObj fioa;
static systhread_mutex fioaMutex;

/************************************************************************************************
	fioaAddArchive
*************************************************************************************************/

int fioaAddArchive(const char* archive)
{
	if(fioaMutex == NULL)
		fioaMutex = systhreadMutexCreate();

	if(fioa.pf == NULL)
		fioa.pf = paAlloc();

	if(fioa.pf == NULL) return -1;

	return paOpen(fioa.pf, archive, "rb");
}

/************************************************************************************************
	fioaKill
*************************************************************************************************/

void fioaKill()
{
	int i;

	if(fioaMutex)
		systhreadMutexKill(fioaMutex);

	for(i = 0; i < FIOA_MAX_FILDES; i++)
	{
		if(fioa.file[i].data)
		{
			Write_Debug("File still opened, size: %d", fioa.file[i].datalen);
			fioaClose(i);
		}
	}

	if(fioa.pf)
		paFree(fioa.pf);

	memset(&fioa, 0x0, sizeof(fioa));
}

/************************************************************************************************
	fioaOpen
*************************************************************************************************/

int fioaOpen(const char* filename)
{
	int fd;

	if(fioa.pf == NULL) return -1;

// use mutex here since we call it from thread completion routine
	systhreadMutexLock(fioaMutex);

	for(fd = 0; fd < FIOA_MAX_FILDES; fd++)
	{
		if(fioa.file[fd].data == NULL)
		{
			if(paFetchMember(fioa.pf, filename, &fioa.file[fd].data, &fioa.file[fd].datalen) == 0)
				fd += FIOA_FD_START;
			else
				fd = -1;

			break;
		}
	}

	systhreadMutexUnlock(fioaMutex);

	return fd;
}

/************************************************************************************************
	fioaOpenWrite
*************************************************************************************************/

int fioaOpenWrite(const char* filename)
{
	return -1; // read only
}

/************************************************************************************************
	fioaClose
*************************************************************************************************/

void fioaClose(int fd)
{
	fd -= FIOA_FD_START;

	if(fd > -1 && fd < FIOA_MAX_FILDES)
	{
		if(fioa.file[fd].data)
			paFreeMember(fioa.file[fd].data);

		memset(&fioa.file[fd], 0x0, sizeof(fioaFD));
	}
}

/************************************************************************************************
	fioaRead
*************************************************************************************************/

size_t fioaRead(int fd, void* data, size_t len)
{
	size_t nb = -1; // -1 to an unsigned int(???)
	fd -= FIOA_FD_START;

	if(fd > -1 && fd < FIOA_MAX_FILDES)
	{
		int offset = fioa.file[fd].marker;

		if(offset < fioa.file[fd].datalen)
		{
			nb = len;
			if(offset + (int)nb > fioa.file[fd].datalen)
			{
				nb = fioa.file[fd].datalen - offset;
			}

			memcpy(data, fioa.file[fd].data + offset, nb);

			fioa.file[fd].marker += nb;
		}
	}

	return nb;
}

/************************************************************************************************
	fioaRead
*************************************************************************************************/
// read only
size_t fioaWrite(int fd, const void* data, size_t len)
{
	fd -= FIOA_FD_START;
	return -1;
}

/************************************************************************************************
	fioaSeek
*************************************************************************************************/

size_t fioaSeek(int fd, size_t offset, int from)
{
	fd -= FIOA_FD_START;
	if(fd > -1 && fd < FIOA_MAX_FILDES)
	{
		switch(from)
		{
			case SEEK_SET:
				fioa.file[fd].marker = offset;
			break;

			case SEEK_CUR:
				fioa.file[fd].marker += offset;
			break;

			case SEEK_END:
				fioa.file[fd].marker = fioa.file[fd].datalen - offset;
			break;
		}

		if(fioa.file[fd].marker < 0)
			fioa.file[fd].marker = 0;

		if(fioa.file[fd].marker > fioa.file[fd].datalen)
			fioa.file[fd].marker = fioa.file[fd].datalen;

		return fioa.file[fd].marker;
	}

	return -1;
}

/************************************************************************************************
	fioaEOF
*************************************************************************************************/

int fioaEOF(int fd)
{
	fd -= FIOA_FD_START;
	if(fd > -1 && fd < FIOA_MAX_FILDES)
	{
		return fioa.file[fd].marker >= fioa.file[fd].datalen;
	}

	return 1;
}

/************************************************************************************************
	fioaFileSize
*************************************************************************************************/

size_t fioaFileSize(const char* filename)
{
	int len = -1;

	paFetchMember(fioa.pf, filename, NULL, &len);

	return (size_t)len;
}

/************************************************************************************************
	fioaAccess
*************************************************************************************************/

int fioaAccess(const char* filename)
{
	return paFetchMember(fioa.pf, filename, NULL, NULL) == 0;
}
