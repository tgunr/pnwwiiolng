#include <syscfg.h> // Path is project specific.

#include "include/vars.h"

#include "fp/fpFPSdata.h"

#include "gr2/gr2Anim.h"
#include "gr2/gr2layer.h"
#include "fp/fpTPS.h"
#include "fp/fpTPSdata.h"
#include "fp/fpFPSinternal.h"
#include "ld/ldpix.h"
#include "veh/vehmain.h"
#include "fio/fio.h"

#define DUMP_TPS_DATA_TO_XML
#undef DUMP_TPS_DATA_TO_XML

#ifdef DUMP_TPS_DATA_TO_XML
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#define XML_DUMP_FILE "c:\\downloads\\tps_dump.txt"
static void fpTPSDumpStates();
#endif // DUMP_TPS_DATA_TO_XML

#define MAX_PATH_SIZE 255

struct gr2AnimationBlend*	fpTPSBasePose;
struct gr2AnimationBlend*	fpTPSCombatBlend;
//struct gr2AnimationBlend*	fpTPSAnimLib[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES][NUM_WEAPONS][TPS_POINTTYPE_NUM_TYPES];
struct gr2AnimationBlend*	fpTPSDeathBlends[TPS_POSETYPE_NUM_TYPES][TPS_NUM_DEATHS];
struct gr2AnimationBlend*	fpTPSTwitchBlends[TPS_POSETYPE_NUM_TYPES][MAX_TWITCHES_PER_POSE];
struct gr2AnimationBlend*	fpTPSCombatEmotes[TPS_COMBATEMOTE_NUM_TYPES][TPS_HAND_NUM_TYPES][TPS_MIXINPOSE_NUM_TYPES];
struct gr2AnimationBlend*	fpTPSBodyEmotes[TPS_BODYEMOTE_NUM_TYPES];
SINT32 fpTPSNumIdleAnims[TPS_POSETYPE_NUM_TYPES][TPS_WEAPONTYPE_NUM_TYPES];

FLOAT32 fpTPSBodyEmoteLengths[TPS_BODYEMOTE_NUM_TYPES];

static gr2Model* faceModel;
static gr2Model* _personalizedModel = nullptr;
static TPS_BLEND_MAP _fpTPSAnimLib; // using tps weapon types
static SINT32 tpsBlendTypeBits, tpsBlendWeapBits, tpsBlendPoseBits, tpsBlendMoveBits, tpsBlendPointBits;

void fpTPSFireRound(void* FireVehicle)
{
	if(!FireVehicle)
		return;

	vehVehicle* v = (vehVehicle*)FireVehicle;

	if(!v->IsMe())
		gr2AnimVisualAttack(v->GetAnim(), TPS_POINTTYPE_ATTACK1);
}

extern void rnUpdateLoadingScreen();

