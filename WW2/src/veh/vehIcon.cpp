#include "syscfg.h" // Path is project specific.
#include "include/vars.h"
#include "gl/glplat.h"

#include "veh/vehIcon.h"
#include "draw/drawIcon.h"
#include "pers/persmain.h"
#include "pers/pershud.h"
#include "math/mathvect.h"
#include "trn/trnmain.h"
#include "math/math3d.h"
#include "ld/ldpix.h"
#include "rn/rnvis.h"
#include "rn/rntext.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "cfg/cfgPrefs.h"
#include "ui/uiMain.h"
#include "pi/piogl.h"
#include "math/Matrixf.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglMaterial.h"
#include "trn/trnvars.h"
#include "gr2/gr2Anim.h"
#include "scen/ScenarioManager.h"
#include "ogl/States.h"
#include "fp/fpFPS.h"
#include "fx/TransformEffect.h"
#include "ob/obdraw.h"

/************************************************************************************************
	vehIcon
*************************************************************************************************/

#define MAX_TRIS_FOR_ICONS 256
#define ICON_FONT 0

enum{ ICON_MODE_NORMAL, ICON_MODE_VEHICLE_TYPE, ICON_MODE_NONE, ICON_MODE_MAX };

#define MaxHandleVisDist	1500.0f		// maximum distance to see handle
#define MaxIdVisDist		1500.0f 	// maximum distance to see vehicle id
#define MaxRangeCircleDist	6000.0f		// maximum range on range circle

#define IconFadeStartDist 	100.0f 		// icon starts to fade at this distance
#define IconFadeEndDist 	4000.0f 	// icon fully faded at this distance
#define IconFadeUpTime 		10000.0f 	// time for icon to fade in when in/out of view
#define IconFadeDownTime 	5000.0f		// time for icon to fade out when in/out of view

#define IconRankDistance	100.0f		// distance we start displaying rank
#define IconColorDistance 	1500.0f		// maximum distance to see colored icon
#define IconSquadDistance 	500.0f		// maximum distance to see squad icon
#define IconFadePercentageToSeeNameAndColor	0.1f	// must be at least 10% transparent to have icon

#define RAINBOW_COLORS_MAX	6

static SINT32 _IconRangeCircleTexId;
drawIconRef _DeathIcon;
drawIconRef _RankIcons;
drawIconRef _ColorRankIcons;
static SINT32 _IconMode;
static const int pulsePeriod = 1000;

static ref_ptr<oglMaterial> iconmaterial;

static void _DrawVehicleIcon(vehVehicle* CurrentVehicle);
static void _DrawVehicleIconDeprecated(vehVehicle* CurrentVehicle);
static void _DrawIconString(const piVECTOR* Pos, FLOAT32 size, FLOAT32 vOffset,
						    const wchar_t* str, UINT32 Color);
static void _DrawIconRank(const piVECTOR* Pos, FLOAT32 size, FLOAT32 vOffset,
							UINT32 Rank, const WCHAR* strNextTo, UINT32 color, vehVehicle*);
static void _DrawIconRankDeprecated(const piVECTOR* Pos, FLOAT32 size, FLOAT32 vOffset,
							        UINT32 Rank, const WCHAR* strNextTo, UINT32 color, vehVehicle*);
static void _DrawIcon(const FLOAT64VECTOR& worldPos, UINT32 color, drawIconRef& icons, FLOAT32 scale = 1.0f, SINT32 index = 1);
static void _DrawIconRangeCircle(const piVECTOR* Pos, FLOAT32 size, FLOAT32 IconRingSize, UINT32 Color);

static void _CalcIconPositionAndSize(vehVehicle*, piVECTOR* Pos, FLOAT32* size);
static void _TransformToPlayerView(UCHAR const* verts, SINT32 numverts, SINT32 stride, piVECTOR const* pos,
									FLOAT32 dx, FLOAT32 dy, FLOAT32 scale);

static UINT32 _CycleIconMode(SINT32 param, UINT32 functionCode, UINT32 unused);
static void   _HandleIconMode(vehVehicle* CurrentVehicle, LOGICAL& DrawRangeCircle, LOGICAL& DrawIconHandle, LOGICAL& DrawIconId, LOGICAL& DrawIconRank, LOGICAL& DrawIconSquad);

/************************************************************************************************
	vehIconInit
*************************************************************************************************/

SINT32 vehIconInit()
{
	_IconRangeCircleTexId = ldpixLoadImageFileToSurface("iconring1.bmp", colorBLACK, LDPIX_NONE);
	_RankIcons = drawIconLoad("gamerank");
	_DeathIcon = drawIconLoad("\\ui\\uidata\\icons\\misc\\icon_death");
	_ColorRankIcons = drawIconLoad("\\ui\\uidata\\chat\\commrank");

	imapInstallNonRemovableControlCallback(IMAP_CYCLE_ICON_MODE, _CycleIconMode);

    if(!ogl::UseDeprecatedPath())
    {
        iconmaterial = new oglMaterial;
        iconmaterial->AddTexture(0, rntextGetTexId(ICON_FONT));
        iconmaterial->SetMaterial(-1);
        iconmaterial->SetShader("icon");
    }

	return 0;
}

/************************************************************************************************
	vehIconKill
*************************************************************************************************/

SINT32 vehIconKill()
{
	if (piIsTexture(_IconRangeCircleTexId, 0))
		piFreeMipTextures(_IconRangeCircleTexId);

	drawIconKill(_DeathIcon);
	drawIconKill(_RankIcons);
	drawIconKill(_ColorRankIcons);

	return 0;
}

/************************************************************************************************
	vehIconCanSeeHandle
*************************************************************************************************/

LOGICAL vehIconCanSeeHandle(struct vehVehicle* Vehicle)
{
#ifdef BETA_RELEASE
#ifndef DONTSHOWNME
	if(Online && !(Vehicle->Flags & VEH_FLAGS_IS_CLONE) && !SCEN_MGR->IsActive())
		return TRUE;
#endif
#endif

	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	// floaters can see each other
	if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER || PlayerVehicle->VehicleType->Type == VEH_TYPE_FLOATER)
		return TRUE;
	else if(!Online && (Vehicle->Flags & VEH_FLAGS_IS_CLONE))
		return TRUE;
	else if(Vehicle->Side == PlayerVehicle->Side)
		return TRUE;

	return FALSE;
}

/************************************************************************************************
	vehIconColor
*************************************************************************************************/

