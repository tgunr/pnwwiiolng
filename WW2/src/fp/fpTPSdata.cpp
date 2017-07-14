#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "str/strpool.h"

#include "fp/fpFPSdata.h"
#include "fp/fpTPSdata.h"
#include "fp/fpTPS.h"
#include "fio/fio.h"
#include "str/stringLib.h"
#include "gr2/gr2layer.h"

#define	TPS_DIR		"TPS\\"
#define TPS_DATA_TYPES			GR2DATA_PREPATH TPS_DIR "types.xml"
#define TPS_DATA_LIB			GR2DATA_PREPATH TPS_DIR "lib.xml"
#define TPS_MESH_DIR			TPS_DIR "meshes\\"
#define TPS_BLEND_DIR			TPS_DIR "blends\\"
#define TPS_MIXIN_DIR			TPS_DIR "mixins\\"
#define TPS_FACES_DIR			GR2DATA_PREPATH TPS_DIR "meshes\\faces\\"
#define TPS_COMBATEMOTES_DIR	TPS_MIXIN_DIR "emotes\\combat\\"
#define TPS_BODYEMOTES_DIR		TPS_MIXIN_DIR "emotes\\fullbody\\"

#define TPS_DATA_DIR		"vehicles\\ground\\groundman\\"
#define	TPS_DATA_FILE		"tpsData.txt"
#define	TPS_SURFACE_PREPEND	"surfaces\\surface_"
#define	TPS_SURFACE_APPEND	".txt"
#define TPS_MAX_BLEND_STATES 4

const SCHAR*					fpTPSDeathFiles[TPS_POSETYPE_NUM_TYPES][TPS_NUM_DEATHS];
const SCHAR*					fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_NUM_TYPES][TPS_HAND_NUM_TYPES][TPS_MIXINPOSE_NUM_TYPES];;
const SCHAR*					fpTPSBodyEmoteFiles[TPS_BODYEMOTE_NUM_TYPES];
const SCHAR*					fpTPSBasePoseFile;
const SCHAR*					fpTPSCombatBlendFile;

const SCHAR*					fpTPSSkeletonFile;
const SCHAR*					fpTPSDamageFile;
const SCHAR*					fpTPSInfantryMeshFiles[NUM_INFANTRY_TYPES][TPS_NUM_LODS];
const SCHAR*					fpTPSWeaponMeshFiles[NUM_WEAPONS][TPS_NUM_LODS];
const SCHAR*					fpTPSWeaponMeshAltFiles[NUM_WEAPONS][TPS_NUM_LODS];
const SCHAR*					fpTPSTwitchFiles[TPS_POSETYPE_NUM_TYPES][MAX_TWITCHES_PER_POSE];
SINT32					fpTPSNumTwitches[TPS_POSETYPE_NUM_TYPES];
fpTPSWeaponData			tpsWeapons[TPS_WEAPONTYPE_NUM_TYPES];
fpTPSTroopData			tpsTroopData;

const SCHAR*					fpTPSParachuteFile;

fpHitLocations			fpHitMap[60];

const SCHAR*					fpTPSDefaultFaceNames[NUM_INFANTRY_TYPES];

#ifdef USE_NEW_DECALS
const SCHAR*					fpTPSPatchTextureNames[NUM_INFANTRY_TYPES * 3];
SINT32					fpTPSPatchTextureIDs[NUM_INFANTRY_TYPES * 3];
#else
const SCHAR*					fpTPSPatchTextureNames[NUM_INFANTRY_TYPES];
SINT32					fpTPSPatchTextureIDs[NUM_INFANTRY_TYPES];
#endif

SINT32					fpTPSDefaultFaceTexIDs[NUM_INFANTRY_TYPES];
SINT32					fpTPSNumFaces;
SINT32					fpTPSFaceTexIDs[MAX_FACES];

fpTPSSurfaceData		fpTPSSurfaces[TPS_MAX_NUM_SURFACES];

const SCHAR* fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_NUM_TYPES] =
{
	"double",
	"getdown",
	"halt",
	"moveout",
	"ammo",
	"jump"
};

const SCHAR* fpTPSBodyEmoteCmds[TPS_BODYEMOTE_NUM_TYPES] =
{
	"salute",
	"salute",
	"salute",
	"salute",
	"gyruss",
	"cheer",
	"laugh",
	"attention"
};

const SCHAR* fpTPSWeaponTypeStrings[TPS_WEAPONTYPE_NUM_TYPES] =
{
	"TPS_WEAPONTYPE_RIFLE",
	"TPS_WEAPONTYPE_SMG",
	"TPS_WEAPONTYPE_PISTOL",
	"TPS_WEAPONTYPE_GRENADE",
	"TPS_WEAPONTYPE_EMPTY",
	"TPS_WEAPONTYPE_SATCHEL",
	"TPS_WEAPONTYPE_BINOCS",
	"TPS_WEAPONTYPE_KNIFE",
	"TPS_WEAPONTYPE_LMG",
	"TPS_WEAPONTYPE_ATR",
	"TPS_WEAPONTYPE_MAP",
	"TPS_WEAPONTYPE_LTMORTAR",
	"TPS_WEAPONTYPE_BUILD",
	"TPS_WEAPONTYPE_ROCKET",
	"TPS_WEAPONTYPE_ANY"
};

const SCHAR* fpTPSWeaponKeyStrings[TPS_WEAPONTYPE_NUM_TYPES] =
{
	"rifle",
	"smg",
	"pistol",
	"grenade",
	"empty",
	"satchel",
	"binocs",
	"knife",
	"lmg",
	"atr",
	"map",
	"ltmortar",
	"build",
	"rocket",
	"any"
};

const SCHAR* fpTPSPoseTypeStrings[TPS_POSETYPE_NUM_TYPES] =
{
	"TPS_POSETYPE_STAND",
	"TPS_POSETYPE_CROUCH",
	"TPS_POSETYPE_PRONE",
	"TPS_POSETYPE_SIT",
	"TPS_POSETYPE_SWIM"
};

const SCHAR* fpTPSPoseKeyStrings[TPS_POSETYPE_NUM_TYPES] =
{
	"stand",
	"crouch",
	"prone",
	"sit",
	"swim"
};

const SCHAR* fpTPSMoveTypeStrings[TPS_MOVETYPE_NUM_TYPES] =
{
	"TPS_MOVETYPE_STILL",
	"TPS_MOVETYPE_IDLE",
	"TPS_MOVETYPE_WALK",
	"TPS_MOVETYPE_FORWARD",
	"TPS_MOVETYPE_SPRINT",
	"TPS_MOVETYPE_BACKWARD",
	"TPS_MOVETYPE_DEPLOYED",
	"TPS_MOVETYPE_LEAN_LEFT",
	"TPS_MOVETYPE_LEAN_RIGHT",
	"TPS_MOVETYPE_EMOTE",
	"TPS_MOVETYPE_TURN",
	"TPS_MOVETYPE_FALL",
	"TPS_MOVETYPE_STRAFE_LEFT",
	"TPS_MOVETYPE_STRAFE_RIGHT",
};

const SCHAR* fpTPSMoveKeyStrings[TPS_MOVETYPE_NUM_TYPES] =
{
	"still",
	"idle",
	"walk",
	"forward",
	"sprint",
	"backward",
	"deployed",
	"leanleft",
	"leanright",
	"emote",
	"turn",
	"fall",
	"strafeleft",
	"straferight"
};

const SCHAR* fpTPSPointTypeStrings[TPS_POINTTYPE_NUM_TYPES] =
{
	"TPS_POINTTYPE_RELAXED",
	"TPS_POINTTYPE_READY",
	"TPS_POINTTYPE_AIM",
	"TPS_POINTTYPE_RELOAD",
	"TPS_POINTTYPE_ACTION1",
	"TPS_POINTTYPE_ACTION2",
	"TPS_POINTTYPE_ATTACK1",
	"TPS_POINTTYPE_ATTACK2",
	"TPS_POINTTYPE_STOW",
	"TPS_POINTTYPE_TOPRONE",
	"TPS_POINTTYPE_TOCROUCH",
	"TPS_POINTTYPE_TOSTAND",
	"TPS_POINTTYPE_MOTION",
	"TPS_POINTTYPE_PARA"
};

const SCHAR* fpTPSPointKeyStrings[TPS_POINTTYPE_NUM_TYPES] =
{
	"relax",
	"ready",
	"aim",
	"reload",
	"action1",
	"action2",
	"attack1",
	"attack2",
	"stow",
	"toprone",
	"tocrouch",
	"tostand",
	"motion",
	"para"
};

