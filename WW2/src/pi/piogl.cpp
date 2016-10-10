#include <syscfg.h> // Path is project specific.
//

#ifdef OPENGL3D

#include <time.h>
#include "ddp.h"
#include "ogl/ogl.h"
#include "pi/pi.h"
#include "mem/mem.h"
#include "versionNo.h"
#include "sysmain.h"
#include "dimain.h"
#include "fio/fio.h"

#if platOS == OS_WIN32
extern HWND			myhwnd;
extern LOGICAL		running;
#endif

#include <vector>
#include <set>

SINT32 DefaultMaterialId;

SINT32 piFloatLikeLong(FLOAT32 f)
{
struct
{
	union
	{
		FLOAT32 f;
		SINT32 l;
	};
} fll;

	fll.f = f;
	return(fll.l);
}

FLOAT32 piLongLikeFloat(SINT32 l)
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

SINT32 piFindBitShift(UINT32 Mask)
{
SINT32 i;

	for (i=0, Mask=~Mask; Mask & 1; Mask>>=1, i++) /* Empty */;

	return(i);
}

SINT32 piFindBitCount(UINT32 bits)
{
	unsigned int c = 0;
/* Naieve version.
	// Performance: Intel Compiler with /OX for values 0 to 1^28 = 2600ms.

	while(bits)
	{
		if(bits &1) ++c;
		bits >>= 1;
	}
*/
/* Brian Kernighan's method (see C Programming Language 2nd Ed, B.W.Kernighan & Dennis M. Ritchie, 1988).
	// Performance: Intel Compiler with /OX for values 0 to 1^28 = 4000ms.

	for (c = 0; bits; c++)
	{
		bits &= bits - 1; // clear the least significant bit set
	}
*/
/* Version optimized for modern CPUs.
	Avoids branching. Intel Compiler with /OX produces code that is 8.6x faster.
*/

	// Performance: Intel Compiler with /OX for values 0 to 1^28 = 310ms.
	static const UINT32 B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};

	c = bits - ((bits >> 1) & B[0]);
	c = ((c >> 2) & B[1]) + (c & B[1]);
	c = ((c >> 4) + c) & B[2];
	c = ((c >> 8) + c) & B[3];
	c = ((c >> 16) + c) & B[4];

	return c;
}

LOGICAL piInit(void)
{
	return(oglInit());
}

void piKill(void)
{
	dimainKill();
	oglKill();
	Write_Log(LOG_OLD, LOG_INFO, "[piKill]");
}

LOGICAL piClearTargets(UINT32 targets)
{
	return(oglClearTargets(targets));
}

LOGICAL piClearPorts(piRECT piRect, UINT32 uTargets)
{
	return oglClearPorts(piRect, uTargets);
}

SINT32 piGetNumberVertices(void)
{
	return(oglGetNumberVertices());
}


LOGICAL piIsTexture(SINT32 texid, SINT32 miplevel)
{
	return(oglIsTexture(texid, miplevel));
}

LOGICAL piIsTexture3D(SINT32 texid)
{
	return(oglIsTexture3D(texid));
}

LOGICAL piTextureHasAlpha(SINT32 texid)
{
	return(oglTextureHasAlpha(texid));
}

SINT32 piGetNamedTexture(const char* name)
{
	return(oglGetNamedTexture(name));
}

LOGICAL piAddTexRef(SINT32 texid)
{
	return(oglAddTexRef(texid));
}

void piUpdateTextures(void)
{
	return(oglUpdateTextures());
}

TEXTURE* piGetTextureData(SINT32 texid)
{
	return(oglGetTextureData(texid));
}

TEXTURE* piGetTextureData2(SINT32 texid)
{
	return(oglGetTextureData2(texid));
}

TEXTURE3D* piGetTexture3DData(SINT32 texid)
{
	return(oglGetTexture3DData(texid));
}

void piSetMinTextureDim(SINT32 mintexdim)
{
	oglSetMinTextureDim(mintexdim);
}

LOGICAL piGetTextureMemory(SINT32* total)
{
int free;

	return(oglGetTextureMemory((int*)total, (int*)&free));
}

