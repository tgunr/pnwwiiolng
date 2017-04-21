#include <syscfg.h> // Path is project specific.

#include <math.h>
#include <ctype.h>

#include "pi/pi.h"

#include "weap/weapammo.h"
#include "weap/weapfx.h"
#include "fx/FXInterface.h" // remove this when we go to new code that just uses weapfx

#include "fio/fio.h"
#include "ml/mlobject.h"
#include "ml/mlutility.h"
#include "ml/mlAtmo.h"
#include "ml/mltypes.h"

#include "ogrp/ogrptype.h"
#include "ob/obmain.h"
#include "ob/obpool.h"
#include "ob/obplyr.h"
#include "ob/obdraw.h"
#include "ob/obdlite.h"

#include "trn/trnop.h"
#include "trn/trncd.h"
#include "trn/trnmain.h"
#include "trn/trnautogun.h"
#include "trn/trnwtr.h"

#include "math/mathvect.h"
#include "math/matheul.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "mem/mem.h"

#include "draw/draw3d.h"
#include "ld/ldpix.h"

#include "veh/vehmain.h"
#include "veh/vehanim.h"

#include "pers/persmain.h"

#include "gr2/gr2Anim.h"
#include "gr2/gr2layer.h"
#include "fp/fpTPS.h"
#include "fp/fpTPSPhysics.h"
#include "fp/fpFPS.h"
#include "fx/fxSnd.h"

#include <list>
#include "ob/obMgr.h"

#include "physics/physicsMain.h"
#include "client/clientSTO.h"
#include "client/GameClient.h"
#include "tut/TutorialManager.h"
#include "fx/TransformEffect.h"
#include "trn/trnvars.h"
#include "random.h"

#define USE_FX2 1

#define GREN_STAT_MODEL 1

FILE* weaplogFile;

/************************************************************************************************
	weapammo
*************************************************************************************************/

#define PROJECTILE_DELTA_SANITY_TIME 3.0
#define MAX_TRACERS_VIS 256
#define WEAP_AMMO_MAXNUM_AMMO 1024
#define _TRACER_SCALE 2.0f
#define _MAX_NUM_OF_COLLISIONS 32
#define MAX_TORPEDO_DROP_SPEED 75

struct TracerFX
{
	FLOAT64VECTOR OldPos;
	FLOAT64VECTOR NewPos;
	FLOAT32 Diameter;
};

struct shrapTarget
{
	FLOAT64VECTOR	targetPos;
	FLOAT64			radius;
	void*			target;
	FLOAT32			maxDist2;
};

// Ammo variable types
enum
{
	WEAP_AMMOVAR_NAME = 1,
	WEAP_AMMOVAR_SUBTYPE,
	WEAP_AMMOVAR_DIAMETER,
	WEAP_AMMOVAR_PROJECTILEWEIGHT,
	WEAP_AMMOVAR_AMMOWEIGHT,
	WEAP_AMMOVAR_INITVELOCITY,
	WEAP_AMMOVAR_TRACERTIME,
    WEAP_AMMOVAR_TRACERSCALE,
	WEAP_AMMOVAR_OBJECT,
	WEAP_AMMOVAR_EXPLOSIVEENERGY,
	WEAP_AMMOVAR_FUZETIME,
	WEAP_AMMOVAR_BALLISTICCOEFFICIENT,
	WEAP_AMMOVAR_LIFETIME,
	WEAP_AMMOVAR_STRETCHOBJECT,
	WEAP_AMMOVAR_DONTDISPLAYFIRSTFRAME,
	WEAP_AMMOVAR_PENETRATIONCONSTANT,
	WEAP_AMMOVAR_RICHOCHETANGLE,
	WEAP_AMMOVAR_FUSESTARTSPEEDLOSS,
	WEAP_AMMOVAR_MODELSPALL,
	WEAP_AMMOVAR_NUMSHRAPNELPIECES,
	WEAP_AMMOVAR_TOTALSHRAPNELMASS,
	WEAP_AMMOVAR_EXPLOSIONSOUND_NEAR,
	WEAP_AMMOVAR_EXPLOSIONSOUND_FAR,
	WEAP_AMMOVAR_NEARFAR_DIST,
	WEAP_AMMOVAR_SELFDESTRUCT_TIME,
	WEAP_AMMOVAR_ISPLACED,
	WEAP_AMMOVAR_GRENADE_ROTATE_RATE,
	WEAP_AMMOVAR_SHAPECHARGEPENETRATION,
	WEAP_AMMOVAR_TORPEDO_RUNDEPTH,
	WEAP_AMMOVAR_TORPEDO_RUNSPEED,
	WEAP_AMMOVAR_FUSE_TYPE,
	WEAP_AMMOVAR_ISSMOKEGRENADE,
	WEAP_AMMOVAR_SHRAPNELPENETRATION,
	WEAP_AMMOVAR_USE_NEW_HE,
	WEAP_AMMOVAR_NHE_SHRAP_DIAM,
	WEAP_AMMOVAR_NHE_SHRAP_WEIGHT,
	WEAP_AMMOVAR_NHE_SHRAP_PC,
	WEAP_AMMOVAR_NHE_SHRAP_ANGLE,
	WEAP_AMMOVAR_NHE_ENERGY_MULT,
	WEAP_AMMOVAR_NHE_MIN_PIECES,
	WEAP_AMMOVAR_NHE_MAX_PIECES,
	WEAP_AMMOVAR_NHE_MAX_PER_VEHICLE,
	WEAP_AMMOVAR_NHE_ADD_SPEED,
	WEAP_AMMOVAR_NHE_TIMEOUT,
	WEAP_AMMOVAR_NHE_MAX_MASS_MULT,
    WEAP_AMMOVAR_MUZZLEFLASH_TYPE,
    WEAP_AMMOVAR_MODELCMX,
    WEAP_AMMOVAR_MODELCMY,
    WEAP_AMMOVAR_MODELCMZ,
    WEAP_AMMOVAR_UNITINERTIAXX,
    WEAP_AMMOVAR_UNITINERTIAYY,
    WEAP_AMMOVAR_UNITINERTIAZZ,
    WEAP_AMMOVAR_UNITINERTIAXY,
    WEAP_AMMOVAR_UNITINERTIAXZ,
    WEAP_AMMOVAR_UNITINERTIAYZ,
    WEAP_AMMOVAR_GRENADEORIENT,
	WEAP_AMMOVAR_GRENADERESTITUTION,
    WEAP_AMMOVAR_HITEFFECT_TYPE,
    WEAP_AMMOVAR_GHOSTROUNDSTO,
    WEAP_AMMOVAR_VARIANCE,
	WEAP_AMMOMAXVAR
};

// fuze type
enum
{
	WEAPAMMO_FUZE_FIXTIME,
	WEAPAMMO_FUZE_FIXPROX,
	WEAPAMMO_FUZE_FIXBARYO,
	WEAPAMMO_FUZE_FIXPROXALT,
	WEAPAMMO_FUZE_VARTIME,
	WEAPAMMO_FUZE_VARBARYO,
	WEAPAMMO_FUZE_VARPROX,
	WEAOAMMO_FUZE_VARPROXALT,
	WEAPAMMO_FUZE_MAX
};

static UCHAR _AmmoTypeVarNames[WEAP_AMMOMAXVAR][32] = {
	"none",
	"name",
	"type",
	"diameter",
	"weight",
	"ammoweight",
	"velocity",
	"tracertime",
    "tracerscale",
	"object",
	"explosive",
	"fuzetime",
	"bc",
	"lifetime",
	"stretchobject",
	"dontdisplayfirstframe",
	"penetrationconstant",
	"ricochetangle",
	"fuzestartspeedloss",
	"modelspall",
	"numshrapnelpieces",
	"totalshrapnelmass",
	"explosionsoundnear",
	"explosionsoundfar",
	"nearfardistance",
	"selfdestructtime",
	"isplaced",
	"grenaderotaterate",
	"shapedchargepenmm",
	"torpedorundepth",
	"torpedorunspeed",
	"fuzetype",
	"issmokegrenade",
	"shrapnelpenetration",
	"use_new_he",
	"nhe_diameter",
	"nhe_weight",
	"nhe_min_pen_const",
	"nhe_ric_angle",
	"nhe_energy_mult",
	"nhe_min_pieces",
	"nhe_max_pieces",
	"nhe_max_per_veh",
	"nhe_add_speed",
	"nhe_timeout",
	"nhe_max_mass_mult",
    "muzzleflash",
	"modelcmx",
	"modelcmy",
	"modelcmz",
	"unitinertiaxx",
	"unitinertiayy",
	"unitinertiazz",
	"unitinertiaxy",
	"unitinertiaxz",
	"unitinertiayz",
	"grenadeorient",
	"grenaderestitution",
    "hiteffect",
	"ghostroundsto",
    "variance"
};

static UCHAR _AmmoSubTypeVarNames[WEAPAMMO_SUBTYPE_MAX][32] =
{
	"ballistic",
	"grenade",
	"airburst",
	"bomb",
	"torpedo",
	"rocket",
	"placed",
	"nuke",
	"knife",
    "mortar"
};

UCHAR _AmmoFuzeTypeVarNames[WEAPAMMO_FUZE_MAX][32] =
{
	"fixtime",
	"fixprox",
	"fixbaryo",
	"fixproxalt",
	"vartime",
	"varbaryo",
	"varprox",
	"varproxalt"
};

//static FLOAT64 PERSON_THICKNESS[4] = {0.0004, 0.0012, 0.001, 0.001};
//static const char* PERSON_LOCATION[4] = {"head", "torso", "arms", "legs"};

// Mach chart
FLOAT32 G1MachChart[G1_CHART_NUM_DIVISIONS];

// ammo types
static UINT32 _NumAmmoTypes;
static weapAmmoType* _AmmoTypeList;

// in-flight ammo list
static UINT32 	_AmmoObjectCounter = 1;
static SINT32	_NumAmmo;
static weapAmmo _Ammo[WEAP_AMMO_MAXNUM_AMMO];
static weapAmmo _AmmoHead;
static weapAmmo _AmmoFree;

// tracer
static SINT32 	_NumTracerFX;
static TracerFX _TracerFXList[MAX_TRACERS_VIS];
static SINT32 	_TracerTexId = -1;
static piLVERTEX _TracerVerts[512];

static OBJECT* _WeapRoundObject;

static OBJECT* _TorpedoWake;
static FLOAT32 TorpedoRunDepth = 1;
static FLOAT32 VarFuzeInterval;

LOGICAL ConcussionEnabled = TRUE;
LOGICAL SpallEnabled = TRUE;
LOGICAL ShrapnelEnabled = TRUE;

static bool IsBouncingGrenade(weapAmmoType const* AmmoType)
{
	return (AmmoType->SubType == WEAPAMMO_SUBTYPE_GRENADE || AmmoType->SubType == WEAPAMMO_SUBTYPE_MORTAR) && AmmoType->FuzeTime > 0;
	//return (AmmoType->SubType == WEAPAMMO_SUBTYPE_GRENADE && AmmoType->FuzeTime > 0);
}

weapAmmoType* 	weapFindAmmoTypeByIndex(UINT32 i)
{
	if(i >= _NumAmmoTypes) return 0;
	return &_AmmoTypeList[i];
}

UINT32 weapFindAmmoTypeByName(weapAmmoType ** AmmoType,UCHAR * VarContents)
{
	UINT32 i;

    if ( AmmoType == nullptr )
    {
        return 0;
    }

	for (i=0;i<_NumAmmoTypes;i++)
	{
		if(!strcmp((char*)VarContents,(char*)_AmmoTypeList[i].Name))
		{
			*AmmoType = &_AmmoTypeList[i];
			return 0;
		}
	}

	Write_Log(LOG_WEAP,LOG_WARN,"Cannot find ammo type %s",VarContents);

	*AmmoType = nullptr;

	return 0;
}