TPSTypes tpsTypesMap;

void fpTPSDataReadTPSFile();
void fpTPSDataReadSurfaceFiles();
void fpTPSDataReadHitLocation(SCHAR* data);
void fpTPSDataInitHitLocations();

void fpTPSDataInit()
{
	//fpTPSSkeletonFile = TPS_DIR "troopskel";
	fpTPSSkeletonFile = TPS_DIR "Inf_BaseSkeleton";

	fpTPSDamageFile = TPS_MESH_DIR "damage\\damagemodel";

	for (int i=0; i < NUM_INFANTRY_TYPES; i++)
	{
		for (int j=0; j < TPS_NUM_LODS; j++)
		{
			fpTPSInfantryMeshFiles[i][j] = "";
		}
	}

	for (int i=0; i < NUM_WEAPONS; i++)
	{
		for (int j=0; j < TPS_NUM_LODS; j++)
		{
			fpTPSWeaponMeshFiles[i][j] = "";
			fpTPSWeaponMeshAltFiles[i][j] = "";
		}
	}

	for (int i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		fpTPSNumTwitches[i] = 0;

		for (int j=0; j < MAX_TWITCHES_PER_POSE; j++)
		{
			fpTPSTwitchFiles[i][j] = "";
		}
	}

	memset (fpTPSSurfaces, 0, sizeof (fpTPSSurfaces));


#if(0)
    // For testing only!
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_1] = TPS_MESH_DIR "british_inf\\test_patch";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_2] = TPS_MESH_DIR "british_inf\\test_patch";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_3] = TPS_MESH_DIR "british_inf\\test_patch";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_4] = TPS_MESH_DIR "british_inf\\test_patch";
#else
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_1] = TPS_MESH_DIR "british_inf\\inf_uk_lod1";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_2] = TPS_MESH_DIR "british_inf\\inf_uk_lod2";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_3] = TPS_MESH_DIR "british_inf\\inf_uk_lod3";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_4] = TPS_MESH_DIR "british_inf\\inf_uk_lod4";
	fpTPSInfantryMeshFiles[BRITISH_INFANTRY][TPS_LOD_5] = TPS_MESH_DIR "british_inf\\inf_uk_lod5";
