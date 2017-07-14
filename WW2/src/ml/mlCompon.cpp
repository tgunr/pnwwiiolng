#include "syscfg.h" // Path is project specific.
// components module

#include "ml/mlCompon.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "ml/mlEngine.h"
#include "ml/mlfoils.h"
#include "ml/mlgear.h"
#include "ml/mlweapons.h"
#include "veh/vehmain.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "ogrp/ogrptype.h"

/************************************************************************************************
	mlCompon
*************************************************************************************************/

enum
{
	ML_COMPONENT_VARNAME_TYPE = 1,
	ML_COMPONENT_VARNAME_SUBTYPE,
	ML_COMPONENT_VARNAME_POSX,
	ML_COMPONENT_VARNAME_POSY,
	ML_COMPONENT_VARNAME_POSZ,
	ML_COMPONENT_VARNAME_ATTPITCH,
	ML_COMPONENT_VARNAME_ATTROLL,
	ML_COMPONENT_VARNAME_ATTYAW,
	ML_COMPONENT_VARNAME_INERTIAPITCH,
	ML_COMPONENT_VARNAME_INERTIAROLL,
	ML_COMPONENT_VARNAME_INERTIAYAW,
	ML_COMPONENT_VARNAME_MASS,
	ML_COMPONENT_VARNAME_DAMAGELEVEL1,
	ML_COMPONENT_VARNAME_DAMAGELEVEL2,
	ML_COMPONENT_VARNAME_DAMAGELEVEL3,
	ML_COMPONENT_VARNAME_DAMAGELEVEL4,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM1,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM2,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM3,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM4,
	ML_COMPONENT_VARNAME_CREWPOSITION,
	ML_COMPONENT_VARNAME_DAMAGEGROUPNUM,
	ML_COMPONENT_VARNAME_HITCHTYPE,
	ML_COMPONENT_VARNAME_HAULFORCE,
	ML_COMPONENT_VARNAME_HAULSPEED,
	ML_COMPONENT_VARNAME_HAULSOUND,
	ML_COMPONENT_VARNAME_OVERRIDEPOS,
	ML_COMPONENT_VARNAME_CRITICALPRIMARY,
	ML_COMPONENT_VARNAME_CRITICALSECONDARY,
	ML_VEHICLEMASS_HACK,
	ML_COMPONENT_VARNAME_MASSOBJECT,
	ML_COMPONENT_VARNAME_INTERALMASS,
	ML_COMPONENT_VARNAME_FREEFLOOD,
	ML_COMPONENT_VARNAME_AIRVOL,
	ML_COMPONENT_VARNAME_VISDAMAGE,
	ML_COMPONENT_VARNAME_VISDAMAGE_PARENT,
	ML_COMPONENT_VARNAME_DAMAGELEVELFAIL,
	ML_COMPONENT_VARNAME_HULKIFFAIL,
	ML_COMPONENT_VARNAME_ACTIVATEIFFAIL,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_5, // this 5 are ONLY used for pak38, m57, 6lb hackery
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_6,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_7,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_8,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_9,
	ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_10,
	ML_COMPONENT_GENERIC_MAXVARS
};

UCHAR _ComponentVarNames[ML_COMPONENT_GENERIC_MAXVARS][32] =
{
	"none",
	"type",
	"subtype",
	"x",
	"y",
	"z",
	"attpitch",
	"attroll",
	"attyaw",
	"inertiapitch",
	"inertiaroll",
	"inertiayaw",
	"mass",
	"damagelevel1",
	"damagelevel2",
	"damagelevel3",
	"damagelevel4",
	"hitmapcomponent1",
	"hitmapcomponent2",
	"hitmapcomponent3",
	"hitmapcomponent4",
	"crewposition",
	"damagegroupnum",
	"towtype",
	"force",
	"speed",
	"haulsound",
	"overridepos",
	"criticalprimary",
	"criticalsecondary",
	"totalmass",
	"massobject",
	"internalmass",
	"freeflood",
	"airvol",
	"visdamage",
	"visdamageparent",
	"damagelevelfail",
	"hulkiffail",
	"activateiffail",
	"hitmapcomponent_5", // this 5 are ONLY used for pak38, m57, 6lb hackery
	"hitmapcomponent_6",
	"hitmapcomponent_7",
	"hitmapcomponent_8",
	"hitmapcomponent_9",
	"hitmapcomponent_10",
};

