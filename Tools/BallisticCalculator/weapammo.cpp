#include "weapammo.h"

// Yes, it's a horrible hack to copy/paste this piecemeal instead
// of including the actual code. Unfortunately, the code is
// absolutely horrible, so good luck trying to isolate a small
// chunk of it for something like this.

#include <cstdint>
#include <cstring>
#include <exception>
#include <sstream>
#include <fstream>

#define PI				3.14159265359f
#define mlDEG_TO_RAD(a) ((a*0.017453293f))

//-------------------------------
UINT32 mlGetAltTemp(FLOAT64* Temp,FLOAT64 Alt)
{
	FLOAT64 ReturnTemp = 0;
	FLOAT64 Theta = 0;

	Theta = 0.751861;

	if (Alt < 11000)
	{
		Theta = 1.0 - Alt/44330;
	}

	ReturnTemp = 288.15 * Theta;

	*Temp = ReturnTemp;

	return 0;
}

UINT32 mlGetAltSpeedOfSound(FLOAT64* Speed,FLOAT64 Alt)
{
	FLOAT64 ReturnSpeedOfSound = 0;
	FLOAT64 Temp = 0;

	mlGetAltTemp(&Temp,Alt);

	ReturnSpeedOfSound = sqrt(1.4*287.06*Temp);

	*Speed = ReturnSpeedOfSound;

	return 0;
}

UINT32 mlInterpolate(FLOAT64* ReturnNum,FLOAT64 Num1,FLOAT64 Num2,FLOAT64 Delta)
{
	FLOAT64 DeltaNum;

	DeltaNum = Num2-Num1;

	*ReturnNum = Num1 + DeltaNum * Delta;

	return 0;
}

UINT32 mlGetAltDensity(FLOAT64* Density, FLOAT64 Alt)
{
	static FLOAT64 DensityTable[202];
	static int tableInited  = 0;

	FLOAT64 ReturnDensity = 0;
	SINT32 i, j;
	FLOAT64 Delta = 0;
	FLOAT64 TableAlt = 0;

	if(!tableInited)
	{
		for(i = 0; i <= 201; i++)
		{
			TableAlt = 100.0 * (FLOAT64)i;
			Delta = 0.05402977;

			if (TableAlt < 20000)
			{
				Delta = 0.2233422 * exp((11000-TableAlt)/6341.36);
			}
			if (TableAlt < 11000)
			{
				Delta = pow(1.0 - TableAlt/44328.6,5.255876);
			}

			DensityTable[i] = 1.22500 * Delta;
		}
		tableInited = 1;
	}

	if(Alt < 0) Alt = 0;
	if(Alt > 20000) Alt = 20000;

	i = (int)(Alt / 100.0);
	j = (int)(Alt / 100.0) + 1;
	Delta = (Alt - 100.0 * (FLOAT64)i)  / 100.0;

	ReturnDensity = (1 - Delta) * DensityTable[i] +  Delta * DensityTable[j];
	*Density = ReturnDensity;

	return 0;
}

//==============================

#define WEAP_AMMOFLAG_FIRSTFRAME				0x00000001

#define G1_CHART_MACH_DELTA 0.005
#define G1_CHART_NUM_DIVISIONS 1001

// Ammo variable types
enum {
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
	WEAP_AMMOMAXVAR
};

static const char _AmmoTypeVarNames[WEAP_AMMOMAXVAR][32] = {
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
	"ghostroundsto"
};

// Mach chart
FLOAT32 G1MachChart[G1_CHART_NUM_DIVISIONS];

