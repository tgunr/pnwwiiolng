#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "trn/trnldr.h"
#include "trn/trnvars.h"
#include "trn/trntypes.h"
#include "ob/obdlite.h"
#include "ob/obslite.h"
#include "ob/obcs.h"
#include "ob/obpool.h"
#include "ld/ldmdl.h"
#include "ld/ldpix.h"
#include "rn/rnmain.h"
#include "locl/loclMain.h"
#include "math/math3d.h"
#include "strt/strtClientMessages.h"
#include "strt/strtTypes.h"
#include "strt/strtDTL.h"
#include "str/strpool.h"
#include "trn/trncd.h"
#include "teul/teulKit.h"
#include "client/clientStrtQueue.h"
#include <stack>
#include "trn/trncloud.h"
#include "ob/obMgr.h"
#include "sdm/BuildTerrain.h"
#include "sdm/sdmDefines.h"
#include "spt/sptmain.h"

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

typedef std::stack< TDTL_TREES* > TREE_POOL;
static TREE_POOL _treePool;

#include "shader/glslWater.h"

#define TRN_EXT					MERGEFILE_EXTENSION

#if platOS == OS_LINUX
	#include "sysUtilLinux.h"
#else
#endif

// defines
#define TRNLDR_MIN_LOAD_TIME			1
#define TRNLDR_MAX_LOAD_TIME			20
#define TRNLDR_MIN_FRAMES_PER_STATE		1
#define TRNLDR_MAX_FRAMES_PER_STATE		1

// typedefs
struct TUV
{
	FLOAT32			tu;
	FLOAT32			tv;
};


// statics
static TUV		tuvlist[4][9]={	{{0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.5f}, {1.0f, 0.0f}, {0.5f, 0.0f}},
								{{0.5f, 0.5f}, {1.0f, 0.0f}, {0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.5f}},
								{{0.5f, 0.5f}, {1.0f, 1.0f}, {1.0f, 0.5f}, {1.0f, 0.0f}, {0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f}},
								{{0.5f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.5f}, {1.0f, 0.0f}, {0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.5f}}};
static SINT8*	alttexname[ALT_TRN_NUMBER]={
											"dunes1.bmp",
											"water1.bmp",
											"dunes1.bmp",
											"farms1.bmp",
											"clear1.bmp",
											"forest1.bmp",
											"rocky1.bmp",
											"icy1.bmp"
											};
static SINT32	alttexid[ALT_TRN_NUMBER];
/*static SINT32	altitude[ALT_TRN_NUMBER]=	{
											ALT_TRN_BELOW,
											ALT_TRN_WATER,
											ALT_TRN_DUNES,
											ALT_TRN_FARMS,
											ALT_TRN_CLEAR,
											ALT_TRN_FOREST,
											ALT_TRN_ROCKY,
											ALT_TRN_ICY
											};*/

static LOGICAL	ObjectsWereAdded;

static LOGICAL FindObjectsInOctetSCLoading(UINT32* objnum, SINT32 sc, UINT32* objcnt, FLOAT32 horizonsquared);
static LOGICAL FindObjectsInOctetSCUnloading(UINT32* objnum, SINT32 sc, UINT32* objcnt, FLOAT32 hyster, FLOAT32 hysterFar);

