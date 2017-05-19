#include <syscfg.h> // Path is project specific.
#include <string>


#include "ml/mltypes.h"
#include "ml/mlgauges.h"
#include "ml/mlAtmo.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "veh/vehmain.h"
#include "math/mathUnitConversion.h"
#include "pers/persmain.h"
#include "trn/trnmain.h"
#include "trn/trntod.h"
#include "draw/draw2d.h"
#include "veh/vehIcon.h"
#include "locl/loclLang.h"
#include "draw/drawIcon.h"
#include "pers/pershud.h"
#include "ml/mlEngine.h"
#include "ui/uiChat.h"
#include "ui/uiMain.h"
#include "trn/trnvars.h"

#include "libxml/tree.h"

#include "phui/View.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"

/************************************************************************************************
	mlReadout
*************************************************************************************************/

enum
{
	_READOUT_NORMAL = 0,
	_READOUT_RANGE,
	_READOUT_BOMBSIGHT_ENGLISH,
	_READOUT_BOMBSIGHT_MKS,
	_READOUT_TORPEDO_STATION
};

static SINT32 	_ReadoutType;
static FLOAT32	_ReadoutRange;
static FLOAT32	_ReadoutAlt;
static WCHAR 	_ReadoutTAS[64];
static WCHAR 	_ReadoutIAS[64];
static FLOAT32	_ReadoutRelBearing, _ReadoutAngleOff, _ReadoutSpeed, _ReadoutTorpedoDepth, _ReadoutTravelTime;
static FLOAT32	_ReadoutLeadAngle, _ReadoutFireAngle;
extern drawIconRef _RankIcons;

#ifdef BETA_RELEASE
extern UINT32 _HighlightHitmapObjectNum;
extern FLOAT64 _StickXPos;
extern FLOAT64 _StickYPos;
extern FLOAT64 _ThrottlePos;
extern FLOAT64 _RudderPos;
#endif

class ReadoutView : public persHUDTextView
{
public:
	ReadoutView();

	virtual void DrawSelf();

	void DisplayReadout();
	void DisplayGroundReadout();
	void DisplayAircraftReadout();
	void DisplayEyeStats();
	void DisplayDeployed();
	void DisplayRange();
	void DisplayBombsight();
	void DisplayTorpedo();
	void DisplayRiding();
	void DisplayRidee(vehVehicle* PlayerVehicle);
	void DisplayVehiclesRiding(vehVehicle* PlayerVehicle, UINT32 RideVehicleHostIndex);
	void DisplayInfantryRiding(vehVehicle* PlayerVehicle, UINT32 RideVehicleHostIndex);
	void DisplayVehicleObjectAnalysis();

	void DrawIcons(vehVehicle* v);
	void DrawRankIcon(SINT32);
protected:
	mlVehicleData* _VehicleData;

	// caches whether the current vehicle is geared or not
	bool _isMyVehicleGeared;
};


phui::View* CreateReadoutView(_xmlNode* xmlnode, phui::View* parent);


/************************************************************************************************
	Constructor
*************************************************************************************************/

