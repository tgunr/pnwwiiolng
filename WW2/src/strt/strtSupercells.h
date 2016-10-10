#pragma once
// Date: Mon Feb 27 18:07:56 CST 2006
//
// Access to the supercell index
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>
class Supercell;

using SUPERCELLS = std::unordered_map< SINT32, Supercell >;
extern SUPERCELLS g_supercells;
