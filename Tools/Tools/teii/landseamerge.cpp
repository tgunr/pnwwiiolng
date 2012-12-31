#include <math.h>
#include <stdio.h>
#include <memory>
#include <list>
#include <algorithm>
#include <vector>

#include "gdal_priv.h"

/************************************************************************************************
	main
*************************************************************************************************/
// ./landseamerge elevation/gib-land.tif elevation/gib-sea.tif use/gib-mask.tif elevation/gib-merged.tif 

int main(int argc, char** argv)
{
	GDALAllRegister();

	GDALDataset* land  = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly );
	GDALDataset* sea  = (GDALDataset*)GDALOpen(argv[2], GA_ReadOnly );
	GDALDataset* mask  = (GDALDataset*)GDALOpen(argv[3], GA_ReadOnly);
	GDALDataset* dst  = (GDALDataset*)GDALOpen(argv[4], GA_Update);

	int xsize = land->GetRasterXSize();
	int ysize = land->GetRasterYSize();

	float* landbuffer = new float[xsize];
	float* seabuffer = new float[xsize];
	unsigned char* maskbuffer = new unsigned char[xsize];
	float* dstbuffer = new float[xsize];
	
	for(int y = 0; y < ysize; y++)
	{
		land->RasterIO(GF_Read, 0, y, xsize, 1, 
						landbuffer, xsize,1, GDT_Float32, 1, NULL, 0, xsize*sizeof(float), 0);
		sea->RasterIO(GF_Read, 0 , y, xsize, 1, 
						seabuffer, xsize,1, GDT_Float32, 1, NULL, 0, xsize*sizeof(float), 0);
		mask->RasterIO(GF_Read, 0, y, xsize, 1, 
						maskbuffer, xsize,1, GDT_Byte, 1, NULL, 0, xsize, 0);
		
		float* landp = landbuffer;
		float* seap = seabuffer;
		unsigned char* maskp = maskbuffer;
		float* dstp = dstbuffer;

		for(int x = 0; x < xsize; x++,landp++,seap++,maskp++,dstp++)
		{
			*dstp = *maskp ? *landp : ((*seap < -1) ? *seap : -1);
		}

		dst->RasterIO(GF_Write, 0, y, xsize, 1, 
						dstbuffer, xsize,1, GDT_Float32, 1, NULL, 0, xsize*sizeof(float), 0);
	}

	GDALClose(land);
	GDALClose(sea);
	GDALClose(mask);
	GDALClose(dst);

	return 0;
}