UINT32 vehIconColor(vehVehicle* Vehicle)
{
	UINT32 IconColor = 0;
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	//check for null player vehicle
	if (PlayerVehicle == nullptr)
		return colorALPHA;

	if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER)
	{
		IconColor = colorYELLOW;
	}else if (Vehicle->Side != PlayerVehicle->Side)
	{
		IconColor = gIconEnemyColor;
	}else
	{
		if(Vehicle->IsMyMissionLeader)
			IconColor = gIconMissionLeaderColor;
		else if (Vehicle->VehicleIsSquaddie)
			IconColor = gIconSquadColor;
		else if(Vehicle->SameMission)
			IconColor = gIconMissionColor;
		else
			IconColor = gIconFriendlyColor;

		if(Vehicle->Rank == 1)
			IconColor = colorGREENALPHA;
	}

	return IconColor  | colorALPHA;
}

/************************************************************************************************
	vehIconColor3f
*************************************************************************************************/

void vehIconColor3f(vehVehicle* Vehicle, float* r, float* g, float* b)
{
	UINT32 IconColor = vehIconColor(Vehicle);
	*r = piRGBA_GETRED(IconColor)/(float)0xff;
	*g = piRGBA_GETGREEN(IconColor)/(float)0xff;
	*b = piRGBA_GETBLUE(IconColor)/(float)0xff;
}

/************************************************************************************************
	vehIconUpdate
*************************************************************************************************/

SINT32 vehIconUpdate(vehVehicle* Vehicle)
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	if(!PlayerVehicle) return 0;

	FLOAT64VECTOR WorldOriginOffset, DeltaVec;
	FLOAT32 Dist, Percent;
	piVECTOR TempVec;
	FLOAT32 TempFloat;


	if(!Vehicle || !Vehicle->VehicleType) return -1;

	mathSubVectors(&DeltaVec, &Vehicle->PosData.RenderPos,&PlayerVehicle->PosData.RenderPos);
	Dist = (FLOAT32)mathExactVectorLength(&DeltaVec);

// Handle and Rank
	if(vehIconCanSeeHandle(Vehicle))
	{
		if(Dist < MaxHandleVisDist && Vehicle->IconTransparency > IconFadePercentageToSeeNameAndColor)
			Vehicle->DrawIconHandle = TRUE;
		else if(Dist >= MaxHandleVisDist || Vehicle->IconTransparency <= IconFadePercentageToSeeNameAndColor)
			Vehicle->DrawIconHandle = FALSE;

		Vehicle->DrawIconRank = (Dist < IconRankDistance) ? TRUE : FALSE;
		Vehicle->DrawIconSquad = (Dist < IconSquadDistance) ? TRUE : FALSE;
	}

// Floater icon visibility
	if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER && PlayerVehicle->VehicleType->Type != VEH_TYPE_FLOATER)
	{
		// Use gear setting (both observer vehicles have a DOF for it so we piggyback on top of it for vis)
		Vehicle->DrawIconHandle	= Vehicle->StateAnimation.GearSetting;
		Vehicle->DrawIconRank	= FALSE;
		Vehicle->DrawIconSquad	= FALSE;
		Vehicle->DrawIconId 	= FALSE;
	}

// ID for Aircraft
	if(Vehicle->VehicleType->Type == VEH_TYPE_AIR)
	{
		if(Dist < MaxIdVisDist && Vehicle->IconTransparency > IconFadePercentageToSeeNameAndColor)
		{
			if(Vehicle->Side == PlayerVehicle->Side)
			{
				Vehicle->DrawIconHandle = TRUE; // Draw handle instead of id
			}else
			{
				Vehicle->DrawIconId = TRUE;
			}
		}else if(Dist >= MaxIdVisDist || Vehicle->IconTransparency <= IconFadePercentageToSeeNameAndColor)
		{
			Vehicle->DrawIconId = FALSE;
			Vehicle->DrawIconHandle = FALSE;
		}
	}

// range circle
	if (Vehicle->VehicleType->Type == VEH_TYPE_AIR)
	{
		if (Dist > MaxRangeCircleDist)
			Percent = 1.0f;
		else
			Percent = (FLOAT32)(sqrt(Dist) / sqrt(MaxRangeCircleDist));

		Vehicle->IconRingSize = Percent;
		Vehicle->DrawRangeCircle = TRUE;
	}

// Icon transparency and color
	if (IconFadeStartDist >= IconFadeEndDist)
	{
		Vehicle->IconTransparency = 1.0f;
	}
	else
	{
		if (Dist > IconFadeEndDist)
			Vehicle->IconTransparency = 0.0f;
		else if (Dist < IconFadeStartDist)
			Vehicle->IconTransparency = 1.0f;
		else
			Vehicle->IconTransparency = 1.0f - (Dist - IconFadeStartDist) / (IconFadeEndDist - IconFadeStartDist);
	}

	// override to scenario logic if we're running a scenario
	SCEN_MGR->SetVehicleIcon(Vehicle);

	// get vector to vehicle from camera
	trnGetWorldOriginOffset(&WorldOriginOffset);
	TempVec.x = (FLOAT32)( Vehicle->PosData.RenderPos.x - WorldOriginOffset.x);
	TempVec.y = (FLOAT32)( Vehicle->PosData.RenderPos.y - WorldOriginOffset.y);
	TempVec.z = (FLOAT32)(-Vehicle->PosData.RenderPos.z - WorldOriginOffset.z);
	mathAddVectors(&TempVec,&TempVec,&playpos);
	math3dNormalize(&TempVec);

	TempFloat = mathVectDotProduct(&TempVec,&playdir);

    TempFloat = TempFloat > 1.0f ? 1.0f : TempFloat < -1.0f ? -1.0f : TempFloat;

	TempFloat = (FLOAT32)acos(TempFloat);
	TempFloat = TempFloat * 180.0f / PI;
	if (TempFloat > FieldOfView * 0.5f * 1.4f)
	{
		TempFloat = (frametime - Vehicle->LastVisChangeTime) / IconFadeDownTime;
		Vehicle->LastVisTransparency -= TempFloat;
		if (Vehicle->LastVisTransparency < 0.0f)
			Vehicle->LastVisTransparency = 0.0f;
	}
	else
	{
		TempFloat = (frametime - Vehicle->LastVisChangeTime) / IconFadeUpTime;
		Vehicle->LastVisTransparency += TempFloat;
		if (Vehicle->LastVisTransparency > 1.0f)
			Vehicle->LastVisTransparency = 1.0f;
	}

	Vehicle->LastVisChangeTime = frametime;
	Vehicle->IconTransparency *= Vehicle->LastVisTransparency;

	if(PlayerVehicle->VehicleType->Type == VEH_TYPE_FLOATER)
		Vehicle->IconTransparency = 1.0f;

	if (Dist > IconColorDistance)
		Vehicle->IconColor = colorBLACK | colorALPHA;
	else
		Vehicle->IconColor = vehIconColor(Vehicle);

	// make sure we're not culled out
	OBJECT* o = Vehicle->VehicleObjectData->PlaneObject;

	if(o->cullcallback && o->cullcallback(o))
	{
		Vehicle->DrawIconHandle = FALSE;
		Vehicle->DrawRangeCircle = FALSE;
		Vehicle->DrawIconId = FALSE;
		Vehicle->DrawIconRank = FALSE;
		Vehicle->DrawIconSquad = FALSE;
	}

	return 0;
}

