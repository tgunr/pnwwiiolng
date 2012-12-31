#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

#include "sdm.h"

static int readfn(int fd,void* buf,int size) { return read(fd, buf, size); }

/************************************************************************************************
	smdhf
*************************************************************************************************/

int main(int argc, char** argv)
{
	if(argc < 6)
	{
		printf("Usage: sdmtobil <left supercell> <bottom supercell> <right supercell> <top supercell>\n");
		return 0;
	}
	
	const char* outname = argv[5];
	char filename[64];
	int fd, outfd;
	FILE* f;
	
/*	double lon1 = atof(argv[1]);
	double lat1 = atof(argv[2]);
	double lon2 = atof(argv[3]);
	double lat2 = atof(argv[4]);

	int sx1 = (int)floor(lon1 / SDM_WORLD_ARC_WIDTH  * SDM_WORLD_WIDTH  / SDM_SUPERCELL_SIZE);
	int sy1 = (int)floor(lat1 / SDM_WORLD_ARC_HEIGHT * SDM_WORLD_HEIGHT / SDM_SUPERCELL_SIZE);
	int sx2 = (int)floor(lon2 / SDM_WORLD_ARC_WIDTH  * SDM_WORLD_WIDTH  / SDM_SUPERCELL_SIZE);
	int sy2 = (int)floor(lat2 / SDM_WORLD_ARC_HEIGHT * SDM_WORLD_HEIGHT / SDM_SUPERCELL_SIZE);
*/	
	double lon1,lat1,lon2,lat2;
	int sx1 = atoi(argv[1]);
	int sy1 = atoi(argv[2]);
	int sx2 = atoi(argv[3]);
	int sy2 = atoi(argv[4]);
		
	lon1 = sx1 * SDM_WORLD_ARC_WIDTH  / SDM_WORLD_WIDTH  * SDM_SUPERCELL_SIZE;
	lat1 = sy1 * SDM_WORLD_ARC_HEIGHT / SDM_WORLD_HEIGHT * SDM_SUPERCELL_SIZE;	
	lon2 = (sx2+1) * SDM_WORLD_ARC_WIDTH  / SDM_WORLD_WIDTH  * SDM_SUPERCELL_SIZE;
	lat2 = (sy2+1) * SDM_WORLD_ARC_HEIGHT / SDM_WORLD_HEIGHT * SDM_SUPERCELL_SIZE;	
	
	int numcols = SDM_SUPERCELL_SIZE_X1 * (sx2 - sx1 + 1) + 1;
	int numrows = SDM_SUPERCELL_SIZE_Y1 * (sy2 - sy1 + 1) + 1;

	unsigned short* heightfield = new unsigned short[numcols*numrows*2];

	sdm_dtl_data dtl;
	
	for(int sy = sy1; sy <= sy2; sy++)
	{
		for(int sx = sx1; sx <= sx2; sx++)
		{
	
			snprintf(filename, 63, "data/terrain/%04x_%04x.sdm", (sx < 0) ? sx & 0x0000ffff : sx, (sy < 0) ? sy & 0x0000ffff : sy);
			
			fd = open(filename, O_RDONLY);
			if(fd > -1)
			{
				sdm_read_dtl_data(fd, readfn, &dtl);
				for(int i = 0; i < SDM_SUPERCELL_SIZE_Y; i++)
				{
					unsigned short* rowptr = &heightfield[(sy - sy1 + 1) * SDM_SUPERCELL_SIZE_Y * numcols + (sx - sx1) * SDM_SUPERCELL_SIZE_X1 - (i+1) * numcols];
					memcpy(rowptr, &dtl.z[SDM_SUPERCELL_SIZE_Y*i], SDM_SUPERCELL_SIZE_X*2);
				}
				
				close(fd);
			}
		}
	}
	
	snprintf(filename, 63, "%s.bil", outname);
	outfd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	if(outfd < 0) return -1;
	write(outfd,heightfield,numcols*numrows*2);
	close(outfd);
	
	snprintf(filename, 63, "%s.hdr", outname);
	f = fopen(filename, "w+");
	if(f)
	{
		fprintf(f, "BYTEORDER M\n");
		fprintf(f, "LAYOUT BIL\n");
		fprintf(f, "NROWS %d\n", numrows);
		fprintf(f, "NCOLS %d\n", numcols);
		fprintf(f, "NBANDS 1\n");
		fprintf(f, "NBITS 16\n");
		fprintf(f, "BANDROWBYTES %d\n", numcols*2);
		fprintf(f, "TOTALROWBYTES %d\n", numcols*2);
		fprintf(f, "ULXMAP %f\n", lon1);
		fprintf(f, "ULYMAP %f\n", lat1);
		fprintf(f, "XDIM  0.00833333333333\n");
		fprintf(f, "YDIM  0.00833333333333\n");

		fclose(f);
	}
	
	return 0;
} 