SINT32 piGetTextureMipmapLevel(SINT32 texid)
{
	return(oglGetTextureMipmapLevel(texid));
}

LOGICAL piReleaseTextureHandles(void)
{
	return(oglReleaseTextureHandles());
}

SINT32 piGetTextureHandle(SINT32 texid, SINT32 miplevel)
{
	return(oglGetTextureHandle(texid, miplevel));
}

SINT32 piGetTexture3DHandle(SINT32 texid)
{
	return(oglGetTexture3DHandle(texid));
}

LOGICAL piLockTextureMemorySurface(SINT32 texid, SINT32 miplevel, piSURFACEDESC* lppisd)
{
	return piGetTextureMemoryDescriptor(texid, miplevel, lppisd);
}

LOGICAL piUnlockTextureMemorySurface(SINT32 texid, SINT32 miplevel)
{
	if (!oglLoadTexture(texid, miplevel))
	{
		return(FALSE);
	}

	return(TRUE);
}

LOGICAL piGetTextureMemoryDescriptor(SINT32 texid, SINT32 miplevel, piSURFACEDESC* lppisd)
{
TEXTURE*	texture;

	texture = piGetTextureData(texid);
	if (texture == nullptr)
		return FALSE;

	lppisd->Width       = texture->po2width[miplevel];
	lppisd->Height      = texture->po2height[miplevel];
	lppisd->Pitch       = texture->comp[miplevel] * lppisd->Width;
	lppisd->Surface     = texture->lpsurf[miplevel];

	if (lppisd->Surface == nullptr)
		return FALSE;

	lppisd->PixelFormat.BitCount = texture->comp[miplevel] * 8;

	lppisd->PixelFormat.RedBitMask		= 0x00ff0000;
	lppisd->PixelFormat.GreenBitMask	= 0x0000ff00;
	lppisd->PixelFormat.BlueBitMask		= 0x000000ff;

	lppisd->PixelFormat.dwRBitMask		= 0x00ff0000;
	lppisd->PixelFormat.dwGBitMask		= 0x0000ff00;
	lppisd->PixelFormat.dwBBitMask		= 0x000000ff;

	if (texture->comp[miplevel] == 4)
	{
		lppisd->PixelFormat.dwRGBAlphaBitMask = 0xff000000;
	}
	else
	{
		lppisd->PixelFormat.dwRGBAlphaBitMask = 0x00000000;
	}

	return(TRUE);
}

LOGICAL piSetTextureName(SINT32 texid, SINT8* name)
{
	return oglSetTextureName(texid, name);
}

LOGICAL piRenderIndexStridedScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, LPVOID lpStridedData, UINT32 VertexCount, UINT16* lpIndices, UINT32 IndexCount)
{
	return(oglRenderIndexStridedScene((oglPRIMITIVETYPE)pipt, (oglVERTEXTYPE)pivt, lpStridedData, VertexCount, lpIndices, IndexCount));
}

LOGICAL	piRenderStridedScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, LPVOID lpStridedData, UINT32 VertexCount)
{
	return oglRenderStridedScene((oglPRIMITIVETYPE)pipt, (oglVERTEXTYPE)pivt, lpStridedData, VertexCount);
}

LOGICAL piProjectSceneOnTexture(SINT32 texid, SINT32 miplevel, piDRAW_OBJECTS DrawObjects)
{
	return(oglProjectSceneOnTexture(texid, miplevel, DrawObjects));
}

void piUnloadTexture(SINT32 texid, SINT32 miplevel)
{
	oglUnloadTexture(texid, miplevel);
}

void piFreeTexture(SINT32 texid, SINT32 miplevel)
{
	oglFreeTexture(texid, miplevel);
}

void piFreeMipTextures(SINT32 texid)
{
	oglFreeMipTextures(texid);
}

void piFreeMipTexture3Ds(SINT32 texid)
{
	oglFreeMipTexture3Ds(texid);
}

LOGICAL piSetActiveTexture(SINT32 texid, SINT32 stage)
{
	// The miplevel (parameter 2) becomes the stage for multitexturing
	// The order was changed in the ogl call to cause the texture
	// stages to break if improperly used thus giving you immediate
	// visual indication that a problem exists.
	return(oglSetActiveTexture(stage, texid));
}

