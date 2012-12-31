#pragma once
// Created: Tue Feb 20 10:40:57 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

// read all of the countries in the weapCountries.cvs file
// into weaponClass. Also records indexes for weapCountryIndex;
extern bool weapParseCountry(char* countryfile);

// read all of the classes from the weapClasses.cvs file
// into weaponClass.
extern bool weapParseClasses(char* classesfile);

// reads all the weapon types out of the XX_XX_weap.cvs
// file of this country and class. Allocate a weapons
// list array and insert into the weapons hash.
extern bool weapParseWeapons(char* weapfile, SINT32 country, SINT32 category);

// read the default file associated with weapSpec
// and attatch to weapHash or weapCatDefaults.
extern bool weapParseSpawn(char* weapfile, SINT32 country, SINT32 category);
