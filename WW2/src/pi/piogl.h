#pragma once

#if platBuild == TEUL_SERVER
static_assert( platBuild != TEUL_SERVER, "Can't include piogl.h in a server build." );
#endif

#include "gl/glew.h"
#include "gl/glplat.h"

#define USE_LINEAR_FOG
//#define USE_EXP_FOG
//#define USE_EXP2_FOG

//#define MAXMIPLEVELS 16
#define MINDYNALITES 16

// mipmapping constants
#define CRT_FLAG_NONE       0x00000000
#define CRT_FLAG_HARDWARE   0x00000001
#define CRT_FLAG_HDWMIPMAP	0x00000002
#define CRT_FLAG_MIPMAP    	0x00000002
#define CRT_FLAG_WRITABLE	0x00000004

// Render state constants

#define piFILL_POINT                      GL_POINT
#define piFILL_WIREFRAME                  GL_LINE
#define piFILL_SOLID                      GL_FILL
#define piTADDRESS_WRAP                   GL_REPEAT
#define piTADDRESS_CLAMP                  GL_CLAMP
#define piSHADE_FLAT                      GL_FLAT
#define piSHADE_GOURAUD                   GL_SMOOTH
#define piSHADE_PHONG					  GL_SMOOTH
#define piBLEND_ZERO                      GL_ZERO
#define piBLEND_ONE                       GL_ONE
#define piBLEND_SRCCOLOR                  GL_SRC_COLOR
#define piBLEND_INVSRCCOLOR               GL_ONE_MINUS_SRC_COLOR
#define piBLEND_SRCALPHA                  GL_SRC_ALPHA
#define piBLEND_INVSRCALPHA               GL_ONE_MINUS_SRC_ALPHA
#define piBLEND_DESTALPHA                 GL_DST_ALPHA
#define piBLEND_INVDESTALPHA              GL_ONE_MINUS_DST_ALPHA
#define piBLEND_DESTCOLOR                 GL_DST_COLOR
#define piBLEND_INVDESTCOLOR              GL_ONE_MINUS_DST_COLOR
#define piBLEND_SRCALPHASAT               GL_SRC_ALPHA_SATURATE
#define piBLEND_BOTHSRCALPHA			  piBLEND_SRCALPHA
#define piBLEND_BOTHINVSRCALPHA			  piBLEND_INVSRCALPHA

#define piBLENDCAPS_ZERO						0x00000001
#define piBLENDCAPS_ONE							0x00000002
#define piBLENDCAPS_SRCCOLOR					0x00000004
#define piBLENDCAPS_INVSRCCOLOR					0x00000008
#define piBLENDCAPS_SRCALPHA					0x00000010
#define piBLENDCAPS_INVSRCALPHA					0x00000020
#define piBLENDCAPS_DESTALPHA					0x00000040
#define piBLENDCAPS_INVDESTALPHA				0x00000080
#define piBLENDCAPS_DESTCOLOR					0x00000100
#define piBLENDCAPS_INVDESTCOLOR				0x00000200
#define piBLENDCAPS_SRCALPHASAT					0x00000400
#define piBLENDCAPS_BOTHSRCALPHA				0x00000800
#define piBLENDCAPS_BOTHINVSRCALPHA				0x00001000

#define piDXT3 0
#define piDXT1 0

#define piCULL_CW                         GL_CW
#define piCULL_CCW                        GL_CCW
#define piTBLEND_DECAL                    GL_DECAL
#define piTBLEND_MODULATE                 GL_MODULATE
#define piTBLEND_BLEND                    GL_BLEND
#define piCMP_NEVER                       GL_NEVER
#define piCMP_LESS                        GL_LESS
#define piCMP_EQUAL                       GL_EQUAL
#define piCMP_LESSEQUAL                   GL_LEQUAL
#define piCMP_GREATER                     GL_GREATER
#define piCMP_NOTEQUAL                    GL_NOTEQUAL
#define piCMP_GREATEREQUAL                GL_GEQUAL
#define piCMP_ALWAYS                      GL_ALWAYS
#define piFOG_NONE                        0
#define piFOG_EXP                         GL_EXP
#define piFOG_EXP2                        GL_EXP2
#define piFOG_LINEAR                      GL_LINEAR

#define piCULLFACEENABLE_DEFAULT          1

#define piANTIALIAS_NONE                  0//D3DANTIALIAS_NONE
#define piANTIALIAS_SORTDEPENDENT         0//D3DANTIALIAS_SORTDEPENDENT
#define piANTIALIAS_SORTINDEPENDENT       0//D3DANTIALIAS_SORTINDEPENDENT

#define piRENDERSTATE_TEXTUREHANDLE       piRENDERSTATE_TEXTUREHANDLE         // texture handle
#define piTEXTUREHANDLE_DEFAULT           0

#define piRENDERSTATE_ANTIALIAS           piRENDERSTATE_ANTIALIAS             // antialiasing mode
#define piANTIALIAS_DEFAULT               piANTIALIAS_NONE

#define piRENDERSTATE_TEXTUREADDRESS      piRENDERSTATE_TEXTUREADDRESS        // texture address
#define piTEXTUREADDRESS_DEFAULT          piTADDRESS_WRAP

#define piRENDERSTATE_TEXTUREPERSPECTIVE  piRENDERSTATE_TEXTUREPERSPECTIVE    // perspective correction
#define piTEXTUREPERSPECIVE_DEFAULT       TRUE

#define piRENDERSTATE_WRAPU               piRENDERSTATE_WRAPU                 // wrap in u direction
#define piWRAPU_DEFAULT                   piTADDRESS_WRAP

#define piRENDERSTATE_WRAPV               piRENDERSTATE_WRAPV                 // wrap in v direction
#define piWRAPV_DEFAULT                   piTADDRESS_WRAP

#define piRENDERSTATE_ZENABLE             piRENDERSTATE_ZENABLE               // enable z test
#define piZENABLE_DEFAULT                 TRUE

#define piRENDERSTATE_FILLMODE            piRENDERSTATE_FILLMODE              // fill mode
#define piFILLMODE_DEFAULT                piFILL_SOLID

#define piRENDERSTATE_SHADEMODE           piRENDERSTATE_SHADEMODE             // shade mode
#define piSHADEMODE_DEFAULT               piSHADE_GOURAUD

#define piRENDERSTATE_LINEPATTERN         piRENDERSTATE_LINEPATTERN           // line pattern
#define piLINEPATTERN_DEFAULT             0

#define piRENDERSTATE_MONOENABLE          piRENDERSTATE_MONOENABLE            // enable mono rendering
#define piMONOENABLE_DEFAULT              FALSE

#define piRENDERSTATE_ROP2                piRENDERSTATE_ROP2                  // raster operation
#define piROP2_DEFAULT                    R2_COPYPEN

#define piRENDERSTATE_PLANEMASK           piRENDERSTATE_PLANEMASK             // physical plane mask
#define piPLANEMASK                       (UINT32)(~0)

#define piRENDERSTATE_ZWRITEENABLE        piRENDERSTATE_ZWRITEENABLE          // enable z writes
#define piZWRITEENABLE_DEFAULT            TRUE

#define piRENDERSTATE_ALPHATESTENABLE     piRENDERSTATE_ALPHATESTENABLE       // enable alpha tests
#define piALPHATESTENABLE_DEFAULT         TRUE

#define piRENDERSTATE_LASTPIXEL           piRENDERSTATE_LASTPIXEL             // draw last pixel in a line
#define piLASTPIXEL_DEFAULT               FALSE

#define piRENDERSTATE_TEXTUREMAG          piRENDERSTATE_TEXTUREMAG            // how textures are magnified
#define piTEXTUREMAG_DEFAULT              piTFG_LINEAR

#define piRENDERSTATE_TEXTUREMIN          piRENDERSTATE_TEXTUREMIN            // how textures are reduced
#define piTEXTUREMIN_DEFAULT              piTFN_LINEAR_MIPMAP_LINEAR

#define piRENDERSTATE_SRCBLEND            piRENDERSTATE_SRCBLEND              // blend factor for source
#define piSRCBLEND_DEFAULT                piBLEND_SRCALPHA

#define piRENDERSTATE_DESTBLEND           piRENDERSTATE_DESTBLEND             // blend factor for destination
#define piDESTBLEND_DEFAULT               piBLEND_INVSRCALPHA

