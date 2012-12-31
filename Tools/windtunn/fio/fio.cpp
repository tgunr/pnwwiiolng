#include "syscfg.h"
#include "fio.h"
#include "fiod.h"
#include "vars.h"

#if platOS == OS_WIN32
#include <direct.h> // _mkdir
#endif

#ifndef FIO_NO_ARCHIVE
#include "fioa.h"
#endif

#define MAX_FIO_STACK 64

static SINT32 fio_switch; 

/************************************************************************************************
	fioInit - sets up the user data directory on Windows
*************************************************************************************************/

void fioInit()
{
	*UserDataPath = 0;

#if platOS != OS_MACOS
	snprintf(UserDataPath, _MAX_PATH, "%s%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"), USERDATA_PREPATH);

	// need to make sure the data directory is create here

	if(!fiodAccess(UserDataPath))
		_mkdir(UserDataPath);
	
#endif
}

/************************************************************************************************
	fioOpen
*************************************************************************************************/

int fioOpen(const char *filename)
{
    int fd = -1;
   
	switch(fio_switch)
    {
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			fd = fiodOpen(filename);
			fiodSetPrepath(PREPATH);
		}
		break;
        case ISDIRECTORY:
       	default:
			fd = fiodOpen(filename);
       	break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
			fd = fioaOpen(filename);
       	break;
#endif
    }
    
    return fd;
}

/************************************************************************************************
	fioOpenWrite
*************************************************************************************************/

int fioOpenWrite(const char *filename)
{
	int fd = -1;
   
	switch(fio_switch)
	{
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			fd = fiodOpenWrite(filename);
			fiodSetPrepath(PREPATH);
		}
		break;
		case ISDIRECTORY:
			fd = fiodOpenWrite(filename);
		break;
#ifndef FIO_NO_ARCHIVE
		case ISARCHIVE:
		    fd = fioaOpenWrite(filename);
		break;
#endif
	}
    
    return fd;
}

/************************************************************************************************
	fioOpenAppend
*************************************************************************************************/
int fioOpenAppend(const char *filename)
{
	int fd = -1;
   
	switch(fio_switch)
	{
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			fd = fiodOpenAppend(filename);
			fiodSetPrepath(PREPATH);
		}
		break;
		case ISDIRECTORY:
			fd = fiodOpenAppend(filename);
		break;
#ifndef FIO_NO_ARCHIVE
		case ISARCHIVE:
		    fd = fioaOpenAppend(filename);
		break;
#endif
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
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            fioaClose(fd);
		break;
#endif
    }
}


/************************************************************************************************
	fioAccess
*************************************************************************************************/

LOGICAL fioAccess(const char *filename)
{
    switch(fio_switch)
    {
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			LOGICAL result = fiodAccess(filename);
			fiodSetPrepath(PREPATH);
			return result;
		}
		break;
        case ISDIRECTORY:
        default:    
            return(fiodAccess(filename));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaAccess(filename);
		break;
#endif
    }
}

/************************************************************************************************
	fioModified
*************************************************************************************************/

LOGICAL fioModified(SINT32 *modified, const char *filename)
{
    switch(fio_switch)
    {
		case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			LOGICAL result = fiodModified(modified,filename);
			fiodSetPrepath(PREPATH);
			return result;
		}
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
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaEOF(fd);
		break;
#endif			
    }
}

/************************************************************************************************
	fioFileSize
*************************************************************************************************/

SINT32 fioFileSize(const char *name)
{
	switch(fio_switch)
   	{
		case -1:
			return -1;
		break;
        case ISUSERDIRECTORY:
		{
			fiodSetPrepath(UserDataPath);
			SINT32 size = fiodFileSize(name);
			fiodSetPrepath(PREPATH);
			return size;
		}
		break;
        case ISDIRECTORY:
        default:    
            return(fiodFileSize(name));
		break;
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaFileSize(name);
		break;
#endif		
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

int fioSeek(int fd, int offset, int from)
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
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaSeek(fd, offset,from);
      	break;
#endif      	
    }
}

/************************************************************************************************
	fioRead
*************************************************************************************************/

int fioRead(int fd, void *dptr, int length)
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
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaRead(fd, dptr,length);
		break;
#endif		
    }
}

/************************************************************************************************
	fioWrite
*************************************************************************************************/

int fioWrite(int fd, const void *dptr, int length)
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
#ifndef FIO_NO_ARCHIVE
        case ISARCHIVE:
            return fioaWrite(fd, dptr,length);
		break;
#endif		
    }
}

/************************************************************************************************
	fioGetLine
*************************************************************************************************/

int fioGetLine(int fd, char *string, int length)
{
	int   i;

	if(fd < 0) return -1;

  for (i=0 ; i < length - 1 ; i++)
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
    
#if platOS == OS_MACOS
// mac newlines, ugh
   	if(string[i] == '\r'){	
    	char c = '\0';
	  	if(fioRead(fd, &c, 1) == 1 ){
	  		fioSeek(fd,-1,SEEK_CUR);
	  		if(c != '\n'){
	  			string[i] = '\0';
	  			return (i+1);
	  		}
	  	}
	}
#endif
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
    LITTLE_TO_NATIVE_ENDIAN_16(s);

	return s;
}

/************************************************************************************************
	fioGetl
*************************************************************************************************/

long fioGetl(int fd)
{
    long l = 0;
      
 	fioRead(fd, &l, 4);
    LITTLE_TO_NATIVE_ENDIAN_32(l);
    
    return l;
}

