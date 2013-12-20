#pragma once

extern int fioaAddArchive(const char* archive);
extern void fioaKill();

extern int  fioaOpen(const char* filename);
extern int  fioaOpenWrite(const char* filename);
extern void fioaClose(int hfile);

extern size_t fioaRead(int fd, void* data, size_t len);
extern size_t fioaWrite(int fd, const void* data, size_t len);

extern size_t fioaSeek(int fd, size_t offset, int from);
extern int fioaEOF(int fd);

extern size_t fioaFileSize(const char* name);
extern int fioaAccess(const char* filename);

extern size_t fioaMap(int fd, const void*& data);