#endif

	fpTPSInfantryMeshFiles[FRENCH_INFANTRY][TPS_LOD_1] = TPS_MESH_DIR "french_inf\\inf_fr_lod1";
	fpTPSInfantryMeshFiles[FRENCH_INFANTRY][TPS_LOD_2] = TPS_MESH_DIR "french_inf\\inf_fr_lod2";
	fpTPSInfantryMeshFiles[FRENCH_INFANTRY][TPS_LOD_3] = TPS_MESH_DIR "french_inf\\inf_fr_lod3";
	fpTPSInfantryMeshFiles[FRENCH_INFANTRY][TPS_LOD_4] = TPS_MESH_DIR "french_inf\\inf_fr_lod4";
	fpTPSInfantryMeshFiles[FRENCH_INFANTRY][TPS_LOD_5] = TPS_MESH_DIR "french_inf\\inf_fr_lod5";

	fpTPSInfantryMeshFiles[GERMAN_INFANTRY][TPS_LOD_1] = TPS_MESH_DIR "german_inf\\inf_de_lod1";
	fpTPSInfantryMeshFiles[GERMAN_INFANTRY][TPS_LOD_2] = TPS_MESH_DIR "german_inf\\inf_de_lod2";
	fpTPSInfantryMeshFiles[GERMAN_INFANTRY][TPS_LOD_3] = TPS_MESH_DIR "german_inf\\inf_de_lod3";
	fpTPSInfantryMeshFiles[GERMAN_INFANTRY][TPS_LOD_4] = TPS_MESH_DIR "german_inf\\inf_de_lod4";
	fpTPSInfantryMeshFiles[GERMAN_INFANTRY][TPS_LOD_5] = TPS_MESH_DIR "german_inf\\inf_de_lod5";

	fpTPSInfantryMeshFiles[AMERICAN_INFANTRY][TPS_LOD_1] = TPS_MESH_DIR "american_inf\\inf_us_lod1";
	fpTPSInfantryMeshFiles[AMERICAN_INFANTRY][TPS_LOD_2] = TPS_MESH_DIR "american_inf\\inf_us_lod2";
	fpTPSInfantryMeshFiles[AMERICAN_INFANTRY][TPS_LOD_3] = TPS_MESH_DIR "american_inf\\inf_us_lod3";
	fpTPSInfantryMeshFiles[AMERICAN_INFANTRY][TPS_LOD_4] = TPS_MESH_DIR "american_inf\\inf_us_lod4";
	//fpTPSInfantryMeshFiles[AMERICAN_INFANTRY][TPS_LOD_5] = TPS_MESH_DIR "american_inf\\inf_us_lod5";

	fpTPSInfantryMeshFiles[BRITISH_PARA_PACK][TPS_LOD_1] = TPS_MESH_DIR "para_brit\\para_uk_chute_lod1";
	fpTPSInfantryMeshFiles[BRITISH_PARA_PACK][TPS_LOD_2] = TPS_MESH_DIR "para_brit\\para_uk_chute_lod2";
	fpTPSInfantryMeshFiles[BRITISH_PARA_PACK][TPS_LOD_3] = TPS_MESH_DIR "para_brit\\para_uk_chute_lod3";
	fpTPSInfantryMeshFiles[BRITISH_PARA_PACK][TPS_LOD_4] = TPS_MESH_DIR "para_brit\\para_uk_chute_lod4";
	fpTPSInfantryMeshFiles[BRITISH_PARA_PACK][TPS_LOD_5] = TPS_MESH_DIR "para_brit\\para_uk_chute_lod5";

	fpTPSInfantryMeshFiles[BRITISH_PARA_NOPACK][TPS_LOD_1] = TPS_MESH_DIR "para_brit\\para_uk_lod1";
	fpTPSInfantryMeshFiles[BRITISH_PARA_NOPACK][TPS_LOD_2] = TPS_MESH_DIR "para_brit\\para_uk_lod2";
	fpTPSInfantryMeshFiles[BRITISH_PARA_NOPACK][TPS_LOD_3] = TPS_MESH_DIR "para_brit\\para_uk_lod3";
	fpTPSInfantryMeshFiles[BRITISH_PARA_NOPACK][TPS_LOD_4] = TPS_MESH_DIR "para_brit\\para_uk_lod4";
	fpTPSInfantryMeshFiles[BRITISH_PARA_NOPACK][TPS_LOD_5] = TPS_MESH_DIR "para_brit\\para_uk_lod5";

	fpTPSInfantryMeshFiles[FRENCH_PARA_PACK][TPS_LOD_1] = TPS_MESH_DIR "para_french\\para_fr_chute_lod1";
	fpTPSInfantryMeshFiles[FRENCH_PARA_PACK][TPS_LOD_2] = TPS_MESH_DIR "para_french\\para_fr_chute_lod2";
	fpTPSInfantryMeshFiles[FRENCH_PARA_PACK][TPS_LOD_3] = TPS_MESH_DIR "para_french\\para_fr_chute_lod3";
	fpTPSInfantryMeshFiles[FRENCH_PARA_PACK][TPS_LOD_4] = TPS_MESH_DIR "para_french\\para_fr_chute_lod4";
	fpTPSInfantryMeshFiles[FRENCH_PARA_PACK][TPS_LOD_5] = TPS_MESH_DIR "para_french\\para_fr_chute_lod5";

	fpTPSInfantryMeshFiles[FRENCH_PARA_NOPACK][TPS_LOD_1] = TPS_MESH_DIR "para_french\\para_fr_lod1";
	fpTPSInfantryMeshFiles[FRENCH_PARA_NOPACK][TPS_LOD_2] = TPS_MESH_DIR "para_french\\para_fr_lod2";
	fpTPSInfantryMeshFiles[FRENCH_PARA_NOPACK][TPS_LOD_3] = TPS_MESH_DIR "para_french\\para_fr_lod3";
	fpTPSInfantryMeshFiles[FRENCH_PARA_NOPACK][TPS_LOD_4] = TPS_MESH_DIR "para_french\\para_fr_lod4";
	fpTPSInfantryMeshFiles[FRENCH_PARA_NOPACK][TPS_LOD_5] = TPS_MESH_DIR "para_french\\para_fr_lod5";

	fpTPSInfantryMeshFiles[GERMAN_PARA_PACK][TPS_LOD_1] = TPS_MESH_DIR "para_germ\\para_de_chute_lod1";
	fpTPSInfantryMeshFiles[GERMAN_PARA_PACK][TPS_LOD_2] = TPS_MESH_DIR "para_germ\\para_de_chute_lod2";
	fpTPSInfantryMeshFiles[GERMAN_PARA_PACK][TPS_LOD_3] = TPS_MESH_DIR "para_germ\\para_de_chute_lod3";
	fpTPSInfantryMeshFiles[GERMAN_PARA_PACK][TPS_LOD_4] = TPS_MESH_DIR "para_germ\\para_de_chute_lod4";
	fpTPSInfantryMeshFiles[GERMAN_PARA_PACK][TPS_LOD_5] = TPS_MESH_DIR "para_germ\\para_de_chute_lod5";

	fpTPSInfantryMeshFiles[GERMAN_PARA_NOPACK][TPS_LOD_1] = TPS_MESH_DIR "para_germ\\para_de_lod1";
	fpTPSInfantryMeshFiles[GERMAN_PARA_NOPACK][TPS_LOD_2] = TPS_MESH_DIR "para_germ\\para_de_lod2";
	fpTPSInfantryMeshFiles[GERMAN_PARA_NOPACK][TPS_LOD_3] = TPS_MESH_DIR "para_germ\\para_de_lod3";
	fpTPSInfantryMeshFiles[GERMAN_PARA_NOPACK][TPS_LOD_4] = TPS_MESH_DIR "para_germ\\para_de_lod4";
	fpTPSInfantryMeshFiles[GERMAN_PARA_NOPACK][TPS_LOD_5] = TPS_MESH_DIR "para_germ\\para_de_lod5";

	fpTPSWeaponMeshFiles[THOMPSON][TPS_LOD_1] = TPS_MESH_DIR "thompson\\thompson";
	fpTPSWeaponMeshFiles[THOMPSON][TPS_LOD_2] = TPS_MESH_DIR "thompson\\thompson2";

	fpTPSWeaponMeshFiles[M1THOMPSON][TPS_LOD_1] = TPS_MESH_DIR "thompson\\m1thompson";
	fpTPSWeaponMeshFiles[M1THOMPSON][TPS_LOD_2] = TPS_MESH_DIR "thompson\\thompson2";

	fpTPSWeaponMeshFiles[MP40][TPS_LOD_1] = TPS_MESH_DIR "mp40\\mp40";
	fpTPSWeaponMeshFiles[MP40][TPS_LOD_2] = TPS_MESH_DIR "mp40\\mp402";

	fpTPSWeaponMeshFiles[MAS][TPS_LOD_1] = TPS_MESH_DIR "mas\\mas";
	fpTPSWeaponMeshFiles[MAS][TPS_LOD_2] = TPS_MESH_DIR "mas\\mas2";

	fpTPSWeaponMeshFiles[SATCHEL][TPS_LOD_1] = TPS_MESH_DIR "satchel\\AlliedSatchel";

	//DEG_CHANGE
	fpTPSWeaponMeshFiles[SATCHEL_AX][TPS_LOD_1] = TPS_MESH_DIR "satchel\\AxisSatchel";
	//DEG_CHANGE_END

	fpTPSWeaponMeshFiles[KAR98K][TPS_LOD_1] = TPS_MESH_DIR "k98k\\k98k";
	fpTPSWeaponMeshFiles[KAR98K][TPS_LOD_2] = TPS_MESH_DIR "k98k\\k98k-2";

	fpTPSWeaponMeshFiles[KAR98KS][TPS_LOD_1] = TPS_MESH_DIR "k98k\\k98k-scope";
	fpTPSWeaponMeshFiles[KAR98KS][TPS_LOD_2] = TPS_MESH_DIR "k98k\\k98k-2";

	fpTPSWeaponMeshFiles[ENFIELD][TPS_LOD_1] = TPS_MESH_DIR "enfield\\enfield";
	fpTPSWeaponMeshFiles[ENFIELD][TPS_LOD_2] = TPS_MESH_DIR "enfield\\enfield-2";

	fpTPSWeaponMeshFiles[ENFIELDMK4][TPS_LOD_1] = TPS_MESH_DIR "enfield\\enfield-scope";
	fpTPSWeaponMeshFiles[ENFIELDMK4][TPS_LOD_2] = TPS_MESH_DIR "enfield\\enfield-2";

	fpTPSWeaponMeshFiles[LEBEL][TPS_LOD_1] = TPS_MESH_DIR "lebel\\lebel";

	fpTPSWeaponMeshFiles[M1935A][TPS_LOD_1] = TPS_MESH_DIR "m1935\\m1935";

	fpTPSWeaponMeshFiles[LUGER][TPS_LOD_1] = TPS_MESH_DIR "luger\\luger";

	fpTPSWeaponMeshFiles[MILLS36MK1][TPS_LOD_1] = TPS_MESH_DIR "uk_gren_he\\uk_gren_he";
	fpTPSWeaponMeshFiles[MILLS36MK1][TPS_LOD_2] = TPS_MESH_DIR "uk_gren_he\\uk_gren_he_lod2";

	fpTPSWeaponMeshFiles[M18][TPS_LOD_1] = TPS_MESH_DIR "uk_gren_smk\\uk_gren_smk";

	fpTPSWeaponMeshFiles[MARK2][TPS_LOD_1] = TPS_MESH_DIR "us_gren_he\\us_gren_he";

	fpTPSWeaponMeshFiles[FRBANG][TPS_LOD_1] = TPS_MESH_DIR "fr_gren_he\\fr_gren_he";

	fpTPSWeaponMeshFiles[FRSMOKE][TPS_LOD_1] = TPS_MESH_DIR "fr_gren_smk\\fr_gren_smk";

	fpTPSWeaponMeshFiles[STIGR][TPS_LOD_1] = TPS_MESH_DIR "de_gren_he\\de_gren_he";

	fpTPSWeaponMeshFiles[HGR39][TPS_LOD_1] = TPS_MESH_DIR "de_gren_smk\\de_gren_smk";

	fpTPSWeaponMeshFiles[WEBLEY][TPS_LOD_1] = TPS_MESH_DIR "enfmk2\\enfmk2";

	fpTPSWeaponMeshFiles[BINOCS][TPS_LOD_1] = TPS_MESH_DIR "binoculars\\binocs";

	fpTPSWeaponMeshFiles[KNIFE_AXIS][TPS_LOD_1] = TPS_MESH_DIR "knife\\knife";

	fpTPSWeaponMeshFiles[KNIFE_ALLIED][TPS_LOD_1] = TPS_MESH_DIR "knife\\knife";

	fpTPSWeaponMeshFiles[BREN][TPS_LOD_1] = TPS_MESH_DIR "bren\\bren";
	fpTPSWeaponMeshFiles[BREN][TPS_LOD_2] = TPS_MESH_DIR "bren\\bren2";

	fpTPSWeaponMeshAltFiles[BREN][TPS_LOD_1] = TPS_MESH_DIR "bren\\bren_deployed";
	fpTPSWeaponMeshAltFiles[BREN][TPS_LOD_2] = TPS_MESH_DIR "bren\\bren2_deployed";

	fpTPSWeaponMeshFiles[MG34][TPS_LOD_1] = TPS_MESH_DIR "mg34\\mg34";
	fpTPSWeaponMeshFiles[MG34][TPS_LOD_2] = TPS_MESH_DIR "mg34\\mg342";

	fpTPSWeaponMeshAltFiles[MG34][TPS_LOD_1] = TPS_MESH_DIR "mg34\\mg34_deployed";
	fpTPSWeaponMeshAltFiles[MG34][TPS_LOD_2] = TPS_MESH_DIR "mg34\\mg342_deployed";

	fpTPSWeaponMeshFiles[FM29][TPS_LOD_1] = TPS_MESH_DIR "fm29\\fm29";
	fpTPSWeaponMeshFiles[FM29][TPS_LOD_2] = TPS_MESH_DIR "fm29\\fm292";

	fpTPSWeaponMeshAltFiles[FM29][TPS_LOD_1] = TPS_MESH_DIR "fm29\\fm29_deployed";
	fpTPSWeaponMeshAltFiles[FM29][TPS_LOD_2] = TPS_MESH_DIR "fm29\\fm292_deployed";

	fpTPSWeaponMeshFiles[AMMO][TPS_LOD_1] = TPS_MESH_DIR "supbox\\supbox";

	fpTPSWeaponMeshFiles[MAS36][TPS_LOD_1] = TPS_MESH_DIR "mas36\\mas36";
	fpTPSWeaponMeshFiles[MAS40][TPS_LOD_1] = TPS_MESH_DIR "mas40\\mas40";

	fpTPSWeaponMeshFiles[LEBELMLE][TPS_LOD_1] = TPS_MESH_DIR "lebel\\lebelSniper";

	fpTPSWeaponMeshFiles[RG_G30][TPS_LOD_1] = TPS_MESH_DIR "k98k\\k98k";
	fpTPSWeaponMeshFiles[RG_G30][TPS_LOD_2] = TPS_MESH_DIR "k98k\\k98k-2";

	fpTPSWeaponMeshAltFiles[RG_G30][TPS_LOD_1] = TPS_MESH_DIR "g30\\g30";
	fpTPSWeaponMeshAltFiles[RG_G30][TPS_LOD_2] = TPS_MESH_DIR "g30\\g30-2";

	fpTPSWeaponMeshFiles[RG_VB][TPS_LOD_1] = TPS_MESH_DIR "lebel\\lebel";

	fpTPSWeaponMeshAltFiles[RG_VB][TPS_LOD_1] = TPS_MESH_DIR "vb\\vb";
	fpTPSWeaponMeshAltFiles[RG_VB][TPS_LOD_2] = TPS_MESH_DIR "vb\\vb-2";

	fpTPSWeaponMeshFiles[RG_MILLS][TPS_LOD_1] = TPS_MESH_DIR "enfield\\enfield";

	fpTPSWeaponMeshAltFiles[RG_MILLS][TPS_LOD_1] = TPS_MESH_DIR "mills-rg\\mills";
	fpTPSWeaponMeshAltFiles[RG_MILLS][TPS_LOD_2] = TPS_MESH_DIR "mills-rg\\mills-2";

	fpTPSWeaponMeshFiles[BOYSMK1][TPS_LOD_1] = TPS_MESH_DIR "boysmk1\\boysmk1";
	fpTPSWeaponMeshFiles[BOYSMK1][TPS_LOD_2] = TPS_MESH_DIR "boysmk1\\boys2";

	fpTPSWeaponMeshAltFiles[BOYSMK1][TPS_LOD_1] = TPS_MESH_DIR "boysmk1\\boysmk1-deployed";
	fpTPSWeaponMeshAltFiles[BOYSMK1][TPS_LOD_2] = TPS_MESH_DIR "boysmk1\\boys2-deployed";

	fpTPSWeaponMeshFiles[PZB39][TPS_LOD_1] = TPS_MESH_DIR "pzb39\\pzb39";
	fpTPSWeaponMeshFiles[PZB39][TPS_LOD_2] = TPS_MESH_DIR "pzb39\\pzb392";

	fpTPSWeaponMeshAltFiles[PZB39][TPS_LOD_1] = TPS_MESH_DIR "pzb39\\pzb39-deployed";
	fpTPSWeaponMeshAltFiles[PZB39][TPS_LOD_2] = TPS_MESH_DIR "pzb39\\pzb392-deployed";

	fpTPSWeaponMeshFiles[BRIDGE_REPAIR][TPS_LOD_1] = TPS_MESH_DIR "bridge_repair\\repairtool";

	fpTPSWeaponMeshFiles[MAP][TPS_LOD_1] = TPS_MESH_DIR "map\\map";

	fpTPSWeaponMeshFiles[BUILD_TOOL][TPS_LOD_1] = TPS_MESH_DIR "shovel\\shovel";

	fpTPSWeaponMeshFiles[M1GARAND][TPS_LOD_1]			= TPS_MESH_DIR "m1garand\\us_weap_rifle";
	fpTPSWeaponMeshFiles[COLT1911][TPS_LOD_1]			= TPS_MESH_DIR "colt1911\\us_weap_pistol";

	fpTPSWeaponMeshFiles[BAR][TPS_LOD_1]				= TPS_MESH_DIR "bar\\us_weap_bar";
	fpTPSWeaponMeshAltFiles[BAR][TPS_LOD_1]				= TPS_MESH_DIR "bar\\us_weap_bar_deployed";

	fpTPSWeaponMeshFiles[SPRINGFIELD1903][TPS_LOD_1]	= TPS_MESH_DIR "springfield\\us_weap_sniper";
	fpTPSWeaponMeshFiles[GEW43][TPS_LOD_1]				= TPS_MESH_DIR "gewehr\\gewehr";
	fpTPSWeaponMeshFiles[GEW41][TPS_LOD_1]				= TPS_MESH_DIR "gewehr41\\gewehr41";
	fpTPSWeaponMeshFiles[M1903A3][TPS_LOD_1]			= TPS_MESH_DIR "springfield\\us_weap_springfield";

	// mortar alt weapon files are used for placement on the back, not the deployed state model

	fpTPSWeaponMeshFiles[LTMORTAR_UK][TPS_LOD_1] = TPS_MESH_DIR "ltmortar_uk\\brit2inMesh";
	fpTPSWeaponMeshFiles[LTMORTAR_UK][TPS_LOD_2] = TPS_MESH_DIR "ltmortar_uk\\brit2inMeshLOD2";

	fpTPSWeaponMeshAltFiles[LTMORTAR_UK][TPS_LOD_1] = TPS_MESH_DIR "ltmortar_uk\\lmortarStowedMesh";
	fpTPSWeaponMeshAltFiles[LTMORTAR_UK][TPS_LOD_2] = TPS_MESH_DIR "ltmortar_uk\\lmortarStowedMeshLOD2";

	fpTPSWeaponMeshFiles[LTMORTAR_FR][TPS_LOD_1] = TPS_MESH_DIR "ltmortar_fr\\brandtMesh";
	fpTPSWeaponMeshFiles[LTMORTAR_FR][TPS_LOD_2] = TPS_MESH_DIR "ltmortar_fr\\brandtMeshLOD2";

	fpTPSWeaponMeshAltFiles[LTMORTAR_FR][TPS_LOD_1] = TPS_MESH_DIR "ltmortar_fr\\lmortarStowedMesh";
	fpTPSWeaponMeshAltFiles[LTMORTAR_FR][TPS_LOD_2] = TPS_MESH_DIR "ltmortar_fr\\lmortarStowedMeshLOD2";

	fpTPSWeaponMeshFiles[LTMORTAR_DE][TPS_LOD_1] = TPS_MESH_DIR "ltmortar_de\\lmortarMesh";
	fpTPSWeaponMeshFiles[LTMORTAR_DE][TPS_LOD_2] = TPS_MESH_DIR "ltmortar_de\\lmortarMeshLOD2";

	fpTPSWeaponMeshAltFiles[LTMORTAR_DE][TPS_LOD_1] = TPS_MESH_DIR "ltmortar_de\\lmortarStowedMesh";
	fpTPSWeaponMeshAltFiles[LTMORTAR_DE][TPS_LOD_2] = TPS_MESH_DIR "ltmortar_de\\lmortarStowedMeshLOD2";

	fpTPSWeaponMeshFiles[BAZOOKA][TPS_LOD_1] = TPS_MESH_DIR "m9bazooka\\m9bazooka";
	fpTPSWeaponMeshFiles[PZSCHRECK][TPS_LOD_1] = TPS_MESH_DIR "panzerschreck\\panzerschreck";
	fpTPSWeaponMeshFiles[USSMOKE][TPS_LOD_1] = TPS_MESH_DIR "us_gren_smk\\m8smoke";

	fpTPSWeaponMeshFiles[PIAT][TPS_LOD_1] = TPS_MESH_DIR "piat\\piat";
	fpTPSWeaponMeshAltFiles[PIAT][TPS_LOD_1] = TPS_MESH_DIR "piat\\piat";

	fpTPSWeaponMeshFiles[STENMK2][TPS_LOD_1]  = TPS_MESH_DIR "stenmk2\\stenmk2";
	fpTPSWeaponMeshFiles[M3A1][TPS_LOD_1]     = TPS_MESH_DIR "m3a1\\m3a1";
	fpTPSWeaponMeshFiles[MP34][TPS_LOD_1]     = TPS_MESH_DIR "mp34\\mp34";
	fpTPSWeaponMeshFiles[FG42G][TPS_LOD_1]    = TPS_MESH_DIR "fg42g\\fg42g";
	fpTPSWeaponMeshAltFiles[FG42G][TPS_LOD_1] = TPS_MESH_DIR "fg42g\\fg42g_Deployed";

	// base pose
	fpTPSBasePoseFile = TPS_BLEND_DIR "stand\\idle\\ui\\relaxed.txt";
	// combat blend
	fpTPSCombatBlendFile = TPS_BLEND_DIR "stand\\idle.txt";

	// end main blend files

	// tpsWeapons data
	memset (tpsWeapons, 0, sizeof (tpsWeapons));

	tpsWeapons[TPS_WEAPONTYPE_RIFLE].attachBone = "riflemount";
	tpsWeapons[TPS_WEAPONTYPE_RIFLE].attachToBone = "lthand";
	tpsWeapons[TPS_WEAPONTYPE_RIFLE].emoteHand = TPS_HAND_RIGHT;
	tpsWeapons[TPS_WEAPONTYPE_RIFLE].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_PISTOL].attachBone = "pistolmount";
	tpsWeapons[TPS_WEAPONTYPE_PISTOL].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_PISTOL].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_PISTOL].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_SMG].attachBone = "smgattach";
	tpsWeapons[TPS_WEAPONTYPE_SMG].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_SMG].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_SMG].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_GRENADE].attachBone = "grenadeattach";
	tpsWeapons[TPS_WEAPONTYPE_GRENADE].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_GRENADE].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_GRENADE].forceWeaponDown = FALSE;

	tpsWeapons[TPS_WEAPONTYPE_SATCHEL].attachBone = "satchelattach";
	tpsWeapons[TPS_WEAPONTYPE_SATCHEL].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_SATCHEL].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_SATCHEL].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_BUILD].attachBone = "satchelattach";
	tpsWeapons[TPS_WEAPONTYPE_BUILD].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_BUILD].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_BUILD].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_KNIFE].attachBone = "knifeattach";
	tpsWeapons[TPS_WEAPONTYPE_KNIFE].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_KNIFE].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_KNIFE].forceWeaponDown = FALSE;

	tpsWeapons[TPS_WEAPONTYPE_BINOCS].attachBone = "binocattach";
	tpsWeapons[TPS_WEAPONTYPE_BINOCS].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_BINOCS].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_BINOCS].forceWeaponDown = FALSE;

	tpsWeapons[TPS_WEAPONTYPE_LMG].attachBone = "lmgattach";
	tpsWeapons[TPS_WEAPONTYPE_LMG].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_LMG].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_LMG].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_ATR].attachBone = "lmgattach";
	tpsWeapons[TPS_WEAPONTYPE_ATR].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_ATR].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_ATR].forceWeaponDown = FALSE;

	tpsWeapons[TPS_WEAPONTYPE_LTMORTAR].attachBone = "MortarConnect";
	tpsWeapons[TPS_WEAPONTYPE_LTMORTAR].attachToBone = "lthand";
	tpsWeapons[TPS_WEAPONTYPE_LTMORTAR].emoteHand = TPS_HAND_RIGHT;
	tpsWeapons[TPS_WEAPONTYPE_LTMORTAR].forceWeaponDown = TRUE;

	tpsWeapons[TPS_WEAPONTYPE_MAP].attachBone = "map-connect";
	tpsWeapons[TPS_WEAPONTYPE_MAP].attachToBone = "lthand";
	tpsWeapons[TPS_WEAPONTYPE_MAP].emoteHand = TPS_HAND_RIGHT;
	tpsWeapons[TPS_WEAPONTYPE_MAP].forceWeaponDown = FALSE;

	tpsWeapons[TPS_WEAPONTYPE_ROCKET].attachBone = "bazattach";
	tpsWeapons[TPS_WEAPONTYPE_ROCKET].attachToBone = "rthand";
	tpsWeapons[TPS_WEAPONTYPE_ROCKET].emoteHand = TPS_HAND_LEFT;
	tpsWeapons[TPS_WEAPONTYPE_ROCKET].forceWeaponDown = TRUE;

	// MIXINS
	// death animations
	memset (fpTPSDeathFiles, 0, sizeof (fpTPSDeathFiles));

	fpTPSDeathFiles[TPS_POSETYPE_STAND][TPS_DEATH_PARA] = TPS_MIXIN_DIR "deaths\\death_para.txt";

	// twitches
	fpTPSNumTwitches[TPS_POSETYPE_STAND] = 3;
	fpTPSTwitchFiles[TPS_POSETYPE_STAND][0] = TPS_MIXIN_DIR "twitches\\stand\\twitchstand1.txt";
	fpTPSTwitchFiles[TPS_POSETYPE_STAND][1] = TPS_MIXIN_DIR "twitches\\stand\\twitchstand2.txt";
	fpTPSTwitchFiles[TPS_POSETYPE_STAND][2] = TPS_MIXIN_DIR "twitches\\stand\\twitchstand3.txt";

	fpTPSNumTwitches[TPS_POSETYPE_CROUCH] = 1;
	fpTPSTwitchFiles[TPS_POSETYPE_CROUCH][0] = TPS_MIXIN_DIR "twitches\\kneel\\twitchkneel1.txt";

	fpTPSNumTwitches[TPS_POSETYPE_PRONE] = 3;
	fpTPSTwitchFiles[TPS_POSETYPE_PRONE][0] = TPS_MIXIN_DIR "twitches\\prone\\twitchprone1.txt";
	fpTPSTwitchFiles[TPS_POSETYPE_PRONE][1] = TPS_MIXIN_DIR "twitches\\prone\\twitchprone2.txt";
	fpTPSTwitchFiles[TPS_POSETYPE_PRONE][2] = TPS_MIXIN_DIR "twitches\\prone\\twitchprone3.txt";

	fpTPSNumTwitches[TPS_POSETYPE_SWIM] = 1;
	fpTPSTwitchFiles[TPS_POSETYPE_SWIM][0] = TPS_MIXIN_DIR "twitches\\swim\\twitchswim1.txt";

	// emotes
	// combat emotes
	/*
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_DOUBLETIME][TPS_HAND_LEFT][TPS_MIXINPOSE_STAND] = TPS_COMBATEMOTES_DIR "doubletime-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_DOUBLETIME][TPS_HAND_RIGHT][TPS_MIXINPOSE_STAND] = TPS_COMBATEMOTES_DIR "doubletime-right.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_DOUBLETIME][TPS_HAND_LEFT][TPS_MIXINPOSE_PRONE] = TPS_COMBATEMOTES_DIR "prone\\doubletime-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_DOUBLETIME][TPS_HAND_RIGHT][TPS_MIXINPOSE_PRONE] = TPS_COMBATEMOTES_DIR "prone\\doubletime-right.txt";

	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_GETDOWN][TPS_HAND_LEFT][TPS_MIXINPOSE_STAND] = TPS_COMBATEMOTES_DIR "getdown-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_GETDOWN][TPS_HAND_RIGHT][TPS_MIXINPOSE_STAND]  = TPS_COMBATEMOTES_DIR "getdown-right.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_GETDOWN][TPS_HAND_LEFT][TPS_MIXINPOSE_PRONE] = TPS_COMBATEMOTES_DIR "prone\\getdown-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_GETDOWN][TPS_HAND_RIGHT][TPS_MIXINPOSE_PRONE]  = TPS_COMBATEMOTES_DIR "prone\\getdown-right.txt";

	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_HALT][TPS_HAND_LEFT][TPS_MIXINPOSE_STAND]  = TPS_COMBATEMOTES_DIR "haltstand-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_HALT][TPS_HAND_RIGHT][TPS_MIXINPOSE_STAND]  = TPS_COMBATEMOTES_DIR "haltstand-right.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_HALT][TPS_HAND_LEFT][TPS_MIXINPOSE_PRONE]  = TPS_COMBATEMOTES_DIR "prone\\halt-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_HALT][TPS_HAND_RIGHT][TPS_MIXINPOSE_PRONE]  = TPS_COMBATEMOTES_DIR "prone\\halt-right.txt";

	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_MOVEOUT][TPS_HAND_LEFT][TPS_MIXINPOSE_STAND]  = TPS_COMBATEMOTES_DIR "moveout-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_MOVEOUT][TPS_HAND_RIGHT][TPS_MIXINPOSE_STAND]  = TPS_COMBATEMOTES_DIR "moveout-right.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_MOVEOUT][TPS_HAND_LEFT][TPS_MIXINPOSE_PRONE]  = TPS_COMBATEMOTES_DIR "prone\\moveout-left.txt";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_MOVEOUT][TPS_HAND_RIGHT][TPS_MIXINPOSE_PRONE]  = TPS_COMBATEMOTES_DIR "prone\\moveout-right.txt";

	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_AMMO][TPS_HAND_LEFT][TPS_MIXINPOSE_STAND]  = "";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_AMMO][TPS_HAND_RIGHT][TPS_MIXINPOSE_STAND]  = "";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_AMMO][TPS_HAND_LEFT][TPS_MIXINPOSE_PRONE]  = "";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_AMMO][TPS_HAND_RIGHT][TPS_MIXINPOSE_PRONE]  = "";

	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_JUMP][TPS_HAND_LEFT][TPS_MIXINPOSE_STAND]  = "";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_JUMP][TPS_HAND_RIGHT][TPS_MIXINPOSE_STAND]  = "";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_JUMP][TPS_HAND_LEFT][TPS_MIXINPOSE_PRONE]  = "";
	fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_JUMP][TPS_HAND_RIGHT][TPS_MIXINPOSE_PRONE]  = "";


	// full body emotes
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_SALUTE_GERMAN] = TPS_BODYEMOTES_DIR "salute_german.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_SALUTE_FRENCH] = TPS_BODYEMOTES_DIR "salute_french.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_SALUTE_BRITISH] = TPS_BODYEMOTES_DIR "salute_british.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_SALUTE_DEFAULT] = TPS_BODYEMOTES_DIR "salute1.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_SURRENDER] = TPS_BODYEMOTES_DIR "monty.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_CHEER] = TPS_BODYEMOTES_DIR "cheer1.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_LAUGH] = TPS_BODYEMOTES_DIR "laugh1.txt";
//	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_DRESSDOWN] = TPS_BODYEMOTES_DIR "officer_chew1.txt";
	fpTPSBodyEmoteFiles[TPS_BODYEMOTE_ATTENTION] = TPS_BODYEMOTES_DIR "attention.txt";
	*/

	// end MIXINS

	fpTPSDefaultFaceNames[BRITISH_INFANTRY] = "face1";

	fpTPSDefaultFaceNames[GERMAN_INFANTRY] = "face2";
	fpTPSDefaultFaceNames[FRENCH_INFANTRY] = "face3";
	fpTPSDefaultFaceNames[BRITISH_HC] = "britishofficerhead";
	fpTPSDefaultFaceNames[GERMAN_HC] = "frenchofficerhead";
	fpTPSDefaultFaceNames[FRENCH_HC] = "germanofficerhead";
	fpTPSDefaultFaceNames[BRITISH_PARA_PACK] = "16";
	fpTPSDefaultFaceNames[GERMAN_PARA_PACK] = "20";
	fpTPSDefaultFaceNames[FRENCH_PARA_PACK] = "16";
	fpTPSDefaultFaceNames[BRITISH_PARA_NOPACK] = "16";
	fpTPSDefaultFaceNames[GERMAN_PARA_NOPACK] = "20";
	fpTPSDefaultFaceNames[FRENCH_PARA_NOPACK] = "16";
	fpTPSDefaultFaceNames[BRITISH_PARA_PACK_SELF] = "16";
	fpTPSDefaultFaceNames[GERMAN_PARA_PACK_SELF] = "20";
	fpTPSDefaultFaceNames[FRENCH_PARA_PACK_SELF] = "16";
	fpTPSDefaultFaceNames[BRITISH_ML] = "face1";
	fpTPSDefaultFaceNames[GERMAN_ML] = "06";
	fpTPSDefaultFaceNames[FRENCH_ML] = "16";
	fpTPSDefaultFaceNames[AMERICAN_INFANTRY] = "face1";

