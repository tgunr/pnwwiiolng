#include <syscfg.h> // Path is project specific.
#include <string>
#include <string.h>

#include "gl/glplat.h"

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"

#include "uimap/IconMapRenderer.h"
#include "ogl/GLCounter.h"
#include "sdm/sdm.h"
#include "sdm/sdmReplacementOctedData.h"

#define ALT_TRN_WATER				0
#define ALT_TRN_DUNES				20
#define ALT_TRN_FARMS				200
#define ALT_TRN_CLEAR				500
#define ALT_TRN_FOREST				1000
#define ALT_TRN_ROCKY				3000
#define ALT_TRN_ICY					10000
#define NUM_ALT_TEXTURES 7

static float mapverts[SDM_SUPERCELL_OCTETS_XY1][2];
static char* alttexiconname[NUM_ALT_TEXTURES] =
{
	"water1",
    "dunes1",
    "farms1",
    "clear1",
    "forest1",
    "rocky1",
    "icy1"
};

static bool vertsinited;
static void initverts();

class IconMapRenderer::SuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	SuperCell(Renderer*, int scx, int scy);
	virtual ~SuperCell();

enum {MAX_SUPERCELL_TEXTURES = 1024};

	bool nosdm;
	bool octetLoaded[SDM_SUPERCELL_OCTETS_X][SDM_SUPERCELL_OCTETS_Y];

	int numtextures;
	RenderBucket* bucket[MAX_SUPERCELL_TEXTURES];
	unsigned int lastdrawtime;

    virtual void Draw(phui::GraphicsContext*);
	// WARNING: It is not safe to call this from multiple threads.
	virtual void Load();

protected:
	int FindBucketAndTexCoords(const char* texname, float uv[4]);

	void LoadNonTexturedOctet(sdm_dtl_data const*);
	void LoadNonReplacementTiles(sdm_replacement_octet_data const*);
	void LoadReplacementTiles(sdm_replacement_octet_data const*);

	GLuint _displayList;
};

/************************************************************************************************
	IconMapRenderer::RenderBucket
*************************************************************************************************/

class IconMapRenderer::RenderBucket
{
public:
	RenderBucket(IconMapRenderer* r) { texid = -1; numverts = 0; renderer = r;}

	int texid;
	int numverts;

	IconMapRenderer* renderer;

	float verts[SDM_SUPERCELL_OCTETS_XY1 * 4][2];
	float texcoords[SDM_SUPERCELL_OCTETS_XY1 * 4][2];

	void AddOctet(float uv[4], int ox, int oy, int rotate);
	void Draw(phui::GraphicsContext*);
};


/************************************************************************************************
	IconMapRenderer
*************************************************************************************************/

/************************************************************************************************
	NewSuperCell
*************************************************************************************************/

SuperCellMapView::Renderer::SuperCell* IconMapRenderer::NewSuperCell(Renderer* r, int scx, int scy)
{
	return new IconMapRenderer::SuperCell(r, scx, scy);
}

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void IconMapRenderer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	SuperCellMapView::Renderer::RenderSuperCells(mv, scx1, scy1, scx2, scy2);

	unsigned int t = (unsigned int)frametime; //mv->ParentLoop()->FrameTime();

	SuperCellMap::iterator it = _scmap.begin();
	for(; it != _scmap.end(); )
	{
		SuperCellMap::iterator eraseit = it++;
		IconMapRenderer::SuperCell* sc = (IconMapRenderer::SuperCell*)eraseit->second.ptr();

		// quick hack to unload cells, nuke after 30 sec
		if(sc->loaded && t - sc->lastdrawtime > 30*1000)
			_scmap.erase(eraseit);
	}
}

/************************************************************************************************
	IconMapRenderer::SuperCell
*************************************************************************************************/

IconMapRenderer::SuperCell::SuperCell(Renderer* r, int x, int y)
							: SuperCellMapView::Renderer::SuperCell(r, x, y)
{
	_displayList = 0;
	loaded = nosdm = loading = false;
	numtextures = 0;
	memset(octetLoaded,0x0,sizeof(octetLoaded));
	lastdrawtime = 0;
	memset(bucket,0x0,sizeof(bucket));
};

IconMapRenderer::SuperCell::~SuperCell()
{
	if(_displayList > 0)
		glDeleteLists(_displayList,1);

	for(int i = 0; i < numtextures; i++)
	{
		delete bucket[i];
	}
};

/************************************************************************************************
	Load
*************************************************************************************************/

