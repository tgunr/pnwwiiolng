#include "syscfg.h" // Path is project specific.


#include "ob/stride_ptr.h"

#include <ctype.h>

#include "mem/mem.h"
#include "fio/fio.h"
#include "ml/mlutility.h"

#include "weap/weapons.h"
#include "weap/weapammo.h"

#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"

#include "trn/trnmain.h"
#include "trn/trncd.h"
#include "math/mathvect.h"
#include "ld/ldpix.h"

#include "draw/draw3d.h"

#include "veh/vehmain.h"

#include "fp/fpFPS.h"
#include "fp/fpTPS.h"
#include "gr2/gr2Anim.h"
#include "tut/TutorialManager.h"

#include "veh/vehfx.h"

static UCHAR _WeaponVarNames[WEAP_VAR_MAXVAR][32] = {
	"none",
	"mass",
	"firetype",
	"rateoffire",
	"notused4",
	"ammotype1",
	"ammotype2",
	"ammotype3",
	"ammotype4",
	"ammotype5",
	"ammotype6",
	"ammotype7",
	"ammotype8",
	"dispersion",
	"ghostrof",
	"ghostammotype",
	"hideonlastdrop",
	"cliprounds",
	"reloadtime",
	"isplaced",
	"nomuzzlesmoke",
	"maxoversupply",
	"infresupply",
	"cliptype",
    "muzzleflash"
};

static UCHAR _WeaponFireTypes[WEAP_MAX_FIRETYPE][32] = {
	"none",
	"singleshot",
	"syncauto",
	"auto",
};

UINT32 _NumWeaponTypes;
weaponType * _WeaponList; // the ammo type list
static std::vector<WEAP_FIRE_CALLBACK> _fireCallbacks; // list of callbacks to fire when a weapon is fire

