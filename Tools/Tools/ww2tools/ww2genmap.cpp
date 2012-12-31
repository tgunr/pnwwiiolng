#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <map>
#include <vector>

#include <gd.h>

#include "sdm.h"

/************************************************************************************************
	ww2genmap
*************************************************************************************************/

#define NUM_ALT_TEXTURES 7
#define ALT_TRN_WATER				0
#define ALT_TRN_DUNES				20
#define ALT_TRN_FARMS				200
#define ALT_TRN_CLEAR				500
#define ALT_TRN_FOREST				1000
#define ALT_TRN_ROCKY				3000
#define ALT_TRN_ICY					10000

#define MAX_ARGS 128

enum{ SIZE_OPT, AREA_OPT, SHOW_OPT, SERVER_OPT, PORT_OPT, VERBOSE_OPT, DIR_OPT, QUALITY_OPT, HELP_OPT };

struct icon_image
{
	gdImagePtr image[8];
	int width[8];
	int height[8];
	int x[8];
	int y[8];
	float mpp;
	bool loaded;
	
	icon_image() { memset(this, 0x0, sizeof(icon_image)); }
};

struct map_options
{
	int imgwidth, imgheight;
	float scxmin, scymin, scxmax, scymax;
	bool showbuildings, showrenderedterrain, showtopo, showcp;
	int quality;
	char filename[128];
	
	float ppmX, ppmY; 	// pixels per meter
	float ppoX, ppoY; 	// pixels per octet
	float ppscX, ppscY;	// pixels per supercell
	
	map_options() { memset(this, 0x0, sizeof(map_options)); quality = 75; }

};

struct imageServer
{
	std::map<int, std::string> requests;
	int listener;
	
	int fdmax;
	fd_set fdset;
	
	imageServer() { FD_ZERO(&fdset); fdmax = 0; listener = 0; }
	
	void newconnection(int fd);
	void readconnection(int fd);
	void closeconnection(int fd);
	void sendimage(int fd, unsigned char*, int len);
	void processrequest(int fd);
	
	void run();
};

static struct option longopts[] = 
{
	{ "size", 			1, NULL, SIZE_OPT },
	{ "area",			1, NULL, AREA_OPT },
	{ "show",			1, NULL, SHOW_OPT },
	{ "server",			0, NULL, SERVER_OPT },
	{ "port",			1, NULL, PORT_OPT },
	{ "verbose",		0, NULL, VERBOSE_OPT },
	{ "dir",			1, NULL, DIR_OPT },
	{ "quality",		1, NULL, QUALITY_OPT },
	{ "help",			0, NULL, HELP_OPT },
	{ NULL,				0, NULL, 0 }
};

static char *alttexiconname[NUM_ALT_TEXTURES]=
{
	"water1_icon",
    "dunes1_icon",
    "farms1_icon",
    "clear1_icon",
    "forest1_icon",
    "rocky1_icon",
    "icy1_icon"
};

static int readfn(int fd,void* buf,int size) { return read(fd, buf, size); }

static int parse_opts(map_options*, int argc, char** argv);
static int parse_url_opts(map_options*, char*);

static gdImagePtr create_image(map_options*);

static void render_supercell(gdImagePtr, map_options*, int scx, int scy, sdm_dtl_data*, sdm_replacement_octet_data*);
static void render_nontextured_supercell(gdImagePtr, map_options*, int scx, int scy, sdm_dtl_data*);
static void render_textured_supercell(gdImagePtr, map_options*, int scx, int scy, sdm_dtl_data*, sdm_replacement_octet_data*);
static void render_octet(gdImagePtr img, map_options* options, float scx, float scy, float ox, float oy, int orient, icon_image* icon);
static void render_terrain_icon(gdImagePtr img, map_options*, float scx, float scy, float ox, float oy, int orient, icon_image* icon);
static void render_topo_octet(gdImagePtr, map_options*, int scx, int scy, int ox, int oy, sdm_dtl_data*);
static void render_topo_contour(gdImagePtr, map_options*, float tri[3][3]);
static void render_cp_octet(gdImagePtr, map_options*, int scx, int scy, int ox, int oy, sdm_dtl_data*);

static icon_image* get_alt_texture(float z);
static icon_image* get_terrain_icon(const char* name);
static icon_image* get_terrain_texture_icon(const char* name);
static icon_image* get_terrain_object_icon(const char* name, bool building=false);
static icon_image* get_icon_image(const char* name);

