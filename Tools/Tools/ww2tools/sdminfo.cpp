#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

#include "sdm.h"

static int readfn(int fd,void* buf,int size) { return read(fd, buf, size); }
static int seekfn(int fd,int offset) { return lseek(fd, offset, SEEK_CUR); }

/************************************************************************************************
	smdhf
*************************************************************************************************/

int main(int argc, char** argv)
{
	char filename[64];
	int fd;
	int sx, sy;
	sdm_strat_data* stratdata = new sdm_strat_data;

	sx = atoi(argv[1]);
	sy = atoi(argv[2]);
	
	snprintf(filename, 63, "data/terrain/%04x_%04x.sdm", (sx < 0) ? sx & 0x0000ffff : sx, (sy < 0) ? sy & 0x0000ffff : sy);
	fd = open(filename, O_RDONLY);
	printf("%s\n", filename);

	if(fd > -1)
	{
		sdm_seek_replacement_data(fd, readfn, seekfn);
		sdm_seek_strat_data(fd, readfn, seekfn);
		sdm_read_strat_data(fd, readfn, stratdata);
		
		for(int i = 0; i < stratdata->numcps; i++)
		{
			sdm_cp& cp = stratdata->cp[i];
			printf("CP \"%s\" %s %c %d %d %d %d %d %d %d %d %c %c\n"
			,	cp.name, cp.fileName
			,	'?'
			,	cp.posx, cp.posy
			,	(int)floor(cp.posx/SDM_SUPERCELL_SIZE), (int)floor(cp.posy/SDM_SUPERCELL_SIZE), cp.oid.id
			,	cp.side, cp.country
			,	cp.oid.objType,'?','?'
//			,	cp.serial, cp.subType, cp.stratFlags, cp.oid.objClass, cp.oid.objType
			);
				
		}

		for(int i = 0; i < stratdata->numfacs; i++)
		{
			sdm_facility& fac = stratdata->facility[i];
			printf("FACILITY \"%s\" %s %c %d %d %d %d %d %d %d %d %d %d %d %d %c %c\n"
			,	fac.name, fac.fileName
			,	'?'
			,	fac.posx, fac.posy
			,	(int)floor(fac.posx/SDM_SUPERCELL_SIZE), (int)floor(fac.posy/SDM_SUPERCELL_SIZE), fac.oid.id
			,	fac.cpid.sx(), fac.cpid.sy(), fac.cpid.id
			,	fac.originalSide, fac.originalCountry, fac.oid.objType, fac.subType, '?', '?'
//			,	fac.serial, fac.subType, fac.stratFlags, fac.oid.objClass, fac.oid.objType
			);

		}

		for(int i = 0; i < stratdata->numblds; i++)
		{
			sdm_building& bld = stratdata->building[i];
			printf("BUILDING \"%s\" %s %c %d %d %d %d %d %c %c %d %c %d %d %c %c --- %d %d %d %d %d\n"
			,	bld.name, bld.fileName
			,	'?'
			,	bld.posx, bld.posy
			,	(int)floor(bld.posx/SDM_SUPERCELL_SIZE), (int)floor(bld.posy/SDM_SUPERCELL_SIZE), bld.oid.id
			,	'x', 'x', bld.pid.id
			,	'?', bld.oid.objType, bld.subType, '?', '?'
			,	bld.oid.objClass, bld.oid.objType, bld.subType, bld.control, bld.stratFlags
			);
		}
		
		for(int i = 0; i < stratdata->numdepots; i++)
		{
			sdm_depot& depot = stratdata->depot[i];
			printf("LINK \"%s\" %d %d %d %d %d %d %d %d %d\n"
			,	depot.name
			,	depot.endDepotId.sx(), depot.endDepotId.sy(), depot.endDepotId.id
			,	depot.firebaseId.sx(), depot.firebaseId.sy(), depot.firebaseId.id
			,	depot.startDepotId.sx(), depot.startDepotId.sy(), depot.startDepotId.id
			);
		}

		for(int i = 0; i < stratdata->numwaypts; i++)
		{
			sdm_waypoint& waypoint = stratdata->waypoint[i];
			printf("waypoint?\n");
		}		
		
		close(fd);
	}

	return 0;
} 