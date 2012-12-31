#include <syscfg.h> // Path is project specific.
/*-----------------------------------------------------------------------
Module: oglVertexBuffer.cpp
Description:
------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "gl/glplat.h"
#include <vector>
#include "ogl/oglVertexBuffer.h"
#include "ogl/ogl.h"
#include "ob/obdraw.h"
#include "ob/obmain.h"
#include "ob/obmgr.h"
#include "ob/obpool.h"
#include "ogl/oglMesh.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/States.h"
#include "gr2/gr2layer.h"
#include "ogl/GLCounter.h"

#ifndef GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV
#define GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV 0x8533
#endif

static Covert<bool> _vbodraw = true;

/*-----------------------------------------------------------------------
Function: oglVertexBufferToggleDraw
Description:
------------------------------------------------------------------------*/
void oglVertexBufferToggleDraw(bool draw)
{
    _vbodraw = draw;
}

/*-----------------------------------------------------------------------
Function: DrawRangleElementsPrimitive::Bind
Description:
------------------------------------------------------------------------*/
void DrawRangeElementsPrimitive::Bind()
{
    if(vao > 0)
        VertexArrayObjectBind::Bind(vao);
    else if(ibo > 0)
        ogl::BindElementArrayBuffer(ibo);
}

/*-----------------------------------------------------------------------
Function: DrawRangleElementsPrimitive::Draw
Description:
------------------------------------------------------------------------*/
void DrawRangeElementsPrimitive::Draw()
{
    Bind();

    if(vao > 0)
        GLCounter::instance().call_glDrawRangeElements(Mode, start, end, count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(byteoffset));
    else if(ibo > 0)
        ogl::DrawRangeElements(Mode, start, end, count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(byteoffset));
    else
        ogl::DrawRangeElements(Mode, start, end, count, GL_UNSIGNED_SHORT, (void*)&indices[0]);
}

/*-----------------------------------------------------------------------
Function: DrawElementsPrimitive::Bind
Description:
------------------------------------------------------------------------*/
void DrawElementsPrimitive::Bind()
{
    if(vao > 0)
        VertexArrayObjectBind::Bind(vao);
    else if(ibo > 0)
        ogl::BindElementArrayBuffer(ibo);
}

/*-----------------------------------------------------------------------
Function: DrawElementsPrimitive::DrawInstanced
Description:
------------------------------------------------------------------------*/
void DrawElementsPrimitive::DrawInstanced(int primcount)
{
    Bind();

    if(vao > 0)
        glDrawElementsInstanced(Mode, count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(byteoffset), primcount);
    else if(ibo > 0)
        ogl::DrawElementsInstanced(Mode, count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(byteoffset), primcount);
    else
        ogl::DrawElementsInstanced(Mode, count, GL_UNSIGNED_SHORT, (void*)&indices[0], primcount);
}

/*-----------------------------------------------------------------------
Function: DrawElementsPrimitive::Draw
Description:
------------------------------------------------------------------------*/
void DrawElementsPrimitive::Draw()
{
    Bind();

    if(vao > 0)
        GLCounter::instance().call_glDrawElements(Mode, count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(byteoffset));
    else if(ibo > 0)
        ogl::DrawElements(Mode, count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(byteoffset));
    else
        ogl::DrawElements(Mode, count, GL_UNSIGNED_SHORT, (void*)&indices[0]);
}

/*-----------------------------------------------------------------------
Function: DrawArraysPrimitive::Bind
Description:
------------------------------------------------------------------------*/
void DrawArraysPrimitive::Bind()
{
    if(vao)
        VertexArrayObjectBind::Bind(vao);
}

/*-----------------------------------------------------------------------
Function: DrawArraysPrimitive::Draw
Description:
------------------------------------------------------------------------*/
void DrawArraysPrimitive::Draw()
{
    Bind();

    if(vao > 0)
        GLCounter::instance().call_glDrawArrays(Mode, start, count);
    else
        ogl::DrawArrays(Mode, start, count);
}

