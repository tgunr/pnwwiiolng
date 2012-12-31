#pragma once

extern  LOGICAL ldtexOpenWork(int texid, int miplevel, piSURFACEDESC* lppitext);
extern  LOGICAL ldtexCloseWork(int texid, int miplevel);
extern  LOGICAL ldtexFillSurface(piSURFACEDESC* lpdest, UINT32 FillColor);
extern  LOGICAL ldtexFillTexture(int texid, int miplevel, UINT32 FillColor);
extern  LOGICAL ldtexCopySubsurfaceToSubsurface(int cwidth, int cheight, int srcoff, piSURFACEDESC* lpsrc, int destoff, piSURFACEDESC* lpdest, UINT32 TransparentColor);
extern  LOGICAL ldtexCopyBitmapToSubsurface(piBITMAPINFO* pbmi, int texid, int miplevel);
extern  LOGICAL ldtexCopyColorTextToSubsurface(SINT32, SINT32, SINT32, piSURFACEDESC*,
									     SINT32, piSURFACEDESC*, UINT32, UINT32);

extern  LOGICAL ldtexFillTextureRegion(int texid, int miplevel, UINT32 FillColor, int x1, int y1, int x2, int y2);
extern  LOGICAL ldtexFillSurfaceRegion(piSURFACEDESC* lpdest, UINT32 FillColor, int x1, int y1, int x2, int y2);

extern  LOGICAL ldtexPaintTexturePixels(int texid, int miplevel, int count, UINT32 FillColor[], int x[], int y[]);
extern  LOGICAL ldtexPaintSurfacePixels(piSURFACEDESC* lpdest, int count, UINT32 FillColor[], int x[], int y[]);

extern  LOGICAL ldtexCopyTex1toTex2Rotated(SINT32 w,
										SINT32 h,
										SINT32 srcOffset,
										piSURFACEDESC* src,
										SINT32 destOffset,
										piSURFACEDESC* dest,
										UINT32 transColor,
										LOGICAL	flipHoriz);

