#pragma once

#define ML_MAX_FLAP_POSITIONS 4
#define ML_MAX_NUM_REYNOLDS 2
#define ML_DEGREES 360

#define ML_MACHEFFECTS_NUM_STEPS 21
#define ML_LOW_MACHNUM_FOR_MACHEFFECTS 0.0
#define ML_HIGH_MACHNUM_FOR_MACHEFFECTS 1.00
#define ML_MACHNUM_STEP 0.05

#define ML_MAX_BLANKING_AREAS 3
#define ML_MAX_FLAPS 5 // neutral + 4 positions

enum
{
	ML_NO_STALL_BUFFET,		 	// smooth flow over wing
	ML_TICKLE_STALL_BUFFET,		// non-chaotic "turbulent" flow over wing
	ML_FULL_STALL_BUFFET		// full blown chaotic flow over wing
};

// what airfoil type this is
// note that vert/horiz stabs and rudders/elevators are treated as
// wings with ailerons, with differing control mechanisms
enum
{
	ML_NO_WINGSECTION,
	ML_NORMAL_WINGSECTION,
	ML_AILERON_SECTION,
	ML_ELEVATOR_SECTION,
	ML_RUDDER_SECTION,
	ML_FUSELAGE_SECTION,
	ML_VORTEX_POINT,
	ML_FLAP_SECTION,
	ML_VORTEX_POINT_AILERON,
	ML_VORTEX_POINT_ELEVATOR,
	ML_VORTEX_POINT_RUDDER,
	ML_GEARDRAG_SECTION,
	ML_CANOPYDRAG_SECTION,
	ML_DEPLOYDRAG_SECTION,
	ML_MAX_WINGSECTION_TYPES
};

enum
{
	ML_AIRFOIL_VARNAME_NONE,
	ML_AIRFOIL_VARNAME_BASEAIRFOIL,
	ML_AIRFOIL_VARNAME_UP10DEGAIRFOIL,
	ML_AIRFOIL_VARNAME_DOWN10DEGAIRFOIL,
	ML_AIRFOIL_VARNAME_STALLFILENAME,
	ML_AIRFOIL_VARNAME_COMPRESSIBILITYFILENAME,
	ML_AIRFOIL_MAX_VARNAME
};

enum
{
	ML_STALLFILE_VARNAME_NONE,
	ML_STALLFILE_VARNAME_POSAOASTARTALPHA,
	ML_STALLFILE_VARNAME_POSAOAFULLALPHA,
	ML_STALLFILE_VARNAME_NEGAOASTARTALPHA,
	ML_STALLFILE_VARNAME_NEGAOAFULLALPHA,
	ML_STALLFILE_VARNAME_CLMOVEMENT,
	ML_STALLFILE_MAX_VARNAME
};

enum
{
	ML_MDIV_NONE,
	ML_MDIV_NEUTRAL_AOA,
	ML_MDIV_POS10DEG_AOA,
	ML_MDIV_NEG10DEG_AOA,
	ML_MDIV_COMPRESS_CL_MOVEMENT,
	ML_MDIV_MAX_VARNAME
};

enum
{
	ML_VARNAME_SPECIAL_NONE,
	ML_VARNAME_SPECIAL_AIRFOILTYPE,
	ML_VARNAME_SPECIAL_AREA,
	ML_VARNAME_SPECIAL_WINGCHORD,
	ML_VARNAME_SPECIAL_ASPECTRATIO,
	ML_VARNAME_SPECIAL_CONTROLMIN,
	ML_VARNAME_SPECIAL_CONTROLMAX,
	ML_VARNAME_SPECIAL_IDRAGAREA,
	ML_VARNAME_SPECIAL_IDRAGK,
	ML_VARNAME_SPECIAL_USESDOWNWASH,
	ML_VARNAME_SPECIAL_DOWNWASHCOMPONENT,
	ML_VARNAME_SPECIAL_DOWNWASHMINAOA,
	ML_VARNAME_SPECIAL_DOWNWASHMAXAOA,

	ML_VARNAME_SPECIAL_USESPOPOUTSLOTS,
	ML_VARNAME_SPECIAL_SLOTAIRFOILTYPE,
	ML_VARNAME_SPECIAL_SLOTDEPLOYMINAOA,
	ML_VARNAME_SPECIAL_SLOTDEPLOYMAXAOA,

