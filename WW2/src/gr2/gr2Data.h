#pragma once

#include <map>
#include <vector>
#include <string>
#include "gr2/gr2State.h"
#include "xml/XMLLoader2.h"

#define GR2DATA_MAX_NAME 128
#define GR2_VEHICLE_SPECS "vehicles\\ground\\mechs\\"
#define GR2_WEAPON_SPECS "vehicles\\ground\\mechs\\weapons\\"
#define GR2_WEAPONS_CONFIG GR2_WEAPON_SPECS "weapons.xml"
#define GR2_MODULE_SPECS "vehicles\\ground\\mechs\\modules\\"
#define GR2_MODULES_CONFIG GR2_MODULE_SPECS "modules.xml"

struct gr2StateSpec;
struct vehType;

typedef std::vector<gr2StateSpec*> gr2StateSpecs;
gr2StateSpec* gr2CreateStateSpec(xmlNode* node);

// base data specs

struct gr2LodSpec
{
	gr2LodSpec() : distance(0), model("")
	{
	}

	float distance;
	std::string model;
};

struct gr2TurretRotationSpec
{
	enum TURRET_TYPE
	{
		TURRET_TYPE_INVALID = 0,
		TURRET_TYPE_TRAVERSE,
		TURRET_TYPE_ELEVATE
	};

	gr2TurretRotationSpec() :
		type(TURRET_TYPE_INVALID),
		tid(-1),
		minrot(0),
		maxrot(0),
		rate(0),
		axis(0),
		invert(false),
		synced(true)
	{
	}

	TURRET_TYPE type;
	int tid; // game vehicle turret index
	float minrot; // left for traverse; down for elevate
	float maxrot; // right for traverse; up for elevate
	float rate; // rotation in degrees per second
	int axis; // which local axis to rotate the bone on
	bool invert; // invert the rotation; useful for mirrored turrets like weapons
	bool synced; // sync to camera?
};

struct gr2TurretSpec
{
	gr2TurretSpec() : bone("")
	{
	}

	gr2TurretRotationSpec traverse;
	gr2TurretRotationSpec elevate;
	std::string bone;
};


struct gr2DamageMapSpec
{
	gr2DamageMapSpec() :
		mesh(""),
		armor(0),
		threshold(0),
		armorClass(.5f)
	{
	}

	std::string mesh;
	SINT32 armor;
	SINT32 threshold;
	float armorClass;		// how good the armor is
};

typedef std::vector<gr2DamageMapSpec> gr2DamageMapSpecs;

struct gr2DamageComponentSpec
{
	gr2DamageComponentSpec() : name(""), state(""), points(0), hideOnDestroy(true)
	{
	}

	UINT32 points;			// hitpoints
	std::string name;
	std::string state;
	bool hideOnDestroy;

	gr2DamageMapSpecs maps;
	std::vector<gr2DamageComponentSpec> components;
	std::vector<std::string> destroys; // what other components to destroy when we're destroyed
};

typedef std::vector<gr2DamageComponentSpec> gr2DamageComponentSpecs;

// base state specs

struct gr2StateSpec
{
	gr2StateSpec() : type(0), flagType(0), baseState(false), valued(false), value(0), name(""), life(0)
	{
	}

	virtual ~gr2StateSpec()
	{
		for(int i = 0; i < (int)states.size(); i++)
		{
			gr2StateSpec* spec = states[i];
			delete spec;
		}

		states.clear();
	}

	int type;
	int flagType;
	int life; // ms; -1 = infinite; 0 = instant or default; > 0 is ms life length
	bool baseState;
	bool valued; // does our value mean something?
	UINT32 value;
	std::string name;
	gr2StateSpecs states;
};

struct gr2AnimStateSpec : gr2StateSpec
{
	gr2AnimStateSpec() : gr2StateSpec(), loops(0), speed(1.0f), easeIn(1.0f), easeOut(1.0f), animation("")
	{
	}

	UINT32 loops; // 0 = infinite
	float speed; // 1.0f = full speed
	float easeIn;
	float easeOut;
	std::string animation;
};

struct gr2SoundStateSpec : gr2StateSpec
{
	gr2SoundStateSpec() : gr2StateSpec(), sound(""), bone(""), volume(1.0f), minVol(1.0f), maxVol(2.0f)
	{
	}

	float volume;
	float minVol;
	float maxVol;
	std::string sound;
	std::string bone;
};

struct gr2EffectStateSpec : gr2StateSpec
{
	gr2EffectStateSpec() : gr2StateSpec(), effect(""), bone(""), update(0)
	{
		offset[0] = offset[1] = offset[2] = 0;
		dir[0] = dir[1] = dir[2] = 0;
	}

	int update;
	float dir[3];
	float offset[3];
	std::string effect;
	std::string bone;
};