ReadoutView::ReadoutView() : persHUDTextView(), _VehicleData(persGetPersonaModelDataPtr()), _isMyVehicleGeared(false)
{
	// if we found the data, find the gearing
	if(_VehicleData && _VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_GROUND)
	{
		if(mlGetNumGears(_VehicleData) > 0)
			_isMyVehicleGeared = true;
	}

	//setup cached strings
	strings["hud.readout.visible"]				= L"";
	strings["hud.readout.invisible"]			= L"";
	strings["hud.readout.position"]				= L"";
	strings["hud.readout.gforce"]				= L"";
	strings["hud.readout.aoa"]					= L"";
	strings["hud.readout.slip"]					= L"";
	strings["hud.readout.gear"]					= L"";
	strings["hud.readout.gear.reverse"]			= L"";
	strings["hud.readout.gear.neutral"]			= L"";
	strings["hud.readout.gear.engine"]			= L"";
	strings["hud.readout.gear.engines"]			= L"";
	strings["hud.readout.gear.kaput"]			= L"";
	strings["hud.readout.riding.ship"]			= L"";
	strings["hud.readout.riding.hoisted"]		= L"";
	strings["hud.readout.riding.lifeboat"]		= L"";
	strings["hud.readout.riding.stowed"]		= L"";
	strings["hud.readout.riding.towed"]			= L"";
	strings["hud.readout.riding.seated"]		= L"";
	strings["hud.readout.vehicles"]				= L"";
	strings["hud.readout.enemy"]				= L"";
	strings["hud.readout.vehicles.onhoist"]		= L"";
	strings["hud.readout.vehicles.towed"]		= L"";
	strings["hud.readout.vehicles.stowed"]		= L"";
	strings["hud.readout.infantry"]				= L"";
	strings["hud.readout.infantry.ship"]		= L"";
	strings["hud.readout.infantry.lifeboat"]	= L"";
	strings["hud.readout.infantry.seat"]		= L"";
	strings["hud.readout.range"]				= L"";
	strings["hud.readout.agl"]					= L"";
	strings["hud.readout.torp.bearing"]			= L"";
	strings["hud.readout.torp.aob"]				= L"";
	strings["hud.readout.torp.speed"]			= L"";
	strings["hud.readout.torp.range"]			= L"";
	strings["hud.readout.torp.depth"]			= L"";
	strings["hud.readout.torp.control"]			= L"";
	strings["hud.readout.torp.control.angle"]	= L"";
	strings["hud.readout.torp.control.time"]	= L"";
	strings["hud.readout.torp.control.invalid"]	= L"";
	strings["loc.mspawn.500"]					= L"";
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void ReadoutView::DrawSelf()
{
	if(VehicleObjectAnalysis)
		SetBackgroundColor(0,0,0,0);

	persHUDTextView::DrawSelf();

 	_VehicleData = persGetPersonaModelDataPtr();
 	if(!_VehicleData) return;

	ResetTextPos();
	DisplayReadout();
	ResizeToText();
}

/************************************************************************************************
	DisplayReadout
*************************************************************************************************/

void ReadoutView::DisplayReadout()
{
#ifdef BETA_RELEASE
	DisplayVehicleObjectAnalysis();
#endif

	if(_VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_FLOATER)
	{
		DisplayEyeStats();
		return;
	}

	switch (_ReadoutType)
	{
		case _READOUT_NORMAL:
			if(_VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_AIR)
				DisplayAircraftReadout();
			else if (_VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_GROUND)
				DisplayGroundReadout();

			DisplayDeployed();
			DisplayRiding();
		break;

		case _READOUT_RANGE:
			DisplayRange();
		break;

		case _READOUT_BOMBSIGHT_ENGLISH:
		case _READOUT_BOMBSIGHT_MKS:
			DisplayBombsight();
		break;

		case _READOUT_TORPEDO_STATION:
			DisplayTorpedo();
		break;
	}
}

/************************************************************************************************
	DisplayGroundReadout
*************************************************************************************************/

void ReadoutView::DisplayGroundReadout()
{
	if(!_isMyVehicleGeared) return;

	phui::GraphicsContext* ctx = GetGraphicsContext();

	bool enginesStarting = ((_VehicleData->MasterStarterOn || _VehicleData->StartersOn) && _VehicleData->ShutdownEngines == 0);
	bool enginesCaught = true;
	bool liveEngine = false;
	UINT32 numEngines = 0;

	FLOAT32 TempNum;
	mlGetGaugeData(&TempNum,ML_DOF_ELEMENT_GEARSHIFT,0);
	bool enginesReversed = (_VehicleData->ReverseToggle || TempNum < 0);

	FLOAT32 LGS;
	mlGetGaugeData(&LGS, ML_DOF_CONTROL_LINEARGEARSHIFT, 0);

	mlComponentListIterator it;
	mlComponentListBegin(_VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		if ( it.comp->Type != ML_COMPONENT_ENGINE ) continue;
		const mlEngineComponentData* Engine = (const mlEngineComponentData*)(it.comp->SpecialData);
		if ( Engine->SubType == ML_ENGINE_SUBTYPE_GEARBOX )
		{
            // Compile warning
			//if ( Engine->InReverse == true )
            if(Engine->InReverse)
				enginesReversed = true;
			continue;
		}
		if ( Engine->SubType != ML_ENGINE_SUBTYPE_ENGINE ) continue;
		++numEngines;
		const mlEngineComponentData* TorqueReceiver = (const mlEngineComponentData*)Engine->PropellorComponent->SpecialData;
		if ( it.comp->CurrentDamage < it.comp->DamageLevels[2] )
		{
			liveEngine = true;
			if ( Engine->EngineCaught != 1 || TorqueReceiver->CurrentRpm <= Engine->EngineData->CriticalRpm )
				enginesCaught = false;
			if ( Engine->InReverse )
				enginesReversed = true;
		}
	}

	if ( 0 == numEngines ) return;

	if ( false == liveEngine )
	{
		// Case 1: Engines are dead
		ctx->SetColor(1, 0, 0);				// Red
		const char* labelStr = (1 == numEngines) ? "hud.readout.gear.engine" : "hud.readout.gear.engines";
		DrawString(L"%ls: %ls\n", strings[labelStr].c_str(), strings["hud.readout.gear.kaput"].c_str());
		return;
	}

	wchar_t gearNum[16];
	const wchar_t* gear;

	bool inNeutral = false;

	if ( TempNum < 0.0f )
		gear = strings["hud.readout.gear.reverse"].c_str();
	else if ( TempNum < 0.05f )
	{
		inNeutral = true;
		gear = strings["hud.readout.gear.neutral"].c_str();
	}
	else
	{
		swprintf(gearNum, sizeof(gearNum) - 1, L"%d", (SINT32)(TempNum + 0.5f));
		gear = gearNum;
	}

	if ( enginesStarting == false && enginesCaught == false )
	{
		// Case 2: Engines are off
		if ( enginesReversed )
			ctx->SetColor(0.7f, 0.5f, 0.3f);		// Orangey
		else
			ctx->SetColor(0.6f, 0.6f, 0.6f);
	}
	else if ( (enginesCaught == false || inNeutral) && (frametime & 512) < 256 )
	{
		if ( enginesReversed )
			ctx->SetColor(0.9f, 0.6f, 0);
		else
			ctx->SetColor(0.6f, 0.6f, 0.6f);
	}
	else if ( inNeutral )
	{
		ctx->SetColor(1.0f, 1.0f, 0);
	}
	else
	{
		if ( enginesReversed )
			ctx->SetColor(1.0f, 0.8f, 0);
		else
			ctx->SetColor(0.3f, 1.0f, 0.3f);
	}

	DrawString(L"%ls: %ls\n", strings["hud.readout.gear"].c_str(), gear);
}

/************************************************************************************************
	DisplayAircraftReadout
*************************************************************************************************/

void ReadoutView::DisplayAircraftReadout()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();

	FLOAT32 VehicleG,AoA,SideSlip;
	FLOAT32 Speed;
	FLOAT64VECTOR TempVec;

	mlGetGaugeData(&SideSlip,ML_DOF_GAUGE_SLIPDEG,0);
	if (SideSlip < -180.0f)
		SideSlip += 360.0f;
	else if (SideSlip > 180.0f)
		SideSlip -= 360.0f;

	TempVec = _VehicleData->VehicleRealVel;
	mathVectorMultiply64(&TempVec,&TempVec,&_VehicleData->VehicleAttInvMat);
	AoA = (FLOAT32)(RAD_TO_DEG(atan2(TempVec.y,TempVec.z)) - 90.0);
	if (AoA < -180.0f)
	{
		AoA += 360.0f;
	}
	if (AoA > 180.0f)
	{
		AoA -= 360.0f;
	}

	VehicleG = _VehicleData->VehicleG;

	Speed = (FLOAT32)(mathExactVectorLength(&_VehicleData->VehicleVel));

	float ail	= (float)_VehicleData->AileronTabPosition * 100;
	float elev	= (float)_VehicleData->ElevatorTabPosition * 100;
	float rud	= (float)_VehicleData->RudderTabPosition * 100;
	const wchar_t* gearstate	= nullptr;
	const wchar_t* flapstate	= nullptr;

	// Localized Strings for use in displaying trim settings
	const wchar_t* left		= uiLocString("hud.readout.left").c_str();
	const wchar_t* right	= uiLocString("hud.readout.right").c_str();
	const wchar_t* up		= uiLocString("hud.readout.up").c_str();
	const wchar_t* down		= uiLocString("hud.readout.down").c_str();
	const wchar_t* sTrim	= uiLocString("hud.readout.trim").c_str();
	const wchar_t* sAil		= uiLocString("hud.readout.ail").c_str();
	const wchar_t* sElev	= uiLocString("hud.readout.elev").c_str();
	const wchar_t* sRud		= uiLocString("hud.readout.rud").c_str();
	// Localized Strings for use in displaying flaps and gear settings
	const wchar_t* sGear	= uiLocString("hud.readout.gear").c_str();
	const wchar_t* sFlaps	= uiLocString("hud.readout.flaps").c_str();
	const wchar_t* jammed	= uiLocString("hud.readout.jammed").c_str();

	if(_VehicleData->GearSetting == 0.0)
		gearstate = up;
	else if(_VehicleData->GearSetting == 1.0)
		gearstate = down;
	else if(_VehicleData->GearJammed)
		gearstate = jammed;
	else
		gearstate = L"?";

	// If flaps are jammed, we don't care if they're up or down:
	if(_VehicleData->FlapsJammed)
		flapstate = jammed;
	else if(_VehicleData->FlapSetting == 0.0)
		flapstate = up;
	else if(_VehicleData->FlapSetting == 1.0)
		flapstate = down;
	// The following case is for players who are using an incremental flap adjustment edit:
	else if((_VehicleData->FlapSetting >= -1.0) && (_VehicleData->FlapSetting <= 1.0))
	{
		int fsi = (int)((_VehicleData->FlapSetting) * 100);
        char fss[16];
        strprintf( fss, "%d", fsi );
		flapstate = UTF16(fss);
	}
	// Somehow we have a flap setting that doesn't make sense:
	else
		flapstate = L"?";


	ctx->SetColor(1,1,0);
	DrawString(L"%ls: %6.1f    ", strings["hud.readout.gforce"].c_str(), VehicleG);

	if (Speed > 20.0f)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls: %5.1f%c    ", strings["hud.readout.aoa"].c_str(), AoA, 0x00B0);
	}
	else
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls: 0.0%c    ", strings["hud.readout.aoa"].c_str(), 0x00B0);
	}

	if (Speed > 20.0f)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls: %5.1f%c  \n", strings["hud.readout.slip"].c_str(), SideSlip, 0x00B0);
	}
	else
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls: 0.0%c  \n", strings["hud.readout.slip"].c_str(), 0x00B0);
	}

	// Display Trim tab settings as percentage (-100% to +100%)
	ctx->SetColor(1,1,0);
	DrawString(L"%ls:   ", sTrim);
	// Aileron
	if(ail > 0)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls %3.1f%% %ls;   ", sAil, fabs(ail), right);
	}
	else if(ail < 0)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls %3.1f%% %ls;   ", sAil, fabs(ail), left);
	}
	else
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls 0.0%%;        ", sAil);
	}
	// Rudder
	if(rud > 0)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls %3.1f%% %ls;   ", sRud, fabs(rud), right);
	}
	else if(rud < 0)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls %3.1f%% %ls;   ", sRud, fabs(rud), left);
	}
	else
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls 0.0%%;        ", sRud);
	}
	// Elevator
	if(elev > 0)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls %3.1f%% %ls;   ", sElev, fabs(elev), up);
	}
	else if(elev < 0)
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls %3.1f%% %ls;   ", sElev, fabs(elev), down);
	}
	else
	{
		ctx->SetColor(1,1,0);
		DrawString(L"%ls 0.0%%;        ", sElev);
	}

	DrawString(L"\n");
	// Flaps and Gear settings
	ctx->SetColor(1,1,0);
	DrawString(L"%ls: %ls      %ls: %ls", sGear, gearstate, sFlaps, flapstate);
}

