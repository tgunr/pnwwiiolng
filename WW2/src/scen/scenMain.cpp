#include <syscfg.h> // Path is project specific.
#include "scen/scenMain.h"
#include "scen/ScenarioManager.h"
#include "xml/XMLLoader2.h"

// types
typedef std::map<std::string,Scenario*> SCENARIO_MAP;

// local variables
static SCENARIO_MAP _cache;

/*
// locally defined types
class ScenarioLoader : public XMLLoader2<ScenarioLoader>
{
public:

	ScenarioLoader(const char* path, Scenario* s) : XMLLoader2<ScenarioLoader>(path)
	{
		AddHandler("scenario", &ScenarioLoader::DoScenario, this);
		AddHandler("param", &ScenarioLoader::DoParam, this);
		AddHandler("terrain", &ScenarioLoader::DoTerrain, this);
		//AddHandler("spawn", &ScenarioLoader::DoSpawn, this);
		//AddHandler("object", &ScenarioLoader::DoObject, this);
		//AddHandler("clone", &ScenarioLoader::DoClone, this);
		//AddHandler("effect", &ScenarioLoader::DoEffect, this);
		//AddHandler("layer", &ScenarioLoader::DoLater, this);
		//AddHandler("waypoint", &ScenarioLoader::DoWaypoint, this);
		//AddHandler("area", &ScenarioLoader::DoArea, this);
		//AddHandler("point", &ScenarioLoader::DoPoint, this);
		//AddHandler("onstart", &ScenarioLoader::DoCommand, this);
		//AddHandler("onspawn", &ScenarioLoader::DoCommand, this);
		//AddHandler("ondespawn", &ScenarioLoader::DoCommand, this);
		//AddHandler("onstop", &ScenarioLoader::DoCommand, this);
		//AddHandler("vehicle", &ScenarioLoader::DoVehicle, this);

		_scenario = s;
	}

	virtual ~ScenarioLoader()
	{
	}

	XML_TAG_HANDLER(DoScenario)
	{
		if(first)
			_scenario->script = xmlGetStringFromNode(node, "script");

		return true;
	}

	XML_TAG_HANDLER(DoParam)
	{
		if(first)
		{
			std::string name	= xmlGetStringFromNode(node, "name");
			std::string value	= xmlGetStringFromNode(node, "value");

			_scenario->config[name] = value;
		}

		return true;
	}

	XML_TAG_HANDLER(DoTerrain)
	{
		if(first)
		{
			// get out map file (optional)
			_scenario->terrainMap = xmlGetStringFromNode(node, "map");

			// see if we have an offset
			_extractNumbers(node, "offset", _scenario->terrainOffset.x(), _scenario->terrainOffset.y(), _scenario->terrainOffset.z());
		}

		return true;
	}
*/

	/*
	XML_TAG_HANDLER(DoSpawn)
	{
		if(first)
		{
			ScenarioObject ob;

			ob.name = xmlGetStringFromNode(node, "group");

			// get our spawn position
			_extractNumbers(node, "position", ob.position.x(), ob.position.y(), ob.position.z());

			// now get our rotation
			_extractNumbers(node, "rotation", ob.attitude.Roll, ob.attitude.Pitch, ob.attitude.Yaw);

			// save our new spawn location
			_scenario->spawns.push_back(ob);
		}

		return true;
	}

	XML_TAG_HANDLER(DoObject)
	{
		if(first)
		{
			ScenarioObject ob;

			ob.name = xmlGetStringFromNode(node, "file");

			// get our spawn position
			_extractNumbers(node, "position", ob.position.x(), ob.position.y(), ob.position.z());

			// now get our rotation
			_extractNumbers(node, "rotation", ob.attitude.Roll, ob.attitude.Pitch, ob.attitude.Yaw);

			// save our new spawn location
			_scenario->objects.push_back(ob);
		}

		return true;
	}

	XML_TAG_HANDLER(DoEffect)
	{
		if(first)
		{
			ScenarioObject effect;

			effect.name = xmlGetStringFromNode(node, "name");
			effect.type = ScenarioObject::UNKNOWN;

			const char* type = xmlGetStringFromNode(node, "type");

			if(!stricmp(type, "emitter"))
				effect.type = ScenarioObject::EMITTER;
			else if(!stricmp(type, "sound"))
				effect.type = ScenarioObject::SOUND;

			// get our spawn position
			_extractNumbers(node, "position", effect.position.x(), effect.position.y(), effect.position.z());

			// now get our rotation
			_extractNumbers(node, "rotation", effect.attitude.Roll, effect.attitude.Pitch, effect.attitude.Yaw);

			// save our new spawn location
			_scenario->effects.push_back(effect);
		}

		return true;
	}

	XML_TAG_HANDLER(DoArea)
	{
		if(first)
		{
			ScenarioArea area;

			area.handler = xmlGetStringFromNode(node, "handler");

			// grab the points that make up this area
			ProcessChildren(node, &area);

			// save our new spawn location
			_scenario->areas.push_back(area);
		}

		return false;
	}

	XML_TAG_HANDLER(DoPoint)
	{
		if(first && userData)
		{
			ScenarioArea* area = (ScenarioArea*)userData;

			Vec3d position;

			// get position of the point
			_extractNumbers(node, "position", position.x(), position.y(), position.z());

			// save our point in this area
			area->points.push_back(position);
		}

		return true;
	}

	XML_TAG_HANDLER(DoCommand)
	{
		if(first)
		{
			TUT_EVENT_TYPE type = TUT_EVENT_GENERAL;

			// what kind command is this
			if(!stricmp((const char*)node->name, "onstart"))
				type = TUT_EVENT_START;
			else if(!stricmp((const char*)node->name, "onstop"))
				type = TUT_EVENT_STOP;
			else if(!stricmp((const char*)node->name, "onspawn"))
				type = TUT_EVENT_SPAWN;
			else if(!stricmp((const char*)node->name, "ondespawn"))
				type = TUT_EVENT_DESPAWN;

			if(type != TUT_EVENT_GENERAL)
				return false;

			// grab our commands and parse them into individual slash commands
			const char* commands = xmlGetStringFromNode(node);
			const char* command = commands;
			const char* end = NULL;

			while((end = strstr(command, "\n")) != NULL)
			{
				// null terminate the current command
				end = '\0';

				// create a new command
				ScenarioCommand newCommand;

				// init it
				newCommand.type = type;
				newCommand.command = command;

				// save our command to run at the specified event
				_scenario->commands.push_back(newCommand);

				// move our command pointer past the current command
				command = end + 1;
			}
		}

		return true;
	}

	XML_TAG_HANDLER(DoVehicle)
	{
		if(first)
		{
			ScenarioVehicleMask mask;

			mask.spawnGroup = xmlGetStringFromNode(node, "spawn");

			// reset it
			mask.spec.fromShort(0);

			std::string id = xmlGetStringFromNode(node, "id");

			if(!id.empty())
			{
				mask.spec = weapFindWeaponByName(id.c_str());
			}
			else
			{
				SINT32 temp = 0;

				if(xmlGetNodeValue(node, XML_DATA_UINT, &temp, "country"))
					mask.spec.wCountry = (UINT8)temp;

				if(xmlGetNodeValue(node, XML_DATA_UINT, &temp, "category"))
					mask.spec.wCategory = (UINT8)temp;

				if(xmlGetNodeValue(node, XML_DATA_UINT, &temp, "class"))
					mask.spec.wClass = (UINT8)temp;

				if(xmlGetNodeValue(node, XML_DATA_UINT, &temp, "type"))
					mask.spec.wType = (UINT8)temp;
			}

			// save our new spawn location
			_scenario->vehicles.push_back(mask);
		}

		return true;
	}
	*/
