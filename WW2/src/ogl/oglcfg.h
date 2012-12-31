#pragma once

	// Defines
	#define ONE_OVER_255						0.003921568627450980392156862745098f

	#define oglRGBA_GETALPHA(rgba)				(UINT32)(((rgba) >> 24) & 0xff)
	#define oglRGBA_GETRED(rgba)				(UINT32)(((rgba) >> 16) & 0xff)
	#define oglRGBA_GETGREEN(rgba)				(UINT32)(((rgba) >> 8) & 0xff)
	#define oglRGBA_GETBLUE(rgba)				(UINT32)((rgba) & 0xff)
	#define oglRGBA_SETALPHA(rgba, a)			(UINT32)(((rgba) & 0x00ffffff) | ((a) << 24))
	#define oglRGB_MAKE(r, g, b)				(UINT32)((((r) << 16) | ((g) << 8) | (b)))
	#define oglRGBA_MAKE(r, g, b, a)			(UINT32)((((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
	#define oglRGBA_TO_RGB(rgba)				(UINT32)(((rgba) & 0xffffff))
	#define oglRGB_TO_RGBA(rgb)					(UINT32)(((rgb) | 0xff000000))
	#define oglRGB_MAKE_FROM_VALUE(r,b,g)		oglRGB_MAKE((UINT32)((r) * 255.0f), (UINT32)((g) * 255.0f), (UINT32)((b) * 255.0f))
	#define oglRGB_MAKE_FROM_COLOR(r,b,g)		oglRGB_MAKE((UINT32)((FLOAT32)(r) * ONE_OVER_255), (UINT32)((FLOAT32)(g) * ONE_OVER_255), (UINT32)((FLOAT32)(b) * ONE_OVER_255))
	#define oglRGBA_MAKE_FROM_VALUE(r,g,b,a)	oglRGBA_MAKE((UINT32)((r) * 255.0f), (UINT32)((g) * 255.0f), (UINT32)((b) * 255.0f), (UINT32)((a) * 255.0f))
	#define oglRGBA_MAKE_FROM_COLOR(r,g,b,a)	oglRGBA_MAKE((UINT32)((FLOAT32)(r) * ONE_OVER_255), (UINT32)((FLOAT32)(g) * ONE_OVER_255), (UINT32)((FLOAT32)(b) * ONE_OVER_255), (UINT32)((FLOAT32)(a) * ONE_OVER_255))

