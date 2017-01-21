#include <syscfg.h> // Path is project specific.
#include "cfg/cfgData.h"

#ifndef _SETTINGS_EXE_
#endif

// This file used in settings.exe too
#if platBuild != BUILD_SERVER
#include "pi/pi.h"
#else
#define piRGBA_MAKE(r, g, b, a) ((unsigned int) (((a) << 24) | ((r) << 0) | ((g) << 8) | ((b) << 16)))
#endif

/*
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA
*/

// see CAboutSheet::OnVisualPerformance
cfgPARAM_SETUP cfgParameters[cfgNUM_TOTAL_PARAMS] =
{
	{"shademode",			cfgTYPE_TEXTURE_SHADE_MODE,		cfgSHADE_MODE_GOURAUD				},
	{"bdithering",			cfgTYPE_TRUE_FALSE,				cfgVALUE_TRUE						},
	{"bspecular",			cfgTYPE_TRUE_FALSE,				cfgVALUE_TRUE						},
	{"sound",				cfgTYPE_STRING,					cfgDEFAULT_SOUND_CARD				},
	{"bsoundenable",		cfgTYPE_TRUE_FALSE,				cfgVALUE_TRUE						},
	{"b3dsoundenable",		cfgTYPE_TRUE_FALSE,				cfgVALUE_TRUE						},
	{"video",				cfgTYPE_STRING,					cfgDEFAULT_VIDEO_CARD				},
	{"resolution",			cfgTYPE_STRING,					cfgDEFAULT_VIDEO_CARD_RESOLUTION	},
	{"bnoframerunahead",	cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"enablevsync",	        cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"gammared",			cfgTYPE_FLOAT,					cfgDEFAULT_COLOR_GAMMA_RED			},
	{"gammagreen",			cfgTYPE_FLOAT,					cfgDEFAULT_COLOR_GAMMA_GREEN		},
	{"gammablue",			cfgTYPE_FLOAT,					cfgDEFAULT_COLOR_GAMMA_BLUE			},
	{"bshowinfo",			cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"shadowsizeselect",	cfgTYPE_SHADOW_SIZE_SELECT,		cfgSPECIAL_FX_SHADOW_SIZE_SELECT_0	},
	{"volumemultiplier",	cfgTYPE_FLOAT,					cfgDEFAULT_VOLUME_MULTIPLIER		},
	{"musicmultiplier",		cfgTYPE_FLOAT,					cfgDEFAULT_MUSIC_MULTIPLIER			},
	{"hudvisible",			cfgTYPE_INTEGER,				cfgDEFAULT_HUD_VISIBLE				},
	{"fogscaleselector",	cfgTYPE_INTEGER,				cfgDEFAULT_FOG_SCALE_SELECTOR		},
	{"maxnumbersounds",		cfgTYPE_INTEGER,				cfgDEFAULT_MAX_NUMBER_SOUNDS		},
	{"suppressmuzzlesmoke",	cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"suppressmuzzlelight", cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                      },
	{"suppresstracersmoke",	cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"lodfieldofview",		cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"lodlimitperframe",	cfgTYPE_TRUE_FALSE,				cfgVALUE_FALSE						},
	{"bcursorenabled",      cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                      },
	{"friendlycolor",		cfgTYPE_INTEGER,				cfgDEFAULT_FRIENDLY_COLOR           },
	{"enemycolor",			cfgTYPE_INTEGER,				cfgDEFAULT_ENEMY_COLOR              },
	{"squadcolor",			cfgTYPE_INTEGER,				cfgDEFAULT_SQUAD_COLOR              },
	{"missioncolor",        cfgTYPE_INTEGER,                cfgDEFAULT_MISSION_COLOR            },
	{"missionleadercolor",  cfgTYPE_INTEGER,                cfgDEFAULT_MISSIONLEADER_COLOR      },
	{"language",			cfgTYPE_INTEGER,				cfgDEFAULT_LANGUAGE_CHOICE	        },
	{"ambientlevel",        cfgTYPE_INTEGER,                cfgDEFAULT_AMBIENT_LEVEL            },
	{"radialclutter",       cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE                       },
	{"radialdensity",       cfgTYPE_FLOAT,                  cfgDEFAULT_RADIAL_CLUTTER_DENSITY   },
	{"radialradius",        cfgTYPE_FLOAT,                  cfgDEFAULT_RADIAL_CLUTTER_RADIUS    },
	{"visplayerlimit",      cfgTYPE_INTEGER,                cfgDEFAULT_VIS_PLAYER_LIMIT         },
	{"usenetcode",				cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE						},
	{"fsaasamples",      	cfgTYPE_FSAA_SAMPLES,           cfgDEFAULT_FSSA_SAMPLES       	    },
	{"fsaahighquality",     cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE      	            },
	{"soundhighquality",    cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE       	            },
	{"soundforcestereo",    cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE       	            },
	{"soundconvertonload",  cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE       	            },
	{"capturealldisplays",  cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE       	            },
	{"enableh2oshaders",    cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                      },
 #if platOS == OS_MACOS
	{"postrenderfilter",    cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                       },
 #else
	{"postrenderfilter",    cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE                       },
#endif
	{"grassshader",         cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE                       },
	{"normalmaps",          cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE                       },
	{"waterreflections",    cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                      },
	{"reflectionupdate",    cfgTYPE_INTEGER,                cfgDEFAULT_UPDATE_RATE              },
	{"wantmessagealert",    cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE                       },
	{"usenetworkroute",     cfgTYPE_INTEGER,                cfgDEFAULT_NETWORK_ROUTE            },
	{"wantsse2",            cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                      },
	{"shadowmaps",          cfgTYPE_TRUE_FALSE,             cfgVALUE_FALSE                      },
	{"wantbloodeffects",    cfgTYPE_TRUE_FALSE,             cfgVALUE_TRUE                       }
};

