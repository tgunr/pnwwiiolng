#include <syscfg.h> // Path is project specific.
#include <string>

#ifdef OPENGL3D

#define GLOBALS
#include "ogl/ogl.h"
#undef GLOBALS

#include "ddp.h"
#include "math/math3d.h"
#include "mem/mem.h"
#include "systhread.h"
#include "ld/lddds.h"
#include "math/mathnum.h"
#include "rn/rnmain.h"
#include "ui/uiChat.h"
#include "rn/rnvis.h"
#include "ob/obdlite.h"
#include "ld/ldtexture.h"
#include "pi/piinfo.h"

#include "include/vars.h"
#include "cfg/cfgCodes.h"
#include "shader/Shader.h"
#include "shader/glsl.h"
#include "fio/fio.h"
#include "ogl/oglGlobalStates.h"
#include "ui/uiUTF.h"
#include "ogl/oglTextureObject.h"
#include "ogl/States.h"

#include <vector>
#include <queue>

#include "math/Matrixf.h"
#include "sys/sysWinISO.h"	// Must be the last thing we include
#include "ogl/GLCounter.h"

#if platOS == OS_MACOS
#include <malloc/malloc.h>
#endif

enum
{
    DOWNSAMPLE_SHADER,
    FILTERX_SHADER,
    FILTERY_SHADER,
    FINAL_SHADER,
    MAX_FILTER_SHADERS
};

enum
{
    DOWNSAMPLE_TEXTURE,
    FILTERX_TEXTURE,
    FILTERY_TEXTURE,
    MAX_FILTER_INDEXES
};

static std::vector<SINT32> g_textureIndex;

static std::deque<TEXTURE *> g_freeTextures;

// Defines
#define DDP_REVERSE_GL_COLORS
#if platOS == OS_MACOS
#undef DDP_REVERSE_GL_COLORS
#include <pthread.h>
#endif

#define OGL_EVENT_LOG
#ifndef EVENT_LOG
#undef OGL_EVENT_LOG
#endif	// EVENT_LOG

#define OGL_EVENT_LOG_EXTREME
#ifndef EVENT_LOG_EXTREME
#undef OGL_EVENT_LOG_EXTREME
#endif	// EVENT_LOG_EXTREME

#define STORE_COMPRESSED_TEXTURES
//#undef STORE_COMPRESSED_TEXTURES

#define USE_DXT_ALPHA_COLLIDERS
#ifndef STORE_COMPRESSED_TEXTURES
#undef USE_DXT_ALPHA_COLLIDERS
#endif	// STORE_COMPRESSED_TEXTURES

#define MAX_RENDER_BUCKETS		256
#define MAXRENDERSTATES			256
#define MAXTEXTURESTAGES		9
#define MAXTEXTURESTATES		32

// Too much of a pain to try and mutex lock textures array
// hopefully I'll put in texture unloading before too long.
#define MINTEXTURES				4096 + 512

#define MIN3DTEXTURES			64
#define MINMATERIALS			16
#define MAXINDEXSOURCES			256
#define MAXVERTEXSOURCES		256
#define MAXLIGHTSTATES          16

#define REVERSE_NEAR_FAR_PLANE  0
#define FOV_Z_FAR               100000.0f

// Statics
static SINT32		texminwidth, texminheight;

static UINT32		renderstates[MAXRENDERSTATES];
static UINT32		savedrenderstates[MAXRENDERSTATES];
static UINT32		stagestates[MAXTEXTURESTAGES][MAXTEXTURESTATES];
static UINT32       lightstates[MAXLIGHTSTATES];

TEXTURE* textures;

static SINT32		numtextures;
static SINT32		numtexture3Ds;
static TEXTURE3D*	texture3Ds;
static SINT32		nummaterials;
static MATERIAL*	materials;
static SINT32		numlights;
static LIGHT*		lights;

static piMATRIX		projection;
static piMATRIX		world;
static piMATRIX		view;
static piMATRIX		texmat[8];
static piMATRIX		cockpitview;
static piMATRIX		piMatUnscaledView;
static piMATRIX     modelview;

static Matrixf reflectionCamera;

static UINT32	    sourceBlend;
static UINT32	    destinationBlend;
static UINT32		_AlphaFunc;

static GLenum crtMode = 0;

static FLOAT32		_AlphaRef;

static std::map<std::string, int> materialnames;

std::map<int,std::string> reverse_materialnames;

static SINT32 _activeMaterial = 0;

static LOGICAL g_statelocked[MAXRENDERSTATES] = {0};
static LOGICAL g_texturelocked = 0;
static LOGICAL g_materiallocked = 0;

static LOGICAL _lockTextureMatrix = FALSE;
static LOGICAL _lockWorldMatrix = FALSE;
static LOGICAL _lockViewMatrix = FALSE;
static LOGICAL _lockUnscaledViewMatrix = FALSE;
static LOGICAL _lockProjectionMatrix = FALSE;
static LOGICAL _lockCockpitMatrix = FALSE;
static LOGICAL _lockViewport = FALSE;

static systhread_mutex textureMutex, materialMutex;

// Used to track the number of objects sent to the rendering engine
static UINT32		totdraw;
static UINT32		totdrawindex;
static UINT32		totvertices;
static UINT32		totprimitives;
static UINT32		totpoints;
static UINT32		totlines;
static UINT32		tottriangles;
static UINT32		renderbuckets[MAX_RENDER_BUCKETS];
static UINT32		renderindexbuckets[MAX_RENDER_BUCKETS];

static SINT32		vx;
static SINT32		vy;
static SINT32		vwidth;
static SINT32		vheight;

static FLOAT32		znearsave;
static FLOAT32		zfarsave;

static ref_ptr<oglTextureObject> tbo;

// Prototypes
static void _RenderScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, UINT32 VertexCount, UINT32 IndexCount,
							void* vertex, SINT32 vertexStride, void* normal, SINT32 normalStride,
							void* color, SINT32 colorStride, void* tex, SINT32 texStride, void* index);
static void _RenderScene3D(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, UINT32 VertexCount, UINT32 IndexCount,
							void* vertex, SINT32 vertexStride, void* normal, SINT32 normalStride,
							void* color, SINT32 colorStride, void* tex, SINT32 texStride, void* index);
static void RenderScene(const oglTVERTEX& pv, int stride, int count, const void* indices);

static SINT32 _GetNewTextureId(void);
static SINT32 _GetNewTexture3DId(void);
static SINT32 _GetNewMaterialId(void);

static void _GetOpenGLState(oglRENDERSTATETYPES state);

static LOGICAL HandleGLError(LOGICAL bMessage)
{
#ifdef OGL_EVENT_LOG_EXTREME
LOGICAL	bError=TRUE;
SINT32		enumber;

	while ((enumber = glGetError()) != GL_NO_ERROR)
	{
	char*	ptr;

		if (bMessage)
		{
			switch (enumber)
			{
			case GL_INVALID_ENUM:
				ptr = "An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_INVALID_VALUE:
				ptr = "A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_INVALID_OPERATION:
				ptr = "The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_STACK_OVERFLOW:
				ptr = "This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_STACK_UNDERFLOW:
				ptr = "This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_OUT_OF_MEMORY:
				ptr = "There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded";
				break;
			default:
				break;
			}

			Write_Debug("GL error 0x%04x encountered\r\n %s", enumber, ptr);
		}

		bError = FALSE;
	}

	return(bError);
#endif
	return(TRUE);
}

oglTextureObject* oglGetTBO()
{
    return tbo.get();
}

unsigned int oglGetTextureName(int id)
{
    if(id < 0 || id >= numtextures)
        return 0;

    return textures[id].genlist;
}

LOGICAL oglSetupRC(void)
{
    oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_FILLMODE, piFILL_SOLID);
	oglSetRenderState(oglRENDERSTATE_ZFUNC, ZFunction);
    oglSetRenderState(oglRENDERSTATE_ZENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, piSHADE_GOURAUD);
    oglSetRenderState(oglRENDERSTATE_TEXTUREPERSPECTIVE, 1);
	oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    oglSetRenderStageState(0, oglTSS_TEXTURE_ENV_MODE, piTSS_MODULATE);

    // No engine state for these
    glHint(GL_FOG_HINT, GL_FASTEST);
	oglDisableTexture();
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, 1));
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);

	oglResize(WantedWidth, WantedHeight);

    if(!HandleGLError(TRUE))
    {
        Write_Debug("[oglSetupRC] GL error encountered");
    }

    return TRUE;
}

void oglResize(GLsizei w, GLsizei h)
{
	// Set Viewport to window dimensions
	oglSetViewportDimensions(0, 0, w, h, 0.0f, 1.0f);
	glDepthRange(0.0f, 1.0f);

	// Viewing transformation
	oglSetMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear color is black
	oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, piRGBA(0, 0, 0, 1));

	// Clear color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (!HandleGLError(TRUE))
	{
		Write_Debug("GL error encountered in oglResize");
	}
}

LOGICAL oglDrawToScreen(SINT32 texid, LOGICAL stretch)
{
LOGICAL		bRetVal = TRUE;
FLOAT32		fLeft = 0.0f;
FLOAT32		fTop = 0.0f;
FLOAT32		fRight = 0.0f;
FLOAT32		fBottom = 0.0f;
FLOAT32		stu, stv;
GLenum		enumFormat;
oglSURFACEDESC	sd;
TEXTURE*	lpt;

#ifdef DDP_REVERSE_GL_COLORS
	enumFormat = GL_BGR_EXT;
#else
	enumFormat = GL_RGB;
#endif

	oglLockTexture(texid, 0, &sd);
	lpt = oglGetTextureData(texid);

    oglSetMatrixMode(GL_PROJECTION);

    glPushMatrix();
    {
        glLoadIdentity();
	    glOrtho(0.0f, (FLOAT32)ddpGetWidth(), (FLOAT32)ddpGetHeight(), 0.0f, 1.0f, -1.0f);

        oglSetMatrixMode(GL_MODELVIEW);

        glPushMatrix();
        {
            glLoadIdentity();
	        oglEnableTexture();
            TextureState::Bind(textures[texid].genlist, GL_TEXTURE_2D);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, oglTADDRESS_WRAP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, oglTADDRESS_WRAP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, oglTADDRESS_WRAP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, oglFILTER_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, oglFILTER_LINEAR);

	        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

            glTexImage2D(GL_TEXTURE_2D,		// Target
                         0,					// Level of detail
                         3,					// Internal format of color components
                         sd.Width,			// Width of texture
                         sd.Height,			// Height of texture
                         0,					// Width of the border
                         enumFormat,		// Format GL_RGB...etc
                         GL_UNSIGNED_BYTE,	// Type of pixel data
                         sd.Surface);		// Pixels

			if (stretch)
			{
				// Stretch to fill screen
				fLeft = 0.0f;
				fRight = (FLOAT32)ddpGetWidth();
				fTop = 0.0f;
				fBottom = (FLOAT32)ddpGetHeight();
			}
			else
			{
				// Center on screen (Remember: if the loaded texture is not power of two on
				// both sides then the resizing process could cause gaps to appear at screen edges)
				fLeft = (FLOAT32)(ddpGetWidth() - sd.Width) * 0.5f;
				fRight = fLeft + sd.Width;
				fTop = (FLOAT32)(ddpGetHeight() - sd.Height) * 0.5f;
				fBottom = fTop + sd.Height;

				if (fLeft < fTop)
				{
					fTop -= fLeft;
					fBottom += fLeft;
					fLeft = 0.0f;
					fRight = (FLOAT32)ddpGetWidth();
				}
				else
				if (fTop < fLeft)
				{
					fLeft -= fTop;
					fRight += fTop;
					fTop = 0.0f;
					fBottom = (FLOAT32)ddpGetHeight();
				}
			}

			stu = lpt->widthfactor[0];
			stv = lpt->heightfactor[0];

            glBegin(GL_TRIANGLES);
            {
		        glTexCoord2f(0.0f, 0.0f);
		        glVertex2f(fLeft, fBottom);

		        glTexCoord2f(0.0f, stv);
		        glVertex2f(fLeft, fTop);

		        glTexCoord2f(stu, stv);
		        glVertex2f(fRight, fTop);

		        glTexCoord2f(0.0f, 0.0f);
		        glVertex2f(fLeft, fBottom);

		        glTexCoord2f(stu, stv);
		        glVertex2f(fRight, fTop);

		        glTexCoord2f(stu, 0.0f);
		        glVertex2f(fRight, fBottom);
            }
            glEnd();

            oglSetMatrixMode(GL_PROJECTION);
        }
        glPopMatrix();

        oglSetMatrixMode(GL_MODELVIEW);
    }
    glPopMatrix();

	oglUnlockTexture(texid, 0);

    ddpFlip();

	if (!HandleGLError(TRUE))
	{
		Write_Debug("GL error encountered in oglDrawToScreen");
	}

	return bRetVal;
}

bool oglCanUseTexture2D(int w, int h)
{
    return GLEW_ARB_texture_non_power_of_two || (isPowerOf2(h) && isPowerOf2(w));
}

/************************************************************************************************
	ogl
*************************************************************************************************/

LOGICAL oglIsTexture(SINT32 texid, SINT32 miplevel)
{
    if(!textures) return FALSE;
    if(texid < 0) return FALSE;
    if(texid >= numtextures) return FALSE;
    if(textures[texid].miplevels == 0) return FALSE;
    if(textures[texid].miplevels <= miplevel) return FALSE;

    if(textures[texid].istga())
    {
        // Do nothing
    }
    if(textures[texid].isdds())
    {
        if(textures[texid].iswritable)
        {
            if(textures[texid].lpsurf[miplevel] == NULL)
            {
                return FALSE;
            }
        }
    }
    else if(textures[texid].iswritable || textures[texid].hasalpha)
    {
        // Need lpsurf for these types
        if(textures[texid].lpsurf[miplevel] == NULL)
        {
            return FALSE;
        }
    }

    return TRUE;
}

LOGICAL oglIsTexture3D(SINT32 texid)
{
	if (texid < 0 || texid >= numtexture3Ds || texture3Ds == NULL)
	{
		return(FALSE);
	}

	return(TRUE);
}

LOGICAL oglTextureHasAlpha(SINT32 texid)
{
	return(texid >= 0 && texid < numtextures && textures[texid].hasalpha);
}

static std::map<std::string,SINT32> textureIndex;
#ifdef BETA_RELEASE
std::map<GLuint, std::string> reverseTextureIndex;
#endif

SINT32 oglGetNamedTexture(const char* name)
{
  std::string str = name;
  std::transform(str.begin(), str.end(),str.begin(), ::toupper);

  std::replace(str.begin(), str.end(), '\\', '/');

  std::map<std::string,SINT32>::iterator it = textureIndex.find(str);
  if ( it != textureIndex.end() )
  {
    return (*it).second;
  }

	return(-1);
}

LOGICAL oglAddTexRef(SINT32 texid)
{
TEXTURE* lptex;

	if (!oglIsTexture(texid, 0))
	{
		return(FALSE);
	}

	lptex = oglGetTextureData(texid);
	lptex->numuses += 1;

	return(TRUE);
}

void oglUpdateTextures(void)
{
}

TEXTURE* oglGetTextureData2(SINT32 texid)
{
    if(texid < 0 || texid >= numtextures || !textures)
    {
        return NULL;
    }

    return &textures[texid];
}

TEXTURE* oglGetTextureData(SINT32 texid)
{
    if(oglIsTexture(texid, 0))
    {
        return &textures[texid];
    }

    return NULL;
}

TEXTURE3D* oglGetTexture3DData(SINT32 texid)
{
	if (oglIsTexture3D(texid))
	{
		return(&texture3Ds[texid]);
	}

	return(NULL);
}

void oglSetMinTextureDim(SINT32 mintexdim)
{
	texminwidth = mintexdim;
	texminheight = mintexdim;
}

LOGICAL oglLockTexture(SINT32 texid, SINT32 miplevel, oglSURFACEDESC* lpsd)
{
	return oglGetTextureMemoryDescriptor(texid, miplevel, lpsd);
}

LOGICAL oglUnlockTexture(SINT32 texid, SINT32 miplevel)
{
	if (!oglLoadTexture(texid, miplevel))
	{
		return(FALSE);
	}

	return(TRUE);
}

LOGICAL oglGetTextureMemory(SINT32* total, SINT32* free)
{
	*total = 0;
	*free = 0;
	return TRUE;
}

SINT32 oglGetTextureMipmapLevel(SINT32 texid)
{
	if (!oglIsTexture(texid, 0))
	{
		return(-1);
	}

	return(textures[texid].miplevels);
}

LOGICAL oglReleaseTextureHandles(void)
{
	return TRUE;
}

SINT32 oglGetTexture3DHandle(SINT32 tex3d)
{
    return -1;
}

SINT32 oglGetTextureHandle(SINT32 texid, SINT32 miplevel)
{
SINT32		retval, i, miplevels;

	if (bTexturesDisabled)
	{
		return(0);
	}

	if (!oglIsTexture(texid, miplevel))
	{
		return(-1);
	}

    if(textures[texid].isdds())
    {
        ldddsGenerateTexture(texid);
        return 0;
    }

	if (textures[texid].lpsurf[miplevel] == NULL)
	{
		return(-1);
	}

	retval = 1;
	if (textures[texid].hdevtex[miplevel] == 0)
	{
		HandleGLError(FALSE);	// Clear older messages

		if (!textures[texid].iswritable)
		{
			// If the texture is read-only and it has already been loaded into texture memory
			// then just acknowledge the "get texture handle" process and return to caller.
			if (textures[texid].comp[miplevel] != 3 && textures[texid].comp[miplevel] != 4)
			{
#ifdef OGL_EVENT_LOG_EXTREME
				Write_Debug("Texture already has compressed color components");
#endif	// OGL_EVENT_LOG_EXTREME
				textures[texid].hdevtex[miplevel] = 1;
				return(retval);
			}
		}

		// How many miplevels does this texture have
		miplevels = textures[texid].miplevels;

		if (textures[texid].genlist == 0)
		{
// extra bit of validity checking now we have threading
#if defined(BETA_RELEASE) && (platOS == OS_MACOS)
//		if(!pthread_main_np())
//			piExitProgram(__FILE__, __LINE__, "glGenTextures called outside main thread!");
#endif

			glGenTextures(1, (GLuint*)&textures[texid].genlist);

/*            if(glGetError() != GL_NO_ERROR)
            {
                GLenum e = glGetError();
                const GLubyte* s = gluErrorString(e);

                Write_Log(LOG_OLD, LOG_WARN, "%s FAILED to generate gl texture", textures[texid].name, s);

                return 0;
            }*/
		}

        TextureState::Bind(textures[texid].genlist, GL_TEXTURE_2D);
		if (!glIsTexture(textures[texid].genlist))
		{
			//	glGenTextures is not supported!
			Write_Debug("glGenTextures is not supported!");
			retval = -1;
		}

		// Set options
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, oglTADDRESS_WRAP);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, oglTADDRESS_WRAP);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, oglTADDRESS_WRAP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, miplevels > 1 ? oglFILTER_LINEARMIPLINEAR : oglFILTER_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, oglFILTER_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, oglFILTER_LINEAR);
		////glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

		if (!HandleGLError(TRUE))
		{
			Write_Debug("GL error encountered in oglGetTextureHandle for texture %d/%d", texid, miplevel);
			retval = -1;
		}

		if (textures[texid].comp[miplevel] != textures[texid].format[miplevel])
		{
#ifdef DDP_REVERSE_GL_COLORS
			glTexImage2D(GL_TEXTURE_2D, miplevel,
                         textures[texid].format[miplevel],
						 textures[texid].po2width[miplevel],
                         textures[texid].po2height[miplevel],
						 0,
                         textures[texid].comp[miplevel] == 3 ? GL_BGR_EXT : GL_BGRA_EXT,
						 GL_UNSIGNED_BYTE,
                         textures[texid].lpsurf[miplevel]);
#else
			glTexImage2D(GL_TEXTURE_2D,
                         miplevel,
                         textures[texid].format[miplevel],
						 textures[texid].po2width[miplevel],
                         textures[texid].po2height[miplevel],
						 0,
                         textures[texid].comp[miplevel] == 3 ? GL_RGB : GL_RGBA,
						 GL_UNSIGNED_BYTE,
                         textures[texid].lpsurf[miplevel]);
#endif
		}

		if (!HandleGLError(TRUE))
		{
			Write_Debug("GL error encountered in oglGetTextureHandle for texture %d/%d", texid, miplevel);
			retval = -1;
		}
