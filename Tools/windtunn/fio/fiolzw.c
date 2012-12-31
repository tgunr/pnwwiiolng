// fiolzw.c
//

/*
	 What am i?
	 Copyright 2001 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Did You Write This?
	Maintaining Programmer : Have You Taken it Over?
	Supporting Programmer  : Have you had Someone Elses Messy Code frobbed off on you?
	
	Rev 1  c2001
*/

#include "syscfg.h"
#include "pi.h"
#include "vars.h"
#include "mem.h"
#include "fiolzw.h"
#include <string.h>
#include "fio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if platOS == OS_WIN32
	// Test purposes ONLY!
	#define USE_ASM
	#undef USE_ASM
#elif platOS == OS_MACOS
	#include <OSUtils.h>
	#undef USE_ASM
#endif

#define USE_PRIMES


#define LZW_CHAIN_CLEAR   256
#define LZW_CHAIN_EOI     257
#define LZW_CHAIN_START   258


static UCHAR     revbyte[256];
static UINT32    *chain;
static UINT16     *codekey;
static UINT16     *highkey;
static int      lzw_max_bits;
static int      lzw_max_mask;
static int      lzw_chain_length;
static int      lzw_chain_block;
static int      nextinchain;
static int      numbits;
static int      bitmask;
static UINT32    workbits;
static int      validbits;

#ifdef USE_PRIMES
static int      primes[8]={509,1021,2039,4093,8191,16381,32749,65521};
#endif


void fiolzwTest(void)
{
#include "fio.h"
UCHAR  *oldread, *read, *oldsave, *save;
int   retlen, length;
int   stream;
UINT32 ttime;
FILE  *river;

  stream = fioOpen("world2.bmp");
  if (stream == -1)
  {
    return;
  }

  length = fioSeek(stream, 0, fioSEEK_END);
  fioSeek(stream, 0, fioSEEK_SET);
  read = (UCHAR*)memAlloc(length);
  if (read == NULL)
  {
    fioClose(stream);
    return;
  }

  if (length != (long)fioRead(stream, read, length))
  {
    fioClose(stream);
    memFree(read);
    return;
  }

  fioClose(stream);

  save = (UCHAR*)memAlloc(length);
  if (save == NULL)
  {
    memFree(read);
    return;
  }

  fiolzwInit(12);

  oldsave = save;

  Write_Debug("Unencoded size: %d", length);

  retlen = length;
  #if platOS == OS_WIN32
	  ttime = GetTickCount();
	  fiolzwEncode(&oldsave, read, &retlen);
	  ttime = GetTickCount() - ttime;
  #elif platOS == OS_MACOS
	  ttime = TickCount();
	  fiolzwEncode(&oldsave, read, &retlen);
	  ttime = TickCount() - ttime;
  #else
  	#error "put in time calculation..."
  #endif
  Write_Debug("Encode time: %d ms", ttime);
  Write_Debug("Encoded size: %d", retlen);
  Write_Debug("Encode ratio: %f", (float)retlen / (float)length);
  Write_Debug("Encode bytes per millisecond: %f", (float)length / (float)ttime);

  memset(read, 0, length);

  oldread = read;
  length = retlen;

  Write_Debug("Encoded size: %d", length);

  retlen = length;
  #if platOS == OS_WIN32
	  ttime = GetTickCount();
	  fiolzwDecode(&oldread, save, &retlen);
	  ttime = GetTickCount() - ttime;
  #elif platOS == OS_MACOS
	  ttime = TickCount();
	  fiolzwDecode(&oldread, save, &retlen);
	  ttime = TickCount() - ttime;
  #else
  	#error "put in time calculation..."
  #endif
  Write_Debug("Decode time: %d ms", ttime);
  Write_Debug("Decoded size: %d", retlen);
  Write_Debug("Decode ratio: %f", (float)retlen / (float)length);
  Write_Debug("Decode bytes per millisecond: %f", (float)retlen / (float)ttime);

  fiolzwKill();

  river = fopen("world2.bmp", "wb");
  fwrite(read, 1, retlen, river);
  fclose(river);

  memFree(read);
  memFree(save);
}


static UCHAR ReverseByte(UCHAR in)
{
UCHAR  out, b1, b2;

  for (out=0, b1=1, b2=0x80 ; b1 ; b1<<=1, b2>>=1)
  {
    if (in & b1)
    {
      out |= b2;
    }
  }

  return(out);
}


static UINT16 ReverseWord(UINT16 in)
{
  return((revbyte[in & 0xff] << 8) | revbyte[in >> 8]);
}


static LOGICAL Clear(void)
{
int   i;

  if (chain == NULL ||
      codekey == NULL ||
      highkey == NULL)
  {
    return(FALSE);
  }

  memset(chain, 0, lzw_chain_block);
  memset(codekey, 0, lzw_chain_block/2);
  memset(highkey, 0, lzw_chain_block/2);

  for (i=0 ; i < 256 ; i++)
  {
    chain[i] = 0x01000000 | i;
  }

  for ( ; i < LZW_CHAIN_START ; i++)
  {
    chain[i] = 0x01000000;
  }

  nextinchain = LZW_CHAIN_START;
  numbits = 9;
  bitmask = (1 << numbits) - 1;

  return(TRUE);
}