/************************************************************************************************
	vehIconDraw
*************************************************************************************************/

SINT32 vehIconDrawDeprecated()
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	if (!PlayerVehicle)	return 0;

	vehVehicle*CurrentVehicle, *_VehicleHead;
	FLOAT64VECTOR origin;
	piMATRIX worldMat;

	vehGetVehicleHead(&_VehicleHead);
	if(!_VehicleHead) return -1;

	trnGetWorldOriginOffset(&origin);
	origin.z = -origin.z;

	worldMat = *piGetWorldMatrix();

    // Ticket# 6019
    ProgramState::Bind(0);

	piSetWorldMatrix(&IdentityMatrix);
	piDisableMaterial();
	piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);
    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle != nullptr; CurrentVehicle = CurrentVehicle->getNext())
	{
		if(CurrentVehicle == PlayerVehicle || CurrentVehicle->MinVehicleLOD < 0 || CurrentVehicle->VehicleDeadTime)
			continue;

		_DrawVehicleIconDeprecated(CurrentVehicle);
	}

	// draw death icons if we have any
	if(HUD_MGR->IsVisible())
	{
		PersHUDManager::HUDObjectList& deaths = HUD_MGR->GetObjects();

		UINT32 r, g, b;
		r = g = b = 0;
		UINT32 color = 0;
		FLOAT64VECTOR pos;

		for(size_t i = 0; i < deaths.size(); ++i)
		{
			PersHUDManager::HUDObject* o = deaths[i];

			if(o->type != PersHUDManager::DEATH)
				continue;

			PersHUDManager::HUDDeath* death = dynamic_cast<PersHUDManager::HUDDeath*>(o);
			if(!death) continue;

			r = piRGBA_GETRED(death->color);
			g = piRGBA_GETGREEN(death->color);
			b = piRGBA_GETBLUE(death->color);

			FLOAT32 alpha = death->GetPlacement(pos);
			if(alpha <= 0) continue;

			// Static for debugging
			static float scale = 1.0f;

			color = UINT32(piRGBA_MAKE(r, g, b, SINT32(255 * alpha)));

			_DrawIcon(pos, color, _DeathIcon, scale);
		}
	}

  	piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
	piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
	piSetWorldMatrix(&worldMat);

	return 0;
}

void vehIconDraw()
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

    if(!PlayerVehicle)
        return;

    vehVehicle*CurrentVehicle, *_VehicleHead;
    //piMATRIX worldMat;

    vehGetVehicleHead(&_VehicleHead);

    if(!_VehicleHead)
        return;

    piSetWorldMatrix(&IdentityMatrix);

    piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);

    for(CurrentVehicle = _VehicleHead; CurrentVehicle != nullptr; CurrentVehicle = CurrentVehicle->getNext())
    {
	    if(CurrentVehicle == PlayerVehicle || CurrentVehicle->MinVehicleLOD < 0)
            continue;

        _DrawVehicleIcon(CurrentVehicle);
    }

	PersHUDManager::HUDObjectList& deaths = HUD_MGR->GetObjects();

	for(size_t i = 0; i < deaths.size(); ++i)
	{
		PersHUDManager::HUDObject* o = deaths[i];

		if(o->type != PersHUDManager::DEATH)
			continue;

		// Static for debugging
        static float scale = 2.5f;

		_DrawIcon(o->pos, colorREDALPHA, _DeathIcon, scale);
	}

    piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
}

/************************************************************************************************
	vehIconMode
*************************************************************************************************/

SINT32 vehIconMode()
{
	return _IconMode;
}

void vehIconSetPulsate(struct vehVehicle* vehicle, SINT32 duration)
{
	if(vehicle)
		vehicle->IconPulse = (duration) ? frametime + duration: 0;
}

/************************************************************************************************
	vehIconDraw2DRank
*************************************************************************************************/

SINT32 vehIconDraw2DRank(UINT32 Rank, int x, int y, LOGICAL colorIcon)
{
	drawIcon2d(colorIcon ? _ColorRankIcons : _RankIcons, Rank-1, (FLOAT32)x, (FLOAT32)y);
	return 0;
}

static void _DrawVehicleIcon(vehVehicle* CurrentVehicle)
{
    piVECTOR Pos;
    LOGICAL DrawRangeCircle, DrawIconHandle, DrawIconId, DrawIconRank, DrawIconSquad;
    FLOAT32 size;
    UINT32 Color;
    FLOAT32 vOffset;
    FLOAT32 minHeight;

    _CalcIconPositionAndSize(CurrentVehicle, &Pos, &size);

    if(!rnvisVectorFOVCheck(&Pos, 1.0f))
        return;

    vOffset = size;

    if(CurrentVehicle->VehicleType->Type == VEH_TYPE_SEA)
        minHeight = 0.0f;
    else if(CurrentVehicle->VehicleObjectData && CurrentVehicle->VehicleType->Type != VEH_TYPE_AIR)
        minHeight = (FLOAT32)CurrentVehicle->VehicleObjectData->HighestPoint + 0.3f;
    else
        minHeight = 2.0f;

	if(CurrentVehicle->VehicleType->Type == VEH_TYPE_AIR)
		vOffset *= 1.5f;

	if(vOffset < minHeight)
		vOffset = minHeight;

    DrawRangeCircle	= CurrentVehicle->DrawRangeCircle;
    DrawIconHandle = CurrentVehicle->DrawIconHandle;
    DrawIconId = CurrentVehicle->DrawIconId;
    DrawIconRank = CurrentVehicle->DrawIconRank;
    DrawIconSquad = CurrentVehicle->DrawIconSquad;

    _HandleIconMode(CurrentVehicle, DrawRangeCircle, DrawIconHandle, DrawIconId, DrawIconRank, DrawIconSquad);

    UINT32 alpha = (UINT32)(CurrentVehicle->IconTransparency * 255.0 + 0.5);

    if(frametime <= CurrentVehicle->IconPulse)
    {
	    if(((CurrentVehicle->IconPulse - frametime) % pulsePeriod) < pulsePeriod/2)
		    size *= 1.3f;
    }

	Color = piRGBA_SETALPHA(CurrentVehicle->IconColor, alpha);

	if(DrawRangeCircle)
		_DrawIconRangeCircle(&Pos, size, CurrentVehicle->IconRingSize, Color);

	if(DrawIconHandle && (_IconMode == ICON_MODE_NORMAL || _IconMode == ICON_MODE_VEHICLE_TYPE))
	{
		if(_IconMode == ICON_MODE_VEHICLE_TYPE && CurrentVehicle->VehicleType->Type != VEH_TYPE_FLOATER)
		{
			_DrawIconString(&Pos, size, vOffset, vehTypeLoc(4, CurrentVehicle->VehicleType), Color);
		}
		else
		{
#ifdef VEHICLES_GAME_INFO
			FLOAT32 squadSize = size * .8f;
			char hist[6];
			_DrawIconString(&Pos, squadSize, vOffset, UTF16(vehGetHistoryStringFor(CurrentVehicle->VehicleHostIndex, hist, sizeof(hist))), Color);
			vOffset += squadSize;
#else
			if(DrawIconSquad && *CurrentVehicle->SquadHandle)
			{
				FLOAT32 squadSize = size * .8f;
				_DrawIconString(&Pos, squadSize, vOffset, uiUTF8toUTF16(CurrentVehicle->SquadHandle), Color);
				vOffset += squadSize;
			}
#endif
			_DrawIconString(&Pos, size, vOffset, chatGetWideCallsign(CurrentVehicle->Chat_Handle), Color);

			if(DrawIconRank)
				_DrawIconRank(&Pos, size, vOffset, CurrentVehicle->Rank, chatGetWideCallsign(CurrentVehicle->Chat_Handle), Color, CurrentVehicle);
		}
	}

	if(DrawIconId)
		_DrawIconString(&Pos, size, -vOffset, uiUTF8toUTF16(CurrentVehicle->VehicleType->Icon), Color);
}