#define piRENDERSTATE_TEXTUREMAPBLEND     piRENDERSTATE_TEXTUREMAPBLEND       // blend mode for map
#define piTEXTUREMAPBLEND_DEFAULT         piTBLEND_MODULATE

#define piDEPTHRANGE_NEAR						0.0 // z buffer range values
#define piDEPTHRANGE_INCOCKPIT					0.1 //
#define piDEPTHRANGE_FAR						1.0 //

#define piRENDERSTATE_CULLMODE            piRENDERSTATE_CULLMODE              // back-face culling mode
#define piCULLMODE_DEFAULT                piCULL_CCW

#define piRENDERSTATE_ZFUNC               piRENDERSTATE_ZFUNC                 // z-comparison function
#define piZFUNC_DEFAULT                   piCMP_LESSEQUAL

#define piZB_FALSE	0//							D3DZB_FALSE
#define piZB_TRUE	1//							D3DZB_TRUE
#define piZB_USEW	2//							D3DZB_USEW

#define piRENDERSTATE_ALPHAREF            piRENDERSTATE_ALPHAREF              // reference alpha value
#define piALPHAREF_DEFAULT                0

#define piRENDERSTATE_ALPHAFUNC           piRENDERSTATE_ALPHAFUNC             // alpha-comparison function
#define piALPHAFUNC_DEFAULT               piCMP_GREATER

#define piRENDERSTATE_DITHERENABLE        piRENDERSTATE_DITHERENABLE          // enable dithering
#define piDITHERENABLE_DEFAULT            TRUE

#define piRENDERSTATE_FOGENABLE           piRENDERSTATE_FOGENABLE             // enable fog
#define piFOGENABLE_DEFAULT               TRUE

#define piRENDERSTATE_SPECULARENABLE      piRENDERSTATE_SPECULARENABLE        // enable specular highlights
#define piSPECULARENABLE                  TRUE

#define piRENDERSTATE_ZVISIBLE            piRENDERSTATE_ZVISIBLE              // enable z-checking
#define piZVISIBLE_DEFAULT                FALSE

#define piRENDERSTATE_SUBPIXEL            piRENDERSTATE_SUBPIXEL              // enable subpixel correction
#define piSUBPIXEL_DEFAULT                FALSE

#define piRENDERSTATE_SUBPIXELX           piRENDERSTATE_SUBPIXELX             // enable x subpixel correction
#define piSUBPIXELX_DEFAULT               FALSE

#define piRENDERSTATE_STIPPLEDALPHA       piRENDERSTATE_STIPPLEDALPHA         // enable stippled alpha
#define piSTIPPLEDALPHA_DEFAULT           TRUE

#define piRENDERSTATE_FOGCOLOR            piRENDERSTATE_FOGCOLOR              // fog color
#define piFOGCOLOR_DEFAULT                piRGBA_MAKE(0x7f,0x7f,0x7f,0x00)

#define piRENDERSTATE_FOGTABLEMODE        piRENDERSTATE_FOGTABLEMODE          // fog mode

#ifdef USE_LINEAR_FOG
#define piFOGTABLEMODE_DEFAULT            piFOG_LINEAR
#elif defined(USE_EXP_FOG)
#define piFOGTABLEMODE_DEFAULT            piFOG_EXP
#else
#define piFOGTABLEMODE_DEFAULT            piFOG_EXP2
#endif

#define piRENDERSTATE_FOGTABLESTART       piRENDERSTATE_FOGTABLESTART         // fog table start
#define piFOGTABLESTART_DEFAULT           piFloatLikeLong(0.3f)

#define piRENDERSTATE_FOGTABLEEND         piRENDERSTATE_FOGTABLEEND           // fog table end
#define piFOGTABLEEND_DEFAULT             piFloatLikeLong(0.7f)

#define piRENDERSTATE_FOGTABLEDENSITY     piRENDERSTATE_FOGTABLEDENSITY       // fog density
#define piFOGTABLEDENSITY_DEFAULT         piFloatLikeLong(0.01f)

#define piRENDERSTATE_STIPPLEENABLE       piRENDERSTATE_STIPPLEENABLE         // enables stippling
#define piSTIPPLEENABLE_DEFAULT           FALSE

#define piRENDERSTATE_EDGEANTIALIAS       piRENDERSTATE_EDGEANTIALIAS         // antialias edges
#define piEDGEANTIALIAS_DEFAULT           FALSE

#define piRENDERSTATE_COLORKEYENABLE      piRENDERSTATE_COLORKEYENABLE        // enable color-key transparency
#define piCOLORKEYENABLE_DEFAULT          FALSE

#define piRENDERSTATE_ALPHABLENDENABLE    piRENDERSTATE_ALPHABLENDENABLE      // enable alpha-blend transparency
#define piALPHABLENDENABLE_DEFAULT        TRUE

#define piRENDERSTATE_BORDERCOLOR         piRENDERSTATE_BORDERCOLOR           // border color
#define piBORDERCOLOR_DEFAULT             piRGBA_MAKE(0,0,0,0)

#define piRENDERSTATE_TEXTUREADDRESSU     piRENDERSTATE_TEXTUREADDRESSU       // u texture address mode
#define piTEXTUREADDRESSU_DEFAULT         piTADDRESS_WRAP

#define piRENDERSTATE_TEXTUREADDRESSV     piRENDERSTATE_TEXTUREADDRESSV       // v texture address mode
#define piTEXTUREADDRESSV_DEFAULT         piTADDRESS_WRAP

#define piRENDERSTATE_MIPMAPLODBIAS       piRENDERSTATE_MIPMAPLODBIAS         // mipmap LOD bias
#define piMIPMAPLODBIAS_DEFAULT           piFloatLikeLong(0.0f)

#define piRENDERSTATE_ZBIAS               piRENDERSTATE_ZBIAS                 // z-bias
#define piZBIAS_DEFAULT                   0

#define piRENDERSTATE_RANGEFOGENABLE      piRENDERSTATE_RANGEFOGENABLE        // enables range-based fog
#define piRANGEFOGENABLE_DEFAULT          TRUE

#define piRENDERSTATE_ANISOTROPY          piRENDERSTATE_ANISOTROPY            // max. anisotropy
#define piANISOTROPY_DEFAULT              0

#define piRENDERSTATE_STIPPLEPATTERN00    piRENDERSTATE_STIPPLEPATTERN00      // first line of stipple pattern
   // Stipple patterns 01 through 30 omitted here.
#define piRENDERSTATE_STIPPLEPATTERN31    piRENDERSTATE_STIPPLEPATTERN31      // last line of stipple pattern


// Light state constants

#define piLIGHTSTATE_MATERIAL             piLIGHTSTATE_MATERIAL
#define piMATERIAL_DEFAULT                0

#define piLIGHTSTATE_AMBIENT              piLIGHTSTATE_AMBIENT
#define piAMBIENT_DEFAULT                 piRGBA_MAKE(0x08,0x08,0x08,0)

#define piLIGHTSTATE_COLORMODEL           piLIGHTSTATE_COLORMODEL
#define piCOLORMODEL_DEFAULT              piCOLOR_RGB

#define piCOLOR_MONO                      1
#define piCOLOR_RGB                       2

#define piLIGHTSTATE_FOGMODE              piLIGHTSTATE_FOGMODE

#ifdef USE_LINEAR_FOG
#define piFOGMODE_DEFAULT                 piFOG_LINEAR
#elif defined(USE_EXP_FOG)
#define piFOGMODE_DEFAULT                 piFOG_EXP
#else
#define piFOGMODE_DEFAULT                 piFOG_EXP2
#endif

#define piLIGHTSTATE_FOGSTART             piLIGHTSTATE_FOGSTART
#define piFOGSTART_DEFAULT                piFloatLikeLong(2700.0f)

#define piLIGHTSTATE_FOGEND               piLIGHTSTATE_FOGEND
#define piFOGEND_DEFAULT                  piFloatLikeLong(6300.0f)

#define piLIGHTSTATE_FOGDENSITY           piLIGHTSTATE_FOGDENSITY

#ifdef USE_LINEAR_FOG
#define piFOGDENSITY_DEFAULT              piFloatLikeLong(1.0f)
#elif defined(USE_EXP_FOG)
#define piFOGDENSITY_DEFAULT              2000
#else
#define piFOGDENSITY_DEFAULT              piFloatLikeLong(0.000278f)
#endif

#define piRENDERSTATE_FOGSTART            piRENDERSTATE_FOGSTART
#define piRENDERSTATE_FOGEND              piRENDERSTATE_FOGEND
#define piRENDERSTATE_FOGDENSITY          piRENDERSTATE_FOGDENSITY