/*
	Scenario* _scenario;
};
*/

// module functions
void scenMainInit()
{
	// load the scenario definitions
}

void scenMainKill()
{
	_cache.clear();
}

Scenario* scenMainLoad(const char* path)
{
	SCENARIO_MAP::iterator found = _cache.find(path);

	// return it from the cache if we have already loaded it
	if(false && found != _cache.end()) // always reload for now
		return found->second;

	// scenario structure to be populated
	Scenario* newScenario = new Scenario;

	std::string locPath = "loc://";
	locPath += path;

	// look up our scenario data
	if(jsContent::GetData(locPath.c_str(), newScenario, false))
	{
		newScenario->Init(path);

		// delete the last entry if we had one
		if(_cache.find(path) != _cache.end())
			delete _cache[path];

		// save it in the cache
		_cache[path] = newScenario;

		return newScenario;
	}

	/*
	// create a loader
	ScenarioLoader loader(path.c_str(), &newScenario);

	// load the scenarion from its xml file
	if(loader.Load())
	{
		// remember which file it was loaded from
		newScenario.file = file;

		// save it in the cache
		_cache[file] = newScenario;

		// return the cached version
		return &(_cache[file]);
	}
	*/

	// something failed
	return NULL;
}

#if(0)
void _extractNumbers(xmlNode* node, const char* name, double& x, double& y, double& z)
{
	std::string value = xmlGetStringFromNode(node, name);

	FLOAT32 _x, _y, _z;
	_x = _y = _z = 0.0f;

	// try to set x,y,z
	if(sscanf(value.c_str(), "%f,%f,%f", &_x, &_y, &_z) != 3)
	{
		_x = _y = _z = 0.0f;

		// otherwise try x,y
		if(sscanf(value.c_str(), "%f,%f", &_x, &_y) != 2)
		{
			_x = _y = _z = 0.0f;

			// otherwise try simply z
			if(sscanf(value.c_str(), "%f", &_z) != 1)
				_x = _y = _z = 0.0f;
		}
	}

	x = _x;
	y = _y;
	z = _z;
}

void _extractNumbers(const char* source, double& x, double& y, double& z)
{
	FLOAT32 _x, _y, _z;
	_x = _y = _z = 0.0f;

	// try to set x,y,z
	if(sscanf(source, "%f,%f,%f", &_x, &_y, &_z) != 3)
	{
		_x = _y = _z = 0.0f;

		// otherwise try x,y
		if(sscanf(source, "%f,%f", &_x, &_y) != 2)
		{
			_x = _y = _z = 0.0f;

			// otherwise try simply z
			if(sscanf(source, "%f", &_z) != 1)
				_x = _y = _z = 0.0f;
		}
	}

	x = _x;
	y = _y;
	z = _z;
}
#endif
