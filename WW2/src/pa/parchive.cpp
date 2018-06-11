#include "syscfg.h" // Path is project specific.
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>

#if platOS == OS_MACOS
     #include <fcntl.h>
#endif

#include "pa/parchive.h"
#include "pa/rijndael-api-fst.h"
#include "pa/md5.h"

#ifndef NOT_WW2OL
#include "pi/pi.h"
#include "wwii/wwiiClientMessages.h"
#endif // NOT_WW2OL

#include "wwii/wwiiErrorTypes.h"

/************************************************************************************************
	parchive
*************************************************************************************************/

#define PA_MAGIC 'crsa'
#define PA_CRYPT_BIT_SIZE 256 // Options are 128, 192, 256 (mil-spec)
#define PA_MAX_NAME 127
#define PA_ENCRYPTED_MEMBER 0x00000001
#define PA_TOC_SIZE 8192
#define PA_MAX_ARC 64

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

struct parc
{
	FILE* f;
	char name[PA_MAX_NAME+1];
	int updatemd5;
};

struct paTOC
{
	parc* pa;
	char name[PA_MAX_NAME+1];
	int offset;
	unsigned int filelen;
	unsigned int datalen;
	unsigned int crc;
	unsigned int flags;
	unsigned int seed;
};

struct paFILE
{
	parc arc[PA_MAX_ARC];

	int updatetoc;
	int numtoc;
	paTOC* toc;
};

extern "C" int ROUNDS;
int ROUNDS;

static keyInstance ky;
static cipherInstance ci;
static int cipherInitialized = 0;

static int paUpdateTOC(paFILE*);

static int create_toc(parc* pa, paTOC**, int* numtoc);
static int toc_compare(const void* , const void*);
static int toc_search(const void*, const void*);
static void fix_path_name(char* name);

static int rijndaelcrypt(unsigned char* block, int size, int direction, int salt);
static void setrijndaelkey(unsigned int* k,int salt, int direction);
static void update_arc_md5(FILE*);
static void sampled_md5( unsigned char*, const unsigned char* data, int len, unsigned long int seed);

unsigned int crc32(unsigned int old_crc, const unsigned char* in_comming, int in_size);
#if(0)
static int validity_check(paTOC* member, unsigned char* data, unsigned char* srcMD5);
#endif

#ifndef NOT_WW2OL
#if(0)
static int die_on_validity_check(paTOC* member, unsigned char* data);
#endif
#endif // NOT_WW2OL

/************************************************************************************************
	paAlloc
*************************************************************************************************/

paFILE* paAlloc()
{
	return (paFILE*)calloc(1,sizeof(paFILE));
}

/************************************************************************************************
	paFree
*************************************************************************************************/

void paFree(paFILE* pf)
{
	int i;

    if(pf->toc)
    {
        free(pf->toc);
        pf->toc = nullptr;
    }

	for(i = 0; i < PA_MAX_ARC; i++)
	{
		if(pf->arc[i].f != nullptr)
		{
	 		paClose(pf, i);
		}
	}

	free(pf);
}

/************************************************************************************************
	paOpen
*************************************************************************************************/

int	paOpen(paFILE* pf, const char* archivename, const char* mode)
{
	int i;

#if platOS == OS_WIN32
    wwiiolpath_string curWorkingDir = L"";
	wchar_t wBuffer[MAX_PATH + 1];

	wBuffer[0] = 0;
	GetCurrentDirectory(MAX_PATH, wBuffer);

	curWorkingDir = wBuffer;

	SetCurrentDirectory(WW2_exePlayarcPath.c_str());

#endif
    
    for(i = 0; i < PA_MAX_ARC; i++)
	{
		if(pf->arc[i].f == nullptr)
		{
			pf->arc[i].f = fopen(archivename, mode);

			if(pf->arc[i].f != nullptr)
			{
				strncpy(pf->arc[i].name, archivename, PA_MAX_NAME);
				pf->updatetoc = 1;

				return i;
			}
		}
	}

#if platOS == OS_WIN32
	SetCurrentDirectory(curWorkingDir.c_str());
#endif

	return -1;
}

/************************************************************************************************
	paClose
*************************************************************************************************/