#ifdef USE_NEW_DECALS
    int q = 0;
    for(int i = 0; i < NUM_INFANTRY_TYPES; i++)
    {
	    fpTPSPatchTextureNames[q++] = "pers1";
	    fpTPSPatchTextureNames[q++] = "affb1";
	    fpTPSPatchTextureNames[q++] = "affs1";
    }
#else
	fpTPSPatchTextureNames[BRITISH_INFANTRY] = "brit5";
	fpTPSPatchTextureNames[GERMAN_INFANTRY] = "germ5";
	fpTPSPatchTextureNames[FRENCH_INFANTRY] = "f4";
	fpTPSPatchTextureNames[BRITISH_HC] = "";
	fpTPSPatchTextureNames[GERMAN_HC] = "";
	fpTPSPatchTextureNames[FRENCH_HC] = "";
	fpTPSPatchTextureNames[BRITISH_PARA_PACK] = "";
	fpTPSPatchTextureNames[GERMAN_PARA_PACK] = "";
	fpTPSPatchTextureNames[FRENCH_PARA_PACK] = "";
	fpTPSPatchTextureNames[BRITISH_PARA_NOPACK] = "";
	fpTPSPatchTextureNames[GERMAN_PARA_NOPACK] = "";
	fpTPSPatchTextureNames[FRENCH_PARA_NOPACK] = "";
	fpTPSPatchTextureNames[BRITISH_PARA_PACK_SELF] = "";
	fpTPSPatchTextureNames[GERMAN_PARA_PACK_SELF] = "";
	fpTPSPatchTextureNames[FRENCH_PARA_PACK_SELF] = "";
	fpTPSPatchTextureNames[BRITISH_ML] = "brit5";
	fpTPSPatchTextureNames[GERMAN_ML] = "germ5";
	fpTPSPatchTextureNames[FRENCH_ML] = "";
