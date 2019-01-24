#include <syscfg.h> // Path is project specific.
// gr2 loading module

/*
	 GR2 XML Loaders
	 Copyright 2001 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Troy Lowe
	Maintaining Programmer :
	Supporting Programmer  :

	Rev 1  c2001
*/

#include "gr2/gr2Loaders.h"
#include "gr2/gr2Data.h"
#include "veh/vehType.h"

//////////////////// VehicleLoader

gr2VehicleLoader::gr2VehicleLoader(const char* path, gr2VehicleSpecMap* specMap, gr2VehicleLoader* handler) :
	XMLLoader2<gr2VehicleLoader>(path),
	_specMap(specMap),
	_spec(NULL)
{
	if(!handler)
		handler = this;

	AddHandler("include", &gr2VehicleLoader::DoInclude, handler);
	AddHandler("vehicle", &gr2VehicleLoader::DoVehicle, handler);
	AddHandler("gr2spec", &gr2VehicleLoader::DoGR2Spec, handler);
	AddHandler("skeleton", &gr2VehicleLoader::DoSkeleton, handler);
	AddHandler("damagemodel", &gr2VehicleLoader::DoDamageModel, handler);
	AddHandler("hitmap", &gr2VehicleLoader::DoHitmap, handler);
	AddHandler("component", &gr2VehicleLoader::DoComponent, handler);
	AddHandler("destroys", &gr2VehicleLoader::DoDestroys, handler);
	AddHandler("map", &gr2VehicleLoader::DoMap, handler);
	AddHandler("lod", &gr2VehicleLoader::DoLOD, handler);
	AddHandler("turret", &gr2VehicleLoader::DoTurret, handler);
	AddHandler("traverse", &gr2VehicleLoader::DoTurretRotation, handler);
	AddHandler("elevate", &gr2VehicleLoader::DoTurretRotation, handler);
	AddHandler("loadout", &gr2VehicleLoader::DoLoadout, handler);
	AddHandler("module", &gr2VehicleLoader::DoModule, handler);
	AddHandler("state", &gr2VehicleLoader::DoState, handler);
	AddHandler("param", &gr2VehicleLoader::DoParam, handler);
}

gr2VehicleLoader::~gr2VehicleLoader()
{
}

bool gr2VehicleLoader::DoInclude(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		const char* file = xmlGetStringFromNode(node, "path");

		if(*file)
		{
			std::string path = GR2_VEHICLE_SPECS;
			path += file;

			gr2VehicleLoader incLoader(path.c_str(), _specMap, this);

			// if this spec fails, the doc stops parsing because
			// false is returned below after we set the handler
			// return scope to DOCUMENT here
			if(!incLoader.Load())
				SetHandlerReturnScope(DOCUMENT);
		}
	}

	return false;
}

bool gr2VehicleLoader::DoVehicle(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		_spec = new gr2VehicleSpec;

		_spec->name		= xmlGetStringFromNode(node, "name");
		_spec->baseDir	= xmlGetStringFromNode(node, "base");
		_spec->klass	= xmlGetStringFromNode(node, "class");

		(*_specMap)[_spec->name] = _spec;
	}

	return true;
}

bool gr2VehicleLoader::DoGR2Spec(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		vehType* type = (vehType*)userData;

		_spec = new gr2VehicleSpec;

		_spec->name		= (const char*)type->Id; //xmlGetStringFromNode(node, "name");
		_spec->baseDir	= xmlGetStringFromNode(node, "base");
		_spec->klass	= xmlGetStringFromNode(node, "class");

		(*_specMap)[_spec->name] = _spec;

		return true;
	}

	return false;
}

bool gr2VehicleLoader::DoSkeleton(xmlNode* node, bool first, void* userData)
{
	if(first)
		_spec->skeleton = xmlGetStringFromNode(node);

	return false;
}

bool gr2VehicleLoader::DoDamageModel(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		_spec->damageModel.levels	= 0;
		_spec->damageModel.points	= 0;
		_spec->damageModel.htag		= "";
		_spec->damageModel.dtag		= "";
		_spec->damageModel.hitmap	= "";

		_spec->damageModel.htag = xmlGetStringFromNode(node, "htag");
		_spec->damageModel.dtag = xmlGetStringFromNode(node, "dtag");

		xmlGetNodeValue(node, XML_DATA_UINT, &_spec->damageModel.levels, "levels");
		xmlGetNodeValue(node, XML_DATA_UINT, &_spec->damageModel.points, "points");
	}

	return true;
}