	ML_VARNAME_SPECIAL_BLANK1_PITCH_START,
	ML_VARNAME_SPECIAL_BLANK1_PITCH_STARTFULL,
	ML_VARNAME_SPECIAL_BLANK1_PITCH_ENDFULL,
	ML_VARNAME_SPECIAL_BLANK1_PITCH_END,
	ML_VARNAME_SPECIAL_BLANK1_YAW_START,
	ML_VARNAME_SPECIAL_BLANK1_YAW_STARTFULL,
	ML_VARNAME_SPECIAL_BLANK1_YAW_ENDFULL,
	ML_VARNAME_SPECIAL_BLANK1_YAW_END,

	ML_VARNAME_SPECIAL_BLANK2_PITCH_START,
	ML_VARNAME_SPECIAL_BLANK2_PITCH_STARTFULL,
	ML_VARNAME_SPECIAL_BLANK2_PITCH_ENDFULL,
	ML_VARNAME_SPECIAL_BLANK2_PITCH_END,
	ML_VARNAME_SPECIAL_BLANK2_YAW_START,
	ML_VARNAME_SPECIAL_BLANK2_YAW_STARTFULL,
	ML_VARNAME_SPECIAL_BLANK2_YAW_ENDFULL,
	ML_VARNAME_SPECIAL_BLANK2_YAW_END,

	ML_VARNAME_SPECIAL_BLANK3_PITCH_START,
	ML_VARNAME_SPECIAL_BLANK3_PITCH_STARTFULL,
	ML_VARNAME_SPECIAL_BLANK3_PITCH_ENDFULL,
	ML_VARNAME_SPECIAL_BLANK3_PITCH_END,
	ML_VARNAME_SPECIAL_BLANK3_YAW_START,
	ML_VARNAME_SPECIAL_BLANK3_YAW_STARTFULL,
	ML_VARNAME_SPECIAL_BLANK3_YAW_ENDFULL,
	ML_VARNAME_SPECIAL_BLANK3_YAW_END,

	ML_VARNAME_SPECIAL_FLAPPOS1_CL,
	ML_VARNAME_SPECIAL_FLAPPOS1_CD,
	ML_VARNAME_SPECIAL_FLAPPOS1_CM,
	ML_VARNAME_SPECIAL_FLAPPOS2_CL,
	ML_VARNAME_SPECIAL_FLAPPOS2_CD,
	ML_VARNAME_SPECIAL_FLAPPOS2_CM,
	ML_VARNAME_SPECIAL_FLAPPOS3_CL,
	ML_VARNAME_SPECIAL_FLAPPOS3_CD,
	ML_VARNAME_SPECIAL_FLAPPOS3_CM,
	ML_VARNAME_SPECIAL_FLAPPOS4_CL,
	ML_VARNAME_SPECIAL_FLAPPOS4_CD,
	ML_VARNAME_SPECIAL_FLAPPOS4_CM,

	ML_VARNAME_SPECIAL_VORTEX_COMPONENT,
	ML_VARNAME_SPECIAL_VORTEX_EFFECT_PERCENT,
	ML_VARNAME_SPECIAL_VORTEX_EFFECT_OFFSET,
	ML_VARNAME_SPECIAL_DOWNWASHPERCENT,
	ML_VARNAME_SPECIAL_STALLSOUND,
	ML_VARNAME_WINGTWIST_COEEFFICIENT,
	ML_VARNAME_INDUCEDDRAG_CLCOMPONENT,

	ML_VARNAME_SPECIAL_SLOTCLOSEAOA,
	ML_VARNAME_SPECIAL_SLOTDEPLOYTIME,

	ML_VARNAME_SPECIAL_WINGSPAN,

	ML_VARNAME_SPECIAL_HORIZSTABTRIMDEGUP,
	ML_VARNAME_SPECIAL_HORIZSTABTRIMDEGDOWN,

	ML_VARNAME_SPECIAL_DOWNWASHMULT,

	ML_VARNAME_SPECIAL_INNERCHORD,
	ML_VARNAME_SPECIAL_OUTERCHORD,

	ML_VARNAME_DIVEBRAKE_AREA,
	ML_VARNAME_DIVEBRAKE_ANGLE,

	ML_VARNAME_INDUCEDDRAGLIFT_1,
	ML_VARNAME_INDUCEDDRAGLIFT_2,
	ML_VARNAME_INDUCEDDRAGLIFT_3,
	ML_VARNAME_INDUCEDDRAGLIFT_4,
	ML_VARNAME_INDUCEDDRAGLIFT_5,
	ML_VARNAME_INDUCEDDRAGLIFT_6,
	ML_VARNAME_INDUCEDDRAGLIFT_7,
	ML_VARNAME_INDUCEDDRAGLIFT_8,
	ML_VARNAME_INDUCEDDRAGMULT,