#endif

	fpTPSParachuteFile = TPS_MESH_DIR "chute\\chute";

	fpTPSDataReadTPSFile();
	fpTPSDataReadSurfaceFiles();
	fpTPSDataInitHitLocations();

	tpsTroopData.deathSounds[0] = "sounds\\troop\\tps_death\\tps_death_1.wav";
	tpsTroopData.deathSounds[1] = "sounds\\troop\\tps_death\\tps_death_2.wav";
	tpsTroopData.deathSounds[2] = "sounds\\troop\\tps_death\\tps_death_3.wav";
	tpsTroopData.deathSounds[3] = "sounds\\troop\\tps_death\\tps_death_4.wav";
	tpsTroopData.deathSounds[4] = "sounds\\troop\\tps_death\\tps_death_5.wav";
	tpsTroopData.deathSounds[5] = "sounds\\troop\\tps_death\\tps_death_6.wav";
	tpsTroopData.deathSounds[6] = "sounds\\troop\\tps_death\\tps_death_7.wav";
	tpsTroopData.deathSounds[7] = "sounds\\troop\\tps_death\\tps_death_8.wav";
	tpsTroopData.deathSounds[8] = "sounds\\troop\\tps_death\\tps_death_9.wav";
	tpsTroopData.deathSounds[9] = "sounds\\troop\\tps_death\\tps_death_10.wav";
}