#if(0)
static LOGICAL DataToRLE(short inlen, short* indata, short* outlen, short* outdata)
{
SINT16		runlen, rundata;

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
#endif

LOGICAL trnldrRLEToData(short inlen, short* indata, short* outlen, short* outdata)
{
SINT16		runlen, rundata;

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

LOGICAL trnldrReadHdrFile(const char* filename, DEMHDR* const hdr)
{
SINT8	line[256];
UINT32	i;
SINT32	river;

	river = fioOpen(filename);
	if (river == -1)
	{
		return(FALSE);
	}

	do
	{
		if (fioGetString(river, line, 256) == 0)
		{
			break;
		}

		if (strncmp(line, "BYTEORDER", 9) == 0)
		{
			for (i=9; i < strlen(line); i++)
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
			for (i=6; i < strlen(line); i++)
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
			for (i=5; i < strlen(line); i++)
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
			for (i=5; i < strlen(line); i++)
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
			for (i=6; i < strlen(line); i++)
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
			for (i=5; i < strlen(line); i++)
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
			for (i=12; i < strlen(line); i++)
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
			for (i=13; i < strlen(line); i++)
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
			for (i=12; i < strlen(line); i++)
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
			for (i=6; i < strlen(line); i++)
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
			for (i=6; i < strlen(line); i++)
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
			for (i=6; i < strlen(line); i++)
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
			for (i=4; i < strlen(line); i++)
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
			for (i=4; i < strlen(line); i++)
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

	fioClose(river);

	return(TRUE);
}


/*
	Reduces edit list to 256 entries and orders the list
*/
void trnldrReduceEditList(TERRAIN_DETAIL* td)
{
SINT32		i, j, k;

	Write_Debug("Removing unused edit list entries");
	if (td->trindx > SUPERCELL_OCTETS_XY)
	{
		for (i=0; i < SUPERCELL_OCTETS_XY; i++)
		{
			for (j=SUPERCELL_OCTETS_XY; j < td->trindx; j++)
			{
				if (td->trinfo[i].txndx == (td->trinfo[j].tonum - 1) &&
					td->trinfo[i].octet == td->trinfo[j].octnum)
				{
					td->trinfo[i].txndx = td->trinfo[j].txndx;
					td->trinfo[i].octet = td->trinfo[j].octet;
					td->trinfo[i].ortdrg = td->trinfo[j].ortdrg;
					td->trinfo[i].ruftrn = td->trinfo[j].ruftrn;
					td->trinfo[j].octet = (UINT16)-1;
				}
				else
				if (td->trinfo[i].txndx == (td->trinfo[j].tonum - 1) &&
					td->trinfo[i].octet > td->trinfo[j].octnum)
				{
					td->trinfo[i].octet -= 1;
				}
			}
		}

		td->trindx = SUPERCELL_OCTETS_XY;
	}

	Write_Debug("Ordering edit list entries");
	for (i=0; i < td->tricnt; i++)
	{
		j = 0;
		for (k=0; k < SUPERCELL_OCTETS_XY; k++)
		{
			if (td->trinfo[k].txndx == i)
			{
				td->trinfo[k].octet = j++;
			}
		}

		if (j == 0)
		{
			Write_Debug("Texture '%s' at index %d was not referenced", td->triname[i], i);
		}
	}

	Write_Debug("Edit list has been minimized and ordered");
}


static LOGICAL GetFullObjectBBox(OBJECT* lptop, piVECTOR* bbox)
{
SINT32			i;
piVectorConstPtr pv;
OBJECT*		lpatt;

	while (lptop != NULL)
	{
		if (lptop->child != NULL)
		{
			if (!GetFullObjectBBox(lptop->child, bbox))
			{
				return(FALSE);
			}
		}

		if (!(lptop->flags & OBJECT_NOTOBJECT))
		{
			lpatt = lptop;
			while (lpatt != NULL)
			{
				pv = lptop->RenderPositions();
				for (i=0; i < lptop->numverts; i++, ++pv)
				{
					if (pv->x < bbox[0].x)
					{
						bbox[0].x = pv->x;
					}

					if (pv->x > bbox[1].x)
					{
						bbox[1].x = pv->x;
					}

					if (pv->y < bbox[0].y)
					{
						bbox[0].y = pv->y;
					}

					if (pv->y > bbox[1].y)
					{
						bbox[1].y = pv->y;
					}

					if (pv->z < bbox[0].z)
					{
						bbox[0].z = pv->z;
					}

					if (pv->z > bbox[1].z)
					{
						bbox[1].z = pv->z;
					}
				}

				lpatt = lpatt->attached;
			}
		}

		lptop = lptop->sibling;
	}

	return(TRUE);
}


LOGICAL trnldrInit(void)
{
SINT8	filename[MAX_PATH], mapname[64];
SINT32	i;

	ObjectsWereAdded = FALSE;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		memset(&trndtl[i].trnstate, 0, sizeof(TRNSTATEBLOCK));
		trndtl[i].trnstate.state = TRN_STATE_WAIT;
	}

	for (i=0; i < ALT_TRN_NUMBER; i++)
	{
		strcpy(filename, TERRAIN_PREPATH);
		strcat(filename, alttexname[i]);
		alttexid[i] = ldpixLoadImageFileToSurface(filename, colorMAGENTA, LDPIX_MIPMAP);
		if (!piIsTexture(alttexid[i], 0))
		{
			return(FALSE);
		}
	}

	trnmainGetMapName(mapname);

	strcpy(filename, TERRAIN_PREPATH);
	strcat(filename, mapname);
	strcat(filename, ".hdr");

	if (!trnldrReadHdrFile(filename, &hdr))
	{
		return(FALSE);
	}

#ifdef USE_VSS
	{
	SINT8		work[512], curpath[_MAX_PATH], workpath[_MAX_PATH], batchfile[_MAX_PATH];

		GetCurrentDirectory(_MAX_PATH, curpath);
		sprintf(work, "%s", PREPATH);
		strcpy(work, &work[1]);
		sprintf(workpath, "%s%s%s", curpath, work, TERRAIN_PREPATH);
		SetCurrentDirectory(workpath);

		strcpy(batchfile, ".\\");
		strcat(batchfile, VSS_SUBDIRECTORY);

		if (OS_ACCESS(batchfile, 00) != 0)
		{
			OS_MKDIR(batchfile);
		}

		SetCurrentDirectory(curpath);
	}
#endif

	return(TRUE);
}

void trnldrKill(void)
{
SINT32	i;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		trnldrReleaseSupercell(i);
		if (trndtl[i].terrain != NULL)
		{
  			obpoolFreeObjectCallback(trndtl[i].terrain);
			obpoolFreeObjectCallback(trndtl[i].terrainorig);
			memFree(trndtl[i].triname[0]);
			memset(&trndtl[i], 0, sizeof(TERRAIN_DETAIL));
		}
	}

	for (i=0; i < ALT_TRN_NUMBER; i++)
	{
		piFreeMipTextures(alttexid[i]);
	}

    while(!_treePool.empty())
    {
        TDTL_TREES* t = _treePool.top();
        _treePool.pop();
        delete [] t;
    }
}

SINT32 trnldrGetTLOGsForSupercell(SINT16 scx, SINT16 scy)
{
SINT32	i;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if (activeTerrainDetail[i] &&
			trndtl[i].scx == scx &&
			trndtl[i].scy == scy)
		{
			return(trndtl[i].tobndx);
		}
	}

	return(0);
}

static LOGICAL ReleaseSupercellTextureObjects(TERRAIN_DETAIL* td)
{
SINT32		i;

	if (td == NULL)
	{
		return(FALSE);
	}

    logTimerBeginMacro();

	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			obpoolFreeObjectCallback(td->nonreplacementtiles[i]);
			td->nonreplacementtiles[i] = NULL;
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL ReleaseSupercellTextureObjectsSequenced(SINT32 sc)
{
SINT32		i;
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*	tsb;


	td = &trndtl[sc];
	tsb = &td->trnstate;

	for (i=tsb->varone; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			obpoolFreeObjectCallback(td->nonreplacementtiles[i]);
			td->nonreplacementtiles[i] = NULL;
		}

		tsb->varone += 1;

		return(FALSE);
	}


	return(TRUE);
}

LOGICAL trnldrReleaseTerrainObject(OBJECT* lpobj, TERRAIN_DETAIL* td)
{
	if (lpobj == NULL)
	{
		return(FALSE);
	}

    logTimerBeginMacro();

	if (deletion != NULL)
	{
		switch(lpobj->modelInstance->GetModelType())
		{
		case ModelTypes::CLOUD:

				trnCloudDeRegisterCloudInstance(lpobj);
				break;

			default:

				break;
		}

        int idx = 0;

		deletion(lpobj, 0, 0, 0, NULL);
		trnUpdateStratData(td, lpobj, -1, FALSE);
	}

	obpoolFreeObjectCallback(lpobj);

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL ReleaseSupercellObjects(TERRAIN_DETAIL* td)
{
SINT32		i;//, place=0;
SINT8		filename[_MAX_PATH];

	if (td == NULL)
	{
		return(FALSE);
	}

    logTimerBeginMacro();

	for (i=0; i < td->tobndx; i++)
	{
	SCHAR*		ptr;
	OBJECT*	lpobj;

		sprintf(filename, "%s_%d_%d", td->trnobj[i].objname, (int)td->trnobj[i].pos.x, (int)td->trnobj[i].pos.y);

		// strip off the directory subname
		ptr = strrchr(filename,'\\');
		if (ptr != NULL)
		{
			strcpy(filename,++ptr);
		}

		lpobj = obmainFindTopmostObject(filename);
		if (!trnldrReleaseTerrainObject(lpobj, td))
		{
#if RELEASE_CLIENT == 0
			//Write_Debug("ReleaseSupercellObjects for supercell %d could not find object '%s' to remove", td->terrain->trnum, filename);
#endif
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL ReleaseSupercellObjectsSequenced(SINT32 sc)
{
SINT32		i;
SINT8		filename[_MAX_PATH];
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*	tsb;

	td = &trndtl[sc];
	tsb = &td->trnstate;

    FLOAT32 horizon, hyster, hysterFar;

    trnmainGetHorizon(&horizon);
	hyster = horizon * 1.1f;	// Hysterisis
	hyster *= hyster;			// To avoid sqrt
	hysterFar = hyster * 10.0f;

	while (FindObjectsInOctetSCUnloading((UINT32*)&i, sc, (UINT32*)&tsb->varone, hyster, hysterFar ))
	{
	SCHAR*		ptr;
	OBJECT*	lpobj;

		sprintf(filename, "%s_%d_%d", td->trnobj[i].objname, (int)td->trnobj[i].pos.x, (int)td->trnobj[i].pos.y);

		// strip off the directory subname
		ptr = strrchr(filename,'\\');
		if (ptr != NULL)
		{
			strcpy(filename,++ptr);
		}

		lpobj = obmainFindTopmostObject(filename);
		if (!trnldrReleaseTerrainObject(lpobj, td))
		{
			Write_Debug("ReleaseSupercellObjectsSequenced for supercell %d could not find object '%s' to remove", td->terrain->trnum, filename);
		}

		return(FALSE);
	}

	return(TRUE);
}

LOGICAL trnldrLoadNormals(OBJECT* tt, short normals[SUPERCELL_SIZE_XY])
{
SINT16		d, v;
SINT32		i, j, k, l, w, h;
piVectorPtr	po;
piVECTOR	n, pn[SUPERCELL_SIZE_XY], *pnt;

    logTimerBeginMacro();

	w = SUPERCELL_SIZE_X1;
	h = SUPERCELL_SIZE_Y1;

	pnt = pn;
	for (i=0; i < SUPERCELL_SIZE_XY; i++, pnt++)
	{
		d = normals[i];

		v = (d & SUPERCELL_NORMAL_MASK_X);
		v = (v / 2048);
		pnt->x = (FLOAT32)(v) / SUPERCELL_NORMAL_MAG_X;

		v = (d & SUPERCELL_NORMAL_MASK_Y);
		v = (v << 5);
		v = (v / 2048);
		pnt->y = (FLOAT32)(v) / SUPERCELL_NORMAL_MAG_Y;

		v = (d & SUPERCELL_NORMAL_MASK_Z);
		v = (v << 10);
		v = ((UINT16)v / 1024);
		pnt->z = -(FLOAT32)(v) / SUPERCELL_NORMAL_MAG_Z;

		//math3dFastNormalize(pnt);
	}

	po = tt->StaticNormals();
	for (i=0; i < h; i+=2)
	{
		for (j=0; j < w; j+=2)
		{
			// 0
			k = SUPERCELL_SIZE_Y1 - (i + 1);
			l = 1;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 1
			k = SUPERCELL_SIZE_Y1 - (i + 0);
			l = 0;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 2
			k = SUPERCELL_SIZE_Y1 - (i + 1);
			l = 0;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 3
			k = SUPERCELL_SIZE_Y1 - (i + 2);
			l = 0;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 4
			k = SUPERCELL_SIZE_Y1 - (i + 2);
			l = 1;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 5
			k = SUPERCELL_SIZE_Y1 - (i + 2);
			l = 2;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 6
			k = SUPERCELL_SIZE_Y1 - (i + 1);
			l = 2;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 7
			k = SUPERCELL_SIZE_Y1 - (i + 0);
			l = 2;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;

			// 8
			k = SUPERCELL_SIZE_Y1 - (i + 0);
			l = 1;
			n = pn[k * (w + 1) + j + l];
			po->x = n.x;
			po->y = n.y;
			po->z = n.z;
			po += 1;
		}
	}
	tt->SyncMutableData();

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL VerifyHeader(TERRAINFILE* const trnfile)
{
LOGICAL		retval=TRUE;

	if (trnfile->company != PLAYNET_COMPANY)
	{
		retval = FALSE;
		Write_Debug("DTL file does not have a valid file marker");
	}
	else
	{
#if RELEASE_CLIENT == 0
		if (trnfile->filetype != PLAYNET_MERGEFILE &&
			trnfile->filetype != PLAYNET_TERRAINFILE)
#else
		if (trnfile->filetype != PLAYNET_MERGEFILE)
#endif
		{
			retval = FALSE;
			Write_Debug("DTL file does not have a valid terrain marker");
		}
		else
		{
#if RELEASE_CLIENT == 0
			if (trnfile->version != MERGEFILE_VERSION &&
				trnfile->version != TERRAINFILE_VERSION)
#else
			if (trnfile->version != MERGEFILE_VERSION)
#endif
			{
				retval = FALSE;
				Write_Debug("DTL file has an invalid version");
			}
		}
	}

	return(retval);
}

LOGICAL trnldrLoadDTL(TERRAIN_DETAIL* td)
{
LOGICAL		retval=TRUE, ReadingSDM;
static SINT8		filename[_MAX_PATH];
static SINT16		data[SUPERCELL_SIZE_XY];
SINT32		hfile;
SINT16		tmpShort;
size_t		sizecheck;
static TERRAINFILE	trnfile;
SINT32     _isdefault;

    logTimerBeginMacro();

STRT_DTL_HEADER		sdh;

	// Clear flags for this terrain detail element
	td->flags &= TDFLAG_PERIPHERY;

    _isdefault = 0;


	sprintf(filename, "%s%04x_%04x%s", TERRAIN_PREPATH, (UINT16)td->scx, (UINT16)td->scy, TRN_EXT);

	hfile = fioOpen(filename);

	ReadingSDM = TRUE;

#if !defined(TERRAIN_EDITOR) && RELEASE_CLIENT == 0

	if (hfile == -1)
	{
		sprintf(filename, "%s%04x_%04x%s", TERRAIN_PREPATH, (UINT16)td->scx, (UINT16)td->scy, TERRAINFILE_EXTENSION);
		Write_Debug("Attempting to substitue DTL file (%s) for SDM", filename);
		hfile = fioOpen(filename);
		ReadingSDM = FALSE;
	}

#endif

	if (hfile == -1)
	{
		Write_Debug("Unable to open %s file (%s)", TRN_EXT, filename);

        // Change back to old archive to get this
        // default.dtl is not ####_####.dtl!

		sprintf(filename, "%sdefault%s", TERRAIN_PREPATH, TRN_EXT);
		hfile = fioOpen(filename);

		ReadingSDM = TRUE;

#if RELEASE_CLIENT == 0

		if (hfile == -1)
		{
			sprintf(filename, "%sdefault%s", TERRAIN_PREPATH, TERRAINFILE_EXTENSION);
			Write_Debug("Attempting to substitue DTL file (%s) for SDM", filename);
			hfile = fioOpen(filename);
			ReadingSDM = FALSE;
		}

#endif

		if (hfile == -1)
		{
			retval = FALSE;
			Write_Debug("Failed to open %s", filename);
		}

        _isdefault = 1; //  Change state for detail/archive
	}

	while (retval)
	{
		Write_Debug("Loading supercell %d file %s", td->terrain->trnum, filename);

		if (fioRead(hfile, &trnfile, sizeof(TERRAINFILE)) != sizeof(TERRAINFILE))
		{
			Write_Debug("Unable to read the DTL file header");
			retval = FALSE;
			continue;
		}

		if (!VerifyHeader(&trnfile))
		{
			Write_Debug("Unable to read the %s file header", TRN_EXT);
			retval = FALSE;
			continue;
		}

		td->dataversion = trnfile.dataversion;	// Data version using simple incremental technique

		// Process point data
		tmpShort = fioGets(hfile);

		td->numheightmapelements = tmpShort / sizeof(SINT16);

		if (td->numheightmapelements > SUPERCELL_SIZE_XY)
		{
			Write_Debug("DTL file %s has excess data at ptcnt (%d)", filename, td->numheightmapelements);
			retval = FALSE;
			continue;
		}

		if (td->numheightmapelements > 0)
		{
			void* readTo = (trnfile.flags & TF_FLAG_DATA_RLE) ? data : td->heightmap;
			if (fioRead(hfile, readTo, (size_t)tmpShort) != (size_t)tmpShort)
			{
				Write_Debug("Unable to read the %s file point data", TRN_EXT);
				retval = FALSE;
				continue;
			}

			if (trnfile.flags & TF_FLAG_DATA_RLE)
				trnldrRLEToData(td->numheightmapelements, data, &td->numheightmapelements, td->heightmap);
		}

		// Process normal data
		tmpShort = fioGets(hfile);

		td->nmcnt = tmpShort / sizeof(SINT16);

		if (td->nmcnt > SUPERCELL_SIZE_XY)
		{
			Write_Debug("DTL file %s has excess normal data at nmcnt (%d)", filename, td->nmcnt);
			retval = FALSE;
			continue;
		}

		if (td->nmcnt > 0)
		{
			void* readTo = (trnfile.flags & TF_FLAG_NORMALS_RLE) ? data : td->nm;
			if (fioRead(hfile, readTo, (size_t)tmpShort) != (size_t)tmpShort)
			{
				Write_Debug("Unable to read the %s file normal data", TRN_EXT);
				retval = FALSE;
				continue;
			}

			if (trnfile.flags & TF_FLAG_NORMALS_RLE)
				trnldrRLEToData(td->nmcnt, data, &td->nmcnt, td->nm);
		}

		// Process triangle data
		tmpShort = fioGets(hfile);

		td->tricnt = tmpShort;

		if (td->tricnt > MAX_TERRAIN_TEXTURES)
		{
			Write_Debug("DTL file %s has excess data at tricnt (%d)", filename, td->tricnt);
			retval = FALSE;
			continue;
		}

		if (td->tricnt > 0)
		{
			tmpShort = td->tricnt * 32;
			if (fioRead(hfile, td->triname[0], (size_t)tmpShort) != (size_t)tmpShort)
			{
				Write_Debug("Unable to read the %s file texture names", TRN_EXT);
				retval = FALSE;
				continue;
			}
		}

		tmpShort = fioGets(hfile);

		td->trindx = tmpShort;

		if (td->trindx > MAX_TERRAIN_EDIT_LIST)
		{
			Write_Debug("DTL file %s has excess data at trindx (%d, max is %d)", filename, td->trindx, MAX_TERRAIN_EDIT_LIST);
			retval = FALSE;
			continue;
		}

		if (td->trindx > 0)
		{
			sizecheck = td->trindx * sizeof(TERRAIN_INFO);
			if (fioRead(hfile, td->trinfo, sizecheck) != sizecheck)
			{
				Write_Debug("Unable to read the %s file edit list", TRN_EXT);
				retval = FALSE;
				continue;
			}
		}

		tmpShort = fioGets(hfile);

		td->tobndx = tmpShort;

		if (td->tobndx >= (MAX_TERRAIN_OCTETS * MAX_TRNOBJ_OCTET))				// We have to add one cloud object per supercell, so check accordingly
		{
			Write_Debug("DTL file %s has excess data at tobndx (%d)", filename, td->tobndx);
			retval = FALSE;
			continue;
		}

		if (td->tobndx > 0)
		{
			sizecheck = td->tobndx * sizeof(SDM_ObjInfo);
			if (fioRead(hfile, td->trnobj, sizecheck) != sizecheck)
			{
				Write_Debug("Unable to read the %s file object list", TRN_EXT);
				retval = FALSE;
				continue;
			}
		}

	    // Load one instance of the cloud model at the center of each supercell
	    SDM_ObjInfo* cloudInstance	= &td->trnobj[td->tobndx];

	    std::string	name = "cloud01";
	    strcpy(cloudInstance->objname, name.c_str());
	    cloudInstance->pos.x = (td->scx + 0.5f) * SDM_SUPERCELL_SIZE;
	    cloudInstance->pos.y = (td->scy + 0.5f) * SDM_SUPERCELL_SIZE;
	    cloudInstance->flags |= TDFLAG_LOADFAR;									// Because we need it to be drawn way out

	    td->tobndx++;

	    // Load one instance of the raincloud model at the center of each supercell
        cloudInstance = &td->trnobj[td->tobndx];

        name = "raincloud01";
        strcpy(cloudInstance->objname, name.c_str());
        cloudInstance->pos.x = (td->scx + 0.5f) * SDM_SUPERCELL_SIZE;
        cloudInstance->pos.y = (td->scy + 0.5f) * SDM_SUPERCELL_SIZE;
        cloudInstance->flags |= TDFLAG_LOADFAR;									// Because we need it to be drawn way out
        cloudInstance->ortdrg = 0;

#if(0)
        float xx = cloudInstance->pos.x;
        float yy = cloudInstance->pos.y;

        unsigned int angle = (unsigned int)(7159 * ((int)xx) + 15137 * ((int)yy)) % 360;
        cloudInstance->ortdrg = (UINT8)(angle / 45);
#endif

        td->tobndx++;

		if (td->toldflags != NULL)
		{
			memFree(td->toldflags);
		}

		td->toldflags = (UINT32*)memAlloc(((td->tobndx + TRNOBJ_LOADED_FLAG_SIZE - 1) / TRNOBJ_LOADED_FLAG_SIZE) * TRNOBJ_LOADED_FLAG_SIZE / 8);

		if (ReadingSDM)
		{
			if (fioRead(hfile, &sdh, sizeof(STRT_DTL_HEADER)) != sizeof(STRT_DTL_HEADER))
			{
				Write_Debug("Unable to read the %s file strat header", TRN_EXT);
				retval = FALSE;
				continue;
			}

			size_t cpSize = sdh.numCPs * sizeof(CP);
			size_t facSize = sdh.numFacilities * sizeof(FACILITY);
			size_t bldgSize = sdh.numBuildings * sizeof(BUILDING);
			size_t linkSize = sdh.numLinks * sizeof(DEPOT_LINK);
			size_t wptSize = sdh.numWaypoints * sizeof(WAYPOINT);

			char* p = (char*)memAlloc(cpSize + facSize + bldgSize + linkSize + wptSize);
			td->pcp = (CP*)p;
			td->numCPs = sdh.numCPs;
			if (sdh.numCPs > 0)
			{
				p += cpSize;
				if (fioRead(hfile, td->pcp, cpSize) != cpSize)
				{
					Write_Debug("Unable to read the %s file strat CPs", TRN_EXT);
					retval = FALSE;
					continue;
				}
			}

			td->numFacilities = sdh.numFacilities;
			if (sdh.numFacilities > 0)
			{
				td->pfac = (FACILITY*)p;
				p += facSize;
				if (fioRead(hfile, td->pfac, facSize) != facSize)
				{
					Write_Debug("Unable to read the %s file strat Facilities", TRN_EXT);
					retval = FALSE;
					continue;
				}

				td->numFacilities = sdh.numFacilities;

				td->facTable = sdmCreateFacilityTable(td->pfac, td->numFacilities, td->numCPs);
			}

			if (sdh.numBuildings > 0)
			{
				td->pbld = (BUILDING*)p;
				p += bldgSize;
				if (fioRead(hfile, td->pbld, bldgSize) != bldgSize)
				{
					Write_Debug("Unable to read the %s file strat Buildings", TRN_EXT);
					retval = FALSE;
					continue;
				}

				td->numBuildings = sdh.numBuildings;

				td->buildingTable = sdmCreateBuildingTable(td->facTable, td->pbld,td->numBuildings,&td->bldHash);
			}

			td->numLinks = sdh.numLinks;
			if (sdh.numLinks > 0)
			{
				td->plink = (DEPOT_LINK*)p;
				p += linkSize;
				if (fioRead(hfile, td->plink, linkSize) != linkSize)
				{
					Write_Debug("Unable to read the %s file strat Links", TRN_EXT);
					retval = FALSE;
					continue;
				}
			}

			td->numWaypoints = sdh.numWaypoints;
			if (sdh.numWaypoints > 0)
			{
				td->pwp = (WAYPOINT*)p;
				p += wptSize;
				if (fioRead(hfile, td->pwp, wptSize) != wptSize)
				{
					Write_Debug("Unable to read the %s file strat Waypoints", TRN_EXT);
					retval = FALSE;
					continue;
				}
			}

            static short count[MaxBushes];

            int bushes[MaxBushes] =
            {
                Azalea,
                AmericanBoxwood1,
                AmericanBoxwood2,
                AmericanBoxwood3,
                AmericanBoxwood4,
                AmericanBoxwood5,
            };

            int size = fioRead(hfile, count, sizeof(short) * MaxBushes);

            FLOAT64VECTOR pos;
            unsigned int bits = 0;
            unsigned char octet = 0;

            if(size == sizeof(short) * MaxBushes)
            {
                float minheight;
                size = fioRead(hfile, &minheight, sizeof(float));

                if(size == sizeof(float))
                {
                    for(int i = 0; i < MaxBushes; i++)
                    {
                        for(int j = 0; j < count[i]; j++)
                        {
                            bits = 0;

                            fioRead(hfile, &octet, sizeof(unsigned char));

                            int r = (int)octet / SDM_SUPERCELL_OCTETS_Y;
                            int c = (int)octet % SDM_SUPERCELL_OCTETS_Y;

                            fioRead(hfile, &bits, sizeof(unsigned int));

                            pos.x = td->scx * SDM_SUPERCELL_SIZE;
                            pos.y = td->scy * SDM_SUPERCELL_SIZE;

                            pos.x += (float)((bits >> 22) & 0x3ff) + c * SDM_OCTET_SIZE;
                            pos.y += (float)((bits >> 12) & 0x3ff) + r * SDM_OCTET_SIZE;
                            pos.z = (float)((bits & 0xfff) * 0.1f) + minheight;
                            pos.z = -pos.z;

                            sptAddTree(NULL, bushes[i], td, pos, true);
                        }
                    }
                }
            }

			// update our building owner and level
            clientStrtUpdateBuildings(td);

			td->stratLoaded = TRUE;
		}
		else
		{
			memset(&sdh, 0, sizeof(STRT_DTL_HEADER));
		}

		break;
	}

	if (hfile != -1)
	{
		fioClose(hfile);
	}

	td->numoctetsacross = SUPERCELL_OCTETS_X;
	td->numoctetsdown = SUPERCELL_OCTETS_Y;

    logTimerEndMacro();
#ifdef ANDREWTEST
    // supercellloaded
/*
    printf("\n\nSuperCell %d, %d\n\n",td->scx, td->scy);

    extern PoolMap _poolmap;

    for (int i=0; i<td->tobndx; i++ )
    {
        SDM_ObjInfo* p = &td->trnobj[i];
        //printf("%s: %10.2f %10.2f\n",p->objname, p->pos.x, p->pos.y);

        std::string	str(p->objname);
        std::string modelname;

		size_t start = str.rfind('\\');
		size_t end = str.rfind('.');

		if(start == std::string::npos)
		{
			modelname = str.substr(0,end);
		}
		else
		{
			modelname = str.substr(start+1,end-start-1);
		}

        PoolMap::iterator it = _poolmap.find(modelname);
        if ( it != _poolmap.end() )
        {
            PoolModel* pm = (*it).second;
            //printf("Loading model: %s\n", modelname.c_str() );
        }
    }
*/
#endif
	return(retval);
}

static LOGICAL SetColorAndZ(SINT32 d, piVectorPtr po, piVertexColorPtr psl)
{
	if (d == TERRAIN_NODATA)
	{
		// Water level
		po->z = 0.0f;
	}
	else
	{
		// Terrain height
		po->z = -(FLOAT32)d * ScaleFactor;
	}

	psl->color = colorWHITE|colorALPHA;

	return(TRUE);
}

LOGICAL trnldrLoadPoints(OBJECT* tt, short heightmap[SUPERCELL_SIZE_XY])
{
//LOGICAL			retval=TRUE;
SINT32			i, j, k, l, w, h, d;
piVectorPtr 		po;
piVertexColorPtr	psl;

    logTimerBeginMacro();

	w = SUPERCELL_SIZE_X1;
	h = SUPERCELL_SIZE_Y1;

	po = tt->StaticPositions();
	psl = tt->StaticColors();

	for (i=0; i < h; i+=2)
	{
		for (j=0; j < w; j+=2)
		{
			// Arrange each octet into triangles
			// 0
			k = SUPERCELL_SIZE_Y1 - (i + 1);
			l = 1;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 1
			k = SUPERCELL_SIZE_Y1 - (i + 0);
			l = 0;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 2
			k = SUPERCELL_SIZE_Y1 - (i + 1);
			l = 0;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 3
			k = SUPERCELL_SIZE_Y1 - (i + 2);
			l = 0;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 4
			k = SUPERCELL_SIZE_Y1 - (i + 2);
			l = 1;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 5
			k = SUPERCELL_SIZE_Y1 - (i + 2);
			l = 2;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 6
			k = SUPERCELL_SIZE_Y1 - (i + 1);
			l = 2;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 7
			k = SUPERCELL_SIZE_Y1 - (i + 0);
			l = 2;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;

			// 8
			k = SUPERCELL_SIZE_Y1 - (i + 0);
			l = 1;
			d = heightmap[k * (w + 1) + j + l];
			SetColorAndZ(d, po, psl);
			++po; ++psl;
		}
	}

	tt->SyncStaticData();
	tt->SyncMutableData();

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL CalculateNormalsAndBounds(TERRAIN_DETAIL* td)
{
OBJECT*		tt;


	tt = td->terrain;

	if (tt->flags & OBJECT_VISIBLE)
	{
		if (!ldmdlCalcSurfaceNormals(tt) ||
			//!ldmdlCalcNormals(tt) ||
			!ldmdlGetBounds(tt))
		{
			return(FALSE);
		}
	}


	return(TRUE);
}

static LOGICAL LightSupercell(TERRAIN_DETAIL* td)
{
	OBJECT*		tt;
	SORTHEADER		sort;

	tt = td->terrain;

	// Light the new terrain tiles
	memset(&sort, 0, sizeof(sort));
	sort.part = tt;
#ifndef USE_3DAPI_LIGHTING
	if (!obdliteCalculate(&sort, FALSE))
	{
		return(FALSE);
	}
#endif
	return(TRUE);
}

static LOGICAL DuplicateBaseSupercell(TERRAIN_DETAIL* td)
{
    logTimerBeginMacro();

    if (td->terrainorig != NULL)
	{
		obpoolFreeObjectCallback(td->terrainorig);
		td->terrainorig = NULL;
	}

	td->terrainorig = obmainCopyObject(td->terrain);
	if (td->terrainorig == NULL)
	{
		Write_Debug("error copying terrain object to collision terrain object");
	}

    if(td->terrainorig->flags & OBJECT_TERRAIN)
    {
        // Remove from standard rendering system.
        obmainRemoveObjectTree(td->terrainorig);
    }

    logTimerEndMacro();

	return(TRUE);
}

static SINT32 FindOctetCount(TERRAIN_DETAIL* td, SINT32 txndx)
{
SINT32		i;
SINT32		count;

	count = 0;
	for (i=td->trindx-1; i >= 0; i--)
	{
		if (td->trinfo[i].txndx == txndx)
		{
			count += 1;
		}
	}

	if (count == 0)
	{
		Write_Debug("Texture index was not found: %d", txndx);
	}

	return(count);
}

static LOGICAL LoadTextures(SINT32 sc)
{
//LOGICAL			retval=TRUE;
SINT8			objname[64], filename[_MAX_PATH], *ptr;
SINT32			i, j, k;
SINT32			texid;
OBJECT*		tt;
OBJECT*		newobj;
piVectorConstPtr	po;
TERRAIN_DETAIL*	td;

	td = &trndtl[sc];
	tt = td->terrain;

	// If the DTL file did not contain any textured octet information, create a default set
	if (td->tricnt == 0)
	{
	FLOAT32		z;

		po = tt->ImmutablePositions();

		for (i=0; i < SUPERCELL_OCTETS_Y; i++)
		{
			for (j=0; j < SUPERCELL_OCTETS_X; j++)
			{
				z = 0.0f;
				for (k=0; k < NUM_VERTICES_PER_OCTET; k++)
				{
					z -= po[k].z;
				}

				z /= (FLOAT32)k;	//	average

				if (z < ALT_TRN_WATER * ScaleFactor)
				{
					k = 0;
				}
				else
				if (z == ALT_TRN_WATER * ScaleFactor)
				{
					k = 1;
				}
				else
				if (z > ALT_TRN_WATER * ScaleFactor &&
					z <= ALT_TRN_DUNES * ScaleFactor)
				{
					k = 2;
				}
				else
				if (z > ALT_TRN_DUNES * ScaleFactor &&
					z <= ALT_TRN_FARMS * ScaleFactor)
				{
					k = 3;
				}
				else
				if (z > ALT_TRN_FARMS * ScaleFactor &&
					z <= ALT_TRN_CLEAR * ScaleFactor)
				{
					k = 4;
				}
				else
				if (z > ALT_TRN_CLEAR * ScaleFactor &&
					z <= ALT_TRN_FOREST * ScaleFactor)
				{
					k = 5;
				}
				else
				if (z > ALT_TRN_FOREST * ScaleFactor &&
					z <= ALT_TRN_ROCKY * ScaleFactor)
				{
					k = 6;
				}
				else
				{
					k = 7;
				}

				trnmainAddAltTexture(&newobj, tt, 0, alttexname[k]);
			}
		}

		td->flags |= TDFLAG_MODIFIED;
		return(TRUE);
	}


	for (i=0; i < td->tricnt; i++)
	{
		if (td->nonreplacementtiles[i] == NULL)
		{
			sprintf(objname, "%s_t%d", td->terrain->name, i);

			if (td->trindx <= SUPERCELL_OCTETS_XY)
			{
			SINT32		num;

				num = FindOctetCount(td, i);
				if (num == 0)
				{
					continue;
				}

				if (!obmainCreateObject(&newobj, objname, num * NUM_VERTICES_PER_OCTET, num * NUM_TRIANGLES_PER_OCTET))
				{
					return(FALSE);
				}
			}
			else
			{
				if (!obmainCreateObject(&newobj, objname, MAX_TERRAIN_VERTICES, MAX_TERRAIN_TRIANGLES))
				{
					return(FALSE);
				}
			}

			newobj->numverts = 0;
			newobj->numtris = 0;
			newobj->texname = strpool(td->triname[i]);

			strcpy(filename, TERRAIN_PREPATH);
			strcat(filename, newobj->texname);
			texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
			if (!piIsTexture(texid, 0))
			{
				Write_Debug("Could not load file as a texture");
				obpoolFreeObjectCallback(newobj);
				return(FALSE);
			}

			newobj->texid = texid;
			newobj->layer = 2;

//			newobj->rot = tt->rot;
			newobj->rot = td->terrain->rot;

			trnmainSetTerrainMatrix(newobj);
			newobj->flags |= OBJECT_USEMATRIX;

			newobj->flags |= OBJECT_LITE;
			newobj->flags |= OBJECT_DYNALITE;
			newobj->flags |= OBJECT_VISIBLE;
//			newobj->flags |= OBJECT_SKIPDRAWING;
			newobj->flags |= OBJECT_INDEXED;
			newobj->flags |= OBJECT_TRIANGLELIST;
//			newobj->flags |= OBJECT_NOSPECULAR;
			newobj->flags |= OBJECT_COLLISION;
			newobj->flags |= OBJECT_TERRAIN;

            // Remove object from standard rendering system
            obmainRemoveObjectTree(newobj);

			if (strcmp(newobj->texname,"null1.bmp") == 0)
			{
				newobj->flags |= OBJECT_SKIPDRAWING;
			}

			// Added materials to objects based on texture names
			strcpy(filename, td->triname[i]);
			ptr = strrchr(filename, '.');
			if (ptr != NULL)
			{
				// get rid of extension
				*ptr = 0;
			}

			Write_Debug("Material = %s", filename);
			newobj->matid = piFindMaterialName(filename);
			if (piIsMaterial(newobj->matid))
			{
				newobj->matname = strpool(filename);
			}
			else
			{
				Write_Debug("Terrain tile '%s' has no material", filename);
				newobj->matid = piFindMaterialName(TERRAIN_DEFAULT_MATERIAL);
				newobj->matname = TERRAIN_DEFAULT_MATERIAL;
			}

#ifndef USE_3DAPI_LIGHTING
			if (!(newobj->flags & OBJECT_DYNALITE))
			{
			int         numlights;
			LIGHT*     lplite;

				numlights = piGetLights(&lplite);
				if (!obsliteCalculate(1, newobj, numlights, lplite, piGetAmbLight()))
				{
					return(FALSE);
				}
			}
#endif

			td->nonreplacementtiles[i] = newobj;
			td->nonreplacementtiles[i]->trnum = sc;
		}
		else
		{
			td->nonreplacementtiles[i]->rot = td->terrain->rot;
		}
	}

	return(TRUE);
}

static LOGICAL LoadTexturesSequenced(SINT32 sc)
{
//LOGICAL			retval=TRUE;
SINT8			objname[64], filename[_MAX_PATH], *ptr;
SINT32			i, j, k;
SINT32			texid;
OBJECT*		tt;
OBJECT*		newobj;
piVectorConstPtr	po;
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*		tsb;

	td = &trndtl[sc];
	tt = td->terrain;
	tsb = &td->trnstate;

	// If the DTL file did not contain any textured octet information, create a default set
	if (tsb->varone == 0)
	{
	FLOAT32		z;

		po = tt->ImmutablePositions();
		for (i=tsb->vartwo; i < SUPERCELL_OCTETS_Y; i++)
		{
			for (j=tsb->varthree; j < SUPERCELL_OCTETS_X; j++)
			{
				z = 0.0f;
				for (k=0; k < NUM_VERTICES_PER_OCTET; k++)
				{
					z -= po[k].z;
				}

				z /= (FLOAT32)k;	//	average

				if (z < ALT_TRN_WATER * ScaleFactor)
				{
					k = 0;
				}
				else
				if (z == ALT_TRN_WATER * ScaleFactor)
				{
					k = 1;
				}
				else
				if (z > ALT_TRN_WATER * ScaleFactor &&
					z <= ALT_TRN_DUNES * ScaleFactor)
				{
					k = 2;
				}
				else
				if (z > ALT_TRN_DUNES * ScaleFactor &&
					z <= ALT_TRN_FARMS * ScaleFactor)
				{
					k = 3;
				}
				else
				if (z > ALT_TRN_FARMS * ScaleFactor &&
					z <= ALT_TRN_CLEAR * ScaleFactor)
				{
					k = 4;
				}
				else
				if (z > ALT_TRN_CLEAR * ScaleFactor &&
					z <= ALT_TRN_FOREST * ScaleFactor)
				{
					k = 5;
				}
				else
				if (z > ALT_TRN_FOREST * ScaleFactor &&
					z <= ALT_TRN_ROCKY * ScaleFactor)
				{
					k = 6;
				}
				else
				{
					k = 7;
				}

				trnmainAddAltTexture(&newobj, tt, 0, alttexname[k]);

				tsb->varthree += 1;
				return(FALSE);
			}

			tsb->vartwo += 1;
			tsb->varthree = 0;
		}

		td->flags |= TDFLAG_MODIFIED;
		return(TRUE);
	}


	for (i=tsb->vartwo; i < td->tricnt; i++)
	{
		if (td->nonreplacementtiles[i] == NULL)
		{
			sprintf(objname, "%s_t%d", td->terrain->name, i);

			if (td->trindx <= SUPERCELL_OCTETS_XY)
			{
			SINT32		num;

				num = FindOctetCount(td, i);
				if (num == 0)
				{
					tsb->vartwo += 1;
					return(FALSE);
				}

				if (!obmainCreateObject(&newobj, objname, num * NUM_VERTICES_PER_OCTET, num * NUM_TRIANGLES_PER_OCTET))
				{
					return(FALSE);
				}
			}
			else
			{
				if (!obmainCreateObject(&newobj, objname, MAX_TERRAIN_VERTICES, MAX_TERRAIN_TRIANGLES))
				{
					return(FALSE);
				}
			}

			newobj->numverts = 0;
			newobj->numtris = 0;
			newobj->texname = strpool(td->triname[i]);

			strcpy(filename, TERRAIN_PREPATH);
			strcat(filename, newobj->texname);
			texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
			if (!piIsTexture(texid, 0))
			{
				Write_Debug("Could not load file as a texture");
				obpoolFreeObjectCallback(newobj);
				return(FALSE);
			}

			newobj->texid = texid;
			newobj->layer = 2;

//			newobj->rot = tt->rot;
			newobj->rot = td->terrain->rot;

			trnmainSetTerrainMatrix(newobj);
			newobj->flags |= OBJECT_USEMATRIX;

			newobj->flags |= OBJECT_LITE;
			newobj->flags |= OBJECT_DYNALITE;
			newobj->flags |= OBJECT_VISIBLE;
//			newobj->flags |= OBJECT_SKIPDRAWING;
			newobj->flags |= OBJECT_INDEXED;
			newobj->flags |= OBJECT_TRIANGLELIST;
//			newobj->flags |= OBJECT_NOSPECULAR;
			newobj->flags |= OBJECT_COLLISION;
			newobj->flags |= OBJECT_TERRAIN;

            // Remove object from standard rendering system
            obmainRemoveObjectTree(newobj);

			if (strcmp(newobj->texname,"null1.bmp") == 0)
			{
				newobj->flags |= OBJECT_SKIPDRAWING;
			}

			// Added materials to objects based on texture names
			strcpy(filename, td->triname[i]);
			ptr = strrchr(filename, '.');
			if (ptr != NULL)
			{
				// get rid of extension
				*ptr = 0;
			}

			Write_Debug("Material = %s", filename);
			newobj->matid = piFindMaterialName(filename);
			if (piIsMaterial(newobj->matid))
			{
				newobj->matname = strpool(filename);
			}
			else
			{
				Write_Debug("Terrain tile '%s' has no material", filename);
				newobj->matid = piFindMaterialName(TERRAIN_DEFAULT_MATERIAL);
				newobj->matname = TERRAIN_DEFAULT_MATERIAL;
			}

#ifndef USE_3DAPI_LIGHTING
			if (!(newobj->flags & OBJECT_DYNALITE))
			{
			int         numlights;
			LIGHT*     lplite;

				numlights = piGetLights(&lplite);
				if (!obsliteCalculate(1, newobj, numlights, lplite, piGetAmbLight()))
				{
					return(FALSE);
				}
			}
#endif

			td->nonreplacementtiles[i] = newobj;
			td->nonreplacementtiles[i]->trnum = sc;
		}
		else
		{
			td->nonreplacementtiles[i]->rot = td->terrain->rot;
		}

		tsb->vartwo += 1;
		return(FALSE);
	}

	return(TRUE);
}

static LOGICAL LoadOctets(TERRAIN_DETAIL* td)
{
SINT32		i, j;

	if (td->flags & TDFLAG_MODIFIED)
	{
		return(TRUE);
	}

    logTimerBeginMacro();

	for (i=0; i < td->trindx; i++)
	{
		for (j=0; j < td->tricnt; j++)
		{
			if (td->nonreplacementtiles[j] == NULL)
			{
				continue;
			}

			if (strcmp(td->triname[td->trinfo[i].txndx], td->nonreplacementtiles[j]->texname) == 0)
			{
				break;
			}
		}

		if (j == td->tricnt)
		{
			Write_Debug("Cannot find texture '%s' for edit entry %d!!", td->triname[td->trinfo[i].txndx], i);
		}
		else
		{
			trnmainAddToObject(	td->nonreplacementtiles[j],
					td->trinfo[i].tonum == 0 ? td->terrain : td->nonreplacementtiles[td->trinfo[i].tonum-1],
					td->trinfo[i].octnum,
					td->trinfo[i].tonum,
					td->trinfo[i].ortdrg >> 4,
					td->trinfo[i].ruftrn & 0xf,
					td->trinfo[i].ruftrn >> 4,
					td->trinfo[i].ortdrg & 0xf);
		}
	}

	if (td->trindx > 0)
	{
		ObjectsWereAdded = TRUE;
	}

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL LoadOctetsSequenced(SINT32 sc)
{
SINT32		i, j;
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*		tsb;
static LOGICAL		trip=FALSE;

	td = &trndtl[sc];
	if (td->flags & TDFLAG_MODIFIED)
	{
		return(TRUE);
	}

	tsb = &td->trnstate;

	if (tsb->varone == 0)
	{
		trip = FALSE;
	}

	for (i=tsb->varone; i < td->trindx; i++)
	{
		for (j=0; j < td->tricnt; j++)
		{
			if (td->nonreplacementtiles[j] == NULL)
			{
				continue;
			}

			if (strcmp(td->triname[td->trinfo[i].txndx], td->nonreplacementtiles[j]->texname) == 0)
			{
				break;
			}
		}

		if (j == td->tricnt)
		{
			Write_Debug("Cannot find texture '%s' for edit entry %d!!", td->triname[td->trinfo[i].txndx], i);
		}
		else
		{
			trnmainAddToObject(	td->nonreplacementtiles[j],
					td->trinfo[i].tonum == 0 ? td->terrain : td->nonreplacementtiles[td->trinfo[i].tonum-1],
					td->trinfo[i].octnum,
					td->trinfo[i].tonum,
					td->trinfo[i].ortdrg >> 4,
					td->trinfo[i].ruftrn & 0xf,
					td->trinfo[i].ruftrn >> 4,
					td->trinfo[i].ortdrg & 0xf);

			trip = TRUE;
		}

		tsb->varone += 1;

		return(FALSE);
	}

	if (trip)
	{
		ObjectsWereAdded = TRUE;
	}


	return(TRUE);
}

static LOGICAL FindStratObject(TERRAIN_DETAIL* td, SINT8* objname, piVECTOR* pos, FULL_ID* fullid, REL_OBJ_ID* ObjId)
{
	SINT8*			filename;
	SINT32			i, posx, posy;
	CP*			pcp;		// CPs
	FACILITY*		pfac;		// Facilities
	BUILDING*		pbld;		// Building

	filename = strrchr(objname, '\\');
	if (filename != NULL)
	{
		filename += 1;
	}
	else
	{
		filename = objname;
	}

	posx = (SINT32)pos->x;
	posy = (SINT32)pos->y;

	i = 0;
	pcp = td->pcp;
	while (i < td->numCPs)
	{
		if (pcp->posx == posx &&
			pcp->posy == posy &&
			strcmp(pcp->fileName, filename) == 0)
		{
			fullid->sx = td->scx;
			fullid->sy = td->scy;
			fullid->id = pcp->oid.id;
//			fullid->type = pcp->oid.objClass;
			fullid->type = 0;
			if (ObjId != NULL)
			{
				*ObjId = pcp->oid;
			}
			return(TRUE);
		}

		pcp += 1;
		i += 1;
	}

	i = 0;
	pfac = td->pfac;
	while (i < td->numFacilities)
	{
		if (pfac->posx == posx &&
			pfac->posy == posy &&
			strcmp(pfac->fileName, filename) == 0)
		{
			fullid->sx = td->scx;
			fullid->sy = td->scy;
			fullid->id = pfac->oid.id;
//			fullid->type = pfac->oid.objClass;
			fullid->type = 0;
			if (ObjId != NULL)
			{
				*ObjId = pfac->oid;
			}
			return(TRUE);
		}

		pfac += 1;
		i += 1;
	}

	i = 0;
	pbld = td->pbld;
	while (i < td->numBuildings)
	{
		if (pbld->posx == posx &&
			pbld->posy == posy &&
			strcmp(pbld->fileName, filename) == 0)
		{
			fullid->sx = td->scx;
			fullid->sy = td->scy;
			fullid->id = pbld->oid.id;
//			fullid->type = pbld->oid.objClass;
			fullid->type = 0;
			if (ObjId != NULL)
			{
				*ObjId = pbld->oid;
			}
			return(TRUE);
		}

		pbld += 1;
		i += 1;
	}

	fullid->sx = SUPERCELL_INVALID;
	fullid->sy = SUPERCELL_INVALID;
	fullid->id = 0;
	fullid->type = 0;

	ObjId->id = 0;
	ObjId->objClass = 0;
	ObjId->objType = 0;

	return(FALSE);
}

static LOGICAL FindSupercellTD(SINT32 scx, SINT32 scy, TERRAIN_DETAIL** ptd)
{
SINT32		i;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if (activeTerrainDetail[i] &&
			trndtl[i].scx == scx &&
			trndtl[i].scy == scy)
		{
			*ptd = &trndtl[i];

			return(TRUE);
		}
	}

	return(FALSE);
}

LOGICAL trnldrFindStratObjectPosition(SINT32 SupercellX, SINT32 SupercellY, SINT32 LocId, FLOAT64VECTOR* pos)
{
SINT32				i;
TERRAIN_DETAIL*		td;
CP*					pcp;		// CPs
FACILITY*			pfac;		// Facilities
BUILDING*			pbld;		// Building

	if (!FindSupercellTD(SupercellX, SupercellY, &td))
	{
		return(FALSE);
	}

	i = 0;
	pcp = td->pcp;
	while (i < td->numCPs)
	{
		if (LocId == pcp->oid.id)
		{
			pos->x = (FLOAT64)pcp->posx;
			pos->y = (FLOAT64)pcp->posy;
			pos->z = 0.0;
			return(TRUE);
		}

		pcp += 1;
		i += 1;
	}

	i = 0;
	pfac = td->pfac;
	while (i < td->numFacilities)
	{
		if (LocId == pfac->oid.id)
		{
			pos->x = (FLOAT64)pfac->posx;
			pos->y = (FLOAT64)pfac->posy;
			pos->z = 0.0;
			return(TRUE);
		}

		pfac += 1;
		i += 1;
	}

	i = 0;
	pbld = td->pbld;
	while (i < td->numBuildings)
	{
		if (LocId == pbld->oid.id)
		{
			pos->x = (FLOAT64)pbld->posx;
			pos->y = (FLOAT64)pbld->posy;
			pos->z = 0.0;
			return(TRUE);
		}

		pbld += 1;
		i += 1;
	}

	return(FALSE);
}

#ifdef ANDREWTEST
//extern void processObjects(TERRAIN_DETAIL* td, SINT32 sc);
//#define LOADEVERYTHING
#endif

static LOGICAL LoadObjects(SINT32 sc)
{
	LOGICAL		KillObject;
	SINT32		i, place=0;
	FULL_ID		fullid;
	REL_OBJ_ID	ObjId;
	TERRAIN_DETAIL* td = &trndtl[sc];

    logTimerBeginMacro();

#ifdef ANDREWTEST
//    processObjects(td, sc);
#endif

    FLOAT32 horizon;
    trnmainGetHorizon(&horizon);
	horizon *= horizon;		// To avoid sqrt

	if (td->tobndx > 0)
	{
		while (FindObjectsInOctetSCLoading((UINT32*)&i, td->terrain->trnum, (UINT32*)&place, horizon))
		{
			KillObject = FALSE;
			memset(&fullid, 0, sizeof(FULL_ID));
			memset(&ObjId, 0, sizeof(REL_OBJ_ID));

			FindStratObject(td, td->trnobj[i].objname, &td->trnobj[i].pos, &fullid, &ObjId);

			if(!trnmainAddObject(td->trnobj[i].objname,
								&td->trnobj[i].pos,
								td->terrain,
								fullid.sx,
								fullid.sy,
								fullid.id,
								&ObjId,
								td->trnobj[i].ortdrg >> 4,
								td->trnobj[i].ruftrn & 0xf,
								td->trnobj[i].ruftrn >> 4,
								td->trnobj[i].ortdrg & 0xf,
                                sc,
								td->trnobj[i].flags & TRNOBJ_NON_PLANAR_BASE,
                                td->trnobj[i],td))
			{
				KillObject = TRUE;
			}
			else
			{
				KillObject = FALSE;
			}
		}

		ObjectsWereAdded = TRUE;
	}

    logTimerEndMacro();

	return(TRUE);
}


static LOGICAL LoadObjectsSequenced(SINT32 sc)
{
LOGICAL		KillObject;
SINT32		i;
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*	tsb;
FULL_ID		fullid;
REL_OBJ_ID	ObjId;
static LOGICAL		trip;


	td = &trndtl[sc];
	tsb = &td->trnstate;

	if (tsb->varone == 0)
	{
		trip = FALSE;
	}

    FLOAT32 horizon;
    trnmainGetHorizon(&horizon);
	horizon *= horizon;		// To avoid sqrt

	if (td->tobndx > 0)
	{
		if (FindObjectsInOctetSCLoading((UINT32*)&i, sc, (UINT32*)&tsb->varone, horizon))
		{
			KillObject = FALSE;
			memset(&fullid, 0, sizeof(FULL_ID));
			memset(&ObjId, 0, sizeof(REL_OBJ_ID));

			FindStratObject(td, td->trnobj[i].objname, &td->trnobj[i].pos, &fullid, &ObjId);

			if (!trnmainAddObject(td->trnobj[i].objname,
								&td->trnobj[i].pos,
								td->terrain,
								fullid.sx,
								fullid.sy,
								fullid.id,
								&ObjId,
								td->trnobj[i].ortdrg >> 4,
								td->trnobj[i].ruftrn & 0xf,
								td->trnobj[i].ruftrn >> 4,
								td->trnobj[i].ortdrg & 0xf,
                                sc,
								td->trnobj[i].flags & TRNOBJ_NON_PLANAR_BASE,
								td->trnobj[i],td))
			{
				KillObject = TRUE;
			}

			trip = TRUE;

			Write_Debug("Object %d (%s) of %d was loaded for supercell %d", tsb->varone, td->trnobj[tsb->varone].objname, td->tobndx, sc);
			return(FALSE);
		}
	}

	if (trip)
	{
		ObjectsWereAdded = TRUE;
	}

	return(TRUE);
}


#if USE_HEIGHT_AT
// Planes[o->numtris].
// Calling function responsible for freeing planes
void FindPlaneEquations(OBJECT* o, PLANE* planes)
{
    if(!planes)
        return;

    int numtris = o->numtris;
    piVectorConstPtr v = o->RenderPositions();

    piVECTOR p1, p2, p3;
    int i1, i2, i3;

    for(int i = 0; i < numtris; i++)
    {
        i1 = o->ptris[i].v1;
        i2 = o->ptris[i].v2;
        i3 = o->ptris[i].v3;

        p1.set(v[i1].x, v[i1].y, v[i1].z);
        p2.set(v[i2].x, v[i2].y, v[i2].z);
        p3.set(v[i3].x, v[i3].y, v[i3].z);

        rnvisCreatePlane(&p1, &p2, &p3, &planes[i]);
    }
}

void FindPlaneEquations(OBJECT* o, TERRAIN_DETAIL* td)
{
    logTimerBeginMacro();

    int numtris = o->numtris;
    int numoctets = numtris / NUM_TRIANGLES_PER_OCTET;

    piVectorConstPtr v = o->RenderPositions();

    int tristart = 0;
    int triend = 8;
    int tri;
    int i1, i2, i3;

    piVECTOR p1, p2, p3;

    for(int i = 0; i < numoctets; i++)
    {
        tri = 0;

        for(int j = tristart; j < triend; j++)
        {
            i1 = o->ptris[j].v1;
            i2 = o->ptris[j].v2;
            i3 = o->ptris[j].v3;

            p1.set(v[i1].x, v[i1].y, v[i1].z);
            p2.set(v[i2].x, v[i2].y, v[i2].z);
            p3.set(v[i3].x, v[i3].y, v[i3].z);

            rnvisCreatePlane(&p1, &p2, &p3, &td->plane[i][tri++]);
        }

        tristart = triend;
        triend += 8;
    }

    logTimerEndMacro();
}
#endif

#if WANT_NON_REPLACEMENT_TILES
static LOGICAL SnapObjectsToGrid(TERRAIN_DETAIL* td)
{
SINT32		i;

    logTimerBeginMacro();

	obmainSnapObjectToXYGrid(td->terrain, (FLOAT32)TERRAIN_SNAPTO_GRIDSIZE, 1);
	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			obmainSnapObjectToXYGrid(td->nonreplacementtiles[i], (FLOAT32)TERRAIN_SNAPTO_GRIDSIZE, 1);
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL SnapObjectsToGridSequenced(SINT32 sc)
{
SINT32		i;
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*	tsb;

	td = &trndtl[sc];
	tsb = &td->trnstate;

	if (tsb->varone == 0)
	{
		obmainSnapObjectToXYGrid(td->terrain, (FLOAT32)TERRAIN_SNAPTO_GRIDSIZE, 1);
		tsb->varone += 1;
		return(FALSE);
	}

	for (i=tsb->varone-1; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			obmainSnapObjectToXYGrid(td->nonreplacementtiles[i], (FLOAT32)TERRAIN_SNAPTO_GRIDSIZE, 1);
		}

		tsb->varone += 1;
		return(FALSE);
	}

	return(TRUE);
}
#endif

static LOGICAL LoadBounds(TERRAIN_DETAIL* td)
{
SINT32		i;

	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			if (td->nonreplacementtiles[i]->flags & OBJECT_VISIBLE)
			{
				if (//!ldmdlCalcNormals(td->txtobj[i]) ||
					!ldmdlGetBounds(td->nonreplacementtiles[i]))
				{
					return(FALSE);
				}
			}
		}
	}

	return(TRUE);
}

static LOGICAL LoadBoundsSequenced(SINT32 sc)
{
SINT32		i;
TERRAIN_DETAIL*	td;
TRNSTATEBLOCK*	tsb;

	td = &trndtl[sc];
	tsb = &td->trnstate;

	for (i=tsb->varone; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			if (td->nonreplacementtiles[i]->flags & OBJECT_VISIBLE)
			{
				//!ldmdlCalcNormals(td->txtobj[i]);
				ldmdlGetBounds(td->nonreplacementtiles[i]);
			}
		}

		tsb->varone += 1;
		return(FALSE);
	}

	return(TRUE);
}

#if(0)
static LOGICAL SetTextureObjectsToDrawable(TERRAIN_DETAIL* td)
{
SINT32		i;

    logTimerBeginMacro();

	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			if (strcmp(td->nonreplacementtiles[i]->texname, "null1.bmp") != 0)
			{
				td->nonreplacementtiles[i]->flags &= ~OBJECT_SKIPDRAWING;
			}
		}
	}

	td->terrain->flags &= ~OBJECT_SKIPDRAWING;

    logTimerEndMacro();

	return(TRUE);
}
#endif

#if(0)
static LOGICAL SetTextureObjectsToUndrawable(TERRAIN_DETAIL* td)
{
SINT32		i;

    logTimerBeginMacro();

	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			td->nonreplacementtiles[i]->flags |= OBJECT_SKIPDRAWING;
		}
	}

	td->terrain->flags |= OBJECT_SKIPDRAWING;

    logTimerEndMacro();

	return(TRUE);
}
#endif