static vehVehicle* CV = nullptr;

static void _DrawVehicleIconDeprecated(vehVehicle* CurrentVehicle)
{
    piVECTOR Pos;
    LOGICAL DrawRangeCircle, DrawIconHandle, DrawIconId, DrawIconRank, DrawIconSquad;
    FLOAT32 size;
    UINT32 Color;
    FLOAT32 vOffset;
    FLOAT32 minHeight;

    CV = CurrentVehicle;

    _CalcIconPositionAndSize(CurrentVehicle, &Pos, &size);

    if(!rnvisVectorFOVCheck(&Pos, 1.0f))
        return;

    vOffset = size;

    if(CurrentVehicle->VehicleType->Type == VEH_TYPE_SEA)
        minHeight = 0.0f;
    else if(CurrentVehicle->VehicleObjectData && CurrentVehicle->VehicleType->Type != VEH_TYPE_AIR)
        minHeight = (FLOAT32)CurrentVehicle->VehicleObjectData->HighestPoint + 0.3f;
    else
        minHeight = 2.0f;

	if(CurrentVehicle->VehicleType->Type == VEH_TYPE_AIR)
		vOffset *= 1.5f;

	if(vOffset < minHeight)
		vOffset = minHeight;

    DrawRangeCircle	= CurrentVehicle->DrawRangeCircle;
    DrawIconHandle = CurrentVehicle->DrawIconHandle;
    DrawIconId = CurrentVehicle->DrawIconId;
    DrawIconRank = CurrentVehicle->DrawIconRank;
    DrawIconSquad = CurrentVehicle->DrawIconSquad;

    _HandleIconMode(CurrentVehicle, DrawRangeCircle, DrawIconHandle, DrawIconId, DrawIconRank, DrawIconSquad);

    UINT32 alpha = (UINT32)(CurrentVehicle->IconTransparency * 255.0 + 0.5);

    if(frametime <= CurrentVehicle->IconPulse)
    {
	    if(((CurrentVehicle->IconPulse - frametime) % pulsePeriod) < pulsePeriod/2)
		    size *= 1.3f;
    }

	Color = piRGBA_SETALPHA(CurrentVehicle->IconColor, alpha);

	if(DrawRangeCircle)
		_DrawIconRangeCircle(&Pos, size, CurrentVehicle->IconRingSize, Color);

	if(DrawIconHandle && (_IconMode == ICON_MODE_NORMAL || _IconMode == ICON_MODE_VEHICLE_TYPE))
	{
		vehVehicle* player = persGetPersonaVehiclePtr();

		bool showEnemyOnly = (player && player->Side != CurrentVehicle->Side && SCEN_MGR->HasFlag(ScenarioManager::ENEMY_ICON_TYPES_ONLY));

		if(showEnemyOnly && CurrentVehicle->VehicleType->Type != VEH_TYPE_FLOATER)
		{
			_DrawIconString(&Pos, size, vOffset, uiLocString("scenario.enemy").c_str(), Color);
		}
		else if(_IconMode == ICON_MODE_VEHICLE_TYPE && CurrentVehicle->VehicleType->Type != VEH_TYPE_FLOATER)
		{
			_DrawIconString(&Pos, size, vOffset, vehTypeLoc(4, CurrentVehicle->VehicleType), Color);
		}
		else
		{
#ifdef BETA_RELEASE
			if(!Online && !CurrentVehicle->IsMe() && (CurrentVehicle->Flags & VEH_FLAGS_IS_CLONE))
			{
				wchar_t buffer[10+1];

                SINT32 selectedLod = 0;
                SINT32 dist = 0;
				OBJECT* po = CurrentVehicle->VehicleObjectData->PlaneObject;

				if(CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON && po->animModel)
                {
                    selectedLod = po->animModel->model.GetLOD() + 1;
                    dist = static_cast<SINT32>(po->distsqr);
                }
                else
                {
                    OBJECT * rootChild = po;

					dist = static_cast<SINT32>(GetDistance(rootChild, false));

                    for (SINT32 i = 0; i < ML_MAX_LODS; i++)
                    {
                        if (CurrentVehicle->VehicleObjectData->LODObjects[i] &&
                            !(CurrentVehicle->VehicleObjectData->LODObjects[i]->flags & OBJECT_SKIPDRAWING))
                        {
                            selectedLod = i+1;
                            break;
                        }
                    }
                }
                
                swprintf(buffer, 10, L"%dm (%d)", dist, selectedLod); // make it 1 based for producer types
                
				FLOAT32 squadSize = size * .8f;
				_DrawIconString(&Pos, squadSize, vOffset, buffer, Color);
				vOffset += squadSize;
			}
			else
			{
#ifdef VEHICLES_GAME_INFO
				FLOAT32 squadSize = size * .8f;
				char hist[6];
				_DrawIconString(&Pos, squadSize, vOffset, UTF16(vehGetHistoryStringFor(CurrentVehicle->VehicleHostIndex, hist, sizeof(hist))), Color);
				vOffset += squadSize;
#else
				if(DrawIconSquad && *CurrentVehicle->SquadHandle)
				{
					FLOAT32 squadSize = size * .8f;
					_DrawIconString(&Pos, squadSize, vOffset, uiUTF8toUTF16(CurrentVehicle->SquadHandle), Color);
					vOffset += squadSize;
				}
#endif
			}
#else
			if(DrawIconSquad && *CurrentVehicle->SquadHandle)
			{
				FLOAT32 squadSize = size * .8f;
				_DrawIconString(&Pos, squadSize, vOffset, uiUTF8toUTF16(CurrentVehicle->SquadHandle), Color);
				vOffset += squadSize;
			}
#endif
			_DrawIconString(&Pos, size, vOffset, chatGetWideCallsign(CurrentVehicle->Chat_Handle), Color);

			if(DrawIconRank)
				_DrawIconRankDeprecated(&Pos, size, vOffset, CurrentVehicle->Rank, chatGetWideCallsign(CurrentVehicle->Chat_Handle), Color, CurrentVehicle);
		}
	}

	if(DrawIconId)
		_DrawIconString(&Pos, size, -vOffset, uiUTF8toUTF16(CurrentVehicle->VehicleType->Icon), Color);
}

