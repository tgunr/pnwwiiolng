#pragma once

enum
{
	ML_NO_COMPONENT,
	ML_COMPONENT_BALLAST,
	ML_COMPONENT_AIRFOIL,
	ML_COMPONENT_ENGINE,
	ML_COMPONENT_PITOT,
	ML_COMPONENT_GEAR,
	ML_COMPONENT_WEAPONMOUNT,
	ML_COMPONENT_CREW,
	ML_COMPONENT_TOWPOINT,
	ML_COMPONENT_HAULPOINT,
	ML_COMPONENT_MAXTYPES
};

enum
{
	ML_STATUS_OK,
	ML_STATUS_OK_IF_FAIL, // only activated when damage group is destroyed
	ML_STATUS_MISSING
};

struct ptkBody;

#define MAX_DAMAGE_GROUPS 4

struct mlComponent
{
	UCHAR	ComponentName[32];
	UINT32	Type;

	UINT32 ComponentStatus;
	LOGICAL ComponentFailed;

// physical properties
	FLOAT64VECTOR 	OriginalRelativePos;
	FLOAT64VECTOR 	CurrentRelativePos;
	FLOAT64 		Mass;
	FLOAT64ATT 		RotInertia;
	FLOAT64ATT 		Attitude;

// "new" physics
	struct ptkBody*	Body;

// bouyancy stuff
	FLOAT64	Vol;
	FLOAT64	AirVolPercentage;
	LOGICAL	IsInternalMass;
	LOGICAL	IsFreeFlooding;

// physics loop
	FLOAT64VECTOR	CurrentForce;
	FLOAT64ATT 		CurrentTorque;
	FLOAT64VECTOR 	LastForce;
	FLOAT64ATT 		LastTorque;

// damage
	UINT32 	DamageComponentNum[4+6];   // the 6 extra only used by pak38, m57 and 6lb hackery DO NOT USE OTHERWISE
	FLOAT64	DamageLevels[4+6];
	FLOAT64 NewDamage[4+6];
	UINT32 	NewDamageNumHits[4+6];
	FLOAT64	PrevDamage;
	FLOAT64	CurrentDamage;
	UINT32 	CriticalPrimary;	// is component a primary critical component?
	UINT32	CriticalSecondary; 	// is component a secondary critical component?
	UINT32 	DamageGroupNum;
	FLOAT64	DamageLevelFail;	// joules to complete failure
	SCHAR	VisDamage[MAX_DAMAGE_GROUPS][32];		// visible damage name
	SCHAR	VisDamageParent[32];// visible damage parent (if one exists)
	LOGICAL	HulkIfFail;			// hulk vehicle and kill everyone if component is destroyed

// position ownership
	UINT32 	CrewPosition;
	UINT32 	OverridePos; 	// does the entered position override the generated position for component? ***instance of actual use?***

// towing
	UINT32 	TowType;
	FLOAT64	HaulAroundForce;
	FLOAT64	HaulAroundSpeed;

// sound
	SCHAR 	SoundName[64];
	SINT32 	SoundId;

// generic user data
	LOGICAL			UserLogical;
	FLOAT64VECTOR 	UserFloat64;

// component type specific data
	void* SpecialData;

	LOGICAL OKIfFail;

	LOGICAL isPak38_6lb_57mm;

};

struct mlComponentList
{
	UINT32 NumComponents;
	mlComponent Components[1];
};

struct mlComponentListIterator
{
	mlComponentList* List;
	int idx;
	mlComponent* comp;
};

using mlComponentVector = std::vector<mlComponent*>;

struct mlAirfoilList;
struct mlEngineList;

/************************************************************************************************
	mlCompon
*************************************************************************************************/

extern UINT32 mlcomponReadComponentFile(mlComponentList** ComponentList,
									   UCHAR* ComponentListFilename,
									   UCHAR* DirName,
									   struct mlAirfoilList* AirfoilList,
									   struct mlEngineList* EngineList,
									   UINT32 VehicleTypeNum,
									   void*  VehicleMasterPtr);

extern UINT32 mlcomponFreeComponentList(mlComponentList* ComponentList);

extern UINT32 mlFindComponent(mlComponent** ComponentPtr,
								mlComponentList* ComponentList, UCHAR* ComponentName);


extern UINT32 mlcomponReadDamageComponents(mlComponentList** ComponentList,
								   UCHAR* ComponentListFilename,
								   UCHAR* DirName,
								   UINT32 VehicleTypeNum,
								   void* VehicleMasterPtr);

extern void mlComponentListBegin(mlComponentList*, mlComponentListIterator*);
extern void mlComponentListNext(mlComponentListIterator*);

