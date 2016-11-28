#pragma once
// Created: Wed Jul 26 11:00:52 2000
//
// The main client-side API for the dbse system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

// initializes all standard dbse callbacks and launches update threads if necessary.
// if no errorFnPtr is defined errors are ignored.
bool dbseInit();

// a wrapper for teulSend, dbse manages the actual connection
// index and may need to return a specific error when no connection
// is available.
SINT32 dbseSend(struct TEUL_CALLBACK* action, void* object);
