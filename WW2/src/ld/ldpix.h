#pragma once

//#define	LOAD_TIMER			1

#define ALPHA_FILE_FOUND    -20

// if the total texture memory is lower than this number, it forces reduction on the top level
// mipmap whenever a texture is loaded

#define REDUCE_TEXTURE_THRESHOLD	10000000

#define LDPIX_NONE                  0x00000000
#define LDPIX_MIPMAP				0x00000001
#define LDPIX_NO_DXT				0x00000002
#define LDPIX_NO_TEXTURE_GENERATE   0x00000004
#define LDPIX_NO_FLIP               0x00000008
#define LDPIX_FLIP                  0x00000010
#define LDPIX_FLIPHORIZONTAL        0x00000020
#define LDPIX_CREATE_SURFACE_DATA   0x00000040
#define LDPIX_SET_FONT_PARMS        0x00000080
#define LDPIX_CLAMP                 0x00000100
#define LDPIX_CLAMP_TO_EDGE         0x00000200
#define LDPIX_CLAMP_TO_BORDER       0x00000400

extern int ldpixLoadImageFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 Flags);
extern LOGICAL ldpixCopyBitsToSurface(UCHAR* bits, int bitsPitch, int width, int height, piSURFACEDESC* lppisd, UINT32 TransparentColor);
extern LOGICAL ldpixCopySurfaceToBits(piSURFACEDESC* lppisd, UCHAR* bits);
extern LOGICAL ldpixCopyAlphaToSurface(UCHAR* bits, int bitsPitch, int width, int height, piSURFACEDESC* lppisd);
extern LOGICAL ldpixSetAlphaToSurface(int AlphaPercent, piSURFACEDESC* lppisd);
extern LOGICAL ldpixCopyBitsToSubsurface(UCHAR* bits, piRGBQUAD* rgb, int bwidth, int bheight, int row, int col, piSURFACEDESC* lppisd, UINT32 TransparentColor);
extern void* ldpixConvert8To24(int width, int height, piRGBQUAD* pal, UCHAR* inbits);
extern void ldpixInvertImage(int bits, int width, int height, UCHAR* inbits);
extern void ldpixScaleImage(piPIXELFORMAT* pixformat, int inwidth, int inheight, int inpitch, UCHAR* inbits,
						 int outwidth, int outheight, int outpitch, UCHAR* outbits);
extern LOGICAL ldpixCopySurfaceToSurface(piSURFACEDESC* lppisd1, piSURFACEDESC* lppisd2);

extern void ldpixSetTextureReduction(LOGICAL);
extern LOGICAL ldpixGetTextureReduction();

