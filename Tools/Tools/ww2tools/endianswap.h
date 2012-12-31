#ifndef _ENDIANSWAP_H
#define _ENDIANSWAP_H

/************************************************************************************************
	endianswap
*************************************************************************************************/

#if IS_BIG_ENDIAN

#define NATIVE_TO_LITTLE16(x) 	endian_swap16((unsigned char*)&x)
#define NATIVE_TO_LITTLE32(x) 	endian_swap32((unsigned char*)&x)
#define NATIVE_TO_LITTLE64(x) 	endian_swap64((unsigned char*)&x)
#define NATIVE_TO_BIG16(x) 		
#define NATIVE_TO_BIG32(x) 		
#define NATIVE_TO_BIG64(x) 	

#else

#define NATIVE_TO_LITTLE16(x) 	
#define NATIVE_TO_LITTLE32(x) 	
#define NATIVE_TO_LITTLE64(x) 	
#define NATIVE_TO_BIG16(x) 		endian_swap16((unsigned char*)&x)
#define NATIVE_TO_BIG32(x) 		endian_swap32((unsigned char*)&x)
#define NATIVE_TO_BIG64(x) 		endian_swap64((unsigned char*)&x)

#endif

#define LITTLE_TO_NATIVE16(x) 	NATIVE_TO_LITTLE16(x) 
#define LITTLE_TO_NATIVE32(x) 	NATIVE_TO_LITTLE32(x) 
#define LITTLE_TO_NATIVE64(x) 	NATIVE_TO_LITTLE64(x) 
#define BIG_TO_NATIVE16(x) 		NATIVE_TO_BIG16(x)
#define BIG_TO_NATIVE32(x) 		NATIVE_TO_BIG32(x)
#define BIG_TO_NATIVE64(x) 		NATIVE_TO_BIG64(x)

#define ENDIAN_SWAPS(x) ( (x << 8)  | ((x >> 8) & 0xff) )
#define ENDIAN_SWAPL(x) ( (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | ((x >> 24) & 0xff) );

#if __APPLE__

#include <libkern/OSByteOrder.h>
#define endian_swap16(c)  *(uint16_t*)c = OSReadSwapInt16(c, 0)
#define endian_swap32(c)  *(uint32_t*)c = OSReadSwapInt32(c, 0)
#define endian_swap64(c)  *(uint64_t*)c = OSReadSwapInt64(c, 0)

#else

#define endian_swap16(c) \
{ \
	(c)[0] ^= (c)[1]; (c)[1] ^= (c)[0]; (c)[0] ^= (c)[1]; \
}	

#define endian_swap32(c) \
{ \
	(c)[0] ^= (c)[3]; (c)[3] ^= (c)[0]; (c)[0] ^= (c)[3]; \
	(c)[1] ^= (c)[2]; (c)[2] ^= (c)[1]; (c)[1] ^= (c)[2]; \
}

#define endian_swap64(c) \
{ \
	(c)[0] ^= (c)[7]; (c)[7] ^= (c)[0]; (c)[0] ^= (c)[7]; \
	(c)[1] ^= (c)[6]; (c)[6] ^= (c)[1]; (c)[1] ^= (c)[6]; \
	(c)[2] ^= (c)[5]; (c)[5] ^= (c)[2]; (c)[2] ^= (c)[5]; \
	(c)[3] ^= (c)[4]; (c)[4] ^= (c)[3]; (c)[3] ^= (c)[4]; \
}

#endif

#endif // _ENDIANSWAP_H
