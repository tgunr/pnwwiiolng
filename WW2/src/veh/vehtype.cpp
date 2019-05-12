#include "syscfg.h" // Path is project specific.
#include <map>
#include <string>
#include <vector>

#include "veh/vehtype.h"
#include "veh/vehxml.h"
#include "fio/fio.h"
#include "weap/weapList.h"
#include "ui/uiMain.h"
#include "ui/uiUTF.h"
#include "veh/vehAnimEnum.h"

#include "weap/weapons.h"
#include "veh/vehanim.h"
#include "ml/mlutility.h"

static void _vehxmlParseMass(vehType*, xmlNodePtr);
static void _ReadFlatFiles(vehType*);

static void _vehxmlParseInclude(vehType*, xmlNodePtr, bool override);

/************************************************************************************************
	vehtype
*************************************************************************************************/

static std::vector<vehType> _VehicleList;
static std::map<std::string, int> _vehtypenames;

static void _vehxmllParseVehicle(vehType*, xmlNodePtr, bool override);
static void _vehxmlParseVehicleAttributes(vehType*, xmlNodePtr);
static void* _vehxmlopen(char const* filename);
static void _ReadInfoText(vehType*);

static std::map<std::string, vehTypeXMLHandler> _xmlHandlers;

void vehTypeRegisterXMLHandler(const char* xmlTag, vehTypeXMLHandler handler)
{
	_xmlHandlers[xmlTag] = handler;
}

/************************************************************************************************
	vehtypeInit
*************************************************************************************************/

int vehtypeInit()
{
	_VehicleList.clear();

	_vehtypenames["float"] = VEH_TYPE_FLOATER;
	_vehtypenames["person"] = VEH_TYPE_PERSON;
	_vehtypenames["air"] = VEH_TYPE_AIR;
	_vehtypenames["ground"] = VEH_TYPE_GROUND;
	_vehtypenames["sea"] = VEH_TYPE_SEA;

	xmlDocPtr doc = vehxmlLoad("vehicles.xml", _vehxmlopen, (xmlInputReadCallback)fioRead, (xmlInputCloseCallback)fioClose);
	if(!doc) return -1;

	xmlNodePtr rootnode = xmlDocGetRootElement(doc);

  int count = 0;
	for(xmlNodePtr node = rootnode->children; node; node = node->next)
	{
		if(node->type == XML_ELEMENT_NODE && (xmlStrcmp((xmlChar*)"vehicle", node->name) == 0))
		{
      count++;
		}
	}

  _VehicleList.reserve(count); // ensure no more memory is used than for count

	for(xmlNodePtr node = rootnode->children; node; node = node->next)
	{
		if(node->type == XML_ELEMENT_NODE && (xmlStrcmp((xmlChar*)"vehicle", node->name) == 0))
		{
			vehType vehtype;
			_vehxmllParseVehicle(&vehtype, node, false);
			_VehicleList.push_back(vehtype);
		}
	}

	for(int i = 0; i < (int)_VehicleList.size(); i++)
	{
		vehType* vehtype = &_VehicleList[i];
		vehtype->VehicleTypeNum = i;

		vehtype->WeaponSpec = weapAssociateIndex((char*)vehtype->Id, i);
		if (vehtype->WeaponSpec.isNull())
		{
		}
		_ReadInfoText(vehtype);
		_ReadFlatFiles(vehtype);
	}

	xmlFreeDoc(doc);

	return 0;
}

/************************************************************************************************
	vehtypeKill
*************************************************************************************************/

int vehtypeKill()
{
	return 0;
}

/************************************************************************************************
	vehtypeGet
*************************************************************************************************/

vehType const* vehtypeGetByIndex(UINT32 i)
{
	if(i < _VehicleList.size())
		return &_VehicleList[i];

	return nullptr;
}

/************************************************************************************************
	vehtypeCount
*************************************************************************************************/

UINT32 vehtypeCount()
{
	return _VehicleList.size();
}

/************************************************************************************************
	vehTypeGetBySpec
*************************************************************************************************/

vehType const* vehTypeGetBySpec(WEAPON_SPEC spec)
{
	int i = 0;
	std::vector<vehType>::iterator it = _VehicleList.begin();
	for(; it != _VehicleList.end(); ++it,i++)
	{
		if(memcmp(&spec, &_VehicleList[i].WeaponSpec, sizeof(spec)) == 0)
			return &_VehicleList[i];
	}

	 return nullptr;
}

vehType const* vehTypeGetById(const char* Id)
{
	int i = 0;
	std::vector<vehType>::iterator it = _VehicleList.begin();
	for(; it != _VehicleList.end(); ++it,i++)
	{
		if(strcmp((char*)_VehicleList[i].Id, Id) == 0)
			return &_VehicleList[i];
	}

	 return nullptr;
}

/************************************************************************************************
	vehtypeWeaponSpec
*************************************************************************************************/

WEAPON_SPEC vehtypeWeaponSpec(UINT32 i)
{
	vehType const* vehtype = vehtypeGetByIndex(i);
	if(vehtype)
		return vehtype->WeaponSpec;

	return NULL_WEAP_SPEC;
}

/************************************************************************************************
	vehTypeLoc
*************************************************************************************************/

const wchar_t* vehTypeLoc(int type, const vehType* vt)
{
	static char key[128+1];
	static const wchar_t* loc;
	loc = L"";

	if(!vt)
		return loc;

	switch(type)
	{
		case 1:
		{
			snprintf(key, 128, "common.vehicles.categories.%d", vt->WeaponSpec.wCategory);
			loc = uiLocString(key).c_str();
		}
		break;
		case 2:
		{
			snprintf(key, 128, "common.vehicles.classes.%d", vt->WeaponSpec.wClass);
			loc = uiLocString(key).c_str();
		}
		break;
		case 3:
		{
			snprintf(key, 128, "vehicles.%d.%d.%d.%d",
				vt->WeaponSpec.wCountry,
				vt->WeaponSpec.wCategory,
				vt->WeaponSpec.wClass,
				vt->WeaponSpec.wType);

			const std::wstring& locDesc = uiLocString(key);

			if(locDesc[0] != L'?')
				loc = locDesc.c_str();
			else
				loc = UTF16(vt->Description);
		}
		break;
		case 4:
		{
			snprintf(key, 128, "vehicles.%d.%d.%d.%d.icon",
				vt->WeaponSpec.wCountry,
				vt->WeaponSpec.wCategory,
				vt->WeaponSpec.wClass,
				vt->WeaponSpec.wType);

			const std::wstring& locDesc = uiLocString(key);

			if(locDesc[0] != L'?')
				loc = locDesc.c_str();
			else
				loc = UTF16(vt->Icon);
		}
		break;
		default:
			// do nothing
		break;
	}

	return loc;
}