#ifdef OGL_EVENT_LOG
		else
		{
		SINT32		compressed, width, height, format, size;

			// GL_TEXTURE_COMPRESSED, GL_TEXTURE_WIDTH, GL_TEXTURE_HEIGHT, GL_TEXTURE_INTERNAL_FORMAT, and GL_TEXTURE_COMPRESSED_IMAGE_SIZE
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (GLint*)&width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint*)&height);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&format);

			if(GLEW_ARB_texture_compression)
			{
                size = 0;
                compressed = 0;

                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, (GLint*)&compressed);

                if(compressed)
                {
                    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, (GLint*)&size);
                }
			}
			else
			{
				compressed = FALSE;
				size = width * height * textures[texid].comp[0];
			}

            Write_Debug("Texture image details: "
						" Image is %12s,"
						" Width: %4d,"
						" Height: %4d,"
						" Format: 0x%04x,"
						" Size: %7d",
						compressed ? "compressed" : "uncompressed",
						width,
						height,
						format,
						size);

			if (!HandleGLError(TRUE))
			{
				Write_Debug("GL error encountered in oglGetTextureHandle for texture %d/%d", texid, miplevel);
				retval = -1;
			}
		}
#endif	// OGL_EVENT_LOG

#ifdef STORE_COMPRESSED_TEXTURES
		if (GLEW_ARB_texture_compression)
		{
			if (!textures[texid].iswritable)
			{
#if(0) // OpenGL error
				glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, (GLint*)&textures[texid].imagesize[miplevel]);
                GLenum glerror = glGetError();

                if(glerror == GL_NO_ERROR)
                {
                    if(textures[texid].lpsurf[miplevel])
                    {
                        memFree(textures[texid].lpsurf[miplevel]);
                    }

                    textures[texid].lpsurf[miplevel] = memAlloc(textures[texid].imagesize[miplevel]);

				    glGetCompressedTexImageARB(GL_TEXTURE_2D, miplevel, textures[texid].lpsurf[miplevel]);
                }

                textures[texid].comp[miplevel] = textures[texid].format[miplevel];
#endif
			}
		}
#endif

		textures[texid].hdevtex[miplevel] = 1;

		// Create the remaining miplevels
		for (i=1; i < miplevels; i++)
		{
			if (textures[texid].hdevtex[i] == 0)
			{
				if (textures[texid].lpsurf[i] == NULL)
				{
					Write_Debug("Found an active mip level with a NULL surface pointer!");
				}

#ifdef DDP_REVERSE_GL_COLORS
				glTexImage2D(GL_TEXTURE_2D, i,
                             textures[texid].format[i],
							 textures[texid].po2width[i],
                             textures[texid].po2height[i],
							 0,
                             textures[texid].comp[i] == 3 ? GL_BGR_EXT : GL_BGRA_EXT,
							 GL_UNSIGNED_BYTE,
                             textures[texid].lpsurf[i]);
#else
				glTexImage2D(GL_TEXTURE_2D,
                             i,
                             textures[texid].format[i],
							 textures[texid].po2width[i],
                             textures[texid].po2height[i],
							 0,
                             textures[texid].comp[i] == 3 ? GL_RGB : GL_RGBA,
							 GL_UNSIGNED_BYTE,
                             textures[texid].lpsurf[i]);
#endif
				if (!HandleGLError(TRUE))
				{
					Write_Debug("GL error encountered in oglGetTextureHandle for texture %d/%d", texid, i);
					retval = -1;
				}
#ifdef OGL_EVENT_LOG
				else
				{
				SINT32		compressed, width, height, format, size;

					// GL_TEXTURE_COMPRESSED, GL_TEXTURE_WIDTH, GL_TEXTURE_HEIGHT, GL_TEXTURE_INTERNAL_FORMAT, and GL_TEXTURE_COMPRESSED_IMAGE_SIZE
					glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_WIDTH, (GLint*)&width);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_HEIGHT, (GLint*)&height);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&format);

					if (GLEW_ARB_texture_compression)
					{
                        size = 0;
                        compressed = 0;

						glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_COMPRESSED_ARB, (GLint*)&compressed);

                        if(compressed)
                        {
                            glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, (GLint*)&size);
                        }
					}
					else
					{
						compressed = FALSE;
						size = width * height * textures[texid].comp[i];
					}

					Write_Debug("Texture mipmap details:"
								" Image is %12s,"
								" Width: %4d,"
								" Height: %4d,"
								" Format: 0x%04x,"
								" Size: %7d",
								compressed ? "compressed" : "uncompressed",
								width,
								height,
								format,
								size);

					if (!HandleGLError(TRUE))
					{
						Write_Debug("GL error encountered in oglGetTextureHandle for texture %d/%d", texid, miplevel);
						retval = -1;
					}
				}
#endif	// OGL_EVENT_LOG

                // Need mipmap surfaces for decals. If reverting to old texture
                // creation (where textures are created during gameplay, these
                // surfaces can be removed.
                if(!textures[texid].iswritable)
                {
				    // These are not needed after initial load
				    memFree(textures[texid].lpsurf[i]);
				    textures[texid].lpsurf[i] = NULL;
                }

                textures[texid].hdevtex[i] = 1;
			}
			else
			{
				Write_Debug("Found an active mip level when there shouldn't be one!");
				retval = -1;
			}
		}
	}

    if(!textures[texid].iswritable)
    {
	    // These are not needed unless writable or for alpha collider
	    memFree(textures[texid].lpsurf[0]);
	    textures[texid].lpsurf[0] = NULL;
    }

    textures[texid].timeaccessed = frametime;

	return(retval);
}

LOGICAL oglProjectSceneOnTexture(SINT32 texid, SINT32 miplevel, DRAW_OBJECTS DrawObjects)
{
	textures[texid].timeaccessed = frametime;

	return TRUE;
}

void oglUnloadTexture(SINT32 texid, SINT32 miplevel)
{
	if (miplevel == 0)
	{
		if (textures[texid].genlist != 0)
		{
// extra bit of validity checking now we have threading
#if defined(BETA_RELEASE) && (platOS == OS_MACOS)
//	if(!pthread_main_np())
//		piExitProgram(__FILE__, __LINE__, "glDeleteTextures called outside main thread!");
#endif
            if(textures[texid].newrelease)
            {
                ldTextureRelease(&textures[texid]);
                textures[texid].newrelease = false;
                // Cannot delete this. New textures reuse texture objects. Only delete on app kill.
                //glDeleteTextures(1, &textures[texid].genlist);
            }
            else
            {
                glDeleteTextures(1, (GLuint*)&textures[texid].genlist);
    			textures[texid].genlist = 0;
            }
		}
	}

	if (textures[texid].lpsurf[miplevel] != NULL)
	{
		memFree(textures[texid].lpsurf[miplevel]);
		textures[texid].lpsurf[miplevel] = NULL;
	}

	textures[texid].hdevtex[miplevel] = 0;

	textures[texid].timeaccessed = frametime;
}

void oglUnloadTexture3D(SINT32 tex3d)
{
SINT32	i, j, texid;

	if (texture3Ds[tex3d].genlist != 0)
	{
		glDeleteTextures(1, (GLuint*)&texture3Ds[tex3d].genlist);

		texture3Ds[tex3d].genlist = 0;
	}

	for (i=0; i < texture3Ds[tex3d].imagedepth; i++)
	{
		texid = texture3Ds[tex3d].texid[i];
		for (j=textures[texid].miplevels-1; j >= 0; j--)
		{
			if (textures[texid].lpsurf[j] != NULL)
			{
				memFree(textures[texid].lpsurf[j]);
				textures[texid].lpsurf[j] = NULL;
			}

			textures[texid].hdevtex[j] = 0;
		}

		textures[texid].timeaccessed = frametime;
		textures[texid].miplevels = 0;
		textures[texid].numuses = 0;
	}

    if(texture3Ds[tex3d].texid)
    {
	    memFree(texture3Ds[tex3d].texid);
	    texture3Ds[tex3d].texid = NULL;
    }
}

void oglFreeTexture(SINT32 texid, SINT32 miplevel)
{
	if (!oglIsTexture(texid, miplevel))
	{
		return;
	}

	oglUnloadTexture(texid, miplevel);
}

void oglFreeTexture3D(SINT32 texid)
{
	if (!oglIsTexture3D(texid))
	{
		return;
	}

	oglUnloadTexture3D(texid);
}

void oglFreeMipTextures(SINT32 texid)
{
SINT32   i;

	if (!oglIsTexture(texid, 0))
	{
		return;
	}

	if (textures[texid].numuses > 1)
	{
		textures[texid].numuses -= 1;
		return;
	}

	if (textures[texid].miplevels == 0)
	{
		Write_Debug("oglReleaseMipTexture called for empty texture %d", texid);
		return;
	}

	for (i=textures[texid].miplevels-1; i >= 0; i--)
	{
		oglFreeTexture(texid, i);
	}

	textures[texid].miplevels = 0;
	textures[texid].numuses = 0;

    if((texid + 1) != textures[texid].id)
    {
        Write_Log(LOG_OLD, LOG_INFO, "[oglFreeMipTextures] Texture ID mismatch. THIS IS BAD!");
    }

    g_freeTextures.push_back(&textures[texid]);

    if(g_freeTextures.size() > (UINT32)numtextures)
    {
        Write_Log(LOG_OLD, LOG_INFO, "[oglFreeMipTextures] free texture size exceeds numtextures!");
    }

    TextureState::Bind(0, textures[texid].target);
}

void oglFreeMipTexture3Ds(SINT32 texid)
{
	if (!oglIsTexture3D(texid))
	{
		return;
	}

	if (texture3Ds[texid].numuses > 1)
	{
		texture3Ds[texid].numuses -= 1;
		return;
	}

	oglFreeTexture3D(texid);

	texture3Ds[texid].numuses = 0;
}

void oglDisableTexturesAboveAndIncluding(int unit)
{
    unit = unit >= MAX_TEXTURE_UNITS ? MAX_TEXTURE_UNITS - 1 : unit < 0 ? 0 : unit;

    while(unit < MAX_TEXTURE_UNITS)
    {
        oglSetActiveTextureStage(unit);
        oglDisableTexture();
        unit++;
    }

    oglSetActiveTextureStage(0);
}

void oglSetActiveTextureStage(SINT32 stage)
{
    TextureState::SetTextureUnit(stage);
}

LOGICAL oglSetActiveTexture(SINT32 stage, SINT32 texid)
{
    if(stage < 0 || stage >= MAX_TEXTURE_UNITS)
    {
        return FALSE;
    }

    if(g_texturelocked)
    {
        return TRUE;
    }

    oglSetActiveTextureStage(stage);

    if(oglIsTexture(texid, 0))
    {
	    oglEnableTexture(textures[texid].target);
    }

	if (oglIsTexture(texid, 0))
	{
		if (textures[texid].hdevtex[0] == 0)
		{
	        oglDisableTexture();

			if (oglGetTextureHandle(texid, 0) == -1)
			{
				Write_Debug("Textures disabled, unable to load new texture at miplevel");
				return(FALSE);
			}
		}

        TextureState::Start();
        TextureState::Bind(stage, textures[texid].genlist, textures[texid].target);
        TextureState::End();

		textures[texid].timeaccessed = frametime;
	}
	else
	{
	    oglDisableTexture();
	}

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglSetActiveTexture");
	}

	return(TRUE);
}

LOGICAL oglSetActiveTexture3D(SINT32 stage, SINT32 texid)
{
    return FALSE;
}

void oglEnableTexture(GLenum tgt)
{
    TextureState::Enable(tgt);
}

void oglDisableTexture()
{
    TextureState::Disable();
}

LOGICAL oglDisableTexture3D(void)
{
	return(TRUE);
}

FLOAT32 oglLongLikeFloat(SINT32 l)
{
struct
{
	union
	{
		FLOAT32 f;
		SINT32 l;
	};

} fll;

	fll.l = l;
	return(fll.f);
}

SINT32 oglFindBitShift(UINT32 Mask)
{
SINT32	i;

	for (i=0, Mask=~Mask; Mask & 1; Mask>>=1, i++) /* Empty */;

	return(i);
}

SINT32 oglFindBitCount(UINT32 Mask)
{
SINT32	i;

	for (i=0; Mask; Mask>>=1)
	{
		if (Mask & 1)
		{
			i += 1;
		}
	}

	return(i);
}

UINT32 oglColorToDescColor(LPVOID lpdd, UINT32 color)
{
SINT32     RedBits, RedShift, RedMask;
SINT32     GreenBits, GreenShift, GreenMask;
SINT32     BlueBits, BlueShift, BlueMask;
SINT32     AlphaBits, AlphaShift, AlphaMask;
oglSURFACEDESC*		lpsd;

	lpsd = (oglSURFACEDESC*)lpdd;
	RedMask     = lpsd->PixelFormat.dwRBitMask;
	GreenMask   = lpsd->PixelFormat.dwGBitMask;
	BlueMask    = lpsd->PixelFormat.dwBBitMask;
	AlphaMask   = lpsd->PixelFormat.dwRGBAlphaBitMask;

	RedBits     = 8 - oglFindBitCount(RedMask);
	RedShift    = oglFindBitShift(RedMask);
	GreenBits   = 8 - oglFindBitCount(GreenMask);
	GreenShift  = oglFindBitShift(GreenMask);
	BlueBits    = 8 - oglFindBitCount(BlueMask);
	BlueShift   = oglFindBitShift(BlueMask);
	AlphaBits   = 8 - oglFindBitCount(AlphaMask);
	AlphaShift  = oglFindBitShift(AlphaMask);

	return(((piRGBA_GETRED(color) >> RedBits) << RedShift) |
			((piRGBA_GETGREEN(color) >> GreenBits) << GreenShift) |
			((piRGBA_GETBLUE(color) >> BlueBits) << BlueShift) |
			((piRGBA_GETALPHA(color) >> AlphaBits) << AlphaShift));
}


SINT32 oglCreateRawTexture3D(SINT32 texid, SINT32 miplevel, SINT32 width, SINT32 height, SINT32 depth, SINT32 bpp, SINT32 alphabits, SINT32 flags)
{
SINT32		pdepth;

	if (width > gMaxTextureWidth ||
		height > gMaxTextureHeight ||
		depth > gMaxTextureWidth)
	{
		Write_Debug("The requested texture dimensions are not supported");
		return(-1);
	}

	if (texid >= 0)
	{
		return(-1);
	}
	else
	{
		texid = _GetNewTexture3DId();
		if (texid < 0)
		{
			return(-1);
		}
	}

	for (pdepth=texminwidth; pdepth < depth && pdepth < gMaxTextureWidth; pdepth *= 2) /* Empty */;

	texture3Ds[texid].name[0] = 0;
	texture3Ds[texid].hasalpha = alphabits;
	texture3Ds[texid].iswritable = flags & CRT_FLAG_WRITABLE;
	texture3Ds[texid].genlist = 0;
	texture3Ds[texid].texid = (SINT32*)memAlloc(8 * sizeof(SINT32));
	texture3Ds[texid].imagedepth = depth;
	texture3Ds[texid].po2depth = pdepth;
	texture3Ds[texid].depthfactor = (FLOAT32)depth / (FLOAT32)pdepth;
	texture3Ds[texid].timeaccessed = frametime;
	texture3Ds[texid].numuses = 1;

	return(texid);
}

void oglTogglePostFilter()
{
    bool supported = piinfoIsSupportedPref(cfgPOST_RENDER_FILTER);

    g_postRenderFilter = !g_postRenderFilter && supported;
    uiChatPrintLine(L"Post filter is %s", g_postRenderFilter ? L"ENABLED" : L"DISABLED");
}