static void calc_icon_image_dimension(gdImagePtr image, icon_image* icon, int i);
static void calc_icon_image_offset(icon_image* icon, int i, int x, int y);

static int sendall(int s, const char *buf, int len);

static bool run_as_server;
static short server_port = 10000;
static std::map<std::string, icon_image> iconmap;
static bool verbose;
std::string sdmdir("data/terrain");
static const char httpimageHeader[] = "HTTP/1.0 200 \nContent-Type: image/jpeg\n\n";
const int httpimageHeaderSize = (sizeof(httpimageHeader)-1);

/************************************************************************************************
	ww2genmap
*************************************************************************************************/

int main(int argc, char** argv)
{
	map_options options;
	
	if(parse_opts(&options, argc, argv) != 0) return 0;
	
	if(run_as_server)
	{
		imageServer server;
		server.run();
	}else
	{
		gdImagePtr img = create_image(&options);
	
		FILE* f = fopen(options.filename, "wb");
		
/*		char* s = strchr(options.filename, '.');
		if(s && !strcmp(s, ".jpg"))
			gdImageJpeg(img, f, options.quality);		
		else
*/			gdImagePng(img, f);

		fclose(f);
	}

	return 0;
}

/************************************************************************************************
	parse_opts
*************************************************************************************************/

static int parse_opts(map_options* options, int argc, char** argv)
{
	char imgsizestr[32];
	char areastr[32];
	std::vector<std::string> showstrings;
	std::map<std::string, std::string> dirs;
	
	char* s;
	char* unit;
	
	int imgwidth, imgheight;
	float ox, oy;
	float width, height;
	
	*imgsizestr = *areastr = 0;
	imgwidth = imgheight = 1024;
	ox = 18; oy = 235;
	width = height = 1;
	
// getopt
	int ch;
	
	optreset = 1;
	optind = 0;
	
	while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1)
	{
		switch(ch) 
		{						
			case HELP_OPT:
				printf("Usage: ww2genmap [--size WIDTHxHEIGHT] [--area SCXxSCY+XWIDTH+YWIDTH] [--show (buildings|renderedterrain|topo|cps)]\n");
				exit(0);
			break;
			
			case SIZE_OPT:
				strncpy(imgsizestr, optarg, 31);
			break;

			case AREA_OPT:
				strncpy(areastr, optarg, 31);
			break;	
			
			case SHOW_OPT:
				showstrings.push_back(optarg);
			break;
			
			case SERVER_OPT:
				run_as_server = true;
			break;
			
			case PORT_OPT:
				server_port = (short)atoi(optarg);
			break;
			
			case VERBOSE_OPT:
				verbose = true;
			break;
			
			case DIR_OPT:
			{
				std::string arg = optarg;
				size_t n;
				n = arg.find('=');
				if(n != std::string::npos)
				{
					std::string key,val;
					key = arg.substr(0,n);
					val = arg.substr(n+1);
					dirs[key] = val;
				}
			}
			break;
			
			case QUALITY_OPT:
				options->quality = atoi(optarg);
			break;
		}
	}	

	strncpy(options->filename, argv[optind] ? argv[optind] : "image.png", 127);		

// area
	if(*areastr)
	{
	 	unit = strchr(areastr,':');
	 	
	 	if(unit)
	 	{
	 		*unit = 0;
	 		s = unit+1;
	 	}else
	 	{
	 		s = areastr;
	 	}
	 	
		s = strtok(s, "x");
		if(s) 
			ox = atof(s);

		s = strtok(NULL, "+");
		if(s) 
			oy = atof(s);

		s = strtok(NULL, "+");
		if(s) 
			width = atof(s);
	
		s = strtok(NULL, "+");
		if(s)
			height = atof(s);
		else
			height = width;
			
		if(unit && !strcmp(unit, "latlon"))
		{
			ox /= SDM_LON_SIZE;	
			oy /= SDM_LAT_SIZE;
			width  /= SDM_LON_SIZE;	
			height /= SDM_LAT_SIZE;
		}
	}
	
	float aspect = height / width;
	
