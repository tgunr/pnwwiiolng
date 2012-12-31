#pragma once

	// Defines
//	#define MAXMIPLEVELS			11
	#define NO_LOCK_TO_REFRESH		0
	#define LOCK_TO_REFRESH			1


	// Typedefs
	struct oglVECTOR
	{
		GLfloat	x;
		GLfloat	y;
		GLfloat	z;

	} ;

	struct oglLOCKED_RECT
	{
		int			Pitch;
		void*		pBits;

	} ;

	typedef UINT32 oglMATERIALHANDLE;
	typedef UINT32 oglTEXTUREHANDLE, *oglLPTEXTUREHANDLE;
	typedef UINT32 oglCOLOR, *oglLPCOLOR;

	enum
	{
		oglFMT_RGB			= GL_RGB,		// No specific color depth but without alpha
		oglFMT_RGBA			= GL_RGBA,		// No specific color depth and with alpha
		oglFMT_R3G3B2		= GL_R3_G3_B2,
		oglFMT_X4R4G4B4		= GL_RGB4,
		oglFMT_X1R5G5B5		= GL_RGB5,
		oglFMT_X8R8G8B8		= GL_RGB8,
		oglFMT_X2R10G10B10	= GL_RGB10,
		oglFMT_X12R12G12B12	= GL_RGB12,
		oglFMT_X16R16G16B16	= GL_RGB16,
		oglFMT_A2R2G2B2		= GL_RGBA2,
		oglFMT_A4R4G4B4		= GL_RGBA4,
		oglFMT_A1R5G5B5		= GL_RGB5_A1,
		oglFMT_A8R8G8B8		= GL_RGBA8,
		oglFMT_A2R10G10B10	= GL_RGB10_A2,
		oglFMT_A12R12G12B12	= GL_RGBA12,
		oglFMT_A16R16G16B16	= GL_RGBA16
	};


	// Render state constants
	#define oglFILL_POINT						GL_POINT
	#define oglFILL_WIREFRAME					GL_LINE
	#define oglFILL_SOLID						GL_FILL

	#define oglTADDRESS_WRAP					GL_REPEAT
	#define oglTADDRESS_CLAMP					GL_CLAMP

	#define oglSHADE_FLAT						GL_FLAT
	#define oglSHADE_GOURAUD					GL_SMOOTH
	#define oglSHADE_PHONG						GL_SMOOTH

	#define oglFILTER_NEAREST					GL_NEAREST
	#define oglFILTER_LINEAR					GL_LINEAR
	#define oglFILTER_NEARESTMIPNEAREST			GL_NEAREST_MIPMAP_NEAREST
	#define oglFILTER_NEARESTMIPLINEAR			GL_NEAREST_MIPMAP_LINEAR
	#define oglFILTER_LINEARMIPNEAREST			GL_LINEAR_MIPMAP_NEAREST
	#define oglFILTER_LINEARMIPLINEAR			GL_LINEAR_MIPMAP_LINEAR

	#define oglBLEND_ZERO						GL_ZERO
	#define oglBLEND_ONE						GL_ONE
	#define oglBLEND_SRCCOLOR					GL_SRC_COLOR
	#define oglBLEND_INVSRCCOLOR				GL_ONE_MINUS_SRC_COLOR
	#define oglBLEND_SRCALPHA					GL_SRC_ALPHA
	#define oglBLEND_INVSRCALPHA				GL_ONE_MINUS_SRC_ALPHA
	#define oglBLEND_DESTALPHA					GL_DST_ALPHA
	#define oglBLEND_INVDESTALPHA				GL_ONE_MINUS_DST_ALPHA
	#define oglBLEND_DESTCOLOR					GL_DST_COLOR
	#define oglBLEND_INVDESTCOLOR				GL_ONE_MINUS_DST_COLOR
	#define oglBLEND_CONSTANTCOLOR				GL_CONSTANT_COLOR
	#define oglBLEND_INVCONSTANTCOLOR			GL_ONE_MINUS_CONSTANT_COLOR
	#define oglBLEND_CONSTANTALPHA				GL_CONSTANT_ALPHA
	#define oglBLEND_INVCONSTANTALPHA			GL_ONE_MINUS_CONSTANT_ALPHA
	#define oglBLEND_SRCALPHASAT				GL_SRC_ALPHA_SATURATE

	// Source availability flags
	#define oglSRCBLENDCAPS_ZERO				0x00000001
	#define oglSRCBLENDCAPS_ONE					0x00000002
	#define oglSRCBLENDCAPS_SRCALPHA			0x00000004
	#define oglSRCBLENDCAPS_INVSRCALPHA			0x00000008
	#define oglSRCBLENDCAPS_DSTALPHA			0x00000010
	#define oglSRCBLENDCAPS_INVDSTALPHA			0x00000020
	#define oglSRCBLENDCAPS_DSTCOLOR			0x00000040
	#define oglSRCBLENDCAPS_INVDSTCOLOR			0x00000080
	#define oglSRCBLENDCAPS_SRCALPHASAT			0x00000100
	#define oglSRCBLENDCAPS_CONSTANTCOLOR		0x00000200
	#define oglSRCBLENDCAPS_INVCONSTANTCOLOR	0x00000400
	#define oglSRCBLENDCAPS_CONSTANTALPHA		0x00000800
	#define oglSRCBLENDCAPS_INVCONSTANTALPHA	0x00001000
	#define oglSRCBLENDCAPS_ALL					0x000001ff
	#define oglSRCBLENDCAPS_IMAGING_ALL			0x00001e00

	// Destination availability flags
	#define oglDSTBLENDCAPS_ZERO				0x00000001
	#define oglDSTBLENDCAPS_ONE					0x00000002
	#define oglDSTBLENDCAPS_SRCCOLOR			0x00000004
	#define oglDSTBLENDCAPS_INVSRCCOLOR			0x00000008
	#define oglDSTBLENDCAPS_SRCALPHA			0x00000010
	#define oglDSTBLENDCAPS_INVSRCALPHA			0x00000020
	#define oglDSTBLENDCAPS_DSTALPHA			0x00000040
	#define oglDSTBLENDCAPS_INVDSTALPHA			0x00000080
	#define oglDSTBLENDCAPS_CONSTANTCOLOR		0x00000100
	#define oglDSTBLENDCAPS_INVCONSTANTCOLOR	0x00000200
	#define oglDSTBLENDCAPS_CONSTANTALPHA		0x00000400
	#define oglDSTBLENDCAPS_INVCONSTANTALPHA	0x00000800
	#define oglDSTBLENDCAPS_ALL					0x000000ff
	#define oglDSTBLENDCAPS_IMAGING_ALL			0x00000f00

	// Cull constants
	#define oglCULL_CW							GL_CW
	#define oglCULL_CCW							GL_CCW

	// Blend constants
	#define oglTBLEND_DECAL						GL_DECAL
	#define oglTBLEND_MODULATE					GL_MODULATE
	#define oglTBLEND_BLEND						GL_BLEND
	#define oglTBLEND_REPLACE					GL_REPLACE

	// Function comparison constants
	#define oglCMP_NEVER						GL_NEVER
	#define oglCMP_LESS							GL_LESS
	#define oglCMP_EQUAL						GL_EQUAL
	#define oglCMP_LESSEQUAL					GL_LEQUAL
	#define oglCMP_GREATER						GL_GREATER
	#define oglCMP_NOTEQUAL						GL_NOTEQUAL
	#define oglCMP_GREATEREQUAL					GL_GEQUAL
	#define oglCMP_ALWAYS						GL_ALWAYS

	// Fog constants
	#define oglFOG_NONE							0
	#define oglFOG_EXP							GL_EXP
	#define oglFOG_EXP2							GL_EXP2
	#define oglFOG_LINEAR						GL_LINEAR

	// Viewport, ZBuffer, Stencil and Accumulation constants
	#define oglCLEAR_VIEWPORT					1
	#define oglCLEAR_ZBUFFER					2
	#define oglCLEAR_STENCIL					4
	#define oglCLEAR_ACCUM						8

	enum oglRENDERSTATETYPES
	{
		oglRENDERSTATE_TEXTUREHANDLE      = 1,    /* Texture handle for legacy interfaces (Texture,Texture2) */
		oglRENDERSTATE_ANTIALIAS          = 2,    /* oglANTIALIASMODE */
		oglRENDERSTATE_TEXTUREADDRESS     = 3,    /* oglTEXTUREADDRESS  */
		oglRENDERSTATE_TEXTUREPERSPECTIVE = 4,    /* TRUE for perspective correction */
		oglRENDERSTATE_WRAPS              = 5,    /* TRUE for wrapping in u */
		oglRENDERSTATE_WRAPT              = 6,    /* TRUE for wrapping in v */
		oglRENDERSTATE_ZENABLE            = 7,    /* oglZBUFFERTYPE (or TRUE/FALSE for legacy) */
		oglRENDERSTATE_FILLMODE           = 8,    /* oglFILL_MODE        */
		oglRENDERSTATE_SHADEMODE          = 9,    /* oglSHADEMODE */
		oglRENDERSTATE_LINEPATTERN        = 10,   /* oglLINEPATTERN */
		oglRENDERSTATE_MONOENABLE         = 11,   /* TRUE to enable mono rasterization */
		oglRENDERSTATE_ROP2               = 12,   /* ROP2 */
		oglRENDERSTATE_PLANEMASK          = 13,   /* DWORD physical plane mask */
		oglRENDERSTATE_ZWRITEENABLE       = 14,   /* TRUE to enable z writes */
		oglRENDERSTATE_ALPHATESTENABLE    = 15,   /* TRUE to enable alpha tests */
		oglRENDERSTATE_LASTPIXEL          = 16,   /* TRUE for last-pixel on lines */
		oglRENDERSTATE_TEXTUREMAG         = 17,   /* oglTEXTUREFILTER */
		oglRENDERSTATE_TEXTUREMIN         = 18,   /* oglTEXTUREFILTER */
		oglRENDERSTATE_SRCBLEND           = 19,   /* oglBLEND */
		oglRENDERSTATE_DESTBLEND          = 20,   /* oglBLEND */
		oglRENDERSTATE_TEXTUREMAPBLEND    = 21,   /* oglTEXTUREBLEND */
		oglRENDERSTATE_CULLMODE           = 22,   /* oglCULL */
		oglRENDERSTATE_ZFUNC              = 23,   /* oglCMPFUNC */
		oglRENDERSTATE_ALPHAREF           = 24,   /* oglFIXED */
		oglRENDERSTATE_ALPHAFUNC          = 25,   /* oglCMPFUNC */
		oglRENDERSTATE_DITHERENABLE       = 26,   /* TRUE to enable dithering */
		oglRENDERSTATE_ALPHABLENDENABLE   = 27,   /* TRUE to enable alpha blending */
		oglRENDERSTATE_FOGENABLE          = 28,   /* TRUE to enable fog */
		oglRENDERSTATE_SPECULARENABLE     = 29,   /* TRUE to enable specular */
		oglRENDERSTATE_ZVISIBLE           = 30,   /* TRUE to enable z checking */
		oglRENDERSTATE_SUBPIXEL           = 31,   /* TRUE to enable subpixel correction */
		oglRENDERSTATE_SUBPIXELX          = 32,   /* TRUE to enable correction in X only */
		oglRENDERSTATE_STIPPLEDALPHA      = 33,   /* TRUE to enable stippled alpha */
		oglRENDERSTATE_FOGCOLOR           = 34,   /* oglCOLOR */
		oglRENDERSTATE_FOGTABLEMODE       = 35,   /* oglFOGMODE */
		oglRENDERSTATE_FOGTABLESTART      = 36,   /* Fog table start    */
		oglRENDERSTATE_FOGTABLEEND        = 37,   /* Fog table end      */
		oglRENDERSTATE_FOGTABLEDENSITY    = 38,   /* Fog table density  */
		oglRENDERSTATE_STIPPLEENABLE      = 39,   /* TRUE to enable stippling */
		oglRENDERSTATE_EDGEANTIALIAS      = 40,   /* TRUE to enable edge antialiasing */
		oglRENDERSTATE_COLORKEYENABLE     = 41,   /* TRUE to enable source colorkeyed textures */
		oglRENDERSTATE_BORDERCOLOR        = 43,   /* Border color for texturing w/border */
		oglRENDERSTATE_TEXTUREADDRESSU    = 44,   /* Texture addressing mode for U coordinate */
		oglRENDERSTATE_TEXTUREADDRESSV    = 45,   /* Texture addressing mode for V coordinate */
		oglRENDERSTATE_MIPMAPLODBIAS      = 46,   /* oglVALUE Mipmap LOD bias */
		oglRENDERSTATE_ZBIAS              = 47,   /* LONG Z bias */

		oglRENDERSTATE_ANISOTROPY         = 49,   /* Max. anisotropy. 1 = no anisotropy */
		oglRENDERSTATE_FLUSHBATCH         = 50,   /* Explicit flush for DP batching (DX5 Only) */
		oglRENDERSTATE_TRANSLUCENTSORTINDEPENDENT=51, /* BOOL enable sort-independent transparency */
		oglRENDERSTATE_STENCILENABLE      = 52,   /* BOOL enable/disable stenciling */
		oglRENDERSTATE_STENCILFAIL        = 53,   /* oglSTENCILOP to do if stencil test fails */
		oglRENDERSTATE_STENCILZFAIL       = 54,   /* oglSTENCILOP to do if stencil test passes and Z test fails */
		oglRENDERSTATE_STENCILPASS        = 55,   /* oglSTENCILOP to do if both stencil and Z tests pass */
		oglRENDERSTATE_STENCILFUNC        = 56,   /* oglCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
		oglRENDERSTATE_STENCILREF         = 57,   /* Reference value used in stencil test */
		oglRENDERSTATE_STENCILMASK        = 58,   /* Mask value used in stencil test */
		oglRENDERSTATE_STENCILWRITEMASK   = 59,   /* Write mask applied to values written to stencil buffer */
		oglRENDERSTATE_TEXTUREFACTOR      = 60,   /* oglCOLOR used for multi-texture blend */
		oglRENDERSTATE_STIPPLEPATTERN00   = 64,   /* Stipple pattern 01...  */
		oglRENDERSTATE_STIPPLEPATTERN01   = 65,
		oglRENDERSTATE_STIPPLEPATTERN02   = 66,
		oglRENDERSTATE_STIPPLEPATTERN03   = 67,
		oglRENDERSTATE_STIPPLEPATTERN04   = 68,
		oglRENDERSTATE_STIPPLEPATTERN05   = 69,
		oglRENDERSTATE_STIPPLEPATTERN06   = 70,
		oglRENDERSTATE_STIPPLEPATTERN07   = 71,
		oglRENDERSTATE_STIPPLEPATTERN08   = 72,
		oglRENDERSTATE_STIPPLEPATTERN09   = 73,
		oglRENDERSTATE_STIPPLEPATTERN10   = 74,
		oglRENDERSTATE_STIPPLEPATTERN11   = 75,
		oglRENDERSTATE_STIPPLEPATTERN12   = 76,
		oglRENDERSTATE_STIPPLEPATTERN13   = 77,
		oglRENDERSTATE_STIPPLEPATTERN14   = 78,
		oglRENDERSTATE_STIPPLEPATTERN15   = 79,
		oglRENDERSTATE_STIPPLEPATTERN16   = 80,
		oglRENDERSTATE_STIPPLEPATTERN17   = 81,
		oglRENDERSTATE_STIPPLEPATTERN18   = 82,
		oglRENDERSTATE_STIPPLEPATTERN19   = 83,
		oglRENDERSTATE_STIPPLEPATTERN20   = 84,
		oglRENDERSTATE_STIPPLEPATTERN21   = 85,
		oglRENDERSTATE_STIPPLEPATTERN22   = 86,
		oglRENDERSTATE_STIPPLEPATTERN23   = 87,
		oglRENDERSTATE_STIPPLEPATTERN24   = 88,
		oglRENDERSTATE_STIPPLEPATTERN25   = 89,
		oglRENDERSTATE_STIPPLEPATTERN26   = 90,
		oglRENDERSTATE_STIPPLEPATTERN27   = 91,
		oglRENDERSTATE_STIPPLEPATTERN28   = 92,
		oglRENDERSTATE_STIPPLEPATTERN29   = 93,
		oglRENDERSTATE_STIPPLEPATTERN30   = 94,
		oglRENDERSTATE_STIPPLEPATTERN31   = 95,

		/*
		* 128 values [128, 255] are reserved for texture coordinate wrap flags.
		* These are constructed with the oglWRAP_U and oglWRAP_V macros. Using
		* a flags word preserves forward compatibility with texture coordinates
		* that are >2D.
		*/
		oglRENDERSTATE_WRAP0              = 128,  /* wrap for 1st texture coord. set */
		oglRENDERSTATE_WRAP1              = 129,  /* wrap for 2nd texture coord. set */
		oglRENDERSTATE_WRAP2              = 130,  /* wrap for 3rd texture coord. set */
		oglRENDERSTATE_WRAP3              = 131,  /* wrap for 4th texture coord. set */
		oglRENDERSTATE_WRAP4              = 132,  /* wrap for 5th texture coord. set */
		oglRENDERSTATE_WRAP5              = 133,  /* wrap for 6th texture coord. set */
		oglRENDERSTATE_WRAP6              = 134,  /* wrap for 7th texture coord. set */
		oglRENDERSTATE_WRAP7              = 135,  /* wrap for 8th texture coord. set */

        oglRENDERSTATE_CLIPPING					= 136,
		oglRENDERSTATE_LIGHTING					= 137,
		oglRENDERSTATE_EXTENTS					= 138,
		oglRENDERSTATE_AMBIENT					= 139,
		oglRENDERSTATE_FOGVERTEXMODE			= 140,
		oglRENDERSTATE_COLORVERTEX				= 141,
		oglRENDERSTATE_LOCALVIEWER				= 142,
		oglRENDERSTATE_NORMALIZENORMALS			= 143,
		oglRENDERSTATE_COLORKEYBLENDENABLE		= 144,
		oglRENDERSTATE_DIFFUSEMATERIALSOURCE	= 145,
		oglRENDERSTATE_SPECULARMATERIALSOURCE	= 146,
		oglRENDERSTATE_AMBIENTMATERIALSOURCE	= 147,
		oglRENDERSTATE_EMISSIVEMATERIALSOURCE	= 148,
		oglRENDERSTATE_VERTEXBLEND				= 151,
		oglRENDERSTATE_CLIPPLANEENABLE			= 152,

		oglRENDERSTATE_BLENDOP					= 171,
		oglRENDERSTATE_CLIENT_TEXTURE			= 173,

        oglRENDERSTATE_ENABLE_VERTEX_ARRAY      = 174,
        oglRENDERSTATE_ENABLE_NORMAL_ARRAY      = 175,
        oglRENDERSTATE_ENABLE_COLOR_ARRAY       = 176,
        oglRENDERSTATE_COLORMASK                = 177,
        oglRENDERSTATE_CLEARCOLOR               = 178,
        oglRENDERSTATE_COLOR4F                  = 179,
        oglRENDERSTATE_CULLFACEENABLE           = 180,
        oglRENDERSTATE_CULLFACE                 = 181,
        oglRENDERSTATE_RASTERIZER_DISCARD       = 182,
        oglRENDERSTATE_DRAWBUFFER               = 183,
        oglRENDERSTATE_READBUFFER               = 184,

        // If reached 255, then increase MAXRENDERSTATES in ogl.cpp
		oglRENDERSTATE_FORCE_DWORD        = 0x7fffffff /* force 32-bit size enum */

	} ;

	// these new guys need to be taken care of...

	enum oglRENDERSTATEFOGTYPE
	{
		oglRENDERSTATE_FOGMODE			= 35,
		oglRENDERSTATE_FOGSTART			= 36,
		oglRENDERSTATE_FOGEND			= 37,
		oglRENDERSTATE_FOGDENSITY		= 38

	} ;

	enum oglBLENDOPSTATETYPES
	{
		oglBLENDOP_ADD              = GL_FUNC_ADD,
		oglBLENDOP_SUBTRACT         = GL_FUNC_SUBTRACT,
		oglBLENDOP_REVSUBTRACT      = GL_FUNC_REVERSE_SUBTRACT,
		oglBLENDOP_MIN              = GL_MIN,
		oglBLENDOP_MAX              = GL_MAX

	} ;

	enum oglPRIMITIVETYPE
	{
		oglPT_POINTLIST		= GL_POINTS,
		oglPT_LINELIST		= GL_LINES,
		oglPT_LINESTRIP		= GL_LINE_STRIP,
		oglPT_TRIANGLELIST	= GL_TRIANGLES,
		oglPT_TRIANGLESTRIP	= GL_TRIANGLE_STRIP,
		oglPT_TRIANGLEFAN	= GL_TRIANGLE_FAN

	} ;

	// Combine RGB/ALPHA texture functions
	enum oglTSS_COMBINE_TEXTURE
	{
		oglTSS_COMBINE		= GL_COMBINE_ARB,
		oglTSS_REPLACE		= GL_REPLACE,
		oglTSS_MODULATE		= GL_MODULATE,
		oglTSS_ADD			= GL_ADD,
		oglTSS_ADD_SIGNED	= GL_ADD_SIGNED_ARB,
		oglTSS_INTERPOLATE	= GL_INTERPOLATE_ARB,
		oglTSS_SUBTRACT		= GL_SUBTRACT_ARB,
		oglTSS_DOT3_RGB		= GL_DOT3_RGB_ARB,		// Use for RGB only
		oglTSS_DOT3_RGBA	= GL_DOT3_RGBA_ARB		// Use for RGB only

	} ;

	// Source 'n' RGB/ALPHA
	enum oglTSS_SOURCE_RGB
	{
		oglTSS_TEXTURE			= GL_TEXTURE,
		oglTSS_CONSTANT			= GL_CONSTANT_ARB,
		oglTSS_PRIMARY_COLOR	= GL_PRIMARY_COLOR_ARB,
		oglTSS_PREVIOUS			= GL_PREVIOUS_ARB

	} ;

	// Operand 'n' RGB/ALPHA
	enum oglTSS_OPERAND_RGB
	{
		oglTSS_SRC_COLOR				= GL_SRC_COLOR,				// Use for RGB only
		oglTSS_SRC_ONE_MINUS_SRC_COLOR	= GL_ONE_MINUS_SRC_COLOR,	// Use for RGB only
		oglTSS_SRC_ALPHA				= GL_SRC_ALPHA,				// Use for RGB and ALPHA
		oglTSS_SRC_ONE_MINUS_SRC_ALPHA	= GL_ONE_MINUS_SRC_ALPHA	// Use for RGB and ALPHA

	} ;

	// Texture stage states combine modes
	enum oglTEXTURESTAGESTATETYPE
	{
		oglTSS_TEXTURE_ENV_MODE,
		oglTSS_COMBINE_RGB,
		oglTSS_COMBINE_ALPHA,
		oglTSS_SOURCE0_RGB,
		oglTSS_SOURCE1_RGB,
		oglTSS_SOURCE2_RGB,
		oglTSS_OPERAND0_RGB,
		oglTSS_OPERAND1_RGB,
		oglTSS_OPERAND2_RGB,
		oglTSS_SOURCE0_ALPHA,
		oglTSS_SOURCE1_ALPHA,
		oglTSS_SOURCE2_ALPHA,
		oglTSS_OPERAND0_ALPHA,
		oglTSS_OPERAND1_ALPHA,
		oglTSS_OPERAND2_ALPHA,
		oglTSS_ADDRESSU,
		oglTSS_ADDRESSV,
		oglTSS_ADDRESSW,
		oglTSS_MAGFILTER,
		oglTSS_MINFILTER,
		oglTSS_MIPFILTER,
        oglTSS_ENABLE_TEXCOORD_ARRAY,
        oglTSS_ENABLE_TEXTURE2D

	} ;

	/*
	* Values for COLORARG1,2 and ALPHAARG1,2 texture blending operations
	* set in texture processing stage controls.
	*/
	#define oglTA_SELECTMASK			0x0000000f  // mask for arg selector
	#define oglTA_DIFFUSE				0x00000000  // select diffuse color
	#define oglTA_CURRENT				0x00000001  // select result of previous stage
	#define oglTA_TEXTURE				0x00000002  // select texture color
	#define oglTA_TFACTOR				0x00000003  // select RENDERSTATE_TEXTUREFACTOR
	#define oglTA_COMPLEMENT			0x00000010  // take 1.0 - x
	#define oglTA_ALPHAREPLICATE		0x00000020  // replicate alpha to color components

	enum
	{
		oglSTENCILOP_KEEP           = 1,
		oglSTENCILOP_ZERO           = 2,
		oglSTENCILOP_REPLACE        = 3,
		oglSTENCILOP_INCRSAT        = 4,
		oglSTENCILOP_DECRSAT        = 5,
		oglSTENCILOP_INVERT         = 6,
		oglSTENCILOP_INCR           = 7,
		oglSTENCILOP_DECR           = 8
	};

	#define oglZB_FALSE				0	// FALSE
	#define oglZB_TRUE				1	// TRUE
	#define oglZB_USEW				2	// USEW

	// Vertex types
	enum oglVERTEXTYPE
	{
		oglVT_VERTEX       	= 0,
		oglVT_LVERTEX       = 1,
		oglVT_TLVERTEX      = 2,
		oglVT_CVERTEX	 	= 3,
		oglVT_GVERTEX		= oglVT_VERTEX,
		oglVT_3DVERTEX		= 4,
        oglVT_TVERTEX       = 5 // Tangent vertex

	} ;

	struct oglUV
	{
		FLOAT32   u;
		FLOAT32   v;

	} ;

	struct oglUVW
	{
		FLOAT32   u;
		FLOAT32   v;
		FLOAT32   w;

	} ;

	struct oglTLVERTEX
	{
		FLOAT32		sx;             /* Screen coordinates */
		FLOAT32		sy;
		FLOAT32		sz;
		FLOAT32		rhw;            /* Reciprocal of homogeneous w */
		UINT32		color;          /* Vertex color */
		UINT32		specular;       /* Specular component of vertex */
		FLOAT32		tu;             /* Texture coordinates */
		FLOAT32		tv;

	} ;

	struct oglLVERTEX
	{
		FLOAT32		x;             /* Homogeneous coordinates */
		FLOAT32		y;
		FLOAT32		z;
		UINT32		reserved;
		UINT32		color;         /* Vertex color */
		UINT32		specular;      /* Specular component of vertex */
		FLOAT32		tu;            /* Texture coordinates */
		FLOAT32		tv;

	} ;

	struct oglVERTEX
	{
		FLOAT32     x;             /* Homogeneous coordinates */
		FLOAT32     y;
		FLOAT32     z;
		FLOAT32     nx;            /* Normal */
		FLOAT32     ny;
		FLOAT32     nz;
		FLOAT32     tu;            /* Texture coordinates */
		FLOAT32     tv;

	} ;

	struct ogl3DVERTEX
	{
		FLOAT32     x;             /* Homogeneous coordinates */
		FLOAT32     y;
		FLOAT32     z;
		FLOAT32     nx;            /* Normal */
		FLOAT32     ny;
		FLOAT32     nz;
		UINT32		color;
		FLOAT32     tu;            /* Texture coordinates */
		FLOAT32     tv;
		FLOAT32     tw;

	} ;

	// "Unified" Vertex structure
	struct oglCVERTEX
	{
		FLOAT32     x;             /* Homogeneous coordinates */
		FLOAT32     y;
		FLOAT32     z;
		FLOAT32     nx;            /* Normal */
		FLOAT32     ny;
		FLOAT32     nz;
		UINT32		color;
		UINT32		specular;
		FLOAT32     tu;            /* Texture coordinates */
		FLOAT32     tv;

	} ;

	struct oglGVERTEX
	{
		float     x;             /* Homogeneous coordinates */
		float     y;
		float     z;
		float     nx;            /* Normal */
		float     ny;
		float     nz;
		float     tu;            /* Texture coordinates */
		float     tv;

	} ;

	struct oglTVERTEX
	{
		float     x;             /* Homogeneous coordinates */
		float     y;
		float     z;
		float     nx;            /* Normal */
		float     ny;
		float     nz;
        float     tx;            /* Tangent */
        float     ty;
        float     tz;
		float     tu;            /* Texture coordinates */
		float     tv;

	} ;

	struct oglPTRSTRIDE
	{
		LPVOID	pdata;
		UINT32	stride;

	} ;

	#define oglMAXTEXCOORD 8

	struct oglSTRIDEDDATA
	{
		oglPTRSTRIDE	position;
		oglPTRSTRIDE	normal;
		oglPTRSTRIDE	diffuse;
		oglPTRSTRIDE	specular;
		oglPTRSTRIDE	textureCoords[oglMAXTEXCOORD];
		oglPTRSTRIDE	indices;

	} ;

	struct oglCOLORVALUE
	{
		FLOAT32		r;
		FLOAT32		g;
		FLOAT32		b;
		FLOAT32		a;

	} ;

	struct oglMATERIAL
	{
		oglCOLORVALUE	Diffuse;        /* Diffuse color RGBA */
		oglCOLORVALUE	Ambient;        /* Ambient color RGB */
		oglCOLORVALUE	Specular;       /* Specular 'shininess' */
		oglCOLORVALUE	Emissive;       /* Emissive color RGB */
		FLOAT32			Power;          /* Sharpness if specular highlight */

	} ;

	#define INVALID_LIGHT			-1
	#define INVALID_INDEXSOURCE		-1
	#define INVALID_VERTEXSOURCE	-1
	#define INVALID_MATERIAL		-1
	#define INVALID_TEXTURE			-1

	// mipmapping constants
	#define CRT_FLAG_NONE       0x00000000
	#define CRT_FLAG_HARDWARE   0x00000001
	#define CRT_FLAG_HDWMIPMAP	0x00000002
	#define CRT_FLAG_MIPMAP    	0x00000002
	#define CRT_FLAG_WRITABLE	0x00000004

	// Typedefs

	// oglPIXELFORMAT
	struct oglPIXELFORMAT
	{
		UINT32	dwSize;			// size of structure
		UINT32	dwFlags;		// pixel format flags
		UINT32	dwFourCC;		// (FOURCC code)

		union
		{
			UINT32	BitCount;
			UINT32	dwRGBBitCount;		// how many bits per pixel
		};

		union
		{
			UINT32	RedBitMask;
			UINT32	dwRBitMask;		// mask for red bit
		};

		union
		{
			UINT32	GreenBitMask;
			UINT32	dwGBitMask;		// mask for green bits
		};

		union
		{
			UINT32	BlueBitMask;
			UINT32	dwBBitMask;		// mask for blue bits
		};

		union
		{
			UINT32	AlphaBitMask;
			UINT32	dwRGBAlphaBitMask;	// mask for alpha channel
		};

	} ;

	// oglSURFACEDESC
	struct oglSURFACEDESC
	{
		UINT32		Height;				// height of surface to be created
		UINT32		Width;				// width of input surface
		SINT32		Pitch;				// distance to start of next line (return value only)
		void*		Surface;			// pointer to the associated surface memory
		oglPIXELFORMAT	PixelFormat;	// pixel format description of the surface

	} ;

	struct TEXTUREFORMAT
	{
		SINT8					formatname[32];
		UINT32					format;
		SINT32					bpp;
		SINT32					red, green, blue, alpha;
		SINT32					unused, lum, pal;

	} ;