/************************************************************************************************
	DisplayDeployed
*************************************************************************************************/

void ReadoutView::DisplayDeployed()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(0,1,0);

    if(!vehmainIsDeployed() && !vehmainIsDeploying())
        return;

    if(!_VehicleData->BombBayDoor)
        return;

    DrawString(L"%ls\n", strings["loc.mspawn.500"].c_str());
}

/************************************************************************************************
	DisplayRange
*************************************************************************************************/

void ReadoutView::DisplayRange()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(1,1,0);

	DrawString(L"%ls: %1.0f\n", strings["hud.readout.range"].c_str(), _ReadoutRange);
}

/************************************************************************************************
	_DisplayBombsight
*************************************************************************************************/

void ReadoutView::DisplayBombsight()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(1,1,0);

	DrawString(L"%ls: %.0f%s, %ls, %ls\n", strings["hud.readout.agl"].c_str(), _ReadoutAlt, (_ReadoutType==_READOUT_BOMBSIGHT_ENGLISH)? L"ft": L"m", _ReadoutTAS,_ReadoutIAS);
}

/************************************************************************************************
	_DisplayTorpedo
*************************************************************************************************/

void ReadoutView::DisplayTorpedo()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(1,1,0);

	DrawString(L"%ls: %1.0f %ls: %1.0f %ls: %1.0f kts %ls: %1.0f m %ls: %1.0f m\n",
							strings["hud.readout.torp.bearing"].c_str(), _ReadoutRelBearing,
							strings["hud.readout.torp.aob"].c_str(), _ReadoutAngleOff,
							strings["hud.readout.torp.speed"].c_str(), _ReadoutSpeed,
							strings["hud.readout.torp.range"].c_str(), _ReadoutRange,
							strings["hud.readout.torp.depth"].c_str(), _ReadoutTorpedoDepth);

	if(_ReadoutTravelTime > 0)
	{
		DrawString(L"%ls: %1.0f %ls: %1.0f %ls: %1.0f s\n",
					strings["hud.readout.torp.control"].c_str(), _ReadoutFireAngle,
					strings["hud.readout.torp.control.angle"].c_str(), _ReadoutLeadAngle,
					strings["hud.readout.torp.control.time"].c_str(), _ReadoutTravelTime);
	}
	else if(_ReadoutTravelTime < 0)
	{
		DrawString(L"%ls\n", strings["hud.readout.torp.control.invalid"].c_str());
	}
}