UINT32 _ReadAmmoTypes()
{
	int AmmoListFile = 0;
	int CurrentAmmoFile = 0;
    UCHAR FileName[300];
    UCHAR Buffer[256];
	UCHAR* VarName = nullptr;
	UCHAR* VarContents = nullptr;
	UINT32 VarNum = 0;
	UINT32 CurrentPos = 0;
	weapAmmoType* AmmoType = nullptr;
	UINT32 i = 0;

	FileName[0] = Buffer[0] = 0;

	OBJECT * Object;

logTimerBeginMacro();
	_NumAmmoTypes = 0;

	sprintf((char*)FileName,"vehicles\\ammo.txt");

	AmmoListFile = fioOpen((char*)FileName);

	if (AmmoListFile == -1)
	{
		Write_Log(LOG_WEAP,LOG_WARN,"Can't find data/vehicles/ammo.txt file!");
		return (UINT32)-1;
	}

	while (fioGetString(AmmoListFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] != '#' && strlen((char*)Buffer) > 0)
		{
			_NumAmmoTypes++;
		}
	}

	fioClose(AmmoListFile);

	_AmmoTypeList = (weapAmmoType*)memAlloc(sizeof(weapAmmoType) * _NumAmmoTypes);

	AmmoListFile = fioOpen((char*)FileName);

	CurrentPos = 0;
	while (fioGetString(AmmoListFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] != '#' && strlen((char*)Buffer) > 0)
		{
			sprintf((char*)FileName,"vehicles\\ammo\\%s",(char*)Buffer);
			CurrentAmmoFile = fioOpen((char*)FileName);

			if (CurrentAmmoFile == -1)
			{
				continue;
			}

			AmmoType = &_AmmoTypeList[CurrentPos];
			AmmoType->LocalIndex = (UINT16)CurrentPos;
			strcpy((char*)AmmoType->Name,(char*)Buffer); // copy the ammo's name
			for (i=0;i<32;i++)
				AmmoType->Name[i] = tolower(AmmoType->Name[i]);

			// ensure that ammo has some form of penetration constant
			AmmoType->PenetrationConstant = 0.00000000001f;
            AmmoType->TracerScale = 4.0f; // WAG
            AmmoType->VelocityVariance = 0.0015f; // .15 % variance

			while (fioGetString(CurrentAmmoFile,(char*)Buffer,255) != 0)
			{
				if (Buffer[0] == '#')
				{
					continue;
				}

				mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

				if (VarName == nullptr || VarContents == nullptr)
				{
					continue;
				}

				mlUtilityFindVarname(&VarNum,
									 VarName,
									 (UCHAR*)&_AmmoTypeVarNames[0][0],
									 32,
									 WEAP_AMMOMAXVAR);




				switch (VarNum)
				{
					case WEAP_AMMOVAR_NAME:
						break;
					case WEAP_AMMOVAR_SUBTYPE:
						mlUtilityFindVarname((UINT32*)&AmmoType->SubType,
										VarContents,
										(UCHAR*)&_AmmoSubTypeVarNames[0][0],
										32,
										WEAPAMMO_SUBTYPE_MAX);
						break;
					case WEAP_AMMOVAR_DIAMETER:
						AmmoType->Diameter = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_PROJECTILEWEIGHT:
						AmmoType->ProjectileWeight = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_AMMOWEIGHT:
						AmmoType->AmmoWeight = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_INITVELOCITY:
						AmmoType->InitVelocity = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_TRACERTIME:
						AmmoType->TracerTime = (FLOAT32)atof((char*)VarContents);
						break;
                    case WEAP_AMMOVAR_TRACERSCALE:
                        AmmoType->TracerScale = (FLOAT32)atof((char*)VarContents);
                        break;
					case WEAP_AMMOVAR_OBJECT:
						if (strcmp((char*)VarContents,"bullet1") == 0)
						{
							AmmoType->UseParticlesForTracer = 1;
							obmgrCreateModelInstance((char*)VarContents);
							Object = obmainFindNamedObject((char*)VarContents);
						}
						else
						{
							Object = obmgrCreateModelInstance((char*)VarContents);

						}
						if (Object != nullptr)
						{
							AmmoType->Object = Object;
							strprintf(Object->name,"%s%dat",VarContents,CurrentPos); // ensure unique name

							ogrpHideObjectAndSubtree(Object);
							obmainMoveToNotObjectTree(Object);
						}

                         if(AmmoType->SubType == WEAPAMMO_SUBTYPE_PLACED)
                         {
                            struct SetNull
                            {
                                void Apply(OBJECT* o)
                                {
                                    if(o)
                                    {
                                        if(!(o->flags & OBJECT_NOTOBJECT))
                                        {
                                            o->drawcallback = nullptr;
                                        }

                                        Apply(o->child);
                                        Apply(o->sibling);
                                        Apply(o->attached);
                                    }
                                }
                            };

                            SetNull setnull;
                            setnull.Apply(Object);
                         }
						break;
					case WEAP_AMMOVAR_EXPLOSIVEENERGY:
						AmmoType->ExplosiveEnergy = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_FUZETIME:
						AmmoType->FuzeTime = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_BALLISTICCOEFFICIENT:
						AmmoType->BallisticCoefficient = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_LIFETIME:
						AmmoType->LifeTime = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_STRETCHOBJECT:
						AmmoType->StretchObject = atoi((char*)VarContents);
						break;
					case WEAP_AMMOVAR_DONTDISPLAYFIRSTFRAME:
						AmmoType->IgnoreFirstFrame = atoi((char*)VarContents);
						break;
					case WEAP_AMMOVAR_PENETRATIONCONSTANT:
						AmmoType->PenetrationConstant = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_SHRAPNELPENETRATION:
						AmmoType->ShrapnelPenetrationConstant = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_RICHOCHETANGLE:
						AmmoType->RichochetAngle = (FLOAT32)(mlDEG_TO_RAD(atof((char*)VarContents)));
						break;
					case WEAP_AMMOVAR_FUSESTARTSPEEDLOSS:
						AmmoType->FuseStartSpeedLoss = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_MODELSPALL:
						AmmoType->ModelSpall = atoi((char*)VarContents);
						break;
					case WEAP_AMMOVAR_NUMSHRAPNELPIECES:
						AmmoType->NumShrapnelPieces = atoi((char*)VarContents);
						break;
					case WEAP_AMMOVAR_TOTALSHRAPNELMASS:
						AmmoType->TotalShrapnelMass = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_EXPLOSIONSOUND_NEAR:
						strcpy(AmmoType->ExplosionNameNear,(char*)VarContents);
						break;
					case WEAP_AMMOVAR_EXPLOSIONSOUND_FAR:
						strcpy(AmmoType->ExplosionNameFar,(char*)VarContents);
						break;
					case WEAP_AMMOVAR_NEARFAR_DIST:
						AmmoType->NearFarSoundDist = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_SELFDESTRUCT_TIME:
						AmmoType->SelfDestructTime = (FLOAT32)atof((char*)VarContents);
						break;
					case WEAP_AMMOVAR_ISPLACED:
						AmmoType->IsPlaced = atoi((char*)VarContents);
						break;
					case WEAP_AMMOVAR_GRENADE_ROTATE_RATE:
						AmmoType->GrenadeRotateRate = (FLOAT32)atof((char*)VarContents) * 2 * PI;
						break;
					case WEAP_AMMOVAR_SHAPECHARGEPENETRATION:
						AmmoType->ShapedChargePen = (FLOAT32)(atof((char*)VarContents) * 0.001);
						break;
					case WEAP_AMMOVAR_TORPEDO_RUNDEPTH:
						AmmoType->TorpedoRunDepth = (FLOAT32)(atof((char*)VarContents));
					break;
					case WEAP_AMMOVAR_TORPEDO_RUNSPEED:
						AmmoType->TorpedoRunSpeed = (FLOAT32)(atof((char*)VarContents));
					break;
					case WEAP_AMMOVAR_FUSE_TYPE:
						mlUtilityFindVarname((UINT32*)&AmmoType->FuzeType, VarContents,
												(UCHAR*)&_AmmoFuzeTypeVarNames[0][0], 32, WEAPAMMO_FUZE_MAX);
						if(AmmoType->FuzeType != WEAPAMMO_FUZE_FIXTIME && AmmoType->FuzeType != WEAPAMMO_FUZE_VARTIME)
							Write_Log(LOG_WEAP,LOG_WARN, "fuze type %s not supported yet", VarContents);
					break;
					case WEAP_AMMOVAR_ISSMOKEGRENADE:
						AmmoType->IsSmokeGrenade = (LOGICAL)atoi((char*)VarContents);
					break;
					// new HE variables
					case WEAP_AMMOVAR_USE_NEW_HE:
						AmmoType->UseNewHE = (SINT32)atoi((char*)VarContents);
						if (AmmoType->UseNewHE)
						{
							AmmoType->ShrapProjDiameter = 0.001f;
							AmmoType->ShrapProjWeight = 0.001f;
							AmmoType->ShrapProjMinPenConst = 0.00000001f;
							AmmoType->ShrapProjRicochetAngle = 89.9999f;
							AmmoType->ShrapEnergyMult = 0.3f;
							AmmoType->MinShrapPieces = 500;
							AmmoType->MaxShrapPieces = 750;
							AmmoType->MaxShrapPerVehicle = 75;
							AmmoType->AddedParticleSpeed = 300.0f;
							AmmoType->ParticleTimeout = 0.03f;
							AmmoType->MaxMassMultiplier = 2.0f;
						}
					break;
					case WEAP_AMMOVAR_NHE_SHRAP_DIAM:
						AmmoType->ShrapProjDiameter = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_SHRAP_WEIGHT:
						AmmoType->ShrapProjWeight = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_SHRAP_PC:
						AmmoType->ShrapProjMinPenConst = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_SHRAP_ANGLE:
						AmmoType->ShrapProjRicochetAngle = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_ENERGY_MULT:
						AmmoType->ShrapEnergyMult = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_MIN_PIECES:
						AmmoType->MinShrapPieces = (SINT32)atoi((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_MAX_PIECES:
						AmmoType->MaxShrapPieces = (SINT32)atoi((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_MAX_PER_VEHICLE:
						AmmoType->MaxShrapPerVehicle = (SINT32)atoi((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_ADD_SPEED:
						AmmoType->AddedParticleSpeed = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_TIMEOUT:
						AmmoType->ParticleTimeout = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_NHE_MAX_MASS_MULT:
						AmmoType->MaxMassMultiplier = (FLOAT32)atof((char*)VarContents);
						if (AmmoType->MaxMassMultiplier < 1.0f)
							AmmoType->MaxMassMultiplier = 1.0f;
					break;
                    case WEAP_AMMOVAR_MUZZLEFLASH_TYPE:
                        AmmoType->MuzzleFlashType = (SINT32)atoi((char*)VarContents);
                    break;
                    case WEAP_AMMOVAR_HITEFFECT_TYPE:
                        {
                            strsafecpy(AmmoType->hiteffect, (char*)VarContents);
                            break;
                        }
					case WEAP_AMMOVAR_MODELCMX:
						AmmoType->ModelCM.x = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_MODELCMY:
						AmmoType->ModelCM.y = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_MODELCMZ:
						AmmoType->ModelCM.z = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_UNITINERTIAXX:
						AmmoType->Ixx = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_UNITINERTIAYY:
						AmmoType->Iyy = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_UNITINERTIAZZ:
						AmmoType->Izz = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_UNITINERTIAXY:
						AmmoType->Ixy = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_UNITINERTIAXZ:
						AmmoType->Ixz = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_UNITINERTIAYZ:
						AmmoType->Iyz = atof((char*)VarContents);
					break;
					case WEAP_AMMOVAR_GRENADEORIENT:
						AmmoType->GrenadeOrient = (FLOAT32)-mlDEG_TO_RAD(atof((char*)VarContents));
					break;
					case WEAP_AMMOVAR_GRENADERESTITUTION:
						AmmoType->GrenadeRestitution = (FLOAT32)atof((char*)VarContents);
					break;
                    case WEAP_AMMOVAR_GHOSTROUNDSTO:
						AmmoType->UseGhostRoundSTO = (LOGICAL)atoi((char*)VarContents);
                        break;
                    case WEAP_AMMOVAR_VARIANCE:
                        AmmoType->VelocityVariance = (FLOAT32)atof((char*)VarContents);
                        break;
					default:
						Write_Log(LOG_WEAP,LOG_WARN,"Unidentified var %s in file %s",VarName,FileName);
					break;
				}
			}

			fioClose(CurrentAmmoFile);

			CurrentPos++;

		}
	}

	fioClose(AmmoListFile);

logTimerEndMacro();

	return 0;
}

/************************************************************************************************
	weapammo
*************************************************************************************************/

// applying damage
static UINT32 weapAmmoApplyDamageToVehicleComponent(weapAmmo* Ammo, vehVehicle* InVehicle, UINT32 ComponentNum, FLOAT64 Energy, UINT32 Counter);
static UINT32 weapAmmoApplyDamageToObject(weapAmmo* Ammo, const FLOAT64& Energy, trnCDTerrainObject* TO, vehVehicle* InShooterVehicle);

// init
static UINT32 _ReadBallisticChart();

// ammo list
static UINT32 _GetNewAmmo(weapAmmo ** AmmoPtr);
static UINT32 _KillAmmo(weapAmmo* Ammo);
static UINT32 _AddToFreeList(weapAmmo * Vehicle);
static UINT32 _RemoveFromList(weapAmmo * Vehicle);
static UINT32 _AddToListHead(weapAmmo * Vehicle);

// render
static bool _DisplayWeaponsInFlight(OBJECT * DummyObject);
static UINT32 _UpdateProjectileDisplay(weapAmmo * Ammo);
static UINT32 _StretchObject(OBJECT * Object,FLOAT32 Stretch);
static UINT32 _DisplayObject(OBJECT * Object);

// ammo explosion
static UINT32 _DoSpall(weapAmmo* Ammo, FLOAT32 OriginalSpeed, FLOAT32 RemainingSpeed, FASTCOLLIDE* Collide,
						piVECTOR const* NewPos, piVECTOR const* OldPos, UINT32 Flags);
static UINT32 _DoExplosionShrapnel(weapAmmo* Ammo,FLOAT64VECTOR const* InPos, FLOAT64VECTOR const* ProjVel, FLOAT64 CurrentTime, bool doshrap=true);
static UINT32 _DoShapedCharge(weapAmmo * Ammo, FLOAT64VECTOR const* InPos, FLOAT64VECTOR const* ProjVel, FLOAT64 CurrentTime);
static UINT32 _DoExplosionConcussion(weapAmmo* Ammo, FLOAT64VECTOR const* InPos, LOGICAL Internal);
static LOGICAL _ApplyShapedChargeDamage(weapAmmo* Ammo, FASTCOLLIDE* Collision, FLOAT64 DeltaTime, FLOAT64 CurrentTime);
static UINT32 _DoKnifeSlash(weapAmmoType const*, FLOAT64VECTOR const* Pos, piMATRIX const* FireAtt, UINT32 Flags, void* FireVehicle);

// projectile collisions
static LOGICAL	_DoProjectileUpdate(weapAmmo*, FLOAT64VECTOR const* PrevPos, FLOAT64 DeltaTime, FLOAT64 CurrentTime);
static LOGICAL 	_DoVehicleCollision(weapAmmo*, FASTCOLLIDE*,
										piVECTOR const* OldPos, piVECTOR const* NewPos, FLOAT64 DeltaTime, FLOAT64 CurrentTime);
static LOGICAL 	_DoTerrainCollision(weapAmmo*, FASTCOLLIDE*,
										piVECTOR const* OldPos, piVECTOR const* NewPos,  FLOAT64 DeltaTime,  FLOAT64 CurrentTime);

static void 	_TransferMomentum(mlVehicleData*, FASTCOLLIDE const*, weapAmmo const*, FLOAT64 SpeedLoss);

static UINT32 	_GetProjectileCollisions(weapAmmo*, UINT32 Flags, FASTCOLLIDE* CollideList, UINT32 MaxCollisions, piVECTOR const* OldPos, piVECTOR const* NewPos,
											FLOAT64 DeltaTime, FLOAT64 CurrentTime);
static LOGICAL	_DoAmmoFuzeCheck(weapAmmo* Ammo, piVECTOR const* OldPos,  piVECTOR const* NewPos,
									FLOAT64 DeltaTime, FLOAT64 CurrentTime, FLOAT64 t);
static SINT32 	_AmmoTerrainCollideList(weapAmmo*, UINT32 Flags, trnCDTerrainObject*** trnCollideList);
static UINT32 	_CollideRoundWithAutoguns(FASTCOLLIDE ** CollideListPtr,  UINT32 * NumCollisionSlotsLeft,
											piVECTOR const* OldPos,  piVECTOR const* NewPos,
								 			FLOAT64 OldPosTime,  FLOAT64 NewPosTime);

static UINT32 _CheckLineSphereCollide(UINT32* Collision, piVECTOR const* SphereCenter, piVECTOR const* UseOldPos, piVECTOR const* UseNewPos, FLOAT32 Radius);
static LOGICAL _IsWater(OBJECT* lpObj);

static UINT32	_DoImprovedShrapnel(weapAmmo* Ammo, FLOAT64VECTOR const* InPos, FLOAT64VECTOR const* ProjVel, FLOAT64 CurrentTime);

// projectile update
static SINT32	_UpdateProjectile(weapAmmo * Ammo);
static LOGICAL 	_UpdateBallisticProjectile(weapAmmo * Ammo);
static LOGICAL 	_UpdateAttachedProjectile(weapAmmo* Ammo);
static LOGICAL 	_UpdateTorpedoProjectile(weapAmmo* Ammo);
static LOGICAL 	_StickTorpedoToWater(weapAmmo* Ammo);

/************************************************************************************************
	weapAmmoInit
*************************************************************************************************/

UINT32 weapAmmoInit()
{
	SINT32 i;

	_ReadAmmoTypes();
	_ReadBallisticChart();

	_NumAmmo = 0;
	_NumTracerFX = 0;
	memset(&_Ammo[0],0,WEAP_AMMO_MAXNUM_AMMO * sizeof(weapAmmo));

	// add all the vehicles on the vehicle list to the free list
	for(i = 0;i < WEAP_AMMO_MAXNUM_AMMO; i++)
		_AddToFreeList(&_Ammo[i]);
	if(_WeapRoundObject == nullptr)
	{
		if(!obmgrCreateModelInstance("tracker"))
		{
			Write_Log(LOG_WEAP,LOG_WARN,"Unable to find tracker object!  No weapons-in-flight using objects will be seen");
		}
		else
		{
			_WeapRoundObject = obmainFindNamedObject("tracker");
			if (_WeapRoundObject->child != nullptr)
			{
				_WeapRoundObject->child->flags |= OBJECT_USEMATRIX | OBJECT_USERDRAW;
				_WeapRoundObject->child->drawcallback = _DisplayWeaponsInFlight;
			}
			else
			{
				_WeapRoundObject->flags |= OBJECT_USEMATRIX | OBJECT_USERDRAW;
				_WeapRoundObject->drawcallback = _DisplayWeaponsInFlight;

			}
			strcpy(_WeapRoundObject->name,"weapammo_tracker");
		}
	}

	_TorpedoWake = obmgrCreateModelInstance("torpedo1");
	if (_TorpedoWake != nullptr)
	{
		strcpy(_TorpedoWake->name,"torpedowake");

		ogrpHideObjectAndSubtree(_TorpedoWake);
		obmainMoveToNotObjectTree(_TorpedoWake);
	}

	_TracerTexId = ldpixLoadImageFileToSurface("tracer1.bmp", colorBLACK, LDPIX_NONE);
	if (!piIsTexture(_TracerTexId, 0))
	{
		Write_Log(LOG_WEAP,LOG_WARN,"Unable to load up the tracer texture map!");
	}
	piAddTexRef(_TracerTexId);

	weapfxInit();

	return 0;
}

/************************************************************************************************
	weapAmmoKill
*************************************************************************************************/

UINT32 weapAmmoKill()
{
	weapAmmo * CurrentAmmo = nullptr;
    weapAmmo* NextAmmo = nullptr;
	weapAmmoType * AmmoType = nullptr;
	UINT32 i;

	_NumTracerFX = 0;
	piFreeMipTextures(_TracerTexId);

	for (CurrentAmmo = _AmmoHead.Next;  CurrentAmmo != nullptr;)
	{
		NextAmmo = CurrentAmmo->Next;
		_KillAmmo(CurrentAmmo);
		CurrentAmmo = NextAmmo;
	}

	for (i=0;i<_NumAmmoTypes;i++)
	{
		AmmoType = &_AmmoTypeList[i];

		if (AmmoType->Object != nullptr)
		{
			ogrpFreeObjectTree(AmmoType->Object);
			AmmoType->Object = nullptr;
		}
	}
	if (_WeapRoundObject != nullptr)
	{
		ogrpFreeObjectTree(_WeapRoundObject);
		_WeapRoundObject = nullptr;
	}
	if (_AmmoTypeList != nullptr)
	{
		memFree(_AmmoTypeList);
		_AmmoTypeList = nullptr;
	}
	if (_TorpedoWake != nullptr)
	{
		ogrpFreeObjectTree(_TorpedoWake);
		_TorpedoWake = nullptr;
	}

    weapfxKill();

	return 0;
}

/************************************************************************************************
	weapAmmoKillAllAmmo
*************************************************************************************************/

UINT32 weapAmmoKillAllAmmo()
{
	weapAmmo* Ammo = nullptr;
	weapAmmo* NextAmmo = nullptr;

	for(Ammo = _AmmoHead.Next; Ammo != nullptr;)
	{
		NextAmmo = Ammo->Next;
		_KillAmmo(Ammo);
		Ammo = NextAmmo;
	}

	return 0;
}

void weapAmmoClearVehicle(vehVehicle* v)
{
	weapAmmo* Ammo = nullptr;
	weapAmmo* NextAmmo = nullptr;

	for(Ammo = _AmmoHead.Next; Ammo != nullptr;)
	{
		NextAmmo = Ammo->Next;
		if ( Ammo->AttachedToVehicle == v )
    {
      Ammo->AttachedToVehicle = nullptr;
    }
		Ammo = NextAmmo;
	}
}

/************************************************************************************************
	weapAmmoObjectCeasingToExist
*************************************************************************************************/

UINT32 weapAmmoObjectCeasingToExist(OBJECT * Object)
{
	weapAmmo* Ammo = nullptr;
	weapAmmo* NextAmmo = nullptr;

	for(Ammo = _AmmoHead.Next; Ammo != nullptr;)
	{
		NextAmmo = Ammo->Next;
		if (Ammo->AttachedObject != nullptr && Ammo->AttachedObject == Object)
			_KillAmmo(Ammo);
		Ammo = NextAmmo;
	}

	return 0;
}


/************************************************************************************************
	weapAmmoSetTorpedoRunDepth
*************************************************************************************************/

void weapAmmoSetTorpedoRunDepth(FLOAT32 depth)
{
	TorpedoRunDepth = depth;
}

/************************************************************************************************
	weapAmmoSetFuzeInterval
*************************************************************************************************/

void weapAmmoSetFuzeInterval(FLOAT32 interval)
{
	if(interval > 6.0f)
		interval = 6.0f;
	else if(interval < 0.5f && interval != 0.0f)
		interval = 0.5f;

	VarFuzeInterval = interval;
}

/************************************************************************************************
	weapAmmoGetFuzeInterval
*************************************************************************************************/

FLOAT32 weapAmmoGetFuzeInterval()
{
	return VarFuzeInterval;
}



/************************************************************************************************
	weapUpdate
*************************************************************************************************/

UINT32 weapUpdate()
{
	weapAmmo* CurrentAmmo = nullptr;
    weapAmmo* NextAmmo = nullptr;

	logTimerBeginMacro();

	if (_AmmoHead.Next != nullptr)
	{
		for (CurrentAmmo = _AmmoHead.Next;
			 CurrentAmmo != nullptr;)
		{
			NextAmmo = CurrentAmmo->Next; // save next pointer in case ammo is deleted when updated

			_UpdateProjectile(CurrentAmmo);
			CurrentAmmo = NextAmmo;
		}
	}

	logTimerEndMacro();

	return 0;
}


struct ObjectFlag2State
{
	ObjectFlag2State(OBJECT* o)
	{
		obj = o;
		flag2 = obj->flag2;
	}

	~ObjectFlag2State()
	{
		obj->flag2 = flag2;
	}

	unsigned int flag2;
	OBJECT* obj;
};

/************************************************************************************************
	weapUpdateVehicleHitmap
*************************************************************************************************/

UINT32 weapUpdateVehicleHitmap(vehVehicle* Vehicle)
{
	if(Vehicle->LastHitmapUpdateTime == frametime) return 0; // hitmap is up-to-date
	Vehicle->LastHitmapUpdateTime = frametime;

	if (Vehicle->VehicleObjectData == nullptr) return 0;

	if (Vehicle->VehicleObjectData->CollideExternObject != nullptr)
	{
		ObjectFlag2State s(Vehicle->VehicleObjectData->CollideExternObject);
		Vehicle->VehicleObjectData->CollideExternObject->flag2 &= ~OBJECT_DONT_UPDATE;

		vehAnimSetDOFObjectVals(Vehicle->VehicleObjectData->CollideExternObject,
								&Vehicle->StateAnimation,
								Vehicle->VehicleType,
								Vehicle);

		mlMoveAndRotateObject(Vehicle->VehicleObjectData->CollideExternObject,
								  &Vehicle->PosData.RenderPos,
								  &Vehicle->PosData.RenderAtt);
	}

	if (Vehicle->VehicleObjectData->CollideInternObject != nullptr)
	{
		ObjectFlag2State s(Vehicle->VehicleObjectData->CollideInternObject);
		Vehicle->VehicleObjectData->CollideInternObject->flag2 &= ~OBJECT_DONT_UPDATE;

		vehAnimSetDOFObjectVals(Vehicle->VehicleObjectData->CollideInternObject,
								&Vehicle->StateAnimation,
								Vehicle->VehicleType,
								Vehicle);

		mlMoveAndRotateObject(Vehicle->VehicleObjectData->CollideInternObject,
							  &Vehicle->PosData.RenderPos,
							  &Vehicle->PosData.RenderAtt);

	}

	if (Vehicle->VehicleObjectData->CollideWalkObject != nullptr)
	{
		ObjectFlag2State s(Vehicle->VehicleObjectData->CollideWalkObject);
		Vehicle->VehicleObjectData->CollideWalkObject->flag2 &= ~OBJECT_DONT_UPDATE;

		vehAnimSetDOFObjectVals(Vehicle->VehicleObjectData->CollideWalkObject,
								&Vehicle->StateAnimation,
								Vehicle->VehicleType,
								Vehicle);

		mlMoveAndRotateObject(Vehicle->VehicleObjectData->CollideWalkObject,
							  &Vehicle->PosData.RenderPos,
							  &Vehicle->PosData.RenderAtt);
	}

	if (Vehicle->VehicleObjectData->HitBox != nullptr)
	{
		ObjectFlag2State s(Vehicle->VehicleObjectData->HitBox);
		Vehicle->VehicleObjectData->HitBox->flag2 &=  ~OBJECT_DONT_UPDATE;

		vehAnimSetDOFObjectVals(Vehicle->VehicleObjectData->HitBox,
								&Vehicle->StateAnimation,
								Vehicle->VehicleType,
								Vehicle);

		mlMoveAndRotateObject(Vehicle->VehicleObjectData->HitBox,
							  &Vehicle->PosData.RenderPos,
							  &Vehicle->PosData.RenderAtt);
	}

	return 0;
}



/************************************************************************************************
	weapAmmoSetRoundPos
*************************************************************************************************/

UINT32 weapAmmoSetRoundPos(piVECTOR* Pos)
{
	piMATRIX Mat1;

	Mat1 = IdentityMatrix;
	Mat1._41 = -(FLOAT32)(Pos->x - playdir.x * 1.1);
	Mat1._42 = -(FLOAT32)(Pos->y - playdir.y * 1.1);
	Mat1._43 = -(FLOAT32)(Pos->z - playdir.z * 1.1);

	ogrpMoveAndRotateSubObject(_WeapRoundObject,&Mat1,&Mat1);

	return 0;
}

/************************************************************************************************
	weapDrawTracers
*************************************************************************************************/

LOGICAL weapDrawTracers()
{
	FLOAT32 MinU = 0,MinV = 0,MaxU = 0,MaxV = 0;
	FLOAT32 Len = 0;
	FLOAT64 Len1 = 0,Len2 = 0;
	piVECTOR Corner1 = {0, 0, 0};
    piVECTOR Corner2 = {0, 0, 0};
    piVECTOR Corner3 = {0, 0, 0};
    piVECTOR Corner4 = {0, 0, 0};
    piVECTOR P1 = {0, 0, 0};
    piVECTOR P2 = {0, 0, 0};
	piVECTOR TempVec1 = {0, 0, 0};
    piVECTOR TempVec2 = {0, 0, 0};
    piVECTOR TempVec3 = {0, 0, 0};
	FLOAT32 MinAngle = 0,Angle = 0,Scale = 0;
	UINT32 VertexCount = 0;
	UINT32 Color = 0;
	SINT32 i;
	FLOAT32 ScaleLen = 0;
	piLVERTEX* pVertices = nullptr;

	if(_NumTracerFX == 0) return TRUE;

	ScaleLen = 1.0f;

	VertexCount = 0;

	pVertices = _TracerVerts;

	piDisableMaterial();
	piSetActiveTexture(_TracerTexId, 0);
	piSetRenderState(piRENDERSTATE_ZBIAS, 0);
	piSetRenderState(piRENDERSTATE_SRCBLEND, piBLEND_ONE);
	piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);


	MinU = 0.0f;
	MaxU = 1.0f;
	MinV = 0.0f;
	MaxV = 1.0f;

	Color = piRGBA_MAKE(192,127,0,191);

	MinAngle = mlDEG_TO_RAD(FieldOfView / piGetWidth());

	for (i=0;i<_NumTracerFX;i++)
	{
		P1.x = (FLOAT32)(_TracerFXList[i].OldPos.x - WorldOriginOffset.x);
		P1.y = (FLOAT32)(_TracerFXList[i].OldPos.y - WorldOriginOffset.y);
		P1.z = (FLOAT32)(-_TracerFXList[i].OldPos.z - WorldOriginOffset.z);
		P2.x = (FLOAT32)(_TracerFXList[i].NewPos.x - WorldOriginOffset.x);
		P2.y = (FLOAT32)(_TracerFXList[i].NewPos.y - WorldOriginOffset.y);
		P2.z = (FLOAT32)(-_TracerFXList[i].NewPos.z - WorldOriginOffset.z);

		// adjust size for sprite factor
		mathSubVectors(&TempVec1,&P2,&P1);
		math3dScaleVector(&TempVec1,&TempVec1,ScaleLen * 0.5f);
		mathAddVectors(&TempVec2,&P2,&P1);
		math3dScaleVector(&TempVec2,&TempVec2,0.5f);
		mathAddVectors(&P2,&TempVec2,&TempVec1);
		mathSubVectors(&P1,&TempVec2,&TempVec1);

		// compute vector in plane of view in-line with
		// line formed by P1-P2
		mathSubVectors(&TempVec1,&P2,&P1);
		Len = mathVectDotProduct(&playdir,&TempVec1);

		TempVec1.x -= Len * playdir.x;
		TempVec1.y -= Len * playdir.y;
		TempVec1.z -= Len * playdir.z;
		math3dNormalize(&TempVec1);

		// now compute a perpendicular vector
		mathVectDoCrossProduct(&TempVec2,&TempVec1,&playdir);

		// next compute the 4 corners
		Corner1 = Corner2 = P1;
		Corner3 = Corner4 = P2;

		math3dScaleVector(&TempVec3,&TempVec1,_TracerFXList[i].Diameter);
		mathSubVectors(&Corner1,&Corner1,&TempVec3);
		mathSubVectors(&Corner2,&Corner2,&TempVec3);
		mathAddVectors(&Corner3,&Corner3,&TempVec3);
		mathAddVectors(&Corner4,&Corner4,&TempVec3);

		math3dScaleVector(&TempVec3,&TempVec2,_TracerFXList[i].Diameter);
		mathAddVectors(&Corner1,&Corner1,&TempVec3);
		mathAddVectors(&Corner4,&Corner4,&TempVec3);
		mathSubVectors(&Corner2,&Corner2,&TempVec3);
		mathSubVectors(&Corner3,&Corner3,&TempVec3);

		// ensure that tracer is at least 1 pixel wide (2 pixels, but
		// texture only occupies 1/2 of the tracer)
		mathSubVectors(&TempVec1,&Corner1,&Corner2);
		mathAddVectors(&TempVec2,&Corner1,&playpos);
		Len1 = mathExactVectorLength(&TempVec1);
		Len2 = mathExactVectorLength(&TempVec2);

		Angle = (FLOAT32)(Len1 / Len2);
		if (Angle < MinAngle * 2.0f)
		{
			Scale = MinAngle / Angle * 2.0f;

			math3dScaleVector(&TempVec3,&TempVec1,0.5f * Scale);
			mathAddVectors(&TempVec1,&Corner1,&Corner2);
			math3dScaleVector(&TempVec1,&TempVec1,0.5f);

			mathSubVectors(&Corner1,&TempVec1,&TempVec3);
			mathAddVectors(&Corner2,&TempVec1,&TempVec3);
		}

		mathSubVectors(&TempVec1,&Corner3,&Corner4);
		mathAddVectors(&TempVec2,&Corner3,&playpos);
		Len1 = mathExactVectorLength(&TempVec1);
		Len2 = mathExactVectorLength(&TempVec2);

		Angle = (FLOAT32)(Len1 / Len2);
		if (Angle < MinAngle * _TRACER_SCALE)
		{
			Scale = MinAngle / Angle * _TRACER_SCALE;

			math3dScaleVector(&TempVec3,&TempVec1,0.5f * Scale);
			mathAddVectors(&TempVec1,&Corner3,&Corner4);
			math3dScaleVector(&TempVec1,&TempVec1,0.5f);

			mathSubVectors(&Corner3,&TempVec1,&TempVec3);
			mathAddVectors(&Corner4,&TempVec1,&TempVec3);
		}

		// now the vertices
		pVertices->x = Corner3.x;
		pVertices->y = Corner3.y;
		pVertices->z = Corner3.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MinU;
		pVertices->tv = MinV;
		pVertices++;

		pVertices->x = Corner2.x;
		pVertices->y = Corner2.y;
		pVertices->z = Corner2.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MaxU;
		pVertices->tv = MinV;
		pVertices++;

		pVertices->x = Corner1.x;
		pVertices->y = Corner1.y;
		pVertices->z = Corner1.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MaxU;
		pVertices->tv = MaxV;
		pVertices++;

		pVertices->x = Corner1.x;
		pVertices->y = Corner1.y;
		pVertices->z = Corner1.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MaxU;
		pVertices->tv = MaxV;
		pVertices++;

		pVertices->x = Corner4.x;
		pVertices->y = Corner4.y;
		pVertices->z = Corner4.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MinU;
		pVertices->tv = MaxV;
		pVertices++;

		pVertices->x = Corner3.x;
		pVertices->y = Corner3.y;
		pVertices->z = Corner3.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MinU;
		pVertices->tv = MinV;
		pVertices++;

		VertexCount += 6;

		// now the vertices
		pVertices->x = Corner2.x;
		pVertices->y = Corner2.y;
		pVertices->z = Corner2.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MinU;
		pVertices->tv = MinV;
		pVertices++;

		pVertices->x = Corner3.x;
		pVertices->y = Corner3.y;
		pVertices->z = Corner3.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MaxU;
		pVertices->tv = MinV;
		pVertices++;

		pVertices->x = Corner4.x;
		pVertices->y = Corner4.y;
		pVertices->z = Corner4.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MaxU;
		pVertices->tv = MaxV;
		pVertices++;

		pVertices->x = Corner4.x;
		pVertices->y = Corner4.y;
		pVertices->z = Corner4.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MaxU;
		pVertices->tv = MaxV;
		pVertices++;

		pVertices->x = Corner1.x;
		pVertices->y = Corner1.y;
		pVertices->z = Corner1.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MinU;
		pVertices->tv = MaxV;
		pVertices++;

		pVertices->x = Corner2.x;
		pVertices->y = Corner2.y;
		pVertices->z = Corner2.z;
		pVertices->color = Color;
		pVertices->specular = colorALPHA100;
		pVertices->tu = MinU;
		pVertices->tv = MinV;
		pVertices++;

		VertexCount += 6;

		if (VertexCount > 511 - 12)
		{

			if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
								&_TracerVerts, VertexCount))
			{
				return(FALSE);
			}

			VertexCount = 0;
		}
	}

	if (VertexCount > 0)
	{
		if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
							&_TracerVerts, VertexCount))
		{
			return(FALSE);
		}

	}

	piSetRenderState(piRENDERSTATE_SRCBLEND, SrcBlend);
	piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
	piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);

	_NumTracerFX = 0;

	return TRUE;
}

class ammoPhysicsBody : public physicsBody
{
public:
	ammoPhysicsBody(weapAmmo* a) :
		physicsBody(COLLIDER_AMMO, COLLIDER_MOVE_MASK),
        current(0),
		Ammo(a)
	{
		// link us to the ammo object
		SetOwner(a);

		// damp angular momentum to prevent crazy spinning
		//_body->setAngularDamping(.05f);
	}

	virtual ~ammoPhysicsBody(){}

	virtual void AdjustNormalFilter()
	{
		if(!_thisContact)
			return;

		if(colliderIsType(_thisContact, COLLIDER_TRN_BUILDING))
		{
			trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(_thisContact);
			if(!TO) return;

            if ( TO->TerrainMasterObject->modelInstance == nullptr )
            {
                return;
            }
			if(!strcasecmp(TO->TerrainMasterObject->modelInstance->GetModelName(), "office1c"))
			{
				OBJECT* ob = colliderData<OBJECT>(_thisContact);
				if(!ob) return;

				if(!strcasecmp(ob->name, "walls2") && !strcasecmp(ob->matname, "interior"))
					collider->setNormalFilter(0, 0, -1.0f);
			}
		}
	}

	virtual bool handleContact(dContact* contact)
	{
		if(colliderIsType(contact->geom.g2, COLLIDER_DRIVABLE_MASK|COLLIDER_NORAGDOLL))
		{
			// special handling for vehicles; grenades tend to get stuck on them
			// this geom's position
			Vec3d geomPos = Vec3Ode(dGeomGetPosition(contact->geom.g2));
			Vec3d bodyPos = GetPosition(false);
			Vec3d dir = bodyPos - geomPos;
			dir.normalize();
			dVector3 normal;

			// only use contacts with normals facing our current velocity (within a 90 degree arc)
			if(dDOT(OdeVec3d(normal, dir), contact->geom.normal) < 0)
				return false;
		}

		OBJECT* object = (OBJECT*)dGeomGetData(contact->geom.g2);

		if(!object)
			object = (OBJECT*)dGeomGetData(contact->geom.g1);

		const FLOAT32 temp = 10.0f;

		// increase friction as speed decreases below 1 meter per second (make the ammo stop)
		contact->surface.mu = 2000.0f + ((_speed < temp) ? 8000.0f * ((temp - _speed) / temp) : 0.0f);

        // Already handled
        if(current == contact->geom.g1)
            return true;

		current = contact->geom.g1;

		if(object && _IsWater(object))
		{
            TransformEffect* te = nullptr;

            te = TransformEffect::AddEffect("watersplash", Vec3f(0, 0, -1), 0, nullptr);

            if(te)
            {
                Vec3d p(Ammo->Body->getPosition());
                p.z() = -p.z();

                te->enableemitters(true);;
                te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                te->setmatrix(Matrixf::translate(p));

                fxSndAddFiniteWorldSound("sounds/grenadesplash.wav",
                                         5,
                                         100,
                                         IDE_TP_PRIORITY_LOW,
                                         Ammo->Body->getPosition());
            }

            Ammo->KillAmmo = TRUE;
		    return false;
		}

		return true; //physicsBody::handleContact(contact);
	}

    dGeomID current;

	weapAmmo* Ammo;
};

