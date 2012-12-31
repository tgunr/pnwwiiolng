#pragma once
// Created: Tue Sep 20 21:02:20 CDT 2005
//
// Header for packing bools
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

static inline SINT32
wwiiPackBool(LOGICAL mode, UCHAR* data, SINT32 offset, bool& value)
{
	if ( mode == TEUL_PACK )
	{
		UCHAR i = (UCHAR)(value) ;
		return teulPackUCHAR(mode, data, offset, &i, 1) ;
	}
	else
	{
		UCHAR i = 0 ;
		offset = teulPackUCHAR(mode, data, offset, &i, 1) ;
		value = (i != 0);
		return offset ;
	}
}

