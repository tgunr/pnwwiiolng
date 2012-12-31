#ifndef DTLOctetHeightField_H
#define DTLOctetHeightField_H

#include <osg/Vec3d>
#include <osg/Vec3f>

#include "sdm.h"

/************************************************************************************************
	DTLOctetHeightField
*************************************************************************************************/

class DTLOctetHeightField
{
public:
	DTLOctetHeightField(int sx, int sy, int ox, int oy, const float h[3][3], const osg::Vec3f n[3][3]);
	
	float TopLeftH() const { return  _h[0][0]; }
	float TopCenterH() const { return _h[1][0]; }
	float TopRightH() const { return _h[2][0]; }

	float CenterLeftH() const { return _h[0][1]; }
	float CenterH() const { return _h[1][1]; }
	float CenterRightH() const { return _h[2][1]; }

	float BottomLeftH() const { return _h[0][2]; }
	float BottomCenterH() const { return _h[1][2]; }
	float BottomRightH() const { return _h[2][2]; }

	osg::Vec3d TopLeft() const { return _origin + osg::Vec3d(-SDM_OCTET_SIZE_2, SDM_OCTET_SIZE_2, TopLeftH()); }
	osg::Vec3d TopCenter() const { return _origin + osg::Vec3d(0, SDM_OCTET_SIZE_2, TopCenterH()); }
	osg::Vec3d TopRight() const { return _origin + osg::Vec3d(SDM_OCTET_SIZE_2, SDM_OCTET_SIZE_2, TopRightH()); }

	osg::Vec3d CenterLeft() const { return _origin + osg::Vec3d(-SDM_OCTET_SIZE_2, 0, CenterLeftH()); }
	osg::Vec3d Center() const { return _origin + osg::Vec3d(0, 0, CenterH()); }
	osg::Vec3d CenterRight() const { return _origin + osg::Vec3d(SDM_OCTET_SIZE_2, 0, CenterRightH()); }

	osg::Vec3d BottomLeft() const { return _origin + osg::Vec3d(-SDM_OCTET_SIZE_2, -SDM_OCTET_SIZE_2, BottomLeftH()); }
	osg::Vec3d BottomCenter() const { return _origin + osg::Vec3d(0, -SDM_OCTET_SIZE_2, BottomCenterH()); }
	osg::Vec3d BottomRight() const { return _origin + osg::Vec3d(SDM_OCTET_SIZE_2, -SDM_OCTET_SIZE_2, BottomRightH()); }

	osg::Vec3f TopLeftNormal() const { return _n[0][0]; }
	osg::Vec3f TopCenterNormal() const { return _n[1][0]; }
	osg::Vec3f TopRightNormal() const { return _n[2][0]; }

	osg::Vec3f CenterLeftNormal() const { return _n[0][1]; }
	osg::Vec3f CenterNormal() const { return _n[1][1]; }
	osg::Vec3f CenterRightNormal() const { return _n[2][1]; }

	osg::Vec3f BottomLeftNormal() const { return _n[0][2]; }
	osg::Vec3f BottomCenterNormal() const { return _n[1][2]; }
	osg::Vec3f BottomRightNormal() const { return _n[2][2]; }
	
	double HeightAt(double x, double y) const;
	osg::Vec3f SurfaceNormalAt(double x, double y) const;

	osg::Vec3d PositionAt(double x, double y) const { return osg::Vec3d(x,y,HeightAt(x,y)); }
	template <typename T>
	osg::Vec3d PositionAt(T const& v) const { return PositionAt(v.x(),v.y()); }
		
	float const* HeightArray() const { return (float const*)_h;}

protected:
	int _sx, _sy, _ox, _oy;
	float _h[3][3];
	osg::Vec3f _n[3][3];
	osg::Vec3d _origin; // the center of the octet
	
	struct PlaneEquation
	{
		void Define(osg::Vec3d const&, osg::Vec3d const&, osg::Vec3d const&);
		double Z(double x, double y) const { return -(_d + _n.x()*x + _n.y()*y) / _n.z(); }
		double _d;
		osg::Vec3d _n;
	};
	
	PlaneEquation _plane[9];
};

#endif // DTLOctetHeightField_H