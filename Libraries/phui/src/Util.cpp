#include "phui-precompiled-header.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define open _open
#define close _close
#define write _write
#define read _read
#define lseek _lseek
#include <GL/gl.h>
#else
#include <unistd.h>
#include <OpenGL/gl.h>
#include "macgl.h"
#endif

#include "phui/Util.h"
//#include "ConvertUTF.h"
#include "wwiiUTF.h"

using namespace phui;

/************************************************************************************************
	phui
*************************************************************************************************/

static int openreadfd(const char* name) 
{ 
	int fd = open(name, O_RDONLY); 
	return fd;
}

static int openwritefd(const char* name) 
{ 
	int fd = open(name, O_WRONLY); 
	return fd;
}

static int openreadFILE(const char* name) 
{ 
	FILE* f = fopen(name, "rb"); 
	return (int)f;
}

static int openwriteFILE(const char* name) 
{ 
	FILE* f = fopen(name, "wb"); 
	return (int)f;
}

static size_t readfd(int fd, void* buf, size_t n) 
{ 
	return read(fd, buf, n); 
}

static size_t readFILE(int fd, void* buf, size_t n) 
{ 
	return fread(buf, 1, n, (FILE*)fd); 
}

static size_t writefd(int fd, const void* buf, size_t n) 
{ 
	return write(fd, buf, n); 
}

static size_t writeFILE(int fd, const void* buf, size_t n) 
{ 
	return fwrite(buf, 1, n, (FILE*)fd); 
}

static int closefd(int fd) 
{ 
	return close(fd); 
}

static int closeFILE(int fd) 
{ 
	return fclose((FILE*)fd); 
}

static size_t sizefn(const char* name)
{
	struct stat st;
	if(stat(name, &st) == 0)
		return st.st_size;
	return 0;
}

static size_t seekfd(int fd, size_t offset)
{
	return lseek(fd, offset, SEEK_CUR);
}

static size_t seekFILE(int fd, size_t offset)
{
	return fseek((FILE*)fd, offset, SEEK_CUR);
}

#include <map>
#include <string>

struct memio
{
	char* data;
	size_t size;
	size_t pos;
};

static std::map<std::string, memio> memIOmap;

void phui::memIOAddBuffer(const char* name, void* data, int size)
{
	memio io;
	io.data = (char*)data;
	io.size = size;
	io.pos = 0;
	
	memIOmap[name] = io;
}

void phui::memIORemoveBuffer(const char* name)
{
	std::map<std::string, memio>::iterator it = memIOmap.find(name);
	if(it != memIOmap.end())
	{
		memIOmap.erase(it);
	}
}

static int openreadmemIO(const char* name) 
{ 
	memio* io = 0;
	
	std::map<std::string, memio>::iterator it = memIOmap.find(name);
	if(it != memIOmap.end())
		io = new memio(it->second);
	
	return (int)io;
}

static int openwritememIO(const char* name) 
{ 
	return 0;
}

static size_t readmemIO(int fd, void* buf, size_t n)
{
	memio* io = (memio*)fd;
	if(io == 0) return -1;
	
	if(io->pos + n > io->size)
		n = io->size - io->pos;
	
	if(n > 0)
	{
		memcpy(buf, io->data+io->pos, n);
		io->pos += n;
	}
	
	return n;
}

static size_t writememIO(int fd, const void* buf, size_t n) 
{ 
	return (size_t)-1;
}

static int closememIO(int fd) 
{ 
	memio* io = (memio*)fd;
	if(io == 0) return -1;
	
	delete io;

	return 0;
}

static size_t sizememIO(const char* name)
{
	std::map<std::string, memio>::iterator it = memIOmap.find(name);
	if(it != memIOmap.end())
		return it->second.size;
				
	return (size_t)-1;
}

static size_t seekmemIO(int fd, size_t offset)
{
	memio* io = (memio*)fd;
	if(io == 0) return -1;

	if(offset > io->size)
		offset = io->size;
		
	io->pos = offset;

	return io->pos;	
}

const IOCallbacks phui::fdIOCallbacks = { openreadfd, openwritefd, readfd, writefd, closefd, sizefn, seekfd };
const IOCallbacks phui::FILEIOCallbacks = { openreadFILE, openwriteFILE, readFILE, writeFILE, closeFILE, sizefn, seekFILE };
const IOCallbacks phui::memIOCallbacks = { openreadmemIO, openwritememIO, readmemIO, writememIO, closememIO, sizememIO, seekmemIO };

wchar_t* phui::str_to_wstr(wchar_t* wstr, const char* str, int maxlen)
{
	wchar_t* w = wstr;
	for(int i = 0; i < maxlen && *str ; i++, w++, str++)
		*w = (unsigned char)*str;
	*w = 0;

	return wstr;
}

