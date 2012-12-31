#include <math.h>
#include <stdio.h>

#include "gdal_priv.h"

#define SQRT2 1.414213562373095

/************************************************************************************************
	srtmfill
*************************************************************************************************/

class GDALDataBuffer
{
public:
	GDALDataBuffer(GDALDataset*);
	~GDALDataBuffer();
	
	double GetPixel(int x, int y);
	
	GDALDataset* Dataset() { return _dataset; }
	
protected:
	void UpdateBuffer(int x, int y);

	GDALDataset* _dataset;
	float* _buffer;
	int _x, _y, _width, _height, _cachewidth, _cacheheight;
	double _ndata;
};

class SRTMFiller
{
public:
	SRTMFiller(GDALDataset* dst, GDALDataset* src, GDALDataset* base);
	~SRTMFiller();
	
	void Fill();
	
protected:
	double CalcVoidPixel(int x, int y);

	GDALDataset* _dataset;
	GDALDataBuffer _src;// _base;
	double _ndata;
};

/************************************************************************************************
	main
*************************************************************************************************/
// ./srtmfill elevation/gib-merged.tif elevation/gib.tif 

int main(int argc, char** argv)
{
	GDALAllRegister();
	
	GDALDataset* src = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly );
//	GDALDataset* base  = (GDALDataset*)GDALOpen(argv[2], GA_ReadOnly );
	GDALDataset* dst = (GDALDataset*)GDALOpen(argv[2], GA_Update);
//	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
  //	GDALDataset* dst = driver->CreateCopy(argv[3], src, 
    //                             				1, NULL, GDALTermProgress, NULL );

	SRTMFiller filler(dst, src, 0);//base);
	filler.Fill();
	
	GDALClose(src);
	GDALClose(dst);
//	GDALClose(base);
	
	return 0;
}

/************************************************************************************************
	SRTMFiller
*************************************************************************************************/

SRTMFiller::SRTMFiller(GDALDataset* dst, GDALDataset* src, GDALDataset* base) :
	_dataset(dst), _src(src)//, _base(base)
{
	_ndata = _src.Dataset()->GetRasterBand(1)->GetNoDataValue();
	_dataset->Reference();
}

SRTMFiller::~SRTMFiller()
{
	GDALClose(_dataset);
}

/************************************************************************************************
	Fill
*************************************************************************************************/

void SRTMFiller::Fill()
{
	int xsize = _dataset->GetRasterXSize();
	int ysize = _dataset->GetRasterYSize();
	
	int bufsize = 1200;
	float* buffer = new float[bufsize];
	
	for(int y = 0; y < ysize; y++)
	{
		int n = 0, xbuf = 0;
printf("%d\n", y);		
		for(int x = 0; x < xsize; x++)
		{
			double h = _src.GetPixel(x,y); 
			if(h == _ndata)
				h = CalcVoidPixel(x,y);
			
			buffer[n++] = h;
			
			if(n == bufsize || x == xsize-1)
			{	
				_dataset->RasterIO(GF_Write, xbuf, y, n, 1, 
									buffer, n,1,
									GDT_Float32, 1, NULL, 0, n*sizeof(float), 0);
				xbuf += n;
				n = 0;
			}
		}
	}
	
	delete [] buffer;
}

/************************************************************************************************
	CalcVoidPixel
*************************************************************************************************/

