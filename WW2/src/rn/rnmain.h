#pragma once

#include "pi/pi.h"

	extern void rnmainRotatePlayer(piVECTOR* rot);
	extern void rnmainMovePlayer(piVECTOR* pos);
	extern void rnmainResetPlayer();
	extern void rnmainSetPlayerInfo(piVECTOR* pos, piVECTOR* rot);
    extern void rnmainSetViewMatrix(piVECTOR* pos, piVECTOR* att);
    extern void rnmainSetViewMatrix2(piMATRIX* view);
    extern void rnmainSetDefaultFogStart(FLOAT32 start);
    extern void rnmainSetDefaultFogEnd(FLOAT32 end);
    extern void rnmainSetDefaultFogColor(UINT32 color);

    extern FLOAT32 rnmainGetDefaultFogStart();
    extern FLOAT32 rnmainGetDefaultFogEnd();
    extern float rnmainGetZBufferNearVal();

	extern LOGICAL rnmainSetDefaultRenderStates();
	extern LOGICAL rnmainInit();
	extern void rnmainKill();
	extern LOGICAL rnmainFOVChange();
	extern LOGICAL rnmainListText(const char* text, ...);
	extern LOGICAL rnmainListText(const wchar_t* text, ...);
	extern LOGICAL rnmainSetFogAttributes();
	extern void  rnUpdateView();
	extern LOGICAL rnmainAdjustFogAttributes(UINT32 FogColor,FLOAT32 FogStartFloat,FLOAT32 FogEndFloat);
	extern LOGICAL rnmainGetDefaultFogAttributes(UINT32 * OutFogColor, FLOAT32 * OutFogStart, FLOAT32 * OutFogEnd);
	extern LOGICAL rnmainMoveView(FLOAT64 PosX, FLOAT64 PosY);

    extern UINT32 rnmainGetViewScaleFactor(FLOAT32 * ScaleFactor);
	extern UINT32 rnmainSetViewScaleFactor(FLOAT32 ScaleFactor);
	extern UINT32 rnmainSetZBufferNearVal(FLOAT32 ZBufferNearVal);
	extern UINT32 rnmainGetZBufferNearVal(FLOAT32* ZBufferNearVal);
	extern UINT32 rnmainGetDefaultFogColor();

	extern LOGICAL rnmainListLocText(SINT32 category, SINT32 locID);
	extern LOGICAL rnmainListColorText(UINT32 uColor, const WCHAR* text, ...);
	extern LOGICAL rnmainListLocColorText(SINT32 category, SINT32 locID, UINT32 uColor);
	extern LOGICAL rnmainListEmoteText(SINT32 category, SINT32 locID, UINT32 uColor, const WCHAR* prepend, const WCHAR* append);

	extern void rnsetServerInfo(UINT32 serverip, UINT32 serverport, UINT32 otp);
	extern void rnconnectToServer(void);