int fpTPSGetTypeIndexFromValue(const char* type, const char* name)
{
	if(!stricmp("pose", type))
	{
		for(int i = 0; i < TPS_POSETYPE_NUM_TYPES; i++)
		{
			if(fpTPSPoseTypeStrings[i] && !stricmp(name, fpTPSPoseTypeStrings[i]))
				return i;
		}
	}
	else if(!stricmp("move", type))
	{
		for(int i = 0; i < TPS_MOVETYPE_NUM_TYPES; i++)
		{
			if(fpTPSMoveTypeStrings[i] && !stricmp(name, fpTPSMoveTypeStrings[i]))
				return i;
		}
	}

	return -1;
}

const char* fpTPSGetTypeNameFromValue(const char* type, int value)
{
	static char buffer[25];

	*buffer = 0;

	if(tpsTypesMap.size())
	{
		if(tpsTypesMap.find(type) != tpsTypesMap.end())
		{
			TPSType& tpsType = tpsTypesMap[type];
			TPSType::const_iterator it = tpsType.begin();

			for(; it != tpsType.end(); ++it)
			{
				if(it->second == value)
				{
					strcpy(buffer, it->first.c_str());
					break;
				}
			}
		}
	}

	if(!buffer[0])
		snprintf(buffer, 24, "%d", value);

	return buffer;
}


