#include <syscfg.h> // Path is project specific.
//
// Date: Tue Jul 12 15:10:07 CDT 2005
//
// First Persona data initialization
//
// Copyright (C) PlayNet, Inc 1999-2014 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Patrick "thunder" Magruder
//

#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "fio/fio.h"
#include "ld/ldpix.h"
#include "str/strpool.h"

#include "fp/fpHash.h"
#include "fp/fpTrooperHashes.h"

#include "xml/XMLLoader2.h"

#define	FPS_DIR		"FPS\\"

#define FPS_DATA_DIR		"vehicles\\ground\\groundman\\"
#define FPS_WEAP_DATA_DIR	FPS_DATA_DIR "weapons\\"
#define	FPS_DATA_FILE		"fpsData.txt"
#define	FPS_VISLOADOUT_FILE	"vehicles/ground/groundman/visloadouts.xml"

// this is atan (1/1000) .. or 1 meter granularity at 1000m
#define	MOUSE_GRANULARITY				0.05729576041450060899707537401941
//#define	TROOPER_DEFAULT_FOV				75

//#define CACHE_FPS_DATA

using fpFPSLoadoutMap = std::map<std::string, fpFPSTrooperLoadout>;

const SCHAR*						arms_anim_files[NUM_WEAPONS][NUM_WEAP_STATES];
const SCHAR*						weap_anim_files[NUM_WEAPONS][NUM_WEAP_STATES];
const SCHAR*						clip_anim_files[NUM_WEAPONS][NUM_WEAP_STATES];
const SCHAR*						weap_default[NUM_WEAPONS];
const SCHAR*						weap_empty_default[NUM_WEAPONS];
const SCHAR*						clip_default[NUM_WEAPONS];

fpFPSWeaponData				fpsWeapons[NUM_WEAPONS];
// moved it a bit for hacker irritation
// fpFPSTrooperData			fpsTrooper;

const SCHAR* skel_model =			FPS_DIR "fullskeleton";

const SCHAR* arm_models[NUM_INFANTRY_TYPES] =
{
	FPS_DIR "arms\\armsmesh-britinf",
	FPS_DIR "arms\\armsmesh-germinf",
	FPS_DIR "arms\\armsmesh-frenchinf",
	FPS_DIR "arms\\armsmesh-britinf",
	FPS_DIR "arms\\armsmesh-germinf",
	FPS_DIR "arms\\armsmesh-frenchinf",
	FPS_DIR "arms\\brit_para_armmesh",
	FPS_DIR "arms\\germ_para_armmesh",
	FPS_DIR "arms\\french_para_armmesh",
	FPS_DIR "arms\\brit_para_armmesh",
	FPS_DIR "arms\\germ_para_armmesh",
	FPS_DIR "arms\\french_para_armmesh",
	FPS_DIR "arms\\brit_para_armmesh",
	FPS_DIR "arms\\germ_para_armmesh",
	FPS_DIR "arms\\french_para_armmesh",
	FPS_DIR "arms\\armsmesh-britinf",
	FPS_DIR "arms\\armsmesh-germinf",
	FPS_DIR "arms\\armsmesh-frenchinf",
	FPS_DIR "arms\\armsmesh-britinf" // americans
};

const SCHAR* replacement_arms_file = FPS_DIR "arms\\replacement_arms_maps";
const SCHAR* default_arms_tex_names[NUM_INFANTRY_TYPES];
const SCHAR* replacement_arms_tex_names[NUM_INFANTRY_TYPES];

const SCHAR* leg_models[NUM_INFANTRY_TYPES] =
{
	FPS_DIR "legs\\brit-legs",
	FPS_DIR "legs\\german-legs",
	FPS_DIR "legs\\french-legs",
	FPS_DIR "legs\\brit-legs",
	FPS_DIR "legs\\german-legs",
	FPS_DIR "legs\\french-legs",
	FPS_DIR "legs\\brit-legs",
	FPS_DIR "legs\\german-legs",
	FPS_DIR "legs\\french-legs",
	FPS_DIR "legs\\brit-legs",
	FPS_DIR "legs\\german-legs",
	FPS_DIR "legs\\french-legs",
	FPS_DIR "legs\\brit-legs",
	FPS_DIR "legs\\german-legs",
	FPS_DIR "legs\\french-legs",
	FPS_DIR "legs\\brit-legs",
	FPS_DIR "legs\\german-legs",
	FPS_DIR "legs\\french-legs",
	FPS_DIR "legs\\brit-legs" // americans
};

const SCHAR* hud_icons[NUM_ICONS] =
{
	"hudsmg1.bmp",
	"hudsatchel1.bmp",
	"hudrifle1.bmp",
	"hudpistol1.bmp",
	"hudgrenade1.bmp",
	"hudsmokegrenade1.bmp",
	"hudbinocs1.bmp",
	"hudknife1.bmp",
	"hudbren1.bmp",
	"hudmg341.bmp",
	"hudchat1.bmp",
	"hudresupply1.bmp",
	"hudriflegrenade1.bmp",
	"hudatr1.bmp",
	"hudltmortar1.bmp"
};

const SCHAR* fpFPSWeaponKeyStrings[NUM_WEAPONS] =
{
	"none",
	"thompson",
	"mp40",
	"mas38",
	"satchel",
	"k98k",
	"enfield",
	"lebel",
	"m1935",
	"luger",
	"mills",
	"mills",
	"stigr",
	"enfmk2",
	"binocs",
	"knife",
	"knife",
	"bren",
	"mg34",
	"fm39",
	"smoke",
	"smoke",
	"smoke",
	"ammocan",
	"mas36",
	"mas40",
	"rg_g30",
	"rg_vb",
	"rg_mills",
	"boyes",
	"pzb",
	"repair",
	"map",
	"enfield",
	"lebel", // french sniper
	"k98k",
	"satchel", //DEG_CHANGE
	"blmortar",
	"flmortar",
	"glmortar",
	"buildtool",
	"garand",
	"bar",
	"colt",
	"springfield_sniper",
	"gewehr",
	"g41",
	"m1thompson",
	"mills",
	"bazooka",
	"pzschreck",
	"m1903a3",
	"smoke",
	"piat",
	"Sten",
	"GreaseGun",
	"mp34",
	"fg42g",
};

const SCHAR* fpFPSWeaponTypeStrings[FPS_WEAPONTYPE_NUM_TYPES] =
{
	"SMG",
	"SATCHEL",
	"RIFLE",
	"AUTO_PISTOL",
	"GRENADE",
	"REVOLVER",
	"ITEM",
	"KNIFE",
	"LMG",
	"RIFLE_GRENADE",
	"ATR",
	"LTMORTAR",
	"BUILD",
	"AUTO_RIFLE",
	"ROCKET"
};

const SCHAR* armTypeStrings[NUM_INFANTRY_TYPES]			=
{
	"ukarmy",
	"dearmy",
	"frarmy",
	"ukhc",
	"dehc",
	"frhc",
	"ukpara",
	"depara",
	"frpara",
	"ukparanopack",
	"deparanopack",
	"frparanopack",
	"ukparaself",
	"deparaself",
	"frparaself",
	"ukml",
	"deml",
	"frml",
	"usarmy"
};

const SCHAR* weapNameStrings[NUM_WEAPONS] =
{
	(SCHAR *)"none",
	(SCHAR *)"thompson",
	(SCHAR *)"mp40",
	(SCHAR *)"mas",
	(SCHAR *)"alliedsatchel",
	(SCHAR *)"kar98k",
	(SCHAR *)"enfield",
	(SCHAR *)"lebel",
	(SCHAR *)"m1935a",
	(SCHAR *)"luger",
	(SCHAR *)"mills36mk1",
	(SCHAR *)"frbang",
	(SCHAR *)"stigr",
	(SCHAR *)"webley",
	(SCHAR *)"binocs",
	(SCHAR *)"axisknife",
	(SCHAR *)"alliedknife",
	(SCHAR *)"bren",
	(SCHAR *)"mg34",
	(SCHAR *)"fm29",
	(SCHAR *)"hgr39",
	(SCHAR *)"m18",
	(SCHAR *)"frsmoke",
	(SCHAR *)"ammo",
	(SCHAR *)"mas36",
	(SCHAR *)"mas40",
	(SCHAR *)"rgg30",
	(SCHAR *)"rgvb",
	(SCHAR *)"rgmills",
	(SCHAR *)"boysmk1",
	(SCHAR *)"pzb39",
	(SCHAR *)"bridgerepair",
	(SCHAR *)"map",
	(SCHAR *)"enfieldmk4",
	(SCHAR *)"lebelmle",
	(SCHAR *)"kar98ks",
	(SCHAR *)"axissatchel",
	(SCHAR *)"ukltmortar",
	(SCHAR *)"frltmortar",
	(SCHAR *)"deltmortar",
	(SCHAR *)"buildtool",
	(SCHAR *)"m1garand",
	(SCHAR *)"browningbar",
	(SCHAR *)"colt1911",
	(SCHAR *)"springfld1903",
	(SCHAR *)"gew43",
	(SCHAR *)"gew41",
	(SCHAR *)"m1thompson",
	(SCHAR *)"mark2",
	(SCHAR *)"bazooka",
	(SCHAR *)"pzschreck",
	(SCHAR *)"m1903a3",
	(SCHAR *)"ussmoke",
	(SCHAR *)"piat",
	(SCHAR *)"stenmk2",
	(SCHAR *)"m3a1",
	(SCHAR *)"mp34",
	(SCHAR *)"fg42g",
};

class fpFPSVisLoadoutsLoader : public XMLLoader2<fpFPSVisLoadoutsLoader>
{
public:

	fpFPSVisLoadoutsLoader(const char* path);
	virtual ~fpFPSVisLoadoutsLoader(){}

	// tag handlers
	XML_TAG_HANDLER(DoVisLoadout);
	XML_TAG_HANDLER(DoWeapon);

private:

	int _getArmsID(const char* name);
	int _getWeaponID(const char* name);
};

static fpFPSLoadoutMap _loadouts;

static void _fpFPSDataHandleWeaponTypeString (fpsWeaponTypes weapon, char* typestring);
static void _fpFPSReadVisualLoadouts();

static void _fpFPSDataReadWeaponFile (fpsWeaponTypes weapon, char* filename)
{
    bool hasWarned = false;
	std::string weapFileName(FPS_WEAP_DATA_DIR);
	weapFileName += filename;

	const SINT32 weapDataFile = fioOpen(weapFileName.c_str());

	if (weapDataFile == -1)
	{
		// BAD!
		WriteLog (LOG_FPS, LOG_INFO, "Unable to open weapon file %s for %d", filename, weapon);
		return;
	}

	static const size_t BUF_SIZE = 255;
	static char	buf[BUF_SIZE + 1];

	while (fioGetString(weapDataFile, buf, BUF_SIZE) != 0)
	{
		if (buf[0] == '#') continue;

		char* varName = nullptr;
		char* varData = nullptr;
		fpFPSDataGetVarnameAndContents(&varName, &varData, buf);

		if (varName == nullptr || varData == nullptr) continue;

		if (stricmp(buf, "weaponType") == 0)
			_fpFPSDataHandleWeaponTypeString (weapon, varData);
		else if (stricmp(buf, "muzzleRise") == 0)
			fpsWeapons[weapon].muzzleRise = (FLOAT32) atof (varData);
		else if (stricmp(buf, "muzzleReturn") == 0)
			fpsWeapons[weapon].muzzleReturn = (FLOAT32) atof (varData);
		else if (stricmp(buf, "muzzleClimbRate") == 0)
			fpsWeapons[weapon].muzzleClimbRate = (FLOAT32) atof (varData);
		else if (stricmp(buf, "fireDelay") == 0)
			fpsWeapons[weapon].fireDelay = (FLOAT32) atof (varData);
		else if (stricmp(buf, "minimumDispersion") == 0)
			fpsWeapons[weapon].minimumDispersion = (FLOAT32) atof (varData);
		else if (stricmp(buf, "unaimedDispersionMod") == 0)
			fpsWeapons[weapon].unaimedDispersionMod = (FLOAT32) atof (varData);
		else if (stricmp(buf, "controlability") == 0)
			fpsWeapons[weapon].controlability = (FLOAT32) atof (varData);
		else if (stricmp(buf, "aimFOV") == 0)
			fpsWeapons[weapon].aimFOV = (FLOAT32) atof (varData);
		else if (stricmp(buf, "bulletJitter") == 0)
			fpsWeapons[weapon].bulletJitter = (FLOAT32) atof (varData);
		else if (stricmp(buf, "horizJitterMult") == 0)
			fpsWeapons[weapon].horizJitterMult = (FLOAT32) atof (varData);
		else if (stricmp(buf, "speedMultiplier") == 0)
			fpsWeapons[weapon].speedMultiplier = (FLOAT32) atof (varData);
		else if (stricmp(buf, "aimVerticalCorrection") == 0)
			fpsWeapons[weapon].aimVerticalCorrection = (FLOAT32) atof (varData);
		else if (stricmp(buf, "aimHorizontalCorrection") == 0)
			fpsWeapons[weapon].aimHorizontalCorrection = (FLOAT32) atof (varData);
		else if (stricmp(buf, "convergenceDistance") == 0)
			fpsWeapons[weapon].convergenceDistance = (FLOAT32) atof (varData);
		else if (stricmp(buf, "ejectDelay") == 0)
			fpsWeapons[weapon].ejectDelay = (FLOAT32) atof (varData);
		else if (stricmp(buf, "shellScale") == 0)
			fpsWeapons[weapon].shellScale = (FLOAT32) atof (varData);
		else if (stricmp(buf, "firepointDisplacement") == 0)
			fpsWeapons[weapon].firepointDisplacement = (FLOAT32) atof (varData);

		else if (stricmp(buf, "firepointZDistance") == 0 || stricmp(buf, "fpYOffset") == 0)
			fpsWeapons[weapon].firepointZDistance = (FLOAT32) atof (varData);
		else if (stricmp(buf, "firepointHorizOffset") == 0 || stricmp(buf, "fpxOffset") == 0)
			fpsWeapons[weapon].firepointHorizOffset = (FLOAT32) atof (varData);
		else if (stricmp(buf, "firepointVertOffset") == 0 || stricmp(buf, "fpZOffset") == 0)
			fpsWeapons[weapon].firepointVertOffset = (FLOAT32) atof (varData);

		else if (stricmp(buf, "grenadeVisDelay") == 0)
			fpsWeapons[weapon].grenadeVisDelay= (FLOAT32)atof(varData);

		else if (stricmp(buf, "weaponBreathMultiplier") == 0)
			fpsWeapons[weapon].weaponBreathMultiplier = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weaponHeartMultiplier") == 0)
			fpsWeapons[weapon].weaponHeartMultiplier = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weaponMoveMultiplier") == 0)
			fpsWeapons[weapon].weaponMoveMultiplier = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weaponPoseMultiplier") == 0)
			fpsWeapons[weapon].weaponPoseMultiplier = (FLOAT32) atof (varData);

		else if (stricmp(buf, "auxAltRGCupConnectTime") == 0)
			fpsWeapons[weapon].auxAltConnectTime[FPS_AUX_RGCUP] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "auxAltRGCupDisconnectTime") == 0)
			fpsWeapons[weapon].auxAltDisconnectTime[FPS_AUX_RGCUP] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "auxAltRGGrenadeConnectTime") == 0)
			fpsWeapons[weapon].auxAltConnectTime[FPS_AUX_RGGRENADE] = (FLOAT32) atof (varData);

		else if (stricmp(buf, "weapDeployUpAngleStand") == 0)
			fpsWeapons[weapon].weapDeployUpAngle[FPS_POSITION_STAND] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployDownAngleStand") == 0)
			fpsWeapons[weapon].weapDeployDownAngle[FPS_POSITION_STAND] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployUpAngleCrouch") == 0)
			fpsWeapons[weapon].weapDeployUpAngle[FPS_POSITION_CROUCH] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployDownAngleCrouch") == 0)
			fpsWeapons[weapon].weapDeployDownAngle[FPS_POSITION_CROUCH] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployUpAngleProne") == 0)
			fpsWeapons[weapon].weapDeployUpAngle[FPS_POSITION_PRONE] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployDownAngleProne") == 0)
			fpsWeapons[weapon].weapDeployDownAngle[FPS_POSITION_PRONE] = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployKneelDeployUpAngle") == 0)
			fpsWeapons[weapon].weapDeployKneelDeployUpAngle = (FLOAT32) atof (varData);
		else if (stricmp(buf, "weapDeployKneelDeployDownAngle") == 0)
			fpsWeapons[weapon].weapDeployKneelDeployDownAngle = (FLOAT32) atof (varData);

		else if (stricmp(buf, "staticDeploy") == 0)
			fpsWeapons[weapon].staticDeploy = strtoul(varData, nullptr, 10);
		else if (stricmp(buf, "grenadeDelayTime") == 0)
			fpsWeapons[weapon].grenadeDelayTime = (SINT32)atol(varData);
		else if (stricmp(buf, "holdfire") == 0)
			fpsWeapons[weapon].holdfire = (SINT32) atol (varData);
		else if (stricmp(buf, "sniperGunSight") == 0)
			fpsWeapons[weapon].sniperGunSight = (SCHAR*) strpool (varData);
        else if (stricmp(buf, "sniperScopeDelay") == 0)
			fpsWeapons[weapon].sniperScopeDelay = atof (varData);
		else if (stricmp(buf, "sniperScopeEndDelay") == 0)
			fpsWeapons[weapon].sniperScopeEndDelay = (FLOAT32) atof (varData);
		else if (stricmp(buf, "hudIconType") == 0)
			fpsWeapons[weapon].hudIconType = (fpFPSIconType) atol (varData);
		else if (stricmp(buf, "ejectImmediate") == 0)
			fpsWeapons[weapon].ejectImmediate = strtoul(varData, nullptr, 10);
		else if (stricmp(buf, "clipBullets") == 0)
			fpsWeapons[weapon].clipBullets = atol(varData);
		else if (stricmp(buf, "reloadSound") == 0)
			fpsWeapons[weapon].reloadSound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "reload2Sound") == 0)
			fpsWeapons[weapon].reload2Sound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "reload3Sound") == 0)
			fpsWeapons[weapon].reload3Sound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "recycleSound") == 0)
			fpsWeapons[weapon].recycleSound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "rgReloadSound") == 0)
			fpsWeapons[weapon].rgReloadSound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "deploySound") == 0)
			fpsWeapons[weapon].deploySound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "undeploySound") == 0)
			fpsWeapons[weapon].undeploySound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "hiddenMeshes") == 0)
			fpsWeapons[weapon].hiddenMeshes = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "weaponModel") == 0)
		{
			static char tString[255];
			strcpy (tString, FPS_DIR);
			strcat (tString, varData);

			fpsWeapons[weapon].weaponModel = (SCHAR*) strpool (tString);
		}
		else if (stricmp(buf, "weaponConnectTo") == 0)
			fpsWeapons[weapon].weaponConnectTo = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "weaponConnect") == 0)
			fpsWeapons[weapon].weaponConnect = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "clipModel") == 0)
		{
			static char tString[255];
			strcpy (tString, FPS_DIR);
			strcat (tString, varData);

			fpsWeapons[weapon].clipModel = (SCHAR*) strpool (tString);
		}
		else if (stricmp(buf, "clipConnectTo") == 0)
			fpsWeapons[weapon].clipConnectTo = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "clipConnect") == 0)
			fpsWeapons[weapon].clipConnect = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "auxRGCupModel") == 0)
		{
			static char tString[255];
			strcpy (tString, FPS_DIR);
			strcat (tString, varData);

			fpsWeapons[weapon].auxModel[FPS_AUX_RGCUP] = (SCHAR*) strpool (tString);
		}
		else if (stricmp(buf, "auxRGCupConnect") == 0)
			fpsWeapons[weapon].auxConnect[FPS_AUX_RGCUP] = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "auxRGCupConnectTo") == 0)
			fpsWeapons[weapon].auxConnectTo[FPS_AUX_RGCUP] = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "auxRGCupConnectToAlt") == 0)
			fpsWeapons[weapon].auxConnectToAlt[FPS_AUX_RGCUP] = (SCHAR*) strpool (varData);
		/*else if (stricmp(buf, "auxRGGrenadeModel") == 0)
		{
			char tString[255];
			memset (tString, 0, sizeof (tString));
			strcat (tString, FPS_DIR);
			strcat (tString, varData);

			fpsWeapons[weapon].auxModel[FPS_AUX_RGGRENADE] = (SCHAR*) strpool (tString);
		}*/ //DEG
		else if (stricmp(buf, "auxRGGrenadeConnect") == 0)
			fpsWeapons[weapon].auxConnect[FPS_AUX_RGGRENADE] = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "auxRGGrenadeConnectTo") == 0)
			fpsWeapons[weapon].auxConnectTo[FPS_AUX_RGGRENADE] = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "auxRGGrenadeConnectToAlt") == 0)
			fpsWeapons[weapon].auxConnectToAlt[FPS_AUX_RGGRENADE] = (SCHAR*) strpool (varData);

		else if (stricmp(buf, "rgFiringRules") == 0)
			fpsWeapons[weapon].rgFiringRules = (SINT32) atol (varData);
		else if (stricmp(buf, "rgFiringAngle") == 0)
			fpsWeapons[weapon].rgFiringAngle = (FLOAT32) atof (varData);

		else if (stricmp(buf, "leanFireOK") == 0)
			fpsWeapons[weapon].leanFireOK = strtoul(varData, nullptr, 10);

		else if (stricmp(buf, "onlyFireDeployed") == 0)
			fpsWeapons[weapon].onlyFireDeployed = strtoul(varData, nullptr, 10);
		else if (stricmp(buf, "suppressAim") == 0)
			fpsWeapons[weapon].suppressAim = strtoul(varData, nullptr, 10);
		else if (stricmp(buf, "deployPivotBone") == 0)
			fpsWeapons[weapon].deployPivotBone = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "perBulletATP") == 0)
			fpsWeapons[weapon].perBulletATP = (FLOAT32) atof (varData);
		else if (stricmp(buf, "riseStopsReload") == 0)
			fpsWeapons[weapon].riseStopsReload = strtoul(varData, nullptr, 10);
		else if (stricmp(buf, "delayAfterFire") == 0)
			fpsWeapons[weapon].delayAfterFire = (FLOAT32) atof (varData);

		else if (stricmp(buf, "sightAdjustSound") == 0)
			fpsWeapons[weapon].sightAdjustSound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "sightAdjustTime") == 0)
			fpsWeapons[weapon].sightAdjustTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "numConvDists") == 0)
			fpsWeapons[weapon].numSightDists = (SINT32) atol (varData);
		else if (stricmp(buf, "startConvDists") == 0)
			fpsWeapons[weapon].startSightDists = (SINT32) atol (varData);
		else if (stricmp(buf, "incConvDists") == 0)
			fpsWeapons[weapon].incSightDists = (SINT32) atol (varData);

		else if (stricmp(buf, "convDist1") == 0 || stricmp(buf, "convDist2") == 0 ||
                 stricmp(buf, "convDist3") == 0 || stricmp(buf, "convDist4") == 0 ||
                 stricmp(buf, "convDist5") == 0 || stricmp(buf, "convDist6") == 0 ||
                 stricmp(buf, "convDist7") == 0 || stricmp(buf, "convDist8") == 0 ||
                 stricmp(buf, "convDist9") == 0 || stricmp(buf, "convDist10") == 0) {
            if (!hasWarned) {
                WriteLog(LOG_FPS, LOG_WARN, "Data file %s is still using old convDistN, consider updating with startConvDists/incConvDists", filename);
                hasWarned = true;
            }
        }

		else if (stricmp(buf, "dryFireSound") == 0)
			fpsWeapons[weapon].dryFireSound = (SCHAR*) strpool (varData);
		else if (stricmp(buf, "dryFireSoundLength") == 0)
			fpsWeapons[weapon].dryFireSoundLength = (FLOAT32) atof (varData);

		else if (stricmp(buf, "placedObject") == 0)
			fpsWeapons[weapon].placedObject = (LOGICAL) atoi (varData);

		else
			WriteLog (LOG_FPS, LOG_INFO, "Unidentified key/value pair in weapon file %s .. %s = %s", filename, buf, varData);
	}

	fioClose (weapDataFile);
}