// image size
	if(*imgsizestr)
	{
		s = strtok(imgsizestr, "x");
		if(s) 
			imgwidth = atoi(s);

		s = strtok(NULL, "x");
		if(s) 
			imgheight = atoi(s);
		else
			imgheight = (int)(imgwidth * aspect);
	}else
	{
		if(aspect != imgheight / (float)imgwidth)
			imgheight = (int)(imgwidth * aspect);
	}	

// show options
	for(std::vector<std::string>::iterator it = showstrings.begin(); it != showstrings.end(); ++it)
	{
		if(*it == "buildings")
			options->showbuildings = true;
		else if(*it == "renderedterrain")
			options->showrenderedterrain = true;
		else if(*it == "topo")
			options->showtopo = true;
		else if(*it == "cps")
			options->showcp = true;
	}

// dirs
	std::map<std::string, std::string>::iterator it;
	
	it = dirs.find("sdm");
	if(it != dirs.end())
		sdmdir = it->second;
	
	options->imgwidth  = imgwidth;
	options->imgheight = imgheight;
	options->scxmin = ox;
	options->scymin = oy;
	options->scxmax = ox + width;
	options->scymax = oy + height;

	options->ppmX 	= options->imgwidth  / (width * SDM_SUPERCELL_SIZE);
	options->ppmY 	= options->imgheight / (height * SDM_SUPERCELL_SIZE);
	options->ppoX 	= options->imgwidth  / (width * SDM_SUPERCELL_OCTETS_X);
	options->ppoY 	= options->imgheight / (height * SDM_SUPERCELL_OCTETS_Y);
	options->ppscX 	= options->imgwidth  / width;
	options->ppscY 	= options->imgheight / height;

	return 0;
}


/************************************************************************************************
	parse_url_opts
*************************************************************************************************/

static int parse_url_opts(map_options* options, char* url)
{
	int argc = 1;
	char* argv[MAX_ARGS] = {"ww2genmap"};
	
	char* s = strrchr(url, '?');
	if(!s) return -1;
	
	s++;
	s = strtok(s, "&");
	if(s)
		argv[argc++] = s;
	
	for(; s && argc < MAX_ARGS;)
	{
		s = strtok(NULL, "&");
		if(s)
			argv[argc++] = s;
	}
	
	parse_opts(options, argc, argv);
	
	return 0;
}

/************************************************************************************************
	create_image
*************************************************************************************************/

static gdImagePtr create_image(map_options* options)
{		
	gdImagePtr img = gdImageCreateTrueColor(options->imgwidth, options->imgheight);

	sdm_dtl_data dtl;
	sdm_replacement_octet_data replace;
	
	int scxmin = (int)floor(options->scxmin);
	int scymin = (int)floor(options->scymin);
	int scxmax = (int)ceil(options->scxmax);
	int scymax = (int)ceil(options->scymax);
	
	for(int scx = scxmin; scx < scxmax; scx++)
	{
		for(int scy = scymin; scy < scymax; scy++)
		{
			char filename[64];
			snprintf(filename, 63, "%s/%04x_%04x.sdm", sdmdir.c_str(), (scx < 0) ? scx & 0x0000ffff : scx, (scy < 0) ? scy & 0x0000ffff : scy);
			int fd =  open(filename, O_RDONLY);
			if(fd > -1)
			{
				sdm_read_dtl_data(fd, readfn, &dtl);
				sdm_read_replacement_data(fd, readfn, &replace);
				close(fd);
			}else
			{
				memset(&dtl, 0x0, sizeof(dtl));
				memset(&replace, 0x0, sizeof(replace));
			}
			
			render_supercell(img, options, scx, scy, &dtl, &replace);
		}
	}
	
	return img;
}

/************************************************************************************************
	render_supercell
*************************************************************************************************/

static void render_supercell(gdImagePtr img, map_options* options, int scx, int scy, 
								sdm_dtl_data* dtl, sdm_replacement_octet_data* replaceoctets)
{
	if(replaceoctets->numtex < 1)
	{   
		render_nontextured_supercell(img, options, scx, scy, dtl);
	}else
	{ 
		render_textured_supercell(img, options, scx, scy, dtl, replaceoctets);
	}
}

/************************************************************************************************
	render_nontextured_supercell
*************************************************************************************************/