	ML_VARNAME_CANOPYNUM,
	ML_VARNAME_GEARNUM,
	ML_VARNAME_POSITION,

	ML_VARNAME_MAXOPENSPEED,
	ML_VARNAME_MAXCLOSESPEED,
	ML_VARNAME_MAXDAMAGESPEED,

	ML_VARNAME_SPECIAL_MAX
};

struct mlComponentList;
struct mlComponent;
struct mlObjectData;

// this structure describes the lift/drag/moment/buffet parameters for a particular alpha
// (alpha = angle of attack for the airfoil)
struct mlAirfoilAlpha
{
	FLOAT32 LiftCoefficient; // force perpendicular to relative wind
	FLOAT32 DragCoefficient; // force in direction of relative wind
	FLOAT32 MomentCoefficient; // torque moment due to center of pressure difference on airfoil
};


// since wing characteristics change for different states/reynolds numbers we
// use a multi-dimensional array for the airfoil chart
struct mlAirfoilData
{
	UCHAR AirfoilName[32];

	// each of the alpha arrays match either low or high reynold numbers
	// with all other reynold numbers interpolated
	// coefficients are interpolated depending on conditions
	mlAirfoilAlpha NeutralAlphaData[ML_DEGREES]; // normal airfoil
	mlAirfoilAlpha Up10DegAlphaData[ML_DEGREES];
	mlAirfoilAlpha Down10DegAlphaData[ML_DEGREES];

	FLOAT64 PosAlphaStartStallBuffetAoA;
	FLOAT64 PosAlphaFullStallBuffetAoA;
	FLOAT64 NegAlphaStartStallBuffetAoA;
	FLOAT64 NegAlphaFullStallBuffetAoA;
	FLOAT64 ClMovement;

	// for each airfoil there is a critical mach speed at zero AoA
	// where the mach shock wave crosses the airfoil "crest" and drag starts to spike
	// this speed decreases "linearly" with AoA up to stall
	// since the "crest" is different for top/bottom of the airfoil we treat
	// these separately.
	// next 2 arrays describe the crest Cp used in the mach shock wave calcs
	FLOAT64 MdivNeutral; // critical mach number for compressibility mach increase
	FLOAT64 MdivPos10DegAoA; // critical mach number, 5deg AoA
	FLOAT64 MdivNeg10DegAoA; // critical mach number, -5deg AoA
	FLOAT64 CompressClMovement;
};

struct mlAirfoilList
{
	UINT32 NumAirfoils;
	mlAirfoilData * AirfoilData;
};

struct mlAirfoilComponentData
{
	UINT32 Subtype;
	UCHAR AirfoilType[32];
	mlAirfoilData * AirfoilData; // used to quickreference airfoil data
	FLOAT64 Area;
	FLOAT64 WingChord;
	FLOAT64 InnerChord;
	FLOAT64 OuterChord;
	FLOAT64 AspectRatio;
	FLOAT64 WingSpan;
	FLOAT64 Width;
	FLOAT64 CLMult; // for wing planform effects
	UINT32 CLMultCalculated; // flag indicating clmult calculated
	FLOAT64 ControlMin;
	FLOAT64 ControlMax;
	FLOAT64 IDragArea;
	FLOAT64 IDragK;

	// next params are for tail surface downwash effects
	UINT32 UsesDownwashEffect;
	FLOAT64 DownwashMinAoA;
	FLOAT64 DownwashMaxAoA;
	struct mlComponent* DownwashComponent; // which component is generating downwash
	FLOAT64 DownwashDownVelocity;
	FLOAT64 DownwashStraightVelocity;
	FLOAT64 DownwashPercent;

	struct mlComponent* VortexComponent; // which component is generating propwash vortex effects
	FLOAT64 VortexEffectPercent; // percent of component affected by propwash
	FLOAT64 VortexEffectOffset; // used to calculate asymmetrical vortex effects

	// next params are for leading edge pop-out slots
	UINT32 UsesPopoutSlots;
	mlAirfoilData * SlotAirfoilData;
	FLOAT64 SlotDeployMinAoA;  // minimum slot open aoa
	FLOAT64 SlotCloseAoA; // minimum slot close aoa
	FLOAT64 SlotDeployMaxAoA; // maximum slot open aoa
	FLOAT64 SlotIsDeployed; // are slots currently deployed?
	FLOAT64 SlotDeploySpeed; // slot deployment time at 100kph
	UINT32 GoingOpenOrClosed; // is slot going open or closed? open = 1

