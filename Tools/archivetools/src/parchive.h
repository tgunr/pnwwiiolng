#ifndef _PARC_H
#define _PARC_H

/************************************************************************************************
	parchive
*************************************************************************************************/

struct tagpaFILE;
typedef struct tagpaFILE paFILE;

paFILE* paAlloc();
void paFree(paFILE*);

int  paOpen(paFILE*, const char* archivename, const char* mode);
void paClose(paFILE*, int archiveIdx);

int paFetchMember (paFILE*, const char* membername, unsigned char** data, int* len);
int paAppendmember(paFILE*, const char* membername, const unsigned char* data, int len);
void paFreeMember(unsigned char* data);

#endif // _PARC_H