double SRTMFiller::CalcVoidPixel(int vx, int vy)
{
	int xsize = _dataset->GetRasterXSize();
	int ysize = _dataset->GetRasterYSize();
	int x,y,i;

	// height values
	float lt, ct, rt;
	float lc, rc;
	float lb, cb, rb;
	
	// weights
	float ltw, ctw, rtw;
	float lcw, rcw;
	float lbw, cbw, rbw;
	float tw, h;
	
	lt = ct = rt = lc = rc = lb = cb = rb = 0;
	ltw = ctw = rtw = lcw = rcw = lbw = cbw = rbw = tw = 0;
	
	// left top
	for(i = 1; i < 10; i++)
	{
		x = vx - i;
		y = vy - i;
		if(x < 0 || y < 0)
		{
			ltw = 0; 
			lt = 0;
			break;
		}
		
		lt = _src.GetPixel(x,y);
		if(lt != _ndata)
		{
			ltw = 1/(i*SQRT2);
			break;
		}
	}
	
	// center top
	for(i = 1;  i < 10; i++)
	{
		x = vx;
		y = vy - i;
		if(y < 0)
		{
			ctw = 0; 
			ct = 0;
			break;
		}
		
		ct = _src.GetPixel(x,y);
		if(ct != _ndata)
		{
			ltw = 1/i;
			break;
		}
	}
		
	//  right top
	for(i = 1; i < 10; i++)
	{
		x = vx + i;
		y = vy - i;
		if(x >= xsize || y < 0)
		{
			rtw = 0; 
			rt = 0;
			break;
		}
		
		rt = _src.GetPixel(x,y);
		if(rt != _ndata)
		{
			rtw = 1/(i*SQRT2);
			break;
		}
	}
		
	// left center
	for(i = 1;  i < 10; i++)
	{
		x = vx - i;
		y = vy;
		if(x < 0)
		{
			lcw = 0; 
			lc = 0;
			break;
		}
		
		lc = _src.GetPixel(x,y);
		if(lc != _ndata)
		{
			lc = 1/i;
			break;
		}
	}	
	
	// right center
	for(i = 1; i < 10; i++)
	{
		x = vx + i;
		y = vy;
		if(x >= xsize)
		{
			rcw = 0; 
			rc = 0;
			break;
		}
		
		rc = _src.GetPixel(x,y);
		if(rc != _ndata)
		{
			rc = 1/i;
			break;
		}
	}	
		
	// left bottom
	for(i = 1; i < 10; i++)
	{
		x = vx - i;
		y = vy + i;
		if(x < 0 || y >= ysize)
		{
			lbw = 0; 
			lb = 0;
			break;
		}
		
		lb = _src.GetPixel(x,y);
		if(lb != _ndata)
		{
			lbw = 1/(i*SQRT2);
			break;
		}
	}
		
	// center bottom
	for(i = 1; i < 10; i++)
	{
		x = vx;
		y = vy + i;
		if(y >= ysize)
		{
			cbw = 0; 
			cb = 0;
			break;
		}
		
		cb = _src.GetPixel(x,y);
		if(cb != _ndata)
		{
			cb = 1/i;
			break;
		}
	}
	
	// right bottom
	for(i = 1; i < 10; i++)
	{
		x = vx + i;
		y = vy + i;
		if(x >= xsize || y >= ysize)
		{
			rbw = 0; 
			rb = 0;
			break;
		}
		
		rb = _src.GetPixel(x,y);
		if(rb != _ndata)
		{
			rbw = 1/(i*SQRT2);
			break;
		}
	}
	
	tw = ltw + ctw + rtw + lcw + rcw + lbw + cbw + rbw;
	
	h = lt*ltw + ct*ctw + rt*rtw + lc*lcw + rc*rcw + lb*lbw + cb*cbw + rb*rbw;
	h /= tw;
	
	return h;
}

/************************************************************************************************
	GDALDataBuffer
*************************************************************************************************/

GDALDataBuffer::GDALDataBuffer(GDALDataset* d)
{
	_dataset = d;
	_buffer = 0;
	_x = _y = _width = _height = -1;
	_cachewidth = d->GetRasterXSize();
	_cacheheight = 64;
	_buffer = new float[_cachewidth*_cacheheight];
	_dataset->Reference();
	_ndata = _dataset->GetRasterBand(1)->GetNoDataValue();
}

GDALDataBuffer::~GDALDataBuffer()
{
	GDALClose(_dataset);
	delete [] _buffer;
}

/************************************************************************************************
	GetPixel
*************************************************************************************************/

double GDALDataBuffer::GetPixel(int x, int y)
{
	int xsize = _dataset->GetRasterXSize();
	int ysize = _dataset->GetRasterYSize();
	
	if(x < 0 || y < 0 || x >= xsize || y >= ysize) return _ndata;
	
	if(x < _x || y < _y || x >= _x+_width || y >= _y+_height)
		UpdateBuffer(x,y);
	
	return _buffer[(y-_y)*_width+(x-_x)];
}

/************************************************************************************************
	UpdateBuffer
*************************************************************************************************/

void GDALDataBuffer::UpdateBuffer(int x, int y)
{	
	int xsize = _dataset->GetRasterXSize();
	int ysize = _dataset->GetRasterYSize();

	int x1 = x - _cachewidth/2;
	int x2 = x + _cachewidth/2;
	int y1 = y - _cacheheight/2;
	int y2 = y + _cacheheight/2;
	
	if(x1 < 0) x1 = 0; if(x2 > xsize) x2 = xsize;
	if(y1 < 0) y1 = 0; if(y2 > ysize) y2 = ysize;
	
	_x = 0;//x1;
	_y = y1;
	_width  = _cachewidth;//x2-x1;
	_height = y2-y1;	
	
	printf("UpdateBuffer %d %d\n", y, x);
	_dataset->RasterIO(GF_Read, _x, _y, _width, _height, 
							_buffer, _width,_height,
							GDT_Float32, 1, NULL, 0, _width*sizeof(float), 0 );
}