static int FindChain(UINT32 code)
{
int   i, j;
UINT32 key;

  key = code % lzw_max_mask;
  if (codekey[key] == 0)
  {
    return(-1);
  }

  i = codekey[key];
  j = highkey[key];

#ifndef USE_ASM

  for ( ; i <= j ; i++)
  {
    if (chain[i] == code)
    {
      return(i);
    }
  }

  return(-1);

#else
/*
  __asm
  {
    mov ebx, j
    mov ecx, ebx
    sub ecx, i
    inc ecx
    mov eax, code
    mov edi, chain
    repne scasd
    jne notfound
    sub ebx, ecx
    mov i, ebx
  }
    return(i);

notfound:

  return(-1);
*/

  __asm
  {
    mov eax, code
    mov ebx, chain
    mov ecx, i
    mov edx, j

label:

    cmp eax, [ebx+ecx*4]
    je  found
    inc ecx
    cmp ecx, edx
    jle label
    jmp notfound

found:

    mov i, ecx
  }

  return(i);

notfound:

  return(-1);

#endif
}


static void AddChain(UINT32 code)
{
UINT32 key;

  key = code % lzw_max_mask;
  if (codekey[key] == 0)
  {
    codekey[key] = nextinchain;
  }

  highkey[key] = nextinchain;

  chain[nextinchain++] = code;
}


static LOGICAL CheckNextLevel(void)
{
LOGICAL  nextlevel=FALSE;

  if (nextinchain >= bitmask)
  {
    if (numbits >= lzw_max_bits)
    {
      nextlevel = TRUE;
    }
  }

  return(nextlevel);
}


static LOGICAL UpdateLevel(void)
{
LOGICAL  retval=FALSE;

  if (nextinchain >= bitmask)
  {
    numbits += 1;
    if (numbits > lzw_max_bits)
    {
      Clear();
      retval = TRUE;
    }
    else
    {
      bitmask = (1 << numbits) - 1;
    }
  }

  return(retval);
}


static int GetValue(UCHAR **pinbits)
{
UCHAR  *inbits;
UINT32 tmp;
int   ret;

  inbits = *pinbits;
  while (validbits < numbits)
  {
    tmp = revbyte[*inbits++];
    tmp <<= validbits;
    workbits |= tmp;
    validbits += 8;
  }

  ret = ReverseWord((UINT16)(workbits & bitmask)) >> (16 - numbits);
  workbits >>= numbits;
  validbits -= numbits;

  *pinbits = inbits;
  return(ret);
}


static void PutValue(UCHAR **poutbits, int code)
{
UCHAR  *outbits;
UINT32 tmp;

  outbits = (UCHAR*)*poutbits;
  tmp = ReverseWord((UINT16)(code & bitmask)) >> (16 - numbits);
  tmp <<= validbits;
  workbits |= tmp;
  validbits += numbits;

  while (validbits >= 8)
  {
    *outbits++ = revbyte[(UCHAR)workbits];
    workbits >>= 8;
    validbits -= 8;
  }

  *poutbits = (UCHAR*)outbits;
}


static void EmptyValue(UCHAR **poutbits)
{
UCHAR  *outbits;

  outbits = (UCHAR*)*poutbits;

  if (validbits)
  {
    *outbits++ = ReverseByte((UCHAR)workbits);
  }

  *poutbits = (UCHAR*)outbits;
}


static int Decode(UCHAR *poutbytes, int code)
{
UINT16  newcode;
int   len=0;

  newcode = (UINT16)chain[code];
  if (newcode > 255)
  {
    len = Decode(poutbytes, newcode);
    poutbytes[len] = (UCHAR)(chain[code] >> 16);
    return(len + 1);
  }

  poutbytes[len++] = (UCHAR)newcode;
  if (code >= LZW_CHAIN_START)
  {
    poutbytes[len++] = (UCHAR)(chain[code] >> 16);
  }

  return(len);
}


LOGICAL fiolzwInit(int depth)
{
int   i;

  if (depth < 9 || depth > 16)
  {
    return(FALSE);
  }

  lzw_max_bits      = depth;
  lzw_chain_length  = (1 << lzw_max_bits);
#ifdef USE_PRIMES
  lzw_max_mask      = primes[depth-9];
#else
  lzw_max_mask      = lzw_chain_length - 1;
#endif
  lzw_chain_block   = (lzw_chain_length * sizeof(UINT32));

  if (chain == NULL)
  {
    chain = (UINT32*)memAlloc(lzw_chain_block);
    if (chain == NULL)
    {
      return(FALSE);
    }
  }

  if (codekey == NULL)
  {
    codekey = (UINT16*)memAlloc(lzw_chain_block/2);
    if (codekey == NULL)
    {
      memFree(chain);
      chain = NULL;
      return(FALSE);
    }
  }

  if (highkey == NULL)
  {
    highkey = (UINT16*)memAlloc(lzw_chain_block/2);
    if (highkey == NULL)
    {
      memFree(chain);
      chain = NULL;
      memFree(codekey);
      codekey = NULL;
      return(FALSE);
    }
  }

  for (i=0 ; i < 256 ; i++)
  {
    revbyte[i] = ReverseByte((UCHAR)i);
  }

  return(TRUE);
}


