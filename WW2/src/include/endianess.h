#pragma once
// define endianness seperate from platform, so that all plats with the
// same endian structure behave in the same way

#if platOS == OS_MACOS

#include <stdint.h>
#include <machine/byte_order.h>

	#define EndianSwap16(c)  (*(c)) = OSReadSwapInt16((c), 0)
	#define EndianSwap32(c)  (*(c)) = OSReadSwapInt32((c), 0)
	#define EndianSwap64(c)  (*(c)) = OSReadSwapInt64((c), 0)

	static inline void EndianSwap16s(uint16_t* x, int n)
	{
		int i;
		for(i=0; i < n; ++i, ++x) EndianSwap16(x);
	}

	static inline void EndianSwap32s(uint32_t* x, int n)
	{
		int i;
		for(i=0; i < n; ++i, ++x) EndianSwap32(x);
	}

	#define ENDIAN_SWAP_16(x) EndianSwap16((uint16_t*)&x)
	#define ENDIAN_SWAP_32(x) EndianSwap32((uint32_t*)&x)
	#define ENDIAN_SWAP_64(x) EndianSwap64((uint64_t*)&x)

	#define ENDIAN_SWAP_16s(x,n) EndianSwap16s((uint16_t*)x,n)
	#define ENDIAN_SWAP_32s(x,n) EndianSwap32s((uint32_t*)x,n)
#else
	// Byte swapping functions. Do not call these directy, 
	// but use the macros below instead.
	_inline void EndianSwap16(unsigned char* c)
	{
		c[0] ^= c[1]; c[1] ^= c[0]; c[0] ^= c[1];
	}	

	_inline void EndianSwap32(unsigned char* c)
	{
		c[0] ^= c[3]; c[3] ^= c[0]; c[0] ^= c[3];
		c[1] ^= c[2]; c[2] ^= c[1]; c[1] ^= c[2];
	}

	_inline void EndianSwap64(unsigned char* c)
	{
		c[0] ^= c[7]; c[7] ^= c[0]; c[0] ^= c[7];
		c[1] ^= c[6]; c[6] ^= c[1]; c[1] ^= c[6];
		c[2] ^= c[5]; c[5] ^= c[2]; c[2] ^= c[5];
		c[3] ^= c[4]; c[4] ^= c[3]; c[3] ^= c[4];
	}

	#define ENDIAN_SWAP_16(x) EndianSwap16((unsigned char*)&x)
	#define ENDIAN_SWAP_32(x) EndianSwap32((unsigned char*)&x)
	#define ENDIAN_SWAP_64(x) EndianSwap64((unsigned char*)&x)
#endif

#if platEndian == platBigEndian

	#define LITTLE_TO_NATIVE_ENDIAN_16(x)	ENDIAN_SWAP_16(x)
	#define NATIVE_TO_LITTLE_ENDIAN_16(x)	ENDIAN_SWAP_16(x)
	#define BIG_TO_NATIVE_ENDIAN_16(x)		
	#define NATIVE_TO_BIG_ENDIAN_16(x)		

	#define LITTLE_TO_NATIVE_ENDIAN_32(x)	ENDIAN_SWAP_32(x)
	#define NATIVE_TO_LITTLE_ENDIAN_32(x)	ENDIAN_SWAP_32(x)
	#define BIG_TO_NATIVE_ENDIAN_32(x)		
	#define NATIVE_TO_BIG_ENDIAN_32(x)		

	#define LITTLE_TO_NATIVE_ENDIAN_64(x)	ENDIAN_SWAP_64(x)
	#define NATIVE_TO_LITTLE_ENDIAN_64(x)	ENDIAN_SWAP_64(x)
	#define BIG_TO_NATIVE_ENDIAN_64(x)		
	#define NATIVE_TO_BIG_ENDIAN_64(x)		

#else	// platEndian == platBigEndian

	#define LITTLE_TO_NATIVE_ENDIAN_16(x) 	
	#define NATIVE_TO_LITTLE_ENDIAN_16(x) 	
	#define BIG_TO_NATIVE_ENDIAN_16(x)		EndianSwap16((unsigned char*)&x)
	#define NATIVE_TO_BIG_ENDIAN_16(x)		EndianSwap16((unsigned char*)&x)

	#define LITTLE_TO_NATIVE_ENDIAN_32(x)	
	#define NATIVE_TO_LITTLE_ENDIAN_32(x)	
	#define BIG_TO_NATIVE_ENDIAN_32(x)		EndianSwap32((unsigned char*)&x)
	#define NATIVE_TO_BIG_ENDIAN_32(x)		EndianSwap32((unsigned char*)&x)

	#define LITTLE_TO_NATIVE_ENDIAN_64(x)	
	#define NATIVE_TO_LITTLE_ENDIAN_64(x)	
	#define BIG_TO_NATIVE_ENDIAN_64(x)		EndianSwap64((unsigned char*)&x)
	#define NATIVE_TO_BIG_ENDIAN_64(x)		EndianSwap64((unsigned char*)&x)

#endif 	// platEndian == platBigEndian
