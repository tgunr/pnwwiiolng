#pragma once
// Created: Fri Aug  6 17:14:44 CDT 2010
//
//! Chat Host implementation of Bailout STO.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatServerSTO.h"

class BailSTO : public ServerSTO
{
public:
	BailSTO(ACTION_OBJECT* const a) : ServerSTO(a) {}

	virtual const char* typeName() const OVERRIDE_FN { return "bail"; }
};