static LOGICAL RenumberSupercellObjects(TERRAIN_DETAIL* td, SINT32 sc)
{
SINT32		i;

    logTimerBeginMacro();

	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != NULL)
		{
			td->nonreplacementtiles[i]->trnum = sc;
		}
	}

	td->terrain->trnum = sc;

    logTimerEndMacro();

	return(TRUE);
}

static LOGICAL MarkObjectLoaded(TERRAIN_DETAIL* td, UINT32 objnum)
{
UINT32		i, j;

	i = objnum / TRNOBJ_LOADED_FLAG_SIZE;
	j = objnum % TRNOBJ_LOADED_FLAG_SIZE;
	j = 1 << j;

	td->toldflags[i] |= j;

	return(TRUE);
}

static LOGICAL IsObjectLoaded(TERRAIN_DETAIL* td, UINT32 objnum)
{
UINT32		i, j;

	i = objnum / TRNOBJ_LOADED_FLAG_SIZE;
	j = objnum % TRNOBJ_LOADED_FLAG_SIZE;
	j = 1 << j;

	if (td->toldflags[i] & j)
	{
		return(TRUE);
	}

	return(FALSE);
}

static LOGICAL FindObjectsInOctetSCLoading(UINT32* objnum, SINT32 sc, UINT32* objcnt, FLOAT32 horizonsquared)
{
//	if(!activeTerrainDetail[sc]) return false;

	SINT32				i;
	FLOAT32				px, py, x, y, d;
	TERRAIN_DETAIL*	td;

	// For 2D distance calculation
	px = -playpos.x + (FLOAT32)WorldOriginOffset.x;
	py = -playpos.y + (FLOAT32)WorldOriginOffset.y;

	i = *objcnt;
	td = &trndtl[sc];
	for (; i < td->tobndx; i++)
	{
		// 2D distance calculation
#ifndef LOADEVERYTHING
		x = td->trnobj[i].pos.x - px;
		y = td->trnobj[i].pos.y - py;
//		x = (FLOAT32)((SINT32)x / 800) * 800.0f;
//		y = (FLOAT32)((SINT32)y / 800) * 800.0f;
		d = x * x + y * y;
		if(td->trnobj[i].flags & TDFLAG_LOADFAR || d < horizonsquared)
#endif
		{
			if (IsObjectLoaded(td, i))
			{
				continue;
			}

			*objnum = i;
			*objcnt = i + 1;
			MarkObjectLoaded(td, i);
			return(TRUE);
		}
	}

	return(FALSE);
}

