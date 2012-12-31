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
class Supercell ;

typedef std::map< std::pair<SINT16,SINT16>, Supercell > SUPERCELLS ;
extern SUPERCELLS g_supercells ;