//static piLVERTEX* _GetStaticIconVertices()
//{
//	static piLVERTEX m_Vertices[MAX_TRIS_FOR_ICONS * 3];
//	return m_Vertices;
//}

/************************************************************************************************
	_DrawIconString
*************************************************************************************************/

static void _DrawIconString(const piVECTOR* Pos,
                            FLOAT32 size,
                            FLOAT32 vOffset,
							const wchar_t* str,
                            UINT32 Color)
{
	SINT32 VertCnt;
	static piLVERTEX Vertices[MAX_TRIS_FOR_ICONS * 3];
	FLOAT32 Width, Height, Scale;

	VertCnt = wcslen(str) * 6;

    if(VertCnt < 6 || VertCnt >= MAX_TRIS_FOR_ICONS * 3)
        return;

	rntext3DString(str, Color, ICON_FONT, Vertices, VertCnt);

	Width = (FLOAT32)rntextGetStringWidth(str, ICON_FONT);
	Height= (FLOAT32)rntextGetHeight(ICON_FONT);
	Scale = 1.0f / Height * size; // arbitrary

	_TransformToPlayerView((UCHAR*)Vertices, VertCnt, sizeof(piLVERTEX), Pos,
							-Width / 2.0f, -Height / 2.0f + vOffset / Scale, Scale);

    if(ogl::UseDeprecatedPath())
    {
	    piSetActiveTexture(rntextGetTexId(ICON_FONT), 0);
 	    piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, &Vertices, VertCnt);
    }
    else
    {
        iconmaterial->AddTexture(0, rntextGetTexId(ICON_FONT));
        iconmaterial->Apply();
 	    piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, &Vertices, VertCnt);
    }
}

/************************************************************************************************
	_DrawIconRankDeprecated
*************************************************************************************************/

static void _DrawIconRankDeprecated(const piVECTOR* Pos, FLOAT32 size, FLOAT32 vOffset,
							        UINT32 Rank, const WCHAR* str, UINT32 Color, vehVehicle* CurrentVehicle)
{
	UINT32 pulse = 0;
	piVECTOR m_Vertices[4];
	FLOAT32 Width, Height, Scale;

	Width = (FLOAT32)rntextGetStringWidth(str, ICON_FONT);
	Height= (FLOAT32)rntextGetHeight(ICON_FONT);
	Scale = 1.0f / Height * size; // arbitrary

	m_Vertices[0].x = 0.0;
	m_Vertices[0].y = 0.0;
	m_Vertices[0].z = 0.0;

	m_Vertices[1].x = 0.0;
	m_Vertices[1].y = Height;
	m_Vertices[1].z = 0.0;

	m_Vertices[2].x = Height;
	m_Vertices[2].y = Height;
	m_Vertices[2].z = 0.0;

	m_Vertices[3].x = Height;
	m_Vertices[3].y = 0.0;
	m_Vertices[3].z = 0.0;

	_TransformToPlayerView((UCHAR*)m_Vertices, 4, sizeof(piVECTOR), Pos,
							-Width / 2.0f - Height, -Height / 2.0f + vOffset / Scale, Scale);

	if(frametime <= CurrentVehicle->IconPulse && ((CurrentVehicle->IconPulse - frametime) % pulsePeriod) < pulsePeriod/2)
		pulse = 1;

	if(pulse)
		Rank = VEHICON_PULSE+1;

	bool dead = false;

	// does this every frame..bleh
	vehVehicle* player = persGetPersonaVehiclePtr();

	if(player && player->Side == CurrentVehicle->Side)
	{
		dead = CurrentVehicle->isHulked();

		if(!dead && CurrentVehicle->VehicleObjectData &&
		   CurrentVehicle->VehicleObjectData->PlaneObject &&
		   CurrentVehicle->VehicleObjectData->PlaneObject->animModel &&
		   CurrentVehicle->VehicleObjectData->PlaneObject->animModel->deathState)
		{
			dead = true;
		}
	}

	if(dead)
		Rank = 28; // skull & crossbones

	drawIcon3d(_RankIcons, Rank-1, m_Vertices, Color);

	// extra icon on the right
	if(dead)
		Rank = 28; // skull & crossbones
	else if(pulse)
		Rank = VEHICON_PULSE+1;
	else if(CurrentVehicle->IsMyMissionLeader)
		Rank = VEHICON_MISSIONLEADER+1;
	else if(CurrentVehicle->IsCommander)
		Rank = VEHICON_HC+1;
	else if(CurrentVehicle->IsTrainer)
		Rank = VEHICON_TRAINER+1;
	else if(CurrentVehicle->IsBuilder)
		Rank = VEHICON_BUILDER+1;
	else
		Rank = 0;

	if(Rank > 0)
	{
		m_Vertices[0].x = 0.0;
		m_Vertices[0].y = 0.0;
		m_Vertices[0].z = 0.0;

		m_Vertices[1].x = 0.0;
		m_Vertices[1].y = Height;
		m_Vertices[1].z = 0.0;

		m_Vertices[2].x = Height;
		m_Vertices[2].y = Height;
		m_Vertices[2].z = 0.0;

		m_Vertices[3].x = Height;
		m_Vertices[3].y = 0.0;
		m_Vertices[3].z = 0.0;

		_TransformToPlayerView((UCHAR*)m_Vertices, 4, sizeof(piVECTOR), Pos,
								Width / 2.0f, -Height / 2.0f + vOffset / Scale, Scale);

        drawIcon3d(_RankIcons, Rank-1, m_Vertices, Color);
	}
}

