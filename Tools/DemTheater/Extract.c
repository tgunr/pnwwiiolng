// extract.c

#define NONLS
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <commctrl.h>
#include <mmsystem.h>
//#define _WIN32_DCOM
#include <objbase.h>
#include <shlobj.h>
#define BIF_USENEWUI	0x40

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

} DEMHDR, *LPDEMHDR;

// static data
static int				min, max;
static BITMAPINFOHEADER	bmi;
static BITMAPFILEHEADER	bmf;
static RGBQUAD			rgb;
static int				bmphdrsize, bmpx, bmpy, width, height;
static FILE				*bmpfile;
static DEMHDR			hdr;

// defines
#define SUPERCELL_SIZE_X		33
#define SUPERCELL_SIZE_X1		32
#define SUPERCELL_SIZE_Y		33
#define SUPERCELL_SIZE_Y1		32
#define SUPERCELL_SIZE_XY		(SUPERCELL_SIZE_X * SUPERCELL_SIZE_Y)
#define SUPERCELL_WIDTH			(SUPERCELL_SIZE_X1 * hdr.xdim)
#define SUPERCELL_HEIGHT		(SUPERCELL_SIZE_Y1 * hdr.ydim)
#define SUPERCELL_ARC_X			0.266667
#define SUPERCELL_ARC_Y			0.2133336
#define MAX_LATITUDE			70.1875
#define MAX_LONGITUDE			180.0

#define TF_FLAG_DATA_RLE		0x0001
#define TF_FLAG_NORMALS_RLE		0x0002

// typedefs
typedef struct tagTERRAINFILE
{
	WORD	company;		// Company initials
	WORD	filetype;		// File type (object, terrain, etc.)
	WORD	version;		// File version in Major/Minor (MSB/LSB) format
	WORD	flags;			// Flags representing data options
	DWORD	dataversion;	// Data version using simple incremental technique
	DWORD	length;			// Length of data

} TERRAINFILE, *LPTERRAINFILE;


static BOOL ReadHdrFile(char *filename, LPDEMHDR hdr)
{
char	line[256];
size_t	i;
long	length;
FILE	*river;

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
		else
		if (strncmp(line, "MIN", 3) == 0)
		{
			for (i=3 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			min = atol(&line[i]);
		}
		else
		if (strncmp(line, "MAX", 3) == 0)
		{
			for (i=3 ; i < strlen(line) ; i++)
			{
				if (!isspace(line[i]))
				{
					break;
				}
			}

			max = atol(&line[i]);
		}
	}
	while (TRUE);

	fclose(river);

	Write_Debug("MIN = %d", min);
	Write_Debug("MAX = %d", max);

	return(TRUE);
}

static BOOL RLEToData(short inlen, short *indata, short *outlen, short *outdata)
{
short		runlen, rundata;

	*outlen = 0;
	while (inlen > 0)
	{
		runlen = *indata++;
		rundata = *indata++;
		inlen -= 2;
		*outlen += runlen;
		while (runlen)
		{
			*outdata++ = rundata;
			runlen -= 1;
		}
	}

	return(TRUE);
}

static BOOL CopyDtlFile(char *filename, char *srcpath, char *dstpath)
{
BOOL		retval=TRUE;
char		*ptr, fullpath[MAX_PATH];
BYTE		nil;
short		*pdem, data[SUPERCELL_SIZE_XY], len;
int			i, j;
long		length;
FILE		*stream, *river;
LPTERRAINFILE	ptf;

	strcpy(fullpath, srcpath);
	strcat(fullpath, filename);
	stream = fopen(fullpath, "rb");
	if (stream == NULL)
	{
		return(TRUE);
	}

	strcpy(fullpath, dstpath);
	strcat(fullpath, filename);
	river = fopen(fullpath, "w+b");
	if (river == NULL)
	{
		fclose(stream);
		return(FALSE);
	}

	fseek(stream, 0, SEEK_END);
	length = ftell(stream);
	fseek(stream, 0, SEEK_SET);

	ptr = malloc(length);
	if (ptr != NULL)
	{
		fread(ptr, length, 1, stream);
		ptf = (LPTERRAINFILE)ptr;
		pdem = (short*)(ptr + sizeof(TERRAINFILE));
		if (ptf->flags & TF_FLAG_DATA_RLE)
		{
			len = *pdem / 2;
			RLEToData(len, pdem + 1, &len, data);
			pdem = (short*)data;
		}
		else
		{
			pdem += 1;
		}

		for (i=0 ; i < SUPERCELL_SIZE_X1 ; i++, pdem++)
		{
			fseek(bmpfile, bmphdrsize + (bmpy - i) * width + bmpx, SEEK_SET);
			for (j=0 ; j < SUPERCELL_SIZE_X1 ; j++, pdem++)
			{
				if (*pdem == -9999)
				{
					nil = 0;
				}
				else
				{
					nil = (*pdem - min) * 254 / (max - min) + 1;
				}

				fwrite(&nil, 1, 1, bmpfile);
			}
		}

		fwrite(ptr, length, 1, river);
		free(ptr);
	}
	else
	{
		retval = FALSE;
	}

	fclose(river);
	fclose(stream);

	return(retval);
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		Write_Debug("Indicates the browse dialog box has finished initializing.\n"
			" The lParam value is zero.");
		break;
	case BFFM_SELCHANGED:
		Write_Debug("Indicates the selection has changed.\n"
			" The lParam parameter points to the item identifier list for the newly selected item.");
		break;
	case BFFM_VALIDATEFAILED:
		Write_Debug("Version 4.71.\n"
			" Indicates the user typed an invalid name into the edit box of the browse dialog.\n"
			" The lParam parameter is the address of a character buffer that contains the invalid name.\n"
			" An application can use this message to inform the user that the name entered was not valid.\n"
			" Return zero to allow the dialog to be dismissed or nonzero to keep the dialog displayed.");
		break;
	}

	return(0);
}