/************************************************************************************************
	_DisplayRiding
*************************************************************************************************/

void ReadoutView::DisplayRiding()
{
	vehVehicle* PlayerVehicle = _VehicleData->VehicleMasterPtr;
	if(PlayerVehicle->VehicleObjectData == nullptr) return;

	// display yourself
	GetGraphicsContext()->SetColor(0,1,0);
	DrawIcons(PlayerVehicle);
	DrawString(L"%ls   \n", chatGetWideCallsign(PlayerVehicle->Chat_Handle));

	// who I'm riding on
	if(PlayerVehicle->RideVehicleHostIndex)
		DisplayRidee(PlayerVehicle);

	// display players riding on you
	DisplayVehiclesRiding(PlayerVehicle, PlayerVehicle->VehicleHostIndex);
	DisplayInfantryRiding(PlayerVehicle, PlayerVehicle->VehicleHostIndex);

	// display infantry riding with you
	if(PlayerVehicle->RideVehicleHostIndex > 0)
	{
		DisplayVehiclesRiding(PlayerVehicle, PlayerVehicle->RideVehicleHostIndex);
		DisplayInfantryRiding(PlayerVehicle, PlayerVehicle->RideVehicleHostIndex);
	}
}

/************************************************************************************************
	_DisplayRidee
*************************************************************************************************/

