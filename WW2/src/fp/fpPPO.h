#pragma once

#include "physics/colliderQuery.h"
#include "xml/XMLLoader2.h"

#define FRU_INDEX 3

struct PPOSpec
{
	SINT32 id; // terrain sto type id
	std::string modelName; // which visual model to use
	Vec3d rayAdjust; // how much to adjust the ray colliders by
	double avoidance; // buffer in meters to add to the avoidance box
	double rayTolerance; // ray tolerance; angle between corner and center ray normals allowed
	double tiltTolerance; // tilt tolerance; tilt allowed from straight up
	double viewOffset; // how far to move the object in front of us for viewing purposes
	double rotation; // degrees to rotate the object around the z
	SINT32 coolDown; // how long in milliseconds to cool down after building this object
	SINT32 coolDownEnd; // stateful frametime end when our current cooldown will be finished (this is a runtime dynamic field)
	SINT32 buildTime; // build time in miliseconds
	UINT32 buildFlags; // what this ppo can be built on; everything else must be avoided (collider flags)
	UINT32 avoidFlags; // what this ppo cant be in contact with to build (collider flags)
	UINT32 surfaceFlags; // which infantry surfaces are okay for this object type (0 - 5)
	bool allowInterior; // can we build this while inside a building
	FLOAT32 treeCheck;		// do we check for tree collisions
	SINT32 enemyFacDistance; // distance from the nearest enemy facility (positive >= negative <=)
	SINT32 friendlyFacDistance; // distance from the nearest friendly facility (positive >= negative <=)
	SINT32 targetDistance;	// distance from the target facility (positive >= negative <=)
	FLOAT32 maxSpeed;		// threshold in meters per second <=
	std::vector<std::string> badTextures; // textures we can't build on
	std::vector<std::string> okTextures; // textures we can build on
};

typedef std::map<int,PPOSpec> PPO_SPEC_MAP;
typedef bool (*PPOCallback)();

class PPOSpecLoader : public XMLLoader2<PPOSpecLoader>
{
public:

	PPOSpecLoader(const char* path, PPO_SPEC_MAP* psm) : XMLLoader2<PPOSpecLoader>(path)
	{
		AddHandler("defaults", &PPOSpecLoader::DoDefaults, this);
		AddHandler("ppo", &PPOSpecLoader::DoPPO, this);
		AddHandler("param", &PPOSpecLoader::DoParam, this);

		_specMap = psm;
	}

	virtual ~PPOSpecLoader()
	{
	}

	// tag handlers
	XML_TAG_HANDLER(DoDefaults)
	{
		if(first)
			ProcessChildren(node, &defaults);

		return false;
	}

	XML_TAG_HANDLER(DoPPO)
	{
		if(first)
		{
			PPOSpec spec;

			// set our defaults
			spec.allowInterior			= defaults.allowInterior;
			spec.avoidance				= defaults.avoidance;
			spec.surfaceFlags			= defaults.surfaceFlags;
			spec.avoidFlags				= defaults.avoidFlags;
			spec.buildFlags				= defaults.buildFlags;
			spec.buildTime				= defaults.buildTime;
			spec.coolDown				= defaults.coolDown;
			spec.coolDownEnd			= 0;
			spec.rayAdjust				= defaults.rayAdjust;
			spec.rayTolerance			= defaults.rayTolerance;
			spec.rotation				= defaults.rotation;
			spec.tiltTolerance			= defaults.tiltTolerance;
			spec.viewOffset				= defaults.viewOffset;
			spec.treeCheck				= defaults.treeCheck;
			spec.enemyFacDistance		= defaults.enemyFacDistance;
			spec.friendlyFacDistance	= defaults.friendlyFacDistance;
			spec.targetDistance			= defaults.targetDistance;
			spec.maxSpeed				= defaults.maxSpeed;

			for(size_t i = 0; i < defaults.badTextures.size(); i++)
				spec.badTextures.push_back(defaults.badTextures[i]);

			for(size_t i = 0; i < defaults.okTextures.size(); i++)
				spec.okTextures.push_back(defaults.okTextures[i]);

			// process any param overrides
			ProcessChildren(node, &spec);

			// get the next available id (1 based)
			spec.id = _specMap->size() + 1;

			// save our spec in the map
			(*_specMap)[spec.id] = spec;
		}

		return false;
	}

