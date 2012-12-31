#include <math.h>
#include <stdio.h>
#include <memory>
#include <list>
#include <algorithm>
#include <vector>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"

#define OGE_USE_SEAWATER 15

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

struct LineSegment
{
	double lon1,lat1,lon2,lat2;
	double m,b;
};

static bool operator < (LineSegment const& a, LineSegment const& b)
{
	return a.lat1 > b.lat1;
}

typedef std::list<LineSegment> LineList;

static void create_sorted_line_list(OGRLayer*,	LineList& lines);
static void create_landsea_mask(GDALDataset* mask, LineList& lines, GDALDataset* usemap);

/************************************************************************************************
	main
*************************************************************************************************/

int main(int argc, char** argv)
{
	GDALAllRegister();
	OGRRegisterAll();

	GDALDataset* usemap  = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly );
	OGRDataSource* coast = OGRSFDriverRegistrar::Open(argv[2]);
	OGRLayer* coastlayer = coast->GetLayer(0);
//	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
//	GDALDataset* dst = driver->CreateCopy(argv[3], src, 1, NULL, GDALTermProgress, NULL );
	GDALDataset* mask  = (GDALDataset*)GDALOpen(argv[3], GA_Update);

	PixelLonLatTransform transform(mask);
	int xsize = mask->GetRasterXSize();
	int ysize = mask->GetRasterYSize();

	double lon1,lat1,lon2,lat2;

	transform.PixelToLonLat(0,0,&lon1,&lat1);
	transform.PixelToLonLat(xsize,ysize,&lon2,&lat2);
	
	OGRPolygon* filter = new OGRPolygon;
	OGRLinearRing ring;
	ring.addPoint(lon1,lat1); ring.addPoint(lon2,lat1); ring.addPoint(lon2,lat2);
	ring.addPoint(lon1,lat1); ring.addPoint(lon1,lat2);
	filter->addRing(&ring);
	coastlayer->SetSpatialFilter(filter);
	
	LineList lines;
	create_sorted_line_list(coastlayer, lines);
	
	create_landsea_mask(mask, lines, usemap);
	
	GDALClose(usemap);
	GDALClose(mask);

	return 0;
}

/************************************************************************************************
	create_landsea_mask
*************************************************************************************************/