/*
void ammoPhysicsBody::BeginStep()
{
	if(!IsEnabled())
		return;

	if(collider)
		collider->findCollisions();

	// clear out old contact joints
	if(collideJoints)
		collideJoints->empty();

	colliderQuery::CollisionList& collisions = collider->getCollisions();
	colliderQuery::CollisionList::iterator it = collisions.begin();

	for(; it != collisions.end(); ++it)
	{
		colliderQuery::Collision& contact = *it;

		dContact dcontact;
		memset(&dcontact,0x0,sizeof(dcontact));
		OdeVec3d(dcontact.geom.pos, contact.pos);
		OdeVec3d(dcontact.geom.normal, contact.normal);
		dcontact.geom.depth = contact.depth;
		dcontact.geom.g1 = contact.g1;
		dcontact.geom.g2 = contact.g2;
		dcontact.surface.mode = dContactSoftERP | dContactSoftCFM | dContactBounce;
		dcontact.surface.mu = dInfinity;
		dcontact.surface.soft_cfm = .0008f; //physicsGetCFM();
		dcontact.surface.soft_erp = .2f; //physicsGetERP();
		dcontact.surface.bounce = 0.25f; // restitution
		dcontact.surface.bounce_vel = 0.1f;

		// let subclasses adjust parameters
		//AdjustCollideJoint(contact, &dcontact);

		if(handleContact(&dcontact))
		{
			_contacts.push_back(dcontact);

			if(!collideJoints)
				collideJoints = new dJointGroup;

			dJointID c = dJointCreateContact(DefaultWorld(), *collideJoints, &dcontact);
			dJointAttach (c, _body->id(), 0);
			_collided = true;
		}
	}

	if(!_collided && _enhancedCollisionCheck)
		rayCheck();

	if(!_collided)
	{ // make sure these braces stay here because of the macro
	 	dOPE(_lastFreePos,=,_body->getPosition());
	}

	_collided = false;

	applyForces();

	dOPE(_prevPos,=,_body->getPosition());
}

void ammoPhysicsBody::EndStep()
{
	if(!IsEnabled())
		return;

	if(collider)
		collider->setDirty(true);

	damp();
}
*/

static void _InitGrenade(weapAmmo* Ammo, SINT32 seed)
{
// go down to float precision to send over network
	Ammo->Velocity.x = (float)Ammo->Velocity.x;
	Ammo->Velocity.y = (float)Ammo->Velocity.y;
	Ammo->Velocity.z = (float)Ammo->Velocity.z;

	weapAmmoType const* AmmoType = Ammo->AmmoType;
	physicsBody* body = Ammo->Body = new ammoPhysicsBody(Ammo);

	FLOAT64VECTOR angvel, up = {0,0,1};
	mathVectDoCrossProduct(&angvel,&Ammo->Velocity, &up);
	mathNormalize(&angvel);
	mathScaleVector(&angvel,&angvel,Ammo->AmmoType->GrenadeRotateRate);

	// add a bit of wobble (seeded so we can send it across network)
	float randval;
	mathGetRandomNumberFromSeed(&seed);
	randval = .5f*(seed/(float)LONG_MAX + 1);
	angvel.x += Ammo->AmmoType->GrenadeRotateRate * .2f * randval;
	mathGetRandomNumberFromSeed(&seed);
	randval = .5f*(seed/(float)LONG_MAX + 1);
	angvel.y += Ammo->AmmoType->GrenadeRotateRate * .2f * randval;
	mathGetRandomNumberFromSeed(&seed);
	randval = .5f*(seed/(float)LONG_MAX + 1);
	angvel.z += Ammo->AmmoType->GrenadeRotateRate * .2f * randval;
	body->setAngularVel(angvel);
	// vary fuze time a little
	mathGetRandomNumberFromSeed(&seed);
	randval = .5f*(seed/(float)LONG_MAX + 1);
	Ammo->VarFuzeInterval = AmmoType->FuzeTime+randval;

	body->setMass(AmmoType->ProjectileWeight, AmmoType->ModelCM
				, AmmoType->Ixx, AmmoType->Iyy,AmmoType->Izz,AmmoType->Ixy,AmmoType->Ixz,AmmoType->Iyz
				, AmmoType->Object->child);

	body->makeCapsuleColliderFromObject(AmmoType->Object->child, 2.5f);
	//body->setRotation(QuatFromAxisAndAngle(Ammo->Velocity,AmmoType->GrenadeOrient));
	body->setRestitution(AmmoType->GrenadeRestitution);
	body->setModelPosition(Ammo->Position);
	body->setLinearVel(Ammo->Velocity);

	//body->getCollider()->setCategoryMask(COLLIDER_AMMO);
	//body->getCollider()->setCollideMask(COLLIDER_MOVE_MASK);
	//body->SetEnabled(false);

	// add our new geom to the collider and activate
	//body->collider->addGeom(ammoBody->geom(), COLLIDER_AMMO, COLLIDER_MOVE_MASK);
	physicsAddObject(body);

#ifdef TPS_ROPE
	// create a rope tail for testing
	fpTPSRope* rope = new fpTPSRope(Ammo->Body->Body());
#endif
}

static void _FireOrdnanceSTO(weapAmmo* Ammo)
{
	if(!Ammo->AmmoType->UseGhostRoundSTO)
		return;

	SINT32 seed = frametime;

	if(IsBouncingGrenade(Ammo->AmmoType))
		_InitGrenade(Ammo, seed);

	if(Online)
	{
		UINT32 lifetime = ((UINT32)Ammo->AmmoType->LifeTime + 1)*1000;
		AttitudeVector att((float)Ammo->Velocity.x,(float)Ammo->Velocity.y,(float)Ammo->Velocity.z);
		WorldPosition_m pos(Ammo->Position.x,Ammo->Position.y,Ammo->Position.z);
		OrdnanceSTO ordnanceSTO(Ammo->AmmoType->LocalIndex, seed, pos, att, lifetime);
		ordnanceSTO.request(PlayerClient()->ChatConnection());
	}
}

static LOGICAL _UpdateGrenade(weapAmmo* Ammo)
{
	if(Ammo->KillAmmo) return TRUE;

    // Keep smokegrenade object drawn until smoke has completed.
    if(Ammo->Flags & WEAP_AMMOFLAG_SPECIALACTIVE)
    {
        return FALSE;
    }

	Ammo->Position = Ammo->Body->getModelPosition();
	Ammo->Att = Ammo->Body->getAttitude();

	FLOAT64 CurrentTime = (FLOAT64)(frametime)/1000.0;
	FLOAT64VECTOR Position = Ammo->Position;

	if((CurrentTime - Ammo->LaunchTime) > Ammo->VarFuzeInterval)
	{

		int n = Ammo->Body->collisionContacts().size();
		if(n > 0)
		{
			FLOAT64VECTOR Normal = {0,0,0};

			// magic to prevent clipping, maybe
			std::vector<dContact>::const_iterator it = Ammo->Body->collisionContacts().begin();
			for(; it != Ammo->Body->collisionContacts().end(); ++it)
			{
				Normal.x += it->geom.normal[0];
				Normal.y += it->geom.normal[1];
				Normal.z += it->geom.normal[2];
			}

			Position.x += .05 * Normal.x / (double)n;
			Position.y += .05 * Normal.y / (double)n;
			Position.z += .05 * Normal.z / (double)n;
		}


		if(Ammo->AmmoType->IsSmokeGrenade)
		{
			if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
            {
                // Have to handle smokegrenades in a special case, since we want
                // the object to remain until the smoke has finished.
				if(weapfxDetonateSmokeGrenade(&Position, Ammo))
				{
					Ammo->KillAmmo = TRUE;
				}
				else
				{
					Ammo->Flags |= WEAP_AMMOFLAG_SPECIALACTIVE;
				}

                return FALSE;
            }
		}
		else
		{
			weapfxAmmoExplosion(Ammo, &Position, nullptr, 0);

			_DoExplosionShrapnel(Ammo, &Position, &Ammo->Velocity, frametime / 1000.0);
			_DoExplosionConcussion(Ammo, &Position, FALSE);
		}
		return TRUE;
	}

	return FALSE;
}


/************************************************************************************************
	weapFireRound
*************************************************************************************************/

UINT32 weapFireRound(FLOAT64VECTOR* RecoilForce,
					 weapAmmoType* AmmoType,
					 FLOAT64VECTOR* Pos,
					 FLOAT64VECTOR* Vel,
					 piMATRIX* FireAtt,
					 FLOAT64 LaunchTime,
					 UINT32 Flags,
					 OBJECT* EmitterObject,
					 FLOAT64VECTOR* RelativePnt,
					 weapWeapon* weapon,
					 void* FireVehicle)
{
	// ghost round grenades can shortcut out here
	/*
	if(Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)
	{
		if(IsBouncingGrenade(AmmoType))
			return 0;
	}
	*/

	weapAmmo* Ammo = nullptr;
    FLOAT64VECTOR ProjVel = {0, 0, 0};
	FLOAT64 Impulse = 0;

	if(!AmmoType) return 0;

	if(AmmoType->SubType == WEAPAMMO_SUBTYPE_KNIFE)
	{
		// just do in 1st person for now
		if(!(Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
			_DoKnifeSlash(AmmoType, Pos, FireAtt, Flags, FireVehicle);

		return 0;
	}

	FLOAT32 InitVelocity = AmmoType->InitVelocity;

	if(AmmoType->SubType == WEAPAMMO_SUBTYPE_GRENADE &&
	 !(Flags & WEAP_AMMOFLAG_IS_GHOSTROUND) &&
	  (Flags & WEAP_AMMOFLAG_FPS_WEAPON))
	{
		InitVelocity *= fpFPSGetGrenadeThrowPower();
	}

	// calculate round's actual velocity
	ProjVel.x = ProjVel.z = 0;
	// vary round's velocity a bit
    float velocityFactor = GetRandom(0.0f, AmmoType->VelocityVariance);
    ProjVel.y = (FLOAT64)InitVelocity - (FLOAT64)((FLOAT64)InitVelocity * (FLOAT64)velocityFactor);
//    WriteLog(LOG_1, LOG_INFO, "WEAP AMMO %s(%d) IVel: %f vFactor: %.2f PVel: %.2f",
//			AmmoType->Name, InitVelocity , velocityFactor, ProjVel.y);

	// add shooter's velocity
	mathVectorMultiply64(&ProjVel, &ProjVel, FireAtt);

	// technically this is impulse, not force. no idea how Hoof came up with this
	Impulse = 1.0 + 0.7 * (InitVelocity / 800.0) * (InitVelocity / 800.0); // propellant recoil
	RecoilForce->x = -AmmoType->ProjectileWeight * ProjVel.x * Impulse;
	RecoilForce->y = -AmmoType->ProjectileWeight * ProjVel.y * Impulse;
	RecoilForce->z = -AmmoType->ProjectileWeight * ProjVel.z * Impulse;

	// this is for bombs that don't have a init velocity so we can tell they fired,
	// recoil is the only way the stupid code knows a mount has fired
	if (InitVelocity < 0.1)
	{
		RecoilForce->x += 0.011;
		RecoilForce->y += 0.011;
		RecoilForce->z += 0.011;
	}

	// launch round
	_GetNewAmmo(&Ammo);
	if(!Ammo) return 0;

	if(AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO)
	{
		Ammo->FireAtt = *FireAtt;

		// Ammo->Flags isn't set to the current Flags yet....?
		// seems like inconsistent behavior will occur because this will be
		// true if the just grabbed Ammo was a ghost round and not whether the
		// currently fired round is a ghost round or not..?
		if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)) // need to transmit this with the new update system
			Ammo->TorpedoRunDepth = (AmmoType->TorpedoRunDepth) ?  AmmoType->TorpedoRunDepth : TorpedoRunDepth;
		else
			Ammo->TorpedoRunDepth = AmmoType->TorpedoRunDepth;

		Ammo->Flags |= WEAP_AMMOFLAG_NOMUZZLEFLASH;
	}

	 // Add shooter's velocity
	mathAddVectors(&ProjVel, &ProjVel, Vel);

	Ammo->LaunchTime = Ammo->LastUpdateTime = Ammo->LastCollideTime = LaunchTime;
	Ammo->AmmoType = AmmoType;
	Ammo->Position = Ammo->LastCollidePos = *Pos;
	Ammo->Velocity = Ammo->LastVel = ProjVel;
	Ammo->Flags |= Flags | WEAP_AMMOFLAG_FIRSTFRAME;
	Ammo->Id = _AmmoObjectCounter++;
	Ammo->FirstUpdate = TRUE;
	Ammo->FireVehicle = FireVehicle;

	// all sorts of fun special casing for ghost grounds
	if(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)
	{
		Ammo->Flags |= WEAP_AMMOFLAG_DONTCOLLIDEWITHFIRER;
	}
	else
	{
		// need to transmit this with the new update system
		Ammo->VarFuzeInterval = VarFuzeInterval;
	}

    // Don't want a muzzle flash for these subtypes.
    switch(AmmoType->SubType)
    {
	    case WEAPAMMO_SUBTYPE_GRENADE:
		case WEAPAMMO_SUBTYPE_BOMB:
		case WEAPAMMO_SUBTYPE_PLACED:
        case WEAPAMMO_SUBTYPE_ROCKET:
        case WEAPAMMO_SUBTYPE_KNIFE:
        {
            Ammo->Flags |= WEAP_AMMOFLAG_NOMUZZLEFLASH;
            break;
        }
    }

	// fx muzzle flash
	weapfxAmmoMuzzleFlash(Ammo, RelativePnt, EmitterObject, Vel, LaunchTime, weapon);

    // Temporary until AMMO is defined properly. Need to know this is a bazooka.
    if(AmmoType->SubType == WEAPAMMO_SUBTYPE_ROCKET && Ammo->AmmoType->TracerTime > 0.0f)
    {
        TransformEffect* te = TransformEffect::AddTracer(Ammo, "bazookaammotrail");

        if(te)
        {
            Emitter* e = te->getemitter(0);
            e->_lifetime = (int)(Ammo->AmmoType->TracerTime * 1000.0f);
        }
    }

	// displacement
    if(weapon && weapon->Displacement != 0)
	{
		FLOAT64VECTOR Displacement = {0,0,0};

		Displacement.y = -weapon->Displacement;
		mathVectorMultiply64(&Displacement, &Displacement, FireAtt);
		mathAddVectors(&Ammo->Position, &Ammo->Position, &Displacement);
		Ammo->LastCollidePos = Ammo->Position;
	}

	if(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)
	{
		 if(Ammo->AmmoType->UseGhostRoundSTO)
		 {
	 		// we made it here but the ghost round will be handled
			// by an STO, as a result, we should kill this ammo;
			// this ammo was simply used to setup the muzzle fx and sound
			_KillAmmo(Ammo);
		 }
		 else
		 {
			weapfxAmmoAttachSound(Ammo);
		 }
	}
	else
	{
		weapfxAmmoAttachSound(Ammo);

		if(AmmoType->IsSmokeGrenade)
    		weapfxAttachSmokeGrenadeToAmmo(Ammo);

		// this is a real round that uses an STO for the
		// ghost round, we should send that out here
		if(AmmoType->UseGhostRoundSTO)
			_FireOrdnanceSTO(Ammo);
	}

	return 0;
}

/************************************************************************************************
	weapPlaceRoundOnObject
*************************************************************************************************/

UINT32 weapPlaceRoundOnObject(piVECTOR * CollidePnt,
								OBJECT * CollideObject,
								piVECTOR const* CollideNorm,
								piVECTOR const* OrigNorm,
								UINT32 Flags,
								FLOAT64 LaunchTime,
								weapAmmoType * AmmoType,
								void*  Vehicle,
								void*  TO)
{
	weapAmmo * Ammo = nullptr;

	if(AmmoType == nullptr)
        return 1;

    if(CollideObject)
    {
        // Ticket # 964
        if(trnwtrIsWaterTexture(CollideObject->texid))
            return 2;
    }

	_GetNewAmmo(&Ammo);

	if(Ammo == nullptr)
        return 3;

	Ammo->LaunchTime = LaunchTime;
	Ammo->LastUpdateTime = LaunchTime;
	Ammo->LastCollideTime = LaunchTime;
	Ammo->AmmoType = AmmoType;

	Ammo->Flags |= WEAP_AMMOFLAG_FIRSTFRAME;

	Ammo->Flags |= Flags | WEAP_AMMOFLAG_ISATTACHED_TO_OBJECT;
	Ammo->Id = _AmmoObjectCounter++;

	Ammo->AttachedObject = CollideObject;
	Ammo->AttachedPnt = *CollidePnt;
	Ammo->AttachedNorm = *CollideNorm;
	Ammo->OrigNorm = *OrigNorm;
	Ammo->AttachedToVehicle = Vehicle;
	Ammo->TO = TO;

	return 0;
}



/************************************************************************************************
	weapCollideWithHitmapObject
*************************************************************************************************/

UINT32 weapCollideWithHitmapObject(FASTCOLLIDE ** CollisionList,
										UINT32 * NumCollisionSlotsLeft,
										OBJECT * CurrentObject,
										OBJECT * TopObject,
										piVECTOR const* OldPnt,
										piVECTOR const* NewPnt,
										UINT32 DontIgnoreBackface,
										UINT32 DontCheckSibs)
{
	OBJECT* Object1 = nullptr;
    OBJECT* Object2 = nullptr;
	FLOAT32 mnx = 0;
    FLOAT32 mny = 0;
    FLOAT32 mnz = 0;
    FLOAT32 mxx = 0;
    FLOAT32 mxy = 0;
    FLOAT32 mxz = 0;
    piVECTOR OldPnt1 = {0, 0, 0};
    piVECTOR OldPnt2 = {0, 0, 0};
    piVECTOR NewPnt1 = {0, 0, 0};
    piVECTOR NewPnt2 = {0, 0, 0};

    piMATRIX MatInv = IdentityMatrix;

	if (CurrentObject == nullptr) return 0;

	Object1 = CurrentObject;

	do
	{
		if (!(Object1->flags & OBJECT_NOTOBJECT) && !(Object1->flag2 & OBJECT_MISSING))
		{
			Object2 = Object1;

			do
			{
				// check bounding box
				// note that the bounding boxes are relative to the center of the vertices
				mnx = Object2->bbox[0].x;
				mny = Object2->bbox[0].y;
				mnz = Object2->bbox[0].z;

				mxx = Object2->bbox[1].x;
				mxy = Object2->bbox[1].y;
				mxz = Object2->bbox[1].z;

				math3dInvertMatrix(&MatInv,&Object2->mat);
				math3dVectorMultiply(&OldPnt2,OldPnt,&MatInv);
				math3dVectorMultiply(&NewPnt2,NewPnt,&MatInv);

				mathSubVectors(&OldPnt1,&OldPnt2,&Object2->pos);
				mathSubVectors(&NewPnt1,&NewPnt2,&Object2->pos);

				if ((mnx <= NewPnt1.x &&
					mxx >= NewPnt1.x) ||
					(mnx <= OldPnt1.x &&
					mxx >= OldPnt1.x))
				{
					if ((mny <= NewPnt1.y &&
						mxy >= NewPnt1.y) ||
						(mny <= OldPnt1.y &&
						mxy >= OldPnt1.y))
					{
						if ((mnz <= NewPnt1.z &&
							mxz >= NewPnt1.z) ||
							(mnz <= OldPnt1.z &&
							mxz >= OldPnt1.z))
						{
                            if(Object2->flag2 & OBJECT_SPEEDTREE)
                            {
                                trncdCollideSpeedTree(Object2, TopObject, CollisionList, NumCollisionSlotsLeft, &OldPnt2, &NewPnt2);
                            }
                            else
                            {
							    // ok lets check the polygons
							    trncdCollideWithSpecificTerrainObject(CollisionList,
															      NumCollisionSlotsLeft,
															      Object2,
															      TopObject,
															      nullptr,
															      &OldPnt2,
															      &NewPnt2,
															      DontIgnoreBackface);
                            }

							Object2 = Object2->attached;
							continue;
						}
					}
				}

				if ((mnx <= NewPnt1.x &&
					mxx >= OldPnt1.x) ||
					(mnx <= OldPnt1.x &&
					mxx >= NewPnt1.x))
				{
					if ((mny <= NewPnt1.y &&
						mxy >= OldPnt1.y) ||
						(mny <= OldPnt1.y &&
						mxy >= NewPnt1.y))
					{
						if ((mnz <= NewPnt1.z &&
							mxz >= OldPnt1.z) ||
							(mnz <= OldPnt1.z &&
							mxz >= NewPnt1.z))
						{
                            if(Object2->flag2 & OBJECT_SPEEDTREE)
                            {
                                trncdCollideSpeedTree(Object2, TopObject, CollisionList, NumCollisionSlotsLeft, &OldPnt2, &NewPnt2);
                            }
                            else
                            {
							    // ok let's check the polygons
							    trncdCollideWithSpecificTerrainObject(CollisionList,
															      NumCollisionSlotsLeft,
															      Object2,
															      TopObject,
															      nullptr,
															      &OldPnt2,
															      &NewPnt2,
															      DontIgnoreBackface);
                            }

							Object2 = Object2->attached;
							continue;
						}
					}
				}
				Object2 = Object2->attached;
			} while (Object2 != nullptr);
		}

		if (Object1->child != nullptr)
		{
			weapCollideWithHitmapObject(CollisionList,
									  NumCollisionSlotsLeft,
									  Object1->child,
									  TopObject,
									  OldPnt,
									  NewPnt,
									  DontIgnoreBackface,
									  0);
		}

		Object1 = Object1->sibling;

	} while (Object1 != nullptr && DontCheckSibs == 0);



	return 0;
}

/************************************************************************************************
	weapCollideRoundWithVehicles
*************************************************************************************************/

UINT32 weapCollideRoundWithVehicles(FASTCOLLIDE ** CollideListPtr,
								 UINT32* NumCollisionSlotsLeft,
								 piVECTOR const* OldPos,
								 piVECTOR const* NewPos,
								 FLOAT64 OldPosTime,
								 FLOAT64 NewPosTime,
								 UINT32 CanHitPlayer,
								 UINT32 CheckOnlyPlayer,
								 FLOAT64VECTOR const* WorldOriginOffset,
								 UINT32 UseHitBoxInstead,
								 SINT32 Side)
{
	vehVehicle*		CurrentVehicle = nullptr;
	vehGetVehicleHead(&CurrentVehicle);
	vehVehicle*		PlayerVehicle = persGetPersonaVehiclePtr();
    piVECTOR		UseOldPos = {0, 0, 0};
    piVECTOR        UseNewPos = {0, 0, 0};
	FLOAT32			DeltaTime = 0;
    piVECTOR		DeltaPos = {0, 0, 0};
    piVECTOR		SphereCenter = {0, 0, 0};
	UINT32			Collision = 0;
	piMATRIX		InvMat = IdentityMatrix;
	FASTCOLLIDE*		TempCollideListPtr = nullptr;
	UINT32			TempNum = 0;
	UINT32			i = 0;
	SINT32			collisionOccurred = 0;
	fpHitLocations	location = FP_HIT_LOCATION_HEAD;
    piVECTOR		CollisionPnt = {0, 0, 0};
	FLOAT64			Dist1 = 0;
    FLOAT64         Dist2 = 0;
    piVECTOR		TempVec = {0, 0, 0};

    if(!PlayerVehicle)
        return 0;

	//if(!Online && PlayerVehicle->VehicleType->Type != VEH_TYPE_PERSON)
	//	return 0; // disable vehicle collisions for non infantry offline

	for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if ((CanHitPlayer == 0) && (CurrentVehicle == PlayerVehicle)) continue;
		if ((CheckOnlyPlayer != 0) && (CurrentVehicle != PlayerVehicle)) continue;
		if (CurrentVehicle->VehicleObjectData == nullptr) continue;
		if (Side > 0 && Side == CurrentVehicle->Side && CurrentVehicle != PlayerVehicle) continue;

		// give 20 seconds after a clone is spawned where you don't collide
        if((CurrentVehicle->Flags & VEH_FLAGS_IS_CLONE) &&
		   (CurrentVehicle->VehicleType->Type != VEH_TYPE_PERSON) &&
		   (frametime - CurrentVehicle->VehicleInitTime < 20000))
            continue;

		// first do line-sphere collide to determine if we're even close
		// adjust old position
		DeltaTime = (FLOAT32)(OldPosTime - frametime / 1000.0);

		if (CurrentVehicle == PlayerVehicle && PlayerVehicle->ModelData != nullptr)
		{
			DeltaPos.x = (FLOAT32)(CurrentVehicle->ModelData->VehicleRealVel.x * DeltaTime);
			DeltaPos.y = (FLOAT32)(CurrentVehicle->ModelData->VehicleRealVel.y * DeltaTime);
			DeltaPos.z = -(FLOAT32)(CurrentVehicle->ModelData->VehicleRealVel.z * DeltaTime);
			mathSubVectors(&UseOldPos,OldPos,&DeltaPos);

			UseNewPos = *NewPos;
			SphereCenter.x = (FLOAT32)(CurrentVehicle->ModelData->RenderPos.x - WorldOriginOffset->x);
			SphereCenter.y = (FLOAT32)(CurrentVehicle->ModelData->RenderPos.y - WorldOriginOffset->y);
			SphereCenter.z = -(FLOAT32)(CurrentVehicle->ModelData->RenderPos.z - WorldOriginOffset->z);
		}
		else
		{
			DeltaPos.x = (FLOAT32)(CurrentVehicle->PosData.Vel.x * DeltaTime);
			DeltaPos.y = (FLOAT32)(CurrentVehicle->PosData.Vel.y * DeltaTime);
			DeltaPos.z = -(FLOAT32)(CurrentVehicle->PosData.Vel.z * DeltaTime);
			mathSubVectors(&UseOldPos,OldPos,&DeltaPos);

			UseNewPos = *NewPos;
			SphereCenter.x = (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - WorldOriginOffset->x);
			SphereCenter.y = (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - WorldOriginOffset->y);
			SphereCenter.z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - WorldOriginOffset->z);
		}

		_CheckLineSphereCollide(&Collision, &SphereCenter, &UseOldPos,
								&UseNewPos, CurrentVehicle->VehicleObjectData->Radius);

		if (Collision != 0)
		{
			if (CurrentVehicle->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
			{
				// infantry don't collide like regular vehicles
				if (UseHitBoxInstead) continue;

				collisionOccurred = gr2AnimDoWeaponCollision(CurrentVehicle->VehicleObjectData->PlaneObject, &UseOldPos, &UseNewPos,
															 	(SINT32*)&location, &CollisionPnt);

				CollisionPnt.z = -CollisionPnt.z;

				if (collisionOccurred)
				{
					mathSubVectors(&TempVec,&UseNewPos,&UseOldPos);
					Dist1 = mathExactVectorLength(&TempVec);
					TempVec = CollisionPnt;
					math3dVectorMultiply(&TempVec,&TempVec,
										&CurrentVehicle->VehicleObjectData->PlaneObject->mat);
					mathSubVectors(&TempVec,&TempVec,&UseOldPos);
					Dist2 = mathExactVectorLength(&TempVec);
					if (Dist2 < Dist1 &&
						*NumCollisionSlotsLeft > 0)
					{
						TempCollideListPtr = *CollideListPtr;
						TempCollideListPtr->Flags |= _AMMOCD_FLAG_ISPERSONA;
						TempCollideListPtr->CollideObject = CurrentVehicle->VehicleObjectData->PlaneObject;
						TempCollideListPtr->TopLevelObject = (OBJECT*)CurrentVehicle;
						TempCollideListPtr->CollideTime = (FLOAT32)(Dist2 / Dist1);
						TempCollideListPtr->CollidePnt = CollisionPnt;
						math3dInvertMatrix(&InvMat,&CurrentVehicle->VehicleObjectData->PlaneObject->mat);
						math3dVectorMultiply(&TempCollideListPtr->OldPos,&UseOldPos,&InvMat);
						math3dVectorMultiply(&TempCollideListPtr->NewPos,&UseNewPos,&InvMat);
						TempCollideListPtr->Alpha = 1.0f;
						TempCollideListPtr->PolyNum = location;
						(*CollideListPtr)++;
						(*NumCollisionSlotsLeft)--;
					}
				}
			}
			else
			{
				weapUpdateVehicleHitmap(CurrentVehicle);

				if (UseHitBoxInstead >= 1)
				{
					TempNum = *NumCollisionSlotsLeft;
					TempCollideListPtr = *CollideListPtr;
					weapCollideWithHitmapObject(CollideListPtr,
											 NumCollisionSlotsLeft,
											 CurrentVehicle->VehicleObjectData->HitBox,
											 (OBJECT*)CurrentVehicle,
											 &UseOldPos,
											 &UseNewPos,
											 1,
											 1);
					TempNum -= *NumCollisionSlotsLeft;
				}
				else
				{

					TempNum = *NumCollisionSlotsLeft;
					TempCollideListPtr = *CollideListPtr;
					weapCollideWithHitmapObject(CollideListPtr,
											 NumCollisionSlotsLeft,
											 CurrentVehicle->VehicleObjectData->CollideExternObject,
											 (OBJECT*)CurrentVehicle,
											 &UseOldPos,
											 &UseNewPos,
											 1,
											 1);
					TempNum -= *NumCollisionSlotsLeft;
					for (i=0;i<TempNum;i++)
					{
						TempCollideListPtr[i].Flags |= _AMMOCD_FLAG_EXTERNALCOMPONENT;
					}

					TempNum = *NumCollisionSlotsLeft;
					TempCollideListPtr = *CollideListPtr;
					weapCollideWithHitmapObject(CollideListPtr,
											 NumCollisionSlotsLeft,
											 CurrentVehicle->VehicleObjectData->CollideInternObject,
											 (OBJECT*)CurrentVehicle,
											 &UseOldPos,
											 &UseNewPos,
											 0,
											 1);
					TempNum -= *NumCollisionSlotsLeft;
					for (i=0;i<TempNum;i++)
					{
						TempCollideListPtr[i].Flags |= _AMMOCD_FLAG_INTERNALCOMPONENT;
					}
				}
			}
		}
	}

	return 0;
}



