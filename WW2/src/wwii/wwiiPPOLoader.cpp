#include <syscfg.h> // Path is project specific.
#include <cstring>
#include <string.h>

#include "xml/XMLLoader2.h"
#include "cfg/cfgPrefs.h"
#include "wwii/wwiiPPOLoader.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "dbug/dbug.h"

#define PPO_SPEC "vehicles/ground/groundman/ppo.xml"

#define XML_TAG_HANDLER_IMPL(handler) bool PPOSpecLoader::handler(xmlNode* node, bool first, void* userData)

#if platOS != OS_WIN32
#define stricmp strcasecmp
#endif

#if platBuild == TEUL_SERVER
#define strlcpy strncpy
#endif

PPOSpecLoader::PPOSpecLoader() : XMLLoader2<PPOSpecLoader>()
{
	AddHandler("defaults", &PPOSpecLoader::DoDefaults, this);
	AddHandler("ppo", &PPOSpecLoader::DoPPO, this);
	AddHandler("param", &PPOSpecLoader::DoParam, this);
}

void PPOSpecLoader::loadSpecMap()
{
        dbugLog(_FLD, "Loading PPO specifications...");
	_ppoSpecMap.clear();
	SetPath(PPO_SPEC);
	fioXmlRegisterInputCallbacks();
	Load();
        dbugLog(_FLD, "Done loading PPO specifications, %d loaded.", _ppoSpecMap.size());
}

// tag handlers
XML_TAG_HANDLER_IMPL(DoDefaults)
{
	if(first)
		ProcessChildren(node, &defaults);

	return false;
}


XML_TAG_HANDLER_IMPL(DoPPO)
{
	if(first)
	{
		PPOSpec spec;

		// set our defaults
		spec.allowInterior		= defaults.allowInterior;
		spec.avoidance			= defaults.avoidance;
		spec.surfaceFlags		= defaults.surfaceFlags;
		spec.avoidFlags			= defaults.avoidFlags;
		spec.buildFlags			= defaults.buildFlags;
		spec.buildTime			= defaults.buildTime;
		spec.coolDown			= defaults.coolDown;
		spec.coolDownEnd		= 0;
		spec.rayAdjust			= defaults.rayAdjust;
		spec.rayTolerance		= defaults.rayTolerance;
		spec.rotation			= defaults.rotation;
		spec.tiltTolerance		= defaults.tiltTolerance;
		spec.viewOffset			= defaults.viewOffset;
		spec.treeCheck			= defaults.treeCheck;
		spec.enemyFacDistance		= defaults.enemyFacDistance;
		spec.friendlyFacDistance	= defaults.friendlyFacDistance;
		spec.targetDistance		= defaults.targetDistance;
		spec.maxSpeed			= defaults.maxSpeed;
		spec.timeout			= defaults.timeout;
		spec.damageThreshold		= defaults.damageThreshold;
		spec.damageCapacity		= defaults.damageCapacity;
		spec.spawnable			= defaults.spawnable;

		for(size_t i = 0; i < defaults.allowSpawn.size(); i++)
			spec.allowSpawn.push_back(defaults.allowSpawn[i]);

		for(size_t i = 0; i < defaults.denySpawn.size(); i++)
			spec.denySpawn.push_back(defaults.denySpawn[i]);

		for(size_t i = 0; i < defaults.allowPlace.size(); i++)
			spec.allowPlace.push_back(defaults.allowPlace[i]);
		
		for(size_t i = 0; i < defaults.denyPlace.size(); i++)
			spec.denyPlace.push_back(defaults.denyPlace[i]);

		for(size_t i = 0; i < defaults.badTextures.size(); i++)
			spec.badTextures.push_back(defaults.badTextures[i]);

		for(size_t i = 0; i < defaults.okTextures.size(); i++)
			spec.okTextures.push_back(defaults.okTextures[i]);

		spec.name  = xmlGetStringFromNode(node, "name");
		spec.description  = xmlGetStringFromNode(node, "description");
		spec.stotype  = atoi(xmlGetStringFromNode(node, "sto-type"));

		// process any param overrides
		ProcessChildren(node, &spec);

		// get the next available id (1 based)
		spec.id = static_cast<SINT32>(_ppoSpecMap.size()) + 1;

		// save our spec in the map
		_ppoSpecMap[spec.id] = spec;
	}

	return false;
}

