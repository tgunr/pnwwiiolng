#pragma once
// Created: Mon Jul 11 18:06:13 CDT 2005
//
// Defines a hash routine for concealment of
// fps variable names from prying eyes
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

static inline UINT64
fpStringHash(const char* str)
{
	UINT64 hash = 0;
	while (*str)
	{
		hash = (hash << 2) + (tolower(*(str++)) - 'a');
	}
	return hash;
}