/************************************************************************************************
	weapAmmoApplyDamageToVehicleComponent
*************************************************************************************************/

static UINT32 weapAmmoApplyDamageToVehicleComponent(weapAmmo* Ammo, vehVehicle* InVehicle,
													UINT32 ComponentNum, FLOAT64 Energy, UINT32 Counter)
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();


	if(InVehicle->isHulked() && !(Ammo->Flags & WEAP_AMMOFLAG_IS_SHAPED_CHARGE)) return 0;

	if ((Ammo->Flags & WEAP_AMMOFLAG_DAMAGE_SPECIFIC_OBJECT) && (Ammo->SpecificDamageObject != (void*)InVehicle))
	{
		return 0;
	}

	if( (Ammo->Flags & WEAP_AMMOFLAG_CANDAMAGE) &&
		(!(Ammo->Flags & WEAP_AMMOFLAG_DONT_SEND_DAMAGE) || InVehicle == PlayerVehicle) )
	{
		return vehApplyDamageToVehicleComponent(InVehicle, ComponentNum, Energy, Counter, Ammo);
	}

	return 0;
}

/************************************************************************************************
	weapAmmoApplyDamageToObject
*************************************************************************************************/

static UINT32 weapAmmoApplyDamageToObject(weapAmmo* Ammo, const FLOAT64& Energy, trnCDTerrainObject* TO, vehVehicle* InShooterVehicle)
{
	if (!TO)
		return 0;

	if (Ammo->Flags & WEAP_AMMOFLAG_DAMAGE_SPECIFIC_OBJECT)
	{
		if (!TO->TrnAutoGun)
			return 0;

		if (Ammo->SpecificDamageObject != (void*)TO->TrnAutoGun)
		{
			return 0;
		}
	}

	if((Ammo->Flags & WEAP_AMMOFLAG_CANDAMAGE) && !(Ammo->Flags & WEAP_AMMOFLAG_DONT_SEND_DAMAGE))
	{
		return vehApplyDamageToObject(Energy, TO, InShooterVehicle, Ammo);
	}

	return 0;
}




/************************************************************************************************
	_ReadBallisticChart
*************************************************************************************************/

static UINT32 _ReadBallisticChart()
{
	int CurrentFile = 0;
	FLOAT32 LastMachNum = 0,CurrentMachNum = 0;
	FLOAT32 LastGNum= 0,CurrentGNum = 0,Slope = 0;
	UINT32 Position= 0;
    UCHAR Buffer[256] = {0};

	CurrentFile = fioOpen("vehicles\\mcg1.txt");

	if (CurrentFile == -1)
	{
		Write_Log(LOG_WEAP,LOG_WARN,"Can't find ballistics chart vehicles\\mcg1.txt");
		return 0;
	}

	Position = 0;
	LastGNum = 0;
	LastMachNum = 0;
	while (fioGetString(CurrentFile,(char*)Buffer,255) != 0)
	{
		sscanf((char*)Buffer,"%f %f",&CurrentMachNum,&CurrentGNum);

		if (LastMachNum == CurrentMachNum)
		{
			LastGNum = CurrentGNum;
		}
		else
		{
			Slope = (CurrentGNum - LastGNum) / (CurrentMachNum - LastMachNum);

			while (LastMachNum < CurrentMachNum - 0.0025)
			{
				if (Position >= G1_CHART_NUM_DIVISIONS)
				{
					break;
				}

				G1MachChart[Position] = LastGNum;

				LastMachNum += (FLOAT32)G1_CHART_MACH_DELTA;
				LastGNum += Slope * (FLOAT32)G1_CHART_MACH_DELTA;
				Position++;
			}

			LastGNum = CurrentGNum;
			LastMachNum = CurrentMachNum;

		}

		if (Position >= G1_CHART_NUM_DIVISIONS)
		{
			break;
		}

	}

	fioClose(CurrentFile);

	return 0;
}



/************************************************************************************************
	_GetNewAmmo
*************************************************************************************************/

static UINT32 _GetNewAmmo(weapAmmo ** AmmoPtr)
{
weapAmmo * Ammo = nullptr;
weapAmmo * TempPtr1 = nullptr,*TempPtr2 = nullptr;

	if(AmmoPtr == nullptr) return 1;

	*AmmoPtr = nullptr;

	if (_AmmoFree.Next == nullptr)
	{
		Write_Log(LOG_WEAP,LOG_WARN,"Ammo free list empty");
		return 1;
	}

	Ammo = _AmmoFree.Next;

	if(_RemoveFromList(Ammo) == 1) return 1;
	if (_AddToListHead(Ammo) == 1) return 1;

	TempPtr1 = Ammo->Next;
	TempPtr2 = Ammo->Prev;
	memset(Ammo,0,sizeof(weapAmmo));
	Ammo->Next = TempPtr1;
	Ammo->Prev = TempPtr2;

    Ammo->inuse = true;

	*AmmoPtr = Ammo;

	_NumAmmo++;

	return 0;
}

/************************************************************************************************
	_KillAmmo
*************************************************************************************************/

static UINT32 _KillAmmo(weapAmmo * Ammo)
{
	if (Ammo == nullptr)
		return 0;

    Ammo->inuse = false;

	_RemoveFromList(Ammo);
	_AddToFreeList(Ammo);
	_NumAmmo--;

	weapfxAmmoDetachAmmo(Ammo);

	if(Ammo->Body)
	{
		physicsRemoveObject(Ammo->Body, true);
		delete Ammo->Body;
		Ammo->Body = 0;
	}

	return 0;
}

/************************************************************************************************
	_AddToFreeList
*************************************************************************************************/

static UINT32 _AddToFreeList(weapAmmo * Ammo)
{
	if(Ammo == nullptr) return 1;

	Ammo->Prev = &_AmmoFree;
	Ammo->Next = _AmmoFree.Next;
	if (_AmmoFree.Next != nullptr)
		_AmmoFree.Next->Prev = Ammo;
	_AmmoFree.Next = Ammo;

	return 0;
}

/************************************************************************************************
	_RemoveFromList
*************************************************************************************************/

static UINT32 _RemoveFromList(weapAmmo * Ammo)
{
	if (Ammo == nullptr) return 1;

	if (Ammo->Next != nullptr)
		Ammo->Next->Prev = Ammo->Prev;
	if (Ammo->Prev != nullptr)
		Ammo->Prev->Next = Ammo->Next;

	Ammo->Prev = Ammo->Next = nullptr;

	return 0;
}

/************************************************************************************************
	_AddToListHead
*************************************************************************************************/

static UINT32 _AddToListHead(weapAmmo * Ammo)
{
	if(Ammo == nullptr) return 1;

	Ammo->Prev = &_AmmoHead;
	Ammo->Next = _AmmoHead.Next;

	if (_AmmoHead.Next != nullptr)
		_AmmoHead.Next->Prev = Ammo;

	_AmmoHead.Next = Ammo;

	return 0;
}



/************************************************************************************************
	_DisplayWeaponsInFlight
*************************************************************************************************/

static bool _DisplayWeaponsInFlight(OBJECT * DummyObject)
{
    if(!obdrawIsRenderPass(RENDER_SCENE))
        return TRUE;

	weapAmmo* CurrentAmmo = nullptr;
    weapAmmo* NextAmmo = nullptr;

	if (_AmmoHead.Next != nullptr)
	{
		for (CurrentAmmo = _AmmoHead.Next; CurrentAmmo != nullptr;)
		{
			NextAmmo = CurrentAmmo->Next;

			if(CurrentAmmo->AmmoType && !CurrentAmmo->AmmoType->UseParticlesForTracer)
				_UpdateProjectileDisplay(CurrentAmmo);

			CurrentAmmo = NextAmmo;
		}
	}

	return 1;
}

/************************************************************************************************
	_UpdateProjectileDisplay
*************************************************************************************************/

static UINT32 _UpdateProjectileDisplay(weapAmmo * Ammo)
{
	piMATRIX Mat = IdentityMatrix;
    piMATRIX Mat1 = IdentityMatrix;
    piVECTOR Att = {0, 0, 0};
	FLOAT64 Speed = 0;
	FLOAT32 Stretch = 0;
	FLOAT32 Scale = 0;
    FLOAT64VECTOR Vec = {0, 0, 0};
	FLOAT32 DeltaTime = 0;
	OBJECT* AmmoDisplayObject = nullptr;

	// check for object
	if(!Ammo || !Ammo->AmmoType || !Ammo->AmmoType->Object) return 0;

    // Leave smoke grenade on ground for an alotted time.
	/* corpses are used to display grenade, so this is no longer needed
    if(Ammo->AmmoType->IsSmokeGrenade)
    {
        // Leave for 2.5 minutes
        if(frametime - (Ammo->LaunchTime * 1000) > 150000)
        {
            Ammo->KillAmmo = TRUE;
        }
    }
	*/

    if(Ammo->KillAmmo)
    {
        _KillAmmo(Ammo);
        return 0;
    }

	// only draw objects for the lifetime of the ammo
	if(Ammo->AmmoType->LifeTime > 0.0 && Ammo->LastUpdateTime > Ammo->LaunchTime + Ammo->AmmoType->LifeTime)
	{
		return 0;
	}

	// check for "ignore first frame" rounds
	if (Ammo->AmmoType->IgnoreFirstFrame == 1 && Ammo->Flags & WEAP_AMMOFLAG_FIRSTFRAME)
		Scale = 0.5;
	else
		Scale = 1.0;

	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO && !Ammo->TorpedoIsRunning)
	{
		Mat = Ammo->FireAtt;
	}
	else if(IsBouncingGrenade(Ammo->AmmoType))
	{
		//Matrixf rotation = Ammo->Body->GetRotationMatrix();
		//rotation.get(Mat);

		Att.x = (float)Ammo->Att.Pitch;
		Att.y = (float)Ammo->Att.Roll;
		Att.z = (float)Ammo->Att.Yaw;
		mathEul_ToMatrix(&Mat,EulOrdXYZs,&Att);
	}
	else
	{
		Att.y = 0;
		Att.z = (FLOAT32)atan2(Ammo->Velocity.x,Ammo->Velocity.y);
		Att.x = (FLOAT32)atan2(Ammo->Velocity.z,sqrt(Ammo->Velocity.x * Ammo->Velocity.x + Ammo->Velocity.y * Ammo->Velocity.y));

		mathEul_ToMatrix(&Mat,EulOrdXYZs,&Att);
	}

	Mat._41 = (FLOAT32)(Ammo->Position.x - WorldOriginOffset.x);
	Mat._42 = (FLOAT32)(Ammo->Position.y - WorldOriginOffset.y);
	Mat._43 = -(FLOAT32)(Ammo->Position.z + WorldOriginOffset.z);

	if((Ammo->AmmoType->GrenadeRotateRate > 0.001 || Ammo->AmmoType->GrenadeRotateRate < -0.001) && Ammo->AmmoType->FuzeTime == 0)
	{
		Att.z = Att.y = 0.0;
		Att.x = (FLOAT32)Ammo->Att.Pitch;
		mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att);
		math3dMatrixMultiply(&Mat,&Mat1,&Mat);
	}
	// check for "stretching" objects
	if (Ammo->AmmoType->StretchObject == 1)
	{
		if (Ammo->PrevRenderPos.x == 0.0 && Ammo->PrevRenderPos.y == 0.0 && Ammo->PrevRenderPos.z == 0.0)
		{
			// determine stretch distance
			Speed = mathExactVectorLength(&Ammo->Velocity);
		}
		else
		{
			mathSubVectors(&Vec,&Ammo->Position,&Ammo->PrevRenderPos);
			DeltaTime = (FLOAT32)(Ammo->LastUpdateTime - Ammo->PrevUpdateTime);
			Speed = mathExactVectorLength(&Vec) / DeltaTime;
		}

		if (Ammo->PrevUpdateTime < Ammo->LaunchTime)
			Stretch = (FLOAT32)(Speed * (Ammo->LastUpdateTime - Ammo->LaunchTime));
		else
			Stretch = (FLOAT32)(Speed * (Ammo->LastUpdateTime - Ammo->PrevUpdateTime));

		if (Stretch > 1.0)
			_StretchObject(Ammo->AmmoType->Object,Stretch * Scale);
		else
			_StretchObject(Ammo->AmmoType->Object,1.0);
	}

	if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO && Ammo->TorpedoIsRunning)
    {
        // Restart sound if necessary
        if(Ammo->SoundID == 0)
            weapfxTorpedo(Ammo);

		AmmoDisplayObject = nullptr;
    }
	else
		AmmoDisplayObject = Ammo->AmmoType->Object;

	if(AmmoDisplayObject)
	{
		ogrpMoveAndRotateSubObject(AmmoDisplayObject, &Mat, &Mat);
		piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);

		_DisplayObject(AmmoDisplayObject);
		piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);

		Mat = IdentityMatrix;
		ogrpMoveAndRotateSubObject(AmmoDisplayObject,&Mat,&Mat);
	}

	return 0;
}

/************************************************************************************************
	_StretchObject
*************************************************************************************************/
// stretch an object in the y axis

UINT32 _StretchObject(OBJECT* Object,FLOAT32 Stretch)
{
	OBJECT * Object1 = nullptr;
	UINT32 i = 0;

	if(Object == nullptr) return 0;

	for (Object1 = Object;Object1 != nullptr; Object1 = Object1->attached)
	{
		piVectorConstPtr ip = Object1->ImmutablePositions();
		piVectorPtr mp = Object1->MutablePositions();

		for (i = 0; i < Object1->numverts; i++, ++ip, ++mp)
		{
			mp->y = ip->y * Stretch;
		}

	}

	if (Object->child != nullptr)
		_StretchObject(Object->child,Stretch);

	if (Object->sibling != nullptr)
		_StretchObject(Object->sibling,Stretch);

	return 0;
}

/************************************************************************************************
	_DisplayObject
*************************************************************************************************/
// this routine handles all the subobjects but doesn't pay attention to type

static UINT32 _DisplayObject(OBJECT* Object)
{
	OBJECT* ObjAtt = nullptr;
    OBJECT* ObjSib = nullptr;

	if(Object == nullptr) return 0;

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			ObjAtt = ObjSib;

			do
			{
				if(!(ObjAtt->flag2 & OBJECT_HIDDEN))
				{
                    obdrawDrawObject(ObjAtt);
				}
				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
			_DisplayObject(ObjSib->child);

		ObjSib = ObjSib->sibling;
	}while (ObjSib != nullptr);

	return 0;
}



/************************************************************************************************
	_DoSpall
*************************************************************************************************/
// don't call this for internal components

static UINT32 _DoSpall(weapAmmo* Ammo,	FLOAT32 OriginalSpeed, FLOAT32 RemainingSpeed,
						FASTCOLLIDE* Collide, piVECTOR const* NewPos, piVECTOR const* OldPos, UINT32 Flags)
{
	UINT32 i;
	weapAmmo TempAmmo;
	weapAmmoType TempAmmoType;
	FLOAT64 CurrentTime = 0;
	FLOAT32 Energy = 0;
	UINT32 NumSpall = 0;
	FLOAT32 IngressSpallVel = 0,EgressSpallVel = 0;
	FLOAT32 IngressSpallMass = 0,EgressSpallMass = 0;
	FLOAT32 Division = 0;
    piVECTOR TempVec = {0, 0, 0};
    piVECTOR DirVec = {0, 0, 0};
    piVECTOR Att = {0, 0, 0};
    FLOAT64VECTOR SpallPnt = {0, 0, 0};
    FLOAT64VECTOR PrevPos = {0, 0, 0};
	FLOAT32 TempNum = 0;
	FLOAT32 TotalKE = 0;
	FLOAT32 Angle = 0;
	piMATRIX NormalToZeroMat = IdentityMatrix,NormalToZeroMatInv = IdentityMatrix;
	SINT32 OutsideCount = 0,InsideCount = 0;
	piMATRIX TempMat = IdentityMatrix;
	UINT32 flip = 0;
	FLOAT32 TempKE = 0;
	FLOAT32 PC = 0;

    memset(&TempAmmo, 0, sizeof(weapAmmo));
    memset(&TempAmmoType, 0, sizeof(weapAmmoType));

	if(!(Ammo->Flags & WEAP_AMMOFLAG_CANDAMAGE) || (Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)) return 0;

#ifdef BETA_RELEASE
	if(!SpallEnabled)
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP spalling disabled");
		return 0;
	}
#endif

	CurrentTime = frametime / 1000.0;

	strncpy((char*)TempAmmoType.Name, "spall", 31);

	TempAmmoType.Diameter = 0.001f;
	TempAmmoType.ProjectileWeight = 0.001f;
	TempAmmoType.PenetrationConstant = Ammo->AmmoType->PenetrationConstant * 0.1f;
	if(TempAmmoType.PenetrationConstant < 0.00000001f) TempAmmoType.PenetrationConstant = 0.00000001f;
	PC = TempAmmoType.PenetrationConstant;
	TempAmmoType.RichochetAngle = 89.9999f;

	Energy = 0.1f * 0.5f * Ammo->AmmoType->ProjectileWeight * (OriginalSpeed * OriginalSpeed - RemainingSpeed * RemainingSpeed);

	NumSpall = (UINT32)(pow(Energy,0.333333f) + 0.5) * 2;

	if (NumSpall == 0) return 0;

	Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) spalling %d pieces of %s",  Ammo->AmmoType->Name, Ammo->Id, NumSpall, Collide->CollideObject->name);

	IngressSpallVel = OriginalSpeed - RemainingSpeed;
	EgressSpallVel = RemainingSpeed;

	IngressSpallMass = (IngressSpallVel == 0.0f)? 0 : Energy * 2.0f / NumSpall / (IngressSpallVel * IngressSpallVel);
	EgressSpallMass  = (EgressSpallVel == 0.0f) ? 0 : Energy * 2.0f / NumSpall / (EgressSpallVel  * EgressSpallVel );

	Division = (RemainingSpeed * RemainingSpeed) / (OriginalSpeed * OriginalSpeed);
	TempVec = Collide->CollideObject->psnorms[Collide->PolyNum];
	TempMat = Collide->CollideObject->mat;
	TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
	math3dVectorMultiply(&TempVec,&TempVec,&TempMat);

	Att.x = Att.y = Att.z = 0.0f;
	Att.z = (FLOAT32)atan2(TempVec.x,TempVec.y);
	Att.x = (FLOAT32)atan2(sqrt(TempVec.y * TempVec.y + TempVec.x * TempVec.x),TempVec.z);
	mathEul_ToMatrixFast(&NormalToZeroMat,EulOrdXYZs,&Att);
	math3dInvertMatrix(&NormalToZeroMatInv,&NormalToZeroMat);

	math3dVectorMultiply(&TempVec,&Collide->CollidePnt,&Collide->CollideObject->mat);
	SpallPnt.x = TempVec.x + WorldOriginOffset.x;
	SpallPnt.y = TempVec.y + WorldOriginOffset.y;
	SpallPnt.z = -(TempVec.z + WorldOriginOffset.z);

	mathSubVectors(&DirVec,NewPos,OldPos);
	DirVec.z = -DirVec.z;
	TempNum = (FLOAT32)mathExactVectorLength(&DirVec);
	DirVec.x /= TempNum;
	DirVec.y /= TempNum;
	DirVec.z /= TempNum;

	TotalKE = 0.0f;
	InsideCount = 0;
	OutsideCount = 0;

	// check for flip
	TempVec.z = 1.0f;
	TempVec.x = TempVec.y = 0.0f;
	math3dVectorMultiply(&TempVec,&TempVec,&NormalToZeroMat);
	TempVec.z = -TempVec.z;

	flip = (math3dDotProduct(&TempVec,&DirVec) < 0) ? 1 : 0;

	for (i=0;i<NumSpall;i++)
	{
		if (mathGetFloat32ChaoticNumber() > Division)
		{
			OutsideCount++;
			// Outside

			// velocity
			TempVec.x = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.0f;
			TempVec.y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.0f;
			TempVec.z = mathGetFloat32ChaoticNumber() * 300.0f;

			Angle = 2 * PI * mathGetFloat32ChaoticNumber();

			TempNum = IngressSpallVel * mathGetFloat32ChaoticNumber();
			TempVec.x += (FLOAT32)(TempNum * sin(Angle));
			TempVec.y += (FLOAT32)(TempNum * cos(Angle));

			math3dVectorMultiply(&TempVec,&TempVec,&NormalToZeroMat);

			// ensure the spall is going the right way :)
			// the reason it might not be is because collisions can occur
			// going either direction on some hitmap polys
			if (!flip)
			{
				TempVec.x = -TempVec.x;
				TempVec.y = -TempVec.y;
				TempVec.z = -TempVec.z;
			}

			// ok setup the ammo and ammotype
			TempAmmoType.ProjectileWeight = IngressSpallMass * 2.0f * mathGetFloat32ChaoticNumber();;

			if (Ammo->AmmoType->ProjectileWeight > 0.0000001)
				TempAmmoType.PenetrationConstant = PC * TempAmmoType.ProjectileWeight /  Ammo->AmmoType->ProjectileWeight;
			else
				TempAmmoType.PenetrationConstant = 0.0000000001f;

			memset(&TempAmmo,0,sizeof(weapAmmo));
			TempAmmo.AmmoType = &TempAmmoType;

			TempAmmo.Velocity.x = TempVec.x;
			TempAmmo.Velocity.y = TempVec.y;
			TempAmmo.Velocity.z = -TempVec.z;

			TempVec.x = TempVec.z = 0.0f;
			TempVec.y = 0.001f;
			math3dVectorMultiply(&TempVec,&TempVec,&NormalToZeroMat);

			mathAddVectors(&PrevPos,&TempVec,&SpallPnt);

			TempVec.x = (FLOAT32)(TempAmmo.Velocity.x * 0.01);
			TempVec.y = (FLOAT32)(TempAmmo.Velocity.y * 0.01);
			TempVec.z = (FLOAT32)(TempAmmo.Velocity.z * 0.01);
			TempAmmo.Position.x = PrevPos.x + TempVec.x;
			TempAmmo.Position.y = PrevPos.y + TempVec.y;
			TempAmmo.Position.z = PrevPos.z + TempVec.z;

			TempNum = (FLOAT32)mathExactVectorLength(&TempVec);

			TempAmmo.Flags = WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_USE_MEDCOLLIDELIST |
							 WEAP_AMMOFLAG_DONTCHECKTERRAIN | WEAP_AMMOFLAG_NOFX  | WEAP_AMMOFLAG_IS_SPALL;

			TempAmmo.Id = i;

			TempKE = (FLOAT32)mathExactVectorLength(&TempAmmo.Velocity) *
						(FLOAT32)mathExactVectorLength(&TempAmmo.Velocity) * 0.5f * TempAmmoType.ProjectileWeight;

			Write_Log(LOG_WEAP, LOG_INFO,"WEAP %s(%d) reverse spall", TempAmmoType.Name, TempAmmo.Id);
			if(_DoProjectileUpdate(&TempAmmo, &PrevPos, 0.01f, CurrentTime) == 0)
				Write_Log(LOG_WEAP, LOG_INFO,"WEAP %s(%d) no hit", TempAmmoType.Name, TempAmmo.Id);

			TotalKE += TempKE;
		}
		else
		{
			InsideCount++;
			// Inside

			// these are in a "cone" in the direction of fire
			// first let's get some generic dispersion along the normal

			// velocity
			TempVec.x = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.0f;
			TempVec.y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.0f;
			TempVec.z = mathGetFloat32ChaoticNumber() * 300.0f;
			math3dVectorMultiply(&TempVec,&TempVec,&NormalToZeroMat);

			// ensure the spall is going the right way :)
			// the reason it might not be is because collisions can occur
			// going either direction on some hitmap polys
			if (flip)
			{
				TempVec.x = -TempVec.x;
				TempVec.y = -TempVec.y;
				TempVec.z = -TempVec.z;
			}

			TempNum = mathGetFloat32ChaoticNumber() * 2.0f * EgressSpallVel;
			TempVec.x += TempNum * DirVec.x;
			TempVec.y += TempNum * DirVec.y;
			TempVec.z += TempNum * DirVec.z;

			// ok setup the ammo and ammotype
			TempAmmoType.ProjectileWeight = EgressSpallMass * 2.0f * mathGetFloat32ChaoticNumber();;

			if (Ammo->AmmoType->ProjectileWeight > 0.0000001)
				TempAmmoType.PenetrationConstant = PC * TempAmmoType.ProjectileWeight /  Ammo->AmmoType->ProjectileWeight;
			else
				TempAmmoType.PenetrationConstant = 0.0000000001f;

			memset(&TempAmmo,0,sizeof(weapAmmo));
			TempAmmo.AmmoType = &TempAmmoType;

			TempAmmo.Velocity.x = TempVec.x;
			TempAmmo.Velocity.y = TempVec.y;
			TempAmmo.Velocity.z = -TempVec.z;

			TempVec.x = TempVec.z = 0.0f;
			TempVec.y = 0.001f;
			math3dVectorMultiply(&TempVec,&TempVec,&NormalToZeroMat);

			mathAddVectors(&PrevPos,&TempVec,&SpallPnt);

			TempVec.x = (FLOAT32)(TempAmmo.Velocity.x * 0.01);
			TempVec.y = (FLOAT32)(TempAmmo.Velocity.y * 0.01);
			TempVec.z = (FLOAT32)(TempAmmo.Velocity.z * 0.01);
			TempAmmo.Position.x = PrevPos.x + TempVec.x;
			TempAmmo.Position.y = PrevPos.y + TempVec.y;
			TempAmmo.Position.z = PrevPos.z + TempVec.z;

			TempNum = (FLOAT32)mathExactVectorLength(&TempVec);


			TempAmmo.Flags = WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_USE_MEDCOLLIDELIST |
							 WEAP_AMMOFLAG_DONTCHECKTERRAIN | WEAP_AMMOFLAG_NOFX | WEAP_AMMOFLAG_IS_SPALL;

			TempAmmo.Id = i;

			TempKE = (FLOAT32)mathExactVectorLength(&TempAmmo.Velocity) *
						(FLOAT32)mathExactVectorLength(&TempAmmo.Velocity) * 0.5f * TempAmmoType.ProjectileWeight;

			Write_Log(LOG_WEAP, LOG_INFO,"WEAP %s(%d) direct spall", TempAmmoType.Name, TempAmmo.Id);
			if(_DoProjectileUpdate(&TempAmmo, &PrevPos, 0.01f, CurrentTime) == 0)
				Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) no hit", TempAmmoType.Name, TempAmmo.Id);

			TotalKE += TempKE;
		}
