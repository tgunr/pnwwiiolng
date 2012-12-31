#pragma once

struct sdm_terrain_info
{
	UINT16	        octet;			// octet number
	UINT16	        octnum;			// original octet number
	UINT8			tonum;			// original texture object number
	UINT8			txndx;			// texture name index
	UINT8			ortdrg;			// orientation/drag LSN/MSN
	UINT8			ruftrn;			// roughness/terrain LSN/MSN
};
