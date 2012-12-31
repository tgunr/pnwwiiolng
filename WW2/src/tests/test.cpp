#include <syscfg.h> // Path is project specific.
//
// Created: Wed Oct  1 23:44:02 CDT 2008
//
// Trivial test wrapper
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "tests/testSuite.h"

using namespace std ;

int
main(int argc, char* argv[])
{
	// Your code here: Do NOT check me in

	cout << "OIC_NOM_USAGE = " << LSTR_OICNOM_USAGE << endl ;
	cout << "OIC_REM_DETAIL = " << LSTR_OICREM_DETAIL << endl ;
}