void paClose(paFILE* pf, int archiveIdx)
{
	if(archiveIdx > -1 && archiveIdx < PA_MAX_ARC)
	{
		if(pf->arc[archiveIdx].updatemd5)
			update_arc_md5(pf->arc[archiveIdx].f);

		if(pf->arc[archiveIdx].f != nullptr)
			fclose(pf->arc[archiveIdx].f);

		memset(&pf->arc[archiveIdx], 0x0, sizeof(parc));
		pf->updatetoc = 1;
	}
}

/************************************************************************************************
	paFetchMember
*************************************************************************************************/

int paFetchMember(paFILE* pf, const char* membername, unsigned char** data, size_t* len)
{
	char name[PA_MAX_NAME+1];
	paTOC* member;
//	int validityCheckFailed = 0;
//	unsigned int crc = 0;

	if(data) *data = nullptr;
	if(len) *len = -1;

	// create toc
	if(pf->updatetoc || pf->toc == nullptr)
		paUpdateTOC(pf);

	if(pf->toc == nullptr) return -1;

	pf->updatetoc = 0;

	strsafecpy(name, membername);
	fix_path_name(name);

	member = (paTOC*) bsearch(name, pf->toc, pf->numtoc, sizeof(paTOC), toc_search);

	if(member)
	{
		unsigned char srcMD5[16];

		if(len) *len = member->datalen;

		if(data)
		{
			if(member->pa->f)
			{
				fseek(member->pa->f, member->offset - 16, SEEK_SET);

				fread(srcMD5, 1, 16, member->pa->f);

				*data = (unsigned char*)calloc(member->filelen + 1, 1);
				if(*data == nullptr)
				{
					return -1;
				}

				fread(*data, 1, member->filelen, member->pa->f);

				if(member->flags & PA_ENCRYPTED_MEMBER)
				{
					rijndaelcrypt(*data, member->filelen, DIR_DECRYPT, member->datalen);
				}

				return 0; //validity_check(member, *data, srcMD5);
			}
		}

		return 0;
	}

	return -1;
}

/************************************************************************************************
	paAppendmember
*************************************************************************************************/

int paAppendmember(paFILE* pf, const char* membername, const unsigned char* memberdata, int len)
{
	int i;
	int encrypt = 1;
	unsigned char* cryptdata = nullptr;
	const unsigned char* writedata;
	int writelen;
	paMemberHead memberhead;
	parcHead archead;
	FILE* f = pf->arc[0].f;

	if(pf->arc[0].f == nullptr) return -1;

	memset(&memberhead, 0x0, sizeof(memberhead));
	memset(&archead, 0x0, sizeof(archead));

	// write archive header
	fseek(pf->arc[0].f, 0, SEEK_SET);

	archead.magic = PA_MAGIC;
	archead.version = 0x0;

	fwrite(&archead, sizeof(parcHead), 1, f);

	// determine if the data is ASCII and encrypt
	for(i = 0; i < len; i++)
	{
		if(!isascii(memberdata[i]))
		{
			encrypt = 0;
			break;
		}
	}

	// encrypt data
	if(encrypt)
	{
		cryptdata = (unsigned char*)calloc(len + 64,1);
		memset(cryptdata, 0xff, len + 64);
		memcpy(cryptdata, memberdata, len);

		writelen = (len + 15) & ~15;	// fast 16 byte align

		rijndaelcrypt(cryptdata, writelen, DIR_ENCRYPT, len);
		writedata = cryptdata;

		memberhead.flags |= PA_ENCRYPTED_MEMBER;
	}else
	{
		writedata = memberdata;
		writelen = len;
	}

	// member header
	fseek(f, 0, SEEK_END);

	strsafecpy(memberhead.name, membername);
	fix_path_name(memberhead.name);

	memberhead.filelen = writelen;
	memberhead.datalen = len;
	memberhead.crc = crc32(0, memberdata, len);
	memberhead.seed = clock();
	sampled_md5(memberhead.md5, memberdata, len, memberhead.seed);

	// Obscure our header information a bit
	// (except md5 and seed,  we don't want entire header changing everytime we build an archive,
	// that's also why we use a fixed salt value, 666)
	rijndaelcrypt((unsigned char*)&memberhead, sizeof(memberhead) - 20, DIR_ENCRYPT, 666);
	fwrite(&memberhead, sizeof(memberhead), 1, f);

	// member data
	fwrite(writedata, writelen, 1, f);

	// md5 needs to be updated
	pf->arc[0].updatemd5 = 1;

	if(cryptdata)
		free(cryptdata);

	return 0;
}