// new stuff

#define piRENDERSTATE_SOFTWAREVERTEXPROCESSING	0
#define piSOFTWAREVERTEXPROCESSING_DEFAULT		0
#define piRENDERSTATE_POINTSIZE					0
#define piPOINTSIZE_DEFAULT						0
#define piRENDERSTATE_POINTSIZE_MIN				0
#define piPOINTSIZE_MIN_DEFAULT					0
#define piRENDERSTATE_POINTSPRITEENABLE			0
#define piPOINTSPRITEENABLE_DEFAULT				0
#define piRENDERSTATE_POINTSCALEENABLE			0
#define piPOINTSCALEENABLE_DEFAULT				0
#define piRENDERSTATE_POINTSCALE_A				0
#define piPOINTSCALE_A_DEFAULT					0
#define piRENDERSTATE_POINTSCALE_B				0
#define piPOINTSCALE_B_DEFAULT					0
#define piRENDERSTATE_POINTSCALE_C				0
#define piPOINTSCALE_C_DEFAULT					0
#define piRENDERSTATE_MULTISAMPLEANTIALIAS		0
#define piMULTISAMPLEANTIALIAS_DEFAULT			0
#define piRENDERSTATE_MULTISAMPLEMASK			0
#define piMULTISAMPLEMASK_DEFAULT				0
#define piRENDERSTATE_PATCHEDGESTYLE			0
#define piPATCHEDGESTYLE_DEFAULT				0
#define piRENDERSTATE_PATCHSEGMENTS				0
#define piPATCHSEGMENTS_DEFAULT					0
#define piRENDERSTATE_DEBUGMONITORTOKEN			0
#define piDEBUGMONITORTOKEN_DEFAULT				0
#define piRENDERSTATE_POINTSIZE_MAX				0
#define piPOINTSIZE_MAX_DEFAULT					0
#define piRENDERSTATE_INDEXEDVERTEXBLENDENABLE	0
#define piINDEXEDVERTEXBLENDENABLE_DEFAULT		0
#define piRENDERSTATE_COLORWRITEENABLE			0
#define piCOLORWRITEENABLE_DEFAULT				0
#define piRENDERSTATE_TWEENFACTOR				0
#define piTWEENFACTOR_DEFAULT					0
#define piBLENDOP_DEFAULT						0
#define piRENDERSTATE_POSITIONORDER				0
#define piPOSITIONORDER_DEFAULT					0
#define piRENDERSTATE_NORMALORDER				0
#define piNORMALORDER_DEFAULT					0
#define piTSS_RESULTARG							0
#define piTSS_TCI_PASSTHRU						0

// Viewport, ZBuffer, Stencil and Accumulation constants

#define piCLEAR_VIEWPORT					1
#define piCLEAR_ZBUFFER						2
#define piCLEAR_STENCIL						4
#define piCLEAR_ACCUM						8


// Crosshairs types

#define piCROSSHAIRS_NONE					0
#define piCROSSHAIRS_POINT					1
#define piCROSSHAIRS_CROSS					2
#define piCROSSHAIRS_DEFAULT				piCROSSHAIRS_NONE


// The desired global graphics texture bits

#define TEX_BPP     16
#define TEX_ALPHA   0


// Colors
#if defined(OPENGL3D)
#include "ogl/oglcolors.h"
#define colorONEOVER255   0.00392156862745098039215686274509804f
#endif

// ZDepth for 3D objects

#define zdepthLAYER0      0.001f
#define zdepthLAYER1      0.002f
#define zdepthLAYER2      0.003f
#define zdepthLAYER3      0.004f
#define zdepthLAYER4      0.005f
#define zdepthLAYER5      0.006f
#define zdepthLAYER6      0.007f
#define zdepthLAYER7      0.008f


// Create Raw Texture flags

#define CRT_FLAG_NONE          0x00000000
#define CRT_FLAG_HARDWARE      0x00000001
#define CRT_FLAG_HDWMIPMAP     0x00000002
#define CRT_FLAG_RENDERTARGET  0x00000004


#define piFVF_XYZ              0x002 //D3DFVF_XYZ
#define piFVF_XYZRHW           0x004 //D3DFVF_XYZRHW
#define piFVF_NORMAL           0x010 //D3DFVF_NORMAL
#define piFVF_DIFFUSE          0x040 //D3DFVF_DIFFUSE
#define piFVF_SPECULAR         0x080 //D3DFVF_SPECULAR
#define piFVF_TEX1             0x100 //D3DFVF_TEX1
#define piLIGHT_ACTIVE         1 //D3DLIGHT_ACTIVE
#define piLIGHT_NO_SPECULAR    2 //D3DLIGHT_NO_SPECULAR



struct VERTEXSOURCE
{
	UINT32 fvf;
	int length;
};

struct INDEXSOURCE
{
	UINT32 fvf;
};

enum TEXTURETYPE
{
	TEXTURE_NONE,
	TEXTURE_DDS,
	TEXTURE_3DF,
	TEXTURE_BMP,
	TEXTURE_PCX,
	TEXTURE_JPG,
	TEXTURE_TIFF,
	TEXTURE_TGA,
	TEXTURE_PNG,
	MAX_TEXTURE_TYPES
};

struct TEXTURE
{
	SINT8                   name[128];

	LOGICAL					hasalpha;
	LOGICAL					iswritable;

	FLOAT32                 widthfactor[MAXMIPLEVELS];
	FLOAT32                 heightfactor[MAXMIPLEVELS];

	UINT32                  timeaccessed;
	UINT32                  target;
	UINT32					genlist;
	UINT32					hdevtex[MAXMIPLEVELS];

	SINT32                  id;
	SINT32                  numuses;
	SINT32                  multiTextureInfoID;
	SINT32                  imagewidth[MAXMIPLEVELS];
	SINT32                  imageheight[MAXMIPLEVELS];
	SINT32                  imagesize[MAXMIPLEVELS];
	SINT32                  po2width[MAXMIPLEVELS];
	SINT32                  po2height[MAXMIPLEVELS];
	SINT32					comp[MAXMIPLEVELS];
	SINT32					format[MAXMIPLEVELS];
	SINT32                  miplevels;
	SINT32                  normalMap;
	SINT32                  specularMap;

	bool                    newrelease;

	TEXTURETYPE             type;

	void* 					lpsurf[MAXMIPLEVELS];

#ifdef __cplusplus
	bool isdds() {return type == TEXTURE_DDS;}
	bool isbmp() {return type == TEXTURE_BMP;}
	bool istga() {return type == TEXTURE_TGA;}
#endif

};

struct TEXTURE3D
{
	SINT8                   name[128];
	LOGICAL					hasalpha;
	LOGICAL					iswritable;
	UINT32					genlist;
	SINT32*					texid;
	SINT32                  imagedepth;
	SINT32                  po2depth;
	FLOAT32                 depthfactor;
	UINT32                  timeaccessed;
	SINT32                  numuses;

};

struct FLOATINT
{
	union
	{
		FLOAT32	f;
		SINT32	i;
	};

};

static inline SINT32 FloatToInt(FLOAT32 f)
{
	FLOATINT  fi;
	fi.f = f + 8388608.0f;
	return(fi.i & 0x007fffff);
}

// Components of the Texture Environment operators
#define piCOMBINE			GL_COMBINE_ARB
#define piCOMBINE_RGB		GL_COMBINE_RGB_ARB
#define piCOMBINE_ALPHA		GL_COMBINE_ALPHA_ARB
#define piSOURCE0_RGB		GL_SOURCE0_RGB_ARB
#define piSOURCE1_RGB		GL_SOURCE1_RGB_ARB
#define piSOURCE2_RGB		GL_SOURCE2_RGB_ARB
#define piSOURCE0_ALPHA		GL_SOURCE0_ALPHA_ARB
#define piSOURCE1_ALPHA		GL_SOURCE1_ALPHA_ARB
#define piSOURCE2_ALPHA		GL_SOURCE2_ALPHA_ARB
#define piOPERAND0_RGB		GL_OPERAND0_RGB_ARB
#define piOPERAND1_RGB		GL_OPERAND1_RGB_ARB
#define piOPERAND2_RGB		GL_OPERAND2_RGB_ARB
#define piOPERAND0_ALPHA	GL_OPERAND0_ALPHA_ARB
#define piOPERAND1_ALPHA	GL_OPERAND1_ALPHA_ARB
#define piOPERAND2_ALPHA	GL_OPERAND2_ALPHA_ARB
#define piRGB_SCALE			GL_RGB_SCALE_ARB
#define piADD_SIGNED		GL_ADD_SIGNED_ARB
#define piINTERPOLATE		GL_INTERPOLATE_ARB
#define piSUBTRACT			GL_SUBTRACT_ARB
#define piCONSTANT			GL_CONSTANT_ARB
#define piPRIMARY_COLOR		GL_PRIMARY_COLOR_ARB
#define piPREVIOUS			GL_PREVIOUS_ARB