void fpTPSDataReadTPSFile()
{
	SINT32	tpsDataFile;
	static char	buf[255];
	static char	troopFileName[255];
	char*	varName, *varData;

	memset (&tpsTroopData, 0, sizeof (tpsTroopData));

	strcpy (troopFileName, TPS_DATA_DIR);
	strcat (troopFileName, TPS_DATA_FILE);

	tpsDataFile = fioOpen(troopFileName);

	if (tpsDataFile == -1)
	{
		// BAD!
		return;
	}

	while (fioGetString(tpsDataFile, (char*)buf, 255) != 0)
	{
		if (buf[0] == '#')
		{
			continue;
		}

		fpFPSDataGetVarnameAndContents(&varName, &varData, buf);

		if ((varName == nullptr) || (varData == nullptr))
			continue;

		if (stricmp(buf, "toProneInTime") == 0)
			tpsTroopData.toProneInTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "toProneOutTime") == 0)
			tpsTroopData.toProneOutTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "toProneWeight") == 0)
			tpsTroopData.toProneWeight = (FLOAT32) atof (varData);
		else if (stricmp(buf, "toStandInTime") == 0)
			tpsTroopData.toStandInTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "toStandOutTime") == 0)
			tpsTroopData.toStandOutTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "toStandWeight") == 0)
			tpsTroopData.toStandWeight = (FLOAT32) atof (varData);
		else if (stricmp(buf, "emoteCombatMaxDistance") == 0)
			tpsTroopData.emoteCombatMaxDistance = (FLOAT32) atof (varData);
		else if (stricmp(buf, "emoteFullBodyMaxDistance") == 0)
			tpsTroopData.emoteFullBodyMaxDistance = (FLOAT32) atof (varData);
		else if (stricmp(buf, "waterSoundHalfDistance") == 0)
			tpsTroopData.waterSoundHalfDistance = (FLOAT32) atof (varData);
		else if (stricmp(buf, "waterSoundMaxDistance") == 0)
			tpsTroopData.waterSoundMaxDistance = (FLOAT32) atof (varData);

		else if (stricmp(buf, "fireSnapUpTime") == 0)
			tpsTroopData.fireSnapUpTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "fireSnapDownTime") == 0)
			tpsTroopData.fireSnapDownTime = (FLOAT32) atof (varData);
		else if (stricmp(buf, "fireHoldTime") == 0)
			tpsTroopData.fireHoldTime = (FLOAT32) atof (varData);

		else if (stricmp(buf, "deathSoundFMin") == 0)
			tpsTroopData.deathSoundFMin = (FLOAT32) atof (varData);
		else if (stricmp(buf, "deathSoundFMax") == 0)
			tpsTroopData.deathSoundFMax = (FLOAT32) atof (varData);

		else if (stricmp(buf, "boneLoc") == 0)
			fpTPSDataReadHitLocation (varData);

		else if (stricmp(buf, "waterSound") == 0)
			tpsTroopData.waterSound = (SCHAR*) strpool (varData);
	}

	fioClose (tpsDataFile);
}

void fpTPSDataReadHitLocation (SCHAR* data)
{
	SINT32	boneNum, hitLoc;

	sscanf (data, "%d %d", &boneNum, &hitLoc);

	if ((boneNum < 0) || (boneNum > 38) || (hitLoc < 0) || (hitLoc > 4))
	{
		WriteLog (LOG_FPS, LOG_INFO, "Bad data passed to fpTPSDataReadHitLocation .. >%s<", data);
		return;
	}

	fpHitMap[boneNum] = (fpHitLocations)hitLoc;
}

void fpTPSDataInitHitLocations()
{
	SINT32 boneCount = 0;

	gr2Model* skeleton = gr2Model::Load(fpTPSSkeletonFile, nullptr, &boneCount, 1);
	if(!skeleton) return;

	for(int i = 0; i < boneCount; i++)
	{
		const char* name = skeleton->grannyModel->Skeleton->Bones[i].Name;

		if(strstr(name, "Head"))
			fpHitMap[i] = FP_HIT_LOCATION_HEAD;
		else if(strstr(name, "Neck"))
			fpHitMap[i] = FP_HIT_LOCATION_HEAD;
		else if(strstr(name, "Pelvis"))
			fpHitMap[i] = FP_HIT_LOCATION_TORSO;
		else if(strstr(name, "Spine"))
			fpHitMap[i] = FP_HIT_LOCATION_TORSO;
		else if(strstr(name, "Thigh"))
			fpHitMap[i] = FP_HIT_LOCATION_LEGS;
		else if(strstr(name, "Calf"))
			fpHitMap[i] = FP_HIT_LOCATION_LEGS;
		else if(strstr(name, "Foot"))
			fpHitMap[i] = FP_HIT_LOCATION_LEGS;
		else if(strstr(name, "Toe"))
			fpHitMap[i] = FP_HIT_LOCATION_LEGS;
		else if(strstr(name, "Clavicle"))
			fpHitMap[i] = FP_HIT_LOCATION_ARMS;
		else if(strstr(name, "UpperArm"))
			fpHitMap[i] = FP_HIT_LOCATION_ARMS;
		else if(strstr(name, "Forearm"))
			fpHitMap[i] = FP_HIT_LOCATION_ARMS;
		else if(strstr(name, "Hand"))
			fpHitMap[i] = FP_HIT_LOCATION_ARMS;
		else if(strstr(name, "Finger"))
			fpHitMap[i] = FP_HIT_LOCATION_ARMS;
		else
			fpHitMap[i] = FP_HIT_LOCATION_NONE;
	}

	gr2FreeModel(skeleton);
}

void fpTPSDataReadSurfaceFiles()
{
	static char	buf[256];
	static char	surfFileName[256];
	SINT32	currFile;
	char*	varName, *varData;
	SINT32	i;

	memset (&fpTPSSurfaces, 0, sizeof (fpTPSSurfaces));

	for (i=0; i < TPS_MAX_NUM_SURFACES; i++)
	{
		snprintf(surfFileName, 255, "vehicles/ground/groundman/surfaces/surface_%02d.txt", i);

		currFile = fioOpen(surfFileName);

		if (currFile == -1)
		{
//			WriteLog (LOG_FPS, LOG_INFO, "Surface File %s not found", surfFileName);
			continue;
		}

		while (fioGetString(currFile, (char*)buf, 255) != 0)
		{
			if (buf[0] == '#')
			{
				continue;
			}

			fpFPSDataGetVarnameAndContents(&varName, &varData, buf);

			if ((varName == nullptr) || (varData == nullptr))
				continue;

			if (strncasecmp(buf, "fps", 3) == 0)
			{
				if (stricmp(buf+3, "CrawlSound") == 0)
					fpTPSSurfaces[i].fpsCrawlSound = (SCHAR*) strpool (varData);
				else if (stricmp(buf+3, "CreepSound") == 0)
					fpTPSSurfaces[i].fpsCreepSound = (SCHAR*) strpool (varData);
				else if (stricmp(buf+3, "CreepAimSound") == 0)
					fpTPSSurfaces[i].fpsCreepAimSound = (SCHAR*) strpool (varData);
				else if (stricmp(buf+3, "TrotSound") == 0)
					fpTPSSurfaces[i].fpsTrotSound = (SCHAR*) strpool (varData);
				else if (stricmp(buf+3, "TrotAimSound") == 0)
					fpTPSSurfaces[i].fpsTrotAimSound = (SCHAR*) strpool (varData);
				else if (stricmp(buf+3, "SprintSound") == 0)
					fpTPSSurfaces[i].fpsSprintSound = (SCHAR*) strpool (varData);
			}
#define CRAWL_PREFIX "tpsCrawl"
#define	CRAWL_LEN (sizeof(CRAWL_PREFIX) - 1)
			else if (strncasecmp(buf, CRAWL_PREFIX, CRAWL_LEN) == 0)
			{
				// crawl
				if (stricmp(buf + CRAWL_LEN, "Sound") == 0)
				{
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_FORWARD] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_BACKWARD] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_TOSTAND] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CRAWL_LEN, "LeftStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_BACKWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_TOSTAND][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CRAWL_LEN, "RightStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_BACKWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_PRONE][TPS_MOVETYPE_TOSTAND][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CRAWL_LEN, "SoundHalfDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_PRONE][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_PRONE][TPS_MOVETYPE_BACKWARD] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_PRONE][TPS_MOVETYPE_TOSTAND] = (FLOAT32) atof (varData);
				}
				else if (stricmp(buf + CRAWL_LEN, "SoundMaxDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_PRONE][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_PRONE][TPS_MOVETYPE_BACKWARD] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_PRONE][TPS_MOVETYPE_TOSTAND] = (FLOAT32) atof (varData);
				}
			}
#define CREEP_PREFIX "tpsCreep"
#define	CREEP_LEN (sizeof(CREEP_PREFIX) - 1)
			else if (strncasecmp(buf, CREEP_PREFIX, CREEP_LEN) == 0)
			{
				// creep
				if (stricmp(buf + CREEP_LEN, "Sound") == 0)
				{
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_BACKWARD] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOPRONE] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOSTAND] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "LeftStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_BACKWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOPRONE][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOSTAND][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "RightStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_BACKWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOPRONE][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOSTAND][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "SoundHalfDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_BACKWARD] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOPRONE] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOSTAND] = (FLOAT32) atof (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "SoundMaxDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_BACKWARD] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOPRONE] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_TOSTAND] = (FLOAT32) atof (varData);
				}
				// creep aim
				else if (stricmp(buf + CREEP_LEN, "AimSound") == 0)
				{
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "AimLeftStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "AimRightStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "AimSoundHalfDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
				}
				else if (stricmp(buf + CREEP_LEN, "AimSoundMaxDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
				}
			}
