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
#include "weap/weapDefs.h"					// For MAX_WEAP_CLASSES
#include "weap/weapList.h"					// For weaponClass[]
#include "weap/weapSpawnXML.h"				// Our header file
#include "wwii/wwiiHash.h"					// for unordered_map
#include "weap/weapSpec.h"

#define XML_TAG_HANDLER_IMPL(handler) bool SpawnLoader::handler(xmlNode* node, bool first, void* userData)

#define SPAWN_SPEC    "/xml/spawn.xml"

SpawnLoader::SpawnLoader() : XMLLoader2<SpawnLoader>(SPAWN_SPEC)
{
        AddHandler("building", &SpawnLoader::DoBuilding, this);
        AddHandler("dof", &SpawnLoader::DoDof, this);
        AddHandler("class", &SpawnLoader::DoClass, this);
        AddHandler("weapon", &SpawnLoader::DoWeapon, this);

	weaponList = nullptr;
	fail = false;
}

XML_TAG_HANDLER_IMPL(DoBuilding)
{
        if(first)
	{
		std::string buildingName  = xmlGetStringFromNode(node, "name");


		// Make sure this is not a duplicate
		if ( weapFindClass(buildingName.c_str()).wClass != 0 )
		{
			dbugLog(_FLA, "spawn.xml: duplicate building: %s", buildingName.c_str());
			fail = true;
		}
		else
		{
			WEAPON_SPEC classSpec(0, 0, firstFreeClass++, 0);

			dbugLog(_FLI, "Adding virtual weapon-class #%u %s:: ", classSpec.wClass, buildingName.c_str());

			weapAddClass(classSpec.wClass, buildingName);

			weaponList = &g_weaponSpawns[classSpec.wClass];
			if ( weaponList->empty() == false )
				throw std::domain_error("weaponSpawnList for vClass already populated");

			classes.clear();

                	ProcessChildren(node, nullptr);


		}

		if ( fail )
			throw std::domain_error("Invalid spawn.xml");
	}

        return false;
}


XML_TAG_HANDLER_IMPL(DoDof)
{
        if(first)
                ProcessChildren(node, nullptr);

        return false;
}

XML_TAG_HANDLER_IMPL(DoWeapon)
{
        if(first)
	{
		char *elText;
		if (xmlGetNodeValue(node, XML_DATA_STRING, &elText, nullptr))
		{
			dbugLog(_FLIB, "weapon:%s ", elText);
			WEAPON* weap = weapLookupWeaponByName(elText);
			if ( weap == nullptr )
			{
				dbugLog(_FLA, "Invalid weapon '%s' in spawn.xml", elText);
				fail = true;
			}
			else
				(*weaponList)[weap->spec()] = weap;
		}
	}
        return false;
}

XML_TAG_HANDLER_IMPL(DoClass)
{
	
        if(first)
	{
		char *elText;
		if (xmlGetNodeValue(node, XML_DATA_STRING, &elText, nullptr))
		{

			WEAPON_SPEC spec = weapFindClass(elText);
			if ( spec == NULL_WEAP_SPEC || spec.wClass == 0 )
			{
				dbugLog(_FLA, "Invalid weapon class '%s' in spawn.xml", elText);
				fail = true;
			}
			// Duplicate, ignore
			else if ( classes.find(spec) == classes.end() )
			{
				classes.insert(spec);

				// Stack it up for later consumption
				dbugLog(_FLIB, "class:%s ", elText);
	
				// Now add the spawnlist for all spawn lists of this class into our list
				for ( auto spawn = g_weaponSpawns.begin(), end = g_weaponSpawns.end(); spawn != end; ++spawn )
				{
					if ( (spawn->first & 255) != spec.wClass )
						continue;
					WEAPON_LIST& weapons = spawn->second;
					for ( it = weapons.begin()
						; it != weapons.end()
						; ++it
					)
					{
						weaponList->insert(*it);
					}
				}

				dbugLog(_FLIB, ", ");
			}
		}
	}

        return false;
}

void SpawnLoader::load()
{
	// Find the highest weapon class number
	UINT8 lastUsedClass = 0;
	for ( UINT8 i = 1; i <= g_maxWeaponClass; ++i )
	{
		if ( weaponClass[i].active == true )
			lastUsedClass = i;
	}

	// So the first free class will be the next one after the last one used
	firstFreeClass = lastUsedClass + 1;

	if ( firstFreeClass >= MAX_WEAP_CLASSES )
		throw std::domain_error("Too many classes");
	try
	{
		fail = false;

		// Get the base path name
		Load();
	}
	catch ( std::exception& e )
	{
		LogAbort("%s: %s", __FUNCTION__, e.what());
	}
}