// There have no DirectX counterparts!

// Combine RGB/ALPHA texture functions
enum piTSS_COMBINE_TEXTURE
{
	piTSS_COMBINE		= GL_COMBINE_ARB,
	piTSS_REPLACE		= GL_REPLACE,
	piTSS_MODULATE		= GL_MODULATE,
	piTSS_ADD			= GL_ADD,
	piTSS_ADD_SIGNED	= GL_ADD_SIGNED_ARB,
	piTSS_INTERPOLATE	= GL_INTERPOLATE_ARB,
	piTSS_SUBTRACT		= GL_SUBTRACT_ARB,
	piTSS_DOT3_RGB		= GL_DOT3_RGB_ARB,		// Use for RGB only
	piTSS_DOT3_RGBA		= GL_DOT3_RGBA_ARB		// Use for RGB only

};

// Source 'n' RGB/ALPHA
enum piTSS_SOURCE_RGB
{
	piTSS_TEXTURE			= GL_TEXTURE,
	piTSS_CONSTANT			= GL_CONSTANT_ARB,
	piTSS_PRIMARY_COLOR		= GL_PRIMARY_COLOR_ARB,
	piTSS_PREVIOUS			= GL_PREVIOUS_ARB

};

// Operand 'n' RGB/ALPHA
enum piTSS_OPERAND_RGB
{
	piTSS_SRC_COLOR					= GL_SRC_COLOR,				// Use for RGB only
	piTSS_SRC_ONE_MINUS_SRC_COLOR	= GL_ONE_MINUS_SRC_COLOR,	// Use for RGB only
	piTSS_SRC_ALPHA					= GL_SRC_ALPHA,				// Use for RGB and ALPHA
	piTSS_SRC_ONE_MINUS_SRC_ALPHA	= GL_ONE_MINUS_SRC_ALPHA	// Use for RGB and ALPHA

};

// Texture stage states combine modes
enum piTEXTURESTAGESTATETYPE
{
	piTSS_TEXTURE_ENV_MODE,
	piTSS_COMBINE_RGB,
	piTSS_COMBINE_ALPHA,
	piTSS_SOURCE0_RGB,
	piTSS_SOURCE1_RGB,
	piTSS_SOURCE2_RGB,
	piTSS_OPERAND0_RGB,
	piTSS_OPERAND1_RGB,
	piTSS_OPERAND2_RGB,
	piTSS_SOURCE0_ALPHA,
	piTSS_SOURCE1_ALPHA,
	piTSS_SOURCE2_ALPHA,
	piTSS_OPERAND0_ALPHA,
	piTSS_OPERAND1_ALPHA,
	piTSS_OPERAND2_ALPHA,
	piTSS_ADDRESSU,
	piTSS_ADDRESSV,
	piTSS_ADDRESSW,
	piTSS_MAGFILTER,
	piTSS_MINFILTER,
	piTSS_MIPFILTER,
	piTSS_ENABLE_TEXTURE2D

};

#define piTA_SELECTMASK        0x0000000f  // mask for arg selector
#define piTA_DIFFUSE           0x00000000  // select diffuse color
#define piTA_CURRENT           0x00000001  // select result of previous stage
#define piTA_TEXTURE           0x00000002  // select texture color
#define piTA_TFACTOR           0x00000003  // select RENDERSTATE_TEXTUREFACTOR

#define piTA_COMPLEMENT        0x00000010  // take 1.0 - x
#define piTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components

/*
 *  Texture State Filter Types
 */
enum piTEXTUREMAGFILTER
{
	piTFG_NONE						= 0,			// none
	piTFG_POINT						= GL_NEAREST,	// nearest
	piTFG_LINEAR					= GL_LINEAR		// linear interpolation

};

enum piTEXTUREMINFILTER
{
	piTFN_NONE						= 0,							// none
	piTFN_POINT						= GL_NEAREST,					// nearest
	piTFN_LINEAR					= GL_LINEAR,					// linear interpolation
	piTFN_NEAREST_MIPMAP_NEAREST	= GL_NEAREST_MIPMAP_NEAREST,	// MIN: nearest, MIP: nearest
	piTFN_LINEAR_MIPMAP_NEAREST		= GL_LINEAR_MIPMAP_NEAREST,		// MIN: linear, MIP: nearest
	piTFN_NEAREST_MIPMAP_LINEAR		= GL_NEAREST_MIPMAP_LINEAR,		// MIN: nearest, MIP: linear
	piTFN_LINEAR_MIPMAP_LINEAR		= GL_LINEAR_MIPMAP_LINEAR		// MIN: linear, MIP: linear

};

// MIP filtering is not available directly; it is combined with the MIN filter (see above)
// We maintain these values to keep the changes in code minimal
enum piTEXTUREMIPFILTER
{
	piTFP_NONE						= 0,			// none
	piTFP_POINT						= GL_NEAREST,	// nearest
	piTFP_LINEAR					= GL_LINEAR		// linear interpolation

};

enum piPRIMITIVETYPE
{
	piPT_POINTLIST		= GL_POINTS,
	piPT_LINELIST		= GL_LINES,
	piPT_LINESTRIP		= GL_LINE_STRIP,
	piPT_TRIANGLELIST	= GL_TRIANGLES,
	piPT_TRIANGLESTRIP	= GL_TRIANGLE_STRIP,
	piPT_TRIANGLEFAN	= GL_TRIANGLE_FAN

};

// Vertex types
enum piVERTEXTYPE
{
	piVT_VERTEX       	= 0,
	piVT_LVERTEX       	= 1,
	piVT_TLVERTEX      	= 2,
	piVT_CVERTEX	 	= 3,
	piVT_GVERTEX		= piVT_VERTEX,
	piVT_3DVERTEX		= 4,
	piVT_TVERTEX        = 5 // Tangent vertex

};

struct piVECTOR
{
	FLOAT32 x;
	FLOAT32 y;
	FLOAT32 z;

	void set(float X, float Y, float Z)
	{
		x = X;
		y = Y;
		z = Z;
	}
};

#define SMALL_NUMBER 1e-10

inline bool operator == (const piVECTOR &a, const piVECTOR &b) { return fabs(a.x - b.x) < SMALL_NUMBER && fabs(a.y - b.y) < SMALL_NUMBER && fabs(a.z - b.z) < SMALL_NUMBER; }

struct piUV
{
	FLOAT32   u;
	FLOAT32   v;

};

struct piUVW
{
	FLOAT32   u;
	FLOAT32   v;
	FLOAT32   w;

};

struct piTLVERTEX
{
	FLOAT32		sx;             /* Screen coordinates */
	FLOAT32		sy;
	FLOAT32		sz;
	FLOAT32		rhw;            /* Reciprocal of homogeneous w */
	UINT32		color;          /* Vertex color */
	UINT32		specular;       /* Specular component of vertex */
	FLOAT32		tu;             /* Texture coordinates */
	FLOAT32		tv;

};

struct piLVERTEX
{
	FLOAT32		x;             /* Homogeneous coordinates */
	FLOAT32		y;
	FLOAT32		z;
	UINT32		reserved;
	UINT32		color;         /* Vertex color */
	UINT32		specular;      /* Specular component of vertex */
	FLOAT32		tu;            /* Texture coordinates */
	FLOAT32		tv;
};

struct piVERTEX
{
	FLOAT32     x;             /* Homogeneous coordinates */
	FLOAT32     y;
	FLOAT32     z;
	FLOAT32     nx;            /* Normal */
	FLOAT32     ny;
	FLOAT32     nz;
	FLOAT32     tu;            /* Texture coordinates */
	FLOAT32     tv;

};

struct pi3DVERTEX
{
	FLOAT32		x;
	FLOAT32		y;
	FLOAT32		z;
	FLOAT32		nx;
	FLOAT32		ny;
	FLOAT32		nz;
	UINT32		color;
	FLOAT32		tu;
	FLOAT32		tv;
	FLOAT32		tw;

};

