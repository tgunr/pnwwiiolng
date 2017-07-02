#pragma once

#include "xml/XMLLoader2.h"

struct PPOSpec
{
	SINT32 id; // terrain sto type id

	std::string name;  // name attribute
	std::string description;  // description attribute
	SINT32 stotype; // sto type attribute

	std::string modelName; // which visual model to use
	FLOAT64VECTOR rayAdjust; // how much to adjust the ray colliders by
	double avoidance; // buffer in meters to add to the avoidance box
	double rayTolerance; // ray tolerance; angle between corner and center ray normals allowed
	double tiltTolerance; // tilt tolerance; tilt allowed from straight up
	double viewOffset; // how far to move the object in front of us for viewing purposes
	double rotation; // degrees to rotate the object around the z
	SINT32 coolDown; // how long in milliseconds to cool down after building this object
	SINT32 coolDownEnd; // stateful frametime end when our current cooldown will be finished (this is a runtime dynamic field)
	SINT32 buildTime; // build time in milliseconds
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
	TeulTimeMS timeout;  // time in milliseconds object can stay alive in the world
	UINT64 damageThreshold;  // threshold damage a single event must cause to accumulate total damage
	UINT64 damageCapacity;  // maximum damage a PPO can withstand before being considered "killed"
	bool spawnable; // true = PPO is spawnable, false = not spawnable
	std::vector<std::string> allowSpawn; // The listed vehicle class/types are allowed to spawn
	std::vector<std::string> denySpawn; // The listed vehicle classes are denied the ability to spawn from ppo.
	std::vector<std::string> allowPlace; // Allows the placement of a ppo by the vehicle classes listed
	std::vector<std::string> denyPlace; // The listed vehicle classes may not place this ppo
};

class PPOSpecLoader : public XMLLoader2<PPOSpecLoader>
{
public:

	// NOTE: This order must track what is in the vehicle/ground/groundman/ppo.xml file
	enum
	{
		_PPO_IFP = 1 								//!< "Removal" stub.
		,	_PPO_GE									//!<Object representing a vehicle.
		,	_PPO_FMS								//!< Fire ordnance.
		,	_PPO_AFRU								//!< Dead vehicle.
		,	_PPO_CFRU								//!< Graphical effect (e.g. smoke).
		,	_PPO_SANDBAGS							//!< Dynamically placed terrain object.
		,	_PPO_TANKTRAP							//!< Player placed object; built on terrain sto.
	};

public:

	PPOSpecLoader();
	virtual ~PPOSpecLoader();

	void loadSpecMap();

	/* returns a PPO spec entry given the ppo ID (also known as subtype to some callers) */
	PPOSpec * ppoGetSpec(int subtype);

	/* returns true if the vehicle class/type is allowed to spawn into the PPO specified */
	bool ppoIsSpawnAllowed(int pposubtype, int vehclass, int vehtype);

	/* returns a list of PPO objects that can be placed by the vehicle requesting */
	bool getAllowedPPOs(int vehclass, int vehtype, std::vector<PPOSpec *> & list, bool nofrus);

	// tag handlers
	XML_TAG_HANDLER(DoDefaults);

	XML_TAG_HANDLER(DoPPO);

	XML_TAG_HANDLER(DoParam);

private:
	std::map<int,PPOSpec> _ppoSpecMap;
	PPOSpec defaults;

	/*
		utility method that populates a string vector from csv string
		but only if the key <name> matches what is provided in <check>
	*/
	bool checkDelimitedField(std::string name, std::string value,
				const char *check, std::vector<std::string> & stack);

	/*
		utility method that populates a UINT32 bit field from csv string
		but only if the key <name> matches what is provided in <check>
	*/
	bool checkDelimitedField(std::string name, std::string value,
				const char *check, UINT32 & bitfield);

	/*
		Searchest a list of strings formatted as "Class/Type" for a generic
		class match, or specific vehicle match.  If found, return a value
		identified, otherwise return its current value.
	*/
	bool checkList(std::vector<std::string> & list, int vehclass, int vehtype, bool allowed, bool setValue);
	bool checkType(const char *pType, int vehtype, bool & retVal, bool setValue);
};
