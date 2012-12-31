#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <map>
#include <sstream>

#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osg/Notify>

#include "BoundingBoxVisitor.h"
#include "endianswap.h"
#include "sdm.h"
#include "DTLSupercellHeightField.h"
#include "DTLOctetHeightField.h"

typedef std::map<std::pair<int,int>, DTLSupercellHeightField*> SupercellDtlMap;
static SupercellDtlMap _supercellDtlMap;

typedef std::map<std::string, osg::BoundingBox> BBoxMap;
static BBoxMap _bboxMap;

static int totalflagged, totalunflagged;

static std::string datapath;
static std::string trnpath;

static osg::BoundingBox get_model_bbox(std::string const& objname)
{
	osg::BoundingBox& bbox = _bboxMap[objname];

	if(!bbox.valid())
	{
		std::string path = trnpath + objname;
		path += ".flt";
		osg::Node* node = osgDB::readNodeFile(path);
		if(node)
		{
			BoundingBoxVisitor bbv;
			bbox = bbv.getBoundingBox(*node);
		}
	}
	return bbox;
}

static int readfn(int fd,void* buf,int size) { return read(fd, buf, size); }
static int seekfn(int fd,int offset) { return lseek(fd, offset, SEEK_CUR); }

static int8_t readc(int fd)
{
	int8_t c;
	readfn(fd, &c, 1);
	return c;
}
static void normalize_path(char* s)
{
	for(; *s; s++)
	{
		*s = tolower(*s);
		if(*s == '\\') *s = '/';
	}
}
static int16_t reads(int fd)
{
	int16_t s;
	readfn(fd, &s, 2);
	LITTLE_TO_NATIVE16(s);
	return s;
}

static float readf(int fd)
{
	float f;
	readfn(fd, &f, 4);
	LITTLE_TO_NATIVE32(f);
	return f;
}

static std::string sdm_file_name(int sx, int sy)
{
	char filename[32];
	snprintf(filename, 31, "%04x_%04x.sdm", (sx < 0) ? sx & 0x0000ffff : sx, (sy < 0) ? sy & 0x0000ffff : sy);

	std::string path = trnpath;
	path += filename;

	return path;
}

static bool sdm_file_name_supercell(std::string const& path, int& sx, int& sy)
{
	if(sscanf(osgDB::getStrippedName(path).c_str(), "%x_%x", &sx, &sy) != 2) return false;

	sx = (short)((unsigned short)sx);
	sy = (short)((unsigned short)sy);
	
	return true;
}

static DTLSupercellHeightField* get_supercell_heightfield(int sx, int sy)
{
	DTLSupercellHeightField* hf;
	SupercellDtlMap::iterator it = _supercellDtlMap.find(std::make_pair(sx,sy));
	if(it == _supercellDtlMap.end())
	{
		_supercellDtlMap[std::make_pair(sx,sy)] = hf = new DTLSupercellHeightField(sx,sy);
		
		int fd = open(sdm_file_name(sx,sy).c_str(), O_RDWR);
		if(fd > -1)
		{
			sdm_read_dtl_data(fd, readfn, hf);	
			close(fd);
		}else
		{
			memset(&hf->z,0x0,sizeof(hf->z));
		}
	}else
	{
		hf = it->second;
	}
	return hf;
}


static void rotate_and_translate_bbox(sdm_trnobj_info const& objinfo, osg::BoundingBox const& bbox, osg::Vec3d& p0, osg::Vec3d& p1, osg::Vec3d& p2, osg::Vec3d& p3, osg::Vec3d& p4, osg::Vec3d& p5)
{
	osg::Matrix mat = osg::Matrix::rotate(-osg::PI_4*(objinfo.ortdrg & 0x7),0,0,1) * osg::Matrix::translate(objinfo.pos[0],objinfo.pos[1],objinfo.pos[2]);

	p0 = bbox.center()  * mat;
	p1 = bbox.corner(0) * mat;
	p2 = bbox.corner(1) * mat;
	p3 = bbox.corner(2) * mat;
	p4 = bbox.corner(3) * mat;
	p5 = osg::Vec3d(0,0,0) * mat;
}

void get_normal_and_height_at_point(osg::Vec3d const& pos, osg::Vec3f& n, double& h)
{
	int sx = pos.x() / SDM_SUPERCELL_SIZE;
	int sy = pos.y() / SDM_SUPERCELL_SIZE;
	int ox = (pos.x() - sx * SDM_SUPERCELL_SIZE) / SDM_OCTET_SIZE;
	int oy = (pos.y() - sy * SDM_SUPERCELL_SIZE) / SDM_OCTET_SIZE;

	DTLSupercellHeightField* hf = get_supercell_heightfield(sx, sy);
	
	float h33[3][3];
	osg::Vec3f vn[3][3]; // don't care
	hf->GetOctetHeights(ox,oy,h33);

	DTLOctetHeightField of(sx, sy, ox, oy, h33, vn);
	n = of.SurfaceNormalAt(pos.x(),pos.y());
	h = of.HeightAt(pos.x(),pos.y());
}

