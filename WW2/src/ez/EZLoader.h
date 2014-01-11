#pragma once

#include "xml/XMLLoader2.h"
#include "ez/EZManager.h"

struct vehType;

// loads ez vehicle position definitions
class EZLoader : public XMLLoader2<EZLoader>
{
public:

	struct EZLevel
	{
		EZManager::CONFIG_LIST config;
		EZManager::CONTROL_LIST controls;
		EZManager::POSITION_LIST positions;
	};

	EZLoader(const char* path);
	virtual ~EZLoader(){}

	// tag handlers
	XML_TAG_HANDLER(DoVehicle);
	XML_TAG_HANDLER(DoInclude);
	XML_TAG_HANDLER(DoEZ);
	XML_TAG_HANDLER(DoConfig);
	XML_TAG_HANDLER(DoControl);
	XML_TAG_HANDLER(DoPosition);
	XML_TAG_HANDLER(DoView);
	XML_TAG_HANDLER(DoCamera);
	XML_TAG_HANDLER(DoTraverse);
	XML_TAG_HANDLER(DoElevation);
	XML_TAG_HANDLER(DoBlocks);
	XML_TAG_HANDLER(DoTurret);

	typedef std::map<SINT32, EZLevel> EZ_LEVEL_MAP;

	// levels we have loaded
	EZ_LEVEL_MAP levels;

	// stateful level stuff
	SINT32 currentLevel;
	SINT32 maxLevelLoaded;

	// top of the include chain; nullptr if we are the top
	EZLoader* rootLoader;
};

// this function determines which ez manager to use
// for this type of vehicle; if it returns false, it
// means no ez manager is defined for this vehicle
// if it finds one, the manager and path strings will
// be populated; the manager will be which ez manager
// class to use and the path will be the ez file used
// to determine the manager type
extern bool ezManagerType(const vehType* vtype, std::string& manager, std::string& path);