#ifdef BETA_RELEASE
		FLOAT64VECTOR CurPos = TempAmmo.Position;
		char name[32];
		snprintf(name, 31, "spall%d\n", i);
		int id = draw3dSetWorldDebugLine(&PrevPos, &CurPos, name, colorYELLOWALPHA);
		draw3dSetDebugObjectFlipZ(id, TRUE);
		draw3dSetDebugObjectAlwaysOnTop(id, FALSE);
#endif
	}

	return 0;
}

/************************************************************************************************
	_DoExplosionShrapnel
*************************************************************************************************/
// throws out shrapnel for explosion

static UINT32 _DoExplosionShrapnel(weapAmmo* Ammo, FLOAT64VECTOR const* InPos, FLOAT64VECTOR const* ProjVel, FLOAT64 CurrentTime, bool doshrap)
{
	UINT32 i = 0;
	FLOAT32 Energy = 0;
	UINT32 NumSpall = 0;
	FLOAT32 SpallVel = 0;
    FLOAT32 SpallMass = 0;
    piVECTOR TempVec = {0, 0, 0};
    piVECTOR DirVec = {0, 0, 0};
    FLOAT64VECTOR PrevPos = {0, 0, 0};
	FLOAT32 TempNum = 0;
	FLOAT32 PC = 0;
	weapAmmo TempAmmo;
	weapAmmoType TempAmmoType;

	if(!(Ammo->Flags & WEAP_AMMOFLAG_CANDAMAGE) || (Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)) return 0;
	if(Ammo->AmmoType->NumShrapnelPieces < 1) return 0;

    memset(&TempAmmoType,0,sizeof(weapAmmoType));

#ifdef BETA_RELEASE
	if(!ShrapnelEnabled)
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP shrapnel disabled");
		return 0;
	}
#endif

	if (Ammo->AmmoType->ShapedChargePen > 0)
		_DoShapedCharge(Ammo, InPos, ProjVel, CurrentTime);

	if(!doshrap) return 0;

#ifdef GREN_STAT_MODEL
	if (Ammo->AmmoType->UseNewHE)
		return _DoImprovedShrapnel (Ammo, InPos, ProjVel, CurrentTime);
#endif

	strncpy((char*)TempAmmoType.Name, "shrapnel", 31);

	TempAmmoType.Diameter = 0.001f;
	TempAmmoType.ProjectileWeight = 0.001f;
	if(Ammo->AmmoType->ShrapnelPenetrationConstant)
		TempAmmoType.PenetrationConstant = Ammo->AmmoType->ShrapnelPenetrationConstant;
	else
		TempAmmoType.PenetrationConstant = Ammo->AmmoType->PenetrationConstant * 0.1f;

	if (TempAmmoType.PenetrationConstant < 0.00000001f)
		TempAmmoType.PenetrationConstant = 0.00000001f;

	PC = TempAmmoType.PenetrationConstant;
	TempAmmoType.RichochetAngle = 89.9999f;

	Energy = Ammo->AmmoType->ExplosiveEnergy * 0.3f;
	NumSpall = (UINT32)(Ammo->AmmoType->NumShrapnelPieces * (mathGetFloat32ChaoticNumber() + 0.5f));

	if (NumSpall == 0) return 0;

	Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) explodes into %d pieces of shrapnel",  Ammo->AmmoType->Name, Ammo->Id, NumSpall);

	SpallMass = Ammo->AmmoType->TotalShrapnelMass / NumSpall;

	SpallVel = (FLOAT32)sqrt(2 * Energy / Ammo->AmmoType->TotalShrapnelMass);

	for (i=0;i<NumSpall;i++)
	{
		// random velocity 300 m/s is totally arbitrary
		TempVec.x = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.f;
		TempVec.y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.f;
		TempVec.z = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 300.f;

		DirVec = TempVec;
		TempNum = (FLOAT32)mathExactVectorLength(&DirVec);
		DirVec.x /= TempNum; DirVec.y /= TempNum; DirVec.z /= TempNum;

		// add in some explosive velocity
		TempNum = SpallVel * mathGetFloat32ChaoticNumber();
		TempVec.x += TempNum * DirVec.x; TempVec.y += TempNum * DirVec.y; TempVec.z += TempNum * DirVec.z;

		// add in projectile's velocity
		TempVec.x += (FLOAT32)ProjVel->x; TempVec.y += (FLOAT32)ProjVel->y; TempVec.z += (FLOAT32)ProjVel->z;

		// ok setup the ammo and ammotype
		TempAmmoType.ProjectileWeight = SpallMass * 2.0f * mathGetFloat32ChaoticNumber();;

		if (Ammo->AmmoType->ProjectileWeight > 0.0000001)
			TempAmmoType.PenetrationConstant = PC * TempAmmoType.ProjectileWeight / Ammo->AmmoType->ProjectileWeight;
		else
			TempAmmoType.PenetrationConstant = 0.0000000001f;

		memset(&TempAmmo,0,sizeof(weapAmmo));
		TempAmmo.AmmoType = &TempAmmoType;

		TempAmmo.Velocity.x = TempVec.x; TempAmmo.Velocity.y = TempVec.y; TempAmmo.Velocity.z = TempVec.z;

		PrevPos = *InPos;

		TempAmmo.Position = PrevPos;
		TempVec.x *= 0.03f; TempVec.y *= 0.03f; TempVec.z *= 0.03f;
		TempAmmo.Position.x += TempVec.x; TempAmmo.Position.y += TempVec.y; TempAmmo.Position.z += TempVec.z;

		TempAmmo.Flags = WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_USE_MEDCOLLIDELIST |
						 WEAP_AMMOFLAG_DONTCHECKTERRAIN | WEAP_AMMOFLAG_NOFX | WEAP_AMMOFLAG_IS_SHRAPNEL;
		TempAmmo.Id = i;

		if(_DoProjectileUpdate(&TempAmmo, &PrevPos, 0.03, CurrentTime + 0.03) == 0)
			Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) no hit", TempAmmoType.Name, TempAmmo.Id);

#ifdef BETA_RELEASE
		FLOAT64VECTOR CurPos = TempAmmo.Position;
		char name[32];
		snprintf(name, 31, "shrap%d\n", i);
		int id = draw3dSetWorldDebugLine(&PrevPos, &CurPos, name, colorREDALPHA);
		draw3dSetDebugObjectFlipZ(id, TRUE);
		draw3dSetDebugObjectAlwaysOnTop(id, FALSE);
#endif
	}

	return 0;
}

/************************************************************************************************
	_DoImprovedShrapnel
*************************************************************************************************/
// throws out shrapnel for explosion
// as a child of _DoExplosionShrapnel it skips a lot of the error checking

static UINT32 _DoImprovedShrapnel(weapAmmo* Ammo, FLOAT64VECTOR const* InPos, FLOAT64VECTOR const* ProjVel, FLOAT64 CurrentTime)
{
	// linear scale between min and max piece counts
	UINT32 numPieces = (UINT32) ((Ammo->AmmoType->MinShrapPieces) +
		((FLOAT32)(Ammo->AmmoType->MaxShrapPieces - Ammo->AmmoType->MinShrapPieces) * mathGetFloat32ChaoticNumber()));

	if (numPieces == 0) return 0;

	UINT32				i = 0;
	weapAmmo			TempAmmo;
	weapAmmoType		TempAmmoType;
	FLOAT32				shrapEnergy = 0;
	FLOAT32				maxPieceSpeed = 0;
	FLOAT32				avgMass = 0;
	FLOAT64VECTOR		PrevPos;
	FLOAT32				PC = 0;
	vehVehicle*			CurrentVehicle = nullptr;
	vehVehicle*			PlayerVehicle = persGetPersonaVehiclePtr();
	trnAutoGun*			AG = nullptr;
	UINT32				maxNumShrapnel = 0;
	shrapTarget			shrapTargetInfo;
	std::list<shrapTarget>	shrapList;
	std::list<shrapTarget>::iterator it;

	memset(&TempAmmoType,0,sizeof(weapAmmoType));
	strncpy((char*)TempAmmoType.Name, "shrapnel", 31);

	// first, calculate a bunch of stuff that is shared by all shrapnel bits for this explosion
	// second, get target data (vechiles & autogunners)
	// third, blow shrapnel at each target as appropriate

	TempAmmoType.Diameter = Ammo->AmmoType->ShrapProjDiameter;
	if(Ammo->AmmoType->ShrapnelPenetrationConstant)
		TempAmmoType.PenetrationConstant = Ammo->AmmoType->ShrapnelPenetrationConstant;
	else
		TempAmmoType.PenetrationConstant = Ammo->AmmoType->PenetrationConstant * 0.1f;

	if (TempAmmoType.PenetrationConstant < Ammo->AmmoType->ShrapProjMinPenConst)
		TempAmmoType.PenetrationConstant = Ammo->AmmoType->ShrapProjMinPenConst;

	PC = TempAmmoType.PenetrationConstant;
	TempAmmoType.RichochetAngle = Ammo->AmmoType->ShrapProjRicochetAngle;

	shrapEnergy = Ammo->AmmoType->ExplosiveEnergy * Ammo->AmmoType->ShrapEnergyMult;

	Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) explodes into %d pieces of shrapnel",  Ammo->AmmoType->Name, Ammo->Id, numPieces);

	avgMass = Ammo->AmmoType->TotalShrapnelMass / numPieces;

	maxPieceSpeed = (FLOAT32)sqrt(2 * shrapEnergy / Ammo->AmmoType->TotalShrapnelMass);

	if (Ammo->AmmoType->MaxShrapPerVehicle > 0)
	{
		maxNumShrapnel = Ammo->AmmoType->MaxShrapPerVehicle;
	}
	else
		maxNumShrapnel = 75;

	// OGS: The above code reads "if MSPV > 0 then use that value, otherwise use 75"
	// so the following if is pointless.
//	if (maxNumShrapnel <= 0) return 0;

	// vehicles are targets
	vehGetVehicleHead(&CurrentVehicle);
	for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if (CurrentVehicle->VehicleObjectData == nullptr) continue;

		if (CurrentVehicle == PlayerVehicle && PlayerVehicle->ModelData != nullptr)
		{
			shrapTargetInfo.targetPos = CurrentVehicle->ModelData->RenderPos;
		}
		else
		{
			shrapTargetInfo.targetPos = CurrentVehicle->PosData.RenderPos;
		}

		shrapTargetInfo.radius = std::max(1.0, CurrentVehicle->VehicleObjectData->Radius - 1.0);
		shrapTargetInfo.target = (void*)CurrentVehicle;
		shrapTargetInfo.maxDist2 = 10000.0;

		// infantry center points are at their feet so half of all shrapnel always miss
 		if(CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON)
		{
			shrapTargetInfo.radius = shrapTargetInfo.radius/2;
			shrapTargetInfo.targetPos.z += shrapTargetInfo.radius;
		}

		shrapList.push_back(shrapTargetInfo);
	}

	// so are autogunners..
	trnGetAutoGunHead(&AG);
	for(;AG; AG = AG->next)
	{
		shrapTargetInfo.targetPos = AG->Pos;
		// hardcoded, straight out of stuff below
		shrapTargetInfo.radius = 5.0;
		shrapTargetInfo.target = (void*)AG;
		shrapTargetInfo.maxDist2 = 625.0f;

		shrapList.push_back(shrapTargetInfo);
	}

	// now iterate the target list and blow shrapnel at them
	it = shrapList.begin();
	for(; it!= shrapList.end(); ++it)
	{
        FLOAT64VECTOR	distVect = {0, 0, 0}, centerVect = {0, 0, 0};
		FLOAT32			massMultiplier = 1.0;
		piMATRIX		centerMatrix = IdentityMatrix;
		piMATRIX		rotYMatrix = IdentityMatrix, rotZMatrix = IdentityMatrix;

		// using radius of vehicle, and distance to vehicle .. use them to figure what % of shrapnel could possibly
		// hit a sphere of that size at that distance
		// surf area of sphere = 4 * pi * r2 .. surf area of cutout is 2 * pi * r2 * (1 - cos(theta))
		// % of surf area of sphere covered by a cutout of angle theta is
		// cutout area / sphere area
		// 0.5 * (1-cos(theta))

		mathSubVectors (&distVect, &it->targetPos, InPos);
		FLOAT64 dist2 = mathExactVectorLength2 (&distVect);

		// quick max distance (squared) check
		if (dist2 > it->maxDist2)
			continue;

		FLOAT64 dist = (FLOAT32) sqrt(dist2);
		FLOAT64 coneAngle = atan2 (it->radius, dist);	// how large is the spray cone for a given target
        FLOAT64 shrapPercent = 0.5 * (1.0 - cos(coneAngle));
		UINT32 finalShrapCount = (SINT32) (numPieces * shrapPercent);

		if (finalShrapCount > maxNumShrapnel)
		{
			// protected from divide by 0 from earlier check
			massMultiplier = finalShrapCount / (FLOAT32)maxNumShrapnel;
			if (massMultiplier > Ammo->AmmoType->MaxMassMultiplier)
				massMultiplier = Ammo->AmmoType->MaxMassMultiplier;

			Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) too much shrapnel %u, reducing to %u .. using mass multiplier %2.2f out of possible %2.2f",
				Ammo->AmmoType->Name, Ammo->Id,
				finalShrapCount, maxNumShrapnel, massMultiplier, (finalShrapCount / (FLOAT32)maxNumShrapnel));

			finalShrapCount = maxNumShrapnel;
		}

		Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) target dist %f, radius %f, theta %f, percent %2.4f",  Ammo->AmmoType->Name, Ammo->Id, dist, it->radius, (mlRAD_TO_DEG(coneAngle)), shrapPercent*100.0);
		Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) sends %d pieces of shrapnel at target (max %d)",
			Ammo->AmmoType->Name, Ammo->Id, finalShrapCount, maxNumShrapnel);

		if (finalShrapCount <= 0) continue;

   		mathScaleVector (&centerVect, &distVect, (1.0/dist));

		// decompose the center direction into euler angles..
		FLOAT32 rotZ = (FLOAT32) atan2 (centerVect.y, centerVect.x);
		FLOAT32 rotY = (FLOAT32) atan2 (centerVect.z, sqrt((centerVect.x*centerVect.x)+(centerVect.y*centerVect.y)));

		generateRotationMatrix (AROUND_Z, mlRAD_TO_DEG(rotZ), &rotZMatrix);
		generateRotationMatrix (AROUND_Y, mlRAD_TO_DEG(rotY), &rotYMatrix);

		math3dMatrixMultiply (&centerMatrix, &rotYMatrix, &rotZMatrix);

		Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) angles %2.4f, %2.4f",  Ammo->AmmoType->Name, Ammo->Id, mlRAD_TO_DEG(rotZ), mlRAD_TO_DEG(rotY));

		for (i=0;i<finalShrapCount;i++)
		{
			// start with a unit vector along X .. bend it to be randomized in cone .. then swivel it based on the above matrix
            piVECTOR		pieceVel = {1, 0, 0};
			piMATRIX		dispRotXMat = IdentityMatrix;
            piMATRIX        dispRotYMat = IdentityMatrix;
            piMATRIX        dispRotMat = IdentityMatrix;

			// now "bend" the velocity based on the a random position inside the cone
			FLOAT32 dispRotY = mathGetFloat32ChaoticNumber() * (FLOAT32)coneAngle;
			FLOAT32 dispRotX = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * PI;

			generateRotationMatrix (AROUND_Y, mlRAD_TO_DEG(dispRotY), &dispRotYMat);
			generateRotationMatrix (AROUND_X, mlRAD_TO_DEG(dispRotX), &dispRotXMat);

			math3dMatrixMultiply (&dispRotMat, &dispRotYMat, &dispRotXMat);

			math3dMultiplyRotation (&pieceVel, &pieceVel, &dispRotMat);

			math3dMultiplyRotation (&pieceVel, &pieceVel, &centerMatrix);

//			startSpeed = (mathGetFloat32ChaoticNumber()) * Ammo->AmmoType->AddedParticleSpeed;
			FLOAT32 startSpeed = Ammo->AmmoType->AddedParticleSpeed;

			FLOAT32 pieceSpeed = maxPieceSpeed * mathGetFloat32ChaoticNumber();

			// trivial check, see if the particle even has enough speed to reach the target in the time allotted for it
			if (((startSpeed + pieceSpeed) * Ammo->AmmoType->ParticleTimeout) < dist)
				continue;

			mathScaleVector (&pieceVel, &pieceVel, (startSpeed + pieceSpeed));

			pieceVel.y = -pieceVel.y;
			pieceVel.z = -pieceVel.z;

			Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) x = %6.4f y=%6.4f z=%6.4f", Ammo->AmmoType->Name, Ammo->Id, pieceVel.x, pieceVel.y, pieceVel.z);

			TempAmmoType.ProjectileWeight = avgMass * 2.0f * mathGetFloat32ChaoticNumber() * massMultiplier;
			TempAmmoType.ProjectileWeight = std::max(TempAmmoType.ProjectileWeight, Ammo->AmmoType->ShrapProjWeight);

			if (Ammo->AmmoType->ProjectileWeight > 0.0000001)
				TempAmmoType.PenetrationConstant = PC * TempAmmoType.ProjectileWeight / Ammo->AmmoType->ProjectileWeight;
//			else
//				TempAmmoType.PenetrationConstant = 0.0000000001f;

			memset(&TempAmmo,0,sizeof(weapAmmo));
			TempAmmo.AmmoType = &TempAmmoType;

			TempAmmo.Velocity.x = pieceVel.x;
			TempAmmo.Velocity.y = pieceVel.y;
			TempAmmo.Velocity.z = pieceVel.z;

			PrevPos = *InPos;
			TempAmmo.Position = PrevPos;

			TempAmmo.Position.x += pieceVel.x * Ammo->AmmoType->ParticleTimeout;
			TempAmmo.Position.y += pieceVel.y * Ammo->AmmoType->ParticleTimeout;
			TempAmmo.Position.z += pieceVel.z * Ammo->AmmoType->ParticleTimeout;

			// specific vehicle only so turn off terrain, ground object collisions
			TempAmmo.Flags = WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_USE_MEDCOLLIDELIST |
				WEAP_AMMOFLAG_DONTCHECKTERRAIN |
				WEAP_AMMOFLAG_DAMAGE_SPECIFIC_OBJECT |
				WEAP_AMMOFLAG_NOFX | WEAP_AMMOFLAG_IS_SHRAPNEL;
			TempAmmo.Id = i;

			TempAmmo.SpecificDamageObject = it->target;

			if(_DoProjectileUpdate(&TempAmmo, &PrevPos, Ammo->AmmoType->ParticleTimeout, CurrentTime + Ammo->AmmoType->ParticleTimeout) == 0)
				Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) no hit", TempAmmoType.Name, TempAmmo.Id);

#ifdef BETA_RELEASE
			FLOAT64VECTOR CurPos = TempAmmo.Position;
			char name[32];
			snprintf(name, 31, "shrap%d\n", i);
			int id = draw3dSetWorldDebugLine(&PrevPos, &CurPos, name, colorREDALPHA);
			draw3dSetDebugObjectFlipZ(id, TRUE);
			draw3dSetDebugObjectAlwaysOnTop(id, FALSE);
#endif
		}
	}
	return 0;
}

/************************************************************************************************
	_DoShapedCharge
*************************************************************************************************/
#include "math/mathUnitConversion.h"
static UINT32 _DoShapedCharge(weapAmmo* Ammo, FLOAT64VECTOR const* InPos, FLOAT64VECTOR const* ProjVel, FLOAT64 CurrentTime)
{
	weapAmmo ShapeChargeAmmo;
	weapAmmoType AmmoType;
	FLOAT32 DeltaTime = 0;

	Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) shaped charge with %.2f mm penetration explodes",  Ammo->AmmoType->Name, Ammo->Id, 1000 * Ammo->AmmoType->ShapedChargePen);

	memset(&AmmoType, 0x0, sizeof(weapAmmoType));
	memset(&ShapeChargeAmmo, 0x0, sizeof(weapAmmo));
	strncpy((char*)AmmoType.Name, "shaped charge jet", 31);

	AmmoType.ShapedChargePen = Ammo->AmmoType->ShapedChargePen;
	AmmoType.PenetrationConstant = Ammo->AmmoType->ShapedChargePen / 63096.0f; // from mm to pc units I guess
	if(AmmoType.PenetrationConstant < 0.00000001f) AmmoType.PenetrationConstant = 0.00000001f;
	AmmoType.RichochetAngle = 89.9999f;
	AmmoType.ModelSpall = 0;
	AmmoType.ProjectileWeight = Ammo->AmmoType->ProjectileWeight * 0.1f;
	AmmoType.ExplosiveEnergy = Ammo->AmmoType->ExplosiveEnergy;
	AmmoType.Diameter = Ammo->AmmoType->Diameter;
	AmmoType.SubType = Ammo->AmmoType->SubType;
	strsafecpy(AmmoType.ExplosionNameFar, Ammo->AmmoType->ExplosionNameFar);
	strsafecpy(AmmoType.ExplosionNameNear,Ammo->AmmoType->ExplosionNameNear);

	ShapeChargeAmmo.AmmoType = &AmmoType;

	FLOAT64VECTOR Vel;
	if (Ammo->Flags & WEAP_AMMOFLAG_ISATTACHED_TO_OBJECT)
	{
		piMATRIX mat = Ammo->AttachedObject->mat;
		piVECTOR Norm;
		mat._41 = mat._42 = mat._43 = 0.0f;
		Norm = Ammo->OrigNorm;
		math3dVectorMultiply(&Norm, &Norm, &mat);
		Norm.z = -Norm.z;

		Vel.x = Norm.x; Vel.y = Norm.y; Vel.z = Norm.z;
	}else
	{
		Vel = *ProjVel;
	}

    Vec3d velocity(Vel);

    double mag = velocity.length();
    velocity.normalize();

    // Make a matrix that takes a ray along the y axis and orients it to the velocity vector
    Matrixd orient;
    orient.makeRotate(velocity, Vec3d(0, 1, 0));
    orient.invert(orient);

    Vec3d start(0, 1, 0);
    std::vector<Vec3d> velocities;

    // Store cone rays. Do this in object space, then translate to world space
    velocities.push_back(start);

    // Rotate up 30 degrees
    Matrixd rot;
    rot.makeRotate(DEG_TO_RAD(30), Vec3d(1, 0, 0));

    start = start * rot;

    // Rotate point around y axis in 30 degree increments.
    rot.makeRotate(DEG_TO_RAD(30), Vec3d(0, 1, 0));
    velocities.push_back(start);

    // This should be enough. Was 5 rays previously
    for(int i = 0; i < 12; i++)
    {
        start = start * rot;
        velocities.push_back(start);
    }

    for(unsigned int i = 0; i < velocities.size(); i++)
    {
        // Restore magnitude
        velocities[i] = velocities[i] * mag;
        // Orient to world space
        velocities[i] = velocities[i] * orient;
    }

	// quick hack to make a "cone of death" in a vain attempt to make shape charge effective
	for(unsigned int i = 0; i < velocities.size(); i++)
	{
		AmmoType.ShapedChargePen = Ammo->AmmoType->ShapedChargePen;

		// full energy / sure why not
		AmmoType.ExplosiveEnergy = Ammo->AmmoType->ExplosiveEnergy;

		velocities[i].get(ShapeChargeAmmo.Velocity);

		DeltaTime = Ammo->AmmoType->ShapedChargePen / 100.0f; // arbitrary

		// arbitrary length collide ray
		ShapeChargeAmmo.Position = *InPos;
		ShapeChargeAmmo.Position.x += 100 * ShapeChargeAmmo.Velocity.x * Ammo->AmmoType->ShapedChargePen;
		ShapeChargeAmmo.Position.y += 100 * ShapeChargeAmmo.Velocity.y * Ammo->AmmoType->ShapedChargePen;
		ShapeChargeAmmo.Position.z += 100 * ShapeChargeAmmo.Velocity.z * Ammo->AmmoType->ShapedChargePen;

		ShapeChargeAmmo.Flags = WEAP_AMMOFLAG_CANDAMAGE | WEAP_AMMOFLAG_USE_MEDCOLLIDELIST |
								WEAP_AMMOFLAG_DONTCHECKTERRAIN | WEAP_AMMOFLAG_IS_SHAPED_CHARGE | WEAP_AMMOFLAG_NOFX;
		ShapeChargeAmmo.Id = Ammo->Id;

		if(_DoProjectileUpdate(&ShapeChargeAmmo, InPos, DeltaTime, CurrentTime + DeltaTime) == 0)
			Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) no hit", AmmoType.Name, ShapeChargeAmmo.Id);
	}

	return 0;
}