/************************************************************************************************
	_vehxmllParseVehicle
*************************************************************************************************/

static void _vehxmllParseVehicle(vehType* vehtype, xmlNodePtr vehnode, bool override)
{
	_vehxmlParseVehicleAttributes(vehtype, vehnode);

	for(xmlNodePtr node = vehnode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;

		if (xmlStrcmp((xmlChar*)"include", node->name) == 0)
		{ 	// this will go away when all files are xml-ized, for now, we get the file names for the non-xml files
			_vehxmlParseInclude(vehtype, node, override);
		}
		else if (xmlStrcmp((xmlChar*)"masses", node->name) == 0)
		{
			_vehxmlParseMass(vehtype, node);
		}
	}
}

/************************************************************************************************
	_vehxmlParseVehicleAttributes
*************************************************************************************************/

static void _vehxmlParseVehicleAttributes(vehType* vehtype, xmlNodePtr vehnode)
{
	xmlChar* xmlstr;

	// Id
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"id");
	if(xmlstr)
	{
		strncpy((char*)vehtype->Id,(char*)xmlstr,31);
		xmlFree(xmlstr);
	}

	// Type
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"type");
	if(xmlstr)
	{
		vehtype->Type = _vehtypenames[(char*)xmlstr];
		xmlFree(xmlstr);
	}

	// Shape
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"shape");
	if(xmlstr)
	{
		strncpy((char*)vehtype->Shape,(char*)xmlstr,31);
		xmlFree(xmlstr);
	}

	// Icon
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"icon");
	if(xmlstr)
	{
		strncpy((char*)vehtype->Icon,(char*)xmlstr,31);
		xmlFree(xmlstr);
	}

	// Description
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"description");
	if(xmlstr)
	{
		strncpy((char*)vehtype->Description,(char*)xmlstr,63);
		xmlFree(xmlstr);
	}

	// OnlineOnly
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"onlineonly");
	if(xmlstr)
	{
		if(	xmlStrcmp((xmlChar*)"false", xmlstr) != 0 &&
			xmlStrcmp((xmlChar*)"no", xmlstr) != 0 &&
			xmlStrcmp((xmlChar*)"0", xmlstr) != 0)
		{
			vehtype->OnlineOnly = TRUE;
		}
		xmlFree(xmlstr);
	}

	// ModelDirectory
	xmlstr = xmlGetProp(vehnode, (xmlChar*)"file");
	if(xmlstr)
	{
		char* s;
		for(s = (char*)xmlstr; *s; s++) { if(*s=='\\') *s = '/';}
		s = strrchr((char*)xmlstr, '/');
		if(*s) *s = '\0';

		strncpy((char*)vehtype->ModelDirectory,(char*)xmlstr,63);
		xmlFree(xmlstr);
	}
}

static void*  _vehxmlopen(char const* filename)
{
	std::string str = "vehicles/";
	str += filename;

	int fd = fioOpen(str.c_str());
	return (void*)fd;
}

static void _ReadInfoText(vehType* vehtype)
{
	std::string filename = "vehicles/";
	filename += (char*)vehtype->ModelDirectory;
	filename += "/";
	filename += (char*)vehtype->Id;
	filename += ".txt";

	int fd = fioOpen(filename.c_str());
	if(fd > -1)
	{
		size_t size = fioFileSize(filename.c_str());
		char* buf = new char[size+1];
		buf[size]='\0';
		fioRead(fd, buf, size);
		vehtype->InfoText = buf;

		delete[] buf;
		fioClose(fd);
	}
}



static void _vehxmlParseInclude(vehType* vehtype, xmlNodePtr incnode, bool override)
{
	xmlChar* filename;
	filename = xmlGetProp(incnode, (xmlChar*)"file");
	if(!filename) return;

	char* ext = strrchr((char*)filename, '.');
	if(ext && strcmp(ext, ".xml") == 0)
	{
		*ext = '\0';
		ext = strrchr((char*)filename, '.');
	}

	if(ext)
	{
		if(strcmp(ext, ".veh") == 0) // ModelFile .veh
		{
			if(vehtype->ModelFile[0] == '\0' || override)
				strncpy((char*)vehtype->ModelFile, (char*)filename, 63);
		}
		else if(strcmp(ext, ".control") == 0) // ControlFile .control
		{
			if(vehtype->ControlFile[0] == '\0' || override)
				strncpy((char*)vehtype->ControlFile, (char*)filename, 63);
		}else if(strcmp(ext, ".hit") == 0) // DamageFile .hit
		{
			if(vehtype->DamageFile[0] == '\0' || override)
				strncpy((char*)vehtype->DamageFile, (char*)filename, 63);
		}else if(strcmp(ext, ".state") == 0) // StateFile .state
		{
			if(vehtype->StateFile[0] == '\0' || override)
				strncpy((char*)vehtype->StateFile, (char*)filename, 63);
		}else if(strcmp(ext, ".turret") == 0) // TurretFile .turret
		{
			if(vehtype->TurretFile[0] == '\0' || override)
				strncpy((char*)vehtype->TurretFile, (char*)filename, 63);
		}else if(strcmp(ext, ".loadout") == 0) // LoadoutFile .loadout
		{
			if(vehtype->LoadoutFile[0] == '\0' || override)
				strncpy((char*)vehtype->LoadoutFile, (char*)filename, 63);
		}else if(strcmp(ext, ".xsound") == 0) // SoundFile .xsound
		{
			if(vehtype->SoundFile[0] == '\0' || override)
				strncpy((char*)vehtype->SoundFile, (char*)filename, 63);
		}else
		{
		}
	}

	xmlFree(filename);
}

