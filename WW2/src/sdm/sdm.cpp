#include <syscfg.h> // Path is project specific.

#include <string.h>
#include <ctype.h>

#include "sdm/sdmDefines.h"
#include "sdm/sdmEnums.h"
#include "sdm/sdm.h"
#include "sdm/sdmObjInfo.h"
#include "sdm/sdmReplacementOctedData.h"

/************************************************************************************************
	sdm
*************************************************************************************************/

#define GETC(fd) readc(fd, readfn)
#define GETS(fd) reads(fd, readfn)
#define GETL(fd) readl(fd, readfn)
#define GETF(fd) readf(fd, readfn)

static void un_rle(SINT16 inlen, SINT16* indata, SINT16* outlen, SINT16* outdata);

using READFN = size_t(*)(int, void*, size_t);

static SINT8   readc(int fd, READFN const);
static SINT16  reads(int fd, READFN const);
static SINT32  readl(int fd, READFN const);
static float	readf(int fd, READFN const);

static void normalize_path(char*);
static void filter_name(char*);

/************************************************************************************************
	read_dtl_data
*************************************************************************************************/

int sdm_read_dtl_data(int fd, READFN const readfn, sdm_dtl_data* dtl)
{
	sdm_header header;
	int datalen;
	SINT16 data[SDM_SUPERCELL_SIZE_XY];

	header.company		= GETS(fd);
	header.filetype		= GETS(fd);
	header.version		= GETS(fd);
	header.flags		= GETS(fd);
	header.dataversion 	= GETL(fd);
	header.length 		= GETL(fd);

	datalen = GETS(fd);
	readfn(fd, data, datalen);

	if (header.flags & SDM_FLAG_DATA_RLE)
	{
		un_rle(datalen/2, data, &dtl->zcnt, dtl->z);
	} else
	{
		memcpy(dtl->z, data, datalen);
		dtl->zcnt = datalen/2;
	}

	datalen = GETS(fd);
	readfn(fd, data, datalen);

	if (header.flags & SDM_FLAG_NORMALS_RLE)
	{
		un_rle(datalen/2, data, &dtl->normcnt, dtl->norm);
	} else
	{
		memcpy(dtl->norm, data, datalen);
		dtl->normcnt = datalen/2;
	}

	return 0;
}

/************************************************************************************************
	sdm_read_replacement_data
*************************************************************************************************/

int sdm_read_replacement_data(int fd, READFN const readfn, sdm_replacement_octet_data* data)
{
	int i;
	sdm_terrain_info* trninfo;
	SDM_ObjInfo* objinfo;

	data->numtex = GETS(fd);
	if(data->numtex < 1) return -1;
	readfn(fd, data->texname, data->numtex * 32);

	data->numtrninfo = GETS(fd);
	if(data->numtrninfo < 1) return -1;

	trninfo = data->trninfo;
    for(i = 0; i < data->numtrninfo; i++, trninfo++)
    {
		trninfo->octet  = GETS(fd);
		trninfo->octnum = GETS(fd);
		trninfo->tonum  = GETC(fd);
		trninfo->txndx  = GETC(fd);
		trninfo->ortdrg = GETC(fd);
		trninfo->ruftrn = GETC(fd);
    }

	data->numobjinfo = GETS(fd);
	if (data->numobjinfo < 1) return -1;

	objinfo = data->objinfo;
    for(i = 0; i < data->numobjinfo; i++, objinfo++)
    {
		for(int p=0; p < sizeof(piMATRIX) / sizeof(float); p++)
		{
			objinfo->transform.m[p/4][p%4] = GETF(fd);
		}
		objinfo->pos.x = GETF(fd);
		objinfo->pos.y = GETF(fd);
		objinfo->pos.z = GETF(fd);
		readfn(fd, objinfo->objname, 32);
		normalize_path(objinfo->objname);
		objinfo->txndx  = GETC(fd);
		objinfo->ortdrg = GETC(fd);
		objinfo->ruftrn = GETC(fd);
		objinfo->flags = GETC(fd);
    }

	return 0;
}

/************************************************************************************************
	sdm_read_strat_data
*************************************************************************************************/

