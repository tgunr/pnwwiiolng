#pragma once
// Created: Tue Jun 13 13:32:25 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Marty "Vandal" Poulin

#include "teul/teulServer.h"

#ifndef MAX_SERVER_TYPES
#define MAX_SERVER_TYPES 64
#endif

#ifndef SERVER_HASH_SIZE
#define SERVER_HASH_SIZE 64
#endif
#ifndef PLAYER_HASH_SIZE
#define PLAYER_HASH_SIZE 64
#endif

static const TeulTimeMS SERVER_FAIL_INTERVAL = 45000ULL ;