// this is the precache stage
// loads models with instantiate set to 0
// loads animations into appropriate place
// ANYTHING added here will need a cleanup in the kill function
LOGICAL fpTPSLoad()
{
	int			i, j, k, l;
	static int			boneCount;

	fpTPSDataInit();

	rnUpdateLoadingScreen();

	for (tpsBlendWeapBits = 31; tpsBlendWeapBits >= 0; tpsBlendWeapBits--)
	{
		if ((1<<tpsBlendWeapBits) & (NUM_WEAPONS-1))
		{
			tpsBlendWeapBits++;
			break;
		}
	}

	for (tpsBlendPoseBits = 31; tpsBlendPoseBits >= 0; tpsBlendPoseBits--)
	{
		if ((1<<tpsBlendPoseBits) & (TPS_POSETYPE_NUM_TYPES-1))
		{
			tpsBlendPoseBits++;
			break;
		}
	}

	for (tpsBlendMoveBits = 31; tpsBlendMoveBits >= 0; tpsBlendMoveBits--)
	{
		if ((1<<tpsBlendMoveBits) & (TPS_MOVETYPE_NUM_TYPES-1))
		{
			tpsBlendMoveBits++;
			break;
		}
	}

	for (tpsBlendPointBits = 31; tpsBlendPointBits >= 0; tpsBlendPointBits--)
	{
		if ((1<<tpsBlendPointBits) & (TPS_POINTTYPE_NUM_TYPES-1))
		{
			tpsBlendPointBits++;
			break;
		}
	}

	tpsBlendTypeBits = TPS_BLENDTYPE_NUM_TYPES;

	Write_Log (LOG_FPS, LOG_INFO, "Blend type bits used = %d",
		tpsBlendTypeBits + tpsBlendWeapBits + tpsBlendPoseBits + tpsBlendMoveBits + tpsBlendPointBits);


	// load, don't instantiate, and get the bone count of the skeleton
	gr2Model* skeleton = gr2Model::Load(fpTPSSkeletonFile, nullptr, &boneCount, 1);

	rnUpdateLoadingScreen();

	// load, don't instantiate, and don't get the bone count of the damage model
	gr2Model::Load(fpTPSDamageFile, skeleton, nullptr, 0);

	rnUpdateLoadingScreen();

	// load, don't instantiate, and don't get the bone count of the trooper and weapon meshes
	for (i=0; i < NUM_INFANTRY_TYPES; i++)
	{
		for (j=0; j < TPS_NUM_LODS; j++)
		{
			if (fpTPSInfantryMeshFiles[i][j][0] != 0)
			{
//				Write_Log (LOG_FPS, LOG_INFO, "Country %d, LOD %d, loading mesh %s", i, j, fpTPSInfantryMeshFiles[i][j]);
				gr2Model::Load(fpTPSInfantryMeshFiles[i][j], skeleton, nullptr, 0, -1, (j < 2));
			}
		}

		rnUpdateLoadingScreen();
	}

	// load, don't instantiate all the weapons
	for (i=0; i < NUM_WEAPONS; i++)
	{
		for (j=0; j < TPS_NUM_LODS; j++)
		{
			if (fpTPSWeaponMeshFiles[i][j][0] != 0)
			{
//				Write_Log (LOG_FPS, LOG_INFO, "Weapon %d, LOD %d, loading weapon mesh %s", i, j, fpTPSWeaponMeshFiles[i][j]);
				gr2Model::Load(fpTPSWeaponMeshFiles[i][j], nullptr, nullptr, 0, -1, (j < 2));
			}

			if (fpTPSWeaponMeshAltFiles[i][j][0] != 0)
			{
//				Write_Log (LOG_FPS, LOG_INFO, "Weapon %d, LOD %d, loading alt weapon mesh %s", i, j, fpTPSWeaponMeshAltFiles[i][j]);
				gr2Model::Load(fpTPSWeaponMeshAltFiles[i][j], nullptr, nullptr, 0, -1, (j < 2));
			}
		}
		rnUpdateLoadingScreen();
	}

	// and finally, the chute model
	gr2Model::Load(fpTPSParachuteFile, nullptr, nullptr, 0);

	// idle animations
	memset(fpTPSNumIdleAnims, 0, sizeof (fpTPSNumIdleAnims));

	UINT32 usedBlends = 0;
	UINT32 unusedBlends = 0;

	TPS_BLEND_MAP::iterator foundBlend;

	// tps blends
	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_MOVETYPE_NUM_TYPES; j++)
		{
			for (k=0; k < TPS_WEAPONTYPE_NUM_TYPES; k++)
			{
				for (l=0; l < TPS_POINTTYPE_NUM_TYPES; l++)
				{
					bool exists = false;
					bool added	= false;

					const char* key = fpTPSCreateBlendKey(i, j, k, l, TPS_BLENDTYPE_TPS, &exists);

					if(exists)
					{
						UINT64 blendID = fpTPSCreateBlendID(i, j, k, l, TPS_BLENDTYPE_TPS);

						foundBlend = _fpTPSAnimLib.find(blendID);

						if(foundBlend == _fpTPSAnimLib.end())
						{
							_fpTPSAnimLib[blendID] = gr2LoadBlendedAnimation((SCHAR*)key, boneCount);
							added = true;
						}
					}

					if(added)
						usedBlends++;
					else
						unusedBlends++;
				}
			}
		}

		rnUpdateLoadingScreen();
	}

	// weapon blends
	if(true)
	{
		for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
		{
			for (j=0; j < TPS_MOVETYPE_NUM_TYPES; j++)
			{
				for (k=0; k < NUM_WEAPONS; k++)
				{
					for (l=0; l < TPS_POINTTYPE_NUM_TYPES; l++)
					{
						bool exists = false;
						bool added	= false;

						const char* key = fpTPSCreateBlendKey(i, j, k, l, TPS_BLENDTYPE_FPS, &exists);

						if(exists)
						{
							UINT64 blendID = fpTPSCreateBlendID(i, j, k, l, TPS_BLENDTYPE_FPS);

							foundBlend = _fpTPSAnimLib.find(blendID);

							if(foundBlend == _fpTPSAnimLib.end())
							{
								_fpTPSAnimLib[blendID] = gr2LoadBlendedAnimation((SCHAR*)key, boneCount);
								added = true;
							}
						}

						if(added)
							usedBlends++;
						else
							unusedBlends++;
					}
				}
			}

			rnUpdateLoadingScreen();
		}
	}

	// mixins
	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_NUM_DEATHS; j++)
		{
			if (fpTPSDeathFiles[i][j] && (fpTPSDeathFiles[i][j][0] != 0))
			{
				fpTPSDeathBlends[i][j] = gr2LoadBlendedAnimation (fpTPSDeathFiles[i][j], boneCount);
			}
			else
			{
				fpTPSDeathBlends[i][j] = nullptr;
			}
		}

		for (j=0; j < MAX_TWITCHES_PER_POSE; j++)
		{
			if (fpTPSTwitchFiles[i][j][0] != 0)
			{
//				Write_Log (LOG_FPS, LOG_INFO, "Pose %d, twitch %d, loading blend %s", i, j, fpTPSTwitchFiles[i][j]);
				fpTPSTwitchBlends[i][j] = gr2LoadBlendedAnimation (fpTPSTwitchFiles[i][j], boneCount);
				if (fpTPSTwitchBlends[i][j] == nullptr)
				{
					char errorMessage[255];

					snprintf (errorMessage, 254, "ERROR loading blend %s", fpTPSTwitchFiles[i][j]);
					Write_Log (LOG_FPS, LOG_INFO, "%s", errorMessage);
					piExitProgram (_FL, errorMessage);
				}
			}
			else
			{
				fpTPSTwitchBlends[i][j] = nullptr;
			}
		}

		rnUpdateLoadingScreen();
	}

	for (i=0; i < TPS_COMBATEMOTE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_HAND_NUM_TYPES; j++)
		{
			for (k=0; k < TPS_MIXINPOSE_NUM_TYPES; k++)
			{
				if (false && fpTPSCombatEmoteFiles[i][j][k][0])
				{
					fpTPSCombatEmotes[i][j][k] = gr2LoadBlendedAnimation (fpTPSCombatEmoteFiles[i][j][k], boneCount);
				}
				else
				{
					fpTPSCombatEmotes[i][j][k] = nullptr;
				}
			}
		}
		rnUpdateLoadingScreen();
	}

	for (i=0; i < TPS_BODYEMOTE_NUM_TYPES; i++)
	{
		if (false && fpTPSBodyEmoteFiles[i][0])
		{
			fpTPSBodyEmotes[i] = gr2LoadBlendedAnimation (fpTPSBodyEmoteFiles[i], boneCount);
			fpTPSBodyEmoteLengths[i] = gr2BlendGetBaseLength (fpTPSBodyEmotes[i]);
		}
		else
		{
			fpTPSBodyEmotes[i] = nullptr;
			fpTPSBodyEmoteLengths[i] = 0;
		}
		rnUpdateLoadingScreen();
	}

	fpTPSBodyEmoteLengths[TPS_BODYEMOTE_ATTENTION] = 4.0f;

	// base pose
	fpTPSBasePose = gr2LoadBlendedAnimation (fpTPSBasePoseFile, boneCount);

	// combat stance
	fpTPSCombatBlend = gr2LoadBlendedAnimation (fpTPSCombatBlendFile, boneCount);

    SINT32 num = 0;