UCHAR _ComponentTypeNames[ML_COMPONENT_MAXTYPES][32] =
{
	"none",
	"ballast",
	"airfoil",
	"engine",
	"pitot",
	"gear",
	"weaponmount",
	"crew",
	"towpoint",
	"haulpoint",
};

/************************************************************************************************
	mlcomponReadComponentFile
*************************************************************************************************/

UINT32 mlcomponReadComponentFile(mlComponentList** ComponentList,
								   UCHAR* ComponentListFilename,
								   UCHAR* DirName,
								   mlAirfoilList* AirfoilList,
								   mlEngineList* EngineList,
								   UINT32 VehicleTypeNum,
								   void* VehicleMasterPtr)
{
	*ComponentList = nullptr;
	if (ComponentListFilename == nullptr || *ComponentListFilename == '\0')
		return 1;

#ifdef PRINT_COMPONENTS
	printf("%s\n", ComponentListFilename);
#endif

	UCHAR UseFileName[128];
	strcpy((char*)UseFileName, (char*)DirName);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)ComponentListFilename);

	SINT32 ComponentListFile = fioOpen((char*)UseFileName);
	if (ComponentListFile < 0)
	{
		return 1;
	}

	const void *fileData;
	size_t fileSize = fioMap(ComponentListFile, fileData);
	if (fileSize <= 0)
	{
		fioClose(ComponentListFile);
		return 1;
	}

	size_t NumComponents = 0;
	for (const char* cur = static_cast<const char*>(fileData);;)
	{
		if (*cur == '[')
			++NumComponents;
		cur = strchr(cur, '\n');
		if (!cur)
			break;
		++cur;
	}
	if (NumComponents == 0)
	{
		fioClose(ComponentListFile);
		return 1;
	}

	*ComponentList = (mlComponentList*)memCalloc(sizeof(mlComponentList) + NumComponents * sizeof(mlComponent), 1);
	(*ComponentList)->NumComponents = NumComponents;

	SINT32 ComponentListPos = -1;
	UCHAR Buffer[256];
	UCHAR * BufferPos;
	UINT32 VarNum;
	UCHAR * VarName;
	UCHAR * VarContents;
	mlComponent * CurrentComponent;

	while (fioGetLine(ComponentListFile, (char*)Buffer, 255) != 0)
	{
		if (Buffer[0] == '#')
			continue;

		if (Buffer[0] == '[')
		{
			// new engine
			// first strip off the "[" and "]" characters
			BufferPos = Buffer + 1;

			BufferPos = (UCHAR*)strtok((char*)BufferPos,"]");
			ComponentListPos++;
			if (ComponentListPos == NumComponents)
			{
				break;
			}

			CurrentComponent = &(*ComponentList)->Components[ComponentListPos];
			strcpy((char*)CurrentComponent->ComponentName,(char*)BufferPos);

			continue;
		}
		else
		{
			if (ComponentListPos < 0)
			{
				continue;
			}

			mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}

			mlUtilityFindVarname(&VarNum, VarName, (UCHAR*)&_ComponentVarNames[0][0], 32, ML_COMPONENT_GENERIC_MAXVARS);
			// if variable not found, try the component-specific variables
			if (VarNum == 0)
			{
				if (CurrentComponent->Type != 0)
				{
					switch (CurrentComponent->Type)
					{
						case ML_COMPONENT_AIRFOIL:
							mlfoilAddSpecialVariable(CurrentComponent->SpecialData,
													 (char*)VarName,
													 (char*)VarContents,
													 AirfoilList,
													 *ComponentList,
													 CurrentComponent,
													 VehicleMasterPtr);
						break;
						case ML_COMPONENT_ENGINE:
							mlengineAddSpecialVariable(CurrentComponent->SpecialData,
													 (char*)VarName,
													 (char*)VarContents,
													 EngineList,
													 *ComponentList,
													 CurrentComponent,
													 AirfoilList,
													 VehicleMasterPtr);
						break;

						case ML_COMPONENT_GEAR:
							mlgearAddSpecialVariable(CurrentComponent->SpecialData,
													 (char*)VarName,
													 (char*)VarContents,
													 *ComponentList,
													 CurrentComponent);

						break;
						case ML_COMPONENT_WEAPONMOUNT:
							mlWeaponMountAddSpecialVariable(CurrentComponent->SpecialData,
															(char*)VarName,
															(char*)VarContents,
															*ComponentList,
															CurrentComponent,
															VehicleMasterPtr);
						default:
						break;
					}

				}
				continue;
			}

			switch (VarNum)
			{
				case ML_COMPONENT_VARNAME_TYPE:
						mlUtilityFindVarname(&CurrentComponent->Type, VarContents,
											(UCHAR*)&_ComponentTypeNames[0][0], 32, ML_COMPONENT_MAXTYPES);

						if ((*ComponentList)->Components[ComponentListPos].Type == 0)
						{
							WriteLog(LOG_VEHICLE, LOG_WARN,"Component type %s in component %s not recognized in file %s\n",
										VarContents, CurrentComponent->ComponentName, UseFileName);
						}
						else
						{
                            // HACK! Until the data is fixed. Some files have repeat types
                            // which cause memory leaks
                            if(CurrentComponent->Type == ML_COMPONENT_WEAPONMOUNT)
                            {
                                if(CurrentComponent->SpecialData)
                                {
                                    delete (mlWeaponMount*)CurrentComponent->SpecialData;
                                    CurrentComponent->SpecialData = nullptr;
                                }
                            }
                            else
                            {
                                if(CurrentComponent->SpecialData)
                                {
                                    memFree(CurrentComponent->SpecialData);
                                    CurrentComponent->SpecialData = nullptr;
                                }
                            }

							switch (CurrentComponent->Type)
							{
								case ML_COMPONENT_AIRFOIL:
										CurrentComponent->SpecialData = memAlloc(sizeof(mlAirfoilComponentData));
									break;

								case ML_COMPONENT_ENGINE:
										CurrentComponent->SpecialData = memAlloc(sizeof(mlEngineComponentData));
									break;

								case ML_COMPONENT_GEAR:
										CurrentComponent->SpecialData = memAlloc(sizeof(mlGearComponentData));
									break;
								case ML_COMPONENT_WEAPONMOUNT:
										CurrentComponent->SpecialData = new mlWeaponMount{};//memAlloc(sizeof(mlWeaponMount));
//										memset(CurrentComponent->SpecialData, 0, sizeof(mlWeaponMount));
										((mlWeaponMount*)CurrentComponent->SpecialData)->MinConvergence = 50.0f;
//										hackAddProtectedBlock(CurrentComponent->SpecialData,
//																sizeof(mlWeaponMount));
									break;
								case ML_COMPONENT_CREW:
                                    {
                                        CurrentComponent->SpecialData = memAlloc(sizeof(LOGICAL));
                                    }
								break;
								default:
									CurrentComponent->SpecialData = nullptr;
								break;

							}
							// initialize the component type
						}
					break;
				case ML_COMPONENT_VARNAME_SUBTYPE:
						switch(CurrentComponent->Type)
						{
							case ML_COMPONENT_AIRFOIL:
								mlfoilSetComponentSubtype( CurrentComponent->SpecialData, (char*)VarContents);
								break;
							case ML_COMPONENT_ENGINE:
									mlengineSetComponentSubtype( CurrentComponent->SpecialData, VarContents, EngineList);
								break;

							case ML_COMPONENT_GEAR:
								mlgearSetComponentSubtype( CurrentComponent->SpecialData, (char*)VarContents);
								break;

							default:
								break;
						}
					break;
// note: in the text files, for intuitiveness for the person making the models, axes of rotation
// are right-handed
//
// also axes are changed as follows:
//
// file		internal
// pitch	pitch
// roll		roll
// yaw		yaw


				case ML_COMPONENT_VARNAME_POSX:
						CurrentComponent->OriginalRelativePos.x = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_POSY:
						CurrentComponent->OriginalRelativePos.y = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_POSZ:
						CurrentComponent->OriginalRelativePos.z = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_ATTPITCH:
						CurrentComponent->Attitude.Pitch = mlDEG_TO_RAD(atof((char*)VarContents));
					break;
				case ML_COMPONENT_VARNAME_ATTROLL:
						CurrentComponent->Attitude.Roll = mlDEG_TO_RAD(atof((char*)VarContents));
					break;
				case ML_COMPONENT_VARNAME_ATTYAW:
						CurrentComponent->Attitude.Yaw = -mlDEG_TO_RAD(atof((char*)VarContents));
					break;
				case ML_COMPONENT_VARNAME_INERTIAPITCH:
						CurrentComponent->RotInertia.Pitch = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_INERTIAROLL:
						CurrentComponent->RotInertia.Roll = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_INERTIAYAW:
						CurrentComponent->RotInertia.Yaw = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_MASS:
						CurrentComponent->Mass = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_DAMAGELEVEL1:
				case ML_COMPONENT_VARNAME_DAMAGELEVEL2:
				case ML_COMPONENT_VARNAME_DAMAGELEVEL3:
				case ML_COMPONENT_VARNAME_DAMAGELEVEL4:
						CurrentComponent->DamageLevels[VarNum - ML_COMPONENT_VARNAME_DAMAGELEVEL1] = atof((char*)VarContents);
				break;
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM1:
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM2:
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM3:
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM4:
				{
					int idx = VarNum - ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM1;
					CurrentComponent->DamageComponentNum[idx] = 0;
					vehFindHitmapObjectNumByName(
							&(CurrentComponent->DamageComponentNum[idx]),
							VarContents,
							VehicleTypeNum);
					if (CurrentComponent->DamageComponentNum[idx] == 0)
					{
						WriteLog(LOG_VEHICLE, LOG_WARN,"Unable to find hitmap object %s, component %s, # = %d",
									VarContents, CurrentComponent->ComponentName, idx + 1);
					}
				}
				break;

                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_5: // these 5 are ONLY used for pak38, m57, 6lb hackery
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_6:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_7:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_8:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_9:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_10:
                {
					UINT32 idx = VarNum - ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_5 + 4;

					CurrentComponent->DamageComponentNum[idx] = 0;

                    CurrentComponent->isPak38_6lb_57mm = true;

					vehFindHitmapObjectNumByName(
                                                 &(CurrentComponent->DamageComponentNum[idx]),
                                                 VarContents,
                                                 VehicleTypeNum);
                }
                    break;

				case ML_COMPONENT_VARNAME_CREWPOSITION:
					CurrentComponent->CrewPosition = atoi((char*)VarContents) - 1;
					// Unsigned cast to force negatives to overflow.
					if((UINT32)CurrentComponent->CrewPosition >= ML_MAX_CREW_POSITIONS)
					{
						WriteLog(LOG_VEHICLE, LOG_WARN, "Invalid Crew Position %d", CurrentComponent->CrewPosition + 1);
						CurrentComponent->CrewPosition = 0;
					}
				break;
				case ML_COMPONENT_VARNAME_DAMAGEGROUPNUM:
						CurrentComponent->DamageGroupNum = atoi((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_HITCHTYPE:
						if (strcmp((char*)VarContents,"light") == 0)
						{
							CurrentComponent->TowType = ML_TOWTYPE_LIGHT;
						}
						else if (strcmp((char*)VarContents,"medium") == 0)
						{
							CurrentComponent->TowType = ML_TOWTYPE_MEDIUM;
						}
						else if (strcmp((char*)VarContents,"heavy") == 0)
						{
							CurrentComponent->TowType = ML_TOWTYPE_HEAVY;
						}
					break;
				case ML_COMPONENT_VARNAME_HAULFORCE:
					CurrentComponent->HaulAroundForce = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_HAULSPEED:
					CurrentComponent->HaulAroundSpeed = atof((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_HAULSOUND:
					strcpy(CurrentComponent->SoundName,(char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_OVERRIDEPOS:
					CurrentComponent->OverridePos = atoi((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_CRITICALPRIMARY:
					CurrentComponent->CriticalPrimary = atoi((char*)VarContents);
					break;
				case ML_COMPONENT_VARNAME_CRITICALSECONDARY:
				{
					CurrentComponent->CriticalSecondary = (UINT32)atoi((char*)VarContents);

					if(CurrentComponent->CriticalSecondary > 0)
					{
						if(CurrentComponent->CriticalSecondary > MAX_SECONDARY_GROUPS)
						{
							WriteLog(LOG_VEHICLE, LOG_WARN, "invalid criticalsecondary value");
							CurrentComponent->CriticalSecondary = 0;
						}
						else
						{
							// update the total number of secondary critical components for this vehicle
							mlVehicleData* data = ((vehVehicle*)VehicleMasterPtr)->ModelData;
							data->TotalSecondaryCriticalComponentCount[CurrentComponent->CriticalSecondary-1]++;
						}
					}
				}
				break;
				case ML_COMPONENT_VARNAME_INTERALMASS:
					CurrentComponent->IsInternalMass = atoi((char*)VarContents);
				break;
				case ML_COMPONENT_VARNAME_FREEFLOOD:
					CurrentComponent->IsFreeFlooding = atoi((char*)VarContents);
				break;
				case ML_COMPONENT_VARNAME_AIRVOL:
					CurrentComponent->AirVolPercentage = atof((char*)VarContents) / 100.0;
				break;
				case ML_COMPONENT_VARNAME_VISDAMAGE:
				{
					int i;
					for(i = 0; i < MAX_DAMAGE_GROUPS; i++)
					{
						if(!CurrentComponent->VisDamage[i][0])
						{
							strsafecpy(CurrentComponent->VisDamage[i], (char*)VarContents);
							break;
						}
					}
				}
				break;
				case ML_COMPONENT_VARNAME_VISDAMAGE_PARENT:
					strsafecpy(CurrentComponent->VisDamageParent, (char*)VarContents);
				break;
				case ML_COMPONENT_VARNAME_DAMAGELEVELFAIL:
					CurrentComponent->DamageLevelFail = atof((char*)VarContents);
				break;
				case ML_COMPONENT_VARNAME_HULKIFFAIL:
					CurrentComponent->HulkIfFail = (LOGICAL)atoi((char*)VarContents);
				break;
				case ML_COMPONENT_VARNAME_ACTIVATEIFFAIL:
					CurrentComponent->OKIfFail = atoi((char*)VarContents);
					if(CurrentComponent->OKIfFail)
						CurrentComponent->ComponentStatus = ML_STATUS_OK_IF_FAIL;
				break;
				case ML_VEHICLEMASS_HACK:
					// to shut up warning message
				break;
				default:
					WriteLog(LOG_VEHICLE, LOG_WARN,"Unidentified variable %s in file %s\n",(char*)VarName,(char*)UseFileName);
				break;
			}
		}
	}
	fioClose (ComponentListFile);

	if(((vehVehicle*)VehicleMasterPtr)->ModelData->PhysicsVersion > 0)
	{
		UINT32 i;
		for(i = 0; i < (*ComponentList)->NumComponents; i++)
		{
			CurrentComponent = &(*ComponentList)->Components[i];
			if(	 CurrentComponent->Mass == 0 &&
				(CurrentComponent->RotInertia.Pitch || CurrentComponent->RotInertia.Roll || CurrentComponent->RotInertia.Yaw) )
			{
				char str[64];
				snprintf(str, 63, "Component %s has rotational inertia but no mass!", CurrentComponent->ComponentName);
				piExitProgram(_FL,str);
			}
		}
	}
#ifdef PRINT_COMPONENTS
{
	int i;
	for(i = 0; i < (*ComponentList)->NumComponents; i++)
	{
		CurrentComponent = &(*ComponentList)->Components[i];
		printf("%d %s %d %d %d %d\n", i, CurrentComponent->ComponentName,
				CurrentComponent->DamageComponentNum[0],CurrentComponent->DamageComponentNum[1],
				CurrentComponent->DamageComponentNum[2],CurrentComponent->DamageComponentNum[3]);
	}
}
#endif

	return 0;
}

/************************************************************************************************
	mlFindComponent
*************************************************************************************************/

UINT32 mlFindComponent(mlComponent** ComponentPtr,
					  	mlComponentList* ComponentList,
					  	UCHAR* ComponentName)
{
	UINT32 i;

	if(ComponentPtr == nullptr) return 1;
	*ComponentPtr = nullptr;
	if(ComponentList == nullptr ||  ComponentName == nullptr) return 1;

	for (i = 0; i < ComponentList->NumComponents; i++)
	{
		if (strcmp((char*)ComponentList->Components[i].ComponentName,(char*)ComponentName) == 0)
		{
			*ComponentPtr = &ComponentList->Components[i];
			break;
		}
	}

	return 0;
}

/************************************************************************************************
	mlcomponFreeComponentList
*************************************************************************************************/

UINT32 mlcomponFreeComponentList(mlComponentList* ComponentList)
{
	if (ComponentList != nullptr)
	{
		for (size_t i=0;i<ComponentList->NumComponents;i++)
		{
			if (ComponentList->Components[i].SpecialData != nullptr)
			{
				if (ComponentList->Components[i].Type == ML_COMPONENT_WEAPONMOUNT)
				{
//					hackRemoveProtectedBlock(ComponentList->Components[i].SpecialData);
					delete (mlWeaponMount*)ComponentList->Components[i].SpecialData;
				}
				else
				{
					memFree(ComponentList->Components[i].SpecialData);
				}
			}
		}

        memFree(ComponentList);
	}

	return 0;
}

/************************************************************************************************
	mlcomponReadDamageComponents
*************************************************************************************************/

UINT32 mlcomponReadDamageComponents(mlComponentList** ComponentList,
								   UCHAR* ComponentListFilename,
								   UCHAR* DirName,
								   UINT32 VehicleTypeNum,
								   void* VehicleMasterPtr)
{
	UCHAR Buffer[256];
	UCHAR * BufferPos;
	UCHAR UseFileName[128];
	UINT32 VarNum;
	UCHAR * VarName;
	UCHAR * VarContents;
	SINT32 ComponentListPos;
	mlComponent * CurrentComponent;

	*ComponentList = nullptr;
	if(ComponentListFilename == nullptr || *ComponentListFilename == '\0')
		return 1;

	strcpy((char*)UseFileName,(char*)DirName);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)ComponentListFilename);

	SINT32 ComponentListFile = fioOpen((char*)UseFileName);
	if (ComponentListFile < 0)
	{
		return 1;
	}

	const void *fileData;
	size_t fileSize = fioMap(ComponentListFile, fileData);
	if (fileSize <= 0)
	{
		fioClose(ComponentListFile);
		return 1;
	}

	size_t NumComponents = 0;
	for (const char* cur = static_cast<const char*>(fileData);;)
	{
		if (*cur == '[')
			++NumComponents;
		cur = strchr(cur, '\n');
		if (!cur)
			break;
		++cur;
	}
	if (NumComponents == 0)
	{
		fioClose(ComponentListFile);
		return 1;
	}

	*ComponentList =  (mlComponentList*)memCalloc(sizeof(mlComponentList) + NumComponents * sizeof(mlComponent), 1);
	(*ComponentList)->NumComponents = NumComponents;

	ComponentListPos = -1;

	while (fioGetLine(ComponentListFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
			continue;

		if (Buffer[0] == '[')
		{
			// new engine
			// first strip off the "[" and "]" characters
			BufferPos = Buffer + 1;

 			BufferPos = (UCHAR*)strtok((char*)BufferPos,"]");
			ComponentListPos++;
			if (ComponentListPos == NumComponents)
				break;

			CurrentComponent = &(*ComponentList)->Components[ComponentListPos];
			strcpy((char*)CurrentComponent->ComponentName,(char*)BufferPos);

			continue;
		}
		else
		{
			if (ComponentListPos < 0)
				continue;

			mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

			if (VarName == nullptr || VarContents == nullptr)
				continue;

			mlUtilityFindVarname(&VarNum, VarName,
								(UCHAR*)&_ComponentVarNames[0][0], 32, ML_COMPONENT_GENERIC_MAXVARS);
			if (VarNum == 0)
				continue;

			switch (VarNum)
			{
				case ML_COMPONENT_VARNAME_TYPE:
					mlUtilityFindVarname(&CurrentComponent->Type, VarContents,
											(UCHAR*)&_ComponentTypeNames[0][0], 32,
											ML_COMPONENT_MAXTYPES);
				break;

				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM1:
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM2:
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM3:
				case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM4:
				{
					UINT32 idx = VarNum - ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM1;

					CurrentComponent->DamageComponentNum[idx] = 0;

					vehFindHitmapObjectNumByName(
							&(CurrentComponent->DamageComponentNum[idx]),
							VarContents,
							VehicleTypeNum);
				}

				break;

                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_5: // these 5 are ONLY used for pak38, m57, 6lb hackery
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_6:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_7:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_8:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_9:
                case ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_10:
				{
					UINT32 idx = VarNum - ML_COMPONENT_VARNAME_DAMAGECOMPONENTNUM_5 + 4;

					CurrentComponent->DamageComponentNum[idx] = 0;

                    CurrentComponent->isPak38_6lb_57mm = true;

					vehFindHitmapObjectNumByName(
                                                 &(CurrentComponent->DamageComponentNum[idx]),
                                                 VarContents,
                                                 VehicleTypeNum);
				}

                    break;

				// move out of cmp
				case ML_VEHICLEMASS_HACK:
					if(VehicleMasterPtr) // horrid hack for concussion resistance hack
						((vehType*)VehicleMasterPtr)->VehicleRealMass = (float)atof((char*)VarContents);
				break;
			}
		}
	}
	fioClose (ComponentListFile);

	return 0;
}

/************************************************************************************************
	mlComponentListBegin
*************************************************************************************************/

void mlComponentListBegin(mlComponentList* List, mlComponentListIterator* it)
{
	it->List = List;
	it->idx = 0;
	mlComponentListNext(it);
}

/************************************************************************************************
	mlComponentListNext
*************************************************************************************************/

void mlComponentListNext(mlComponentListIterator* it)
{
	it->comp = nullptr;

	for(; it->idx < (int)it->List->NumComponents; it->idx++)
	{
		mlComponent* comp = &it->List->Components[it->idx];
		if(comp && comp->ComponentStatus == ML_STATUS_OK)
		{
			it->comp = comp;
			it->idx++;
			break;
		}
	}
}
