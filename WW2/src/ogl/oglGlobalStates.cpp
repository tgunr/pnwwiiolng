#include <syscfg.h> // Path is project specific.
#include "ogl/oglGlobalStates.h"
#include "ogl/States.h"
#include <assert.h>
#include "ogl/GLCounter.h"

enum VertexAttributeTypes
{
	VERTEX_ARRAY	= 0,
	NORMAL_ARRAY,
	COLOR_ARRAY,
	INDEX_ARRAY,
	//TEXTURE_COORD_ARRAY,
	EDGE_FLAG_ARRAY,
	VERTEX_ATTRIBUTE_TYPE_COUNT
};

#define MAX_VERTEX_ATTRIBUTES   16
static	LOGICAL	vertexAttributeArrayEnabled[VERTEX_ATTRIBUTE_TYPE_COUNT]	= {FALSE};
static	LOGICAL vertexAttributeArrayUpdated[VERTEX_ATTRIBUTE_TYPE_COUNT]	= {FALSE};
static	LOGICAL	textureUnitCoordArrayEnabled[MAX_TEXTURE_UNITS]				= {FALSE};
static	LOGICAL	textureUnitCoordArrayUpdated[MAX_TEXTURE_UNITS]				= {FALSE};
static  LOGICAL vertexAttributeDataArrayEnabled[MAX_VERTEX_ATTRIBUTES]      = {FALSE};
static  LOGICAL vertexAttributeDataArrayUpdated[MAX_VERTEX_ATTRIBUTES]      = {FALSE};
static	GLenum	activeTextureUnit											= 0xffff;

static bool	elementArrayBufferUpdated = false;
static bool	arrayBufferUpdated = false;
static bool	newArrayBufferContext = false;
static bool useDeprecatedPath = true;

static const GLvoid* normalPointer = (GLvoid*)0xffff;
static const GLvoid* vertexPointer = (GLvoid*)0xffff;
static const GLvoid* colorPointer = (GLvoid*)0xffff;
static const GLvoid* texPointer[MAX_TEXTURE_UNITS] = {(GLvoid*)0xffff};
static const GLvoid* attrPointer[MAX_VERTEX_ATTRIBUTES] = {(GLvoid*)0xffff};

static inline void UpdateArrayBufferContext()
{
    VertexArrayObjectBind::Bind(0);

	if(!arrayBufferUpdated)
	{
		ogl::BindArrayBuffer(0);
	}
}

static inline void UpdateElementArrayBufferContext()
{
    VertexArrayObjectBind::Bind(0);

    if(!elementArrayBufferUpdated)
	{
		ogl::BindElementArrayBuffer(0);
	}
}

static inline void UpdateVertexAttributeContexts()
{
	GLenum	oglClientType;

    VertexArrayObjectBind::Bind(0);

	for(int i= 0; i < VERTEX_ATTRIBUTE_TYPE_COUNT; i++)
	{
		if((vertexAttributeArrayUpdated[i] == FALSE) && (vertexAttributeArrayEnabled[i] == TRUE))
		{
			switch(i)
			{
				case VERTEX_ARRAY:
                    oglClientType = GL_VERTEX_ARRAY;
                    break;
				case NORMAL_ARRAY:
                    oglClientType = GL_NORMAL_ARRAY;
                    break;
				case COLOR_ARRAY:
                    oglClientType = GL_COLOR_ARRAY;
                    break;
				case INDEX_ARRAY:
                    oglClientType = GL_INDEX_ARRAY;
                    break;
				case EDGE_FLAG_ARRAY:
                    oglClientType = GL_EDGE_FLAG_ARRAY;
                    break;
					assert(FALSE);
			}

            glDisableClientState(oglClientType);
			vertexAttributeArrayEnabled[i] = FALSE;
		}

		vertexAttributeArrayUpdated[i] = FALSE;
	}

	for(GLenum i = 0; i < MAX_TEXTURE_UNITS; i++)
	{
		if((textureUnitCoordArrayUpdated[i] == FALSE) && (textureUnitCoordArrayEnabled[i] == TRUE))
		{
			if(activeTextureUnit != i)
			{
				glClientActiveTextureARB(GL_TEXTURE0 + i);
				activeTextureUnit = i;
			}

			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			textureUnitCoordArrayEnabled[i] = FALSE;
            texPointer[i] = (GLvoid*)0xffff;
		}
		textureUnitCoordArrayUpdated[i] = FALSE;
	}

    if(glDisableVertexAttribArray)
    {
        for(int q = 0; q < MAX_VERTEX_ATTRIBUTES; q++)
        {
            if(!vertexAttributeDataArrayUpdated[q] && vertexAttributeDataArrayEnabled[q])
            {
                // Disable user vertex attributes
                glDisableVertexAttribArray(q);
                vertexAttributeDataArrayEnabled[q] = FALSE;
                attrPointer[q] = (GLvoid*)0xffff;
            }

            // Disable next pass
            vertexAttributeDataArrayUpdated[q] = FALSE;
        }
    }

	newArrayBufferContext		= FALSE;
	arrayBufferUpdated			= FALSE;
}