SINT32 oglCreateRawTexture(SINT32 texid, SINT32 miplevel, SINT32 width, SINT32 height, SINT32 bpp, SINT32 alphabits, SINT32 flags)
{
LOGICAL		newtex;
SINT32		pwidth, pheight;

	if (width > gMaxTextureWidth ||
		height > gMaxTextureHeight)
	{
		Write_Debug("The requested texture dimensions are not supported");
		return(-1);
	}

	if (texid >= 0)
	{
		// must have first level mipmap
		if (!oglIsTexture(texid, 0))
		{
			return(-1);
		}

		newtex = FALSE;
	}
	else
	{
		texid = _GetNewTextureId();
		if (texid < 0)
		{
			return(-1);
		}

		newtex = TRUE;
	}

	for (pwidth=texminwidth; pwidth < width && pwidth < gMaxTextureWidth; pwidth *= 2) /* Empty */;
	for (pheight=texminheight; pheight < height && pheight < gMaxTextureHeight; pheight *= 2) /* Empty */;

	if (alphabits != 0)
	{
		textures[texid].comp[miplevel] = 4;

		if (flags & CRT_FLAG_WRITABLE)
		{
			textures[texid].format[miplevel] = GL_RGBA;
		}
		else
		if (alphabits == 1 && gSupportDXT1A)
		{
			textures[texid].format[miplevel] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		}
		else
		if (gSupportDXT3)
		{
			textures[texid].format[miplevel] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		}
		else
		if (gSupportDXT5)
		{
			textures[texid].format[miplevel] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else
		{
			textures[texid].format[miplevel] = GL_RGBA;
		}
	}
	else
	{
		textures[texid].comp[miplevel] = 3;

		if (flags & CRT_FLAG_WRITABLE)
		{
			textures[texid].format[miplevel] = GL_RGB;
		}
		else
		if (gSupportDXT1)
		{
			textures[texid].format[miplevel] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
		else
		if (gSupportDXT3)
		{
			textures[texid].format[miplevel] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		}
		else
		if (gSupportDXT5)
		{
			textures[texid].format[miplevel] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else
		{
			textures[texid].format[miplevel] = GL_RGB;
		}
	}

	if (flags & CRT_FLAG_HARDWARE)
	{
		Write_Debug("CRT_FLAG_HARDWARE not supported in OpenGL version");
	}
   else
    {
		textures[texid].hasalpha = alphabits;
		textures[texid].iswritable = flags & CRT_FLAG_WRITABLE;
		textures[texid].timeaccessed = frametime;
		textures[texid].numuses = 1;
		if ((flags & CRT_FLAG_MIPMAP) && width != height)
		{
			Write_Debug("Texture is not square!");
		}

		do
		{
			textures[texid].format[miplevel] = textures[texid].format[0];
			textures[texid].comp[miplevel] = textures[texid].comp[0];
			textures[texid].lpsurf[miplevel] = memAlloc(pwidth * pheight * textures[texid].comp[0]);
			textures[texid].hdevtex[miplevel] = 0;
			textures[texid].imagewidth[miplevel] = width;
			textures[texid].imageheight[miplevel] = height;
			textures[texid].imagesize[miplevel] = width * height * textures[texid].comp[0];
			textures[texid].po2width[miplevel] = pwidth;
			textures[texid].po2height[miplevel] = pheight;
			textures[texid].widthfactor[miplevel] = (FLOAT32)width / (FLOAT32)pwidth;
			textures[texid].heightfactor[miplevel] = (FLOAT32)height / (FLOAT32)pheight;
			if (textures[texid].miplevels <= miplevel)
			{
				textures[texid].miplevels = miplevel + 1;
			}

			if (width == MinTextureDim &&
				height == MinTextureDim)
			{
				break;
			}

			if (width >= 2) width /= 2;
			if (height >= 2) height /= 2;

			if (pwidth >= 2) pwidth /= 2;
			if (pheight >= 2) pheight /= 2;

			miplevel += 1;
		}
		while (flags & CRT_FLAG_MIPMAP);
    }

	return(texid);
}

LOGICAL oglLoadTexture(SINT32 texid, SINT32 miplevel)
{
	if (!oglIsTexture(texid, miplevel))
	{
		return(FALSE);
	}

	// Signal SetActiveTexture to load the texture to video memory
	textures[texid].hdevtex[miplevel] = 0;

	textures[texid].timeaccessed = frametime;

	return(TRUE);
}

LOGICAL oglGetTextureFactors(SINT32 texid, SINT32 miplevel, FLOAT32* widthfactor, FLOAT32* heightfactor)
{
	if (!oglIsTexture(texid, miplevel))
	{
		return(FALSE);
	}

	*widthfactor = textures[texid].widthfactor[miplevel];
	*heightfactor = textures[texid].heightfactor[miplevel];

	return(TRUE);
}

/*
 * Materials
*/
LOGICAL oglIsMaterial(SINT32 matid)
{
	if (matid < 0 || matid >= nummaterials || materials == NULL ||
		materials[matid].numuses == 0)
	{
		return(FALSE);
	}

	return(TRUE);
}

void oglAddMaterialName(const char* name, int id)
{
    materialnames[name] = id;
    reverse_materialnames[id]=name;
}

SINT32 oglFindMaterialName(const char* name)
{
    std::map<std::string, int>::iterator itr;

    itr = materialnames.find(name);

    if(itr != materialnames.end())
    {
        return itr->second;
    }

    return -1;
}

LOGICAL oglAddMatRef(SINT32 texid)
{
TEXTURE*	lptex;

	if (!oglIsTexture(texid, 0))
	{
		return(FALSE);
	}

	lptex = oglGetTextureData(texid);
	lptex->numuses += 1;
	return(TRUE);
}

SINT32 oglGetMaterialTexId(SINT32 matid)
{
	if (!oglIsMaterial(matid))
	{
		return(-1);
	}

	return(materials[matid].texid);
}

LOGICAL oglGetMaterialTexName(SINT32 matid, char* texname)
{
	if (!oglIsMaterial(matid))
	{
    texname[0] = 0;
		return(FALSE);
	}

	strcpy(texname, materials[matid].texname);

	return(TRUE);
}

LOGICAL oglSetMaterialName(SINT32 matid, const char* name)
{
	if (!oglIsMaterial(matid))
	{
		return(FALSE);
	}

#ifdef DONT_USE
	materials[matid].name = memAlloc(strlen(name) + 1);
#endif
	strcpy(materials[matid].name, name);

	return(TRUE);
}

const piMATERIAL& oglGetMaterial(int id)
{
    if(!oglIsMaterial(id))
    {
        static piMATERIAL error;
        return error;
    }

    return materials[id].material;
}

SINT32 oglGetMaterials(MATERIAL** lpmat)
{
	*lpmat = materials;
	return(nummaterials);
}

void oglFreeMaterial(SINT32 matid)
{
	if (oglIsMaterial(matid))
	{
		if (materials[matid].numuses != 0)
		{
			materials[matid].numuses -= 1;

            if (materials[matid].numuses == 0)
			{
                if(glIsList(materials[matid].displayList))
                {
                    glDeleteLists(materials[matid].displayList, 1);
                }
			}
		}
	}
}

#include "math/SuperFastHash.h"
SINT32 oglCreateMaterial(LPVOID lpmat)
{
    SINT32 matid;

	matid = _GetNewMaterialId();
	if (matid < 0)
	{
		return(-1);
	}

	Write_Debug("CreateMaterial: id=%d", matid);

	materials[matid].matid = matid;
	materials[matid].numuses += 1;

	if (lpmat)
	{
		piMATERIAL* material = (piMATERIAL*)lpmat;
		materials[matid].material.diffuse 	= material->diffuse;
		materials[matid].material.ambient	= material->ambient;
		materials[matid].material.specular	= material->specular;
		materials[matid].material.emissive	= material->emissive;
		materials[matid].material.power		= material->power;
	}

	return(matid);
}

LOGICAL oglSetActiveMaterial(SINT32 matid)
{
#if USE_STATESET
    _activeMaterial = matid;
#else
    if(g_materiallocked)
    {
        return TRUE;
    }

    if(_activeMaterial == matid)
    {
        return TRUE;
    }

    if (!oglIsMaterial(matid))
    {
	    return FALSE;
    }

    _activeMaterial = matid;

    if(ogl::UseDeprecatedPath())
    {
#if USE_STATESET
        if(materials[matid].dirty)
        {
            if(materials[matid].displayList > 0)
            {
                glDeleteLists(materials[matid].displayList, 1);
                materials[matid].displayList = 0;
            }

            materials[matid].dirty = false;
        }

        if(materials[matid].displayList)
        {
            glCallList(materials[matid].displayList);
            return 1;
        }

        materials[matid].displayList = glGenLists(1);

        glNewList(materials[matid].displayList, GL_COMPILE);

        glMaterialfv(GL_FRONT, GL_AMBIENT, (GLfloat*)&materials[matid].material.ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat*)&materials[matid].material.diffuse);
        glMaterialfv(GL_FRONT, GL_EMISSION, (GLfloat*)&materials[matid].material.emissive);
        glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat*)&materials[matid].material.specular);
        glMaterialf(GL_FRONT, GL_SHININESS, materials[matid].material.power);

        glEndList();
#else
        glMaterialfv(GL_FRONT, GL_AMBIENT, (GLfloat*)&materials[matid].material.ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat*)&materials[matid].material.diffuse);
        glMaterialfv(GL_FRONT, GL_EMISSION, (GLfloat*)&materials[matid].material.emissive);
        glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat*)&materials[matid].material.specular);
        glMaterialf(GL_FRONT, GL_SHININESS, materials[matid].material.power);
#endif
    }
#endif

	return TRUE;
}

LOGICAL oglAddMaterialReference(SINT32 matid)
{
	if (!oglIsMaterial(matid))
	{
		return FALSE;
	}

	materials[matid].numuses += 1;

	return TRUE;
}

LOGICAL oglDisableMaterial(void)
{
    extern SINT32 DefaultMaterialId;

    oglSetActiveMaterial(DefaultMaterialId);

    return TRUE;
}

/*
 * Matrices
*/
piMATRIX* oglGetProjectionMatrix(void)
{
	return(&projection);
}

void oglSetOrthoProjectionMatrix(float r, float l, float t, float b, float znear, float zfar)
{
    Matrixf proj;

#if REVERSE_NEAR_FAR_PLANE
    float zmax = znear;
    float zmin = zfar;
#else
    float zmax = zfar;
    float zmin = znear;
#endif

    // Left handed coordinate system
    proj(0, 0) = 2.0f / (r - l);
    proj(1, 1) = 2.0f / (t - b);
    proj(2, 2) = 1.0f / (zmax - zmin);

    // Translate center to origin
    proj(3, 0) = (l + r) / (l - r);
    proj(3, 1) = (t + b) / (b - t);
    proj(3, 2) = znear / (zmin - zmax);

    BackClip = zfar;
    FrontClip = znear;

    oglSetProjectionMatrix((piMATRIX*)proj.ptr());
}

void oglSetProjectionMatrix(const piMATRIX* newproj)
{
    if(!_lockProjectionMatrix)
    {
        if(newproj->_11 != projection._11 ||
           newproj->_12 != projection._12 ||
           newproj->_13 != projection._13 ||
           newproj->_14 != projection._14 ||
           newproj->_21 != projection._21 ||
           newproj->_22 != projection._22 ||
           newproj->_23 != projection._23 ||
           newproj->_24 != projection._24 ||
           newproj->_31 != projection._31 ||
           newproj->_32 != projection._32 ||
           newproj->_33 != projection._33 ||
           newproj->_34 != projection._34 ||
           newproj->_41 != projection._41 ||
           newproj->_42 != projection._42 ||
           newproj->_43 != projection._43 ||
           newproj->_44 != projection._44)
        {
            if(ogl::UseDeprecatedPath())
            {
                projection = *newproj;

                GLenum mode = oglGetMatrixMode();
                oglSetMatrixMode(GL_PROJECTION);

                glLoadMatrixf((GLfloat*)&projection);

                if(mode != 0)
                {
                    oglSetMatrixMode(mode);
                }
            }
            else
            {
                projection = *newproj;
                glslSetProjectionMatrix();
            }
        }
    }
}

void oglSetProjectionMatrix(const Matrixf& newproj)
{
    if(!_lockProjectionMatrix)
    {
        if(newproj(0, 0) != projection._11 ||
           newproj(0, 1) != projection._12 ||
           newproj(0, 2) != projection._13 ||
           newproj(0, 3) != projection._14 ||
           newproj(1, 0) != projection._21 ||
           newproj(1, 1) != projection._22 ||
           newproj(1, 2) != projection._23 ||
           newproj(1, 3) != projection._24 ||
           newproj(2, 0) != projection._31 ||
           newproj(2, 1) != projection._32 ||
           newproj(2, 2) != projection._33 ||
           newproj(2, 3) != projection._34 ||
           newproj(3, 0) != projection._41 ||
           newproj(3, 1) != projection._42 ||
           newproj(3, 2) != projection._43 ||
           newproj(3, 3) != projection._44)
        {
            if(ogl::UseDeprecatedPath())
            {
                newproj.get(projection);

                GLenum mode = oglGetMatrixMode();
                oglSetMatrixMode(GL_PROJECTION);

                glLoadMatrixf((GLfloat*)&projection);

                if(mode != 0)
                {
                    oglSetMatrixMode(mode);
                }
            }
            else
            {
                newproj.get(projection);
                glslSetProjectionMatrix();
            }
        }
    }
}

const piMATRIX& oglGetModelViewMatrix()
{
    return modelview;
}

void oglGetModelViewProjectionMatrix(Matrixf& mvp)
{
    mvp = Matrixf(modelview) * Matrixf(projection);
}

piMATRIX* oglGetWorldMatrix(void)
{
	return(&world);
}

const Matrixf& oglGetWaterReflectionCamera()
{
    return reflectionCamera;
}

void oglSetWaterReflectionCamera(float height)
{
    static Matrixf cam;

    cam.identity();

    cam(2, 2) = -1;
    cam(3, 2) = 2.0f * height;

    reflectionCamera = cam * Matrixf(view);
}

void oglSetWorldMatrix(const Matrixf& newworld)
{
    if(!_lockWorldMatrix)
    {
        if(newworld(0, 0) != world._11 ||
           newworld(0, 1) != world._12 ||
           newworld(0, 2) != world._13 ||
           newworld(0, 3) != world._14 ||
           newworld(1, 0) != world._21 ||
           newworld(1, 1) != world._22 ||
           newworld(1, 2) != world._23 ||
           newworld(1, 3) != world._24 ||
           newworld(2, 0) != world._31 ||
           newworld(2, 1) != world._32 ||
           newworld(2, 2) != world._33 ||
           newworld(2, 3) != world._34 ||
           newworld(3, 0) != world._41 ||
           newworld(3, 1) != world._42 ||
           newworld(3, 2) != world._43 ||
           newworld(3, 3) != world._44)
        {
            if(ogl::UseDeprecatedPath())
            {
                math3dMatrixMultiply(&modelview, (piMATRIX*)newworld.ptr(), &view);
                newworld.get(world);
                glLoadMatrixf((GLfloat*)&modelview);
            }
            else
            {
                math3dMatrixMultiply(&modelview, (piMATRIX*)newworld.ptr(), &view);
                newworld.get(world);
                glslSetModelViewMatrix();
            }
        }
    }
}

void oglSetWorldMatrix(const piMATRIX* newworld)
{
    if(!_lockWorldMatrix)
    {
        if(newworld->_11 != world._11 ||
           newworld->_12 != world._12 ||
           newworld->_13 != world._13 ||
           newworld->_14 != world._14 ||
           newworld->_21 != world._21 ||
           newworld->_22 != world._22 ||
           newworld->_23 != world._23 ||
           newworld->_24 != world._24 ||
           newworld->_31 != world._31 ||
           newworld->_32 != world._32 ||
           newworld->_33 != world._33 ||
           newworld->_34 != world._34 ||
           newworld->_41 != world._41 ||
           newworld->_42 != world._42 ||
           newworld->_43 != world._43 ||
           newworld->_44 != world._44)
        {
            if(ogl::UseDeprecatedPath())
            {
                math3dMatrixMultiply(&modelview, newworld, &view);
                world = *newworld;
                glLoadMatrixf((GLfloat*)&modelview);
            }
            else
            {
                math3dMatrixMultiply(&modelview, newworld, &view);
                world = *newworld;
                glslSetModelViewMatrix();
            }
        }
    }
}

piMATRIX* oglGetViewMatrix(void)
{
	return(&view);
}

void oglSetViewMatrix(const Matrixf& newview)
{
    if(!_lockViewMatrix)
    {
        if(newview(0, 0) != view._11 ||
           newview(0, 1) != view._12 ||
           newview(0, 2) != view._13 ||
           newview(0, 3) != view._14 ||
           newview(1, 0) != view._21 ||
           newview(1, 1) != view._22 ||
           newview(1, 2) != view._23 ||
           newview(1, 3) != view._24 ||
           newview(2, 0) != view._31 ||
           newview(2, 1) != view._32 ||
           newview(2, 2) != view._33 ||
           newview(2, 3) != view._34 ||
           newview(3, 0) != view._41 ||
           newview(3, 1) != view._42 ||
           newview(3, 2) != view._43 ||
           newview(3, 3) != view._44)
        {
            if(ogl::UseDeprecatedPath())
            {
                math3dMatrixMultiply(&modelview, &world, (piMATRIX*)newview.ptr());
                newview.get(view);
                glLoadMatrixf((GLfloat*)&modelview);
            }
            else
            {
                math3dMatrixMultiply(&modelview, &world, (piMATRIX*)newview.ptr());
                newview.get(view);
                glslSetModelViewMatrix();
            }
        }
    }
}

void oglSetViewMatrix(const piMATRIX* newview)
{
    if(!_lockViewMatrix)
    {
        if(newview->_11 != view._11 ||
           newview->_12 != view._12 ||
           newview->_13 != view._13 ||
           newview->_14 != view._14 ||
           newview->_21 != view._21 ||
           newview->_22 != view._22 ||
           newview->_23 != view._23 ||
           newview->_24 != view._24 ||
           newview->_31 != view._31 ||
           newview->_32 != view._32 ||
           newview->_33 != view._33 ||
           newview->_34 != view._34 ||
           newview->_41 != view._41 ||
           newview->_42 != view._42 ||
           newview->_43 != view._43 ||
           newview->_44 != view._44)
        {
            if(ogl::UseDeprecatedPath())
            {
                math3dMatrixMultiply(&modelview, &world, newview);
                view = *newview;
                glLoadMatrixf((GLfloat*)&modelview);
            }
            else
            {
                math3dMatrixMultiply(&modelview, &world, newview);
                view = *newview;
                glslSetModelViewMatrix();
            }
        }
    }
}

static float	maxHorizFOV = (float)140.0f * PI / 180.0f;

LOGICAL oglSetFieldOfView(FLOAT32 fov, FLOAT32 zmin, FLOAT32 zmax)
{
	float	baseFOV = (float)fov * PI / 180.0f;
	float	fourToThree = 4.0f / 3.0f;						// That's what the vertical FOV was based on until now, so we still have to use it
	float	horizFOV;
	float	vertFOV;
	float	linearFactor;									// The FOV angle is not a linear representation of the aspectRatio
	float	aspectRatio = (FLOAT32)ddpGetWidth() / (FLOAT32)ddpGetHeight();

	vertFOV = 2.0f * atan(tan(baseFOV/2.0f) / fourToThree);
	horizFOV = 2.0f * atan(aspectRatio * tan(vertFOV/2.0f));

	if(horizFOV > maxHorizFOV)
	{
		horizFOV = maxHorizFOV;
		vertFOV = 2.0f * atan(tan(horizFOV/2.0f) / aspectRatio);		//recalculate the vertical FOV so it matches maxHorizFOV
	}

	linearFactor = tan(baseFOV/2) / tan(horizFOV/2);
	zmin = zmin * linearFactor;

    piMATRIX proj = IdentityMatrix;

#if REVERSE_NEAR_FAR_PLANE
    float zmax2 = zmin;
    float zmin2 = FOV_Z_FAR;
#else
    float zmax2 = FOV_Z_FAR;
    float zmin2 = zmin;
#endif

	proj._11 = 1.0f / tan(horizFOV/2.0f);
	proj._22 = 1.0f / tan(vertFOV/2.0f);
	proj._33 = (zmax2 + zmin2) / (zmax2 - zmin2);
	proj._34 = 1;
	proj._43 = -2 * zmax2 * zmin2 / (zmax2 - zmin2);
	proj._44 = 0.0;

	oglSetProjectionMatrix(&proj);

	FieldOfView = fov;
	HorizontalFieldOfView = horizFOV;
	FrontClip = zmin;
	BackClip = zmax;

    rnvisSetupClipPlanes();

	return(TRUE);
}

FLOAT32 oglGetFieldOfViewZMin()
{
    return FrontClip;
}

FLOAT32 oglGetFieldOfViewZMax()
{
    return FOV_Z_FAR;
}

/*------------------------------------------------------------------------------------------
Mouse click world position.
Returns x, y, z relative world position.
-------------------------------------------------------------------------------------------*/
bool oglGetScreenToWorld(int sx, int sy, double& x, double& y, double& z)
{
#define MY_GLU_UN_PROJECT 1

#if MY_GLU_UN_PROJECT
    // Create ModelViewProjection matrix
    Matrixf mv = Matrixf(oglGetModelViewMatrix());
    Matrixf proj = Matrixf(*oglGetProjectionMatrix());
    Matrixf mvp = mv * proj;

    // Map mouse position to [-1, 1] normalized coordinates
    Vec3f v;
    v.x() = 2.0f * (sx) / WantedWidth - 1.0f;

    // OpenGL 0,0 is bottom left...windows it top left, so reverse (WantedHeight - sy)
    v.y() = 2.0f * (WantedHeight - sy) / WantedHeight - 1.0f;

    // Get z value at this click position
    float depth;
	glReadPixels(sx, int(v.y()), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    // Same as above..[-1, 1]
    v.z() = 2.0f * depth - 1.0f;

    // Put into world
    mvp.invert(mvp);
    v = v * mvp;

    x = v.x();
    y = v.y();
    z = v.z();
#else
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;

    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = WantedWidth;
    viewport[3] = WantedHeight;

    const piMATRIX& m1 = oglGetModelViewMatrix();

    modelview[0] = m1._11;
    modelview[1] = m1._12;
    modelview[2] = m1._13;
    modelview[3] = m1._14;
    modelview[4] = m1._21;
    modelview[5] = m1._22;
    modelview[6] = m1._23;
    modelview[7] = m1._24;
    modelview[8] = m1._31;
    modelview[9] = m1._32;
    modelview[10] = m1._33;
    modelview[11] = m1._34;
    modelview[12] = m1._41;
    modelview[13] = m1._42;
    modelview[14] = m1._43;
    modelview[15] = m1._44;

    const piMATRIX& m2 = *oglGetProjectionMatrix();

    projection[0] = m2._11;
    projection[1] = m2._12;
    projection[2] = m2._13;
    projection[3] = m2._14;
    projection[4] = m2._21;
    projection[5] = m2._22;
    projection[6] = m2._23;
    projection[7] = m2._24;
    projection[8] = m2._31;
    projection[9] = m2._32;
    projection[10] = m2._33;
    projection[11] = m2._34;
    projection[12] = m2._41;
    projection[13] = m2._42;
    projection[14] = m2._43;
    projection[15] = m2._44;

	winX = (float)sx;
	winY = (float)WantedHeight - (float)sy;

    // z depth at this click.
	glReadPixels(sx, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
    // Get world pos
	gluUnProject(winX, winY, winZ, modelview, projection, viewport, &x, &y, &z);
#endif

    return true;
}

static inline void piMATRIXtoDoubleArray(double a[], const piMATRIX b)
{
    a[0] = b._11;
    a[1] = b._12;
    a[2] = b._13;
    a[3] = b._14;

    a[4] = b._21;
    a[5] = b._22;
    a[6] = b._23;
    a[7] = b._24;

    a[8] = b._31;
    a[9] = b._32;
    a[10] = b._33;
    a[11] = b._34;

    a[12] = b._41;
    a[13] = b._42;
    a[14] = b._43;
    a[15] = b._44;
}

LOGICAL oglGetWorldToScreen(double x, double y, double z, double& sx, double& sy, double& sz)
{
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

#if DONT_USE_GLGET_FOR_ANYTHING
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);
#else
    piMATRIXtoDoubleArray(modelMatrix, modelview);
    piMATRIXtoDoubleArray(projMatrix, projection);

    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = WantedWidth;
    viewport[3] = WantedHeight;
#endif

	if(gluProject(x, y, z, modelMatrix, projMatrix, viewport, &sx, &sy, &sz))
		return true;

	return false;
}

LOGICAL oglGetViewportDimensions(SINT32* x, SINT32* y, SINT32* width, SINT32* height, FLOAT32* znear, FLOAT32* zfar)
{
	*x = vx;
	*y = vy;
	*width = vwidth;
	*height = vheight;
	*znear = znearsave;
	*zfar = zfarsave;

	return(TRUE);
}

void oglLockViewport(bool lock)
{
    _lockViewport = lock;
}

void oglSetViewport(int left, int right, int width, int height)
{
    if(!_lockViewport)
    {
        glViewport(left, right, width, height);
    }
}

LOGICAL oglSetViewportDimensions(SINT32 x, SINT32 y, SINT32 width, SINT32 height, FLOAT32 znear, FLOAT32 zfar)
{
FLOAT32		xf;
GLenum		mode;

	mode = oglGetMatrixMode();
	oglSetMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	xf = (FLOAT32)width / (FLOAT32)height;

	vx = x;
	vy = y;
	vwidth = width;
	vheight = height;

	oglSetViewport(x, y, width, height);

	oglSetViewportZ(znear, zfar);

	glLoadMatrixf((GLfloat*)&projection);
	oglSetMatrixMode(mode);

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglSetViewportDimensions");
	}

	return(TRUE);
}


