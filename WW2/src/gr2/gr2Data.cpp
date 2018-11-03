#include <syscfg.h> // Path is project specific.
// gr2 data module

/*
	 GR2 Data
	 Copyright 2001 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Troy Lowe
	Maintaining Programmer :
	Supporting Programmer  :

	Rev 1  c2001
*/

#include "gr2/gr2Data.h"
#include "gr2/gr2Loaders.h"
#include "gr2/gr2State.h"
#include "veh/vehType.h"

// defines

// types

// local data

static bool _init = false;
static gr2VehicleSpecMap _vehicleSpecs;
static gr2ModuleDataSpecMap _moduleSpecs;
//static gr2WeaponDataSpecMap _weaponSpecs;

// local function declares

//static void _loadWeapons(const char* config);
static void _loadModules(const char* config);
static gr2ModuleDataSpec* _loadModule(const char* config);
static gr2VehicleSpec* _loadVehicle(const char* config);

const std::string gr2ObjectConfig::defValue = "";

// public functions

void gr2InitData()
{
	if(_init) return;

	_init = true;

	// loads up each gr2 weapon
	//_loadWeapons(GR2_WEAPONS_CONFIG);
	//_loadModules(GR2_MODULES_CONFIG); // precache modules
}

void gr2KillData()
{
	gr2VehicleSpecMap::iterator vit = _vehicleSpecs.begin();
	for(; vit != _vehicleSpecs.end(); vit++)
	{
		gr2VehicleSpec* spec = vit->second;
		delete spec;
	}

	gr2ModuleDataSpecMap::iterator wit = _moduleSpecs.begin();
	for(; wit != _moduleSpecs.end(); wit++)
	{
		gr2ModuleDataSpec* spec = wit->second;
		delete spec;
	}

	_vehicleSpecs.clear();
	_moduleSpecs.clear();
}

gr2ModuleDataSpec* gr2GetModuleDataSpec(const char* name)
{
	gr2ModuleDataSpecMap::iterator it = _moduleSpecs.find(name);
	if(it != _moduleSpecs.end()) return it->second;

	std::string path = GR2_MODULE_SPECS;

	path += name;
	path += ".xml";

	return _loadModule(path.c_str());
}

gr2VehicleSpec* gr2GetVehicleSpec(const char* name)
{
	gr2VehicleSpecMap::iterator it = _vehicleSpecs.find(name);
	if(it != _vehicleSpecs.end()) return it->second;

	std::string path = GR2_VEHICLE_SPECS;

	path += name;
	path += "\\";
	path += "spec.xml";

	return _loadVehicle(path.c_str());
}

gr2StateSpec* gr2CreateStateSpec(xmlNode* node)
{
    gr2StateSpec* spec			= NULL;
	std::string type			= xmlGetStringFromNode(node, "type");
	std::string flag			= xmlGetStringFromNode(node, "flag");
	GR2_STATE_TYPE stateType	= GST_GENERIC;
	GR2_FLAG_TYPE flagType		= GFT_ACTION;

	for(int i = 0; i < GST_NUM_TYPES; i++)
	{
		if(!strcasecmp(type.c_str(), GR2_STATE_TYPE_NAMES[i]))
		{
			stateType = (GR2_STATE_TYPE)i;
			break;
		}
	}

	for(int i = 0; i < GFT_NUM_TYPES; i++)
	{
		if(!strcasecmp(flag.c_str(), GR2_FLAG_TYPE_NAMES[i]))
		{
			flagType = (GR2_FLAG_TYPE)i;
			break;
		}
	}

	switch(stateType)
	{
		case GST_ANIMATION:
		{
			gr2AnimStateSpec* aspec = new gr2AnimStateSpec;

			if(aspec)
			{
				aspec->loops			= 0;
				aspec->speed			= 1.0f;
				aspec->easeIn			= 1.0f;
				aspec->easeOut			= 1.0f;
				aspec->animation		= xmlGetStringFromNode(node);

				xmlGetNodeValue(node, XML_DATA_SINT, &aspec->loops, "loops");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &aspec->speed, "speed");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &aspec->easeIn, "easein");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &aspec->easeOut, "easeout");

				spec = aspec;
			}
		}
		break;
		case GST_SOUND:
		{
			gr2SoundStateSpec* sspec = new gr2SoundStateSpec;

			if(sspec)
			{
				sspec->sound = xmlGetStringFromNode(node);

				xmlGetNodeValue(node, XML_DATA_FLOAT, &sspec->volume, "vol");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &sspec->minVol, "minvol");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &sspec->maxVol, "maxvol");

				spec = sspec;
			}
		}
		break;
		case GST_EFFECT:
		{
			gr2EffectStateSpec* espec = new gr2EffectStateSpec;

			if(espec)
			{
				espec->effect	= xmlGetStringFromNode(node);
				espec->bone		= xmlGetStringFromNode(node, "bone");

				xmlGetNodeValue(node, XML_DATA_UINT, &espec->update, "update");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &espec->offset[0], "ox");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &espec->offset[1], "oy");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &espec->offset[2], "oz");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &espec->dir[0], "dx");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &espec->dir[1], "dy");
				xmlGetNodeValue(node, XML_DATA_FLOAT, &espec->dir[2], "dz");

				spec = espec;
			}
		}
		break;
		case GST_DAMAGE:
		{
			gr2DamageStateSpec* dspec = new gr2DamageStateSpec;

			if(dspec)
			{
				dspec->component	= xmlGetStringFromNode(node, "component");
				spec				= dspec;
			}
		}
		break;
		default:
			spec = new gr2StateSpec;
		break;
	}

	// init common state members
	if(spec)
	{
		spec->type		= stateType;
		spec->flagType	= flagType;
		spec->name		= xmlGetStringFromNode(node, "name");
		spec->value		= 0;

		xmlGetNodeValue(node, XML_DATA_BOOL, &spec->baseState, "base");
		xmlGetNodeValue(node, XML_DATA_SINT, &spec->life, "life");

		// whether our value means something
		spec->valued = xmlGetNodeValue(node, XML_DATA_UINT, &spec->value, "value");

		if(spec->valued)
			spec->value = (1<<spec->value);
	}

	return spec;
}

// old, modules are now loaded on the fly as needed
static void _loadModules(const char* config)
{
	gr2ModuleDataLoader loader(config, &_moduleSpecs);

	// precache modules
	loader.Load();
}

static gr2ModuleDataSpec* _loadModule(const char* config)
{
	gr2ModuleDataLoader loader(config, &_moduleSpecs);

	// precache modules
	if(loader.Load())
		return loader.getSpec();

	return NULL;
}

static gr2VehicleSpec* _loadVehicle(const char* config)
{
	gr2VehicleLoader loader(config, &_vehicleSpecs);

	if(loader.Load())
		return loader.getSpec();

	return NULL;
}

void gr2ParseVehicleSpec(vehType* type, xmlNodePtr node)
{
	gr2VehicleLoader loader("", &_vehicleSpecs);
	loader.Process(node, type);
}
