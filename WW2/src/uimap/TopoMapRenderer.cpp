#include <syscfg.h> // Path is project specific.
#include <vector>
#include <math.h>

#include "gl/glplat.h"
#include "ogl/ogl.h"

#include "phui/Util.h"
#include "phui/GraphicsContext.h"

#include "uimap/TopoMapRenderer.h"
#include "ogl/States.h"

#include "sdm/sdmReplacementOctedData.h"

struct AltLine
{
	float z;
	float x1,y1;
	float x2,y2;
};

class TopoMapRenderer::ContourSuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	ContourSuperCell(SuperCellMapView::Renderer* r, int x, int y) : SuperCellMapView::Renderer::SuperCell(r, x, y) {}
 	~ContourSuperCell() {}

	void Load();
	void Draw(phui::GraphicsContext*);

	void AddTriangleContours(float tri[3][3]);

	std::vector<AltLine> _lines;
};

class TopoMapRenderer::ShadedSuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	ShadedSuperCell(SuperCellMapView::Renderer* r, int x, int y) : SuperCellMapView::Renderer::SuperCell(r, x, y), _texid(0) {}
 	~ShadedSuperCell() { }

	void Load();
	void Draw(phui::GraphicsContext*);

	GLuint _texid;
};

static int compare_tri_z(const void* t1, const void* t2);

/************************************************************************************************
	TopoMapRenderer
*************************************************************************************************/

TopoMapRenderer::TopoMapRenderer()
{
	_altstart = 0;
	_altend = 10000;
	_altspacing = 100;
	_sdmdata = 0;
}

TopoMapRenderer::~TopoMapRenderer()
{
}

SuperCellMapView::Renderer::SuperCell* TopoMapRenderer::NewSuperCell(Renderer* r, int scx, int scy)
{
	return new TopoMapRenderer::ContourSuperCell(r,scx,scy);
}

/************************************************************************************************
	Load
*************************************************************************************************/

void TopoMapRenderer::ContourSuperCell::Load()
{
	if(loaded) return;

	sdm_dtl_data dtl;
	sdm_replacement_octet_data replaceoctets;
	((TopoMapRenderer*)renderer)->_sdmdata(scx, scy, &dtl, &replaceoctets);

	for(int row = 0; row < SDM_SUPERCELL_SIZE_Y1; row++)
	{
		for(int col = 0; col < SDM_SUPERCELL_SIZE_X1; col++)
		{
			// written for clarity not speed
			float z_bl = dtl.z[(SDM_SUPERCELL_SIZE_Y1-row) * SDM_SUPERCELL_SIZE_X + col];
			float z_br = dtl.z[(SDM_SUPERCELL_SIZE_Y1-row) * SDM_SUPERCELL_SIZE_X + col+1];
			float z_tl = dtl.z[(SDM_SUPERCELL_SIZE_Y1-row-1) * SDM_SUPERCELL_SIZE_X + col];
			float z_tr = dtl.z[(SDM_SUPERCELL_SIZE_Y1-row-1) * SDM_SUPERCELL_SIZE_X + col+1];

			float b	= row/(float)SDM_SUPERCELL_SIZE_Y1;
			float t = (row+1)/(float)SDM_SUPERCELL_SIZE_Y1;
			float l = col/(float)SDM_SUPERCELL_SIZE_X1;
			float r = (col+1)/(float)SDM_SUPERCELL_SIZE_X1;

			float tri[3][3];

			// test first triangle
			if((row+col) & 0x1)
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

			AddTriangleContours(tri);

			// test second
			if((row+col) & 0x1)
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

			AddTriangleContours(tri);
		}
	}

	loaded = true;
	loading = false;
}