struct gr2DamageStateSpec : gr2StateSpec
{
	gr2DamageStateSpec() : gr2StateSpec(), component("")
	{
	}

	std::string component;
};

typedef std::vector<gr2LodSpec> gr2ModelSpecs;
typedef std::map<std::string,gr2ModelSpecs> gr2ShapeSpecs;
typedef std::pair<std::string,std::string> gr2ObjectConfigParam;
//typedef std::vector<gr2ObjectConfigParam> gr2ObjectConfig;
typedef std::vector<gr2TurretSpec> gr2TurretSpecs;

// common data

struct gr2ObjectConfig : public std::vector<gr2ObjectConfigParam>
{
	static const std::string defValue;

	const std::string& find(const char* name) const
	{
		std::string temp = name;
		return find(temp);
	}

	const std::string& find(std::string& name) const
	{
		gr2ObjectConfig::const_iterator cit = begin();
		for(; cit != end(); cit++)
		{
			if(!stricmp(cit->first.c_str(), name.c_str()))
				return cit->second;
		}

		return defValue;
	}
};

struct gr2DamageModelSpec
{
	UINT32 levels;
	UINT32 points;			// total hitpoints
	std::string htag;
	std::string dtag;
	std::string hitmap;

	gr2DamageComponentSpecs components;
};

// common data
struct gr2ModuleDataSpec
{
	virtual ~gr2ModuleDataSpec()
	{
		for(int i = 0; i < (int)states.size(); i++)
		{
			gr2StateSpec* spec = states[i];
			delete spec;
		}

		states.clear();
	}

	std::string name;
	std::string klass;
	std::string baseDir;
	gr2ShapeSpecs shapes;
	gr2StateSpecs states;
	gr2ObjectConfig config; // these can be overridden in the instance module data
	gr2DamageModelSpec damageModel;
};

// instance data
struct gr2ModuleSpec
{
	std::string name; // this overrides data spec if set
	std::string klass; // this overrides data spec if set
	gr2ObjectConfig config; // instance overrides
	gr2ModuleDataSpec* data;

	const char* Path(std::string& file)
	{
		static std::string fullPath;
		fullPath = (data) ? data->baseDir: "";
		fullPath += file;
		return fullPath.c_str();
	}
};

/*
struct gr2WeaponDataSpec
{
	virtual ~gr2WeaponDataSpec()
	{
		for(int i = 0; i < (int)states.size(); i++)
		{
			gr2StateSpec* spec = states[i];
			delete spec;
		}

		states.clear();
	}

	std::string name;
	std::string klass;
	std::string gameWeapon;
	std::string baseDir;
	gr2ShapeSpecs shapes;
	gr2StateSpecs states;
	gr2ObjectConfig config;
	gr2DamageModelSpec damageModel;
};

// instance data
struct gr2WeaponSpec
{
	int index;
	int mount;
	int ammo;
	int ammoType;
	float fireDelay;
	std::string name;
	std::string attach; // weapon bone
	gr2WeaponDataSpec* spec;

	const char* Path(std::string& file)
	{
		static std::string fullPath;
		fullPath = (spec) ? spec->baseDir: "";
		fullPath += file;
		return fullPath.c_str();
	}
};
*/
//typedef std::vector<gr2WeaponSpec> gr2Loadout;
typedef std::vector<gr2ModuleSpec> gr2Loadout;

struct gr2VehicleSpec
{
	virtual ~gr2VehicleSpec()
	{
		for(int i = 0; i < (int)states.size(); i++)
		{
			gr2StateSpec* spec = states[i];
			delete spec;
		}

		states.clear();
	}

	std::string name;
	std::string klass; // which vehicle class to instantiate
	std::string baseDir;
	std::string skeleton;
	gr2ModelSpecs model;
	gr2Loadout loadout;
	gr2StateSpecs states;
	gr2TurretSpecs turrets;
	gr2ObjectConfig config;
	gr2DamageModelSpec damageModel;

	const char* Path(std::string& file)
	{
		static std::string fullPath;
		fullPath = baseDir;
		fullPath += file;
		return fullPath.c_str();
	}
};

typedef std::map<std::string,gr2VehicleSpec*> gr2VehicleSpecMap;
typedef std::map<std::string,gr2ModuleDataSpec*> gr2ModuleDataSpecMap;
//typedef std::map<std::string,gr2WeaponDataSpec*> gr2WeaponDataSpecMap;

void gr2InitData();
void gr2KillData();

void gr2ParseVehicleSpec(vehType* type, xmlNodePtr node);
gr2VehicleSpec* gr2GetVehicleSpec(const char* name);
gr2ModuleDataSpec* gr2GetModuleDataSpec(const char* name);
//gr2WeaponDataSpec* gr2GetWeaponDataSpec(const char* name);