//
// znear, zfar are clamped into (0.0,1.0)
//

LOGICAL oglGetViewportZ(FLOAT32* znear, FLOAT32* zfar)
{
	*znear = znearsave;
	*zfar = zfarsave;

    return(TRUE);
}

//
// In cockpit recalculation
// because we want to go near-sighted for instruments
//

LOGICAL oglSetViewportZ(FLOAT32 znear, FLOAT32 zfar)
{
	znearsave = znear;
	zfarsave = zfar;

   	glDepthRange(znear, zfar);

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglSetViewportZ");
	}

    return(TRUE);
}

// Parse the extensions list returned from OpenGL
static LOGICAL ParseExtensions(char* tp)
{
	return(TRUE);
}

static LOGICAL _internalOglInit(void)
{
	SINT8*		tp, *ptr;
	SINT32		i, j;
SINT32		numcomps, *comps;

    glGetIntegerv(GL_MATRIX_MODE, (GLint*)&crtMode);

#if platOS == OS_MACOS
	//g_postRenderFilter = FALSE;
#endif
	for(i = 0; i < MAXRENDERSTATES; i++)
    {
       _GetOpenGLState((oglRENDERSTATETYPES)i);
    }

    for(i = 0; i < MAXTEXTURESTAGES; i++)
    {
        for(j = 0; j < MAXTEXTURESTATES; j++)
        {
            stagestates[i][j] = 0xffffffff;
        }
    }

    memset(lightstates, 0, sizeof(UINT32) * MAXLIGHTSTATES);

	numtextures = MINTEXTURES;
	textures = (TEXTURE*)memAlloc(sizeof(TEXTURE) * numtextures);
	if ( textures == NULL )
		throw "Out of memory for textures";

    for(i = 0; i < MINTEXTURES; i++)
    {
        textures[i].id = i + 1;
        g_freeTextures.push_back(&textures[i]);
    }

	numtexture3Ds = MIN3DTEXTURES;
	texture3Ds = (TEXTURE3D*)memAlloc(sizeof(TEXTURE3D) * numtexture3Ds);
	if ( texture3Ds == NULL )
		throw "Out of memory for texture3Ds";

	nummaterials = MINMATERIALS;
	materials = (MATERIAL*)memAlloc(sizeof(MATERIAL) * nummaterials);
	if ( materials == NULL )
		throw "Out of memory for materials";

	// Extract data from OpenGL
	tp = (SINT8*)glGetString(GL_VENDOR);
	if (tp != NULL)
	{
		Write_Debug("Vendor:    %s", tp);
	}

	tp = (SINT8*)glGetString(GL_VERSION);
	static char OGL_Requirements[128];
	sprintf(OGL_Requirements, "OpenGL drivers %u.%u or higher are required", OPENGL_MAJOR_VERSION, OPENGL_MINOR_VERSION);
	if (tp != NULL)
	{
		Write_Debug("Version:   %s", tp);
		gOpenGLMajorVersion = atoi(tp);
		gOpenGLMinorVersion = 0;
		tp = strchr(tp, '.');
		if (tp != NULL)
		{
			tp += 1;
			gOpenGLMinorVersion = atoi(tp);
		}

		Write_Debug("OpenGL version: %d.%d", gOpenGLMajorVersion, gOpenGLMinorVersion);

		if (!(gOpenGLMajorVersion > OPENGL_MAJOR_VERSION || (gOpenGLMajorVersion == OPENGL_MAJOR_VERSION && gOpenGLMinorVersion >= OPENGL_MINOR_VERSION)))
		{
			throw OGL_Requirements;
		}
	}
	else
	{
		throw OGL_Requirements;
	}

	tp = (SINT8*)glGetString(GL_RENDERER);

    if (tp != NULL)
	{
		Write_Debug("Renderer:  %s", tp);

#if platOS != OS_MACOS
        if(strstr(tp, "GDI Generic"))
        {
	        static const TCHAR* _ERR_TITLE = _T("Video initialization error");
	        std::wstring message = UTF16("OpenGL drivers may need to be updated. Click ok to continue");
	        MessageBox(NULL, message.c_str(), _ERR_TITLE, MB_ICONEXCLAMATION|MB_OK);
        }
#endif
	}

	tp = (SINT8*)glGetString(GL_EXTENSIONS);

    if (tp != NULL)
	{
		if (!ParseExtensions(tp))
		{
			throw "OpenGL drivers are incompatible";
		}
	}

	glGetIntegerv(GL_MAX_LIGHTS, (GLint*)&gMaxLights);
	lights = (LIGHT*)memAlloc(sizeof(LIGHT) * gMaxLights);
	if ( lights == NULL )
		throw "Out of memory for lights";

	numlights = 0;

	texminwidth = 1;
	texminheight = 1;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&gMaxTextureWidth);
	gMaxTextureHeight = gMaxTextureWidth;

	MaxTextureWidth = gMaxTextureWidth;
	MaxTextureHeight = gMaxTextureHeight;

	gMipmapEnable = bMipmapEnable;

    // Do not set these to IDENTITY.
    memset(&projection, 0, sizeof(piMATRIX));
    memset(&world, 0, sizeof(piMATRIX));
    memset(&view, 0, sizeof(piMATRIX));

    // Do not set these to IDENTITY.
    for(int i = 0; i < 8; i++)
    {
        memset(&texmat[i], 0, sizeof(piMATRIX));
    }

	// Some generic setup
	if (!oglSetupRC())
	{
		throw "Unable to prepare render context";
	}

	gSupportedSrcBlendModes = oglSRCBLENDCAPS_ALL;

	if (GLEW_ARB_imaging)
	{
		gSupportedSrcBlendModes |= oglSRCBLENDCAPS_IMAGING_ALL;
	}

	SupportedSrcBlendModes = gSupportedSrcBlendModes;

	gSupportedDstBlendModes = oglDSTBLENDCAPS_ALL;

	if (GLEW_ARB_imaging)
	{
		gSupportedDstBlendModes |= oglDSTBLENDCAPS_IMAGING_ALL;
	}

	SupportedDestBlendModes = gSupportedDstBlendModes;

	FrontClip = 1.0f;
	BackClip = 1000000.0f;

	// This causes a second, post texture lighting pass just for specular (looks better)
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, false);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, true);

	gSupportDXT1 = FALSE;
	gSupportDXT1A = FALSE;
	gSupportDXT3 = FALSE;
	gSupportDXT5 = FALSE;

	if (GLEW_ARB_texture_compression && GLEW_EXT_texture_compression_s3tc)
	{
		glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, (GLint*)&numcomps);
		Write_Debug("Number of compressed texture formats supported: %d", numcomps);
		if (numcomps > 0)
		{
			comps = (SINT32*)memAlloc(numcomps * sizeof(SINT32));
			if (comps == NULL)
				throw "Out of memory for compressed textures";
			glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS_ARB, (GLint*)comps);
			for (i=0; i < numcomps; i++)
			{
				switch (comps[i])
				{
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	// 0x83F0
					ptr = "DXT1 - RGB ";
					gSupportDXT1 = TRUE;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	// 0x83F1
					ptr = "DXT1 - RGBA";
					gSupportDXT1A = TRUE;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	// 0x83F2
					ptr = "DXT3 - RGBA";
					gSupportDXT3 = TRUE;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	// 0x83F3
					ptr = "DXT5 - RGBA";
					gSupportDXT5 = TRUE;
					break;
				default:
					ptr = "Unknown!";
					break;
				}

				Write_Debug(" Format %d: %s (0x%08x)", i, ptr, comps[i]);
			}

			memFree(comps);
		}
	}

	SupportDXT1 = gSupportDXT1 && gSupportDXT1A;	// DXT1 Opaque / one-bit alpha
	SupportDXT2 = FALSE;							// DXT2 Explicit alpha
	SupportDXT3 = gSupportDXT3;						// DXT3 Explicit alpha
	SupportDXT4 = FALSE;							// DXT4 Interpolated alpha - Alpha-premultiplied
	SupportDXT5 = gSupportDXT5;						// DXT5 Interpolated alpha - Alpha-premultiplied

	glGetIntegerv(GL_MAX_CLIP_PLANES, (GLint*)&gMaxUserClipPlanes);	// 6 is the minimum required by the OpenGL spec
	MaxUserClipPlanes = gMaxUserClipPlanes;

	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, (GLint*)&gMaxTextureUnits);
    Write_Log(LOG_OLD, LOG_INFO, "[oglInit] Max Texture Units: %d", gMaxTextureUnits);

	sourceBlend = SrcBlend;
	destinationBlend = DestBlend;

	_AlphaFunc = piALPHAFUNC_DEFAULT;;
	_AlphaRef = piALPHAREF_DEFAULT;

	// Temporary texture formats
	DefTextureFormat = oglFMT_X8R8G8B8;
	DefTextureAlphaFormat = oglFMT_A8R8G8B8;

	oglResetRenderBuckets();
	oglResetRenderIndexBuckets();

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglInit");
	}

	textureMutex = systhreadMutexCreate();
 	materialMutex = systhreadMutexCreate();

    modelview = IdentityMatrix;

#if(0)
    if(!ogl::UseDeprecatedPath())
    {
        // UBO's take a painfully long time to compile/link, so using TBO's instead until
        // problem is fixed (Believe to be driver related)
        tbo = oglTextureObject::GetOrCreateTextureObject("instancedata", 32768, GL_RGBA32F_ARB);
    }
#endif

    Write_Debug("Completed oglInit");

    return TRUE;
}

// Stub function that will report an error during the setup of the video card
// and notify the user
static inline void _initializationError(const char* problem)
{
	Write_Log(LOG_RENDER, LOG_WARN, "Error during OpenGL Initialization: %s", problem);

#if platOS == OS_WIN32 || platOS == OS_WIN64
	static const TCHAR* _ERR_TITLE = _T("Video initialization error");
	static const TCHAR* _ERR_STR_1 = _T("The game was unable to initialize the video display: ");
	static const TCHAR* _ERR_STR_2 = _T("\nPlease check your video card configuration and try again.");

	std::wstring message = _ERR_STR_1;
	message += UTF16(problem);
	message += _ERR_STR_2;
	MessageBox(NULL, message.c_str(), _ERR_TITLE, MB_ICONEXCLAMATION|MB_OK);
#elif platOS == OS_MACOS
	MacVideoInitializationFailed(problem);
#endif
}

LOGICAL oglInit(void)
{
	try
	{
		return _internalOglInit();
	}
	catch(std::exception& exception) { _initializationError(exception.what()); }
	catch(const char* error) { _initializationError(error); }
	catch(...) { _initializationError("[unknown error]"); }

	return FALSE;
}

void oglKill(void)
{
SINT32		i;
SINT32		total, free;

	oglDisplayRenderBuckets();
	oglDisplayRenderIndexBuckets();

	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);

	// Lights
	for (i=0; i < numlights; i++)
	{
		oglReleaseLight(i);
	}

	numlights = 0;

	if (lights != NULL)
	{
		memFree(lights);
		lights = NULL;
	}

	// Materials
	for (i=0; i < nummaterials; i++)
	{
		oglFreeMaterial(i);
	}

	nummaterials = 0;

	if (materials != NULL)
	{
		memFree(materials);
		materials = NULL;
	}

	// Textures
	if (oglGetTextureMemory(&total, &free))
	{
		Write_Debug("Texture memory (before free): total = %d, free = %d, in use = %d", total, free, total - free);
	}

	for (i=0; i < numtexture3Ds; i++)
	{
		texture3Ds[i].numuses = 1;  // force to one use
		oglFreeMipTexture3Ds(i);
	}

    ldTextureKill();

	for (i=0; i < numtextures; i++)
	{
        if(textures[i].numuses != 0)
        {
	        textures[i].numuses = 1;  // force to one use
	        oglFreeMipTextures(i);
        }
	}

	if (oglGetTextureMemory(&total, &free))
	{
		Write_Debug("Texture memory (after free): total = %d, free = %d, in use = %d", total, free, total - free);
	}

	numtexture3Ds = 0;

	if (texture3Ds != NULL)
	{
		memFree(texture3Ds);
		texture3Ds = NULL;
	}

	numtextures = 0;

	if (textures != NULL)
	{
		memFree(textures);
		textures = NULL;
	}

	if(textureMutex)
		systhreadMutexKill(textureMutex);
	if(materialMutex)
		systhreadMutexKill(materialMutex);

    GLCounterKill();
}

LOGICAL oglClearPorts(piRECT piRect, UINT32 uTargets)
{
	glEnable(GL_SCISSOR_TEST);

	glScissor(piRect.left,
		      ddpGetHeight() - piRect.bottom,
			  piRect.right - piRect.left,
			  piRect.bottom - piRect.top);

	oglClearTargets(uTargets);

    glDisable(GL_SCISSOR_TEST);

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglClearPorts");
	}

	return(FALSE);
}

LOGICAL oglClearTargets(UINT32 targets)
{
GLbitfield	clrbits=0;

	if (targets & oglCLEAR_VIEWPORT)
	{
		clrbits |= GL_COLOR_BUFFER_BIT;
	}

	if (targets & oglCLEAR_ZBUFFER)
	{
		clrbits |= GL_DEPTH_BUFFER_BIT;
	}

	if (targets & oglCLEAR_STENCIL)
	{
		clrbits |= GL_STENCIL_BUFFER_BIT;
	}

	if (targets & oglCLEAR_ACCUM)
	{
		clrbits |= GL_ACCUM_BUFFER_BIT;
	}

	glClear(clrbits);

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglClearTargets");
	}

	return(TRUE);
}


SINT32 oglGetNumberVertices(void)
{
#ifdef USE_EXECUTE_BUFFERS
	return(curd3ddesc.dwMaxVertexCount);
#else
	return(65535);  // DrawPrimitive can handle a lot of vertices (see docs)
#endif
}

void oglPushRenderState(UINT32 s)
{
    savedrenderstates[s] = renderstates[s];
}

void oglPopRenderState(UINT32 s)
{
    oglSetRenderState((oglRENDERSTATETYPES)s, savedrenderstates[s]);
    savedrenderstates[s] = 0xFFFFFFFF;
}

UINT32 oglGetEngineRenderState(UINT32 state)
{
    return renderstates[state];
}

UINT32 oglGetEngineRenderStageState(UINT32 stage, UINT32 state)
{
    return stagestates[stage][state];
}