void TopoMapRenderer::ContourSuperCell::Draw(phui::GraphicsContext* ctx)
{
	if(!loaded)
		return;

	ctx->PushState();

	glTranslatef((float)scx, (float)scy, 0);

#if USE_STATESET
    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetRenderState(new Color4F(0, 0, 0, 1));
        stateset->SetMode(GL_LIGHTING, 0);
    }

    stateset->Apply();

    static std::vector<float> lines;

	std::vector<AltLine>::iterator it = _lines.begin();

    for(; it != _lines.end(); ++it)
	{
        lines.push_back(it->x1);
        lines.push_back(it->y1);
        lines.push_back(it->x2);
        lines.push_back(it->y2);
	}

    ogl::SetVertexPointer(2, GL_FLOAT, 0, &lines[0]);
    ogl::DrawArrays(GL_LINE_STRIP, 0, lines.size() / 2);

    lines.clear();
#else
    TextureState::Disable();
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 0, 1));

	glBegin(GL_LINES);
// should optimize this into a big line strip
	std::vector<AltLine>::iterator it = _lines.begin();
	for(; it != _lines.end(); ++it)
	{
		glVertex2f(it->x1, it->y1);
		glVertex2f(it->x2, it->y2);
	}
	glEnd();
#endif

	ctx->PopState();
}

/************************************************************************************************
	AddTriangleContours
*************************************************************************************************/

void TopoMapRenderer::ContourSuperCell::AddTriangleContours(float tri[3][3])
{
	AltLine line;
	TopoMapRenderer* topo = (TopoMapRenderer*)renderer;
	float zmin, zmax, t;
	float x1, x2, y1, y2, z1, z2;

	qsort((void*)tri, 3, 3*sizeof(float), compare_tri_z);
	zmin = tri[0][2];
	zmax = tri[2][2];

	float zstart = floor((zmin - topo->_altstart) / topo->_altspacing) * topo->_altspacing + topo->_altstart;
	float zend = zmax;

	if(zstart < topo->_altstart)	zstart = topo->_altstart;
	if(zend > topo->_altend)		zend = topo->_altend;

	for(float z = zstart; z <= zend; z += topo->_altspacing)
	{
		if(tri[0][2] < z && tri[1][2] > z)
		{
			line.z = z;

			x1 = tri[0][0]; y1 = tri[0][1]; z1 = tri[0][2];
			x2 = tri[1][0]; y2 = tri[1][1]; z2 = tri[1][2];

			t = (z - z2) / (z1 - z2);
			line.x1 = x1 * t + (1 - t) * x2;
			line.y1 = y1 * t + (1 - t) * y2;

			x2 = tri[2][0]; y2 = tri[2][1]; z2 = tri[2][2];

			t = (z - z2) / (z1 - z2);
			line.x2 = x1 * t + (1 - t) * x2;
			line.y2 = y1 * t + (1 - t) * y2;

			_lines.push_back(line);

		}else if(tri[1][2] < z && tri[2][2] > z)
		{
			line.z = z;

			x1 = tri[2][0]; y1 = tri[2][1]; z1 = tri[2][2];
			x2 = tri[1][0]; y2 = tri[1][1]; z2 = tri[1][2];

			t = (z - z2) / (z1 - z2);
			line.x1 = x1 * t + (1 - t) * x2;
			line.y1 = y1 * t + (1 - t) * y2;

			x2 = tri[0][0]; y2 = tri[0][1]; z2 = tri[0][2];

			t = (z - z2) / (z1 - z2);
			line.x2 = x1 * t + (1 - t) * x2;
			line.y2 = y1 * t + (1 - t) * y2;

			_lines.push_back(line);
		}
	}
}

/************************************************************************************************
	compare_tri_z
*************************************************************************************************/

static int compare_tri_z(const void* t1, const void* t2)
{
	float* tri1 = (float*)t1;
	float* tri2 = (float*)t2;

	if(tri1[2] < tri2[2]) return -1;
	if(tri1[2] > tri2[2]) return 1;

	return 0;
}

/************************************************************************************************
	ShadedSuperCell
*************************************************************************************************/

void TopoMapRenderer::ShadedSuperCell::Load()
{
}

void TopoMapRenderer::ShadedSuperCell::Draw(phui::GraphicsContext*)
{
}
