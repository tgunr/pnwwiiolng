#pragma once

#define NUMFLAREGFX   6
#define NUMFLAREFX    7

struct FLARE
{
	int       gfx;
	float		  scale;
	float		  loc;              // postion along axis
	float     red, green, blue; // amount of natural color in each flare

};


extern LOGICAL oblfInit();
extern void  oblfUpdate();
extern void oblfKill();
