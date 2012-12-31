#ifndef OSGGDALDATASET_H
#define OSGGDALDATASET_H

#include <memory>

#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <osg/ref_ptr>
#include <osg/Referenced>

/************************************************************************************************
	osgGDAL Dataset
*************************************************************************************************/

namespace osgGDAL
{

class PixelLonLatTransform
{
public:
	PixelLonLatTransform(GDALDataset* d) { SetDataset(d); }
	PixelLonLatTransform();

	void SetDataset(GDALDataset*);

	void PixelToLonLat(int x, int y, double* lon, double* lat) const;
	void LonLatToPixel(double lon, double lat, int* x, int* y) const;

	void PixelToLonLatWindow(int const srcwin[4], double lonlatwin[4]) const;
	void LonLatToPixelWindow(double const lonlatwin[4], int srcwin[4]) const;

	void PixelFrom(PixelLonLatTransform const&, int srcx, int srcy, int* dstx, int* dsty) const;	
	void PixelWindowFrom(PixelLonLatTransform const&, int const srcwin[4], int dstwin[4]) const;

	void SubPixelToLonLat(double x, double y, double* lon, double* lat) const;
	void LonLatToSubPixel(double lon, double lat, double* x, double* y) const;

	void SubPixelToLonLatWindow(double const srcwin[4], double lonlatwin[4]) const;
	void LonLatToSubPixelWindow(double const lonlatwin[4], double srcwin[4]) const;

	void SubPixelFrom(PixelLonLatTransform const&, double srcx, double srcy, double* dstx, double* dsty) const;	
	void SubPixelWindowFrom(PixelLonLatTransform const&, double const srcwin[4], double dstwin[4]) const;

protected:
	std::auto_ptr<OGRCoordinateTransformation> _lonlattoproj, _projtolonlat;
	double _a[6], _b[6];

	bool _projected;
};

class Dataset : public osg::Referenced
{
public:
	Dataset();
	Dataset(const char* name, int mode=GA_ReadOnly);
	Dataset(GDALDataset* d);
	virtual ~Dataset();
	
	void SetDataset(GDALDataset* d);

	GDALDataset* GetDataset() const { return _dataset; }
	PixelLonLatTransform const& Transform() const { return _lonlattransform; }

protected:
	GDALDataset* _dataset;
	PixelLonLatTransform _lonlattransform;
};

};

#endif // OSGGDALDATASET_H