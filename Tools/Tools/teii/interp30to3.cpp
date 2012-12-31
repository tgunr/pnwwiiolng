#include <math.h>
#include <stdio.h>
#include <memory>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"

class PixelLonLatTransform
{
public:
	PixelLonLatTransform(GDALDataset* d) { SetDataset(d); }
	PixelLonLatTransform();

	void SetDataset(GDALDataset*);
	void PixelToLonLat(int x, int y, double* lon, double* lat) const;
	void LonLatToPixel(double lon, double lat, int* x, int* y) const;

protected:
	std::auto_ptr<OGRCoordinateTransformation> _lonlattoproj, _projtolonlat;
	double _a[6], _b[6];

	bool _projected;
};

static float hermite4(float frac, float* ptr);

/************************************************************************************************
	main
*************************************************************************************************/

int main(int argc, char** argv)
{
	GDALAllRegister();
	
	GDALDataset* src = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly );
	GDALDataset* dst  = (GDALDataset*)GDALOpen(argv[2], GA_Update );
	
	PixelLonLatTransform srctransform(src);
	PixelLonLatTransform dsttransform(dst);
	
	int xsize = dst->GetRasterXSize();
	int ysize = dst->GetRasterYSize();
	int srcxsize = xsize/10+2;
	double lon, lat;

	float* srcbuffer = new float[4*srcxsize];
	float* dstbuffer = new float[xsize];

	int srcx, srcy;
	dsttransform.PixelToLonLat(0,0, &lon,&lat);
	srctransform.LonLatToPixel(lon,lat, &srcx, &srcy);

	for(int y = 0; y < ysize; y++)
	{		
		src->RasterIO(GF_Read, srcx-1, srcy+y/10-1, srcxsize, 4, 
						srcbuffer, srcxsize, 4, GDT_Float32, 1, NULL, 0, srcxsize*sizeof(float), 0);

		float* h = dstbuffer;
		for(int x = 0; x < xsize; x++, h++)
		{
			float xx[4];
			float yy[4];
			float xp = (x%10)/10.0;
			float yp = (y%10)/10.0;

			for(int iy = 0; iy < 4; ++iy) 
			{
				float* data2 = srcbuffer+ x/10 + iy*srcxsize;

				for(int ix = 0; ix < 4; ++ix ) 
				{
					xx[ix] = data2[ix];
				}
				yy[iy] = hermite4(xp,xx);

			}
			*h = hermite4(yp,yy);
		}

		dst->RasterIO(GF_Write, 0, y, xsize, 1, 
						dstbuffer, xsize,1, GDT_Float32, 1, NULL, 0, xsize*sizeof(float), 0);
	}

	GDALClose(src);
	GDALClose(dst);

	return 0;
}

/************************************************************************************************
	hermite4
*************************************************************************************************/

static float hermite4( float frac, float * ptr )
{
  const float    c     = (ptr[2] - ptr[0]) * 0.5f;
  const float    v     = ptr[1] - ptr[2];
  const float    w     = c + v;
  const float    a     = w + v + (ptr[3] - ptr[1]) * 0.5f;
  const float    b     = w + a;
  return ((((a * frac) - b) * frac + c) * frac + ptr[1]);
}

/************************************************************************************************
	PixelLonLatTransform
*************************************************************************************************/

PixelLonLatTransform::PixelLonLatTransform()
{
	memset(_a, 0x0, sizeof(_a));
	memset(_b, 0x0, sizeof(_b));
	_projected = false;
}

/************************************************************************************************
	SetDataset
*************************************************************************************************/

void PixelLonLatTransform::SetDataset(GDALDataset* dataset)
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

void PixelLonLatTransform::PixelToLonLat(int x, int y, double* lon, double* lat) const
{
	*lon = _a[0] + _a[1] * x + _a[2] * y;
	*lat = _a[3] + _a[4] * x + _a[5] * y;
	
	if(_projected)
		_projtolonlat->Transform(1, lon, lat);
}

/************************************************************************************************
	LonLatToPixel
*************************************************************************************************/

void PixelLonLatTransform::LonLatToPixel(double lon, double lat, int* x, int* y) const
{
	if(_projected)
		_lonlattoproj->Transform(1, &lon, &lat);

	*x = (int)round( _b[0] + _b[1] * lon + _b[2] * lat );
	*y = (int)round( _b[3] + _b[4] * lon + _b[5] * lat );
}