char* phui::wstr_to_str(char* str, const wchar_t* wstr, int maxlen)
{   
	char* s = str;
	for(int i = 0; i < maxlen && *wstr; i++, wstr++, s++)
		*s = (char)*wstr;
	*s = '\0';

	return str;
}

void phui::box_verts_with_bounds(phui::BoxVerts& box, float left, float bottom, float right, float top)
{
	box.verts[0] = left;  box.verts[1] = bottom;
	box.verts[2] = right; box.verts[3] = bottom;
	box.verts[4] = right; box.verts[5] = top;
	box.verts[6] = left;  box.verts[7] = top;
}

void phui::box_verts_with_pos(phui::BoxVerts& box, float left, float bottom, float width, float height)
{
	box.verts[0] = left;			box.verts[1] = bottom;
	box.verts[2] = left+width;		box.verts[3] = bottom;
	box.verts[4] = left+width;		box.verts[5] = bottom+height;
	box.verts[6] = left;			box.verts[7] = bottom+height;
}

void phui::draw_gl_quad(const float* coords, const float* uv)
{
	glBegin(GL_QUADS);
		if(uv) glTexCoord2f(uv[0],uv[1]);
        glVertex2f(coords[0],coords[1]);

       if(uv) glTexCoord2f(uv[2],uv[3]);
        glVertex2f(coords[2],coords[3]);
		
        if(uv) glTexCoord2f(uv[4],uv[5]);
		glVertex2f(coords[4],coords[5]);
        
        if(uv) glTexCoord2f(uv[6],uv[7]);
        glVertex2f(coords[6],coords[7]);
    glEnd();
}

/************************************************************************************************
	utf8_to_utf16
*************************************************************************************************/
/*
const wchar_t* phui::utf8_to_utf16(const unsigned char* val)
{
	const UTF8* valptr = (const UTF8*)val;

	static wchar_t wval[8][8192] ;
	static int instance = 0 ;

	// make sure we're within the bounds of the array
	instance = instance & 7 ;

	// grab the current instance
	wchar_t *wvalptr = wval[instance];

	// thank you unicode.org
	ConversionResult result = ConvertUTF8toUTF16(&valptr, (UTF8*)val+strlen((char*)val), &wvalptr, wval[instance]+8191, lenientConversion);

	// terminate it
	*wvalptr = 0;
	
	if(result != conversionOK)
		str_to_wstr(wval[instance], (char*)val, 8191);
	
	// return the result and increment the instance
	return wval[instance++];
}
*/

#define CONVERT_SIZE PHUI_MAX_BUFFER-1

const wchar_t* phui::utf8_to_utf16(const unsigned char* val)
{
	static wchar_t wval[8][CONVERT_SIZE+1] ;
	static int instance = 0 ;

	// make sure we're within the bounds of the array
	instance = instance & 7 ;

	// grab the current instance
	wchar_t* wptr = wval[instance];

	*wptr = 0;

	WWII::ConvertChars((const char*)val, wptr, sizeof(wval[instance]));

	return (const wchar_t*)wval[instance++];
}

/************************************************************************************************
	utf16_to_utf8
*************************************************************************************************/


const unsigned char* phui::utf16_to_utf8(const wchar_t* wptr)
{
	static unsigned char val[8][CONVERT_SIZE+1];
	static int instance = 0 ;
	
	// make sure we're within the bounds of the array
	instance = instance & 7 ;
	
	// grab the current instance
	unsigned char* ptr = val[instance];

	*ptr = 0;
	
	WWII::ConvertChars(wptr, (char*)ptr, sizeof(val[instance]));
	
	return (const unsigned char*)val[instance++];
}

/************************************************************************************************
	html_encode
*************************************************************************************************/

#define WSTRLEN(x) (sizeof(x)/sizeof(wchar_t)-1)
#define WSTRBTYES(x) ((sizeof(x) - sizeof(wchar_t)))

#define ENTITY_COPY(x) \
{ \
	memcpy(dst, x, WSTRBTYES(x)); \
	dst += WSTRLEN(x); \
	i += WSTRLEN(x); \
}

const wchar_t* phui::html_encode(const wchar_t* text)
{
	static wchar_t buffer[PHUI_MAX_BUFFER];
		
	wchar_t* dst = buffer;
	const wchar_t* src = text;
	int i = 0;

	for(; i < PHUI_MAX_BUFFER-12 && *src; src++)
	{
		if(*src == '\'')
		{
			ENTITY_COPY(L"&#39;");
		}else if(*src == '\"')
		{
			ENTITY_COPY(L"&#34;");	
		}else if(*src == '<')
		{
			ENTITY_COPY(L"&lt;");
		}else if(*src == '>')
		{
			ENTITY_COPY(L"&gt;");
		}else
		{
			*dst++ = *src;
			i++; 
		}
	}

	*dst = '\0';
	
	return buffer;
}