SINT32 weaponsInit()
{
	int WeaponListFile;
	int CurrentWeaponFile;
	UCHAR FileName[300];
	UCHAR Buffer[256];
	UCHAR* VarName;
	UCHAR* VarContents;
	UINT32 VarNum;
	UINT32 CurrentPos;
	weaponType* WeaponType;
	UINT32 i;

	_NumWeaponTypes = 0;

	sprintf((char*)FileName,"vehicles\\weapons.txt");

	WeaponListFile = fioOpen((char*)FileName);

	if (WeaponListFile == -1)
	{
		WriteDebug("Can't find data/vehicles/weapons.txt file!");
		return -1;
	}

	while (fioGetString(WeaponListFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] != '#' && strlen((char*)Buffer) > 0)
		{
			_NumWeaponTypes++;
		}
	}

	fioClose(WeaponListFile);

	_WeaponList = (weaponType*)memAlloc(sizeof(weaponType) * _NumWeaponTypes);

	WeaponListFile = fioOpen((char*)FileName);

	CurrentPos = 0;
	while (fioGetString(WeaponListFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] != '#' && strlen((char*)Buffer) > 0)
		{
			sprintf((char*)FileName,"vehicles\\weapons\\%s",(char*)Buffer);
			CurrentWeaponFile = fioOpen((char*)FileName);

			if (CurrentWeaponFile == -1)
				continue;

			WeaponType = &_WeaponList[CurrentPos];
			strcpy((char*)WeaponType->Name,(char*)Buffer); // copy the ammo's name

			for (i=0;i<strlen((char*)WeaponType->Name);i++)
				WeaponType->Name[i] = (UCHAR)tolower(WeaponType->Name[i]);

			while (fioGetString(CurrentWeaponFile,(char*)Buffer,255) != 0)
			{
				if (Buffer[0] == '#')
					continue;

				mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

				if (VarName == nullptr || VarContents == nullptr)
					continue;

				mlUtilityFindVarname(&VarNum,
									 VarName,
									 (UCHAR*)&_WeaponVarNames[0][0],
									 32,
									 WEAP_VAR_MAXVAR);

				switch (VarNum)
				{
					case WEAP_VAR_MASS:
						WeaponType->Mass = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_VAR_FIRETYPE:
						mlUtilityFindVarname(&WeaponType->FireType,
											VarContents,
											(UCHAR*)&_WeaponFireTypes[0][0],
											32,
											WEAP_MAX_FIRETYPE);
					break;
					case WEAP_VAR_RATEOFFIRE:
						WeaponType->RateOfFire = (FLOAT32)(60.0 / atof((char*)VarContents));
					break;
					case WEAP_VAR_GHOST_ROF:
						WeaponType->GhostRateOfFire = (FLOAT32)(60.0 / atof((char*)VarContents));
					break;
					case WEAP_VAR_GHOST_AMMOTYPE:
                        {
						    weapFindAmmoTypeByName(&WeaponType->GhostAmmoType,VarContents);

                            if(WeaponType->GhostAmmoType == nullptr)
                            {
                                char s[128];
                                strprintf(s, "ghostammotype %s not found", VarContents);
                                WriteDebug(s);
#if defined(BETA_RELEASE) && !defined (_DEBUG) && (platOS == OS_WIN32) // temp define until they fix the data
                                piExitProgram(_FL, s);
#endif
                            }

                            break;
                        }
					case WEAP_VAR_AMMOTYPE1:
					case WEAP_VAR_AMMOTYPE2:
					case WEAP_VAR_AMMOTYPE3:
					case WEAP_VAR_AMMOTYPE4:
					case WEAP_VAR_AMMOTYPE5:
					case WEAP_VAR_AMMOTYPE6:
					case WEAP_VAR_AMMOTYPE7:
					case WEAP_VAR_AMMOTYPE8:
                        {
						    weapFindAmmoTypeByName(&WeaponType->AmmoType[VarNum-WEAP_VAR_AMMOTYPE1],VarContents);

                            if(WeaponType->AmmoType[VarNum-WEAP_VAR_AMMOTYPE1] == nullptr)
                            {
                                char s[128];
                                strprintf(s, "ammotype %s not found", VarContents);
                                WriteDebug(s);
#if defined(BETA_RELEASE) && !defined (_DEBUG) && (platOS == OS_WIN32) // temp define until they fix the data
                               piExitProgram(_FL, s);
#endif
                            }

                            break;
                        }
					case WEAP_VAR_DISPERSION:
						WeaponType->Dispersion = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_VAR_HIDEONLASTDROP:
						WeaponType->HideOnLastDrop = atoi((char*)VarContents);
					break;
					case WEAP_VAR_NUMROUNDSINCLIP:
						WeaponType->NumRoundsInClip = atoi((char*)VarContents);
					break;
					case WEAP_VAR_RELOADTIME:
						WeaponType->ReloadTime = (FLOAT32)atof((char*)VarContents);
					break;
					case WEAP_VAR_WEAPONPLACED:
						WeaponType->IsPlacedWeapon = atoi((char*)VarContents);
						break;
					case WEAP_VAR_NOMUZZLESMOKE:
						WeaponType->NoMuzzleSmoke = atoi((char*)VarContents);
					break;
                    case WEAP_VAR_MUZZLEFLASH:
                        {
                            strcpy(WeaponType->muzzleflash, (char*)VarContents);
                            break;
                        }
					case WEAP_VAR_CLIPTYPE:
					{
						// defaults to clip (0)
						if(strcmp("na", (char*)VarContents) == 0)
							WeaponType->ClipType = WEAPAMMO_CLIPTYPE_NA;
						else if(strcmp("clip", (char*)VarContents) == 0)
							WeaponType->ClipType = WEAPAMMO_CLIPTYPE_CLIP;
						else if(strcmp("magazine", (char*)VarContents) == 0)
							WeaponType->ClipType = WEAPAMMO_CLIPTYPE_MAGAZINE;
						else if(strcmp("belt", (char*)VarContents) == 0)
							WeaponType->ClipType = WEAPAMMO_CLIPTYPE_BELT;
						else if(strcmp("reload", (char*)VarContents) == 0)
							WeaponType->ClipType = WEAPAMMO_CLIPTYPE_RELOAD;
						else if(strcmp("drum", (char*)VarContents) == 0)
							WeaponType->ClipType = WEAPAMMO_CLIPTYPE_DRUM;
					}
					break;
					default:
						WriteDebug("Unidentified var %s in file %s",VarName,FileName);
					break;
				}
			}

			fioClose(CurrentWeaponFile);

			CurrentPos++;

		}
	}

	fioClose(WeaponListFile);

	return 0;

}

