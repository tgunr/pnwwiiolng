// fiolzw.h
//


#ifndef fiolzw_HEADER

	#define fiolzw_HEADER

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


#ifdef __cplusplus
extern "C"
{
#endif

  // Temporary
  void fiolzwTest(void);

  LOGICAL fiolzwInit(int depth);
  void fiolzwKill(void);
  LOGICAL fiolzwDecode(UCHAR **outbits, UCHAR *inbits, int *length);
  LOGICAL fiolzwEncode(UCHAR **outbits, UCHAR *inbits, int *length);


#ifdef __cplusplus
}
#endif

#endif