/* NOT USED
static LOGICAL FindObjectsInOctetLoading(UINT32* objnum, UINT32* reccnt, UINT32* objcnt)
{
SINT32				i, j;
FLOAT32				px, py, x, y, d, horizon, horizonFar;
TERRAIN_DETAIL*	td;

	trnmainGetHorizon(&horizon);
	horizon *= horizon;		// To avoid sqrt
	horizonFar = horizon * 4.0f;

	// For 2D distance calculation
	px = -playpos.x + (FLOAT32)WorldOriginOffset.x;
	py = -playpos.y + (FLOAT32)WorldOriginOffset.y;

	i = *reccnt;
	j = *objcnt;
	for (; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if (!activeTerrainDetail[i])
		{
			continue;
		}

		td = &trndtl[i];
		for (; j < td->tobndx; j++)
		{
			// 2D distance calculation
			x = td->trnobj[j].pos.x - px;
			y = td->trnobj[j].pos.y - py;
			x = (FLOAT32)((SINT32)x / 800) * 800.0f;
			y = (FLOAT32)((SINT32)y / 800) * 800.0f;
			d = x * x + y * y;
			if((td->trnobj[j].flags & TDFLAG_LOADFAR && d < 100000.0f) || (!(td->trnobj[j].flags & TDFLAG_LOADFAR) && d < horizon))
			{
				if (IsObjectLoaded(td, j))
				{
					continue;
				}

				*objnum = j;
				*reccnt = i;
				*objcnt = j + 1;
				MarkObjectLoaded(td, j);

				return(TRUE);
			}
		}

		j = 0;
	}


	return(FALSE);
}
*/
static LOGICAL MarkObjectUnloaded(TERRAIN_DETAIL* td, UINT32 objnum)
{
UINT32		i, j;

	i = objnum / TRNOBJ_LOADED_FLAG_SIZE;
	j = objnum % TRNOBJ_LOADED_FLAG_SIZE;
	j = 1 << j;

	td->toldflags[i] &= ~j;

	return(TRUE);
}