// "Unified" Vertex structure
struct piUNIFIED_VERTEX
{
	FLOAT32		x, y, z;
	FLOAT32		nx, ny, nz;
	UINT32		color;
	FLOAT32		tu, tv;
};
typedef struct piUNIFIED_VERTEX piCVERTEX;

struct piGVERTEX
{
	float     x;             /* Homogeneous coordinates */
	float     y;
	float     z;
	float     nx;            /* Normal */
	float     ny;
	float     nz;
	float     tu;            /* Texture coordinates */
	float     tv;

};

struct piTVERTEX
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

};

struct piVERTEX_COLOR
{
	UINT32 color;
	UINT32 specular;

};

struct matrix_alt // for mac debugging
{
		FLOAT32	_11, _12, _13, _14;
		FLOAT32	_21, _22, _23, _24;
		FLOAT32	_31, _32, _33, _34;
		FLOAT32	_41, _42, _43, _44;
};

union piMATRIX
{
	struct
	{
		FLOAT32	_11, _12, _13, _14;
		FLOAT32	_21, _22, _23, _24;
		FLOAT32	_31, _32, _33, _34;
		FLOAT32	_41, _42, _43, _44;
	};

	FLOAT32 m[4][4];

};

struct piMATRIX3x3
{
	float _11, _12, _13;
	float _21, _22, _23;
	float _31, _32, _33;

	piMATRIX3x3()
	{
		_11 = 1;
		_12 = 0;
		_13 = 0;
		_21 = 0;
		_22 = 1;
		_23 = 0;
		_31 = 0;
		_32 = 0;
		_33 = 1;
	}

	piMATRIX3x3(piMATRIX& m)
	{
		set(m);
	}

	void set(piMATRIX& m)
	{
		_11 = m._11;
		_12 = m._12;
		_13 = m._13;
		_21 = m._21;
		_22 = m._22;
		_23 = m._23;
		_31 = m._31;
		_32 = m._32;
		_33 = m._33;
	}
};

class piQUAT
{
public:
	FLOAT32 x;
	FLOAT32 y;
	FLOAT32 z;
	FLOAT32 w;

	void set(float X, float Y, float Z, float W)
	{
		x = X;
		y = Y;
		z = Z;
		w = W;
	}

};

// Loading 3ds file information

class piCOLORVALUE
{
public:
	FLOAT32 r;
	FLOAT32 g;
	FLOAT32 b;
	FLOAT32 a;

	void set(float R, float G, float B, float A)
	{
		r = R;
		g = G;
		b = B;
		a = A;
	}

};

// piPIXELFORMAT
class piPIXELFORMAT
{
public:
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

};

// piSURFACEDESC
struct piSURFACEDESC
{
	UINT32		Height;				// height of surface to be created
	UINT32		Width;				// width of input surface
	SINT32		Pitch;				// distance to start of next line (return value only)
	void* 		Surface;			// pointer to the associated surface memory
	piPIXELFORMAT	PixelFormat;	// pixel format description of the surface
};

typedef UINT32 piMATERIALHANDLE;
typedef UINT32 piTEXTUREHANDLE;

typedef UINT32 piCOLOR;

#define piRGBA_GETALPHA(rgb)		(((rgb) >> 24) & 0xff)
#define piRGBA_GETRED(rgb)			(((rgb) >> 0) & 0xff)
#define piRGBA_GETGREEN(rgb)		(((rgb) >> 8) & 0xff)
#define piRGBA_GETBLUE(rgb)			(((rgb) >> 16) & 0xff)
#define piRGBA_SETALPHA(rgba, x)	(((x) << 24) | ((rgba) & 0x00ffffff))
#define piRGBA_MAKE(r, g, b, a)		((piCOLOR) (((a) << 24) | ((r) << 0) | ((g) << 8) | ((b) << 16)))
#define piRGBA_TORGB(rgba)			((piCOLOR) ((rgba) & 0xffffff))
#define piRGB_MAKE(r, g, b)			((piCOLOR) (((r) << 0) | ((g) << 8) | ((b) << 16)))
#define piRGB_TORGBA(rgb)			((piCOLOR) ((rgb) | 0xff000000))
#define piRGB(r, g, b)				(0xff000000L | (((long)((r) * 255)) << 0) | (((long)((g) * 255)) << 8) | (((long)((b) * 255)) << 16))
#define piRGBA(r, g, b, a)			((((SINT32)((a) * 255)) << 24) | (((SINT32)((r) * 255)) << 0) | \
									(((SINT32)((g) * 255)) <<  8) | (((SINT32)((b) * 255)) << 16))


struct piMATERIAL
{
	piCOLORVALUE	diffuse;        /* Diffuse color RGBA */
	piCOLORVALUE	ambient;        /* Ambient color RGB */
	piCOLORVALUE	specular;       /* Specular 'shininess' */
	piCOLORVALUE	emissive;       /* Emissive color RGB */
	FLOAT32			power;          /* Sharpness if specular highlight */

};

#if platOS == OS_WIN32
#pragma pack(push, 1)
#endif

#if platOS == OS_MACOS
#pragma options align=packed
#endif
struct piRGBQUAD
{
	UCHAR	rgbRed;
	UCHAR	rgbGreen;
	UCHAR	rgbBlue;
	UCHAR	rgbReserved;
};

struct piRGBTRIPLE
{
	UCHAR rgbtRed;
	UCHAR rgbtGreen;
	UCHAR rgbtBlue;
};

struct piBITMAPINFOHEADER
{
	UINT32		biSize;
	SINT32		biWidth;
	SINT32		biHeight;
	UINT16		biPlanes;
	UINT16		biBitCount;
	UINT32		biCompression;
	UINT32		biSizeImage;
	SINT32		biXPelsPerMeter;
	SINT32		biYPelsPerMeter;
	UINT32		biClrUsed;
	UINT32		biClrImportant;

};

struct piBITMAPINFO
{
	piBITMAPINFOHEADER	bmiHeader;
	piRGBQUAD			bmiColors[1];

};

struct piBITMAPFILEHEADER
{    /* bmfh */
	UINT16		bfType;
	UINT32		bfSize;
	UINT16		bfReserved1;
	UINT16		bfReserved2;
	UINT32		bfOffBits;
};

// Compression specifications. The following values are possible:
#ifndef BI_RGB
	#define BI_RGB 0
#endif
#ifndef BI_RLE4
	#define BI_RLE4 1
#endif
#ifndef BI_RLE8
	#define BI_RLE8 2
#endif
#ifndef BI_BITFIELDS
	#define BI_BITFIELDS 3
#endif

#if platOS == OS_WIN32
#pragma pack(pop)
#endif

#if platOS == OS_MACOS
#pragma options align=reset
#endif

struct MATERIAL
{
	SINT8				name[32];
	SINT8				texname[_MAX_PATH];
	SINT8               alphaname[_MAX_PATH];
	piMATERIAL          material;
	SINT32              matid;
	SINT32              texid;
	SINT32				numuses;
	SINT32              displayList;

	bool dirty;

};

struct BBOX
{
	piVECTOR			corner[2];
};

struct piINDICES
{
	UINT16    v1;            /* Vertex indices */
	UINT16    v2;
	UINT16    v3;

};

enum piLIGHTTYPE
{
	piLIGHT_POINT=1,
	piLIGHT_SPOT,
	piLIGHT_DIRECTIONAL,
	piLIGHT_PARALLELPOINT,
	piLIGHT_GLSPOT

};

struct piLIGHT
{
	UINT32          dltType;
	piCOLORVALUE    dcvColor;
	piCOLORVALUE    dcvSpecular;
	piCOLORVALUE    dcvAmbient;
	piVECTOR        dvPosition;
	piVECTOR        dvDirection;
	FLOAT32         dvRange;
	FLOAT32         dvFalloff;
	FLOAT32         dvAttenuation0;
	FLOAT32         dvAttenuation1;
	FLOAT32         dvAttenuation2;
	FLOAT32         dvTheta;
	FLOAT32         dvPhi;

};

enum piLIGHTSTATETYPE
{
	piLIGHTSTATE_MATERIAL          = 1,
	piLIGHTSTATE_AMBIENT           = 2,
	piLIGHTSTATE_COLORMODEL        = 3,
	piLIGHTSTATE_FOGMODE           = 4,
	piLIGHTSTATE_FOGSTART          = 5,
	piLIGHTSTATE_FOGEND            = 6,
	piLIGHTSTATE_FOGDENSITY        = 7,
	piLIGHTSTATE_COLORVERTEX       = 8,
	piLIGHTSTATE_FORCE_DWORD         = 0x7fffffff /* force 32-bit size enum */

};

