#pragma once

  #pragma pack(1)
  // PCX header definition
  struct PCXHEADER
  {
	  char	manu;					//	0	Manufacturer	1	Constant Flag  10 = ZSoft .PCX
	  char	version;			//	1	Version 	1	Version information:
											  //	0 = Version 2.5
											  //	2 = Version 2.8 w/palette information
											  //	3 = Version 2.8 w/o palette information
											  //	5 = Version 3.0
	  char	encoding; 		//	2	Encoding	1	1 = .PCX run length encoding
	  char	bpp;					//	3	Bits per pixel	1	Number of bits/pixel per plane
	  SINT16	xmin, ymin;		//	4	Window  	8	Picture Dimensions
	  SINT16	xmax, ymax;		//			(Xmin, Ymin) - (Xmax - Ymax)
											  //			in pixels, inclusive
	  SINT16	hres;					//	12	HRes    	2	Horizontal Resolution of creating device
	  SINT16	vres;					//	14	VRes    	2	Vertical Resolution of creating device
	  UCHAR	cmap[48];			//	16	Colormap	48	Color palette setting, see text
	  UCHAR	reserved;			//	64	Reserved	1
	  UCHAR	nplanes;			//	65	NPlanes	        1	Number of color planes
	  SINT16	bpl;					//	66	Bytes per Line	2	Number of bytes per scan line per
											  //			color plane (always even for .PCX files)
	  SINT16	pinfo;				//	68	Palette Info	2	How to interpret palette - 1 = color/BW,
											  //			2 = grayscale
	  UCHAR	filler[58];		//	70	Filler  	58	blank to fill out 128 byte header

  };

  #pragma pack()

extern int ldpcxLoadPCXFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 WantFlags);