bool gr2VehicleLoader::DoHitmap(xmlNode* node, bool first, void* userData)
{
	if(first)
		_spec->damageModel.hitmap = xmlGetStringFromNode(node);

	return false;
}

bool gr2VehicleLoader::DoComponent(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2DamageComponentSpec component;

		component.name	= xmlGetStringFromNode(node, "name");
		component.state = xmlGetStringFromNode(node, "state");

		xmlGetNodeValue(node, XML_DATA_UINT, &component.points, "points");
		xmlGetNodeValue(node, XML_DATA_BOOL, &component.hideOnDestroy, "hideondestroy");

		if(ProcessChildren(node, &component))
		{
			if(userData)
				((gr2DamageComponentSpec*)userData)->components.push_back(component);
			else
				_spec->damageModel.components.push_back(component);
		}
	}

	return false;
}

bool gr2VehicleLoader::DoDestroys(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		gr2DamageComponentSpec* spec = (gr2DamageComponentSpec*)userData;
		spec->destroys.push_back(xmlGetStringFromNode(node, "name"));
	}

	return false;
}

bool gr2VehicleLoader::DoMap(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		gr2DamageComponentSpec* spec = (gr2DamageComponentSpec*)userData;
		gr2DamageMapSpec mapSpec;

		mapSpec.mesh = xmlGetStringFromNode(node, "mesh");

		xmlGetNodeValue(node, XML_DATA_SINT, &mapSpec.armor, "armor");
		xmlGetNodeValue(node, XML_DATA_SINT, &mapSpec.threshold, "min");
		xmlGetNodeValue(node, XML_DATA_FLOAT, &mapSpec.armorClass, "ac");

		if(ProcessChildren(node, &mapSpec))
			spec->maps.push_back(mapSpec);
	}

	return false;
}

bool gr2VehicleLoader::DoLOD(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2LodSpec lod;

		lod.distance = 0.0f;
		lod.model = xmlGetStringFromNode(node);

		if(lod.model.length() && (xmlGetNodeValue(node, XML_DATA_FLOAT, &lod.distance, "distance") || !_spec->model.size()))
			_spec->model.push_back(lod);
	}

	return false;
}

bool gr2VehicleLoader::DoTurret(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2TurretSpec turret;

		turret.bone	= xmlGetStringFromNode(node, "bone");

		if(turret.bone.length() && ProcessChildren(node, &turret))
			_spec->turrets.push_back(turret);
	}

	return false;
}

bool gr2VehicleLoader::DoTurretRotation(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		gr2TurretSpec* turret = (gr2TurretSpec*)userData;

		if(!strcasecmp((const char*)node->name, "traverse"))
		{
			turret->traverse.type = gr2TurretRotationSpec::TURRET_TYPE_TRAVERSE;
			turret->traverse.synced = true;

			xmlGetNodeValue(node, XML_DATA_SINT, &turret->traverse.tid, "tid");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &turret->traverse.minrot, "minrot");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &turret->traverse.maxrot, "maxrot");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &turret->traverse.rate, "rate");
			xmlGetNodeValue(node, XML_DATA_SINT, &turret->traverse.axis, "axis");
			xmlGetNodeValue(node, XML_DATA_BOOL, &turret->traverse.invert, "invert");
			xmlGetNodeValue(node, XML_DATA_BOOL, &turret->traverse.synced, "synced");
		}
		else if(!strcasecmp((const char*)node->name, "elevate"))
		{
			turret->elevate.type = gr2TurretRotationSpec::TURRET_TYPE_ELEVATE;
			turret->elevate.synced = true;

			xmlGetNodeValue(node, XML_DATA_SINT, &turret->elevate.tid, "tid");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &turret->elevate.minrot, "minrot");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &turret->elevate.maxrot, "maxrot");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &turret->elevate.rate, "rate");
			xmlGetNodeValue(node, XML_DATA_SINT, &turret->elevate.axis, "axis");
			xmlGetNodeValue(node, XML_DATA_BOOL, &turret->elevate.invert, "invert");
			xmlGetNodeValue(node, XML_DATA_BOOL, &turret->elevate.synced, "synced");
		}
	}

	return false;
}