struct LIGHT
{
	SCHAR           name[32];
	UINT32			flags;
	piLIGHT			light;
	SINT32        	userdata1;
	FLOAT32         userdata2;

};

struct piRECT
{
	SINT32 	left;
	SINT32	top;
	SINT32	right;
	SINT32	bottom;
};

struct piPOINT
{
	SINT32	x;
	SINT32	y;
};

enum piRENDERSTATETYPE
{
	piRENDERSTATE_TEXTUREHANDLE      = 1,    /* Texture handle for legacy interfaces (Texture,Texture2) */
	piRENDERSTATE_ANTIALIAS          = 2,    /* D3DANTIALIASMODE */
	piRENDERSTATE_TEXTUREADDRESS     = 3,    /* D3DTEXTUREADDRESS  */
	piRENDERSTATE_TEXTUREPERSPECTIVE = 4,    /* TRUE for perspective correction */
	piRENDERSTATE_WRAPU              = 5,    /* TRUE for wrapping in u */
	piRENDERSTATE_WRAPV              = 6,    /* TRUE for wrapping in v */
	piRENDERSTATE_ZENABLE            = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
	piRENDERSTATE_FILLMODE           = 8,    /* D3DFILL_MODE        */
	piRENDERSTATE_SHADEMODE          = 9,    /* D3DSHADEMODE */
	piRENDERSTATE_LINEPATTERN        = 10,   /* D3DLINEPATTERN */
	piRENDERSTATE_MONOENABLE         = 11,   /* TRUE to enable mono rasterization */
	piRENDERSTATE_ROP2               = 12,   /* ROP2 */
	piRENDERSTATE_PLANEMASK          = 13,   /* DWORD physical plane mask */
	piRENDERSTATE_ZWRITEENABLE       = 14,   /* TRUE to enable z writes */
	piRENDERSTATE_ALPHATESTENABLE    = 15,   /* TRUE to enable alpha tests */
	piRENDERSTATE_LASTPIXEL          = 16,   /* TRUE for last-pixel on lines */
	piRENDERSTATE_TEXTUREMAG         = 17,   /* D3DTEXTUREFILTER */
	piRENDERSTATE_TEXTUREMIN         = 18,   /* D3DTEXTUREFILTER */
	piRENDERSTATE_SRCBLEND           = 19,   /* D3DBLEND */
	piRENDERSTATE_DESTBLEND          = 20,   /* D3DBLEND */
	piRENDERSTATE_TEXTUREMAPBLEND    = 21,   /* D3DTEXTUREBLEND */
	piRENDERSTATE_CULLMODE           = 22,   /* D3DCULL */
	piRENDERSTATE_ZFUNC              = 23,   /* D3DCMPFUNC */
	piRENDERSTATE_ALPHAREF           = 24,   /* D3DFIXED */
	piRENDERSTATE_ALPHAFUNC          = 25,   /* D3DCMPFUNC */
	piRENDERSTATE_DITHERENABLE       = 26,   /* TRUE to enable dithering */
	piRENDERSTATE_ALPHABLENDENABLE   = 27,   /* TRUE to enable alpha blending */
	piRENDERSTATE_FOGENABLE          = 28,   /* TRUE to enable fog */
	piRENDERSTATE_SPECULARENABLE     = 29,   /* TRUE to enable specular */
	piRENDERSTATE_ZVISIBLE           = 30,   /* TRUE to enable z checking */
	piRENDERSTATE_SUBPIXEL           = 31,   /* TRUE to enable subpixel correction */
	piRENDERSTATE_SUBPIXELX          = 32,   /* TRUE to enable correction in X only */
	piRENDERSTATE_STIPPLEDALPHA      = 33,   /* TRUE to enable stippled alpha */
	piRENDERSTATE_FOGCOLOR           = 34,   /* D3DCOLOR */
	piRENDERSTATE_FOGTABLEMODE       = 35,   /* D3DFOGMODE */
	piRENDERSTATE_FOGTABLESTART      = 36,   /* Fog table start    */
	piRENDERSTATE_FOGTABLEEND        = 37,   /* Fog table end      */
	piRENDERSTATE_FOGTABLEDENSITY    = 38,   /* Fog table density  */
	piRENDERSTATE_STIPPLEENABLE      = 39,   /* TRUE to enable stippling */
	piRENDERSTATE_EDGEANTIALIAS      = 40,   /* TRUE to enable edge antialiasing */
	piRENDERSTATE_COLORKEYENABLE     = 41,   /* TRUE to enable source colorkeyed textures */
	piRENDERSTATE_BORDERCOLOR        = 43,   /* Border color for texturing w/border */
	piRENDERSTATE_TEXTUREADDRESSU    = 44,   /* Texture addressing mode for U coordinate */
	piRENDERSTATE_TEXTUREADDRESSV    = 45,   /* Texture addressing mode for V coordinate */
	piRENDERSTATE_MIPMAPLODBIAS      = 46,   /* D3DVALUE Mipmap LOD bias */
	piRENDERSTATE_ZBIAS              = 47,   /* LONG Z bias */
	piRENDERSTATE_RANGEFOGENABLE     = 48,   /* Enables range-based fog */
	piRENDERSTATE_ANISOTROPY         = 49,   /* Max. anisotropy. 1 = no anisotropy */
	piRENDERSTATE_FLUSHBATCH         = 50,   /* Explicit flush for DP batching (DX5 Only) */
	piRENDERSTATE_TRANSLUCENTSORTINDEPENDENT=51, /* BOOL enable sort-independent transparency */
	piRENDERSTATE_STENCILENABLE      = 52,   /* BOOL enable/disable stenciling */
	piRENDERSTATE_STENCILFAIL        = 53,   /* D3DSTENCILOP to do if stencil test fails */
	piRENDERSTATE_STENCILZFAIL       = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
	piRENDERSTATE_STENCILPASS        = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
	piRENDERSTATE_STENCILFUNC        = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
	piRENDERSTATE_STENCILREF         = 57,   /* Reference value used in stencil test */
	piRENDERSTATE_STENCILMASK        = 58,   /* Mask value used in stencil test */
	piRENDERSTATE_STENCILWRITEMASK   = 59,   /* Write mask applied to values written to stencil buffer */
	piRENDERSTATE_TEXTUREFACTOR      = 60,   /* D3DCOLOR used for multi-texture blend */
	piRENDERSTATE_STIPPLEPATTERN00   = 64,   /* Stipple pattern 01...  */
	piRENDERSTATE_STIPPLEPATTERN01   = 65,
	piRENDERSTATE_STIPPLEPATTERN02   = 66,
	piRENDERSTATE_STIPPLEPATTERN03   = 67,
	piRENDERSTATE_STIPPLEPATTERN04   = 68,
	piRENDERSTATE_STIPPLEPATTERN05   = 69,
	piRENDERSTATE_STIPPLEPATTERN06   = 70,
	piRENDERSTATE_STIPPLEPATTERN07   = 71,
	piRENDERSTATE_STIPPLEPATTERN08   = 72,
	piRENDERSTATE_STIPPLEPATTERN09   = 73,
	piRENDERSTATE_STIPPLEPATTERN10   = 74,
	piRENDERSTATE_STIPPLEPATTERN11   = 75,
	piRENDERSTATE_STIPPLEPATTERN12   = 76,
	piRENDERSTATE_STIPPLEPATTERN13   = 77,
	piRENDERSTATE_STIPPLEPATTERN14   = 78,
	piRENDERSTATE_STIPPLEPATTERN15   = 79,
	piRENDERSTATE_STIPPLEPATTERN16   = 80,
	piRENDERSTATE_STIPPLEPATTERN17   = 81,
	piRENDERSTATE_STIPPLEPATTERN18   = 82,
	piRENDERSTATE_STIPPLEPATTERN19   = 83,
	piRENDERSTATE_STIPPLEPATTERN20   = 84,
	piRENDERSTATE_STIPPLEPATTERN21   = 85,
	piRENDERSTATE_STIPPLEPATTERN22   = 86,
	piRENDERSTATE_STIPPLEPATTERN23   = 87,
	piRENDERSTATE_STIPPLEPATTERN24   = 88,
	piRENDERSTATE_STIPPLEPATTERN25   = 89,
	piRENDERSTATE_STIPPLEPATTERN26   = 90,
	piRENDERSTATE_STIPPLEPATTERN27   = 91,
	piRENDERSTATE_STIPPLEPATTERN28   = 92,
	piRENDERSTATE_STIPPLEPATTERN29   = 93,
	piRENDERSTATE_STIPPLEPATTERN30   = 94,
	piRENDERSTATE_STIPPLEPATTERN31   = 95,