static void render_nontextured_supercell(gdImagePtr img, map_options* options, int scx, int scy, sdm_dtl_data* dtl)
{	
	int oxmin = (scx < options->scxmin) ? (int)floor((options->scxmin - scx) * SDM_SUPERCELL_OCTETS_X) : 0;
	int oymin = (scy < options->scymin) ? (int)floor((options->scymin - scy) * SDM_SUPERCELL_OCTETS_Y) : 0;
	int oxmax = (scx+1 > options->scxmax) ? (int)ceil((scx+1 - options->scxmax) * SDM_SUPERCELL_OCTETS_X) : SDM_SUPERCELL_OCTETS_X;
	int oymax = (scy+1 > options->scymax) ? (int)ceil((scy+1 - options->scymax) * SDM_SUPERCELL_OCTETS_Y) : SDM_SUPERCELL_OCTETS_Y;
	
	for(int oy = oymin; oy < oymax; oy++)
	{
		for(int ox = oxmin; ox < oxmax; ox++)
		{
			float z = dtl->z[(2*(SDM_SUPERCELL_OCTETS_Y-oy-1) + 1) * SDM_SUPERCELL_SIZE_X + (2*ox + 1)];
			int orient = ((scx*11+scy*29+ox*47+oy*71) % 8); // random orientation
			orient -= orient % 2;
	
			icon_image* icon = get_alt_texture(z);
			if(!icon->image[orient]) continue;
			
			render_octet(img, options, scx, scy, ox, oy, orient, icon);	
			
			if(options->showtopo)
				render_topo_octet(img, options, scx, scy, ox, oy, dtl);
				
			if(options->showcp)
				render_cp_octet(img, options, scx, scy, ox, oy, dtl);
		}
	}
}

/************************************************************************************************
	render_textured_supercell
*************************************************************************************************/

static void render_textured_supercell(gdImagePtr img, map_options* options, int scx, int scy, 
										sdm_dtl_data* dtl, sdm_replacement_octet_data* replaceoctets)
{
	icon_image* icons[SDM_MAX_TERRAIN_TEXTURES];
	memset(icons, 0x0, sizeof(icons));
	
	// find null bmp id which id's replacement tiles
	int nulltex = -1;
	for(int i = 0; i < replaceoctets->numtex; i++)
	{
		if(strcmp(replaceoctets->texname[i], "null1.bmp") == 0)
		{
			nulltex = i;
			break;
		}
	}

	int oxmin = (scx < options->scxmin) ? (int)floor((options->scxmin - scx) * SDM_SUPERCELL_OCTETS_X) : 0;
	int oymin = (scy < options->scymin) ? (int)floor((options->scymin - scy) * SDM_SUPERCELL_OCTETS_Y) : 0;
	int oxmax = (scx+1 > options->scxmax) ? (int)ceil((options->scxmax - scx) * SDM_SUPERCELL_OCTETS_X) : SDM_SUPERCELL_OCTETS_X;
	int oymax = (scy+1 > options->scymax) ? (int)ceil((options->scymax - scy) * SDM_SUPERCELL_OCTETS_Y) : SDM_SUPERCELL_OCTETS_Y;

	// non-replacement tiles
	int i = 0;
	for(int oy = 0; oy < SDM_SUPERCELL_OCTETS_X; oy++)
	{
		for(int ox = 0; ox < SDM_SUPERCELL_OCTETS_Y; ox++, i++)
		{   
			if(ox < oxmin || oy < oymin || ox >= oxmax || oy >= oymax) continue;
			
			if(options->showtopo)
				render_topo_octet(img, options, scx, scy, ox, oy, dtl);

			sdm_terrain_info* info = (sdm_terrain_info*)&replaceoctets->trninfo[i];
	
			if(info->txndx == nulltex) continue;
		
			int orient = (info->ortdrg & 0x3)*2; // orientation in 90 degree increments (different than sdm data below which is in 45 degree increments)

			if(options->showrenderedterrain)
			{
				if(icons[info->txndx] == NULL)
					icons[info->txndx] = get_terrain_texture_icon(replaceoctets->texname[info->txndx]);		
			}else
			{
				if(icons[info->txndx] == NULL)
					icons[info->txndx] = get_terrain_icon(replaceoctets->texname[info->txndx]);
			}
						
			if(icons[info->txndx]->image[orient])
				render_octet(img, options, scx, scy, ox, oy, orient, icons[info->txndx]);
		}
	}

	// terrain objects
	int supercellX = (int)(scx * SDM_OCTET_SIZE * SDM_SUPERCELL_OCTETS_X);
	int supercellY = (int)(scy * SDM_OCTET_SIZE * SDM_SUPERCELL_OCTETS_Y);
	
	for(int i = 0; i < replaceoctets->numobjinfo; i++)
	{
		sdm_trnobj_info const* info = &replaceoctets->objinfo[i];
		float ox = (info->pos[0] - supercellX)/SDM_OCTET_SIZE;
		float oy = (info->pos[1] - supercellY)/SDM_OCTET_SIZE;

		if(ox < oxmin || oy < oymin || ox >= oxmax || oy >= oymax) continue;

		int orient = (info->ortdrg & 0x7);
		icon_image* icon = NULL;
		
		icon = get_terrain_icon(info->objname);

		if(icon->image[orient] && options->showrenderedterrain)
			icon = get_terrain_object_icon(info->objname);
	
		if(icon->image[orient])
		{				
			render_octet(img, options, scx, scy, (int)ox, (int)oy, orient, icon);
		}else // other terrain object
		{
			if(options->showbuildings)
			{
				icon = get_terrain_object_icon(info->objname, true);

				if(icon && icon->image[orient])
					render_terrain_icon(img, options, scx, scy, ox, oy, orient, icon);
			}
		}
	}
}