static void _DrawIconRank(const piVECTOR* Pos, FLOAT32 size, FLOAT32 vOffset,
							UINT32 Rank, const WCHAR* str, UINT32 Color, vehVehicle* CurrentVehicle)
{
	UINT32 pulse = 0;
	piVECTOR m_Vertices[4];
	FLOAT32 Width, Height, Scale;

	Width = (FLOAT32)rntextGetStringWidth(str, ICON_FONT);
	Height= (FLOAT32)rntextGetHeight(ICON_FONT);
	Scale = 1.0f / Height * size; // arbitrary

	m_Vertices[0].x = 0.0;
	m_Vertices[0].y = 0.0;
	m_Vertices[0].z = 0.0;

	m_Vertices[1].x = 0.0;
	m_Vertices[1].y = Height;
	m_Vertices[1].z = 0.0;

	m_Vertices[2].x = Height;
	m_Vertices[2].y = Height;
	m_Vertices[2].z = 0.0;

	m_Vertices[3].x = Height;
	m_Vertices[3].y = 0.0;
	m_Vertices[3].z = 0.0;

	_TransformToPlayerView((UCHAR*)m_Vertices, 4, sizeof(piVECTOR), Pos,
							-Width / 2.0f - Height, -Height / 2.0f + vOffset / Scale, Scale);

	if(frametime <= CurrentVehicle->IconPulse && ((CurrentVehicle->IconPulse - frametime) % pulsePeriod) < pulsePeriod/2)
		pulse = 1;

	if(pulse)
		Rank = VEHICON_PULSE+1;

    iconmaterial->AddTexture(0, _RankIcons->texid);
    iconmaterial->Apply();

	bool dead = false;

	// does this every frame..bleh
	vehVehicle* player = persGetPersonaVehiclePtr();

	if(player && player->Side == CurrentVehicle->Side)
	{
		dead = CurrentVehicle->isHulked();

		if(!dead && CurrentVehicle->VehicleObjectData &&
		   CurrentVehicle->VehicleObjectData->PlaneObject &&
		   CurrentVehicle->VehicleObjectData->PlaneObject->animModel &&
		   CurrentVehicle->VehicleObjectData->PlaneObject->animModel->deathState)
		{
			dead = true;
		}
	}

	if(dead)
		Rank = 28; // skull & crossbones

	drawIcon3d(_RankIcons, Rank-1, m_Vertices, Color);

	// extra icon on the right
	if(dead)
		Rank = 28; // skull & crossbones
	else if(pulse)
		Rank = VEHICON_PULSE+1;
	else if(CurrentVehicle->IsMyMissionLeader)
		Rank = VEHICON_MISSIONLEADER+1;
	else if(CurrentVehicle->IsCommander)
		Rank = VEHICON_HC+1;
	else if(CurrentVehicle->IsTrainer)
		Rank = VEHICON_TRAINER+1;
	else if(CurrentVehicle->IsBuilder)
		Rank = VEHICON_BUILDER+1;
	else
		Rank = 0;

	if(Rank > 0)
	{
		m_Vertices[0].x = 0.0;
		m_Vertices[0].y = 0.0;
		m_Vertices[0].z = 0.0;

		m_Vertices[1].x = 0.0;
		m_Vertices[1].y = Height;
		m_Vertices[1].z = 0.0;

		m_Vertices[2].x = Height;
		m_Vertices[2].y = Height;
		m_Vertices[2].z = 0.0;

		m_Vertices[3].x = Height;
		m_Vertices[3].y = 0.0;
		m_Vertices[3].z = 0.0;

		_TransformToPlayerView((UCHAR*)m_Vertices, 4, sizeof(piVECTOR), Pos,
								Width / 2.0f, -Height / 2.0f + vOffset / Scale, Scale);

        drawIcon3d(_RankIcons, Rank-1, m_Vertices, Color);
	}
}

/************************************************************************************************
	_DrawIcon - arbitrary icon drawing
*************************************************************************************************/

static void _DrawIcon(const FLOAT64VECTOR& worldPos, UINT32 color, drawIconRef& icons, FLOAT32 scale, SINT32 index)
{
	piVECTOR pos;
	piVECTOR dir;
	FLOAT32 dist;

	pos.x = (FLOAT32)(worldPos.x - WorldOriginOffset.x);
	pos.y = (FLOAT32)(worldPos.y - WorldOriginOffset.y);
	pos.z = (FLOAT32)(-worldPos.z);

	dir.x = pos.x + playpos.x;
	dir.y = pos.y + playpos.y;
	dir.z = pos.z + playpos.z;

	dist = (FLOAT32)mathExactVectorLength(&dir);

	if(dist < 0.001f)
		return;

	// normalize unit vector
	dir.x /= dist;
	dir.y /= dist;
	dir.z /= dist;

	// move it away
	pos.x -= dir.x;// * 1.0f;
	pos.y -= dir.y;// * 1.0f;
	pos.z -= dir.z;// * 1.0f;

    if(!rnvisVectorFOVCheck(&pos, 1.0f))
        return;

	dist -= 1.0f;

	// Calculate size we should display icon
	// arbitrary scaling (size "1" at 30 m) * scale
	FLOAT32 size = (dist * (1.0f / 30.0f) * (FieldOfView / 90.0f));

	if(size < 0.25f)
		size = 0.25f;

	piVECTOR m_Vertices[4];
	FLOAT32 width;

    // Static for debugging
	static float height = 1.0f;
	width = height;
    scale = (1.0f / height * size) * scale; // arbitrary

	m_Vertices[0].x = 0.0f;
	m_Vertices[0].y = 0.0f;
	m_Vertices[0].z = 0.0f;

	m_Vertices[1].x = 0.0;
	m_Vertices[1].y = height;
	m_Vertices[1].z = 0.0f;

	m_Vertices[2].x = height;
	m_Vertices[2].y = height;
	m_Vertices[2].z = 0.0f;

	m_Vertices[3].x = height;
	m_Vertices[3].y = 0.0;
	m_Vertices[3].z = 0.0f;

	_TransformToPlayerView((UCHAR*)m_Vertices,
							4,
							sizeof(piVECTOR),
							&pos,
							-.5f, //-width / 2.0f - height,
							-height / 2.0f / scale,
							scale);

	if(!ogl::UseDeprecatedPath())
	{
		iconmaterial->AddTexture(0, icons->texid);
		iconmaterial->Apply();
	}

	drawIcon3d(icons, index-1, m_Vertices, color);
}

/************************************************************************************************
	_DrawIconRangeCircle
*************************************************************************************************/

