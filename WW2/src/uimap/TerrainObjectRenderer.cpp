#include <syscfg.h> // Path is project specific.
#include <math.h>
#include <string.h>

#include "gl/glplat.h"

#include "phui/Util.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"

#include "uimap/TerrainObjectRenderer.h"

#include "math/Matrixf.h"
#include "ogl/GLCounter.h"

#include "sdm/sdmObjInfo.h"
#include "sdm/sdmReplacementOctedData.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define zRENDER_QUADS

class RenderBucket : public phui::Referenced
{
public:
	RenderBucket(int texid) : _texid(texid) {}

	void AddQuad(float box[8], float uv[8])
	{
#ifdef RENDER_QUADS
		for(int i=0; i <8; i++)
		{
			verts.push_back(box[i]);
			texcoords.push_back(uv[i]);
		}
#else
        static int offsets[12] = { 0,1,2,3,4,5, 0,1,4,5,6,7 };
		for(int i=0; i <12; i++)
		{
			verts.push_back(box[offsets[i]]);
			texcoords.push_back(uv[offsets[i]]);
		}

#endif
    }

	void Draw(phui::GraphicsContext* ctx)
	{
		ctx->SetTexture(_texid);
		glVertexPointer(2, GL_FLOAT, 0, &verts[0]);
		glTexCoordPointer(2, GL_FLOAT, 0, &texcoords[0]);
#ifdef RENDER_QUADS
		GLCounter::instance().call_glDrawArrays(GL_QUADS, 0, verts.size()/2);
#else
		GLCounter::instance().call_glDrawArrays(GL_TRIANGLES, 0, verts.size()/2);
#endif
	}

protected:
	int _texid;
	std::vector<float> verts;
	std::vector<float> texcoords;
};

class TerrainObjectRenderer::SuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	SuperCell(TerrainObjectRenderer* r, int x, int y) : SuperCellMapView::Renderer::SuperCell(r, x, y)
		, celldata{}, _renderer{r}, _displayList{0}, _buckets{}
	{
	}

 	~SuperCell()
	{
		if(_displayList > 0)
			glDeleteLists(_displayList,1);
	}

	virtual void Draw(phui::GraphicsContext* ctx)
	{
		if(!loaded)
			return;

		//if(_displayList == 0)
		{
//			_displayList = glGenLists(1);
//			glNewList(_displayList, GL_COMPILE);

			ctx->EnableTexture();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			BucketMap::iterator it = _buckets.begin();
			for(; it != _buckets.end(); ++it)
			{
				it->second->Draw(ctx);
			}
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

//			glEndList();
		}
//		glCallList(_displayList);
	}

	RenderBucket* GetBucket(int texid)
	{
		phui::ref_ptr<RenderBucket>& bucket = _buckets[texid];
		if(!bucket.valid())
			bucket = new RenderBucket(texid);

		return bucket.ptr();
	}

	virtual void Load()
	{
		if(loaded) return;

		SDM_ObjInfo*	_info;
		int				texid, _x, _y, _width, _height;
		float			uv[8];
		RenderBucket*	bucket;
		float			_box[8];
		Vec3f			corners[4];
		Matrixf			transform;

		_renderer->GetTerrainSDMData(scx, scy, &celldata);

		int i = 0;
		for(; i < celldata.numobjinfo; i++)
		{
			_info = &celldata.objinfo[i];

			_renderer->GetTerrainTextureData(_info, &texid, &_x, &_y, &_width, &_height, uv);
			if(texid < 0) continue;

			bucket = GetBucket(texid);

			transform.set(_info->transform);
			corners[0].set(float(_x), float(_y), 0.0f);
			corners[1].set(float(_x + _width), float(_y), 0.0f);
			corners[2].set(float(_x + _width), float(_y + _height), 0.0f);
			corners[3].set(float(_x), float(_y + _height), 0.0f);

			for(int cIdx = 0; cIdx < 4; cIdx++)
			{
				corners[cIdx] = corners[cIdx] * transform;
			}
			_box[0] = corners[0].x() / SDM_SUPERCELL_SIZE;
			_box[1] = corners[0].y() / SDM_SUPERCELL_SIZE;
			_box[2] = corners[1].x() / SDM_SUPERCELL_SIZE;
			_box[3] = corners[1].y() / SDM_SUPERCELL_SIZE;
			_box[4] = corners[2].x() / SDM_SUPERCELL_SIZE;
			_box[5] = corners[2].y() / SDM_SUPERCELL_SIZE;
			_box[6] = corners[3].x() / SDM_SUPERCELL_SIZE;
			_box[7] = corners[3].y() / SDM_SUPERCELL_SIZE;

			bucket->AddQuad(_box, uv);
		}

		loaded = true;
		loading = false;
	}

	sdm_replacement_octet_data celldata;
	TerrainObjectRenderer* _renderer;
	GLuint _displayList;
	typedef std::map<int, phui::ref_ptr<RenderBucket> > BucketMap;
	BucketMap _buckets;
};

TerrainObjectRenderer::TerrainObjectRenderer() : GetTerrainSDMData(0), GetTerrainTextureData(0)
{
}

SuperCellMapView::Renderer::SuperCell* TerrainObjectRenderer::NewSuperCell(Renderer* r, int scx, int scy)
{
	return new TerrainObjectRenderer::SuperCell((TerrainObjectRenderer*)r, scx, scy);
}