/************************************************************************************************
	render_octet
*************************************************************************************************/

static void render_octet(gdImagePtr img, map_options* options, float scx, float scy, float ox, float oy, int orient, icon_image* icon)
{
	float destx = (scx - options->scxmin) * options->ppscX + ox * options->ppoX;
	float desty = options->imgheight - options->ppoY - ( (scy - options->scymin) * options->ppscY + oy * options->ppoY);

	gdImageCopyResized(img, icon->image[orient], (int)destx, (int)desty, 0, 0, (int)options->ppoX, (int)options->ppoY, 
						icon->width[orient], icon->height[orient]);
}

/************************************************************************************************
	render_terrain_icon
*************************************************************************************************/

static void render_terrain_icon(gdImagePtr img, map_options* options, float scx, float scy, float ox, float oy, int orient, icon_image* icon)
{
	float destx = (scx - options->scxmin) * options->ppscX  + ox * options->ppoX + icon->x[orient] * options->ppmX;
	float desty = options->imgheight - icon->height[orient] * options->ppmY - ( (scy - options->scymin) *  options->ppscY + oy * options->ppoY)
					- icon->y[orient] * options->ppmY;

	gdImageCopyResized(img, icon->image[orient], (int)destx, (int)desty, 0, 0, (int)(options->ppmX*icon->width[orient]), (int)(options->ppmY*icon->height[orient]), 
						icon->width[orient], icon->height[orient]);
}

/************************************************************************************************
	render_topo_octet
*************************************************************************************************/

static void render_topo_octet(gdImagePtr img, map_options* options, int scx, int scy, int ox, int oy, sdm_dtl_data* dtl)
{  
	float z_bl = dtl->z[(SDM_SUPERCELL_SIZE_Y1-oy) * SDM_SUPERCELL_SIZE_X + ox];
	float z_br = dtl->z[(SDM_SUPERCELL_SIZE_Y1-oy) * SDM_SUPERCELL_SIZE_X + ox+1];
	float z_tl = dtl->z[(SDM_SUPERCELL_SIZE_Y1-oy-1) * SDM_SUPERCELL_SIZE_X + ox];
	float z_tr = dtl->z[(SDM_SUPERCELL_SIZE_Y1-oy-1) * SDM_SUPERCELL_SIZE_X + ox+1];
	
	float b	= oy * options->ppoY + scy * options->ppscY;
	float t = (oy+1) * options->ppoY + scy * options->ppscY;
	float l = ox * options->ppoX + scx * options->ppscX;
	float r = (ox+1) * options->ppoX + scx * options->ppscX;
	
	float tri[3][3];
	
	// test first triangle
	if((oy+ox) & 0x1)
	{
		tri[0][0] = r;
		tri[0][1] = b;
		tri[0][2] = z_br;
		
		tri[1][0] = l;
		tri[1][1] = b;
		tri[1][2] = z_bl;

		tri[2][0] = l;
		tri[2][1] = t;
		tri[2][2] = z_tl;	
	}else
	{
		tri[0][0] = l;
		tri[0][1] = b;
		tri[0][2] = z_bl;
		
		tri[1][0] = l;
		tri[1][1] = t;
		tri[1][2] = z_tl;

		tri[2][0] = r;
		tri[2][1] = t;
		tri[2][2] = z_tr;
	}
	
	render_topo_contour(img, options, tri);
	
	// test second
	if((oy+ox) & 0x1)
	{
		tri[0][0] = r;
		tri[0][1] = b;
		tri[0][2] = z_br;
		
		tri[1][0] = l;
		tri[1][1] = t;
		tri[1][2] = z_tl;

		tri[2][0] = r;
		tri[2][1] = t;
		tri[2][2] = z_tr;	
	}else
	{
		tri[0][0] = l;
		tri[0][1] = b;
		tri[0][2] = z_bl;
		
		tri[1][0] = r;
		tri[1][1] = t;
		tri[1][2] = z_tr;

		tri[2][0] = r;
		tri[2][1] = b;
		tri[2][2] = z_br;
	}
	
	render_topo_contour(img, options, tri);
}

