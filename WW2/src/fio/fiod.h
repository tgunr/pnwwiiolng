#pragma once
// The defines for the Archive portion of fiod

#include <wwii/wwiiUTF.h>

namespace IO { class MappedFile; }

extern const fullpath_t* fiodGetPrepath();
extern void fiodSetUserpath(bool userpath);

struct UsingUserdir {
	UsingUserdir(bool userdir = true) { fiodSetUserpath(userdir); }
	~UsingUserdir() { fiodSetUserpath(false); }
	operator bool(void) const { return false; }
};

extern bool fiodLoad(const char* filename, char*& outPtr, unsigned int* outSize=nullptr);
extern IO::MappedFile* fiodGetMappedFile(int fd);
extern int fiodOpen(const char* filename, bool userdir = false, int prereadSize = -1);
extern int fiodOpenWrite(const char* filename, bool userdir = false);
extern void fiodClose(int hfile);

extern size_t fiodRead(int hfile, void* dptr, size_t length);
extern size_t fiodWrite(int hfile, const void* dptr, size_t length);
extern size_t fiodSeek(int hfile, size_t offset, int from);
extern size_t fiodMap(int hfile, const void*& data);
extern int fiodEOF(int hfile);

extern LOGICAL fiodAccess(const char* filename, bool userdir = false);
extern LOGICAL fiodModified(SINT32* modified, const char* filename, bool userdir = false);
extern size_t fiodFileSize(const char* name, bool userdir = false);

extern void fiodmkdir(const char* path);

#if platBuild == BUILD_CLIENT

extern void fiodGetDirFileList(SCHAR* dirToSearch, SCHAR*** fileList, SINT32* numFiles);
extern void fiocfgMkDir(const char* path, bool userdir = false);

#if platOS == OS_WIN32
    #define fiocfgOpen fiodOpen
    #define fiocfgOpenWrite fiodOpenWrite
    LOGICAL fiocfgAccess(const char* filename, bool userdir = false);
    #define fiocfgModified fiodModified
    #define fiocfgFileSize fiodFileSize
#else
size_t fiocfgRead(int fd, void* dptr, size_t length);
    size_t fiocfgOpen(const char* filename, bool userdir = false);
    size_t fiocfgOpenWrite(const char* filename, bool userdir = false);
    LOGICAL fiocfgAccess(const char* filename, bool userdir = false);
    LOGICAL fiocfgModified(SINT32* modified, const char* filename, bool userdir = false);
    SINT32 fiocfgFileSize(const char* name, bool userdir = false);
#endif

#else // platBuild == BUILD_CLIENT

extern size_t fiodGetString(int hfile, char* string, size_t length);

template<typename _Type>
static int fiodGet(int fd, _Type& into)
{
    return fiodRead(fd, &into, sizeof(int));
}

#endif

#if RECORD_FILES
    extern void dumpFileRefs();
#endif


//==============

#if platBuild == BUILD_CLIENT
	char* os_file_path(char* path);
	wchar_t* os_file_path(wchar_t* path);
#else
	#define os_file_path(x) (x)
#endif

//--------------

template <size_t N>
UINT32 GetFullPath(fullpath_t(&path)[N], const char* name) {
	fullpath_t wname[MAX_PATH];
	WWII::ConvertChars(name, wname, array_size(wname));
	wname[array_size(wname)-1] = L'\0';

	return pathprintf(path, fiodGetPrepath(), os_file_path(wname));
}
