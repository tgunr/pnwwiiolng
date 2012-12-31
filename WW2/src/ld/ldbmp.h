#pragma once

extern int ldbmpLoadDIBToSurface(piBITMAPINFO* bi, UINT32 TransparentColor, LOGICAL WantMipmap);
extern LOGICAL ldbmpLoadAlphaToSurface(int texid, int miplevel, UCHAR* bits, int width, int height, UINT32 bitspitch);
extern int ldbmpLoadBMPResourceToSurface(/*LPSTR*/char * Resource, UINT16 TransparentColor);
extern LOGICAL ldbmpLoadBMPFile(const char* Filename, piBITMAPINFO** bmi);
extern int ldbmpLoadBMPFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 WantFlags);
extern LOGICAL ldbmpLoadBMPAlphaFileToSurface(const char* Filename, int texid, int miplevel);