LOGICAL piEnableTexture()
{
	oglEnableTexture();
	return TRUE;
}

LOGICAL piDisableTexture(void)
{
	oglDisableTexture();
	return TRUE;
}

UINT32 piColorToDescColor(piSURFACEDESC* lppisd, UINT32 color)
{
	return(oglColorToDescColor(lppisd, color));
}

SINT32 piCreateRawTexture(SINT32 texid, SINT32 miplevel, SINT32 width, SINT32 height, SINT32 bpp, SINT32 alphabits, SINT32 flags)
{
	return(oglCreateRawTexture(texid, miplevel, width, height, bpp, alphabits, flags));
}

//////////////////////////////////////////////////////////////////////
// Function: piGetNewTextureID
// Description: Retrieve a new texture ID without going through piCreateRawTexture.
// This is used for loading DDS texture formats.
//////////////////////////////////////////////////////////////////////
SINT32 piGetNewTextureID()
{
	return oglGetNewTextureID();
}

LOGICAL piLoadTexture(SINT32 texid, SINT32 miplevel)
{
	return(oglLoadTexture(texid, miplevel));
}

LOGICAL piSetRenderStageState(UINT32 stage, piTEXTURESTAGESTATETYPE dwState, UINT32 dwValue)
{
	return(oglSetRenderStageState(stage, (oglTEXTURESTAGESTATETYPE)dwState, dwValue));
}

LOGICAL piGetTextureFactors(SINT32 texid, SINT32 miplevel, FLOAT32* widthfactor, FLOAT32* heightfactor)
{
	return(oglGetTextureFactors(texid, miplevel, widthfactor, heightfactor));
}

LOGICAL piIsMaterial(SINT32 matid)
{
	return(oglIsMaterial(matid));
}

SINT32 piGetMaterialTexId(SINT32 matid)
{
	return(oglGetMaterialTexId(matid));
}

LOGICAL piGetMaterialTexName(SINT32 matid, char* texname)
{
	return(oglGetMaterialTexName(matid, texname));
}

SINT32 piFindMaterialName(const SINT8* name)
{
	return(oglFindMaterialName(name));
}

bool piIsMaterialInterior(int id)
{
	static std::set<int> _int;

	static bool firsttime = true;

	if(firsttime)
	{
		firsttime = false;

		MATERIAL* m = nullptr;

		int num = oglGetMaterials(&m);

		for(int i = 0; i < num; i++)
		{
			if(!strncmp(m[i].name, "interior", 8))
			{
				_int.insert(m[i].matid);
				continue;
			}

			if(!strncmp(m[i].name, "int_", 4))
			{
				_int.insert(m[i].matid);
				continue;
			}

			if(!strncmp(m[i].name, "dark int", 8))
			{
				_int.insert(m[i].matid);
				continue;
			}

			if(!strncmp(m[i].name, "med int", 7))
			{
				_int.insert(m[i].matid);
				continue;
			}
		}
	}

	if(id < 0)
	{
		return false;
	}

	if(_int.find(id) != _int.end())
	{
		return true;
	}

	return false;
}

void piFreeMaterial(SINT32 matid)
{
	oglFreeMaterial(matid);
}

SINT32 piCreateMaterial(piMATERIAL* lpmat)
{
	return(oglCreateMaterial(lpmat));
}

LOGICAL piSetActiveMaterial(SINT32 matid)
{
	return(oglSetActiveMaterial(matid));
}

LOGICAL piDisableMaterial(void)
{
	return(oglSetActiveMaterial(DefaultMaterialId));
//	return(oglDisableMaterial());
}

SINT32 piGetLights(LIGHT** lplight)
{
	return(oglGetLights(lplight));
}

UINT32 piGetAmbLight(void)
{
	return(oglGetAmbLight());
}

void piSetAmbLight(UINT32 color)
{
	oglSetAmbLight(color);
}

SINT32 piGetMaterials(MATERIAL** lpmat)
{
	return(oglGetMaterials(lpmat));
}

