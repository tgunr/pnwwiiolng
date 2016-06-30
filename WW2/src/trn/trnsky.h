#pragma once

#define USE_LENS_FLARE
//#undef USE_LENS_FLARE

#define RAIN_CLOUD_HEIGHT       800.0f // Approx height of vertices in the .FLT file
#define INTEROPLATE_DIST        500.0f // WAG
#define START_SUNSHINE_HEIGHT   RAIN_CLOUD_HEIGHT + INTEROPLATE_DIST
#define MIN_FOG_SCALE           1.0f//0.5f
#define MAX_FOG_SCALE           1.0f

// Skybox values
enum
{
    SKY_NORTH,
    SKY_EAST,
    SKY_SOUTH,
    SKY_WEST,
    SKY_TOP
};

enum ENUM_TERRAIN_COLOR
{
    IDE_SKY_COLOR_RED,
    IDE_SKY_COLOR_GREEN,
    IDE_SKY_COLOR_BLUE,
    IDE_SKY_COLOR_NIGHT_RED,
    IDE_SKY_COLOR_NIGHT_GREEN,
    IDE_SKY_COLOR_NIGHT_BLUE,
    IDE_SKY_CLEAR_AMBIENT_RED,
    IDE_SKY_CLEAR_AMBIENT_GREEN,
    IDE_SKY_CLEAR_AMBIENT_BLUE,
    IDE_SKY_NIGHT_AMBIENT_RED,
    IDE_SKY_NIGHT_AMBIENT_GREEN,
    IDE_SKY_NIGHT_AMBIENT_BLUE,
    IDE_SUN_NOON_RED_COLOR,
    IDE_SUN_NOON_GREEN_COLOR,
    IDE_SUN_NOON_BLUE_COLOR,
    IDE_SUN_ITSELF_NOON_RED_COLOR,
    IDE_SUN_ITSELF_NOON_GREEN_COLOR,
    IDE_SUN_ITSELF_NOON_BLUE_COLOR,
    IDE_SUN_NIGHT_RED_COLOR,
    IDE_SUN_NIGHT_GREEN_COLOR,
    IDE_SUN_NIGHT_BLUE_COLOR,
    IDE_SUNSET_RED_COLOR,
    IDE_SUNSET_GREEN_COLOR,
    IDE_SUNSET_BLUE_COLOR,
    IDE_HORIZ_COLOR_RED,
    IDE_HORIZ_COLOR_GREEN,
    IDE_HORIZ_COLOR_BLUE,
    IDE_MOON_RED_COLOR,
    IDE_MOON_GREEN_COLOR,
    IDE_MOON_BLUE_COLOR,
    IDE_MOON_DAY_RED_COLOR,
    IDE_MOON_DAY_GREEN_COLOR,
    IDE_MOON_DAY_BLUE_COLOR,

    MAX_TERRAIN_COLORS

};

	extern LOGICAL trnskyInit(const SINT8* name, SINT32 mesh, FLOAT32 scale);
	extern LOGICAL trnskyUpdate(void);
	extern LOGICAL trnskyDrawDeprecated(void);
    extern void trnskyDraw();
	extern void trnskyKill(void);
	extern LIGHT* trnskyGetSunSource(void);
	extern LIGHT* trnskyGetMoonSource(void);

    extern FLOAT32 trnskyGetColor(SINT32 i);
    extern FLOAT32 trnskyGetAltitude();

    extern SINT32 trnskyGetSetting();
    extern SINT32 trnskyGetFogOption();

    extern void trnskyGetFogDensity(FLOAT32& start, FLOAT32& end);
    extern void trnskySetFogStartDensity(FLOAT32 start);
    extern void trnskySetFogEndDensity(FLOAT32 end);

    extern void* trnskyGetSunObject();

    extern void trnskyChangeSetting(SINT32 setting, SINT32 fogoption, bool Override);
    extern void trnskyResetFade();
    extern void trnskyInitSkyBox();

#ifdef USE_LENS_FLARE
    extern void trnskyDrawFlare();
    extern void trnskyPreDrawFlare();
    extern void trnskyReadObstruction();
    extern void trnskyAdjustBrightness();
    extern void trnskyReadControlPixels();
    extern void trnskyDrawAfterFlare();

    extern FLOAT32 trnskyGetAfterBright();
#endif
