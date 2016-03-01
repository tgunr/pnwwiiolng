#include <syscfg.h> // Path is project specific.

#include "ogl/oglMesh.h"

#include "fio/fio.h"
#include <algorithm>
#include "mem/mem.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/States.h"
#include "ogl/GLCounter.h"
#include "math/SuperFastHash.h"

static Covert<bool> _draw = true;

using namespace ogl;
using namespace ogl::VertexAttributes;

typedef std::map<unsigned int, unsigned int> VAOS;
static VAOS vaos;

void oglMeshToggleDraw(bool draw)
{
    _draw = draw;
}

UINT32	ogl::VertexElementSize(UINT32 vertexType)
{
	UINT32 vertexSize = 0;

	if(vertexType & POSITION)	vertexSize += POSITION_SIZE;
	if(vertexType & NORMAL)		vertexSize += NORMAL_SIZE;
	if(vertexType & COLOR)		vertexSize += COLOR_SIZE;
	if(vertexType & TEX0)		vertexSize += TEX0_SIZE;
	if(vertexType & TEX1)		vertexSize += TEX1_SIZE;
	if(vertexType & ATTRIB6)	vertexSize += ATTRIB6_SIZE;
	if(vertexType & ATTRIB7)	vertexSize += ATTRIB7_SIZE;
	if(vertexType & ATTRIB15)	vertexSize += ATTRIB15_SIZE;
	if(vertexType & INDEX)		vertexSize += INDEX_SIZE;

	return vertexSize;
}


//////////////////////////////////////////////////////////////////////
//	StaticBuffer
//////////////////////////////////////////////////////////////////////
StaticBuffer::StaticBuffer()
	:	vertexType(0),
        vertexSize(0),
		ibData(nullptr),
		vbName(0),
		ibName(0)
{
	;
}


//////////////////////////////////////////////////////////////////////
//	StaticBuffer
//
//	Creates the vertex- and index buffer against OGL, reading from a file
//////////////////////////////////////////////////////////////////////
StaticBuffer::StaticBuffer(int hfile)
	:	vertexType(0),
        vertexSize(0),
		ibData(nullptr),
		vbName(0),
		ibName(0)
{
	SINT32		vertexCnt	= fioGetl(hfile);

	vertexSize	= fioGetl(hfile);

	void* vbData = memAlloc(vertexCnt*vertexSize);
	fioRead(hfile, vbData, vertexCnt*vertexSize);

	// Static vertex buffer
	glGenBuffersARB(1, &vbName);
	BindArrayBuffer(vbName);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertexCnt*vertexSize, vbData, GL_STATIC_DRAW_ARB);
	BindArrayBuffer(0);

	assert(vbName);

	memFree(vbData);
	vbData = nullptr;


	SINT32		indexCnt	= fioGetl(hfile);

	ibData = memAlloc(indexCnt*sizeof(VertexIdx));
	fioReads(hfile, (SINT16*)ibData, indexCnt);

	// Static index buffer
	glGenBuffersARB(1, &ibName);
	BindElementArrayBuffer(ibName);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexCnt*sizeof(VertexIdx), ibData, GL_STATIC_DRAW_ARB);
	BindElementArrayBuffer(0);

	assert(ibName);

	memFree(ibData);
	ibData = nullptr;
}

//////////////////////////////////////////////////////////////////////
//	~StaticBuffer()
//////////////////////////////////////////////////////////////////////
StaticBuffer::~StaticBuffer()
{
	rawVertexData.clear();
	rawIndexData.clear();

//	if(vbData)	memFree(vbData);
	if(ibData)	memFree(ibData);

	if(vbName)	glDeleteBuffersARB(1, &vbName);
	if(ibName)	glDeleteBuffersARB(1, &ibName);
}

