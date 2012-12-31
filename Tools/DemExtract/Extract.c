// extract.c

#define NONLS
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <commctrl.h>
#include <mmsystem.h>
#include "dbug.h"
#include "barewnd.h"

// global data
extern int			threadsactive;
extern HINSTANCE	hinst;
extern HWND			myhwnd, pbhwnd;

// typedefs
typedef struct tagDEMHDR
{
	// data found in header file (*.hdr)
	char	byteorder;		//	= 'M';
	char	layout[8];		//	= "BIL";
	int		nrows;			//	= 6000;
	int		ncols;			//	= 4800;
	int		nbands;			//	= 1;
	int		nbits;			//	= 16;
	int		bandrowbytes;	//	= 9600;
	int		totalrowbytes;	//	= 9600;
	int		bandgapbytes;	//	= 0;
	int		nodata;			//	= -9999;
	double	ulxmap;			//	= 20.00416666666667;
	double	ulymap;			//	= 39.99583333333333;
	double	xdim;			//	= 0.00833333333333;
	double	ydim;			//	= 0.00833333333333;

	float	uly, ulx;		// upper left of rectangle to extract
	float	lry, lrx;		// lower right of rectangle to extract
	int		yskipstart, yskipend, ylength;
	int		xskipstart, xskipend, xlength;
	int		ytotal, xtotal;
	int		yfilepos, xfilepos;
	FILE	*outfile;

} DEMHDR, *LPDEMHDR;

// static data
static short		min;
static short		max;
static const char	rootpath[]={"\\demfiles\\"};
static const int	nsbounds[5]			={ 90, 40,-10,-60,-90};
static const int	ewbounds[4][11]		={	{10,-180,-140,-100, -60, -20,  20,  60, 100, 140, 180},
											{10,-180,-140,-100, -60, -20,  20,  60, 100, 140, 180},
											{10,-180,-140,-100, -60, -20,  20,  60, 100, 140, 180},
											{ 7,-180,-120, -60,   0,  60, 120, 180}};
static const char	datafiles[4][9][8]	={	{"w180n90","w140n90","w100n90","w060n90","w020n90","e020n90","e060n90","e100n90","e140n90"},
											{"w180n40","w140n40","w100n40","w060n40","w020n40","e020n40","e060n40","e100n40","e140n40"},
											{"w180s10","w140s10","w100s10","w060s10","w020s10","e020s10","e060s10","e100s10","e140s10"},
											{"w180s60","w120s60","w060s60","w000s60","e060s60","e120s60"}};