const char* const mgc1_txt = 
"0.00	0.2629\n\
0.05	0.2558\n\
0.10	0.2487\n\
0.15	0.2413\n\
0.20	0.2344\n\
0.25	0.2278\n\
0.30	0.2214\n\
0.35	0.2155\n\
0.40	0.2104\n\
0.45	0.2061\n\
0.50	0.2032\n\
0.55	0.2020\n\
0.60	0.2034\n\
0.70	0.2165\n\
0.725	0.2230\n\
0.75	0.2313\n\
0.775	0.2417\n\
0.80	0.2546\n\
0.825	0.2706\n\
0.85	0.2901\n\
0.875	0.3136\n\
0.90	0.3415\n\
0.925	0.3734\n\
0.95	0.4084\n\
0.975	0.4448\n\
1.0	0.4805\n\
1.025	0.5136\n\
1.05	0.5427\n\
1.075	0.5677\n\
1.10	0.5883\n\
1.125	0.6053\n\
1.15	0.6191\n\
1.20	0.6393\n\
1.25	0.6518\n\
1.30	0.6589\n\
1.35	0.6621\n\
1.40	0.6625\n\
1.45	0.6607\n\
1.50	0.6573\n\
1.55	0.6528\n\
1.60	0.6474\n\
1.65	0.6413\n\
1.70	0.6347\n\
1.75	0.6280\n\
1.80	0.6210\n\
1.85	0.6141\n\
1.90	0.6072\n\
1.95	0.6003\n\
2.00	0.5934\n\
2.05	0.5867\n\
2.10	0.5804\n\
2.15	0.5743\n\
2.20	0.5685\n\
2.25	0.5630\n\
2.30	0.5577\n\
2.35	0.5527\n\
2.40	0.5481\n\
2.45	0.5438\n\
2.50	0.5397\n\
2.60	0.5325\n\
2.70	0.5264\n\
2.80	0.5211\n\
2.90	0.5168\n\
3.00	0.5133\n\
3.10	0.5105\n\
3.20	0.5084\n\
3.30	0.5067\n\
3.40	0.5054\n\
3.50	0.5040\n\
3.60	0.5030\n\
3.70	0.5022\n\
3.80	0.5016\n\
3.90	0.5010\n\
4.00	0.5006\n\
4.20	0.4998\n\
4.40	0.4995\n\
4.60	0.4992\n\
4.80	0.4990\n\
5.00	0.4988";


void ReadBallisticChart(void) {
	UINT32 Position = 0;
	FLOAT32 LastGNum = 0;
	FLOAT32 LastMachNum = 0;

	std::stringstream file(mgc1_txt);
	char buffer[255];
	while ( file.getline(buffer, 255) ) {
		FLOAT32 CurrentMachNum, CurrentGNum;
		sscanf((char*)buffer,"%f %f",&CurrentMachNum,&CurrentGNum);

		if ( LastMachNum == CurrentMachNum ) {
			LastGNum = CurrentGNum;
		} else {
			FLOAT32 Slope = (CurrentGNum - LastGNum) / (CurrentMachNum - LastMachNum);

			while (LastMachNum < CurrentMachNum - 0.0025) {
				if ( Position >= G1_CHART_NUM_DIVISIONS ) {
					break;
				}

				G1MachChart[Position] = LastGNum;

				LastMachNum += (FLOAT32)G1_CHART_MACH_DELTA;
				LastGNum += Slope * (FLOAT32)G1_CHART_MACH_DELTA;
				++Position;
			}

			LastGNum = CurrentGNum;
			LastMachNum = CurrentMachNum;
		}

		if (Position >= G1_CHART_NUM_DIVISIONS) {
			break;
		}
	}
}

//==============================

weapAmmoType ReadAmmoType(const std::string& name) {
	weapAmmoType ammo_type = {};
	weapAmmoType* AmmoType = &ammo_type;

	std::ifstream file(name);
	if ( ! file.is_open() )
		throw std::runtime_error("Could not open file \'" + name + "\'");

	strncpy((char*)AmmoType->Name, name.c_str(), 31);
	for ( int i = 0 ; i < 32 ; ++i )
		AmmoType->Name[i] = tolower(AmmoType->Name[i]);

	// ensure that ammo has some form of penetration constant
	AmmoType->PenetrationConstant = 0.00000000001f;
	AmmoType->TracerScale = 4.0f;

	while ( ! file.eof() ) {
		while ( file.peek() == '#') {
			file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}

		std::string line;
		std::getline(file, line);
		size_t delim = line.find('=');

		auto trim = [] (std::string& s) {
			if ( s.length() == 0 ) return;
			size_t beg = s.find_first_not_of(' ');
			size_t end = s.find_last_not_of(' ');
			s = s.substr(beg, end-beg+1);
		};
		std::string VarName(line.substr(0, delim));
		std::string VarContents_(line.substr(delim+1));
		trim(VarName);
		trim(VarContents_);

		const char* VarContents = VarContents_.c_str();

		if ( VarName.length() == 0 || VarContents_.length() == 0 )
			continue;

		uint32_t VarNum = 0;
		for ( ; VarNum < WEAP_AMMOMAXVAR ; ++VarNum ) {
			if ( _AmmoTypeVarNames[VarNum] == VarName )
				break;
		}

		switch ( VarNum ) {
		case WEAP_AMMOVAR_NAME:
			break;
		case WEAP_AMMOVAR_SUBTYPE:
			/*mlUtilityFindVarname((UINT32*)&AmmoType->SubType,
				VarContents,
				(UCHAR*)&_AmmoSubTypeVarNames[0][0],
				32,
				WEAPAMMO_SUBTYPE_MAX);*/
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
			/*if (strcmp((char*)VarContents,"bullet1") == 0)
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
				sprintf(Object->name,"%s%dat",VarContents,CurrentPos); // ensure unique name

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
			}*/
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
			/*mlUtilityFindVarname((UINT32*)&AmmoType->FuzeType, VarContents,
				(UCHAR*)&_AmmoFuzeTypeVarNames[0][0], 32, WEAPAMMO_FUZE_MAX);
			if(AmmoType->FuzeType != WEAPAMMO_FUZE_FIXTIME && AmmoType->FuzeType != WEAPAMMO_FUZE_VARTIME)
				Write_Log(LOG_WEAP,LOG_WARN, "fuze type %s not supported yet", VarContents);*/
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
			strncpy(AmmoType->hiteffect, (char*)VarContents, 31);
			break;
		}
		case WEAP_AMMOVAR_MODELCMX:
			//AmmoType->ModelCM.x = atof((char*)VarContents);
			break;
		case WEAP_AMMOVAR_MODELCMY:
			//AmmoType->ModelCM.y = atof((char*)VarContents);
			break;
		case WEAP_AMMOVAR_MODELCMZ:
			//AmmoType->ModelCM.z = atof((char*)VarContents);
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
		default:
			//Write_Log(LOG_WEAP,LOG_WARN,"Unidentified var %s in file %s",VarName,FileName);
			break;
		}
	}

	return ammo_type;
}