	XML_TAG_HANDLER(DoParam)
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
					spec->rayAdjust.set((double)x, (double)y, (double)z);
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
			else if(!stricmp(name.c_str(), "buildflags"))
			{
				spec->buildFlags = 0;

				char* flag = strtok((char*)value.c_str(),",");

				while(flag != nullptr)
				{
					spec->buildFlags |= (1<<(UINT32)atoi(flag));
					flag = strtok(nullptr, ",");
				}
			}
			else if(!stricmp(name.c_str(), "avoidflags"))
			{
				spec->avoidFlags = 0;

				char* flag = strtok((char*)value.c_str(),",");

				while(flag != nullptr)
				{
					spec->avoidFlags |= (1<<(UINT32)atoi(flag));
					flag = strtok(nullptr, ",");
				}
			}
			else if(!stricmp(name.c_str(), "surfaceflags"))
			{
				spec->surfaceFlags = 0;

				char* flag = strtok((char*)value.c_str(),",");

				while(flag != nullptr)
				{
					spec->surfaceFlags |= (1<<(UINT32)atoi(flag));
					flag = strtok(nullptr, ",");
				}
			}
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
			else if(!stricmp(name.c_str(), "badtextures"))
			{
				spec->badTextures.clear();

				char* texture = strtok((char*)value.c_str(),",");

				while(texture != nullptr)
				{
					spec->badTextures.push_back(texture);
					texture = strtok(nullptr, ",");
				}
			}
			else if(!stricmp(name.c_str(), "oktextures"))
			{
				spec->okTextures.clear();

				char* texture = strtok((char*)value.c_str(),",");

				while(texture != nullptr)
				{
					spec->okTextures.push_back(texture);
					texture = strtok(nullptr, ",");
				}
			}
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
		}

		return true;
	}

	PPO_SPEC_MAP* _specMap;
	PPOSpec defaults;
};

namespace phui { class View; }

struct PlayerPlacedObject
{
	PlayerPlacedObject(PPOSpec* s);
	virtual ~PlayerPlacedObject();

	//bool GetTransform(piMATRIX& t);
	bool Update();
	bool Build();
	bool Execute();
	void Interrupt();
	bool Modified();
	void SetMaterial(struct OBJECT* o);
	SINT32 CoolDown();

	void UpdateColliders();
	bool CalculatePlacement(piVECTOR& pos, piVECTOR& att, bool checksOnly = false);

	void SetInvalid(const char* key, bool force = false);

	// callbacks
	void SetUpdateCallback(PPOCallback uc);
	void SetExecuteCallback(PPOCallback ec);
	void SetValidityCallback(PPOCallback vc);

	struct OBJECT* obj;
	bool valid;
	bool lastValid;
	bool failOnInvalid;
	bool interruptNextUpdate;
	bool pauseBuild;

	dGeomID abox; // avoidance box

	PPOSpec* spec;

	colliderQuery collider;
	colliderQuery::CollisionList hits;

	Vec3d finalPos;
	piVECTOR finalAtt;

	SINT32 buildAt;
	SINT32 lastUpdate;
	phui::View* buildUI;

	PPOCallback updateCallback;
	PPOCallback executeCallback;
	PPOCallback validityCallback;
};

extern PlayerPlacedObject* fpPPOCreate(int type, bool save = true);
extern bool fpPPOReady(int type);
extern PPOSpec* fpPPOGetSpec(int type);
extern void fpPPOSetSpec(int type);
extern void fpPPOCreateMaterial();
extern void fpPPOResetMaterial();

extern void fpPPOSet(PlayerPlacedObject* to);
extern PlayerPlacedObject* fpPPOGet();
extern bool fpPPOUpdate();
extern void fpPPOKill();
extern void fpPPOCancel();
extern void fpPPOAlert(const char* key, ...);
extern void fpPPOAlert(const wchar_t* message = nullptr);