using namespace ogl;	// This can't be used for the declaration of inlined function with VC ... strange

void ogl::SetUseDeprecatedPath(bool flag)
{
    useDeprecatedPath = flag;
}

bool ogl::UseDeprecatedPath()
{
    return useDeprecatedPath;
}

void ogl::SetVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
	static	GLint			crtSize		= 0xffff;
	static	GLenum			crtType		= 0xffffffff;
	static	GLsizei			crtStride	= 0xffffffff;

	UpdateArrayBufferContext();
	if((vertexAttributeArrayEnabled[VERTEX_ARRAY] == FALSE))
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		vertexAttributeArrayEnabled[VERTEX_ARRAY] = TRUE;
	}
	if((vertexPointer != pointer) || newArrayBufferContext || (crtSize != size) || (crtType != type) || (crtStride != stride))
	{
		glVertexPointer(size, type, stride, pointer);
		crtSize = size; crtType = type; crtStride = stride; vertexPointer = pointer;
	}
	vertexAttributeArrayUpdated[VERTEX_ARRAY] = TRUE;
}


void ogl::SetNormalPointer(GLenum type, GLsizei stride, const GLvoid* pointer)
{
	static	GLenum			crtType		= 0xffffffff;
	static	GLsizei			crtStride	= 0xffffffff;


	UpdateArrayBufferContext();
	if((vertexAttributeArrayEnabled[NORMAL_ARRAY] == FALSE))
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		vertexAttributeArrayEnabled[NORMAL_ARRAY] = TRUE;
	}
	if((normalPointer != pointer) || newArrayBufferContext || (crtType != type) || (crtStride != stride))
	{
		glNormalPointer(type, stride, pointer);
		crtType = type; crtStride = stride; normalPointer = pointer;
	}
	vertexAttributeArrayUpdated[NORMAL_ARRAY] = TRUE;
}


void ogl::SetColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
	static	GLint			crtSize		= 0xffff;
	static	GLenum			crtType		= 0xffffffff;
	static	GLsizei			crtStride	= 0xffffffff;

	UpdateArrayBufferContext();
	if(vertexAttributeArrayEnabled[COLOR_ARRAY] == FALSE)
	{
		glEnableClientState(GL_COLOR_ARRAY);
		vertexAttributeArrayEnabled[COLOR_ARRAY] = TRUE;
	}
	if((colorPointer != pointer) || newArrayBufferContext || (crtSize != size) || (crtType != type) || (crtStride != stride))
	{
		glColorPointer(size, type, stride, pointer);
		crtSize = size; crtType = type; crtStride = stride; colorPointer = pointer;
	}
	vertexAttributeArrayUpdated[COLOR_ARRAY] = TRUE;
}


void ogl::SetTextureUnitCoordPointer(GLenum oglTextureUnit, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
			GLenum			textureUnitIndex				= oglTextureUnit - GL_TEXTURE0;
	static	GLint			crtSize[MAX_TEXTURE_UNITS]		= {0xffff};
	static	GLenum			crtType[MAX_TEXTURE_UNITS]		= {0xffffffff};
	static	GLsizei			crtStride[MAX_TEXTURE_UNITS]	= {0xffffffff};

	assert(textureUnitIndex < MAX_TEXTURE_UNITS);

	UpdateArrayBufferContext();
	if(textureUnitCoordArrayEnabled[textureUnitIndex] == FALSE)
	{
		if(activeTextureUnit != textureUnitIndex)
		{
			glClientActiveTextureARB(oglTextureUnit);
			activeTextureUnit = textureUnitIndex;
		}
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		textureUnitCoordArrayEnabled[textureUnitIndex] = TRUE;
	}
	if((texPointer[textureUnitIndex] != pointer) || newArrayBufferContext || (crtSize[textureUnitIndex] != size) || (crtType[textureUnitIndex] != type) || (crtStride[textureUnitIndex] != stride))
	{
		if(activeTextureUnit != textureUnitIndex)
		{
			glClientActiveTextureARB(oglTextureUnit);
			activeTextureUnit = textureUnitIndex;
		}
		glTexCoordPointer(size, type, stride, pointer);
		crtSize[textureUnitIndex] = size; crtType[textureUnitIndex] = type; crtStride[textureUnitIndex] = stride; texPointer[textureUnitIndex] = pointer;
	}
	textureUnitCoordArrayUpdated[textureUnitIndex] = TRUE;
}

