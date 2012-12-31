// extract.c

#define NONLS
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <direct.h>
#include <io.h>
#include <objbase.h>
#include <shlobj.h>
#define BIF_USENEWUI	0x40
#include "dbug.h"
#include "barewnd.h"

// global data
extern int			threadsactive;
extern BOOL			threadabort;
extern HINSTANCE	hinst;
extern HWND			myhwnd, pbhwnd;

// Defines
#define SUPERCELL_SIZE_X		33
#define SUPERCELL_SIZE_X1		32
#define SUPERCELL_SIZE_Y		33
#define SUPERCELL_SIZE_XY		(SUPERCELL_SIZE_X * SUPERCELL_SIZE_Y)

#define PLAYNET_COMPANY			('P' + ('N' << 8))		// short for Playnet
#define PLAYNET_TERRAINFILE		('T' + ('R' << 8))		// short for Terrain (File Type)

// NOTE: Be sure and change a version element every time you change the object file data or structure!!
#define TERRAINFILE_MAJOR		2
#define TERRAINFILE_MINOR		2
#define TERRAINFILE_VERSION		((TERRAINFILE_MAJOR << 8) + TERRAINFILE_MINOR)	// version; MSB is major, LSB is minor

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
static const char	rootpath[]={"\\demfiles\\"};
static char			trnpath[MAX_PATH];
static int			defaultcells;
static int			min, max;

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

			min = atoi(&line[i]);
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

			max = atoi(&line[i]);
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
	Write_Debug("MIN = %d", min);
	Write_Debug("MAX = %d", max);

	return(TRUE);
}

static BOOL DataToRLE(short inlen, short *indata, short *outlen, short *outdata)
{
short		runlen, rundata;

	*outlen = 0;
	while (inlen > 0)
	{
		runlen = 1;
		rundata = *indata++;
		inlen -= 1;
		while(rundata == *indata)
		{
			if (inlen == 0)
			{
				break;
			}

			runlen += 1;
			indata += 1;
			inlen -= 1;
		}

		*outlen += 2;
		*outdata++ = runlen;
		*outdata++ = rundata;
	}

	return(TRUE);
}