/************************************************************************************************
	paFreeMember
*************************************************************************************************/

void paFreeMember(unsigned char* data)
{
	if(data)
		free(data);
}

/************************************************************************************************
	paUpdateTOC
*************************************************************************************************/

static int paUpdateTOC(paFILE* pf)
{
	int i, tocidx, totaltoc;
	paTOC* toc[PA_MAX_ARC];
	int numtoc[PA_MAX_ARC];

	if(pf->toc)
		free(pf->toc);

	// create individual arc toc's
	totaltoc = 0;
	for(i = 0, tocidx = 0; i < PA_MAX_ARC; i++)
	{
		if(pf->arc[i].f != nullptr)
		{
			if(create_toc(&pf->arc[i], &toc[tocidx], &numtoc[tocidx]) == 0)
			{
				totaltoc += numtoc[tocidx];
				tocidx++;
			}
		}
	}
	pf->toc = (paTOC*)calloc(totaltoc, sizeof(paTOC));
	pf->numtoc = totaltoc;

	if(pf->toc)
	{
		// merge and sort 'em
		paTOC* curtoc = pf->toc;
		for(i = 0; i < tocidx; i++)
		{
			memcpy(curtoc, toc[i], numtoc[i] * sizeof(paTOC));
			curtoc += numtoc[i];
		}

        qsort(pf->toc, pf->numtoc, sizeof(paTOC), toc_compare);
	}

	for(i = 0; i < tocidx; i++)
	{
		free(toc[i]);
	}

	return 0;
}

/************************************************************************************************
	create_toc
*************************************************************************************************/

#define zPRINT_TOC_ENTRIES

static int create_toc(parc* pa, paTOC** toc, int* numtoc)
{
	paMemberHead memberhead;
	int maxtoc = PA_TOC_SIZE;
	int i;

	*toc = nullptr;
	*numtoc = 0;

	*toc = (paTOC*)calloc(maxtoc, sizeof(paTOC));
	if(toc == nullptr) return -1;
#ifdef PRINT_TOC_ENTRIES
  printf("---------- %s ------------\n",pa->name);
#endif
	if(pa->f)
	{
		fseek(pa->f, sizeof(parcHead), SEEK_SET);

		for(i = 0; ; i++)
		{
			if(i >= maxtoc)
			{
				paTOC* newtoc = (paTOC*)calloc(maxtoc + PA_TOC_SIZE, sizeof(paTOC));
				if(newtoc == nullptr) break;

				memcpy(newtoc, *toc, sizeof(paTOC) * maxtoc);

				free(*toc);
				*toc = newtoc;

				maxtoc += PA_TOC_SIZE;
			}

			if(fread(&memberhead, sizeof(memberhead),1, pa->f) != 1) break;

			rijndaelcrypt((unsigned char*)&memberhead, sizeof(memberhead) - 20, DIR_DECRYPT, 666);

#ifdef PRINT_TOC_ENTRIES
      printf("%s [%d]\n",memberhead.name,memberhead.datalen);
#endif
			strsafecpy((*toc)[i].name, memberhead.name);
			(*toc)[i].pa = pa;
			(*toc)[i].offset = ftell(pa->f);
			(*toc)[i].datalen = memberhead.datalen;
			(*toc)[i].filelen = memberhead.filelen;
			(*toc)[i].crc = memberhead.crc;
			(*toc)[i].flags = memberhead.flags;
			(*toc)[i].seed = memberhead.seed;

			fseek(pa->f, memberhead.filelen, SEEK_CUR);
		}

		*numtoc = i;
	}

	return 0;
}

/************************************************************************************************
	toc_compare
*************************************************************************************************/

