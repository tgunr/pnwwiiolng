#ifndef PN_WEAP_SPAWNXML_H
#define PN_WEAP_SPAWNXML_H 1
// Created: Wed Nov 28 14:09:32 CST 2007
//
// Loads the spawn.xml file and creates virtual
// weapClasses for the spawn system
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
// Modified: Ian "frisbone" Frisbie Sep 11, 2016
//

#include "xml/XMLLoader2.h"

class SpawnLoader : public XMLLoader2<SpawnLoader>
{
public:

        SpawnLoader();
        virtual ~SpawnLoader() { }

	void load();

        // tag handlers
        XML_TAG_HANDLER(DoBuilding);

        XML_TAG_HANDLER(DoDof);

        XML_TAG_HANDLER(DoWeapon);

        XML_TAG_HANDLER(DoClass);

private:

	WEAPON_LIST& weaponList;
	std::set<WEAPON_SPEC> classes;
	WEAPON_LIST::iterator it;
	bool fail;
	UINT8 firstFreeClass;

};

#endif // PN_WEAP_SPAWNXML_H