//////////////////////////////////////////////////////////////////////
//	StaticBuffer::Buffer
//////////////////////////////////////////////////////////////////////
void StaticBuffer::Buffer()
{
    unsigned char*	crtFinalVertexPtr	= nullptr;
    unsigned char*	crtRawVertexPtr		= nullptr;

	if(IsBuffered()) return;

	void* vbData = memAlloc(rawVertexData.size() * vertexSize);
	crtFinalVertexPtr = (unsigned char*)vbData;

	// Reduce the raw vertex data
	for(UINT32	i=0; i < rawVertexData.size(); i++)
	{
		crtRawVertexPtr		= (unsigned char*)&(rawVertexData[i]);


		if(vertexType & POSITION)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, POSITION_SIZE);
			crtFinalVertexPtr += POSITION_SIZE;
		}
		crtRawVertexPtr += POSITION_SIZE;

		if(vertexType & NORMAL)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, NORMAL_SIZE);
			crtFinalVertexPtr += NORMAL_SIZE;
		}
		crtRawVertexPtr += NORMAL_SIZE;

		if(vertexType & TEX0)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, TEX0_SIZE);
			crtFinalVertexPtr += TEX0_SIZE;
		}
		crtRawVertexPtr += TEX0_SIZE;

		if(vertexType & ATTRIB6)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, ATTRIB6_SIZE);
			crtFinalVertexPtr += ATTRIB6_SIZE;
		}
		crtRawVertexPtr += ATTRIB6_SIZE;

		if(vertexType & ATTRIB7)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, ATTRIB7_SIZE);
			crtFinalVertexPtr += ATTRIB7_SIZE;
		}
		crtRawVertexPtr += ATTRIB7_SIZE;

		if(vertexType & ATTRIB15)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, ATTRIB15_SIZE);
			crtFinalVertexPtr += ATTRIB15_SIZE;
		}
		crtRawVertexPtr += ATTRIB15_SIZE;

/*		if(vertexType & COLOR)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, COLOR_SIZE);
			crtFinalVertexPtr += COLOR_SIZE;
		}
		crtRawVertexPtr += COLOR_SIZE;

		if(vertexType & TEX1)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, TEX1);
			crtFinalVertexPtr += TEX1;
		}
		crtRawVertexPtr += TEX1;



		if(vertexType & INDEX)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, INDEX);
			crtFinalVertexPtr += INDEX;
		}
		crtRawVertexPtr += INDEX;*/
	}

	// Static vertex buffer
	glGenBuffersARB(1, &vbName);
	BindArrayBuffer(vbName);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, rawVertexData.size()*vertexSize, vbData, GL_STATIC_DRAW_ARB);
	BindArrayBuffer(0);

	// Static index buffer
	glGenBuffersARB(1, &ibName);
	BindElementArrayBuffer(ibName);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, rawIndexData.size()*sizeof(VertexIdx), &rawIndexData[0], GL_STATIC_DRAW_ARB);
	BindElementArrayBuffer(0);

	rawVertexData.clear();
	rawIndexData.clear();

	if(vbData)
	{
		memFree(vbData);
		vbData = nullptr;
	}
	if(ibData)
	{
		memFree(ibData);
		ibData = nullptr;
	}
}