#ifdef USE_NEW_DECALS
    num = NUM_INFANTRY_TYPES * 3;
#else
    num = NUM_INFANTRY_TYPES;
#endif

	// identify patch textures
	for (i=0; i < num; i++)
	{
		if (false && fpTPSPatchTextureNames[i][0])
		{
			fpTPSPatchTextureIDs[i] = piGetNamedTexture(fpTPSPatchTextureNames[i]);
			if (fpTPSPatchTextureIDs[i] < 0)
			{
				char errorMessage[255];

				snprintf (errorMessage, 254, "ERROR finding patch %s", fpTPSPatchTextureNames[i]);
				Write_Log (LOG_FPS, LOG_INFO, "%s", errorMessage);
				piExitProgram (_FL, errorMessage);
			}
		}
		else
			fpTPSPatchTextureIDs[i] = -1;

		rnUpdateLoadingScreen();
	}

	faceModel = gr2LoadModel("TPS/meshes/faces/faces", nullptr, nullptr, 1);

	rnUpdateLoadingScreen();

#ifdef REPLACE_MODEL_TEXTURES
    // Load textures for texture replacement
    _personalizedModel = gr2LoadModel("TPS/meshes/infset/infset", nullptr, nullptr, 1);
#endif

	rnUpdateLoadingScreen();

	/*
	faceIDs = gr2ModelGetLoadedTextures (faceModel, &fpTPSNumFaces);
	if (fpTPSNumFaces > MAX_FACES)
	{
		char errorMessage[255];

		snprintf (errorMessage, 254, "ERROR too many faces in face file .. %d > %d", fpTPSNumFaces, MAX_FACES);
		Write_Log (LOG_FPS, LOG_INFO, "%s", errorMessage);
		piExitProgram (_FL, errorMessage);
	}
	memcpy (fpTPSFaceTexIDs, faceIDs, sizeof(SINT32)*fpTPSNumFaces);
	*/