/************************************************************************************************
	fioGetf
*************************************************************************************************/

float fioGetf(int fd)
{
    float f = 0;
	
	fioRead(fd, &f, 4);
    LITTLE_TO_NATIVE_ENDIAN_32(f);
    
    return f;
}

/************************************************************************************************
	fioGetd
*************************************************************************************************/

double fioGetd(int fd)
{
 	double d = 0;
   
   	fioRead(fd, &d, 8);
   	LITTLE_TO_NATIVE_ENDIAN_64(d);
 
 	return d;
}

/************************************************************************************************
	fioReads
*************************************************************************************************/

int fioReads(int fd, SINT16 *sp, int numshorts)
{   
	int nb = fioRead(fd, sp, numshorts * sizeof(SINT16));

#if platEndian == bigEndian    
{
 	int i;
	for(i = 0; i < numshorts; i++, sp++)
	{
		ENDIAN_SWAP_16(*sp);
	}
}
#endif
   
    return nb;
}

/************************************************************************************************
	fioReadl
*************************************************************************************************/

int fioReadl(int fd, SINT32 *lp, int numlongs)
{	
	int nb = fioRead(fd, lp, numlongs * sizeof(SINT32));
	    
#if platEndian == bigEndian      
	{
    	int i;
    	for(i = 0; i < numlongs; i++, lp++)
    	{
    		ENDIAN_SWAP_32(*lp);
    	}
	}
#endif
    
    return nb;
}

/************************************************************************************************
	fioReadf
*************************************************************************************************/

int fioReadf(int fd, FLOAT32 *fp, int numfloats)
{	
	int nb = fioRead(fd, fp, numfloats * sizeof(FLOAT32));
		
#if platEndian == bigEndian  
    {
    	int i;
    	for(i = 0; i < numfloats; i++, fp++)
    	{
    		ENDIAN_SWAP_32(*fp);
    	}
    }
#endif

    return nb;
}

/************************************************************************************************
	fioReadd
*************************************************************************************************/

int fioReadd(int fd, FLOAT64 *dp, int numdoubles)
{	
	int nb = fioRead(fd, dp, numdoubles * sizeof(FLOAT64));
    
#if platEndian == bigEndian  
    {
		int i;
    	for(i = 0; i < numdoubles; i++, dp++)
    	{
    		ENDIAN_SWAP_32(*dp);
    	}
    }
 #endif
    
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
	NATIVE_TO_LITTLE_ENDIAN_16(s);
	fioWrite(fd, &s, 2);
}

/************************************************************************************************
	fioPutl
*************************************************************************************************/

void fioPutl(int fd, SINT32 l)
{   
	NATIVE_TO_LITTLE_ENDIAN_32(l);
	fioWrite(fd, &l, 4);
}

/************************************************************************************************
	fioPutf
*************************************************************************************************/

void fioPutf(int fd, FLOAT32 f)
{   
	NATIVE_TO_LITTLE_ENDIAN_32(f);
	fioWrite(fd, &f, 4);
}

/************************************************************************************************
	fioPutd
*************************************************************************************************/

void fioPutd(int fd, FLOAT64 d)
{   
	NATIVE_TO_LITTLE_ENDIAN_64(d);
	fioWrite(fd, &d, 8);
}

/************************************************************************************************
	fioWrites
*************************************************************************************************/

void fioWrites(int fd, const SINT16 *sp, int numshorts)
{
#if platEndian == bigEndian  
	{
		int i;
		for(i = 0; i < numshorts; sp++, i++)
		{
			fioPuts(fd, *sp);
		}
	}
#else
		fioWrite(fd, (void*)sp, numshorts * sizeof(SINT16));
#endif
}

/************************************************************************************************
	fioWritel
*************************************************************************************************/

void fioWritel(int fd, const SINT32 *lp, int numlongs)
{
#if platEndian == bigEndian  
	{
		int i;
		for(i = 0; i < numlongs; lp++, i++)
		{
			fioPutl(fd, *lp);
		}
	}
#else
		fioWrite(fd, (void*)lp, numlongs * sizeof(SINT32));
#endif
}

/************************************************************************************************
	fioWritef
*************************************************************************************************/

void fioWritef(int fd, const FLOAT32 *fp, int numfloats)
{
#if platEndian == bigEndian  
	{
		int i;
		for(i = 0; i < numfloats; fp++, i++)
		{
			fioPutf(fd, *fp);
		}
	}
#else
		fioWrite(fd, (void*)fp, numfloats * sizeof(FLOAT32));
#endif
}

/************************************************************************************************
	fioWrited
*************************************************************************************************/

void fioWrited(int fd, const FLOAT64 *dp, int numdoubles)
{
#if platEndian == bigEndian  
	{
		int i;
		for(i = 0; i < numdoubles; dp++, i++)
		{
			fioPutd(fd, *dp);
		}
	}
#else
		fioWrite(fd, (void*)dp, numdoubles * sizeof(FLOAT64));
#endif
}


#include "libxml/xmlIO.h"

static int xmlinputmatch(const char* filename) { return 1; }

void fioXmlRegisterInputCallbacks()
{
	xmlRegisterInputCallbacks(xmlinputmatch, (xmlInputOpenCallback)fioOpen, (xmlInputReadCallback)fioRead, (xmlInputCloseCallback)fioClose);
}