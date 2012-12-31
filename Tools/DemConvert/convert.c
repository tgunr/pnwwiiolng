// DemConvert.c : Defines the entry point for the application.
//

#include "BareWnd.h"
#include <commctrl.h>
#include <mmsystem.h>
#include <math.h>

extern int			threadsactive;
extern HINSTANCE	hinst;
extern HWND			myhwnd, pbhwnd;

// Defines
// Do recreation of normals only
#define RECREATE_NORMALS
#undef RECREATE_NORMALS

// 36.869897645844021296855612559093 degrees
#define PERSPECTIVE_ANGLE	0.64350110879328438680280922871732
// (cos(PERSPECTIVE_ANGLE))
#define PERSPECTIVE_FACTOR	0.8
#define PERSPECTIVE_EDGE	70.0
#define MULTIPLIER			65536
// Distance between adjacent map points
#define VECTOR_DISTANCE		400.0f

// typedefs
typedef struct tagVECTOR
{
	float	x;
	float	y;
	float	z;
} VECTOR, *LPVECTOR;

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

} DEMHDR, *LPDEMHDR;

// Inline functions
/*
 * Normalizes the vector a
 */
__inline LPVECTOR math3dNormalize(LPVECTOR lpa)
{
float inv_mod;

	inv_mod = lpa->x * lpa->x + lpa->y * lpa->y + lpa->z * lpa->z;
	if (inv_mod)
	{
		inv_mod = (float)(1.0 / sqrt(inv_mod));

		lpa->x *= inv_mod;
		lpa->y *= inv_mod;
		lpa->z *= inv_mod;
	}

	return lpa;
}

/*
 * Calculates cross product of a and b.
 */
__inline LPVECTOR math3dCrossProduct(LPVECTOR lpd, LPVECTOR lpa, LPVECTOR lpb)
{
VECTOR  d;

	d.x = lpa->y * lpb->z - lpa->z * lpb->y;
	d.y = lpa->z * lpb->x - lpa->x * lpb->z;
	d.z = lpa->x * lpb->y - lpa->y * lpb->x;

	*lpd = d;

	return lpd;
}

/*
 * lpd = lpa - lpb
 * lpd can be equal to lpa or lpb
 */
__inline LPVECTOR math3dSubtractVectors(LPVECTOR lpd, LPVECTOR lpa, LPVECTOR lpb)
{
VECTOR  d;

	d.x = lpa->x - lpb->x;
	d.y = lpa->y - lpb->y;
	d.z = lpa->z - lpb->z;

	*lpd = d;

	return lpd;
}

/*
 * lpd = lpa + lpb
 * lpd can be equal to lpa or lpb
 */
__inline LPVECTOR math3dAddVectors(LPVECTOR lpd, LPVECTOR lpa, LPVECTOR lpb)
{
VECTOR  d;

	d.x = lpa->x + lpb->x;
	d.y = lpa->y + lpb->y;
	d.z = lpa->z + lpb->z;

	*lpd = d;

	return lpd;
}

// Statics
static short	min, max;


// Static funcitons