void ReadoutView::DisplayRidee(vehVehicle* PlayerVehicle)
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	vehVehicle* CurrentVehicle;
	UINT32 Rank;
	UCHAR* Handle;

	CurrentVehicle = vehFindVehicle(PlayerVehicle->RideVehicleHostIndex);
	if(CurrentVehicle)
	{
		ctx->SetColor(1,1,0);

		if(CurrentVehicle->VehicleType->Type == VEH_TYPE_SEA)
		{
			if(PlayerVehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
			{
				DrawString(L"%ls\n", strings["hud.readout.riding.ship"].c_str());
			}
			else if(PlayerVehicle->RideSeatNum == VEH_HOIST_RIDE_SEAT)
			{
				DrawString(L"%ls\n", strings["hud.readout.riding.hoisted"].c_str());
			}
			else
			{
				if(PlayerVehicle->VehicleType->Type == VEH_TYPE_PERSON)
				{
					DrawString(L"%ls %d\n", strings["hud.readout.riding.lifeboat"].c_str(), PlayerVehicle->RideSeatNum+1);
				}
				else
				{
					DrawString(L"%ls %d\n", strings["hud.readout.riding.stowed"].c_str(), PlayerVehicle->RideSeatNum+1);
				}
			}
		}
		else
		{
			if(PlayerVehicle->RideSeatNum == VEH_TOWED_VEHICLE)
			{
				DrawString(L"%ls\n", strings["hud.readout.riding.towed"].c_str(), PlayerVehicle->RideSeatNum+1);
			}
			else
			{
				DrawString(L"%ls %d\n", strings["hud.readout.riding.seated"].c_str(), PlayerVehicle->RideSeatNum+1);
			}
		}

		if(CurrentVehicle->Side == PlayerVehicle->Side)
		{
			Handle = CurrentVehicle->Handle;
			Rank = CurrentVehicle->Rank;
		}else
		{
			Handle = CurrentVehicle->VehicleType->Icon;
			Rank = 25; // !
			ctx->SetColor(1,0,0);
		}

		DrawIcons(CurrentVehicle);
		DrawString(L"  %ls\n", uiUTF8toUTF16(Handle));
	}
}

/************************************************************************************************
	DisplayVehiclesRiding
*************************************************************************************************/

void ReadoutView::DisplayVehiclesRiding(vehVehicle* PlayerVehicle, UINT32 RideVehicleHostIndex)
{
	if(!Online)
		return;

	phui::GraphicsContext* ctx = GetGraphicsContext();
	vehVehicle* CurrentVehicle;
	vehVehicle* _VehicleHead;
	const WCHAR* Handle;
	UINT32 Rank;
	LOGICAL firstone = TRUE;
	LOGICAL firstEnemy = TRUE;

	vehGetVehicleHead(&_VehicleHead);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		LOGICAL display = TRUE;

		if(CurrentVehicle == PlayerVehicle) continue;
		if(CurrentVehicle->VehicleType->Type != VEH_TYPE_GROUND) continue;

		if(RideVehicleHostIndex == CurrentVehicle->RideVehicleHostIndex)
		{
			if(firstone)
			{
				ctx->SetColor(0,0,1);
				DrawString(L"\n", strings["hud.readout.vehicles"].c_str());
				firstone = FALSE;
			}

			if(CurrentVehicle->Side == PlayerVehicle->Side)
			{
				ctx->SetColor(0,0,1);
				Handle = uiUTF8toUTF16(CurrentVehicle->Handle);
				Rank = CurrentVehicle->Rank;
			}
			else
			{
				ctx->SetColor(1,0,0);
				Handle = strings["hud.readout.enemy"].c_str();
				Rank = 25; // !

				if(!firstEnemy)
					display = FALSE;

				firstEnemy = FALSE;
			}

			if(display)
			{
				DrawIcons(CurrentVehicle);

				if(CurrentVehicle->Side != PlayerVehicle->Side)
					DrawString(L"  %ls\n", Handle);
				else if(CurrentVehicle->RideSeatNum == VEH_HOIST_RIDE_SEAT)
					DrawString(L"  %ls (%ls)\n", Handle, strings["hud.readout.vehicles.onhoist"].c_str());
				else if(CurrentVehicle->RideSeatNum == VEH_TOWED_VEHICLE)
					DrawString(L"  %ls (%ls)\n", Handle, strings["hud.readout.vehicles.towed"].c_str());
				else
					DrawString(L"  %ls (%ls %d)\n", Handle, strings["hud.readout.vehicles.stowed"].c_str(), CurrentVehicle->RideSeatNum+1);
			}
		}
	}
}

