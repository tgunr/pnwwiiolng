#pragma once

#include "pi/pi.h"
#include <vector>
#include "osgImport/Referenced.h"
#include "osgImport/ref_ptr.h"

#if(0)
#define wglAllocateMemoryNV
#define wglFreeMemoryNV
#endif

#define DEF_MODE_NV_VAR_IMPLEMENTED
#undef  DEF_MODE_NV_VAR_IMPLEMENTED

enum PrimitiveType
{
    DRAW_ELEMENTS,
    DRAW_RANGE_ELEMENTS,
    DRAW_ARRAYS
};

class oglVertexBuffer;

struct BasePrimitive
{
    int PrimType;
    int size;
    int byteoffset;
    unsigned int vao;
    unsigned int ibo;
    GLenum Mode;

    virtual void Bind() = 0;
    virtual void Draw() = 0;
    virtual void DrawInstanced(int primcount) = 0;

    virtual ~BasePrimitive()
    {
        if(GLEW_ARB_vertex_array_object)
        {
            if(vao > 0)
            {
                glDeleteVertexArrays(1, &vao);
                vao = 0;
            }
        }
    }

    BasePrimitive(int type, GLenum mode) :
        PrimType(type),
        size(0),
        byteoffset(0),
        vao(0),
        ibo(0),
        Mode(mode)
    {
    }
};

struct DrawRangeElementsPrimitive : public BasePrimitive
{
    std::vector<unsigned short> indices;
    int start;
    int end;
    int count;

    DrawRangeElementsPrimitive(GLenum mode) :
        BasePrimitive(DRAW_RANGE_ELEMENTS, mode),
        start(0),
        end(0),
        count(0)
    {
    }

    virtual ~DrawRangeElementsPrimitive()
    {
    }

    virtual void Bind();
    virtual void Draw();
    virtual void DrawInstanced(int primcount)
    {
        // No GL implementation
    }
};

struct DrawElementsPrimitive : public BasePrimitive
{
    std::vector<unsigned short> indices;
    int count;

    DrawElementsPrimitive(GLenum mode) :
        BasePrimitive(DRAW_ELEMENTS, mode),
        count(0)
    {
    }

    virtual ~DrawElementsPrimitive()
    {
    }

    virtual void Bind();
    virtual void Draw();
    virtual void DrawInstanced(int primcount);
};

struct DrawArraysPrimitive : public BasePrimitive
{
    int count;
    int start;

    DrawArraysPrimitive(GLenum mode) :
        BasePrimitive(DRAW_ARRAYS, mode),
        count(0),
        start(0)
    {
    }

    virtual ~DrawArraysPrimitive()
    {
    }

    virtual void Bind();
    virtual void Draw();
    virtual void DrawInstanced(int primcount);
};

class oglVertexBuffer : public Referenced
{
public:

    enum EnumAttrib
    {
        ATTRIBUTE_POSITION,
        ATTRIBUTE_NORMAL,
        ATTRIBUTE_TEXCOORD0,
        ATTRIBUTE_TEXCOORD1,
        ATTRIBUTE_TEXCOORD2,
        ATTRIBUTE_TEXCOORD3,
        ATTRIBUTE_TANGENT,
        ATTRIBUTE_BINORMAL,
        ATTRIBUTE_COLOR,
        ATTRIBUTE9,
        ATTRIBUTE10,
        ATTRIBUTE11,
        ATTRIBUTE12,
        ATTRIBUTE13,
        ATTRIBUTE14,
        ATTRIBUTE_TANGENTNORMAL,
	    OVB_NORMAL,
	    OVB_COLOR,
	    OVB_TEXCOORD0,
	    OVB_TEXCOORD1,
	    OVB_TEXCOORD2,
	    OVB_TEXCOORD3,
	    OVB_POSITION,
	    OVB_NUM_ATTRIBS
    };

    enum
    {
        MODE_NONE,
        MODE_ARB_VAO,
	    MODE_ARB_VBO,
	    MODE_NV_VAR,
	    MODE_SYSTEM_RAM,
	    MODE_UNKNOWN
    };

    oglVertexBuffer();
    virtual ~oglVertexBuffer();

    bool UpdatePartialBuffer(unsigned int offset, const GLvoid* data, unsigned int size, int id = 0);
    bool IsValid() {return Valid;}

	void SetStride(EnumAttrib attr,
                   unsigned int count,
                   GLenum type,
                   unsigned int stride,
                   unsigned int offset,
                   int id = 0);
    void AddDrawElementsPrimitive(GLenum type, int count, const int* indices, int id = 0);
    void AddDrawElementsPrimitive(GLenum mode, int count, const piINDICES* indices, int id = 0);
    void AddDrawElementsPrimitive(GLenum mode, int count, const short* indices, int id = 0);
    void AddDrawRangeElementsPrimitive(GLenum mode, int count, const piINDICES* indices, int id = 0);
    void AddDrawRangeElementsPrimitive(GLenum mode, int count, const int* indices, int id = 0);
    void AddDrawRangeElementsPrimitive(GLenum mode, int count, const short* indices, int id = 0);
    void AddDrawArraysPrimitive(GLenum mode, int start, int count, int id = 0);
    void Apply(struct OBJECT* o, bool isstatic = true, int mode = MODE_ARB_VBO);
    void Apply(struct PoolNode* node, bool isstatic = true, int mode = MODE_ARB_VBO);
    void Apply(struct gr2MeshData* node, bool isstatic = true, int mode = MODE_ARB_VBO);

    void Buffer();
    void UpdateBuffer(const GLvoid* data, int id = 0);
    void DrawPrimitive(unsigned int index = 0, int id = 0);
    void DrawInstancedPrimitive(int count, unsigned int index = 0, int id = 0);
    void Bind(unsigned int index = 0, int id = 0);
    void SetActiveAttribute(int id, int type, bool active);

    unsigned int indexID() {return IndiceID;}

    int AddBuffer(const GLvoid* data,
                  unsigned int bytesize,
                  bool isStatic,
                  int mode = MODE_ARB_VBO);

private:

    struct StrideData
    {
	    StrideData() :
		    Attr(OVB_POSITION),
		    Type(GL_FLOAT),
		    Count(-1),
		    Offset(-1),
		    Stride(-1),
            active(false)
	    {
	    }

	    inline bool IsActive() const
        {
            return Count > -1 && active;
        }

	    EnumAttrib Attr;
        GLenum Type;
	    int Count;
	    int Offset;
	    int Stride;
        bool active;
    };

    unsigned int BufferSize;
    bool Valid;
    unsigned char* BufferSegment;

	// ARB VBO mode
	unsigned int StaticBufferID;
	unsigned int DynamicBufferID;
	unsigned int IndiceID;

	struct DataBuffer
	{
        DataBuffer() :
            Mode(MODE_ARB_VBO),
			offset(0),
            abo(0),
            bytesize(0),
            segment(nullptr),
            isStatic(true),
            UsesAttribute(false)
		{
		}

        void Bind();
        void Unbind();

		int Mode;
        int offset;
        unsigned int abo;

        unsigned int bytesize;

		unsigned char* segment;

        bool isStatic;
        bool UsesAttribute;

        std::vector<BasePrimitive*> Primitives;

        StrideData VertexAttribs[OVB_NUM_ATTRIBS];
 	};

    std::vector<DataBuffer*> buffers;
};