static void create_landsea_mask(GDALDataset* mask, LineList& lines, GDALDataset* usemap)
{
	PixelLonLatTransform transform(mask);
	PixelLonLatTransform usetransform(usemap);
	
	int xsize = mask->GetRasterXSize();
	int ysize = mask->GetRasterYSize();

	double lon1,lat1,lon2,lat2;
	transform.PixelToLonLat(0,0,&lon1,&lat1);
	transform.PixelToLonLat(xsize,ysize,&lon2,&lat2);
	
	int usex, usey;
	usetransform.LonLatToPixel(lon1, lat1, &usex, &usey);

	unsigned char* maskbuffer = new unsigned char[xsize];
	unsigned char* usebuffer = new unsigned char[xsize];
	
	for(int y = 0; y < ysize; y++)
	{
		printf("%d ", y);
		transform.PixelToLonLat(0,y,&lon1,&lat1);
		transform.PixelToLonLat(xsize,y+1,&lon2,&lat2);

		// generate list of intersections and update line list
		std::vector<double> intersections;
		
		LineList::iterator it = lines.begin();
		for(; it != lines.end();)
		{
			LineList::iterator cur = it++;			
			LineSegment& segment = *cur;
			
			if(segment.lat2 > lat1)
			{
				lines.erase(cur);
				continue;
			}
			
			if(segment.lat1 < lat1)
			{
				break;
			}
			
			int x,y;
			double ilon;
			
			if(segment.lon2 != segment.lon1)
				ilon = (lat1 - segment.b) / segment.m;
			else
				ilon = segment.lon1;
				
			transform.LonLatToPixel(ilon, lat1, &x, &y);

			if(x>=0 && x<xsize)
				intersections.push_back(ilon);
		}
		std::sort(intersections.begin(),intersections.end());

		usemap->RasterIO(GF_Read, usex, usey+y, xsize, 1, 
						usebuffer, xsize,1, GDT_Byte, 1, NULL, 0, xsize*sizeof(char), 0);
									
		int pixeltype = (usebuffer[0] == OGE_USE_SEAWATER) ? 0x00 : 0xff;
		int miscount = 0;
		
		if(!intersections.empty())
		{
			unsigned char* line = maskbuffer;
			unsigned char* useline = usebuffer;
			
			int n = intersections.size();
			int lastx=0;
			for(int i=0; i < n; i++)
			{
				printf("%lf ", intersections[i]);

				int lx,ly;
				transform.LonLatToPixel(intersections[i], lat1, &lx,&ly);
				for(int x=lastx; x < lx && x < xsize; x++, line++, useline++)
				{
					*line = pixeltype;
					if( (*line == 0x0 && *useline != OGE_USE_SEAWATER) ||
						(*line != 0x0 && *useline == OGE_USE_SEAWATER) )
					{
						miscount++;
					}
				}
	
				lastx = lx;
				pixeltype = (pixeltype == 0x00) ? 0xff : 0x00;
			}
			
			for(int x=lastx; x < xsize; x++, line++, useline++)
			{
				*line = pixeltype;
				if( (*line == 0x0 && *useline != OGE_USE_SEAWATER) ||
					(*line != 0x0 && *useline == OGE_USE_SEAWATER) )
				{
					miscount++;
				}
			}	
		}else
		{
			memset(maskbuffer, pixeltype, xsize);
			
			unsigned char* useline = usebuffer;
			for(int x = 0; x < xsize; x++, useline++)
			{
				if( (pixeltype == 0x0 && *useline != OGE_USE_SEAWATER) ||
					(pixeltype != 0x0 && *useline == OGE_USE_SEAWATER) )
				{
					miscount++;
				}
			}
		}
		
		if(miscount / (float)xsize > .5)
		{
			printf(" inverting, miscount: %f ", miscount / (float)xsize);
			unsigned char* line = maskbuffer;
			for(int x = 0; x < xsize; x++, line++)
			{
				*line = (*line == 0x00) ? 0xff : 0x00;
			}		
		}
		printf("\n");
		
		mask->RasterIO(GF_Write, 0, y, xsize, 1, 
					maskbuffer, xsize,1, GDT_Byte, 1, NULL, 0, xsize*sizeof(char), 0);
	}

	delete [] maskbuffer;
	delete [] usebuffer;
}

/************************************************************************************************
	create_sorted_line_list
*************************************************************************************************/

static void create_sorted_line_list(OGRLayer* layer, LineList& lines)
{
	layer->ResetReading();
	OGRFeature* feature = layer->GetNextFeature();

	for(; feature; feature = layer->GetNextFeature())
	{
		OGRGeometry* ogrgeom = feature->GetGeometryRef();
		OGRLineString* ogrline = (OGRLineString*)(ogrgeom);
		
		int n = ogrline->getNumPoints();
		LineSegment segment;
		
		double lon1,lat1,lon2,lat2;
		for(int i = 0; i < n; i++)
		{
			lon2 = ogrline->getX(i);			
			lat2 = ogrline->getY(i);
	
			segment.lon1 = lon1; segment.lat1 = lat1;
			segment.lon2 = lon2; segment.lat2 = lat2;
			
			if(segment.lat1 < segment.lat2)
			{
				std::swap(segment.lon1, segment.lon2);
				std::swap(segment.lat1, segment.lat2);
			}
			
			if(i > 0)
			{
				if(segment.lon2 != segment.lon1)
					segment.m = (segment.lat2-segment.lat1)/(segment.lon2-segment.lon1);
				segment.b = segment.lat1 - segment.lon1 * segment.m;
				lines.push_back(segment);
			}
			
			lon1 = lon2; lat1 = lat2;
		}
	}
	
	lines.sort();
	
/*	LineList::iterator it = lines.begin();
	int n = lines.size();
	for(int i = 0; i < 1000; i++, ++it)
	{
		LineSegment& seg = *it;
		
//		if(i > 100000 && i < 150000)
		printf ("%lf %lf %lf %lf\n", seg.lon1,seg.lat1, seg.lon2,seg.lat2);
	}*/
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
