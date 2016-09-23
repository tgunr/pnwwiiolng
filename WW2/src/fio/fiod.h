#pragma once
// The defines for the Archive portion of fiod

namespace IO { class MappedFile; }

extern const char* fiodGetPrepath(void);
extern void fiodSetPrepath(char* newpath);

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
    #define fiocfgAccess fiodAccess
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