	FLOAT64 LastTwistTorque; // used by wing twist factors
	FLOAT64 WingTwistCoefficient;

	FLOAT64 DiveBrakeArea; // equivelant flatplate area added via divebrakes
	FLOAT64 DiveBrakeAngle; // faked autodive feature (tilts the airfoil by this much)

	FLOAT64 LastCl;
	struct mlComponent* InducedDragCoefficientsComponent; // which component to get cl from

	UINT32 ControlSurfaceJammed; // is the control surface jammed?
	UINT32 ControlSurfaceFree; // is the control surface free-floating?

	// next params are for airfoil blanking effects
	//
	// blanking works like eclipses, there is a region of full blanking,
	// and a "border" around it of partial blanking
	FLOAT64 BlankYawStart[ML_MAX_BLANKING_AREAS]; // this is where blanking starts
	FLOAT64 BlankYawStartFull[ML_MAX_BLANKING_AREAS]; // now we're at full blanking
	FLOAT64 BlankYawEndFull[ML_MAX_BLANKING_AREAS]; // end of full blanking
	FLOAT64 BlankYawEnd[ML_MAX_BLANKING_AREAS]; // no more blanking

	FLOAT64 BlankPitchStart[ML_MAX_BLANKING_AREAS];
	FLOAT64 BlankPitchStartFull[ML_MAX_BLANKING_AREAS];
	FLOAT64 BlankPitchEndFull[ML_MAX_BLANKING_AREAS];
	FLOAT64 BlankPitchEnd[ML_MAX_BLANKING_AREAS];
	LOGICAL IsBlanked;

	FLOAT64 FlapCl[ML_MAX_FLAPS];
	FLOAT64 FlapCd[ML_MAX_FLAPS];
	FLOAT64 FlapCm[ML_MAX_FLAPS];

	FLOAT64 HorizStabTrimUpDeg;
	FLOAT64 HorizStabTrimDownDeg;

	UINT32 StallSound;
	SINT32  rflapSoundId;
	SINT32  lflapSoundId;

	UINT32 VortexOn;
	UINT32 VortexNum;

	struct mlComponent* InducedDragLiftComp[8];
	FLOAT64 InducedDragMult; // multiplies the effective area/induced drag by this value

	FLOAT64 DownWashMult;

	SINT32 Position; // for components tied to some sort of position
	FLOAT64 MaxDamageSpeed;
	struct mlComponent* DamageComponent; // apply damage to this component as well (only used for gear at the moment)

	FLOAT64 CenterOfLift; // percent of chord lift is applied
};

extern FLOAT64 MachEffectsOnCoefficientOfLift[ML_MACHEFFECTS_NUM_STEPS];
extern FLOAT64 mlMachEffectsOnCoefficientOfDrag[ML_MACHEFFECTS_NUM_STEPS];

/************************************************************************************************
	mlfoils
*************************************************************************************************/

extern UINT32 mlfoilsReadAirfoilFile(mlAirfoilList** AirfoilList,
									 UCHAR* AirfoilListFilename,
									 UCHAR* DirName);

extern UINT32 mlfoilsFreeAirfoilData(mlAirfoilList* AirfoilList);

extern UINT32 mlfoilSetComponentSubtype(void* SpecialData, char * SubType);

extern UINT32 mlfoilAddSpecialVariable(void* SpecialData,
										 char* VarName,
										 char* VarContents,
										 mlAirfoilList* AirfoilList,
										 struct mlComponentList* ComponentList,
										 struct mlComponent* Component,
										 void* VehicleMasterPtr);

extern UINT32 mlGetAirfoilForcesAndTorques(FLOAT64VECTOR* RetForce,
											FLOAT64ATT* RetTorque,
											FLOAT64* ClMovement,
											struct mlComponent* Component,
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
											struct OBJECT * VehicleObject,
											SINT8* LeftSlatSound,
											SINT8* RightSlatSound,
											struct mlObjectData* VehicleObjectData,
											UINT32 DiveBrakeOn);

extern UINT32 mlFoilsGetBaseClCdCm(FLOAT64* Cl,FLOAT64* Cd,FLOAT64* Cm,FLOAT64 Angle, mlAirfoilData * AirfoilData);