piMATRIX* piGetProjectionMatrix(void)
{
	return(oglGetProjectionMatrix());
}

void piSetProjectionMatrix(const piMATRIX* newproj)
{
	oglSetProjectionMatrix(newproj);
}

piMATRIX* piGetWorldMatrix(void)
{
	return(oglGetWorldMatrix());
}

void piSetWorldMatrix(const piMATRIX* newworld)
{
	oglSetWorldMatrix(newworld);
}

piMATRIX* piGetViewMatrix(void)
{
	return(oglGetViewMatrix());
}

void piSetViewMatrix(const piMATRIX* newview)
{
	oglSetViewMatrix(newview);
}

LOGICAL piSetFieldOfView(FLOAT32 fov, FLOAT32 zmin, FLOAT32 zmax)
{
	return(oglSetFieldOfView(fov, zmin, zmax));
}

LOGICAL piGetViewportDimensions(SINT32* x, SINT32* y, SINT32* width, SINT32* height, FLOAT32* znear, FLOAT32* zfar)
{
	return(oglGetViewportDimensions(x, y, width, height, znear, zfar));
}

LOGICAL piSetViewportDimensions(SINT32 x, SINT32 y, SINT32 width, SINT32 height, FLOAT32 znear, FLOAT32 zfar)
{
	return(oglSetViewportDimensions(x, y, width, height, znear, zfar));
}

LOGICAL piGetViewportZ(FLOAT32* znear, FLOAT32* zfar)
{
	return(oglGetViewportZ(znear, zfar));
}

LOGICAL piSetViewportZ(FLOAT32 znear, FLOAT32 zfar)
{
	return(oglSetViewportZ(znear, zfar));
}

LOGICAL piBeginScene(void)
{
	return(oglBeginScene());
}

LOGICAL piEndScene(void)
{
	return(oglEndScene());
}

void piSetRenderState(int StateType, UINT32 State)
{
	oglSetRenderState((oglRENDERSTATETYPES)StateType, State);
}

LOGICAL piDisplayRenderStates(void)
{
	return(oglDisplayRenderStates());
}

LOGICAL piSetLightState(SINT32 lightNum, UINT32 State)
{
	return(oglSetLightState(lightNum, State));
}

void piResetRenderStats(void)
{
	oglResetRenderStats();
}

void piGetRenderStats(UINT32* points, UINT32* lines, UINT32* triangles, UINT32* vertices)
{
	oglGetRenderStats(points, lines, triangles, vertices);
}

void piGetRenderTypes(UINT32* draw, UINT32* drawindex)
{
	oglGetRenderTypes(draw, drawindex);
}

void piResetRenderBuckets(void)
{
	oglResetRenderBuckets();
}

void piDisplayRenderBuckets(void)
{
	oglDisplayRenderBuckets();
}

void piDisplayRenderIndexBuckets(void)
{
	oglDisplayRenderIndexBuckets();
}

LOGICAL piRenderScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, LPVOID lpVertices, UINT32 VertexCount)
{
	return(oglRenderScene((oglPRIMITIVETYPE)pipt, (oglVERTEXTYPE)pivt, lpVertices, VertexCount));
}

LOGICAL piRenderIndexScene(piPRIMITIVETYPE pipt, piVERTEXTYPE pivt, LPVOID lpVertices, UINT32 VertexCount, void* lpIndices, UINT32 IndexCount)
{
	return(oglRenderIndexScene((oglPRIMITIVETYPE)pipt, (oglVERTEXTYPE)pivt, lpVertices, VertexCount, lpIndices, IndexCount));
}

void* piGetInstance(void)
{
#if platOS == OS_WIN32
	return(ddpGetInstance());
#elif platOS == OS_MACOS
	return nullptr;
#elif platOS == OS_LINUX
	return nullptr;
#else
	#error "something here?"
#endif
}

void piPostRenderFilterScene()
{
}

SINT32 piGetTotalPaints(void)
{
	return(ddpGetTotalPaints());
}

SINT32 piGetHeight(void)
{
	return(ddpGetHeight());
}

SINT32 piGetWidth(void)
{
	return(ddpGetWidth());
}