/************************************************************************************************
	_TraverseHitTreeAndApplyConcussionDamage
*************************************************************************************************/
// Note to self, completely rewrite our concussion model!

static UINT32 _TraverseHitTreeAndApplyConcussionDamage(weapAmmo* Ammo, vehVehicle* CurrentVehicle,  OBJECT* CurrentObject,
														const FLOAT64& Energy, piVECTOR* Epicenter, SINT32 Counter, UINT32 UseSibs,
														LOGICAL ExplosionInternal, LOGICAL ComponentInternal)
{
	OBJECT * ObjSib = nullptr;
    piVECTOR Pos = {0, 0, 0};
	FLOAT64 Damage = 0;
	FLOAT32 Dist = 0;

	if (CurrentObject == nullptr) return 0;

	ObjSib = CurrentObject;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			if (ObjSib->trnum > 0)
			{
				math3dVectorMultiply(&Pos, &ObjSib->pos, &ObjSib->mat);
				mathSubVectors(&Pos, &Pos, Epicenter);
				Dist = (FLOAT32)mathExactVectorLength(&Pos);

//				Dist += CurrentVehicle->VehicleType->DamageConcussionResistance[ObjSib->trnum-1]; // Old crappy way
				if(!ExplosionInternal && CurrentVehicle->VehicleType->VehicleRealMass && ComponentInternal)
					Dist += 35; //  New crappy way. 35m is old value used as baseline DamageConcussionResistance of char, we'll the scale damage based on mass.

				if (Dist < 0.01f) Dist = 0.01f;

				Damage = Energy / (95.0f * 95.0f * Dist * Dist); // 95 a completely arbitrary value pulled out of Hoof's ass.

				if(!ExplosionInternal && CurrentVehicle->VehicleType->VehicleRealMass && ComponentInternal)
					Damage *= 31500 / CurrentVehicle->VehicleType->VehicleRealMass; // Scale based proportional to vehicle Mass. 31500kg is mass of the Char.

				if (Damage > Energy) Damage = Energy;

				if (Damage > 10.0f)
				{
					Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s concussion to component %s of %s(%s) at %.1f m",
								ExplosionInternal ? "internal" : "external", ObjSib->name, CurrentVehicle->VehicleType->Id, CurrentVehicle->Handle, Dist);
					weapAmmoApplyDamageToVehicleComponent(Ammo, CurrentVehicle, ObjSib->trnum, Damage, Counter);
				}
			}
		}

		if (ObjSib->child)
			_TraverseHitTreeAndApplyConcussionDamage(Ammo, CurrentVehicle, ObjSib->child, Energy, Epicenter, Counter, 1, ExplosionInternal, ComponentInternal);

		ObjSib = ObjSib->sibling;
	} while (ObjSib && UseSibs);

	return 0;
}

/************************************************************************************************
	_DoExplosionConcussion
*************************************************************************************************/

static UINT32 _DoExplosionConcussion(weapAmmo* Ammo, FLOAT64VECTOR const* InPos, LOGICAL Internal)
{
	mlVehicleData * PlayerModelData = persGetPersonaModelDataPtr();
	trnCDTerrainObject * TOHead = nullptr;
	trnCDTerrainObject * TO = nullptr;
	trnCDTerrainObject ** UseTO = nullptr;
	FLOAT64 Energy = 0;
	FLOAT64 TempNum = 0;
	FLOAT64 MaxDistSqr = 0;
	FLOAT64 DistSqr = 0;
	piVECTOR TempVec = {0, 0, 0};
	FLOAT64 Damage = 0;
    piVECTOR Pos = {0, 0, 0},Epicenter = {0, 0, 0};
	vehVehicle* CurrentVehicle = nullptr;
	vehVehicle* PlayerVehicle = nullptr;
	FASTCOLLIDE CollideList[_MAX_NUM_OF_COLLISIONS];
	FASTCOLLIDE * CollideListPtr = nullptr;
	UINT32 TempCDNum = 0;
	SINT32 NumCollides = 0;
	SINT32 MaxNumInList = 0;
	SINT32 i = 0;

	PlayerVehicle = persGetPersonaVehiclePtr();

    // Leave here to guarantee visible concussion effect
	{
		persSetConcussionEffect(Ammo);
	}

	if(!(Ammo->Flags & WEAP_AMMOFLAG_CANDAMAGE) || (Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)) return 0;
	if(!Ammo->AmmoType->ExplosiveEnergy) return 0;
	if(Ammo->Flags & WEAP_AMMOFLAG_IS_SHAPED_CHARGE) return 0;

#ifdef BETA_RELEASE
	if(!ConcussionEnabled)
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP concussion disabled");
		return 0;
	}
#endif

	Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) concussive explosion",  Ammo->AmmoType->Name, Ammo->Id);

	// compute threshold, distance at which 10j of energy applied
	Energy = Ammo->AmmoType->ExplosiveEnergy;
	MaxDistSqr = Energy / 5.0f / 100.0f / 100.0f;

	Epicenter.x = (FLOAT32)(InPos->x - WorldOriginOffset.x);
	Epicenter.y = (FLOAT32)(InPos->y - WorldOriginOffset.y);
	Epicenter.z = -(FLOAT32)(InPos->z - WorldOriginOffset.z);

	// first check for building damage
	trncdGetTOHead(&TOHead);

	if (!(Ammo->Flags & WEAP_AMMOFLAG_DONTCHECKGROUNDOBJECTS))
	{
		for (TO = TOHead;TO != nullptr;)
		{
			if (TO->StratId == 0 && !TO->IsSTO())
			{
				TO = TO->Next;
				continue;
			}

			TempNum = Epicenter.x - TO->Center.x;
			if (TempNum < 0.0f) TempNum = -TempNum;

			TempNum -= TO->Radius;
			if (TempNum < 0.0f) TempNum = 0.0f;

			DistSqr = TempNum * TempNum;

			TempNum = Epicenter.y - TO->Center.y;
			if (TempNum < 0.0f)  TempNum = -TempNum;

			TempNum -= TO->Radius;
			if (TempNum < 0.0f) TempNum = 0.0f;
			DistSqr += TempNum * TempNum;

			TempNum = Epicenter.z - TO->Center.z;
			if (TempNum < 0.0f) TempNum = -TempNum;

			TempNum -= TO->Radius;
			if (TempNum < 0.0f) TempNum = 0.0f;

			DistSqr += TempNum * TempNum;

			if (DistSqr > MaxDistSqr)
			{
				TO = TO->Next;
				continue;
			}

			// ok we're going to inflict damage
			mathSubVectors(&TempVec,&TO->Center,&Epicenter);
			TempNum = (FLOAT32)mathExactVectorLength(&TempVec);
			TempNum -= TO->Radius;
			if (TempNum < 0.01f) TempNum = 0.01f;

			Write_Log(LOG_WEAP, LOG_INFO, "WEAP concussion to %s at %.1fm", TO->TerrainMasterObject->name, TempNum);

			TempNum *= 95.0f;

			Damage = Energy / (TempNum * TempNum);
			if (Damage > Energy) Damage = Energy;

			// OGS->Ramp: If AutoGun is NOT null and ... other stuff
			if (TO->ObjId.objClass != CLASS_BUILDING ||
				(TO->ObjId.objType != BUILDING_CAPTURE &&
				TO->TrnAutoGun == nullptr))
			{
				weapAmmoApplyDamageToObject(Ammo, Damage, TO, nullptr);
			}

			// OGS->Ramp: If AutoGun is NOT null and ... other stuff
			if (TO->ObjId.objClass != CLASS_BUILDING ||
				(TO->ObjId.objType != BUILDING_CAPTURE &&
				TO->TrnAutoGun != nullptr))
			{
				trnAutoGun* AG;
				piVECTOR P1;

				AG = TO->TrnAutoGun;

                if(AG != nullptr)
                {
                    // check for LOS
				    P1.x = (FLOAT32)(AG->Pos.x + AG->EyePos.x - WorldOriginOffset.x);
				    P1.y = (FLOAT32)(AG->Pos.y + AG->EyePos.y - WorldOriginOffset.y);
				    P1.z = -(FLOAT32)(AG->Pos.z + AG->EyePos.z - WorldOriginOffset.z);

				     // if direct line to view point, then apply damage
				    if(!(Ammo->Flags & WEAP_AMMOFLAG_DONT_SEND_DAMAGE) && !trncdCollideDownMediumList(&Epicenter,&P1))
					    weapAmmoApplyDamageToObject(Ammo, Damage, TO, nullptr);
                }
			}

			TO = TO->Next;
		}
	}

	// next check for vehicles

	// this is done in 3 parts:
	// 1) max distance check cull
	// 2) line to center of vehicle to check for cover
	// 3) modified distance check for each subcomponent

	if (!(Ammo->Flags & WEAP_AMMOFLAG_DONTCHECKVEHICLES))
	{
		vehGetVehicleHead(&CurrentVehicle);
		while (CurrentVehicle != nullptr)
		{
			if (CurrentVehicle->VehicleObjectData == nullptr)
			{
				CurrentVehicle = CurrentVehicle->getNext();
				continue;
			}

			// max distance check cull
			Pos.x = (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - WorldOriginOffset.x);
			Pos.y = (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - WorldOriginOffset.y);
			Pos.z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - WorldOriginOffset.z);

			mathSubVectors(&TempVec,&Pos,&Epicenter);
			TempNum = (FLOAT32)mathExactVectorLength(&TempVec);
			TempNum -= CurrentVehicle->VehicleObjectData->Radius;
			if (TempNum < 0.0f)
			{
				TempNum = 0.0f;
			}
			TempNum *= TempNum;

			if (TempNum > MaxDistSqr)
			{
				CurrentVehicle = CurrentVehicle->getNext();
				continue;
			}

			// line to center of vehicle
			if (CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				Pos.z -= 1.0f; // FPS center is at the feet, need to adjust for this
			}

			if (Ammo->VehicleData == nullptr)
			{
				if (PlayerModelData == nullptr)
				{
					break;
				}
				else
				{
					if (Ammo->Flags & WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST)
					{
						UseTO = PlayerModelData->ShortTrnList;
						MaxNumInList = ML_NUM_SHORT_LIST_ELEMENTS;
					}
					else
					{
						UseTO = PlayerModelData->MedTrnList;
						MaxNumInList = ML_NUM_MED_LIST_ELEMENTS;
					}
				}
			}
			else
			{
				MaxNumInList = 0;
				if (Ammo->Flags & WEAP_AMMOFLAG_USE_MEDCOLLIDELIST)
				{
					MaxNumInList = ML_NUM_MED_LIST_ELEMENTS;
					UseTO = Ammo->VehicleData->MedTrnList;
				}
				else if (Ammo->Flags & WEAP_AMMOFLAG_USE_MED60DEG_COLLIDELIST)
				{
					MaxNumInList = ML_NUM_MED_FRONT_CONE_LIST_ELEMENTS;
					UseTO = Ammo->VehicleData->MedFront60DegList;
				}
				else if (Ammo->Flags & WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST)
				{
					MaxNumInList = ML_NUM_SHORT_LIST_ELEMENTS;
					UseTO = Ammo->VehicleData->ShortTrnList;
				}
			}

			// first check for any buildings, since they stop concussion cold for now
			TempCDNum = _MAX_NUM_OF_COLLISIONS;
			CollideListPtr = CollideList;
			memset(CollideListPtr,0,sizeof(FASTCOLLIDE) * _MAX_NUM_OF_COLLISIONS);
			if (!(Ammo->Flags & WEAP_AMMOFLAG_DONTCHECKGROUNDOBJECTS) &&
				(MaxNumInList != 0))
			{
				trncdCollideWithListOfTerrainObjects(&CollideListPtr,
													 &TempCDNum,
													 UseTO,
													 MaxNumInList,
													 &Epicenter,
													 &Pos,
													 0);
			}

			NumCollides = _MAX_NUM_OF_COLLISIONS - TempCDNum;
			if (NumCollides > 0)
			{
				// shock wave "blocked", ingore this vehicle
				CurrentVehicle = CurrentVehicle->getNext();
				continue;

			}

			if ((Ammo->Flags & WEAP_AMMOFLAG_DONTCHECKVEHICLES) == 0)
			{

				weapCollideRoundWithVehicles(&CollideListPtr,
										  &TempCDNum,
										  &Epicenter,
										  &Pos,
										  frametime,
										  frametime,
										  !(Ammo->Flags & WEAP_AMMOFLAG_CANTHITPLAYER),
										  Ammo->Flags & WEAP_AMMOFLAG_ONLY_CHECKPLAYER,
										  &WorldOriginOffset,
										  0,
										  0);
			}
			NumCollides = _MAX_NUM_OF_COLLISIONS - TempCDNum;
			if (NumCollides > 0)
			{
			UINT32 DontContinue;

				DontContinue = 0;

				for (i=0;i<NumCollides;i++)
				{
					if ((vehVehicle*)CollideList[i].TopLevelObject != CurrentVehicle)
					{
						DontContinue = 1;
						break;
					}
				}
				if (DontContinue == 1)
				{
					// shock wave "blocked", ingore this vehicle
					CurrentVehicle = CurrentVehicle->getNext();
					continue;
				}
			}

			// ok time to do some concussion damage
			if (CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
				mathSubVectors(&TempVec,&Pos,&Epicenter);
				TempNum = (FLOAT32)mathExactVectorLength(&TempVec);
				if (TempNum < 0.01f)
				{
					TempNum = 0.01f;
				}
				TempNum *= 95.0f;

				Damage = Energy / (TempNum * TempNum);
				if (Damage > Energy)
				{
					Damage = Energy;
				}

				// modify the resulting damage by the fps concussion mod
				Damage *= fpFPSGetConcussionMod();

				if (Damage > 10.0f)
				{
					Write_Log(LOG_WEAP, LOG_INFO, "WEAP Concussion to %s(%s) at %.1fm",
								CurrentVehicle->VehicleType->Id, CurrentVehicle->Handle, (FLOAT32)mathExactVectorLength(&TempVec));
//					for (i=0;i<4;i++)
//						weapAmmoApplyDamageToVehicleComponent(Ammo, CurrentVehicle, i+1, Damage, Ammo->Id);
					// replace one hack with another
					weapAmmoApplyDamageToVehicleComponent(Ammo, CurrentVehicle, 5, Damage, Ammo->Id);	// head
					weapAmmoApplyDamageToVehicleComponent(Ammo, CurrentVehicle, 1, Damage, Ammo->Id);	// torso
					weapAmmoApplyDamageToVehicleComponent(Ammo, CurrentVehicle, 20, Damage, Ammo->Id);	// arms
					weapAmmoApplyDamageToVehicleComponent(Ammo, CurrentVehicle, 29, Damage, Ammo->Id);	// leg
				}
			}
			else
			{
				weapUpdateVehicleHitmap(CurrentVehicle);

				if (!(Ammo->Flags & WEAP_AMMOFLAG_DONT_SEND_DAMAGE) || CurrentVehicle == PlayerVehicle)
				{
					_TraverseHitTreeAndApplyConcussionDamage(Ammo, CurrentVehicle,  CurrentVehicle->VehicleObjectData->CollideExternObject,
															 Energy,  &Epicenter, Ammo->Id, 0, Internal, FALSE);
					_TraverseHitTreeAndApplyConcussionDamage(Ammo, CurrentVehicle,  CurrentVehicle->VehicleObjectData->CollideInternObject,
															 Energy,  &Epicenter,  Ammo->Id,  0, Internal, TRUE);
				}

			}

			CurrentVehicle = CurrentVehicle->getNext();
		}
	}

	return 0;
}

/************************************************************************************************
	_ApplyShapedChargeDamage
*************************************************************************************************/

static LOGICAL _ApplyShapedChargeDamage(weapAmmo* Ammo, FASTCOLLIDE* Collision, FLOAT64 DeltaTime, FLOAT64 CurrentTime)
{
	if(!(Collision->Flags & _AMMOCD_FLAG_ISAVEHICLE)) return TRUE;

	UINT32 compNum = (Collision->Flags & _AMMOCD_FLAG_ISPERSONA) ? Collision->PolyNum+1 : Collision->CollideObject->trnum;

	if(!(Ammo->Flags & WEAP_AMMOFLAG_CANDAMAGE)) return TRUE;

	Write_Log(LOG_WEAP, LOG_INFO, "WEAP applying shaped charge damage to %s(%d), t = %lf s", Collision->CollideObject->name,
				Collision->PolyNum, CurrentTime - DeltaTime * (1 - Collision->CollideTime));

	weapAmmoApplyDamageToVehicleComponent(Ammo, (vehVehicle*)Collision->TopLevelObject, compNum, Ammo->AmmoType->ExplosiveEnergy, Ammo->Id);

	return TRUE;
}

/************************************************************************************************
	_DoKnifeSlash
*************************************************************************************************/

UINT32 _DoKnifeSlash(weapAmmoType const* AmmoType, FLOAT64VECTOR const* Pos, piMATRIX const* FireAtt, UINT32 Flags, void* FireVehicle)
{
	FLOAT64 DeltaTime = 0, CurrentTime = 0;
    FLOAT64VECTOR PrevPos = {0, 0, 0}, CurPos = {0, 0, 0};
    FLOAT64VECTOR ProjVel = {0, 0, 0};
	weapAmmoType KnifeAmmoType;
	weapAmmo KnifeAmmo;

	KnifeAmmoType = *AmmoType;
	memset(&KnifeAmmo, 0x0, sizeof(KnifeAmmo));

	CurrentTime = (FLOAT64)frametime/1000.0;
	DeltaTime = 0.0;

	ProjVel.x = ProjVel.z = 0.0; ProjVel.y = 1.2;
	mathVectorMultiply64(&ProjVel, &ProjVel, FireAtt);

	// chest
	PrevPos.x = Pos->x;
	PrevPos.y = Pos->y;
	PrevPos.z = Pos->z;

	// fire to
	CurPos.x = ProjVel.x + Pos->x;
	CurPos.y = ProjVel.y + Pos->y;
	CurPos.z = ProjVel.z + Pos->z;

	KnifeAmmoType.ExplosiveEnergy = 0.0f;
	KnifeAmmoType.ProjectileWeight = 2 * AmmoType->ExplosiveEnergy; // make KE the explosive energy with v = 2 m/s

	KnifeAmmo.AmmoType = &KnifeAmmoType;
	KnifeAmmo.Flags = Flags | WEAP_AMMOFLAG_CANTHITPLAYER | WEAP_AMMOFLAG_NOMUZZLEFLASH;
	KnifeAmmo.Velocity = ProjVel;
	KnifeAmmo.Position = CurPos;
	KnifeAmmo.FireVehicle = FireVehicle;

	_DoProjectileUpdate(&KnifeAmmo, &PrevPos, DeltaTime, CurrentTime);

#ifdef BETA_RELEASE
	int id = draw3dSetWorldDebugLine(&PrevPos, &CurPos, "knife", colorREDALPHA);
	draw3dSetDebugObjectFlipZ(id,TRUE);
	draw3dSetDebugObjectAlwaysOnTop(id, FALSE);
#endif

	return 0;
}

#include "pi/piogl.h"

/************************************************************************************************
	_DoProjectileUpdate
*************************************************************************************************/

static LOGICAL _DoProjectileUpdate(weapAmmo* Ammo, FLOAT64VECTOR const* PrevPos, FLOAT64 DeltaTime, FLOAT64 CurrentTime)
{
    FLOAT64VECTOR oldPos64 = {0, 0, 0}, newPos64 = {0, 0, 0};
    piVECTOR oldPos = {0, 0, 0}, newPos = {0, 0, 0};

	mathSubVectors(&oldPos64, PrevPos, &WorldOriginOffset); 		oldPos64.z = -oldPos64.z;
	mathSubVectors(&newPos64, &Ammo->Position, &WorldOriginOffset);	newPos64.z = -newPos64.z;  // pick a sign any sign

	// do we do enough double/float flip flops?
    oldPos.x = (FLOAT32)oldPos64.x; oldPos.y = (FLOAT32)oldPos64.y;  oldPos.z = (FLOAT32)oldPos64.z;
    newPos.x = (FLOAT32)newPos64.x; newPos.y = (FLOAT32)newPos64.y;  newPos.z = (FLOAT32)newPos64.z;

#ifdef BETA_RELEASE
	if(!(Ammo->Flags & (WEAP_AMMOFLAG_IS_SPALL | WEAP_AMMOFLAG_IS_SHRAPNEL | WEAP_AMMOFLAG_IS_GHOSTROUND)))
	{
		int id = draw3dSetWorldDebugLine(PrevPos, &Ammo->Position, "projray", colorRED33 | colorALPHA);
		draw3dSetDebugObjectFlipZ(id,TRUE);
		draw3dSetDebugObjectAlwaysOnTop(id, FALSE);
	}
#endif

	// check to see if we've gone off
	LOGICAL AmmoStopped = _DoAmmoFuzeCheck(Ammo, &oldPos, &newPos, DeltaTime, CurrentTime, 0);

	if(AmmoStopped)
		return TRUE;	// Ammo did stop

	if(oldPos == newPos)	return false;

	FASTCOLLIDE CollideList[_MAX_NUM_OF_COLLISIONS]{};

	UINT32 NumCollisions = _GetProjectileCollisions(Ammo, Ammo->Flags, CollideList, _MAX_NUM_OF_COLLISIONS, &oldPos, &newPos, DeltaTime, CurrentTime);
	if(NumCollisions <= 0)
		return FALSE;	// Ammo didn't collide but it didn't stop either.

	mlUtilitySortCollideList(CollideList, NumCollisions); // sort from nearest to furthest

	for(UINT32 i = 0; !AmmoStopped && i < NumCollisions; i++)
	{
		// keep checking fuze between hits
		AmmoStopped = _DoAmmoFuzeCheck(Ammo, &oldPos, &newPos, DeltaTime, CurrentTime, CollideList[i].CollideTime);

		if(!AmmoStopped)
		{
			if(CollideList[i].Flags & _AMMOCD_FLAG_ISAVEHICLE)
			{
				vehVehicle* Vehicle = (vehVehicle*)CollideList[i].TopLevelObject;
				if(!vehVehicleInList(Vehicle)) // extra level of validity checking
				{
#ifdef BETA_RELEASE
					piExitProgram(_FL, "corrupt collide list");
#endif
					break; // something's hosed let's just break out
				}

				AmmoStopped = _DoVehicleCollision(Ammo, &CollideList[i], &oldPos, &newPos, DeltaTime, CurrentTime);

				// hulk hack
				if(Vehicle->isHulked() && (Ammo->Flags & WEAP_AMMOFLAG_IS_SHAPED_CHARGE) )
				{
					_ApplyShapedChargeDamage(Ammo, &CollideList[i], DeltaTime, CurrentTime);
					AmmoStopped = TRUE;

					continue;
				}

				// Special case for shaped charges, damage is applied to the* next* component
				if((Ammo->Flags & WEAP_AMMOFLAG_IS_SHAPED_CHARGE) && !AmmoStopped && (i < NumCollisions - 1))
				{
					_ApplyShapedChargeDamage(Ammo, &CollideList[i+1], DeltaTime, CurrentTime);
					AmmoStopped = TRUE;

					continue;
				}
			} else
			{
				AmmoStopped = _DoTerrainCollision(Ammo, &CollideList[i], &oldPos, &newPos, DeltaTime, CurrentTime);
			}
		}
	}

	return AmmoStopped;
}

/************************************************************************************************
	_DoAmmoFuzeCheck
*************************************************************************************************/