//////////////////////////////////////////////////////////////////////
//	StaticBuffer::AddMesh
//////////////////////////////////////////////////////////////////////
LOGICAL StaticBuffer::AddMesh(std::vector<RawVertex>& withRawVertexData, const VertexIdx* indices, VertexIdx elementCnt, MeshContext* meshContext, int bufferIdx)
{
			VertexIdx             existingElementIndex;
			VertexIdx							minIdx				= 0;
			VertexIdx							maxIdx				= 0;
			UINT32								idxOffset			= 0;
	const	VertexIdx*							crtIndex			= indices;


	if(this->rawVertexData.empty() && !IsBuffered())
	{
		this->vertexType = meshContext->GetVertexType();
		vertexSize = meshContext->GetVertexSize();
	}
	minIdx = ~minIdx;
	idxOffset = rawIndexData.size()*sizeof(VertexIdx);

	if(	((this->rawIndexData.size() + elementCnt) < 65535)	&&					// This might skip a few vertices that could fit, but that's OK
		this->vertexType == meshContext->GetVertexType()	)
	{
		for(UINT32 i=0; i < elementCnt; i++, crtIndex++)
		{
            RawVertex* vertx = &withRawVertexData[*crtIndex];
            size_t code = SuperFastHash((const char*)vertx,sizeof(RawVertex));

            std::pair<RawVertexIndexer::iterator,RawVertexIndexer::iterator> rvi = vertexIndexer.equal_range(code);
            bool newVertex = true;

            if ( rvi.first != rvi.second ) // possible match (might be more than 1)
            {
                for (RawVertexIndexer::iterator it=rvi.first; it!=rvi.second; ++it)
                {
                    // RawVertex* test = &rawVertexData[ (*it).second ];
                    if (code == (*it).first)
                    {
                        newVertex = false;
                        existingElementIndex = (*it).second;
                        break;
                    }
                }
            }

            if ( newVertex ) // new
			{
				rawIndexData.push_back( (ogl::VertexIdx)rawVertexData.size() );
				rawVertexData.push_back( *vertx );

                vertexIndexer.insert( std::pair<size_t,VertexIdx>( code, (VertexIdx)rawVertexData.size()-1) );
			}
			else
            {
                rawIndexData.push_back( existingElementIndex );
            }

			if(rawIndexData.back() < minIdx)
                minIdx = rawIndexData.back();
			if(rawIndexData.back() > maxIdx)
                maxIdx = rawIndexData.back();
		}

		meshContext->SetBufferPointer(this);
		meshContext->SetBufferHandle(bufferIdx);
		meshContext->SetMinIdx(minIdx);
		meshContext->SetMaxIdx(maxIdx);
		meshContext->SetIdxOffset(idxOffset);
		meshContext->SetElementCnt(elementCnt);

		return TRUE;
	}
	// This buffer uses a different vertex type or is full
	return FALSE;
}


//////////////////////////////////////////////////////////////////////
//	StaticBuffer::SaveToFile
//////////////////////////////////////////////////////////////////////
void StaticBuffer::SaveToFile(int hfile)
{
	unsigned char*	crtFinalVertexPtr	= nullptr;
	unsigned char*	crtRawVertexPtr		= nullptr;


	fioPutl(hfile, rawVertexData.size());									// Count of unique vertices
	fioPutl(hfile, vertexSize);

	void* vbData = memAlloc(rawVertexData.size() * vertexSize);
	crtFinalVertexPtr = (unsigned char*)vbData;

	// Reduce the raw vertex data
	for(UINT32	i=0; i < rawVertexData.size(); i++)
	{
		crtRawVertexPtr		= (unsigned char*)&(rawVertexData[i]);


		if(vertexType & POSITION)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, POSITION_SIZE);
			crtFinalVertexPtr += POSITION_SIZE;
		}
		crtRawVertexPtr += POSITION_SIZE;

		if(vertexType & NORMAL)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, NORMAL_SIZE);
			crtFinalVertexPtr += NORMAL_SIZE;
		}
		crtRawVertexPtr += NORMAL_SIZE;

		if(vertexType & TEX0)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, TEX0_SIZE);
			crtFinalVertexPtr += TEX0_SIZE;
		}
		crtRawVertexPtr += TEX0_SIZE;

		if(vertexType & ATTRIB6)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, ATTRIB6_SIZE);
			crtFinalVertexPtr += ATTRIB6_SIZE;
		}
		crtRawVertexPtr += ATTRIB6_SIZE;

		if(vertexType & ATTRIB7)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, ATTRIB7_SIZE);
			crtFinalVertexPtr += ATTRIB7_SIZE;
		}
		crtRawVertexPtr += ATTRIB7_SIZE;

		if(vertexType & ATTRIB15)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, ATTRIB15_SIZE);
			crtFinalVertexPtr += ATTRIB15_SIZE;
		}
		crtRawVertexPtr += ATTRIB15_SIZE;

