#pragma once
// Copyright 2008 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

LOGICAL draw2dPoint(SINT32 x, SINT32 y, FLOAT32 z, UINT32 Color);
LOGICAL draw2dLine(SINT32 x1, SINT32 y1, SINT32 x2, SINT32 y2, FLOAT32 z, UINT32 Color);
LOGICAL draw2dRectangle(SINT32 x1, SINT32 y1, SINT32 x2, SINT32 y2, FLOAT32 z, UINT32 Color);
LOGICAL draw2dTransparentRectangle(SINT32 x1, SINT32 y1, SINT32 x2, SINT32 y2, FLOAT32 z, UINT32 Color);
LOGICAL draw2dTexture(SINT32 texid, SINT32 miplevel, SINT32 x, SINT32 y, FLOAT32 z, SINT32 Width, SINT32 Height, UINT32 Color);
LOGICAL draw2dTexture2(SINT32 texid, SINT32 miplevel, SINT32 x, SINT32 y, FLOAT32 z, SINT32 Width, SINT32 Height, UINT32 Color);
LOGICAL draw2dSubTexture(SINT32 texid, SINT32 miplevel, SINT32 x, SINT32 y, piRECT* wrect, FLOAT32 z, piRECT* rect, UINT32 Color);
LOGICAL draw2dByUV(FLOAT32 x,
					FLOAT32 y,
					FLOAT32 z,
					piUV uvCoords[2],
					SINT32 iWidth,
					SINT32 iHeight,
					UINT32 uColor);
