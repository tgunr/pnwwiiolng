#include "osgGDAL/Image.h"

/************************************************************************************************
	Image
*************************************************************************************************/

osgGDAL::Image::Image()
{
	_rband = _gband = _bband = _aband = -1; 
	SetDataset(0);
}

osgGDAL::Image::Image(GDALDataset* d)
{
	_rband = _gband = _bband = _aband = -1; 
	SetDataset(new osgGDAL::Dataset(d));
}

osgGDAL::Image::Image(const char* name, int mode)
{
	_rband = _gband = _bband = _aband = -1; 
	SetDataset(new osgGDAL::Dataset((GDALDataset*)GDALOpen(name, (GDALAccess)mode)));
}

osgGDAL::Image::Image(osgGDAL::Dataset* d)
{ 
	_rband = _gband = _bband = _aband = -1; 
	SetDataset(d);
}

osgGDAL::Image::~Image()
{
}

/************************************************************************************************
	CreateReducedImage
*************************************************************************************************/

osg::Image* osgGDAL::Image::CreateReducedImage(double lonlatwin[4], int reducelevel, int format)
{
	int srcwin[4];		
	_dataset->Transform().LonLatToPixelWindow(lonlatwin, srcwin);
	
	return CreateReducedImage(srcwin, reducelevel, format);
}

/************************************************************************************************
	CreateReducedImage
*************************************************************************************************/

osg::Image* osgGDAL::Image::CreateReducedImage(int srcwin[4], int reducelevel, int format)
{
	int reduce = (1<<reducelevel);

	int srcwidth  = srcwin[2] - srcwin[0];
	int srcheight = srcwin[3] - srcwin[1];
	
	if(srcwidth < 0 || srcheight < 0) return 0;
	
	int dstwidth  = srcwidth  / reduce;
	int dstheight = srcheight / reduce;
	
	int dstwin[4] = { 0,0, dstwidth,dstheight };

	osg::Image* image = new osg::Image;
	image->allocateImage(dstwidth, dstheight, 1, format, GL_UNSIGNED_BYTE);

	CopySubImage(image, srcwin, dstwin);
	
	return image;
}

/************************************************************************************************
	CreateImage
*************************************************************************************************/

osg::Image* osgGDAL::Image::CreateImage(double lonlatwin[4], int format, int width, int height)
{
	int srcwin[4];
	_dataset->Transform().LonLatToPixelWindow(lonlatwin, srcwin);
	return CreateImage(srcwin, format, width, height);
}

/************************************************************************************************
	CreateImage
*************************************************************************************************/

osg::Image* osgGDAL::Image::CreateImage(int srcwin[4], int format, int width, int height)
{
	if(width == 0)
		width  = srcwin[2] - srcwin[0];
	if(height == 0)
		height = srcwin[3] - srcwin[1];
		
	osg::Image* image = new osg::Image;
	image->allocateImage(width, height, 1, format, GL_UNSIGNED_BYTE);

	int dstwin[4] = {0,0,width,height};
	CopySubImage(image, srcwin, dstwin);
	
	return image;
}

/************************************************************************************************
	CopySubImage
*************************************************************************************************/

void osgGDAL::Image::CopySubImage(osg::Image* image, int srcwin[4], int dstwin[4])
{
	// bounds checking here...
	
	int format = image->getPixelFormat();
	int x = srcwin[0];
	int y = srcwin[1];
	
	int srcwidth  = srcwin[2] - srcwin[0];
	int srcheight = srcwin[3] - srcwin[1];
	
	int dstwidth  = dstwin[2] - dstwin[0];
	int dstheight = dstwin[3] - dstwin[1];
	
	unsigned char* data = image->data(dstwin[0], dstwin[1]);
	int rowsize = image->getRowSizeInBytes();
	int bands[1];
	int bpp = 0;
	
	if (format == GL_RGB) 
		bpp = 3;
	else if(format == GL_RGBA)
		bpp = 4;
	else if(format == GL_INTENSITY)
		bpp = 1;
		
	if(bpp == 0) return;
	
	if(_rband > 0)
	{
		bands[0] = _rband;
		_dataset->GetDataset()->RasterIO(GF_Read, x,y, srcwidth,srcheight, 
							data, dstwidth,dstheight,
							GDT_Byte, 1, bands, bpp, rowsize, 0 );	
	}
	
	if(_gband > 0 && bpp > 1)
	{
		bands[0] = _gband;
		_dataset->GetDataset()->RasterIO(GF_Read, x,y, srcwidth,srcheight, 
							data+1, dstwidth,dstheight,
							GDT_Byte, 1, bands, bpp, rowsize, 0 );	
	}
	
	if(_bband  > 0 && bpp > 2)
	{
		bands[0] = _bband;
		_dataset->GetDataset()->RasterIO(GF_Read, x,y, srcwidth,srcheight, 
							data+2, dstwidth,dstheight,
							GDT_Byte, 1, bands, bpp, rowsize, 0 );
	}
	
	if(_aband > 0 && bpp > 3)
	{
		bands[0] = _aband;
		_dataset->GetDataset()->RasterIO(GF_Read, x,y, srcwidth,srcheight, 
							data+3, dstwidth,dstheight,
							GDT_Byte, 1, bands, bpp, rowsize, 0 );

	}
}