/*		if(vertexType & COLOR)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, COLOR_SIZE);
			crtFinalVertexPtr += COLOR_SIZE;
		}
		crtRawVertexPtr += COLOR_SIZE;

		if(vertexType & TEX1)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, TEX1);
			crtFinalVertexPtr += TEX1;
		}
		crtRawVertexPtr += TEX1;

		if(vertexType & TANGENT)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, TANGENT);
			crtFinalVertexPtr += TANGENT;
		}
		crtRawVertexPtr += TANGENT;

		if(vertexType & INDEX)
		{
			memcpy(crtFinalVertexPtr, crtRawVertexPtr, INDEX);
			crtFinalVertexPtr += INDEX;
		}
		crtRawVertexPtr += INDEX;*/
	}

	fioWrite(hfile, vbData, rawVertexData.size()*vertexSize);

    // Index buffer data
	fioPutl(hfile, rawIndexData.size());									// Count of indices
	fioWrite(hfile, &rawIndexData[0], rawIndexData.size()*sizeof(VertexIdx));	// Default vertex data

	memFree(vbData);
}

//////////////////////////////////////////////////////////////////////
//	MeshContext
//////////////////////////////////////////////////////////////////////
MeshContext::MeshContext(int fd, const std::vector<StaticBuffer*>& staticBuffers)
	:	modelViewMatrix(nullptr),
        vao(0),
        staticBuffer(nullptr)
{
	vertexType = fioGetl(fd);
	vertexSize = fioGetl(fd);
	bufferHandle = fioGetl(fd);
	minIdx = fioGets(fd);
	maxIdx = fioGets(fd);
	idxOffset = fioGetl(fd);
	elementCnt = fioGets(fd);

	if(bufferHandle > -1)
    {
        staticBuffer = staticBuffers[bufferHandle];
    }
}

/*-----------------------------------------------------------------------
Function: Apply
Description: Create VAO (vertex array objects) for fast(er) drawing.
------------------------------------------------------------------------*/
void MeshContext::Apply()
{
    if(!ogl::UseDeprecatedPath() && staticBuffer && vao == 0)
    {
        ogl::Disable();

        static unsigned int key[3];

        key[0] = staticBuffer->GetVertexBufferName();
        key[1] = staticBuffer->GetIndexBufferName();
        key[2] = vertexType;

        size_t code = SuperFastHash((const char*)key, sizeof(unsigned int) * 3);

        if(vaos.find(code) != vaos.end())
        {
            vao = vaos.find(code)->second;
            return;
        }

        glGenVertexArrays(1, &vao);

        VertexArrayObjectBind::Bind(vao);
        ArrayBufferBind::Bind(staticBuffer->GetVertexBufferName());

        int offset = 0;

        if(vertexType & POSITION)
        {
            glEnableVertexAttribArray(oglVertexBuffer::ATTRIBUTE_POSITION);
            glVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(offset));
            offset += POSITION_SIZE;
        }

        if(vertexType & NORMAL)
        {
            glEnableVertexAttribArray(oglVertexBuffer::ATTRIBUTE_NORMAL);
            glVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(offset));
            offset += NORMAL_SIZE;
        }

        if(vertexType & TEX0)
        {
            glEnableVertexAttribArray(oglVertexBuffer::ATTRIBUTE_TEXCOORD0);
            glVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(offset));

            if(vertexType & TEX1)
            {
                glEnableVertexAttribArray(oglVertexBuffer::ATTRIBUTE_TEXCOORD1);
                glVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(offset));
            }

            if(vertexType & TEX2)
            {
                glEnableVertexAttribArray(oglVertexBuffer::ATTRIBUTE_TEXCOORD2);
                glVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(offset));
            }

            offset += TEX0_SIZE;
        }

        if(vertexType & ATTRIB6)
        {
            glEnableVertexAttribArray(oglVertexBuffer::ATTRIBUTE_TANGENT);
            glVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(offset));
            offset += ATTRIB6_SIZE;
        }

        ElementBufferBind::Bind(staticBuffer->GetIndexBufferName());
        VertexArrayObjectBind::Bind(0);

        vaos[code] = vao;
    }
}