static void _vehxmlParseMass(vehType* vehtype, xmlNodePtr massnode)
{
	int i = 0;
	for(xmlNodePtr node = massnode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcmp((xmlChar*)"mass", node->name) != 0) continue;
		if(i >= VEH_MAX_INERTIA_COMPONENTS) break;

		vehInertiaComponent* in = &vehtype->Inertia[i];
		xmlChar* xmlstr;

		xmlstr = xmlGetProp(node, (xmlChar*)"name");
		if(xmlstr)
		{
			strncpy(in->name, (char*)xmlstr, 63);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"mass");
		if(xmlstr)
		{
			in->mass = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"volume");
		if(xmlstr)
		{
			in->vol = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"modelcmx");
		if(xmlstr)
		{
			in->cm.x = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"modelcmy");
		if(xmlstr)
		{
			in->cm.y = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"modelcmz");
		if(xmlstr)
		{
			in->cm.z = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiaxx");
		if(xmlstr)
		{
			in->I[0][0] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiayy");
		if(xmlstr)
		{
			in->I[1][1] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiazz");
		if(xmlstr)
		{
			in->I[2][2] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiaxy");
		if(xmlstr)
		{
			in->I[0][1] = in->I[1][0] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiaxz");
		if(xmlstr)
		{
			in->I[0][2] = in->I[2][0] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiayz");
		if(xmlstr)
		{
			in->I[1][2] = in->I[2][1] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiayx");
		if(xmlstr)
		{
			in->I[0][1] = in->I[1][0] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiazx");
		if(xmlstr)
		{
			in->I[0][2] = in->I[2][0] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}

		xmlstr = xmlGetProp(node, (xmlChar*)"unitinertiazy");
		if(xmlstr)
		{
			in->I[1][2] = in->I[2][1] = atof((char*)xmlstr);
			xmlFree(xmlstr);
		}
		i++;
	}
}


static UINT32 _ReadSoundFile(vehType* CurrentVehicle, UCHAR* ModelDirectory, UCHAR* InVarContents)
{
	SINT32 VehicleSoundFile;
	UCHAR Buffer[256];
	UCHAR FileName[128];
	UCHAR* VarName;
	UCHAR* VarContents;
	SINT32 Num;

	if (CurrentVehicle == nullptr || ModelDirectory == nullptr || InVarContents == nullptr)
		return 0;

	sprintf((char*)FileName,"vehicles\\%s\\%s",ModelDirectory,InVarContents);

	VehicleSoundFile = fioOpen((char*)FileName);

	if (VehicleSoundFile == -1)
	{
		Write_Debug("Can't find Sound file %s for vehicle %s", FileName, CurrentVehicle->Id);
		return 0;
	}

	Num = -1;
	while (fioGetString(VehicleSoundFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
			continue;

		if (strlen((char*)Buffer) < 2)
			continue;

		mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

		if (VarName == nullptr || VarContents == nullptr)
		{
			if (strcmp((char*)VarName,"newspeedrange") == 0)
			{
				Num++;
				if (Num >= VEH_MAX_ENGINESOUND_SPEED_RANGES)
				{
					Write_Debug("Too many speed ranges in file %s",FileName);
					Num--;
				}
			}
			continue;
		}

		if(strcmp((char*)VarName,"engine") == 0)
		{
			strncpy((char*)CurrentVehicle->EngineSound[0], (char*)VarContents, 31);
			continue;
		}else if(strcmp((char*)VarName,"engine2") == 0)
		{
			strncpy((char*)CurrentVehicle->EngineSound[1], (char*)VarContents, 31);
			continue;
		}else if(strcmp((char*)VarName,"engine3") == 0)
		{
			strncpy((char*)CurrentVehicle->EngineSound[2], (char*)VarContents, 31);
			continue;
		}else if(strcmp((char*)VarName,"engine4") == 0)
		{
			strncpy((char*)CurrentVehicle->EngineSound[3], (char*)VarContents, 31);
			continue;
		}else if(strcmp((char*)VarName,"stukasiren") == 0)
		{
			strncpy((char*)CurrentVehicle->StukaSound, (char*)VarContents, 31);
			continue;
		}

		if (Num >= 0)
		{
			if(strcmp((char*)VarName,"enginespeedstart") == 0)
			{
				CurrentVehicle->EngineSpeedStart[Num] = (FLOAT32)atof((char*)VarContents);
				continue;
			}
			else if(strcmp((char*)VarName,"enginespeedend") == 0)
			{
				CurrentVehicle->EngineSpeedEnd[Num] = (FLOAT32)atof((char*)VarContents);
				continue;
			}
			else if(strcmp((char*)VarName,"enginepitchstart") == 0)
			{
				CurrentVehicle->EnginePitchStart[Num] = (FLOAT32)atof((char*)VarContents);
				continue;
			}
			else if(strcmp((char*)VarName,"enginepitchend") == 0)
			{
				CurrentVehicle->EnginePitchEnd[Num] = (FLOAT32)atof((char*)VarContents);
				continue;
			}
		}

        if(strcmp((char*)VarName, "hastreadsound") == 0)
        {
            CurrentVehicle->bHasTreadSound = atoi((char*)VarContents);
			continue;
        }

        if(strcmp((char*)VarName, "hasbrakesound") == 0)
        {
            CurrentVehicle->bHasBrakeSound = atoi((char*)VarContents);
			continue;
        }

        if(strcmp((char*)VarName, "rollsound") == 0)
        {
            strncpy((char*)CurrentVehicle->szTreadSound, (char*)VarContents, 31);
			continue;
        }

        if(strcmp((char*)VarName, "brakesound") == 0)
        {
            strncpy((char*)CurrentVehicle->szBrakeSound, (char*)VarContents, 31);
			continue;
        }
    }

	fioClose(VehicleSoundFile);

	return 0;
}

static UINT32 _ReadLoadoutFile(vehType * CurrentVehicle,
							   UCHAR * ModelDirectory,
							   UCHAR * InVarContents)
{
SINT32 VehicleLoadoutFile;
UCHAR Buffer[256];
UCHAR FileName[128];
SINT32 CurrentMount;
SINT32 Loadout;
SINT32 CurrentWeapon;
SINT32 TempNum;
UCHAR* VarName;
UCHAR* VarContents;

	if (CurrentVehicle == nullptr || ModelDirectory == nullptr || InVarContents == nullptr)
	{
		return 0;
	}

	sprintf((char*)FileName,"vehicles\\%s\\%s",ModelDirectory,InVarContents);

	VehicleLoadoutFile = fioOpen((char*)FileName);

	if (VehicleLoadoutFile == -1)
	{
		Write_Debug("Can't find Loadout file %s for vehicle %s",
					FileName,
					CurrentVehicle->Id);
		return 0;
	}

	CurrentMount = -1;
	Loadout = -1;
	CurrentWeapon = -1;

	while (fioGetString(VehicleLoadoutFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (strlen((char*)Buffer) < 2)
		{
			continue;
		}

		mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

		if (VarName == nullptr || VarContents == nullptr)
		{
			continue;
		}

		if (strncmp((char*)VarName,"sound",5) == 0 &&
			strlen((char*)VarName) == 6)
		{
			TempNum = 0;
			sscanf((char*)VarName,"sound%d",&TempNum);
			if (TempNum > 0 && TempNum <= VEH_MAX_MOUNTSOUNDS)
			{
				strcpy((char*)CurrentVehicle->MountSounds[TempNum-1],(char*)VarContents);
			}
			continue;
		}

		if (strcmp((char*)VarName,"loadout") == 0)
		{
			Loadout = atoi((char*)VarContents) - 1;
			CurrentMount = -1;
			continue;
		}

		if (Loadout < 0 || Loadout > VEH_MAX_LOADOUTS)
		{
			continue;
		}

		if (strcmp((char*)VarName,"mountnum") == 0)
		{
			CurrentMount = atoi((char*)VarContents) - 1;
			CurrentWeapon = -1;
			continue;
		}

		if (CurrentMount < 0 || CurrentMount > VEH_MAX_MOUNTS)
		{
			continue;
		}

		if (strcmp((char*)VarName,"mountsound") == 0)
		{
			TempNum = 0;
			sscanf((char*)VarContents,"sound%d",&TempNum);
			if (TempNum > 0 && TempNum <= VEH_MAX_MOUNTSOUNDS)
			{
				CurrentVehicle->Loadout[Loadout].MountSoundNum[CurrentMount] = TempNum;
			}
			continue;
		}

		if (strcmp((char*)VarName,"mountobject") == 0)
		{
			strcpy((char*)CurrentVehicle->Loadout[Loadout].MountObject[CurrentMount],(char*)VarContents);
			continue;
		}
		if (strcmp((char*)VarName,"weapon") == 0 && strlen((char*)VarName) == 6)
		{
			CurrentWeapon++;
			weapFindWeaponTypeByName(&(CurrentVehicle->Loadout[Loadout].WeaponTypes[CurrentMount][CurrentWeapon]),VarContents);
			if (CurrentVehicle->Loadout[Loadout].WeaponTypes[CurrentMount][CurrentWeapon] == nullptr)
			{
				Write_Debug("Unrecognized weapon %s, in mount %d, loadout %d",
							VarContents,
							CurrentMount,
							Loadout);
				CurrentWeapon--;
			}
			continue;
		}
		if (strcmp((char*)VarName,"weaponobject") == 0 && strlen((char*)VarName) != 6)
		{
			if (CurrentWeapon >= 0 && CurrentWeapon < VEH_MAX_WEAPONS_PER_MOUNT)
			{
				strcpy((char*)CurrentVehicle->Loadout[Loadout].WeaponObject[CurrentMount][CurrentWeapon],(char*)VarContents);
			}
			continue;
		}
		if (strcmp((char*)VarName,"recoilnum") == 0)
		{
			CurrentVehicle->Loadout[Loadout].RecoilNum[CurrentMount] = atoi((SCHAR*)VarContents);
			continue;
		}
		if (strcmp((char*)VarName,"recoiltime") == 0)
		{
			CurrentVehicle->Loadout[Loadout].RecoilTime[CurrentMount] = (FLOAT32)atof((SCHAR*)VarContents);
			continue;
		}

        if(strcmp((SCHAR*)VarName, "shockdust") == 0)
        {
            CurrentVehicle->Loadout[Loadout].WeaponTypes[CurrentMount][CurrentWeapon]->bShockDust = (LOGICAL)strtoul((SCHAR*)VarContents, nullptr, 10);
			continue;
		}

        if(strcmp((SCHAR*)VarName, "maxoversupply") == 0)
        {
            CurrentVehicle->Loadout[Loadout].WeaponTypes[CurrentMount][CurrentWeapon]->MaxOverSupply = atoi((SCHAR*)VarContents);
			continue;
		}

		// percent of resupply given to this weapon by infantry resuppliers
        if(strcmp((SCHAR*)VarName, "infresupply") == 0)
        {
			CurrentVehicle->Loadout[Loadout].WeaponTypes[CurrentMount][CurrentWeapon]->InfResupply = (FLOAT32)atof((SCHAR*)VarContents);
			continue;
		}
	}

	fioClose(VehicleLoadoutFile);

	return 0;
}

static UINT32 _DetermineBlockedCondition(SINT8 * VarContents)
{
UINT32 RetVal;

	RetVal = 0;

	if(VarContents && VarContents[0])
	{
		if (strcmp(VarContents,"undeployed") == 0)
		{
			RetVal = VEH_TURRET_NEUTRAL_ON_BOMBBAYCLOSED;
		}
		if (strcmp(VarContents,"deployed") == 0)
		{
			RetVal = VEH_TURRET_NEUTRAL_ON_BOMBBAYOPEN;
		}
		if (strcmp(VarContents,"canopyopen") == 0)
		{
			RetVal = VEH_TURRET_NEUTRAL_ON_CANOPYOPEN;
		}
		if (strcmp(VarContents,"canopyclosed") == 0)
		{
			RetVal = VEH_TURRET_NEUTRAL_ON_CANOPYCLOSED;
		}
		if (strcmp(VarContents,"hatchopen") == 0)
		{
			RetVal = VEH_TURRET_NEUTRAL_ON_HATCHOPEN;
		}
		if (strcmp(VarContents,"hatchclosed") == 0)
		{
			RetVal = VEH_TURRET_NEUTRAL_ON_HATCHCLOSED;
		}

	}

	return RetVal;
}

static UINT32 _ReadTurretFile(vehType * CurrentVehicle,
							   UCHAR * ModelDirectory,
							   UCHAR * InVarContents)
{
SINT32 VehicleTurretFile;
UCHAR Buffer[256];
UCHAR FileName[128];
SINT8 TempStr[128];
SINT32 CurrentPos;
SINT32 TurretNum;
UCHAR* VarName;
UCHAR* VarContents;
UCHAR* DummyPtr;

UCHAR _BombsightTypeNames[3][32] =
{
	"none",
	"britmk9a",
	"germlotfec7b"
};

	if (CurrentVehicle == nullptr || ModelDirectory == nullptr || InVarContents == nullptr)
	{
		return 0;
	}

	sprintf((char*)FileName,"vehicles\\%s\\%s",ModelDirectory,InVarContents);

	VehicleTurretFile = fioOpen((char*)FileName);

	if (VehicleTurretFile == -1)
	{
		Write_Debug("Can't find turret file %s for vehicle %s",
					FileName,
					CurrentVehicle->Id);
		return 0;
	}

	CurrentPos = -1;
	TurretNum = -1;

	while (fioGetString(VehicleTurretFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (strlen((char*)Buffer) < 2)
		{
			continue;
		}

		mlUtilityGetVarnameAndContents1(&VarName,&VarContents,Buffer);

		if (VarName == nullptr || VarContents == nullptr)
		{
			continue;
		}

		if (VarContents[0] == '=')
		{
			mlUtilityGetVarnameAndContents1(&VarContents,&DummyPtr,VarContents);
		}

		if (strcmp((char*)VarName,"multicrewpos") == 0)
		{
		SINT32 Pos,Pos1;

			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			Pos = atoi((char*)VarName);
			if (Pos < 1 || Pos > VEH_MAX_CREW_POSITIONS)
			{
				continue;
			}
			Pos1 = atoi((char*)VarContents);
			if (Pos1 < 1 || Pos1 > 2)
			{
				Pos1 = 1;
			}

			CurrentVehicle->MultiCrewPos[Pos-1] = Pos1;
		}

		if (strcmp((char*)VarName,"position") == 0)
		{
			CurrentPos = atoi((char*)VarContents) - 1;
			TurretNum++;
			continue;
		}

		if (CurrentPos < 0 || CurrentPos > VEH_MAX_CREW_POSITIONS)
		{
			continue;
		}

		if (TurretNum < VEH_MAX_CREW_POSITIONS && TurretNum >= 0)
		{
			CurrentVehicle->TurretNum[CurrentPos] = TurretNum + 1;
		}

		if (strcmp((char*)VarName,"elevationrate") == 0)
		{
//            CurrentVehicle->TurretElevateRate[CurrentPos] = (FLOAT32)atof((char*)VarContents);
            CurrentVehicle->TurretElevateRate[CurrentPos] = (FLOAT32)atof((char*)VarContents) * (FLOAT32)4.0;
		}

		if (strcmp((char*)VarName,"traverserate") == 0)
		{
//            CurrentVehicle->TurretTraverseRate[CurrentPos] = (FLOAT32)atof((char*)VarContents);
            CurrentVehicle->TurretTraverseRate[CurrentPos] = (FLOAT32)atof((char*)VarContents) * (FLOAT32)4.0;
        }

        if(strcmp((char*)VarName, "turretelevate") == 0)
        {
            CurrentVehicle->turretSound[CurrentPos]["turretelevate"] = (char*)VarContents;
        }

        if(strcmp((char*)VarName, "turrettraverse") == 0)
        {
            CurrentVehicle->turretSound[CurrentPos]["turrettraverse"] = (char*)VarContents;
        }

        if(strcmp((char*)VarName, "turretgeneral") == 0)
        {
            CurrentVehicle->turretSound[CurrentPos]["turretgeneral"] = (char*)VarContents;
        }

		if (strcmp((char*)VarName,"type") == 0)
		{
			if (strcmp((char*)VarContents,"360turret") == 0)
			{
				CurrentVehicle->TurretType[CurrentPos] = VEH_TURRET_TYPE_360;
			}
			else
			if (strcmp((char*)VarContents,"limitedturret") == 0)
			{
				CurrentVehicle->TurretType[CurrentPos] = VEH_TURRET_TYPE_LIMITED;
			}
			else
			if (strcmp((char*)VarContents,"flexible") == 0)
			{
				CurrentVehicle->TurretType[CurrentPos] = VEH_TURRET_TYPE_FLEXIBLE;
			}
			else
			if (strcmp((char*)VarContents, "gravity") == 0)
			{
				CurrentVehicle->TurretType[CurrentPos] = VEH_TURRET_TYPE_GRAVITY;

			}
			else
			{
				Write_Debug("Unrecognized turret %d type \"%s\"",
							CurrentPos,
							VarContents);
			}
		}

		if (strcmp((char*)VarName,"hoistturret") == 0)
		{
			CurrentVehicle->TurretAux[CurrentPos] = atoi((char*)VarContents);
		}

		if (strcmp((char*)VarName,"neutral") == 0)
		{
			if (strcmp((char*)VarContents,"canopyclosed") == 0)
			{
				CurrentVehicle->TurretNeutral[CurrentPos] = VEH_TURRET_NEUTRAL_ON_CANOPYCLOSED;
			}
			else
			if (strcmp((char*)VarContents,"canopyopen") == 0)
			{
				CurrentVehicle->TurretNeutral[CurrentPos] = VEH_TURRET_NEUTRAL_ON_CANOPYOPEN;
			}
			else
			if (strcmp((char*)VarContents,"hatchclosed") == 0)
			{
				CurrentVehicle->TurretNeutral[CurrentPos] = VEH_TURRET_NEUTRAL_ON_HATCHCLOSED;
			}
			else
			if (strcmp((char*)VarContents,"hatchopen") == 0)
			{
				CurrentVehicle->TurretNeutral[CurrentPos] = VEH_TURRET_NEUTRAL_ON_HATCHOPEN;
			}
			else
			if (strcmp((char*)VarContents,"bombbayclosed") == 0)
			{
				CurrentVehicle->TurretNeutral[CurrentPos] = VEH_TURRET_NEUTRAL_ON_BOMBBAYCLOSED;
			}
			else
			if (strcmp((char*)VarContents,"bombbayopen") == 0)
			{
				CurrentVehicle->TurretNeutral[CurrentPos] = VEH_TURRET_NEUTRAL_ON_BOMBBAYOPEN;
			}

		}

		if (strcmp((char*)VarName,"neutraltraverse") == 0)
		{
			CurrentVehicle->TurretTraverseNeutral[CurrentPos] = (FLOAT32)atof((char*)VarContents);
		}
		if (strcmp((char*)VarName,"neutralelevate") == 0)
		{
			CurrentVehicle->TurretElevateNeutral[CurrentPos] = (FLOAT32)atof((char*)VarContents);
		}

		if (strcmp((char*)VarName,"gunsightfile") == 0)
		{
			sprintf((char*)TempStr,"vehicles\\%s\\%s",ModelDirectory,VarContents);
			strcpy(CurrentVehicle->GunsightFile[CurrentPos],TempStr);
		}

		if (strcmp((char*)VarName,"instgunsightfile") == 0)
		{
			sprintf((char*)TempStr,"vehicles\\%s\\%s",ModelDirectory,VarContents);
			strcpy(CurrentVehicle->InstGunsightFile[CurrentPos],TempStr);
		}

		if (strcmp((char*)VarName,"gunsightalignmentmountobject") == 0)
		{
			strcpy(CurrentVehicle->GunsightAlignmentObj[CurrentPos],(char*)VarContents);
		}
		if (strcmp((char*)VarName,"instgunsightalignmentmountobject") == 0)
		{
			strcpy(CurrentVehicle->InstGunsightAlignmentObj[CurrentPos],(char*)VarContents);
		}
		if (strcmp((char*)VarName,"gunsightalignmentrange") == 0)
		{
			CurrentVehicle->GunsightAlignmentRange[CurrentPos] = (FLOAT32)(atof((char*)VarContents));
		}
		if (strcmp((char*)VarName,"gunsightblocked") == 0)
		{
			CurrentVehicle->GunsightBlocked[CurrentPos] = _DetermineBlockedCondition((char*)VarContents);
		}
		if (strcmp((char*)VarName,"instsightblocked") == 0)
		{
			CurrentVehicle->InstGunsightBlocked[CurrentPos] = _DetermineBlockedCondition((char*)VarContents);
		}
		if (strcmp((char*)VarName,"bombsight") == 0)
		{
			mlUtilityFindVarname((UINT32*)&CurrentVehicle->BombSightType,
										VarContents,
										(UCHAR*)&_BombsightTypeNames[0][0],
										32,
										3);
			CurrentVehicle->BombSightPosition = CurrentPos;
		}
		if (strcmp((char*)VarName,"bombmount") == 0)
		{
			strcpy(CurrentVehicle->BombSightMount,(char*)VarContents);
		}
	}

	fioClose(VehicleTurretFile);

	return 0;
}

static UINT32 _ReadDamageFile(vehType * CurrentVehicle,
							   UCHAR * ModelDirectory,
							   UCHAR * VarContents)
{
SINT32 VehicleDamageFile;
UCHAR Buffer[256];
UCHAR* BufferPtr;
UCHAR FileName[128];
UCHAR* VarName;
UCHAR* VarContents1;

	if (CurrentVehicle == nullptr || ModelDirectory == nullptr || VarContents == nullptr)
	{
		return 0;
	}

	sprintf((char*)FileName,"vehicles\\%s\\%s",ModelDirectory,VarContents);

	VehicleDamageFile = fioOpen((char*)FileName);

	if (VehicleDamageFile == -1)
	{
		Write_Debug("Can't find damage hit list file %s for vehicle %s",
					FileName,
					CurrentVehicle->Id);
		return 0;
	}

	CurrentVehicle->NumDamageObjects = 0;

	while (fioGetString(VehicleDamageFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (strlen((char*)Buffer) < 2)
		{
			continue;
		}

		if (Buffer[0] == '-')
		{
			BufferPtr = &Buffer[1];
			mlUtilityGetVarnameAndContents(&VarName,&VarContents1,BufferPtr);

			if (strcmp((SCHAR*)VarName,"hitsound1") == 0)
			{
				strcpy(CurrentVehicle->HitSounds[0],(SCHAR*)VarContents1);
			}
			else
			if (strcmp((SCHAR*)VarName,"hitsound2") == 0)
			{
				strcpy(CurrentVehicle->HitSounds[1],(SCHAR*)VarContents1);
			}
			else
			if (strcmp((SCHAR*)VarName,"hitsound3") == 0)
			{
				strcpy(CurrentVehicle->HitSounds[2],(SCHAR*)VarContents1);
			}
			else
			if (strcmp((SCHAR*)VarName,"hitenergymaxvolume") == 0)
			{
				CurrentVehicle->HitMaxVolumeEnergy = (FLOAT32)atof((SCHAR*)VarContents1);
			}
            else if(!strcmp((SCHAR*)VarName, "shraphitsound1"))
            {
                strncpy(CurrentVehicle->ShrapHitSounds[0],
                       (SCHAR*)VarContents1, 31);
            }
            else if(!strcmp((SCHAR*)VarName, "shraphitsound2"))
            {
                 strncpy(CurrentVehicle->ShrapHitSounds[1],
                        (SCHAR*)VarContents1, 31);
            }
            else if(!strcmp((SCHAR*)VarName, "shraphitsound3"))
            {
                 strncpy(CurrentVehicle->ShrapHitSounds[2],
                        (SCHAR*)VarContents1, 31);
            }
			else
			{
				Write_Debug("Invalid line -%s=%s in %s",
							VarName,
							VarContents1,
							FileName);
			}
		}
		else
		{

			BufferPtr = (UCHAR*)strtok((char*)Buffer,",");
			strcpy((char*)CurrentVehicle->DamageObjects[CurrentVehicle->NumDamageObjects],(char*)Buffer);
			BufferPtr = (UCHAR*)strtok(nullptr,",");
			if (BufferPtr != nullptr)
			{
				CurrentVehicle->DamageArmorThickness[CurrentVehicle->NumDamageObjects] =
							(FLOAT32)(atof((char*)BufferPtr) / 1000.0); // thickness is in mm, need it in meters

				BufferPtr = (UCHAR*)strtok(nullptr,",");

				if (BufferPtr != nullptr)
				{
					CurrentVehicle->DamageFaceHardness[CurrentVehicle->NumDamageObjects] =
								(FLOAT32)atof((char*)BufferPtr);

					BufferPtr = (UCHAR*)strtok(nullptr,",");

					if (BufferPtr != nullptr)
					{
						CurrentVehicle->DamageAverageHardness[CurrentVehicle->NumDamageObjects] =
									(FLOAT32)atof((char*)BufferPtr);

						BufferPtr = (UCHAR*)strtok(nullptr,",");
						if (BufferPtr != nullptr)
						{
							CurrentVehicle->DamageConcussionResistance[CurrentVehicle->NumDamageObjects] =
											atoi((char*)BufferPtr);

							BufferPtr = (UCHAR*)strtok(nullptr,",");
							if(BufferPtr != nullptr)
							{
								CurrentVehicle->DamageThreshold[CurrentVehicle->NumDamageObjects] =
											(FLOAT32)atof((char*)BufferPtr);
							}
						}
					}

				}
			}
			CurrentVehicle->NumDamageObjects++;
		}
	}

	fioClose(VehicleDamageFile);

	return 0;
}
static UINT32 _ReadStateFile(vehType* CurrentVehicle,  UCHAR* ModelDirectory,  UCHAR* VarContentsIn)
{
	SINT32 VehicleStateFile;
	UCHAR Buffer[256];
	UCHAR FileName[128];
	SINT32 StateNum;
	vehanimVehicleStateType * StateType;
	UCHAR * VarName;
	UCHAR * VarContents;
	SINT32 VisDamCount;

	if (CurrentVehicle == nullptr || ModelDirectory == nullptr || VarContentsIn == nullptr) return 0;

	StateNum = 0;
	VisDamCount = 0;

	sprintf((char*)FileName,"vehicles\\%s\\%s",ModelDirectory,VarContentsIn);
	VehicleStateFile = fioOpen((char*)FileName);

	if (VehicleStateFile == -1)
	{
		return 0;
	}

	while (fioGetString(VehicleStateFile,(char*)Buffer,255) != 0 &&  StateNum < VEH_MAX_STATE_BITS)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		VarName = nullptr;
		mlUtilityGetVarnameAndContents1(&VarName,&VarContents,Buffer);

		if (VarName == nullptr || VarContents == nullptr)
		{
			continue;
		}

		StateType = &CurrentVehicle->StateType[StateNum];

		if (strcmp((char*)VarName,"dofstate") == 0)
		{
			StateType->StateType = VehAnim::STATE_TYPE_DOF;

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			StateType->MultiCrewPos = atoi((char*)VarName);

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			strcpy((char*)StateType->Name,(char*)VarName);

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr)
			{
				continue;
			}

			if (strcmp((char*)VarName,"gear") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_GEAR;
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->Data = atoi((char*)VarContents); // gear animation time in ms
			}
			else if (strcmp((char*)VarName,"flap") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_FLAPS;

				sscanf((char*)VarContents,"%d %d",&StateType->Data,&StateType->Data1);
				//StateType->Data = atoi(VarContents);
			}
			else if (strcmp((char*)VarName,"prop") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_PROP;
				sscanf((char*)VarContents,"%d %d",&StateType->Data,&StateType->Data1);
			}
			else if (strcmp((char*)VarName,"canopy") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_CANOPY;
				sscanf((char*)VarContents,"%d %d",&StateType->Data,&StateType->Data1);
			}
			else if (strcmp((char*)VarName,"hatch") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_HATCH;
				sscanf((char*)VarContents,"%d %d",&StateType->Data,&StateType->Data1);
			}
			else if (strcmp((char*)VarName,"bombbaydoor") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_BOMBBAYDOOR;
				StateType->Data = atoi((SCHAR*)VarContents);
			}
			else if (strcmp((char*)VarName,"divebrake") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_DIVEBRAKE;
				StateType->Data = atoi((SCHAR*)VarContents);
			}
			else
			{
			}


			StateNum++;
			continue;
		}
		else if (strcmp((char*)VarName,"smokestate") == 0)
		{
			StateType->StateType = VehAnim::STATE_TYPE_SMOKE;

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			StateType->MultiCrewPos = atoi((char*)VarName);

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			strcpy((char*)StateType->Name,(char*)VarName);

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);

			if (VarName == nullptr)
			{
				continue;
			}

			strcpy((char*)StateType->Emitter,(char*)VarName);

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);

			if (VarName == nullptr)
			{
				continue;
			}

			if (strcmp((char*)VarName,"exhaust") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_EXHAUST;
				StateType->Data = atoi((char*)VarContents);
			}
			else if (strcmp((char*)VarName,"wingvortex") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_WINGTIPVORTEX;
				StateType->Data = atoi((char*)VarContents);
			}
			else if (strcmp((char*)VarName,"fuelfire") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_FUELFIRE;
				StateType->Data = atoi((char*)VarContents);
			}
			else if (strcmp((char*)VarName,"fuelleak") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_FUELLEAK;
				StateType->Data = atoi((char*)VarContents);
			}
			else if (strcmp((char*)VarName,"oilleak") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_OILLEAK;
				StateType->Data = atoi((char*)VarContents);
			}
			else if (strcmp((char*)VarName,"glycolleak") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_GLYCOLLEAK;
				sscanf((char*)VarContents,"%d %d",&StateType->Data,&StateType->Data1);
			}
			else if (strcmp((char*)VarName,"enginefire") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_ENGINEFIRE;
				StateType->Data = atoi((char*)VarContents);
			}
			else if (strcmp((char*)VarName,"tankfire") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_TANKFIRE;
				StateType->Data = atoi((char*)VarContents);
			}
            else if(strcmp((char*)VarName, "treaddust") == 0)
            {
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_TREADDUST;
				StateType->Data = atoi((char*)VarContents);
            }
			else
			{
			}
			StateNum++;
			continue;
		}
		else if (strcmp((char*)VarName,"sound") == 0)
		{
			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}

			StateType->MultiCrewPos = atoi((char*)VarName);

			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}

			strcpy((char*)StateType->Name,(char*)VarName);

			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr)
			{
				continue;
			}

			StateType->StateType = VehAnim::STATE_TYPE_SOUND;
			if (strcmp((char*)VarName,"enginesound") == 0)
			{
				if (VarContents == nullptr)
				{
					continue;
				}
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_ENGINESOUND;
				StateType->Data = atoi((char*)VarContents);
			}

			if (strcmp((char*)VarName,"stukasiren") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_STUKASOUND;
			}

			StateNum++;
			continue;
		}
		else if (strcmp((char*)VarName,"effect") == 0)
		{
			StateType->StateType = VehAnim::STATE_TYPE_EFFECT;
			VarName = nullptr;

			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			StateType->MultiCrewPos = atoi((char*)VarName);

			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);
			if (VarName == nullptr || VarContents == nullptr)
			{
				continue;
			}
			strcpy((char*)StateType->Name,(char*)VarName);

			if (strcmp((char*)VarContents,"crashexplosion") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_CRASHEXPLOSION;
			}

			if (strcmp((char*)VarContents,"crashejectriders") == 0)
			{
				StateType->StateSubType = VehAnim::STATE_SUBTYPE_CRASHEJECTRIDERS;
			}

			StateNum++;
			continue;
		}else if (!strcmp((char*)VarName,"visdamage")) // visible damage states
		{
			StateType->StateType = VehAnim::STATE_TYPE_VISDAMAGE;
			VarName = nullptr;

			mlUtilityGetVarnameAndContents1(&VarName, &VarContents, VarContents);
			if(VarName)
            {
				strncpy(StateType->Name,(char*)VarName, 31);

                if(!strncmp((char*)VarName, "turret", 6) ||
                   !strncmp((char*)VarName, "engine", 6) ||
                   !strncmp((char*)VarName, "fuel", 4) ||
                   !strncmp((char*)VarName, "lwing", 5) ||
                   !strncmp((char*)VarName, "rwing", 5) ||
                   !strncmp((char*)VarName, "hulk", 4))
                {
                    // Used to create explosion on damaged objects
                    StateType->StateSubType = VehAnim::STATE_SUBTYPE_CRASHEXPLOSION;
                }
                else if(!strncmp((char*)VarName, "backleftwheel", 13) ||
                        !strncmp((char*)VarName, "backrightwheel", 14) ||
                        !strncmp((char*)VarName, "frontrightwheel", 15) ||
                        !strncmp((char*)VarName, "frontleftwheel", 14) ||
                        !strncmp((char*)VarName, "lefttread", 9) ||
                        !strncmp((char*)VarName, "righttread", 10) ||
                        !strncmp((char*)VarName, "tail", 4) ||
                        !strncmp((char*)VarName, "lhstab", 6) ||
                        !strncmp((char*)VarName, "rhstab", 6) ||
                        !strncmp((char*)VarName, "cvstab", 6) ||
                        !strncmp((char*)VarName, "prop", 4) ||
                        !strncmp((char*)VarName, "wheel1", 6) ||
                        !strncmp((char*)VarName, "wheel2", 6) ||
                        !strncmp((char*)VarName, "wheel3", 6) ||
                        !strncmp((char*)VarName, "wheel4", 6) ||
                        !strncmp((char*)VarName, "wheel5", 6) ||
                        !strncmp((char*)VarName, "wheel6", 6) ||
                        !strncmp((char*)VarName, "wheel7", 6) ||
                        !strncmp((char*)VarName, "wheel8", 6))
                {
                    // Used to throw debris/smoke when wheel/tracks are blown off.
                    StateType->StateSubType = VehAnim::STATE_SUBTYPE_WHEELEJECT;
                }
            }

			if(VarContents)
			{
				mlUtilityGetVarnameAndContents1(&VarName, &VarContents, VarContents);
				if(VarName)
					strncpy(StateType->MassObject,(char*)VarName, 31);
			}

			if(VarContents)
			{
				mlUtilityGetVarnameAndContents1(&VarName, &VarContents, VarContents);
				if(VarName)
					strncpy(StateType->Emitter,(char*)VarName, 31);
			}

			if(VarContents)
			{
				mlUtilityGetVarnameAndContents1(&VarName, &VarContents, VarContents);
				if(VarName)
					StateType->EjectSpeed = atof((char*)VarName);
			}

			if(VarContents)
			{
				strncpy(StateType->ParentVisDamage,(char*)VarContents, 31);
			}

			StateType->Data = VisDamCount++;

			StateNum++;
			continue;
		}
	}

	CurrentVehicle->NumStates = StateNum;

	fioClose(VehicleStateFile);

	return 0;
}

static void _ReadFlatFiles(vehType* vehtype)
{
	if(vehtype->StateFile[0])
		_ReadStateFile(vehtype, vehtype->ModelDirectory, vehtype->StateFile);

	if(vehtype->DamageFile[0])
		_ReadDamageFile(vehtype, vehtype->ModelDirectory, vehtype->DamageFile);

	if(vehtype->TurretFile[0])
		_ReadTurretFile(vehtype, vehtype->ModelDirectory, vehtype->TurretFile);

	if(vehtype->LoadoutFile[0])
		_ReadLoadoutFile(vehtype, vehtype->ModelDirectory, vehtype->LoadoutFile);

	if(vehtype->SoundFile[0])
		_ReadSoundFile(vehtype, vehtype->ModelDirectory, vehtype->SoundFile);

	vehtype->VehicleIsMultiCrew = FALSE;

	for (int j=0;j<VEH_MAX_CREW_POSITIONS;j++)
	{
		if (vehtype->MultiCrewPos[j] > 1)
		{
			vehtype->VehicleIsMultiCrew = TRUE;
			break;
		}
	}
}