//--------------

weapAmmo CreateRound(weapAmmoType& ammo_type) {
	weapAmmo round;
	memset(&round,0,sizeof(weapAmmo));

	weapAmmoType* AmmoType = &ammo_type;
	weapAmmo* Ammo = &round;

	FLOAT32 InitVelocity = AmmoType->InitVelocity;

	// calculate round's actual velocity
	FLOAT64VECTOR ProjVel = {0, 0, 0};
	ProjVel.x = ProjVel.z = 0;
	ProjVel.y = InitVelocity; //* (1.0 + 0.02 * (mathGetFloat32ChaoticNumber() *  mathGetFloat32ChaoticNumber() *  2.0f - 1.0f));

	Ammo->LaunchTime = Ammo->LastUpdateTime = Ammo->LastCollideTime = 0.0f;
	Ammo->AmmoType = AmmoType;
	Ammo->Position = Ammo->LastCollidePos = {0.0f,0.0f,0.0f};
	Ammo->Velocity = Ammo->LastVel = ProjVel;
	Ammo->Flags |= WEAP_AMMOFLAG_FIRSTFRAME;
	Ammo->Id = 0;
	Ammo->FirstUpdate = true;
	Ammo->FireVehicle = nullptr;

	return round;
}

//--------------

void UpdateProjectile(weapAmmo* Ammo, double CurrentTime) {
	LOGICAL KillAmmo = false;
	FLOAT64 Speed = 0;
	FLOAT64 Mach = 0;
	FLOAT64 Cd = 0;
	FLOAT64 Density = 0;
	FLOAT64 Drag = 0;
	UINT32 MachIdx = 0;
	FLOAT64VECTOR Accel = {0, 0, 0};
	FLOAT64VECTOR AvgAcc = {0, 0, 0};
	FLOAT64VECTOR LastPosition = {0, 0, 0};
	FLOAT64 DeltaTime = 0;

	DeltaTime = CurrentTime - Ammo->LastUpdateTime;
	if ( DeltaTime < FLT_MIN )
		return;
	Ammo->LastUpdateTime = CurrentTime;

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

	// turn off first frame flag on 2nd update
	if (Ammo->PrevUpdateTime != 0.0)
		Ammo->Flags &= ~WEAP_AMMOFLAG_FIRSTFRAME;
}

//--------------

FLOAT64 PiercingPotential(const weapAmmo& round) {
	const weapAmmo* Ammo = &round;

	FLOAT64 AmmoSpeed = mathExactVectorLength(&Ammo->Velocity);
	FLOAT64 Obliquity = 0; // Assume straight-on collision.
	FLOAT64 PenetrationConstant = Ammo->AmmoType->PenetrationConstant;
	FLOAT64 PenetrationThickness = pow(AmmoSpeed,1/0.625) * (PenetrationConstant * pow(cos(Obliquity),1.5));

	return PenetrationThickness;
}