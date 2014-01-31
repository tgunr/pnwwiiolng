#pragma once
// Created: Thu May 18 11:20:59 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// The header file for the Database Wrapper stuff
//
// Originating Programmer: Shane "Elmo" Wickson

#include "dbse/dbseHandle.h"

//Connects to all the databases that are passed to it.
//The first parameter is an array of pointers to connection structs, and the
//	second parameter is the number of structures to use out of that array

DB::handle_t* dbseGetConnection(const struct DBCONNECTION& connection, bool report, bool autoReconnect=true);

const char* escapeString(const char* src, char* dest, size_t destSize);