/************************************************************************************************
	render_topo_contour
*************************************************************************************************/

static void render_topo_contour(gdImagePtr img, map_options* options, float tri[3][3])
{
}

/************************************************************************************************
	render_cp_octet
*************************************************************************************************/

static void render_cp_octet(gdImagePtr img, map_options* options, int scx, int scy, int ox, int oy, sdm_dtl_data*)
{
}

/************************************************************************************************
	get_alt_texture
*************************************************************************************************/

static icon_image* get_alt_texture(float z)
{
	static icon_image* alticons[NUM_ALT_TEXTURES];
	
	int k;
	
	if     (z > ALT_TRN_ROCKY)  k = 6;
	else if(z > ALT_TRN_FOREST) k = 5;
	else if(z > ALT_TRN_CLEAR)  k = 4;
	else if(z > ALT_TRN_FARMS)  k = 3;
	else if(z > ALT_TRN_DUNES)  k = 2;
	else if(z > ALT_TRN_WATER)  k = 1;
	else                        k = 0;
	
	if(alticons[k] == NULL)
	{
		std::string path = "data/terrain/icons/";
		path += alttexiconname[k];
	
		alticons[k] = get_icon_image(path.c_str());
	}
	
	return alticons[k];
}

/************************************************************************************************
	get_terrain_icon
*************************************************************************************************/
// old 16x16 icons

static icon_image* get_terrain_icon(const char* name)
{
	std::string path = "data/terrain/icons/";
	std::string filename = name;
	
	size_t n;
	n = filename.rfind('.');
	if(n != std::string::npos)
		filename.erase(n);

	n = filename.rfind('\\');
	if(n != std::string::npos)
		filename = filename.substr(n+1);
	
	path += filename;
	path += "_icon";

	return get_icon_image(path.c_str());
}

/************************************************************************************************
	get_terrain_texture_icon
*************************************************************************************************/
// this is the actual in-game texture used on non-replacement tiles

static icon_image* get_terrain_texture_icon(const char* name)
{
	std::string path = "data/terrain/";
	path += name; 
	
	size_t n;
	n = path.rfind('.');
	if(n != std::string::npos)
		path.erase(n);
	
	path += ".png";

	return get_icon_image(path.c_str());
}

/************************************************************************************************
	get_terrain_object_icon
*************************************************************************************************/
// rendered terrain object icons

static icon_image* get_terrain_object_icon(const char* name, bool building)
{
	std::string path = "data/terrain/";
	std::string filename = name;
	std::string dir;
	size_t n;
	
	n = filename.rfind('\\');
	if(n != std::string::npos)
	{
		dir = filename.substr(0,n); dir += "/";
		filename = filename.substr(n+1);
		
		for(int i = 0; i < (int)dir.size(); i++) dir[i] = tolower(dir[i]);
	}
	
	if(building && dir == "to/") return 0;
	
	path += dir; path += "icons/";
	path += filename; path += "_icon";

	return get_icon_image(path.c_str());
}

/************************************************************************************************
	get_icon_image
*************************************************************************************************/

