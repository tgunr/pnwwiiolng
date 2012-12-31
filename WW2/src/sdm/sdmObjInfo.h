#pragma once

struct SDM_ObjInfo
{
	piMATRIX	transform;
	piVECTOR	pos;			// object position
	char		objname[32];	// object name in pool
	UINT8		txndx;			// texture object index
	UINT8		ortdrg;			// orientation/drag LSN/MSN
	UINT8		ruftrn;			// roughness/terrain LSN/MSN
	UINT8		flags;			//
};