const char* cfgTextureShadeModeValueStrings[cfgNUM_TEXTURE_SHADE_MODE_VALUES] =
{
	"flat",
	"gouraud",
	"phong",
};

const char* cfgTextureMagFilterValueStrings[cfgNUM_TEXTURE_MAG_FILTER_VALUES] =
{
	"none",
	"point",
	"linear"
};

const char* cfgTextureMinFilterValueStrings[cfgNUM_TEXTURE_MIN_FILTER_VALUES] =
{
	"none",
	"point",
	"linear"
};

const char* cfgTextureMipFilterValueStrings[cfgNUM_TEXTURE_MIP_FILTER_VALUES] =
{
	"none",
	"point",
	"linear"
};

const char* cfgTextureAntialiasingValueStrings[cfgNUM_TEXTURE_ANTIALIASING_VALUES] =
{
	"none",
	"sortdependant",
	"sortindependant"
};

const char* cfgTrueFalseValueStrings[cfgNUM_TRUE_FALSE_VALUES] =
{
	"false",
	"true"
};

int cfgShadowSizeSelectValues[cfgNUM_SPECIAL_FX_SHADOW_SIZE_SELECT_VALUES] =
{
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8
};

int cfgFSAASamplesValues[cfgNUM_FSAA_SAMPLE_VALUES] =
{
	0, // Off
	2,
	4,
	6
};

void* Values[cfgNUM_TYPE_VALUES] =
{
	cfgDefaultIntegers, // INTEGER
	cfgDefaultFloats, // FLOAT
	cfgDefaultStrings,
	cfgTextureShadeModeValueStrings,
	cfgTextureMagFilterValueStrings,
	cfgTextureMinFilterValueStrings,
	cfgTextureMipFilterValueStrings,
	cfgTextureAntialiasingValueStrings,
	cfgShadowSizeSelectValues,
	cfgFSAASamplesValues,
	cfgTrueFalseValueStrings
};

int NumValues[cfgNUM_TYPE_VALUES] =
{
	cfgNUM_DEFAULT_INTEGERS,
	cfgNUM_DEFAULT_FLOATS,
	cfgNUM_DEFAULT_STRINGS,
	cfgNUM_TEXTURE_SHADE_MODE_VALUES,
	cfgNUM_TEXTURE_MAG_FILTER_VALUES,
	cfgNUM_TEXTURE_MIN_FILTER_VALUES,
	cfgNUM_TEXTURE_MIP_FILTER_VALUES,
	cfgNUM_TEXTURE_ANTIALIASING_VALUES,
	cfgNUM_SPECIAL_FX_SHADOW_SIZE_SELECT_VALUES,
	cfgNUM_FSAA_SAMPLE_VALUES,
	cfgNUM_TRUE_FALSE_VALUES
};

const char* cfgDefaultStrings[cfgNUM_DEFAULT_STRINGS] =
{
	"Primary Sound Driver",	// DEFAULT_SOUND_CARD
	"Primary Display Driver",	// DEFAULT_VIDEO_CARD
	"800x600x32"				// DEFAULT_VIDEO_MODE
};

int cfgDefaultIntegers[cfgNUM_DEFAULT_INTEGERS] =
{
	2,		// DEFAULT_HUD_VISIBLE
	0,		// DEFAULT_FOG_SCALE_SELECTOR
	16,		// DEFAULT_MAX_NUMBER_SOUNDS
	piRGBA_MAKE(0,0,0xff,0),	// DEFAULT_FRIENDLY_COLOR
	piRGBA_MAKE(0xff,0,0,0),	// DEFAULT_ENEMY_COLOR
	piRGBA_MAKE(0xff,0xff,0xff,0),	// DEFAULT_SQUAD_COLOR
	piRGBA_MAKE(0, 0, 128, 0), // DEFAULT_MISSION_COLOR
	piRGBA_MAKE(80, 240, 120, 0), // DEFAULT_MISSIONLEADER_COLOR
	0,			// DEFAULT_LANGUAGE_CHOICE
	0,          // DEFAULT_AMBIENT_LEVEL
	1,          // cfgDEFAULT_VIS_PLAYER_LIMIT
	2,			// cfgDEFAULT_FSSA_SAMPLES
	5,          // cfgDEFAULT_UPDATE_RATE
	0           // cfgDEFAULT_NETWORK_ROUTE
};

float cfgDefaultFloats[cfgNUM_DEFAULT_FLOATS] =
{
	1.0f,		// DEFAULT_COLOR_GAMMA_RED
	1.0f,		// DEFAULT_COLOR_GAMMA_GREEN
	1.0f,		// DEFAULT_COLOR_GAMMA_BLUE
	100.0f,		// DEFAULT_VOLUME_MULTIPLIER
	1.0f,		// DEFAULT_MUSIC_MULTIPLIER
	0.5f,       // DEFAULT_RADIAL_CLUTTER_DENSITY
	0.5f        // DEFAULT_RADIAL_CLUTTER_RADIUS
};

void* Defaults[cfgNUM_TYPE_VALUES] =
{
	cfgDefaultIntegers,
	cfgDefaultFloats,
	cfgDefaultStrings,
	cfgTextureShadeModeValueStrings,
	cfgTextureMagFilterValueStrings,
	cfgTextureMinFilterValueStrings,
	cfgTextureMipFilterValueStrings,
	cfgTextureAntialiasingValueStrings,
	cfgShadowSizeSelectValues,
	cfgFSAASamplesValues,
	cfgTrueFalseValueStrings
};