/*
bool gr2VehicleLoader::DoMount(xmlNode* node, bool first, void* userData)
{
	if(!first)
	{
		_mountIndex++;
		_weaponIndex = 0;
	}

	return true;
}

bool gr2VehicleLoader::DoWeapon(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2WeaponDataSpec* weapDataSpec = gr2GetWeaponDataSpec(xmlGetStringFromNode(node, "spec"));

		if(weapDataSpec)
		{
			gr2WeaponSpec weaponSpec;

			weaponSpec.index		= _weaponIndex++;
			weaponSpec.mount		= _mountIndex;
			weaponSpec.ammo			= 0;
			weaponSpec.ammoType		= 0;
			weaponSpec.fireDelay	= 0.0f;
			weaponSpec.spec			= weapDataSpec;
			weaponSpec.name			= xmlGetStringFromNode(node, "name");
			weaponSpec.attach		= xmlGetStringFromNode(node, "attach");

			xmlGetNodeValue(node, XML_DATA_SINT, &weaponSpec.ammo, "ammo");
			xmlGetNodeValue(node, XML_DATA_SINT, &weaponSpec.ammoType, "ammotype");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &weaponSpec.fireDelay, "firedelay");

			_spec->loadout.push_back(weaponSpec);
		}
	}

	return false;
}
*/

bool gr2VehicleLoader::DoLoadout(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		std::string defaultClass = xmlGetStringFromNode(node, "defaultClass");
		ProcessChildren(node, &defaultClass);
	}

	return false;
}

bool gr2VehicleLoader::DoModule(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2ModuleSpec moduleSpec;

		moduleSpec.name			= xmlGetStringFromNode(node, "name");
		moduleSpec.data			= gr2GetModuleDataSpec(xmlGetStringFromNode(node, "spec"));
		moduleSpec.klass		= xmlGetStringFromNode(node, "class");

		if(!moduleSpec.klass.size()) // if a class wasn't set specifically
		{
			if(moduleSpec.data && moduleSpec.data->klass.size()) // check the data spec for one
				moduleSpec.klass = moduleSpec.data->klass;
			else if(userData) // otherwise check for a default class
				moduleSpec.klass = *(std::string*)userData;
			else // no class, no use
				return false;
		}

		// load any config params
		ProcessChildren(node, &moduleSpec);

		// add the module to the vehicle's loadout spec
		_spec->loadout.push_back(moduleSpec);
	}

	return false;
}

bool gr2VehicleLoader::DoState(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2StateSpec* parent	= (userData) ? (gr2StateSpec*)userData: NULL;
		gr2StateSpec* state		= gr2CreateStateSpec(node);

		if(state && ProcessChildren(node, state))
		{
			if(parent)
				parent->states.push_back(state);
			else
				_spec->states.push_back(state);
		}
	}

	return false;
}

bool gr2VehicleLoader::DoParam(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		std::string name  = xmlGetStringFromNode(node, "name");
		std::string value = xmlGetStringFromNode(node, "value");

		if(name.length() && value.length())
		{
			gr2ObjectConfigParam param(name, value);

			if(userData)
				((gr2ModuleSpec*)userData)->config.push_back(param); // specific module config
			else
				_spec->config.push_back(param);
		}
	}

	return false;
}

///////////////// gr2ModuleDataLoader

gr2ModuleDataLoader::gr2ModuleDataLoader(const char* path, gr2ModuleDataSpecMap* specMap, gr2ModuleDataLoader* handler) :
	XMLLoader2<gr2ModuleDataLoader>(path), _specMap(specMap), _currentSpec(NULL)
{
	if(!handler)
		handler = this;

	AddHandler("spec", &gr2ModuleDataLoader::DoSpec, handler);
	AddHandler("module", &gr2ModuleDataLoader::DoModule, handler);
	AddHandler("shape", &gr2ModuleDataLoader::DoShape, handler);
	AddHandler("lod", &gr2ModuleDataLoader::DoLOD, handler);
	AddHandler("damagemodel", &gr2ModuleDataLoader::DoDamageModel, handler);
	AddHandler("hitmap", &gr2ModuleDataLoader::DoHitmap, handler);
	AddHandler("component", &gr2ModuleDataLoader::DoComponent, handler);
	AddHandler("map", &gr2ModuleDataLoader::DoMap, handler);
	AddHandler("state", &gr2ModuleDataLoader::DoState, handler);
	AddHandler("param", &gr2ModuleDataLoader::DoParam, handler);
}

gr2ModuleDataLoader::~gr2ModuleDataLoader()
{
}