/************************************************************************************************
	DisplayInfantryRiding
*************************************************************************************************/

void ReadoutView::DisplayInfantryRiding(vehVehicle* PlayerVehicle, UINT32 RideVehicleHostIndex)
{
	if(!Online)
		return;

	phui::GraphicsContext* ctx = GetGraphicsContext();
	vehVehicle* CurrentVehicle;
	vehVehicle* _VehicleHead;
	const WCHAR* Handle;
	UINT32 Rank;
	LOGICAL firstone = TRUE;
	LOGICAL firstEnemy = TRUE;

	vehGetVehicleHead(&_VehicleHead);

	for (CurrentVehicle = _VehicleHead; CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		LOGICAL display = TRUE;

		if(CurrentVehicle == PlayerVehicle) continue;
		if(CurrentVehicle->VehicleType->Type != VEH_TYPE_PERSON) continue;

		if(RideVehicleHostIndex == CurrentVehicle->RideVehicleHostIndex)
		{
			if(firstone)
			{
				ctx->SetColor(0,1,0);
				DrawString(L"%ls\n", strings["hud.readout.infantry"].c_str());
				firstone = FALSE;
			}

			if(CurrentVehicle->Side == PlayerVehicle->Side)
			{
				ctx->SetColor(0,1,0);
				Handle = uiUTF8toUTF16(CurrentVehicle->Handle);
				Rank = CurrentVehicle->Rank;
			}
			else
			{
				ctx->SetColor(1,0,0);
				Handle = strings["hud.readout.enemy"].c_str();
				Rank = 25; // !

				if(!firstEnemy)
					display = FALSE;

				firstEnemy = FALSE;
			}

			if(display)
			{
				DrawIcons(CurrentVehicle);

				if(CurrentVehicle->Side != PlayerVehicle->Side)
				{
					DrawString(L"  %ls\n", Handle);
				}
				else if(CurrentVehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
				{
					DrawString(L"  %ls (%ls)\n", Handle, strings["hud.readout.infantry.ship"].c_str());
				}
				else
				{
					if(PlayerVehicle->VehicleType->Type == VEH_TYPE_SEA)
						DrawString(L"  %ls (%ls %d)\n", Handle, strings["hud.readout.infantry.lifeboat"].c_str(), CurrentVehicle->RideSeatNum+1);
					else
						DrawString(L"  %ls (%ls %d)\n", Handle, strings["hud.readout.infantry.seat"].c_str(), CurrentVehicle->RideSeatNum+1);
				}
			}
		}
	}
}

void ReadoutView::DrawIcons(vehVehicle* v)
{
	if(v->IsMissionLeader)
		DrawRankIcon(39);
	else if(v->IsHC())
		DrawRankIcon(36);
	else if(v->IsTrainer)
		DrawRankIcon(38);
	else if(v->IsBuilder)
		DrawRankIcon(37);
	else
		DrawRankIcon(-1);

	DrawRankIcon(v->Rank);
}

/************************************************************************************************
	DrawRankIcon
*************************************************************************************************/

void ReadoutView::DrawRankIcon(SINT32 Rank)
{
	phui::GraphicsContext* ctx = GetGraphicsContext();

	if(Rank >= 0)
	{
		int texid = drawIconGetTexId(_RankIcons);
		piSetActiveTexture(-1,0);
		ctx->EnableTexture();
		ctx->SetTexture(texid);

		float u1,v1,u2,v2;
		drawIconGetUV(_RankIcons, Rank-1, &u1, &v1, &u2, &v2);

		phui::BoxVerts box;
		float uv[8] = {u1,v1, u2,v1, u2,v2, u1,v2};

		phui::box_verts_with_pos(box, 0, 0, _lineheight, _lineheight);

		ctx->DrawQuad(box.verts, uv);
	}

	float move = _lineheight * 1.25f;

	ctx->MovePenPosition(move, 0);
	_textposx += move;
}

/************************************************************************************************
	mlSetNormalReadout
*************************************************************************************************/

SINT32 mlSetNormalReadout()
{
	_ReadoutType = _READOUT_NORMAL;
	return 0;
}

/************************************************************************************************
	mlSetRangeReadout
*************************************************************************************************/

SINT32 mlSetRangeReadout(FLOAT32 Range)
{
	_ReadoutType = _READOUT_RANGE;
	_ReadoutRange = Range;
	return 0;

}

/************************************************************************************************
	mlSetBombsightReadoutEnglish
*************************************************************************************************/

SINT32 mlSetBombsightReadoutEnglish(FLOAT32 Alt, FLOAT32 TAS)
{
	FLOAT64 Density1,Density2;
	FLOAT32 IAS;
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	_ReadoutType = _READOUT_BOMBSIGHT_ENGLISH;
	_ReadoutAlt = Alt * 3.281f + 0.5f;
	swprintf(_ReadoutTAS, 63, L"TAS: %1.0fmph", TAS * 2.237f + 0.5f);

	if (VehicleData != nullptr)
	{
		mlGetAltDensity(&Density1,0);
		mlGetAltDensity(&Density2,VehicleData->VehiclePos.z);
		IAS = (FLOAT32)(TAS * sqrt(fabs(Density2 / Density1)));
		swprintf(_ReadoutIAS, 63, L"IAS: %1.0fmph",IAS * 2.237f + 0.5f);
	}

	return 0;
}

/************************************************************************************************
	mlSetBombsightReadoutMKS
*************************************************************************************************/

SINT32 mlSetBombsightReadoutMKS(FLOAT32 Alt, FLOAT32 TAS)
{
	FLOAT64 Density1,Density2;
	FLOAT32 IAS;
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	_ReadoutType = _READOUT_BOMBSIGHT_MKS;
	_ReadoutAlt = Alt;
	swprintf(_ReadoutTAS, 63, L"TAS: %1.0fkph", TAS * 3.6f);

	if (VehicleData != nullptr)
	{
		mlGetAltDensity(&Density1,0);
		mlGetAltDensity(&Density2,VehicleData->VehiclePos.z);
		IAS = (FLOAT32)(TAS * sqrt(fabs(Density2 / Density1)));
		swprintf(_ReadoutIAS, 63, L"IAS: %1.0fkph",IAS * 3.6f + 0.5f);
	}

	return 0;
}

/************************************************************************************************
	mlSetTorpedoReadout
*************************************************************************************************/

LOGICAL mlSetTorpedoReadout(FLOAT32 relBearing, FLOAT32 angleOff, FLOAT32 range, FLOAT32 speed, FLOAT32 depth,
							FLOAT32 fireAngle, FLOAT32 leadAngle, FLOAT32 travelTime, LOGICAL ConvertToEnglishUnits)
{
	_ReadoutType 		= _READOUT_TORPEDO_STATION;
	_ReadoutRelBearing 	= RAD_TO_DEG(relBearing);
	_ReadoutAngleOff 	= RAD_TO_DEG(angleOff);
	_ReadoutRange 		= (ConvertToEnglishUnits) ? METER_TO_YD(range) : range;
	_ReadoutSpeed 		= MPS_TO_KTS(speed);  // this will always be converted to knots no matter what
	_ReadoutTorpedoDepth= (ConvertToEnglishUnits) ? METER_TO_FT(depth) : depth;
	_ReadoutLeadAngle 	= RAD_TO_DEG(leadAngle);
	_ReadoutTravelTime 	= travelTime;
	_ReadoutFireAngle 	= RAD_TO_DEG(fireAngle);

	return TRUE;
}

/************************************************************************************************
	CreateReadoutView
*************************************************************************************************/

phui::View* CreateReadoutView(_xmlNode* xmlnode, phui::View* parent)
{
	return new ReadoutView;
}

/************************************************************************************************
	_DisplayEyeStats
*************************************************************************************************/

void ReadoutView::DisplayEyeStats()
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if(!VehicleData) return;

// Icon visiblity
	int visible = VehicleData->VehicleMasterPtr->StateAnimation.GearSetting;
	DrawString(L"%ls\n", (visible) ? strings["hud.readout.visible"].c_str(): strings["hud.readout.invisible"].c_str());

// world position
	DrawString(L"%ls: %.0lf, %.0lf, %.0lf\n", strings["hud.readout.position"].c_str(), VehicleData->VehiclePos.x, VehicleData->VehiclePos.y, VehicleData->VehiclePos.z);
	DrawString(L"%ls: %.0lf, %.0lf, %.0lf\n", strings["hud.readout.position"].c_str(),
               VehicleData->VehiclePos.x - WorldOriginOffset.x,
               VehicleData->VehiclePos.y - WorldOriginOffset.y,
              -VehicleData->VehiclePos.z);
}