void fiolzwKill(void)
{
  if (chain != NULL)
  {
    memFree(chain);
    chain = NULL;
  }

  if (codekey != NULL)
  {
    memFree(codekey);
    codekey = NULL;
  }

  if (highkey != NULL)
  {
    memFree(highkey);
    highkey = NULL;
  }
}


LOGICAL fiolzwDecode(UCHAR **poutbytes, UCHAR *pinbytes, int *lenbytes)
{
LOGICAL  retval=TRUE;
UCHAR  *hold, *outbytes, *inbytes;
int   choff, lastoff, numbytes, len;
UINT32 dhold=0x01000000;

  if (!Clear())
  {
    return(FALSE);
  }

  validbits = 0;
  workbits = 0;
  outbytes = *poutbytes;
  numbytes = 0;
  inbytes = pinbytes;
  hold = (UCHAR*)&dhold;
  do
  {
    choff = GetValue(&inbytes);
    if (choff > nextinchain)
    {
      Write_Debug("Error decrypting LZW data");
      retval = FALSE;
      break;  // ERROR!!!
    }

    if (choff == LZW_CHAIN_EOI)
    {
      ////Write_Debug("EOI processed");
      break;
    }

    if (choff == LZW_CHAIN_CLEAR)
    {
      Clear();
      choff = GetValue(&inbytes);
      if (choff > nextinchain)
      {
        Write_Debug("Error decrypting LZW data");
        retval = FALSE;
        break;  // ERROR!!!
      }

      if (choff == LZW_CHAIN_EOI)
      {
        Write_Debug("EOI processed");
        break;
      }

      len = Decode(outbytes, choff);
      numbytes += len;
      outbytes += len;

      lastoff = choff;
    }
    else
    {
      if (chain[choff])
      {
        len = Decode(outbytes, choff);
      }
      else
      {
        len = Decode(outbytes, lastoff);
        outbytes[len++] = *outbytes;
      }

      numbytes += len;

      hold[0] = lastoff & 0xff;
      hold[1] = lastoff >> 8;
      hold[2] = *outbytes;

      AddChain(dhold);
      lastoff = choff;
      outbytes += len;

      if (!CheckNextLevel())
      {
        UpdateLevel();
      }
    }
  }
  while (1);

  *lenbytes = outbytes - *poutbytes;
  *poutbytes = outbytes;

  return(retval);
}


LOGICAL fiolzwEncode(UCHAR **poutbytes, UCHAR *pinbytes, int *lenbytes)
{
UCHAR  *hold, *outbytes, *inbytes, ch;
int   i, length, choff, lastoff, holdoff;
UINT32 dhold=0x01000000;
int   numinbytes;

  if (!Clear())
  {
    return(FALSE);
  }

  validbits = 0;
  workbits = 0;
  outbytes = *poutbytes;
  length = *lenbytes;
  PutValue(&outbytes, LZW_CHAIN_CLEAR);
  inbytes = pinbytes;
  hold = (UCHAR*)&dhold;
  ch = *inbytes++;
  numinbytes = 1;
  lastoff = ch;
  hold[0] = ch;
  hold[1] = 0;
  holdoff = 2;
  for (i=1 ; i < length ; i++)
  {
    ch = *inbytes++;
    numinbytes += 1;
    hold[holdoff++] = ch;
    choff = FindChain(dhold);
    if (choff >= 0)
    {
      lastoff = choff;
      hold[0] = lastoff & 0xff;
      hold[1] = lastoff >> 8;
      holdoff = 2;
    }
    else
    {
      PutValue(&outbytes, lastoff);
      if (CheckNextLevel())
      {
        PutValue(&outbytes, LZW_CHAIN_CLEAR);
      }

      if (!UpdateLevel())
      {
        AddChain(dhold);
      }

      lastoff = ch;
      hold[0] = lastoff & 0xff;
      hold[1] = lastoff >> 8;
      holdoff = 2;
    }
  }

  if (choff >= 0)
  {
    PutValue(&outbytes, choff);
  }

  PutValue(&outbytes, LZW_CHAIN_EOI);
  EmptyValue(&outbytes);

  *lenbytes = outbytes - *poutbytes;
  *poutbytes = outbytes;

  return(TRUE);
}

#ifdef __cplusplus
}
#endif
