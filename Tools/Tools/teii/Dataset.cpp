#include "osgGDAL/Dataset.h"

using namespace osgGDAL;

/************************************************************************************************
	Dataset
*************************************************************************************************/
 
osgGDAL::Dataset::Dataset(): _dataset(0)
{
}

osgGDAL::Dataset::Dataset(const char* name, int mode): _dataset(0)
{
	SetDataset((GDALDataset*)GDALOpen(name, (GDALAccess)mode));
}

osgGDAL::Dataset::Dataset(GDALDataset* d) : _dataset(0)
{
	SetDataset(d); 
}

osgGDAL::Dataset::~Dataset()
{
	if(_dataset)
		GDALClose(_dataset);
}

/************************************************************************************************
	SetDataset
*************************************************************************************************/

void osgGDAL::Dataset::SetDataset(GDALDataset* d)
{
	if(_dataset != d)
	{
		if(_dataset)
			GDALClose(_dataset);

		_dataset = d;
		
		if(_dataset)
			_dataset->Reference();
	}
	_lonlattransform.SetDataset(d);
}

/************************************************************************************************
	osgGDAL::PixelLonLatTransform
*************************************************************************************************/

osgGDAL::PixelLonLatTransform::PixelLonLatTransform()
{
	memset(_a, 0x0, sizeof(_a));
	memset(_b, 0x0, sizeof(_b));
	_projected = false;
}

/************************************************************************************************
	SetDataset
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::SetDataset(GDALDataset* dataset)
{
	dataset->GetGeoTransform(_a);

	double d = (_a[1] * _a[5] - _a[2] * _a[4]);
	_b[0] = (_a[2]*_a[3] - _a[5]*_a[0]) / d;
	_b[1] =  _a[5] / d;
	_b[2] = -_a[2] / d;
	_b[3] = (_a[4]*_a[0] - _a[1]*_a[3]) / d;
	_b[4] = -_a[4] / d;
	_b[5] =  _a[1] / d;

	OGRSpatialReference srs(dataset->GetProjectionRef());
	
	_projected = srs.IsProjected();
	if(_projected)
	{
		OGRSpatialReference* latlon = srs.CloneGeogCS();

		_projtolonlat.reset(OGRCreateCoordinateTransformation(&srs, latlon));
		_lonlattoproj.reset(OGRCreateCoordinateTransformation(latlon, &srs));

		delete latlon;
	}
}

/************************************************************************************************
	PixelToLonLat
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::PixelToLonLat(int x, int y, double* lon, double* lat) const
{
	*lon = _a[0] + _a[1] * x + _a[2] * y;
	*lat = _a[3] + _a[4] * x + _a[5] * y;
	
	if(_projected)
		_projtolonlat->Transform(1, lon, lat);
}

/************************************************************************************************
	LonLatToPixel
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::LonLatToPixel(double lon, double lat, int* x, int* y) const
{
	if(_projected)
		_lonlattoproj->Transform(1, &lon, &lat);

	*x = (int)round( _b[0] + _b[1] * lon + _b[2] * lat );
	*y = (int)round( _b[3] + _b[4] * lon + _b[5] * lat );
}

/************************************************************************************************
	PixelToLonLatWindow
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::PixelToLonLatWindow(int const srcwin[4], double lonlatwin[4]) const
{
	PixelToLonLat(srcwin[0],srcwin[1], &lonlatwin[0], &lonlatwin[1]);
	PixelToLonLat(srcwin[2],srcwin[3], &lonlatwin[2], &lonlatwin[3]);
}

/************************************************************************************************
	LonLatToPixelWindow
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::LonLatToPixelWindow(double const lonlatwin[4], int srcwin[4]) const
{
	LonLatToPixel(lonlatwin[0], lonlatwin[1], &srcwin[0], &srcwin[1]);
	LonLatToPixel(lonlatwin[2], lonlatwin[3], &srcwin[2], &srcwin[3]);
}


/************************************************************************************************
	PixelFrom
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::PixelFrom(PixelLonLatTransform const& transform, 
												int srcx, int srcy, int* dstx, int* dsty) const
{
	double lon, lat;
	transform.PixelToLonLat(srcx, srcy, &lon, &lat);
	LonLatToPixel(lon, lat, dstx, dsty);
}

/************************************************************************************************
	PixelWindowFrom
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::PixelWindowFrom(PixelLonLatTransform const& transform, 
														int const srcwin[4], int dstwin[4]) const
{
	double lonlatwin[4];
	transform.PixelToLonLatWindow(srcwin,lonlatwin);
	LonLatToPixelWindow(lonlatwin, dstwin);
}

/************************************************************************************************
	SubPixelToLonLat
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::SubPixelToLonLat(double x, double y, double* lon, double* lat) const
{
	*lon = _a[0] + _a[1] * x + _a[2] * y;
	*lat = _a[3] + _a[4] * x + _a[5] * y;
	
	if(_projected)
		_projtolonlat->Transform(1, lon, lat);
}

/************************************************************************************************
	LonLatToSubPixel
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::LonLatToSubPixel(double lon, double lat, double* x, double* y) const
{
	if(_projected)
		_lonlattoproj->Transform(1, &lon, &lat);

	*x = ( _b[0] + _b[1] * lon + _b[2] * lat );
	*y = ( _b[3] + _b[4] * lon + _b[5] * lat );
}

/************************************************************************************************
	SubPixelToLonLatWindow
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::SubPixelToLonLatWindow(double const srcwin[4], double lonlatwin[4]) const
{
	SubPixelToLonLat(srcwin[0],srcwin[1], &lonlatwin[0], &lonlatwin[1]);
	SubPixelToLonLat(srcwin[2],srcwin[3], &lonlatwin[2], &lonlatwin[3]);
}

/************************************************************************************************
	LonLatToSubPixelWindow
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::LonLatToSubPixelWindow(double const lonlatwin[4], double srcwin[4]) const
{
	LonLatToSubPixel(lonlatwin[0], lonlatwin[1], &srcwin[0], &srcwin[1]);
	LonLatToSubPixel(lonlatwin[2], lonlatwin[3], &srcwin[2], &srcwin[3]);
}


/************************************************************************************************
	SubPixelFrom
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::SubPixelFrom(PixelLonLatTransform const& transform, 
												double srcx, double srcy, double* dstx, double* dsty) const
{
	double lon, lat;
	transform.SubPixelToLonLat(srcx, srcy, &lon, &lat);
	LonLatToSubPixel(lon, lat, dstx, dsty);
}

/************************************************************************************************
	SubPixelWindowFrom
*************************************************************************************************/

void osgGDAL::PixelLonLatTransform::SubPixelWindowFrom(PixelLonLatTransform const& transform, 
														double const srcwin[4], double dstwin[4]) const
{
	double lonlatwin[4];
	transform.SubPixelToLonLatWindow(srcwin,lonlatwin);
	LonLatToSubPixelWindow(lonlatwin, dstwin);
}