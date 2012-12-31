#pragma once
// MD5.H - header file for MD5C.C

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if 0 // original
/* MD5 context. */
typedef struct {
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;
#endif

#define UINTM unsigned int
#define COND unsigned int

struct MD5Context
{
	UINTM buf[4];
	UINTM bits[2];
	unsigned char in[64];
};


typedef struct MD5Context MD5_CTX;

#if 0 // original
void MD5Init PROTO_LIST ((MD5_CTX*));
void MD5Update PROTO_LIST  ((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX*));
#endif

void MD5Init(struct MD5Context* context);
void MD5Update(struct MD5Context* context, unsigned char const* buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context* context);
void MD5Transform(UINTM buf[4], UINTM const in[16]);

int convertMD5StringToHex(const unsigned char* md5string, unsigned char* md5hex);

void getMD5FromString (const char* string, unsigned char* digest);

COND getMD5FromFile (const char* filename, unsigned char* digest);


#ifdef __cplusplus
}
#endif

