#pragma once

#include "gr2/gr2Data.h"
#include "xml/XMLLoader2.h"

class gr2VehicleLoader : public XMLLoader2<gr2VehicleLoader>
{
public:

	gr2VehicleLoader(const char* path, gr2VehicleSpecMap* specMap, gr2VehicleLoader* handler = NULL);
	virtual ~gr2VehicleLoader();

	// tag handlers
	XML_TAG_HANDLER(DoInclude);
	XML_TAG_HANDLER(DoVehicle);
	XML_TAG_HANDLER(DoGR2Spec);
	XML_TAG_HANDLER(DoSkeleton);
	XML_TAG_HANDLER(DoDamageModel);
	XML_TAG_HANDLER(DoHitmap);
	XML_TAG_HANDLER(DoComponent);
	XML_TAG_HANDLER(DoDestroys);
	XML_TAG_HANDLER(DoMap);
	XML_TAG_HANDLER(DoLoadout);
	XML_TAG_HANDLER(DoModule);
	XML_TAG_HANDLER(DoLOD);
	XML_TAG_HANDLER(DoTurret);
	XML_TAG_HANDLER(DoTurretRotation);
	XML_TAG_HANDLER(DoState);
	XML_TAG_HANDLER(DoParam);

	gr2VehicleSpec* getSpec() const { return _spec; }

private:

	gr2VehicleSpecMap* _specMap;
	gr2VehicleSpec* _spec;
};

class gr2ModuleDataLoader : public XMLLoader2<gr2ModuleDataLoader>
{
public:

	gr2ModuleDataLoader(const char* path, gr2ModuleDataSpecMap* specMap, gr2ModuleDataLoader* handler = NULL);
	virtual ~gr2ModuleDataLoader();

	// tag handlers
	XML_TAG_HANDLER(DoSpec);
	XML_TAG_HANDLER(DoModule);
	XML_TAG_HANDLER(DoDamageModel);
	XML_TAG_HANDLER(DoHitmap);
	XML_TAG_HANDLER(DoComponent);
	XML_TAG_HANDLER(DoDestroys);
	XML_TAG_HANDLER(DoMap);
	XML_TAG_HANDLER(DoShape);
	XML_TAG_HANDLER(DoLOD);
	XML_TAG_HANDLER(DoState);
	XML_TAG_HANDLER(DoParam);

	gr2ModuleDataSpec* getSpec() const { return _currentSpec; }

private:

	gr2ModuleDataSpecMap* _specMap;
	gr2ModuleDataSpec* _currentSpec;

};

