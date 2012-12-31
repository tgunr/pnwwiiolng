#pragma once

struct DrawIcons
{
	int texid;
	int numIcons;
	piUV* uv;
} ;

typedef struct DrawIcons* drawIconRef;

extern drawIconRef drawIconLoad(const char* iconFile);
extern void drawIconKill(drawIconRef);

extern void drawIcon2d(drawIconRef, int iconID, float x, float y);
extern void drawIcon3d(drawIconRef, int iconID, piVECTOR verts[4], unsigned int color);

extern void drawIconGetUV(drawIconRef, int iconID, float* u1, float* v1, float* u2, float* v2);
extern int drawIconGetTexId(drawIconRef);