	/*
	 * 128 values [128, 255] are reserved for texture coordinate wrap flags.
	 * These are constructed with the D3DWRAP_U and D3DWRAP_V macros. Using
	 * a flags word preserves forward compatibility with texture coordinates
	 * that are >2D.
	 */
	piRENDERSTATE_WRAP0              = 128,  /* wrap for 1st texture coord. set */
	piRENDERSTATE_WRAP1              = 129,  /* wrap for 2nd texture coord. set */
	piRENDERSTATE_WRAP2              = 130,  /* wrap for 3rd texture coord. set */
	piRENDERSTATE_WRAP3              = 131,  /* wrap for 4th texture coord. set */
	piRENDERSTATE_WRAP4              = 132,  /* wrap for 5th texture coord. set */
	piRENDERSTATE_WRAP5              = 133,  /* wrap for 6th texture coord. set */
	piRENDERSTATE_WRAP6              = 134,  /* wrap for 7th texture coord. set */
	piRENDERSTATE_WRAP7              = 135,  /* wrap for 8th texture coord. set */
	piRENDERSTATE_CLIPPING					= 136,
	piRENDERSTATE_LIGHTING					= 137,
	piRENDERSTATE_EXTENTS					= 138,
	piRENDERSTATE_AMBIENT					= 139,
	piRENDERSTATE_FOGVERTEXMODE				= 140,
	piRENDERSTATE_COLORVERTEX				= 141,
	piRENDERSTATE_LOCALVIEWER				= 142,
	piRENDERSTATE_NORMALIZENORMALS			= 143,
	piRENDERSTATE_COLORKEYBLENDENABLE		= 144,
	piRENDERSTATE_DIFFUSEMATERIALSOURCE		= 145,
	piRENDERSTATE_SPECULARMATERIALSOURCE	= 146,
	piRENDERSTATE_AMBIENTMATERIALSOURCE		= 147,
	piRENDERSTATE_EMISSIVEMATERIALSOURCE	= 148,
	piRENDERSTATE_VERTEXBLEND				= 151,
	piRENDERSTATE_CLIPPLANEENABLE			= 152,

	piRENDERSTATE_BLENDOP					= 171,

	piRENDERSTATE_COLORMASK                 = 177,
	piRENDERSTATE_CLEARCOLOR                = 178,
	piRENDERSTATE_COLOR4F                   = 179,
	piRENDERSTATE_CULLFACEENABLE            = 180,
	piRENDERSTATE_CULLFACE                  = 181,
	piRENDERSTATE_RASTERIZER_DISCARD        = 182,
	piRENDERSTATE_DRAWBUFFER                = 183,
	piRENDERSTATE_READBUFFER                = 184,

	// If reached 255, then increase MAXRENDERSTATES in ogl.cpp
	piRENDERSTATE_FORCE_DWORD        = 0x7fffffff /* force 32-bit size enum */

};


// these new guys need to be taken care of...

enum piRENDERSTATEFOGTYPE
{
	piRENDERSTATE_FOGSTART			= 36,
	piRENDERSTATE_FOGEND			= 37,
	piRENDERSTATE_FOGDENSITY		= 38
};

struct piPTRSTRIDE
{
	LPVOID	pdata;
	UINT32	stride;
};

#define piMAXTEXCOORD 8

struct piSTRIDEDDATA
{
	piPTRSTRIDE	position;
	piPTRSTRIDE	normal;
	piPTRSTRIDE	diffuse;
	piPTRSTRIDE	specular;
	piPTRSTRIDE	textureCoords[piMAXTEXCOORD];
	piPTRSTRIDE indices;
};


typedef LOGICAL (*piDRAW_OBJECTS)(void);

#define piLIGHTING_DEFAULT			1

#define piSTENCILENABLE_DEFAULT		0
#define piSTENCILFAIL_DEFAULT		0
#define piSTENCILZFAIL_DEFAULT		0
#define piSTENCILPASS_DEFAULT		0
#define piSTENCILFUNC_DEFAULT		0
#define piSTENCILREF_DEFAULT		0
#define piSTENCILMASK_DEFAULT		0
#define piSTENCILWRITEMASK_DEFAULT	0
#define piTEXTUREFACTOR_DEFAULT		0
#define piWRAP0_DEFAULT				0
#define piWRAP1_DEFAULT				0
#define piWRAP2_DEFAULT				0
#define piWRAP3_DEFAULT				0
#define piWRAP4_DEFAULT				0
#define piWRAP5_DEFAULT				0
#define piWRAP6_DEFAULT				0
#define piWRAP7_DEFAULT				0
#define piCLIPPING_DEFAULT			0
#define piEXTENTS_DEFAULT			0
#define piFOGVERTEXMODE_DEFAULT		0
#define piCOLORVERTEX_DEFAULT		0
#define piLOCALVIEWER_DEFAULT		0
#define piNORMALIZENORMALS_DEFAULT	0
#define piCOLORKEYBLENDENABLE_DEFAULT		0
#define piDIFFUSEMATERIALSOURCE_DEFAULT		0
#define piSPECULARMATERIALSOURCE_DEFAULT	0
#define piAMBIENTMATERIALSOURCE_DEFAULT		0
#define piEMISSIVEMATERIALSOURCE_DEFAULT	0
#define piVERTEXBLEND_DEFAULT		0
#define piCLIPPLANEENABLE_DEFAULT	0
#define piMAXANISOTROPY_DEFAULT		0
#define piTTFF_DISABLE				0
#define piTEXTUREMIP_DEFAULT		piTFP_LINEAR

// fix these...
#define piTADDRESS_MIRROR			0
#define piTADDRESS_BORDER			0

#define piTTFF_COUNT1				0
#define piTTFF_COUNT2				0
#define piTTFF_COUNT3				0
#define piTTFF_COUNT4				0
#define piTTFF_PROJECTED			0

#if platOS == OS_WIN32
	HRESULT                 DIErrorSummary(HRESULT code);

	#define DDError(code)   code
	#define D3DError(code)  code
	#define DIError(code)   DIErrorSummary(code)
#else
	#define DDError(code)   code
	#define D3DError(code)  code
	#define DIError(code)   code
#endif

#define piReadTimeStamp32 GetMicroSeconds
#define piReadTimeStamp64 GetMicroSeconds

extern SINT32 piFloatLikeLong(FLOAT32 f);
extern FLOAT32 piLongLikeFloat(SINT32 l);
extern SINT32 piFindBitShift(UINT32 Mask);
extern SINT32 piFindBitCount(UINT32 Mask);

