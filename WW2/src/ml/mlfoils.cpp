#include <syscfg.h> // Path is project specific.

#include "ob/stride_ptr.h"

#include <math.h>

#include "include/defs.h"

#include "ml/mlfoils.h"
#include "ml/mlutility.h"
#include "ml/mltypes.h"
#include "ml/mlCompon.h"
#include "ml/mlAtmo.h"
#include "ml/mlgauges.h"
#include "ml/mlEngine.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "math/mathvect.h"
#include "math/mathUnitConversion.h"
#include "rn/rnmain.h"
#include "veh/vehmain.h"

#include "ob/obmain.h"
#include "fx/fxMgr.h"
#include "fx/FXInterface.h"
#include "locl/loclMain.h"

#if defined(USE_OSG)
#include "osgFX/fxSound.h"
#else
#include "fx/fxSnd.h"
#endif

#if defined(USE_OSG)
#undef PI
#define PI osg::PI
#endif

#define _BUFFET_CP 0.25 // 0.2 since we are +/- 1.0 on the magnitude
#define _BUFFET_CD 0.05 // 0.01 since we are +/- 1.0 on the magnitude
#define _BUFFET_CM 0.002 // 0.004 since we are +/- 1.0 on the magnitude
#define _BUFFET_CL_MOVEMENT 0.025 // variation in ClMovement when fully stalled

static UINT32 _GetClCmCd(FLOAT64* Cl,
				  FLOAT64* Cm,
				  FLOAT64* Cd,
				  FLOAT64* StallAmount,
				  FLOAT64* ClMovement,
				  FLOAT64 InAoAindex,
				  mlAirfoilComponentData * AirfoilComponentData,
				  FLOAT64 FlapPosition,
				  FLOAT64 DeltaTime,
				  FLOAT64 ElevPos,
				  FLOAT64 RudderPos,
				  FLOAT64 AileronPos,
				  FLOAT64 FlapDamage,
				  FLOAT64VECTOR * ComponentVel,
				  FLOAT64 GroundEffect,
				  FLOAT64 AirfoilXPos,
				  mlComponent * Component,
					SINT8 * LeftSlatSound,
					SINT8 * RightSlatSound,
					mlObjectData * VehicleObjectData);

static UINT32 _AdjustClCdCmForStall(FLOAT64 * Cl,
								 FLOAT64 * Cd,
								 FLOAT64 * Cm,
								 FLOAT64 * StallAmount,
								 FLOAT64 * ClMovement,
								 FLOAT64 AoAindex,
								 mlAirfoilComponentData * AirfoilComponentData);

static UINT32 _AdjustAoAForAspectRatio(FLOAT64* AoAindex,
									   mlAirfoilComponentData * AirfoilComponentData,
									   FLOAT64 AirfoilXPos,
									   FLOAT64 GroundEffect,
									   mlComponent * Component);
#if(0)
static UINT32 _GetAlphaAdjust(FLOAT64 * DeltaAoA,
					   FLOAT64 AoAindex,
					   mlAirfoilComponentData* AirfoilComponentData);
#endif

static UINT32 _GetIndecies(SINT32* IntAoAIndex1, SINT32* IntAoAIndex2, FLOAT64 AoAindex);

static UINT32 _ReadAirfoilPolar(mlAirfoilAlpha * AlphaArray, UCHAR* PolarFilename, UCHAR* DataDir);
static UINT32 _ReadStallFile(mlAirfoilData * AirfoilData, UCHAR* StallFileName, UCHAR* DataDir);
static UINT32 _ReadCompressibilityFile(mlAirfoilData * AirfoilData,
								UCHAR* CompressFileName,
								UCHAR* DataDir);

static UINT32 _DetermineIfBlanked(FLOAT64* Blanked,
						   mlAirfoilComponentData* AirfoilComponentData,
						   FLOAT64VECTOR* ComponentVel);

static UINT32 _AdjustClCdCmForCompressibility(FLOAT64 * Cl,
									   FLOAT64 * Cd,
									   FLOAT64 * Cm,
									   FLOAT64 * ClMovement,
									   mlAirfoilData * AirfoilData,
									   FLOAT64 AoA,
									   FLOAT64 Speed,
									   FLOAT64 SpeedOfSound,
									   mlAirfoilComponentData * AirfoilComponentData);

static UINT32	_AdjustClCdCmForFlaps(FLOAT64 * Cl,
									  FLOAT64 * Cd,
									  FLOAT64 * Cm,
									  FLOAT64 AoAindex,
									  mlAirfoilComponentData* AirfoilComponentData,
									  FLOAT64 FlapPosition,
									  FLOAT64 FlapDamage);

static UCHAR _WingSectionTypes[ML_MAX_WINGSECTION_TYPES][32] = {
													"none",
													"normal",
													"aileron",
													"elevator",
													"rudder",
													"fuselage",
													"vortexpoint",
													"flap",
													"vortexpointaileron",
													"vortexpointelevator",
													"vortexpointrudder",
													"gear",
													"canopy",
													"deploy"
													};

static UCHAR _AirFoilVarName[ML_AIRFOIL_MAX_VARNAME][32] = {"none",
												 "baseairfoil",
												 "up10degairfoil",
												 "down10degairfoil",
												 "stallfile",
												 "compressibilityfile",
												};


static UCHAR _StallFileVarName[ML_STALLFILE_MAX_VARNAME][32] = {"none",
														"stallposaoa_startalpha",
														"stallposaoa_fullalpha",
														"stallnegaoa_startalpha",
														"stallnegaoa_fullalpha",
														"clmovement",
														};

static UCHAR _CompressibilityFileVarName[ML_MDIV_MAX_VARNAME][32] = {"none",
															 "mdivneutral",
															 "mdivpos10deg",
															 "mdivneg10deg",
															 "compressclmovement",
														};


static UCHAR _AirFoilComponentSpecialVars[ML_VARNAME_SPECIAL_MAX][32] = {"none",
																	"airfoiltype",
																	"area",
																	"wingchord",
																	"aspectratio",
																	"controlmin",
																	"controlmax",
																	"dragarea",
																	"dragk",
																	"usesdownwash",
																	"downwashcomponent",
																	"downwashminaoa",
																	"downwashmaxaoa",
																	"usespopoutslots",
																	"slotairfoiltype",
																	"slotdeployminaoa",
																	"slotdeploymaxaoa",
																	"blank1_pitchstart",
																	"blank1_pitchstartfull",
																	"blank1_pitchendfull",
																	"blank1_pitchend",
																	"blank1_yawstart",
																	"blank1_yawstartfull",
																	"blank1_yawendfull",
																	"blank1_yawend",
																	"blank2_pitchstart",
																	"blank2_pitchstartfull",
																	"blank2_pitchendfull",
																	"blank2_pitchend",
																	"blank2_yawstart",
																	"blank2_yawstartfull",
																	"blank2_yawendfull",
																	"blank2_yawend",
																	"blank3_pitchstart",
																	"blank3_pitchstartfull",
																	"blank3_pitchendfull",
																	"blank3_pitchend",
																	"blank3_yawstart",
																	"blank3_yawstartfull",
																	"blank3_yawendfull",
																	"blank3_yawend",
																	"flappos1cl",
																	"flappos1cd",
																	"flappos1cm",
																	"flappos2cl",
																	"flappos2cd",
																	"flappos2cm",
																	"flappos3cl",
																	"flappos3cd",
																	"flappos3cm",
																	"flappos4cl",
																	"flappos4cd",
																	"flappos4cm",
																	"vortexcomponent",
																	"vortexeffectpercent",
																	"vortexoffset",
																	"downwashpercent",
																	"stallsound",
																	"wingtwistco",
																	"induceddragcomponent",
																	"slotcloseaoa",
																	"slotdeploytime",
																	"wingspan",
																	"horizstabtrimdegup",
																	"horizstabtrimdegdown",
																	"downwashmult",
																	"innerchord",
																	"outerchord",
																	"divebrakearea",
																	"divebrakeangle",
																	"induceddragcomponent1",
																	"induceddragcomponent2",
																	"induceddragcomponent3",
																	"induceddragcomponent4",
																	"induceddragcomponent5",
																	"induceddragcomponent6",
																	"induceddragcomponent7",
																	"induceddragcomponent8",
																	"induceddragmult",
																	"canopynum",
																	"gearnum",
																	"position",
																	"maxopenspeed",
																	"maxclosespeed",
																	"maxdamagespeed",
																};

// this array is used to determine addition of lift due to compressibility effects
// drag effects are computed from the lift increase in the wingtip induced drag
// part
//
// this chart comes from a graph on page 282 of "Theory of Flight" by
// Richard von Mises
static FLOAT64 _MachEffectsOnCoefficientOfLift[ML_MACHEFFECTS_NUM_STEPS] =
{
	1.0,   // 0
	1.01,  // 0.05
	1.02,  // 0.10
	1.03,  // 0.15
	1.04,  // 0.20
	1.05,  // 0.25
	1.07,  // 0.30
	1.095, // 0.35
	1.12,  // 0.40
	1.15,  // 0.45
	1.20,  // 0.50
	1.24,  // 0.55
	1.27,  // 0.60
	1.28,  // 0.65
	1.275, // 0.70
	1.245, // 0.75
	1.2,   // 0.80
	1.19,  // 0.85
	1.2,   // 0.90
	1.2,   // 0.95
	1.2,   // 1.00
};

// drag from Mdiv to Mach 1.0
// percent is percent of mach1.0 - mdiv
//
// extrapolated from G1 ballistic drag curves
FLOAT64 mlMachEffectsOnCoefficientOfDrag[ML_MACHEFFECTS_NUM_STEPS] =
{
	1.00, // 0%
	1.01, // 5%
	1.02, // 10%
	1.04, // 15%
	1.07, // 20%
	1.12, // 25%
	1.28, // 30%
	1.50, // 35%
	1.72, // 40%
	2.04, // 45%
	2.45, // 50%
	2.97, // 55%
	3.60, // 60%
	4.40, // 65%
	5.36, // 70%
	6.52, // 75%
	7.91, // 80%
	9.49, // 85%
	10.22, // 90%
	12.02, // 95%
	13.79, // 100%
};

UINT32 _NoHorizStab;

UINT32 mlfoilsFreeAirfoilData(mlAirfoilList * AirfoilList)
{
	if (AirfoilList->AirfoilData != NULL)
	{
		memFree(AirfoilList->AirfoilData);
		AirfoilList->AirfoilData = NULL;
	}
	AirfoilList->NumAirfoils = 0;

	return 0;
}