static LOGICAL _DoAmmoFuzeCheck(weapAmmo* Ammo, piVECTOR const* OldPos,  piVECTOR const* NewPos,
								FLOAT64 DeltaTime, FLOAT64 CurrentTime, FLOAT64 t)
{
	if(!(Ammo->AmmoType->ExplosiveEnergy > 0 && Ammo->AmmoType->NumShrapnelPieces > 0))
		return FALSE;

    FLOAT64VECTOR Pos = {0, 0, 0};
	FLOAT64 CurPosTime = 0, PrevTime = 0, DetontateTime = 0;

	CurPosTime = CurrentTime - DeltaTime * (1 - t);
	PrevTime = CurrentTime - DeltaTime;

	// See if it's detonated
	if( (Ammo->FuseStartTime != 0) && (CurPosTime > Ammo->FuseStartTime + Ammo->AmmoType->FuzeTime) )
		DetontateTime = Ammo->FuseStartTime + Ammo->AmmoType->FuzeTime;
	else if( (Ammo->AmmoType->SelfDestructTime > 0) && (CurPosTime > Ammo->LaunchTime + Ammo->AmmoType->SelfDestructTime) )
		DetontateTime = Ammo->LaunchTime + Ammo->AmmoType->SelfDestructTime;
	else if( (Ammo->AmmoType->FuzeType == WEAPAMMO_FUZE_VARTIME) && (Ammo->VarFuzeInterval > 0) && (CurPosTime > Ammo->LaunchTime + Ammo->VarFuzeInterval) )
		DetontateTime = Ammo->LaunchTime + Ammo->VarFuzeInterval;
	else
		return FALSE;

	if(t != 0 && DeltaTime > 0.0f)
		t = (DetontateTime - PrevTime) /  DeltaTime;

	Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) detonating, t = %lf s", Ammo->AmmoType->Name, Ammo->Id, DetontateTime);
	Pos.x = (NewPos->x - OldPos->x) * t + OldPos->x;
	Pos.y = (NewPos->y - OldPos->y) * t + OldPos->y;
	Pos.z = (NewPos->z - OldPos->z) * t + OldPos->z;

	Pos.x = Pos.x + WorldOriginOffset.x;
	Pos.y = Pos.y + WorldOriginOffset.y;
	Pos.z = -(Pos.z + WorldOriginOffset.z);

	if(!Ammo->AmmoType->IsSmokeGrenade)
	{
		weapfxAmmoExplosion(Ammo, &Pos, nullptr, 0);
		_DoExplosionShrapnel(Ammo, &Pos, &Ammo->Velocity, DetontateTime);
		_DoExplosionConcussion(Ammo, &Pos, Ammo->LastVehicleHit != nullptr);
	}

	return TRUE;
}

/************************************************************************************************
	_DoVehicleCollision
*************************************************************************************************/

static LOGICAL _DoVehicleCollision(weapAmmo* Ammo, FASTCOLLIDE* Collision, piVECTOR const* OldPos,
									piVECTOR const* NewPos, FLOAT64 DeltaTime,  FLOAT64 CurrentTime)
{
	if(Ammo->AmmoType->IsSmokeGrenade)
		return FALSE;

	if(Collision->Alpha < 0.5)
		return FALSE;

	vehVehicle* Vehicle = (vehVehicle*)Collision->TopLevelObject;
	if((Ammo->Flags & WEAP_AMMOFLAG_DONTCOLLIDEWITHFIRER) && (Ammo->FireVehicle == Vehicle))
		return FALSE;

	// don't collide with vehicles if we're a ghost round on the first update; prevents mortar rounds colliding
	// with 3rd person firing vehicles since we don't know the firing vehicle in that case (its an STO)
	if(Ammo->AmmoType->UseGhostRoundSTO && Ammo->FirstUpdate)
		return FALSE;

	// Caution: OGS: AmmoStopped is defined as a LOGICAL but used as a
	// SINT32 to pass to weapfxAmmoExplosion (arg #4). We also set it
	// to FALSE, TRUE and 1 which may have ambiguous effects.
	LOGICAL AmmoStopped = FALSE;
	LOGICAL Richocheted = FALSE;
	LOGICAL DoSpall = TRUE;
    piVECTOR DeltaVec = {0, 0, 0};
    FLOAT64 ResidualSpeed = 0;
	FLOAT64 PenetrationConstant = 0;
    FLOAT64 Obliquity = 0;
    FLOAT64 Thickness = 0;
    FLOAT64 Deflection = 0;
    FLOAT64 VelocityLimit = 0;
	FLOAT64 KE = 0;
    FLOAT64 KELimit = 0;
	UINT32 compNum = 0;
	FLOAT64 PenetrationThickness=0;

	FLOAT64 AmmoCollideTime = CurrentTime - DeltaTime * (1 - Collision->CollideTime);
	FLOAT64 AmmoSpeed = mathExactVectorLength(&Ammo->Velocity);
	mathSubVectors(&DeltaVec, &Collision->NewPos, &Collision->OldPos);
	FLOAT64 DeltaDist = mathExactVectorLength(&DeltaVec);
	FLOAT64VECTOR AmmoDir = Ammo->Velocity;
	AmmoDir.x /= AmmoSpeed; AmmoDir.y /= AmmoSpeed; AmmoDir.z /= AmmoSpeed;

//	Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) t = %lf s .. Oldpos (%f, %f, %f)  Newpos (%f, %f, %f)",  Ammo->AmmoType->Name, Ammo->Id,
//								AmmoCollideTime,
//								OldPos->x, OldPos->y, OldPos->z, NewPos->x, NewPos->y, NewPos->z);

	if(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND)
	{
		AmmoStopped = 1;
		KE = .5 * Ammo->AmmoType->ProjectileWeight * AmmoSpeed * AmmoSpeed;
		ResidualSpeed = 0;
	}else
	{
		// determine obliquity
		if(Collision->Flags & _AMMOCD_FLAG_INTERNALCOMPONENT || Collision->Flags & _AMMOCD_FLAG_ISPERSONA)
		{
			Obliquity = 0; // internal components don't use obliquity calculations
		}else
		{
			Obliquity = mathVectDotProduct(&(Collision->CollideObject->psnorms[Collision->PolyNum]), &DeltaVec);
			Obliquity /= DeltaDist;
			if(Obliquity < 0.0)  Obliquity = -Obliquity;
			if(Obliquity >  1.0) Obliquity = 1.0;
			Obliquity = acos(Obliquity);
		}

		// get armor thickness
		if(Collision->Flags & _AMMOCD_FLAG_ISPERSONA)
		{
			PenetrationConstant = 0.0000001463f; // Arbitrary value for Infantry* A HACK* in lieu of proper damage modelling.

			// Cast to UINT32 so that any negative values would go large.
			if((UINT32)Collision->PolyNum >= 38)
				Collision->PolyNum = 0;

			Thickness = gr2AnimGetLocationThickness(Collision->PolyNum);
		}else
		{
			PenetrationConstant = Ammo->AmmoType->PenetrationConstant;
			vehGetHitmapComponentThickness(&Thickness, Vehicle, Collision->CollideObject->trnum);
		}

		// determine if a ricochet occured
		if(Ammo->AmmoType->RichochetAngle != 0.0 && (Obliquity > Ammo->AmmoType->RichochetAngle))
		{
			// if we're overmatching bigtime anyway, so that the round will plough through the armor sideways,
			// we'll still penetrate, although the relative thickness of the armor goes up bigtime
			Deflection = PI/2 - Obliquity;
			Thickness *= 1.3;
		}else
		{
			Deflection = 0.0;
		}

		// Shaped charge special case
		if(Ammo->Flags & WEAP_AMMOFLAG_IS_SHAPED_CHARGE)
		{
			if(Ammo->AmmoType->ShapedChargePen > Thickness)
			{
				if(!(Collision->Flags & _AMMOCD_FLAG_ISPERSONA))
					Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) penetrating %s(%d), thickness %.2lf mm at %.0lf deg, t = %lf s", Ammo->AmmoType->Name, Ammo->Id,
								Collision->CollideObject->name, Collision->PolyNum, Thickness * 1000.0, mlRAD_TO_DEG(Obliquity), AmmoCollideTime);
				AmmoStopped = FALSE;
			}else
			{
				if(!(Collision->Flags & _AMMOCD_FLAG_ISPERSONA))
					Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) stopped in %s(%d), thickness %.2lf mm at %.0lf deg, t = %lf s",  Ammo->AmmoType->Name, Ammo->Id,
								Collision->CollideObject->name, Collision->PolyNum, Thickness * 1000.0, mlRAD_TO_DEG(Obliquity), AmmoCollideTime);
				AmmoStopped = TRUE;
			}

			return AmmoStopped;
		}

		VelocityLimit = pow((Thickness) / (PenetrationConstant * pow(cos(Obliquity),1.5)), 0.625);
		KELimit	= .5 * Ammo->AmmoType->ProjectileWeight * VelocityLimit * VelocityLimit;
		KE 		= .5 * Ammo->AmmoType->ProjectileWeight * AmmoSpeed * AmmoSpeed;
		PenetrationThickness = pow(AmmoSpeed,1/0.625) * (PenetrationConstant * pow(cos(Obliquity),1.5));

		// weap logging message
		if(Collision->Flags & _AMMOCD_FLAG_ISPERSONA)
		{
			if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
				Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) striking %s of %s(%s) at %.0lf m/s, KE %.0lf J, t = %lf s",
				Ammo->AmmoType->Name, Ammo->Id, Vehicle->VehicleType->DamageObjects[Collision->PolyNum], Vehicle->VehicleType->Id, Vehicle->Handle,
//				Ammo->AmmoType->Name, Ammo->Id, Vehicle->VehicleType->DamageObjects PERSON_LOCATION[Collision->PolyNum], Vehicle->VehicleType->Id, Vehicle->Handle,
					AmmoSpeed, KE, AmmoCollideTime);
		}else
		{
			if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
				Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) striking %s component %s(%d) of %s(%s) at %.0lf m/s, KE %.0lf J, thickness %.2lf mm at %.0lf deg, t = %lf s",
					Ammo->AmmoType->Name, Ammo->Id, (Collision->Flags & _AMMOCD_FLAG_INTERNALCOMPONENT) ? "internal" : "external",
					Collision->CollideObject->name, Collision->PolyNum, Vehicle->VehicleType->Id, Vehicle->Handle,
					AmmoSpeed, KE, Thickness * 1000.0, mlRAD_TO_DEG(Obliquity), AmmoCollideTime);
		}

		if(KE <= KELimit) // not penetrating
		{
			AmmoStopped = TRUE;
			ResidualSpeed = 0;

			// Round ricocheted
			if((Ammo->AmmoType->RichochetAngle != 0.0) && (Obliquity > Ammo->AmmoType->RichochetAngle))
			{
				FLOAT64 RichochetSpeed, Depth, Ratio;

				// calc amount of energy imparted to component from richochet
				RichochetSpeed = cos(Obliquity) * AmmoSpeed;
				Depth = PenetrationConstant * pow(AmmoSpeed,1.6) * pow(cos(Obliquity),1.5);
				Ratio = (Thickness > 0.0001) ? (Depth / Thickness) : (Depth / 0.0001);
				RichochetSpeed += sin(Deflection) * AmmoSpeed * (1.0 - Ratio);
				KE = RichochetSpeed * RichochetSpeed * 0.5 * Ammo->AmmoType->ProjectileWeight;
				DoSpall = FALSE;
				Richocheted = TRUE;
			}
		}else // penetrated, bleed off energy
		{
			ResidualSpeed = sqrt((KE - KELimit) / .5 / Ammo->AmmoType->ProjectileWeight);
			KE = KELimit;

			// start fuse
			if( (Ammo->FuseStartTime == 0) && (Ammo->AmmoType->ExplosiveEnergy > 0) &&
				(AmmoSpeed - ResidualSpeed > Ammo->AmmoType->FuseStartSpeedLoss) )
			{
				Ammo->FuseStartTime = AmmoCollideTime;
				if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
					Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) fuse started, timer = %.4lf s, t = %lf s", Ammo->AmmoType->Name, Ammo->Id, Ammo->AmmoType->FuzeTime, Ammo->FuseStartTime);
			}
		}
	}

	// fx
	if(KE > 0 && Vehicle != Ammo->LastVehicleHit)
    {
       weapfxAmmoHit(Ammo, Collision, OldPos, NewPos, Vehicle, (Richocheted) ? 0 : !AmmoStopped, KE);

       if(Richocheted /*sic*/ && Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_ROCKET)
       {
           weapfxRicochetEffect(*Ammo, Collision[0], "manysparks");
       }
    }

	// apply damage
	compNum = (Collision->Flags & _AMMOCD_FLAG_ISPERSONA) ? Collision->PolyNum+1 : Collision->CollideObject->trnum;
	weapAmmoApplyDamageToVehicleComponent(Ammo, Vehicle, compNum, KE, Ammo->Id);

#ifdef BETA_RELEASE
	if(weaplogFile)
	{
		piMATRIX mat;
		math3dInvertMatrix(&mat, &Collision->CollideObject->mat);

		piVECTOR StartPos = *OldPos;
		math3dVectorMultiply(&StartPos, &StartPos, &mat);

		piVECTOR EndPos = *NewPos;
		math3dVectorMultiply(&EndPos, &EndPos, &mat);

		// use points instead of polynum since that's specific to the MDL format
        piVECTOR v1 = {0, 0, 0};
        piVECTOR v2 = {0, 0, 0};
        piVECTOR v3 = {0, 0, 0};

        // ptris nullptr for FPS.
        if(Collision->Flags & _AMMOCD_FLAG_ISPERSONA)
        {
            v1 = Collision->CollidePnt;
            v2 = Collision->CollidePnt;
            v3 = Collision->CollidePnt;
        }
        else
        {
            v1 = Collision->CollideObject->RenderPositions()[Collision->CollideObject->ptris[Collision->PolyNum].v1];
            v2 = Collision->CollideObject->RenderPositions()[Collision->CollideObject->ptris[Collision->PolyNum].v2];
            v3 = Collision->CollideObject->RenderPositions()[Collision->CollideObject->ptris[Collision->PolyNum].v3];
        }

        fprintf(weaplogFile
				, "%s %d %d %f %f %f %f %f %f %f %f %f %f %s %s %s %d %f %f %f %f %f %f %f %f %f\n"
				, Ammo->AmmoType->Name, Ammo->Id, frametime, AmmoCollideTime
				, StartPos.x, StartPos.y, -StartPos.z
				, Collision->CollidePnt.x, Collision->CollidePnt.y, -Collision->CollidePnt.z
				, EndPos.x, EndPos.y, -EndPos.z
				, Vehicle->VehicleType->Id, Vehicle->Handle
				, Collision->CollideObject->name, Collision->PolyNum
				, v1.x, v1.y, -v1.z
				, v2.x, v2.y, -v2.z
				, v3.x, v3.y, -v3.z
				);

		fflush(weaplogFile);
	}
#endif

	// note:  may need to supress spall in the specific object case .. don't have to at the moment
	// because shrapnel uses an ammotype that is guaranteed to not have modelspall set
	if(	DoSpall && Ammo->AmmoType->ModelSpall &&
		(Collision->Flags & _AMMOCD_FLAG_EXTERNALCOMPONENT) && !(Collision->Flags & _AMMOCD_FLAG_ISPERSONA) )
	{
		if(PenetrationThickness > .9*Thickness)
			_DoSpall(Ammo, (FLOAT32)AmmoSpeed, (FLOAT32)ResidualSpeed, Collision, NewPos, OldPos, Ammo->Flags);
	}

	// logging
	if(!Richocheted)
	{
		if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
			Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) %s %s(%d) %.2lf mm, lost %.0lf m/s, speed now %.0lf m/s, t = %lf s",
					Ammo->AmmoType->Name, Ammo->Id, (AmmoStopped) ? "stopped in" : "penetrated", Collision->CollideObject->name, Collision->PolyNum,
					(AmmoStopped) ? PenetrationThickness*1000.0 : Thickness*1000.0,
					AmmoSpeed - ResidualSpeed, ResidualSpeed, AmmoCollideTime);
	}
	else
	{
		if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
			Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) richocheting off %s(%d) with a thickness of %.2lf mm at %.0lf deg, t = %lf s",
					Ammo->AmmoType->Name, Ammo->Id, Collision->CollideObject->name, Collision->PolyNum, Thickness * 1000.0, mlRAD_TO_DEG(Obliquity), AmmoCollideTime);
	}

	// commented out because it doesn't do anything
    /*
     TBD - I guess then we should fix it ???

//	if(Vehicle && Vehicle->ModelData && Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
//		_TransferMomentum(Vehicle->ModelData, Collision, Ammo, AmmoSpeed - ResidualSpeed);
     */

	if(AmmoStopped)
		ResidualSpeed = 0;

	// Update Ammo Velocity
	if(AmmoSpeed > 0.0001)
	{
		Ammo->Velocity.x *= ResidualSpeed / AmmoSpeed;
		Ammo->Velocity.y *= ResidualSpeed / AmmoSpeed;
		Ammo->Velocity.z *= ResidualSpeed / AmmoSpeed;
	}

	// check for contact fuse or if explosive round has stopped
	if(	Ammo->AmmoType->ExplosiveEnergy > 0 && Ammo->AmmoType->NumShrapnelPieces > 0 && !Richocheted  &&
		((Ammo->AmmoType->FuzeTime == 0 && Ammo->FuseStartTime != 0) || AmmoStopped) )
	{
		FLOAT64VECTOR CollidePos;
/*		CollidePos.x = Collision->CollidePnt.x; CollidePos.y = Collision->CollidePnt.y; CollidePos.z = Collision->CollidePnt.z;
		mathVectorMultiply64(&CollidePos, &CollidePos, &Collision->CollideObject->mat);
		mathAddVectors(&CollidePos, &CollidePos, &WorldOriginOffset); */

		CollidePos.x = (NewPos->x - OldPos->x) * Collision->CollideTime + OldPos->x;
		CollidePos.y = (NewPos->y - OldPos->y) * Collision->CollideTime + OldPos->y;
		CollidePos.z = (NewPos->z - OldPos->z) * Collision->CollideTime + OldPos->z;
		mathAddVectors(&CollidePos, &CollidePos, &WorldOriginOffset);

		if(Vehicle != Ammo->LastVehicleHit) // add some fudge to try and prevent shrapnel clipping
		{
			CollidePos.x -= AmmoDir.x * .01;
			CollidePos.y -= AmmoDir.y * .01;
			CollidePos.z += AmmoDir.z * .01;
		}

		CollidePos.z = -CollidePos.z;

		if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
			Write_Log(LOG_WEAP,LOG_INFO,"WEAP %s(%d) fuze detonates", Ammo->AmmoType->Name, Ammo->Id);

		weapfxAmmoExplosion(Ammo, &CollidePos, Collision, (Richocheted) ? 0 : !AmmoStopped);
		bool doshrap = Collision->Flags & _AMMOCD_FLAG_INTERNALCOMPONENT
			|| Vehicle->VehicleType->Type == VEH_TYPE_AIR || Vehicle->VehicleType->Type == VEH_TYPE_SEA
			|| Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_GRENADE;

#ifdef BETA_RELEASE
		if(weaplogFile) doshrap = true;
#endif
		_DoExplosionShrapnel(Ammo, &CollidePos, (AmmoStopped) ? &AmmoDir : &Ammo->Velocity, AmmoCollideTime, doshrap);
		_DoExplosionConcussion(Ammo, &CollidePos, Collision->Flags & _AMMOCD_FLAG_INTERNALCOMPONENT);

		AmmoStopped = TRUE;
	}
	Ammo->LastVehicleHit = Vehicle;

	return AmmoStopped;
}

/************************************************************************************************
	_DoTerrainCollision
*************************************************************************************************/

static LOGICAL _DoTerrainCollision(weapAmmo* Ammo, FASTCOLLIDE* Collision, piVECTOR const* OldPos, piVECTOR const* NewPos,
									 FLOAT64 DeltaTime,  FLOAT64 CurrentTime)
{
	trnCDTerrainObject* TO = nullptr;
	FLOAT64 KE = 0;
    FLOAT64VECTOR CollidePos = {0, 0, 0};
	FLOAT64 AmmoSpeed = 0, AmmoCollideTime = 0;

	Ammo->LastVehicleHit = nullptr;

	if(Collision->Alpha < 0.5) return FALSE;

    if(Ammo->Flags & WEAP_AMMOFLAG_SPECIALACTIVE)
    {
        return FALSE;
    }

	TO = (Collision->TopLevelObject && Collision->TopLevelObject->child) ? (trnCDTerrainObject*)Collision->TopLevelObject->child->lpspare : nullptr;

	if(TO && TO->DamageLevel == FB_CLOSED_VISLEVEL)  // cannot collide with invisible objects
		return FALSE;

	KE = 0.5 * Ammo->AmmoType->ProjectileWeight * mathExactVectorLength2(&Ammo->Velocity);

	CollidePos.x = Collision->CollidePnt.x; CollidePos.y = Collision->CollidePnt.y; CollidePos.z = Collision->CollidePnt.z;

    if(Collision->CollideObject)
    {
	    mathVectorMultiply64(&CollidePos, &CollidePos, &Collision->CollideObject->mat);
    }

    mathAddVectors(&CollidePos, &CollidePos, &WorldOriginOffset);
        CollidePos.z = -CollidePos.z;

	// weapon fx
	if(!(Ammo->Flags & WEAP_AMMOFLAG_NOFX))
	{
		// water splash
		if(_IsWater(Collision->CollideObject))
			weapfxAmmoWaterSplash(Ammo, &CollidePos, Collision->CollideObject, Collision->PolyNum, KE);

		// weapon explosion fx
		if(!(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO && !Ammo->TorpedoIsRunning)) // don't want the torp to explode just yet
		{
			weapfxAmmoHit(Ammo, Collision, OldPos, NewPos, nullptr, 0, 0);
		}
	}

	AmmoSpeed = mathExactVectorLength(&Ammo->Velocity);
	AmmoCollideTime = CurrentTime - DeltaTime * (1 - Collision->CollideTime);

	if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
	{
		Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) striking %s(%d) of terrain %s at %.0lf m/s, t = %lf s",
		Ammo->AmmoType->Name,
		Ammo->Id,
		(Collision->CollideObject) ? Collision->CollideObject->name : "null",
		Collision->PolyNum,
		TO ? TO->TerrainObject->name : "",
		AmmoSpeed,
		AmmoCollideTime);
	}

	// apply the damage
	if(TO)
	{
		if(TO->StratId != 0 && (TO->ObjId.objClass != CLASS_BUILDING || TO->ObjId.objType != BUILDING_CAPTURE) )
		{
			weapAmmoApplyDamageToObject(Ammo, (FLOAT32)KE,
				TO, Ammo->VehicleData ? (vehVehicle*)Ammo->VehicleData->VehicleMasterPtr : nullptr);
		}
		else if(TO->IsSTO())
		{
			weapAmmoApplyDamageToObject(Ammo, (FLOAT32)KE,
				TO, Ammo->VehicleData ? (vehVehicle*)Ammo->VehicleData->VehicleMasterPtr : nullptr);
		}

		// alert the manager we hit something
		if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
			TUT_MGR->DoEvent(TUT_EVENT_HIT,
										TO,
										Collision->CollideObject,
										&CollidePos.x,
										&CollidePos.y,
										&CollidePos.z);
	}

	// explode
	if(!(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO && !Ammo->TorpedoIsRunning))
	{
		if(Ammo->AmmoType->IsSmokeGrenade)
		{
			if(!_IsWater(Collision->CollideObject))
			{
                weapfxAmmoDetachAmmo(Ammo);

                // Only create STO for ownship smoke grenades
                if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
                {
				    if(weapfxDetonateSmokeGrenade(&CollidePos, Ammo))
						Ammo->KillAmmo = TRUE;
					else
						Ammo->Flags |= WEAP_AMMOFLAG_SPECIALACTIVE;

                    return FALSE;
                }
			}
		}else
		{
			FLOAT64VECTOR AmmoDir;
			AmmoSpeed = mathExactVectorLength(&Ammo->Velocity);
			AmmoDir = Ammo->Velocity;
			AmmoDir.x /= AmmoSpeed; AmmoDir.y /= AmmoSpeed; AmmoDir.z /= AmmoSpeed;
			// add some fudge to try and prevent shrapnel clipping
			CollidePos.x -= AmmoDir.x * .01;
			CollidePos.y -= AmmoDir.y * .01;
			CollidePos.z -= AmmoDir.z * .01;

			weapfxAmmoExplosion(Ammo, &CollidePos, Collision, 0);
			_DoExplosionShrapnel(Ammo, &CollidePos, &Ammo->Velocity, AmmoCollideTime);
			_DoExplosionConcussion(Ammo, &CollidePos, FALSE);
		}
	}

	// round always stopped no penetration
	return TRUE;
}

/************************************************************************************************
	_TransferMomentum
*************************************************************************************************/
/*
static void _TransferMomentum(mlVehicleData* VehicleData, FASTCOLLIDE const* Collision, weapAmmo const* Ammo, FLOAT64 SpeedLoss)
{
    FLOAT64VECTOR Impulse = {0, 0, 0}, Pos = {0, 0, 0};

	Impulse = Ammo->Velocity;
	mathNormalize(&Impulse);

	Impulse.x *= SpeedLoss * Ammo->AmmoType->ProjectileWeight;
	Impulse.y *= SpeedLoss * Ammo->AmmoType->ProjectileWeight;
	Impulse.z *= SpeedLoss * Ammo->AmmoType->ProjectileWeight;

	Pos.x = Collision->CollidePnt.x; Pos.y = Collision->CollidePnt.y; Pos.z = Collision->CollidePnt.z;
	mathVectorMultiply64(&Pos, &Pos, &Collision->CollideObject->mat);
	mathAddVectors(&Pos, &Pos, &WorldOriginOffset); Pos.z = -Pos.z;

	mlVehicleAddImpulse(VehicleData, &Pos, &Impulse);
}

*/

/************************************************************************************************
	_GetProjectileCollisions
*************************************************************************************************/

