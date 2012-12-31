#pragma once
// Created: Tue Apr 10 15:53:59 2001
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Originating Programmer: Eric "Meat" Hall

extern void dbseLoadIndexes(void) ;
extern void dbseSaveIndexChanges(void) ;

extern SINT32 dbseInitStratLoader();

extern SINT32 dbseLoadFile(SCHAR* filename);

extern SINT32 dbseResolveReferences();

extern SINT32 dbseCloseLoader();

