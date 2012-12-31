#pragma once
// Created: Tue 03 Feb 2004, 11:53:00
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Receive client packet describing hardware information.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "include/cpuFeatures.h"

#pragma pack(push, 1)

#define MAX_INFO_REQ_TEXT 500
struct COMPUTER_INFO_REQ
{
	UINT16 size;
	char text[MAX_INFO_REQ_TEXT+1];
} ;

struct PIXEL_FORMAT_DESC
{
	UCHAR	formatNumber;

	UINT16  nSize;
	UINT16  nVersion;

	UINT32 dwFlags;

	UCHAR  iPixelType;
	UCHAR  cColorBits;

	UCHAR  cRedBits;
	UCHAR  cGreenBits;
	UCHAR  cBlueBits;
	UCHAR  cAlphaBits;

	UCHAR  cAccumBits;
	UCHAR  cAccumRedBits;
	UCHAR  cAccumGreenBits;
	UCHAR  cAccumBlueBits;
	UCHAR  cAccumAlphaBits;

	UCHAR  cDepthBits;
	UCHAR  cStencilBits;

} ;

#define MAX_OGL_EXTENSIONS 1024
#define MAX_OGL_EXTN_SPACE (MAX_OGL_EXTENSIONS>>3)

struct PLAY_OGL_EXTENSIONS
{
	UINT16	numExtensions;
	char	bitmap[MAX_OGL_EXTN_SPACE];
} ;


struct PLAY_COMP_TEXTURE
{
	UINT32 value;
} ;

#define MAX_COMPRESSED_TEXTURE 8

struct PLAY_COMP_TEXTURES
{
	UINT16				num;
	PLAY_COMP_TEXTURE	desc[MAX_COMPRESSED_TEXTURE+1];
} ;

#define MAX_OGL_VENDOR_NAME 16
#define MAX_OGL_CARD_NAME 48

struct PLAY_OGL
{
	char				vendor[MAX_OGL_VENDOR_NAME+1];
	char				cardName[MAX_OGL_CARD_NAME+1];
	int					oglVersion;
	PLAY_OGL_EXTENSIONS	extension;
	PIXEL_FORMAT_DESC	pixelFormat;
	PLAY_COMP_TEXTURES	compTextures;
} ;

struct PLAY_DX_VERS
{
	LOGICAL	isInstalled;
	UINT32	hiVersion;
	UINT32	lowVersion;
} ;

struct PLAY_DX
{
	int				driverVersion; // not sent yet...
	PLAY_DX_VERS	draw;
	PLAY_DX_VERS	d3d;
} ;

struct PLAY_VIDEO
{
	PLAY_OGL	ogl;
	PLAY_DX		dx;
} ;

typedef UINT32   WIN_MMVERSION_;  /* major (high byte), minor (low byte) */

#define MAX_MIX_NAME 40

struct PLAY_MIXER_CAPS
{
	SINT16			wMid;                   /* manufacturer id */
	SINT16			wPid;                   /* product id */
	WIN_MMVERSION_	vDriverVersion;         /* version of the driver */
	SCHAR			szPname[MAX_MIX_NAME+1]; /* product name */
	SINT32			fdwSupport;             /* misc. support bits */
	SINT32			cDestinations;          /* count of destinations */
} ;

struct PLAY_SOUND
{
	LOGICAL			isValid;
	PLAY_MIXER_CAPS mixer;
} ;

struct PLAY_MEM
{
	SINT32 dwLength;
	SINT32 dwMemoryLoad;
	SINT32 dwTotalPhys;
	SINT32 dwAvailPhys;
	SINT32 dwTotalPageFile;
	SINT32 dwAvailPageFile;
	SINT32 dwTotalVirtual;
	SINT32 dwAvailVirtual;
} ;

struct PLAY_COMPUTER_INFO
{
	PLAY_VIDEO			video;
	PLAY_SOUND			sound;
	CPUFeatures			cpu;
	PLAY_MEM			memory;

	UINT32				version;

	UINT32 CurrentVersion() const { return 1 ; }
} ;

#pragma pack(pop)

namespace PgInfo
{
	enum { CallbackX = 199 };
}

enum
{
	PLAY_INFO_REQ_RECV_Y,
	PLAY_INFO_SEND_Y
};
