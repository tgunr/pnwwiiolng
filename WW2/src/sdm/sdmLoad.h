#pragma once
// Created: Tue Apr 10 15:53:59 2001
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Originating Programmer: Eric "Meat" Hall

void dbseLoadIndexes();
void dbseSaveIndexChanges();

SINT32 dbseInitStratLoader();

SINT32 dbseLoadFile(SCHAR* filename);

SINT32 dbseResolveReferences();

SINT32 dbseCloseLoader();

