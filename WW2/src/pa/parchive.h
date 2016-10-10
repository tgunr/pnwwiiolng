#pragma once

/************************************************************************************************
	parchive
*************************************************************************************************/

struct paFILE;

paFILE* paAlloc();
void paFree(paFILE*);

int  paOpen(paFILE*, const char* archivename, const char* mode);
void paClose(paFILE*, int archiveIdx);

int paFetchMember (paFILE*, const char* membername, unsigned char** data, size_t* len);
int paAppendmember(paFILE*, const char* membername, const unsigned char* data, int len);
void paFreeMember(unsigned char* data);