extern LOGICAL	piInit(void);
extern LOGICAL piClearTargets(UINT32 targets);
extern LOGICAL piClearPorts(piRECT piRect, UINT32 uTargets);
extern LOGICAL piIsTexture(SINT32 texid, SINT32 miplevel);
extern LOGICAL piIsTexture3D(SINT32 texid);
extern LOGICAL	piTextureHasAlpha(SINT32 texid);
extern LOGICAL	piAddTexRef(SINT32 texid);
extern void	piUpdateTextures(void);
extern LOGICAL piGetTextureMemory(SINT32* total);
extern LOGICAL	piReleaseTextureHandles(void);
extern LOGICAL piUnlockTextureMemorySurface(SINT32 texid, SINT32 miplevel);
extern LOGICAL piSetActiveTexture(SINT32 texid, SINT32 miplevel);
extern LOGICAL piDisableTexture(void);
extern LOGICAL piEnableTexture();
extern LOGICAL piIsMaterial(SINT32 matid);
extern LOGICAL piGetMaterialTexName(SINT32 matid, SINT8* texname);
extern SINT32 piFindMaterialName(const SINT8* name);
extern LOGICAL piSetActiveMaterial(SINT32 matid);
extern LOGICAL piDisableMaterial(void);
extern LOGICAL piSetUnscaledViewMatrix(piMATRIX* lpNewView);
extern LOGICAL	piSetFieldOfView(FLOAT32 fov, FLOAT32 zmin, FLOAT32 zmax);
extern LOGICAL piBeginScene(void);
extern LOGICAL piEndScene(void);
extern void piSetRenderState(int StateType, UINT32 State);
extern LOGICAL piDisplayRenderStates(void);
extern LOGICAL piSetLightState(SINT32 lightnum, UINT32 State);
extern LOGICAL	piFlip(void);
extern LOGICAL	piSetDDRender(SYS_DDRENDERFUNC newfunc);
extern LOGICAL	piSetD3DRender(SYS_D3DRENDERFUNC newfunc);
extern LOGICAL	piCreateSurface(piSURFACEDESC* lppisd, void** lpdds);
extern LOGICAL piSetCockpitViewMatrix(piMATRIX* newview);
extern LOGICAL	piMaterialHasSpecular(SINT32 matid);
extern LOGICAL	piSetTextureName(SINT32 texid, SINT8* name);
extern LOGICAL piGetTextureMemoryDescriptor(SINT32 texid, SINT32 miplevel, piSURFACEDESC* lpddsd);
extern LOGICAL	piProjectSceneOnTexture(SINT32 texid, SINT32 miplevel, piDRAW_OBJECTS DrawObjects);
extern LOGICAL piLockTextureMemorySurface(SINT32 texid, SINT32 miplevel, piSURFACEDESC* lpddsd);
extern LOGICAL	piGetTextureUVAlpha(UINT32* alpha, SINT32 texid, SINT32 miplevel, FLOAT32 u, FLOAT32 v);
extern LOGICAL	piGetViewportDimensions(SINT32* x, SINT32* y, SINT32* width, SINT32* height, FLOAT32* znear, FLOAT32* zfar);
extern LOGICAL	piSetViewportDimensions(SINT32 x, SINT32 y, SINT32 width, SINT32 height, FLOAT32 znear, FLOAT32 zfar);
extern LOGICAL piGetViewportZ(FLOAT32* znear, FLOAT32* zfar);
extern LOGICAL piSetViewportZ(FLOAT32 znear, FLOAT32 zfar);
extern LOGICAL piRenderScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, void* lpVertices, UINT32 VertexCount);
extern LOGICAL piRenderIndexScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, void* lpVertices, UINT32 VertexCount, void* lpIndices, UINT32 IndexCount);
extern LOGICAL	piSetRenderStageState(UINT32 stage, piTEXTURESTAGESTATETYPE dwState, UINT32 dwValue);
extern LOGICAL piGetTextureFactors(SINT32 texid, SINT32 miplevel, FLOAT32* widthfactor, FLOAT32* heightfactor);
extern LOGICAL	piRenderIndexStridedScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, LPVOID lpStridedData, UINT32 VertexCount, UINT16* lpIndices, UINT32 IndexCount);
extern LOGICAL	piRenderStridedScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, LPVOID lpStridedData, UINT32 VertexCount);

extern SINT32 piGetNewTextureID();
extern SINT32 piGetTotalPaints(void);
extern SINT32 piGetHeight(void);
extern SINT32 piGetWidth(void);
extern SINT32 piGetNumberVertices(void);
extern SINT32 piGetNamedTexture(const char* name);
extern SINT32 piGetTextureMipmapLevel(SINT32 texid);
extern LOGICAL piLoadTexture(SINT32 texid, SINT32 miplevel);
extern SINT32 piGetMaterialTexId(SINT32 matid);
extern SINT32 piCreateMaterial(piMATERIAL* lpmat);
extern SINT32 piGetLights(LIGHT** lplight);
extern SINT32 piGetMaterials(MATERIAL** lpmat);
extern SINT32 piFloatLikeLong(FLOAT32 f);
extern SINT32 piCreateRawTexture(SINT32 texid, SINT32 miplevel, SINT32 width, SINT32 height, SINT32 bpp, SINT32 alphabits, SINT32 hardware);

extern UINT32 piGetAmbLight(void);
extern UINT32 piColorToDescColor(piSURFACEDESC* lpddsd, UINT32 color);

extern void piKill(void);
extern void piSetMinTextureDim(SINT32 mintexdim);
extern void piUnloadTexture(SINT32 texid, SINT32 miplevel);
extern void piFreeTexture(SINT32 texid, SINT32 miplevel);
extern void piFreeMipTextures(SINT32 texid);
extern void piFreeMipTexture3Ds(SINT32 texid);
extern void piFreeMaterial(SINT32 matid);
extern void piSetAmbLight(UINT32 color);
extern void piResetFPS(void);
extern UINT32 piCapture(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero);
extern void piResetRenderStats(void);
extern void piResetRenderBuckets(void);
extern void piDisplayRenderBuckets(void);
extern void piDisplayRenderIndexBuckets(void);
extern void piGetRenderTypes(UINT32* draw, UINT32* drawindex);
extern void piGetRenderStats(UINT32* points, UINT32* lines, UINT32* triangles, UINT32* vertices);
extern void* piGetInstance(void);
extern void piPostRenderFilterScene();

extern void piLockState(SINT32 state, LOGICAL lock);
extern void piLockTextureState(LOGICAL lock);
extern void piLockMaterialState(LOGICAL lock);
extern void piLockWorldMatrix(LOGICAL lock);
extern void piLockViewMatrix(LOGICAL lock);
extern void piLockProjectionMatrix(LOGICAL lock);
extern void piLockTextureMatrix(LOGICAL lock);

extern FLOAT32 piGetFPS(void);
extern FLOAT32 piGetFieldOfViewZMin();
extern FLOAT32	piGetFieldOfViewZMax();
extern LOGICAL piGetWorldToScreen(double x, double y, double z, double& sx, double& sy, double& sz);

extern piMATRIX* piGetProjectionMatrix(void);
extern piMATRIX* piGetWorldMatrix(void);
extern piMATRIX* piGetViewMatrix(void);
extern piMATRIX* piGetUnscaledViewMatrix(void);
extern piMATRIX* piGetCockpitViewMatrix(void);

extern SINT32 piGetTextureHandle(SINT32 texid, SINT32 miplevel);
extern SINT32 piGetTexture3DHandle(SINT32 texid);

extern TEXTURE* piGetTextureData(SINT32 texid);
extern TEXTURE* piGetTextureData2(SINT32 texid);
extern TEXTURE3D* piGetTexture3DData(SINT32 texid);

extern void piCloseProgram(void);
extern void piExitProgram(const SINT8* file, SINT32 line, const SINT8* MessageString);

extern piMATRIX* piGetTextureMatrix(int unit = 0);

extern LOGICAL piSetLight(SINT32 numlight, LIGHT* light);
extern SINT32 piCreateLight(LIGHT* light);

extern void piReleaseVertexSource(SINT32);
extern void piReleaseIndexSource(SINT32);
extern VERTEXSOURCE* piGetVertexData(SINT32);
extern LOGICAL piSetVertexSource(SINT32,UINT32,UINT32);
extern INDEXSOURCE* piGetIndexData(SINT32);
extern LOGICAL piSetIndexSource(SINT32,UINT32);
extern LOGICAL piRenderSourceIndexedScene(piPRIMITIVETYPE,UINT32,UINT32,UINT32,UINT32);
extern void piResetIndexSource(void);
extern LOGICAL piRenderSourceScene(piPRIMITIVETYPE,UINT32,UINT32);
extern void piResetVertexSource(void);
extern void piDisableRenderStates();

extern void piCreateModelView(piMATRIX* returnMatrix,
					   piVECTOR* eye,
					   piVECTOR* center,
					   piVECTOR* up,
					   piQUAT* axisangle);

extern void piDrawScene(UINT32 mode,
				 FLOAT32* coords,
				 FLOAT32* normals,
				 FLOAT32* texcoords,
				 UINT32* colors,
				 UINT16* indices,
				 SINT32 stride,
				 SINT32 vertexcount,
				 SINT32 indexcount);

extern SINT32 piConvertTextureFormat(int texid, UINT32 format);
extern SINT32 piGetActiveMaterial();
extern UINT32 piGetEngineRenderState(UINT32 state);
extern UINT32 piGetEngineRenderStageState(UINT32 stage, UINT32 state);
extern bool piIsMaterialInterior(int id);
extern void piSetViewMatrix(const piMATRIX* newview);
extern void piSetWorldMatrix(const piMATRIX* newworld);
extern void piSetProjectionMatrix(const piMATRIX* newproj);
extern void piSetTextureMatrix(const piMATRIX* newtexmat);