UINT32 mlfoilsReadAirfoilFile(mlAirfoilList ** AirfoilList,
							  UCHAR * AirfoilListFilename,
							  UCHAR * DirName)
{
SINT32 AirfoilFile;
SINT32 NumAirfoils;
SINT32 AirfoilListPos;
UCHAR Buffer[256];
UCHAR * BufferPos;
UCHAR UseFileName[128];
UINT32 VarNum;
UCHAR * VarName;
UCHAR * VarContents;

	if(!AirfoilListFilename || !AirfoilListFilename[0])
		return 1;

	strcpy((char*)UseFileName,(char*)DirName);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)AirfoilListFilename);

	AirfoilFile = fioOpen((char*)UseFileName);
	if (AirfoilFile == -1)
	{
		Write_Debug("Can't find airfoil list filename %s\n",(char*)AirfoilListFilename);
		*AirfoilList = NULL;
		return 1;
	}

	// first determine how many airfoils we have
	NumAirfoils = 0;

	while (fioGetString(AirfoilFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '[')
		{
			NumAirfoils++;
		}
	}
	fioClose(AirfoilFile);

	AirfoilFile = fioOpen((char*)UseFileName);
	if (AirfoilFile == -1)
	{
		*AirfoilList = NULL;

		return 1;
	}

	*AirfoilList = (mlAirfoilList*)memAlloc(sizeof(mlAirfoilList));
	memset(*AirfoilList,0,sizeof(mlAirfoilList));
	(*AirfoilList)->AirfoilData = (mlAirfoilData*)(memAlloc(NumAirfoils * sizeof(mlAirfoilData)));
	memset((*AirfoilList)->AirfoilData,0,NumAirfoils*sizeof(mlAirfoilData));
	(*AirfoilList)->NumAirfoils = NumAirfoils;

	AirfoilListPos = -1;
	while (fioGetString(AirfoilFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (Buffer[0] == '[')
		{
			// new airfoil
			BufferPos = Buffer + 1;
			BufferPos = (UCHAR*)strtok((char*)BufferPos,"]");
			AirfoilListPos++;
			if (AirfoilListPos == NumAirfoils)
			{
				break;
			}

			strcpy((char*)(*AirfoilList)->AirfoilData[AirfoilListPos].AirfoilName,(char*)BufferPos);

			continue;
		}
		else
		{
			if (AirfoilListPos < 0)
			{
				continue;
			}

			mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

			if (VarName == NULL || VarContents == NULL)
			{
				continue;
			}

			mlUtilityFindVarname(&VarNum,
								VarName,
								(UCHAR*)&_AirFoilVarName[0][0],
								32,
								ML_AIRFOIL_MAX_VARNAME);

			if (VarNum == 0)
			{
				Write_Debug("Unidentified variable %s in file %s\n",VarName,AirfoilListFilename);
				continue;
			}

			switch (VarNum)
			{
				case ML_AIRFOIL_VARNAME_BASEAIRFOIL:
					_ReadAirfoilPolar((*AirfoilList)->AirfoilData[AirfoilListPos].NeutralAlphaData,
										VarContents,
										DirName);
					break;
				case ML_AIRFOIL_VARNAME_UP10DEGAIRFOIL:
					_ReadAirfoilPolar((*AirfoilList)->AirfoilData[AirfoilListPos].Up10DegAlphaData,
										VarContents,
										DirName);
					break;
				case ML_AIRFOIL_VARNAME_DOWN10DEGAIRFOIL:
					_ReadAirfoilPolar((*AirfoilList)->AirfoilData[AirfoilListPos].Down10DegAlphaData,
										VarContents,
										DirName);
					break;
				case ML_AIRFOIL_VARNAME_STALLFILENAME:
					_ReadStallFile(&(*AirfoilList)->AirfoilData[AirfoilListPos],
										VarContents,
										DirName);
					break;
				case ML_AIRFOIL_VARNAME_COMPRESSIBILITYFILENAME:
					_ReadCompressibilityFile(&(*AirfoilList)->AirfoilData[AirfoilListPos],
										VarContents,
										DirName);

					break;
				default:
						Write_Debug("Unidentified variable %s in file %s\n",VarName,AirfoilListFilename);
						break;
			}


		}

	}
	fioClose(AirfoilFile);

	return 0;

}

static UINT32 _ReadStallFile(mlAirfoilData * AirfoilData, UCHAR* StallFileName, UCHAR* DataDir)
{
SINT32 StallFile;
UCHAR UseFileName[128];
UINT32 VarNum;
UCHAR * VarName;
UCHAR * VarContents;
UCHAR Buffer[256];

	strcpy((char*)UseFileName,(char*)DataDir);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)StallFileName);

	StallFile = fioOpen((char*)UseFileName);

	if (StallFile == -1)
	{
		Write_Debug("Can't open stall file %s\n",(char*)StallFileName);
		return 1;
	}

	while (fioGetString(StallFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}


		mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

		if (VarName == NULL || VarContents == NULL)
		{
			continue;
		}


		mlUtilityFindVarname(&VarNum,
							VarName,
							(UCHAR*)&_StallFileVarName[0][0],
							32,
							ML_STALLFILE_MAX_VARNAME);

		if (VarNum == 0)
		{
			Write_Debug("Unidentified variable %s in file %s\n",VarName,StallFileName);
			continue;
		}

		switch (VarNum)
		{
			case ML_STALLFILE_VARNAME_POSAOASTARTALPHA:
					AirfoilData->PosAlphaStartStallBuffetAoA = atof((char*)VarContents);
				break;
			case ML_STALLFILE_VARNAME_POSAOAFULLALPHA:
					AirfoilData->PosAlphaFullStallBuffetAoA = atof((char*)VarContents);
				break;
			case ML_STALLFILE_VARNAME_NEGAOASTARTALPHA:
					AirfoilData->NegAlphaStartStallBuffetAoA = atof((char*)VarContents);
				break;
			case ML_STALLFILE_VARNAME_NEGAOAFULLALPHA:
					AirfoilData->NegAlphaFullStallBuffetAoA = atof((char*)VarContents);
				break;
			case ML_STALLFILE_VARNAME_CLMOVEMENT:
					AirfoilData->ClMovement = atof((char*)VarContents);
				break;
			default:
					Write_Debug("Unidentified variable %s in file %s\n",(char*)VarName,(char*)StallFileName);
					break;
		}
		continue;


	}
	fioClose(StallFile);

	return 0;
}

static UINT32 _ReadCompressibilityFile(mlAirfoilData * AirfoilData,
								UCHAR* CompressFileName,
								UCHAR* DataDir)
{
SINT32 CompressFile;
UCHAR UseFileName[128];
UINT32 VarNum;
UCHAR * VarName;
UCHAR * VarContents;
UCHAR Buffer[256];

	strcpy((char*)UseFileName,(char*)DataDir);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)CompressFileName);

	CompressFile = fioOpen((char*)UseFileName);

	if (CompressFile == -1)
	{
		Write_Debug("Can't open Compressibility file %s\n",(char*)CompressFileName);
		return 1;
	}

	while (fioGetString(CompressFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}


		mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

		if (VarName == NULL || VarContents == NULL)
		{
			continue;
		}


		mlUtilityFindVarname(&VarNum,
							VarName,
							(UCHAR*)&_CompressibilityFileVarName[0][0],
							32,
							ML_MDIV_MAX_VARNAME);

		if (VarNum == 0)
		{
			Write_Debug("Unidentified variable %s in file %s\n",VarName,CompressFileName);
			continue;
		}

		switch (VarNum)
		{
			case ML_MDIV_NEUTRAL_AOA:
				AirfoilData->MdivNeutral = atof((char*)VarContents);
				break;
			case ML_MDIV_POS10DEG_AOA:
				AirfoilData->MdivPos10DegAoA = atof((char*)VarContents);
				break;
			case ML_MDIV_NEG10DEG_AOA:
				AirfoilData->MdivNeg10DegAoA = atof((char*)VarContents);
				break;

			case ML_MDIV_COMPRESS_CL_MOVEMENT:
				AirfoilData->CompressClMovement = atof((char*)VarContents);
				break;
			default:
					Write_Debug("Unidentified variable %s in file %s\n",(char*)VarName,(char*)CompressFileName);
					break;
		}
		continue;


	}
	fioClose(CompressFile);

	return 0;
}



static UINT32 _ReadAirfoilPolar(mlAirfoilAlpha * AlphaArray, UCHAR* PolarFilename, UCHAR* DataDir)
{
SINT32 PolarFile;
UCHAR UseFileName[128];
FLOAT64 Alpha;
FLOAT64 Cl;
FLOAT64 Cd;
FLOAT64 Cm;
SINT32 IntAlpha;
UCHAR Buffer[256];

	strcpy((char*)UseFileName,(char*)DataDir);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)PolarFilename);

	PolarFile = fioOpen((char*)UseFileName);

	if (PolarFile == -1)
	{
		Write_Debug("Can't open polar file %s\n",(char*)PolarFilename);
		return 1;
	}

	while (fioGetString(PolarFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		sscanf((char*)Buffer,"%lf %lf %lf %lf",&Alpha,&Cl,&Cd,&Cm);
		IntAlpha = (int)(Alpha);
		AlphaArray[IntAlpha+180].LiftCoefficient = (FLOAT32)Cl;
		AlphaArray[IntAlpha+180].DragCoefficient = (FLOAT32)Cd;
		AlphaArray[IntAlpha+180].MomentCoefficient = (FLOAT32)Cm;
	}
	fioClose(PolarFile);

	return 0;
}

UINT32 mlfoilSetComponentSubtype(void*  SpecialData, char * SubType)
{
mlAirfoilComponentData * ComponentData;

	if (SpecialData == NULL || SubType == NULL)
	{
		return 1;
	}

	ComponentData = (mlAirfoilComponentData*)SpecialData;

	mlUtilityFindVarname(&ComponentData->Subtype,
						(UCHAR*)SubType,
						(UCHAR*)&_WingSectionTypes[0][0],
						32,
						ML_MAX_WINGSECTION_TYPES);

	return 0;
}