static void _DrawIconRangeCircle(const piVECTOR* Pos, FLOAT32 size, FLOAT32 IconRingSize, UINT32 Color)
{
	piVECTOR vecCorner1,vecCorner2,vecCorner3,vecCorner4;
	FLOAT32 U, V, a, b, Ratio;
	SINT32 uVCount;
	static piLVERTEX m_Vertices[MAX_TRIS_FOR_ICONS * 3];
	uVCount = 0;

	//  corners, arbitrary 1.7
	vecCorner1.x = 1.7f;	vecCorner1.y = 0.0f;	vecCorner1.z = 0.0f;
	vecCorner2.x = 0.0f; 	vecCorner2.y = 1.7f; 	vecCorner2.z = 0.0f;
	vecCorner3.x = -1.7f; 	vecCorner3.y = 0.0f; 	vecCorner3.z = 0.0f;
	vecCorner4.x = 0.0f;	vecCorner4.y = -1.7f; 	vecCorner4.z = 0.0f;

	// each quadrant
	// circle starts at the bottom and expands clockwise
	if (IconRingSize >= 0.25f)
	{
		// 1st quadrant full
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner4.x;
		m_Vertices[uVCount].y = vecCorner4.y;
		m_Vertices[uVCount].z = vecCorner4.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.0f;
		m_Vertices[uVCount++].tv = 0.0f;

		m_Vertices[uVCount].x = vecCorner3.x;
		m_Vertices[uVCount].y = vecCorner3.y;
		m_Vertices[uVCount].z = vecCorner3.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 1.0f;
		m_Vertices[uVCount++].tv = 0.0f;
	}
	else
	{
		// partial 1st quadrant
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner4.x;
		m_Vertices[uVCount].y = vecCorner4.y;
		m_Vertices[uVCount].z = vecCorner4.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.0f;
		m_Vertices[uVCount++].tv = 0.0f;

		a = PI/2 * (IconRingSize - 0.0f) / 0.25f;
		b = PI - PI/4 - a;
		Ratio = (FLOAT32)(sin(a) / sin(b)) / 1.4142f;

		U = Ratio;
		V = 0.0f;

		m_Vertices[uVCount].x = vecCorner4.x + Ratio * (vecCorner3.x - vecCorner4.x);
		m_Vertices[uVCount].y = vecCorner4.y + Ratio * (vecCorner3.y - vecCorner4.y);
		m_Vertices[uVCount].z = vecCorner4.z + Ratio * (vecCorner3.z - vecCorner4.z);
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = U;
		m_Vertices[uVCount++].tv = V;
	}

	if (IconRingSize >= 0.5f)
	{
		// 2nd quadrant full
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner3.x;
		m_Vertices[uVCount].y = vecCorner3.y;
		m_Vertices[uVCount].z = vecCorner3.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 1.0f;
		m_Vertices[uVCount++].tv = 0.0f;

		m_Vertices[uVCount].x = vecCorner2.x;
		m_Vertices[uVCount].y = vecCorner2.y;
		m_Vertices[uVCount].z = vecCorner2.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 1.0f;
		m_Vertices[uVCount++].tv = 1.0f;
	}
	else if (IconRingSize >= 0.25f)
	{
		// 2nd quadrant partial
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner3.x;
		m_Vertices[uVCount].y = vecCorner3.y;
		m_Vertices[uVCount].z = vecCorner3.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 1.0f;
		m_Vertices[uVCount++].tv = 0.0f;

		a = PI/2 * (IconRingSize - 0.25f) / 0.25f;
		b = PI - PI/4 - a;
		Ratio = (FLOAT32)(sin(a) / sin(b)) / 1.4142f;

		U = 1.0f;
		V = Ratio;

		m_Vertices[uVCount].x = vecCorner3.x + Ratio * (vecCorner2.x - vecCorner3.x);
		m_Vertices[uVCount].y = vecCorner3.y + Ratio * (vecCorner2.y - vecCorner3.y);
		m_Vertices[uVCount].z = vecCorner3.z + Ratio * (vecCorner2.z - vecCorner3.z);
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = U;
		m_Vertices[uVCount++].tv = V;
	}

	if (IconRingSize >= 0.75f)
	{
		// 3rd quadrant full
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner2.x;
		m_Vertices[uVCount].y = vecCorner2.y;
		m_Vertices[uVCount].z = vecCorner2.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 1.0f;
		m_Vertices[uVCount++].tv = 1.0f;

		m_Vertices[uVCount].x = vecCorner1.x;
		m_Vertices[uVCount].y = vecCorner1.y;
		m_Vertices[uVCount].z = vecCorner1.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.0f;
		m_Vertices[uVCount++].tv = 1.0f;

	}
	else if (IconRingSize >= 0.5f)
	{
		// 3rd quadrant partial
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner2.x;
		m_Vertices[uVCount].y = vecCorner2.y;
		m_Vertices[uVCount].z = vecCorner2.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 1.0f;
		m_Vertices[uVCount++].tv = 1.0f;

		a = PI/2 * (IconRingSize - 0.5f) / 0.25f;
		b = PI - PI/4 - a;
		Ratio = (FLOAT32)(sin(a) / sin(b)) / 1.4142f;

		U = 1.0f - Ratio;
		V = 1.0f;

		m_Vertices[uVCount].x = vecCorner2.x + Ratio * (vecCorner1.x - vecCorner2.x);
		m_Vertices[uVCount].y = vecCorner2.y + Ratio * (vecCorner1.y - vecCorner2.y);
		m_Vertices[uVCount].z = vecCorner2.z + Ratio * (vecCorner1.z - vecCorner2.z);
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = U;
		m_Vertices[uVCount++].tv = V;
	}

	if (IconRingSize >= 1.0f)
	{
		// 4th quadrant full
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner1.x;
		m_Vertices[uVCount].y = vecCorner1.y;
		m_Vertices[uVCount].z = vecCorner1.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.0f;
		m_Vertices[uVCount++].tv = 1.0f;

		m_Vertices[uVCount].x = vecCorner4.x;
		m_Vertices[uVCount].y = vecCorner4.y;
		m_Vertices[uVCount].z = vecCorner4.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.0f;
		m_Vertices[uVCount++].tv = 0.0f;
	}
	else if (IconRingSize >= 0.75f)
	{
		// 4th quadrant partial
		m_Vertices[uVCount].x = m_Vertices[uVCount].y = m_Vertices[uVCount].z = 0.0f;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.5f;
		m_Vertices[uVCount++].tv = 0.5f;

		m_Vertices[uVCount].x = vecCorner1.x;
		m_Vertices[uVCount].y = vecCorner1.y;
		m_Vertices[uVCount].z = vecCorner1.z;
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = 0.0f;
		m_Vertices[uVCount++].tv = 1.0f;

		a = PI/2 * (IconRingSize - 0.75f) / 0.25f;
		b = PI - PI/4 - a;
		Ratio = (FLOAT32)(sin(a) / sin(b)) / 1.4142f;

		U = 0.0f;
		V = 1.0f - Ratio;

		m_Vertices[uVCount].x = vecCorner1.x + Ratio * (vecCorner4.x - vecCorner1.x);
		m_Vertices[uVCount].y = vecCorner1.y + Ratio * (vecCorner4.y - vecCorner1.y);
		m_Vertices[uVCount].z = vecCorner1.z + Ratio * (vecCorner4.z - vecCorner1.z);
		m_Vertices[uVCount].color = Color;
		m_Vertices[uVCount].specular = colorALPHA100;
		m_Vertices[uVCount].tu = U;
		m_Vertices[uVCount++].tv = V;
	}

	_TransformToPlayerView((UCHAR*)m_Vertices, uVCount, sizeof(piLVERTEX), Pos, 0, 0, size);

	if (uVCount > 0)
	{
        if(ogl::UseDeprecatedPath())
        {
		    piSetActiveTexture(_IconRangeCircleTexId, 0);
		    piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, &m_Vertices, uVCount);
        }
        else
        {
            iconmaterial->AddTexture(0, _IconRangeCircleTexId);
            iconmaterial->Apply();
		    piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, &m_Vertices, uVCount);
        }
	}
}