XML_TAG_HANDLER_IMPL(DoParam)
{
	if(first && userData)
	{
		PPOSpec* spec	= (PPOSpec*)userData;
		std::string name	= xmlGetStringFromNode(node, "name");
		std::string value	= xmlGetStringFromNode(node, "value");

		if(!stricmp(name.c_str(), "modelname"))
		{
			spec->modelName = value;
		}
		else if(!stricmp(name.c_str(), "rayadjust"))
		{
			FLOAT32 x, y, z;
			x = y = z = 0.0f;

			if(sscanf(value.c_str(), "%f,%f,%f", &x, &y, &z) == 3)
			{
				spec->rayAdjust.x = x;
				spec->rayAdjust.y = y;
				spec->rayAdjust.z = z;
			}
		}
		else if(!stricmp(name.c_str(), "avoidance"))
		{
			spec->avoidance = atof(value.c_str());
		}
		else if(!stricmp(name.c_str(), "raytolerance"))
		{
			spec->rayTolerance = atof(value.c_str());
		}
		else if(!stricmp(name.c_str(), "tilttolerance"))
		{
			spec->tiltTolerance = atof(value.c_str());
		}
		else if(!stricmp(name.c_str(), "viewoffset"))
		{
			spec->viewOffset = atof(value.c_str());
		}
		else if(!stricmp(name.c_str(), "rotation"))
		{
			spec->rotation = atof(value.c_str());
		}
		else if(!stricmp(name.c_str(), "cooldown"))
		{
			spec->coolDown = atoi(value.c_str());
		}
		else if(!stricmp(name.c_str(), "buildtime"))
		{
			spec->buildTime = atoi(value.c_str());
		}
		else if(checkDelimitedField(name, value, "buildflags", spec->buildFlags)) {}
		else if(checkDelimitedField(name, value, "avoidflags", spec->avoidFlags)) {}
		else if(checkDelimitedField(name, value, "surfaceflags", spec->surfaceFlags)) {}
		else if(!stricmp(name.c_str(), "allowinterior"))
		{
			spec->allowInterior = false;

			if(!stricmp(value.c_str(), "true"))
				spec->allowInterior = true;
		}
		else if(!stricmp(name.c_str(), "treecheck"))
		{
			FLOAT32 radius = 0.0f;

			if(sscanf(value.c_str(), "%f", &radius) == 1)
				spec->treeCheck = radius;
		}
		else if(checkDelimitedField(name, value, "badtextures", spec->badTextures)) {}
		else if(checkDelimitedField(name, value, "oktextures", spec->okTextures)) {}
		else if(!stricmp(name.c_str(), "maxspeed"))
		{
			FLOAT32 speed;

			if(sscanf(value.c_str(), "%f", &speed) == 1)
				spec->maxSpeed = speed;
		}
		else if(!stricmp(name.c_str(), "enemyfacdist"))
		{
			spec->enemyFacDistance = atoi(value.c_str());
		}
		else if(!stricmp(name.c_str(), "friendlyfacdist"))
		{
			spec->friendlyFacDistance = atoi(value.c_str());
		}
		else if(!stricmp(name.c_str(), "targetdist"))
		{
			spec->targetDistance = atoi(value.c_str());
		}
		else if(!stricmp(name.c_str(), "timeout-minutes"))
		{
			spec->timeout = (TeulTimeMS)(strtof(value.c_str(), nullptr) * 60.0 * 1000.0);
		}
		else if(!stricmp(name.c_str(), "damage-threshold"))
		{
			if (isdigit(value[0]))
				spec->damageThreshold = strtoull(value.c_str(), nullptr, 10);
		}
		else if(!stricmp(name.c_str(), "damage-capacity"))
		{
			if (isdigit(value[0]))
				spec->damageCapacity = strtoull(value.c_str(), nullptr, 10);
		}
		else if(!stricmp(name.c_str(), "spawnable"))
		{
			spec->spawnable = false;

			if(!stricmp(value.c_str(), "true"))
				spec->spawnable = true;
		}
		else if(checkDelimitedField(name, value, "allowspawn", spec->allowSpawn)) {}
		else if(checkDelimitedField(name, value, "denyspawn", spec->denySpawn)) {}
		else if(checkDelimitedField(name, value, "allowplace", spec->allowPlace)) {}
		else if(checkDelimitedField(name, value, "denyplace", spec->denyPlace)) {}

	}

	return true;
}

bool PPOSpecLoader::checkDelimitedField(std::string name, std::string value, 
			const char *check, std::vector<std::string> & stack)
{
	bool retVal = false;

	if(!stricmp(name.c_str(), check))
	{
		stack.clear();

		char* nextval = strtok((char*)value.c_str(),",");

		while(nextval != nullptr)
		{
			stack.push_back(nextval);
			nextval = strtok(nullptr, ",");
		}

		retVal = true;
	}

	return retVal;
}


bool PPOSpecLoader::checkDelimitedField(std::string name, std::string value, 
			const char *check, UINT32 & bitfield)
{
	std::vector<std::string> temp;
	bool retVal = checkDelimitedField(name, value, check, temp);
	if (retVal)
	{
		bitfield = 0;  // first clear out the field first, we might be overriding defaults
		for (size_t i=0; i < temp.size(); i++)
		{
			bitfield |= (1<<(UINT32)atoi(temp[i].c_str()));
		}
	}
	return retVal;
}

