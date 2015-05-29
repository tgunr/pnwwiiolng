/*
 *  diddle.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 5/13/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */
#if platOS != OS_MACOS
#include "wwii/wwiiCovert.h"
#include "winnt.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;
#define NTSIGNATURE(ptr) ((LPVOID)((BYTE*)(ptr) + ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew))
#define SIZE_OF_NT_SIGNATURE (sizeof(DWORD))
#define PEFHDROFFSET(ptr) ((LPVOID)((BYTE*)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE))
#define OPTHDROFFSET(ptr) ((LPVOID)((BYTE*)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE+sizeof(IMAGE_FILE_HEADER)))
#define SECHDROFFSET(ptr) ((LPVOID)((BYTE*)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE+sizeof(IMAGE_FILE_HEADER)+sizeof(IMAGE_OPTIONAL_HEADER)))
class HideawayStorage
{
public:
    const static size_t MemSizeElements = 4;
private:
    size_t memory[MemSizeElements];
public:
    inline size_t* getMemory() { return memory; }
};

class Hideaway
{
private:
    HideawayStorage& storage;

    volatile size_t Mask1;
    volatile size_t Mask2;
    volatile size_t Mask3;
    volatile size_t Mask4;
    size_t thiscrap;

public:
    Hideaway(HideawayStorage& st) : storage(st)
    {
        size_t crap;
        Mask1 = (size_t)&thiscrap;
        Mask2 = 0xffffffff-(size_t)&thiscrap;
        Mask3 = (size_t)&crap;
        Mask4 = 0xffffffff-(size_t)&crap;
    }

    inline void save(size_t value, size_t* to )
    {
        to[3] = (value & (255L<<8*3))^Mask1;
        to[0] = (value & (255L<<0))^Mask2;
        to[2] = (value & (255L<<8*2))^Mask3;
        to[1] = (value & (255L<<8*1))^Mask4;
    }

    inline size_t recover( size_t* from )
    {
        return ((from[3])^Mask1)
        |  ((from[0])^Mask2)
        |  ((from[2])^Mask3)
        |  ((from[1])^Mask4);
    }

    inline void set(size_t value)
    {
        save(value, storage.getMemory());
    }

    inline size_t get()
    {
        return recover( storage.getMemory() );
    }

    inline bool compare( size_t value )
    {
        size_t memory[HideawayStorage::MemSizeElements];
        save( value, memory );
        return memory[0] == storage.getMemory()[0]
            && memory[1] == storage.getMemory()[1]
            && memory[3] == storage.getMemory()[3]
        && memory[2] == storage.getMemory()[2];
    }

};

static inline unsigned int diddle_reflect(unsigned int ulReflect, const char cChar)
{
	unsigned int ulValue = 0;
	for (int iPos = 1; iPos < (cChar + 1); iPos++)
	{
		if (ulReflect & 1)
		{
			ulValue |= (1 << (cChar - iPos));
		}
		ulReflect >>= 1;
	}
	return ulValue;
}
static inline unsigned int diddle_crc(const unsigned char* sData, unsigned int ulDataLength)
{
	unsigned int ulTable[256];
	unsigned int ulPolynomial = 0x04C11DB7;
	for (int iCodes = 0; iCodes <= 0xFF; iCodes++)
	{
		ulTable[iCodes] = diddle_reflect(iCodes, 8) << 24;
		for (int iPos = 0; iPos < 8; iPos++)
		{
			ulTable[iCodes] = (ulTable[iCodes] << 1)
				^ ((ulTable[iCodes] & (1 << 31)) ? ulPolynomial : 0);
		}
		ulTable[iCodes] = diddle_reflect(ulTable[iCodes], 32);
	}
	unsigned int ulCRC = (0xffffffff);
	while (ulDataLength--)
	{
		ulCRC = (ulCRC >> 8) ^ ulTable[(ulCRC & 0xFF) ^ *sData++];
	}
	return ulCRC ^ (0xffffffff);
}

#endif