static bool is_nonplanar(sdm_trnobj_info const& objinfo, osg::BoundingBox const& bbox)
{
	osg::Vec3d p0,p1,p2,p3,p4,p5;
	rotate_and_translate_bbox(objinfo, bbox, p0,p1,p2,p3,p4,p5);
	
	osg::Vec3f n0,n1,n2,n3,n4;
	double h0,h1,h2,h3,h4;
	get_normal_and_height_at_point(p0,n0,h0);
	get_normal_and_height_at_point(p1,n1,h1);
	get_normal_and_height_at_point(p2,n2,h2);
	get_normal_and_height_at_point(p3,n3,h3);
	get_normal_and_height_at_point(p4,n4,h4);
	
	p0.z() = h0;
	p1.z() = h0;
	p2.z() = h0;
	p3.z() = h0;
	p4.z() = h0;
	
	osg::Matrix shear;
	shear(0,2) = n0.x() / -n0.z();
	shear(1,2) = n0.y() / -n0.z();
	
	shear = osg::Matrix::translate(-p0.x(),-p0.y(),0) * shear;
	
	osg::Vec3d ps0,ps1,ps2,ps3,ps4;
	ps0 = p0 * shear;
	ps1 = p1 * shear;
	ps2 = p2 * shear;
	ps3 = p3 * shear;
	ps4 = p4 * shear;

/*	std::cout 
	<< h0 << " " << ps0.z() << " "
	<< h1 << " " << ps1.z() << " "
	<< h2 << " " << ps2.z() << " "
	<< h3 << " " << ps3.z() << " "
	<< h4 << " " << ps4.z() << " "
	<< "\n";*/
	
	return (
			fabs(h0-ps0.z()) > 0.1 ||
			fabs(h1-ps1.z()) > 0.1 ||
			fabs(h2-ps2.z()) > 0.1 ||
			fabs(h3-ps3.z()) > 0.1 ||
			fabs(h4-ps4.z()) > 0.1 
		   );
}

bool is_projectable(const char* path)
{
	return strstr(path, "ag/") || strstr(path, "bld/") ||  strstr(path, "brdg/") || strstr(path, "cp/") || strstr(path, "fac/");
}

static void process_sdm(std::string const& path)
{	
	int sx,sy;
	sdm_file_name_supercell(path,sx,sy);

	std::cout <<  "Processing: " << path << " " << sx << "," << sy << std::endl;
	
	int fd = open(path.c_str(), O_RDWR);
	if(fd < 0) return;
	
	sdm_seek_replacement_data(fd, readfn, seekfn);

	int numtex = reads(fd);
	if(numtex < 1) goto done;
	lseek(fd, numtex * 32, SEEK_CUR);
	
	int numtrninfo = reads(fd);
	if(numtrninfo < 1) goto done;
	
	lseek(fd, numtrninfo * (2+2+1+1+1+1), SEEK_CUR);

	int numobjinfo = reads(fd);
	if(numobjinfo < 1)  goto done;

	for(int i = 0; i < numobjinfo; i++)
    {	
		sdm_trnobj_info objinfo;
		objinfo.pos[0] = readf(fd);
		objinfo.pos[1] = readf(fd);
		objinfo.pos[2] = readf(fd);
		readfn(fd, objinfo.objname, 32);
		normalize_path(objinfo.objname);
		objinfo.txndx  = readc(fd);
		objinfo.ortdrg = readc(fd);
		objinfo.ruftrn = readc(fd);
		objinfo.flags = readc(fd);

		if(!is_projectable(objinfo.objname)) continue;
		osg::BoundingBox bbox = get_model_bbox(objinfo.objname);
		
		if(bbox.valid())
		{
			if(is_nonplanar(objinfo, bbox))
			{
				objinfo.flags |= SDM_TRNOBJ_NON_PLANAR_BASE;
				std::cout << "Flagging Object " << objinfo.objname << " in " << path << " " << sx << "," << sy << std::endl;
				totalflagged++;
			}else
			{
				objinfo.flags &= ~SDM_TRNOBJ_NON_PLANAR_BASE;
				totalunflagged++;
			}
			
			lseek(fd, -1, SEEK_CUR);
			write(fd, &objinfo.flags, 1);
		}
  }
done:
	close(fd);
}

int main(int argc, char** argv)
{
	osg::setNotifyLevel(osg::FATAL);
	if(argc < 2) return -1;

	datapath = argv[1];
	trnpath = datapath + std::string("/terrain/");
	
	DIR* trndir = opendir(trnpath.c_str());
	if(!trndir) return -1;
	
	for(;;)
	{
		dirent* ent = readdir(trndir);
		if(!ent) break;
	
		std::string filename = trnpath;
		filename += ent->d_name;

		if(osgDB::getLowerCaseFileExtension(filename) != "sdm") continue;

		process_sdm(filename);
	}
	
	closedir(trndir);

	std::cout << "Total Objects Flagged: " << totalflagged << " " << 100*totalflagged/(float)(totalflagged+totalunflagged) << "% of Total Projectable Objects" << std::endl;

	return 0;
} 