static int toc_compare(const void* t1, const void* t2)
{
	return strcmp(((paTOC*)t1)->name, ((paTOC*)t2)->name);
}

/************************************************************************************************
	toc_search
*************************************************************************************************/

static int toc_search(const void* name, const void* toc)
{
	return strcmp((char*)name, ((paTOC*)toc)->name);
}

/************************************************************************************************
	fix_path_name
*************************************************************************************************/

static void fix_path_name(char* name)
{
	int i,j;
	char t[PA_MAX_NAME+1];
	int len = strlen(name);
	if(len > PA_MAX_NAME) len = PA_MAX_NAME;

	// remove initial dot and slashes
	for(i=0, j=0; i < len; i++)
	{
		if(name[i] != '\\' && name[i] != ':' &&
			name[i] != '/' && name[i] != '.') break;
	}

	for(; i < len; i++)
	{
		// convert slashes
		if((name[i] == '\\') || (name[i] == ':')) name[i] = '/';
		// convert to lower case
		name[i] = tolower(name[i]);
    	// strip double slashes
        if(i > 0)
        {
            if((name[i] == '/') &&  (name[i-1] == '/'))
            {
                // nuking slash by doing nothing
               ;
            }
            else
            {
                t[j] = name[i];
                j++;
            }
        }
        else
        {
            t[j] = name[i];
            j++;
        }
	}
    t[j] = '\0';

    strsafecpy(name, t, PA_MAX_NAME);
}

/************************************************************************************************
	rijndaelcrypt
*************************************************************************************************/

static int rijndaelcrypt(unsigned char* block, int size, int direction, int salt)
{
	unsigned char* out_block;

	setrijndaelkey(nullptr, salt, direction);

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

/************************************************************************************************
	setrijndaelkey
*************************************************************************************************/

static void setrijndaelkey(unsigned int* k,int salt, int direction)
{
// We need to make these memory locations bubble around for
// the key crackers, Make sure they have to track 1024 bits at least

unsigned int thekey[32];
unsigned int iv[32];
unsigned int temp; // temporary holding spot for key-part
int i;

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
	 	cipherInit(&ci, MODE_ECB, nullptr);
		cipherInitialized = 1;
	}

	if(k == nullptr) // for unknown keys
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

	makeKey(&ky, (unsigned char)direction, PA_CRYPT_BIT_SIZE, (char*)thekey);
}

/************************************************************************************************
	update_arc_md5
*************************************************************************************************/

void update_arc_md5(FILE* f)
{
	MD5_CTX ctx;
	unsigned char buffer[1024];
	unsigned char md5[16];
	int len;

	MD5Init(&ctx);

	fseek(f, sizeof(parcHead), SEEK_SET);

	for(;;)
	{
		len = fread(buffer, 1, 1024, f);
		if(len < 1) break;
		MD5Update(&ctx, buffer, len);
	}

	MD5Final(md5, &ctx);

	fseek(f, 8 ,SEEK_SET);
	fwrite(md5, 16, 1,f );
}

/************************************************************************************************
	sampled_md5
*************************************************************************************************/

void sampled_md5(unsigned char* md5, const unsigned char* data, int len, unsigned long int seed)
{
	int i;
	int numsamples, samplelen, sampleoffset;
	int rand1, rand2;

	MD5_CTX ctx;

	// arbitrary sampling, 5% of file
	samplelen = 64;
	numsamples = (int)(len * 0.00078125f); // 1/64 * .05

	if(numsamples < 1)
	{
		samplelen = len;
		numsamples = 1;
	}

	MD5Init(&ctx);

	for(i = 0; i < numsamples; i++)
	{
		seed = seed * 1103515245 + 12345;
		rand1 = ((seed >> 16) & 0x7FFF);

		seed = seed * 1103515245 + 12345;
		rand2 = ((seed  >> 16) & 0x7FFF);

		sampleoffset = (len != samplelen) ? (rand1 * rand2) % (len - samplelen) : 0;

		MD5Update(&ctx, data + sampleoffset , samplelen);
	}

	MD5Final(md5, &ctx);
}

/************************************************************************************************
	crc
*************************************************************************************************/