void IconMapRenderer::SuperCell::Load()
{
	if(loaded) return;

	sdm_dtl_data dtl;
	// HACK: Having such a large structure on the stack was causing serious problems.
	static sdm_replacement_octet_data replaceoctets;
	IconMapRenderer* ir = (IconMapRenderer*)renderer;

	ir->GetSDMData(scx, scy, &dtl, &replaceoctets);

	if(replaceoctets.numtex < 1)
	{
		LoadNonTexturedOctet(&dtl);
	}else
	{
		LoadNonReplacementTiles(&replaceoctets);
		LoadReplacementTiles(&replaceoctets);
	}

	loaded = true;
	loading = false;
}

/************************************************************************************************
	IconMapRenderer::SuperCell::FindBucketAndTexCoords
*************************************************************************************************/

int IconMapRenderer::SuperCell::FindBucketAndTexCoords(const char* objname, float uv[4])
{
	int k = -1;

	if(!vertsinited) initverts();
	if(!objname[0]) return -1;

	int texid = ((IconMapRenderer*)renderer)->GetOctetTexture((IconMapRenderer*)renderer, objname, uv);
	if(texid > -1)
	{
		// see if it already exists
		for(int i = 0; i < numtextures; i++)
		{
			if(bucket[i]->texid == texid)
			{
				k = i;
				break;
			}
		}

		assert(numtextures < MAX_SUPERCELL_TEXTURES);
		if(k < 0) // not found, so add it
		{
			bucket[numtextures] = new IconMapRenderer::RenderBucket((IconMapRenderer*)renderer);
			bucket[numtextures]->texid = texid;
			k = numtextures++;
		}
	}

	return k;
}

/************************************************************************************************
	IconMapRenderer::SuperCell::Draw
*************************************************************************************************/

void IconMapRenderer::SuperCell::Draw(phui::GraphicsContext* ctx)
{
	if(!loaded)
		return;

//	if(_displayList == 0)
	{
		//_displayList = glGenLists(1);
		//glNewList(_displayList, GL_COMPILE);
		ctx->EnableTexture();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glTranslatef((float)scx, (float)scy,0);

		for(int i = 0; i < numtextures;i++)
		{
			if(bucket[i]->numverts > 0)
				bucket[i]->Draw(ctx);
		}

		glTranslatef((float)-scx, (float)-scy,0);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		//glEndList();
	}
//	glCallList(_displayList);

	lastdrawtime = frametime; //renderer->_mv->ParentLoop()->FrameTime();
}

/************************************************************************************************
	IconMapRenderer::SuperCell::LoadNonTexturedOctet
*************************************************************************************************/

void IconMapRenderer::SuperCell::LoadNonTexturedOctet(sdm_dtl_data const* dtl)
{
//	IconMapRenderer* ir = (IconMapRenderer*)renderer;

	int i = 0;
	for(int y = 0; y < SDM_SUPERCELL_OCTETS_Y; y++)
	{
		for(int x = 0; x < SDM_SUPERCELL_OCTETS_X; x++, i++)
		{
			int j,k, orient;

			float z = dtl->z[(2*(SDM_SUPERCELL_OCTETS_Y-y-1) + 1) * SDM_SUPERCELL_SIZE_X + (2*x + 1)]; // center point, change to average?

			if     (z > ALT_TRN_ROCKY)  j = 6;
			else if(z > ALT_TRN_FOREST) j = 5;
			else if(z > ALT_TRN_CLEAR)  j = 4;
			else if(z > ALT_TRN_FARMS)  j = 3;
			else if(z > ALT_TRN_DUNES)  j = 2;
			else if(z > ALT_TRN_WATER)  j = 1;
			else                        j = 0;

			orient = i & 3; // random orientation

			float uv[4];
			k = FindBucketAndTexCoords(alttexiconname[j], uv);
			if(k > -1)
				bucket[k]->AddOctet(uv, x,y, orient);

			octetLoaded[x][y] = true;
		}
	}
}

/************************************************************************************************
	IconMapRenderer::SuperCell::LoadNonReplacementTiles
*************************************************************************************************/

void IconMapRenderer::SuperCell::LoadNonReplacementTiles(sdm_replacement_octet_data const* replaceoctets)
{
	// find null bmp id which id's replacement tiles
	int nulltex = -1;
	int i;
	for(i = 0; i < replaceoctets->numtex; i++)
	{
		if(strcmp(replaceoctets->texname[i], "null1.bmp") == 0)
		{
			nulltex = i;
			break;
		}
	}

	// non-replacement tiles
	i = 0;
	for(int y = 0; y < SDM_SUPERCELL_OCTETS_Y; y++)
	{
		for(int x = 0; x < SDM_SUPERCELL_OCTETS_X; x++, i++)
		{
			sdm_terrain_info* info = (sdm_terrain_info*)&replaceoctets->trninfo[i];

			if(info->txndx != nulltex)
			{
				float uv[4];
				int k  = FindBucketAndTexCoords(replaceoctets->texname[info->txndx], uv);
				if(k > -1)
				{
					int orient = info->ortdrg & 0x3; // orientation in 90 degree increments (different than sdm data below which is in 45 degree increments)
					bucket[k]->AddOctet(uv, x,y, orient);
					octetLoaded[x][y] = true;
				}
			}
		}
	}
}