PPOSpec* PPOSpecLoader::ppoGetSpec(int subtype)
{
	if(_ppoSpecMap.empty())
		loadSpecMap();

	std::map<int,PPOSpec>::iterator found = _ppoSpecMap.find(subtype);

	if(found != _ppoSpecMap.end())
		return &found->second;

	return nullptr;
}

bool PPOSpecLoader::checkType(const char *pType, int vehtype, bool & retVal, bool setValue)
{
	bool done = true;

	// if no veh type is specified, its a generic "applies to all"
	if (pType == nullptr)
	{
		dbugLog(_FLD, "Don't have type specified, returning %d", setValue);
		retVal = setValue;
	}
	else
	{
		dbugLog(_FLD, "Have a type, check type match");
		// veh type is specified, make sure it matches
		int nType = atoi(pType);	
		if (nType == vehtype)
		{
			dbugLog(_FLD, "Types match, returning %d", setValue);
			retVal = setValue;
		}
		else
		{
			dbugLog(_FLD, "Types don't match, returning %d", retVal);
			done = false;
		}
	}

	return done;
}	

bool PPOSpecLoader::checkList(std::vector<std::string> & tlist, int vehclass, int vehtype, bool allowed, bool setValue)
{
	bool retVal = allowed;

	dbugLog(_FLD, "checkList(list:%d, vehclass=%d, vehtype=%d, allowed=%d, setValue=%d",
		tlist.size(), vehclass, vehtype, allowed, setValue);

	// first lets see if this vehicle class/type is being spacifically mentioned
	for (size_t i=0; i < tlist.size(); i++)
	{
		// String in format Nc/Nt  (Class/Type)
		size_t nmSize = tlist[i].size()+1;
		char *tempClassName = new char[nmSize];
		strlcpy(tempClassName, tlist[i].c_str(), nmSize);
	
		dbugLog(_FLD, "tempClassName = %s", tempClassName);
	
		char *pClass = strtok(tempClassName, "/");
		char *pType = strtok( nullptr, "/" );
		if (pClass != nullptr)
		{
			dbugLog(_FLD, "pClass = %s", pClass);

			// check if veh class matches spec
			int nClass = atoi(pClass);
			if (nClass == vehclass)
			{
				dbugLog(_FLD, "nClass = %d", nClass);

				if (checkType(pType, vehtype, retVal, setValue))
					break;
			}	
			else
			{
				dbugLog(_FLD, "Class doesn't match, returning %d", retVal);
			}
		}
		else
		{
			dbugLog(_FLD, "No class parsed from list");
			if (checkType(pType, vehtype, retVal, setValue))
				break;
		}

		delete tempClassName;
	}

	return retVal;
}

bool PPOSpecLoader::ppoIsSpawnAllowed(int pposubtype, int vehclass, int vehtype)
{
	dbugLog(_FLD, "ppoIsSpawnAllowed( pposubtype = %d, vehclas = %d, vehtype = %d", pposubtype, vehclass, vehtype);
	bool retVal = false;

	PPOSpec *ppo = ppoGetSpec(pposubtype);
	if (ppo != nullptr)
	{
		dbugLog(_FLD, "Found spec %s, spawnable flag %d", ppo->name.c_str(), ppo->spawnable);
		if (ppo->spawnable)
		{
			// first scan through the allow list
			retVal = checkList(ppo->allowSpawn, vehclass, vehtype, retVal, true);

			dbugLog(_FLD, "checkList returned %d for allowSpawn check", retVal);

			// then override with any in the deny list
			retVal = checkList(ppo->denySpawn, vehclass, vehtype, retVal, false);

			dbugLog(_FLD, "checkList returned %d for denySpawn check", retVal);
		}
	}
	else
	{
		dbugLog(_FLW, "Unable to match a spec to subtype %d", pposubtype);
	}

	dbugLog(_FLD, "Spawn allowed flag: %d", retVal);

	return retVal;
}

bool PPOSpecLoader::getAllowedPPOs(int vehclass, int vehtype, std::vector<PPOSpec *> & list, bool nofrus)
{
	// always clear out the list first
	list.clear();

	// iterate through all ppos loaded
	for (std::map<int,PPOSpec>::iterator it=_ppoSpecMap.begin(); it!=_ppoSpecMap.end(); ++it)
	{
		bool allow = false;

		if (it->second.stotype != ServerTrackedObject::_UMS ||
			!nofrus	)
		{
			// first check to see what is allowed
			allow = checkList(it->second.allowPlace, vehclass, vehtype, allow, true);

			// now override with any that are denied
			allow = checkList(it->second.denyPlace, vehclass, vehtype, allow, false);

			// if still allowed, add it to list
			if (allow)
				list.push_back(&it->second);
		}
	}

	// successful if we have any that can be placed
	return list.size() > 0;
}