/************************************************************************************************
	DisplayVehicleObjectAnalysis
*************************************************************************************************/

void ReadoutView::DisplayVehicleObjectAnalysis()
{
	if(!VehicleObjectAnalysis || !_VehicleData) return;

	phui::GraphicsContext* ctx = GetGraphicsContext();
	FLOAT32 BrainOxy,ATPLevel,BloodPressure,BloodLevel;
	FLOAT32 TempNum, TempNum1 = 0.0f;
	FLOAT64VECTOR TempVec;

	ctx->SetColor(1,0,0);

	mlGetGaugeData(&TempNum,ML_DOF_GAUGE_ASIKPH,0);
	mlGetGaugeData(&TempNum1,ML_DOF_GAUGE_ASIMPH,0);
	DrawString("Speed: %fkph IAS, %fmph IAS\n",TempNum,TempNum1);

	mlGetGaugeData(&TempNum,ML_DOF_GAUGE_ALT1M,0);
	DrawString("Altitude: %fmeters\n",TempNum);

	mlGetGaugeData(&TempNum,ML_DOF_GAUGE_MANPRESSATM,0);
	DrawString("Eng Manifold: %fata\n",TempNum);

	mlGetGaugeData(&TempNum,ML_DOF_GAUGE_RPM,0);
	DrawString("RPM: %lf",TempNum);

	mlGetGaugeData(&TempNum,ML_DOF_GAUGE_PROPRPMSETTING,0);
	DrawString("RPM setting: %lf\n",TempNum);

	mlGetGaugeData(&TempNum,ML_DOF_ELEMENT_PROPPITCH,0);
	DrawString("Prop Angle: %f\n",TempNum);

	mlGetGaugeData(&TempNum,ML_DOF_ELEMENT_GEARSHIFT,0);
	DrawString(L"%ls: %f\n", strings["hud.readout.gear"].c_str(), TempNum);

	mlGetGaugeData(&TempNum,ML_DOF_GAUGE_MAGCOMPASS,0);
	if (TempNum < 0) TempNum += 360.0f;
	if(_VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_PERSON)	 TempNum += 180.0f;
	if (TempNum > 360.0f) TempNum -= 360.0f;

	DrawString("Compass: %d\n",(SINT32)(TempNum + 0.5f));

	// aoa
	TempVec = _VehicleData->VehicleRealVel;
	mathVectorMultiply64(&TempVec,&TempVec,&_VehicleData->VehicleAttInvMat);
	TempNum = (FLOAT32)(RAD_TO_DEG(atan2(TempVec.y,TempVec.z)) - 90.0);
	DrawString(L"AoA: %f°\n",TempNum);

	// g
	TempVec = _VehicleData->VehicleAcc;
	TempVec.x = -TempVec.x;
	TempVec.z = -TempVec.z;
	TempVec.z += 9.8;
	mathVectorMultiply64(&TempVec,&TempVec,&_VehicleData->VehicleAttInvMat);
	TempNum = (FLOAT32)(TempVec.z / 9.8);
	DrawString("G's: %f gs\n",TempNum);

	persGetHealthStats(&BrainOxy,&ATPLevel,&BloodPressure,&BloodLevel);

	DrawString("Brain Oxygen: %f\n",BrainOxy);
	DrawString("ATP Level: %f\n",ATPLevel);
	DrawString("Blood Pressure: %f\n",BloodPressure);
	DrawString("Blood Level: %f\n",BloodLevel);


#ifdef BETA_RELEASE
	if (_VehicleData->VehicleMasterPtr->VehicleType != nullptr && _HighlightHitmapObjectNum != 0)
	{
		DrawString("Highlighted Hitmap Obj: %s\n",
					_VehicleData->VehicleMasterPtr->VehicleType->DamageObjects[_HighlightHitmapObjectNum-1]);
		DrawString("Obj Thickness: %f\n",
				_VehicleData->VehicleMasterPtr->VehicleType->DamageArmorThickness[_HighlightHitmapObjectNum-1]);
	}

	DrawString("Elev Pos: %f\n",_StickYPos);
	DrawString("Rudder Pos: %f\n",_RudderPos);
	DrawString("Aileron Pos: %f\n",_StickXPos);
#endif

	DrawString("Elev Trim: %f\n",_VehicleData->ElevatorTabPosition);
	DrawString("Rudder Trim: %f\n",_VehicleData->RudderTabPosition);
	DrawString("Aileron Trim: %f\n",_VehicleData->AileronTabPosition);
	DrawString("CG Position: %lf %lf %lf\n",
		_VehicleData->VehiclePos.x, _VehicleData->VehiclePos.y,_VehicleData->VehiclePos.z);

	DrawString("Model Position: %lf %lf %lf\n",
		_VehicleData->AdjustedForCGPos.x, _VehicleData->AdjustedForCGPos.y,_VehicleData->AdjustedForCGPos.z);
}