/************************************************************************************************
	IconMapRenderer::SuperCell::LoadReplacementTiles
*************************************************************************************************/

void IconMapRenderer::SuperCell::LoadReplacementTiles(sdm_replacement_octet_data const* replaceoctets)
{
// replacement tiles (sdm includes all terrain objects but we only use the ones that have icon textures and therefore are actually replacement tiles)
	for(int i = 0; i < replaceoctets->numobjinfo; i++)
	{
		SDM_ObjInfo const* info = &replaceoctets->objinfo[i];

		float uv[4];
		int k = FindBucketAndTexCoords(info->objname, uv);
		if(k > -1)
		{
			int orient = (info->ortdrg & 0x7) >> 1; // low 4 bits is the orientation in 45 degree increments
													// we only do it in 90 on the map, so we bit shift down ,i.e. divide by 2
			int supercellX = (int)(scx * SDM_OCTET_SIZE * SDM_SUPERCELL_OCTETS_X);
			int supercellY = (int)(scy * SDM_OCTET_SIZE * SDM_SUPERCELL_OCTETS_Y);
			int x = abs((int)((info->pos.x - supercellX)/SDM_OCTET_SIZE));
			int y = abs((int)((info->pos.y - supercellY)/SDM_OCTET_SIZE));

			if(x < SDM_SUPERCELL_OCTETS_X && y < SDM_SUPERCELL_OCTETS_Y && !octetLoaded[x][y])
			{
				bucket[k]->AddOctet(uv, x,y, orient);
				octetLoaded[x][y] = true;
			}
		}
	}
}


/************************************************************************************************
	IconMapRenderer::RenderBucket::Draw
*************************************************************************************************/

void IconMapRenderer::RenderBucket::Draw(phui::GraphicsContext* ctx)
{
	if(texid > -1)
	{
		ctx->SetTexture(texid);
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
		GLCounter::instance().call_glDrawArrays(GL_QUADS, 0, numverts);
	}
}

/************************************************************************************************
	IconMapRenderer::RenderBucket::AddOctet
*************************************************************************************************/

void IconMapRenderer::RenderBucket::AddOctet(float uv[4], int ox, int oy, int rotate)
{
	float u0 = uv[0];
	float u1 = uv[2];
	float v0 = uv[3];
	float v1 = uv[1];

	float rotu[4][4] = { {u0, u0, u1, u1}, {u0, u1, u1, u0}, {u1, u1, u0, u0}, {u1, u0, u0, u1} };
	float rotv[4][4] = { {v0, v1, v1, v0}, {v1, v1, v0, v0}, {v1, v0, v0, v1}, {v0, v0, v1, v1} };

	int topleft		= oy * SDM_SUPERCELL_OCTETS_X1 + ox;
	int topright	= topleft + 1;
	int bottomright = topright + SDM_SUPERCELL_OCTETS_X1;
	int bottomleft  = bottomright - 1;

	if(numverts + 4 > SDM_SUPERCELL_OCTETS_XY1 * 4) return;

//	bool flipY	= false;

	float* v = (float*)&verts[numverts];
	*v++ = mapverts[bottomleft][0];
	*v++ = mapverts[bottomleft][1];
	*v++ = mapverts[topleft][0];
	*v++ = mapverts[topleft][1];
	*v++ = mapverts[topright][0];
	*v++ = mapverts[topright][1];
	*v++ = mapverts[bottomright][0];
	*v++ = mapverts[bottomright][1];

	float* tc = (float*)&texcoords[numverts];
	*tc++ = rotu[rotate][0]; *tc++ = rotv[rotate][0];
	*tc++ = rotu[rotate][1]; *tc++ = rotv[rotate][1];
	*tc++ = rotu[rotate][2]; *tc++ = rotv[rotate][2];
	*tc++ = rotu[rotate][3]; *tc++ = rotv[rotate][3];

	numverts += 4;
}

/************************************************************************************************
	initverts
*************************************************************************************************/

static void initverts()
{
    float* v = (float*)mapverts;
	float dx = 1 / (float)SDM_SUPERCELL_OCTETS_X;
	float dy = 1 / (float)SDM_SUPERCELL_OCTETS_Y;

	float y = 0;
    for(int i = 0; i < SDM_SUPERCELL_OCTETS_Y1; i++)
    {
		float x = 0;
		for(int j = 0; j < SDM_SUPERCELL_OCTETS_X1; j++)
        {
            *v++ = x;
            *v++ = y;
			x += dx;
        }
		y += dy;
    }

    vertsinited = true;
}