#ifdef DUMP_TPS_DATA_TO_XML
	fpTPSDumpStates();
#endif // DUMP_TPS_DATA_TO_XML

	gr2FreeModel(skeleton);

	return TRUE;
}

// kill the TPS system
void fpTPSKill()
{
	TPS_BLEND_MAP::iterator it = _fpTPSAnimLib.begin();

	for(; it != _fpTPSAnimLib.end(); ++it)
		gr2FreeBlendedAnimation(it->second);

	_fpTPSAnimLib.clear();

	int i, j, k;

	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_NUM_DEATHS; j++)
		{
			if (fpTPSDeathFiles[i][j] == nullptr)
				continue;

			if (fpTPSDeathFiles[i][j][0] != 0)
			{
				gr2FreeBlendedAnimation (fpTPSDeathBlends[i][j]);
			}
		}
	}

	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < MAX_TWITCHES_PER_POSE; j++)
		{
			if (fpTPSTwitchBlends[i][j])
			{
				gr2FreeBlendedAnimation(fpTPSTwitchBlends[i][j]);
			}
		}
	}

	for (i=0; i < TPS_COMBATEMOTE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_HAND_NUM_TYPES; j++)
		{
			for (k=0; k < TPS_MIXINPOSE_NUM_TYPES; k++)
			{
				gr2FreeBlendedAnimation(fpTPSCombatEmotes[i][j][k]);
			}
		}
	}

	for (i=0; i < TPS_BODYEMOTE_NUM_TYPES; i++)
	{
		gr2FreeBlendedAnimation(fpTPSBodyEmotes[i]);
	}

	gr2FreeBlendedAnimation(fpTPSBasePose);
	gr2FreeBlendedAnimation(fpTPSCombatBlend);

	gr2FreeModel (faceModel);

    if(_personalizedModel)
    {
        gr2FreeModel(_personalizedModel);
    }
}