static LOGICAL IsObjectUnloaded(TERRAIN_DETAIL* td, UINT32 objnum)
{
UINT32		i, j;

	i = objnum / TRNOBJ_LOADED_FLAG_SIZE;
	j = objnum % TRNOBJ_LOADED_FLAG_SIZE;
	j = 1 << j;

	if (!(td->toldflags[i] & j))
	{
		return(TRUE);
	}

	return(FALSE);
}

static LOGICAL FindObjectsInOctetSCUnloading(UINT32* objnum, SINT32 sc, UINT32* objcnt, FLOAT32 hyster, FLOAT32 hysterFar)
{
	SINT32				i;
	FLOAT32				px, py, x, y, d;
TERRAIN_DETAIL*	td;

	// For 2D distance calculation
	px = -playpos.x + (FLOAT32)WorldOriginOffset.x;
	py = -playpos.y + (FLOAT32)WorldOriginOffset.y;

	i = *objcnt;
	td = &trndtl[sc];
	for (; i < td->tobndx; i++)
	{
		// 2D distance calculation
#ifndef LOADEVERYTHING
		x = td->trnobj[i].pos.x - px;
		y = td->trnobj[i].pos.y - py;
//		x = (FLOAT32)((SINT32)x / 800) * 800.0f;
//		y = (FLOAT32)((SINT32)y / 800) * 800.0f;
		d = x * x + y * y;
		if( (td->trnobj[i].flags & TDFLAG_LOADFAR
             && d > hysterFar)
           || (!(td->trnobj[i].flags & TDFLAG_LOADFAR)
               && d > hyster) )
#endif
		{
			if (IsObjectUnloaded(td, i))
			{
				continue;
			}

			*objnum = i;
			*objcnt = i + 1;
			MarkObjectUnloaded(td, i);
			return(TRUE);
		}
	}

	return(FALSE);
}

