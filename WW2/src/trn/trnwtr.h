#pragma once

#include "ob/obmain.h"

enum WATER_PARAMS
{
    HDR_MULTIPLIER,
    FRESNEL_BIAS,
    FRESNEL_SCALE,
    FRESNEL_POWER,
    BUMP_SCALE,
    WATER_AMOUNT,
    REFLECTION_AMOUNT,
    BUMP_SPEED,
    TEXTURE_SCALE,
    SHALLOW_COLOR,
    DEEP_COLOR,
    REFLECTION_COLOR,
    MAX_WATER_PARAMS
};

extern void trnwtrDrawReplacementWater(OBJECT* o);
extern void trnwtrDrawNonReplacementWater(OBJECT* o);

extern void trnwtrSetShaderFloat(WATER_PARAMS param, FLOAT32 value);
extern void trnwtrSetShaderFloatArray(WATER_PARAMS param, SINT32 index, FLOAT32 value);
extern void trnwtrSetShaderColor(WATER_PARAMS param, SINT32 index, FLOAT32 value);
extern void trnwtrToggle();
extern void trnwtrSetRenderPass();

extern FLOAT32 trnwtrGetShaderFloat(WATER_PARAMS type);
extern FLOAT32* trnwtrGetShaderFloatArray(WATER_PARAMS type);
extern UINT32 trnwtrGetShaderColor(WATER_PARAMS type);

extern SINT32 trnwtrGetTexture(SINT32& texid, SINT32& depth);

#define USE_WATER_EFFECT
//#undef USE_WATER_EFFECT

	extern LOGICAL trnwtrInit(SINT8* name);
	extern void trnwtrKill(void);

	extern LOGICAL trnwtrIsWaterTexture(SINT32 texid);

	extern LOGICAL trnwtrUpdate(void);
	extern LOGICAL trnwtrPreDraw(OBJECT* lpobj);
	extern LOGICAL trnwtrPreDraw2(OBJECT* lpobj);
	extern LOGICAL trnwtrPreDrawMultitexture(OBJECT* lpobj, piMATRIX* mtmat);
	extern LOGICAL trnwtrPostDraw(void);
    extern LOGICAL trnwtrIsWaterDrawn(FLOAT32& maxheight, FLOAT32& distance);