#ifdef DUMP_TPS_DATA_TO_XML
void fpTPSDumpStates()
{
	int fd = OS_OPEN(XML_DUMP_FILE, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD | S_IWRITE);
	if(fd < 0)
		return;

	char line[1024];
	int i, j, k, l;

	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_MOVETYPE_NUM_TYPES; j++)
		{
			for (k=0; k < TPS_WEAPONTYPE_NUM_TYPES; k++)
			{
				for (l=0; l < TPS_POINTTYPE_NUM_TYPES; l++)
				{
					if(fpTPSAnimLib[i][j][k][l])
					{
						*line = 0;
						sprintf(line, "<blend type=\"troop\" path=\"%s\">\n",gr2GetPathFromBlend(fpTPSAnimLib[i][j][k][l]));
						OS_WRITE(fd, line, strlen(line));
						sprintf(line, "\t<state name=\"pose\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("pose", i));
						OS_WRITE(fd, line, strlen(line));
						sprintf(line, "\t<state name=\"move\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("move", j));
						OS_WRITE(fd, line, strlen(line));
						sprintf(line, "\t<state name=\"weapon\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("weapon", k));
						OS_WRITE(fd, line, strlen(line));
						sprintf(line, "\t<state name=\"pointtype\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("pointtype", l));
						OS_WRITE(fd, line, strlen(line));
						sprintf(line, "</blend>\n");
						OS_WRITE(fd, line, strlen(line));
					}
				}
			}
		}
	}

	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < MAX_TWITCHES_PER_POSE; j++)
		{
			if(fpTPSTwitchBlends[i][j])
			{
				*line = 0;
				sprintf(line, "<blend type=\"twitch\" path=\"%s\">\n",gr2GetPathFromBlend(fpTPSTwitchBlends[i][j]));
				OS_WRITE(fd, line, strlen(line));
				sprintf(line, "\t<state name=\"pose\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("pose", i));
				OS_WRITE(fd, line, strlen(line));
				sprintf(line, "\t<state name=\"twitch\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("twitch", j));
				OS_WRITE(fd, line, strlen(line));
				sprintf(line, "</blend>\n");
				OS_WRITE(fd, line, strlen(line));
			}
		}
	}

	for (i=0; i < TPS_POSETYPE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_NUM_DEATHS; j++)
		{
			if(fpTPSDeathBlends[i][j])
			{
				*line = 0;
				sprintf(line, "<blend type=\"death\" path=\"%s\">\n",gr2GetPathFromBlend(fpTPSDeathBlends[i][j]));
				OS_WRITE(fd, line, strlen(line));
				sprintf(line, "\t<state name=\"pose\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("pose", i));
				OS_WRITE(fd, line, strlen(line));
				sprintf(line, "\t<state name=\"death\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("death", j));
				OS_WRITE(fd, line, strlen(line));
				sprintf(line, "</blend>\n");
				OS_WRITE(fd, line, strlen(line));
			}
		}
	}

	for (i=0; i < TPS_COMBATEMOTE_NUM_TYPES; i++)
	{
		for (j=0; j < TPS_HAND_NUM_TYPES; j++)
		{
			for (k=0; k < TPS_MIXINPOSE_NUM_TYPES; k++)
			{
				if(fpTPSCombatEmotes[i][j][k])
				{
					*line = 0;
					sprintf(line, "<blend type=\"combatemote\" path=\"%s\">\n",gr2GetPathFromBlend(fpTPSCombatEmotes[i][j][k]));
					OS_WRITE(fd, line, strlen(line));
					sprintf(line, "\t<state name=\"combatemote\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("combatemote", i));
					OS_WRITE(fd, line, strlen(line));
					sprintf(line, "\t<state name=\"hand\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("hand", j));
					OS_WRITE(fd, line, strlen(line));
					sprintf(line, "\t<state name=\"mixin\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("mixin", k));
					OS_WRITE(fd, line, strlen(line));
					sprintf(line, "</blend>\n");
					OS_WRITE(fd, line, strlen(line));
				}
			}
		}
	}

	for (i=0; i < TPS_BODYEMOTE_NUM_TYPES; i++)
	{
		if(fpTPSBodyEmotes[i])
		{
			*line = 0;
			sprintf(line, "<blend type=\"bodyemote\" path=\"%s\">\n",gr2GetPathFromBlend(fpTPSBodyEmotes[i]));
			OS_WRITE(fd, line, strlen(line));
			sprintf(line, "\t<state name=\"bodyemote\" value=\"%s\"/>\n",fpTPSGetTypeNameFromValue("bodyemote", i));
			OS_WRITE(fd, line, strlen(line));
			sprintf(line, "</blend>\n");
			OS_WRITE(fd, line, strlen(line));
		}
	}

	OS_CLOSE(fd);
}
#endif

const char* fpTPSCreateBlendKey(int pose, int move, int weap, int point, int type, bool* exists)
{
	static char key[128+1];

	const char* weapStr = (type == TPS_BLENDTYPE_FPS) ? fpFPSWeaponKeyStrings[weap] : fpTPSWeaponKeyStrings[weap];

	snprintf(key, 128, "gr2Data\\TPS\\blends\\%s_%s_%s_%s.txt",
					fpTPSPoseKeyStrings[pose],
					fpTPSMoveKeyStrings[move],
					weapStr,
					fpTPSPointKeyStrings[point]);

	if(exists)
		*exists = (fioAccess(key)) ? true: false;

	return strstr(key, "T");
}

UINT64 fpTPSCreateBlendID(int pose, int move, int weap, int point, int type)
{
	UINT64 ID = 0;

	ID |= type;

	ID <<= tpsBlendPoseBits;
	ID |= pose;

	ID <<= tpsBlendMoveBits;
	ID |= move;

	ID <<= tpsBlendWeapBits;
	ID |= weap;

	ID <<= tpsBlendPointBits;
	ID |= point;

	return ID;
}

#ifdef GR2_BLEND_EDITOR
TPS_BLEND_MAP& fpTPSGetBlends()
{
	return _fpTPSAnimLib;
}
#endif

gr2AnimationBlend* fpTPSGetBlend(int pose, int move, int weap, int point, int type)
{
	UINT64 id = fpTPSCreateBlendID(pose, move, weap, point, type);

	TPS_BLEND_MAP::iterator found = _fpTPSAnimLib.find(id);

	if(found != _fpTPSAnimLib.end())
		return found->second;

#ifndef GR2_BLEND_EDITOR
	if(type == TPS_BLENDTYPE_TPS)
	{
		id = fpTPSCreateBlendID(pose, move, TPS_WEAPONTYPE_ANY, point, type);

		found = _fpTPSAnimLib.find(id);

		if(found != _fpTPSAnimLib.end())
			return found->second;
	}
#endif

	return nullptr;
}