//////////////////////////////////////////////////////////////////////
//	MeshContext
//////////////////////////////////////////////////////////////////////
MeshContext::MeshContext(UINT32 withVertexType)
	:	modelViewMatrix(nullptr),
		vertexSize(0),
		bufferHandle(-1),
		minIdx(0),
		maxIdx(0),
		idxOffset(0),
		elementCnt(0),
        vao(0),
        staticBuffer(nullptr)
{
	this->vertexType = withVertexType;
	vertexSize = ogl::VertexElementSize(withVertexType);
}


//////////////////////////////////////////////////////////////////////
//	GetPoolSize
//////////////////////////////////////////////////////////////////////
int MeshContext::GetPoolSize()
{
	int size = 0;

	size += sizeof(UINT32);				// vertexType
	size += sizeof(GLsizei);			// vertexSize
	size += sizeof(UINT32);				// bufferHandle
	size += sizeof(VertexIdx);			// minIdx
	size += sizeof(VertexIdx);			// maxIdx
	size += sizeof(UINT32);				// idxOffset
	size += sizeof(VertexIdx);			// elementCnt;

	return size;
}


//////////////////////////////////////////////////////////////////////
//	GetPoolSize
//////////////////////////////////////////////////////////////////////
void MeshContext::CopyToPool(UINT8* pool)
{
	memcpy(pool, &vertexType, sizeof(vertexType));
	pool += sizeof(vertexType);
	memcpy(pool, &vertexSize, sizeof(vertexSize));
	pool += sizeof(vertexSize);
	memcpy(pool, &bufferHandle, sizeof(bufferHandle));
	pool += sizeof(bufferHandle);
	memcpy(pool, &minIdx, sizeof(minIdx));
	pool += sizeof(minIdx);
	memcpy(pool, &maxIdx, sizeof(maxIdx));
	pool += sizeof(maxIdx);
	memcpy(pool, &idxOffset, sizeof(idxOffset));
	pool += sizeof(idxOffset);
	memcpy(pool, &elementCnt, sizeof(elementCnt));
	pool += sizeof(elementCnt);
}


//////////////////////////////////////////////////////////////////////
//	MeshContext::AddVertexAttribute
//////////////////////////////////////////////////////////////////////
void MeshContext::AddVertexAttribute(void* data, UINT32 attribute)
{
	vertexType |= attribute;
	vertexSize = VertexElementSize(vertexType);
}


//////////////////////////////////////////////////////////////////////
//	SetOGLModelViewMatrix
//////////////////////////////////////////////////////////////////////
void MeshContext::SetOGLModelViewMatrix(const piMATRIX& m)
{
	if((staticBuffer != nullptr) && modelViewMatrix)
	{// Make sure the mesh isn't in world space already
		oglSetWorldMatrix(modelViewMatrix);
	}
	else
	{// FIXME: rewrite oglSetWorldMatrix()
		oglSetWorldMatrix(&m);
	}
}

void MeshContext::DrawInstanced(int primcount) const
{
#ifdef BETA_RELEASE
    // Verify VBO objects
    if(!_draw)
        return;
#endif

    if(!ogl::UseDeprecatedPath())
    {
        if(staticBuffer && vao > 0)
        {
            VertexArrayObjectBind::Bind(vao);
            glDrawElementsInstanced(GL_TRIANGLES, elementCnt, GL_UNSIGNED_SHORT, BUFFER_OFFSET(idxOffset), primcount);
            return;
        }

        VertexArrayObjectBind::Bind(0);
        UINT32 crtAttributeOffset = 0;

        BindArrayBuffer(staticBuffer->GetVertexBufferName());

        if(vertexType & POSITION)
        {
            SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += POSITION_SIZE;
        }

        if(vertexType & NORMAL)
	    {
            SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += NORMAL_SIZE;
	    }

	    if(vertexType & TEX0)
	    {
            SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            if(vertexType & TEX1)
                SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            if(vertexType & TEX2)
                SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

		    crtAttributeOffset += TEX0_SIZE;
	    }

	    if(vertexType & ATTRIB6)
	    {
		    SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += ATTRIB6_SIZE;
	    }

	    BindElementArrayBuffer(staticBuffer->GetIndexBufferName());
	    DrawElementsInstanced(GL_TRIANGLES, elementCnt, GL_UNSIGNED_SHORT, BUFFER_OFFSET(idxOffset), primcount);
    }
}

