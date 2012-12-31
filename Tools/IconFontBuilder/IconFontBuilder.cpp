#include <fstream>
#include <string>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

#include "wideCharList.h"
/////////////////////////////////////
// This file (wideCharList.h) has to be synchronous with the definition in chatHandle.cpp
// I didn't move the structure into a header for the WW2 project because of it's size
/////////////////////////////////////

#pragma pack(4)

typedef struct
{
  int   x, y, width;
  float u1, u2, v1, v2;
}CHARLAYOUT;

typedef struct
{
  int   AvgWidth;
  int   Height;
  CHARLAYOUT  CharLayout[WIDE_CHARACTERS];
}FONTLAYOUT;

FONTLAYOUT  fontLayout;

void main()
{
	const int			imageSize1D			= 2048;
	const int			charBitmapWidth		= 24;
	const int			charBitmapHeight	= 24;
	const int			maxCharacterCntX	= imageSize1D / charBitmapWidth;	
	const int			maxCharacterCntY	= imageSize1D / charBitmapHeight;	

	FT_Library			fontLibrary;
	FT_Face				fontFace;
	FT_Glyph			fontGlyph;
	FT_BitmapGlyph		bitmapGlyph;

	static unsigned char	image[imageSize1D][imageSize1D] = {0};
	int						maxFontDescender;


	if(FT_Init_FreeType(&fontLibrary))	exit(-1);
	if(FT_New_Face(fontLibrary, "arial.ttf", 0, &fontFace))	exit(-1);
	FT_Set_Pixel_Sizes(fontFace, charBitmapWidth, charBitmapHeight);

	maxFontDescender = abs(fontFace->size->metrics.descender) / 64;

	for(int i=0; i < 63; i++)
	{
		if(i == 0)
		{// The no-break space character is the default for all non-mapped ones
			if(FT_Load_Glyph(fontFace, FT_Get_Char_Index(fontFace, L'\u00a0'), FT_LOAD_DEFAULT))		exit(-1);
		}
		else if(FT_Load_Glyph(fontFace, FT_Get_Char_Index(fontFace, s_wideCharacters[i]), FT_LOAD_DEFAULT))		exit(-1);

		FT_Get_Glyph(fontFace->glyph, &fontGlyph);
		if(FT_Glyph_To_Bitmap(&fontGlyph, ft_render_mode_normal, 0, 1))		exit(-1);
	  
		bitmapGlyph = (FT_BitmapGlyph)fontGlyph;
		FT_Bitmap& bitmap = bitmapGlyph->bitmap;

		int	currentPosX			= i % (imageSize1D / charBitmapWidth) * charBitmapWidth;
		int	currentPosY			= i / (imageSize1D / charBitmapWidth) * charBitmapHeight + charBitmapHeight;
		int glyphDescender		= (fontFace->glyph->metrics.height - fontFace->glyph->metrics.horiBearingY) / 64;
		int glyphBearing		= fontFace->glyph->metrics.horiBearingX / 64;
		int currentPosXAligned	= currentPosX + glyphBearing;
		int currentPosYAligned	= imageSize1D - currentPosY + maxFontDescender - glyphDescender + bitmap.rows;

		fontLayout.CharLayout[i].x		= currentPosX;
		fontLayout.CharLayout[i].y		= currentPosY;
		fontLayout.CharLayout[i].width	= fontFace->glyph->metrics.horiAdvance / 64;
		fontLayout.CharLayout[i].u1		= (float)currentPosX / imageSize1D;
		fontLayout.CharLayout[i].u2		= (float)(currentPosX + fontLayout.CharLayout[i].width) / imageSize1D;
		fontLayout.CharLayout[i].v1		= (float)(imageSize1D - currentPosY) / (imageSize1D - 1);
		fontLayout.CharLayout[i].v2		= (float)(imageSize1D - (currentPosY - charBitmapHeight)) / imageSize1D;


		for(int crtLine=0; crtLine < bitmap.rows; crtLine++)
		{
			memcpy(&image[currentPosYAligned - crtLine - 1][currentPosXAligned], bitmap.buffer + (bitmap.pitch * crtLine), bitmap.pitch);
		}
	}

	// Yes, this copy&paste spaghetti code - who cares here?!
	if(FT_Init_FreeType(&fontLibrary))	exit(-1);
	if(FT_New_Face(fontLibrary, "simhei.ttf", 0, &fontFace))	exit(-1);
	FT_Set_Pixel_Sizes(fontFace, charBitmapWidth, charBitmapHeight);

	maxFontDescender = abs(fontFace->size->metrics.descender) / 64;

	for(int i=63; i < WIDE_CHARACTERS; i++)
	{
		if(FT_Load_Glyph(fontFace, FT_Get_Char_Index(fontFace, s_wideCharacters[i]), FT_LOAD_DEFAULT))		exit(-1);

		FT_Get_Glyph(fontFace->glyph, &fontGlyph);
		if(FT_Glyph_To_Bitmap(&fontGlyph, ft_render_mode_normal, 0, 1))		exit(-1);
	  
		bitmapGlyph = (FT_BitmapGlyph)fontGlyph;
		FT_Bitmap& bitmap = bitmapGlyph->bitmap;

		int	currentPosX			= i % (imageSize1D / charBitmapWidth) * charBitmapWidth;
		int	currentPosY			= i / (imageSize1D / charBitmapWidth) * charBitmapHeight + charBitmapHeight;
		int glyphDescender		= (fontFace->glyph->metrics.height - fontFace->glyph->metrics.horiBearingY) / 64;
		int glyphBearing		= fontFace->glyph->metrics.horiBearingX / 64;
		int currentPosXAligned	= currentPosX + glyphBearing;
		int currentPosYAligned	= imageSize1D - currentPosY + maxFontDescender - glyphDescender + bitmap.rows;

		fontLayout.CharLayout[i].x		= currentPosX;
		fontLayout.CharLayout[i].y		= currentPosY;
		fontLayout.CharLayout[i].width	= fontFace->glyph->metrics.horiAdvance / 64;
		fontLayout.CharLayout[i].u1		= (float)currentPosX / imageSize1D;
		fontLayout.CharLayout[i].u2		= (float)(currentPosX + fontLayout.CharLayout[i].width) / imageSize1D;
		fontLayout.CharLayout[i].v1		= (float)(imageSize1D - currentPosY) / (imageSize1D - 1);
		fontLayout.CharLayout[i].v2		= (float)(imageSize1D - (currentPosY - charBitmapHeight)) / imageSize1D;


		for(int crtLine=0; crtLine < bitmap.rows; crtLine++)
		{
			memcpy(&image[currentPosYAligned - crtLine - 1][currentPosXAligned], bitmap.buffer + (bitmap.pitch * crtLine), bitmap.pitch);
		}
	}

	maxFontDescender = abs(fontFace->size->metrics.descender) / 64;

	fontLayout.AvgWidth				= charBitmapWidth;
	fontLayout.Height				= charBitmapHeight;

	FILE* datFile = fopen("test.dat", "wb+");
    fwrite(&fontLayout, sizeof(FONTLAYOUT), 1, datFile);
    fclose(datFile);
	
#pragma pack(1)
	struct
	{
		unsigned short int	type;
		unsigned int		size;
		unsigned short int	reserved1, reserved2;
		unsigned int		offset;
	}fileHeader;
#pragma pack(4)

	struct
	{
	   unsigned int			size;
	   int					width,height;
	   unsigned short int	planes;
	   unsigned short int	bits;
	   unsigned int			compression;
	   unsigned int			imagesize;
	   int					xresolution,yresolution;
	   unsigned int			ncolours;
	   unsigned int			importantcolours;
	}infoHeader;

	infoHeader.size				= sizeof(infoHeader);
	infoHeader.width			= imageSize1D;
	infoHeader.height			= imageSize1D;
	infoHeader.planes			= 1;
	infoHeader.bits				= 8;
	infoHeader.compression		= 0;
	infoHeader.imagesize		= infoHeader.width * infoHeader.height * (infoHeader.bits/8);
	infoHeader.xresolution		= 0;
	infoHeader.yresolution		= 0;
	infoHeader.ncolours			= 256;
	infoHeader.importantcolours	= 256;

	struct
	{
		char	red;
		char	green;
		char	blue;
		char	alpha;
	}palette[256];

	for(int i=0; i<256; i++)
	{
		// Generate the grayscale pallete
		palette[i].red = i; palette[i].green = i; palette[i].blue = i; palette[i].alpha = i;
	}

	fileHeader.type			= 'MB';
	fileHeader.size			= sizeof(fileHeader) + sizeof(infoHeader) + sizeof(palette) + infoHeader.imagesize;
	fileHeader.reserved1	= 0;
	fileHeader.reserved2	= 0;
	fileHeader.offset		= sizeof(fileHeader) + sizeof(infoHeader) + sizeof(palette);

	std::string	fileName = "test.bmp";
	FILE*		fp = fopen(fileName.c_str(), "wb");;

	fwrite(&fileHeader, sizeof(fileHeader), 1, fp);
	fwrite(&infoHeader, sizeof(infoHeader), 1, fp);
	fwrite(&palette, sizeof(palette), 1, fp);
	fwrite(&image, infoHeader.imagesize, 1, fp);

	fclose(fp);
}