static BOOL ReadHdrFile(char *file, LPDEMHDR hdr)
{
char	filename[_MAX_PATH];
char	line[256];
size_t	i;
long	length;
FILE	*river;

	strcpy(filename, file);
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

		i = strlen(line) - 1;
		if (i > 0)
		{
			do
			{
				if (!isspace(line[i]))
				{
					break;
				}

				i -= 1;
			}
			while(TRUE);

			line[i+1] = 0;
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
			hdr->layout[strlen(hdr->layout)] = 0;
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

static BOOL WriteHdrFile(char *outfile, LPDEMHDR hdr)
{
char	hdrfile[256], *ptr;
FILE	*stream;

	strcpy(hdrfile, outfile);
	ptr = strrchr(hdrfile, '.');
	if (ptr)
	{
		strcpy(ptr, ".hdr");
	}
	else
	{
		strcat(hdrfile, ".hdr");
	}

	Write_Debug("Output header file: %s", hdrfile);

	stream = fopen(hdrfile, "w");
	if (stream == NULL)
	{
		return(FALSE);
	}

	Write_Debug("Writing output header file...");

	fprintf(stream, "BYTEORDER     I\r\n");
	fprintf(stream, "LAYOUT        %s\r\n", hdr->layout);
	fprintf(stream, "NROWS         %d\r\n", hdr->nrows);
	fprintf(stream, "NCOLS         %d\r\n", hdr->ncols);
	fprintf(stream, "NBANDS        %d\r\n", hdr->nbands);
	fprintf(stream, "NBITS         %d\r\n", hdr->nbits);
	fprintf(stream, "BANDROWBYTES  %d\r\n", hdr->bandrowbytes);
	fprintf(stream, "TOTALROWBYTES %d\r\n", hdr->totalrowbytes);
	fprintf(stream, "BANDGAPBYTES  %d\r\n", hdr->bandgapbytes);
	fprintf(stream, "NODATA        %d\r\n", hdr->nodata);
	fprintf(stream, "ULXMAP        %f\r\n", hdr->ulxmap);
	fprintf(stream, "ULYMAP        %f\r\n", hdr->ulymap);
	fprintf(stream, "XDIM          %f\r\n", hdr->xdim);
	fprintf(stream, "YDIM          %f\r\n", hdr->ydim);
	fprintf(stream, "MIN           %d\r\n", min);
	fprintf(stream, "MAX           %d\r\n", max);

	fclose(stream);

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
	Write_Debug("MIN = %d", min);
	Write_Debug("MAX = %d", max);

	return(TRUE);
}

static BOOL ConvertFile(char *infile, char *outfile, LPDEMHDR inhdr, LPDEMHDR outhdr)
{
int		ini, outi, j;
int		intotalrowbytes, outtotalrowbytes;
int		outnrows, outncols;
int		ifrac, irefrac;
short	*inval1, *inval2, *outval1, invala, invalb, outvala;
short	innodata, outnodata;
long	percent;
double	delta, frac, refrac;
double	inrow, outrow;
FILE	*instream, *outstream;

	instream = fopen(infile, "rb");
	if (instream == NULL)
	{
		return(FALSE);
	}

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	outstream = fopen(outfile, "w+b");
	if (outstream == NULL)
	{
		fclose(instream);
		return(FALSE);
	}

	memcpy(outhdr, inhdr, sizeof(DEMHDR));
	outhdr->nrows = (int)((double)outhdr->nrows / PERSPECTIVE_FACTOR);
	outhdr->ydim *= PERSPECTIVE_FACTOR;

	frac = 0.0;
	delta = 0.0;
	do
	{
		delta += inhdr->ydim;
		if (delta >= inhdr->ulymap)
		{
			break;
		}

		frac += PERSPECTIVE_FACTOR;
		if (frac > 1.0)
		{
			frac -= 1.0;
		}
	}
	while (TRUE);

	// There is a lot of water in the world so make it the ldefault min/max
	min = 0;
	max = 0;

	refrac = 1.0 - frac;

	inrow = inhdr->ulymap;
	intotalrowbytes = inhdr->totalrowbytes;
	innodata = inhdr->nodata;
	inval1 = malloc(inhdr->totalrowbytes);
	if (inval1 == NULL)
	{
		return(FALSE);
	}

	inval2 = malloc(inhdr->totalrowbytes);
	if (inval2 == NULL)
	{
		return(FALSE);
	}

	outrow = outhdr->ulymap;
	outtotalrowbytes = outhdr->totalrowbytes;
	outnodata = outhdr->nodata;
	outnrows = outhdr->nrows;
	outncols = outhdr->ncols;
	outval1 = malloc(outhdr->totalrowbytes);
	if (outval1 == NULL)
	{
		return(FALSE);
	}

	for (ini=outi=0 ; outi < outnrows ; outi++)
	{
		ifrac = (int)(frac * MULTIPLIER);
		irefrac = (int)(refrac * MULTIPLIER);

		fseek(instream, (ini) * intotalrowbytes, SEEK_SET);
		fread(inval1, inhdr->totalrowbytes, 1, instream);

		fseek(instream, (ini + 1) * intotalrowbytes, SEEK_SET);
		fread(inval2, inhdr->totalrowbytes, 1, instream);

		for (j=0 ; j < outncols ; j++)
		{
			invala = inval1[j];
			invalb = inval2[j];
			if (invala == innodata ||
				invalb == innodata)
			{
				outval1[j] = outnodata;
			}
			else
			{
				outvala = (short)(((int)invala * irefrac + (int)invalb * ifrac) / MULTIPLIER);
				if (max < outvala)
				{
					max = outvala;
				}

				if (min > outvala)
				{
					min = outvala;
				}

				outval1[j] = outvala;
			}

			percent = (long)((float)outi / (float)outnrows * 100.0f);
			SendMessage(pbhwnd, PBM_SETPOS, percent, 0);
		}

		fseek(outstream, outi * outtotalrowbytes, SEEK_SET);
		fwrite(outval1, outhdr->totalrowbytes, 1, outstream);

		frac -= PERSPECTIVE_FACTOR;
		if (frac < 0.0)
		{
			frac += 1.0;
		}

		refrac = 1.0 - frac;

		outrow -= outhdr->ydim;
		while (outrow < inrow)
		{
			inrow -= inhdr->ydim;
			ini += 1;
		}
	}

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	free(inval1);
	free(inval2);
	free(outval1);

	fclose(instream);
	fclose(outstream);

	return(TRUE);
}

static BOOL CreateNormals(float **row, short *outcolor, LPDEMHDR hdr)
{
int			i, ncols;
VECTOR		v1, v2, v3, vv1, vv2, vv;
VECTOR		nv;

	ncols = hdr->ncols + 1;
	for (i=1 ; i < ncols ; i++)
	{
		v3.x = 0.0f;
		v3.y = 0.0f;
		v3.z = -row[1][i];

		// Triangle 1
		v1.x = -VECTOR_DISTANCE;
		v1.y =  VECTOR_DISTANCE;
		v1.z = -row[0][i-1];
		v2.x = 0.0f;
		v2.y =  VECTOR_DISTANCE;
		v2.z = -row[0][i];
		math3dSubtractVectors(&vv1, &v1, &v3);
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		nv = vv;

		// Triangle 2
		vv1 = vv2;
		v2.x =  VECTOR_DISTANCE;
		v2.y =  VECTOR_DISTANCE;
		v2.z = -row[0][i+1];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		// Triangle 3
		vv1 = vv2;
		v2.x =  VECTOR_DISTANCE;
		v2.y = 0.0f;
		v2.z = -row[1][i+1];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		// Triangle 4
		vv1 = vv2;
		v2.x =  VECTOR_DISTANCE;
		v2.y = -VECTOR_DISTANCE;
		v2.z = -row[2][i+1];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		// Triangle 5
		vv1 = vv2;
		v2.x = 0.0f;
		v2.y = -VECTOR_DISTANCE;
		v2.z = -row[2][i];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		// Triangle 6
		vv1 = vv2;
		v2.x = -VECTOR_DISTANCE;
		v2.y = -VECTOR_DISTANCE;
		v2.z = -row[2][i-1];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		// Triangle 7
		vv1 = vv2;
		v2.x = -VECTOR_DISTANCE;
		v2.y = 0.0f;
		v2.z = -row[1][i-1];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		// Triangle 8
		vv1 = vv2;
		v2.x = -VECTOR_DISTANCE;
		v2.y =  VECTOR_DISTANCE;
		v2.z = -row[0][i-1];
		math3dSubtractVectors(&vv2, &v2, &v3);
		math3dCrossProduct(&vv, &vv1, &vv2);
		math3dNormalize(&vv);
		math3dAddVectors(&nv, &nv, &vv);

		math3dNormalize(&nv);
		outcolor[i-1] = (short)((((short)(nv.x * 16.0f) & 0x1f) << 11) +
								(((short)(nv.y * 16.0f) & 0x1f) << 6) +
								(((short)(-nv.z * 63.0f + 0.5f) & 0x3f) << 0));
	}

	// Finish up for next row
	memcpy(row[0], row[1], (hdr->ncols + 2) * sizeof(float));
	memcpy(row[1], row[2], (hdr->ncols + 2) * sizeof(float));

	return(TRUE);
}

static BOOL DemToNormals(char *filename, LPDEMHDR hdr)
{
char				outfile[256], *ptr;
short				*incolor, *outcolor, nodata;
int					i, j;
int					width, height, totalrowbytes, ncols;
long				percent;
float				*row[3];
DWORD				retval=1;
FILE				*hdrfile, *normfile;

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

	hdrfile = fopen(outfile, "rb");
	if (hdrfile != NULL)
	{
		SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

		ptr = strrchr(outfile, '.');
		if (ptr)
		{
			strcpy(ptr, "_normals");
			strcat(ptr, ".dem");
		}
		else
		{
			strcat(outfile, "_normals");
			strcat(outfile, ".dem");
		}

		Write_Debug("Output normals file: %s", outfile);

		normfile = fopen(outfile, "w+b");
		if (normfile != NULL)
		{
			width = hdr->ncols;
			height = hdr->nrows;
			totalrowbytes = hdr->totalrowbytes;
			ncols = hdr->ncols;
			nodata = hdr->nodata;

			row[0] = malloc((hdr->ncols + 2) * sizeof(float));
			memset(row[0], 0, (hdr->ncols + 2) * sizeof(float));
			row[1] = malloc((hdr->ncols + 2) * sizeof(float));
			row[2] = malloc((hdr->ncols + 2) * sizeof(float));

			incolor = malloc(totalrowbytes);
			outcolor = malloc(totalrowbytes);

			fread(incolor, totalrowbytes, 1, hdrfile);
			for (j=0 ; j < width ; j++)
			{
				if (incolor[j] == nodata)
				{
					row[1][j+1] = 0.0f;
				}
				else
				{
					row[1][j+1] = (float)incolor[j];
				}
			}

			row[1][0] = row[1][ncols];
			row[1][ncols+1] = row[1][1];

			height -= 1;
			for (i=0 ; i < height ; i++)
			{
				fread(incolor, totalrowbytes, 1, hdrfile);
				for (j=0 ; j < width ; j++)
				{
					if (incolor[j] == nodata)
					{
						row[2][j+1] = 0.0f;
					}
					else
					{
						row[2][j+1] = (float)incolor[j];
					}
				}

				row[2][0] = row[2][ncols];
				row[2][ncols+1] = row[2][1];

				CreateNormals(row, outcolor, hdr);

				fwrite(outcolor, totalrowbytes, 1, normfile);

				percent = (long)((float)i / (float)height * 100.0f);
				SendMessage(pbhwnd, PBM_SETPOS, percent, 0);
			}

			memset(row[2], 0, (hdr->ncols + 2) * sizeof(float));
			CreateNormals(row, outcolor, hdr);

			fwrite(outcolor, totalrowbytes, 1, normfile);

			free(incolor);
			free(outcolor);

			free(row[0]);
			free(row[1]);
			free(row[2]);

			fclose(normfile);
		}

		fclose(hdrfile);
	}

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	return(TRUE);
}

static BOOL DemToBmp(char *filename, LPDEMHDR hdr)
{
char				outfile[256], *ptr;
int					bits;
short				*incolor;
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

	hdrfile = fopen(outfile, "rb");
	if (hdrfile != NULL)
	{
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

		Write_Debug("Output bitmap file: %s", outfile);

		bmpfile = fopen(outfile, "w+b");
		if (bmpfile != NULL)
		{
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

			incolor = malloc(hdr->totalrowbytes);
			outcolor = malloc(twidth);
			height -= 1;
			for (i=height ; i >= 0 ; i--)
			{
				fseek(bmpfile, bits + i * twidth, SEEK_SET);
				fread(incolor, hdr->totalrowbytes, 1, hdrfile);
				for (j=0 ; j < width ; j++)
				{
					if (incolor[j] == hdr->nodata)
					{
						outcolor[j] = 0;
					}
					else
					{
						outcolor[j] = (incolor[j] - min) * 254 / (max - min) + 1;
					}
				}

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

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	return(TRUE);
}

DWORD WINAPI convertMap(void *dummy)
{
char			infile[256], outfile[256], hdrfile[256], *ptr;
OPENFILENAME	ofn;
DEMHDR			inhdr, outhdr;

	threadsactive += 1;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = myhwnd;
	ofn.hInstance = hinst;
	ofn.lpstrFilter = "DEM Files (*.dem)\0*.dem\0All Files (*.*)\0*.*\0\0";
	ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	strcpy(infile, "");
	ofn.lpstrFile = infile;
	ofn.nMaxFile = 256;
	ofn.lpstrDefExt = ".dem";
	if (GetOpenFileName(&ofn))
	{
		strcpy(hdrfile, infile);
		ptr = strrchr(hdrfile, '.');
		if (ptr != NULL)
		{
			strcpy(ptr, ".hdr");
		}

		Write_Debug("File to convert: %s", hdrfile);

		memset(&inhdr, 0, sizeof(inhdr));
		if (!ReadHdrFile(hdrfile, &inhdr))
		{
			threadsactive -= 1;
			return(0);
		}

		if (inhdr.nbits != 16)
		{
			Write_Debug("Cannot process a file with nbit value of %d", inhdr.nbits);
			threadsactive -= 1;
			return(0);
		}

		strcpy(outfile, infile);

		ptr = strrchr(outfile, '.');
		if (ptr != NULL)
		{
			*ptr = 0;
		}

		strcat(outfile, "_dc.dem");

		Write_Debug("Output file: %s", outfile);

#ifndef RECREATE_NORMALS

		memset(&outhdr, 0, sizeof(outhdr));
		if (!ConvertFile(infile, outfile, &inhdr, &outhdr))
		{
			threadsactive -= 1;
			return(0);
		}

		if (!WriteHdrFile(outfile, &outhdr))
		{
			threadsactive -= 1;
			return(0);
		}

#else

		ptr = strrchr(outfile, '.');
		if (ptr)
		{
			strcpy(ptr, ".hdr");
		}
		else
		{
			strcat(outfile, ".hdr");
		}

		ReadHdrFile(outfile, &outhdr);

#endif

		if (!DemToNormals(outfile, &outhdr))
		{
			threadsactive -= 1;
			return(0);
		}

#ifndef RECREATE_NORMALS

		DemToBmp(outfile, &outhdr);

#endif

		Write_Debug("Processing complete");

		PlaySound("SystemExclamation",NULL ,SND_ASYNC|SND_ALIAS);
	}

	threadsactive -= 1;

	return(1);
}