bool gr2ModuleDataLoader::DoSpec(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		const char* file = xmlGetStringFromNode(node);

		if(*file)
		{
			std::string path = GR2_MODULE_SPECS;
			path += file;

			gr2ModuleDataLoader specLoader(path.c_str(), _specMap, this);

			// if this spec fails, the doc stops parsing because
			// false is returned below after we set the handler
			// return scope to DOCUMENT here
			if(!specLoader.Load())
				SetHandlerReturnScope(DOCUMENT);
		}
	}

	return false;
}

bool gr2ModuleDataLoader::DoModule(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		_currentSpec				= new gr2ModuleDataSpec;
		_currentSpec->name			= xmlGetStringFromNode(node, "name");
		_currentSpec->klass			= xmlGetStringFromNode(node, "class");
		_currentSpec->baseDir		= xmlGetStringFromNode(node, "base");

		if(ProcessChildren(node))
		{
			(*_specMap)[_currentSpec->name] = _currentSpec;
		}
		else
		{
			delete _currentSpec;
			_currentSpec = NULL;
		}
	}

	return false;
}

bool gr2ModuleDataLoader::DoDamageModel(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		_currentSpec->damageModel.levels	= 0;
		_currentSpec->damageModel.htag		= "";
		_currentSpec->damageModel.dtag		= "";
		_currentSpec->damageModel.hitmap	= "";

		_currentSpec->damageModel.htag = xmlGetStringFromNode(node, "htag");
		_currentSpec->damageModel.dtag = xmlGetStringFromNode(node, "dtag");

		xmlGetNodeValue(node, XML_DATA_UINT, &_currentSpec->damageModel.levels, "levels");
	}

	return true;
}

bool gr2ModuleDataLoader::DoHitmap(xmlNode* node, bool first, void* userData)
{
	if(first)
		_currentSpec->damageModel.hitmap = xmlGetStringFromNode(node);

	return false;
}

bool gr2ModuleDataLoader::DoComponent(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2DamageComponentSpec component;

		component.name	= xmlGetStringFromNode(node, "name");
		component.state = xmlGetStringFromNode(node, "state");

		xmlGetNodeValue(node, XML_DATA_UINT, &component.points, "points");

		if(ProcessChildren(node, &component))
			if(userData)
				((gr2DamageComponentSpec*)userData)->components.push_back(component);
			else
				_currentSpec->damageModel.components.push_back(component);
	}

	return false;
}

bool gr2ModuleDataLoader::DoMap(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		gr2DamageComponentSpec* spec = (gr2DamageComponentSpec*)userData;
		gr2DamageMapSpec mapSpec;

		mapSpec.mesh = xmlGetStringFromNode(node, "mesh");

		xmlGetNodeValue(node, XML_DATA_UINT, &mapSpec.armor, "armor");
		xmlGetNodeValue(node, XML_DATA_UINT, &mapSpec.threshold, "min");
		xmlGetNodeValue(node, XML_DATA_FLOAT, &mapSpec.armorClass, "ac");

		if(ProcessChildren(node, &mapSpec))
			spec->maps.push_back(mapSpec);
	}

	return false;
}

bool gr2ModuleDataLoader::DoShape(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		std::string shape = xmlGetStringFromNode(node, "name");
		gr2ModelSpecs spec;

		if(shape.length() && ProcessChildren(node, &spec))
			_currentSpec->shapes[shape] = spec;
	}

	return false;
}

bool gr2ModuleDataLoader::DoLOD(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		gr2ModelSpecs* spec = (gr2ModelSpecs*)userData;
		gr2LodSpec lod;

		lod.distance = 0.0f;
		lod.model = xmlGetStringFromNode(node);

		if(lod.model.length() && (xmlGetNodeValue(node, XML_DATA_FLOAT, &lod.distance, "distance") || !spec->size()))
			spec->push_back(lod);
	}

	return false;
}

bool gr2ModuleDataLoader::DoState(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		gr2StateSpec* parent	= (userData) ? (gr2StateSpec*)userData: NULL;
		gr2StateSpec* state		= gr2CreateStateSpec(node);

		if(state && ProcessChildren(node, state))
		{
			if(parent)
				parent->states.push_back(state);
			else
				_currentSpec->states.push_back(state);
		}
	}

	return false;
}

bool gr2ModuleDataLoader::DoParam(xmlNode* node, bool first, void* userData)
{
	if(first)
	{
		std::string name  = xmlGetStringFromNode(node, "name");
		std::string value = xmlGetStringFromNode(node, "value");

		if(name.length() && value.length())
			_currentSpec->config.push_back(gr2ObjectConfigParam(name, value));
	}

	return false;
}