static BOOL ReadHdrFile(char *file, LPDEMHDR hdr)
{
char	filename[_MAX_PATH];
char	line[256];
size_t	i;
long	length;
float	x1, y1, x2, y2, tskip;
FILE	*river;

	strcpy(filename, rootpath);
	strcat(filename, file);
	strcat(filename, ".tar\\");
	strcat(filename, file);
	strcat(filename, ".hdr");
	river = fopen(filename, "r");
	if (river == NULL)
	{
		return(FALSE);
	}

	Write_Debug("Reading input header file...");

	fseek(river, 0, SEEK_END);
	length = ftell(river);
	fseek(river, 0, SEEK_SET);

	do
	{
		if (fgets(line, 256, river) == NULL)
		{
			break;
		}

		if (strncmp(line, "BYTEORDER", 9) == 0)
		{
			for (i=9 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->byteorder = line[i];
		}
		else
		if (strncmp(line, "LAYOUT", 6) == 0)
		{
			for (i=6 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			strcpy(hdr->layout, &line[i]);
			hdr->layout[strlen(hdr->layout)-1] = 0;
		}
		else
		if (strncmp(line, "NROWS", 5) == 0)
		{
			for (i=5 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->nrows = atol(&line[i]);
		}
		else
		if (strncmp(line, "NCOLS", 5) == 0)
		{
			for (i=5 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->ncols = atol(&line[i]);
		}
		else
		if (strncmp(line, "NBANDS", 6) == 0)
		{
			for (i=6 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->nbands = atol(&line[i]);
		}
		else
		if (strncmp(line, "NBITS", 5) == 0)
		{
			for (i=5 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->nbits = atol(&line[i]);
		}
		else
		if (strncmp(line, "BANDROWBYTES", 12) == 0)
		{
			for (i=12 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->bandrowbytes = atol(&line[i]);
		}
		else
		if (strncmp(line, "TOTALROWBYTES", 13) == 0)
		{
			for (i=13 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->totalrowbytes = atol(&line[i]);
		}
		else
		if (strncmp(line, "BANDGAPBYTES", 12) == 0)
		{
			for (i=12 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->bandgapbytes = atol(&line[i]);
		}
		else
		if (strncmp(line, "NODATA", 6) == 0)
		{
			for (i=6 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->nodata = atol(&line[i]);
		}
		else
		if (strncmp(line, "ULXMAP", 6) == 0)
		{
			for (i=6 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->ulxmap = atof(&line[i]);
		}
		else
		if (strncmp(line, "ULYMAP", 6) == 0)
		{
			for (i=6 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->ulymap = atof(&line[i]);
		}
		else
		if (strncmp(line, "XDIM", 4) == 0)
		{
			for (i=4 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->xdim = atof(&line[i]);
		}
		else
		if (strncmp(line, "YDIM", 4) == 0)
		{
			for (i=4 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			hdr->ydim = atof(&line[i]);
		}
	}
	while (TRUE);

	fclose(river);

	y1 = hdr->uly;
	x1 = hdr->ulx;		// upper left of rectangle to extract
	y2 = hdr->lry;
	x2 = hdr->lrx;		// lower right of rectangle to extract

	hdr->yskipstart = 0;
	if (y1 < hdr->ulymap)
	{
		hdr->yskipstart = (int)(fabs(hdr->ulymap - y1) / hdr->ydim);
	}

	hdr->yskipend = 0;
	tskip = (float)(hdr->ulymap - hdr->nrows * hdr->ydim);
	if (y2 > tskip)
	{
		hdr->yskipend = (int)(fabs(tskip - y2) / hdr->ydim);
	}

	hdr->ylength = hdr->nrows - hdr->yskipstart - hdr->yskipend;

	hdr->xskipstart = 0;
	if (x1 > hdr->ulxmap)
	{
		hdr->xskipstart = (int)(fabs(x1 - hdr->ulxmap) / hdr->xdim);
	}

	hdr->xskipend = 0;
	tskip = (float)(hdr->ulxmap + hdr->ncols * hdr->xdim);
	if (x2 < tskip)
	{
		hdr->xskipend = (int)(fabs(tskip - x2) / hdr->xdim);
	}

	hdr->xlength = hdr->ncols - hdr->xskipstart - hdr->xskipend;

	min = 32767;
	max = -32768;

	Write_Debug("BYTEORDER = %c", hdr->byteorder);
	Write_Debug("LAYOUT = %s", hdr->layout);
	Write_Debug("NROWS = %d", hdr->nrows);
	Write_Debug("NCOLS = %d", hdr->ncols);
	Write_Debug("NBANDS = %d", hdr->nbands);
	Write_Debug("NBITS = %d", hdr->nbits);
	Write_Debug("BANDROWBYTES = %d", hdr->bandrowbytes);
	Write_Debug("TOTALROWBYTES = %d", hdr->totalrowbytes);
	Write_Debug("BANDGAPBYTES = %d", hdr->bandgapbytes);
	Write_Debug("NODATA = %d", hdr->nodata);
	Write_Debug("ULXMAP = %f", hdr->ulxmap);
	Write_Debug("ULYMAP = %f", hdr->ulymap);
	Write_Debug("XDIM = %f", hdr->xdim);
	Write_Debug("YDIM = %f", hdr->ydim);

	return(TRUE);
}

static BOOL ReadDemFile(char *file, LPDEMHDR hdr)
{
char	text[256], filename[_MAX_PATH];
unsigned char	tmp;
unsigned char	data[8];
short	ht;
int		i, j, k, nbytes, eorskip;
int		yskipstart, yskipend;
int		xskipstart, xskipend;
int		ylength, xlength;
int		yfilepos, xfilepos;
int		ytotal, xtotal;
long	length;
long	stime;
double	xpos, xposstart;
double	ypos, yposstart;
FILE	*stream, *outfile;

	strcpy(filename, rootpath);
	strcat(filename, file);
	strcat(filename, ".tar\\");
	strcat(filename, file);
	strcat(filename, ".dem");
	stream = fopen(filename, "rb");
	if (stream == NULL)
	{
		return(FALSE);
	}

	//setvbuf(stream, NULL, _IOFBF, hdr->totalrowbytes);

	fseek(stream, 0, SEEK_END);
	length = ftell(stream);
	fseek(stream, 0, SEEK_SET);

	nbytes = hdr->nbits / 8;
	if (length != (hdr->nrows * hdr->totalrowbytes))
	{
		return(FALSE);
	}

	outfile = hdr->outfile;

	yskipstart = hdr->yskipstart;
	yskipend = hdr->yskipend;
	xskipstart = hdr->xskipstart;
	xskipend = hdr->xskipend;

	ytotal = hdr->ytotal;
	xtotal = hdr->xtotal;

	stime = GetTickCount();
	eorskip = hdr->totalrowbytes - hdr->ncols * nbytes;
	if (yskipstart)
	{
		fseek(stream, yskipstart * hdr->totalrowbytes, SEEK_SET);
	}

	ypos = yposstart = hdr->ulymap - yskipstart * hdr->ydim;
	xposstart = hdr->ulxmap + xskipstart * hdr->xdim;
	yfilepos = hdr->yfilepos;
	ylength = hdr->ylength;
	for (i=yskipstart ; i < (hdr->nrows - yskipend) ; i++)
	{
		if (xskipstart)
		{
			fseek(stream, xskipstart * nbytes, SEEK_CUR);
		}

		xpos = xposstart;
		xfilepos = hdr->xfilepos;
		xlength = hdr->xlength;

		length = (yfilepos++ * xtotal + xfilepos) * nbytes;
		fseek(outfile, length, SEEK_SET);

		for (j=xskipstart ; j < (hdr->ncols - xskipend) ; j++)
		{
			fread(data, nbytes, 1, stream);
			if (hdr->byteorder == 'M')
			{
				for (k=0 ; k < nbytes/2 ; k++)
				{
					tmp = data[k];
					data[k] = data[nbytes-k-1];
					data[nbytes-k-1] = tmp;
				}
			}

			fwrite(data, nbytes, 1, outfile);
			xpos += hdr->xdim;
			ht = *(short*)data;
			if (ht == hdr->nodata)
			{
				ht = 0;
			}

			if (ht < min)
			{
				min = ht;
			}

			if (ht > max)
			{
				max = ht;
			}
		}

		if (xskipend)
		{
			fseek(stream, xskipend * nbytes, SEEK_CUR);
		}

		if (eorskip)
		{
			fseek(stream, eorskip, SEEK_CUR);
		}

		ypos -= hdr->ydim;
	}

	stime = GetTickCount() - stime;
	fclose(stream);

	sprintf(text, "Start:  x=%f, y=%f\r\n", xposstart, yposstart);
	OutputDebugString(text);
	sprintf(text, "Finish: x=%f, y=%f\r\n", xpos - hdr->xdim, ypos + hdr->ydim);
	OutputDebugString(text);
	sprintf(text, "Processed in %ld ms\r\n", stime);
	OutputDebugString(text);

	return(TRUE);
}

static BOOL DemToBmp(char *filename, LPDEMHDR hdr)
{
char				outfile[256], *ptr;
int					bits;
short				*incolor, innodata;
char				*outcolor;
int					i, j;
int					width, height, twidth;
long				percent;
DWORD				retval=1;
FILE				*hdrfile, *bmpfile;
BITMAPINFOHEADER	bmi;
BITMAPFILEHEADER	bmf;
RGBQUAD				rgb;

	strcpy(outfile, filename);
	ptr = strrchr(outfile, '.');
	if (ptr)
	{
		strcpy(ptr, ".dem");
	}
	else
	{
		strcat(outfile, ".dem");
	}

	Write_Debug("Output bitmap file: %s", outfile);

	hdrfile = fopen(outfile, "rb");
	if (hdrfile != NULL)
	{
		//setvbuf(hdrfile, NULL, _IOFBF, hdr->totalrowbytes);

		SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

		ptr = strrchr(outfile, '.');
		if (ptr)
		{
			strcpy(ptr, ".bmp");
		}
		else
		{
			strcat(outfile, ".bmp");
		}

		bmpfile = fopen(outfile, "w+b");
		if (bmpfile != NULL)
		{
			//setvbuf(bmpfile, NULL, _IOFBF, hdr->ncols);
			width = hdr->ncols;
			twidth = (width + 3) & ~3;
			height = hdr->nrows;

			memset(&bmf, 0, sizeof(BITMAPFILEHEADER));
			bmf.bfType = 'M' << 8 | 'B';
			bmf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + height * twidth;
			bmf.bfReserved1 = 0;
			bmf.bfReserved2 = 0;
			bmf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
			fwrite(&bmf, sizeof(BITMAPFILEHEADER), 1, bmpfile);

			memset(&bmi, 0, sizeof(BITMAPINFOHEADER));
			bmi.biSize = sizeof(BITMAPINFOHEADER);
			bmi.biWidth = width;
			bmi.biHeight = height;
			bmi.biPlanes = 1;
			bmi.biBitCount = 8;
			bmi.biCompression = BI_RGB;
			bmi.biSizeImage = height * twidth;
			fwrite(&bmi, sizeof(BITMAPINFOHEADER), 1, bmpfile);

			rgb.rgbBlue = 255;
			rgb.rgbGreen = 64;
			rgb.rgbRed = 0;
			rgb.rgbReserved = 0;
			fwrite(&rgb, sizeof(RGBQUAD), 1, bmpfile);
			for (i=1 ; i < 256 ; i++)
			{
				rgb.rgbBlue = i;
				rgb.rgbGreen = i;
				rgb.rgbRed = i;
				rgb.rgbReserved = 0;
				fwrite(&rgb, sizeof(RGBQUAD), 1, bmpfile);
			}

			// Size of data just written to file
			bits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);

			innodata = hdr->nodata;
			incolor = malloc(hdr->totalrowbytes);
			outcolor = malloc(twidth);

			height -= 1;
			for (i=height ; i >= 0 ; i--)
			{
				fread(incolor, hdr->totalrowbytes, 1, hdrfile);
				for (j=0 ; j < width ; j++)
				{
					if (incolor[j] == innodata)
					{
						outcolor[j] = 0;
					}
					else
					{
						outcolor[j] = (incolor[j] - min) * 254 / (max - min) + 1;
					}
				}

				fseek(bmpfile, bits + i * twidth, SEEK_SET);
				fwrite(outcolor, twidth, 1, bmpfile);

				percent = (long)((float)(height - i) / (float)height * 100.0f);
				SendMessage(pbhwnd, PBM_SETPOS, percent, 0);
			}

			free(incolor);
			free(outcolor);
			fclose(bmpfile);
		}

		fclose(hdrfile);
	}

	return(TRUE);
}

DWORD WINAPI extractMap(void *dummy)
{
char				text[256], outfile[256], file[256], *ptr;
int					i, j, nsstart, nsend, ewstart, ewend;
long				present, total, percent;
DWORD				retval=1;
float				x1, y1, x2, y2;
LPDEMDETAIL			detail;
LPDEMHDR			hdr;
OPENFILENAME		ofn;
FILE				*hdrfile;

	threadsactive += 1;

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	detail = dummy;
	hdr = malloc(sizeof(DEMHDR));
	memset(hdr, 0, sizeof(DEMHDR));

	y1 =  (float)detail->ulnsdeg;
	y1 += (float)detail->ulnsmin / 60.0f;
	y1 += (float)detail->ulnssec / 3600.0f;
	x1 =  (float)detail->ulewdeg;
	x1 += (float)detail->ulewmin / 60.0f;
	x1 += (float)detail->ulewsec / 3600.0f;
	sprintf(text, "Upper left is at %f x %f\r\n", x1, y1);
	OutputDebugString(text);

	y2 =  (float)detail->lrnsdeg;
	y2 += (float)detail->lrnsmin / 60.0f;
	y2 += (float)detail->lrnssec / 3600.0f;
	x2 =  (float)detail->lrewdeg;
	x2 += (float)detail->lrewmin / 60.0f;
	x2 += (float)detail->lrewsec / 3600.0f;
	sprintf(text, "Lower right is at %f x %f\r\n", x2, y2);
	OutputDebugString(text);

	if (detail->ulns == 'S')
	{
		y1 = -y1;
	}

	if (detail->ulew == 'W')
	{
		x1 = -x1;
	}

	if (detail->lrns == 'S')
	{
		y2 = -y2;
	}

	if (detail->lrew == 'W')
	{
		x2 = -x2;
	}

	for (i=0 ; i < 4 ; i++)
	{
		if (y1 <= nsbounds[i] &&
			y1 > nsbounds[i+1])
		{
			nsstart = i;
			break;
		}
	}

	for (i=0 ; i < 4 ; i++)
	{
		if (y2 < nsbounds[i] &&
			y2 >= nsbounds[i+1])
		{
			nsend = i;
			break;
		}
	}

	hdr->uly = y1;
	hdr->ulx = x1;		// upper left of rectangle to extract
	hdr->lry = y2;
	hdr->lrx = x2;		// lower right of rectangle to extract

	for (i=nsstart ; i <= nsend && retval ; i++)
	{
		for (j=1 ; j < ewbounds[i][0] ; j++)
		{
			if (x1 >= ewbounds[i][j] &&
				x1 < ewbounds[i][j+1])
			{
				ewstart = j - 1;
				break;
			}
		}

		for (j=1 ; j < ewbounds[i][0] ; j++)
		{
			if (x2 > ewbounds[i][j] &&
				x2 <= ewbounds[i][j+1])
			{
				ewend = j - 1;
				break;
			}
		}

		for (j=ewstart ; j <= ewend && retval ; j++)
		{
			strcpy(file, datafiles[i][j]);
			if (!ReadHdrFile(file, hdr))
			{
				retval = 0;
				break;
			}

			if (i == nsstart)
			{
				hdr->xtotal += hdr->xlength;
			}
		}

		hdr->ytotal += hdr->ylength;
	}

	present = 0;
	total = (ewend - ewstart + 1) * (nsend - nsstart + 1);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = myhwnd;
	ofn.hInstance = hinst;
	ofn.lpstrFilter = "DEM Files (*.dem)\0*.dem\0All Files (*.*)\0*.*\0\0";
	ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	wsprintf(outfile, "%c%03d%c%02d",	(y1 < 0.0f ? 's' : 'n'), (int)fabs(y1),
										(x1 < 0.0f ? 'w' : 'e'), (int)fabs(x1));
	ofn.lpstrFile = outfile;
	ofn.nMaxFile = 256;
	ofn.lpstrDefExt = ".dem";
	if (GetSaveFileName(&ofn))
	{
		hdr->outfile = fopen(ofn.lpstrFile, "w+b");
		if (hdr->outfile != NULL)
		{
			Write_Debug("File to extract: %s", ofn.lpstrFile);
			//setvbuf(hdr->outfile, NULL, _IOFBF, hdr->totalrowbytes);
			hdr->yfilepos = 0;
			for (i=nsstart ; i <= nsend && retval ; i++)
			{
				for (j=1 ; j < ewbounds[i][0] ; j++)
				{
					if (x1 >= ewbounds[i][j] &&
						x1 < ewbounds[i][j+1])
					{
						ewstart = j - 1;
						break;
					}
				}

				for (j=1 ; j < ewbounds[i][0] ; j++)
				{
					if (x2 > ewbounds[i][j] &&
						x2 <= ewbounds[i][j+1])
					{
						ewend = j - 1;
						break;
					}
				}

				hdr->xfilepos = 0;
				for (j=ewstart ; j <= ewend && retval ; j++)
				{
					strcpy(file, datafiles[i][j]);
					if (!ReadHdrFile(file, hdr))
					{
						retval = 0;
						break;
					}

					if (!ReadDemFile(file, hdr))
					{
						retval = 0;
						break;
					}

					hdr->xfilepos += hdr->xlength;

					present += 1;
					percent = (long)((float)present / (float)total * 100.0f);
					SendMessage(pbhwnd, PBM_SETPOS, percent, 0);
				}

				hdr->yfilepos += hdr->ylength;
			}

			fclose(hdr->outfile);
		}

		if (retval != 0)
		{
			ptr = strrchr(outfile, '.');
			if (ptr)
			{
				strcpy(ptr, ".hdr");
			}
			else
			{
				strcat(outfile, ".hdr");
			}

			Write_Debug("Output header file: %s", outfile);

			hdrfile = fopen(outfile, "wb");
			if (hdrfile != NULL)
			{
				hdr->nrows = hdr->ytotal;
				hdr->ncols = hdr->xtotal;
				hdr->bandgapbytes = hdr->ncols * hdr->nbits / 8;
				hdr->totalrowbytes = hdr->ncols * hdr->nbits / 8;
				fprintf(hdrfile, "BYTEORDER     I\r\n");
				fprintf(hdrfile, "LAYOUT        %s\r\n", hdr->layout);
				fprintf(hdrfile, "NROWS         %d\r\n", hdr->nrows);
				fprintf(hdrfile, "NCOLS         %d\r\n", hdr->ncols);
				fprintf(hdrfile, "NBANDS        %d\r\n", hdr->nbands);
				fprintf(hdrfile, "NBITS         %d\r\n", hdr->nbits);
				fprintf(hdrfile, "BANDROWBYTES  %d\r\n", hdr->bandgapbytes);
				fprintf(hdrfile, "TOTALROWBYTES %d\r\n", hdr->totalrowbytes);
				fprintf(hdrfile, "BANDGAPBYTES  %d\r\n", hdr->bandgapbytes);
				fprintf(hdrfile, "NODATA        %d\r\n", hdr->nodata);
				fprintf(hdrfile, "ULXMAP        %f\r\n", x1);
				fprintf(hdrfile, "ULYMAP        %f\r\n", y1);
				fprintf(hdrfile, "XDIM          %f\r\n", hdr->xdim);
				fprintf(hdrfile, "YDIM          %f\r\n", hdr->ydim);
				fclose(hdrfile);
			}

			DemToBmp(outfile, hdr);
		}

		PlaySound("SystemExclamation",NULL ,SND_ASYNC|SND_ALIAS);
	}

	free(hdr);
	free(detail);

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	threadsactive -= 1;

	return(retval);
}
