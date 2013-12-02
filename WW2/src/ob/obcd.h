#pragma once

#include "ob/obmain.h"

struct COLLISION
{
	struct OBJECT*	object;
	struct OBJECT*	colobj;
	LOGICAL		collide;
	SINT32		colpoly;
	piVECTOR	oldpos;
	piVECTOR	newpos;
	piVECTOR	colpos;
	piVECTOR	colnormal;
	FLOAT32		colangle;

};

struct FASTCOLLIDE
{
	struct OBJECT* CollideObject;
	struct OBJECT* TopLevelObject;
	struct trnCDTerrainObject*	TO;			// actually a trnCDTerrainObject
	UINT32 PolyNum;
	piVECTOR CollidePnt; // relative to CollideObject
	piVECTOR OldPos,NewPos; // relative to CollideObject
	FLOAT32 CollideTime;
	FLOAT32 Alpha;
	UINT32 Flags;
};


extern LOGICAL obcdCheckObjectCollision(OBJECT* lpobj, COLLISION* collision);
extern LOGICAL obcdCheck(COLLISION* collision);
extern LOGICAL obcdPlayer(piVECTOR* oldpos, piVECTOR* newpos);
extern LOGICAL obcdObject(OBJECT* object, piVECTOR* oldpos, piVECTOR* newpos);