BOOL GetDirectory(char *prompt, char *path)
{
BOOL			retval=TRUE;
char			outfile[MAX_PATH];
BROWSEINFO		binfo;
LPITEMIDLIST	pidl;
LPMALLOC		pIMalloc;

	if (S_OK != CoInitialize(NULL))
	{
		Write_Debug("CoInitialize failed");
		return(FALSE);
	}

	if (NOERROR == SHGetMalloc(&pIMalloc))
	{
		memset(&binfo, 0, sizeof(BROWSEINFO));
		binfo.hwndOwner			= myhwnd;
		binfo.pidlRoot			= NULL;
		binfo.pszDisplayName	= outfile;
		binfo.lpszTitle			= prompt;
		binfo.ulFlags			= BIF_EDITBOX|BIF_USENEWUI;
		binfo.lpfn				= BrowseCallbackProc;
		binfo.lParam			= 0;
		binfo.iImage			= 0;

		pidl = SHBrowseForFolder(&binfo);
		if (pidl != NULL)
		{
			if (!SHGetPathFromIDList(pidl, path))
			{
				Write_Debug("SHGetPathFromIDList failed");
				retval = FALSE;
			}

			pIMalloc->lpVtbl->Free(pIMalloc, pidl);
		}
		else
		{
			Write_Debug("SHBrowseForFolder was aborted");
			retval = FALSE;
		}

		pIMalloc->lpVtbl->Release(pIMalloc);
	}
	else
	{
		Write_Debug("SHGetMalloc failed");
		retval = FALSE;
	}

	CoUninitialize();

	if (retval)
	{
		Write_Debug("Directory chosen: %s", path);
	}

	return(retval);
}

