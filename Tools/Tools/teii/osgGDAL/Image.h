#ifndef OSGGDALIMAGE_H
#define OSGGDALIMAGE_H

#include <osg/Image>

#include "osgGDAL/Dataset.h"

namespace osgGDAL
{

/************************************************************************************************
	osgGDALImage
*************************************************************************************************/

class  Image : public osg::Referenced
{
public:
	Image();
	Image(const char* name, int mode=GA_ReadOnly);
	Image(GDALDataset*);
	Image(osgGDAL::Dataset*);
	virtual ~Image();
	
	void SetDataset(osgGDAL::Dataset* d) {_dataset = d;}

	void SetBands(int r, int g=-1, int b=-1, int a=-1) { _rband = r; _gband = g; _bband = b; _aband = a; }
	osgGDAL::Dataset* Dataset()  { return _dataset.get(); }
	PixelLonLatTransform const& Transform() const { return _dataset->Transform(); }
	
	osg::Image* CreateReducedImage(double lonlatwin[4], int reducelevel, int format=GL_RGB);
	osg::Image* CreateReducedImage(int srcwin[4], int reducelevel, int format=GL_RGB);
	osg::Image* CreateImage(double lonlatwin[4], int format=GL_RGB, int width=0, int height=0);
	osg::Image* CreateImage(int srcwin[4], int format=GL_RGB, int width=0, int height=0);
	void CopySubImage(osg::Image* dstimage, int srcwin[4], int dstwin[4]);
	

protected:
	osg::ref_ptr<osgGDAL::Dataset> _dataset;
	int _rband, _gband, _bband, _aband;
};

}
#endif // OSGGDALIMAGE_H