FLOAT32 piGetFPS(void)
{
	return(ddpGetFPS());
}

FLOAT32 piGetFieldOfViewZMin()
{
	return oglGetFieldOfViewZMin();
}

FLOAT32 piGetFieldOfViewZMax()
{
	return oglGetFieldOfViewZMax();
}

LOGICAL piGetWorldToScreen(double x, double y, double z, double& sx, double& sy, double& sz)
{
	return oglGetWorldToScreen(x, y, z, sx, sy, sz);
}

void piResetFPS(void)
{
	ddpResetFPS();
}

UINT32 piCapture(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero)
{
	if(keyState == 1)
	{
		ddpCapture();
	}

	return 1;
}

LOGICAL piFlip(void)
{
	return(ddpFlip());
}

LOGICAL piSetDDRender(DDWND_DDRENDERFUNC newfunc)
{
	return(ddpSetDDRender(newfunc));
}

LOGICAL piSetD3DRender(DDWND_D3DRENDERFUNC newfunc)
{
	return(ddpSetD3DRender(newfunc));
}

LOGICAL piCreateSurface(piSURFACEDESC* lppisd, void** lpdds)
{
	lppisd->PixelFormat.dwRBitMask = 0xff0000;
	lppisd->PixelFormat.dwGBitMask = 0x00ff00;
	lppisd->PixelFormat.dwBBitMask = 0x0000ff;

	lppisd->Pitch = lppisd->Width * 3;
	lppisd->PixelFormat.dwRGBBitCount = 24;
	lppisd->PixelFormat.dwRGBAlphaBitMask = 0;

	// Surfaces are always 24 bit RGB
	lppisd->Surface = memAlloc(lppisd->Width * lppisd->Height * 3);

	*lpdds = (piSURFACEDESC*)memAlloc(sizeof(piSURFACEDESC));
	memcpy(*lpdds, lppisd, sizeof(piSURFACEDESC));

	return(TRUE);
}

LOGICAL piLockSurface(void* lpdds, piSURFACEDESC* lppisd)
{
	piSURFACEDESC* pd = (piSURFACEDESC*)lpdds;

	lppisd->Width = pd->Width;
	lppisd->Height = pd->Height;
	lppisd->Pitch = pd->Pitch;
	lppisd->Surface = pd->Surface;

	memcpy(&lppisd->PixelFormat, &pd->PixelFormat, sizeof(piPIXELFORMAT));

	return(TRUE);
}

void piCloseProgram(void)
{
	Write_Debug("piCloseProgram was called");

#if platOS == OS_WIN32
	running = 0;
	PostMessageA(myhwnd, WM_CLOSE, 0, 0);
#endif

	exit(0);
}

LOGICAL	piGetTextureUVAlpha(UINT32* alpha, SINT32 texid, SINT32 miplevel, FLOAT32 u, FLOAT32 v)
{
	return(oglGetTextureUVAlpha(alpha, texid, miplevel, u, v));
}

piMATRIX* piGetCockpitViewMatrix(void)
{
	return(oglGetCockpitViewMatrix());
}

LOGICAL piSetCockpitViewMatrix(piMATRIX* newview)
{
	return(oglSetCockpitViewMatrix(newview));
}

LOGICAL piMaterialHasSpecular(SINT32 matid)
{
	return(oglMaterialHasSpecular(matid));
}

piMATRIX* piGetUnscaledViewMatrix(void)
{
	return(oglGetUnscaledViewMatrix());
}

LOGICAL piSetUnscaledViewMatrix(piMATRIX* lpNewMatrix)
{
	return(oglSetUnscaledViewMatrix(lpNewMatrix));
}

piMATRIX* piGetTextureMatrix(int unit)
{
	return &oglGetTextureMatrix(unit);
}

void piSetTextureMatrix(const piMATRIX* newtexmat)
{
	oglSetTextureMatrix(newtexmat);
}

