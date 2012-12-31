#pragma once

#define NUMFLAREGFX   6
#define NUMFLAREFX    7

struct FLARE
{
	int       gfx;
	float		  scale;
	float		  loc;              // postion along axis
	float     red, green, blue; // amount of natural color in each flare

} ;


extern LOGICAL oblfInit(void);
extern void  oblfUpdate(void);
extern void oblfKill(void);
