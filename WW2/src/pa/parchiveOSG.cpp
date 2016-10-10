#include <syscfg.h> // Path is project specific.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <map>
#include <vector>
#include <string>

#include "pa/parchive.h"
#include "pa/rijndael-api-fst.h"
#include "pa/md5.h"

/************************************************************************************************
	parchive
*************************************************************************************************/

#define PA_MAGIC 'crsa'
#define PA_CRYPT_BIT_SIZE 256 // Options are 128, 192, 256 (mil-spec)
#define PA_MAX_NAME 127
#define PA_ENCRYPTED_MEMBER 0x00000001
#define PA_TOC_SIZE 8192

#ifdef __ppc__
#define PA_BIG_ENDIAN 1
#endif

#ifndef PA_BIG_ENDIAN
#define PA_BIG_ENDIAN 0
#endif

static void endianswap32(unsigned char* c)
{
	c[0] ^= c[3]; c[3] ^= c[0]; c[0] ^= c[3];
	c[1] ^= c[2]; c[2] ^= c[1]; c[1] ^= c[2];
}

struct parcHead
{
	int magic;
	int version;
	unsigned char md5[16];
};

struct paMemberHead
{
	char name[PA_MAX_NAME+1];
	unsigned int filelen;
	unsigned int datalen;
	unsigned int crc;
	unsigned int flags;
	unsigned int seed;
	unsigned char md5[16];
};

struct paTOC
{
	FILE* f;
	std::string path;
	int offset;
	unsigned int filelen;
	unsigned int datalen;
	unsigned int crc;
	unsigned int flags;
	unsigned int seed;
};

struct paFILE
{
	~paFILE()
	{
		std::vector<FILE*>::iterator it = arcfiles.begin();
		for(;it != arcfiles.end(); ++it)
			fclose(*it);
	}
	int open(const char* archivename, const char* mode);
	int fetch(const char*, unsigned char**, int*, bool);
	int fetch(paTOC const&, unsigned char**, int*);

	std::vector<FILE*> arcfiles;
	std::multimap<std::string, paTOC> tocMap;
};

extern "C" int ROUNDS;
int ROUNDS;

static keyInstance ky;
static cipherInstance ci;
static int cipherInitialized = 0;

static void fix_path_name(char* dst, const char* src);
static int rijndaelcrypt(unsigned char* block, int size, int direction, int salt);
static void setrijndaelkey(unsigned int* k,int salt, int direction);

static paFILE* gPArchive; // total hack for pastream
paFILE* paArchive() { return gPArchive; }

paFILE* paAlloc()
{
	gPArchive = new paFILE;
	return gPArchive;
}
void paFree(paFILE* pf)
{
	if(pf == gPArchive)
		gPArchive = 0;
	delete pf;
}

int	paOpen(paFILE* pf, const char* archivename, const char* mode)
{
	return pf->open(archivename, mode);
}

int paFILE::open(const char* archivename, const char* mode)
{
	FILE* f = fopen(archivename, mode);
	if(f)
	{
		arcfiles.push_back(f);

		fseek(f, sizeof(parcHead), SEEK_SET);
		paMemberHead memberhead;

		for(;;)
		{
			if(fread(&memberhead, sizeof(memberhead),1,f) != 1) break;

			rijndaelcrypt((unsigned char*)&memberhead, sizeof(memberhead) - 20, DIR_DECRYPT, 666);

#if PA_BIG_ENDIAN
			endianswap32((unsigned char*)&memberhead.filelen);
			endianswap32((unsigned char*)&memberhead.datalen);
			endianswap32((unsigned char*)&memberhead.crc);
			endianswap32((unsigned char*)&memberhead.flags);
			endianswap32((unsigned char*)&memberhead.seed);
#endif
			paTOC toc;
			toc.path =  memberhead.name;
			toc.f = f;
			toc.offset = ftell(f);
			toc.datalen = memberhead.datalen;
			toc.filelen = memberhead.filelen;
			toc.crc = memberhead.crc;
			toc.flags = memberhead.flags;
			toc.seed = memberhead.seed;

			size_t n = toc.path.find_last_of('/');
			std::string key = n != std::string::npos ? toc.path.substr(n+1) : toc.path;

			tocMap.insert(std::make_pair(key,toc));

			fseek(f, memberhead.filelen, SEEK_CUR);
		}

		return 0;
	}
	return -1;
}

