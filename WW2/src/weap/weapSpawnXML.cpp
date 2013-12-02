#include <syscfg.h> // Path is project specific.
// Created: Wed Nov 28 14:09:32 CST 2007
//
// Load spawn.xml and create virtual weaponClasses for the
// spawn system which incorporate the vehicles tagged to
// spawn at specific buildings.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "fio/fiod.h"						// for fiodGetPrepath()
#include "tinyxml/tinyxml.h"			// For TinyXML functions
#include "weap/weapDefs.h"					// For MAX_WEAP_CLASSES
#include "weap/weapList.h"					// For weaponClass[]
#include "weap/weapSpawnXML.h"				// Our header file
#include "wwii/wwiiHash.h"					// for hash_map

// By-building, what can spawn here.
#if platOS == OS_LINUX
typedef std::unordered_map<std::string, WEAPON_LIST> BUILDING_SPAWNS;
#else
typedef stdext::hash_map<std::string, WEAPON_LIST> BUILDING_SPAWNS;
#endif
static BUILDING_SPAWNS s_buildingSpawns;

//////////////////////////////////////////////////////////////////////
// Find the highest weapon class number

static UINT8
_getFirstFreeWeaponClass()
{
	UINT8 lastUsedClass = 0;
	for ( UINT8 i = 1; i < MAX_WEAP_CLASSES; ++i )
	{
		if ( weaponClass[i].active == true )
			lastUsedClass = i;
	}

	// So the first free class will be the next one after the last one used
	return lastUsedClass + 1;
}

//////////////////////////////////////////////////////////////////////
//

bool
weapLoadSpawnXML()
{

	try
	{
		// Find out where we start our virtual classes.
		UINT8 firstFreeClass = _getFirstFreeWeaponClass();

		if ( firstFreeClass >= MAX_WEAP_CLASSES )
			throw std::domain_error("Too many classes");

		// Get the base path name
		std::string filename(fiodGetPrepath());
		filename += "/xml/spawn.xml";

		// Try and open a TinyXmlDocument.
		TiXmlDocument doc(filename.c_str());

		if ( doc.LoadFile() == false )
			throw std::runtime_error("Load failed");						///Terminal

		// Look for the spawner root object
		TiXmlNode* root = doc.FirstChild("SPAWNER");
		if ( root == NULL )
			throw std::runtime_error("Unable to find root object");		///Terminal

		std::set<WEAPON_SPEC> classes;
		WEAPON_LIST::iterator it;

		bool fail = false;

		TiXmlNode* node;
		for ( node = root->FirstChild("building")
			; node != NULL
			; node = node->NextSibling("building")
			)
		{
			TiXmlElement* building = node->ToElement();
			const std::string buildingName = building->Attribute("name");

			// Make sure this is not a duplicate
			if ( weapFindClass(buildingName.c_str()).wClass != 0 )
			{
				dbugLog(_FLA, "spawn.xml: duplicate building: %s", buildingName.c_str());
				fail = true;
				continue;
			}

			WEAPON_SPEC classSpec = NULL_WEAP_SPEC;
			classSpec.wClass = firstFreeClass++;

			dbugLog(_FLI, "Adding virtual weapon-class #%u %s:: ", classSpec.wClass, buildingName.c_str());

			weapAddClass(classSpec.wClass, buildingName.c_str());

			WEAPON_LIST& weaponList = g_weaponSpawns[classSpec.wClass];
			if ( weaponList.empty() == false )
				throw std::domain_error("weaponSpawnList for vClass already populated");

			// Go through the DOF and build a list of classes.

			TiXmlElement* dof;

			classes.clear();

			// Grab a unique list of all the classes this dof introduces.
			for ( dof = building->FirstChildElement("dof")
				; dof != NULL
				; dof = dof->NextSiblingElement("dof")
				)
			{
				TiXmlElement* child = NULL;
				for ( child = dof->FirstChildElement()
					; child != NULL
					; child = child->NextSiblingElement()
					)
				{
					const char* elType = child->Value();
					const char* elText = child->GetText();

					// Weapons can be specified individually
					if ( strcasecmp(elType, "weapon") == 0 )
					{
						dbugLog(_FLIB, "weapon:%s ", elText);
						WEAPON* weap = weapLookupWeaponByName(elText);
						if ( weap == NULL )
						{
							dbugLog(_FLA, "Invalid weapon '%s' in spawn.xml", elText);
							fail = true;
							continue;
						}
						weaponList[weap->spec()] = weap;
						continue;
					}

					// Otherwise we only understand classes
					if ( strcasecmp(elType, "class") != 0 )
						continue;

					WEAPON_SPEC spec = weapFindClass(elText);
					if ( spec == NULL_WEAP_SPEC || spec.wClass == 0 )
					{
						dbugLog(_FLA, "Invalid weapon class '%s' in spawn.xml", elText);
						fail = true;
						continue;
					}

					// Duplicate, ignore
					if ( classes.find(spec) != classes.end() )
						continue;

					classes.insert(spec);

					// Stack it up for later consumption
					dbugLog(_FLIB, "class:%s ", elText);

					// Now add the spawnlist for all spawn lists of this class into our list
					WEAPON_SPAWNS::iterator spawn;
					for ( spawn = g_weaponSpawns.begin(); spawn != g_weaponSpawns.end(); ++spawn )
					{
						if ( (spawn->first & 255) != spec.wClass )
							continue;
						WEAPON_LIST& weapons = spawn->second;
						for ( it = weapons.begin()
							; it != weapons.end()
							; ++it
							)
						{
							weaponList.insert(*it);
						}
					}

					dbugLog(_FLIB, ", ");
				}
			}

			if ( fail == true )
				throw std::domain_error("Invalid spawn.xml");
		}
	}
	catch ( std::exception& e )
	{
		LogAbort("%s: %s", __FUNCTION__, e.what());
	}

	return true;
}
