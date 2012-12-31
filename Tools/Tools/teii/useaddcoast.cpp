#include <math.h>
#include <stdio.h>
#include <memory>
#include <list>
#include <algorithm>
#include <vector>

#include "gdal_priv.h"
#include "usemap_oge.h"

/************************************************************************************************
	main
*************************************************************************************************/
// ./usemapcoast

int main(int argc, char** argv)
{
	GDALAllRegister();

	GDALDataset* src  = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly );
	GDALDataset* mask  = (GDALDataset*)GDALOpen(argv[2], GA_ReadOnly);
	GDALDataset* dst  = (GDALDataset*)GDALOpen(argv[3], GA_Update);

	int xsize = src->GetRasterXSize();
	int ysize = src->GetRasterYSize();

	unsigned char* srcbuffer = new unsigned char[xsize];
	unsigned char* maskbuffer = new unsigned char[xsize];
	unsigned char* dstbuffer = new unsigned char[xsize];
	
	for(int y = 0; y < ysize; y++)
	{
		src->RasterIO(GF_Read, 0, y, xsize, 1, 
						srcbuffer,  xsize,1, GDT_Byte, 1, NULL, 0, xsize, 0);
		mask->RasterIO(GF_Read, 0, y, xsize, 1, 
						maskbuffer, xsize,1, GDT_Byte, 1, NULL, 0, xsize, 0);
		
		unsigned char* srcp = srcbuffer;
		unsigned char* maskp = maskbuffer;
		unsigned char* dstp = dstbuffer;

		for(int x = 0; x < xsize; x++,srcp++,maskp++,dstp++)
		{
			unsigned char use = *srcp;
			if(*maskp) // land
			{
				if(use == OGE_USE_SEA_WATER )
				{
					use = OGE_USE_LAND_WATER_AND_SHORE;
					
				}
			}else // sea
			{
				if(	use != OGE_USE_INLAND_WATER &&
					use != OGE_USE_SEA_WATER &&
					use != OGE_USE_WATER_AND_ISLAND_FRINGE &&
					use != OGE_USE_LAND_WATER_AND_SHORE &&
					use != OGE_USE_LAND_AND_WATER_RIVERS)
				{
					use = OGE_USE_LAND_WATER_AND_SHORE;
				}
			
			}
			
			*dstp = use;
		}

		dst->RasterIO(GF_Write, 0, y, xsize, 1, 
						dstbuffer, xsize,1, GDT_Byte, 1, NULL, 0, xsize*sizeof(float), 0);
	}

	GDALClose(src);
	GDALClose(mask);
	GDALClose(dst);

	return 0;
}