// Set default GL states
static void _GetOpenGLState(oglRENDERSTATETYPES state)
{
    switch(state)
    {
    case oglRENDERSTATE_COLORMASK:
        {
            GLboolean params[4];
            glGetBooleanv(GL_COLOR_WRITEMASK, params);

            UINT32 color = piRGBA_MAKE(params[0], params[1], params[2], params[3]);
            renderstates[state] = color;
            break;
        }
	case oglRENDERSTATE_WRAPS:
        {
            GLint param;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_WRAPT:
        {
            GLint param;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_ZENABLE:
        {
            renderstates[state] = glIsEnabled(GL_DEPTH_TEST);
		    break;
        }
	case oglRENDERSTATE_FILLMODE:
        {
            GLint param[2];
            glGetIntegerv(GL_POLYGON_MODE, param);
            renderstates[state] = param[0];
            break;
        }
	case oglRENDERSTATE_SHADEMODE:
        {
            GLint param;
            glGetIntegerv(GL_SHADE_MODEL, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_LINEPATTERN:
        {
            renderstates[state] = glIsEnabled(GL_LINE_STIPPLE);
            break;
        }
	case oglRENDERSTATE_ZWRITEENABLE:
        {
            GLboolean param;
            glGetBooleanv(GL_DEPTH_WRITEMASK, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_ALPHATESTENABLE:
        {
            renderstates[state] = glIsEnabled(GL_ALPHA_TEST);
            break;
        }
	case oglRENDERSTATE_TEXTUREMAG:
        {
            GLint param;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &param);
            renderstates[state] = param;
    		break;
        }
	case oglRENDERSTATE_TEXTUREMIN:
        {
            GLint param;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &param);
            renderstates[state] = param;
    		break;
        }
	case oglRENDERSTATE_SRCBLEND:
        {
            GLint param;
            glGetIntegerv(GL_BLEND_SRC, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_DESTBLEND:
        {
            GLint param;
            glGetIntegerv(GL_BLEND_DST, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_TEXTUREMAPBLEND:
        {
            GLint param;
            glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &param);
            renderstates[state] = param;
            break;
        }
    case oglRENDERSTATE_CULLMODE:
        {
            GLint param;
            glGetIntegerv(GL_FRONT_FACE, &param);
            renderstates[state] = param;
            break;
        }
    case oglRENDERSTATE_ZFUNC:
        {
            GLint param;
            glGetIntegerv(GL_DEPTH_TEST, &param);
            renderstates[state] = param;
            break;
        }
	case oglRENDERSTATE_ALPHAREF:
        {
            GLfloat param;
            glGetFloatv(GL_ALPHA_TEST_REF, &param);
            renderstates[state] = FloatToInt(param * 255.0f);
            break;
        }
	case oglRENDERSTATE_ALPHAFUNC:
        {
            GLint param;
            glGetIntegerv(GL_ALPHA_TEST_FUNC, &param);
            renderstates[state] = param;
            break;
        }
    case oglRENDERSTATE_DITHERENABLE:
        {
            renderstates[state] = glIsEnabled(GL_DITHER);
            break;
        }
    case oglRENDERSTATE_ALPHABLENDENABLE:
        {
            renderstates[state] = glIsEnabled(GL_BLEND);
            break;
        }
	case oglRENDERSTATE_FOGENABLE:
        {
            renderstates[state] = glIsEnabled(GL_FOG);
            break;
        }
	case oglRENDERSTATE_FOGMODE:
        {
            GLint param;
            glGetIntegerv(GL_FOG_MODE, &param);
            renderstates[state] = param;
            break;
        }
    case oglRENDERSTATE_STIPPLEENABLE:
        {
            renderstates[state] = glIsEnabled(GL_LINE_STIPPLE);
            break;
        }
	case oglRENDERSTATE_LIGHTING:
        {
            renderstates[state] = glIsEnabled(GL_LIGHTING);
            break;
        }
    default:
        {
            renderstates[state] = 0xffffffff;
        }
    }
}

#if(0)// USE_STATESET
#else
void oglSetRenderState(oglRENDERSTATETYPES enumStateType, UINT32 uState)
{
	if (enumStateType >= MAXRENDERSTATES)
        return;

    if(g_statelocked[enumStateType])
    {
        return;
    }

#if USE_STATESET
#else
    if (renderstates[enumStateType] == uState)
    {
        return;
    }
#endif

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error PRIOR to set render state %d to value %d", enumStateType, uState);
	}

	switch(enumStateType)
	{
    case oglRENDERSTATE_DRAWBUFFER:
        {
            glDrawBuffer(uState);
            break;
        }
    case oglRENDERSTATE_READBUFFER:
        {
            glReadBuffer(uState);
            break;
        }
    case oglRENDERSTATE_RASTERIZER_DISCARD:
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_RASTERIZER_DISCARD, uState);
#else
            // Turn on/off rasterization (no fragment shader)
            if(uState)
                glEnable(GL_RASTERIZER_DISCARD);
            else
                glDisable(GL_RASTERIZER_DISCARD);
#endif
            break;
        }
    case oglRENDERSTATE_COLORMASK:
        {
            // Use piRGBA_MAKE(r, g, b, a) with r,g,b,a either 1 or 0 to make this state

            GLboolean r = piRGBA_GETRED(uState) ? GL_TRUE : GL_FALSE;
            GLboolean g = piRGBA_GETGREEN(uState) ? GL_TRUE : GL_FALSE;
            GLboolean b = piRGBA_GETBLUE(uState) ? GL_TRUE : GL_FALSE;
            GLboolean a = piRGBA_GETALPHA(uState) ? GL_TRUE : GL_FALSE;

            glColorMask(r, g, b, a);

            break;
        }
    case oglRENDERSTATE_TEXTUREHANDLE:
        Write_Debug("Unimplemented oglRENDERSTATE_TEXTUREHANDLE %d", uState);
        break;

    case oglRENDERSTATE_ANTIALIAS:
        if (uState == TRUE)
		{
#if USE_STATESET
            States::ApplyRenderMode(GL_POLYGON_SMOOTH, 1);
#else
            glEnable(GL_POLYGON_SMOOTH);
#endif
            glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
		}
        else
		{
#if USE_STATESET
            States::ApplyRenderMode(GL_POLYGON_SMOOTH, 0);
#else
            glDisable(GL_POLYGON_SMOOTH);
#endif
		}

        break;
	case oglRENDERSTATE_TEXTUREPERSPECTIVE:
		if (uState == TRUE)
		{
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		}
		else
		{
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_DONT_CARE);
		}

		break;

	case oglRENDERSTATE_WRAPS:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, uState);
		break;

	case oglRENDERSTATE_WRAPT:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, uState);
		break;

	case oglRENDERSTATE_ZENABLE:
#if USE_STATESET
        States::ApplyRenderMode(GL_DEPTH_TEST, uState);
#else
		if (uState == TRUE)
		{
	 		glEnable(GL_DEPTH_TEST);
		}
		else
		{
	 		glDisable(GL_DEPTH_TEST);
		}
#endif
		break;

	case oglRENDERSTATE_FILLMODE:
		glPolygonMode(GL_FRONT, uState);
		break;

	case oglRENDERSTATE_SHADEMODE:
		glShadeModel(uState);
		break;

	case oglRENDERSTATE_LINEPATTERN:
		if (uState == TRUE)
		{
			glLineStipple(1, (GLushort)uState);
#if USE_STATESET
            States::ApplyRenderMode(GL_LINE_STIPPLE, 1);
#else
			glEnable(GL_LINE_STIPPLE);
#endif
		}
		else
		{
#if USE_STATESET
            States::ApplyRenderMode(GL_LINE_STIPPLE, 0);
#else
			glDisable(GL_LINE_STIPPLE);
#endif
		}

		break;

	case oglRENDERSTATE_MONOENABLE:
		break;

	case oglRENDERSTATE_ROP2:
		Write_Debug("Unimplemented oglRENDERSTATE_ROP2 %d", uState);
		break;

	case oglRENDERSTATE_PLANEMASK:
		Write_Debug("Unimplemented oglRENDERSTATE_PLANEMASK %d", uState);
		break;

	case oglRENDERSTATE_ZWRITEENABLE:
#if USE_STATESET
        States::HaveAppliedAttribute(RS_DEPTHMASK);
#endif
        glDepthMask((GLboolean)uState);
		break;

	case oglRENDERSTATE_ALPHATESTENABLE:
        {
            if(ogl::UseDeprecatedPath())
            {
#if USE_STATESET
                States::ApplyRenderMode(GL_ALPHA_TEST, uState);
#else
		        if(uState == TRUE)
			        glEnable(GL_ALPHA_TEST);
		        else
			        glDisable(GL_ALPHA_TEST);
#endif
            }
        }

		break;

	case oglRENDERSTATE_LASTPIXEL:
		break;

	case oglRENDERSTATE_TEXTUREMAG:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, uState);
		break;

	case oglRENDERSTATE_TEXTUREMIN:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, uState);
		break;

	case oglRENDERSTATE_SRCBLEND:
		if (uState == oglBLEND_INVSRCCOLOR)				// ATI cards don't do INVSRCCOLOR correctly,
			sourceBlend = oglBLEND_DESTCOLOR;				// so change it to DESTCOLOR (which is not
		else											// quite what we want, but it will do the
			sourceBlend = uState;							// job) see persmain.c for where it's used.
		glBlendFunc(sourceBlend, destinationBlend);
		break;


	case oglRENDERSTATE_DESTBLEND:
		destinationBlend = uState;
		glBlendFunc(sourceBlend, destinationBlend);
		break;

	case oglRENDERSTATE_TEXTUREMAPBLEND:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, uState);
		break;

    case oglRENDERSTATE_CULLMODE:
        {
            glFrontFace(uState);
    		break;
		}
    case oglRENDERSTATE_CULLFACE:
        {
#if USE_STATESET
            States::HaveAppliedAttribute(RS_CULLFACE);
#endif
            glCullFace(uState);
            break;
        }
    case oglRENDERSTATE_ZFUNC:
        {
#if USE_STATESET
            States::HaveAppliedAttribute(RS_DEPTHFUNC);
#endif
            glDepthFunc(uState);

            break;
        }

	case oglRENDERSTATE_ALPHAREF:
        {
#if USE_STATESET
            States::HaveAppliedAttribute(RS_ALPHAFUNC);
#endif
		    _AlphaRef = (FLOAT32)uState * colorONEOVER255;
		    glAlphaFunc(_AlphaFunc, _AlphaRef);

            break;
        }
	case oglRENDERSTATE_ALPHAFUNC:
        {
#if USE_STATESET
            States::HaveAppliedAttribute(RS_ALPHAFUNC);
#endif
		    _AlphaFunc = uState;
		    glAlphaFunc(_AlphaFunc, _AlphaRef);

		    break;
        }
    case oglRENDERSTATE_CULLFACEENABLE:
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_CULL_FACE, uState);
#else
            if(uState)
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);
#endif
            break;
        }
    case oglRENDERSTATE_DITHERENABLE:
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_DITHER, uState);
#else
            if (uState == TRUE)
		    {
	            glEnable(GL_DITHER);
		    }
            else
		    {
	            glDisable(GL_DITHER);
		    }
#endif
        }
        break;

    case oglRENDERSTATE_ALPHABLENDENABLE:
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_BLEND, uState);
#else
            if (uState == TRUE)
            {
                glEnable(GL_BLEND);
            }
            else
            {
                glDisable(GL_BLEND);
            }
#endif
        }
        break;

	case oglRENDERSTATE_BLENDOP:
		// Only if "GL_ARB_imaging" extension is supported
		if (GLEW_ARB_imaging)
		{
			glBlendEquation(uState);
		}

		break;

    case oglRENDERSTATE_SPECULARENABLE:
		bSpecular = uState;
        //Write_Debug("Unimplemented render state set: oglRENDERSTATE_SPECULARENABLE %d", uState);
        break;

    case oglRENDERSTATE_ZVISIBLE:
        Write_Debug("Unimplemented render state set: oglRENDERSTATE_ZVISIBLE %d", uState);
        break;

    case oglRENDERSTATE_SUBPIXEL:
        Write_Debug("Unimplemented render state set: oglRENDERSTATE_SUBPIXEL %d", uState);
        break;

    case oglRENDERSTATE_SUBPIXELX:
        Write_Debug("Unimplemented render state set: oglRENDERSTATE_SUBPIXELX %d", uState);
        break;

    case oglRENDERSTATE_STIPPLEDALPHA:
        Write_Debug("Unimplemented render state set: oglRENDERSTATE_STIPPLEDALPHA %d", uState);
        break;

	case oglRENDERSTATE_FOGENABLE:
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_FOG, uState);
#else
            if (uState == TRUE)
		    {
                glEnable(GL_FOG);
		    }
            else
		    {
                glDisable(GL_FOG);
		    }
#endif
        }
        break;

    case oglRENDERSTATE_FOGCOLOR:
    {
        GLfloat	Red = piRGBA_GETRED(uState) * colorONEOVER255;
        GLfloat	Green = piRGBA_GETGREEN(uState) * colorONEOVER255;
        GLfloat	Blue = piRGBA_GETBLUE(uState) * colorONEOVER255;
        GLfloat	Alpha = piRGBA_GETALPHA(uState) * colorONEOVER255;

#if USE_STATESET
        Fog* fog = dynamic_cast<Fog*>(States::GetDefaultAttribute(RS_FOG));
        fog->ApplyColor(Vec4f(Red, Green, Blue, Alpha));
#else
        GLfloat	Color[4];

        Color[0] = Red;
  		Color[1] = Green;
    	Color[2] = Blue;
     	Color[3] = Alpha;

        glFogfv(GL_FOG_COLOR, Color);
#endif
	    break;
    }
    case oglRENDERSTATE_CLEARCOLOR:
        {
            float r = piRGBA_GETRED(uState) * colorONEOVER255;
            float g = piRGBA_GETGREEN(uState) * colorONEOVER255;
            float b = piRGBA_GETBLUE(uState) * colorONEOVER255;
            float a = piRGBA_GETALPHA(uState) * colorONEOVER255;

            glClearColor(r, g, b, a);
            break;
        }
    case oglRENDERSTATE_COLOR4F:
        {
            float r = piRGBA_GETRED(uState) * colorONEOVER255;
            float g = piRGBA_GETGREEN(uState) * colorONEOVER255;
            float b = piRGBA_GETBLUE(uState) * colorONEOVER255;
            float a = piRGBA_GETALPHA(uState) * colorONEOVER255;

            glColor4f(r, g, b, a);
            break;
        }
	case oglRENDERSTATE_FOGMODE:
		if (uState == oglFOG_NONE)
		{
#if USE_STATESET
            States::ApplyRenderMode(GL_FOG, 0);
#else
            glDisable(GL_FOG);
#endif
		}
		else
		{
			if (bFogEnabled)
			{
#if USE_STATESET
                States::ApplyRenderMode(GL_FOG, 1);
#else
				glEnable(GL_FOG);
#endif
				glFogf(GL_FOG_MODE, (GLfloat)uState);
			}
		}

        break;

    case oglRENDERSTATE_FOGSTART:
	{
	    GLfloat fStart = oglLongLikeFloat(uState);
#if USE_STATESET
        Fog* fog = dynamic_cast<Fog*>(States::GetDefaultAttribute(RS_FOG));
        fog->ApplyStart(fStart);
#else
        glFogf(GL_FOG_START, fStart);
#endif
        break;
	}

    case oglRENDERSTATE_FOGEND:
	{
	    GLfloat fEnd = oglLongLikeFloat(uState);
#if USE_STATESET
        Fog* fog = dynamic_cast<Fog*>(States::GetDefaultAttribute(RS_FOG));
        fog->ApplyEnd(fEnd);
#else
        glFogf(GL_FOG_END, fEnd);
#endif
        break;
	}

    case oglRENDERSTATE_FOGDENSITY:
	{
        GLfloat density = 0.0f;
	    density = oglLongLikeFloat(uState);
#if USE_STATESET
        Fog* fog = dynamic_cast<Fog*>(States::GetDefaultAttribute(RS_FOG));
        fog->ApplyDensity(density);
#else
        glFogf(GL_FOG_DENSITY, density);
#endif
        break;
	}

    case oglRENDERSTATE_STIPPLEENABLE:
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_LINE_STIPPLE, uState);
#else
            if (uState == TRUE)
		    {
                glEnable(GL_LINE_STIPPLE);
		    }
            else
		    {
                glDisable(GL_LINE_STIPPLE);
		    }
#endif
        }

        break;

    case oglRENDERSTATE_EDGEANTIALIAS:
        if (uState == TRUE)
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_LINE_SMOOTH, 1);
            States::ApplyRenderMode(GL_POINT_SMOOTH, 1);
#else
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
#endif
            glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
            glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
        }
        else
        {
#if USE_STATESET
            States::ApplyRenderMode(GL_LINE_SMOOTH, 0);
            States::ApplyRenderMode(GL_POINT_SMOOTH, 0);
#else
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_POINT_SMOOTH);
#endif
        }

        break;

	case oglRENDERSTATE_COLORKEYENABLE:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_COLORKEYENABLE %d", uState);
		break;

	case oglRENDERSTATE_BORDERCOLOR:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_BORDERCOLOR %d", uState);
		break;

	case oglRENDERSTATE_TEXTUREADDRESSU:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_TEXTUREADDRESSU %d", uState);
		break;

	case oglRENDERSTATE_TEXTUREADDRESSV:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_TEXTUREADDRESSV %d", uState);
		break;

	case oglRENDERSTATE_MIPMAPLODBIAS:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_MIPMAPLODBIAS %d", uState);
		break;

	case oglRENDERSTATE_ZBIAS:
        //glPixelTransferi(GL_DEPTH_BIAS, (GLint)uState);
		break;

	case oglRENDERSTATE_ANISOTROPY:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_ANISOTROPY %d", uState);
		break;

	case oglRENDERSTATE_FLUSHBATCH:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_FLUSHBATCH %d", uState);
		break;

	case oglRENDERSTATE_TRANSLUCENTSORTINDEPENDENT:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_TRANSLUCENTSORTINDEPENDENT %d", uState);
		break;

	case oglRENDERSTATE_STENCILENABLE:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILENABLE %d", uState);
		break;

	case oglRENDERSTATE_STENCILFAIL:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILFAIL %d", uState);
		break;

	case oglRENDERSTATE_STENCILZFAIL:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILZFAIL %d", uState);
		break;

	case oglRENDERSTATE_STENCILPASS:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILPASS %d", uState);
		break;

	case oglRENDERSTATE_STENCILFUNC:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILFUNC %d", uState);
		break;

	case oglRENDERSTATE_STENCILREF:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILREF %d", uState);
		break;

	case oglRENDERSTATE_STENCILMASK:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILMASK %d", uState);
		break;

	case oglRENDERSTATE_STENCILWRITEMASK:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_STENCILWRITEMASK %d", uState);
		break;

	case oglRENDERSTATE_TEXTUREFACTOR:
		Write_Debug("Unimplemented render state set: oglRENDERSTATE_TEXTUREFACTOR %d", uState);
		break;

	case oglRENDERSTATE_LIGHTING:
#if USE_STATESET
        States::ApplyRenderMode(GL_LIGHTING, uState);
#else
        if (uState)
		{
			glEnable(GL_LIGHTING);
		}
		else
		{
            glDisable(GL_LIGHTING);
		}
#endif
		break;

	case oglRENDERSTATE_AMBIENT:
		{
            if(ogl::UseDeprecatedPath())
            {
		        FLOAT32 colors[4];

			    colors[0] = (FLOAT32)piRGBA_GETRED(uState) * colorONEOVER255;
			    colors[1] = (FLOAT32)piRGBA_GETGREEN(uState) * colorONEOVER255;
			    colors[2] = (FLOAT32)piRGBA_GETBLUE(uState) * colorONEOVER255;
			    colors[3] = 1.0f;

			    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat*)colors);
            }
		}
		break;

	default:
	//	Write_Debug("Unimplemented render state set: default %d, 0x%x", enumStateType, uState);
		break;
	};

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error during set render state %d to value %d", enumStateType, uState);
	}

    renderstates[enumStateType] = uState;
}
#endif

LOGICAL oglDisplayRenderStates(void)
{
SINT32		i;

	for (i=0; i < MAXRENDERSTATES; i++)
	{
		if (renderstates[i] != 0xffffffff)
		{
			Write_Debug("Render state %d = 0x%08x", i, renderstates[i]);
		}
	}

	return(TRUE);
}

void oglResetRenderStats(void)
{
	totdraw			= 0;
	totdrawindex	= 0;
	totprimitives	= 0;
	totpoints		= 0;
	totlines		= 0;
	tottriangles	= 0;
	totvertices		= 0;
}

void oglGetRenderStats(UINT32* points, UINT32* lines, UINT32* triangles, UINT32* vertices)
{
	totprimitives;
	*points		= totpoints;
	*lines		= totlines;
	*triangles	= tottriangles;
	*vertices	= totvertices;
}

void oglGetRenderTypes(UINT32* draw, UINT32* drawindex)
{
	*draw		= totdraw;
	*drawindex	= totdrawindex;
}

void oglResetRenderBuckets(void)
{
	memset(renderbuckets, 0, sizeof(UINT32) * MAX_RENDER_BUCKETS);
}

void oglResetRenderIndexBuckets(void)
{
	memset(renderindexbuckets, 0, sizeof(UINT32) * MAX_RENDER_BUCKETS);
}

void oglUpdateRenderBuckets(UINT32 PrimitiveCount)
{
	totprimitives += PrimitiveCount;
	if (PrimitiveCount >= MAX_RENDER_BUCKETS)
	{
		renderbuckets[MAX_RENDER_BUCKETS-1] += 1;
	}
	else
	{
		renderbuckets[PrimitiveCount] += 1;
	}
}

void oglUpdateRenderIndexBuckets(UINT32 PrimitiveCount)
{
	totprimitives += PrimitiveCount;
	if (PrimitiveCount >= MAX_RENDER_BUCKETS)
	{
		renderindexbuckets[MAX_RENDER_BUCKETS-1] += 1;
	}
	else
	{
		renderindexbuckets[PrimitiveCount] += 1;
	}
}

void oglDisplayRenderBuckets(void)
{
#ifdef OGL_EVENT_LOG
SINT32		i, num, tottriangles, totrunning, totpaints, halfpaints;

	tottriangles = 0;
	for (i=0; i < MAX_RENDER_BUCKETS; i++)
	{
		tottriangles += renderbuckets[i];
	}

	if (tottriangles != 0)
	{
		totpaints = ddpGetTotalPaints();
		if (totpaints != 0)
		{
			halfpaints = totpaints / 2;
			totrunning = 0;
			Write_Debug("Triangle render bucket breakdown:");
			for (i=0; i < MAX_RENDER_BUCKETS; i++)
			{
				num = renderbuckets[i];
				totrunning += num;
				Write_Debug(" %3d = %7d (%7d) - %7.3f : %7.3f", i, num, (num + halfpaints) / totpaints, (FLOAT32)(num * 100) / (FLOAT32)tottriangles, (FLOAT32)(totrunning * 100) / (FLOAT32)tottriangles);
			}
		}
	}
#endif	// OGL_EVENT_LOG
}

void oglDisplayRenderIndexBuckets(void)
{
#ifdef OGL_EVENT_LOG
SINT32		i, num, tottriangles, totrunning, totpaints, halfpaints;

	tottriangles = 0;
	for (i=0; i < MAX_RENDER_BUCKETS; i++)
	{
		tottriangles += renderindexbuckets[i];
	}

	if (tottriangles != 0)
	{
		totpaints = ddpGetTotalPaints();
		if (totpaints != 0)
		{
			halfpaints = totpaints / 2;
			totrunning = 0;
			Write_Debug("Triangle render index bucket breakdown:");
			for (i=0; i < MAX_RENDER_BUCKETS; i++)
			{
				num = renderindexbuckets[i];
				totrunning += num;
				Write_Debug(" %3d = %7d (%7d) - %7.3f : %7.3f", i, num, (num + halfpaints) / totpaints, (FLOAT32)(num * 100) / (FLOAT32)tottriangles, (FLOAT32)(totrunning * 100) / (FLOAT32)tottriangles);
			}
		}
	}
#endif	// OGL_EVENT_LOG
}

// Vertex Source
SINT32 oglCreateVertexSource(char* name, UINT32 Length, UINT32 vertexformat)
{
	return(INVALID_VERTEXSOURCE);
}

LOGICAL oglIsVertexSource(SINT32 vertexid)
{
	return(FALSE);
}

LOGICAL oglSetVertexSource(SINT32 srcid, UINT32 Stride, UINT32 FVF)
{
	return(FALSE);
}

LOGICAL oglResetVertexSource(void)
{
	return(FALSE);
}

LOGICAL oglLockVertexSource(SINT32 srcid, UINT32 OffsetToLock, UINT32 SizeToLock, UINT8** ppbData, UINT32 Flags)
{
	return(FALSE);
}

LOGICAL oglUnlockVertexSource(SINT32 srcid)
{
	return(FALSE);
}

LOGICAL oglReleaseVertexSource(SINT32 srcid)
{
	return(FALSE);
}

// Index Source
SINT32 oglCreateIndexSource(char* name, UINT32 Length, UINT32 indexformat)
{
	return(INVALID_INDEXSOURCE);
}