static void _fpFPSDataHandleWeaponTypeString (fpsWeaponTypes weapon, char* typestring)
{
	if (typestring == nullptr)
		return;

	int	type;

	for (type=FPS_WEAPONTYPE_SMG; type < FPS_WEAPONTYPE_NUM_TYPES; type++)
	{
		if (stricmp (typestring, fpFPSWeaponTypeStrings[type]) == 0)
			break;
	}

	if (type == FPS_WEAPONTYPE_NUM_TYPES)
	{
		WriteLog (LOG_FPS, LOG_INFO, "Unable to find match for weapon type string %s");
		return;
	}

	fpsWeapons[weapon].weaponType = (fpFPSWeaponType)type;

	static char defaultTypeFileName[255];
	strprintf(defaultTypeFileName, "default_%s.txt", fpFPSWeaponTypeStrings[type]);

	_fpFPSDataReadWeaponFile (weapon, defaultTypeFileName);
}

void fpFPSDataReadFPSFile()
{
	SINT32	fpsDataFile;
	char*	varName, *varData;
	static char	buf[255];
	static char	troopFileName[255];

	strcpy (troopFileName, FPS_DATA_DIR);
	strcat (troopFileName, FPS_DATA_FILE);

	fpsDataFile = fioOpen(troopFileName);

	if (fpsDataFile == -1)
	{
		// BAD!
		return;
	}


	/*

     TBD - is this test code??

    UINT64 val1 = fpStringHash("piatFile");
	UINT64 val1 = fpStringHash("m1garandFile");
	UINT64 val2 = fpStringHash("colt1911File");
	UINT64 val3 = fpStringHash("springfld1903File");
	UINT64 val4 = fpStringHash("bronwingbarFile");
	UINT64 val5 = fpStringHash("gew43File");
	*/

	while (fioGetString(fpsDataFile, (char*)buf, 255) != 0)
	{
		if (buf[0] == '#')
		{
			continue;
		}

		fpFPSDataGetVarnameAndContents(&varName, &varData, buf);

		if ((varName == nullptr) || (varData == nullptr))
			continue;

		// for easy lookup of new variable hashes
		//if(stricmp(varName, "concussionMod") == 0)
		//	int i = 0;

		UINT64 hash = fpStringHash(buf);

		switch (hash)
		{
		case swimDisplacement_hash:
			fpsTrooper.swimDisplacement = (FLOAT32) atof (varData);
		break;
		case eyeRideDisplacement_hash:
			fpsTrooper.eyeRideDisplacement = (FLOAT32) atof (varData);
		break;
		case eyeHeights_hash:
		{
			sscanf(varData, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
				&fpsTrooper.eyeHeight[FPS_EYE_DEAD],
				&fpsTrooper.eyeHeight[FPS_EYE_PRONE],
				&fpsTrooper.eyeHeight[FPS_EYE_PRONEMOVE],
				&fpsTrooper.eyeHeight[FPS_EYE_PRONEAIM],
				&fpsTrooper.eyeHeight[FPS_EYE_PRONEDEPLOY],
				&fpsTrooper.eyeHeight[FPS_EYE_CROUCH],
				&fpsTrooper.eyeHeight[FPS_EYE_CROUCHMOVE],
				&fpsTrooper.eyeHeight[FPS_EYE_CROUCHAIM],
				&fpsTrooper.eyeHeight[FPS_EYE_CROUCHDEPLOY],
				&fpsTrooper.eyeHeight[FPS_EYE_STAND],
				&fpsTrooper.eyeHeight[FPS_EYE_STANDMOVE],
				&fpsTrooper.eyeHeight[FPS_EYE_STANDAIM],
				&fpsTrooper.eyeHeight[FPS_EYE_STANDDEPLOY]);
		}
		break;
		/*
		case standEyeHeightDisplacement_hash:
			fpsTrooper.eyeHeight[FPS_POSITION_STAND] = (FLOAT32) atof (varData);
		break;
		case crouchEyeHeightDisplacement_hash:
			fpsTrooper.eyeHeight[FPS_POSITION_CROUCH] = (FLOAT32) atof (varData);
		break;
		case proneEyeHeightDisplacement_hash:
			fpsTrooper.eyeHeight[FPS_POSITION_PRONE] = (FLOAT32) atof (varData);
		break;
		case sitEyeHeightDisplacement_hash:
			fpsTrooper.eyeHeight[FPS_POSITION_SITTING] = (FLOAT32) atof (varData);
		break;
		*/
		case defaultFOV_hash:
			fpsTrooper.defaultFOV = (FLOAT32) atof (varData);
		break;
		case FOVSpeed_hash:
			fpsTrooper.FOVSpeed = (FLOAT32) atof (varData);
		break;
		case forwardAccel_hash:
			fpsTrooper.forwardAccel = (FLOAT32) atof (varData);
		break;
		case strafeAccel_hash:
			fpsTrooper.strafeAccel = (FLOAT32) atof (varData);
		break;
		case stillSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_STILL] = (FLOAT32) atof (varData);
		break;
		case sprintSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_SPRINT] = (FLOAT32) atof (varData);
		break;
		case trotSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_TROT] = (FLOAT32) atof (varData);
		break;
		case quickSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_QUICK] = (FLOAT32) atof (varData);
		break;
		case walkSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_WALK] = (FLOAT32) atof (varData);
		break;
		case creepSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_CREEP] = (FLOAT32) atof (varData);
		break;
		case crawlSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL] = (FLOAT32) atof (varData);
		break;
		case swimSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_SWIM] = (FLOAT32) atof (varData);
		break;
		case backwardsSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_BACKWARDS] = (FLOAT32) atof (varData);
		break;
		case backwardsCrouchSpeed_hash:
			fpsTrooper.movementSpeeds[FPS_SPEED_BACKWARDS_CROUCH] = (FLOAT32) atof (varData);
		break;
		/*
		case standToCrouchTime_hash:
			fpsTrooper.transitionTimes[FPS_POSITION_STAND][FPS_POSITION_CROUCH] = (FLOAT32) atof (varData);
		break;
		case standToProneTime_hash:
			fpsTrooper.transitionTimes[FPS_POSITION_STAND][FPS_POSITION_PRONE] = (FLOAT32) atof (varData);
		break;
		case crouchToStandTime_hash:
			fpsTrooper.transitionTimes[FPS_POSITION_CROUCH][FPS_POSITION_STAND] = (FLOAT32) atof (varData);
		break;
		case crouchToProneTime_hash:
			fpsTrooper.transitionTimes[FPS_POSITION_CROUCH][FPS_POSITION_PRONE] = (FLOAT32) atof (varData);
		break;
		case proneToStandTime_hash:
			fpsTrooper.transitionTimes[FPS_POSITION_PRONE][FPS_POSITION_STAND] = (FLOAT32) atof (varData);
		break;
		case proneToCrouchTime_hash:
			fpsTrooper.transitionTimes[FPS_POSITION_PRONE][FPS_POSITION_CROUCH] = (FLOAT32) atof (varData);
		break;
		*/
		case aimControl_hash:
			fpsTrooper.aimControl = (FLOAT32) atof (varData);
		break;
		case weaponChangeTime_hash:
			fpsTrooper.weaponChangeTime = ((FLOAT32) atof (varData) * .75f); // speed it up a little
		break;
		case maxSafeVehicleSpeed_hash:
			fpsTrooper.maxSafeVehicleSpeed = (FLOAT32) atof (varData);
		break;
		case killVehicleSpeed_hash:
			fpsTrooper.killVehicleSpeed = (FLOAT32) atof (varData);
		break;
		case unrideJoules_hash:
			fpsTrooper.unrideJoules = (FLOAT32) atof (varData);
		break;
		case maxRotationSpeed_hash:
			fpsTrooper.maxRotationSpeed = (FLOAT32) atof (varData);
		break;
		case keyRotationSpeed_hash:
			fpsTrooper.keyRotationSpeed = (FLOAT32) atof (varData);
		break;
		case maxEyeUpAngle_hash:
			fpsTrooper.maxEyeUpAngle = (FLOAT32) atof (varData);
		break;
		case maxEyeDownAngle_hash:
			fpsTrooper.maxEyeDownAngle = (FLOAT32) atof (varData);
		break;
		case maxRidingEyeUpAngle_hash:
			fpsTrooper.maxRidingEyeUpAngle = (FLOAT32) atof (varData);
		break;
		case maxRidingEyeDownAngle_hash:
			fpsTrooper.maxRidingEyeDownAngle = (FLOAT32) atof (varData);
		break;
		case maxEyeAngle_hash:
			fpsTrooper.maxHorizEyeAngle = (FLOAT32) atof (varData);
		break;
		case minEyeMovement_hash:
			fpsTrooper.minEyeMovement = (FLOAT32) atof (varData);
		break;
		case eyeMoveMultiplier_hash:
			fpsTrooper.eyeMoveMultiplier = (FLOAT32) atof (varData);
		break;
		case vertGranularityMultiplier_hash:
			fpsTrooper.vertGranularityMultiplier = (FLOAT32) atof (varData);
		break;
		case deployedGranularityMultiplier_hash:
			fpsTrooper.deployedGranularityMultiplier = (FLOAT32) atof (varData);
		break;
		case maxProneDeployedDownAngle_hash:
			fpsTrooper.maxProneDeployedDownAngle = (FLOAT32) atof (varData);
		break;
		case maxProneDeployedUpAngle_hash:
			fpsTrooper.maxProneDeployedUpAngle = (FLOAT32) atof (varData);
		break;
		case maxProneDeployedLeftAngle_hash:
			fpsTrooper.maxProneDeployedLeftAngle = (FLOAT32) atof (varData);
		break;
		case maxProneDeployedRightAngle_hash:
			fpsTrooper.maxProneDeployedRightAngle = (FLOAT32) atof (varData);
		break;
		case proneDeployedClimbMultiplier_hash:
			fpsTrooper.proneDeployedClimbMultiplier = (FLOAT32) atof (varData);
		break;
		case proneDeployedHorizShiftMultiplier_hash:
			fpsTrooper.proneDeployedHorizShiftMultiplier = (FLOAT32) atof (varData);
		break;
		case proneDeployedVertShiftMultiplier_hash:
			fpsTrooper.proneDeployedVertShiftMultiplier = (FLOAT32) atof (varData);
		break;
		case atpStandStill_hash:
			fpsTrooper.atpStandStill = (FLOAT32) atof (varData);
		break;
		case atpStandForward_hash:
			fpsTrooper.atpStandForward = (FLOAT32) atof (varData);
		break;
		case atpStandBackward_hash:
			fpsTrooper.atpStandBackward = (FLOAT32) atof (varData);
		break;
		case atpStandForwardWalk_hash:
			fpsTrooper.atpStandForwardWalk = (FLOAT32) atof (varData);
		break;
		case atpStandBackwardWalk_hash:
			fpsTrooper.atpStandBackwardWalk = (FLOAT32) atof (varData);
		break;
		case atpStandStillRelaxed_hash:
			fpsTrooper.atpStandStillRelaxed = (FLOAT32) atof (varData);
		break;
		case atpStandForwardRelaxed_hash:
			fpsTrooper.atpStandForwardRelaxed = (FLOAT32) atof (varData);
		break;
		case atpStandBackwardRelaxed_hash:
			fpsTrooper.atpStandBackwardRelaxed = (FLOAT32) atof (varData);
		break;
		case atpStandForwardWalkRelaxed_hash:
			fpsTrooper.atpStandForwardWalkRelaxed = (FLOAT32) atof (varData);
		break;
		case atpStandBackwardWalkRelaxed_hash:
			fpsTrooper.atpStandBackwardWalkRelaxed = (FLOAT32) atof (varData);
		break;
		case atpStandSprint_hash:
			fpsTrooper.atpStandSprint = (FLOAT32) atof (varData);
		break;
		case atpCrouchStill_hash:
			fpsTrooper.atpCrouchStill = (FLOAT32) atof (varData);
		break;
		case atpCrouchForward_hash:
			fpsTrooper.atpCrouchForward = (FLOAT32) atof (varData);
		break;
		case atpCrouchStillRelaxed_hash:
			fpsTrooper.atpCrouchStillRelaxed = (FLOAT32) atof (varData);
		break;
		case atpCrouchForwardRelaxed_hash:
			fpsTrooper.atpCrouchForwardRelaxed = (FLOAT32) atof (varData);
		break;
		case atpProneStill_hash:
			fpsTrooper.atpProneStill = (FLOAT32) atof (varData);
		break;
		case atpProneForward_hash:
			fpsTrooper.atpProneForward = (FLOAT32) atof (varData);
		break;
		case atpSwimStill_hash:
			fpsTrooper.atpSwimStill = (FLOAT32) atof (varData);
		break;
		case atpSwimForward_hash:
			fpsTrooper.atpSwimForward = (FLOAT32) atof (varData);
		break;
		case atpSitStill_hash:
			fpsTrooper.atpSitStill = (FLOAT32) atof (varData);
		break;
		case atpSprintMinimum_hash:
			fpsTrooper.atpSprintMinimum = (FLOAT32) atof (varData);
		break;
		case atpSprintAllowAgain_hash:
			fpsTrooper.atpSprintAllowAgain = (FLOAT32) atof (varData);
		break;
		case atpBackwardMinimum_hash:
			fpsTrooper.atpBackwardMinimum = (FLOAT32) atof (varData);
		break;
		case atpCreepMinimum_hash:
			fpsTrooper.atpCreepMinimum = (FLOAT32) atof (varData);
		break;
		case atpCrawlMinimum_hash:
			fpsTrooper.atpCrawlMinimum = (FLOAT32) atof (varData);
		break;
		case atpSwimMinimum_hash:
			fpsTrooper.atpSwimMinimum = (FLOAT32) atof (varData);
		break;

        case restPulse_hash:
			fpsTrooper.restPulse = (FLOAT32) atof (varData);
		break;
		case maxPulse_hash:
			fpsTrooper.maxPulse = (FLOAT32) atof (varData);
		break;
		case maxHeartBreathMultiplier_hash:
			fpsTrooper.maxHeartBreathMultiplier = (FLOAT32) atof (varData);
		break;
		case timeToMaxHeartBreathMultiplier_hash:
			fpsTrooper.timeToMaxHeartBreathMultiplier = (FLOAT32) atof (varData);
		break;
		case maxPoseMultiplier_hash:
			fpsTrooper.maxPoseMultiplier = (FLOAT32) atof (varData);
		break;
		case timeToMaxPoseMultiplier_hash:
			fpsTrooper.timeToMaxPoseMultiplier = (FLOAT32) atof (varData);
		break;
		case rejuvTimeMultiplier_hash:
			fpsTrooper.rejuvTimeMultiplier = (FLOAT32) atof (varData);
		break;
		case moveMultiplier_hash:
			fpsTrooper.moveMultiplier = (FLOAT32) atof (varData);
		break;
		case heartMultiplier_hash:
			fpsTrooper.heartMultiplier = (FLOAT32) atof (varData);
		break;
		case breathMultiplier_hash:
			fpsTrooper.breathMultiplier = (FLOAT32) atof (varData);
		break;
		case poseMultiplier_hash:
			fpsTrooper.poseMultiplier = (FLOAT32) atof (varData);
		break;
		case pulseBreathRatio_hash:
			fpsTrooper.pulseBreathRatio = (FLOAT32) atof (varData);
		break;
		case standPoseWobbleMultiplier_hash:
			fpsTrooper.standPoseWobbleMultiplier = (FLOAT32) atof (varData);
		break;
		case kneelPoseWobbleMultiplier_hash:
			fpsTrooper.kneelPoseWobbleMultiplier = (FLOAT32) atof (varData);
		break;
		case pronePoseWobbleMultiplier_hash:
			fpsTrooper.pronePoseWobbleMultiplier = (FLOAT32) atof (varData);
		break;
		case minArmWobbleTime_hash:
			fpsTrooper.minArmWobbleTime = (FLOAT32) atof (varData);
		break;
		case maxArmWobbleTime_hash:
			fpsTrooper.maxArmWobbleTime = (FLOAT32) atof (varData);
		break;
		case horizWobbleAngleMultiplier_hash:
			fpsTrooper.horizWobbleAngleMultiplier = (FLOAT32) atof (varData);
		break;
		case vertWobbleAngleMultiplier_hash:
			fpsTrooper.vertWobbleAngleMultiplier = (FLOAT32) atof (varData);
		break;
		case fallFarTime_hash:
			fpsTrooper.fallFarTime = (FLOAT32) atof (varData);
		break;
		case fallShortTime_hash:
			fpsTrooper.fallShortTime = (FLOAT32) atof (varData);
		break;
		case maxWobbleAngle_hash:
			fpsTrooper.maxWobbleAngle = (FLOAT32) atof (varData);
		break;
		case aimBreathTime_hash:
			fpsTrooper.aimBreathTime = (FLOAT32) atof (varData);
		break;
		case aimStartValue_hash:
			fpsTrooper.aimStartValue = (FLOAT32) atof (varData);
		break;
		case bridgeDamageJoules_hash:
			fpsTrooper.bridgeDamageJoules = (FLOAT64) atof (varData);
		break;
		case bridgeRepairJoules_hash:
			fpsTrooper.bridgeRepairJoules = (FLOAT64) atof (varData);
		break;
		case redScreenFadeTime_hash:
			fpsTrooper.redScreenFadeTime = (FLOAT32) atof (varData);
		break;
		case redScreenMinAlpha_hash:
			fpsTrooper.redScreenMinAlpha = (FLOAT32) atof (varData);
		break;
		case concussionMod_hash:
			fpsTrooper.concussionMod = (FLOAT32) atof (varData);
		break;
		case parachuteDeployTime_hash:
			fpsTrooper.parachuteDeployTime = (FLOAT32) atof (varData);
		break;
		case parachuteWornATPMultiplier_hash:
			fpsTrooper.parachuteWornATPMultiplier = (FLOAT32) atof (varData);
		break;
		case paratroopATPModifier_hash:
			fpsTrooper.paratroopATPModifier = (FLOAT32) atof (varData);
		break;
		case parachuteFallSpeed_hash:
			fpsTrooper.parachuteFallSpeed = (FLOAT32) atof (varData);
		break;
		case minSafeFallDistance_hash:
			fpsTrooper.minSafeFallDistance = (FLOAT32) atof (varData);
		break;

		case overSupplyRadius_hash:
			fpsTrooper.overSupplyRadius = (FLOAT32) atof (varData);
		break;

		case leanLeftHoriz_hash:
			fpsTrooper.leanLeftHoriz = (FLOAT32) atof (varData);
		break;
		case leanLeftVert_hash:
			fpsTrooper.leanLeftVert = (FLOAT32) atof (varData);
		break;
		case leanRightHoriz_hash:
			fpsTrooper.leanRightHoriz = (FLOAT32) atof (varData);
		break;
		case leanRightVert_hash:
			fpsTrooper.leanRightVert = (FLOAT32) atof (varData);
		break;
		case leanTime_hash:
			fpsTrooper.leanTime = (FLOAT32) atof (varData);
		break;

		case thompson1928File_hash:
			_fpFPSDataReadWeaponFile (THOMPSON, varData);
		break;
		case m1thompsonFile_hash:
			_fpFPSDataReadWeaponFile (M1THOMPSON, varData);
		break;
		case mp40File_hash:
			_fpFPSDataReadWeaponFile (MP40, varData);
		break;
		case masFile_hash:
			_fpFPSDataReadWeaponFile (MAS, varData);
		break;
		case satchelFile_hash:
			_fpFPSDataReadWeaponFile (SATCHEL, varData);
		break;
		case satchelAxFile_hash:
			_fpFPSDataReadWeaponFile (SATCHEL_AX, varData);
		break;//DEG_CHANGE
		case kar98kFile_hash:
			_fpFPSDataReadWeaponFile (KAR98K, varData);
		break;
		case kar98kSFile_hash:
			_fpFPSDataReadWeaponFile (KAR98KS, varData);
		break;
		case enfieldFile_hash:
			_fpFPSDataReadWeaponFile (ENFIELD, varData);
		break;
		case enfieldMk4File_hash:
			_fpFPSDataReadWeaponFile (ENFIELDMK4, varData);
		break;
		case lebelFile_hash:
			_fpFPSDataReadWeaponFile (LEBEL, varData);
		break;
		case mas36File_hash:
			_fpFPSDataReadWeaponFile (MAS36, varData);
		break;
		case mas40File_hash:
			_fpFPSDataReadWeaponFile (MAS40, varData);
		break;
		case lebelMleFile_hash:
			_fpFPSDataReadWeaponFile (LEBELMLE, varData);
		break;
		case m1935aFile_hash:
			_fpFPSDataReadWeaponFile (M1935A, varData);
		break;
		case lugerFile_hash:
			_fpFPSDataReadWeaponFile (LUGER, varData);
		break;
		case mills36mk1File_hash:
			_fpFPSDataReadWeaponFile (MILLS36MK1, varData);
		break;
        case frbangFile_hash:
			_fpFPSDataReadWeaponFile (FRBANG, varData);
		break;
		case stigrFile_hash:
			_fpFPSDataReadWeaponFile (STIGR, varData);
		break;
		case enfmk2File_hash:
			_fpFPSDataReadWeaponFile (WEBLEY, varData);
		break;
		case binocsFile_hash:
			_fpFPSDataReadWeaponFile (BINOCS, varData);
		break;
		case knife_axisFile_hash:
			_fpFPSDataReadWeaponFile (KNIFE_AXIS, varData);
		break;
		case knife_alliedFile_hash:
			_fpFPSDataReadWeaponFile (KNIFE_ALLIED, varData);
		break;
		case brenFile_hash:
			_fpFPSDataReadWeaponFile (BREN, varData);
		break;
		case mg34File_hash:
			_fpFPSDataReadWeaponFile (MG34, varData);
		break;
		case fm29File_hash:
			_fpFPSDataReadWeaponFile (FM29, varData);
		break;
		case hgr39File_hash:
			_fpFPSDataReadWeaponFile (HGR39, varData);
		break;
		case m18File_hash:
			_fpFPSDataReadWeaponFile (M18, varData);
		break;
		case frsmokeFile_hash:
			_fpFPSDataReadWeaponFile (FRSMOKE, varData);
		break;
		case buildToolFile_hash:
			_fpFPSDataReadWeaponFile (BUILD_TOOL, varData);
		break;

		case m1garandFile_hash:
			_fpFPSDataReadWeaponFile (M1GARAND, varData);
		break;
		case colt1911File_hash:
			_fpFPSDataReadWeaponFile (COLT1911, varData);
		break;
		case springfld1903File_hash:
			_fpFPSDataReadWeaponFile (SPRINGFIELD1903, varData);
		break;
		case bronwingbarFile_hash:
			_fpFPSDataReadWeaponFile (BAR, varData);
		break;
		case gew43File_hash:
			_fpFPSDataReadWeaponFile (GEW43, varData);
		break;
		case gew41File_hash:
			_fpFPSDataReadWeaponFile (GEW41, varData);
		break;
		case mark2File_hash:
			_fpFPSDataReadWeaponFile (MARK2, varData);
		break;
		case ammoFile_hash:
			_fpFPSDataReadWeaponFile (AMMO, varData);
		break;
		case g30File_hash:
			_fpFPSDataReadWeaponFile (RG_G30, varData);
		break;
		case vbFile_hash:
			_fpFPSDataReadWeaponFile (RG_VB, varData);
		break;
		case millsFile_hash:
			_fpFPSDataReadWeaponFile (RG_MILLS, varData);
		break;
		case boysmk1File_hash:
			_fpFPSDataReadWeaponFile (BOYSMK1, varData);
		break;
		case pzb39File_hash:
			_fpFPSDataReadWeaponFile (PZB39, varData);
		break;
		case bridgeRepairFile_hash:
			_fpFPSDataReadWeaponFile (BRIDGE_REPAIR, varData);
		break;
		case ukltmortarFile_hash:
			_fpFPSDataReadWeaponFile (LTMORTAR_UK, varData);
		break;
		case frltmortarFile_hash:
			_fpFPSDataReadWeaponFile (LTMORTAR_FR, varData);
		break;
		case deltmortarFile_hash:
			_fpFPSDataReadWeaponFile (LTMORTAR_DE, varData);
		break;

		case bazookaFile_hash:
			_fpFPSDataReadWeaponFile (BAZOOKA, varData);
		break;
		case pzschreckFile_hash:
			_fpFPSDataReadWeaponFile (PZSCHRECK, varData);
		break;

		case piatFile_hash:
			_fpFPSDataReadWeaponFile (PIAT, varData);
		break;

		case m1903a3File_hash:
			_fpFPSDataReadWeaponFile (M1903A3, varData);
		break;

		case ussmokeFile_hash:
			_fpFPSDataReadWeaponFile (USSMOKE, varData);
		break;

		case stenmk2File_hash:
			_fpFPSDataReadWeaponFile(STENMK2, varData);
		break;
		case m3a1File_hash:
			_fpFPSDataReadWeaponFile(M3A1, varData);
		break;
		case mp34File_hash:
			_fpFPSDataReadWeaponFile(MP34, varData);
		break;
		case fg42gFile_hash:
			_fpFPSDataReadWeaponFile (FG42G, varData);
		break;

		case crawlSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_CRAWL] = (SCHAR*) strpool (varData);
		break;
		case creepSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_CREEP] = (SCHAR*) strpool (varData);
		break;
		case creepAimSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_CREEP_AIM] = (SCHAR*) strpool (varData);
		break;
		case trotSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_TROT] = (SCHAR*) strpool (varData);
		break;
		case trotAimSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_TROT_AIM] = (SCHAR*) strpool (varData);
		break;
		case sprintSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_SPRINT] = (SCHAR*) strpool (varData);
		break;
		case waterSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_WATER] = (SCHAR*) strpool (varData);
		break;
		case fallNormalSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_FALL_NORMAL] = (SCHAR*) strpool (varData);
		break;
		case fallNormalHurtSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_FALL_NORMAL_HURT] = (SCHAR*) strpool (varData);
		break;
		case fallWaterSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_FALL_WATER] = (SCHAR*) strpool (varData);
		break;
		case fallWaterHurtSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_FALL_WATER_HURT] = (SCHAR*) strpool (varData);
		break;
		case aimBreathSound_hash:
			fpsTrooper.aimBreathSound = (SCHAR*) strpool (varData);
		break;
		case fallShortSound_hash:
			fpsTrooper.fallShortSound = (SCHAR*) strpool (varData);
		break;
		case fallFarSound_hash:
			fpsTrooper.fallFarSound = (SCHAR*) strpool (varData);
		break;
		case paratroopFreeFallSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_FREEFALL] = (SCHAR*) strpool (varData);
		break;
		case parachuteOpeningSound_hash:
			fpsTrooper.parachuteOpeningSound = (SCHAR*) strpool (varData);
		break;
		case parachuteDeployedSound_hash:
			fpsTrooper.moveSounds[FPS_SOUND_PARACHUTE_FALL] = (SCHAR*) strpool (varData);
		break;
		case paratroopLandingSound_hash:
			fpsTrooper.paratroopLandingSound = (SCHAR*) strpool (varData);
		break;
		case paratroopLandingWaterSound_hash:
			fpsTrooper.paratroopLandingWaterSound = (SCHAR*) strpool (varData);
		break;

		default:
			WriteDebug("Unknown hash: '%s' : %" PR64BitUT "", buf, hash);
		}
	}

	fioClose (fpsDataFile);
}