DWORD WINAPI extractMap(void *dummy)
{
char				filename[MAX_PATH], srcpath[MAX_PATH], dstpath[MAX_PATH], path[MAX_PATH];
char				*ptr, hdrfile[MAX_PATH];
long				present, total, percent;
DWORD				retval=1;
int					i, j, scl, sct, scr, scb;
double				x1, y1, x2, y2;
LPDEMDETAIL			detail;
OPENFILENAME		ofn;
FILE				*river;

	threadsactive += 1;

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	detail = dummy;

	y1 =  (double)detail->ulnsdeg;
	y1 += (double)detail->ulnsmin / 60.0;
	y1 += (double)detail->ulnssec / 3600.0;
	x1 =  (double)detail->ulewdeg;
	x1 += (double)detail->ulewmin / 60.0;
	x1 += (double)detail->ulewsec / 3600.0;
	Write_Debug("Upper left is at %f x %f\r\n", x1, y1);

	y2 =  (double)detail->lrnsdeg;
	y2 += (double)detail->lrnsmin / 60.0;
	y2 += (double)detail->lrnssec / 3600.0;
	x2 =  (double)detail->lrewdeg;
	x2 += (double)detail->lrewmin / 60.0;
	x2 += (double)detail->lrewsec / 3600.0;
	Write_Debug("Lower right is at %f x %f\r\n", x2, y2);

	// Round up
	y1 += SUPERCELL_ARC_Y / 2.0;
	if (y1 > MAX_LATITUDE)
	{
		y1 = MAX_LATITUDE;
		Write_Debug("Maximum latitude was exceeded. Reset latitude to fabs(%f)", y1);
	}

	if (detail->ulns == 'S')
	{
		y1 = -y1;
	}

	// Round up
	x1 += SUPERCELL_ARC_X / 2.0;
	if (x1 > MAX_LONGITUDE)
	{
		x1 = MAX_LONGITUDE;
		Write_Debug("Maximum longitude was exceeded. Reset longitude to fabs(%f)", x1);
	}

	if (detail->ulew == 'W')
	{
		x1 = -x1;
	}

	// Do NOT round up
	if (y2 > MAX_LATITUDE)
	{
		y2 = MAX_LATITUDE;
		Write_Debug("Maximum latitude was exceeded. Reset latitude to fabs(%f)", y2);
	}

	if (detail->lrns == 'S')
	{
		y2 = -y2;
	}

	// Do NOT round up
	if (x2 > MAX_LONGITUDE)
	{
		x2 = MAX_LONGITUDE;
		Write_Debug("Maximum longitude was exceeded. Reset longitude to fabs(%f)", x2);
	}

	if (detail->lrew == 'W')
	{
		x2 = -x2;
	}

	scl = (int)(x1 / SUPERCELL_ARC_X);
	sct = (int)(y1 / SUPERCELL_ARC_Y);
	scr = (int)(x2 / SUPERCELL_ARC_X);
	scb = (int)(y2 / SUPERCELL_ARC_Y);

	present = 0;
	total = sct - scb;

	if (GetDirectory("Select the terrain source folder", srcpath))
	{
		if (GetDirectory("Select or create the output terrain folder", dstpath))
		{
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = myhwnd;
			ofn.hInstance = hinst;
			ofn.lpstrFilter = "DEM Header Files (*.hdr)\0*.hdr\0All Files (*.*)\0*.*\0\0";
			ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
			strcpy(hdrfile, "");
			ofn.lpstrFile = hdrfile;
			ofn.nMaxFile = 256;
			ofn.lpstrDefExt = ".dem";
			if (GetOpenFileName(&ofn))
			{
				if (!ReadHdrFile(hdrfile, &hdr))
				{
					min = 0;
					max = 12000;
				}

				ptr = strrchr(dstpath, '\\');
				if (ptr != NULL)
				{
					ptr += 1;
					strcpy(hdrfile, ptr);
				}

				Write_Debug("DEM min = %d, max = %d", min, max);

				Write_Debug("Output bitmap file: %s", path);

				strcat(dstpath, "\\");
				strcpy(path, dstpath);
				strcat(path, hdrfile);
				strcat(path, ".bmp");

				bmpfile = fopen(path, "w+b");
				if (bmpfile != NULL)
				{
					//setvbuf(bmpfile, NULL, _IOFBF, hdr->ncols);
					width = (scr - scl + 1) * 32;
					height = (sct - scb + 1) * 32;

					hdr.ncols = width + 1;
					hdr.nrows = height + 1;

					memset(&bmf, 0, sizeof(BITMAPFILEHEADER));
					bmf.bfType = 'M' << 8 | 'B';
					bmf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + height * width;
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
					bmi.biSizeImage = height * width;
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

					bmphdrsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
					bmpy = height - 1;
					hdr.ulxmap = scl * SUPERCELL_WIDTH;
					hdr.ulymap = sct * SUPERCELL_HEIGHT;
					for (i=sct ; i >= scb && retval ; i--)
					{
						bmpx = 0;
						wsprintf(path, "%s\\%04x\\", srcpath, (WORD)i);
						for (j=scl ; j <= scr ; j++)
						{
							wsprintf(filename, "%04x_%04x.dtl", (WORD)j, (WORD)i);
							if (!CopyDtlFile(filename, path, dstpath))
							{
								retval = FALSE;
								break;
							}

							percent = (long)((float)(sct - i) / (float)total * 100.0f);
							SendMessage(pbhwnd, PBM_SETPOS, percent, 0);

							bmpx += 32;
						}

						bmpy -= 32;
					}

					fclose(bmpfile);
				}

				strcpy(path, dstpath);
				strcat(path, hdrfile);
				strcat(path, ".hdr");
				river = fopen(path, "wb");
				if (river != NULL)
				{
					hdr.bandrowbytes = hdr.ncols * hdr.nbits / 8;
					hdr.totalrowbytes = hdr.ncols * hdr.nbits / 8;
					hdr.bandgapbytes = 0;
					fprintf(river, "BYTEORDER     I\r\n");
					fprintf(river, "LAYOUT        %s\r\n", hdr.layout);
					fprintf(river, "NROWS         %d\r\n", hdr.nrows);
					fprintf(river, "NCOLS         %d\r\n", hdr.ncols);
					fprintf(river, "NBANDS        %d\r\n", hdr.nbands);
					fprintf(river, "NBITS         %d\r\n", hdr.nbits);
					fprintf(river, "BANDROWBYTES  %d\r\n", hdr.bandrowbytes);
					fprintf(river, "TOTALROWBYTES %d\r\n", hdr.totalrowbytes);
					fprintf(river, "BANDGAPBYTES  %d\r\n", hdr.bandgapbytes);
					fprintf(river, "NODATA        %d\r\n", hdr.nodata);
					fprintf(river, "ULXMAP        %f\r\n", hdr.ulxmap);
					fprintf(river, "ULYMAP        %f\r\n", hdr.ulymap);
					fprintf(river, "XDIM          %f\r\n", hdr.xdim);
					fprintf(river, "YDIM          %f\r\n", hdr.ydim);
					fclose(river);
				}

				PlaySound("SystemExclamation",NULL ,SND_ASYNC|SND_ALIAS);
			}
		}
	}

	free(detail);

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	threadsactive -= 1;

	return(retval);
}
