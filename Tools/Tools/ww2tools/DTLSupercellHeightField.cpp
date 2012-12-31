#include "DTLSupercellHeightField.h"

template<typename T> 
void GetOctetValues(const int16_t* sc, int ox, int oy, T h[3][3]);

/************************************************************************************************
	DTLSupercellHeightField
*************************************************************************************************/

DTLSupercellHeightField::DTLSupercellHeightField(int sx, int sy): _sx(sx), _sy(sy)
{
	zcnt = 0;
	normcnt = 0;
}

DTLSupercellHeightField::DTLSupercellHeightField(): _sx(0), _sy(0) 
{ 
	zcnt = 0; 
	normcnt = 0; 
}


void DTLSupercellHeightField::GetOctetHeights(int ox, int oy, float h[3][3]) const
{
	if(ox > -1 && oy > -1 && ox < SDM_SUPERCELL_OCTETS_X && oy < SDM_SUPERCELL_OCTETS_Y)
		GetOctetValues(z, ox, oy, h);
}

void DTLSupercellHeightField::GetOctetNormals(int ox, int oy, osg::Vec3f n[3][3]) const
{
	if(ox > -1 && oy > -1 && ox < SDM_SUPERCELL_OCTETS_X && oy < SDM_SUPERCELL_OCTETS_Y)
	{
		int16_t in[3][3];
		GetOctetValues(norm, ox, oy, in);
		
		int16_t* inp = (int16_t*)in;
		osg::Vec3f* np = (osg::Vec3f*)n;
		for(int i = 0; i < 9; i++, np++, inp++)
		{
			np->x() = (((*inp & SUPERCELL_NORMAL_MASK_X) << 0 ) >> 11) / SUPERCELL_NORMAL_MAG_X;
			np->y() = (((*inp & SUPERCELL_NORMAL_MASK_Y) << 5 ) >> 11) / SUPERCELL_NORMAL_MAG_Y;
			np->z() = (((*inp & SUPERCELL_NORMAL_MASK_Z) << 10) >> 10) / SUPERCELL_NORMAL_MAG_Z;
			if(np->x() == 0 && np->y() == 0 && np->z() == 0) np->z() = 1.0;
			np->normalize();
		}
	}
}

template<typename T>
void GetOctetValues(const int16_t* sc, int ox, int oy, T h[3][3])
{
	int vx = ox*2;
	int vy = (SDM_SUPERCELL_OCTETS_Y-1-oy)*2;

	h[0][0] = sc[SDM_SUPERCELL_SIZE_X*(vy+0)+(vx+0)];
	h[1][0] = sc[SDM_SUPERCELL_SIZE_X*(vy+0)+(vx+1)];
	h[2][0] = sc[SDM_SUPERCELL_SIZE_X*(vy+0)+(vx+2)];

	h[0][1] = sc[SDM_SUPERCELL_SIZE_X*(vy+1)+(vx+0)];
	h[1][1] = sc[SDM_SUPERCELL_SIZE_X*(vy+1)+(vx+1)];
	h[2][1] = sc[SDM_SUPERCELL_SIZE_X*(vy+1)+(vx+2)];

	h[0][2] = sc[SDM_SUPERCELL_SIZE_X*(vy+2)+(vx+0)];
	h[1][2] = sc[SDM_SUPERCELL_SIZE_X*(vy+2)+(vx+1)];
	h[2][2] = sc[SDM_SUPERCELL_SIZE_X*(vy+2)+(vx+2)];
}