#define TROT_PREFIX "tpsTrot"
#define	TROT_LEN (sizeof(TROT_PREFIX) - 1)
			else if (strncasecmp(buf, TROT_PREFIX, TROT_LEN) == 0)
			{
				// trot
				if (stricmp(buf + TROT_LEN, "Sound") == 0)
				{
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_FORWARD] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_BACKWARD] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_TOPRONE] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + TROT_LEN, "LeftStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_BACKWARD][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_TOPRONE][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + TROT_LEN, "RightStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_FORWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_BACKWARD][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					//fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_TOPRONE][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + TROT_LEN, "SoundHalfDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_BACKWARD] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_TOPRONE] = (FLOAT32) atof (varData);
				}
				else if (stricmp(buf + TROT_LEN, "SoundMaxDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_FORWARD] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_BACKWARD] = (FLOAT32) atof (varData);
					//fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_TOPRONE] = (FLOAT32) atof (varData);
				}
				// trot aim
				else if (stricmp(buf + TROT_LEN, "AimSound") == 0)
				{
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + TROT_LEN, "AimLeftStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + TROT_LEN, "AimRightStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + TROT_LEN, "AimSoundHalfDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK] = (FLOAT32) atof (varData);
				}
				else if (stricmp(buf + TROT_LEN, "AimSoundMaxDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_WALK] = (FLOAT32) atof (varData);
				}
			}
#define SPRINT_PREFIX "tpsSprint"
#define	SPRINT_LEN (sizeof(SPRINT_PREFIX) - 1)
			else if (strncasecmp(buf, SPRINT_PREFIX, SPRINT_LEN) == 0)
			{
				// sprint
				if (stricmp(buf + SPRINT_LEN, "Sound") == 0)
				{
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_SPRINT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSLoopSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_SPRINT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + SPRINT_LEN, "LeftStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_SPRINT][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_SPRINT][TPS_FOOTSTEP_LEFT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + SPRINT_LEN, "RightStep") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_STAND][TPS_MOVETYPE_SPRINT][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
					fpTPSSurfaces[i].fpTPSFootstepSounds[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_SPRINT][TPS_FOOTSTEP_RIGHT] = (SCHAR*) strpool (varData);
				}
				else if (stricmp(buf + SPRINT_LEN, "SoundHalfDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_SPRINT] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepHalfDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_SPRINT] = (FLOAT32) atof (varData);
				}
				else if (stricmp(buf + SPRINT_LEN, "SoundMaxDistance") == 0)
				{
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_STAND][TPS_MOVETYPE_SPRINT] = (FLOAT32) atof (varData);
					fpTPSSurfaces[i].fpTPSFootstepMaxDistance[TPS_POSETYPE_CROUCH][TPS_MOVETYPE_SPRINT] = (FLOAT32) atof (varData);
				}
			}
		}

		fioClose (currFile);
	}
}

fpTPSWeaponClass fpTPSConvertWeaponClass(fpTPSWeaponType type)
{
	switch (type)
	{
		case TPS_WEAPONTYPE_SMG:
		case TPS_WEAPONTYPE_LMG:
		case TPS_WEAPONTYPE_RIFLE:
		case TPS_WEAPONTYPE_PISTOL:
		case TPS_WEAPONTYPE_ATR:
			return CLASS_FIREARM;
		case TPS_WEAPONTYPE_LTMORTAR:
		case TPS_WEAPONTYPE_GRENADE:
		case TPS_WEAPONTYPE_SATCHEL:
		case TPS_WEAPONTYPE_ROCKET:
			return CLASS_EXPLOSIVE;
        default:
            // ignore non-leathal types
            break;
	}

	return CLASS_ITEM;
}

fpTPSAttackType fpTPSGetAttack(fpTPSWeaponType type)
{
	switch (type)
	{
		case TPS_WEAPONTYPE_SMG:
		case TPS_WEAPONTYPE_LMG:
			return ATTACK_AUTOMATIC;
		case TPS_WEAPONTYPE_RIFLE:
		case TPS_WEAPONTYPE_PISTOL:
		case TPS_WEAPONTYPE_ATR:
			return ATTACK_SINGLESHOT;
		case TPS_WEAPONTYPE_LTMORTAR:
		case TPS_WEAPONTYPE_ROCKET:
			return ATTACK_MORTAR;
		case TPS_WEAPONTYPE_GRENADE:
			return ATTACK_THROWN;
		case TPS_WEAPONTYPE_SATCHEL:
			return ATTACK_PLACED;
		case TPS_WEAPONTYPE_KNIFE:
			return ATTACK_MELEE;
        default:
            // ignore non-leathal types
            break;
	}

	return ATTACK_NONE;
}


fpTPSWeaponType	fpTPSConvertWeaponTypes(fpFPSWeaponType type)
{
	switch (type)
	{
		case FPS_WEAPONTYPE_SMG:
			return TPS_WEAPONTYPE_SMG;
		case FPS_WEAPONTYPE_SATCHEL:
			return TPS_WEAPONTYPE_SATCHEL;
		case FPS_WEAPONTYPE_RIFLE:
		case FPS_WEAPONTYPE_AUTO_RIFLE:
			return TPS_WEAPONTYPE_RIFLE;
		case FPS_WEAPONTYPE_AUTO_PISTOL:
		case FPS_WEAPONTYPE_REVOLVER:
			return TPS_WEAPONTYPE_PISTOL;
		case FPS_WEAPONTYPE_GRENADE:
			return TPS_WEAPONTYPE_GRENADE;
		case FPS_WEAPONTYPE_BUILD:
			return TPS_WEAPONTYPE_BUILD;
		case FPS_WEAPONTYPE_ITEM:
			return TPS_WEAPONTYPE_EMPTY;
		case FPS_WEAPONTYPE_KNIFE:
			return TPS_WEAPONTYPE_KNIFE;
		case FPS_WEAPONTYPE_LMG:
			return TPS_WEAPONTYPE_LMG;
		case FPS_WEAPONTYPE_LTMORTAR:
			return TPS_WEAPONTYPE_LTMORTAR;
		case FPS_WEAPONTYPE_ATR:
			return TPS_WEAPONTYPE_ATR;
		case FPS_WEAPONTYPE_ROCKET:
			return TPS_WEAPONTYPE_ROCKET;
        default:
            // ignore non-leathal types
            break;
	}

	WriteLog (LOG_FPS, LOG_INFO, "fpTPSConvertWeaponTypes: Invalid weapon type passed %d!", type);
	//piExitProgram (_FL, "fpTPSConvertWeaponTypes: Invalid weapon type passed" );

	//dumb
	return TPS_WEAPONTYPE_EMPTY;
}

// emote 0 is no emote
// emote 1 -> TPS_COMBATEMOTE_NUM_TYPES is a combat emote
// emote
fpTPSBodyEmoteType fpTPSGetEmoteType (SINT32 emoteID)
{
	if (emoteID > 0)
	{
		if (emoteID > TPS_COMBATEMOTE_NUM_TYPES)
		{
			if (emoteID > (TPS_COMBATEMOTE_NUM_TYPES+TPS_BODYEMOTE_NUM_TYPES))
				return (fpTPSBodyEmoteType)TPS_EMOTETYPE_NONE;
			else
				return (fpTPSBodyEmoteType)TPS_EMOTETYPE_FULLBODY;
		}
		else
			return (fpTPSBodyEmoteType)TPS_EMOTETYPE_COMBAT;
	}
	return (fpTPSBodyEmoteType)TPS_EMOTETYPE_NONE;
}