LOGICAL oglIsIndexSource(SINT32 srcid)
{
	return(FALSE);
}

LOGICAL oglSetIndexSource(SINT32 srcid, UINT32 base)
{
	return(FALSE);
}

LOGICAL oglResetIndexSource(void)
{
	return(FALSE);
}

LOGICAL oglLockIndexSource(SINT32 srcid, UINT32 OffsetToLock, UINT32 SizeToLock, UINT8** ppbData, UINT32 Flags)
{
	return(FALSE);
}

LOGICAL oglUnlockIndexSource(SINT32 srcid)
{
	return(FALSE);
}

LOGICAL oglReleaseIndexSource(SINT32 srcid)
{
	return(FALSE);
}

LOGICAL oglUpdateRenderStats(int pipt, UINT32 VertexCount)
{
UINT32	PrimitiveCount=(UINT32)-1;

	totvertices += VertexCount;
	switch (pipt)
	{
	case oglPT_POINTLIST:
		PrimitiveCount = VertexCount;
		totpoints += VertexCount;
		break;
	case oglPT_LINELIST:
		PrimitiveCount = VertexCount / 2;
		totlines += VertexCount / 2;
		break;
	case oglPT_LINESTRIP:
		PrimitiveCount = VertexCount - 1;
		totlines += VertexCount - 1;
		break;
	case oglPT_TRIANGLELIST:
		PrimitiveCount = VertexCount / 3;
		tottriangles += VertexCount / 3;
		break;
	case oglPT_TRIANGLESTRIP:
		PrimitiveCount = VertexCount - 2;
		tottriangles += VertexCount - 2;
		break;
	case oglPT_TRIANGLEFAN:
		PrimitiveCount = VertexCount - 2;
		tottriangles += VertexCount - 2;
		break;
	default:
		return(FALSE);
	}

	if (PrimitiveCount != (UINT32)-1) // god it never ends
	{
		oglUpdateRenderBuckets(PrimitiveCount);
	}

	totdraw += 1;

	return(TRUE);
}

LOGICAL oglUpdateRenderIndexStats(int pipt, UINT32 VertexCount, UINT32 IndexCount)
{
UINT32	PrimitiveCount=(UINT32)-1;

	if (IndexCount == 0)
	{
		return(oglUpdateRenderStats(pipt, VertexCount));
	}

	totvertices += VertexCount;
	switch (pipt)
	{
	case oglPT_POINTLIST:
		PrimitiveCount = IndexCount;
		totpoints += IndexCount;
		break;
	case oglPT_LINELIST:
		PrimitiveCount = IndexCount / 2;
		totlines += IndexCount / 2;
		break;
	case oglPT_LINESTRIP:
		PrimitiveCount = IndexCount - 1;
		totlines += IndexCount - 1;
		break;
	case oglPT_TRIANGLELIST:
		PrimitiveCount = IndexCount / 3;
		tottriangles += IndexCount / 3;
		break;
	case oglPT_TRIANGLESTRIP:
		PrimitiveCount = IndexCount - 2;
		tottriangles += IndexCount - 2;
		break;
	case oglPT_TRIANGLEFAN:
		PrimitiveCount = IndexCount - 2;
		tottriangles += IndexCount - 2;
		break;
	default:
		return(FALSE);
	}

	if (PrimitiveCount != (UINT32)-1) // signed, unsigned? who cares? apparently not microsoft compiler writers
	{
		oglUpdateRenderIndexBuckets(PrimitiveCount);
	}

	totdrawindex += 1;

	return(TRUE);
}

LOGICAL oglBeginScene(void)
{
	return(TRUE);
}

LOGICAL oglEndScene(void)
{
	return(TRUE);
}


LOGICAL oglGetTextureMemoryDescriptor(SINT32 texid, SINT32 miplevel, oglSURFACEDESC* lpsd)
{
TEXTURE*	texture;

	texture = oglGetTextureData(texid);
	if(texture == NULL) return FALSE;

	lpsd->Width       = texture->po2width[miplevel];
	lpsd->Height      = texture->po2height[miplevel];
	lpsd->Pitch       = texture->comp[miplevel] * lpsd->Width;
	lpsd->Surface     = texture->lpsurf[miplevel];

	if (lpsd->Surface == NULL)
		return FALSE;

	lpsd->PixelFormat.BitCount = texture->comp[miplevel] * 8;

#if platOS == OS_WIN32
	lpsd->PixelFormat.RedBitMask	= 0x00ff0000;
	lpsd->PixelFormat.GreenBitMask	= 0x0000ff00;
	lpsd->PixelFormat.BlueBitMask	= 0x000000ff;

	lpsd->PixelFormat.dwRBitMask	= 0x00ff0000;
	lpsd->PixelFormat.dwGBitMask	= 0x0000ff00;
	lpsd->PixelFormat.dwBBitMask	= 0x000000ff;
#else
	lpsd->PixelFormat.RedBitMask	= 0xff000000;
	lpsd->PixelFormat.GreenBitMask	= 0x00ff0000;
	lpsd->PixelFormat.BlueBitMask	= 0x0000ff00;

	lpsd->PixelFormat.dwRBitMask	= 0xff000000;
	lpsd->PixelFormat.dwGBitMask	= 0x00ff0000;
	lpsd->PixelFormat.dwBBitMask	= 0x0000ff00;
#endif

	if (texture->comp[miplevel] == 4)
	{
#if platOS == OS_WIN32
		lpsd->PixelFormat.dwRGBAlphaBitMask = 0xff000000;
#else
		lpsd->PixelFormat.dwRGBAlphaBitMask = 0x000000ff;
#endif
	}
	else
	{
		lpsd->PixelFormat.dwRGBAlphaBitMask = 0x00000000;
	}

	return(TRUE);
}

#ifdef USE_DXT_ALPHA_COLLIDERS

static UINT32 GetColorFromDXT1Block(UINT32 wanted, UINT8* pblock, UINT32 transparent)
{
SINT32		i, j;
UINT8		tc, index[4][4];
UINT16		color[4];
UINT32		r, g, b, a, result;

	color[0] = *((UINT16*)pblock);
	pblock += 2;
	color[1] = *((UINT16*)pblock);
	pblock += 2;

	// All colors are in [1]555/[A]RGB format (Alpha is MSB, if present)

	if (color[0] > color[1])
	{
		// Four-color block: derive the other two colors.
		// 00 = color[0], 01 = color[1], 10 = color[2], 11 = color[3]
		// These 2-bit codes correspond to the 2-bit fields
		// stored in the 64-bit block.
		color[2] = (2 * color[0] + color[1] + 1) / 3;
		color[3] = (color[0] + 2 * color[1] + 1) / 3;
	}
	else
	{
		// Three-color block: derive the other color.
		// 00 = color[0],  01 = color[1],  10 = color[2],
		// 11 = color[3] = transparent.
		// These 2-bit codes correspond to the 2-bit fields
		// stored in the 64-bit block.
		color[2] = (color[0] + color[1]) / 2;
		color[3] = transparent;
	}

	// Create the indices for later reference
	for (i=0; i < 4; i++)
	{
		tc = *pblock++;
		for (j=0; j < 4; j++)
		{
			index[i][j] = tc & 0x03;
			tc >>= 2;
		}
	}

	result = color[index[wanted / 4][wanted & 3]];	// &3 = fast modulo 4
	a = result >> 15;
	if (a != 0)
	{
		a = 0xff;
	}

	r = ((result >> 10) & 0x1f) << 3;
	g = ((result >> 5) & 0x1f) << 3;
	b = (result & 0x1f) << 3;

	result = piRGBA_MAKE(r, g, b, a);

	return(result);
}

static UINT32 GetColorFromDXT3Block(UINT32 wanted, UINT8* pblock)
{
SINT32		i, j;
UINT8		tc, alpha[4][4], index[4][4];
UINT16		ta, color[4];
UINT32		r, g, b, a, result;

	// Create the alphas for later reference
	for (i=0; i < 4; i++)
	{
		ta = *((UINT16*)pblock);
		pblock += 2;
		for (j=0; j < 4; j++)
		{
			alpha[i][j] = ta & 0x000f;
			ta >>= 4;
		}
	}

	// All colors are in 565/RGB format (Red is MSB)

	// Four-color block
	color[0] = *((UINT16*)pblock);
	pblock += 2;
	color[1] = *((UINT16*)pblock);
	pblock += 2;

	// Derive the other two colors.
	color[2] = (2 * color[0] + color[1] + 1) / 3;
	color[3] = (color[0] + 2 * color[1] + 1) / 3;

	// Create the indices for later reference
	for (i=0; i < 4; i++)
	{
		tc = *pblock++;
		for (j=0; j < 4; j++)
		{
			index[i][j] = tc & 0x03;
			tc >>= 2;
		}
	}

	result = color[index[wanted / 4][wanted & 3]];
	a = alpha[wanted / 4][wanted & 3] << 4;
	if (a != 0)
	{
		a |= 0x0f;
	}

	r = (result >> 11) << 3;
	g = ((result >> 5) & 0x3f) << 2;
	b = (result & 0x1f) << 3;

	result = piRGBA_MAKE(r, g, b, a);

	return(result);
}

// Needs work...does not provide accurate results on some occasions.
static void _dxt135_decode_imageblock(const GLubyte* block,
                                      GLint wanted,
                                      GLuint type,
                                      GLvoid* texel)
{
   GLubyte* rgba = (GLubyte*)texel;

   const GLubyte* c0_lo  = block + 0;
   const GLubyte* c0_hi  = block + 1;
   const GLubyte* c1_lo  = block + 2;
   const GLubyte* c1_hi  = block + 3;
   const GLubyte* bits_0 = block + 4;
   const GLubyte* bits_1 = block + 5;
   const GLubyte* bits_2 = block + 6;
   const GLubyte* bits_3 = block + 7;

    GLushort color0 = *c0_lo + *c0_hi * 256;
    GLushort color1 = *c1_lo + *c1_hi * 256;

    GLuint bits = *bits_0 + 256 * (*bits_1 + 256 * (*bits_2 + 256 * *bits_3));

   // GLubyte bit_pos = 2 * (j * 4 + i);
    GLubyte code = (GLubyte) ((bits >> wanted) & 3);

    switch (code)
    {
    case 0:
        {
            rgba[0] = ((color0 >> 8) & 0xf8) * 255 / 0xf8;
            rgba[1] = ((color0 >> 3) & 0xfc) * 255 / 0xfc;
            rgba[2] = ((color0 << 3) & 0xf8) * 255 / 0xf8;
            rgba[3] = 255;
            break;
        }
    case 1:
        {
            rgba[0] = ((color1 >> 8) & 0xf8) * 255 / 0xf8;
            rgba[1] = ((color1 >> 3) & 0xfc) * 255 / 0xfc;
            rgba[2] = ((color1 << 3) & 0xf8) * 255 / 0xf8;
            rgba[3] = 255;
            break;
        }
    case 2:
        {
            if(color0 > color1)
            {
                rgba[0] = (((color0 >> 8) & 0xf8) * 255 / 0xf8 * 2 + ((color1 >> 8) & 0xf8) * 255 / 0xf8 ) / 3;
                rgba[1] = (((color0 >> 3) & 0xfc) * 255 / 0xfc * 2 + ((color1 >> 3) & 0xfc) * 255 / 0xfc ) / 3;
                rgba[2] = (((color0 << 3) & 0xf8) * 255 / 0xf8 * 2 + ((color1 << 3) & 0xf8) * 255 / 0xf8 ) / 3;
                rgba[3] = 255;
            }
            else
            {
                rgba[0] = (((color0 >> 8) & 0xf8) * 255 / 0xf8 + ((color1 >> 8) & 0xf8) * 255 / 0xf8 ) / 2;
                rgba[1] = (((color0 >> 3) & 0xfc) * 255 / 0xfc + ((color1 >> 3) & 0xfc) * 255 / 0xfc ) / 2;
                rgba[2] = (((color0 << 3) & 0xf8) * 255 / 0xf8 + ((color1 << 3) & 0xf8) * 255 / 0xf8 ) / 2;
                rgba[3] = 255;
            }
            break;
        }
    case 3:
        {
            /* don't understand the spec. Is the dxt_type switch necessary for other code cases too? */
            if ((type > 1) || (color0 > color1))
            {
                rgba[0] = (((color0 >> 8) & 0xf8) * 255 / 0xf8 + ((color1 >> 8) & 0xf8) * 255 / 0xf8 * 2) / 3;
                rgba[1] = (((color0 >> 3) & 0xfc) * 255 / 0xfc + ((color1 >> 3) & 0xfc) * 255 / 0xfc * 2) / 3;
                rgba[2] = (((color0 << 3) & 0xf8) * 255 / 0xf8 + ((color1 << 3) & 0xf8) * 255 / 0xf8 * 2) / 3;
                rgba[3] = 255;
            }
            else
            {
                rgba[0] = 0;
                rgba[1] = 0;
                rgba[2] = 0;
                rgba[3] = 255;
                if (type == 1)
                {
                    rgba[3] = 0;
                }
            }
            break;
        }
      default:
      /* CANNOT happen (I hope) */
         break;
   }
}

// Needs work...does not provide accurate results on some occasions.
static UINT32 GetColorFromDXT5Block(UINT32 wanted, const GLubyte* block)
{
    UCHAR texel[4];
    UCHAR* rgba = texel;

    _dxt135_decode_imageblock(block + 8, wanted, 2, texel);

    GLubyte code;
    GLushort alpha0 = *block++;
    GLushort alpha1 = *block++;
    GLubyte bit_pos = wanted;//3 * ((j&3) * 4 + (i&3));

    /* simple 32bit version */
    GLuint bits_low = *block | (*++block << 8) | (*++block << 16) | (*++block << 24);
    GLuint bits_high = *++block | (*++block << 8);

    //GLuint bits_low = *((GLuint*)blksrc)++;
    //GLuint bits_high = *(GLuint*)blksrc;

    if(bit_pos < 30)
    {
        code = (GLubyte) ((bits_low >> bit_pos) & 7);
    }
    else if (bit_pos == 30)
    {
        code = (GLubyte) ((bits_low >> 30) & 3) | ((bits_high << 2) & 4);
    }
    else
    {
        code = (GLubyte) ((bits_high >> (bit_pos - 32)) & 7);
    }

    if(alpha0 > alpha1)
    {
        switch (code)
        {
        case 0:
            rgba[3] = (GLubyte)alpha0;
            break;
        case 1:
            rgba[3] = (GLubyte)alpha1;
            break;
        case 2:
            rgba[3] = (GLubyte)(6 * alpha0 + 1 * alpha1 + 3) / 7;
            break;
        case 3:
            rgba[3] = (GLubyte)(5 * alpha0 + 2 * alpha1 + 3) / 7;
            break;
        case 4:
            rgba[3] = (GLubyte)(4 * alpha0 + 3 * alpha1 + 3) / 7;
            break;
        case 5:
            rgba[3] = (GLubyte)(3 * alpha0 + 4 * alpha1 + 3) / 7;
            break;
        case 6:
            rgba[3] = (GLubyte)(2 * alpha0 + 5 * alpha1 + 3) / 7;
            break;
        case 7:
            rgba[3] = (GLubyte)(1 * alpha0 + 6 * alpha1 + 3) / 7;
            break;
        }
    }
    else
    {
        switch (code)
        {
        case 0:
            rgba[3] = (GLubyte)alpha0;
            break;
        case 1:
            rgba[3] = (GLubyte)alpha1;
            break;
        case 2:
            rgba[3] = (GLubyte)(4 * alpha0 + 1 * alpha1 + 2) / 5;
            break;
        case 3:
            rgba[3] = (GLubyte)(3 * alpha0 + 2 * alpha1 + 2) / 5;
            break;
        case 4:
            rgba[3] = (GLubyte)(2 * alpha0 + 3 * alpha1 + 2) / 5;
            break;
        case 5:
            rgba[3] = (GLubyte)(1 * alpha0 + 4 * alpha1 + 2) / 5;
            break;
        case 6:
            rgba[3] = 0;
            break;
        case 7:
            rgba[3] = 255;
            break;
        }
    }

    UINT32 color = piRGBA_MAKE(rgba[0], rgba[1], rgba[2], rgba[3]);

    return color;
}

