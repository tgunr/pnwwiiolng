#include <string.h>

#include "DTLOctetHeightField.h"

#define TERRAIN_NODATA			-9999

/************************************************************************************************
	DTLOctetHeightField
*************************************************************************************************/

DTLOctetHeightField::DTLOctetHeightField(int sx, int sy, int ox, int oy, const float h[3][3], const osg::Vec3f n[3][3])
		: _sx(sx), _sy(sy), _ox(ox), _oy(oy)
		, _origin(sx * SDM_SUPERCELL_SIZE + (ox+.5) * SDM_OCTET_SIZE
				, sy * SDM_SUPERCELL_SIZE + (oy+.5) * SDM_OCTET_SIZE, 0)

{
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			if(h[i][j] == TERRAIN_NODATA)
			{
				_h[i][j] = 0;
				_n[i][j].set(0,0,1);
			}else
			{
				_h[i][j] = h[i][j];
				_n[i][j] = n[i][j];
			}
		}
	}

	osg::Vec3d v1(-SDM_OCTET_SIZE_2,SDM_OCTET_SIZE_2,TopLeftH());
	osg::Vec3d v2(0,SDM_OCTET_SIZE_2,TopCenterH());
	osg::Vec3d v3(SDM_OCTET_SIZE_2,SDM_OCTET_SIZE_2,TopRightH());
	
	osg::Vec3d v8(-SDM_OCTET_SIZE_2,0,CenterLeftH());
	osg::Vec3d v9(0,0,CenterH());
	osg::Vec3d v4(SDM_OCTET_SIZE_2,0,CenterRightH());

	osg::Vec3d v7(-SDM_OCTET_SIZE_2,-SDM_OCTET_SIZE_2,BottomLeftH());
	osg::Vec3d v6(0,-SDM_OCTET_SIZE_2,BottomCenterH());
	osg::Vec3d v5(SDM_OCTET_SIZE_2,-SDM_OCTET_SIZE_2,BottomRightH());

	_plane[0].Define(v9, v8, v1);
	_plane[1].Define(v9, v1, v2);
	_plane[2].Define(v9, v2, v3);
	_plane[3].Define(v9, v3, v4);
	_plane[4].Define(v9, v4, v5);
	_plane[5].Define(v9, v5, v6);
	_plane[6].Define(v9, v6, v7);
	_plane[7].Define(v9, v7, v8);
}

double DTLOctetHeightField::HeightAt(double x, double y) const
{
	x -= _origin.x();
	y -= _origin.y();
	 
	int i = 0;

	if(x < 0)
	{
		if(y > 0)
			i = (-x > y) ? 0 : 1;
		else
			i = (-x > -y) ? 7 : 6;
	}else
	{
		if(y > 0)
			i = (x > y) ? 3 : 2;
		else
			i = (x > -y) ? 4 : 5;
	}
	
	
	return _plane[i].Z(x, y);
}

osg::Vec3f DTLOctetHeightField::SurfaceNormalAt(double x, double y) const
{
	x -= _origin.x();
	y -= _origin.y();
	 
	int i = 0;

	if(x < 0)
	{
		if(y > 0)
			i = (-x > y) ? 0 : 1;
		else
			i = (-x > -y) ? 7 : 6;
	}else
	{
		if(y > 0)
			i = (x > y) ? 3 : 2;
		else
			i = (x > -y) ? 4 : 5;
	}
	osg::Vec3f n = -_plane[i]._n;
	n.normalize();
	return n;
}

void DTLOctetHeightField::PlaneEquation::Define(osg::Vec3d const& v1, osg::Vec3d const& v2, osg::Vec3d const& v3)
{
	osg::Vec3d v13 = v3 - v1;
	osg::Vec3d v12 = v2 - v1;
	_n = v12 ^ v13;
	
	_d = -(_n.x()*v1.x() + _n.y()*v1.y() + _n.z()*v1.z());
}