/************************************************************************************************
	_CalcIconPositionAndSize
*************************************************************************************************/
/*
static void _CalcIconPositionAndSize(vehVehicle* CurrentVehicle, piVECTOR* Pos, FLOAT32* size)
{
	float x, y, z, w, h;

	if(obmainGetScreenBBOX(CurrentVehicle->VehicleObjectData->PlaneObject, x, y, z, w, h))
	{
		Pos->x = x + w/2;
		Pos->y = y + h;
		Pos->z = z;

		FLOAT64VECTOR origin;
		piVECTOR dir;
		FLOAT32 Dist;
		piVECTOR wPos; // world pos

		// Calculate distance from player to vehicle
		trnGetWorldOriginOffset(&origin);
		origin.z = -origin.z;

		wPos.x =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - origin.x);
		wPos.y =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - origin.y);
		wPos.z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - origin.z);

		dir.x = wPos.x + playpos.x;
		dir.y = wPos.y + playpos.y;
		dir.z = wPos.z + playpos.z;

		Dist = (FLOAT32)mathExactVectorLength(&dir);
		*size = 1.0f;

		if(Dist < 0.001f) return;

		// normalize unit vector
		dir.x /= Dist; dir.y /= Dist; dir.z /= Dist;

		Dist -= 1.0f;

		// Calculate size we should display icon
		*size = Dist * (1.0f / 30.0f) * (FieldOfView / 90.0f); // arbitrary scaling (size "1" at 30 m)

		if(*size < 0.25f) *size = 0.25f;
	}
}
*/

static void _CalcIconPositionAndSize(vehVehicle* CurrentVehicle, piVECTOR* Pos, FLOAT32* size)
{
	FLOAT64VECTOR origin;
	piVECTOR dir;
	FLOAT32 Dist;
	FLOAT32 offset;

// Calculate distance from player to vehicle
	trnGetWorldOriginOffset(&origin);
	origin.z = -origin.z;

	Pos->x =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - origin.x);
	Pos->y =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - origin.y);
	Pos->z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - origin.z);

	if(CurrentVehicle->VehicleType->Type == VEH_TYPE_SEA && CurrentVehicle->VehicleObjectData)
		Pos->z += -CurrentVehicle->VehicleObjectData->HighestPoint; // z flip of course

	dir.x = Pos->x + playpos.x;
	dir.y = Pos->y + playpos.y;
	dir.z = Pos->z + playpos.z;

	Dist = (FLOAT32)mathExactVectorLength(&dir);
	*size = 1.0f;

	if(Dist < 0.001f) return;

// normalize unit vector
	dir.x /= Dist; dir.y /= Dist; dir.z /= Dist;

	if(CurrentVehicle->VehicleObjectData)
	{
		if(CurrentVehicle->VehicleType->Type != VEH_TYPE_SEA)
		{ // Move icon in front of vehicle in player's direction by radius of vehicle
			FLOAT32 PushDist = 30.0f;
			offset = CurrentVehicle->VehicleObjectData->Radius;

			if(CurrentVehicle->VehicleObjectData->HighestPoint > PushDist / 2.0)
				PushDist = (FLOAT32)(2 * CurrentVehicle->VehicleObjectData->HighestPoint);

			// if we're too close push closer to origin
			if(Dist < offset + PushDist)
				offset *= Dist / (offset + PushDist);

			Pos->x -= dir.x * offset;
			Pos->y -= dir.y * offset;
			Pos->z -= dir.z * offset;

			Dist -= offset;
		}
	}else
	{
		Pos->x -= dir.x * 1.0f;
		Pos->y -= dir.y * 1.0f;
		Pos->z -= dir.z * 1.0f;

		Dist -= 1.0f;
	}

	// Calculate size we should display icon
	*size = Dist * (1.0f / 30.0f) * (FieldOfView / 90.0f); // arbitrary scaling (size "1" at 30 m)

	if(*size < 0.25f) *size = 0.25f;
}

/************************************************************************************************
	_TransformToPlayerView
*************************************************************************************************/

static void _TransformToPlayerView(UCHAR const* verts, SINT32 numverts, SINT32 stride, piVECTOR const* pos,
										FLOAT32 dx, FLOAT32 dy, FLOAT32 scale)
{
	SINT32 i;
	piVECTOR* v;
	FLOAT32 x,y;

	for(i = 0; i < numverts; i++, verts += stride)
	{
		v = (piVECTOR*)verts;

		v->x += dx;
		v->y += dy;

		v->x *= scale;
		v->y *= scale;

		x = v->x;
		y = v->y;

		v->x = pos->x + x * playright.x + y * playup.x;
		v->y = pos->y + x * playright.y + y * playup.y;
		v->z = pos->z + x * playright.z + y * playup.z;
	}
}

/************************************************************************************************
	_CycleIconMode
*************************************************************************************************/

static UINT32 _CycleIconMode(SINT32 param, UINT32 functionCode, UINT32 unused)
{
	if(param == 1)
	{
		_IconMode++;
		if(_IconMode >= ICON_MODE_MAX) _IconMode = 0;
		cfgPrefsSetGlobal(cfgICON_MODE, &_IconMode, false);
	}

	return 0;
}

/************************************************************************************************
	_HandleIconMode
*************************************************************************************************/

static void
_HandleIconMode(vehVehicle* CurrentVehicle,
					LOGICAL& DrawRangeCircle,
					LOGICAL& DrawIconHandle,
					LOGICAL& DrawIconId,
					LOGICAL& DrawIconRank,
					LOGICAL& DrawIconSquad
					)
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	if(!PlayerVehicle) return;

	switch(_IconMode)
	{
		case ICON_MODE_VEHICLE_TYPE:
			DrawIconRank 	= FALSE;
			DrawIconSquad	= FALSE;
		break;

		case ICON_MODE_NONE:
			DrawIconHandle	= FALSE;
			DrawIconId 		= FALSE;
			DrawRangeCircle	= FALSE;
			DrawIconRank 	= FALSE;
			DrawIconSquad	= FALSE;
		break;
	}
}

void vehSetIconMode(SINT32 mode)
{
	_IconMode = mode;

	if(_IconMode < ICON_MODE_NORMAL || _IconMode >= ICON_MODE_MAX)
		_IconMode = ICON_MODE_NORMAL;
}