// Query to determine the alpha value associated with the
// texel location specified by the u and v coordinates
LOGICAL oglGetTextureUVAlpha(UINT32* alpha, SINT32 texid, SINT32 miplevel, FLOAT32 u, FLOAT32 v)
{
UINT8*		cptr;
UINT16*		sptr;
UINT32*		lptr, color;
SINT32		width, height, x, y, stride;
UINT32		AlphaBits, AlphaShift, AlphaMask;
FLOAT32		uf, vf;
#ifdef DONT_USE
FLOAT64		ui, vi;
#endif
TEXTURE*	lptex;
piSURFACEDESC	pisd;

	if (!piIsTexture(texid, miplevel))
	{
		return(FALSE);
	}

#ifdef DONT_USE
	uf = (FLOAT32)modf(u, &ui);
	vf = (FLOAT32)modf(v, &vi);
	if (uf < 0.0f)
	{
		uf = 1.0f + uf;
	}
	if (vf < 0.0f)
	{
		vf = 1.0f + vf;
	}
#endif

	uf = u - (FLOAT32)floor(u);
	vf = v - (FLOAT32)floor(v);

	if (uf > 1.0f || uf < 0.0f || vf > 1.0f || vf < 0.0f)
	{
		// we've got a problem ...
		return (FALSE);
	}

	lptex = oglGetTextureData(texid);
	if (!lptex->hasalpha)
	{
		*alpha = 255;
		return(TRUE);
	}

	width = lptex->imagewidth[miplevel];
	height = lptex->imageheight[miplevel];

	x = (SINT32)((FLOAT32)width * uf);
	y = (SINT32)((FLOAT32)height * vf);

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x >= width) x = width-1;
	if(y >= height) y = height-1;

	if (!piLockTextureMemorySurface(texid, miplevel, &pisd))
	{
		return(FALSE);
	}

	*alpha = 0;
	if (lptex->format[miplevel] == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
	{
	SINT32		xa, ya, xb, yb;

		xa = x / 4;
		ya = y / 4;
		xb = x & 3;	// fast mod 4
		yb = y & 3;	// fast mod 4
		*alpha = GetColorFromDXT1Block(yb * 4 + xb, (UINT8*)pisd.Surface + (ya * (width / 4) + xa) * 8, 0) >> 24;
	}
	else
	if (lptex->format[miplevel] == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
	{
	SINT32		xa, ya, xb, yb;

		xa = x / 4;
		ya = y / 4;
		xb = x & 3;	// fast mod 4
		yb = y & 3;	// fast mod 4
		*alpha = GetColorFromDXT3Block(yb * 4 + xb, (UINT8*)pisd.Surface + (ya * (width / 4) + xa) * 16) >> 24;
	}
    else if(lptex->format[miplevel] == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
    {
        SINT32 xa, ya, xb, yb;

        xa = x / 4;
        ya = y / 4;
        xb = x & 3;
        yb = y & 3;

        GLubyte* block;

        block = (GLubyte*)pisd.Surface + (ya * ((width)/ 4) + xa) * 16;

        UINT32 color = GetColorFromDXT5Block(yb * 4 + xb, block);

        *alpha = color >> 24;
    }
	else
	{
		AlphaMask   = pisd.PixelFormat.dwRGBAlphaBitMask;
		if (AlphaMask == 0)
		{
			*alpha = 255;
		}
		else
		{
			AlphaBits   = 8 - piFindBitCount(AlphaMask);
			AlphaShift  = piFindBitShift(AlphaMask);

			if (pisd.PixelFormat.dwRGBBitCount <= 8)
			{
				cptr = (UINT8*)pisd.Surface;
				stride = pisd.Pitch;
				x = y * stride + x;
				color = cptr[x];
			}
			else
			if (pisd.PixelFormat.dwRGBBitCount <= 16)
			{
				sptr = (UINT16*)pisd.Surface;
				stride = pisd.Pitch / 2;
				x = y * stride + x;
				color = sptr[x];
			}
			else
			if (pisd.PixelFormat.dwRGBBitCount <= 24)
			{
				cptr = (UINT8*)pisd.Surface;
				stride = pisd.Pitch;
				x = (y * stride + x) * 3;
				color = cptr[x] | (cptr[x+1] << 8) | (cptr[x+2] << 16);
			}
			else
			if (pisd.PixelFormat.dwRGBBitCount <= 32)
			{
				lptr = (UINT32*)pisd.Surface;
				stride = pisd.Pitch / 4;
				x = y * stride + x;
				color = lptr[x];
			}

			color = ((color & AlphaMask) >> AlphaShift) << AlphaBits;
			*alpha = color;
		}
	}

//	if (!piUnlockTextureMemorySurface(texid, miplevel))
//	{
//		return(FALSE);
//	}

	return(TRUE);
}

#else	// USE_DXT_ALPHA_COLLIDERS

LOGICAL oglGetTextureUVAlpha(UINT32* alpha, SINT32 texid, SINT32 miplevel, FLOAT32 u, FLOAT32 v)
{
UINT8*		cptr;
UINT16*		sptr;
UINT32*		lptr, color;
SINT32		width, height, x, y, stride;
UINT32		AlphaBits, AlphaShift, AlphaMask;
FLOAT32		uf, vf;
#ifdef DONT_USE
double		ui, vi;
#endif
TEXTURE*	lptex;
void*	lpdds;
oglSURFACEDESC sd;

	if (!oglIsTexture(texid, miplevel))
	{
		return(FALSE);
	}

#ifdef DONT_USE
	uf = (FLOAT32)modf(u, &ui);
	vf = (FLOAT32)modf(v, &vi);
	if (uf < 0.0f)
	{
		uf = 1.0f + uf;
	}
	if (vf < 0.0f)
	{
		vf = 1.0f + vf;
	}
#endif

	uf = u - (FLOAT32)floor(u);
	vf = v - (FLOAT32)floor(v);

	if (uf > 1.0f || uf < 0.0f || vf > 1.0f || vf < 0.0f)
	{
		// we've got a problem ...
		return (FALSE);
	}

	lptex = oglGetTextureData(texid);
	if (!lptex->hasalpha)
	{
		*alpha = 255;
		return(TRUE);
	}


	width = lptex->imagewidth[miplevel];
	height = lptex->imageheight[miplevel];

	x = (SINT32)((FLOAT32)width * uf);
	y = (SINT32)((FLOAT32)height * vf);

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x >= width) x = width-1;
	if(y >= height) y = height-1;

	lpdds = oglGetTextureMemorySurface(texid, miplevel);
	if (lpdds == NULL)
	{
		return(FALSE);
	}

	oglGetTextureMemoryDescriptor(texid, miplevel, &sd);

	AlphaMask   = sd.PixelFormat.dwRGBAlphaBitMask;
	if (AlphaMask == 0)
	{
		*alpha = 255;
		return(TRUE);
	}

	AlphaBits   = 8 - oglFindBitCount(AlphaMask);
	AlphaShift  = oglFindBitShift(AlphaMask);

	if (sd.PixelFormat.dwRGBBitCount <= 8)
	{
		cptr = (UINT8*)sd.Surface;
		stride = sd.Pitch;
		x = y * stride + x;
		color = cptr[x];
	}
	else
	if (sd.PixelFormat.dwRGBBitCount <= 16)
	{
		sptr = (UINT16*)sd.Surface;
		stride = sd.Pitch >> 1;
		x = y * stride + x;
		color = sptr[x];
	}
	else
	if (sd.PixelFormat.dwRGBBitCount <= 24)
	{
		cptr = (UINT8*)sd.Surface;
		stride = sd.Pitch;
		x = (y * stride + x) * 3;
		color = cptr[x] | (cptr[x+1] << 8) | (cptr[x+2] << 16);
	}
	else
	if (sd.PixelFormat.dwRGBBitCount <= 32)
	{
		lptr = (UINT32*)sd.Surface;
		stride = sd.Pitch >> 2;
		x = y * stride + x;
		color = lptr[x];
	}

	color = ((color & AlphaMask) >> AlphaShift) << AlphaBits;

	*alpha = color;

	return(TRUE);
}

#endif	// USE_DXT_ALPHA_COLLIDERS

SINT32 oglGetTextureUVColor(UINT32* pColor,
                            SINT32 iTexID,
                            SINT32 iMipLevel,
                            FLOAT32 u,
                            FLOAT32 v)
{
    UINT8* cPtr = NULL;

    UINT16* sPtr = NULL;

    UINT32 uColor = 0;
 //   UINT32 uAlpha = 0;
    UINT32 uAlphaBits = 0;
    UINT32 uAlphaShift = 0;
    UINT32 uAlphaMask = 0;
    UINT32* lPtr = NULL;

    FLOAT32 uf, vf;

    SINT32 iWidth = 0;
    SINT32 iHeight = 0;
    SINT32 x, y;
    SINT32 iStride;

    TEXTURE* lpTex = NULL;

    piSURFACEDESC pisd;

    if(!piIsTexture(iTexID, iMipLevel))
    {
        return -1;
    }

    uf = u - (FLOAT32)floor(u);
    vf = v - (FLOAT32)floor(v);

    if(uf > 1.0f || uf < 0.0f || vf > 1.0f || vf < 0.0f)
    {
        return -1;
    }

    lpTex = oglGetTextureData(iTexID);

    iWidth = lpTex->imagewidth[iMipLevel];
    iHeight = lpTex->imageheight[iMipLevel];

    x = (SINT32)((FLOAT32)iWidth * uf);
    y = (SINT32)((FLOAT32)iHeight * vf);

    if(!piLockTextureMemorySurface(iTexID, iMipLevel, &pisd))
    {
        return -1;
    }

//#if platOS != OS_MACOS
	if(lpTex->format[iMipLevel] == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
	{
        SINT32		xa, ya, xb, yb;

        xa = x / 4;
        ya = y / 4;
        xb = x & 3;	// fast modulo 4
        yb = y & 3;	// fast modulo 4

        uColor = GetColorFromDXT1Block(yb * 4 + xb, (UINT8*)pisd.Surface + (ya * (iWidth / 4) + xa) * 8, 0);
	}
    else if(lpTex->format[iMipLevel] == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
    {
        SINT32		xa, ya, xb, yb;

        xa = x / 4;
        ya = y / 4;
        xb = x & 3;	// fast modulo 4
        yb = y & 3;	// fast modulo 4

        uColor = GetColorFromDXT3Block(yb * 4 + xb, (UINT8*)pisd.Surface + (ya * (iWidth / 4) + xa) * 16);
    }
	else
//#endif
	{
        uAlphaMask = pisd.PixelFormat.dwRGBAlphaBitMask;

        uAlphaBits = 8 - piFindBitCount(uAlphaMask);
        uAlphaShift  = piFindBitShift(uAlphaMask);

        if(pisd.PixelFormat.dwRGBBitCount <= 8)
        {
            cPtr = (UINT8*)pisd.Surface;
            iStride = pisd.Pitch;
            x = y * iStride + x;
            uColor = cPtr[x];
        }
        else if(pisd.PixelFormat.dwRGBBitCount <= 16)
        {
            sPtr = (UINT16*)pisd.Surface;
            iStride = pisd.Pitch / 2;
            x = y * iStride + x;
            uColor = sPtr[x];
        }
        else if(pisd.PixelFormat.dwRGBBitCount <= 24)
        {
            cPtr = (UINT8*)pisd.Surface;
            iStride = pisd.Pitch;
            x = (y * iStride + x) * 3;
            uColor = cPtr[x] | (cPtr[x+1] << 8) | (cPtr[x+2] << 16);
        }
        else if(pisd.PixelFormat.dwRGBBitCount <= 32)
        {
            lPtr = (UINT32*)pisd.Surface;

            iStride = pisd.Pitch / 4;
            x = y * iStride + x;
            uColor = lPtr[x];
        }

        uColor = ((uColor & uAlphaMask) >> uAlphaShift) << uAlphaBits;
	}

    *pColor = uColor;

    return 0;
}

piMATRIX* oglGetCockpitViewMatrix(void)
{
	return(&cockpitview);
}

LOGICAL oglSetCockpitViewMatrix(piMATRIX* lpnewview)
{
    if(_lockCockpitMatrix)
    {
        return TRUE;
    }

	cockpitview = *lpnewview;

	return(TRUE);
}

LOGICAL oglMaterialHasSpecular(SINT32 matid)
{
	if (!oglIsMaterial(matid))
	{
		return(FALSE);
	}

	// shininess must be greater than 1 for specular to be visible
	if (materials[matid].material.power <= 1.0f)
	{
		return(FALSE);
	}

	// the color must be non-zero for specular to be visible
	if (materials[matid].material.specular.r == 0.0f &&
		materials[matid].material.specular.g == 0.0f &&
		materials[matid].material.specular.b == 0.0f &&
		materials[matid].material.specular.a == 0.0f)
	{
		return(FALSE);
	}

	return(TRUE);
}

piMATRIX* oglGetUnscaledViewMatrix(void)
{
	return &piMatUnscaledView;
}

LOGICAL oglSetUnscaledViewMatrix(piMATRIX* lpNewView)
{
    if(_lockUnscaledViewMatrix)
    {
        return TRUE;
    }

	piMatUnscaledView = *lpNewView;

	return TRUE;
}

void* oglGetTextureMemorySurface(SINT32 iTexID, SINT32 iMipLevel)
{
	if (!oglIsTexture(iTexID, iMipLevel))
	{
		return NULL;
	}

	return(textures[iTexID].lpsurf[iMipLevel]);
}

LOGICAL oglSetRenderStageState(UINT32 stage, oglTEXTURESTAGESTATETYPE type, UINT32 state)
{
	oglSetActiveTextureStage(stage);

    // Check change for global states only. Non global states pass through always
	if(stagestates[stage][type] != state)
	{
		if(!HandleGLError(TRUE))
		{
			Write_Debug("GL error PRIOR to set render stage state %d to value %d", type, state);
		}

		switch(type)
		{
		case piTSS_TEXTURE_ENV_MODE:
            {
			    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, state);
                break;
            }
		case piTSS_COMBINE_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, state);
                break;
            }
		case piTSS_SOURCE0_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, state);
                break;
            }
		case piTSS_SOURCE1_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, state);
                break;
            }
		case piTSS_SOURCE2_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, state);
                break;
            }
		case piTSS_OPERAND0_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, state);
                break;
            }
		case piTSS_OPERAND1_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, state);
                break;
            }
		case piTSS_OPERAND2_RGB:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, state);
                break;
            }
		case piTSS_COMBINE_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, state);
                break;
            }
		case piTSS_SOURCE0_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, state);
                break;
            }
		case piTSS_SOURCE1_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, state);
                break;
            }
		case piTSS_SOURCE2_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, state);
                break;
            }
		case piTSS_OPERAND0_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, state);
                break;
            }
		case piTSS_OPERAND1_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, state);
                break;
            }
		case piTSS_OPERAND2_ALPHA:
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, state);
                break;
            }
        case oglTSS_ENABLE_TEXTURE2D:
            {
                if(state)
                {
                    TextureState::Enable(GL_TEXTURE_2D);
                }
                else
                {
                    TextureState::Disable();
                }

                break;
            }
        }

        stagestates[stage][type] = state;
    }

    // Not global...always check
    switch(type)
    {
    case oglTSS_ADDRESSU:
        {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
			break;
        }
    case oglTSS_ADDRESSV:
        {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
			break;
        }
    case oglTSS_ADDRESSW:
        {
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, state);
			break;
        }
    case oglTSS_MAGFILTER:
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state);
            break;
        }
    case oglTSS_MINFILTER:
        {
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state);
            break;
		}
	}

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error during set render stage state %d to value %d", type, state);
	}

	return(TRUE);
}

/************************************************************************************************
	oglSetColorAtTextureUV
*************************************************************************************************/

// Set the color value associated with the texel
// location specified by the u and v coordinates
LOGICAL oglSetColorAtTextureUV(SINT32 iTexID,
                                SINT32 iMipLevel,
                                FLOAT32 u,
                                FLOAT32 v,
                                UINT32 uColor)
{
    return FALSE;
}

/************************************************************************************************
	oglDuplicateRawTexture
*************************************************************************************************/

SINT32 oglDuplicateRawTexture(SINT32 texid, SINT32 miplevel)
{
SINT32	newid, bpp, alphabits;

	if (textures[texid].comp[miplevel] == 4)
	{
		// format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
		if (textures[texid].format[miplevel] == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		{
			alphabits = 1;
		}
		else
		// format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT || format == RGBA
		if (textures[texid].format[miplevel] == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
			textures[texid].format[miplevel] == GL_RGBA)
		{
			alphabits = 8;
		}

		bpp = 32;
	}
	else
	if (textures[texid].comp[miplevel] == 3)
	{
		// format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT
		if (textures[texid].format[miplevel] == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
		{
			alphabits = 0;
		}
		else
		// format == GL_RGB
		if (textures[texid].format[miplevel] == GL_RGB)
		{
			alphabits = 0;
		}

		bpp = 32;
	}

	newid = oglCreateRawTexture(-1, miplevel,
							textures[texid].imagewidth[miplevel],
							textures[texid].imageheight[miplevel],
							bpp,
							alphabits,
							CRT_FLAG_NONE);

	if (newid < 0)
		return -1;

	memcpy(textures[newid].lpsurf[miplevel], textures[texid].lpsurf[miplevel], textures[texid].imagesize[miplevel]);

	return newid;
}

/************************************************************************************************
	oglGetTextureMatrix
*************************************************************************************************/

piMATRIX& oglGetTextureMatrix(int unit)
{
	return(texmat[unit]);
}

void oglSetMatrixMode(GLenum mode)
{
    if(crtMode != mode)
    {
	    glMatrixMode(mode);
	    crtMode = mode;
    }
}

GLenum oglGetMatrixMode()
{
    return crtMode;
}

/************************************************************************************************
	oglSetTextureMatrix
*************************************************************************************************/

void oglSetTextureMatrix(const piMATRIX* newtexmat)
{
    if(!_lockTextureMatrix)
    {
        int unit = TextureState::currentunit;

        if(ogl::UseDeprecatedPath())
        {
            if(texmat[unit]._11 != newtexmat->_11 ||
               texmat[unit]._12 != newtexmat->_12 ||
               texmat[unit]._13 != newtexmat->_13 ||
               texmat[unit]._14 != newtexmat->_14 ||
               texmat[unit]._21 != newtexmat->_21 ||
               texmat[unit]._22 != newtexmat->_22 ||
               texmat[unit]._23 != newtexmat->_23 ||
               texmat[unit]._24 != newtexmat->_24 ||
               texmat[unit]._31 != newtexmat->_31 ||
               texmat[unit]._32 != newtexmat->_32 ||
               texmat[unit]._33 != newtexmat->_33 ||
               texmat[unit]._34 != newtexmat->_34 ||
               texmat[unit]._41 != newtexmat->_41 ||
               texmat[unit]._42 != newtexmat->_42 ||
               texmat[unit]._43 != newtexmat->_43 ||
               texmat[unit]._44 != newtexmat->_44)
            {
	            texmat[unit] = *newtexmat;

                oglSetMatrixMode(GL_TEXTURE);
                glLoadMatrixf((GLfloat*)&texmat[unit]);
                oglSetMatrixMode(GL_MODELVIEW);
            }
        }
        else
        {
            texmat[unit] = *newtexmat;
            glslSetTextureMatrix(unit, texmat[unit]);
        }
    }
}

void oglSetTextureMatrix(const Matrixf& newtexmat)
{
    if(!_lockTextureMatrix)
    {
        int unit = TextureState::currentunit;

        if(texmat[unit]._11 != newtexmat(0, 0) ||
           texmat[unit]._12 != newtexmat(0, 1) ||
           texmat[unit]._13 != newtexmat(0, 2) ||
           texmat[unit]._14 != newtexmat(0, 3) ||
           texmat[unit]._21 != newtexmat(1, 0) ||
           texmat[unit]._22 != newtexmat(1, 1) ||
           texmat[unit]._23 != newtexmat(1, 2) ||
           texmat[unit]._24 != newtexmat(1, 3) ||
           texmat[unit]._31 != newtexmat(2, 0) ||
           texmat[unit]._32 != newtexmat(2, 1) ||
           texmat[unit]._33 != newtexmat(2, 2) ||
           texmat[unit]._34 != newtexmat(2, 3) ||
           texmat[unit]._41 != newtexmat(3, 0) ||
           texmat[unit]._42 != newtexmat(3, 1) ||
           texmat[unit]._43 != newtexmat(3, 2) ||
           texmat[unit]._44 != newtexmat(3, 3))
        {
            if(ogl::UseDeprecatedPath())
            {
	            newtexmat.get(texmat[unit]);

                oglSetMatrixMode(GL_TEXTURE);
                glLoadMatrixf((GLfloat*)&texmat[unit]);
                oglSetMatrixMode(GL_MODELVIEW);
            }
            else
            {
                newtexmat.get(texmat[unit]);
                glslSetTextureMatrix(unit, texmat[unit]);
            }
        }
    }
}

/************************************************************************************************
	oglSetTextureName
*************************************************************************************************/

LOGICAL oglSetTextureName(SINT32 texid, const SINT8* name)
{
	if (!oglIsTexture(texid, 0))
	{
		return FALSE;
	}

  if ( name[0] == 0 )
  {
    printf("null texture name\n");
  }

  TEXTURE* p = &textures[texid];

	strncpy(p->name, name, 128);

  std::string str = name;
  std::transform(str.begin(), str.end(),str.begin(), ::toupper);

  std::replace(str.begin(), str.end(), '\\', '/');

  textureIndex[str] = texid;

#ifdef BETA_RELEASE
  GLuint g = p->genlist;
  if ( g )
  {
    reverseTextureIndex[g] = str;
  }
#endif

	return TRUE;
}

/************************************************************************************************
	oglSetLight
*************************************************************************************************/

LOGICAL oglSetLight(SINT32 numlight, LIGHT* light)
{
    if(ogl::UseDeprecatedPath() && !g_statelocked[piRENDERSTATE_LIGHTING])
    {
	    if(numlight > -1 && numlight < numlights)
	    {
            piLIGHT* pil;
            FLOAT32	val[4];

		    lights[numlight] = *light;

		    if (light->flags & LITE_ON)
		    {
			    pil = &lights[numlight].light;
			    if (pil->dltType == piLIGHT_DIRECTIONAL)
			    {
				    val[0] = -pil->dvDirection.x;
				    val[1] = -pil->dvDirection.y;
				    val[2] = -pil->dvDirection.z;
				    val[3] = 0.0f;
			    }
			    else
			    {
				    val[0] = pil->dvPosition.x;
				    val[1] = pil->dvPosition.y;
				    val[2] = pil->dvPosition.z;
				    val[3] = 1.0f;
			    }

                // [DCF] glGet functions execute all queued GL commands. (slow)
#if defined(USE_GL_ERROR)
                GLenum errr = glGetError();
#endif
			    oglSetLightState(numlight, TRUE);

#if defined(USE_GL_ERROR)
      errr = glGetError(); if ( errr != 0 ) printf("error %u",(unsigned int)errr);
#endif
			    glLightfv(GL_LIGHT0+numlight, GL_AMBIENT, (GLfloat*)&pil->dcvAmbient);

#if defined(USE_GL_ERROR)
      errr = glGetError(); if ( errr != 0 ) printf("error %u",(unsigned int)errr);
#endif
			    glLightfv(GL_LIGHT0+numlight, GL_DIFFUSE, (GLfloat*)&pil->dcvColor);

#if defined(USE_GL_ERROR)
      errr = glGetError(); if ( errr != 0 ) printf("error %u",(unsigned int)errr);
#endif
			    glLightfv(GL_LIGHT0+numlight, GL_POSITION, (GLfloat*)val);

#if defined(USE_GL_ERROR)
      errr = glGetError(); if ( errr != 0 ) printf("error %u",(unsigned int)errr);
#endif
			    glLightfv(GL_LIGHT0+numlight, GL_SPECULAR, (GLfloat*)&pil->dcvSpecular);

#if defined(USE_GL_ERROR)
      errr = glGetError(); if ( errr != 0 ) printf("error %u",(unsigned int)errr);
#endif

#if(0)
			    if (bSpecular)
			    {
				    glLightfv(GL_LIGHT0+numlight, GL_SPECULAR, (GLfloat*)&pil->dcvSpecular);
			    }
			    else
			    {
				    val[0] = 0.0f;
				    val[1] = 0.0f;
				    val[2] = 0.0f;
				    val[3] = 1.0f;
				    glLightfv(GL_LIGHT0+numlight, GL_SPECULAR, (GLfloat*)val);
			    }
#endif

#if(1)
			    if (pil->dltType == piLIGHT_POINT)
			    {
				    if (pil->dvRange > 1.0f)
				    {
					    glLightf(GL_LIGHT0+numlight, GL_CONSTANT_ATTENUATION, (GLfloat)(pil->dvAttenuation0 < 1.0f ? 1.0f : pil->dvAttenuation0));
					    glLightf(GL_LIGHT0+numlight, GL_LINEAR_ATTENUATION, (GLfloat)(pil->dvAttenuation1 == 0.0f ? 1.0f / pil->dvRange : pil->dvAttenuation1));
					    glLightf(GL_LIGHT0+numlight, GL_QUADRATIC_ATTENUATION, (GLfloat)pil->dvAttenuation2);
				    }
				    else
				    {
					    oglSetLightState(numlight, FALSE);
				    }
			    }
#endif
		    }
		    else
		    {
                // Force to be opposite direction so shaders do not use this light
                float dir[4] = {-light->light.dvDirection.x, -light->light.dvDirection.y, -light->light.dvDirection.z, 0.0f};
			    glLightfv(GL_LIGHT0+numlight, GL_POSITION, dir);

                static float black[4] = {0, 0, 0, 0};

			    glLightfv(GL_LIGHT0+numlight, GL_AMBIENT, black);
			    glLightfv(GL_LIGHT0+numlight, GL_DIFFUSE, black);
			    glLightfv(GL_LIGHT0+numlight, GL_SPECULAR, black);

			    oglSetLightState(numlight, FALSE);
		    }
	    }

	    if (!HandleGLError(TRUE))
	    {
		    Write_Debug(" GL error in oglSetLight");
	    }
    }

    return TRUE;
}

/************************************************************************************************
	oglCreateLight
*************************************************************************************************/

SINT32 oglCreateLight(piLIGHT* light)
{
SINT32	numlight;

	if (numlights < gMaxLights)
	{
		memset(&lights[numlights], 0, sizeof(lights[numlights]));
		lights[numlights].light = *light;
		lights[numlights].flags = 1;	// Enable the light
		numlight = numlights++;

		oglSetLight(numlight, &lights[numlight]);

        glLightf(GL_LIGHT0+numlight, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0+numlight, GL_LINEAR_ATTENUATION, 0.0f);
        glLightf(GL_LIGHT0+numlight, GL_QUADRATIC_ATTENUATION, 0.0f);
	}
	else
		return -1;

	return numlight;
}

/************************************************************************************************
	oglReleaseLight
*************************************************************************************************/

LOGICAL oglReleaseLight(SINT32 numlight)
{
	if (numlight < 0 || numlight >= numlights)
	{
		return(FALSE);
	}

	memset(&lights[numlight], 0, sizeof(lights[numlight]));
	oglSetLightState(numlight, FALSE);

	numlights--;

#ifdef DONT_USE
	if (numlight < numlights)
	{
		memcpy(&lights[numlight], &lights[numlight + 1], (numlights - numlight) * sizeof(lights[numlight]));
	}
#endif

	return(TRUE);
}

/************************************************************************************************
	oglSetLightState
*************************************************************************************************/

LOGICAL oglSetLightState(SINT32 numlight, UINT32 State)
{
    if(g_statelocked[piRENDERSTATE_LIGHTING])
    {
        return TRUE;
    }

    if(numlight < 0 || numlight >= numlights)
    {
        return FALSE;
    }

    if(lightstates[numlight] != State)
    {
        if(State)
            glEnable(GL_LIGHT0 + numlight);
        else
            glDisable(GL_LIGHT0 + numlight);

        lightstates[numlight] = State;
    }

    return TRUE;
}

/************************************************************************************************
	oglGetLights
*************************************************************************************************/

SINT32 oglGetLights(LIGHT** lplight)
{
	*lplight = lights;
	return(numlights);
}

/************************************************************************************************
	oglGetAmbLight
*************************************************************************************************/

oglCOLOR oglGetAmbLight(void)
{
#if USE_STATESET
    LightModel* lm = dynamic_cast<LightModel*>(States::GetDefaultAttribute(RS_LIGHTMODEL));
    const Vec4f& amb = lm->GetAmbientColor();
    return piRGBA(amb.r(), amb.g(), amb.b(), amb.a());
#else
	return(renderstates[oglRENDERSTATE_AMBIENT]);
#endif
}

/************************************************************************************************
	oglSetAmbLight
*************************************************************************************************/

void oglSetAmbLight(oglCOLOR color)
{
#if USE_STATESET
#else
	oglSetRenderState(oglRENDERSTATE_AMBIENT, color);
#endif
}

/************************************************************************************************
	oglRenderIndexScene
*************************************************************************************************/

LOGICAL oglRenderIndexScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpVertices, UINT32 VertexCount, void*  lpIndices, UINT32 IndexCount)
{
SINT32		stride;

	switch (oglvt)
	{
		case oglVT_VERTEX:
		{
			oglVERTEX* pv = (oglVERTEX*)lpVertices;
			stride = sizeof(oglVERTEX);
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							&pv->x, stride,
							&pv->nx, stride,
							NULL, 0,
							&pv->tu,stride,
							lpIndices);
		}
		break;

		case oglVT_LVERTEX:
		{
			oglLVERTEX* pv = (oglLVERTEX*)lpVertices;
			stride = sizeof(oglLVERTEX);
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							&pv->x, stride,
							NULL, 0,
							&pv->color, stride,
							&pv->tu, stride,
							lpIndices);
		}
		break;

		case oglVT_TLVERTEX:
		{
			oglTLVERTEX* pv = (oglTLVERTEX*)lpVertices;
			stride = sizeof(oglTLVERTEX);
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							&pv->sx, stride,
							NULL, 0,
							&pv->color, stride,
							&pv->tu, stride,
							lpIndices);
		}
		break;

		case oglVT_CVERTEX:
		{
			piUNIFIED_VERTEX* pv = (piUNIFIED_VERTEX*)lpVertices;
			stride = sizeof(piUNIFIED_VERTEX);
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							&pv->x, stride,
							&pv->nx, stride,
							&pv->color, stride,
							&pv->tu, stride,
							lpIndices);
		}
		break;

		case oglVT_3DVERTEX:
		{
			ogl3DVERTEX*	pv = (ogl3DVERTEX*)lpVertices;
			stride = sizeof(ogl3DVERTEX);
			_RenderScene3D(oglt, oglvt, VertexCount, IndexCount,
							&pv->x, stride,
							&pv->nx, stride,
							&pv->color, stride,
							&pv->tu, stride,
							lpIndices);
		}
		break;
        // Tangent vertex
        case oglVT_TVERTEX:
            {
                oglTVERTEX* pv = (oglTVERTEX*)lpVertices;
			    stride = sizeof(oglTVERTEX);

                RenderScene(*pv, stride, IndexCount, lpIndices);
                break;
            }
	}

	return TRUE;
}