UINT32 weapFindWeaponTypeByName(weaponType ** WeaponType,UCHAR * WeaponName)
{
UINT32 i;

	if (WeaponType == nullptr || WeaponName == nullptr)
	{
		return 1;
	}

	*WeaponType = nullptr;

	for (i=0;i<_NumWeaponTypes;i++)
	{
		if (strcmp((char*)_WeaponList[i].Name,(char*)WeaponName) == 0)
		{
			*WeaponType = &_WeaponList[i];
			break;
		}
	}

	return 0;
}

UINT32 weaponsKill()
{
	if (_WeaponList != nullptr)
	{
		memFree(_WeaponList);
		_WeaponList = nullptr;
	}

	return 0;
}


UINT32 weapInitializeWeapon(weapWeapon * Weapon,weaponType * WeaponType)
{
	if (Weapon == nullptr || WeaponType == nullptr)
	{
		return 1;
	}

//	memset(Weapon,0,sizeof(weapWeapon));
	Weapon->WeaponType = WeaponType;

	return 0;
}

UINT32 weapCreateWeaponByName(weapWeapon * Weapon,
							  UCHAR * WeaponName)
{
weaponType * WeaponType;


	weapFindWeaponTypeByName(&WeaponType,WeaponName);

	if (WeaponType == nullptr)
	{
		return 1;
	}

	weapInitializeWeapon(Weapon,WeaponType);

	return 0;
}