int sdm_read_strat_data(int fd, READFN const readfn, sdm_strat_data* strat_data)
{
	int i;
	sdm_strat_header header;
	sdm_cp*			cp		= strat_data->cp;
	sdm_facility*   fac		= strat_data->facility;
	sdm_building*   bld		= strat_data->building;
	sdm_depot*		depot   = strat_data->depot;
	sdm_waypoint*   waypt   = strat_data->waypoint;

	strat_data->numcps   = header.numCPs		= GETS(fd);
	strat_data->numfacs  = header.numFacilities	= GETS(fd);
	strat_data->numblds  = header.numBuildings  = GETS(fd);
	strat_data->numdepots= header.numLinks		= GETS(fd);
	strat_data->numwaypts= header.numWaypoints  = GETS(fd);
	header.reserved1		= GETS(fd);
	header.reserved2		= GETS(fd);
	header.reserved3		= GETS(fd);

	if(header.numCPs > SDM_MAX_STRAT_LIST)			return -1;
	if(header.numFacilities > SDM_MAX_STRAT_LIST)   return -1;
	if(header.numBuildings > SDM_MAX_STRAT_LIST)	return -1;
	if(header.numLinks > SDM_MAX_STRAT_LIST)		return -1;
	if(header.numLinks > SDM_MAX_STRAT_LIST)		return -1;

	for(i = 0; i < header.numCPs; i++, cp++)
	{
		cp->oid.id			= GETS(fd);
		cp->oid.objClass	= GETC(fd);
		cp->oid.objType		= GETC(fd);
		cp->serial			= GETL(fd);
		cp->posx			= GETL(fd);
		cp->posy			= GETL(fd);
		cp->subType			= GETC(fd);
		cp->side			= GETC(fd);
		cp->country			= GETC(fd);
		cp->stratFlags		= GETC(fd);
		readfn(fd,cp->name,SDM_MAX_STRT_NAME_LENGTH);
		readfn(fd,cp->fileName,SDM_MAX_OBJNAME_LENGTH);
		normalize_path((char*)cp->fileName);
		filter_name((char*)cp->name);
	}

	for(i = 0; i < header.numFacilities; i++, fac++)
	{
		fac->oid.id			= GETS(fd);
		fac->oid.objClass   = GETC(fd);
		fac->oid.objType	= GETC(fd);
		fac->serial			= GETL(fd);
		fac->cpid.cell.ox   = GETS(fd) & 0xfff0;
		fac->cpid.cell.oy   = GETS(fd) & 0xfff0;
		fac->cpid.id		= GETS(fd);
		fac->cpid.objClass  = GETC(fd);
		fac->cpid.objType   = GETC(fd);
		fac->posx			= GETL(fd);
		fac->posy			= GETL(fd);
		fac->subType		= GETC(fd);
		fac->originalSide   = GETC(fd);
		fac->originalCountry= GETC(fd);
		fac->stratFlags		= GETC(fd);
		readfn(fd,fac->name,SDM_MAX_STRT_NAME_LENGTH);
		readfn(fd,fac->fileName,SDM_MAX_OBJNAME_LENGTH);
		normalize_path((char*)fac->fileName);
		filter_name((char*)fac->name);
	}

	for(i = 0; i < header.numBuildings; i++, bld++)
	{
		bld->oid.id			= GETS(fd);
		bld->oid.objClass   = GETC(fd);
		bld->oid.objType	= GETC(fd);
		bld->serial			= GETL(fd);
		bld->pid.id			= GETS(fd);
		bld->pid.objClass   = GETC(fd);
		bld->pid.objType	= GETC(fd);
		bld->posx			= GETL(fd);
		bld->posy			= GETL(fd);
		bld->subType		= GETC(fd);
		GETC(fd); // control used to be stored here.
		bld->stratFlags		= GETC(fd);
		readfn(fd,bld->name,SDM_MAX_STRT_NAME_LENGTH);
		readfn(fd,bld->fileName,SDM_MAX_OBJNAME_LENGTH);
		normalize_path((char*)bld->fileName);
		filter_name((char*)bld->name);
	}

	for(i = 0; i < header.numLinks; i++, depot++)
	{
		depot->oid.id				= GETS(fd);
		depot->oid.objClass			= GETC(fd);
		depot->oid.objType			= GETC(fd);
		depot->serial				= GETL(fd);
		depot->startDepotId.cell.ox = GETS(fd) & 0xfff0;
		depot->startDepotId.cell.oy = GETS(fd) & 0xfff0;
		depot->startDepotId.id		= GETS(fd);
		depot->startDepotId.objClass= GETC(fd);
		depot->startDepotId.objType = GETC(fd);
		depot->endDepotId.cell.ox   = GETS(fd) & 0xfff0;
		depot->endDepotId.cell.oy   = GETS(fd) & 0xfff0;
		depot->endDepotId.id		= GETS(fd);
		depot->endDepotId.objClass  = GETC(fd);
		depot->endDepotId.objType   = GETC(fd);
		depot->firebaseId.cell.ox   = GETS(fd) & 0xfff0;
		depot->firebaseId.cell.oy   = GETS(fd) & 0xfff0;
		depot->firebaseId.id		= GETS(fd);
		depot->firebaseId.objClass  = GETC(fd);
		depot->firebaseId.objType   = GETC(fd);
		depot->distance				= GETL(fd);
		depot->stratFlags			= GETC(fd);
		readfn(fd,depot->name,SDM_MAX_STRT_NAME_LENGTH);
		filter_name((char*)depot->name);
	}

	for(i = 0; i < header.numWaypoints; i++, waypt++)
	{
		waypt->oid.id			= GETS(fd);
		waypt->oid.objClass		= GETC(fd);
		waypt->oid.objType		= GETC(fd);
		waypt->serial			= GETL(fd);
		waypt->pid.id			= GETS(fd);
		waypt->pid.objClass		= GETC(fd);
		waypt->pid.objType		= GETC(fd);
		waypt->posx				= GETL(fd);
		waypt->posy				= GETL(fd);
		waypt->subType			= GETC(fd);
		waypt->nextpt.cell.ox   = GETS(fd) & 0xfff0;
		waypt->nextpt.cell.oy   = GETS(fd) & 0xfff0;
		waypt->nextpt.id		= GETS(fd);
		waypt->nextpt.objClass  = GETC(fd);
		waypt->nextpt.objType   = GETC(fd);
		waypt->lid.cell.ox		= GETS(fd) & 0xfff0;
		waypt->lid.cell.oy		= GETS(fd) & 0xfff0;
		waypt->lid.id			= GETS(fd);
		waypt->lid.objClass		= GETC(fd);
		waypt->lid.objType		= GETC(fd);
		waypt->distance			= GETL(fd);
		waypt->stratFlags		= GETC(fd);
	}

	return 0;
}