/************************************************************************************************
	oglRenderIndexStridedScene
*************************************************************************************************/

LOGICAL oglRenderIndexStridedScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpStridedData, UINT32 VertexCount, void*  lpIndices, UINT32 IndexCount)
{
oglSTRIDEDDATA* sverts = (oglSTRIDEDDATA*)lpStridedData;

	switch (oglvt)
	{
		case oglVT_VERTEX:
		{
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							sverts->position.pdata, sverts->position.stride,
						  	sverts->normal.pdata, sverts->normal.stride,
							sverts->diffuse.pdata, sverts->diffuse.stride,
							sverts->textureCoords[0].pdata,  sverts->textureCoords[0].stride,
							lpIndices);
		}
		break;

		case oglVT_LVERTEX:
		{
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							sverts->position.pdata, sverts->position.stride,
							NULL, 0,
							sverts->diffuse.pdata, sverts->diffuse.stride,
							sverts->textureCoords[0].pdata,  sverts->textureCoords[0].stride,
							lpIndices);
		}
		break;

		case oglVT_TLVERTEX:
		{
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							sverts->position.pdata, sverts->position.stride,
							NULL, 0,
							sverts->diffuse.pdata, sverts->diffuse.stride,
							sverts->textureCoords[0].pdata,  sverts->textureCoords[0].stride,
							lpIndices);
		}
		break;

		case oglVT_CVERTEX:
		{
			_RenderScene(oglt, oglvt, VertexCount, IndexCount,
							sverts->position.pdata, sverts->position.stride,
							sverts->normal.pdata, sverts->normal.stride,
							sverts->diffuse.pdata, sverts->diffuse.stride,
							sverts->textureCoords[0].pdata,  sverts->textureCoords[0].stride,
							lpIndices);
		}
		break;

		case oglVT_3DVERTEX:
		{
			_RenderScene3D(oglt, oglvt, VertexCount, IndexCount,
							sverts->position.pdata, sverts->position.stride,
							sverts->normal.pdata, sverts->normal.stride,
							sverts->diffuse.pdata, sverts->diffuse.stride,
							sverts->textureCoords[0].pdata,  sverts->textureCoords[0].stride,
							lpIndices);
		}
		break;
        // Tangent vertex
        case oglVT_TVERTEX:
            {
                // TODO
                break;
            }
	}

	return TRUE;
}

/************************************************************************************************
	oglRenderIndexStridedScene
*************************************************************************************************/

LOGICAL	oglRenderStridedScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpStridedData, UINT32 VertexCount)
{
	return oglRenderIndexStridedScene( oglt, oglvt, lpStridedData, VertexCount, NULL, 0);
}

/************************************************************************************************
	oglRenderScene
*************************************************************************************************/

LOGICAL oglRenderScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpVertices, UINT32 VertexCount)
{
	return oglRenderIndexScene(oglt, oglvt, lpVertices, VertexCount, NULL, 0);
}

// Start with a clean slate, since other elements of this app do not go through the
// engine for rendering.
void oglDisableRenderStates()
{
}

void oglDrawScene(UINT32 mode,
                  FLOAT32* coords,
                  FLOAT32* normals,
                  FLOAT32* texcoords,
                  UINT32* colors,
                  UINT16* indices,
                  SINT32 stride,
                  SINT32 vertexcount,
                  SINT32 indexcount)
{
    if(coords)
    {
        ogl::SetVertexPointer(3, GL_FLOAT, stride, coords);
    }

    if(normals)
    {
        ogl::SetNormalPointer(GL_FLOAT, stride, normals);
    }

    if(!piGetEngineRenderState(piRENDERSTATE_LIGHTING) && colors)
    {
        ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, stride, colors);
    }

    if(texcoords)
    {
		ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, texcoords);
    }

    if(indices)
    {
		ogl::DrawElements(mode, indexcount, GL_UNSIGNED_SHORT, indices);
    }
    else
    {
		ogl::DrawArrays(mode, 0, vertexcount);
    }
}

/************************************************************************************************
	_RenderScene
*************************************************************************************************/

#ifdef TESTMAC
//extern "C" void collectvertexptrs(void*p,unsigned int sz);
#endif

static void RenderScene(const oglTVERTEX& pv, int stride, int count, const void* indices)
{
	oglUpdateRenderIndexStats(GL_TRIANGLES, count, count);

    if(ogl::UseDeprecatedPath())
    {
        ogl::SetVertexPointer(3, GL_FLOAT, stride, &pv.x);
        ogl::SetNormalPointer(GL_FLOAT, stride, &pv.nx);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, &pv.tu);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 3, GL_FLOAT, stride, &pv.tx);

        ogl::DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
    }
    else
    {
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, stride, &pv.x);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, stride, &pv.nx);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, stride, &pv.tu);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TANGENT, 3, GL_FLOAT, 0, stride, &pv.tx);

        ogl::DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
    }
}

static void _RenderScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, UINT32 VertexCount, UINT32 IndexCount,
							void* vertex, SINT32 vertexStride, void* normal, SINT32 normalStride,
							void* color, SINT32 colorStride, void* tex, SINT32 texStride, void* index)
{
	oglUpdateRenderIndexStats(oglt, VertexCount, IndexCount);

    if(ogl::UseDeprecatedPath())
    {
	    // Weird Transformed and Lit vertex stuff
	    if (oglvt == oglVT_TLVERTEX)
	    {
		    oglSetRenderState(oglRENDERSTATE_ZENABLE, 0);
		    oglSetMatrixMode(GL_MODELVIEW);
		    glPushMatrix();
		    glLoadIdentity();
		    oglSetMatrixMode(GL_PROJECTION);
		    glPushMatrix();
		    glLoadIdentity();
		    glOrtho((GLfloat)vx, (GLfloat)vx + vwidth, (GLfloat)vy + vheight, (GLfloat)vy, 1.0, -1.0);
	    }

	    // Vertex
	    if (vertex)
	    {
#ifdef TESTMAC
//        collectvertexptrs(vertex,VertexCount);
#endif
		    ogl::SetVertexPointer(3, GL_FLOAT, vertexStride, vertex);
	    }

	    // Normal
	    if(normal)
	    {
            ogl::SetNormalPointer(GL_FLOAT, normalStride, normal);
	    }

#if USE_STATESET
        if(!States::IsModeEnabled(GL_LIGHTING) && color)
        {
            ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, colorStride, color);
        }
#else
	    // Color
        if(!piGetEngineRenderState(piRENDERSTATE_LIGHTING) && color)
        {
            ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, colorStride, color);
        }
#endif

	    // Tex Coord
	    if (tex)
	    {
		    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, texStride, tex);
	    }

	    // And finally draw...
	    if (index)
	    {
		    ogl::DrawElements(oglt, IndexCount, GL_UNSIGNED_SHORT, index);
	    }
	    else
	    {
		    ogl::DrawArrays(oglt, 0, VertexCount);
	    }

	    // Weird Transformed and Lit vertex stuff
	    if (oglvt == oglVT_TLVERTEX)
	    {
            glPopMatrix();
		    oglSetMatrixMode(GL_MODELVIEW);
		    glPopMatrix();
		    oglSetRenderState(oglRENDERSTATE_ZENABLE, 1);
	    }
    }
    else
    {
        // Shader set by calling function

        if(vertex)
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, vertexStride, vertex);

        if(normal)
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_NORMAL, 3, GL_FLOAT, 0, normalStride, normal);

        if(color)
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_UNSIGNED_BYTE, 0, colorStride, color);

        if(tex)
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, texStride, tex);

        if(index)
            ogl::DrawElements(oglt, IndexCount, GL_UNSIGNED_SHORT, index);
        else
            ogl::DrawArrays(oglt, 0, VertexCount);
    }
}

/************************************************************************************************
	_RenderScene3D
*************************************************************************************************/

static void _RenderScene3D(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, UINT32 VertexCount, UINT32 IndexCount,
							void* vertex, SINT32 vertexStride, void* normal, SINT32 normalStride,
							void* color, SINT32 colorStride, void* tex, SINT32 texStride, void* index)
{
	oglUpdateRenderIndexStats(oglt, VertexCount, IndexCount);

	if (oglvt != oglVT_3DVERTEX)
	{
		return;
	}

	// Vertex
	if (vertex)
	{
		ogl::SetVertexPointer(3, GL_FLOAT, vertexStride, vertex);
	}

	// Normal
	if (normal)
	{
		ogl::SetNormalPointer(GL_FLOAT, normalStride, normal);
	}

	// Color
	if (color)
	{
		ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, colorStride, color);
	}

	// Tex Coord
	if (tex)
	{
		ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 3, GL_FLOAT, texStride, tex);
	}

	// And finally draw...
	if (index)
	{
		ogl::DrawElements(oglt, IndexCount, GL_UNSIGNED_SHORT, index);
	}
	else
	{
		ogl::DrawArrays(oglt, 0, VertexCount);
	}

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in _RenderScene");
	}
}

/************************************************************************************************
	_GetNewTextureId
*************************************************************************************************/

static SINT32 _GetNewTextureId(void)
{
    if(g_freeTextures.empty())
    {
	    piExitProgram(__FILE__,__LINE__, "Textures exhausted!");
    }

    systhreadMutexLock(textureMutex);

    TEXTURE* t = g_freeTextures.front();
    g_freeTextures.pop_front();

    // Always keep the id
    SINT32 texid = t->id;
    memset(t, 0, sizeof(TEXTURE));

    t->id = texid;
    t->normalMap = -1;
    t->specularMap = -1;
    t->target = GL_TEXTURE_2D;
    t->numuses = 1;

	systhreadMutexUnlock(textureMutex);

    // Array index is 0 based
    return texid - 1;
}

/************************************************************************************************
	oglGetNewTextureID
*************************************************************************************************/
SINT32 oglGetNewTextureID()
{
    return _GetNewTextureId();
}


/************************************************************************************************
	_GetNewTexture3DId
*************************************************************************************************/

static SINT32 _GetNewTexture3DId(void)
{
SINT32			i, num;
TEXTURE3D*		ptex;

	for (i = 1; i < numtexture3Ds; i++)
	{
		if (texture3Ds[i].numuses == 0)
		{
			memset(&texture3Ds[i], 0, sizeof(TEXTURE3D));
			return(i);
		}
	}

	num = numtexture3Ds + MIN3DTEXTURES;
	ptex = (TEXTURE3D*)memAlloc(sizeof(TEXTURE3D) * num);
	if (ptex == NULL)
	{
		return(-1);
	}

	memset(ptex, 0, sizeof(TEXTURE3D) * num);
	memmove(ptex, texture3Ds, sizeof(TEXTURE3D) * numtexture3Ds);
	memFree(texture3Ds);
	texture3Ds = ptex;
	numtexture3Ds = num;

	return(i);
}

/************************************************************************************************
	_GetNewMaterialId
*************************************************************************************************/

static SINT32 _GetNewMaterialId(void)
{
SINT32			i;
SINT32			num;
MATERIAL*	pmat;

	systhreadMutexLock(materialMutex);

	for (i=0; i < nummaterials; i++)
	{
		if (materials[i].numuses == 0)
		{
			systhreadMutexUnlock(materialMutex);
            materials[i].dirty = true;

			return(i);
		}
	}

	num = nummaterials + MINMATERIALS;
	pmat = (MATERIAL*)memAlloc(sizeof(MATERIAL) * num);
	if (pmat == NULL)
	{
		systhreadMutexUnlock(materialMutex);
		return(-1);
	}

	memmove(pmat, materials, sizeof(MATERIAL) * nummaterials);
	memFree(materials);
	materials = pmat;
	nummaterials = num;

	systhreadMutexUnlock(materialMutex);

	return(i);
}

void oglLockState(SINT32 state, LOGICAL lock)
{
    g_statelocked[state] = lock;
}

void oglLockTextureState(LOGICAL lock)
{
    TextureState::Lock(lock);
}

void oglLockMaterialState(LOGICAL lock)
{
    g_materiallocked = lock;
}

void oglLockWorldMatrix(LOGICAL lock)
{
    _lockWorldMatrix = lock;
}

void oglLockViewMatrix(LOGICAL lock)
{
    _lockViewMatrix = lock;
}

void oglLockUnscaledViewMatrix(LOGICAL lock)
{
    _lockUnscaledViewMatrix = lock;
}

void oglLockCockpitMatrix(LOGICAL lock)
{
    _lockCockpitMatrix = lock;
}

void oglLockProjectionMatrix(LOGICAL lock)
{
    _lockProjectionMatrix = lock;
}

void oglLockTextureMatrix(LOGICAL lock)
{
    _lockTextureMatrix = lock;
}

void oglCreateModelView(piMATRIX* returnMatrix,
                        piVECTOR* eye,
                        piVECTOR* center,
                        piVECTOR* up,
                        piQUAT* axisangle) // w = angle in degrees, xyz = axis
{
    GLenum mode = oglGetMatrixMode();

    oglSetMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    {
        glLoadIdentity();

        if(axisangle)
        {
            glRotatef(axisangle->w, axisangle->x, axisangle->y, axisangle->z);
        }
        else
        {
            // Crazy game coordinate stuff
            glRotatef(180, 1, 0, 0);
        }

        gluLookAt(eye->x, eye->y, eye->z,
                  center->x, center->y, center->z,
                  up->x, up->y, up->z);
        glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)returnMatrix);
    }
    glPopMatrix();

    oglSetMatrixMode(mode);
}

SINT32 oglGetActiveMaterial()
{
    return _activeMaterial;
}

#endif