/*
static LOGICAL FindObjectsInOctetUnloading(UINT32* objnum, SINT32* reccnt, UINT32* objcnt)
{
SINT32				i, j;
FLOAT32				px, py, x, y, d, horizon, hyster, hysterFar;
TERRAIN_DETAIL*	td;

	trnmainGetHorizon(&horizon);
	hyster = horizon * 1.1f;	// Hysterisis
	hyster *= hyster;			// To avoid sqrt
	hysterFar = hyster * 4.0f;

	// For 2D distance calculation
	px = -playpos.x + (FLOAT32)WorldOriginOffset.x;
	py = -playpos.y + (FLOAT32)WorldOriginOffset.y;

	i = *reccnt;
	j = *objcnt;
	for (; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if (!activeTerrainDetail[i])
		{
			continue;
		}

		td = &trndtl[i];
		for (; j < td->tobndx; j++)
		{
			// 2D distance calculation
			x = td->trnobj[j].pos.x - px;
			y = td->trnobj[j].pos.y - py;
			x = (FLOAT32)((SINT32)x / 800) * 800.0f;
			y = (FLOAT32)((SINT32)y / 800) * 800.0f;
			d = x * x + y * y;
			if((td->trnobj[j].flags & TDFLAG_LOADFAR && d > hysterFar) || (!(td->trnobj[j].flags & TDFLAG_LOADFAR) && d >= hyster))
			{
				if (IsObjectUnloaded(td, i))
				{
					continue;
				}

				*objnum = j;
				*reccnt = i;
				*objcnt = j + 1;
				MarkObjectUnloaded(td, j);
				return(TRUE);
			}
		}

		j = 0;
	}

	return(FALSE);
}
*/

