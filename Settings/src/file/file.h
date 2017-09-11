#ifndef _FILE_H_
#define _FILE_H_

#include <string>

std::string fileUrlDecode(const char* src);
int fileRead(int handle, char *buffer, int length);
int fileClose(int handle);
int fileOpen(const char *file);
int fileOpenWrite(const char *file);
int fileWrite(int handle, const void *data, size_t size);
int fileSeek(int handle, int offset, int from);

void fileInit();

bool fileAccess(const char *file);

#endif