#pragma once

struct DrawIcons
{
	int texid;
	int numIcons;
	piUV* uv;
};

using drawIconRef = struct DrawIcons*;

drawIconRef drawIconLoad(const char* iconFile);
void drawIconKill(drawIconRef);

void drawIcon2d(drawIconRef, int iconID, float x, float y);
void drawIcon3d(drawIconRef, int iconID, piVECTOR verts[4], unsigned int color);

void drawIconGetUV(drawIconRef, int iconID, float* u1, float* v1, float* u2, float* v2);
int drawIconGetTexId(drawIconRef);
