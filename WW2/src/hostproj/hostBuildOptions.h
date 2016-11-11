#pragma once
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library

#ifndef DEBUG
#undef DOROTHY
#endif

#define BRIDGES
#define SINGLE_CELL_HOST

#ifndef PN_DISABLE_AUTH
static constexpr const bool useAuthServer = true;
#else
static constexpr const bool useAuthServer = false;
#endif