int paFILE::fetch(paTOC const& toc, unsigned char** data, int* len)
{
	if(len)
		*len = toc.datalen;

	if(data)
	{
		if(!toc.f) return -1;
		fseek(toc.f, toc.offset - 16, SEEK_SET);

		unsigned char srcMD5[16];
		fread(srcMD5, 1, 16, toc.f);

		*data = (unsigned char*)calloc(toc.filelen, 1);
		if(*data == NULL) return -1;

		fread(*data, 1, toc.filelen, toc.f);

		if(toc.flags & PA_ENCRYPTED_MEMBER)
			rijndaelcrypt(*data, toc.filelen, DIR_DECRYPT, toc.datalen);
	}
	return 0;
}

int paFILE::fetch(const char* membername, unsigned char** data, int* len, bool matchpath)
{
	char name[PA_MAX_NAME+1];
	fix_path_name(name, membername);

	std::string path = name;

	size_t n = path.find_last_of('/');
	std::string key = n != std::string::npos ? path.substr(n+1) : path;

	std::multimap<std::string, paTOC>::iterator it = tocMap.find(key);
	if(it == tocMap.end()) return -1;

	if(matchpath)
	{
		for(; it != tocMap.end() && it->first == key; ++it)
		{
			if(it->second.path == path)
			{
				return fetch(it->second, data, len);
			}
		}
	}else
	{
		return fetch(it->second, data, len);
	}

	return 0;
}

int paFetchMemberByName(paFILE* pf, const char* membername, unsigned char** data, int* len)
{
	return pf->fetch(membername, data, len, false);
}

int paFetchMember(paFILE* pf, const char* membername, unsigned char** data, int* len)
{
	return pf->fetch(membername, data, len, true);
}

void paClose(paFILE* pf, int archiveIdx)
{
}

int paAppendmember(paFILE* pf, const char* membername, const unsigned char* memberdata, int len)
{
	return -1;
}

void paFreeMember(unsigned char* data)
{
	if(data)
		free(data);
}

static void fix_path_name(char* dst, const char* src)
{
	char* dp = dst;
	const char* sp = src;
	char ch;

	// remove initial dot and slashes
	for(; *sp; ++sp)
	{
		ch = *sp;
		if(ch != '\\' && ch != ':' &&
			ch != '/' && ch != '.') break;
	}

	for(; *sp; ++sp)
	{
		ch = *sp;
		// convert slashes
		if((ch == '\\') || (ch == ':')) ch = '/';
		// convert to lower case
		ch = tolower(ch);
    	// strip double slashes
        if(sp > src)
        {
            if((ch == '/') &&  (*(sp-1) == '/'))
            {
                // nuking slash by doing nothing
                ;
            }
            else
            {
                *dp++ = ch;
            }
        }
        else
        {
            *dp++ = ch;
        }
	}
    *dp = '\0';
}

static int rijndaelcrypt(unsigned char* block, int size, int direction, int salt)
{
	unsigned char* out_block;

	setrijndaelkey(NULL, salt, direction);

	out_block = (unsigned char*)calloc(size, 1);

	// WE MUST HAVE 16 byte blocks , thats 128 bits
	if(direction == DIR_ENCRYPT)
		blockEncrypt(&ci, &ky, block, size * 8, out_block);
	else
		blockDecrypt(&ci, &ky, block, size * 8, out_block);

	memcpy(block, out_block,size);

	free(out_block);

	return(0);
}