static UINT32 _GetProjectileCollisions(weapAmmo* Ammo, UINT32 Flags, FASTCOLLIDE* CollideList, UINT32 MaxCollisions, piVECTOR const* OldPos, piVECTOR const* NewPos,
										FLOAT64 DeltaTime, FLOAT64 CurrentTime)
 {
 	UINT32 NumCollisions = 0;
 	UINT32 PrevNumCollisions = 0, CollisionSlotsLeft = 0;
	UINT32 i = 0;
	FASTCOLLIDE*  CollideListPtr = nullptr;

//	Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) .. Oldpos (%f, %f, %f)  Newpos (%f, %f, %f)",  Ammo->AmmoType->Name, Ammo->Id,
//								OldPos->x, OldPos->y, OldPos->z, NewPos->x, NewPos->y, NewPos->z);
//
	logTimerBeginMacro();

	CollisionSlotsLeft = MaxCollisions;
	PrevNumCollisions = 0;
	CollideListPtr = CollideList;

// Get Replacement Terrain Object Collisions
	logTimerBeginExtMacro(trncdCollideWithListOfTerrainObjects);
	if(!(Flags & WEAP_AMMOFLAG_DONTCHECKGROUNDOBJECTS))
	{
		SINT32 trnCollideListSize;
		trnCDTerrainObject** trnCollideList;

		trnCollideListSize = _AmmoTerrainCollideList(Ammo, Flags, &trnCollideList);
		if(trnCollideListSize && trnCollideList)
			trncdCollideWithListOfTerrainObjects(&CollideListPtr, &CollisionSlotsLeft,  trnCollideList, trnCollideListSize,  OldPos, NewPos, 0);
	}

	NumCollisions = MaxCollisions - CollisionSlotsLeft;
	for(i = PrevNumCollisions; i < NumCollisions; i++)
		CollideList[i].Flags |= _AMMOCD_FLAG_ISABUILDING;
	PrevNumCollisions = NumCollisions;
 	logTimerEndExtMacro(trncdCollideWithListOfTerrainObjects);

	logTimerBeginExtMacro(trncdCollideWithBaseTerrain);
//  Get Non Replacement Terrain Collisions
	if(!(Flags & WEAP_AMMOFLAG_DONTCHECKTERRAIN))
	{
		NumCollisions += trncdCollideWithBaseTerrain(CollideListPtr, CollisionSlotsLeft, OldPos, NewPos);
		CollideListPtr += NumCollisions - PrevNumCollisions;
		CollisionSlotsLeft -= NumCollisions - PrevNumCollisions;

		for(i = PrevNumCollisions; i < NumCollisions; i++)
			CollideList[i].Flags |= _AMMOCD_FLAG_ISTERRAIN;
		PrevNumCollisions = NumCollisions;

	}
	logTimerEndExtMacro(trncdCollideWithBaseTerrain);

// Get Vehicle Collisions
	logTimerBeginExtMacro(weapCollideRoundWithVehicles);
	if(!(Flags & WEAP_AMMOFLAG_DONTCHECKVEHICLES))
	{
		weapCollideRoundWithVehicles(&CollideListPtr, &CollisionSlotsLeft, OldPos, NewPos,
									  CurrentTime - DeltaTime, CurrentTime,
									  !(Flags & WEAP_AMMOFLAG_CANTHITPLAYER), Flags & WEAP_AMMOFLAG_ONLY_CHECKPLAYER,
									  &WorldOriginOffset, 0, 0);
	}
	NumCollisions = MaxCollisions - CollisionSlotsLeft;
	for(i = PrevNumCollisions; i < NumCollisions; i++)
		CollideList[i].Flags |= _AMMOCD_FLAG_ISAVEHICLE;
	PrevNumCollisions = NumCollisions;
 	logTimerEndExtMacro(weapCollideRoundWithVehicles);

// Get Autogunner Collisions
	logTimerBeginExtMacro(_CollideRoundWithAutoguns);
	if(!(Flags & WEAP_AMMOFLAG_DONTCHECKGROUNDOBJECTS))
	{
		_CollideRoundWithAutoguns(&CollideListPtr, &CollisionSlotsLeft, OldPos, NewPos,
									CurrentTime - DeltaTime, CurrentTime);

	}
	NumCollisions = MaxCollisions - CollisionSlotsLeft;
	for(i = PrevNumCollisions; i < NumCollisions; i++)
		CollideList[i].Flags |= _AMMOCD_FLAG_ISAUTOGUN;
	PrevNumCollisions = NumCollisions;
 	logTimerEndExtMacro(_CollideRoundWithAutoguns);

 	logTimerEndMacro();

 	return NumCollisions;
 }

/************************************************************************************************
	_AmmoTerrainCollideList
*************************************************************************************************/
// gets the terrain list the ammo needs to collide with

static SINT32 _AmmoTerrainCollideList(weapAmmo* Ammo, UINT32 Flags, trnCDTerrainObject*** trnCollideList)
{
	mlVehicleData* VehicleData = (Ammo && Ammo->VehicleData) ? Ammo->VehicleData : persGetPersonaModelDataPtr();
	SINT32 trnCollideListSize = 0;

	*trnCollideList = nullptr;

	if(VehicleData)
	{
		if(Flags & WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST)
		{
			trnCollideListSize = ML_NUM_SHORT_LIST_ELEMENTS;
			*trnCollideList = VehicleData->ShortTrnList;
		}else // use medium by default then
		{
			trnCollideListSize = ML_NUM_MED_LIST_ELEMENTS;
			*trnCollideList = VehicleData->MedTrnList;
		}
	}

	return trnCollideListSize;
}

/************************************************************************************************
	_CollideRoundWithAutoguns
*************************************************************************************************/

UINT32 _CollideRoundWithAutoguns(FASTCOLLIDE ** CollideListPtr,
								 UINT32* NumCollisionSlotsLeft,
								 piVECTOR const* OldPos,
								 piVECTOR const* NewPos,
								 FLOAT64 OldPosTime,
								 FLOAT64 NewPosTime)
{
	trnAutoGun * AG = nullptr;
    piVECTOR SphereCenter = {0, 0, 0};
	FLOAT32 Radius = 0;
	UINT32 Collision = 0;

	trnGetAutoGunHead(&AG);

	if(!AG) return 0;

	Radius = 5.0;

	while (AG != nullptr)
	{
		SphereCenter.x = (FLOAT32)(AG->Pos.x - WorldOriginOffset.x);
		SphereCenter.y = (FLOAT32)(AG->Pos.y - WorldOriginOffset.y);
		SphereCenter.z = -(FLOAT32)(AG->Pos.z - WorldOriginOffset.z);

		_CheckLineSphereCollide(&Collision,
								&SphereCenter,
								OldPos,
								NewPos,
								Radius);

		if(Collision)
		{
			weapCollideWithHitmapObject(CollideListPtr, NumCollisionSlotsLeft,
										AG->CollisionObject, AG->Object,
										OldPos, NewPos, 0, 1);
		}

		AG = AG->next;
	}

	return 0;
}


/************************************************************************************************
	_CheckLineSphereCollide
*************************************************************************************************/

static UINT32 _CheckLineSphereCollide(UINT32 * Collision, piVECTOR const* SphereCenter,
									piVECTOR const* UseOldPos, piVECTOR const* UseNewPos, FLOAT32 Radius)
{
	FLOAT32 a = 0,b = 0,c = 0;
	FLOAT32 c1 = 0,c2 = 0,c3 = 0;
	FLOAT32 A = 0,B = 0,C = 0;
	FLOAT64 Det = 0;
	FLOAT64 T1 = 0,T2 = 0;

	*Collision = 0;

	// quick trivial reject based on the following math:
	// A = dist from oldpos to sphereCenter
	// B = dist from oldpos to newpos
	// R = sphere radius
	// C = dist from oldpos to sphere surface
	//
	// A = C + R ->  a2 = c2 + r2 + 2cr
	// a2 - r2 = c2 + 2cr
	//
	// we're looking to reject cases where b2 < c2
	//
	// since a2 - r2 > c2
	//
	// we can reject cases where b2 < (a2 - r2) 100% of the time
/*
#ifdef GREN_STAT_MODEL
	{
		FLOAT32		b2, a2, r2;
		piVECTOR	aVec, bVec;

		mathSubVectors(&aVec, UseOldPos, SphereCenter);
		mathSubVectors(&bVec, UseOldPos, UseNewPos);

		r2 = Radius * Radius;

		b2 = mathExactVectorLength2(&bVec);
		a2 = mathExactVectorLength2(&aVec);

		if (b2 < (a2 - r2))
			return 0;
	}
#endif
*/
	c1 = UseOldPos->x - SphereCenter->x;
	c2 = UseOldPos->y - SphereCenter->y;
	c3 = UseOldPos->z - SphereCenter->z;
	a = UseNewPos->x - UseOldPos->x;
	b = UseNewPos->y - UseOldPos->y;
	c = UseNewPos->z - UseOldPos->z;

	A = a * a + b * b + c * c;
	B = 2 * a * c1 + 2 * b * c2 + 2 * c * c3;
	C = c1 * c1 + c2 * c2 + c3 * c3 - Radius * Radius;

	// determinate check
	Det = B * B - 4 * A * C;
	if (Det < 0)
	{
		return 0;
	}

    if(_isnan(Det) || !_finite(Det))
    {
        return 0;
    }

	Det = sqrt(Det);

	T1 = (-B + Det) / (2 * A);

	// check T1
	if (T1 >= 0.0 && T1 <= 1.0)
	{
		*Collision = 1;
		return 0;
	}

	// check T2
	T2 = (-B - Det) / (2 * A);
	if (T2 >= 0.0 && T2 <= 1.0)
	{
		*Collision = 1;
		return 0;
	}

	// check if T1 and T2 surround time zero to 1
	if ((T1 < 0.0 && T2 > 1.0) || (T1 > 1.0 && T2 < 0.0))
	{
		*Collision = 1;
		return 0;
	}

	return 0;

}

/************************************************************************************************
	_IsWater
*************************************************************************************************/

static LOGICAL _IsWater(OBJECT* lpObj)
{
    if(lpObj == nullptr)  return FALSE;

    if(lpObj->collider && colliderIsType(lpObj->collider->getCategoryMask(), COLLIDER_TRN_WATER))
    {
	    return TRUE;
    }

    if(trnwtrIsWaterTexture(lpObj->texid))
    {
        return TRUE;
    }

    FLOAT32 fRollCoef = 0.0f;
    mlGetObjectBumpinessAndFriction(nullptr, nullptr, nullptr, nullptr, &fRollCoef, nullptr, lpObj);

    vehVehicle*pVehicle = persGetPersonaVehiclePtr();
    if(pVehicle && pVehicle->VehicleType)
   	{
        if(pVehicle->VehicleType->Type == VEH_TYPE_SEA)
        {
            if(fRollCoef > 0.999f && fRollCoef < 1.001f)
            {
                return TRUE;
            }
        }else if(fRollCoef > 198.99f && fRollCoef < 199.001f)
        {
            return TRUE;
        }
    }else if(fRollCoef > 198.99f && fRollCoef < 199.001f)
    {
        return TRUE;
    }

    return FALSE;
}

/************************************************************************************************
	_UpdateProjectile
*************************************************************************************************/

static SINT32 _UpdateProjectile(weapAmmo * Ammo)
{
	FLOAT64 CurrentTime = 0;
	LOGICAL KillAmmo = FALSE;

	if(!Ammo || !Ammo->AmmoType) return -1;

	CurrentTime = (FLOAT64)frametime/1000.0;
	Ammo->LastVehicleHit = nullptr;

	// sto rounds can come in anytime throughout their lifetime
	const FLOAT64 SANITY_DELTA = ((Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND) && Ammo->AmmoType->UseGhostRoundSTO) ?
									PROJECTILE_DELTA_SANITY_TIME + Ammo->AmmoType->LifeTime + 1:
									PROJECTILE_DELTA_SANITY_TIME;

	// check for expired rounds
	if(	(CurrentTime - Ammo->LastUpdateTime > SANITY_DELTA) ||
		(CurrentTime > (Ammo->LaunchTime + Ammo->AmmoType->LifeTime) + 2) )
	{
		if(!(Ammo->Flags & WEAP_AMMOFLAG_IS_GHOSTROUND))
			Write_Log(LOG_WEAP, LOG_INFO, "WEAP %s(%d) timed out after %f s", Ammo->AmmoType->Name, Ammo->Id, Ammo->AmmoType->LifeTime);

	    KillAmmo = TRUE;
	}
    else
	{
		Ammo->PrevRenderPos = Ammo->Position;

		// update ammo position
		if(Ammo->Flags & WEAP_AMMOFLAG_ISATTACHED_TO_OBJECT) // probably should be an ammo type instead of a flag
			KillAmmo = _UpdateAttachedProjectile(Ammo);
		else if(Ammo->AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO)
			KillAmmo = _UpdateTorpedoProjectile(Ammo);
		else if(IsBouncingGrenade(Ammo->AmmoType))
			KillAmmo = _UpdateGrenade(Ammo);
		else
			KillAmmo = _UpdateBallisticProjectile(Ammo);

		Ammo->PrevUpdateTime = Ammo->LastUpdateTime;
		Ammo->LastUpdateTime = Ammo->LastCollideTime = CurrentTime;
		Ammo->FirstUpdate = FALSE;
		Ammo->LastCollidePos = Ammo->Position;
	}

	if(KillAmmo)
		_KillAmmo(Ammo);

	return 0;
}

/************************************************************************************************
	_UpdateBallisticProjectile
*************************************************************************************************/

static LOGICAL _UpdateBallisticProjectile(weapAmmo* Ammo)
{
	LOGICAL KillAmmo = FALSE;
	FLOAT64 Speed = 0;
    FLOAT64 Mach = 0;
    FLOAT64 Cd = 0;
    FLOAT64 Density = 0;
    FLOAT64 Drag = 0;
	UINT32 MachIdx = 0;
    FLOAT64VECTOR Accel = {0, 0, 0};
    FLOAT64VECTOR AvgAcc = {0, 0, 0};
    FLOAT64VECTOR LastPosition = {0, 0, 0};
	FLOAT64 CurrentTime = 0;
    FLOAT64 DeltaTime = 0;

	CurrentTime = (FLOAT64)frametime/1000.0;
	DeltaTime = CurrentTime - Ammo->LastUpdateTime;

	if(DeltaTime < FLT_MIN)
    {
        return true;
    }

	// Get mach number for drag
	Speed = mathExactVectorLength(&Ammo->Velocity);
	mlGetAltSpeedOfSound(&Mach, Ammo->Position.z);
	Mach = Speed / Mach;
	MachIdx = (UINT32)(floor(Mach / G1_CHART_MACH_DELTA));

	// Drag Coefficient
	if(MachIdx + 1 < G1_CHART_NUM_DIVISIONS)
	{
		FLOAT64 Delta = (Mach / G1_CHART_MACH_DELTA) - MachIdx;
		mlInterpolate(&Cd ,G1MachChart[MachIdx],G1MachChart[MachIdx+1], Delta);
	}else
	{
		Cd = G1MachChart[G1_CHART_NUM_DIVISIONS - 1];
	}

	mlGetAltDensity(&Density, Ammo->Position.z);

	// NOTE: the 1790 is a constant that makes the Cd table work with
	// G1 ballistic coefficients and this particular formula
	Drag = Density * Cd * Speed / (Ammo->AmmoType->BallisticCoefficient * 1790.0f);
	Accel.x = -Drag * Ammo->Velocity.x;
	Accel.y = -Drag * Ammo->Velocity.y;
	Accel.z = -Drag * Ammo->Velocity.z;

	AvgAcc.x = (Accel.x + Ammo->LastAcc.x) * 0.5;
	AvgAcc.y = (Accel.y + Ammo->LastAcc.y) * 0.5;
	AvgAcc.z = (Accel.z + Ammo->LastAcc.z) * 0.5;

	Ammo->Velocity.x += AvgAcc.x * DeltaTime;
	Ammo->Velocity.y += AvgAcc.y * DeltaTime;
	Ammo->Velocity.z += AvgAcc.z * DeltaTime;

	Ammo->LastAcc = Accel;
    LastPosition = Ammo->Position;

	Ammo->Velocity.z -= 9.8 * DeltaTime; // gravity

	// update position
	Ammo->Position.x += Ammo->LastVel.x * DeltaTime + 0.5 * AvgAcc.x * DeltaTime * DeltaTime;
	Ammo->Position.y += Ammo->LastVel.y * DeltaTime + 0.5 * AvgAcc.y * DeltaTime * DeltaTime;
	Ammo->Position.z += Ammo->LastVel.z * DeltaTime + 0.5 * AvgAcc.z * DeltaTime * DeltaTime;

	Ammo->LastVel = Ammo->Velocity;

	// tracer fx
	if( Ammo->AmmoType->UseParticlesForTracer &&
	  !(Ammo->Flags & WEAP_AMMOFLAG_FIRSTFRAME && Ammo->AmmoType->IgnoreFirstFrame) &&
		(Ammo->AmmoType->TracerTime > 0) &&
		(Ammo->LastUpdateTime < Ammo->LaunchTime + Ammo->AmmoType->TracerTime) )
	{
		weapfxAmmoTracer(Ammo, &LastPosition);
	}

	// turn off first frame flag on 2nd update
	if (Ammo->PrevUpdateTime != 0.0)
		Ammo->Flags &= ~WEAP_AMMOFLAG_FIRSTFRAME;

	// Update Projectile (hit things, explode, etc)
	KillAmmo = _DoProjectileUpdate(Ammo, &Ammo->LastCollidePos, CurrentTime - Ammo->LastCollideTime, CurrentTime);

	// check for flyby sound
	if(Ammo->AmmoType->Object)
		weapfxAmmoFlyby(Ammo);

	return KillAmmo;
}

/************************************************************************************************
	_UpdateAttachedProjectile
*************************************************************************************************/

static LOGICAL _UpdateAttachedProjectile(weapAmmo* Ammo)
{
	LOGICAL KillAmmo = FALSE;
    piVECTOR Temp32Vec = {0, 0, 0};
	vehVehicle* Vehicle = nullptr;
	FLOAT64 CurrentTime = 0, DeltaTime = 0;

	CurrentTime = (FLOAT64)(frametime)/1000.0;
	DeltaTime = CurrentTime - Ammo->LastUpdateTime;

	// position
	Temp32Vec = Ammo->AttachedPnt;
	Temp32Vec.x += Ammo->AttachedNorm.x * 0.02f;
	Temp32Vec.y += Ammo->AttachedNorm.y * 0.02f;
	Temp32Vec.z += Ammo->AttachedNorm.z * 0.02f;

	Vehicle = (vehVehicle*)Ammo->AttachedToVehicle;

	if(Vehicle)
		weapUpdateVehicleHitmap(Vehicle);

	if ( ! (Ammo->AttachedObject && obmainValidateObject(Ammo->AttachedObject)) ) {
		// HACK: If the object is gone, at least trigger the explosion effect.
		// This is mostly an issue when a PPO is destroyed with satchels attached:
		// Currently the two events are processed out-of-order, so the killing
		// explosion won't render on other clients.
		weapfxAmmoExplosion(Ammo, &Ammo->Position, nullptr, 0);
		return TRUE;
	}

	math3dVectorMultiply(&Temp32Vec,&Temp32Vec,&Ammo->AttachedObject->mat);
	mathAddVectors(&Ammo->Position,&WorldOriginOffset,&Temp32Vec);
	Ammo->Position.z = -Ammo->Position.z;

	// attitude
	Temp32Vec = Ammo->AttachedNorm;
	math3dVectorMultiply(&Temp32Vec,&Temp32Vec,&Ammo->AttachedObject->mat);
	Temp32Vec.x -= Ammo->AttachedObject->mat._41;
	Temp32Vec.y -= Ammo->AttachedObject->mat._42;
	Temp32Vec.z -= Ammo->AttachedObject->mat._43;
	Ammo->Velocity.x = -Temp32Vec.x;
	Ammo->Velocity.y = -Temp32Vec.y;
	Ammo->Velocity.z = Temp32Vec.z;

	if((CurrentTime + DeltaTime - Ammo->LaunchTime > Ammo->AmmoType->FuzeTime) &&  Ammo->AmmoType->FuzeTime > 0)
	{
		weapfxAmmoExplosion(Ammo, &Ammo->Position, nullptr, 0);

		_DoExplosionShrapnel(Ammo, &Ammo->Position, &Ammo->Velocity, frametime / 1000.0);
		_DoExplosionConcussion(Ammo, &Ammo->Position, FALSE);

		KillAmmo = TRUE;
	}

	return KillAmmo;
}

/************************************************************************************************
	_UpdateTorpedoProjectile
*************************************************************************************************/
// just runs in a straight line stuck on the surface of the water at the moment, no real physics

static LOGICAL _UpdateTorpedoProjectile(weapAmmo* Ammo)
{
	LOGICAL KillAmmo = FALSE;
	FLOAT64 CurrentTime = 0, DeltaTime = 0;
    FLOAT64VECTOR TorpPos = {0, 0, 0};

	CurrentTime = (FLOAT64)(frametime)/1000.0;
	DeltaTime = CurrentTime - Ammo->LastUpdateTime;

	if(Ammo->TorpedoIsRunning)
	{
		// Update Position
		Ammo->LastVel = Ammo->Velocity;
		Ammo->Position.x += Ammo->Velocity.x * DeltaTime;
		Ammo->Position.y += Ammo->Velocity.y * DeltaTime;

		// stick torpedo to water and check if colllided with land
		KillAmmo = _StickTorpedoToWater(Ammo);

		if(!KillAmmo)
		{
			// offset to running depth
			TorpPos = Ammo->LastCollidePos;
			TorpPos.z -= Ammo->TorpedoRunDepth;
			Ammo->Position.z -= Ammo->TorpedoRunDepth;
			KillAmmo = _DoProjectileUpdate(Ammo, &TorpPos, CurrentTime - Ammo->LastCollideTime, CurrentTime);
			Ammo->Position.z += Ammo->TorpedoRunDepth; // and back up
		}
        else
        {
            // Set off explosion if torpedo hits land
		    weapfxAmmoExplosion(Ammo, &Ammo->Position, nullptr, 0);

            if(Ammo->iParticleID > 0)
            {
               // FXStopEffect(Ammo->iParticleID);
            }
        }

	}else
	{
		KillAmmo = _UpdateBallisticProjectile(Ammo);

		if(KillAmmo) // torpedo hit ground
		{
			// display splash, check if hit too hard, set torp velocity
			FLOAT64VECTOR dir = {0, 1, 0};
			FLOAT64 speed;

			// not doing this because we don't have a #$%!@ fire message
			//speed = mathExactVectorLength(&Ammo->Velocity);
			if(1)//speed < MAX_TORPEDO_DROP_SPEED)
			{
				mathVectorMultiply64(&dir, &dir, &Ammo->FireAtt);
				speed = sqrt(dir.x * dir.x + dir.y * dir.y);

	 			Ammo->Velocity.x = Ammo->AmmoType->TorpedoRunSpeed * dir.x / speed;
		 		Ammo->Velocity.y = Ammo->AmmoType->TorpedoRunSpeed * dir.y / speed;
				Ammo->Velocity.z = 0;

				Ammo->TorpedoIsRunning = TRUE;
				Ammo->Flags &= ~WEAP_AMMOFLAG_DONTCHECKVEHICLES; 	// make sure ghost rounds actually hit
				Ammo->Flags &= ~WEAP_AMMOFLAG_ONLY_CHECKPLAYER; 	// check everybody
				Ammo->Flags &= ~WEAP_AMMOFLAG_CANTHITPLAYER; 		// ok is this it? enough  negative logic. :)

				KillAmmo = FALSE; // nope keep on trucking

                weapfxTorpedo(Ammo);
			}
		}
	}

	return KillAmmo;
}

/************************************************************************************************
	_StickTorpedoToWater
*************************************************************************************************/

static LOGICAL _StickTorpedoToWater(weapAmmo* Ammo)
{
	LOGICAL KillAmmo = FALSE;
	OBJECT* CollideObject = nullptr;
    piVECTOR CollidePos = {0, 0, 0};
    FLOAT64VECTOR WorldCollidePos = {0, 0, 0};
    piVECTOR V1 = {0, 0, 0},V2 = {0, 0, 0};

	V1.x = (FLOAT32)(Ammo->Position.x - WorldOriginOffset.x);
	V1.y = (FLOAT32)(Ammo->Position.y - WorldOriginOffset.y);
	V1.z = (FLOAT32)(Ammo->Position.z - WorldOriginOffset.z); V1.z = -V1.z;
	V2 = V1; V2.z += 10000.0; V1.z -= 10000.0;

	trncdCollideWithVehiclesAndTerrain(&V1,&V2,&CollidePos,&CollideObject,nullptr,nullptr,nullptr,FALSE,TRUE, FALSE);

	if(CollideObject)
	{
		WorldCollidePos.x = CollidePos.x; WorldCollidePos.y = CollidePos.y; WorldCollidePos.z = CollidePos.z;
		mathVectorMultiply64(&WorldCollidePos, &WorldCollidePos, &CollideObject->mat);
		mathAddVectors(&WorldCollidePos, &WorldCollidePos, &WorldOriginOffset); WorldCollidePos.z = -WorldCollidePos.z;

		KillAmmo = !_IsWater(CollideObject);
		Ammo->Position.z = WorldCollidePos.z + 0.1;
	}

	return KillAmmo;
}


#define AVERAGE_STO_LAG 350

bool OrdnanceSTO::activate()
{
	weapAmmo* Ammo;

	_GetNewAmmo(&Ammo);
	if(!Ammo) return false;

	weapAmmoType* AmmoType = weapFindAmmoTypeByIndex(clientAmmoIndex());
	if(!AmmoType || !AmmoType->UseGhostRoundSTO) return false;

	FLOAT64VECTOR Pos;
	Pos.x = position().x();
	Pos.y = position().y();
	Pos.z = position().z();

	FLOAT64VECTOR ProjVel;
	ProjVel.x = attitude().x();
	ProjVel.y = attitude().y();
	ProjVel.z = attitude().z();

	Ammo->PrevUpdateTime = 0;
	Ammo->LaunchTime = Ammo->LastUpdateTime = Ammo->LastCollideTime = (frametime - age() - AVERAGE_STO_LAG)/1000.0;
	Ammo->AmmoType = AmmoType;
	Ammo->Position = Ammo->LastCollidePos = Pos;
	Ammo->Velocity = Ammo->LastVel = ProjVel;
	Ammo->Flags = WEAP_AMMOFLAG_IS_GHOSTROUND | WEAP_AMMOFLAG_DONT_SEND_DAMAGE;
	Ammo->Id = _AmmoObjectCounter++;
	Ammo->FirstUpdate = TRUE;
	Ammo->FireVehicle = 0;

	if(Ammo->AmmoType->IgnoreFirstFrame)
		Ammo->Flags |= WEAP_AMMOFLAG_FIRSTFRAME;

	if(!Ammo->AmmoType->MuzzleFlashType)
		Ammo->Flags |= WEAP_AMMOFLAG_NOMUZZLEFLASH;

	if(IsBouncingGrenade(AmmoType))
		_InitGrenade(Ammo, randomSeed());

	weapfxAmmoAttachSound(Ammo);

	return ServerTrackedObject::activate();
}

void OrdnanceSTO::deactivate()
{
	// client will handle ammo getting killed
	 ServerTrackedObject::deactivate();
}