UINT32 weapFireWeapon(FLOAT64VECTOR * RecoilForce,
					  weapWeapon * Weapon,
					  FLOAT64VECTOR * Pos,
					  FLOAT64VECTOR * Vel,
					  piMATRIX * FireAtt,
					  FLOAT64 FireDelta,
					  FLOAT64 CurrentTime,
					  UINT32 CanCauseDamage,
					  UINT32 FiredFromGhost,
					  UINT32 Flags,
					  OBJECT * EmitterObject,
					  FLOAT64VECTOR * RelativePos,
					  FLOAT64 * LastFireTime,
					  void* FireVehicle) // note: FireDelta is in seconds
{
	FLOAT64			FireInterval = 0.0f;
	weapAmmoType*	AmmoType = nullptr;
    FLOAT64VECTOR	TempRecoilForce = {0, 0, 0};
	piMATRIX		FinalFireAtt = IdentityMatrix;
    piVECTOR		Dispersion = {0, 0, 0};
	SINT32			AmmoGroup = 0;
	UINT32			FiredWeapon = 0;
    FLOAT64VECTOR	worldOriginOffset = {0, 0, 0};
    piVECTOR		V1 = {0, 0, 0};
    piVECTOR        V2 = {0, 0, 0};
    piVECTOR		origV1 = {0, 0, 0};
    piVECTOR		CollidePnt = {0, 0, 0};
	OBJECT*			CollideObject = nullptr;
	SINT32			PolyNum = 0;
	vehVehicle*		CollideVehicle = nullptr;
	trnCDTerrainObject*	TO = nullptr;
	LOGICAL			reloading = FALSE;

	if (Weapon->WeaponType == nullptr)
	{
		WriteDebug("Attempted to fire a weapon without a weapontype");
		return 0;
	}

	/* for debugging turret info
	if(FireVehicle)
	{
		vehVehicle* v = (vehVehicle*)FireVehicle;

		if(!Online || !strcasecmp((const char*)v->Handle, "spamster") || !strcasecmp((const char*)v->Handle, "agentx"))
		{
			vehanimVehicleStateAnimations& anims = v->StateAnimation;
			FLOAT64ATT& att =  (v->ModelData) ? v->ModelData->VehicleAtt: v->PosData.Att;

			if(v->ModelData)
			{
				FLOAT32 trav0 = anims.LastSendTurretTrav[0]  / TURRET_GRANULARITY * 2.0f - 1.0f;
				FLOAT32 elev0 =  anims.LastSendTurretElev[0] / TURRET_GRANULARITY * 2.0f - 1.0f;
				FLOAT32 trav1 = anims.LastSendTurretTrav[1]  / TURRET_GRANULARITY * 2.0f - 1.0f;
				FLOAT32 elev1 =  anims.LastSendTurretElev[1] / TURRET_GRANULARITY * 2.0f - 1.0f;

				uiChatPrintLine("FP: pitch=%.2f roll=%.2f yaw=%.2f trav0=%.2f elev0=%.2f trav1=%.2f elev1=%.2f", att.Pitch, att.Roll, att.Yaw, trav0, elev0, trav1, elev1);
			}
			else
			{
				uiChatPrintLine("TP: pitch=%.2f roll=%.2f yaw=%.2f trav0=%.2f elev0=%.2f trav1=%.2f elev1=%.2f", att.Pitch, att.Roll, att.Yaw, anims.TurretTraverse[0], anims.TurretElev[0], anims.TurretTraverse[1], anims.TurretElev[1]);
			}
		}
	}
	*/

	FireInterval = (!FiredFromGhost) ? Weapon->WeaponType->RateOfFire : Weapon->WeaponType->GhostRateOfFire;
	AmmoGroup = Weapon->AmmoTypeInUse;

	if (Weapon->LastFireTime < CurrentTime - FireDelta)
	{
		Weapon->LastFireTime = CurrentTime - FireDelta;

		if (Weapon->WeaponType->FireType == WEAP_FIRETYPE_AUTO_NORMAL &&
			FiredFromGhost != 0 &&
			(!(Flags & WEAP_AMMOFLAG_TPS_WEAPON)))
		{
			// apply artificial stagger
			Weapon->LastFireTime += mathGetFloat32ChaoticNumber() *
								Weapon->WeaponType->GhostRateOfFire;
		}
	}

	RecoilForce->x = RecoilForce->y = RecoilForce->z = 0;
	while (Weapon->LastFireTime < CurrentTime &&
			(Weapon->ReloadStartTime == 0 ||
			 CurrentTime - Weapon->ReloadStartTime > Weapon->WeaponType->ReloadTime))
	{
		// fire next round, if any rounds left
		if (((Flags & WEAP_AMMOFLAG_FPS_WEAPON) && (fpFPSStateWeaponReadyToFire())) ||
			(!(Flags & WEAP_AMMOFLAG_FPS_WEAPON) && ((Weapon->NumRoundsAmmoLeft[AmmoGroup] == -1 ||	Weapon->NumRoundsAmmoLeft[AmmoGroup] > 0)))
			)
//		if (Weapon->NumRoundsAmmoLeft[AmmoGroup] == -1 ||
//			Weapon->NumRoundsAmmoLeft[AmmoGroup] > 0)
		{
			// determine next ammo type
			if (FiredFromGhost == 1)
			{
				if (Flags & WEAP_AMMOFLAG_TPS_WEAPON)
				{
					SINT32	ammoIndex;

					ammoIndex = gr2AnimGetAmmoTypeNum((vehVehicle*)FireVehicle);

					if (ammoIndex < 0)
					{
						AmmoType = Weapon->WeaponType->GhostAmmoType;
					}
					else
					{
						AmmoType = Weapon->WeaponType->AmmoType[gr2AnimGetAmmoTypeNum((vehVehicle*)FireVehicle)];
					}
				}
				else
				{
					AmmoType = Weapon->WeaponType->GhostAmmoType;
				}
			}
			else
			{
				if (Weapon->NumRoundsInBeltSequence[AmmoGroup] == 0)
				{
					if (Flags & WEAP_AMMOFLAG_TPS_WEAPON)
					{
						AmmoType = Weapon->WeaponType->AmmoType[gr2AnimGetAmmoTypeNum((vehVehicle*)FireVehicle)];
					}
					else if (Flags & WEAP_AMMOFLAG_FPS_WEAPON)
					{
						AmmoType = Weapon->WeaponType->AmmoType[fpFPSGetAmmoTypeNum(true)];
					}
					else
					{
						AmmoType = Weapon->WeaponType->AmmoType[0];
					}
				}
				else
				{
					UINT32 position = Weapon->BeltSequencePosition[AmmoGroup];	// Its covert, reduce uses
					AmmoType = Weapon->BeltSequence[AmmoGroup][position];

					++position;
					if (position >= Weapon->NumRoundsInBeltSequence[AmmoGroup])
					{
						position = 0;
					}

					Weapon->BeltSequencePosition[AmmoGroup] = position;
				}
			}

			// FIXME - this is goofy
			// radius of the generated cone is
			// tan (1.6) * distance ..
			// or .. 27.932529196587492237457195028782 meters at 1000 meters
			// calculate dispersion ( 80% of rounds will be within the dispersion)
			Dispersion.x = (FLOAT32)(1.6 * Weapon->WeaponType->Dispersion);

			if (Flags & WEAP_AMMOFLAG_FPS_WEAPON)
			{
				FLOAT32	 fpsDispersion;

				fpsDispersion = fpFPSGetBulletDispersion();
				if (fpsDispersion != 0.0)
					Dispersion.x += fpFPSGetBulletDispersion();
				else
					Dispersion.x = 0.0;
			}

			Dispersion.x *= mathGetFloat32ChaoticNumber() * mathGetFloat32ChaoticNumber();

			Dispersion.y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * PI;
			Dispersion.x = (FLOAT32)mlDEG_TO_RAD(Dispersion.x);
			Dispersion.z = 0;

			// ghost rounds already have some dispersion due to lag, no need to in exacerbating the problem
			// so we significating reduce the dispersion effect for ghost rounds
			if(FiredFromGhost == TRUE)
				Dispersion.set(0,0,0); //mathScaleVector(&Dispersion, &Dispersion, .01f);

			mathEul_ToMatrix(&FinalFireAtt,EulOrdXYZs,&Dispersion);
			math3dMatrixMultiply(&FinalFireAtt,&FinalFireAtt,FireAtt);

			// fire next round
			TempRecoilForce.x = TempRecoilForce.y = TempRecoilForce.z = 0;

			if (CanCauseDamage == 1)
			{
				Flags |= WEAP_AMMOFLAG_CANDAMAGE;
			}
			else
			{
				Flags &= ~WEAP_AMMOFLAG_CANDAMAGE;
			}

			if (Weapon->WeaponType->NoMuzzleSmoke == 1)
			{
				Flags |= WEAP_AMMOFLAG_NOMUZZLEFLASH_SMOKE;
			}

			if (FiredFromGhost == TRUE)
			{
				Flags |= WEAP_AMMOFLAG_IS_GHOSTROUND;
			}

			if (Weapon->WeaponType->IsPlacedWeapon == 1)
			{
				TempRecoilForce.x = TempRecoilForce.y = TempRecoilForce.z = 0.0;

				trnGetWorldOriginOffset(&worldOriginOffset);
				worldOriginOffset.z = -worldOriginOffset.z;

				V1.x = (FLOAT32)(Pos->x - worldOriginOffset.x);
				V1.y = (FLOAT32)(Pos->y - worldOriginOffset.y);
				V1.z = -(FLOAT32)(Pos->z - worldOriginOffset.z);

				V2.x = V2.z = 0.0f;

                // [DCF] HACK!
                V2.y = FiredFromGhost ? 4.0f : 2.0f;

				math3dVectorMultiply(&V2,&V2,&FinalFireAtt);

                V2.z = -V2.z;

                // Yet another HACK!
                if(FiredFromGhost)
                {
                    mathSubVectors(&V2,&V1,&V2);
                }
                else
                {
                    mathAddVectors(&V2,&V1,&V2);
                }

				// adjust firepoint back a little
				piVECTOR dir;

				mathSubVectors(&dir, &V2, &V1);
				mathReallyFastNormalize(dir);
				math3dScaleVector(&dir, &dir, .2f);
				mathSubVectors(&V1, &V1, &dir);
				// end adjustment

#if(0)
                FLOAT64VECTOR v1 =
                {
                    V1.x + worldOriginOffset.x,
                    V1.y + worldOriginOffset.y,
                    V1.z
                };

                FLOAT64VECTOR v2 =
                {
                    V2.x + worldOriginOffset.x,
                    V2.y + worldOriginOffset.y,
                    V2.z
                };

                draw3dSetWorldDebugLine(&v1, &v2, "hello", colorBLUEALPHA);
#endif

				origV1 = V1;

				if (trncdCollideWithVehiclesAndTerrain(&V1,
														&V2,
														&CollidePnt,
														&CollideObject,
														&PolyNum,
														&CollideVehicle,
														&TO,
														FALSE,
														FALSE,
														FALSE) == TRUE)
				{
					piVECTOR	diff;
					piVECTOR norm = CollideObject->psnorms[PolyNum];
					norm.x = -norm.x;
					norm.y = -norm.y;
					norm.z = -norm.z;

					// sanity check
					mathSubVectors (&diff, &V1, &V2);
					if (mathExactVectorLength2(&diff) > (FiredFromGhost ? 16.0f : 4.0f))
					{
						FiredWeapon = 0;
/*						trncdCollideWithVehiclesAndTerrain(&origV1,
														&V2,
														&CollidePnt,
														&CollideObject,
														&PolyNum,
														&CollideVehicle,
														(void**)&TO,
														FALSE,
														FALSE,
														FALSE);
*/					}
					else
					{
						unsigned int error = weapPlaceRoundOnObject(&CollidePnt,
                                                                    CollideObject,
                                                                    &CollideObject->psnorms[PolyNum],
                                                                    &norm,
                                                                    Flags,
                                                                    Weapon->LastFireTime,
                                                                    AmmoType,
                                                                    CollideVehicle,
                                                                    TO);

                        if(!error)
                        {
						    TempRecoilForce.x = TempRecoilForce.y = TempRecoilForce.z = 0.1;
    						FiredWeapon = 1;
                        }
					}
				}
				else
				{
					FiredWeapon = 0;
				}
			}
			else
			{
				FiredWeapon = 1;
				weapFireRound(&TempRecoilForce,
							  AmmoType,
							  Pos,
							  Vel,
							  &FinalFireAtt,
							  Weapon->LastFireTime,
							  Flags,
							  EmitterObject,
							  RelativePos,
							  Weapon,
							  FireVehicle);

				// let the manage know we fired a round
				TUT_MGR->DoEvent(TUT_EVENT_WEAPON_FIRE, Weapon->WeaponType);

                // Show dust coming off of tanks when firing big guns.
                if(Weapon->WeaponType->bShockDust)
                {
                    if(FireVehicle != nullptr)
                    {
                        vehVehicle* lpVeh = (vehVehicle*)FireVehicle;

                        vehfxCreateShockSmoke(lpVeh->iFXVehicleID);
                    }
                }
			}

			if ((Weapon->NumRoundsAmmoLeft[AmmoGroup] == -1) && FiredWeapon && (Flags & WEAP_AMMOFLAG_FPS_WEAPON))
			{
				fpFPSFireRound(Weapon, FireAtt);
			}

			if (LastFireTime != nullptr)
			{
				*LastFireTime = Weapon->LastFireTime;
			}

			if (FireDelta != 0.0)
			{
				RecoilForce->x += TempRecoilForce.x / FireDelta;
				RecoilForce->y += TempRecoilForce.y / FireDelta;
				RecoilForce->z += TempRecoilForce.z / FireDelta;
			}
			else
			{
				WriteDebug ("FireDelta = 0.0!");
			}

			// These variables were added because the variables are Covert,
			// which makes repeatedly referencing the values* expensive*
			// It would be a good idea to try and eliminate some of the above
			// direct references too.

			SINT16 roundsLeft = Weapon->NumRoundsAmmoLeft[AmmoGroup];
			UINT32 roundsInClip = Weapon->WeaponType->NumRoundsInClip;
			UINT16 totalRounds = Weapon->NumTotalRounds[AmmoGroup];
			UINT16 overSupplyRounds = Weapon->NumOverSupplyRounds[AmmoGroup];

			if(FiredWeapon == 1 && roundsLeft > 0)
			{

				if (FiredFromGhost == 1)
				{
					if (Weapon->WeaponType->GhostRateOfFire >= Weapon->WeaponType->RateOfFire &&
						Weapon->WeaponType->RateOfFire > 0)
					{
						SINT32 NumClipsOrig = 0;
						SINT32 NumClipsEnd;

						if (roundsInClip > 0)
						{
							NumClipsOrig = (totalRounds - roundsLeft) / roundsInClip;
						}

						roundsLeft -=
							(UINT32)(Weapon->WeaponType->GhostRateOfFire / Weapon->WeaponType->RateOfFire + 0.5);

						if (roundsLeft < 0)
						{
							roundsLeft = 0;
						}

						if (roundsInClip > 0)
						{
							NumClipsEnd = (totalRounds - roundsLeft) / roundsInClip;

							if (NumClipsEnd < NumClipsOrig)
							{
								roundsLeft = totalRounds - NumClipsOrig * roundsInClip;
							}
						}
					}
					else
					{
						--roundsLeft;
					}
				}
				else if(!SuperTroop)
				{
					--roundsLeft;
				}
                // AI guns should reload after rounds empty...otherwise they reload after one shot if supertroop
                else if(SuperTroop && (Flags & WEAP_AMMOFLAG_IS_AUTOGUNROUND))
                {
                    --roundsLeft;
                }

				// handle oversupplied weapons
				if(overSupplyRounds > 0 && totalRounds > 0 && roundsLeft < totalRounds)
				{
					UINT16 roundsWanted = totalRounds - roundsLeft;

					roundsWanted = std::min(roundsWanted, (UINT16)overSupplyRounds);

					if(roundsWanted < overSupplyRounds)
						overSupplyRounds -= roundsWanted;
					else
						overSupplyRounds = 0;

					Weapon->NumOverSupplyRounds[AmmoGroup] = overSupplyRounds;

					roundsLeft += roundsWanted;
				}

				// Its covert: we've been trying to avoid reading/write to it
				Weapon->NumRoundsAmmoLeft[AmmoGroup] = roundsLeft;

				if (Flags & WEAP_AMMOFLAG_FPS_WEAPON)
				{
					fpFPSFireRound(Weapon, FireAtt, FireVehicle);
				}

				if (roundsInClip > 0)
				{
					if (Flags & WEAP_AMMOFLAG_FPS_WEAPON)
					{
						if (fpFPSReloadCheck (Weapon, AmmoGroup))
						{
							reloading = 1;
							fpFPSReload (Weapon);
						}
						else
							reloading = 0;
					}
					else if (roundsLeft > 0 && (totalRounds - roundsLeft) % roundsInClip == 0)
					{
						reloading = 1;
					}
					else
					{
						reloading = 0;
					}

					if (reloading)
					{
						// we're reloading the gun
						Weapon->ReloadStartTime = CurrentTime;

						if (Weapon->NextAmmoToUse != 0)
						{
							Weapon->AmmoTypeInUse = Weapon->NextAmmoToUse - 1;

							Weapon->NextAmmoToUse = 0;
						}
					}
				}
			}

			if(FiredWeapon && FiredFromGhost && FireVehicle && (Flags & WEAP_AMMOFLAG_TPS_WEAPON))
				fpTPSFireRound(FireVehicle);

			// advance lastfiretime
			Weapon->LastFireTime += FireInterval;
			if(FireVehicle) ((vehVehicle*)FireVehicle)->LastFireTime = frametime;

			// guarantee single-shot ghost gunfire always fires
			if (FiredFromGhost == TRUE &&
				Weapon->WeaponType->FireType == WEAP_FIRETYPE_SINGLESHOT &&
				(Flags & WEAP_AMMOFLAG_IS_AUTOGUNROUND) == 0)
			{
				Weapon->LastFireTime = 0;
			}

			// add 10% variance
			Weapon->LastFireTime += (0.1 * FireInterval) * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
		}
		else
		{
			break;
		}

		if (Weapon->WeaponType->FireType == WEAP_FIRETYPE_SINGLESHOT)
		{
			break;
		}
	}

	if(FiredWeapon)
		combattime = frametime;

	return FiredWeapon;
}