// can't use the mlutility function because it strips spaces, and I need those
UINT32 fpFPSDataGetVarnameAndContents(SCHAR ** varname, SCHAR ** varcontents, SCHAR * inputString)
{
	*varname = strtok((char*)inputString,"=");

	if (*varname != nullptr)
	{
		*varcontents = strtok(nullptr,"=");
		*varcontents = strtok((char*)*varcontents,"\n"); // strip off the ending newline

		// strip off leading & trailing spaces
		while (*varname[0] == ' ')
		{
			*varname = *varname + 1;
		}

		while ((*varname)[strlen(*varname)-1] == ' ')
		{
			(*varname)[strlen(*varname)-1] = 0;
		}

		if (*varcontents != nullptr)
		{
			while (*varcontents[0] == ' ')
			{
				*varcontents = *varcontents + 1;
			}
			while ((*varcontents)[strlen(*varcontents)-1] == ' ')
			{
				(*varcontents)[strlen(*varcontents)-1] = 0;
			}
		}

	}
	else
	{
		*varcontents = nullptr;
	}


	return 0;
}

void fpFPSWeaponData::clear()
{
	CSAFE_fpFPSWeaponData::clear();

	clipBullets = 0;			// number of bullets in current clip
	convergenceAngle = 0;
	suppressAim = 0;
	perBulletATP = 0;			// ATP drain per bullet
	delayAfterFire = 0;			// how long after the weapon is fired to delay reload & recycle animations
}

void fpFPSTrooperData::clear()
{
	CSAFE_fpFPSTrooperData::clear();

	initialized = 0;
	binocsEnabled = 0;
	isParatroop = 0;
	parachuteWorn = 0;
	parachuteOpen = 0;
	parachuteOKToOpen = 0;
	readingMap = 0;
	autoReload = 0;

	forwardAccel = 0.0f;
	strafeAccel = 0.0f;

	atpStandStill = 0.0f;
	atpStandForward = 0.0f;
	atpStandForwardWalk = 0.0f;
	atpStandBackward = 0.0f;
	atpStandBackwardWalk = 0.0f;
	atpStandStillRelaxed = 0.0f;
	atpStandForwardRelaxed = 0.0f;
	atpStandForwardWalkRelaxed = 0.0f;
	atpStandBackwardRelaxed = 0.0f;
	atpStandBackwardWalkRelaxed = 0.0f;
	atpStandSprint = 0.0f;
	atpCrouchStill = 0.0f;
	atpCrouchForward = 0.0f;
	atpCrouchStillRelaxed = 0.0f;
	atpCrouchForwardRelaxed = 0.0f;
	atpProneStill = 0.0f;
	atpProneForward = 0.0f;
	atpSwimStill = 0.0f;
	atpSwimForward = 0.0f;
	atpSitStill = 0.0f;

	atpSprintMinimum = 0.0f;
	atpSprintAllowAgain = 0.0f;
	atpCreepMinimum = 0.0f;
	atpCrawlMinimum = 0.0f;
	atpSwimMinimum = 0.0f;
	atpBackwardMinimum = 0.0f;

	concussionMod = 0.0f;
	doGravity = FALSE;

	defaultFOV = 0.0f;
	desiredFOV = 0.0f;
}