unsigned int crc_array[] =
{
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
  0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
  0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
  0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
  0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
  0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
  0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
  0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
  0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
  0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
  0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
  0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
  0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
  0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
  0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
  0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
  0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
  0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
  0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
  0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
  0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
  0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
  0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
  0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
  0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
  0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
  0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
  0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
  0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
  0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
  0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
  0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
  0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
  0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
  0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
  0x2d02ef8d
};

// Macro to manipulate the chars according to the above array

#define CRC32(c, b) (crc_array[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))

unsigned int crc32(unsigned int old_crc, const unsigned char* in_comming, int in_size)
{
unsigned int crc; 	// Our internal crc variable
int i;

	if (in_comming == nullptr)
		return(0);

	if(in_size <= 0)
		return(0);

	crc = old_crc ^ 0xffffffff;

	for(i=0; i < in_size; i++)
		crc = CRC32(crc, *in_comming++);

	crc ^= 0xffffffff;

  return (crc);
}

#ifndef NOT_WW2OL

/************************************************************************************************
	die_on_validity_check
*************************************************************************************************/

#if(0)
static int die_on_validity_check(paTOC* member, unsigned char* data)
{
	const char* ext = strrchr(member->name, '.');

	if(ext && !strcmp(ext, ".bmp"))
	{
		piBITMAPINFOHEADER* bmpHeader = (piBITMAPINFOHEADER*)(data + sizeof (piBITMAPFILEHEADER));
		int numcolors = (bmpHeader->biClrUsed) ? bmpHeader->biClrUsed : 256;
		unsigned int bmpsize = 14 + bmpHeader->biSize + numcolors * sizeof(piRGBQUAD) + bmpHeader->biWidth * bmpHeader->biHeight * bmpHeader->biBitCount * bmpHeader->biPlanes / 8;
		if(member->datalen < bmpsize + 1024) return 0; // for some reason the sizes are sometimes off even in valid bmp's.
	}

	return 1;
}
#endif

/************************************************************************************************
	validity_check
*************************************************************************************************/

#if(0)
static int validity_check(paTOC* member, unsigned char* data, unsigned char* srcMD5)
{
	unsigned int crc = 0;
	int validityCheckFailed = 0;
	unsigned char dstMD5[16];

	crc = crc32(0, data, member->datalen);

	if(member->crc != crc)// && die_on_validity_check(member, *data))
	{
		validityCheckFailed = 1;
/*		char str[256];
		snprintf(str, 255, "Validity check on archive %s failed",  member->pa->name);

		piExitProgram(_FL, str);
		*data = nullptr;
*/
	}

	sampled_md5(dstMD5, data, member->datalen, member->seed);

	if(memcmp(srcMD5, dstMD5, 16))
	{
		validityCheckFailed = 1;
/*		volatile int x, y;
		x = 1;
		y = 0;
		x =  x / y; //ctd
		*data = nullptr;
*/
	}

	if(validityCheckFailed)
	{
		WWII_ERROR_INFO errInfo;
		memset(&errInfo, 0x0, sizeof(errInfo));

		errInfo.errorType = WW_ERR_VALIDITY_CHECK;
		errInfo.archiveWasCorrupt = 1;

		strncpy(errInfo.data.validityInfo.archive.string, member->pa->name, WWII_ERR_STRING::_MAX_LEN);
		errInfo.data.validityInfo.archive.length = strlen(errInfo.data.validityInfo.archive.string);

		strncpy(errInfo.data.validityInfo.filename.string, member->name, WWII_ERR_STRING::_MAX_LEN);
		errInfo.data.validityInfo.filename.length = strlen(errInfo.data.validityInfo.filename.string);

		errInfo.data.validityInfo.crc = crc;
		convertMD5StringToHex(dstMD5, errInfo.data.validityInfo.md5);

		wwiiQueuedSend_WWII_ERROR_INFO(&errInfo);
	}

	return 0;
}
#endif

#else // NOT_WW2OL

/************************************************************************************************
	validity_check
*************************************************************************************************/

static int validity_check(paTOC* member, unsigned char* data, unsigned char* srcMD5)
{
	return 0;
}

#endif // NOT_WW2OL
