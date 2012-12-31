#pragma once

#include "xml/XMLLoader2.h"

// ----------- defines

// ----------- types

/************************************************************************************************
	ScenarioLoader
*************************************************************************************************/

class ScenarioLoader : public XMLLoader2<ScenarioLoader>
{
public:

	ScenarioLoader(const char* path);
	virtual ~ScenarioLoader(){}

	// tag handlers
	XML_TAG_HANDLER(DoContent);
	XML_TAG_HANDLER(DoLink);
	XML_TAG_HANDLER(DoNode);

private:


};