/*-----------------------------------------------------------------------
Function: DrawArraysPrimitive::DrawInstanced
Description:
------------------------------------------------------------------------*/
void DrawArraysPrimitive::DrawInstanced(int primcount)
{
    Bind();

    if(vao > 0)
        glDrawArraysInstanced(Mode, start, count, primcount);
    else
        ogl::DrawArraysInstanced(Mode, start, count, primcount);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::oglVertexBuffer
Description:
------------------------------------------------------------------------*/
oglVertexBuffer::oglVertexBuffer() :
    Valid(false),
	BufferSize(0),
    StaticBufferID(0),
    DynamicBufferID(0),
    IndiceID(0),
	BufferSegment(NULL)
{
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::~oglVertexBuffer
Description:
------------------------------------------------------------------------*/
oglVertexBuffer::~oglVertexBuffer()
{
    if(BufferSegment)
        memFree(BufferSegment);

    BufferSegment = NULL;

    for(unsigned int i = 0; i < buffers.size(); i++)
    {
        for(unsigned int j = 0; j < buffers[i]->Primitives.size(); j++)
        {
            BasePrimitive* p = buffers[i]->Primitives[j];
            delete p;
        }

        delete buffers[i];
    }

    if(StaticBufferID)
	{
		glDeleteBuffersARB(1, (GLuint*)&StaticBufferID);
		StaticBufferID = 0;
	}

	if(DynamicBufferID)
	{
		glDeleteBuffersARB(1, (GLuint*)&DynamicBufferID);
		DynamicBufferID = 0;
	}

	if(IndiceID)
	{
		glDeleteBuffersARB(1, (GLuint*)&IndiceID);
		IndiceID = 0;
	}
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::DrawInstancedPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::DrawInstancedPrimitive(int count, unsigned int index, int id)
{
#ifdef BETA_RELEASE
    if(!_vbodraw)
        return;
#endif

    DataBuffer& db = *buffers[id];

    if(index < db.Primitives.size())
    {
        db.Bind();
        db.Primitives[index]->DrawInstanced(count);
    }
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::DrawPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::DrawPrimitive(unsigned int index, int id)
{
#ifdef BETA_RELEASE
    if(!_vbodraw)
        return;
#endif

    DataBuffer& db = *buffers[id];

    if(index < db.Primitives.size())
    {
        db.Bind();
        db.Primitives[index]->Draw();
    }
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::Bind
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::Bind(unsigned int primitiveindex, int id)
{
    DataBuffer& db = *buffers[id];

    if(primitiveindex < db.Primitives.size())
    {
        db.Bind();
        db.Primitives[primitiveindex]->Bind();
    }
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::DataBuffer::Bind
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::DataBuffer::Bind()
{
    switch(Mode)
	{
    case MODE_ARB_VAO:
        {
            break;
        }
	case MODE_ARB_VBO:
        {
            ogl::BindArrayBuffer(abo);

            if(VertexAttribs[OVB_TEXCOORD0].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD0;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, attr->Count, attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

		    if(VertexAttribs[OVB_TEXCOORD1].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD1;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, attr->Count, attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

		    if(VertexAttribs[OVB_TEXCOORD2].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD2;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE2, attr->Count, attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

		    if(VertexAttribs[OVB_TEXCOORD3].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD3;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE3, attr->Count, attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

		    if(VertexAttribs[OVB_COLOR].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_COLOR;
			    ogl::SetColorPointer(attr->Count, attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

		    if(VertexAttribs[OVB_NORMAL].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_NORMAL;
                ogl::SetNormalPointer(attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

		    if(VertexAttribs[OVB_POSITION].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_POSITION;
			    ogl::SetVertexPointer(attr->Count, attr->Type, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
		    }

            if(UsesAttribute)
            {
                for(int i = 0; i <= ATTRIBUTE_TANGENTNORMAL; ++i)
                {
                    if(VertexAttribs[i].IsActive())
                    {
			            StrideData* attr = VertexAttribs + i;
                        ogl::SetVertexAttribPointer(attr->Attr, attr->Count, attr->Type, 0, attr->Stride, BUFFER_OFFSET(offset + attr->Offset));
                    }
                }
            }

            break;
        }
	case MODE_SYSTEM_RAM:
        {
            unsigned char* start = segment + offset;

		    if(VertexAttribs[OVB_TEXCOORD0].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD0;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, attr->Count, attr->Type, attr->Stride, start + attr->Offset);
		    }

		    if(VertexAttribs[OVB_TEXCOORD1].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD1;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, attr->Count, attr->Type, attr->Stride, start + attr->Offset);
		    }

		    if(VertexAttribs[OVB_TEXCOORD2].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD2;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE2, attr->Count, attr->Type, attr->Stride, start + attr->Offset);
		    }

		    if(VertexAttribs[OVB_TEXCOORD3].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_TEXCOORD3;
			    ogl::SetTextureUnitCoordPointer(GL_TEXTURE3, attr->Count, attr->Type, attr->Stride, start + attr->Offset);
		    }

		    if(VertexAttribs[OVB_COLOR].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_COLOR;
			    ogl::SetColorPointer(attr->Count, attr->Type, attr->Stride, start + attr->Offset);
		    }

		    if(VertexAttribs[OVB_NORMAL].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_NORMAL;
			    ogl::SetNormalPointer(attr->Type, attr->Stride, start + attr->Offset);
		    }

		    if(VertexAttribs[OVB_POSITION].IsActive())
		    {
			    StrideData* attr = VertexAttribs + OVB_POSITION;
			    ogl::SetVertexPointer(attr->Count, attr->Type, attr->Stride, start + attr->Offset);
		    }

            if(UsesAttribute)
            {
                for(int i = 0; i <= ATTRIBUTE_TANGENTNORMAL; ++i)
                {
                    if(VertexAttribs[i].IsActive())
                    {
			            StrideData* attr = VertexAttribs + i;
                        ogl::SetVertexAttribPointer(attr->Attr, attr->Count, attr->Type, 0, attr->Stride, start + attr->Offset);
                    }
                }
            }

		    break;
        }
	}
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddBuffer
Description:
------------------------------------------------------------------------*/
int oglVertexBuffer::AddBuffer(const GLvoid* data,
                               unsigned int bytesize,
                               bool isStatic,
                               int mode)
{
    DataBuffer* db = new DataBuffer;

    db->bytesize = bytesize;

    db->segment = (unsigned char*)memAlloc(bytesize);
    memcpy(db->segment, data, bytesize);

    db->isStatic = isStatic;
    db->Mode = mode;

    if(mode == MODE_ARB_VAO)
    {
        if(ogl::UseDeprecatedPath())
        {
            db->Mode = MODE_ARB_VBO;
        }
    }

    buffers.push_back(db);

    return (int)(buffers.size() - 1);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::SetActiveAttribute
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::SetActiveAttribute(int id, int type, bool active)
{
    if(id < (int)buffers.size())
    {
        DataBuffer& db = *buffers[id];
        db.VertexAttribs[type].active = active;
    }
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::SetStride
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::SetStride(EnumAttrib attr,
                                unsigned int count,
                                GLenum type,
                                unsigned int stride,
                                unsigned int offset,
                                int id)
{
    DataBuffer& db = *buffers[id];

	db.VertexAttribs[attr].Attr = attr;
	db.VertexAttribs[attr].Type = type;
	db.VertexAttribs[attr].Count = count;
	db.VertexAttribs[attr].Offset = offset;
	db.VertexAttribs[attr].Stride = stride;
    db.VertexAttribs[attr].active = true;

    if(attr >= ATTRIBUTE_POSITION && attr <= ATTRIBUTE_TANGENTNORMAL)
        db.UsesAttribute = true;
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawRangeElementsPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawRangeElementsPrimitive(GLenum mode, int count, const piINDICES* indices, int id)
{
    DataBuffer& db = *buffers[id];

    DrawRangeElementsPrimitive* p = new DrawRangeElementsPrimitive(mode);
    p->count = count * 3;
    p->size = p->count * sizeof(unsigned short);

    int start = INT_MAX;
    int end = INT_MIN;

    // Make sure all types GL_UNSIGNED_SHORT
    for(int i = 0; i < count; i++)
    {
        start = std::min(start, (int)indices[i].v1);
        start = std::min(start, (int)indices[i].v2);
        start = std::min(start, (int)indices[i].v3);

        end = std::max(end, (int)indices[i].v1);
        end = std::max(end, (int)indices[i].v2);
        end = std::max(end, (int)indices[i].v3);

        p->indices.push_back(indices[i].v1);
        p->indices.push_back(indices[i].v2);
        p->indices.push_back(indices[i].v3);
    }

    p->start = start;
    p->end = end;

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawRangeElementsPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawRangeElementsPrimitive(GLenum mode, int count, const int* indices, int id)
{
    DataBuffer& db = *buffers[id];

    DrawRangeElementsPrimitive* p = new DrawRangeElementsPrimitive(mode);
    p->count = count;
    p->size = p->count * sizeof(unsigned short);

    int start = INT_MAX;
    int end = INT_MIN;

    // Make sure all types GL_UNSIGNED_SHORT
    for(int i = 0; i < count; i++)
    {
        start = std::min(start, indices[i]);
        end = std::max(end, indices[i]);

        p->indices.push_back((unsigned short)indices[i]);
    }

    p->start = start;
    p->end = end;

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawRangeElementsPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawRangeElementsPrimitive(GLenum mode, int count, const short* indices, int id)
{
    DataBuffer& db = *buffers[id];

    DrawRangeElementsPrimitive* p = new DrawRangeElementsPrimitive(mode);
    p->count = count;
    p->size = p->count * sizeof(unsigned short);

    int start = INT_MAX;
    int end = INT_MIN;

    // Make sure all types GL_UNSIGNED_SHORT
    for(int i = 0; i < count; i++)
    {
        int q = indices[i];

        start = std::min(start, q);
        end = std::max(end, q);

        p->indices.push_back((unsigned short)indices[i]);
    }

    p->start = start;
    p->end = end;

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawElementsPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawElementsPrimitive(GLenum mode, int count, const int* indices, int id)
{
    DataBuffer& db = *buffers[id];

    DrawElementsPrimitive* p = new DrawElementsPrimitive(mode);
    p->count = count;
    p->size = p->count * sizeof(unsigned short);

    // Make sure all types GL_UNSIGNED_SHORT
    for(int i = 0; i < count; i++)
    {
        p->indices.push_back((unsigned short)indices[i]);
    }

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawElementsPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawElementsPrimitive(GLenum mode, int count, const short* indices, int id)
{
    DataBuffer& db = *buffers[id];

    DrawElementsPrimitive* p = new DrawElementsPrimitive(mode);
    p->count = count;
    p->size = p->count * sizeof(unsigned short);

    // Make sure all types GL_UNSIGNED_SHORT
    for(int i = 0; i < count; i++)
    {
        p->indices.push_back((unsigned short)indices[i]);
    }

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawElementsPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawElementsPrimitive(GLenum mode, int count, const piINDICES* indices, int id)
{
    DataBuffer& db = *buffers[id];

    DrawElementsPrimitive* p = new DrawElementsPrimitive(mode);
    p->count = count * 3;
    p->size = p->count * sizeof(unsigned short);

    for(int i = 0; i < count; i++)
    {
        p->indices.push_back(indices[i].v1);
        p->indices.push_back(indices[i].v2);
        p->indices.push_back(indices[i].v3);
    }

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::AddDrawArraysPrimitive
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::AddDrawArraysPrimitive(GLenum mode, int start, int count, int id)
{
    DataBuffer& db = *buffers[id];

    DrawArraysPrimitive* p = new DrawArraysPrimitive(mode);

    p->start = start;
    p->count = count;
    p->size = p->count * sizeof(unsigned short);

    db.Primitives.push_back(p);
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::UpdateBuffer
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::UpdateBuffer(const GLvoid* data, int id)
{
    DataBuffer& db = *buffers[id];

    switch(db.Mode)
	{
	case MODE_ARB_VBO:
    case MODE_ARB_VAO:
        {
            ArrayBufferBind::Bind(db.abo);

            // Avoid sync issues with glBufferSubDataARB
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, db.bytesize, NULL, GL_DYNAMIC_DRAW_ARB);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, db.bytesize, data, GL_DYNAMIC_DRAW_ARB);

		    break;
	    }
	case MODE_SYSTEM_RAM:
        {
		    memcpy(BufferSegment, data, BufferSize);
		    break;
        }
	}
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::UpdatePartialBuffer
Description:
------------------------------------------------------------------------*/
bool oglVertexBuffer::UpdatePartialBuffer(unsigned int offset_in_bytes, const GLvoid* data, unsigned int size, int id)
{
    logTimerBeginMacro();

    DataBuffer& db = *buffers[id];

    switch(db.Mode)
	{
	case MODE_ARB_VBO:
    case MODE_ARB_VAO:
	    {
            ArrayBufferBind::Bind(db.abo);
            glBufferSubData(GL_ARRAY_BUFFER_ARB, db.offset + offset_in_bytes, size, data);
		    break;
	    }
	case MODE_SYSTEM_RAM:
        {
		    memcpy(BufferSegment + db.offset + offset_in_bytes, data, size);
		    break;
	    }
    }

    logTimerEndMacro();

	return true;
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::Buffer
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::Buffer()
{
	bool isstatic = true;

    int vbostaticsize = 0;
    int vbodynamicsize = 0;
    int ramsize = 0;

    for(unsigned int i = 0; i < buffers.size(); i++)
    {
        switch(buffers[i]->Mode)
        {
        case MODE_ARB_VAO:
        case MODE_ARB_VBO:
            {
                if(buffers[i]->isStatic)
                    vbostaticsize += buffers[i]->bytesize;
                else
                    vbodynamicsize += buffers[i]->bytesize;
                break;
            }
        case MODE_SYSTEM_RAM:
            {
                ramsize += buffers[i]->bytesize;
                break;
            }
        }
    }

    if(vbostaticsize > 0)
    {
        unsigned char* compoundbuff = (unsigned char*)memAlloc(vbostaticsize);
        unsigned char* copy = compoundbuff;

        int offset = 0;

        glGenBuffersARB(1, (GLuint*)&StaticBufferID);

        for(unsigned int i = 0; i < buffers.size(); i++)
        {
            if(buffers[i]->isStatic)
            {
                memcpy(copy, buffers[i]->segment, buffers[i]->bytesize);
                copy += buffers[i]->bytesize;
                buffers[i]->offset = offset;
                buffers[i]->abo = StaticBufferID;
                offset += buffers[i]->bytesize;
                memFree(buffers[i]->segment);
            }
        }

        ogl::BindArrayBuffer(StaticBufferID);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, vbostaticsize, compoundbuff, GL_STATIC_DRAW_ARB);
	    ogl::BindArrayBuffer(0);

        memFree(compoundbuff);
    }

    if(vbodynamicsize > 0)
    {
        unsigned char* compoundbuff = (unsigned char*)memAlloc(vbodynamicsize);
        unsigned char* copy = compoundbuff;

        int offset = 0;
        glGenBuffersARB(1, (GLuint*)&DynamicBufferID);

        for(unsigned int i = 0; i < buffers.size(); i++)
        {
            if(!buffers[i]->isStatic)
            {
                memcpy(copy, buffers[i]->segment, buffers[i]->bytesize);
                copy += buffers[i]->bytesize;
                buffers[i]->offset = offset;
                buffers[i]->abo = DynamicBufferID;
                offset += buffers[i]->bytesize;
                memFree(buffers[i]->segment);
            }
        }

        ogl::BindArrayBuffer(DynamicBufferID);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, vbodynamicsize, compoundbuff, GL_DYNAMIC_DRAW_ARB);
	    ogl::BindArrayBuffer(0);

        memFree(compoundbuff);
    }

    if(ramsize > 0)
    {
        unsigned char* compoundbuff = (unsigned char*)memAlloc(ramsize);
        unsigned char* copy = compoundbuff;

        int offset = 0;

        for(unsigned int i = 0; i < buffers.size(); i++)
        {
            memcpy(copy, buffers[i]->segment, buffers[i]->bytesize);
            copy += buffers[i]->bytesize;
            buffers[i]->offset = offset;
            offset += buffers[i]->bytesize;
            memFree(buffers[i]->segment);

            buffers[i]->segment = compoundbuff;
        }

        BufferSegment = compoundbuff;
    }

    int indexsize = 0;

    for(unsigned int i = 0; i < buffers.size(); i++)
    {
        switch(buffers[i]->Mode)
        {
        case MODE_ARB_VAO:
        case MODE_ARB_VBO:
            {
                for(unsigned int j = 0; j < buffers[i]->Primitives.size(); j++)
                {
                    BasePrimitive* p = buffers[i]->Primitives[j];

                    switch(p->PrimType)
                    {
                    case DRAW_ELEMENTS:
                        {
                            indexsize += p->size;
                            break;
                        }
                    case DRAW_RANGE_ELEMENTS:
                        {
                            indexsize += p->size;
                            break;
                        }
                    }
                }

                break;
            }
        }
    }

    if(indexsize)
    {
        unsigned char* indices = (unsigned char*)memAlloc(indexsize);
        unsigned char* copy = indices;

        int offset = 0;

        glGenBuffersARB(1, &IndiceID);

        for(unsigned int i = 0; i < buffers.size(); i++)
        {
            switch(buffers[i]->Mode)
            {
            case MODE_ARB_VAO:
            case MODE_ARB_VBO:
                {
                    for(unsigned int j = 0; j < buffers[i]->Primitives.size(); j++)
                    {
                        BasePrimitive* p = buffers[i]->Primitives[j];

                        switch(p->PrimType)
                        {
                        case DRAW_ELEMENTS:
                            {
                                DrawElementsPrimitive* de = dynamic_cast<DrawElementsPrimitive*>(p);

                                memcpy(copy, &de->indices[0], de->size);
                                de->indices.clear();

                                copy += de->size;

                                de->byteoffset = offset;
                                de->ibo = IndiceID;
                                offset += de->size;

                                break;
                            }
                        case DRAW_RANGE_ELEMENTS:
                            {
                                DrawRangeElementsPrimitive* de = dynamic_cast<DrawRangeElementsPrimitive*>(p);

                                memcpy(copy, &de->indices[0], de->size);
                                de->indices.clear();

                                copy += de->size;

                                de->byteoffset = offset;
                                de->ibo = IndiceID;
                                offset += de->size;

                                break;
                            }
                        }
                    }
                }
            }
        }

        ogl::BindElementArrayBuffer(IndiceID);
        glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexsize, indices, GL_STATIC_DRAW_ARB);
        ogl::BindElementArrayBuffer(0);

        memFree(indices);
    }

    for(unsigned int i = 0; i < buffers.size(); i++)
    {
        switch(buffers[i]->Mode)
        {
        case MODE_ARB_VAO:
            {
                ogl::Disable();

                for(unsigned int q = 0; q < buffers[i]->Primitives.size(); q++)
                {
                    BasePrimitive* bp = buffers[i]->Primitives[q];

                    glGenVertexArrays(1, &bp->vao);

                    VertexArrayObjectBind::Bind(bp->vao);
                    ArrayBufferBind::Bind(buffers[i]->abo);

                    for(int q = ATTRIBUTE_POSITION; q <= ATTRIBUTE_TANGENTNORMAL; q++)
                    {
                        if(buffers[i]->VertexAttribs[q].IsActive())
                        {
                            const StrideData& sd = buffers[i]->VertexAttribs[q];
                            glEnableVertexAttribArray(sd.Attr);
                            glVertexAttribPointer(sd.Attr, sd.Count, sd.Type, 0, sd.Stride, BUFFER_OFFSET(buffers[i]->offset + sd.Offset));
                        }
                    }

                    if(bp->ibo > 0)
                        ElementBufferBind::Bind(bp->ibo);

                    VertexArrayObjectBind::Bind(0);

                    ogl::Disable();
                }

                break;
            }
        }
    }

    Valid = true;
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::Apply
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::Apply(OBJECT* o, bool isstatic, int mode)
{
    piSTRIDEDDATA sd;
    o->StridedRenderData(&sd);

    AddBuffer(sd.position.pdata, o->numverts * sizeof(piUNIFIED_VERTEX), isstatic, mode);

    if(ogl::UseDeprecatedPath())
    {
        SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, sd.position.stride, 0);
        SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, sd.normal.stride, 12);
        SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, sd.textureCoords[0].stride, 28);

	    unsigned int count = obdrawGetMultiTextureCount(o->texid);

	    if(count >= 1)
            SetStride(oglVertexBuffer::OVB_TEXCOORD1, 2, GL_FLOAT, sd.textureCoords[0].stride, 28);

        if(count >= 2)
            SetStride(oglVertexBuffer::OVB_TEXCOORD2, 2, GL_FLOAT, sd.textureCoords[0].stride, 28);
    }
    else
    {
        SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, sd.position.stride, 0);
        SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, sd.normal.stride, 12);
        SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, sd.textureCoords[0].stride, 28);

	    unsigned int count = obdrawGetMultiTextureCount(o->texid);

	    if(count >= 1)
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, sd.textureCoords[0].stride, 28);

        if(count >= 2)
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, sd.textureCoords[0].stride, 28);
    }

    if(o->flags & OBJECT_INDEXED)
    {
        AddDrawRangeElementsPrimitive(GL_TRIANGLES,
                                      o->numtris,
                                      o->ptris);
    }
    else
    {
        AddDrawArraysPrimitive(GL_TRIANGLES, 0, o->numverts);
    }

    Buffer();
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::Apply
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::Apply(struct PoolNode* node, bool isstatic, int mode)
{
    static unsigned int stride = sizeof(piUNIFIED_VERTEX);

    if(node)
    {
        if(!(node->flags & OBJECT_NOTOBJECT))
        {
            const piUNIFIED_VERTEX* v = node->vertices;

            node->vboid = AddBuffer(&v[0], node->objdata->numverts * sizeof(piUNIFIED_VERTEX), isstatic, mode);

            if(ogl::UseDeprecatedPath())
            {
                SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, 0, node->vboid);
                SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, 12, node->vboid);
                SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, stride, 28, node->vboid);

	            unsigned int count = obdrawGetMultiTextureCount(node->texid);

	            if(count >= 1)
                    SetStride(oglVertexBuffer::OVB_TEXCOORD1, 2, GL_FLOAT, stride, 28, node->vboid);

                if(count >= 2)
                    SetStride(oglVertexBuffer::OVB_TEXCOORD2, 2, GL_FLOAT, stride, 28, node->vboid);
            }
            else
            {
                SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, stride, 0, node->vboid);
                SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, stride, 12, node->vboid);
                SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, stride, 28, node->vboid);

	            unsigned int count = obdrawGetMultiTextureCount(node->texid);

	            if(count >= 1)
                    SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 2, GL_FLOAT, stride, 28, node->vboid);

                if(count >= 2)
                    SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 2, GL_FLOAT, stride, 28, node->vboid);
            }

            if(node->flags & OBJECT_INDEXED)
            {
                AddDrawRangeElementsPrimitive(GL_TRIANGLES,
                                              node->objdata->numtris,
                                              node->indices,
                                              node->vboid);
            }
            else
            {
                AddDrawArraysPrimitive(GL_TRIANGLES, 0, node->objdata->numverts, node->vboid);
            }
        }

        Apply(node->sibling, isstatic, mode);
        Apply(node->child, isstatic, mode);
        Apply(node->attached, isstatic, mode);
    }
}

/*-----------------------------------------------------------------------
Function: oglVertexBuffer::Apply
Description:
------------------------------------------------------------------------*/
void oglVertexBuffer::Apply(struct gr2MeshData* node, bool isstatic, int mode)
{
#define Offset(type, member) ((unsigned int)&(((type*)NULL)->member))

    int groupcount = 0;
    granny_tri_material_group* group = NULL;

    if(node->is_normal)
    {
        int count = (int)GrannyGetMeshVertexCount(node->GrannyMesh);

        if(ogl::UseDeprecatedPath())
        {
            node->vboid = AddBuffer(&node->pngt[0], count * sizeof(granny_pngt3332_vertex), isstatic, mode);
            int stride = sizeof(granny_pngt3332_vertex);

            SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, Position[0]), node->vboid);
            SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, Normal[0]), node->vboid);
            SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, UV[0]), node->vboid);
            SetStride(oglVertexBuffer::OVB_TEXCOORD1, 3, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, Tangent[0]), node->vboid);
        }
        else
        {
            node->vboid = AddBuffer(&node->pwngt[0], count * sizeof(granny_pwngt34332_vertex), isstatic, mode);
            int stride = sizeof(granny_pwngt34332_vertex);

            SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, Position[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, Normal[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, UV[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, Tangent[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 4, GL_UNSIGNED_BYTE, stride, Offset(granny_pwngt34332_vertex, BoneWeights[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 4, GL_UNSIGNED_BYTE, stride, Offset(granny_pwngt34332_vertex, BoneIndices[0]), node->vboid);
        }
    }
    else
    {
        int count = (int)GrannyGetMeshVertexCount(node->GrannyMesh);

        if(ogl::UseDeprecatedPath())
        {
            if(node->pnt)
            {
                node->vboid = AddBuffer(&node->pnt[0], count * sizeof(granny_pnt332_vertex), isstatic, mode);
                int stride = sizeof(granny_pnt332_vertex);

                SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, Offset(granny_pnt332_vertex, Position[0]), node->vboid);
                SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, Offset(granny_pnt332_vertex, Normal[0]), node->vboid);
                SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, stride, Offset(granny_pnt332_vertex, UV[0]), node->vboid);
            }
            else
            {
                node->vboid = AddBuffer(&node->pngt[0], count * sizeof(granny_pngt3332_vertex), isstatic, mode);
                int stride = sizeof(granny_pngt3332_vertex);

                SetStride(oglVertexBuffer::OVB_POSITION, 3, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, Position[0]), node->vboid);
                SetStride(oglVertexBuffer::OVB_NORMAL, 3, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, Normal[0]), node->vboid);
                SetStride(oglVertexBuffer::OVB_TEXCOORD0, 2, GL_FLOAT, stride, Offset(granny_pngt3332_vertex, UV[0]), node->vboid);
            }
        }
        else
        {
            node->vboid = AddBuffer(&node->pwngt[0], count * sizeof(granny_pwngt34332_vertex), isstatic, mode);
            int stride = sizeof(granny_pwngt34332_vertex);

            SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, Position[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, Normal[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, stride, Offset(granny_pwngt34332_vertex, UV[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD1, 4, GL_UNSIGNED_BYTE, stride, Offset(granny_pwngt34332_vertex, BoneWeights[0]), node->vboid);
            SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD2, 4, GL_UNSIGNED_BYTE, stride, Offset(granny_pwngt34332_vertex, BoneIndices[0]), node->vboid);
        }
    }

	group = GrannyGetMeshTriangleGroups(node->GrannyMesh);
	groupcount = GrannyGetMeshTriangleGroupCount(node->GrannyMesh);

    for(int i = 0; i < groupcount; ++i)
    {
        short* indice;

        indice = node->indexArray;
        indice += 3 * group[i].TriFirst;

        AddDrawElementsPrimitive(GL_TRIANGLES,
                                 group[i].TriCount * 3,
                                 indice,
                                 node->vboid);
	}
}
