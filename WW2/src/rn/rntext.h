#pragma once

#include "chat/chatHandle.h"
#include "phui/FTFontReplacement.h"

extern int rntextGetHeight(int Font);
extern int rntextInit(void);
extern void rntextKill(void);
extern SINT32 rntextGetStringWidth(const WCHAR*, SINT32);
extern LOGICAL rntext3DString(const WCHAR* pszText, UINT32 uColor, SINT32 iFont, struct piLVERTEX* plVertices, SINT32 iNumVertices);
extern SINT32 rntextGetTexId(SINT32 iFont);

#define MAX_CHAR_LAYOUT		Chat::WIDE_HANDLE_CHARACTERS_SUPPORTED

struct CHARLAYOUT
{
	int   x, y, width;
	float u1, u2, v1, v2;
};

struct FONTLAYOUT
{
	int   AvgWidth;
	int   Height;
	CHARLAYOUT  CharLayout[MAX_CHAR_LAYOUT];
};