// Return FALSE if more work is needed for specified supercell
static LOGICAL UpdateSupercellState(SINT32 sc)
{
LOGICAL		retval;
SINT32		i;
TERRAIN_DETAIL*	td;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

    logTimerBeginMacro();

	retval = FALSE;

	td = &trndtl[sc];
	switch (td->trnstate.state)
	{
	case TRN_STATE_ERROR:			// Error occurred during the load/unload process
		retval = TRUE;
		break;
	case TRN_STATE_WAIT:			// Wait for visibility or deletion of supercell
		if (td->trnstate.nextstate[0] != 0)
		{
			Write_Debug("State started for supercell %d", sc);

			td->trnstate.state = td->trnstate.nextstate[0];
			td->scx = td->trnstate.nextscx[0];
			td->scy = td->trnstate.nextscy[0];
			RenumberSupercellObjects(td, sc);
			switch (sc)
			{
			case 6:
			case 7:
			case 8:
			case 11:
			case 12:
			case 13:
			case 16:
			case 17:
			case 18:
				td->flags &= ~TDFLAG_PERIPHERY;
				break;
			default:
				td->flags |= TDFLAG_PERIPHERY;
				break;
			}

			if (td->trnstate.state == TRN_STATE_LOAD_OBJECTS)
			{
				td->trnstate.varone = 0;
			}

			if (td->trnstate.state == TRN_STATE_UNLOAD_OBJECTS)
			{
				td->trnstate.varone = 0;
			}

			if (td->trnstate.state == TRN_STATE_UNLOAD_OTHERS)
			{
				td->trnstate.varone = 0;
			}

			for (i=0; i < (MAX_TSB_NEXTSTATES - 1); i++)
			{
				td->trnstate.nextstate[i] = td->trnstate.nextstate[i+1];
				td->trnstate.nextscx[i] = td->trnstate.nextscx[i+1];
				td->trnstate.nextscy[i] = td->trnstate.nextscy[i+1];
			}

			td->trnstate.nextstate[i] = 0;

			break;
		}

		retval = TRUE;
		break;
	case TRN_STATE_LOAD_LOAD:		// Load all elements of a supercell for immediate visibility
		if (!trnldrLoadSupercell(sc))
		{
			Write_Debug("TRN_STATE_LOAD_LOAD failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_MODIFY;
		Write_Debug("State TRN_STATE_LOAD_LOAD finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_MODIFY:		// Modify the supercell elements bases on terrain data
		if (!trnldrModifySupercell(sc))
		{
			Write_Debug("TRN_STATE_LOAD_MODIFY failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_WAIT;
		Write_Debug("State TRN_STATE_LOAD_MODIFY finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_DEFAULT:	// Load the default data into a supercell object
		if (!trnldrLoadSupercell(sc))
		{
			Write_Debug("TRN_STATE_LOAD_DEFAULT failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_FILE;
		Write_Debug("State TRN_STATE_LOAD_DEFAULT finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_FILE:		// Load the DTL file
		if (!trnldrLoadDTL(td))
		{
			Write_Debug("TRN_STATE_LOAD_FILE failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_NORMALS;
		Write_Debug("State TRN_STATE_LOAD_FILE finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_NORMALS:	// Translate terrain normals into vertex normals
        if (!trnldrLoadNormals(td->terrain, td->nm))
		{
			Write_Debug("TRN_STATE_LOAD_NORMALS failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_POINTS;
		Write_Debug("State TRN_STATE_LOAD_NORMALS finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_POINTS:		// Translate terrain points into vertex x, y and z
        if (!trnldrLoadPoints(td->terrain, td->heightmap))
		{
			Write_Debug("TRN_STATE_LOAD_POINTS failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_CALC;
		Write_Debug("State TRN_STATE_LOAD_POINTS finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_CALC:		// Calculate the surface normals and bounding box
		if (!CalculateNormalsAndBounds(td))
		{
			Write_Debug("TRN_STATE_LOAD_CALC failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_LIGHT;
		Write_Debug("State TRN_STATE_LOAD_CALC finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_LIGHT:		// Light surface of terrain
		if (!LightSupercell(td))
		{
			Write_Debug("TRN_STATE_LOAD_LIGHT failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_DUPBASE;
		Write_Debug("State TRN_STATE_LOAD_LIGHT finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_DUPBASE:	// Duplicate the base terrain cell
		if (!DuplicateBaseSupercell(td))
		{
			Write_Debug("TRN_STATE_LOAD_DUPBASE failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_TEXTURES;
		td->trnstate.varone = td->tricnt;
		td->trnstate.vartwo = 0;
		td->trnstate.varthree = 0;
		Write_Debug("State TRN_STATE_LOAD_DUPBASE finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_TEXTURES:	// Load terrain texture objects
		if (!LoadTexturesSequenced(sc))
		{
			break;
		}

#if USE_HEIGHT_AT
        FindPlaneEquations(td->terrainorig, td);
        td->flags |= TDFLAG_PLANES_CREATED;
#endif

		td->trnstate.state = TRN_STATE_LOAD_OCTETS;
		td->trnstate.varone = 0;
		Write_Debug("State TRN_STATE_LOAD_TEXTURES finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_OCTETS:		// Do octet substitution into texture objects
		loading = TRUE;

		if (!LoadOctetsSequenced(sc))
		{
			loading = FALSE;
			break;
		}

		loading = FALSE;
		td->trnstate.state = TRN_STATE_LOAD_BOUNDS;
		td->trnstate.varone = 0;
		Write_Debug("State TRN_STATE_LOAD_OCTETS finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_BOUNDS:		// Calculate bounding information for texture objects
		loading = TRUE;

		if (!LoadBoundsSequenced(sc))
		{
			loading = FALSE;
			break;
		}

		loading = FALSE;
		if (td->flags & TDFLAG_PERIPHERY)
		{
			td->trnstate.state = TRN_STATE_WAIT;
		}
		else
		{
			td->trnstate.state = TRN_STATE_LOAD_OBJECTS;
			td->trnstate.varone = 0;
		}

		Write_Debug("State TRN_STATE_LOAD_BOUNDS finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_OBJECTS:	// Create terrain objects and place on terrain
		loading = TRUE;

		if (!LoadObjectsSequenced(sc))
		{
			loading = FALSE;
			break;
		}

		loading = FALSE;

		if (td->flags & TDFLAG_LOADUNLOAD)
		{
			td->flags &= ~TDFLAG_LOADUNLOAD;
			td->trnstate.state = TRN_STATE_WAIT;
			break;
		}

		td->trnstate.state = TRN_STATE_LOAD_SNAP;
		td->trnstate.varone = 0;
		Write_Debug("State TRN_STATE_LOAD_OBJECTS finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_SNAP:		// Snap objects to XY grid
#if WANT_NON_REPLACEMENT_TILES
        if (!SnapObjectsToGridSequenced(sc))
		{
			break;
		}
#endif
		td->trnstate.state = TRN_STATE_LOAD_DONE;
		Write_Debug("State TRN_STATE_LOAD_SNAP finished for supercell %d", sc);
		break;
	case TRN_STATE_LOAD_DONE:		// Finished all loading
		td->trnstate.state = TRN_STATE_WAIT;
		Write_Debug("State TRN_STATE_LOAD_DONE finished for supercell %d", sc);
		break;
	case TRN_STATE_UNLOAD_FULL:		// Unload all elements of a supercell and release
		if (!trnldrReleaseSupercell(sc))
		{
			Write_Debug("TRN_STATE_UNLOAD_FULL failed");
			td->trnstate.state = TRN_STATE_ERROR;
			break;
		}

		td->trnstate.state = TRN_STATE_WAIT;
		Write_Debug("State TRN_STATE_UNLOAD_FULL finished for supercell %d", sc);
		break;
	case TRN_STATE_UNLOAD_OBJECTS:	// Delete terrain objects from existing supercell

		if (!ReleaseSupercellObjectsSequenced(sc))
		{
			break;
		}

		if (td->flags & TDFLAG_LOADUNLOAD)
		{
			td->flags &= ~TDFLAG_LOADUNLOAD;
			td->trnstate.state = TRN_STATE_WAIT;
			break;
		}


		if (td->flags & TDFLAG_PERIPHERY)
		{
			td->trnstate.state = TRN_STATE_WAIT;
		}
		else
		{
			td->trnstate.state = TRN_STATE_UNLOAD_OTHERS;
			td->trnstate.varone = 0;
		}

		Write_Debug("State TRN_STATE_UNLOAD_OBJECTS finished for supercell %d", sc);
		break;
	case TRN_STATE_UNLOAD_OTHERS:	// Unload remainder of supercell and release

		if (!ReleaseSupercellTextureObjectsSequenced(sc))
		{
			break;
		}

		td->numheightmapelements = 0;
		td->tricnt	= 0;
		td->trindx	= 0;
		td->tobndx	= 0;

		if(td->pcp)
			memFree(td->pcp);
		td->pcp = NULL;
		td->numCPs = 0;

		if (td->numFacilities > 0)
		{
			memFree(td->facTable);
			td->pfac = NULL;
			td->numFacilities = 0;
		}

		if (td->numBuildings > 0)
		{
			td->buildingTable->destroy(td->buildingTable);
			td->pbld = NULL;
			td->numBuildings = 0;
		}

		if (td->numLinks > 0)
		{
			td->plink = NULL;
			td->numLinks = 0;
		}

		if (td->numWaypoints > 0)
		{
			td->pwp = NULL;
			td->numWaypoints = 0;
		}

        if(td->m_trees)
        {
            sptFreeClutter(td->m_trees);
//            delete td->m_trees;
			_treePool.push(td->m_trees);
            td->m_trees = NULL;
        }

		if ( td->bldHash.building != NULL)
		{
			memFree(td->bldHash.building);
			td->bldHash.building = NULL;
		}

		td->bldHash.maxBldID = 0;

		if (td->toldflags != NULL)
		{
			memFree(td->toldflags);
			td->toldflags = NULL;
		}

		trncdDTLColliderFree(td->collider);
		td->collider = NULL;

		td->stratLoaded = FALSE;

		td->trnstate.state = TRN_STATE_WAIT;
		Write_Debug("State TRN_STATE_UNLOAD_OTHERS finished for supercell %d", sc);
		break;
	default:
		Write_Debug("Undefined state during UpdateSupercellState");
		td->trnstate.state = TRN_STATE_ERROR;
		break;
	}

    logTimerEndMacro();

	return(retval);
}

LOGICAL trnldrUpdate(void)
{
LOGICAL		retval;
UINT32		start, elapse, numticks;
SINT32		numframes, numupdates;
LOGICAL		loadingstate;

static SINT32		cell=0;

	// Number of frames to wait between updates
	numframes = TRNLDR_MIN_FRAMES_PER_STATE * FloatToInt(InvFrameFactor);
	if (numframes < 1)
	{
		numframes = 1;
	}
	else
	if (numframes > TRNLDR_MAX_FRAMES_PER_STATE)
	{
		numframes = TRNLDR_MAX_FRAMES_PER_STATE;
	}

	if ((piGetTotalPaints() % numframes) != 0)
    {
		return(TRUE);
	}

    logTimerBeginMacro();

	// Number of ticks to use during the update process
	numticks = FloatToInt(TRNLDR_MAX_LOAD_TIME * FrameFactor);
	if (numticks < TRNLDR_MIN_LOAD_TIME)
	{
		numticks = TRNLDR_MIN_LOAD_TIME;
	}

	// Start timing this cycle of the update process
	start = timeGetTime();

	numupdates = 0;
	loadingstate = TerrainStable;

	// Handle all initial loading tasks
	// These activities must be done first as a basis for further loading processes
	do
	{
		if ((trndtl[cell].trnstate.state >= TRN_STATE_LOAD_DEFAULT &&	// Load the default data into a supercell object
			trndtl[cell].trnstate.state <= TRN_STATE_LOAD_CALC) ||		// Calculate the surface normals and bounding box
			trndtl[cell].trnstate.state == TRN_STATE_WAIT)				// Wait for visibility or deletion of supercell

		{
			// Returns FALSE if more work is needed
			retval = UpdateSupercellState(cell);
			if (retval == FALSE)
			{
				numupdates += 1;
			}
		}
		else
		{
			retval = TRUE;
		}

		// Abort based on time
		elapse = timeGetTime() - start;
		if (elapse >= numticks)
		{
			break;
		}

		// If this supercell's state is stable, continue to next supercell
		if (retval == TRUE)
		{
			cell += 1;
			if (cell >= NUM_TERRAIN_RECTANGLES)
			{
				// Always break between loops
				cell = 0;
				break;
			}
		}
	}
	while (TRUE);

	// Only if time is remaining and the initial loading tasks are complete
	if (elapse < numticks &&
		numupdates == 0)
	{
		// Handle center (most likely to be viewed) supercells first
		// This should reduce visibility issues with slow computers
		do
		{
			if (!(trndtl[cell].flags & TDFLAG_PERIPHERY))
			{
				// Returns FALSE if more work is needed
				retval = UpdateSupercellState(cell);
				if (retval == FALSE)
				{
					numupdates += 1;
				}
			}
			else
			{
				retval = TRUE;
			}

			// Abort based on time
			elapse = timeGetTime() - start;
			if (elapse >= numticks)
			{
				break;
			}

			// If this supercell's state is stable, continue to next supercell
			if (retval == TRUE)
			{
				cell += 1;
				if (cell >= NUM_TERRAIN_RECTANGLES)
				{
					// Always break between loops
					cell = 0;
					break;
				}
			}
		}
		while (TRUE);
	}

	// Only if time is remaining and the initial loading tasks are complete
	if (elapse < numticks &&
		numupdates == 0)
	{
		// Handle periphery supercells first
		do
		{
			// Returns FALSE if more work is needed
			retval = UpdateSupercellState(cell);
			if (retval == FALSE)
			{
				numupdates += 1;
			}

			// Abort based on time
			elapse = timeGetTime() - start;
			if (elapse >= numticks)
			{
				break;
			}

			// If this supercell's state is stable, continue to next supercell
			if (retval == TRUE)
			{
				cell += 1;
				if (cell >= NUM_TERRAIN_RECTANGLES)
				{
					// Always break between loops
					cell = 0;
					break;
				}
			}
		}
		while (TRUE);
	}

	TerrainStable = numupdates == 0;
	if (!loadingstate && TerrainStable)
	{
		if (ObjectsWereAdded)
		{
			TerrainLightingUpdate = TRUE;
			ObjectsWereAdded = FALSE;
		}
	}

	if (TerrainStable && ft50msticks)
	{
	static LOGICAL	unload=FALSE;
	static SINT32	cell=0;

		if (!unload)
		{
			if (activeTerrainDetail[cell])
			{
				trndtl[cell].flags |= TDFLAG_LOADUNLOAD;
				trndtl[cell].trnstate.state = TRN_STATE_LOAD_OBJECTS;
				trndtl[cell].trnstate.varone = 0;
				//trndtl[cell].trnstate.nextscx[0] = trndtl[cell].scx;
				//trndtl[cell].trnstate.nextscy[0] = trndtl[cell].scy;
			}

			cell += 1;
			if (cell >= NUM_TERRAIN_RECTANGLES)
			{
				cell = 0;
				unload = TRUE;
			}
		}
		else
		{
			if (activeTerrainDetail[cell])
			{
				//trndtl[cell].flags |= TDFLAG_LOADUNLOAD;
				//trndtl[cell].trnstate.state = TRN_STATE_UNLOAD_OBJECTS;
				//trndtl[cell].trnstate.varone = 0;
				//trndtl[cell].trnstate.nextscx[0] = trndtl[cell].scx;
				//trndtl[cell].trnstate.nextscy[0] = trndtl[cell].scy;
			}

			cell += 1;
			if (cell >= NUM_TERRAIN_RECTANGLES)
			{
				cell = 0;
				unload = FALSE;
			}
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

LOGICAL trnldrFinishUpdate(void)
{
LOGICAL		retval;
SINT32		cell=0;
LOGICAL		loadingstate;
UINT32		iterations=0;

    logTimerBeginMacro();

	loadingstate = TerrainStable;

	// Handle all tasks
	do
	{
		// Returns FALSE if more work is needed
		retval = UpdateSupercellState(cell);

		if (Online == 1)
		{
			// Only process the network stuff if we've
			// been here for a while.
			if(((++iterations) & 0x1f) == 0)
			{
				teulUpdateTransport(); // process any new messages
			}
		}

		// If this supercell's state is stable, continue to next supercell
		if (retval == TRUE)
		{
			// Always break between loops
			cell += 1;
			if (cell >= NUM_TERRAIN_RECTANGLES)
			{
				// Always break between loops
				break;
			}
		}
	}
	while (TRUE);

	TerrainStable = TRUE;
	if (!loadingstate)
	{
		if (ObjectsWereAdded)
		{
			TerrainLightingUpdate = TRUE;
			ObjectsWereAdded = FALSE;
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

LOGICAL trnldrFinishUpdateRelocate(void)
{
LOGICAL		retval;
SINT32		cell=0;
LOGICAL		loadingstate;

    logTimerBeginMacro();

	loadingstate = TerrainStable;
	if (!TerrainStable)
	{
		trnldrFinishUpdate();
	}

	// Mark all tasks that need to be updated
	do
	{
		if (activeTerrainDetail[cell])
		{
			trndtl[cell].flags |= TDFLAG_LOADUNLOAD;
			trndtl[cell].trnstate.nextstate[0] = TRN_STATE_LOAD_OBJECTS;
			trndtl[cell].trnstate.nextscx[0] = trndtl[cell].scx;
			trndtl[cell].trnstate.nextscy[0] = trndtl[cell].scy;
		}

		cell += 1;
		if (cell >= NUM_TERRAIN_RECTANGLES)
		{
			cell = 0;
			break;
		}
	}
	while(TRUE);

	UINT32 iterations = 0;
	// Handle all tasks
	do
	{
		// Returns FALSE if more work is needed
		retval = UpdateSupercellState(cell);

		if (Online == 1)
		{
			// Only process the network stuff if we've
			// been here for a while.
			if(((++iterations) & 0x1f) == 0)
			{
				teulUpdateTransport(); // process any new messages
			}
		}

		// If this supercell's state is stable, continue to next supercell
		if (retval == TRUE)
		{
			// Always break between loops
			cell += 1;
			if (cell >= NUM_TERRAIN_RECTANGLES)
			{
				// Always break between loops
				break;
			}
		}
	}
	while (TRUE);

	TerrainStable = TRUE;
	if (!loadingstate)
	{
		if (ObjectsWereAdded)
		{
			TerrainLightingUpdate = TRUE;
			ObjectsWereAdded = FALSE;
		}
	}

    logTimerEndMacro();

	return(TRUE);
}

LOGICAL trnldrStartLoadingAll(SINT32 sc, SINT16 scx, SINT16 scy)
{
SINT32		i;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

	// Only center 9 supercells can get this call
	// so make sure the Periphery flag is clear
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		if (trndtl[sc].trnstate.nextstate[i] == 0)
		{
			Write_Debug("State added to queue %d in trnldrStartLoadingAll for supercell %d", i, sc);
			trndtl[sc].trnstate.nextstate[i] = TRN_STATE_LOAD_DEFAULT;
			trndtl[sc].trnstate.nextscx[i] = scx;
			trndtl[sc].trnstate.nextscy[i] = scy;
			break;
		}
	}

	if (i == MAX_TSB_NEXTSTATES)
	{
		Write_Debug("State queue is full in trnldrStartLoadingAll for supercell %d", sc);
	}

	return(TRUE);
}

LOGICAL trnldrStartLoadingFirstHalf(SINT32 sc, SINT16 scx, SINT16 scy)
{
SINT32		i;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

	// Supercells that are just coming into existence
	// and take one of the periphery slots only gets
	// the first half of the data loaded
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		if (trndtl[sc].trnstate.nextstate[i] == 0)
		{
			Write_Debug("State added to queue %d in trnldrStartLoadingFirstHalf for supercell %d", i, sc);
			trndtl[sc].trnstate.nextstate[i] = TRN_STATE_LOAD_DEFAULT;
			trndtl[sc].trnstate.nextscx[i] = scx;
			trndtl[sc].trnstate.nextscy[i] = scy;
			break;
		}
	}

	if (i == MAX_TSB_NEXTSTATES)
	{
		Write_Debug("State queue is full in trnldrStartLoadingFirstHalf for supercell %d", sc);
	}

	return(TRUE);
}

LOGICAL trnldrStartLoadingSecondHalf(SINT32 sc, SINT16 scx, SINT16 scy)
{
SINT32		i;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

	// Only periphery supercells that are moving to
	// one of the center 9 supercells can get this call
	// so make sure the Periphery flag is clear
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		if (trndtl[sc].trnstate.nextstate[i] == 0)
		{
			Write_Debug("State added to queue %d in trnldrStartLoadingSecondHalf for supercell %d", i, sc);
			trndtl[sc].trnstate.nextstate[i] = TRN_STATE_LOAD_OBJECTS;
			trndtl[sc].trnstate.nextscx[i] = scx;
			trndtl[sc].trnstate.nextscy[i] = scy;
			break;
		}
	}

	if (i == MAX_TSB_NEXTSTATES)
	{
		Write_Debug("State queue is full in trnldrStartLoadingSecondHalf for supercell %d", sc);
	}

	return(TRUE);
}

LOGICAL trnldrStartUnloadingAll(SINT32 sc)
{
SINT32		i;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

	// Only supercells that are not periphery supercells
	// but must be completely scrapped should get this call
	// This is probably due to a warp to another position
	// in the world
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		if (trndtl[sc].trnstate.nextstate[i] == 0)
		{
			Write_Debug("State added to queue %d in trnldrStartUnloadingAll for supercell %d", i, sc);
			trndtl[sc].trnstate.nextstate[i] = TRN_STATE_UNLOAD_OBJECTS;
			trndtl[sc].trnstate.nextscx[i] = trndtl[sc].scx;
			trndtl[sc].trnstate.nextscy[i] = trndtl[sc].scy;
			break;
		}
	}

	if (i == MAX_TSB_NEXTSTATES)
	{
		Write_Debug("State queue is full in trnldrStartUnloadingAll for supercell %d", sc);
	}

	return(TRUE);
}

LOGICAL trnldrStartUnloadingFirstHalf(SINT32 sc)
{
SINT32		i;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

	// Supercells that are moving from a center supercell
	// to an outer, periphery supercell location get
	// this call. They become periphery objects
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		if (trndtl[sc].trnstate.nextstate[i] == 0)
		{
			Write_Debug("State added to queue %d in trnldrStartUnloadingFirstHalf for supercell %d", i, sc);
			trndtl[sc].trnstate.nextstate[i] = TRN_STATE_UNLOAD_OBJECTS;
			trndtl[sc].trnstate.nextscx[i] = trndtl[sc].scx;
			trndtl[sc].trnstate.nextscy[i] = trndtl[sc].scy;
			break;
		}
	}

	if (i == MAX_TSB_NEXTSTATES)
	{
		Write_Debug("State queue is full in trnldrStartUnloadingFirstHalf for supercell %d", sc);
	}

	return(TRUE);
}

LOGICAL trnldrStartUnloadingSecondHalf(SINT32 sc)
{
SINT32		i;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

	// Supercells that are going away need to be fully
	// destroyed. This call finishes that destruction
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		if (trndtl[sc].trnstate.nextstate[i] == 0)
		{
			Write_Debug("State added to queue %d in trnldrStartUnloadingSecondHalf for supercell %d", i, sc);
			trndtl[sc].trnstate.nextstate[i] = TRN_STATE_UNLOAD_OTHERS;
			trndtl[sc].trnstate.nextscx[i] = trndtl[sc].scx;
			trndtl[sc].trnstate.nextscy[i] = trndtl[sc].scy;
			break;
		}
	}

	if (i == MAX_TSB_NEXTSTATES)
	{
		Write_Debug("State queue is full in trnldrStartUnloadingSecondHalf for supercell %d", sc);
	}

	return(TRUE);
}

LOGICAL trnldrReleaseSupercell(SINT32 sc)
{
SINT32		i;
TERRAIN_DETAIL*	td;

	if (sc < 0 || sc >= NUM_TERRAIN_RECTANGLES)
	{
		return(FALSE);
	}

    logTimerBeginMacro();

	Write_Debug("trnldrReleaseSupercell called for supercell %d", sc);

	td = &trndtl[sc];

	//clientStrtQueueReleaseSupercell(td->scx, td->scy);

	ReleaseSupercellTextureObjects(td);

	ReleaseSupercellObjects(td);

	td->numheightmapelements = 0;
	td->tricnt	= 0;
	td->trindx	= 0;
	td->tobndx	= 0;

	if(td->pcp)
		memFree(td->pcp);
	td->pcp = NULL;
	td->numCPs = 0;

	if (td->numFacilities > 0)
	{
		memFree(td->facTable);
		td->pfac = NULL;
		td->numFacilities = 0;
	}

	if (td->numBuildings > 0)
	{
		td->buildingTable->destroy(td->buildingTable);
		td->pbld = NULL;
		td->numBuildings = 0;
	}

	if (td->numLinks > 0)
	{
		td->plink = NULL;
		td->numLinks = 0;
	}

	if (td->numWaypoints > 0)
	{
		td->pwp = NULL;
		td->numWaypoints = 0;
	}

    if(td->m_trees)
    {
        sptFreeClutter(td->m_trees);
//        delete td->m_trees;

		_treePool.push(td->m_trees);
        td->m_trees = NULL;
    }


	td->trnstate.state = TRN_STATE_WAIT;
	for (i=0; i < MAX_TSB_NEXTSTATES; i++)
	{
		trndtl[sc].trnstate.nextstate[i] = 0;
	}

	if ( td->bldHash.building != NULL)
	{
		memFree(td->bldHash.building);
		td->bldHash.building = NULL;
	}

	td->bldHash.maxBldID = 0;

	if (td->toldflags != NULL)
	{
		memFree(td->toldflags);
		td->toldflags = NULL;
	}

	trncdDTLColliderFree(td->collider);
	td->collider = NULL;

	td->stratLoaded = FALSE;

#ifdef SPATIAL_DB
    SDB_Empty_Supercell( td );
#endif

    logTimerEndMacro();

	return(TRUE);
}

LOGICAL trnldrSetTextureRotation(OBJECT* lpobj, SINT32 tuv, SINT32 octet)
{
SINT32		i, num;
FLOAT32		widthfactor, heightfactor;
TUV*		ptuv;
piTexCoordPtr tex;

    extern LOGICAL trnwtrIsWaterTexture(SINT32);

	if (lpobj == NULL)
	{
		return(FALSE);
	}

    if(trnwtrIsWaterTexture(lpobj->texid))
    {
        // Don't rotate water
	    ptuv = &tuvlist[0][0];
        // Non replacement tiles
        glslOceanSetup(lpobj);
    }
    else
    {
	    ptuv = &tuvlist[tuv & 0x3][0];
    }

	piGetTextureFactors(lpobj->texid, 0, &widthfactor, &heightfactor);

	num = octet * NUM_VERTICES_PER_OCTET;
	tex = lpobj->StaticTexCoords();
	for (i=0; i < NUM_VERTICES_PER_OCTET; num++, i++)
	{
		tex[num].u	= ptuv[i].tu * widthfactor;
		tex[num].v	= ptuv[i].tv * heightfactor;
	}

	return(TRUE);
}

void trnldrInitializeSuperCellVertices(OBJECT* tt, int scx, int scy)
{
    SINT32 i, j, k, l, w, h, ofs, ndx;
    FLOAT32 xl, xr, yt, yb;

	yb = scy * SUPERCELL_HEIGHT;
	xl = scx * SUPERCELL_WIDTH;
	yt = yb + SUPERCELL_HEIGHT;
	xr = xl + SUPERCELL_WIDTH;

	w = SUPERCELL_SIZE_X1;
	h = SUPERCELL_SIZE_Y1;

	piVectorPtr pos = tt->StaticPositions();
	piVectorPtr norm = tt->StaticNormals();
	piVertexColorPtr color = tt->StaticColors();
	piTexCoordPtr tex = tt->StaticTexCoords();
	piINDICES* pt = tt->ptris;

	tt->rot.x = (FLOAT32)WorldOriginOffset.x;
	tt->rot.y = (FLOAT32)WorldOriginOffset.y;
	tt->rot.z = 0;

    piVECTOR n;
    n.set(0, 0, -1);

	TUV* ptuv = tuvlist[0];

	ofs = 0;
	ndx = 0;
	for (i=0; i < h; i+=2)
	{
		for (j=0; j < w; j+=2)
		{
			// Arrange each octet into triangles
			// 0
			k = 1;
			l = 1;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[0].tu;
			tex->v = ptuv[0].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 1
			k = 0;
			l = 0;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[1].tu;
			tex->v = ptuv[1].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 2
			k = 1;
			l = 0;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[2].tu;
			tex->v = ptuv[2].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 3
			k = 2;
			l = 0;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[3].tu;
			tex->v = ptuv[3].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 4
			k = 2;
			l = 1;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[4].tu;
			tex->v = ptuv[4].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 5
			k = 2;
			l = 2;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[5].tu;
			tex->v = ptuv[5].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 6
			k = 1;
			l = 2;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[6].tu;
			tex->v = ptuv[6].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 7
			k = 0;
			l = 2;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[7].tu;
			tex->v = ptuv[7].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;

			// 8
			k = 0;
			l = 1;
			pos->x = xl + (FLOAT32)(j + l) * xstep;
			pos->y = yb + (FLOAT32)(i + k) * ystep;
			pos->x -= tt->rot.x;
			pos->y -= tt->rot.y;
			pos->z = 0.0f;
			norm->x = n.x;
			norm->y = n.y;
			norm->z = n.z;
			tex->u = ptuv[8].tu;
			tex->v = ptuv[8].tv;
			color->specular = colorALPHA100;

			++pos; ++norm; ++tex; ++color;
			ofs		+= 1;


			// 1
			pt->v1	= ndx + 1;
			pt->v2	= ndx + 2;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 2
			pt->v1	= ndx + 2;
			pt->v2	= ndx + 3;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 3
			pt->v1	= ndx + 3;
			pt->v2	= ndx + 4;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 4
			pt->v1	= ndx + 4;
			pt->v2	= ndx + 5;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 5
			pt->v1	= ndx + 5;
			pt->v2	= ndx + 6;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 6
			pt->v1	= ndx + 6;
			pt->v2	= ndx + 7;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 7
			pt->v1	= ndx + 7;
			pt->v2	= ndx + 8;
			pt->v3	= ndx + 0;

			pt		+= 1;

			// 8
			pt->v1	= ndx + 8;
			pt->v2	= ndx + 1;
			pt->v3	= ndx + 0;

			pt		+= 1;

			ndx		+= NUM_VERTICES_PER_OCTET;
		}
	}
	tt->numverts = SUPERCELL_OCTETS_XY * NUM_VERTICES_PER_OCTET;
	tt->SyncMutableData();

	if (tt->numverts != ofs)
	{
		Write_Debug("Incorrect number of vertices created");
	}

	if (tt->numverts != ndx)
	{
		Write_Debug("Incorrect number of triangles created");
	}

	tt->numtris = SUPERCELL_OCTETS_XY * NUM_TRIANGLES_PER_OCTET;

	trnmainSetTerrainMatrix(tt);
	tt->flags |= OBJECT_USEMATRIX;

	tt->flags |= OBJECT_LITE;
	tt->flags |= OBJECT_DYNALITE;
	tt->flags |= OBJECT_VISIBLE;
//	tt->flags |= OBJECT_SKIPDRAWING;
	tt->flags |= OBJECT_INDEXED;
	tt->flags |= OBJECT_TRIANGLELIST;
//	tt->flags |= OBJECT_NOSPECULAR;
	tt->flags |= OBJECT_COLLISION;
	tt->flags |= OBJECT_TERRAIN;

    // Remove object from standard rendering system
    obmainRemoveObjectTree(tt);
}

LOGICAL trnldrLoadSupercell(SINT32 index)
{
OBJECT*			tt;
TERRAIN_DETAIL*	td;

    logTimerBeginMacro();

	td = &trndtl[index];
	tt = td->terrain;

	if(_treePool.empty())
		_treePool.push(new TDTL_TREES[SUPERCELL_OCTETS_X * SUPERCELL_OCTETS_Y]);

    td->m_trees = _treePool.top();

	_treePool.pop();

    sptFreeTrees(td->m_trees);
	//    td->m_trees = new TDTL_TREES[SUPERCELL_OCTETS_X * SUPERCELL_OCTETS_Y];


	td->numheightmapelements = SUPERCELL_SIZE_XY;
	td->nmcnt = SUPERCELL_SIZE_XY;
	for(int i=0; i < SUPERCELL_SIZE_XY; i++)
	{
		td->heightmap[i] = (SINT16)TERRAIN_NODATA;		// sea level
		td->nm[i] = (SINT16)TERRAIN_NONORMALS;	// straight up
	}

    trnldrInitializeSuperCellVertices(tt, td->scx, td->scy);

    logTimerEndMacro();

#ifndef USE_3DAPI_LIGHTING
	if (!(tt->flags & OBJECT_DYNALITE))
	{
	int         numlights;
	LIGHT*     lplite;

		numlights = piGetLights(&lplite);
		if (!obsliteCalculate(1, tt, numlights, lplite, piGetAmbLight()))
		{
			return(FALSE);
		}
	}
#endif

	return(TRUE);
}

LOGICAL trnldrModifySupercell(SINT32 sc)
{
//LOGICAL			retval=TRUE;
TERRAIN_DETAIL*	td;

#ifdef EVENT_LOG_TIMES
LARGE_INTEGER norm_count1, norm_count2;
static __int64    count=0;
#endif //EVENT_LOG_TIMES



#ifdef EVENT_LOG_TIMES
	if (!QueryPerformanceCounter(&norm_count1))
	{
		//Error
	}
#endif //EVENT_LOG_TIMES

	td = &trndtl[sc];

	if (loading)
	{
		Write_Debug("'loading' was TRUE upon entry to trnldrModifySupercell");
	}

	if (!trnldrLoadDTL(td))
	{

		return(FALSE);
	}

    if (!trnldrLoadNormals(td->terrain, td->nm))
	{

		return(FALSE);
	}

    if (!trnldrLoadPoints(td->terrain, td->heightmap))
	{

		return(FALSE);
	}

	if (!CalculateNormalsAndBounds(td))
	{

		return(FALSE);
	}

	if (!LightSupercell(td))
	{

		return(FALSE);
	}

	if (!DuplicateBaseSupercell(td))
	{

		return(FALSE);
	}

	if (!LoadTextures(sc))
	{

		return(FALSE);
	}

	loading = TRUE;

	if (!LoadOctets(td))
	{

		loading = FALSE;
		return(FALSE);
	}

	if (!LoadBounds(td))
	{

		loading = FALSE;
		return(FALSE);
	}

#if WANT_NON_REPLACEMENT_TILES
    if(!SnapObjectsToGrid(td))
    {
	    return(FALSE);
    }
#endif

#if USE_HEIGHT_AT
    FindPlaneEquations(td->terrainorig, td);
    td->flags |= TDFLAG_PLANES_CREATED;
#endif

	if (!LoadObjects(sc))
	{

		loading = FALSE;
		return(FALSE);
	}

	loading = FALSE;

	td->trnstate.state = TRN_STATE_WAIT;

#ifdef EVENT_LOG_TIMES
	if (QueryPerformanceCounter(&norm_count2))
	{
		count += norm_count2.QuadPart - norm_count1.QuadPart;
		QueryPerformanceFrequency(&norm_count2);
		count = 0;
	}
#endif //EVENT_LOG_TIMES

	return(TRUE);
}