//////////////////////////////////////////////////////////////////////
//	Draw
//
//	Update the related states and draw this batch
//////////////////////////////////////////////////////////////////////
void MeshContext::Draw() const
{
#ifdef BETA_RELEASE
    // Verify VBO objects
    if(!_draw)
        return;
#endif

    if(staticBuffer && vao > 0)
    {
        VertexArrayObjectBind::Bind(vao);
        GLCounter::instance().call_glDrawRangeElements(GL_TRIANGLES, minIdx, maxIdx, elementCnt, GL_UNSIGNED_SHORT, BUFFER_OFFSET(idxOffset));
        return;
    }

    VertexArrayObjectBind::Bind(0);
    UINT32 crtAttributeOffset = 0;

    if(ogl::UseDeprecatedPath())
    {
	    BindArrayBuffer(staticBuffer->GetVertexBufferName());

	    // Vertex attribute position
	    if(vertexType & POSITION)
	    {
		    SetVertexPointer(3, GL_FLOAT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += POSITION_SIZE;
	    }

	    // Vertex attribute normal
	    if(vertexType & NORMAL)
	    {
		    SetNormalPointer(GL_FLOAT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += NORMAL_SIZE;
	    }

	    // Vertex attribute color
	    if(vertexType & COLOR)
	    {
		    SetColorPointer(2, GL_UNSIGNED_INT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += COLOR_SIZE;
	    }

	    // Vertex attribute tex0
	    if(vertexType & TEX0)
	    {
		    SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            if(vertexType & TEX1)
                SetTextureUnitCoordPointer(GL_TEXTURE1, 2, GL_FLOAT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            if(vertexType & TEX2)
                SetTextureUnitCoordPointer(GL_TEXTURE2, 2, GL_FLOAT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            crtAttributeOffset += TEX0_SIZE;
	    }

	    // Vertex attribute tangent
	    if(vertexType & ATTRIB6)
	    {
		    SetVertexAttribPointer(6, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += ATTRIB6_SIZE;
	    }

	    if(vertexType & ATTRIB7)
	    {
		    SetVertexAttribPointer(7, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += ATTRIB7_SIZE;
	    }

	    if(vertexType & ATTRIB15)
	    {
		    SetVertexAttribPointer(15, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += ATTRIB15_SIZE;
	    }

	    // Vertex attribute index
	    if(vertexType & INDEX)
	    {
		    SetTextureUnitCoordPointer(GL_TEXTURE3, 4, GL_FLOAT, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += INDEX_SIZE;
	    }

	    BindElementArrayBuffer(staticBuffer->GetIndexBufferName());

	    DrawRangeElements(GL_TRIANGLES, minIdx, maxIdx, elementCnt, GL_UNSIGNED_SHORT, BUFFER_OFFSET(idxOffset));
    }
    else
    {
	    BindArrayBuffer(staticBuffer->GetVertexBufferName());

        if(vertexType & POSITION)
        {
            SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += POSITION_SIZE;
        }

        if(vertexType & NORMAL)
	    {
            SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += NORMAL_SIZE;
	    }

	    if(vertexType & TEX0)
	    {
            SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            if(vertexType & TEX1)
                SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

            if(vertexType & TEX2)
                SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));

		    crtAttributeOffset += TEX0_SIZE;
	    }

	    if(vertexType & ATTRIB6)
	    {
		    SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, 0, vertexSize, BUFFER_OFFSET(crtAttributeOffset));
		    crtAttributeOffset += ATTRIB6_SIZE;
	    }

	    BindElementArrayBuffer(staticBuffer->GetIndexBufferName());
	    DrawRangeElements(GL_TRIANGLES, minIdx, maxIdx, elementCnt, GL_UNSIGNED_SHORT, BUFFER_OFFSET(idxOffset));
    }
}