LOGICAL weapIsWeaponReloading(weapWeapon * Weapon, FLOAT64 CurrentTime)
{
	if (Weapon == nullptr)
	{
		return FALSE;
	}

	if (Weapon->ReloadStartTime == 0)
	{
		return FALSE;
	}

	if (CurrentTime - Weapon->ReloadStartTime < Weapon->WeaponType->ReloadTime)
	{
		return TRUE;
	}


	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: weapResupplyWeapon
// Description: resupplies a weapon's ammo
//
// Weapon - is the weapon to be resupplied
// percentOfTotal - if 0, then clip size is used, otherwise its the percent (0 to 1) of the start ammo load to add
// allowOversupply - allows the weapon to be oversupplied, limited by the MaxOverSupply variable set in data (defaults to 0)
// while any weapon can be oversupplied and reloading will take from oversupply for any weapon, currently only infantry have
// a mechanism for dealing with oversupply properly, so vehicles probably shouldn't use it for now
//////////////////////////////////////////////////////////////////////

UINT32 weapResupplyWeapon(weapWeapon* Weapon, FLOAT32 percentOfTotal, bool allowOversupply)
{
	if(Weapon == nullptr || Weapon->WeaponType == nullptr)
		return FALSE;

	UINT32 numResupplied	= 0;
	UINT32 resuppliedRounds = 0;
	UINT32 resupplyRounds	= 0;
	UINT32 maxOversupply	= 0;

	for(int i = 0; i < 4; i++)
	{
		// NumTotalRounds is a Covert variable, so repeated direct access is expensive.
		const UINT16 totalRounds	= Weapon->NumTotalRounds[i];
		const SINT16 roundsLeft		= Weapon->NumRoundsAmmoLeft[i];

		// continue if this pool never had rounds or if rounds left is set to infinite
		if(!totalRounds || roundsLeft < 0)
			continue;

		resuppliedRounds = 0;
		resupplyRounds = (percentOfTotal > 0) ? (UINT32)(totalRounds * percentOfTotal):
														  std::max(Weapon->WeaponType->NumRoundsInClip, 1U);

		maxOversupply = totalRounds * Weapon->WeaponType->MaxOverSupply;

		// if we have some rounds to supply and need some in the general bullet pool, add em
		if(resupplyRounds && roundsLeft < totalRounds)
		{
			resuppliedRounds = std::min(totalRounds - (UINT32)roundsLeft, resupplyRounds);
			Weapon->NumRoundsAmmoLeft[i] += resuppliedRounds;
			resupplyRounds -= resuppliedRounds;
		}

		// if oversupply is allowed, and we have some rounds left, add them
		if(allowOversupply && maxOversupply && resupplyRounds > 0 && Weapon->NumOverSupplyRounds[i] < maxOversupply)
		{
			Weapon->NumOverSupplyRounds[i] += resupplyRounds;
			resuppliedRounds += resupplyRounds;

			if(Weapon->NumOverSupplyRounds[i] > maxOversupply)
			{
				resuppliedRounds -= (Weapon->NumOverSupplyRounds[i] - maxOversupply);
				Weapon->NumOverSupplyRounds[i] = maxOversupply;
			}
		}

		// if we resupplied any rounds, increment the counter
		if(resuppliedRounds)
			++numResupplied;
	}

	return numResupplied;
}

void weapAddFireCallback(WEAP_FIRE_CALLBACK callback)
{
	_fireCallbacks.push_back(callback);
}
