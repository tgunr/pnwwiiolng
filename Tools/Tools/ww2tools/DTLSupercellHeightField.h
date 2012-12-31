#ifndef DTLSupercellHeightField_H
#define DTLSupercellHeightField_H

#include <osg/Vec3f>
#include "sdm.h"

/************************************************************************************************
	DTLSupercellHeightField
*************************************************************************************************/

class DTLSupercellHeightField : public sdm_dtl_data
{
public:
	DTLSupercellHeightField(int sx, int sy);
	DTLSupercellHeightField();

	void GetOctetHeights(int ox, int oy, float z[3][3]) const;	
	void GetOctetNormals(int ox, int oy, osg::Vec3f n[3][3]) const;	

	bool InSupercell(double x, double y) const;
	
	void SetSupercell(int sx, int sy) { _sx = sx; _sy = sy; }
	
	int SupercellX() const { return _sx; }
	int SupercellY() const { return _sy; }
	
protected:
	int _sx, _sy;
};

#endif // DTLSupercellHeightField_H