void ogl::SetVertexAttribPointer(unsigned int index,
                                 GLint size,
                                 GLenum type,
                                 GLboolean normalized,
                                 GLsizei stride,
                                 const GLvoid* ptr)
{
    static int crtSize[MAX_VERTEX_ATTRIBUTES] = {-1};
    static unsigned int crtType[MAX_VERTEX_ATTRIBUTES] = {-1};
    static int crtStride[MAX_VERTEX_ATTRIBUTES] = {-1};

    UpdateArrayBufferContext();

    if(glVertexAttribPointer && index < MAX_VERTEX_ATTRIBUTES)
    {
        if(!vertexAttributeDataArrayEnabled[index])
        {
            glEnableVertexAttribArray(index);
            vertexAttributeDataArrayEnabled[index] = TRUE;
        }

        if(newArrayBufferContext ||
           attrPointer[index] != ptr ||
           crtSize[index] != size ||
           crtType[index] != type ||
           crtStride[index] != stride)
        {
            attrPointer[index] = ptr;
            crtSize[index] = size;
            crtType[index] = type;
            crtStride[index] = stride;

            glVertexAttribPointer(index, size, type, normalized, stride, ptr);
        }
    }

    vertexAttributeDataArrayUpdated[index] = TRUE;
}

void ogl::BindElementArrayBuffer(GLuint name)
{
    ElementBufferBind::Bind(name);
    elementArrayBufferUpdated = name == 0 ? 0 : 1;
}

void ogl::BindArrayBuffer(GLuint name)
{
    newArrayBufferContext = ArrayBufferBind::Bind(name);
    arrayBufferUpdated = name == 0 ? 0 : 1;
}


void ogl::DrawArrays(GLenum mode, GLint first, GLsizei count)
{
	UpdateVertexAttributeContexts();
	GLCounter::instance().call_glDrawArrays(mode, first, count);
}

void ogl::DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, int primcount)
{
	UpdateVertexAttributeContexts();
	glDrawArraysInstanced(mode, first, count, primcount);
}


void ogl::DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	UpdateVertexAttributeContexts();
	UpdateElementArrayBufferContext();

	GLCounter::instance().call_glDrawElements(mode, count, type, indices);

 	elementArrayBufferUpdated = false;
}


void ogl::DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, int primcount)
{
	UpdateVertexAttributeContexts();
	UpdateElementArrayBufferContext();

    glDrawElementsInstanced(mode, count, type, indices, primcount);

 	elementArrayBufferUpdated = false;
}


void ogl::DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices)
{
	UpdateVertexAttributeContexts();
	UpdateElementArrayBufferContext();

	GLCounter::instance().call_glDrawRangeElements(mode, start, end, count, type, indices);

	elementArrayBufferUpdated = false;
}

void ogl::Disable()
{
	GLenum	oglClientType;

    VertexArrayObjectBind::Bind(0);

	for(int i=0; i < VERTEX_ATTRIBUTE_TYPE_COUNT; i++)
	{
		if(vertexAttributeArrayEnabled[i] == TRUE)
		{
			switch(i)
			{
				case VERTEX_ARRAY:			oglClientType = GL_VERTEX_ARRAY;		break;
				case NORMAL_ARRAY:			oglClientType = GL_NORMAL_ARRAY;		break;
				case COLOR_ARRAY:			oglClientType = GL_COLOR_ARRAY;			break;
				case INDEX_ARRAY:			oglClientType = GL_INDEX_ARRAY;			break;
				case EDGE_FLAG_ARRAY:		oglClientType = GL_EDGE_FLAG_ARRAY;		break;
					assert(FALSE);
			}
			glDisableClientState(oglClientType);
			vertexAttributeArrayEnabled[i] = FALSE;
		}
		vertexAttributeArrayUpdated[i] = FALSE;
	}

	for(GLenum i=0; i < MAX_TEXTURE_UNITS; i++)
	{
		if(textureUnitCoordArrayEnabled[i] == TRUE)
		{
			if(activeTextureUnit != i)
			{
				glClientActiveTextureARB(GL_TEXTURE0+i);
				activeTextureUnit = i;
			}
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			textureUnitCoordArrayEnabled[i] = FALSE;
		}
		textureUnitCoordArrayUpdated[i] = FALSE;
        texPointer[i] = (GLvoid*)0xffff;
	}

    if(glDisableVertexAttribArray)
    {
        for(int q = 0; q < MAX_VERTEX_ATTRIBUTES; q++)
        {
            if(vertexAttributeDataArrayEnabled[q])
            {
                // Disable user vertex attributes
                glDisableVertexAttribArray(q);
                vertexAttributeDataArrayEnabled[q] = FALSE;
                attrPointer[q] = (GLvoid*)0xffff;
            }

            vertexAttributeDataArrayUpdated[q] = FALSE;
        }
    }

    normalPointer = (GLvoid*)0xffff;
    vertexPointer = (GLvoid*)0xffff;
    colorPointer = (GLvoid*)0xffff;

	BindArrayBuffer(0);
	BindElementArrayBuffer(0);
}
