#pragma once
// Created: Tue Apr  4 17:48:57 CDT 2006
//
// Defines the host implementation of a vehicle STO
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class VehicleSTO : public ServerSTO
{
public:
	VehicleSTO(ACTION_OBJECT* const a) : ServerSTO(a) {}
	virtual const char* typeName() const { return "veh" ; }
} ;