void fpFPSDataInit ()
{
	int		i, j;
	SINT32	binocsEnabled;

	// clear out the anim_files array
	for (i=0; i < NUM_WEAPONS; i++)
	{
		fpsWeapons[i].clear();

		weap_default[i] = "";
		clip_default[i] = "";
		weap_empty_default[i] = "";

		for (j=0; j < NUM_WEAP_STATES; j++)
		{
			arms_anim_files[i][j] = "";
			weap_anim_files[i][j] = "";
			clip_anim_files[i][j] = "";
		}

		// set all the weapon multipliers to 1.0
		fpsWeapons[i].weaponBreathMultiplier = 1.0f;
		fpsWeapons[i].weaponHeartMultiplier = 1.0f;
		fpsWeapons[i].weaponMoveMultiplier = 1.0f;
		fpsWeapons[i].weaponPoseMultiplier = 1.0f;
		fpsWeapons[i].controlability = 1.0f;
		fpsWeapons[i].speedMultiplier = 1.0f;
		fpsWeapons[i].unaimedDispersionMod = 1.0f;
	}

	// then fill in as needed

	// THOMPSON animation files
	arms_anim_files[THOMPSON][RELOAD] = FPS_DIR "thompson\\animations\\reload\\armsmesh-britinf-reload(65)";
	weap_anim_files[THOMPSON][RELOAD] = FPS_DIR "thompson\\animations\\reload\\thompson-reload(65)";
	//clip_anim_files[THOMPSON][RELOAD] = FPS_DIR "thompson\\animations\\reload\\thompson-mag-reload(65)";

	arms_anim_files[THOMPSON][STAND_FIRE] = FPS_DIR "thompson\\animations\\fire\\armsmesh-thomp-fire(3)";
	weap_anim_files[THOMPSON][STAND_FIRE] = FPS_DIR "thompson\\animations\\fire\\thompson-fire(3)";

	arms_anim_files[THOMPSON][CROUCH_FIRE] = FPS_DIR "thompson\\animations\\fire\\crouch-thompson-fire(5)";
	weap_anim_files[THOMPSON][CROUCH_FIRE] = FPS_DIR "thompson\\animations\\fire\\thompson-fire(3)";

	arms_anim_files[THOMPSON][AIM_FIRE] = FPS_DIR "thompson\\animations\\fire\\aim-thompson-fire(3)";
	weap_anim_files[THOMPSON][AIM_FIRE] = FPS_DIR "thompson\\animations\\fire\\thompson-fire(3)";

	arms_anim_files[THOMPSON][LOWER] = FPS_DIR "thompson\\animations\\thompson-lowered(1)";

	arms_anim_files[THOMPSON][IDLE] = FPS_DIR "thompson\\animations\\thomp-stand(1)";
	arms_anim_files[THOMPSON][KNEEL] = FPS_DIR "thompson\\animations\\creep-thompson(1)";
	arms_anim_files[THOMPSON][AIM] = FPS_DIR "thompson\\animations\\thomp-stand-aim(1)";
	arms_anim_files[THOMPSON][AIM_SMG] = FPS_DIR "thompson\\animations\\thomp-stand-aim(1)";
	arms_anim_files[THOMPSON][TROT] = FPS_DIR "thompson\\animations\\thomp-stand(1)";
	arms_anim_files[THOMPSON][CREEP] = FPS_DIR "thompson\\animations\\creep-thompson(1)";
	arms_anim_files[THOMPSON][CRAWL] = FPS_DIR "thompson\\animations\\crawl-thompson(30)";
	arms_anim_files[THOMPSON][SPRINT] = FPS_DIR "thompson\\animations\\sprint-thompson(15)";
	//arms_anim_files[THOMPSON][SIT] = FPS_DIR "thompson\\animations\\thompson-arms-sit(1)";
	arms_anim_files[THOMPSON][SIT] = FPS_DIR "thompson\\animations\\thompson-lowered(1)";

	//M1THOMPSON animation files
	arms_anim_files[M1THOMPSON][RELOAD] = FPS_DIR "thompson\\animations\\reload\\arms_m1_reload(90)";
		weap_anim_files[M1THOMPSON][RELOAD] = FPS_DIR "thompson\\animations\\reload\\weap_m1_reload(90)";
	//clip_anim_files[M1THOMPSON][RELOAD] = FPS_DIR "thompson\\animations\\reload\\thompson-mag-reload(65)";

	arms_anim_files[M1THOMPSON][STAND_FIRE] = FPS_DIR "thompson\\animations\\fire\\arms_m1_fire(3)";
		weap_anim_files[M1THOMPSON][STAND_FIRE] = FPS_DIR "thompson\\animations\\fire\\weap_m1_fire(3)";

	arms_anim_files[M1THOMPSON][CROUCH_FIRE] = FPS_DIR "thompson\\animations\\fire\\arms_m1_fire(3)";
		weap_anim_files[M1THOMPSON][CROUCH_FIRE] = FPS_DIR "thompson\\animations\\fire\\weap_m1_fire(3)";

	arms_anim_files[M1THOMPSON][AIM_FIRE] = FPS_DIR "thompson\\animations\\fire\\arms_m1_aim_fire(3)";
		weap_anim_files[M1THOMPSON][AIM_FIRE] = FPS_DIR "thompson\\animations\\fire\\weap_m1_fire(3)";

	arms_anim_files[M1THOMPSON][LOWER] = FPS_DIR "thompson\\animations\\arms_m1_lowered(1)";

	arms_anim_files[M1THOMPSON][IDLE] = FPS_DIR "thompson\\animations\\arms_m1_neutral(1)";
	arms_anim_files[M1THOMPSON][KNEEL] = FPS_DIR "thompson\\animations\\arms_m1_neutral(1)";
	arms_anim_files[M1THOMPSON][AIM] = FPS_DIR "thompson\\animations\\arms_m1_aim(1)";
	arms_anim_files[M1THOMPSON][AIM_SMG] = FPS_DIR "thompson\\animations\\arms_m1_aim(1)";
	arms_anim_files[M1THOMPSON][TROT] = FPS_DIR "thompson\\animations\\arms_m1_neutral(1)";
	arms_anim_files[M1THOMPSON][CREEP] = FPS_DIR "thompson\\animations\\arms_m1_neutral(1)";
	arms_anim_files[M1THOMPSON][CRAWL] = FPS_DIR "thompson\\animations\\arms_m1_crawl(30)";
	arms_anim_files[M1THOMPSON][SPRINT] = FPS_DIR "thompson\\animations\\arms_m1_sprint(15)";
	//arms_anim_files[M1THOMPSON][SIT] = FPS_DIR "thompson\\animations\\thompson-arms-sit(1)";
	arms_anim_files[M1THOMPSON][SIT] = FPS_DIR "thompson\\animations\\arms_m1_sit(1)";


	// MP40 animation files
	arms_anim_files[MP40][RELOAD] = FPS_DIR "mp40\\animations\\reload\\armsmesh-mp40-reload(65)";
	weap_anim_files[MP40][RELOAD] = FPS_DIR "mp40\\animations\\reload\\mp40-reload(65)";
	arms_anim_files[MP40][STAND_FIRE] = FPS_DIR "mp40\\animations\\fire\\arms-mp40-fire(3)";
	weap_anim_files[MP40][STAND_FIRE] = FPS_DIR "mp40\\animations\\fire\\mp40-fire(3)";
	arms_anim_files[MP40][CROUCH_FIRE] = FPS_DIR "mp40\\animations\\fire\\crouch-mp40-fire(3)";
	weap_anim_files[MP40][CROUCH_FIRE] = FPS_DIR "mp40\\animations\\fire\\mp40-fire(3)";
	arms_anim_files[MP40][AIM_FIRE] = FPS_DIR "mp40\\animations\\fire\\aim-mp40-fire(3)";
	weap_anim_files[MP40][AIM_FIRE] = FPS_DIR "mp40\\animations\\fire\\mp40-fire(3)";
	arms_anim_files[MP40][LOWER] = FPS_DIR "mp40\\animations\\mp40-lowered(1)";
	arms_anim_files[MP40][IDLE] = FPS_DIR "mp40\\animations\\mp40-stand(1)";
	arms_anim_files[MP40][KNEEL] = FPS_DIR "mp40\\animations\\creep-mp40(1)";
	arms_anim_files[MP40][AIM] = FPS_DIR "mp40\\animations\\mp40-stand-aim(1)";
	arms_anim_files[MP40][AIM_SMG] = FPS_DIR "mp40\\animations\\mp40-stand-aim(1)";
	arms_anim_files[MP40][TROT] = FPS_DIR "mp40\\animations\\mp40-stand(1)";
	arms_anim_files[MP40][CREEP] = FPS_DIR "mp40\\animations\\creep-mp40(1)";
	arms_anim_files[MP40][CRAWL] = FPS_DIR "mp40\\animations\\crawl-mp40(30)";
	arms_anim_files[MP40][SPRINT] = FPS_DIR "mp40\\animations\\sprint-mp40(15)";
	arms_anim_files[MP40][SIT] = FPS_DIR "mp40\\animations\\mp40-lowered(1)";

	// MAS animation files
	arms_anim_files[MAS][RELOAD] = FPS_DIR "mas\\animations\\reload\\armsmesh-mas-reload(65)";
	weap_anim_files[MAS][RELOAD] = FPS_DIR "mas\\animations\\reload\\mas-reload(65)";
	arms_anim_files[MAS][STAND_FIRE] = FPS_DIR "mas\\animations\\fire\\arms-mas-fire(3)";
    weap_anim_files[MAS][STAND_FIRE] = FPS_DIR "mas\\animations\\fire\\gunmesh-mas-fire(3)";
	arms_anim_files[MAS][CROUCH_FIRE] = FPS_DIR "mas\\animations\\fire\\crouch-mas-fire(3)";
    weap_anim_files[MAS][CROUCH_FIRE] = FPS_DIR "mas\\animations\\fire\\gunmesh-mas-fire(3)";
	arms_anim_files[MAS][AIM_FIRE] = FPS_DIR "mas\\animations\\fire\\aim-mas-fire(3)";
	weap_anim_files[MAS][AIM_FIRE] = FPS_DIR "mas\\animations\\fire\\gunmesh-mas-fire(3)";
	arms_anim_files[MAS][LOWER] = FPS_DIR "mas\\animations\\mas-lowered(1)";
	arms_anim_files[MAS][IDLE] = FPS_DIR "mas\\animations\\mas-stand(1)";
	arms_anim_files[MAS][KNEEL] = FPS_DIR "mas\\animations\\creep-mas(1)";
	arms_anim_files[MAS][AIM] = FPS_DIR "mas\\animations\\mas-stand-aim(1)";
	arms_anim_files[MAS][AIM_SMG] = FPS_DIR "mas\\animations\\mas-stand-aim(1)";
	arms_anim_files[MAS][TROT] = FPS_DIR "mas\\animations\\mas-stand(1)";
	arms_anim_files[MAS][CREEP] = FPS_DIR "mas\\animations\\creep-mas(1)";
	arms_anim_files[MAS][CRAWL] = FPS_DIR "mas\\animations\\crawl-mas(30)";
	arms_anim_files[MAS][SPRINT] = FPS_DIR "mas\\animations\\sprint-mas(15)";
	arms_anim_files[MAS][SIT] = FPS_DIR "mas\\animations\\mas-lowered(1)";

	// SATCHEL animation files
	arms_anim_files[SATCHEL][STAND_FIRE] = FPS_DIR "satchel\\animations\\fire\\satchel-fire(1)";
	arms_anim_files[SATCHEL][RELOAD] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";
	arms_anim_files[SATCHEL][LOWER] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";
	arms_anim_files[SATCHEL][IDLE] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL][KNEEL] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL][TROT] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL][CREEP] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL][CRAWL] = FPS_DIR "satchel\\animations\\crawl-satchel(30)";
	arms_anim_files[SATCHEL][SPRINT] = FPS_DIR "satchel\\animations\\sprint-satchel(15)";
    arms_anim_files[SATCHEL][SIT] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";

    // SATCHEL_AX animation files
	//DEG_CHANGE
	weap_default[SATCHEL_AX] = FPS_DIR "satchel\\gunbone-satchel(1)";
	arms_anim_files[SATCHEL_AX][STAND_FIRE] = FPS_DIR "satchel\\animations\\fire\\satchel-fire(1)";
	arms_anim_files[SATCHEL_AX][RELOAD] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";
	arms_anim_files[SATCHEL_AX][LOWER] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";
	arms_anim_files[SATCHEL_AX][IDLE] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL_AX][KNEEL] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL_AX][TROT] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL_AX][CREEP] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[SATCHEL_AX][CRAWL] = FPS_DIR "satchel\\animations\\crawl-satchel(30)";
	arms_anim_files[SATCHEL_AX][SPRINT] = FPS_DIR "satchel\\animations\\sprint-satchel(15)";
	arms_anim_files[SATCHEL_AX][SIT] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";

	// KAR98K animation files
	arms_anim_files[KAR98K][RELOAD] = FPS_DIR "kar98k\\animations\\reload\\armsmesh-kar98k-reload(75)";
	weap_anim_files[KAR98K][RELOAD] = FPS_DIR "kar98k\\animations\\reload\\kar98k-reload(75)";
	arms_anim_files[KAR98K][RECYCLE] = FPS_DIR "kar98k\\animations\\recycle\\armsmesh-kar98k-recycle(60)";
	weap_anim_files[KAR98K][RECYCLE] = FPS_DIR "kar98k\\animations\\recycle\\kar98k-recycle(60)";
	arms_anim_files[KAR98K][STAND_FIRE] = FPS_DIR "kar98k\\animations\\fire\\arms-kar98k-fire(3)";
	arms_anim_files[KAR98K][CROUCH_FIRE] = FPS_DIR "kar98k\\animations\\fire\\crouch-kar98k-fire(3)";
	arms_anim_files[KAR98K][AIM_FIRE] = FPS_DIR "kar98k\\animations\\fire\\aim-kar98k-fire(3)";
	arms_anim_files[KAR98K][LOWER] = FPS_DIR "kar98k\\animations\\kar98k-lowered(1)";
	arms_anim_files[KAR98K][IDLE] = FPS_DIR "kar98k\\animations\\kar98k-stand(1)";
	arms_anim_files[KAR98K][KNEEL] = FPS_DIR "kar98k\\animations\\creep-kar98k(1)";
	arms_anim_files[KAR98K][AIM] = FPS_DIR "kar98k\\animations\\kar98k-stand-aim(1)";
	arms_anim_files[KAR98K][TROT] = FPS_DIR "kar98k\\animations\\kar98k-stand(1)";
	arms_anim_files[KAR98K][CREEP] = FPS_DIR "kar98k\\animations\\creep-kar98k(1)";
	arms_anim_files[KAR98K][CRAWL] = FPS_DIR "kar98k\\animations\\crawl-kar98k(30)";
	arms_anim_files[KAR98K][SPRINT] = FPS_DIR "kar98k\\animations\\sprint-kar98k(15)";
	arms_anim_files[KAR98K][SIT] = FPS_DIR "kar98k\\animations\\kar98k-lowered(1)";

	arms_anim_files[GEW43][RELOAD] = FPS_DIR "gewehr\\animations\\reload\\arms_gewehr_reload(72)";
		weap_anim_files[GEW43][RELOAD] = FPS_DIR "gewehr\\animations\\reload\\weap_gewehr_reload(72)";
	arms_anim_files[GEW43][STAND_FIRE] = FPS_DIR "gewehr\\animations\\fire\\arms_gewehr_fire(3)";
		weap_anim_files[GEW43][STAND_FIRE] = FPS_DIR "gewehr\\animations\\fire\\weap_gewehr_fire(3)";
	arms_anim_files[GEW43][CROUCH_FIRE] = FPS_DIR "gewehr\\animations\\fire\\arms_gewehr_fire(3)";
		weap_anim_files[GEW43][CROUCH_FIRE] = FPS_DIR "gewehr\\animations\\fire\\weap_gewehr_fire(3)";
	arms_anim_files[GEW43][AIM_FIRE] = FPS_DIR "gewehr\\animations\\fire\\arms_gewehr_fire_aim(3)";
		weap_anim_files[GEW43][AIM_FIRE] = FPS_DIR "gewehr\\animations\\fire\\weap_gewehr_fire(3)";
	arms_anim_files[GEW43][LOWER] = FPS_DIR "gewehr\\animations\\arms_gewehr_lowered(1)";
	arms_anim_files[GEW43][IDLE] = FPS_DIR "gewehr\\animations\\arms_gewehr_neutral(1)";
	arms_anim_files[GEW43][KNEEL] = FPS_DIR "gewehr\\animations\\arms_gewehr_neutral(1)";
	arms_anim_files[GEW43][AIM] = FPS_DIR "gewehr\\animations\\arms_gewehr_aim(1)";
	arms_anim_files[GEW43][TROT] = FPS_DIR "gewehr\\animations\\arms_gewehr_neutral(1)";
	arms_anim_files[GEW43][CREEP] = FPS_DIR "gewehr\\animations\\arms_gewehr_neutral(1)";
	arms_anim_files[GEW43][CRAWL] = FPS_DIR "gewehr\\animations\\arms_gewehr_crawl(30)";
	arms_anim_files[GEW43][SPRINT] = FPS_DIR "gewehr\\animations\\arms_gewehr_sprint(15)";
	arms_anim_files[GEW43][SIT] = FPS_DIR "gewehr\\animations\\arms_gewehr_lowered(1)";

	arms_anim_files[GEW41][RELOAD]      = FPS_DIR "gewehr41\\animations\\reload\\arms_gewehr41_reload";
	weap_anim_files[GEW41][RELOAD]      = FPS_DIR "gewehr41\\animations\\reload\\weap_gewehr41_reload";
	arms_anim_files[GEW41][STAND_FIRE]  = FPS_DIR "gewehr41\\animations\\fire\\arms_gewehr41_fire";
	weap_anim_files[GEW41][STAND_FIRE]  = FPS_DIR "gewehr41\\animations\\fire\\weap_gewehr41_fire";
	arms_anim_files[GEW41][CROUCH_FIRE] = FPS_DIR "gewehr41\\animations\\fire\\arms_gewehr41_fire";
	weap_anim_files[GEW41][CROUCH_FIRE] = FPS_DIR "gewehr41\\animations\\fire\\weap_gewehr41_fire";
	arms_anim_files[GEW41][AIM_FIRE]    = FPS_DIR "gewehr41\\animations\\fire\\arms_gewehr41_fire_aim";
	weap_anim_files[GEW41][AIM_FIRE]    = FPS_DIR "gewehr41\\animations\\fire\\weap_gewehr41_fire";
	arms_anim_files[GEW41][LOWER]       = FPS_DIR "gewehr41\\animations\\arms_gewehr41_lowered";
	arms_anim_files[GEW41][IDLE]        = FPS_DIR "gewehr41\\animations\\arms_gewehr41_neutral";
	arms_anim_files[GEW41][KNEEL]       = FPS_DIR "gewehr41\\animations\\arms_gewehr41_neutral";
	arms_anim_files[GEW41][AIM]         = FPS_DIR "gewehr41\\animations\\arms_gewehr41_aim";
	arms_anim_files[GEW41][TROT]        = FPS_DIR "gewehr41\\animations\\arms_gewehr41_neutral";
	arms_anim_files[GEW41][CREEP]       = FPS_DIR "gewehr41\\animations\\arms_gewehr41_neutral";
	arms_anim_files[GEW41][CRAWL]       = FPS_DIR "gewehr41\\animations\\arms_gewehr41_crawl";
	arms_anim_files[GEW41][SPRINT]      = FPS_DIR "gewehr41\\animations\\arms_gewehr41_sprint";
	arms_anim_files[GEW41][SIT]         = FPS_DIR "gewehr41\\animations\\arms_gewehr41_lowered";

	// KAR98KS Sniper animation files
	//weap_default[KAR98KS] = FPS_DIR "kar98k\\gunbone-kar98k";
	//clip_default[KAR98KS] = FPS_DIR "kar98k\\gunbone-5roundstripperclip";

	arms_anim_files[KAR98KS][RELOAD] = FPS_DIR "kar98k\\animations\\reload\\armsmesh-kar98k-reload(75)";
	weap_anim_files[KAR98KS][RELOAD] = FPS_DIR "kar98k\\animations\\reload\\kar98k-reload(75)";
	arms_anim_files[KAR98KS][RECYCLE] = FPS_DIR "kar98k\\animations\\recycle\\armsmesh-kar98k-recycle(60)";
	weap_anim_files[KAR98KS][RECYCLE] = FPS_DIR "kar98k\\animations\\recycle\\kar98k-recycle(60)";
	arms_anim_files[KAR98KS][STAND_FIRE] = FPS_DIR "kar98k\\animations\\fire\\arms-kar98k-fire(3)";
	arms_anim_files[KAR98KS][CROUCH_FIRE] = FPS_DIR "kar98k\\animations\\fire\\crouch-kar98k-fire(3)";
	arms_anim_files[KAR98KS][AIM_FIRE] = FPS_DIR "kar98k\\animations\\fire\\arms-kar98k-fire(3)";
	arms_anim_files[KAR98KS][LOWER] = FPS_DIR "kar98k\\animations\\kar98k-lowered(1)";
	arms_anim_files[KAR98KS][IDLE] = FPS_DIR "kar98k\\animations\\kar98k-stand(1)";
	arms_anim_files[KAR98KS][KNEEL] = FPS_DIR "kar98k\\animations\\creep-kar98k(1)";
	arms_anim_files[KAR98KS][AIM] = FPS_DIR "kar98k\\animations\\kar98k-stand-aim(1)";
	arms_anim_files[KAR98KS][TROT] = FPS_DIR "kar98k\\animations\\kar98k-stand(1)";
	arms_anim_files[KAR98KS][CREEP] = FPS_DIR "kar98k\\animations\\creep-kar98k(1)";
	arms_anim_files[KAR98KS][CRAWL] = FPS_DIR "kar98k\\animations\\crawl-kar98k(30)";
	arms_anim_files[KAR98KS][SPRINT] = FPS_DIR "kar98k\\animations\\sprint-kar98k(15)";
    arms_anim_files[KAR98KS][SIT] = FPS_DIR "kar98k\\animations\\kar98k-lowered(1)";

	// ENFIELD animation files
	arms_anim_files[ENFIELD][RELOAD] = FPS_DIR "enfield\\animations\\reload\\armsmesh-enfield-reload(115)";
	weap_anim_files[ENFIELD][RELOAD] = FPS_DIR "enfield\\animations\\reload\\enfield-reload(115)";
	arms_anim_files[ENFIELD][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\armsmesh-enfield-recycle(30)";
	weap_anim_files[ENFIELD][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\enfield-recycle(30)";
	arms_anim_files[ENFIELD][STAND_FIRE] = FPS_DIR "enfield\\animations\\fire\\arms-enfield-fire(3)";
	arms_anim_files[ENFIELD][CROUCH_FIRE] = FPS_DIR "enfield\\animations\\fire\\crouch-enfield-fire(3)";
	arms_anim_files[ENFIELD][AIM_FIRE] = FPS_DIR "enfield\\animations\\fire\\aim-enfield-fire(3)";
	arms_anim_files[ENFIELD][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[ENFIELD][IDLE] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[ENFIELD][KNEEL] = FPS_DIR "enfield\\animations\\creep-enfield(1)";
	arms_anim_files[ENFIELD][AIM] = FPS_DIR "enfield\\animations\\enfield-stand-aim(1)";
	arms_anim_files[ENFIELD][TROT] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[ENFIELD][CREEP] = FPS_DIR "enfield\\animations\\creep-enfield(1)";
	arms_anim_files[ENFIELD][CRAWL] = FPS_DIR "enfield\\animations\\crawl-enfield(30)";
	arms_anim_files[ENFIELD][SPRINT] = FPS_DIR "enfield\\animations\\sprint-enfield(15)";
	arms_anim_files[ENFIELD][SIT] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";

	// BAZOOKA temp animation files
	arms_anim_files[BAZOOKA][RELOAD] = FPS_DIR "m9bazooka\\animations\\reload\\arms_bazooka_reload";
	//weap_anim_files[BAZOOKA][RELOAD] = FPS_DIR "enfield\\animations\\reload\\enfield-reload(115)";
	//arms_anim_files[BAZOOKA][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\armsmesh-enfield-recycle(60)";
	//weap_anim_files[BAZOOKA][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\enfield-recycle(60)";
	arms_anim_files[BAZOOKA][STAND_FIRE] = FPS_DIR "m9bazooka\\animations\\fire\\arms_bazooka_fire";
	arms_anim_files[BAZOOKA][CROUCH_FIRE] = FPS_DIR "m9bazooka\\animations\\fire\\arms_bazooka_fire";
	arms_anim_files[BAZOOKA][AIM_FIRE] = FPS_DIR "m9bazooka\\animations\\fire\\arms_bazooka_fire";
	arms_anim_files[BAZOOKA][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[BAZOOKA][IDLE] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_neutral";
	arms_anim_files[BAZOOKA][KNEEL] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_neutral";
	arms_anim_files[BAZOOKA][AIM] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_aim";
	arms_anim_files[BAZOOKA][TROT] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_forward";
	arms_anim_files[BAZOOKA][CREEP] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_forward";
	arms_anim_files[BAZOOKA][CRAWL] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_forward";
	arms_anim_files[BAZOOKA][SPRINT] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_sprint";
	arms_anim_files[BAZOOKA][SIT] = FPS_DIR "m9bazooka\\animations\\arms_bazooka_neutral";

	// PZSCHRECK temp animation files
	arms_anim_files[PZSCHRECK][RELOAD] = FPS_DIR "panzerschreck\\animations\\reload\\arms_panzerschreck_reload(72)";
	arms_anim_files[PZSCHRECK][STAND_FIRE] = FPS_DIR "panzerschreck\\animations\\fire\\arms_panzerschreck_fire(3)";
	arms_anim_files[PZSCHRECK][CROUCH_FIRE] = FPS_DIR "panzerschreck\\animations\\fire\\arms_panzerschreck_fire(3)";
	arms_anim_files[PZSCHRECK][AIM_FIRE] = FPS_DIR "panzerschreck\\animations\\fire\\arms_panzerschreck_fire(3)";
	arms_anim_files[PZSCHRECK][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[PZSCHRECK][IDLE] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_neutral";
	arms_anim_files[PZSCHRECK][KNEEL] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_neutral";
	arms_anim_files[PZSCHRECK][AIM] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_aim";
	arms_anim_files[PZSCHRECK][TROT] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_forward";
	arms_anim_files[PZSCHRECK][CREEP] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_forward";
	arms_anim_files[PZSCHRECK][CRAWL] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_forward";
	arms_anim_files[PZSCHRECK][SPRINT] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_sprint";
	arms_anim_files[PZSCHRECK][SIT] = FPS_DIR "panzerschreck\\animations\\arms_panzerschreck_neutral";

	// PIAT temp animation files
	arms_anim_files[PIAT][RELOAD] = FPS_DIR "piat\\animations\\reload\\arms_piat_reload(72)";
		weap_anim_files[PIAT][RELOAD] = FPS_DIR "piat\\animations\\reload\\weap_piat_reload(72)";
	arms_anim_files[PIAT][STAND_FIRE] = FPS_DIR "piat\\animations\\fire\\arms_piat_fire(3)";
		weap_anim_files[PIAT][STAND_FIRE] = FPS_DIR "piat\\animations\\fire\\weap_piat_fire(3)";
	arms_anim_files[PIAT][CROUCH_FIRE] = FPS_DIR "piat\\animations\\fire\\arms_piat_fire(3)";
		weap_anim_files[PIAT][CROUCH_FIRE] = FPS_DIR "piat\\animations\\fire\\weap_piat_fire(3)";
	arms_anim_files[PIAT][AIM_FIRE] = FPS_DIR "piat\\animations\\fire\\arms_piat_fire(3)";
		weap_anim_files[PIAT][AIM_FIRE] = FPS_DIR "piat\\animations\\fire\\weap_piat_fire(3)";
	arms_anim_files[PIAT][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[PIAT][IDLE] = FPS_DIR "piat\\animations\\arms_piat_neutral";
	arms_anim_files[PIAT][PRONE] = FPS_DIR "piat\\animations\\arms_piat_neutral";
	arms_anim_files[PIAT][KNEEL] = FPS_DIR "piat\\animations\\arms_piat_neutral";
	arms_anim_files[PIAT][AIM] = FPS_DIR "piat\\animations\\arms_piat_aim";
	arms_anim_files[PIAT][TROT] = FPS_DIR "piat\\animations\\arms_piat_forward";
	arms_anim_files[PIAT][CREEP] = FPS_DIR "piat\\animations\\arms_piat_forward";
	arms_anim_files[PIAT][CRAWL] = FPS_DIR "piat\\animations\\arms_piat_forward";
	arms_anim_files[PIAT][SPRINT] = FPS_DIR "piat\\animations\\arms_piat_forward";
	arms_anim_files[PIAT][SIT] = FPS_DIR "piat\\animations\\arms_piat_neutral";

	// Endfeld Mark 4 Snipper
	arms_anim_files[ENFIELDMK4][RELOAD] = FPS_DIR "enfield\\animations\\reload\\armsmesh-enfield-reload(115)";
		weap_anim_files[ENFIELDMK4][RELOAD] = FPS_DIR "enfield\\animations\\reload\\enfield-reload(115)";
	arms_anim_files[ENFIELDMK4][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\armsmesh-enfield-recycle(30)";
		weap_anim_files[ENFIELDMK4][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\enfield-recycle(30)";
	arms_anim_files[ENFIELDMK4][STAND_FIRE] = FPS_DIR "enfield\\animations\\fire\\arms-enfield-fire(3)";
	arms_anim_files[ENFIELDMK4][CROUCH_FIRE] = FPS_DIR "enfield\\animations\\fire\\crouch-enfield-fire(3)";
    arms_anim_files[ENFIELDMK4][AIM_FIRE] = FPS_DIR "enfield\\animations\\fire\\aim-enfield-fire(3)";
	arms_anim_files[ENFIELDMK4][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[ENFIELDMK4][IDLE] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[ENFIELDMK4][KNEEL] = FPS_DIR "enfield\\animations\\creep-enfield(1)";
	arms_anim_files[ENFIELDMK4][AIM] = FPS_DIR "enfield\\animations\\enfield-stand-aim(1)";
	arms_anim_files[ENFIELDMK4][TROT] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[ENFIELDMK4][CREEP] = FPS_DIR "enfield\\animations\\creep-enfield(1)";
	arms_anim_files[ENFIELDMK4][CRAWL] = FPS_DIR "enfield\\animations\\crawl-enfield(30)";
	arms_anim_files[ENFIELDMK4][SPRINT] = FPS_DIR "enfield\\animations\\sprint-enfield(15)";
    arms_anim_files[ENFIELDMK4][SIT] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";

	weap_empty_default[M1GARAND] = FPS_DIR "garand\\animations\\weap_garand_empty(1)";
		weap_anim_files[M1GARAND][RELOAD2] = FPS_DIR "garand\\animations\\fire\\weap_garand_empty(3)";
	arms_anim_files[M1GARAND][RELOAD] = FPS_DIR "garand\\animations\\reload\\arms_garand_reload(140)";
		weap_anim_files[M1GARAND][RELOAD] = FPS_DIR "garand\\animations\\reload\\weap_garand_reload(140)";
	arms_anim_files[M1GARAND][STAND_FIRE] = FPS_DIR "garand\\animations\\fire\\arms_garand_fire(3)";
		weap_anim_files[M1GARAND][STAND_FIRE] = FPS_DIR "garand\\animations\\fire\\weap_garand_fire(3)";
	arms_anim_files[M1GARAND][CROUCH_FIRE] = FPS_DIR "garand\\animations\\fire\\arms_garand_fire(3)";
		weap_anim_files[M1GARAND][CROUCH_FIRE] = FPS_DIR "garand\\animations\\fire\\weap_garand_fire(3)";
	arms_anim_files[M1GARAND][AIM_FIRE] = FPS_DIR "garand\\animations\\fire\\arms_garand_fire_aim(3)";
		weap_anim_files[M1GARAND][AIM_FIRE] = FPS_DIR "garand\\animations\\fire\\weap_garand_fire(3)";
	arms_anim_files[M1GARAND][LOWER] = FPS_DIR "garand\\animations\\arms_garand_lowered(1)";
	arms_anim_files[M1GARAND][IDLE] = FPS_DIR "garand\\animations\\arms_garand_neutral(1)";
	arms_anim_files[M1GARAND][KNEEL] = FPS_DIR "garand\\animations\\arms_garand_neutral(1)";
	arms_anim_files[M1GARAND][AIM] = FPS_DIR "garand\\animations\\arms_garand_aim(1)";
	arms_anim_files[M1GARAND][TROT] = FPS_DIR "garand\\animations\\arms_garand_neutral(1)";
	arms_anim_files[M1GARAND][CREEP] = FPS_DIR "garand\\animations\\arms_garand_neutral(1)";
	arms_anim_files[M1GARAND][CRAWL] = FPS_DIR "garand\\animations\\arms_garand_crawl(30)";
	arms_anim_files[M1GARAND][SPRINT] = FPS_DIR "garand\\animations\\arms_garand_sprint(15)";
    arms_anim_files[M1GARAND][SIT] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";

	arms_anim_files[SPRINGFIELD1903][RELOAD] = FPS_DIR "springfield\\animations\\reload\\arms_springfield_reload1(30)";
	arms_anim_files[SPRINGFIELD1903][RELOAD2] = FPS_DIR "springfield\\animations\\reload\\arms_springfield_reload2(30)";
	arms_anim_files[SPRINGFIELD1903][RELOAD3] = FPS_DIR "springfield\\animations\\reload\\arms_springfield_reload3(45)";
		weap_anim_files[SPRINGFIELD1903][RELOAD] = FPS_DIR "springfield\\animations\\reload\\weap_springfield_reload_1(30)";
		weap_anim_files[SPRINGFIELD1903][RELOAD2] = FPS_DIR "springfield\\animations\\reload\\weap_springfield_reload_2(30)";
		weap_anim_files[SPRINGFIELD1903][RELOAD3] = FPS_DIR "springfield\\animations\\reload\\weap_springfield_reload_3(45)";
	arms_anim_files[SPRINGFIELD1903][RECYCLE] = FPS_DIR "springfield\\animations\\recycle\\arms_springfield_recycle(60)";
		weap_anim_files[SPRINGFIELD1903][RECYCLE] = FPS_DIR "springfield\\animations\\recycle\\weap_springfield_recycle(60)";
	arms_anim_files[SPRINGFIELD1903][STAND_FIRE] = FPS_DIR "springfield\\animations\\fire\\arms_springfield_fire(3)";
	arms_anim_files[SPRINGFIELD1903][CROUCH_FIRE] = FPS_DIR "springfield\\animations\\fire\\arms_springfield_crouch(3)";
    arms_anim_files[SPRINGFIELD1903][AIM_FIRE] = FPS_DIR "springfield\\animations\\fire\\arms_springfield_fire_aim";
	arms_anim_files[SPRINGFIELD1903][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[SPRINGFIELD1903][IDLE] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[SPRINGFIELD1903][KNEEL] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[SPRINGFIELD1903][AIM] = FPS_DIR "springfield\\animations\\arms_springfield_aim(1)";
	arms_anim_files[SPRINGFIELD1903][TROT] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[SPRINGFIELD1903][CREEP] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[SPRINGFIELD1903][CRAWL] = FPS_DIR "springfield\\animations\\arms_springfield_crawl(30)";
	arms_anim_files[SPRINGFIELD1903][SPRINT] = FPS_DIR "springfield\\animations\\arms_springfield_sprint(15)";
    arms_anim_files[SPRINGFIELD1903][SIT] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";

	arms_anim_files[M1903A3][RELOAD] = FPS_DIR "springfield\\animations\\reload\\arms_springfield_reload_clip(185)";
		weap_anim_files[M1903A3][RELOAD] = FPS_DIR "springfield\\animations\\reload\\weap_springfield_reload_clip(185)";
	arms_anim_files[M1903A3][RECYCLE] = FPS_DIR "springfield\\animations\\recycle\\arms_springfield_recycle(60)";
		weap_anim_files[M1903A3][RECYCLE] = FPS_DIR "springfield\\animations\\recycle\\weap_springfield_recycle(60)";
	arms_anim_files[M1903A3][STAND_FIRE] = FPS_DIR "springfield\\animations\\fire\\arms_springfield_fire(3)";
	arms_anim_files[M1903A3][CROUCH_FIRE] = FPS_DIR "springfield\\animations\\fire\\arms_springfield_crouch(3)";
    arms_anim_files[M1903A3][AIM_FIRE] = FPS_DIR "springfield\\animations\\fire\\arms_springfield_fire_aim";
	arms_anim_files[M1903A3][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[M1903A3][IDLE] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[M1903A3][KNEEL] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[M1903A3][AIM] = FPS_DIR "springfield\\animations\\arms_springfield_aim(1)";
	arms_anim_files[M1903A3][TROT] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[M1903A3][CREEP] = FPS_DIR "springfield\\animations\\arms_springfield_neutral(1)";
	arms_anim_files[M1903A3][CRAWL] = FPS_DIR "springfield\\animations\\arms_springfield_crawl(30)";
	arms_anim_files[M1903A3][SPRINT] = FPS_DIR "springfield\\animations\\arms_springfield_sprint(15)";
    arms_anim_files[M1903A3][SIT] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";

	// LEBEL animation files
	weap_default[LEBEL] = FPS_DIR "lebel\\gunbone-lebel";
	clip_default[LEBEL] = FPS_DIR "lebel\\lebel-gunbone-5roundstripperclip";
	arms_anim_files[LEBEL][RELOAD] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload(75)";
	weap_anim_files[LEBEL][RELOAD] = FPS_DIR "lebel\\animations\\reload\\lebel-reload(75)";
	clip_anim_files[LEBEL][RELOAD] = FPS_DIR "lebel\\animations\\reload\\lebel-clip-reload(75)";
	arms_anim_files[LEBEL][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\armsmesh-lebel-recycle(60)";
	weap_anim_files[LEBEL][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\lebel-recycle(60)";
	clip_anim_files[LEBEL][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\lebel-clip-recycle(60)";
	arms_anim_files[LEBEL][STAND_FIRE] = FPS_DIR "lebel\\animations\\fire\\arms-lebel-fire(3)";
	weap_anim_files[LEBEL][STAND_FIRE] = FPS_DIR "lebel\\animations\\fire\\lebel-fire(3)";
	arms_anim_files[LEBEL][CROUCH_FIRE] = FPS_DIR "lebel\\animations\\fire\\crouch-lebel-fire(3)";
	weap_anim_files[LEBEL][CROUCH_FIRE] = FPS_DIR "lebel\\animations\\fire\\lebel-fire(3)";
	arms_anim_files[LEBEL][AIM_FIRE] = FPS_DIR "lebel\\animations\\fire\\aim-lebel-fire(3)";
	weap_anim_files[LEBEL][AIM_FIRE] = FPS_DIR "lebel\\animations\\fire\\lebel-fire(3)";
	arms_anim_files[LEBEL][LOWER] = FPS_DIR "lebel\\animations\\lebel-lowered(1)";
	arms_anim_files[LEBEL][IDLE] = FPS_DIR "lebel\\animations\\lebel-stand(1)";
	arms_anim_files[LEBEL][KNEEL] = FPS_DIR "lebel\\animations\\creep-lebel(1)";
	arms_anim_files[LEBEL][AIM] = FPS_DIR "lebel\\animations\\lebel-stand-aim(1)";
	arms_anim_files[LEBEL][TROT] = FPS_DIR "lebel\\animations\\lebel-stand(1)";
	arms_anim_files[LEBEL][CREEP] = FPS_DIR "lebel\\animations\\creep-lebel(1)";
	arms_anim_files[LEBEL][CRAWL] = FPS_DIR "lebel\\animations\\crawl-lebel(30)";
	arms_anim_files[LEBEL][SPRINT] = FPS_DIR "lebel\\animations\\sprint-lebel(15)";
	arms_anim_files[LEBEL][SIT] = FPS_DIR "lebel\\animations\\lebel-lowered(1)";

	// MAS36 animation files
	arms_anim_files[MAS36][RELOAD] = FPS_DIR "mas36\\animations\\reload\\armsmesh-mas36-reload";
	weap_anim_files[MAS36][RELOAD] = FPS_DIR "mas36\\animations\\reload\\gunmesh-mas36-reload";
	arms_anim_files[MAS36][RECYCLE] = FPS_DIR "mas36\\animations\\recycle\\armsmesh-mas36-recycle";
	weap_anim_files[MAS36][RECYCLE] = FPS_DIR "mas36\\animations\\recycle\\gunmesh-mas36-recycle";
	arms_anim_files[MAS36][STAND_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire";
	arms_anim_files[MAS36][CROUCH_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire-creep";
	arms_anim_files[MAS36][AIM_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-aim-fire";
	arms_anim_files[MAS36][LOWER] = FPS_DIR "mas36\\animations\\lowered";
	arms_anim_files[MAS36][IDLE] = FPS_DIR "mas36\\animations\\neutral";
	arms_anim_files[MAS36][KNEEL] = FPS_DIR "mas36\\animations\\creep";
	arms_anim_files[MAS36][AIM] = FPS_DIR "mas36\\animations\\stand-aim";
	arms_anim_files[MAS36][TROT] = FPS_DIR "mas36\\animations\\neutral";
	arms_anim_files[MAS36][CREEP] = FPS_DIR "mas36\\animations\\creep";
	arms_anim_files[MAS36][CRAWL] = FPS_DIR "mas36\\animations\\crawl";
	arms_anim_files[MAS36][SPRINT] = FPS_DIR "mas36\\animations\\sprint";
	arms_anim_files[MAS36][SIT] = FPS_DIR "mas36\\animations\\lowered";

	arms_anim_files[MAS40][RELOAD]      = FPS_DIR "mas40\\animations\\reload\\arms_mas40_reloading";
	weap_anim_files[MAS40][RELOAD]      = FPS_DIR "mas40\\animations\\reload\\weap_mas40_reloading";
	arms_anim_files[MAS40][STAND_FIRE]  = FPS_DIR "mas40\\animations\\fire\\arms_mas40_fire";
	weap_anim_files[MAS40][STAND_FIRE]  = FPS_DIR "mas40\\animations\\fire\\weap_mas40_fire";
	arms_anim_files[MAS40][CROUCH_FIRE] = FPS_DIR "mas40\\animations\\fire\\arms_mas40_fire";
	weap_anim_files[MAS40][CROUCH_FIRE] = FPS_DIR "mas40\\animations\\fire\\weap_mas40_fire";
	arms_anim_files[MAS40][AIM_FIRE]    = FPS_DIR "mas40\\animations\\fire\\arms_mas40_fire_aim";
	weap_anim_files[MAS40][AIM_FIRE]    = FPS_DIR "mas40\\animations\\fire\\weap_mas40_fire";
	arms_anim_files[MAS40][LOWER]       = FPS_DIR "mas40\\animations\\arms_mas40_lowered";
	arms_anim_files[MAS40][IDLE]        = FPS_DIR "mas40\\animations\\arms_mas40_neutral";
	arms_anim_files[MAS40][KNEEL]       = FPS_DIR "mas40\\animations\\arms_mas40_neutral";
	arms_anim_files[MAS40][AIM]         = FPS_DIR "mas40\\animations\\arms_mas40_aim";
	arms_anim_files[MAS40][TROT]        = FPS_DIR "mas40\\animations\\arms_mas40_neutral";
	arms_anim_files[MAS40][CREEP]       = FPS_DIR "mas40\\animations\\arms_mas40_neutral";
	arms_anim_files[MAS40][CRAWL]       = FPS_DIR "mas40\\animations\\arms_mas40_crawl";
	arms_anim_files[MAS40][SPRINT]      = FPS_DIR "mas40\\animations\\arms_mas40_sprint";
	arms_anim_files[MAS40][SIT]         = FPS_DIR "mas40\\animations\\arms_mas40_lowered";

	// M1935A animation files
	arms_anim_files[M1935A][RELOAD] = FPS_DIR "m1935a\\animations\\reload\\armsmesh-m1935a-reload(65)";
	weap_anim_files[M1935A][RELOAD] = FPS_DIR "m1935a\\animations\\reload\\m1935a-reload(65)";
	arms_anim_files[M1935A][STAND_FIRE] = FPS_DIR "m1935a\\animations\\fire\\arms-m1935a-fire(3)";
	weap_anim_files[M1935A][STAND_FIRE] = FPS_DIR "m1935a\\animations\\fire\\m1935a-fire(3)";
	arms_anim_files[M1935A][CROUCH_FIRE] = FPS_DIR "m1935a\\animations\\fire\\arms-m1935a-fire(3)";
	weap_anim_files[M1935A][CROUCH_FIRE] = FPS_DIR "m1935a\\animations\\fire\\m1935a-fire(3)";
	arms_anim_files[M1935A][AIM_FIRE] = FPS_DIR "m1935a\\animations\\fire\\aim-m1935a-fire(3)";
	weap_anim_files[M1935A][AIM_FIRE] = FPS_DIR "m1935a\\animations\\fire\\m1935a-fire(3)";
    arms_anim_files[M1935A][PRONE_FIRE] = FPS_DIR "m1935a\\animations\\fire\\aim-m1935a-fire(3)";
	weap_anim_files[M1935A][PRONE_FIRE] = FPS_DIR "m1935a\\animations\\fire\\m1935a-fire(3)";
	arms_anim_files[M1935A][LOWER] = FPS_DIR "m1935a\\animations\\m1935a-lowered(1)";
	arms_anim_files[M1935A][IDLE] = FPS_DIR "m1935a\\animations\\m1935a-stand(1)";
	arms_anim_files[M1935A][KNEEL] = FPS_DIR "m1935a\\animations\\m1935a-stand(1)";
	arms_anim_files[M1935A][AIM] = FPS_DIR "m1935a\\animations\\m1935a-stand-aim(1)";
	//arms_anim_files[M1935A][PRONE] = FPS_DIR "m1935a\\animations\\prone-m1935a(1)";
	arms_anim_files[M1935A][PRONE] = FPS_DIR "m1935a\\animations\\m1935a-stand-aim(1)";
	arms_anim_files[M1935A][TROT] = FPS_DIR "m1935a\\animations\\m1935a-stand(1)";
	arms_anim_files[M1935A][CREEP] = FPS_DIR "m1935a\\animations\\m1935a-stand(1)";
	arms_anim_files[M1935A][CRAWL] = FPS_DIR "m1935a\\animations\\crawl-m1935a(30)";
	arms_anim_files[M1935A][SPRINT] = FPS_DIR "m1935a\\animations\\sprint-m1935a(15)";
	arms_anim_files[M1935A][SIT] = FPS_DIR "m1935a\\animations\\m1935a-lowered(1)";

	weap_empty_default[COLT1911] = FPS_DIR "colt\\animations\\fire\\weap_colt_empty(1)";
	arms_anim_files[COLT1911][RELOAD] = FPS_DIR "colt\\animations\\reload\\arms_colt_reload(65)";
	weap_anim_files[COLT1911][RELOAD] = FPS_DIR "colt\\animations\\reload\\weap_colt_reload(65)";

	arms_anim_files[COLT1911][STAND_FIRE] = FPS_DIR "colt\\animations\\fire\\arms_colt_fire(3)";
	weap_anim_files[COLT1911][STAND_FIRE] = FPS_DIR "colt\\animations\\fire\\weap_colt_fire(3)";

	arms_anim_files[COLT1911][CROUCH_FIRE] = FPS_DIR "colt\\animations\\fire\\arms_colt_fire(3)";
	weap_anim_files[COLT1911][CROUCH_FIRE] = FPS_DIR "colt\\animations\\fire\\weap_colt_fire(3)";

	arms_anim_files[COLT1911][AIM_FIRE] = FPS_DIR "colt\\animations\\arms_colt_aim(1)";
	weap_anim_files[COLT1911][AIM_FIRE] = FPS_DIR "colt\\animations\\fire\\weap_colt_fire(3)";

    arms_anim_files[COLT1911][PRONE_FIRE] = FPS_DIR "colt\\animations\\fire\\arms_colt_aim(1)";
	weap_anim_files[COLT1911][PRONE_FIRE] = FPS_DIR "colt\\animations\\fire\\weap_colt_fire(3)";

	arms_anim_files[COLT1911][LOWER] = FPS_DIR "colt\\animations\\arms_colt_lowered(1)";
	arms_anim_files[COLT1911][IDLE] = FPS_DIR "colt\\animations\\arms_colt_stand(1)";
	arms_anim_files[COLT1911][KNEEL] = FPS_DIR "colt\\animations\\arms_colt_stand(1)";
	arms_anim_files[COLT1911][AIM] = FPS_DIR "colt\\animations\\arms_colt_aim(1)";
	//arms_anim_files[COLT1911][PRONE] = FPS_DIR "m1935a\\animations\\prone-m1935a(1)";
	arms_anim_files[COLT1911][PRONE] = FPS_DIR "colt\\animations\\arms_colt_aim(1)";
	arms_anim_files[COLT1911][TROT] = FPS_DIR "colt\\animations\\arms_colt_stand(1)";
	arms_anim_files[COLT1911][CREEP] = FPS_DIR "colt\\animations\\arms_colt_stand(1)";
	arms_anim_files[COLT1911][CRAWL] = FPS_DIR "colt\\animations\\arms_colt_crawl(30)";
	arms_anim_files[COLT1911][SPRINT] = FPS_DIR "colt\\animations\\arms_colt_sprint(15)";
	arms_anim_files[COLT1911][SIT] = FPS_DIR "colt\\animations\\arms_colt_sit(1)";

	// LUGER animation files
	weap_empty_default[LUGER] = FPS_DIR "luger\\animations\\fire\\luger-empty(1)";
	arms_anim_files[LUGER][RELOAD] = FPS_DIR "luger\\animations\\reload\\armsmesh-luger-reload(65)";
	weap_anim_files[LUGER][RELOAD] = FPS_DIR "luger\\animations\\reload\\luger-reload(65)";
	arms_anim_files[LUGER][STAND_FIRE] = FPS_DIR "luger\\animations\\fire\\arms-luger-fire(3)";
	weap_anim_files[LUGER][STAND_FIRE] = FPS_DIR "luger\\animations\\fire\\luger-fire(3)";
	arms_anim_files[LUGER][CROUCH_FIRE] = FPS_DIR "luger\\animations\\fire\\arms-luger-fire(3)";
	weap_anim_files[LUGER][CROUCH_FIRE] = FPS_DIR "luger\\animations\\fire\\luger-fire(3)";
    arms_anim_files[LUGER][AIM_FIRE] = FPS_DIR "luger\\animations\\fire\\arms-prone-luger-fire(3)";
	weap_anim_files[LUGER][AIM_FIRE] = FPS_DIR "luger\\animations\\fire\\luger-fire(3)";
	arms_anim_files[LUGER][PRONE_FIRE] = FPS_DIR "luger\\animations\\fire\\arms-prone-luger-fire(3)";
	weap_anim_files[LUGER][PRONE_FIRE] = FPS_DIR "luger\\animations\\fire\\luger-fire(3)";
	arms_anim_files[LUGER][LOWER] = FPS_DIR "luger\\animations\\luger-lowered(1)";
	arms_anim_files[LUGER][IDLE] = FPS_DIR "luger\\animations\\luger-stand(1)";
	arms_anim_files[LUGER][KNEEL] = FPS_DIR "luger\\animations\\luger-stand(1)";
	arms_anim_files[LUGER][AIM] = FPS_DIR "luger\\animations\\luger-stand-aim(1)";
	arms_anim_files[LUGER][PRONE] = FPS_DIR "luger\\animations\\luger-stand-aim(1)";
	arms_anim_files[LUGER][TROT] = FPS_DIR "luger\\animations\\luger-stand(1)";
	arms_anim_files[LUGER][CREEP] = FPS_DIR "luger\\animations\\luger-stand(1)";
	arms_anim_files[LUGER][CRAWL] = FPS_DIR "luger\\animations\\crawl-luger(30)";
	arms_anim_files[LUGER][SPRINT] = FPS_DIR "luger\\animations\\sprint-luger(15)";
	arms_anim_files[LUGER][SIT] = FPS_DIR "luger\\animations\\luger-lowered(1)";

	// MILLS36MK1 animation files
	arms_anim_files[MILLS36MK1][RELOAD] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[MILLS36MK1][STAND_FIRE] = FPS_DIR "mills36mk1\\animations\\fire\\charge-arms-mills36mk1(45)";
	weap_anim_files[MILLS36MK1][STAND_FIRE] = FPS_DIR "mills36mk1\\animations\\fire\\charge-mills36mk1(45)";
	arms_anim_files[MILLS36MK1][THROW] = FPS_DIR "mills36mk1\\animations\\fire\\fire-arms-mills36mk1(20)";
	arms_anim_files[MILLS36MK1][LOWER] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[MILLS36MK1][IDLE] = FPS_DIR "mills36mk1\\animations\\mills36mk1-stand(1)";
	arms_anim_files[MILLS36MK1][KNEEL] = FPS_DIR "mills36mk1\\animations\\mills36mk1-stand(1)";
	arms_anim_files[MILLS36MK1][AIM] = FPS_DIR "mills36mk1\\animations\\mills36mk1-stand(1)";
	arms_anim_files[MILLS36MK1][PRONE] = FPS_DIR "mills36mk1\\animations\\mills36mk1-stand(1)";
	arms_anim_files[MILLS36MK1][TROT] = FPS_DIR "mills36mk1\\animations\\mills36mk1-stand(1)";
	arms_anim_files[MILLS36MK1][CREEP] = FPS_DIR "mills36mk1\\animations\\mills36mk1-stand(1)";
	arms_anim_files[MILLS36MK1][CRAWL] = FPS_DIR "mills36mk1\\animations\\crawl-mills36mk1(30)";
	arms_anim_files[MILLS36MK1][SPRINT] = FPS_DIR "mills36mk1\\animations\\sprint-mills36mk1(15)";
	arms_anim_files[MILLS36MK1][SIT] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";

	// FRBANG animation files
	arms_anim_files[FRBANG][RELOAD] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[FRBANG][STAND_FIRE] = FPS_DIR "frenchgrenade\\animations\\fire\\charge-arms-mills36mk1(45)";
	weap_anim_files[FRBANG][STAND_FIRE] = FPS_DIR "frenchgrenade\\animations\\fire\\charge-mills36mk1(45)";
	arms_anim_files[FRBANG][THROW] = FPS_DIR "frenchgrenade\\animations\\fire\\fire-arms-mills36mk1(20)";
	arms_anim_files[FRBANG][LOWER] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[FRBANG][IDLE] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-stand(1)";
	arms_anim_files[FRBANG][KNEEL] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-stand(1)";
	arms_anim_files[FRBANG][AIM] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-stand(1)";
	arms_anim_files[FRBANG][PRONE] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-stand(1)";
	arms_anim_files[FRBANG][TROT] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-stand(1)";
	arms_anim_files[FRBANG][CREEP] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-stand(1)";
	arms_anim_files[FRBANG][CRAWL] = FPS_DIR "frenchgrenade\\animations\\crawl-mills36mk1(30)";
	arms_anim_files[FRBANG][SPRINT] = FPS_DIR "frenchgrenade\\animations\\sprint-mills36mk1(15)";
	arms_anim_files[FRBANG][SIT] = FPS_DIR "frenchgrenade\\animations\\mills36mk1-lowered(1)";

	// STIGR animation files
	//weap_default[STIGR] = FPS_DIR "stigr\\gunbone-stigr(1)";

	arms_anim_files[STIGR][RELOAD] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	arms_anim_files[STIGR][STAND_FIRE] = FPS_DIR "stigr\\animations\\fire\\charge-arms-stigr(55)";

	arms_anim_files[STIGR][THROW] = FPS_DIR "stigr\\animations\\fire\\fire-arms-stigr(20)";

	arms_anim_files[STIGR][LOWER] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	arms_anim_files[STIGR][IDLE] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[STIGR][KNEEL] = FPS_DIR "stigr\\animations\\stigr-stand(1)";

	arms_anim_files[STIGR][AIM] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[STIGR][PRONE] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[STIGR][TROT] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[STIGR][CREEP] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[STIGR][CRAWL] = FPS_DIR "stigr\\animations\\crawl-stigr(30)";
	arms_anim_files[STIGR][SPRINT] = FPS_DIR "stigr\\animations\\sprint-stigr(15)";
	//arms_anim_files[STIGR][SIT] = FPS_DIR "stigr\\animations\\stigr-arms-sit(1)";
	arms_anim_files[STIGR][SIT] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	// HGR39 animation files
	arms_anim_files[HGR39][RELOAD] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	arms_anim_files[HGR39][STAND_FIRE] = FPS_DIR "stigr\\animations\\fire\\charge-arms-stigr(55)";

	arms_anim_files[HGR39][THROW] = FPS_DIR "stigr\\animations\\fire\\fire-arms-stigr(20)";

	arms_anim_files[HGR39][LOWER] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	arms_anim_files[HGR39][IDLE] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[HGR39][KNEEL] = FPS_DIR "stigr\\animations\\stigr-stand(1)";

	arms_anim_files[HGR39][AIM] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[HGR39][PRONE] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[HGR39][TROT] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[HGR39][CREEP] = FPS_DIR "stigr\\animations\\stigr-stand(1)";
	arms_anim_files[HGR39][CRAWL] = FPS_DIR "stigr\\animations\\crawl-stigr(30)";
	arms_anim_files[HGR39][SPRINT] = FPS_DIR "stigr\\animations\\sprint-stigr(15)";
	//arms_anim_files[HGR39][SIT] = FPS_DIR "stigr\\animations\\stigr-arms-sit(1)";
	arms_anim_files[HGR39][SIT] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	// M18 animation files
	arms_anim_files[M18][RELOAD] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[M18][STAND_FIRE] = FPS_DIR "m18\\animations\\fire\\armsCharge";
	arms_anim_files[M18][THROW] = FPS_DIR "m18\\animations\\fire\\m18_armmesh_fire(20)";
	arms_anim_files[M18][LOWER] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[M18][IDLE] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[M18][KNEEL] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[M18][AIM] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[M18][PRONE] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[M18][TROT] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[M18][CREEP] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[M18][CRAWL] = FPS_DIR "m18\\animations\\m18_armmesh_crawl(30)";
	arms_anim_files[M18][SPRINT] = FPS_DIR "m18\\animations\\m18_armmesh_sprint(15)";
	arms_anim_files[M18][SIT] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";

	// MARK2 animation files
	arms_anim_files[MARK2][RELOAD] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[MARK2][STAND_FIRE] = FPS_DIR "mk2grenade\\animations\\fire\\arms_mk2_charge(30)";
		weap_anim_files[MARK2][STAND_FIRE] = FPS_DIR "mk2grenade\\animations\\fire\\weap_mk2_charge(30)";
	arms_anim_files[MARK2][THROW] = FPS_DIR "m18\\animations\\fire\\m18_armmesh_fire(20)";
	arms_anim_files[MARK2][LOWER] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[MARK2][IDLE] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[MARK2][KNEEL] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[MARK2][AIM] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[MARK2][PRONE] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[MARK2][TROT] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[MARK2][CREEP] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[MARK2][CRAWL] = FPS_DIR "m18\\animations\\m18_armmesh_crawl(30)";
	arms_anim_files[MARK2][SPRINT] = FPS_DIR "m18\\animations\\m18_armmesh_sprint(15)";
	arms_anim_files[MARK2][SIT] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";

	// FRSMOKE animation files
	arms_anim_files[FRSMOKE][RELOAD] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[FRSMOKE][STAND_FIRE] = FPS_DIR "frsmoke\\animations\\fire\\armsCharge";
	weap_anim_files[FRSMOKE][STAND_FIRE] = FPS_DIR "frsmoke\\animations\\fire\\weapCharge";
	arms_anim_files[FRSMOKE][THROW] = FPS_DIR "m18\\animations\\fire\\m18_armmesh_fire(20)";
	arms_anim_files[FRSMOKE][LOWER] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";
	arms_anim_files[FRSMOKE][IDLE] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[FRSMOKE][KNEEL] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[FRSMOKE][AIM] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[FRSMOKE][PRONE] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[FRSMOKE][TROT] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[FRSMOKE][CREEP] = FPS_DIR "m18\\animations\\m18_armmesh_neutral(1)";
	arms_anim_files[FRSMOKE][CRAWL] = FPS_DIR "m18\\animations\\m18_armmesh_crawl(30)";
	arms_anim_files[FRSMOKE][SPRINT] = FPS_DIR "m18\\animations\\m18_armmesh_sprint(15)";
	arms_anim_files[FRSMOKE][SIT] = FPS_DIR "mills36mk1\\animations\\mills36mk1-lowered(1)";

	// USSMOKE animation files
	arms_anim_files[USSMOKE][RELOAD] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_lowered(1)";
	arms_anim_files[USSMOKE][STAND_FIRE] = FPS_DIR "m8smoke\\animations\\fire\\arms_m8smoke_charge(45)";
		weap_anim_files[USSMOKE][STAND_FIRE] = FPS_DIR "m8smoke\\animations\\fire\\weap_m8smoke_charge(45)";
	arms_anim_files[USSMOKE][THROW] = FPS_DIR "m8smoke\\animations\\fire\\arms_m8smoke_fire(25)";
	arms_anim_files[USSMOKE][LOWER] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_lowered(1)";
	arms_anim_files[USSMOKE][IDLE] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_neutral(1)";
	arms_anim_files[USSMOKE][KNEEL] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_neutral(1)";
	arms_anim_files[USSMOKE][AIM] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_neutral(1)";
	arms_anim_files[USSMOKE][PRONE] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_neutral(1)";
	arms_anim_files[USSMOKE][TROT] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_neutral(1)";
	arms_anim_files[USSMOKE][CREEP] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_neutral(1)";
	arms_anim_files[USSMOKE][CRAWL] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_crawl(30)";
	arms_anim_files[USSMOKE][SPRINT] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_sprint(15)";
	arms_anim_files[USSMOKE][SIT] = FPS_DIR "m8smoke\\animations\\arms_m8smoke_lowered(1)";

	// ENFMK2 animation files
	arms_anim_files[WEBLEY][RELOAD] = FPS_DIR "enfmk2\\animations\\reload\\armsmesh-enfmk2-reload-1(20)";
	weap_anim_files[WEBLEY][RELOAD] = FPS_DIR "enfmk2\\animations\\reload\\enfmk2-reload-1(20)";
	arms_anim_files[WEBLEY][RELOAD2] = FPS_DIR "enfmk2\\animations\\reload\\armsmesh-enfmk2-reload-2(20)";
	weap_anim_files[WEBLEY][RELOAD2] = FPS_DIR "enfmk2\\animations\\reload\\enfmk2-reload-2(20)";
	arms_anim_files[WEBLEY][RELOAD3] = FPS_DIR "enfmk2\\animations\\reload\\armsmesh-enfmk2-reload-3(30)";
	weap_anim_files[WEBLEY][RELOAD3] = FPS_DIR "enfmk2\\animations\\reload\\enfmk2-reload-3(30)";
	arms_anim_files[WEBLEY][CROUCH_FIRE] = FPS_DIR "webley\\animations\\fire\\armsFire";
	weap_anim_files[WEBLEY][CROUCH_FIRE] = FPS_DIR "webley\\animations\\fire\\weapFire";
	arms_anim_files[WEBLEY][STAND_FIRE] = FPS_DIR "webley\\animations\\fire\\armsFire";
	weap_anim_files[WEBLEY][STAND_FIRE] = FPS_DIR "webley\\animations\\fire\\weapFire";
	arms_anim_files[WEBLEY][AIM_FIRE] = FPS_DIR "webley\\animations\\fire\\armsAimFire";
	weap_anim_files[WEBLEY][AIM_FIRE] = FPS_DIR "webley\\animations\\fire\\weapFire";
	arms_anim_files[WEBLEY][PRONE_FIRE] = FPS_DIR "webley\\animations\\fire\\armsAimFire";
	weap_anim_files[WEBLEY][PRONE_FIRE] = FPS_DIR "webley\\animations\\fire\\weapFire";
	arms_anim_files[WEBLEY][LOWER] = FPS_DIR "enfmk2\\animations\\enfmk2-lowered(1)";
	arms_anim_files[WEBLEY][IDLE] = FPS_DIR "enfmk2\\animations\\enfmk2-stand(1)";
	arms_anim_files[WEBLEY][KNEEL] = FPS_DIR "enfmk2\\animations\\enfmk2-stand(1)";
	arms_anim_files[WEBLEY][AIM] = FPS_DIR "webley\\animations\\fire\\armsAim";
	arms_anim_files[WEBLEY][PRONE] = FPS_DIR "enfmk2\\animations\\prone-enfmk2(1)";
	arms_anim_files[WEBLEY][TROT] = FPS_DIR "enfmk2\\animations\\enfmk2-stand(1)";
	arms_anim_files[WEBLEY][CREEP] = FPS_DIR "enfmk2\\animations\\creep-enfmk2(1)";
	arms_anim_files[WEBLEY][CRAWL] = FPS_DIR "enfmk2\\animations\\crawl-enfmk2(30)";
	arms_anim_files[WEBLEY][SPRINT] = FPS_DIR "enfmk2\\animations\\sprint-enfmk2(15)";
	arms_anim_files[WEBLEY][SIT] = FPS_DIR "enfmk2\\animations\\enfmk2-lowered(1)";

	// KNIFE_AXIS animation files
	arms_anim_files[KNIFE_AXIS][STAND_FIRE] = FPS_DIR "knives\\animations\\attack1(30)";
    arms_anim_files[KNIFE_AXIS][CROUCH_FIRE] = FPS_DIR "knives\\animations\\attack2(30)";
	arms_anim_files[KNIFE_AXIS][PRONE_FIRE] = FPS_DIR "knives\\animations\\attack3(30)";
	arms_anim_files[KNIFE_AXIS][RELOAD] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";
	arms_anim_files[KNIFE_AXIS][LOWER] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";
	arms_anim_files[KNIFE_AXIS][IDLE] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_AXIS][KNEEL] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_AXIS][TROT] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_AXIS][CREEP] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_AXIS][CRAWL] = FPS_DIR "stigr\\animations\\crawl-stigr(30)";
	arms_anim_files[KNIFE_AXIS][SPRINT] = FPS_DIR "stigr\\animations\\sprint-stigr(15)";
	arms_anim_files[KNIFE_AXIS][SIT] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	// KNIFE_ALLIED animation files
	arms_anim_files[KNIFE_ALLIED][STAND_FIRE] = FPS_DIR "knives\\animations\\attack1(30)";
	arms_anim_files[KNIFE_ALLIED][CROUCH_FIRE] = FPS_DIR "knives\\animations\\attack2(30)";
	arms_anim_files[KNIFE_ALLIED][PRONE_FIRE] = FPS_DIR "knives\\animations\\attack3(30)";
	arms_anim_files[KNIFE_ALLIED][RELOAD] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";
	arms_anim_files[KNIFE_ALLIED][LOWER] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";
	arms_anim_files[KNIFE_ALLIED][IDLE] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_ALLIED][KNEEL] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_ALLIED][TROT] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_ALLIED][CREEP] = FPS_DIR "knives\\animations\\knife-default(1)";
	arms_anim_files[KNIFE_ALLIED][CRAWL] = FPS_DIR "stigr\\animations\\crawl-stigr(30)";
	arms_anim_files[KNIFE_ALLIED][SPRINT] = FPS_DIR "stigr\\animations\\sprint-stigr(15)";
	arms_anim_files[KNIFE_ALLIED][SIT] = FPS_DIR "stigr\\animations\\stigr-lowered(1)";

	// BREN animation files
	arms_anim_files[BREN][RELOAD] = FPS_DIR "bren\\animations\\reload\\armsmesh-reload(65)";
	weap_anim_files[BREN][RELOAD] = FPS_DIR "bren\\animations\\reload\\gunmesh-reload(65)";
	arms_anim_files[BREN][STAND_FIRE] = FPS_DIR "bren\\animations\\fire\\armsmesh-bren-fire(3)";
	weap_anim_files[BREN][STAND_FIRE] = FPS_DIR "bren\\animations\\fire\\gun-fire";
	arms_anim_files[BREN][AIM_FIRE] = FPS_DIR "bren\\animations\\fire\\armsmesh-bren-fire(3)";
	weap_anim_files[BREN][AIM_FIRE] = FPS_DIR "bren\\animations\\fire\\gun-fire";
	arms_anim_files[BREN][CROUCH_FIRE] = FPS_DIR "bren\\animations\\fire\\armsmesh-bren-fire(3)";
	weap_anim_files[BREN][CROUCH_FIRE] = FPS_DIR "bren\\animations\\fire\\gun-fire";
	arms_anim_files[BREN][LOWER] = FPS_DIR "bren\\animations\\armmesh-lowered(1)";
	arms_anim_files[BREN][IDLE] = FPS_DIR "bren\\animations\\neutral";
	arms_anim_files[BREN][KNEEL] = FPS_DIR "bren\\animations\\neutral";
	arms_anim_files[BREN][AIM] = FPS_DIR "bren\\animations\\neutral";
	arms_anim_files[BREN][TROT] = FPS_DIR "bren\\animations\\neutral";
	arms_anim_files[BREN][CREEP] = FPS_DIR "bren\\animations\\neutral";
	arms_anim_files[BREN][CRAWL] = FPS_DIR "mp40\\animations\\crawl-mp40(30)";
	arms_anim_files[BREN][SPRINT] = FPS_DIR "bren\\animations\\sprint";
    arms_anim_files[BREN][SIT] = FPS_DIR "bren\\animations\\armmesh-lowered(1)";
	arms_anim_files[BREN][DEPLOYING] = FPS_DIR "bren\\animations\\deploy\\armsmesh-deploy";
	weap_anim_files[BREN][DEPLOYING] = FPS_DIR "bren\\animations\\deploy\\gunmesh-deploy";
	arms_anim_files[BREN][DEPLOYED] = FPS_DIR "bren\\animations\\deployed";
	weap_anim_files[BREN][DEPLOYED] = FPS_DIR "bren\\animations\\gunmesh-deployed";
	arms_anim_files[BREN][UNDEPLOYING] = FPS_DIR "bren\\animations\\deploy\\armsmesh-undeploy";
	weap_anim_files[BREN][UNDEPLOYING] = FPS_DIR "bren\\animations\\deploy\\gunmesh-undeploy";
	arms_anim_files[BREN][DEPLOYED_FIRE] = FPS_DIR "bren\\animations\\fire\\armsmesh-bren-fire-deployed(3)";
	weap_anim_files[BREN][DEPLOYED_FIRE] = FPS_DIR "bren\\animations\\fire\\gunmesh-bren-fire-deployed";
	arms_anim_files[BREN][DEPLOYED_RELOAD] = FPS_DIR "bren\\animations\\reload\\armsmesh-reload(65)";
	weap_anim_files[BREN][DEPLOYED_RELOAD] = FPS_DIR "bren\\animations\\reload\\gunmesh-reload-deployed";

	arms_anim_files[BAR][RELOAD] = FPS_DIR "bar\\animations\\reload\\arms_bar_reload(75)";
		weap_anim_files[BAR][RELOAD] = FPS_DIR "bar\\animations\\reload\\weap_bar_reload(75)";
	arms_anim_files[BAR][STAND_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire(3)";
		weap_anim_files[BAR][STAND_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire(3)";
	arms_anim_files[BAR][AIM_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire_aimed(3)";
		weap_anim_files[BAR][AIM_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire(3)";
	arms_anim_files[BAR][CROUCH_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire(3)";
		weap_anim_files[BAR][CROUCH_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire(3)";
	arms_anim_files[BAR][LOWER] = FPS_DIR "bar\\animations\\arms_bar_lowered(1)";
	arms_anim_files[BAR][IDLE] = FPS_DIR "bar\\animations\\arms_bar_stand(1)";
	arms_anim_files[BAR][KNEEL] = FPS_DIR "bar\\animations\\arms_bar_stand(1)";
	//arms_anim_files[BAR][AIM] = FPS_DIR "bar\\animations\\arms_bar_stand(1)";
	arms_anim_files[BAR][AIM] = FPS_DIR "bar\\animations\\arms_bar_aim(1)";
	arms_anim_files[BAR][TROT] = FPS_DIR "bar\\animations\\arms_bar_stand(1)";
	arms_anim_files[BAR][CREEP] = FPS_DIR "bar\\animations\\arms_bar_stand(1)";
	arms_anim_files[BAR][CRAWL] = FPS_DIR "mp40\\animations\\crawl-mp40(30)";
	arms_anim_files[BAR][SPRINT] = FPS_DIR "bar\\animations\\arms_bar_sprint(15)";
    arms_anim_files[BAR][SIT] = FPS_DIR "bren\\animations\\armmesh-lowered(1)";
	arms_anim_files[BAR][DEPLOYING] = FPS_DIR "bar\\animations\\deploy\\arms_bar_deploy(65)";
		weap_anim_files[BAR][DEPLOYING] = FPS_DIR "bar\\animations\\deploy\\weap_bar_deploy(65)";
	arms_anim_files[BAR][DEPLOYED] = FPS_DIR "bar\\animations\\arms_bar_deployed(1)";
		weap_anim_files[BAR][DEPLOYED] = FPS_DIR "bar\\animations\\weap_bar_deployed(1)";
	arms_anim_files[BAR][UNDEPLOYING] = FPS_DIR "bar\\animations\\deploy\\arms_bar_deploy(65)";
		weap_anim_files[BAR][UNDEPLOYING] = FPS_DIR "bar\\animations\\deploy\\weap_bar_deploy(65)";
	arms_anim_files[BAR][DEPLOYED_FIRE] = FPS_DIR "bar\\animations\\fire\\arms_bar_fire_deployed(3)";
		weap_anim_files[BAR][DEPLOYED_FIRE] = FPS_DIR "bar\\animations\\weap_bar_deployed(1)";
	arms_anim_files[BAR][DEPLOYED_RELOAD] = FPS_DIR "bar\\animations\\reload\\arms_bar_reload(75)";
		weap_anim_files[BAR][DEPLOYED_RELOAD] = FPS_DIR "bar\\animations\\reload\\weap_bar_reload_deployed(75)";

	// MG34 animation files
	arms_anim_files[MG34][RELOAD] = FPS_DIR "mg34\\animations\\reload\\armsmesh-reload(65)";
	weap_anim_files[MG34][RELOAD] = FPS_DIR "mg34\\animations\\reload\\gunmesh-reload(65)";
	arms_anim_files[MG34][STAND_FIRE] = FPS_DIR "mg34\\animations\\fire\\armsmesh-mg34-fire(3)";
	weap_anim_files[MG34][STAND_FIRE] = FPS_DIR "mg34\\animations\\fire\\fire(3)";
	arms_anim_files[MG34][AIM_FIRE] = FPS_DIR "mg34\\animations\\fire\\armsmesh-mg34-fire(3)";
	weap_anim_files[MG34][AIM_FIRE] = FPS_DIR "mg34\\animations\\fire\\fire(3)";
	arms_anim_files[MG34][CROUCH_FIRE] = FPS_DIR "mg34\\animations\\fire\\armsmesh-mg34-fire(3)";
	weap_anim_files[MG34][CROUCH_FIRE] = FPS_DIR "mg34\\animations\\fire\\fire(3)";
	arms_anim_files[MG34][LOWER] = FPS_DIR "mg34\\animations\\mg34_armmesh_lowered(1)";
	arms_anim_files[MG34][IDLE] = FPS_DIR "mg34\\animations\\neutral";
	arms_anim_files[MG34][KNEEL] = FPS_DIR "mg34\\animations\\neutral";
	arms_anim_files[MG34][AIM] = FPS_DIR "mg34\\animations\\neutral";
	arms_anim_files[MG34][TROT] = FPS_DIR "mg34\\animations\\neutral";
	arms_anim_files[MG34][CREEP] = FPS_DIR "mg34\\animations\\neutral";
	arms_anim_files[MG34][CRAWL] = FPS_DIR "mp40\\animations\\crawl-mp40(30)";
	arms_anim_files[MG34][SPRINT] = FPS_DIR "mg34\\animations\\mg34_armmesh_sprint(15)";
	arms_anim_files[MG34][SIT] = FPS_DIR "mg34\\animations\\mg34_armmesh_lowered(1)";
	arms_anim_files[MG34][DEPLOYING] = FPS_DIR "mg34\\animations\\deploy\\armsmesh-deploy";
	weap_anim_files[MG34][DEPLOYING] = FPS_DIR "mg34\\animations\\deploy\\gunmesh-deploy";
	arms_anim_files[MG34][DEPLOYED] = FPS_DIR "mg34\\animations\\deployed";
	weap_anim_files[MG34][DEPLOYED] = FPS_DIR "mg34\\animations\\gunmesh-deployed";
	arms_anim_files[MG34][UNDEPLOYING] = FPS_DIR "mg34\\animations\\deploy\\armsmesh-undeploy";
	weap_anim_files[MG34][UNDEPLOYING] = FPS_DIR "mg34\\animations\\deploy\\gunmesh-undeploy";
	arms_anim_files[MG34][DEPLOYED_FIRE] = FPS_DIR "mg34\\animations\\fire\\armsmesh-mg34-firedeployed(3)";
	weap_anim_files[MG34][DEPLOYED_FIRE] = FPS_DIR "mg34\\animations\\fire\\gunmesh-mg34-fire-deployed";
	arms_anim_files[MG34][DEPLOYED_RELOAD] = FPS_DIR "mg34\\animations\\reload\\armsmesh-reload(65)";
	weap_anim_files[MG34][DEPLOYED_RELOAD] = FPS_DIR "mg34\\animations\\reload\\gunmesh-reload-deployed";

	// FM29 animation files
	arms_anim_files[FM29][RELOAD] = FPS_DIR "fm29\\animations\\reload\\armsmesh-reload(65)";
	weap_anim_files[FM29][RELOAD] = FPS_DIR "fm29\\animations\\reload\\gunmesh-reload(65)";
	arms_anim_files[FM29][STAND_FIRE] = FPS_DIR "fm29\\animations\\fire\\armsmesh-chat-fire(3)";
	weap_anim_files[FM29][STAND_FIRE] = FPS_DIR "fm29\\animations\\fire\\gun-fire(3)";
	arms_anim_files[FM29][AIM_FIRE] = FPS_DIR "fm29\\animations\\fire\\armsmesh-chat-fire(3)";
	weap_anim_files[FM29][AIM_FIRE] = FPS_DIR "fm29\\animations\\fire\\gun-fire(3)";
	arms_anim_files[FM29][CROUCH_FIRE] = FPS_DIR "fm29\\animations\\fire\\armsmesh-chat-fire(3)";
	weap_anim_files[FM29][CROUCH_FIRE] = FPS_DIR "fm29\\animations\\fire\\gun-fire(3)";
	arms_anim_files[FM29][LOWER] = FPS_DIR "fm29\\animations\\chat_armmesh_lowered(1)";
	arms_anim_files[FM29][IDLE] = FPS_DIR "fm29\\animations\\neutral";
	arms_anim_files[FM29][KNEEL] = FPS_DIR "fm29\\animations\\neutral";
	arms_anim_files[FM29][AIM] = FPS_DIR "fm29\\animations\\neutral";
	arms_anim_files[FM29][TROT] = FPS_DIR "fm29\\animations\\neutral";
	arms_anim_files[FM29][CREEP] = FPS_DIR "fm29\\animations\\neutral";
	arms_anim_files[FM29][CRAWL] = FPS_DIR "mp40\\animations\\crawl-mp40(30)";
	arms_anim_files[FM29][SPRINT] = FPS_DIR "fm29\\animations\\chat_armmesh_sprint(15)";
	arms_anim_files[FM29][SIT] = FPS_DIR "fm29\\animations\\chat_armmesh_lowered(1)";
	arms_anim_files[FM29][DEPLOYING] = FPS_DIR "fm29\\animations\\deploy\\armsmesh-deploy";
	weap_anim_files[FM29][DEPLOYING] = FPS_DIR "fm29\\animations\\deploy\\gunmesh-deploy";
	arms_anim_files[FM29][DEPLOYED] = FPS_DIR "fm29\\animations\\deployed";
	weap_anim_files[FM29][DEPLOYED] = FPS_DIR "fm29\\animations\\gunmesh-deployed";
	arms_anim_files[FM29][UNDEPLOYING] = FPS_DIR "fm29\\animations\\deploy\\armsmesh-undeploy";
	weap_anim_files[FM29][UNDEPLOYING] = FPS_DIR "fm29\\animations\\deploy\\gunmesh-undeploy";
	arms_anim_files[FM29][DEPLOYED_FIRE] = FPS_DIR "fm29\\animations\\fire\\armsmesh-chat-fire-deployed(3)";
	weap_anim_files[FM29][DEPLOYED_FIRE] = FPS_DIR "fm29\\animations\\fire\\gunmesh-chat-fire-deployed";
	arms_anim_files[FM29][DEPLOYED_RELOAD] = FPS_DIR "fm29\\animations\\reload\\armsmesh-reload(65)";
	weap_anim_files[FM29][DEPLOYED_RELOAD] = FPS_DIR "fm29\\animations\\reload\\gunmesh-reload-deployed";

	// Light Mortars
	// UK
	arms_anim_files[LTMORTAR_UK][LOWER] = FPS_DIR "ltmortar_uk\\animations\\armsLowered";
	arms_anim_files[LTMORTAR_UK][IDLE] = FPS_DIR "ltmortar_uk\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_UK][KNEEL] = FPS_DIR "ltmortar_uk\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_UK][AIM] = FPS_DIR "ltmortar_uk\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_UK][TROT] = FPS_DIR "ltmortar_uk\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_UK][CREEP] = FPS_DIR "ltmortar_uk\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_UK][CRAWL] = FPS_DIR "ltmortar_uk\\animations\\armsCrawl";
	arms_anim_files[LTMORTAR_UK][PRONE] = FPS_DIR "ltmortar_uk\\animations\\armsProne";
	arms_anim_files[LTMORTAR_UK][SPRINT] = FPS_DIR "ltmortar_uk\\animations\\armsSprint";
    arms_anim_files[LTMORTAR_UK][SIT] = FPS_DIR "ltmortar_uk\\animations\\armsSit";
	arms_anim_files[LTMORTAR_UK][DEPLOYING] = FPS_DIR "ltmortar_uk\\animations\\armsDeploy";
	arms_anim_files[LTMORTAR_UK][DEPLOYED] = FPS_DIR "ltmortar_uk\\animations\\armsDeployed";
	arms_anim_files[LTMORTAR_UK][UNDEPLOYING] = FPS_DIR "ltmortar_uk\\animations\\armsUndeploy";
	arms_anim_files[LTMORTAR_UK][DEPLOYED_FIRE] = FPS_DIR "ltmortar_uk\\animations\\fire\\armsFire";
	weap_anim_files[LTMORTAR_UK][DEPLOYED_FIRE] = FPS_DIR "ltmortar_uk\\animations\\fire\\weapFire";

	// fr
	arms_anim_files[LTMORTAR_FR][LOWER] = FPS_DIR "ltmortar_fr\\animations\\armsLowered";
	arms_anim_files[LTMORTAR_FR][IDLE] = FPS_DIR "ltmortar_fr\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_FR][KNEEL] = FPS_DIR "ltmortar_fr\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_FR][AIM] = FPS_DIR "ltmortar_fr\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_FR][TROT] = FPS_DIR "ltmortar_fr\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_FR][CREEP] = FPS_DIR "ltmortar_fr\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_FR][CRAWL] = FPS_DIR "ltmortar_fr\\animations\\armsCrawl";
	arms_anim_files[LTMORTAR_FR][PRONE] = FPS_DIR "ltmortar_fr\\animations\\armsProne";
	arms_anim_files[LTMORTAR_FR][SPRINT] = FPS_DIR "ltmortar_fr\\animations\\armsSprint";
    arms_anim_files[LTMORTAR_FR][SIT] = FPS_DIR "ltmortar_fr\\animations\\armsSit";
	arms_anim_files[LTMORTAR_FR][DEPLOYING] = FPS_DIR "ltmortar_fr\\animations\\armsDeploy";
	arms_anim_files[LTMORTAR_FR][DEPLOYED] = FPS_DIR "ltmortar_fr\\animations\\armsDeployed";
	arms_anim_files[LTMORTAR_FR][UNDEPLOYING] = FPS_DIR "ltmortar_fr\\animations\\armsUndeploy";
	arms_anim_files[LTMORTAR_FR][DEPLOYED_FIRE] = FPS_DIR "ltmortar_fr\\animations\\fire\\armsFire";
	weap_anim_files[LTMORTAR_FR][DEPLOYED_FIRE] = FPS_DIR "ltmortar_fr\\animations\\fire\\weapFire";

	// de
	arms_anim_files[LTMORTAR_DE][LOWER] = FPS_DIR "ltmortar_de\\animations\\armsLowered";
	arms_anim_files[LTMORTAR_DE][IDLE] = FPS_DIR "ltmortar_de\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_DE][KNEEL] = FPS_DIR "ltmortar_de\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_DE][AIM] = FPS_DIR "ltmortar_de\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_DE][TROT] = FPS_DIR "ltmortar_de\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_DE][CREEP] = FPS_DIR "ltmortar_de\\animations\\armsNeutral";
	arms_anim_files[LTMORTAR_DE][CRAWL] = FPS_DIR "ltmortar_de\\animations\\armsCrawl";
	arms_anim_files[LTMORTAR_DE][PRONE] = FPS_DIR "ltmortar_de\\animations\\armsProne";
	arms_anim_files[LTMORTAR_DE][SPRINT] = FPS_DIR "ltmortar_de\\animations\\armsSprint";
    arms_anim_files[LTMORTAR_DE][SIT] = FPS_DIR "ltmortar_de\\animations\\armsSit";
	arms_anim_files[LTMORTAR_DE][DEPLOYING] = FPS_DIR "ltmortar_de\\animations\\armsDeploy";
	arms_anim_files[LTMORTAR_DE][DEPLOYED] = FPS_DIR "ltmortar_de\\animations\\armsDeployed";
	arms_anim_files[LTMORTAR_DE][UNDEPLOYING] = FPS_DIR "ltmortar_de\\animations\\armsUndeploy";
	arms_anim_files[LTMORTAR_DE][DEPLOYED_FIRE] = FPS_DIR "ltmortar_de\\animations\\fire\\armsFire";
	weap_anim_files[LTMORTAR_DE][DEPLOYED_FIRE] = FPS_DIR "ltmortar_de\\animations\\fire\\weapFire";

	// AMMO animation files
	arms_anim_files[AMMO][STAND_FIRE] = FPS_DIR "supbox\\animations\\fire\\supbox_armmesh_fire(3)";
	arms_anim_files[AMMO][RELOAD] = FPS_DIR "supbox\\animations\\supbox_armmesh_lowered(1)";
	arms_anim_files[AMMO][LOWER] = FPS_DIR "supbox\\animations\\supbox_armmesh_lowered(1)";
	arms_anim_files[AMMO][IDLE] = FPS_DIR "supbox\\animations\\supbox_armmesh_neutral(1)";
	arms_anim_files[AMMO][KNEEL] = FPS_DIR "supbox\\animations\\supbox_armmesh_neutral(1)";
	arms_anim_files[AMMO][TROT] = FPS_DIR "supbox\\animations\\supbox_armmesh_neutral(1)";
	arms_anim_files[AMMO][CREEP] = FPS_DIR "supbox\\animations\\supbox_armmesh_neutral(1)";
	arms_anim_files[AMMO][CRAWL] = FPS_DIR "supbox\\animations\\supbox_armmesh_crawl(30)";
	arms_anim_files[AMMO][SPRINT] = FPS_DIR "supbox\\animations\\supbox_armmesh_sprint(15)";
	arms_anim_files[AMMO][SIT] = FPS_DIR "supbox\\animations\\supbox_armmesh_lowered(1)";

	// RG_G30 (K98k based Rifle Grenade) animation files
	arms_anim_files[RG_G30][RELOAD] = FPS_DIR "kar98k\\animations\\reload\\armsmesh-kar98k-reload(75)";
	weap_anim_files[RG_G30][RELOAD] = FPS_DIR "kar98k\\animations\\reload\\kar98k-reload(75)";
	arms_anim_files[RG_G30][RECYCLE] = FPS_DIR "kar98k\\animations\\recycle\\armsmesh-kar98k-recycle(60)";
	weap_anim_files[RG_G30][RECYCLE] = FPS_DIR "kar98k\\animations\\recycle\\kar98k-recycle(60)";
	arms_anim_files[RG_G30][DEPLOYING] = FPS_DIR "g30\\animations\\armsmesh-g30-cupattach";
	weap_anim_files[RG_G30][DEPLOYING] = FPS_DIR "g30\\animations\\gunmesh-g30-cupattach";
	arms_anim_files[RG_G30][UNDEPLOYING] = FPS_DIR "g30\\animations\\armsmesh-g30-cupdetach";
	weap_anim_files[RG_G30][UNDEPLOYING] = FPS_DIR "g30\\animations\\gunmesh-g30-cupdetach";
	arms_anim_files[RG_G30][STAND_FIRE] = FPS_DIR "kar98k\\animations\\fire\\arms-kar98k-fire(3)";
	arms_anim_files[RG_G30][CROUCH_FIRE] = FPS_DIR "kar98k\\animations\\fire\\crouch-kar98k-fire(3)";
	arms_anim_files[RG_G30][AIM_FIRE] = FPS_DIR "kar98k\\animations\\fire\\aim-kar98k-fire(3)";
	arms_anim_files[RG_G30][IDLE] = FPS_DIR "kar98k\\animations\\kar98k-stand(1)";
	arms_anim_files[RG_G30][KNEEL] = FPS_DIR "kar98k\\animations\\creep-kar98k(1)";
	arms_anim_files[RG_G30][AIM] = FPS_DIR "kar98k\\animations\\kar98k-stand-aim(1)";
	arms_anim_files[RG_G30][TROT] = FPS_DIR "kar98k\\animations\\kar98k-stand(1)";
	arms_anim_files[RG_G30][CREEP] = FPS_DIR "kar98k\\animations\\creep-kar98k(1)";
	arms_anim_files[RG_G30][CRAWL] = FPS_DIR "kar98k\\animations\\crawl-kar98k(30)";
	arms_anim_files[RG_G30][SPRINT] = FPS_DIR "kar98k\\animations\\sprint-kar98k(15)";
	arms_anim_files[RG_G30][SIT] = FPS_DIR "kar98k\\animations\\kar98k-lowered(1)";
	arms_anim_files[RG_G30][LOWER] = FPS_DIR "kar98k\\animations\\kar98k-lowered(1)";
	arms_anim_files[RG_G30][DEPLOYED_FIRE] = FPS_DIR "kar98k\\animations\\fire\\arms-kar98k-fire(3)";
	arms_anim_files[RG_G30][DEPLOYED_RELOAD] = FPS_DIR "g30\\animations\\reload\\armsmesh-reload";
	weap_anim_files[RG_G30][DEPLOYED_RELOAD] = FPS_DIR "g30\\animations\\reload\\gunmesh-g30";

    // RG_MILLS (ENFIELD based british rifle grenade) animation files
	arms_anim_files[RG_MILLS][RELOAD] = FPS_DIR "enfield\\animations\\reload\\armsmesh-enfield-reload(115)";
	weap_anim_files[RG_MILLS][RELOAD] = FPS_DIR "enfield\\animations\\reload\\enfield-reload(115)";
	arms_anim_files[RG_MILLS][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\armsmesh-enfield-recycle(30)";
	weap_anim_files[RG_MILLS][RECYCLE] = FPS_DIR "enfield\\animations\\recycle\\enfield-recycle(30)";
	arms_anim_files[RG_MILLS][DEPLOYING] = FPS_DIR "mills\\animations\\armsmesh-mills-cupattach";
	weap_anim_files[RG_MILLS][DEPLOYING] = FPS_DIR "mills\\animations\\gunmesh-mills-cupattach";
	arms_anim_files[RG_MILLS][UNDEPLOYING] = FPS_DIR "mills\\animations\\armsmesh-mills-cupdetach";
	weap_anim_files[RG_MILLS][UNDEPLOYING] = FPS_DIR "mills\\animations\\gunmesh-mills-cupdetach";
	arms_anim_files[RG_MILLS][STAND_FIRE] = FPS_DIR "enfield\\animations\\fire\\arms-enfield-fire(3)";
	arms_anim_files[RG_MILLS][CROUCH_FIRE] = FPS_DIR "enfield\\animations\\fire\\crouch-enfield-fire(3)";
	arms_anim_files[RG_MILLS][AIM_FIRE] = FPS_DIR "enfield\\animations\\fire\\aim-enfield-fire(3)";
	arms_anim_files[RG_MILLS][LOWER] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[RG_MILLS][IDLE] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[RG_MILLS][KNEEL] = FPS_DIR "enfield\\animations\\creep-enfield(1)";
	arms_anim_files[RG_MILLS][DEPLOYED] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[RG_MILLS][AIM] = FPS_DIR "enfield\\animations\\enfield-stand-aim(1)";
	arms_anim_files[RG_MILLS][TROT] = FPS_DIR "enfield\\animations\\enfield-stand(1)";
	arms_anim_files[RG_MILLS][CREEP] = FPS_DIR "enfield\\animations\\creep-enfield(1)";
	arms_anim_files[RG_MILLS][CRAWL] = FPS_DIR "enfield\\animations\\crawl-enfield(30)";
	arms_anim_files[RG_MILLS][SPRINT] = FPS_DIR "enfield\\animations\\sprint-enfield(15)";
	arms_anim_files[RG_MILLS][SIT] = FPS_DIR "enfield\\animations\\enfield-lowered(1)";
	arms_anim_files[RG_MILLS][DEPLOYED_FIRE] = FPS_DIR "enfield\\animations\\fire\\arms-enfield-fire(3)";
	arms_anim_files[RG_MILLS][DEPLOYED_RELOAD] = FPS_DIR "mills\\animations\\reload\\armsmesh-reload";
	weap_anim_files[RG_MILLS][DEPLOYED_RELOAD] = FPS_DIR "mills\\animations\\reload\\gunmesh-reload";

	// LEBELMLE animation files
	arms_anim_files[LEBELMLE][RELOAD] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload1";
	arms_anim_files[LEBELMLE][RELOAD2] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload2";
	arms_anim_files[LEBELMLE][RELOAD3] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload3";
		weap_anim_files[LEBELMLE][RELOAD] = FPS_DIR "lebel\\animations\\reload\\lebel-reload1";
		weap_anim_files[LEBELMLE][RELOAD2] = FPS_DIR "lebel\\animations\\reload\\lebel-reload2";
		weap_anim_files[LEBELMLE][RELOAD3] = FPS_DIR "lebel\\animations\\reload\\lebel-reload3";
	arms_anim_files[LEBELMLE][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\armsmesh-lebel-recycle(60)";
		weap_anim_files[LEBELMLE][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\lebel-recycle(60)";
	arms_anim_files[LEBELMLE][STAND_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire";
	arms_anim_files[LEBELMLE][CROUCH_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire-creep";
	arms_anim_files[LEBELMLE][AIM_FIRE] = FPS_DIR "lebel\\animations\\fire\\aim-lebel-fire(3)";
	arms_anim_files[LEBELMLE][LOWER] = FPS_DIR "mas36\\animations\\lowered";
	arms_anim_files[LEBELMLE][IDLE] = FPS_DIR "mas36\\animations\\neutral";
	arms_anim_files[LEBELMLE][KNEEL] = FPS_DIR "mas36\\animations\\creep";
	arms_anim_files[LEBELMLE][AIM] = FPS_DIR "lebel\\animations\\lebel-stand-aim(1)";
	arms_anim_files[LEBELMLE][TROT] = FPS_DIR "mas36\\animations\\neutral";
	arms_anim_files[LEBELMLE][CREEP] = FPS_DIR "mas36\\animations\\creep";
	arms_anim_files[LEBELMLE][CRAWL] = FPS_DIR "mas36\\animations\\crawl";
	arms_anim_files[LEBELMLE][SPRINT] = FPS_DIR "mas36\\animations\\sprint";
	arms_anim_files[LEBELMLE][SIT] = FPS_DIR "mas36\\animations\\lowered";

	// RG_VB (MAS36 based french rifle grenade) animation files
	arms_anim_files[RG_VB][RELOAD] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload1";
	arms_anim_files[RG_VB][RELOAD2] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload2";
	arms_anim_files[RG_VB][RELOAD3] = FPS_DIR "lebel\\animations\\reload\\armsmesh-lebel-reload3";
	weap_anim_files[RG_VB][RELOAD] = FPS_DIR "lebel\\animations\\reload\\lebel-reload1";
	weap_anim_files[RG_VB][RELOAD2] = FPS_DIR "lebel\\animations\\reload\\lebel-reload2";
	weap_anim_files[RG_VB][RELOAD3] = FPS_DIR "lebel\\animations\\reload\\lebel-reload3";
	arms_anim_files[RG_VB][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\armsmesh-lebel-recycle(60)";
	weap_anim_files[RG_VB][RECYCLE] = FPS_DIR "lebel\\animations\\recycle\\lebel-recycle(60)";
	arms_anim_files[RG_VB][STAND_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire";
	arms_anim_files[RG_VB][CROUCH_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire-creep";
	arms_anim_files[RG_VB][AIM_FIRE] = FPS_DIR "lebel\\animations\\fire\\aim-lebel-fire(3)";
	arms_anim_files[RG_VB][LOWER] = FPS_DIR "mas36\\animations\\lowered";
	arms_anim_files[RG_VB][IDLE] = FPS_DIR "mas36\\animations\\neutral";
	arms_anim_files[RG_VB][KNEEL] = FPS_DIR "mas36\\animations\\creep";
	arms_anim_files[RG_VB][AIM] = FPS_DIR "lebel\\animations\\lebel-stand-aim(1)";
	arms_anim_files[RG_VB][TROT] = FPS_DIR "mas36\\animations\\neutral";
	arms_anim_files[RG_VB][CREEP] = FPS_DIR "mas36\\animations\\creep";
	arms_anim_files[RG_VB][CRAWL] = FPS_DIR "mas36\\animations\\crawl";
	arms_anim_files[RG_VB][SPRINT] = FPS_DIR "mas36\\animations\\sprint";
	arms_anim_files[RG_VB][SIT] = FPS_DIR "mas36\\animations\\lowered";
	arms_anim_files[RG_VB][DEPLOYING] = FPS_DIR "vb\\animations\\armsmesh-vb-cupattach";
	weap_anim_files[RG_VB][DEPLOYING] = FPS_DIR "vb\\animations\\gunmesh-vb-cupattach";
	arms_anim_files[RG_VB][UNDEPLOYING] = FPS_DIR "vb\\animations\\armsmesh-vb-cupdetach";
	weap_anim_files[RG_VB][UNDEPLOYING] = FPS_DIR "vb\\animations\\gunmesh-vb-cupdetach";
	arms_anim_files[RG_VB][DEPLOYED_FIRE] = FPS_DIR "mas36\\animations\\fire\\armsmesh-mas36-fire";
	arms_anim_files[RG_VB][DEPLOYED_RELOAD] = FPS_DIR "vb\\animations\\reload\\armsmesh-reload";
	weap_anim_files[RG_VB][DEPLOYED_RELOAD] = FPS_DIR "vb\\animations\\reload\\gunmesh-reload";

	// BOYSMK1 animation files
	arms_anim_files[BOYSMK1][RECYCLE] = FPS_DIR "boysmk1\\animations\\recycle\\armsmesh-recycle";
	weap_anim_files[BOYSMK1][RECYCLE] = FPS_DIR "boysmk1\\animations\\recycle\\gunmesh-recycle";

	arms_anim_files[BOYSMK1][LOWER] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";

	arms_anim_files[BOYSMK1][IDLE] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";
	arms_anim_files[BOYSMK1][KNEEL] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";

	arms_anim_files[BOYSMK1][AIM] = FPS_DIR "boysmk1\\animations\\prone\\armsmesh-neutral";
	arms_anim_files[BOYSMK1][TROT] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";
	arms_anim_files[BOYSMK1][CREEP] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";

	arms_anim_files[BOYSMK1][CRAWL] = FPS_DIR "boysmk1\\animations\\prone\\armsmesh-crawl";
	arms_anim_files[BOYSMK1][SPRINT] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";

	//arms_anim_files[BOYSMK1][SIT] = FPS_DIR "boysmk1\\animations\\armsmesh-sit";
	arms_anim_files[BOYSMK1][SIT] = FPS_DIR "boysmk1\\animations\\armsmesh-neutral";

	arms_anim_files[BOYSMK1][DEPLOYING] = FPS_DIR "boysmk1\\animations\\deploy\\armsmesh-deploy";
	weap_anim_files[BOYSMK1][DEPLOYING] = FPS_DIR "boysmk1\\animations\\deploy\\gunmesh-deploy";


	arms_anim_files[BOYSMK1][DEPLOYED] = FPS_DIR "boysmk1\\animations\\deployed";
	weap_anim_files[BOYSMK1][DEPLOYED] = FPS_DIR "boysmk1\\animations\\gunmesh-deployed";

	arms_anim_files[BOYSMK1][UNDEPLOYING] = FPS_DIR "boysmk1\\animations\\deploy\\armsmesh-undeploy";
	weap_anim_files[BOYSMK1][UNDEPLOYING] = FPS_DIR "boysmk1\\animations\\deploy\\gunmesh-undeploy";

	arms_anim_files[BOYSMK1][DEPLOYED_FIRE] = FPS_DIR "boysmk1\\animations\\fire\\armsmesh-boys-fire-deployed";
	weap_anim_files[BOYSMK1][DEPLOYED_FIRE] = FPS_DIR "boysmk1\\animations\\gunmesh-deployed";

	arms_anim_files[BOYSMK1][DEPLOYED_RELOAD] = FPS_DIR "boysmk1\\animations\\reload\\armsmesh-reload";
	weap_anim_files[BOYSMK1][DEPLOYED_RELOAD] = FPS_DIR "boysmk1\\animations\\reload\\gunmesh-reload";

	// PZB39 animation files
	arms_anim_files[PZB39][LOWER] = FPS_DIR "pzb39\\animations\\armsmesh-neutral";

	arms_anim_files[PZB39][IDLE] = FPS_DIR "pzb39\\animations\\armsmesh-neutral";
	arms_anim_files[PZB39][KNEEL] = FPS_DIR "pzb39\\animations\\armsmesh-neutral";

	arms_anim_files[PZB39][AIM] = FPS_DIR "pzb39\\animations\\prone\\armsmesh-neutral";
	arms_anim_files[PZB39][TROT] = FPS_DIR "pzb39\\animations\\armsmesh-neutral";
	arms_anim_files[PZB39][CREEP] = FPS_DIR "pzb39\\animations\\armsmesh-neutral";

	arms_anim_files[PZB39][CRAWL] = FPS_DIR "pzb39\\animations\\prone\\armsmesh-crawl";
	arms_anim_files[PZB39][SPRINT] = FPS_DIR "pzb39\\animations\\armmesh_sprint(15)";

	//arms_anim_files[PZB39][SIT] = FPS_DIR "pzb39\\animations\\armsmesh-sit";
	arms_anim_files[PZB39][SIT] = FPS_DIR "pzb39\\animations\\armsmesh-neutral";

	arms_anim_files[PZB39][DEPLOYING] = FPS_DIR "pzb39\\animations\\deploy\\armsmesh-deploy";
	weap_anim_files[PZB39][DEPLOYING] = FPS_DIR "pzb39\\animations\\deploy\\gunmesh-deploy";

	arms_anim_files[PZB39][DEPLOYED] = FPS_DIR "pzb39\\animations\\armsmesh-deployed";
	weap_anim_files[PZB39][DEPLOYED] = FPS_DIR "pzb39\\animations\\gunmesh-deployed";

	arms_anim_files[PZB39][UNDEPLOYING] = FPS_DIR "pzb39\\animations\\deploy\\armsmesh-undeploy";
	weap_anim_files[PZB39][UNDEPLOYING] = FPS_DIR "pzb39\\animations\\deploy\\gunmesh-undeploy";

	arms_anim_files[PZB39][DEPLOYED_FIRE] = FPS_DIR "pzb39\\animations\\fire\\armsmesh-fire";
	weap_anim_files[PZB39][DEPLOYED_FIRE] = FPS_DIR "pzb39\\animations\\gunmesh-deployed";

	arms_anim_files[PZB39][DEPLOYED_RELOAD] = FPS_DIR "pzb39\\animations\\reload\\armsmesh-reload";
	weap_anim_files[PZB39][DEPLOYED_RELOAD] = FPS_DIR "pzb39\\animations\\reload\\gunmesh-reload";

	arms_anim_files[PZB39][RECYCLE] = FPS_DIR "pzb39\\animations\\reload\\armsmesh-reload";
	weap_anim_files[PZB39][RECYCLE] = FPS_DIR "pzb39\\animations\\reload\\gunmesh-reload";

	weap_anim_files[STENMK2][RELOAD]      = FPS_DIR "stenmk2\\Weapon animations\\weap_sten_reloading";
	weap_anim_files[STENMK2][STAND_FIRE]  = FPS_DIR "stenmk2\\Weapon animations\\weap_sten_fire";
	weap_anim_files[STENMK2][CROUCH_FIRE] = FPS_DIR "stenmk2\\Weapon animations\\weap_sten_fire";
	weap_anim_files[STENMK2][AIM_FIRE]    = FPS_DIR "stenmk2\\Weapon animations\\weap_sten_fire";
	arms_anim_files[STENMK2][RELOAD]      = FPS_DIR "stenmk2\\Arms animations\\arms_sten_reloading";
	arms_anim_files[STENMK2][STAND_FIRE]  = FPS_DIR "stenmk2\\Arms animations\\arms_sten_fire";
	arms_anim_files[STENMK2][CROUCH_FIRE] = FPS_DIR "stenmk2\\Arms animations\\arms_sten_fire";
	arms_anim_files[STENMK2][AIM_FIRE]    = FPS_DIR "stenmk2\\Arms animations\\arms_sten_aimfire";
	arms_anim_files[STENMK2][LOWER]       = FPS_DIR "stenmk2\\Arms animations\\arms_sten_lowered";
	arms_anim_files[STENMK2][IDLE]        = FPS_DIR "stenmk2\\Arms animations\\arms_sten_neutral";
	arms_anim_files[STENMK2][KNEEL]       = FPS_DIR "stenmk2\\Arms animations\\arms_sten_neutral";
	arms_anim_files[STENMK2][AIM]         = FPS_DIR "stenmk2\\Arms animations\\arms_sten_aim";
	arms_anim_files[STENMK2][AIM_SMG]     = FPS_DIR "stenmk2\\Arms animations\\arms_sten_aim";
	arms_anim_files[STENMK2][TROT]        = FPS_DIR "stenmk2\\Arms animations\\arms_sten_neutral";
	arms_anim_files[STENMK2][CREEP]       = FPS_DIR "stenmk2\\Arms animations\\arms_sten_neutral";
	arms_anim_files[STENMK2][CRAWL]       = FPS_DIR "stenmk2\\Arms animations\\arms_sten_crawl";
	arms_anim_files[STENMK2][SPRINT]      = FPS_DIR "stenmk2\\Arms animations\\arms_sten_sprint";
	arms_anim_files[STENMK2][SIT]         = FPS_DIR "stenmk2\\Arms animations\\arms_sten_lowered";

	weap_anim_files[M3A1][RELOAD]      = FPS_DIR "m3a1\\Weapon animations\\weap_greasegun_reloading";
	weap_anim_files[M3A1][STAND_FIRE]  = FPS_DIR "m3a1\\Weapon animations\\weap_greasegun_fire";
	weap_anim_files[M3A1][CROUCH_FIRE] = FPS_DIR "m3a1\\Weapon animations\\weap_greasegun_fire";
	weap_anim_files[M3A1][AIM_FIRE]    = FPS_DIR "m3a1\\Weapon animations\\weap_greasegun_fire";
	arms_anim_files[M3A1][RELOAD]      = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_reloading";
	arms_anim_files[M3A1][STAND_FIRE]  = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_fire";
	arms_anim_files[M3A1][CROUCH_FIRE] = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_fire";
	arms_anim_files[M3A1][AIM_FIRE]    = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_aimfire";
	arms_anim_files[M3A1][LOWER]       = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_lowered";
	arms_anim_files[M3A1][IDLE]        = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_neutral";
	arms_anim_files[M3A1][KNEEL]       = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_neutral";
	arms_anim_files[M3A1][AIM]         = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_aim";
	arms_anim_files[M3A1][AIM_SMG]     = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_aim";
	arms_anim_files[M3A1][TROT]        = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_neutral";
	arms_anim_files[M3A1][CREEP]       = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_neutral";
	arms_anim_files[M3A1][CRAWL]       = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_crawl";
	arms_anim_files[M3A1][SPRINT]      = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_sprint";
	arms_anim_files[M3A1][SIT]         = FPS_DIR "m3a1\\Arms animations\\arms_greasegun_lowered";

	weap_anim_files[MP34][RELOAD]      = FPS_DIR "mp34\\Weapon animations\\weap_mp34_reloading";
	weap_anim_files[MP34][STAND_FIRE]  = FPS_DIR "mp34\\Weapon animations\\weap_mp34_fire";
	weap_anim_files[MP34][CROUCH_FIRE] = FPS_DIR "mp34\\Weapon animations\\weap_mp34_fire";
	weap_anim_files[MP34][AIM_FIRE]    = FPS_DIR "mp34\\Weapon animations\\weap_mp34_fire";
	arms_anim_files[MP34][RELOAD]      = FPS_DIR "mp34\\Arms animations\\arms_mp34_reloading";
	arms_anim_files[MP34][STAND_FIRE]  = FPS_DIR "mp34\\Arms animations\\arms_mp34_fire";
	arms_anim_files[MP34][CROUCH_FIRE] = FPS_DIR "mp34\\Arms animations\\arms_mp34_fire";
	arms_anim_files[MP34][AIM_FIRE]    = FPS_DIR "mp34\\Arms animations\\arms_mp34_aimfire";
	arms_anim_files[MP34][LOWER]       = FPS_DIR "mp34\\Arms animations\\arms_mp34_lowered";
	arms_anim_files[MP34][IDLE]        = FPS_DIR "mp34\\Arms animations\\arms_mp34_neutral";
	arms_anim_files[MP34][KNEEL]       = FPS_DIR "mp34\\Arms animations\\arms_mp34_neutral";
	arms_anim_files[MP34][AIM]         = FPS_DIR "mp34\\Arms animations\\arms_mp34_aim";
	arms_anim_files[MP34][AIM_SMG]     = FPS_DIR "mp34\\Arms animations\\arms_mp34_aim";
	arms_anim_files[MP34][TROT]        = FPS_DIR "mp34\\Arms animations\\arms_mp34_neutral";
	arms_anim_files[MP34][CREEP]       = FPS_DIR "mp34\\Arms animations\\arms_mp34_neutral";
	arms_anim_files[MP34][CRAWL]       = FPS_DIR "mp34\\Arms animations\\arms_mp34_crawl";
	arms_anim_files[MP34][SPRINT]      = FPS_DIR "mp34\\Arms animations\\arms_mp34_sprint";
	arms_anim_files[MP34][SIT]         = FPS_DIR "mp34\\Arms animations\\arms_mp34_lowered";

	weap_anim_files[FG42G][RELOAD]          = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_reload";
	weap_anim_files[FG42G][STAND_FIRE]      = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_fire";
	weap_anim_files[FG42G][AIM_FIRE]        = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_fire";
	weap_anim_files[FG42G][CROUCH_FIRE]     = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_fire";
	weap_anim_files[FG42G][DEPLOYING]       = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_deploy";
	weap_anim_files[FG42G][DEPLOYED]        = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_deployed";
	weap_anim_files[FG42G][UNDEPLOYING]     = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_deploy";
	weap_anim_files[FG42G][DEPLOYED_FIRE]   = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_deployed";
	weap_anim_files[FG42G][DEPLOYED_RELOAD] = FPS_DIR "fg42g\\Weapon Animations\\weap_FG42_reloadDeployed";
	arms_anim_files[FG42G][RELOAD]          = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_reload";
	arms_anim_files[FG42G][STAND_FIRE]      = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_fire";
	arms_anim_files[FG42G][AIM_FIRE]        = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_aimFire";
	arms_anim_files[FG42G][CROUCH_FIRE]     = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_fire";
	arms_anim_files[FG42G][LOWER]           = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_lowered";
	arms_anim_files[FG42G][IDLE]            = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_neutral";
	arms_anim_files[FG42G][KNEEL]           = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_neutral";
	arms_anim_files[FG42G][AIM]             = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_aim";
	arms_anim_files[FG42G][TROT]            = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_neutral";
	arms_anim_files[FG42G][CREEP]           = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_neutral";
	arms_anim_files[FG42G][CRAWL]           = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_crawl";
	arms_anim_files[FG42G][SPRINT]          = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_sprint";
	arms_anim_files[FG42G][SIT]             = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_neutral";
	arms_anim_files[FG42G][DEPLOYING]       = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_deploy";
	arms_anim_files[FG42G][DEPLOYED]        = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_deployed";
	arms_anim_files[FG42G][UNDEPLOYING]     = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_deploy";
	arms_anim_files[FG42G][DEPLOYED_FIRE]   = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_fireDeployed";
	arms_anim_files[FG42G][DEPLOYED_RELOAD] = FPS_DIR "fg42g\\Arms Animations\\arms_FG42_reload";

	// BRIDGE_REPAIR animation files
	arms_anim_files[BRIDGE_REPAIR][STAND_FIRE] = FPS_DIR "satchel\\animations\\fire\\satchel-fire(1)";

	arms_anim_files[BRIDGE_REPAIR][RELOAD] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";

	arms_anim_files[BRIDGE_REPAIR][LOWER] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";

	arms_anim_files[BRIDGE_REPAIR][IDLE] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[BRIDGE_REPAIR][KNEEL] = FPS_DIR "satchel\\animations\\satchel-stand(1)";

	arms_anim_files[BRIDGE_REPAIR][TROT] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[BRIDGE_REPAIR][CREEP] = FPS_DIR "satchel\\animations\\satchel-stand(1)";
	arms_anim_files[BRIDGE_REPAIR][CRAWL] = FPS_DIR "satchel\\animations\\crawl-satchel(30)";
	arms_anim_files[BRIDGE_REPAIR][SPRINT] = FPS_DIR "satchel\\animations\\sprint-satchel(15)";
	//arms_anim_files[BRIDGE_REPAIR][SIT] = FPS_DIR "satchel\\animations\\satchel-arms-sit(1)";
	arms_anim_files[BRIDGE_REPAIR][SIT] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";

	// BUILD_TOOL animation files
	arms_anim_files[BUILD_TOOL][STAND_FIRE] = FPS_DIR "shovel\\animations\\shovel_default(1)";

	arms_anim_files[BUILD_TOOL][RELOAD] = FPS_DIR "shovel\\animations\\shovel_default(1)";

	arms_anim_files[BUILD_TOOL][LOWER] = FPS_DIR "satchel\\animations\\satchel-lowered(1)";

	arms_anim_files[BUILD_TOOL][IDLE] = FPS_DIR "shovel\\animations\\shovel_default(1)";
	arms_anim_files[BUILD_TOOL][KNEEL] = FPS_DIR "shovel\\animations\\shovel_default(1)";

	arms_anim_files[BUILD_TOOL][TROT] = FPS_DIR "shovel\\animations\\shovel_default(1)";
	arms_anim_files[BUILD_TOOL][CREEP] = FPS_DIR "shovel\\animations\\shovel_default(1)";
	arms_anim_files[BUILD_TOOL][CRAWL] = FPS_DIR "shovel\\animations\\shovel_default(1)";
	arms_anim_files[BUILD_TOOL][SPRINT] = FPS_DIR "shovel\\animations\\shovel_sprint(15)";
	//arms_anim_files[BUILD_TOOL][SIT] = FPS_DIR "satchel\\animations\\satchel-arms-sit(1)";
	arms_anim_files[BUILD_TOOL][SIT] = FPS_DIR "shovel\\animations\\shovel_default(1)";

	// for reference:
	// 1 minute = 0.016667 degrees
	// 1 meter at 1000 meters is 0.05729576041450060899707537401941 degrees
	// 1 minute = 0.29m error at 1000m (approx 1 foot)

	// Trooper specific data (one-of)

	// need to remember the binocs setting, everything else gets blown away
	binocsEnabled = fpsTrooper.binocsEnabled;

	fpsTrooper.clear();

	fpsTrooper.binocsEnabled = binocsEnabled;

	fpsTrooper.mouseGranularity = MOUSE_GRANULARITY;

	fpsTrooper.atpMinDispersionATP = 0.4f;

	fpsTrooper.eyeHeight[FPS_EYE_DEAD]					= .26f;
	fpsTrooper.eyeHeight[FPS_EYE_PRONE]				= .313f;
	fpsTrooper.eyeHeight[FPS_EYE_PRONEMOVE]		= .313f;
	fpsTrooper.eyeHeight[FPS_EYE_PRONEAIM]			= .4f;
	fpsTrooper.eyeHeight[FPS_EYE_PRONEDEPLOY]		= .4f;
	fpsTrooper.eyeHeight[FPS_EYE_CROUCH]				= 1.229f;
	fpsTrooper.eyeHeight[FPS_EYE_CROUCHMOVE]		= 1.3f;
	fpsTrooper.eyeHeight[FPS_EYE_CROUCHAIM]			= 1.322f;
	fpsTrooper.eyeHeight[FPS_EYE_CROUCHDEPLOY]	= 1.142f;
	fpsTrooper.eyeHeight[FPS_EYE_STAND]				= 1.4f;
	fpsTrooper.eyeHeight[FPS_EYE_STANDMOVE]		= 1.602f;
	fpsTrooper.eyeHeight[FPS_EYE_STANDAIM]			= 1.702f;
	fpsTrooper.eyeHeight[FPS_EYE_STANDDEPLOY]		= 1.577f;

	fpFPSDataReadFPSFile();

	// calculate the fall deceleration time
	fpsTrooper.parachuteDecelerationTime = fpsTrooper.parachuteDeployTime +
		(2.0f * (fpsTrooper.minSafeFallDistance - 0.5f * 9.8f * fpsTrooper.parachuteDeployTime * fpsTrooper.parachuteDeployTime)) /
		(std::max(fpsTrooper.parachuteFallSpeed, (fpsTrooper.parachuteDeployTime * 9.8f)) + fpsTrooper.parachuteFallSpeed);

	memset (default_arms_tex_names, 0, sizeof (default_arms_tex_names));
//	default_arms_tex_names[BRITISH_PARA_PACK] = "b_2";
//	default_arms_tex_names[FRENCH_PARA_PACK] = "french-arms";
//	default_arms_tex_names[GERMAN_PARA_PACK] = "germ-arms";
	default_arms_tex_names[BRITISH_PARA_PACK] = "british_torso";
	default_arms_tex_names[FRENCH_PARA_PACK] = "french_torso";
	default_arms_tex_names[GERMAN_PARA_PACK] = "german_torso";

	memset (replacement_arms_tex_names, 0, sizeof (replacement_arms_tex_names));
	replacement_arms_tex_names[BRITISH_PARA_PACK] = "brit_torso_para";
	replacement_arms_tex_names[FRENCH_PARA_PACK] = "french_torso_para";
	replacement_arms_tex_names[GERMAN_PARA_PACK] = "germ_torso_para";

	// init weapon sight tables
	for(int i = 0; i < NUM_WEAPONS; i++)
	{
		if(fpsWeapons[i].numSightDists)
		{
			if(!fpsWeapons[i].startSightDists) fpsWeapons[i].startSightDists = 100;
			if(!fpsWeapons[i].incSightDists) fpsWeapons[i].incSightDists = 100;
		}

#ifdef CACHE_FPS_DATA
		if(fpsWeapons[i].weaponModel)
			gr2LoadModel(fpsWeapons[i].weaponModel, nullptr, nullptr, 0);

		if(fpsWeapons[i].clipModel)
			gr2LoadModel(fpsWeapons[i].clipModel, nullptr, nullptr, 0);

		for (int j=0; j < FPS_NUM_AUX_MODELS; j++)
		{
			if (fpsWeapons[i].auxModel[j])
				gr2LoadModel(fpsWeapons[i].auxModel[j], nullptr, nullptr, 0);
		}
#endif
	}

#ifdef CACHE_FPS_DATA
	gr2Model* skeleton = gr2Model::Load(skel_model, nullptr, nullptr, 1);

	for(int i = 0; i < NUM_INFANTRY_TYPES; i++)
	{
		if(arm_models[i])
			gr2LoadModel(arm_models[i], skeleton, nullptr, 0);

		if(leg_models[i])
			gr2LoadModel(leg_models[i], nullptr, nullptr, 0);
	}

	gr2FreeModel(skeleton);
#endif

	_fpFPSReadVisualLoadouts();
}

fpFPSTrooperLoadout* fpFPSGetTrooperLoadout(const char* id)
{
	fpFPSLoadoutMap::iterator found = _loadouts.find(id);
	if(found != _loadouts.end()) return &(found->second);

	return nullptr;
}

void _fpFPSReadVisualLoadouts()
{
	fpFPSVisLoadoutsLoader loader(FPS_VISLOADOUT_FILE);

	// read the xml file and load in the visual loadouts
	loader.Load(&_loadouts);
}

fpFPSVisLoadoutsLoader::fpFPSVisLoadoutsLoader(const char* path) : XMLLoader2<fpFPSVisLoadoutsLoader>(path)
{
	AddHandler("visloadout", &fpFPSVisLoadoutsLoader::DoVisLoadout, this);
	AddHandler("weapon", &fpFPSVisLoadoutsLoader::DoWeapon, this);
}

bool fpFPSVisLoadoutsLoader::DoVisLoadout(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		fpFPSLoadoutMap* loadouts = (fpFPSLoadoutMap*)userData;
		fpFPSTrooperLoadout loadout;

		loadout.id					= xmlGetStringFromNode(node, "id");
		loadout.arms				= _getArmsID(xmlGetStringFromNode(node, "arms"));
		loadout.atpMultiplier		= 1.0f;
		loadout.isPara				= false;

		xmlGetNodeValue(node, XML_DATA_FLOAT, &loadout.atpMultiplier, "atp");
		xmlGetNodeValue(node, XML_DATA_BOOL, &loadout.isPara, "para");

		if(ProcessChildren(node, &loadout))
			(*loadouts)[loadout.id] = loadout;
	}

	return false;
}

bool fpFPSVisLoadoutsLoader::DoWeapon(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		fpFPSTrooperLoadout* loadout = (fpFPSTrooperLoadout*)userData;
		fpFPSLoadoutWeapon weapon;

		weapon.index				= loadout->weapons.size();
		weapon.weapon				= _getWeaponID(xmlGetStringFromNode(node));
		weapon.aux					= false;
		weapon.lastResort			= false;

		xmlGetNodeValue(node, XML_DATA_BOOL, &weapon.aux, "aux");
		xmlGetNodeValue(node, XML_DATA_SINT, &weapon.index, "index");
		xmlGetNodeValue(node, XML_DATA_BOOL, &weapon.lastResort, "lastresort");

		loadout->weapons.push_back(weapon);
	}

	return false;
}

int fpFPSVisLoadoutsLoader::_getArmsID(const char* name)
{
	for(int i = 0; i < NUM_INFANTRY_TYPES; i++)
	{
		if(!stricmp(name, armTypeStrings[i]))
			return i;
	}

	return -1;
}

int fpFPSVisLoadoutsLoader::_getWeaponID(const char* name)
{
	for(int i = 0; i < NUM_WEAPONS; i++)
	{
		if(!stricmp(name, weapNameStrings[i]))
			return i;
	}

	return -1;
}

fpFPSTrooperData			fpsTrooper;
