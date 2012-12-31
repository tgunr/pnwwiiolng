#pragma once

#include "ogl/ogl.h"
#include "ogl/oglVertexBuffer.h"
#include "math/SuperFastHash.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

namespace ogl
{
	typedef	UINT16	VertexIdx;

	struct RawVertex
	{
		GLfloat	p_x,	p_y,	p_z;			// Position
		GLfloat n_x,	n_y,	n_z;			// Normal
		GLfloat t0_u,	t0_v;					// Texture coordinate 0
		//GLuint	color, specular					// Color
		//GLfloat t1_u,	t1_v;						// Texture coordinate 1
		//GLshort vertexIdx;						// Vertex program index value
		GLfloat tan_x, tan_y, tan_z;		    // 3 component tangent data
		GLfloat bin_x, bin_y, bin_z;		    // 3 component tangent data
		GLfloat nrm_x, nrm_y, nrm_z;		    // 3 component tangent data
	};
	inline bool operator == (const RawVertex &a, const RawVertex &b) { return !memcmp(&a, &b, sizeof(RawVertex)); }

	namespace	VertexAttributes
	{
		enum VertexAttribute
		{
			POSITION			= 0x00000001,
			NORMAL				= 0x00000002,
			COLOR				= 0x00000004,
			TEX0				= 0x00000008,	// Uses texture unit 0
			TEX1				= 0x00000010,	// Uses texture unit 1
			TEX2				= 0x00000020,	// Uses texture unit 2
			ATTRIB6				= 0x00000040,	// Uses attribute index 6
			ATTRIB7				= 0x00000080,	// Uses attribute index 7
			ATTRIB15			= 0x00000100,	// Uses attribute index 15
			INDEX				= 0x00000200	// Uses texture unit 3
		};

		enum VertexElementSize
		{	// Make sure this matches to the RawVertex definition
			POSITION_SIZE	= 12,
			NORMAL_SIZE		= 12,
			COLOR_SIZE		= 8,
			TEX0_SIZE		= 8,
			TEX1_SIZE		= 0,
			TEX2_SIZE		= 0,
			ATTRIB6_SIZE	= 12,
			ATTRIB7_SIZE	= 12,
			ATTRIB15_SIZE	= 12,
			INDEX_SIZE		= 2
		};
	}

	UINT32	VertexElementSize(UINT32 vertexType);

  typedef std::multimap<size_t,VertexIdx> RawVertexIndexer;

	class MeshContext;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//	StaticBuffer
	//
	//	Static Vertex Buffer
	//////////////////////////////////////////////////////////////////////////////////////////////////
	class StaticBuffer
	{
		private:

			UINT32					vertexType;					// Vertex type flags
			UINT32					vertexSize;					// Vertex size
			std::vector<RawVertex>	rawVertexData;				// Raw vertex data
			std::vector<VertexIdx>	rawIndexData;				// Raw Index data

//			void*					vbData;						// Tightly fitted vertex data
			void*					ibData;

			GLuint					vbName;						// Vertex buffer name
			GLuint					ibName;						// Index buffer name

            RawVertexIndexer vertexIndexer;


		public:

			StaticBuffer();
			//StaticBuffer(OBJECT* modelTree);
			StaticBuffer(int hfile);
			~StaticBuffer();		// Buffer objects are not deleted at program termination (does anyone still care?)

			void			Buffer();
			LOGICAL			AddMesh(std::vector<RawVertex>& rawVertexData, const VertexIdx* indices, VertexIdx elementCnt, MeshContext* meshContext, int bufferIdx);
			void			SaveToFile(int hfile);

			UINT32			GetVertexBufferName()	const	{return vbName;}
			UINT32			GetIndexBufferName()	const	{return ibName;}

			LOGICAL			IsBuffered()			const	{return vbName != 0 || ibName != 0;}

	}; // class StaticBuffer


	//////////////////////////////////////////////////////////////////////////////////////////////////
	//	MeshContext
	//
	//	Context for mesh properties, data and buffers
	//	Handles related global states against OGL
	//////////////////////////////////////////////////////////////////////////////////////////////////
	class MeshContext
	{
		private:

			const	piMATRIX*		modelViewMatrix;				// ModelInstance::modelViewMatrices has ownership

					UINT32			vertexType;						// Vertex type flags
					GLsizei			vertexSize;						// The size of one vertex in bytes
					int				bufferHandle;					// The index of the buffer that has to be resolved after .mdl loading
					VertexIdx		minIdx;							// Lowest index value used in this context
					VertexIdx		maxIdx;							// Highest index value used in this context
					UINT32			idxOffset;						// Index offset in the mesh buffer
					VertexIdx		elementCnt;						// The amount of vertices indexed
                    unsigned int vao;
			const	StaticBuffer*	staticBuffer;


		public:

					MeshContext(int fd, const std::vector<StaticBuffer*>& staticBuffers);
					MeshContext(UINT32 vertexType);
					~MeshContext()
                    {
                        //if(vao)
                       // {
                       //     glDeleteVertexArrays(1, &vao);
                       //     vao = 0;
                       // }
                    }

			void	SetModelViewMatrix(const piMATRIX* m)		{modelViewMatrix = m;}
			void	SetOGLModelViewMatrix(const piMATRIX& m);
			void	SetBufferPointer(StaticBuffer* newPointer)			{this->staticBuffer = newPointer;}
			void	SetBufferHandle(int newHandle)			{this->bufferHandle = newHandle;}
			void	SetMinIdx(VertexIdx min)					{minIdx = min;}
			void	SetMaxIdx(VertexIdx max)					{maxIdx = max;}
			void	SetIdxOffset(UINT32 offset)					{idxOffset = offset;}
			void	SetElementCnt(VertexIdx cnt)				{elementCnt = cnt;}
            void    SetVertexType(UINT32 type)                  {vertexType = type;}
			void	AddVertexAttribute(void* data, UINT32 attribute);
            void Apply();

			int		GetPoolSize();
			void	CopyToPool(UINT8* pool);

			UINT32	GetVertexType()			const	{return vertexType;}
			UINT32	GetVertexSize()			const	{return vertexSize;}
			LOGICAL	IsBuffered()			const	{return staticBuffer != NULL;}

			void	Draw() const;
            void DrawInstanced(int primcount) const;

	}; // MeshContext
} // ogl