static icon_image* get_icon_image(const char* name)
{
	std::string path = name;
	size_t n = path.rfind('.');

	if(n != std::string::npos)
		path.erase(n);	
	icon_image& icon = iconmap[path];
	if(!icon.loaded)
	{
		std::string filename = path;
		filename += ".png";

		FILE* f = fopen(filename.c_str(), "rb");
		if(f)
		{

			gdImagePtr pngimage = gdImageCreateFromPng(f);
			if(!pngimage)
			{
				printf("%s\n",filename.c_str());
				exit(-1);
			}
			for(int i = 0; i < 8;  i++)
			{
				calc_icon_image_dimension(pngimage, &icon, i);
				icon.image[i] = gdImageCreateTrueColor(icon.width[i], icon.height[i]);				
				gdImageAlphaBlending(icon.image[i], 0);
				
				if(i & 0x1)
				{
					int** row = icon.image[i]->tpixels;
					for(int y = 0; y < icon.height[i]; y++, row++)
					{
						unsigned char* pixel = (unsigned char*)*row;
						for(int x = 0; x < icon.width[i]; x++, pixel += 4)
							*pixel = gdAlphaTransparent;
					}
				}
			}
		
			gdImageCopy(icon.image[0], pngimage, 0,0, 0,0, icon.width[0],icon.height[0]);
			
			// make magenta transparent
			int** row = icon.image[0]->tpixels;
			for(int y = 0; y < icon.height[0]; y++, row++)
			{
				unsigned char* pixel = (unsigned char*)*row;
				for(int x = 0; x < icon.width[0]; x++, pixel += 4)
				{
					if(pixel[1] == 0xff && pixel[2] == 0x0 && pixel[3] == 0xff)
						pixel[0] = gdAlphaTransparent;
				}
			}
			
			for(int i = 1; i < 8; i++)
				gdImageCopyRotated(icon.image[i], icon.image[0], icon.width[i]/2, icon.height[i]/2, 0, 0, icon.width[0], icon.height[0], -45*i);
						
			fclose(f);
			gdImageDestroy(pngimage);
				
			filename = path;
			filename += ".xy";

			f = fopen(filename.c_str(), "rb");
			if(f)
			{
				fscanf(f, "%d %d %f", &icon.x[0], &icon.y[0], &icon.mpp);
				
				for(int i = 1; i < 8; i++)
					calc_icon_image_offset(&icon, i, icon.x[0], icon.y[0]);

				fclose(f);
			}
		}else
		{
//			printf("can't open %s\n", filename.c_str());
		}
		
		icon.loaded = true;
	}
	
	return &icon;
}

/************************************************************************************************
	calc_icon_image_dimension
*************************************************************************************************/

static void calc_icon_image_dimension(gdImagePtr image, icon_image* icon, int i)
{
	float x,y;
	float angle = 45*i*M_PI/180.0;
	float sine = sin(angle);
	float cosine = cos(angle);
	float w2 = gdImageSX(image) >> 1;
	float h2 = gdImageSY(image) >> 1;
	float minx, miny, maxx, maxy;
	
	minx = miny = 10000;
	maxx = maxy = 0;
	
	x = w2 * cosine + h2 * sine;
	y = h2 * cosine - w2 * sine;
	minx = std::min(minx, x); maxx = std::max(maxx, x);
	miny = std::min(miny, y); maxy = std::max(maxy, y);
	
	x = -w2 * cosine + h2 * sine;
	y = h2 * cosine - -w2 * sine;
	minx = std::min(minx, x); maxx = std::max(maxx, x);
	miny = std::min(miny, y); maxy = std::max(maxy, y);

	x = -w2 * cosine + -h2 * sine;
	y = -h2 * cosine - -w2 * sine;
	minx = std::min(minx, x); maxx = std::max(maxx, x);
	miny = std::min(miny, y); maxy = std::max(maxy, y);

	x = w2 * cosine + -h2 * sine;
	y = -h2 * cosine - w2 * sine;
	minx = std::min(minx, x); maxx = std::max(maxx, x);
	miny = std::min(miny, y); maxy = std::max(maxy, y);
		
	icon->width[i] = (int)ceil(maxx - minx);
	icon->height[i] = (int)ceil(maxy - miny);
}

/************************************************************************************************
	calc_icon_image_offset
*************************************************************************************************/