void piExitProgram(const SINT8* file, SINT32 line, const SINT8* MessageString)
{
	FILE* fp = Fio::OpenLogFile();

#if platOS == OS_WIN32
	running = 0;

#if defined(DEBUG_WW2) || defined(BETA_RELEASE)
	if(MessageString)
	{
		static const char msgFmt[] = "Application crashed and will now exit.\n\nFile: %s\nLine: %u\nMessage: %s\n";
		const size_t fileNameLen = strlen(file);
		const size_t msgLen = strlen(MessageString);
		char* msg   = (char*)alloca(sizeof(msgFmt) + fileNameLen + msgLen + 16);
		sprintf(msg, msgFmt, file, line, MessageString);
		MessageBoxA(nullptr, msg, "Application crash/exit", MB_ICONERROR | MB_OK);
	}
#endif
	PostMessageA(myhwnd, WM_CLOSE, 0, 0);
#endif
    
	abort(); // Abort raises SIGABRT, which will be trapped by the crash handler
}

extern SINT32	frametime;

LOGICAL piSetLight(SINT32 numlight, LIGHT* light)
{
	return oglSetLight(numlight, light);
}

SINT32 piCreateLight(LIGHT* light)
{
	return oglCreateLight(&light->light);
}

void piReleaseVertexSource(int srcid)
{
}

void piReleaseIndexSource(int srcid)
{
}

VERTEXSOURCE* piGetVertexData(int srcid)
{
	return nullptr;
}

LOGICAL piSetVertexSource(SINT32 srcid, UINT32 Stride, UINT32 FVF)
{
	return FALSE;
}

INDEXSOURCE* piGetIndexData(int srcid)
{
	return nullptr;
}

LOGICAL piSetIndexSource(SINT32 srcid, UINT32 unique)
{
	return TRUE;
}

LOGICAL piRenderSourceIndexedScene(piPRIMITIVETYPE PrimitiveType, UINT32 MinIndex, UINT32 NumVertices, UINT32 StartIndex, UINT32 PrimitiveCount)
{
	return TRUE;
}

void piResetIndexSource(void)
{
}

LOGICAL piRenderSourceScene(piPRIMITIVETYPE PrimitiveType, UINT32 StartVertex, UINT32 PrimitiveCount)
{
	return TRUE;
}

void piResetVertexSource(void)
{
}

SINT32 piConvertTextureFormat(SINT32 texid, UINT32 format)
{
	return 0;
}

UINT32 piGetEngineRenderState(UINT32 state)
{
	return oglGetEngineRenderState(state);
}

UINT32 piGetEngineRenderStageState(UINT32 stage, UINT32 state)
{
	return oglGetEngineRenderStageState(stage, state);
}

void piDisableRenderStates()
{
	oglDisableRenderStates();
}

void piLockState(SINT32 state, LOGICAL lock)
{
	oglLockState(state, lock);
}

void piLockTextureState(bool lock)
{
	oglLockTextureState(lock);
}

void piLockMaterialState(LOGICAL lock)
{
	oglLockMaterialState(lock);
}

void piLockWorldMatrix(LOGICAL lock)
{
	oglLockWorldMatrix(lock);
}

void piLockViewMatrix(LOGICAL lock)
{
	oglLockViewMatrix(lock);
}

void piLockProjectionMatrix(LOGICAL lock)
{
	oglLockProjectionMatrix(lock);
}

void piLockTextureMatrix(LOGICAL lock)
{
	oglLockTextureMatrix(lock);
}

void piCreateModelView(piMATRIX* returnMatrix,
					   piVECTOR* eye,
					   piVECTOR* center,
					   piVECTOR* up,
					   piQUAT* axisangle)
{
	oglCreateModelView(returnMatrix, eye, center, up, axisangle);
}

SINT32 piGetActiveMaterial()
{
	return oglGetActiveMaterial();
}

void piDrawScene(UINT32 mode,
				 FLOAT32* coords,
				 FLOAT32* normals,
				 FLOAT32* texcoords,
				 UINT32* colors,
				 UINT16* indices,
				 SINT32 stride,
				 SINT32 vertexcount,
				 SINT32 indexcount)
{
	oglDrawScene(mode,
				 coords,
				 normals,
				 texcoords,
				 colors,
				 indices,
				 stride,
				 vertexcount,
				 indexcount);
}

#endif
