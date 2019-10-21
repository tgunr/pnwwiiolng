#pragma once
// Core teulKit callbacks. (TeulKit calls it's core TeulModule).
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

extern TEUL_CALLBACK*		SYN_TOKEN;

extern TEUL_CALLBACK*		KLIVE_TOKEN;

//! Initialize teul module callbacks.
SINT32	teulInitModCallbacks();