static void calc_icon_image_offset(icon_image* icon, int i, int offx, int offy)
{
	float x,y;
	float angle = 45*i*M_PI/180.0;
	float sine = sin(angle);
	float cosine = cos(angle);
	int width = icon->width[0];
	int height = icon->height[0];
	float minx, miny;
	
	minx = miny = 10000;
	
	x = (offx) * cosine + (offy) * sine;
	y = (offy) * cosine - (offx) * sine;
	minx = std::min(minx, x); miny = std::min(miny, y);

	x = (offx+width) * cosine + (offy) * sine;
	y = (offy) * cosine - (offx+width) * sine;
	minx = std::min(minx, x); miny = std::min(miny, y);
	
	x = (offx+width) * cosine + (offy+height) * sine;
	y = (offy+height) * cosine - (offx+width) * sine;
	minx = std::min(minx, x); miny = std::min(miny, y);

	x = (offx) * cosine + (offy+height) * sine;
	y = (offy+height) * cosine - (offx) * sine;
	minx = std::min(minx, x); miny = std::min(miny, y);

	icon->x[i] = (int)floor(minx);
	icon->y[i] = (int)floor(miny);
}
	
/************************************************************************************************
	run
*************************************************************************************************/

void imageServer::run()
{
 	struct sockaddr_in localaddr;

	// create socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) return;
    
    int yes=1;
 	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) return;
 	
    // bind
	memset(&localaddr, 0x0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = INADDR_ANY;
    localaddr.sin_port = htons(server_port);
   
    if (bind(listener, (struct sockaddr*)&localaddr, sizeof(localaddr)) == -1) return;

    // listen
    if (listen(listener, 10) == -1) return;
    
    // add the serverfd to the fds set
    FD_SET(listener, &fdset);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) 
    {
      	fd_set readfds = fdset; // copy it
      	if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) break;

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) 
        {
            if (FD_ISSET(i, &readfds)) 
            { // we got one!!
       			if (i == listener)
					newconnection(i);
                else 
              		readconnection(i);
            }
        }
    }
} 

/************************************************************************************************
	newconnection
*************************************************************************************************/

void imageServer::newconnection(int fd)
{
	int connection;  
 	socklen_t addrlen;
	struct sockaddr_in remoteaddr;

	addrlen = sizeof(remoteaddr);
	if ((connection = accept(listener, (struct sockaddr*)&remoteaddr,  &addrlen)) == -1) return;

    FD_SET(connection, &fdset);
    if (connection > fdmax)
        fdmax = connection;
}

/************************************************************************************************
	readconnection
*************************************************************************************************/

void imageServer::readconnection(int fd)
{
	char buf[128];
	int nbytes;

	if ((nbytes = recv(fd, buf, sizeof(buf)-1, 0)) <= 0) 
	{
	   closeconnection(fd);
	} else 
	{
		buf[nbytes] = 0;

		std::string& request = requests[fd];
		request += buf;
		
		bool requestend = false;
		int n = request.size();
		
		if(n > 0 && request[n-1] == '\n')
		{
			// check for end of request
			for(int i = n-1; i >= 0; i--)
			{
				if(request[i] == '\r') continue;
				
				if(request[i] == '\n')
					requestend = true;
	
				break;
			}
		}
		
		if(requestend)
		{
			processrequest(fd);
		 	closeconnection(fd);
		}
	}
 }
 
/************************************************************************************************
	closeconnection
*************************************************************************************************/

void imageServer::closeconnection(int fd)
{
 	close(fd);
    FD_CLR(fd, &fdset);
    
    std::string& request = requests[fd];
  	request = "";
}

/************************************************************************************************
	sendimage
*************************************************************************************************/

void imageServer::sendimage(int fd, unsigned char* buf, int len)
{
	sendall(fd, httpimageHeader, httpimageHeaderSize);
	sendall(fd, (char*)buf, len);
}

/************************************************************************************************
	processrequest
*************************************************************************************************/

void imageServer::processrequest(int fd)
{
	/*map_options options;
	std::string& request = requests[fd];

	char type[5];
	char url[1024];
	if(sscanf(request.c_str(), "%4s %1023s", type, url) != 2) return;
	if(strcmp(type, "GET") != 0) return;

	parse_url_opts(&options, url);
		
	gdImagePtr img = create_image(&options);	

	int len;
	void* buf = gdImageJpegPtr(img, &len, options.quality);

	if(buf)
	{
		sendimage(fd, (unsigned char*)buf, len);
		gdFree(buf);
	}*/
}

/************************************************************************************************
	sendall
*************************************************************************************************/

static int sendall(int s, const char *buf, int len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 