static BOOL OutputDtlFile(int x, int y, short scx, short scy, LPDEMHDR hdr, FILE *dem, FILE *norm)
{
char		filename[256];
int			nbytes, ncols, offset, sor;
int			i, totalrowbytes;
short		len, nodata, *pdata, data[SUPERCELL_SIZE_Y][SUPERCELL_SIZE_X], outdata[SUPERCELL_SIZE_XY * 2];
TERRAINFILE	trnfile;
FILE		*stream;

	nbytes = hdr->nbits / 8;
	nodata = hdr->nodata;
	totalrowbytes = hdr->totalrowbytes;
	ncols = hdr->ncols;

	sor = y * totalrowbytes;
	offset = sor + x * nbytes;

	for (i=0 ; i < SUPERCELL_SIZE_Y ; i++)
	{
		fseek(dem, offset, SEEK_SET);
		fread(&data[i], nbytes * SUPERCELL_SIZE_X1, 1, dem);
		if ((x + SUPERCELL_SIZE_X1) >= ncols)
		{
			fseek(dem, sor, SEEK_SET);
		}

		fread(&data[i][SUPERCELL_SIZE_X1], nbytes, 1, dem);

		sor += totalrowbytes;
		offset += totalrowbytes;
	}

	pdata = (short*)&data;
	for (i=0 ; i < SUPERCELL_SIZE_XY ; i++)
	{
		if (pdata[i] != nodata)
		{
			break;
		}
	}

	if (i == SUPERCELL_SIZE_XY)
	{
		defaultcells += 1;
		if (defaultcells == 1)
		{
			wsprintf(filename, "%s\\Default.dtl", trnpath);
			stream = fopen(filename, "w+b");
			if (stream == NULL)
			{
				Write_Debug("Unable to open terrain file '%s'", filename);
				return(FALSE);
			}

			memset(&trnfile, 0, sizeof(TERRAINFILE));
			trnfile.company = PLAYNET_COMPANY;
			trnfile.filetype = PLAYNET_TERRAINFILE;
			trnfile.version = TERRAINFILE_VERSION;
			trnfile.length = 3 * sizeof(WORD);

			DataToRLE(SUPERCELL_SIZE_XY, (short*)data, &len, outdata);
			if (len > SUPERCELL_SIZE_XY)
			{
				len = SUPERCELL_SIZE_XY * nbytes;
			}
			else
			{
				len *= nbytes;
				memcpy(data, outdata, len);
				trnfile.flags |= TF_FLAG_DATA_RLE;
			}

			trnfile.length += sizeof(WORD) + len;
			fseek(stream, sizeof(TERRAINFILE), SEEK_SET);
			fwrite(&len, 2, 1, stream);
			fwrite(data, len, 1, stream);

			sor = y * totalrowbytes;
			offset = sor + x * nbytes;

			for (i=0 ; i < SUPERCELL_SIZE_Y ; i++)
			{
				fseek(norm, offset, SEEK_SET);
				fread(&data[i], nbytes * SUPERCELL_SIZE_X1, 1, norm);
				if ((x + SUPERCELL_SIZE_X1) >= ncols)
				{
					fseek(norm, sor, SEEK_SET);
				}

				fread(&data[i][SUPERCELL_SIZE_X1], nbytes, 1, norm);

				sor += totalrowbytes;
				offset += totalrowbytes;
			}

			DataToRLE(SUPERCELL_SIZE_XY, (short*)data, &len, outdata);
			if (len > SUPERCELL_SIZE_XY)
			{
				len = SUPERCELL_SIZE_XY * nbytes;
			}
			else
			{
				len *= nbytes;
				memcpy(data, outdata, len);
				trnfile.flags |= TF_FLAG_NORMALS_RLE;
			}

			trnfile.length += sizeof(WORD) + len;
			fwrite(&len, 2, 1, stream);
			fwrite(data, len, 1, stream);

			i = 0;
			fwrite(&i, 2, 1, stream);
			fwrite(&i, 2, 1, stream);
			fwrite(&i, 2, 1, stream);

			fseek(stream, 0, SEEK_SET);
			fwrite(&trnfile, sizeof(TERRAINFILE), 1, stream);

			fclose(stream);
		}
	}
	else
	{
		wsprintf(filename, "%s\\%04x\\%04x_%04x.dtl", trnpath, (UINT16)scy, (UINT16)scx, (UINT16)scy);
		stream = fopen(filename, "w+b");
		if (stream == NULL)
		{
			Write_Debug("Unable to open terrain file '%s'", filename);
			return(FALSE);
		}

		memset(&trnfile, 0, sizeof(TERRAINFILE));
		trnfile.company = PLAYNET_COMPANY;
		trnfile.filetype = PLAYNET_TERRAINFILE;
		trnfile.version = TERRAINFILE_VERSION;
		trnfile.length = 3 * sizeof(WORD);

		DataToRLE(SUPERCELL_SIZE_XY, (short*)data, &len, outdata);
		if (len > SUPERCELL_SIZE_XY)
		{
			len = SUPERCELL_SIZE_XY * nbytes;
		}
		else
		{
			len *= nbytes;
			memcpy(data, outdata, len);
			trnfile.flags |= TF_FLAG_DATA_RLE;
		}

		trnfile.length += sizeof(WORD) + len;
		fseek(stream, sizeof(TERRAINFILE), SEEK_SET);
		fwrite(&len, 2, 1, stream);
		fwrite(data, len, 1, stream);

		sor = y * totalrowbytes;
		offset = sor + x * nbytes;

		for (i=0 ; i < SUPERCELL_SIZE_Y ; i++)
		{
			fseek(norm, offset, SEEK_SET);
			fread(&data[i], nbytes * SUPERCELL_SIZE_X1, 1, norm);
			if ((x + SUPERCELL_SIZE_X1) >= ncols)
			{
				fseek(norm, sor, SEEK_SET);
			}

			fread(&data[i][SUPERCELL_SIZE_X1], nbytes, 1, norm);

			sor += totalrowbytes;
			offset += totalrowbytes;
		}

		DataToRLE(SUPERCELL_SIZE_XY, (short*)data, &len, outdata);
		if (len > SUPERCELL_SIZE_XY)
		{
			len = SUPERCELL_SIZE_XY * nbytes;
		}
		else
		{
			len *= nbytes;
			memcpy(data, outdata, len);
			trnfile.flags |= TF_FLAG_NORMALS_RLE;
		}

		trnfile.length += sizeof(WORD) + len;
		fwrite(&len, 2, 1, stream);
		fwrite(data, len, 1, stream);

		i = 0;
		fwrite(&i, 2, 1, stream);
		fwrite(&i, 2, 1, stream);
		fwrite(&i, 2, 1, stream);

		fseek(stream, 0, SEEK_SET);
		fwrite(&trnfile, sizeof(TERRAINFILE), 1, stream);

		fclose(stream);
	}

	return(TRUE);
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

static BOOL ReadDemFile(char *filename, LPDEMHDR hdr)
{
BOOL		retval=TRUE;
char		*ptr, dirname[256];
short		scx, scy;
int			x, y, height;
long		length, stime;
long		percent;
FILE		*stream, *river;

	stream = fopen(filename, "rb");
	if (stream == NULL)
	{
		Write_Debug("Could not open the DEM file for processing");
		return(FALSE);
	}

	ptr = strrchr(filename, '.');
	if (ptr != NULL)
	{
		strcpy(ptr, "_normals.dem");
	}
	else
	{
		strcat(filename, "_normals.dem");
	}

	river = fopen(filename, "rb");
	if (river == NULL)
	{
		Write_Debug("Could not open the DEM file for processing");
		return(FALSE);
	}

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	stime = GetTickCount();

	fseek(stream, 0, SEEK_END);
	length = ftell(stream);
	fseek(stream, 0, SEEK_SET);

	_mkdir(trnpath);

	scy = ((hdr->nrows - 1) / SUPERCELL_SIZE_X1 / 2) - 1;
	height = hdr->nrows - 1;
	for (y=0 ; y < hdr->nrows && retval ; y+=SUPERCELL_SIZE_X1)
	{
		wsprintf(dirname, "%s\\%04x", trnpath, (UINT16)scy);
		if (_access(dirname, 0))
		{
			if (_mkdir(dirname))
			{
				Write_Debug("Failed to create directory '%s'", dirname);
				retval = FALSE;
				break;
			}
		}

		scx = -hdr->ncols / SUPERCELL_SIZE_X1 / 2;
		for (x=0 ; x < hdr->ncols ; x+=SUPERCELL_SIZE_X1)
		{
			if (!OutputDtlFile(x, y, scx, scy, hdr, stream, river))
			{
				retval = FALSE;
				break;
			}

			scx += 1;
		}

		scy -= 1;

		percent = (long)((float)y / (float)height * 100.0f);
		SendMessage(pbhwnd, PBM_SETPOS, percent, 0);

		if (threadabort)
		{
			break;
		}
	}

	stime = GetTickCount() - stime;
	fclose(stream);

	Write_Debug("Processed in %ld ms\r\n", stime);

	SendMessage(pbhwnd, PBM_SETPOS, 0, 0);

	return(retval);
}

DWORD WINAPI extractMap(void *dummy)
{
BOOL			retval=TRUE;
char			*ptr, filename[256], outfile[256];
LPDEMHDR		hdr;
OPENFILENAME	ofn;

	threadsactive += 1;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = myhwnd;
	ofn.hInstance = hinst;
	ofn.lpstrFilter = "DEM Files (*.dem)\0*.dem\0All Files (*.*)\0*.*\0\0";
	ofn.Flags = OFN_HIDEREADONLY;

	strcpy(filename, "");
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 256;
	ofn.lpstrDefExt = ".dem";
	if (GetSaveFileName(&ofn))
	{
		if (GetDirectory("Select the terrain output folder", trnpath))
		{
			strcpy(outfile, ofn.lpstrFile);
			ptr = strrchr(outfile, '.');
			if (ptr != NULL)
			{
				strcpy(ptr, ".hdr");
			}
			else
			{
				strcat(outfile, ".hdr");
			}

			hdr = malloc(sizeof(DEMHDR));
			memset(hdr, 0, sizeof(DEMHDR));
			if (!ReadHdrFile(outfile, hdr))
			{
				retval = FALSE;
			}

			if (retval)
			{
				ptr = strrchr(outfile, '.');
				if (ptr != NULL)
				{
					strcpy(ptr, ".dem");
				}
				else
				{
					strcat(outfile, ".dem");
				}

				defaultcells = 0;
				if (!ReadDemFile(outfile, hdr))
				{
					retval = FALSE;
				}
				else
				{
					Write_Debug("Default terrain cell referenced %d times", defaultcells);
				}
			}

			free(hdr);
		}
		else
		{
			retval = FALSE;
		}

		PlaySound("SystemExclamation",NULL ,SND_ASYNC|SND_ALIAS);
	}
	else
	{
		retval = FALSE;
	}

	threadsactive -= 1;

	return(retval);
}
