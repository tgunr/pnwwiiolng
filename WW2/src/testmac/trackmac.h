/*
 *  trackmac.h
 *  WW2OL
 *
 *  Created by Andrew Wulf on 11/30/10.
 *  Copyright 2010 Idle Diversions. All rights reserved.
 *
 */
#ifdef TESTMAC

#ifdef __cplusplus
#define EXTERN__C extern "C"
#else
#define EXTERN__C extern
#endif

EXTERN__C void __cyg_profile_func_enter (void* this_fn, void* call_site);
EXTERN__C void __cyg_profile_func_exit  (void* this_fn, void* call_site);
EXTERN__C void getStack(char* buffer, int maxsize );
extern int TrackActive;

#define ARRAY_SIZE 500

#undef EXTERN__C

#endif