/************************************************************************************************
	sdm_seek_replacement_data
*************************************************************************************************/

int sdm_seek_replacement_data(int fd, sdm_read readfn, sdm_seek seekfn)
{
	sdm_header header;
	int datalen;

	header.company		= GETS(fd);
	header.filetype		= GETS(fd);
	header.version		= GETS(fd);
	header.flags		= GETS(fd);
	header.dataversion 	= GETL(fd);
	header.length 		= GETL(fd);

	datalen = GETS(fd);
	seekfn(fd, datalen);
	datalen = GETS(fd);
	seekfn(fd, datalen);

	return 0;
}

/************************************************************************************************
	sdm_seek_strat_data
*************************************************************************************************/

int sdm_seek_strat_data(int fd, sdm_read readfn, sdm_seek seekfn)
{
	int numtex, numtrninfo, numobjinfo;

	numtex  = GETS(fd);
	seekfn(fd,numtex * 32);
	numtrninfo = GETS(fd);
	seekfn(fd, numtrninfo * (2+2+1+1+1+1));
	numobjinfo =  GETS(fd);
	seekfn(fd, numobjinfo * sizeof(SDM_ObjInfo));

	return 0;
}

/************************************************************************************************
	un_rle
*************************************************************************************************/

static void un_rle(SINT16 inlen, SINT16* indata, SINT16* outlen, SINT16* outdata)
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
}

/************************************************************************************************
	readc
*************************************************************************************************/

static SINT8 readc(int fd, READFN const readfn)
{
	SINT8 c;
	readfn(fd, &c, 1);
	return c;
}

/************************************************************************************************
	reads
*************************************************************************************************/

static SINT16 reads(int fd, READFN const readfn)
{
	SINT16 s;
	readfn(fd, &s, 2);
	return s;
}

/************************************************************************************************
	readl
*************************************************************************************************/

static SINT32 readl(int fd, READFN const readfn)
{
	SINT32 l;
	readfn(fd, &l, 4);
	return l;
}

/************************************************************************************************
	readf
*************************************************************************************************/

static float readf(int fd, READFN const readfn)
{
	float f;
	readfn(fd, &f, 4);
	return f;
}

static void normalize_path(char* s)
{
	for(; *s; s++)
	{
		*s = tolower(*s);
#ifdef SDM_NORMALIZE_PATH
		if(*s == '\\') *s = '/';
#endif
	}
}

static void filter_name(char* s)
{
	for(; *s; s++)
	{
		if(*s < 0)
		{
			switch(*s)
			{
				case -55:
					*s = 'E';
				break;
				case -10:
					*s = 'o';
				break;
				case -4:
					*s = 'u';
				break;
				default:
					*s = ' ';
				break;
			}
		}
	}
}
