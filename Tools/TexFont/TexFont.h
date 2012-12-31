// texfont.h
//

#define TEXTWH      256

#define CELLWIDTH	16
#define CELLHEIGHT  25

#define NUMCHARS	160	// Number of characters on the bmp
#define STARTCHAR	32	// The character that we start with 
						// (usually 32, because > 32 is unprintable)

typedef struct
{
  int   x, y, width;
  float u1, u2, v1, v2;

} CHARLAYOUT;

typedef struct
{
  int   AvgWidth;
  int   Height;
  CHARLAYOUT  CharLayout[NUMCHARS];

} FONTLAYOUT;