UINT32 mlfoilAddSpecialVariable(void*  SpecialData,
								 char * VarName,
								 char * VarContents,
								 mlAirfoilList * AirfoilList,
								 mlComponentList * ComponentList,
								 mlComponent * Component,
								 void* VehicleMasterPtr)
{
	mlAirfoilComponentData * ComponentData;
	UINT32 VarNum;
	UINT32 Found;
	UINT32 i;
	mlVehicleData* VehicleData = ((vehVehicle*)VehicleMasterPtr)->ModelData;

	if (SpecialData == NULL ||
		VarName == NULL ||
		VarContents == NULL ||
		AirfoilList == NULL)
	{
		return 1;
	}


	ComponentData = (mlAirfoilComponentData*)SpecialData;

	mlUtilityFindVarname(&VarNum, (UCHAR*)VarName,
						(UCHAR*)&_AirFoilComponentSpecialVars[0][0], 32, ML_VARNAME_SPECIAL_MAX);


	switch (VarNum)
	{
		case ML_VARNAME_SPECIAL_AIRFOILTYPE:
			strcpy((char*)ComponentData->AirfoilType,(char*)VarContents);

			Found = 0;
			for (i=0;i<AirfoilList->NumAirfoils;i++)
			{
				if (strncmp((char*)VarContents,(char*)AirfoilList->AirfoilData[i].AirfoilName,strlen((char*)VarContents)) == 0)
				{
					ComponentData->AirfoilData = &AirfoilList->AirfoilData[i];

					Found = 1;
					break;
				}
			}

			if (Found == 0)
			{
				ComponentData->AirfoilData = NULL;
				Write_Debug("Airfoil type %s for component %s not found",VarContents,Component->ComponentName);
			}

			break;
		case ML_VARNAME_SPECIAL_AREA:
			ComponentData->Area = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_WINGCHORD:
			ComponentData->WingChord = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_INNERCHORD:
			ComponentData->InnerChord = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_OUTERCHORD:
			ComponentData->OuterChord = atof(VarContents);
			break;
		case ML_VARNAME_DIVEBRAKE_AREA:
			ComponentData->DiveBrakeArea = atof(VarContents);
			break;
		case ML_VARNAME_DIVEBRAKE_ANGLE:
			ComponentData->DiveBrakeAngle = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_ASPECTRATIO:
			ComponentData->AspectRatio = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_WINGSPAN:
			ComponentData->WingSpan = atof(VarContents);
			break;

		case ML_VARNAME_SPECIAL_CONTROLMIN:
			ComponentData->ControlMin = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_CONTROLMAX:
			ComponentData->ControlMax = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_IDRAGAREA:
			ComponentData->IDragArea = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_IDRAGK:
			ComponentData->IDragK = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_USESDOWNWASH:
			ComponentData->UsesDownwashEffect = atoi(VarContents);
			break;
		case ML_VARNAME_SPECIAL_DOWNWASHCOMPONENT:
			mlFindComponent(&ComponentData->DownwashComponent,ComponentList,(UCHAR*)VarContents);
			break;
		case ML_VARNAME_SPECIAL_DOWNWASHMINAOA:
			ComponentData->DownwashMinAoA = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_DOWNWASHMAXAOA:
			ComponentData->DownwashMaxAoA = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_DOWNWASHPERCENT:
			ComponentData->DownwashPercent = atof(VarContents);
			break;

		case ML_VARNAME_SPECIAL_USESPOPOUTSLOTS:
			ComponentData->UsesPopoutSlots = atoi(VarContents);
			break;
		case ML_VARNAME_SPECIAL_SLOTAIRFOILTYPE:
			Found = 0;
			for (i=0;i<AirfoilList->NumAirfoils;i++)
			{
				if (strncmp(VarContents,(char*)AirfoilList->AirfoilData[i].AirfoilName,strlen(VarContents)) == 0)
				{
					ComponentData->SlotAirfoilData = &AirfoilList->AirfoilData[i];

					Found = 1;
					break;
				}
			}

			if (Found == 0)
			{
				ComponentData->SlotAirfoilData = NULL;
				Write_Debug("Airfoil slot deployed type %s for component %s not found",VarContents,Component->ComponentName);
			}
			break;

		case ML_VARNAME_SPECIAL_SLOTDEPLOYMINAOA:
			ComponentData->SlotDeployMinAoA = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_SLOTCLOSEAOA:
			ComponentData->SlotCloseAoA = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_SLOTDEPLOYMAXAOA:
			ComponentData->SlotDeployMaxAoA = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_SLOTDEPLOYTIME:
			ComponentData->SlotDeploySpeed = atof(VarContents) / 3.6;

		case ML_VARNAME_SPECIAL_BLANK1_PITCH_START:
			ComponentData->BlankPitchStart[0] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_PITCH_STARTFULL:
			ComponentData->BlankPitchStartFull[0] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_PITCH_ENDFULL:
			ComponentData->BlankPitchEndFull[0] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_PITCH_END:
			ComponentData->BlankPitchEnd[0] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_YAW_START:
			ComponentData->BlankYawStart[0] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_YAW_STARTFULL:
			ComponentData->BlankYawStartFull[0] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_YAW_ENDFULL:
			ComponentData->BlankYawEndFull[0] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK1_YAW_END:
			ComponentData->BlankYawEnd[0] = -atof(VarContents);
			break;

		case ML_VARNAME_SPECIAL_BLANK2_PITCH_START:
			ComponentData->BlankPitchStart[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_PITCH_STARTFULL:
			ComponentData->BlankPitchStartFull[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_PITCH_ENDFULL:
			ComponentData->BlankPitchEndFull[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_PITCH_END:
			ComponentData->BlankPitchEnd[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_YAW_START:
			ComponentData->BlankYawStart[1] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_YAW_STARTFULL:
			ComponentData->BlankYawStartFull[1] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_YAW_ENDFULL:
			ComponentData->BlankYawEndFull[1] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK2_YAW_END:
			ComponentData->BlankYawEnd[1] = -atof(VarContents);
			break;

		case ML_VARNAME_SPECIAL_BLANK3_PITCH_START:
			ComponentData->BlankPitchStart[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_PITCH_STARTFULL:
			ComponentData->BlankPitchStartFull[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_PITCH_ENDFULL:
			ComponentData->BlankPitchEndFull[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_PITCH_END:
			ComponentData->BlankPitchEnd[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_YAW_START:
			ComponentData->BlankYawStart[2] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_YAW_STARTFULL:
			ComponentData->BlankYawStartFull[2] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_YAW_ENDFULL:
			ComponentData->BlankYawEndFull[2] = -atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_BLANK3_YAW_END:
			ComponentData->BlankYawEnd[2] = -atof(VarContents);
			break;

		case ML_VARNAME_SPECIAL_FLAPPOS1_CL:
			ComponentData->FlapCl[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS1_CD:
			ComponentData->FlapCd[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS1_CM:
			ComponentData->FlapCm[1] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS2_CL:
			ComponentData->FlapCl[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS2_CD:
			ComponentData->FlapCd[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS2_CM:
			ComponentData->FlapCm[2] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS3_CL:
			ComponentData->FlapCl[3] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS3_CD:
			ComponentData->FlapCd[3] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS3_CM:
			ComponentData->FlapCm[3] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS4_CL:
			ComponentData->FlapCl[4] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS4_CD:
			ComponentData->FlapCd[4] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_FLAPPOS4_CM:
			ComponentData->FlapCm[4] = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_VORTEX_COMPONENT:
			mlFindComponent(&ComponentData->VortexComponent,ComponentList,(UCHAR*)VarContents);
			break;
		case ML_VARNAME_SPECIAL_VORTEX_EFFECT_PERCENT:
			ComponentData->VortexEffectPercent = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_VORTEX_EFFECT_OFFSET:
			ComponentData->VortexEffectOffset = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_STALLSOUND:
			ComponentData->StallSound = atoi(VarContents);
			break;
		case ML_VARNAME_WINGTWIST_COEEFFICIENT:
			ComponentData->WingTwistCoefficient = atof(VarContents);
			break;
		case ML_VARNAME_INDUCEDDRAG_CLCOMPONENT:
			mlFindComponent(&ComponentData->InducedDragCoefficientsComponent,ComponentList,(UCHAR*)VarContents);
			break;
		case ML_VARNAME_SPECIAL_HORIZSTABTRIMDEGUP:
			ComponentData->HorizStabTrimUpDeg = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_HORIZSTABTRIMDEGDOWN:
			ComponentData->HorizStabTrimDownDeg = atof(VarContents);
			break;
		case ML_VARNAME_SPECIAL_DOWNWASHMULT:
			ComponentData->DownWashMult = atof(VarContents);
			break;

		case ML_VARNAME_INDUCEDDRAGLIFT_1:
		case ML_VARNAME_INDUCEDDRAGLIFT_2:
		case ML_VARNAME_INDUCEDDRAGLIFT_3:
		case ML_VARNAME_INDUCEDDRAGLIFT_4:
		case ML_VARNAME_INDUCEDDRAGLIFT_5:
		case ML_VARNAME_INDUCEDDRAGLIFT_6:
		case ML_VARNAME_INDUCEDDRAGLIFT_7:
		case ML_VARNAME_INDUCEDDRAGLIFT_8:
			mlFindComponent(&ComponentData->InducedDragLiftComp[VarNum - ML_VARNAME_INDUCEDDRAGLIFT_1],ComponentList,(UCHAR*)VarContents);
		break;

		case ML_VARNAME_INDUCEDDRAGMULT:
			ComponentData->InducedDragMult = atof(VarContents);
		break;

		case ML_VARNAME_CANOPYNUM:
		case ML_VARNAME_GEARNUM:
		case ML_VARNAME_POSITION:
			ComponentData->Position = atoi(VarContents) - 1;
			if(ComponentData->Position < 0 || ComponentData->Position >= ML_MAX_CREW_POSITIONS)
				ComponentData->Position = 0;
		break;
		case ML_VARNAME_MAXOPENSPEED:
			if(ComponentData->Subtype == ML_GEARDRAG_SECTION)
			{
				VehicleData->GearMaxOpenSpeed = mathUnitAsciiToMetric(VarContents);
			}else if(ComponentData->Subtype == ML_CANOPYDRAG_SECTION)
			{
				VehicleData->CanopyMaxOpenSpeed[ComponentData->Position] = mathUnitAsciiToMetric(VarContents);
			}else if(ComponentData->Subtype == ML_DEPLOYDRAG_SECTION)
			{
				VehicleData->BombBayMaxOpenSpeed = mathUnitAsciiToMetric(VarContents);
			}
		break;
		case ML_VARNAME_MAXCLOSESPEED:
			if(ComponentData->Subtype == ML_GEARDRAG_SECTION)
			{
				VehicleData->GearMaxCloseSpeed = mathUnitAsciiToMetric(VarContents);
			}else if(ComponentData->Subtype == ML_CANOPYDRAG_SECTION)
			{
				VehicleData->CanopyMaxCloseSpeed[ComponentData->Position] = mathUnitAsciiToMetric(VarContents);
			}else if(ComponentData->Subtype == ML_DEPLOYDRAG_SECTION)
			{
				VehicleData->BombBayMaxCloseSpeed = mathUnitAsciiToMetric(VarContents);
			}
		break;
		case ML_VARNAME_MAXDAMAGESPEED:
			ComponentData->MaxDamageSpeed = mathUnitAsciiToMetric(VarContents);
		break;
		default:
			Write_Debug("Unidentified variable %s in component %s\n",VarName,Component->ComponentName);
		break;
	}

	return 0;
}

UINT32 mlGetAirfoilForcesAndTorques(FLOAT64VECTOR* RetForce,
									FLOAT64ATT* RetTorque,
									FLOAT64* ClMovement,
									mlComponent* Component,
									FLOAT64VECTOR* InComponentVel,
									FLOAT64 Alt,
									FLOAT64 AGL,
									FLOAT64 FlapPosition,
									FLOAT64 DeltaTime,
									FLOAT64 ElevPos,
									FLOAT64 RudderPos,
									FLOAT64 AileronPos,
									FLOAT64* StallAmount,
									FLOAT64 HorizTabPosition,
									OBJECT * VehicleObject,
									SINT8 * LeftSlatSound,
									SINT8 * RightSlatSound,
									mlObjectData * VehicleObjectData,
									UINT32 DiveBrakeOn)
{
	FLOAT64 Density = 0.0f;
	FLOAT64 SpeedOfSound = 0.0f;
	FLOAT64 AoA = 0.0f;
	FLOAT64 AoAindex = 0.0f;
	//FLOAT64 DeltaAoA = 0.0f;
	FLOAT64 AirSpeed = 0.0f;
    FLOAT64 AirSpeed1 = 0.0f;
	FLOAT64 Cl = 0.0f, Cd = 0.0f, Cm = 0.0f;
	FLOAT64 Q = 0.0f, Q1 = 0.0f;
	FLOAT64 Lift = 0.0f;
	FLOAT64 Drag = 0.0f;
	FLOAT64 Moment = 0.0f;
	FLOAT64 TempNum = 0.0f;
	FLOAT64 LiftReduction = 0.0f;
	FLOAT64 c = 0.0f;
	FLOAT64 Blanked = 0.0f;
	FLOAT64 UseCl = 0.0f;
	FLOAT64 DamageRatio = 0.0f;
	FLOAT64 DiveBrakeDrag = 0.0f;
	FLOAT64 TempElevPos = 0.0f;
    FLOAT64 TempRudderPos = 0.0f;
    FLOAT64 TempAileronPos = 0.0f;
    FLOAT64 Ratio1 = 0.0f;
    FLOAT64 Ratio2 = 0.0f;
	FLOAT64 GroundEffect = 0.0f;
	mlEngineComponentData * PropellerData = NULL;
	mlAirfoilComponentData * AirfoilComponentData = NULL;
	FLOAT64VECTOR ComponentVel;
	FLOAT64VECTOR Force;
	FLOAT64ATT TempAtt;
	UINT32 i = 0;
	UINT32 TempInt = 0;

	Force.x = Force.y = Force.z = 0;

	if (Component->SpecialData == NULL)
	{
		RetTorque->Pitch = RetTorque->Roll = RetTorque->Yaw = 0;
		RetForce->x = RetForce->y = RetForce->z = 0;
		return 1;
	}
	AirfoilComponentData = (mlAirfoilComponentData*)Component->SpecialData;

	if (AirfoilComponentData->AirfoilData == NULL)
	{
		return 0;
	}

	// resolve damage
	TempInt = 0;
	for (i=0;i<4;i++)
	{
		TempInt += Component->NewDamageNumHits[i];
	}

	if (TempInt > 0)
	{
		for (i=0;i<4;i++)
		{
			Component->CurrentDamage += Component->NewDamage[i];
		}

		if (Component->CurrentDamage > Component->DamageLevels[2] &&
			AirfoilComponentData->ControlSurfaceJammed == 0 &&
			AirfoilComponentData->ControlSurfaceFree == 0)
		{
			for (i=0;i<TempInt;i++)
			{
				if ((double)rand() / (double)RAND_MAX < 0.1)
				{
					if ((double)rand() / (double)RAND_MAX < 0.5)
					{
						AirfoilComponentData->ControlSurfaceJammed = 1;
					}
					else
					{
						AirfoilComponentData->ControlSurfaceFree = 1;
					}
				}
			}
		}
	}

	DamageRatio = (Component->DamageLevels[3] <= 0.0001) ? 0.0 : Component->CurrentDamage / Component->DamageLevels[3];
	if(DamageRatio > 1.0)
		DamageRatio = 1.0;

	ComponentVel = *InComponentVel;

	// calc atmospheric conditions at alt
	mlGetAltDensity(&Density,Alt);
	mlGetAltSpeedOfSound(&SpeedOfSound,Alt);  // for compressibility effects

	// calc ground effect
	// ground effect 0.0 = full ground effect (no ID, downwash effects, or Cl adjust for aspect ratio)
	// and 1.0 equals normal flight
	GroundEffect = 1.0;
	if (AirfoilComponentData->WingSpan > 0.001)
	{
		if (AGL >= 0.0 && AGL < AirfoilComponentData->WingSpan)
		{
			GroundEffect = sqrt(1.0 -
					((AirfoilComponentData->WingSpan - AGL)/AirfoilComponentData->WingSpan) *
					((AirfoilComponentData->WingSpan - AGL)/AirfoilComponentData->WingSpan));
		}
	}

	// adjust AoA and ComponentVel for propwash effects
	if (AirfoilComponentData->VortexComponent != NULL)
	{
		if (AirfoilComponentData->VortexComponent->SpecialData != NULL)
		{
			PropellerData = (mlEngineComponentData*)AirfoilComponentData->VortexComponent->SpecialData;
			// add in backwash velocity
			ComponentVel.y += PropellerData->VortexBackWashVelocity *
					AirfoilComponentData->VortexEffectPercent;
			ComponentVel.z += PropellerData->VortexRotationSpeed *
							AirfoilComponentData->VortexEffectOffset;
							AirfoilComponentData->VortexEffectPercent;
		}
	}

	// first we're gonna need the AoA of the airflow relative to the wing segment
	AoA = atan2(ComponentVel.z,ComponentVel.y);
	AoAindex = mlRAD_TO_DEG(AoA);


	// Adjust AoA for wingtwist effects
	if (AirfoilComponentData->WingTwistCoefficient != 0.0)
	{
		AoAindex += AirfoilComponentData->LastTwistTorque * AirfoilComponentData->WingTwistCoefficient;
	}


	if (HorizTabPosition > 0.0)
	{
		AoAindex -= AirfoilComponentData->HorizStabTrimUpDeg * HorizTabPosition;
	}
	else
	{
		AoAindex += AirfoilComponentData->HorizStabTrimDownDeg * (-HorizTabPosition);
	}

	if (DiveBrakeOn != 0)
	{
		AoAindex += AirfoilComponentData->DiveBrakeAngle;
	}

	AoAindex = fmod(AoAindex,360.0);
	if (AoAindex > 180)
	{
		AoAindex -= 360;
	}

	TempElevPos = ElevPos;
	TempRudderPos = RudderPos;
	TempAileronPos = AileronPos;

	Ratio1 = 0.0;
	if (Component->CurrentDamage >  Component->DamageLevels[1])
	{
		Ratio1 = (Component->CurrentDamage - Component->DamageLevels[1]) /
					(Component->DamageLevels[3] - Component->DamageLevels[1]);

		if(Ratio1 > 1.0)
			Ratio1 = 1.0;

		Ratio2 = 1.0 - (Ratio1 * 0.7 + 0.3);
		TempElevPos *= Ratio2;
		TempRudderPos *= Ratio2;
		TempAileronPos *= Ratio2;
	}

	_GetClCmCd(&Cl,
				&Cm,
				&Cd,
				StallAmount,
				ClMovement,
				AoAindex,
				AirfoilComponentData,
				FlapPosition,
				DeltaTime,
				TempElevPos,
				TempRudderPos,
				TempAileronPos,
				Ratio1,
				&ComponentVel,
				GroundEffect,
				Component->OriginalRelativePos.x,
				Component,
				LeftSlatSound,
				RightSlatSound,
				VehicleObjectData);

	if (AirfoilComponentData->UsesPopoutSlots == 1)
	{
		if (Component->CurrentRelativePos.x < 0.0)
		{
			mlUpdateLeftSlotStatus((FLOAT32)AirfoilComponentData->SlotIsDeployed);
		}
		else
		{
			mlUpdateRightSlotStatus((FLOAT32)AirfoilComponentData->SlotIsDeployed);
		}
	}

	_AdjustClCdCmForCompressibility(&Cl,
									&Cd,
									&Cm,
									&TempNum,
									AirfoilComponentData->AirfoilData,
									AoAindex,
									ComponentVel.y,
									SpeedOfSound,
									AirfoilComponentData);

	*ClMovement += TempNum;

	*ClMovement -= AirfoilComponentData->WingChord * 0.25 * DamageRatio;

	Cl *= (1.0 - (DamageRatio * (0.7 + 0.2 * ((double)rand() / (double)RAND_MAX * 2.0 - 1.0))));
	Cd += DamageRatio * (0.05 + ((double)rand() / (double)RAND_MAX * 2.0 - 1.0) * 0.1);
	Cm *= (1.0 - DamageRatio);

	Blanked = 0.0;

	_DetermineIfBlanked(&Blanked,AirfoilComponentData,&ComponentVel);

	// dive brakes
	if (DiveBrakeOn != 0)
	{
		DiveBrakeDrag = 1.11 * AirfoilComponentData->DiveBrakeArea * cos(mlDEG_TO_RAD(AoAindex));
		if (DiveBrakeDrag < 0.0)
		{
			DiveBrakeDrag = -DiveBrakeDrag;
		}
	}
	else
	{
		DiveBrakeDrag = 0.0;
	}

	Force.x = Force.y = Force.z = 0;

	AirSpeed = sqrt(ComponentVel.y*ComponentVel.y + ComponentVel.z*ComponentVel.z);

	// Q is 1/2 density times velocity squared
	Q = 0.5 * Density * AirSpeed * AirSpeed;

	// we calculate the airfoil lift force via this:
	//
	// lift (Z value) = Cl * Q * S
	//
	// where Cl is the coefficient of lift for the AoA,
	// Q is 1/2 density times velocity squared
	// and S is the surface area of the airfoil segment
	// airfoil lift force is relative to the airflow so
	// we will rotate the force later to compensate for AoA
	Lift = Cl * Q * AirfoilComponentData->Area;
	Force.z = Lift;

	// we calculate the moment due to lift via this:
	//
	// moment = Cm * c * Q * S
	//
	// where Cm is the coefficient of moment
	// and c is ?????unknown?????
	c = 1.0;
	Moment = Cm * c * Q * AirfoilComponentData->Area * AirfoilComponentData->WingChord;
	RetTorque->Pitch = RetTorque->Roll = RetTorque->Yaw = 0;
	RetTorque->Pitch = Moment;
	AirfoilComponentData->LastTwistTorque = Moment;

	// we calculate the parasitic/pressure drag via this:
	//
	// drag (Z value) = Cd * Q * S
	//
	// where Cd is the coefficient of drag for the AoA
	// Q is based on total airspeed
	//
	// first drag in the crosssection direction
	// then in the perpendicular direction
	AirSpeed1 = ComponentVel.x;
	Q1 = 0.5 * Density * AirSpeed1 * AirSpeed1;
	Drag = Cd * Q * AirfoilComponentData->Area;
	Force.y = Drag + DiveBrakeDrag * Q;
	Drag = Cd * Q1 * AirfoilComponentData->Area;
	Force.x = Drag;

	// now we handle induced drag
	// induced drag or windtip vortex drag is simply
	// a product of lift, and follows the equation:
	//
	// drag = K/AR * Cl^2 * Q * S
	//
	// where K is a constant (equal to 1/PI for an elliptical wing, larger for others)
	// AR is the wing aspect ratio
	// and Cl is the coefficient of lift
	//
	// since Vortex Drag is really just a "redirection" of lift, we reduce lift
	// by the amount to make the right triangle, with one side
	// being drag, the hypotenuse being total lift, and the other side being
	// the "adjusted" lift.  Then the "adjustment" is the total lift minus
	// the "adjusted" lift.

	if ((AirfoilComponentData->Subtype == ML_VORTEX_POINT ||
		 AirfoilComponentData->Subtype == ML_VORTEX_POINT_AILERON ||
		 AirfoilComponentData->Subtype == ML_VORTEX_POINT_ELEVATOR ||
		 AirfoilComponentData->Subtype == ML_VORTEX_POINT_RUDDER) &&
		AirfoilComponentData->AspectRatio != 0.0)
	{
		if (AirfoilComponentData->InducedDragLiftComp[0] != NULL)
		{
		FLOAT64 TempLift;
		FLOAT64 TempArea;
		mlAirfoilComponentData * Airfoil;

			TempLift = 0.0;
			TempArea = 0.0;

			for (i=0;i<8;i++)
			{
				if (AirfoilComponentData->InducedDragLiftComp[i] != NULL)
				{
					Airfoil = (mlAirfoilComponentData*)(AirfoilComponentData->InducedDragLiftComp[i]->SpecialData);
					TempLift += Airfoil->LastCl * Airfoil->Area;
					TempArea += Airfoil->Area;
				}
			}

			if (TempArea > 0.00001 && Q > 10.0)
			{
				UseCl = TempLift / TempArea;

				Drag = AirfoilComponentData->IDragK / AirfoilComponentData->AspectRatio * Q *
							UseCl*UseCl * AirfoilComponentData->IDragArea;

			}
		}
		else
		{
			UseCl = Cl;

			if (AirfoilComponentData->InducedDragCoefficientsComponent != NULL)
			{
				UseCl = ((mlAirfoilComponentData*)
					(AirfoilComponentData->InducedDragCoefficientsComponent->SpecialData))->LastCl;
			}

			Drag = AirfoilComponentData->IDragK / AirfoilComponentData->AspectRatio * Q *
						UseCl*UseCl * AirfoilComponentData->IDragArea;
		}

		Drag *= AirfoilComponentData->InducedDragMult;

		Lift = UseCl * Q * AirfoilComponentData->IDragArea;
		if (fabs(Drag) > fabs(Lift))
		{
			// something hozed if this is the case
			Force.y += Drag * (1.0 - *StallAmount) * GroundEffect;
		}
		else
		{
			TempNum = sqrt(Lift * Lift - Drag * Drag);
			if (Lift < 0.0)
			{
				LiftReduction = Lift + TempNum;
			}
			else
			{
				LiftReduction = Lift - TempNum;
			}

			Force.y += Drag * (1.0 - *StallAmount) * GroundEffect;
			Force.z += -LiftReduction * (1.0 - *StallAmount) * GroundEffect;
		}

// hack for wingtip vortecies
		if (fabs(UseCl * ComponentVel.y * 0.5 * Density) > 60)
		{
			AirfoilComponentData->VortexOn = 1;

			if (Component->OriginalRelativePos.x < 0.0)
			{
				AirfoilComponentData->VortexNum = 1;
			}
			else
			{
				AirfoilComponentData->VortexNum = 2;
			}
		}
		else
		{
			AirfoilComponentData->VortexOn = 0;

		}
	}

	AirfoilComponentData->LastCl = Cl;

	// now make the forces relative to the component
	TempAtt.Pitch = -AoA;
	TempAtt.Roll = TempAtt.Yaw = 0;

	TempNum = Force.y;
	Force.y = Force.y * cos(-AoA) + Force.z * sin(-AoA);
	Force.z = -TempNum * sin(-AoA) + Force.z * cos(-AoA);

	// in order to get wind tunnel results right, we must eliminate the horiz stab from the equation
	{
		if (AirfoilComponentData->Subtype == ML_ELEVATOR_SECTION && _NoHorizStab)
		{
			Force.x = 0;
			Force.y = 0;
			Force.z = 0;
		}
	}

	if (Blanked > 0.0)
	{
		AirfoilComponentData->IsBlanked = TRUE;
		Force.x *= (0.5 + 0.3 * Blanked);
		Force.y *= (0.5 + 0.3 * Blanked);
		Force.z *= (0.5 + 0.3 * Blanked);
	}else
		AirfoilComponentData->IsBlanked = FALSE;

	// finally compute downwash velocity
	AirfoilComponentData->DownwashDownVelocity = AirSpeed * Cl / PI /
				AirfoilComponentData->AspectRatio * GroundEffect;

	if(AirfoilComponentData->DownwashDownVelocity < 0)
		AirfoilComponentData->DownwashDownVelocity = 0;

	if (AirfoilComponentData->DownwashDownVelocity <= AirSpeed)
	{
		AirfoilComponentData->DownwashStraightVelocity = sqrt(AirSpeed * AirSpeed -
			AirfoilComponentData->DownwashDownVelocity * AirfoilComponentData->DownwashDownVelocity)
			 * GroundEffect;
	}
	else
	{
		AirfoilComponentData->DownwashStraightVelocity = 0;
	}

	*RetForce = Force;

	return 0;
}



static UINT32 _GetClCmCd(FLOAT64* Cl,
				  FLOAT64* Cm,
				  FLOAT64* Cd,
				  FLOAT64* StallAmount,
				  FLOAT64* ClMovement,
				  FLOAT64 InAoAindex,
				  mlAirfoilComponentData * AirfoilComponentData,
				  FLOAT64 FlapPosition,
				  FLOAT64 DeltaTime,
				  FLOAT64 ElevPos,
				  FLOAT64 RudderPos,
				  FLOAT64 AileronPos,
				  FLOAT64 FlapDamage,
				  FLOAT64VECTOR * ComponentVel,
				  FLOAT64 GroundEffect,
				  FLOAT64 AirfoilXPos,
				  mlComponent * Component,
					SINT8 * LeftSlatSound,
					SINT8 * RightSlatSound,
					mlObjectData * VehicleObjectData)
{
	FLOAT64 InterpolationDist;
	SINT32 IntAoAIndex1;
	SINT32 IntAoAIndex2;
	FLOAT64 TempNum1,TempNum2;
	FLOAT64 ControlInput;
	FLOAT64 DeflectionAngle;
	mlAirfoilAlpha* AlphaData1,*AlphaData2;
	mlAirfoilData * AirfoilData;
	FLOAT64 AoAindex;
	FLOAT64 TempNum;
	FLOAT64 DeploySpeed;
	FLOAT64 A;
	FLOAT64 Speed;
	FLOAT32 Volume;

	AirfoilData = AirfoilComponentData->AirfoilData;


	if (AirfoilData == NULL)
	{
		*Cl = 0;
		*Cd = 0;
		*Cm = 0;

		return 1;
	}

	AoAindex = InAoAindex;

	_AdjustAoAForAspectRatio(&AoAindex,AirfoilComponentData,AirfoilXPos,GroundEffect,Component);

	if (AirfoilComponentData->UsesPopoutSlots)
	{
		if (AirfoilComponentData->GoingOpenOrClosed == 1)
		{
			if (AoAindex >= AirfoilComponentData->SlotDeployMaxAoA ||
				AoAindex <= AirfoilComponentData->SlotCloseAoA)
			{
				AirfoilComponentData->GoingOpenOrClosed = 0;
			}
		}
		else
		{
			if (AoAindex >= AirfoilComponentData->SlotDeployMinAoA &&
				AoAindex <= AirfoilComponentData->SlotDeployMaxAoA)
			{
				AirfoilComponentData->GoingOpenOrClosed = 1;
			}
		}

		DeploySpeed = AirfoilComponentData->SlotDeploySpeed;
		if (DeploySpeed == 0.0)
		{
			DeploySpeed = 0.1;
		}

		TempNum = Speed = mathExactVectorLength(ComponentVel);

		if (TempNum < 3.0)
		{
			DeploySpeed = DeploySpeed * (27.8 * 27.8 / 9.0);
		}
		else
		{
			DeploySpeed = DeploySpeed * (27.8 * 27.8 / (TempNum * TempNum));
		}

		if (AirfoilComponentData->GoingOpenOrClosed == 0)
		{
			// determine constant A
			A = 1.0 / DeploySpeed / DeploySpeed;

			// determine current T for current position
			TempNum = sqrt(fabs((1.0 - AirfoilComponentData->SlotIsDeployed) / A));

			// now compute movement in deltatime
			TempNum = (DeltaTime / 1000.0 + TempNum);
			TempNum *= TempNum;

			TempNum *= A;
			TempNum = 1.0 - TempNum;
		}
		else
		{
			// determine constant A
			A = 1.0 / DeploySpeed / DeploySpeed;

			// determine current T for current position
			TempNum = sqrt(fabs((AirfoilComponentData->SlotIsDeployed) / A));

			// now compute movement in deltatime
			TempNum = (DeltaTime / 1000.0 + TempNum);
			TempNum *= TempNum;

			TempNum *= A;

		}
		// play slot bang sound
		if ((TempNum <= 0.001 && AirfoilComponentData->SlotIsDeployed > 0.001) ||
			(TempNum >= 0.999 && AirfoilComponentData->SlotIsDeployed < 0.999))
		{
			if (Speed > 50.0)
			{
				Volume = 1.0f;
			}
			else
			{
				if (Speed < 25.0)
				{
					Volume = 0.0f;
				}
				else
				{
					Volume = (FLOAT32)((Speed - 25.0)/25.0 * (Speed - 25.0)/25.0);
				}
			}
			if (Component->CurrentRelativePos.x < 0.0)
			{
				if(LeftSlatSound[0] && Volume > 0.0f)
				{
					if (VehicleObjectData != NULL)
					{
						if (VehicleObjectData->PlaneObject != NULL)
						{
						OBJECT * TempObj;

							if (VehicleObjectData->HitMap != NULL)
							{
								TempObj = VehicleObjectData->HitMap;
							}
							else
							{
								TempObj = VehicleObjectData->PlaneObject;
							}

							if(AirfoilComponentData->lflapSoundId > 0)
							{
#if defined(USE_OSG)
                                fxSoundStopSound(AirfoilComponentData->lflapSoundId);
#else
								fxSndStopSound(AirfoilComponentData->lflapSoundId);
#endif
								AirfoilComponentData->lflapSoundId = 0;
							}

#if defined(USE_OSG)
                            osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                            osg::Group* o = ((osg::Node*)TempObj->osgNode)->asGroup();

                            int id = fxSoundPlayObjectSound((char*)LeftSlatSound,
                                                            pos,
                                                            finite_sound,
                                                            sound_high,
                                                            20.0f,
                                                            100.0f,
                                                            o);

                            AirfoilComponentData->lflapSoundId = id;
#else
                            SINT32 id = fxSndGetNewSound();

                            if(id > 0)
                            {
                                fxSndSetSoundName(id, (SCHAR*)LeftSlatSound);
                                fxSndSetPriority(id, IDE_FP_PRIORITY_HIGH);
                                fxSndSetSoundType(id, TYPE_FINITE_SOUND);
                                fxSndSetVolume(id, Volume);
                                fxSndSetPitch(id, 1.0f);
                                fxSndSetPosition(id,
                                                 Component->OriginalRelativePos.x,
                                                 Component->OriginalRelativePos.y,
                                                 Component->OriginalRelativePos.z);
                                fxSndSetEmitterObject(id, TempObj);
                                fxSndSetPlayType(id, DEF_PLAYTYPE_OBJECT);
                                fxSndSetMaxDistance(id, 100.0f);
                                fxSndSetMinDistance(id, 20.0f);

                                fxSndInitSound(id);
								AirfoilComponentData->lflapSoundId = id;
                            }
#endif
						}
					}
				}
			}
			else
			{
				if(RightSlatSound[0] && Volume > 0.0f)
				{
					if (VehicleObjectData != NULL)
					{
						if (VehicleObjectData->PlaneObject != NULL)
						{
						OBJECT * TempObj;

							if (VehicleObjectData->HitMap != NULL)
							{
								TempObj = VehicleObjectData->HitMap;
							}
							else
							{
								TempObj = VehicleObjectData->PlaneObject;
							}

							if(AirfoilComponentData->rflapSoundId > 0)
							{
#if defined(USE_OSG)
                                fxSoundStopSound(AirfoilComponentData->rflapSoundId);
#else
								fxSndStopSound(AirfoilComponentData->rflapSoundId);
#endif
								AirfoilComponentData->rflapSoundId = 0;
							}

#if defined(USE_OSG)
                            osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                            osg::Group* o = ((osg::Node*)TempObj->osgNode)->asGroup();

                            int id = fxSoundPlayObjectSound((char*)RightSlatSound,
                                                            pos,
                                                            finite_sound,
                                                            sound_high,
                                                            20.0f,
                                                            100.0f,
                                                            o);

                            AirfoilComponentData->rflapSoundId = id;
#else
                            SINT32 id = fxSndGetNewSound();

                            if(id > 0)
                            {
                                fxSndSetSoundName(id, (SCHAR*)RightSlatSound);
                                fxSndSetPriority(id, IDE_FP_PRIORITY_HIGH);
                                fxSndSetSoundType(id, TYPE_FINITE_SOUND);
                                fxSndSetVolume(id, Volume);
                                fxSndSetPitch(id, 1.0f);
                                fxSndSetPosition(id,
                                                 Component->OriginalRelativePos.x,
                                                 Component->OriginalRelativePos.y,
                                                 Component->OriginalRelativePos.z);
                                fxSndSetEmitterObject(id, TempObj);
                                fxSndSetPlayType(id, DEF_PLAYTYPE_OBJECT);
                                fxSndSetMaxDistance(id, 100.0f);
                                fxSndSetMinDistance(id, 20.0f);

                                fxSndInitSound(id);
								AirfoilComponentData->rflapSoundId = id;
                            }
#endif
						}
					}
				}
			}
		}
		AirfoilComponentData->SlotIsDeployed = TempNum;

		if (AirfoilComponentData->SlotIsDeployed < 0.0)
		{
			AirfoilComponentData->SlotIsDeployed = 0.0;
		}
		if (AirfoilComponentData->SlotIsDeployed > 1.0)
		{
			AirfoilComponentData->SlotIsDeployed = 1.0;
		}


		if (AirfoilComponentData->SlotIsDeployed > 0.001)
		{
			AirfoilData = AirfoilComponentData->SlotAirfoilData;
		}
	}

// handle deflected airfoils here
	if (AirfoilComponentData->Subtype == ML_ELEVATOR_SECTION ||
		AirfoilComponentData->Subtype == ML_RUDDER_SECTION ||
		AirfoilComponentData->Subtype == ML_AILERON_SECTION)
	{
		// determine control type
		ControlInput = 0.0;

		switch(AirfoilComponentData->Subtype)
		{
			case ML_ELEVATOR_SECTION:
			case ML_VORTEX_POINT_ELEVATOR:
				ControlInput = ElevPos;
				break;
			case ML_RUDDER_SECTION:
			case ML_VORTEX_POINT_RUDDER:
				ControlInput = -RudderPos;
				break;
			case ML_AILERON_SECTION:
			case ML_VORTEX_POINT_AILERON:
				ControlInput = -AileronPos;
				break;
			default:
				break;
		}

		if (ControlInput < 0.0)
		{
			mlInterpolate(&DeflectionAngle,0.0,AirfoilComponentData->ControlMin,fabs(ControlInput));
		}
		else
		{
			mlInterpolate(&DeflectionAngle,0.0,AirfoilComponentData->ControlMax,fabs(ControlInput));
		}

		DeflectionAngle /= 10.0;


		// positive deflection is surface* down*
		if (DeflectionAngle > 0.0)
		{
			AlphaData1 = AirfoilData->NeutralAlphaData;
			AlphaData2 = AirfoilData->Down10DegAlphaData;
		}
		else
		{
			AlphaData1 = AirfoilData->NeutralAlphaData;
			AlphaData2 = AirfoilData->Up10DegAlphaData;
		}

		DeflectionAngle = fabs(DeflectionAngle);

	}
	else
	{
		// just use standard undeflected airfoil
		AlphaData1 = AirfoilData->NeutralAlphaData;
		AlphaData2 = AirfoilData->NeutralAlphaData;
		DeflectionAngle = 0;
	}


	_GetIndecies(&IntAoAIndex1,&IntAoAIndex2,AoAindex);

	InterpolationDist = AoAindex - (IntAoAIndex1 - 180.0);


	mlInterpolate(&TempNum1,
				 AlphaData1[IntAoAIndex1].LiftCoefficient,
				 AlphaData2[IntAoAIndex1].LiftCoefficient,
				 DeflectionAngle);
	mlInterpolate(&TempNum2,
				 AlphaData1[IntAoAIndex2].LiftCoefficient,
				 AlphaData2[IntAoAIndex2].LiftCoefficient,
				 DeflectionAngle);
	mlInterpolate(Cl,TempNum1,TempNum2,InterpolationDist);

	mlInterpolate(&TempNum1,
				 AlphaData1[IntAoAIndex1].DragCoefficient,
				 AlphaData2[IntAoAIndex1].DragCoefficient,
				 DeflectionAngle);
	mlInterpolate(&TempNum2,
				 AlphaData1[IntAoAIndex2].DragCoefficient,
				 AlphaData2[IntAoAIndex2].DragCoefficient,
				 DeflectionAngle);
	mlInterpolate(Cd,TempNum1,TempNum2,InterpolationDist);

	mlInterpolate(&TempNum1,
				 AlphaData1[IntAoAIndex1].MomentCoefficient,
				 AlphaData2[IntAoAIndex1].MomentCoefficient,
				 DeflectionAngle);
	mlInterpolate(&TempNum2,
				 AlphaData1[IntAoAIndex2].MomentCoefficient,
				 AlphaData2[IntAoAIndex2].MomentCoefficient,
				 DeflectionAngle);
	mlInterpolate(Cm,TempNum1,TempNum2,InterpolationDist);

	_AdjustClCdCmForFlaps(Cl,Cd,Cm,AoAindex,AirfoilComponentData,FlapPosition,FlapDamage);

	_AdjustClCdCmForStall(Cl,Cd,Cm,StallAmount,ClMovement,AoAindex,AirfoilComponentData);

	return 0;
}

UINT32 mlFoilsGetBaseClCdCm(FLOAT64* Cl,FLOAT64* Cd,FLOAT64* Cm,FLOAT64 Angle, mlAirfoilData * AirfoilData)
{
FLOAT64 AoAindex;
FLOAT64 InterpolationDist;
SINT32 IntAoAIndex1;
SINT32 IntAoAIndex2;
mlAirfoilAlpha* AlphaData1;
FLOAT64 TempNum1,TempNum2;

	AoAindex = mlRAD_TO_DEG(Angle);
	_GetIndecies(&IntAoAIndex1,&IntAoAIndex2,AoAindex);
	InterpolationDist = AoAindex - (IntAoAIndex1 - 180.0);

	AlphaData1 = AirfoilData->NeutralAlphaData;

	TempNum1 = AlphaData1[IntAoAIndex1].LiftCoefficient;
	TempNum2 = AlphaData1[IntAoAIndex2].LiftCoefficient;
	mlInterpolate(Cl,TempNum1,TempNum2,InterpolationDist);

	TempNum1 = AlphaData1[IntAoAIndex1].DragCoefficient;
	TempNum2 = AlphaData1[IntAoAIndex2].DragCoefficient;
	mlInterpolate(Cd,TempNum1,TempNum2,InterpolationDist);

	TempNum1 = AlphaData1[IntAoAIndex1].MomentCoefficient;
	TempNum2 = AlphaData1[IntAoAIndex2].MomentCoefficient;
	mlInterpolate(Cm,TempNum1,TempNum2,InterpolationDist);

	return 0;
}



	// it is assumed that the airfoil position is on the unstalled Cl location
	// and that this is 25% of the chord.  When the wing fully stalls
	// it goes to 40% of the chord, so we return a ClMovement of 25% back
	// scaled by the degree of stall
	// moving "back" is negative for ClMovement

static UINT32 _AdjustClCdCmForStall(FLOAT64 * Cl,
							 FLOAT64 * Cd,
							 FLOAT64 * Cm,
							 FLOAT64 * StallAmount,
							 FLOAT64 * ClMovement,
							 FLOAT64 AoAindex,
							 mlAirfoilComponentData * AirfoilComponentData)
{
FLOAT64 AoA;
FLOAT64 BuffetAmount;
FLOAT64 Magnitude;
mlAirfoilData * AirfoilData;
FLOAT64 TempNum;

	*StallAmount = 0.0;
	AirfoilData = AirfoilComponentData->AirfoilData;

	// do stall buffet if appropriate

	AoA = AoAindex;

	if (AoA > 180.0)
	{
		AoA -= 360.0;
	}
	if (AoA < -180.0)
	{
		AoA += 360.0;
	}

	BuffetAmount = 0.0;
	*ClMovement = 0.0;
	Magnitude = 0.0;
	if (AoA < 90.0 && AoA > -90.0)
	{
		if (AoA > 0.0 && AoA < 90.0)
		{
			if (AoA > AirfoilData->PosAlphaFullStallBuffetAoA)
			{
				BuffetAmount = (double)rand() / (double)RAND_MAX * 2.0 - 1.0;

				Magnitude = (AirfoilData->PosAlphaFullStallBuffetAoA -
								 AirfoilData->PosAlphaStartStallBuffetAoA) /
								 (AoA - AirfoilData->PosAlphaFullStallBuffetAoA);

				Magnitude = (1.0 - Magnitude);
				Magnitude = Magnitude * 2.0 - 1.0;
				if (Magnitude < -1.0)
				{
					Magnitude = -1.0;
				}

			}
			else
			{
				if (AoA > AirfoilData->PosAlphaStartStallBuffetAoA)
				{
					Magnitude = (AoA - AirfoilData->PosAlphaStartStallBuffetAoA) /
								(AirfoilData->PosAlphaFullStallBuffetAoA -
								 AirfoilData->PosAlphaStartStallBuffetAoA);

					if (Magnitude < -1.0)
					{
						Magnitude = -1.0;
					}
					BuffetAmount = (double)rand() / (double)RAND_MAX * 2.0 - 1.0;
					BuffetAmount *= Magnitude;

				}

				// otherwise no stall buffet
			}
		}
		else
		{
			if (AoA < AirfoilData->NegAlphaFullStallBuffetAoA)
			{
				BuffetAmount = (double)rand() / (double)RAND_MAX * 2.0 - 1.0;

				Magnitude = (AirfoilData->NegAlphaFullStallBuffetAoA -
								 AirfoilData->NegAlphaStartStallBuffetAoA) /
								 (AoA - AirfoilData->NegAlphaFullStallBuffetAoA);

				Magnitude = (1.0 - Magnitude);
				Magnitude = Magnitude * 2.0 - 1.0;
				if (Magnitude < -1.0)
				{
					Magnitude = -1.0;
				}
			}
			else
			{
				if (AoA < AirfoilData->NegAlphaStartStallBuffetAoA)
				{
					Magnitude = (AoA - AirfoilData->NegAlphaStartStallBuffetAoA) /
								(AirfoilData->NegAlphaFullStallBuffetAoA -
								 AirfoilData->NegAlphaStartStallBuffetAoA);

					if (Magnitude < -1.0)
					{
						Magnitude = -1.0;
					}
					BuffetAmount = (double)rand() / (double)RAND_MAX * 2.0 - 1.0;
					BuffetAmount *= Magnitude;

				}

				 // otherwise no stall buffet
			}

		}
	}


	// do cl movement for excessive stall
	*ClMovement = 0.0;
	if (AoA > 90.0 || AoA < -90.0)
	{
		*ClMovement = AirfoilComponentData->WingChord * 0.25;
	}
	else
	{
		if (AoA > AirfoilData->PosAlphaFullStallBuffetAoA)
		{
			TempNum = 1.0 - (90.0 - AoA) / (90.0 - AirfoilData->PosAlphaFullStallBuffetAoA);
			*ClMovement = AirfoilComponentData->WingChord * 0.25 * TempNum;
		}
		if (AoA < AirfoilData->NegAlphaFullStallBuffetAoA)
		{
			TempNum = 1.0 - (-90.0 - AoA) / (-90.0 - AirfoilData->PosAlphaFullStallBuffetAoA);
			*ClMovement = AirfoilComponentData->WingChord * 0.25 * TempNum;
		}


	}

	*StallAmount = fabs(Magnitude);
	if (*StallAmount > 1.0)
	{
		*StallAmount = 1.0;
	}
	*Cl += BuffetAmount * _BUFFET_CP;
	*Cd += BuffetAmount * _BUFFET_CD;
	*Cm += BuffetAmount * _BUFFET_CM;
	*ClMovement += BuffetAmount * _BUFFET_CL_MOVEMENT;

	return 0;
}

static UINT32 _GetIndecies(SINT32* IntAoAIndex1, SINT32* IntAoAIndex2, FLOAT64 AoAindex)
{
	*IntAoAIndex1 = (SINT32)(floor(AoAindex));
	*IntAoAIndex2 = *IntAoAIndex1 + 1;

	if (*IntAoAIndex1 < -180)
	{
		*IntAoAIndex1 += 360;
	}
	if (*IntAoAIndex1 > 179)
	{
		*IntAoAIndex1 -= 360;
	}
	if (*IntAoAIndex2 < -180)
	{
		*IntAoAIndex2 += 360;
	}
	if (*IntAoAIndex2 > 179)
	{
		*IntAoAIndex2 -= 360;
	}
	*IntAoAIndex1 += 180;
	*IntAoAIndex2 += 180;

	return 0;
}

//// not reliable, AoA on a normally stable plane tends to diverge to stall

// rewritten to do AoA for planform calculations instead
static UINT32 _AdjustAoAForAspectRatio(FLOAT64* AoAindex,
									   mlAirfoilComponentData * AirfoilComponentData,
									   FLOAT64 AirfoilXPos,
									   FLOAT64 GroundEffect,
									   mlComponent * Component)
{
FLOAT64 Width;
FLOAT64 MinY,MaxY;
FLOAT64 TempNum,TempNum2,TempNum3;
FLOAT64 ChordIncrement;
FLOAT64 Accumulator;
FLOAT64 WingArea;
UINT32 NumDivs;
UINT32 i;
UINT32 AbleToComputeCLMult;

	// first check if we have to calculate clmult
	if (AirfoilComponentData->CLMultCalculated == 0)
	{
		AbleToComputeCLMult = 0;
		AirfoilComponentData->CLMult = 1.0;
		if (AirfoilComponentData->WingChord > 0.001 &&
			AirfoilComponentData->WingSpan > 0.001 &&
			AirfoilComponentData->AspectRatio > 0.001 &&
			AirfoilComponentData->InnerChord > 0.001)
		{
			Width = AirfoilComponentData->Area / AirfoilComponentData->WingChord;
			if (Width > 0.001)
			{
				TempNum = AirfoilXPos;
				if (TempNum < 0.0)
				{
					TempNum = -TempNum;
				}
				MinY = TempNum - Width / 2.0;
				MaxY = TempNum + Width / 2.0;
				if (MinY < 0.0)
				{
					MinY = 0.0;
				}
				if (MaxY > AirfoilComponentData->WingSpan / 2)
				{
					MaxY = AirfoilComponentData->WingSpan / 2;
				}

				if (MinY < MaxY)
				{
					NumDivs = (UINT32)((MaxY - MinY) / 0.01 + 0.5);
					if (NumDivs == 0)
					{
						NumDivs = 1;
					}

					WingArea = AirfoilComponentData->WingSpan * AirfoilComponentData->WingSpan /
								AirfoilComponentData->AspectRatio;

					ChordIncrement = (AirfoilComponentData->OuterChord - AirfoilComponentData->InnerChord) /
										NumDivs;

					TempNum = MinY;
					TempNum3 = AirfoilComponentData->InnerChord;
					Accumulator = 0.0;
					for (i=0;i<NumDivs;i++,TempNum += 0.01)
					{
						TempNum2 =
							0.5 * (1 + (4 * WingArea / PI /
											AirfoilComponentData->WingSpan /
											TempNum3) *
										sqrt(1 - (2 * TempNum / AirfoilComponentData->WingSpan) *
												 (2 * TempNum / AirfoilComponentData->WingSpan)));
						Accumulator += TempNum2;
						TempNum3 += ChordIncrement;
					}
					AirfoilComponentData->CLMult = Accumulator / NumDivs;
					AbleToComputeCLMult = 1;
				}
			}

		}
		if (AbleToComputeCLMult != 1)
		{
			Write_Debug("Unable to compute CLMult for wing section %s",
						Component->ComponentName);
		}
		else
		{
			Write_Debug("Component %s CLMult = %lf",Component->ComponentName,AirfoilComponentData->CLMult);
		}

		AirfoilComponentData->CLMultCalculated = 1;
	}

	if (AirfoilComponentData->CLMult > 0.001)
	{
		TempNum = AirfoilComponentData->CLMult;

		// factor in ground effect
		*AoAindex = (GroundEffect * *AoAindex) +
					((1.0 - GroundEffect) * TempNum * *AoAindex);
	}

	return 0;
}

#if(0)
static UINT32 _GetAlphaAdjust(FLOAT64 * DeltaAoA,
					   FLOAT64 AoAindexInput,
					   mlAirfoilComponentData* AirfoilComponentData)
{
	mlAirfoilComponentData * DownwashAirfoilData;
	FLOAT64 Angle;

	*DeltaAoA = 0;

	if (AirfoilComponentData == NULL)
	{
		return 1;
	}

	if (AirfoilComponentData->DownwashComponent == NULL)
	{
		return 1;
	}

	if (AirfoilComponentData->DownwashComponent->SpecialData == NULL)
	{
		return 1;
	}

	DownwashAirfoilData = (mlAirfoilComponentData*)
				AirfoilComponentData->DownwashComponent->SpecialData;

	if (AoAindexInput > AirfoilComponentData->DownwashMinAoA &&
		AoAindexInput < AirfoilComponentData->DownwashMaxAoA)
	{
		Angle = atan2(DownwashAirfoilData->DownwashDownVelocity *
						AirfoilComponentData->DownWashMult,
				      DownwashAirfoilData->DownwashStraightVelocity);

		if (AirfoilComponentData->DownwashPercent != 0.0)
		{
			Angle *= AirfoilComponentData->DownwashPercent;
		}

		*DeltaAoA = -mlRAD_TO_DEG(Angle);

	}

	return 0;

	return 0;
}
#endif

// the basic principal of this is that there are two boxes for each
// blanking attitude regime.  The inner one is "complete" blanking, the outer one
// is the start of blanking.  Think of how an eclipse of the moon works.  Things
// get darker while the sun is only partially eclipsed until it's fully eclipsed.
// Blanking works the same way
static UINT32 _DetermineIfBlanked(FLOAT64* Blanked,
						   mlAirfoilComponentData* AirfoilComponentData,
						   FLOAT64VECTOR* ComponentVel)
{
FLOAT64ATT VelAtt;
FLOAT64 Blanking;

FLOAT64 PitchBlankStart;
FLOAT64 PitchBlankStartFull;
FLOAT64 PitchBlankEndFull;
FLOAT64 PitchBlankEnd;
FLOAT64 YawBlankStart;
FLOAT64 YawBlankStartFull;
FLOAT64 YawBlankEndFull;
FLOAT64 YawBlankEnd;
FLOAT64 TempNum;
UINT32 i;

	Blanking = 0.0;

	VelAtt.Pitch = -atan2(ComponentVel->z,ComponentVel->y);
	TempNum = sqrt(ComponentVel->y*ComponentVel->y+ComponentVel->z*ComponentVel->z);

	VelAtt.Yaw = atan2(ComponentVel->x,TempNum);

	VelAtt.Pitch = mlRAD_TO_DEG(-VelAtt.Pitch);
	VelAtt.Yaw = mlRAD_TO_DEG(-VelAtt.Yaw);
	VelAtt.Roll = 0;

	if (VelAtt.Pitch > 90.0)
	{
		VelAtt.Pitch -= 180.0;

		if (VelAtt.Yaw > 0)
		{
			VelAtt.Yaw = 180.0 - VelAtt.Yaw;
		}
		else
		{
			VelAtt.Yaw = -180.0 - VelAtt.Yaw;
		}
	}

	if (VelAtt.Pitch < -90.0)
	{
		VelAtt.Pitch += 180.0;

		if (VelAtt.Yaw > 0)
		{
			VelAtt.Yaw = 180.0 - VelAtt.Yaw;
		}
		else
		{
			VelAtt.Yaw = -180.0 - VelAtt.Yaw;
		}
	}

	for (i=0;i<ML_MAX_BLANKING_AREAS;i++)
	{
		PitchBlankStart = AirfoilComponentData->BlankPitchStart[i];
		PitchBlankStartFull = AirfoilComponentData->BlankPitchStartFull[i];
		PitchBlankEndFull = AirfoilComponentData->BlankPitchEndFull[i];
		PitchBlankEnd = AirfoilComponentData->BlankPitchEnd[i];


		if (PitchBlankStart > PitchBlankEnd)
		{
			TempNum = PitchBlankStart;
			PitchBlankStart = PitchBlankEnd;
			PitchBlankEnd = TempNum;

			TempNum = PitchBlankStartFull;
			PitchBlankStartFull = PitchBlankEndFull;
			PitchBlankEndFull = TempNum;
		}

		YawBlankStart = AirfoilComponentData->BlankYawStart[i];
		YawBlankStartFull = AirfoilComponentData->BlankYawStartFull[i];
		YawBlankEndFull = AirfoilComponentData->BlankYawEndFull[i];
		YawBlankEnd = AirfoilComponentData->BlankYawEnd[i];

		if (YawBlankStart > YawBlankEnd)
		{
			TempNum = YawBlankStart;
			YawBlankStart = YawBlankEnd;
			YawBlankEnd = TempNum;

			TempNum = YawBlankStartFull;
			YawBlankStartFull = YawBlankEndFull;
			YawBlankEndFull = TempNum;
		}

		if (PitchBlankStart == 0.0 &&
			PitchBlankStartFull == 0.0 &&
			PitchBlankEndFull == 0.0 &&
			PitchBlankEnd == 0.0 &&
			YawBlankStart == 0.0 &&
			YawBlankStartFull == 0.0 &&
			YawBlankEndFull == 0.0 &&
			YawBlankEnd == 0.0)
		{
			continue;
		}

		// first handle complete blanking

		if ((VelAtt.Pitch <= PitchBlankEndFull && VelAtt.Pitch >= PitchBlankStartFull) &&
			(VelAtt.Yaw <= YawBlankEndFull && VelAtt.Yaw >= YawBlankStartFull))
		{
			Blanking = 1.0;
			continue;
		}

		if (!(VelAtt.Pitch > PitchBlankEnd || VelAtt.Pitch < PitchBlankStart ||
			  VelAtt.Yaw > YawBlankEnd || VelAtt.Yaw < YawBlankStart))
		{
			if (VelAtt.Pitch > PitchBlankStartFull && VelAtt.Pitch < PitchBlankEndFull)
			{
				if (VelAtt.Yaw < YawBlankStartFull)
				{
					TempNum = (VelAtt.Yaw - YawBlankStart) /
						      (YawBlankStartFull - YawBlankStart);

					if (Blanking < TempNum)
					{
						Blanking = TempNum;
					}
				}
				else
				{
					TempNum = (YawBlankEnd - VelAtt.Yaw) /
							  (YawBlankEnd - YawBlankEndFull);

					if (Blanking < TempNum)
					{
						Blanking = TempNum;
					}

				}

			}

			if (VelAtt.Yaw > YawBlankStartFull && VelAtt.Yaw < YawBlankEndFull)
			{
				if (VelAtt.Pitch < PitchBlankStartFull)
				{
					TempNum = (VelAtt.Pitch - PitchBlankStart) /
						      (PitchBlankStartFull - PitchBlankStart);

					if (Blanking < TempNum)
					{
						Blanking = TempNum;
					}

				}
				else
				{
					TempNum = (PitchBlankEnd - VelAtt.Pitch) /
							  (PitchBlankEnd - PitchBlankEndFull);

					if (Blanking < TempNum)
					{
						Blanking = TempNum;
					}

				}

			}


		}
	}

	*Blanked = Blanking;


	return 0;
}

// note: this routine is only good to about mach 0.95
static UINT32 _AdjustClCdCmForCompressibility(FLOAT64 * Cl,
									   FLOAT64 * Cd,
									   FLOAT64 * Cm,
									   FLOAT64 * ClMovement,
									   mlAirfoilData * AirfoilData,
									   FLOAT64 AoA,
									   FLOAT64 Speed,
									   FLOAT64 SpeedOfSound,
									   mlAirfoilComponentData * AirfoilComponentData)
{
FLOAT64 MachNum;
UINT32 Index1,Index2;
FLOAT64 TempNum;
FLOAT64 Delta;
FLOAT64 UseAoA;
FLOAT64 Mdiv;

	if (SpeedOfSound == 0.0)
	{
		MachNum = 0.0;
	}
	else
	{
		MachNum = fabs(Speed / SpeedOfSound);
	}

	// first Cl
	if (MachNum > ML_HIGH_MACHNUM_FOR_MACHEFFECTS)
	{
		*Cl *= _MachEffectsOnCoefficientOfLift[ML_MACHEFFECTS_NUM_STEPS-1];
	}
	else
	{
		Index1 = (UINT32)(MachNum / ML_MACHNUM_STEP);
		Index2 = Index1 + 1;

		Delta = (MachNum / ML_MACHNUM_STEP) - (double)(Index1);

		mlInterpolate(&TempNum,
					 _MachEffectsOnCoefficientOfLift[Index1],
					 _MachEffectsOnCoefficientOfLift[Index2],
					 Delta);

		*Cl *= TempNum;
	}

	// now Cd

	// calc Mdiv
	UseAoA = AoA;
	if (UseAoA > AirfoilData->PosAlphaStartStallBuffetAoA)
	{
		UseAoA = AirfoilData->PosAlphaStartStallBuffetAoA;
	}

	if (UseAoA < AirfoilData->NegAlphaStartStallBuffetAoA)
	{
		UseAoA = AirfoilData->NegAlphaStartStallBuffetAoA;
	}

	if (UseAoA < 0.0)
	{
		Mdiv = AirfoilData->MdivNeg10DegAoA - AirfoilData->MdivNeutral;
		Mdiv /= 10.0;
		Mdiv *= -UseAoA;
		Mdiv = AirfoilData->MdivNeutral + Mdiv;
	}
	else
	{
		Mdiv = AirfoilData->MdivPos10DegAoA - AirfoilData->MdivNeutral;
		Mdiv /= 10.0;
		Mdiv *= UseAoA;
		Mdiv = AirfoilData->MdivNeutral + Mdiv;
	}
	*ClMovement = 0.0;
	if (MachNum > Mdiv)
	{
		// figure out Cd change
		if (MachNum <= 1.0)
		{
			Delta = 1.0 - Mdiv;
			TempNum = (MachNum - Mdiv) / Delta;
			Index1 = (UINT32)(TempNum * ML_MACHEFFECTS_NUM_STEPS);
			Index2 = Index1 + 1;

			if (Index2 <= ML_MACHEFFECTS_NUM_STEPS)
			{
				*Cd *= mlMachEffectsOnCoefficientOfDrag[ML_MACHEFFECTS_NUM_STEPS-1];
			}
			else
			{

				Delta = TempNum * ML_MACHEFFECTS_NUM_STEPS - (FLOAT64)Index1;

				mlInterpolate(&TempNum,
					 mlMachEffectsOnCoefficientOfDrag[Index1],
					 mlMachEffectsOnCoefficientOfDrag[Index2],
					 Delta);

				*Cd *= TempNum;
			}
			TempNum = 1.0 - (1.0 - MachNum) / (1.0 - Mdiv);
			*ClMovement = TempNum * AirfoilData->CompressClMovement *
							AirfoilComponentData->WingChord;
		}
		else
		{
			*Cd *= mlMachEffectsOnCoefficientOfDrag[ML_MACHEFFECTS_NUM_STEPS-1];
			*ClMovement = AirfoilData->CompressClMovement * AirfoilComponentData->WingChord;
		}
	}

	return 0;
}

static UINT32	_AdjustClCdCmForFlaps(FLOAT64 * Cl,
									  FLOAT64 * Cd,
									  FLOAT64 * Cm,
									  FLOAT64 AoAindex,
									  mlAirfoilComponentData* AirfoilComponentData,
									  FLOAT64 FlapPosition,
									  FLOAT64 FlapDamage)
{
FLOAT64 FlapCl,FlapCd,FlapCm;
FLOAT64 UseFlapPosition;
UINT32 Index1,Index2;
FLOAT64 Delta;
mlAirfoilData * AirfoilData;
FLOAT64 Magnitude;

	AirfoilData = AirfoilComponentData->AirfoilData;

	UseFlapPosition = FlapPosition;

	if (FlapPosition > 4.0)
	{
		UseFlapPosition = 4.0;
	}
	if (FlapPosition < 0.0)
	{
		UseFlapPosition = 0.0;
	}

	if (UseFlapPosition >= 0.0 && UseFlapPosition < 1.0)
	{
		Index1 = 0;
		Index2 = 1;
		Delta = UseFlapPosition;
	}

	if (UseFlapPosition >= 1.0 && UseFlapPosition < 2.0)
	{
		Index1 = 1;
		Index2 = 2;
		Delta = UseFlapPosition - 1.0;
	}

	if (UseFlapPosition >= 2.0 && UseFlapPosition < 3.0)
	{
		Index1 = 2;
		Index2 = 3;
		Delta = UseFlapPosition - 2.0;
	}

	if (UseFlapPosition >= 3.0 && UseFlapPosition <= 4.0)
	{
		Index1 = 3;
		Index2 = 4;
		Delta = UseFlapPosition - 3.0;
	}

	mlInterpolate(&FlapCl,
				 AirfoilComponentData->FlapCl[Index1],
				 AirfoilComponentData->FlapCl[Index2],
				 Delta);
	mlInterpolate(&FlapCd,
				 AirfoilComponentData->FlapCd[Index1],
				 AirfoilComponentData->FlapCd[Index2],
				 Delta);
	mlInterpolate(&FlapCm,
				 AirfoilComponentData->FlapCm[Index1],
				 AirfoilComponentData->FlapCm[Index2],
				 Delta);


	// now that we have the flap cl, cd, cm, add it in based on stall state

	if (AoAindex > AirfoilData->NegAlphaStartStallBuffetAoA &&
		AoAindex < AirfoilData->PosAlphaStartStallBuffetAoA)
	{
		// no stall, add the cl, cd, cm straight in
		*Cl += FlapCl * (1.0 - FlapDamage * 0.7);
		*Cd += FlapCd + 0.2 * (FlapDamage);
		*Cm += FlapCm * (1.0 - FlapDamage * 0.7);

	}
	else
	{
		if (AoAindex > AirfoilData->NegAlphaFullStallBuffetAoA &&
			AoAindex <= AirfoilData->NegAlphaStartStallBuffetAoA)
		{
			Magnitude = (AoAindex - AirfoilData->NegAlphaFullStallBuffetAoA) /
			    (AirfoilData->NegAlphaStartStallBuffetAoA - AirfoilData->NegAlphaFullStallBuffetAoA);

			*Cl += FlapCl * Magnitude * (1.0 - FlapDamage * 0.7);
			*Cd += FlapCd + 0.2 * (FlapDamage);
			*Cm += FlapCm * Magnitude * (1.0 - FlapDamage * 0.7);
		}

		if (AoAindex >= AirfoilData->PosAlphaStartStallBuffetAoA &&
			AoAindex < AirfoilData->PosAlphaFullStallBuffetAoA)
		{
			Magnitude = (AoAindex - AirfoilData->PosAlphaFullStallBuffetAoA) /
				(AirfoilData->PosAlphaStartStallBuffetAoA - AirfoilData->PosAlphaFullStallBuffetAoA);

			*Cl += FlapCl * Magnitude * (1.0 - FlapDamage * 0.7);
			*Cd += FlapCd + 0.2 * (FlapDamage);
			*Cm += FlapCm * Magnitude * (1.0 - FlapDamage * 0.7);

		}

	}

	return 0;
}