static void setrijndaelkey(unsigned int* k,int salt, int direction)
{
// We need to make these memory locations bubble around for
// the key crackers, Make sure they have to track 1024 bits at least

unsigned int thekey[32];
unsigned int iv[32];
unsigned int temp; // temporary holding spot for key-part
int i;

#if PA_BIG_ENDIAN
unsigned char swp[4];
unsigned char* p;
#endif
		iv[0] = 0x1ef98e18;
		iv[1] = 0xd5765acd;
		iv[2] = 0x79a7d470;
		iv[3] = 0x1f5e4f74;
		iv[4] = 0x5288822b;
		iv[5] = 0x10f3dab4;
		iv[6] = 0x1e8891e3;
		iv[7] = 0x47f4b273;
        iv[8] = 0xea9a477a;
        iv[9] = 0x8ac3d3e8;
        iv[10] = 0x1ec0336a;
        iv[11] = 0xe8775f64;
        iv[12] = 0xbf7b1067;
        iv[13] = 0xc43d3470;
        iv[14] = 0x4738ac3b;
        iv[15] = 0x49a30c2b;
        iv[16] = 0xf4b930f6;
        iv[17] = 0x228358df;
        iv[18] = 0xe98b3c0b;
        iv[19] = 0x06ceceed;
        iv[20] = 0x2079c6d7;
        iv[21] = 0xfaa3e98b;
        iv[22] = 0x2256ff45;
        iv[23] = 0xbd0f2079;
        iv[24] = 0x49a30c2b;
        iv[25] = 0xc5d2ec98;
        iv[26] = 0x85d3b636;
        iv[27] = 0xd6a6b07c;
        iv[28] = 0x4e354fdf;
        iv[29] = 0x06c32079;
        iv[30] = 0xb46344cf;
        iv[31] = 0x2592e742;

	if(!cipherInitialized)
	{
#if PA_BIG_ENDIAN
		for(i=0; i < 32; i++)
		{
		memcpy(swp,&(iv[i]),4);
		p = (unsigned char*)&(iv[i]);
		*p = swp[3];
		*(p+1) = swp[2];
		*(p+2) = swp[1];
		*(p+3) = swp[0];
		}
#endif
	 	cipherInit(&ci, MODE_ECB, NULL);
		cipherInitialized = 1;
	}

	if(k == NULL) // for unknown keys
	{
		thekey[0] = 0x647ebd31;
		thekey[1] = 0x35c0949c;
		thekey[2] = 0xa95ce60a;
		thekey[3] = 0x5082858a;
		thekey[4] = 0x41c1089d;
		thekey[5] = 0x5b4bc5d9;
		thekey[6] = 0xcbf44e60;
		thekey[7] = 0x008bca66;
	}
	else
	{
		memcpy(thekey,k,32);
	}

    for(i=8; i < 32; i++)
        thekey[i] ^= iv[i];

	// add a little salt to the key
    // and make 1024 bits bubble a bit
    // for the memory peekers
		thekey[0] ^= salt;
		thekey[3] += salt;
		thekey[7] ^= salt;
        thekey[9] ^= salt;
        thekey[13] ^= salt;
        thekey[14] ^= salt;
        thekey[17] ^= salt;
        thekey[20] ^= salt;
        thekey[22] ^= salt;
        thekey[21] ^= salt;
        thekey[23] ^= salt;
        thekey[25] ^= salt;
        thekey[27] ^= salt;
        thekey[28] ^= salt;
        thekey[30] ^= salt;
        thekey[31] ^= salt;

	// now scramble them
		temp  = thekey[0];
		thekey[0] = thekey[7];
		temp  = thekey[7];
		thekey[7] = thekey[4];
		temp  = thekey[4];
		thekey[4] = thekey[3];
		temp  = thekey[3];
		thekey[3] = thekey[5];
		temp  = thekey[5];
		thekey[5] = thekey[6];

#if PA_BIG_ENDIAN
		for(i=0; i < 8; i++)
		{
		memcpy(swp,&(thekey[i]),4);
		p = (unsigned char*)&(thekey[i]);
		*p = swp[3];
		*(p+1) = swp[2];
		*(p+2) = swp[1];
		*(p+3) = swp[0];
		}
#endif
	makeKey(&ky, (unsigned char)direction, PA_CRYPT_BIT_SIZE, (char*)thekey);
}
