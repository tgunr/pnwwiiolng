#pragma once
//	 Functions and variables to support localization
//	 Copyright 2000 Playnet Inc, All rights reserved
//
//	Originating Programmer : Tim Haulton
//
//	Rev 0.1  9/21/00

enum loclLocalizationLanguage
{
	loclEnglish = 0,
	loclLastLanguage
} ;

enum loclLocalizationStringID
{
	loclDEFAULT_MESSAGE = 0,
	loclANIMATION_TEXTURE_MSG,
	loclRELEASE_PARTICLE_MSG,
	loclPARTICLE_COUNT_MSG,
	loclCALLBACK_SIZE_MSG,
	loclSPECIFIC_SIZE_MSG,
	loclDYANMIC_LIGHTING_ON_MSG,
	loclDYANMIC_LIGHTING_OFF_MSG,
	loclAFTERNOON_LIGHTING_MSG,
	loclDUSK_LIGHTING_MSG,
	loclNIGHT_LIGHTING_MSG,
	loclSUNRISE_LIGHTING_MSG,
	loclNOON_LIGHTING_MSG,
	loclACCELERATED_SUN_MOVEMENT_MSG,
	loclNORMAL_SUN_MOVEMENT_MSG,
	loclPROP_PITCH_ADJUSTER_MSG,
	loclSET_MAX_RPM_MSG,
	loclSET_CONTINUOUS_RPM_MSG,
	loclSET_ECONOMY_RPM_MSG,
	loclCLUTCH_MUST_BE_DISENGAGED_MSG,
	loclSHIFTED_TO_NEUTRAL_MSG,
	loclGEAR_IN_REVERSE_MSG,
	loclNEUTRAL_TO_TOGGLE_REVERSE_GEAR_MSG,
	loclSHIFTED_TO_FORWARD_GEARS_MSG,
	loclSHIFTED_TO_REVERSE_GEARS_MSG,
	loclSHIFTED_GEARS_MSG,
	loclENGINE_RPM_AND_THROTTLE_POSITION_MSG,
	loclSHIFTED_INTO_NEUTRAL_BLOWER_MSG,
	loclSUPERCHARGER_PRIMARY_SPEED_SETTING_MSG,
	loclSUPERCHARGER_SECOND_SPEED_SETTING_MSG,
	loclENGINE_BOOST_MSG,
	loclSTARTER_ON_MSG,
	loclSTARTER_OFF_MSG,
	loclENGINE_RUNNING_MSG,
	loclENGINE_STOPPED_MSG,
	loclENGINE_RATIO_MSG,
	loclENGINE_TEMPERATURE_MSG,
	loclSTALL_VOLUME_MSG,
	loclECONOMY_POWER_MODE_SELECTED_MSG,
	loclCONTINUOUS_POWER_MODE_SELECTED_MSG,
	loclMILITARY_POWER_MODE_SELECTED_MSG,
	loclWAR_EMERGENCY_POWER_MODE_SELECTED_MSG,
	loclAILERON_RESPONSE_MSG,
	loclCREW_HIT_MSG,
	loclTOGGLING_GEAR_MSG,
	loclTAIL_IS_NOW_LOCKED_MSG,
	loclTAIL_IS_NOW_UNLOCKED_MSG,
	loclMAIN_BOUNDING_BOX_CHECK_MSG,
	loclTOTAL_INTERECTS_MSG,
	loclLEFT_SLOT_RETRACTED_MSG,
	loclRIGHT_SLOT_RETRACTED_MSG,
	loclLEFT_SLOT_DEPLOYED_MSG,
	loclRIGHT_SLOT_DEPLOYED_MSG,
	loclAIRFOIL_COMPONENT_DATA_MSG,
	loclSWITCHING_TO_GEAR_UP_MSG,
	loclSWITCHING_TO_GEAR_DOWN_MSG,
	loclAVERAGE_TIME_MSG,
	loclNUMBER_OF_OBJECTS_MSG,
	loclPOSITION_OF_X_MSG,
	loclTPOSITION_OF_X_MSG,
	loclEXTERNAL_FIELD_OF_VIEW_MSG,
	loclPERSONA_CREATED_ON_HOST_MSG,
	loclCREW_KILLED_MSG,
	loclFIELD_OF_VIEW_MSG,
	loclNO_GROUND_UNDER_MY_FEET_MSG,
	loclBBOX_TOPOBJECT_MSG,
	loclOUT_OF_EDIT_BUFFER_SPACE_MSG,
	loclDUPLICATE_OBJECTS_IN_OCTET_NOT_ALLOWED_MSG,
	loclCANNOT_FIND_PLACE_TO_SET_OBJECT_MSG,
	loclNO_MORE_ROOM_FOR_OBJECT_PLACEMENT_MSG,
	loclCOULD_NOT_LOAD_FILE_AS_TEXTURE_MSG,
	loclVERTEX_COUNT_MSG,
	loclLOGGING_ON_MSG,
	loclUNSUCCESSFUL_LOGON_ATTEMPT_MSG,
	loclSUCCESSFUL_LOGON_ATTEMPT_MSG,
	loclCLIENT_INITIALIZATION_MSG,
	loclHOST_RUNNING_MSG,
	loclCLIENT_RUNNING_MSG,
	loclCURRENT_STEP_MSG,
	loclSEQUENCE_DONE_MSG,
	loclENABLING_FUNCTION_MSG,
	loclDISABLING_FUNCTION_MSG,
	loclEXECUTING_FUNCTION_MSG,
	loclSTARTING_SEQUENCE_MSG,
	loclCANNOT_CREATE_NEW_VEHICLE_MSG,
	loclVEHICLE_SELECTED_MSG,
	loclPOSITION_XYZ_MSG,
	loclKILLING_ROUND_MSG,
	loclCOLLIDE_MSG,
	loclLAUNCHING_ROUND_MSG,
	loclLastStringSPHERE_COLLISION_OCCURENCE_MSG,
	loclAMMODEPLETED,
} ;

// This opens a file (using fio subsystem) to load in a group of message strings.
// The message strings are placed in a global/ array with size dynamically
// allocated. This call will erase all previously loaded messages in the array.
// Each line of the localization file has the following syntax: <ID>,<STRING>
// where ID is a unique 32-bit number, no greater than HighestStringNumber,
// and STRING is terminated by a newline character. The files will be named
// "<language>.loc". This function returns TRUE if the file is found and loads
// properly, and returns FALSE if the file is not found or cannot be loaded.
// The function locl_Message_Table_Kill is the "Kill Switch" used to free
// the allocated memory.
extern LOGICAL loclLoadLocalizationStringTable(SINT32 LanguageID);

// This function returns a signed character string from the localization
// table. If there is no string for a given ID (that is to say, an invalid
// ID is passed), the default message (message 0) will be returned.
extern const WCHAR* loclGetLocalizationStringByID(SINT32 MessageID);

// This function returns an SINT32 for the currently installed
// language identifier
extern SINT32 loclGetLanguageID(void);

//This function returns a signed character string with the Language Name.
extern SCHAR* loclGetLanguageName(void);

//This function frees the memory in the message table.
extern LOGICAL locl_Message_Table_Kill(void);
