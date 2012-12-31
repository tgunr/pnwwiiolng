#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1200
#define SIZE1 1201
#define DIM "0.000833333333333"

int main(int argc, char** argv)
{
	int lon1,lat1,lon2,lat2;
	const char* outname;
	char filename[64];
	char basename[64];
	
	lon1 = atoi(argv[1]);
	lat1 = atoi(argv[2]);
	lon2 = atoi(argv[3]);
	lat2 = atoi(argv[4]);
	outname = argv[5];

	int nrows = (lat1-lat2+1)*SIZE + 1;
	int ncols = (lon2-lon1+1)*SIZE + 1;
	int rowbytes = ncols*sizeof(short);
	
	snprintf(filename, 63, "%s.hdr", outname);
	FILE* hdrfile = fopen(filename, "wb");
	fprintf(hdrfile,	
			"BYTEORDER M\n"
			"LAYOUT BIL\n"
			"NROWS %d\n"
			"NCOLS %d\n"
			"NBANDS 1\n"
			"NBITS 16\n"
			"BANDROWBYTES %d\n"
			"TOTALROWBYTES %d\n"
			"BANDGAPBYTES 0\n"
			"NODATA -32768\n"
			"ULXMAP %d\n"
			"ULYMAP %d\n"
			"XDIM " DIM "\n"
			"YDIM " DIM "\n", 
			nrows, ncols, rowbytes, rowbytes, lon1, lat1+1);
	fclose(hdrfile);
	
	snprintf(filename, 63, "%s.bil", outname);
	FILE* outfile = fopen(filename, "wb");
	char block[SIZE*SIZE*sizeof(short)];
	memset(block,0x0,sizeof(block));

	for(int lat = lat1; lat >= lat2; lat--)
	{
		for(int lon = lon1; lon <= lon2; lon++)
		{
			fwrite(block, 1, sizeof(block), outfile);
		}
	}
	fwrite(block, 1, sizeof(short)*((lat1-lat2+1) + (lon2-lon1+1) - 1), outfile);

	fseek(outfile, 0, SEEK_SET);
	
	for(int lat = lat1; lat >= lat2; lat--)
	{
		for(int lon = lon1; lon <= lon2; lon++)
		{
			snprintf(basename, 63,"N%d%s%03d", lat, lon>=0?"E":"W", lon>=0?lon:-lon); 
			snprintf(filename, 63, "%s.bil", basename);
			
			printf("%s\n", filename);
			
			if(access(filename, O_RDONLY) != 0)
			{
				char call[64];
	
				snprintf(call, 63, "unzip %s.hgt.zip", basename);
				system(call);
				printf("%s\n", call);

				snprintf(call, 63, "mv %s.hgt %s", basename, filename);
				system(call);
				printf("%s\n", call);
			}

			FILE* srcfile = fopen(filename, "rb");
			memset(block, 0x0, SIZE1*sizeof(short));

			int px = (lon-lon1)*SIZE;
			int py = (lat1-lat)*SIZE;
			printf("%d %d\n", px, py);
		
			for(int y = 0; y < SIZE1; y++)
			{
				if(y == SIZE && lat != lat2) continue;
				
				fseek(outfile, (y+py)*rowbytes + px*sizeof(short), SEEK_SET);
				
				if(srcfile)
					fread(block, 1, SIZE1*sizeof(short), srcfile);
				
				if(lon != lon2)
					fwrite(block, 1, SIZE*sizeof(short), outfile);
				else
					fwrite(block, 1, SIZE1*sizeof(short), outfile);
			}
			
			if(srcfile)
				fclose(srcfile);
		}
	}
	fclose(outfile);
	
	return 0;
}