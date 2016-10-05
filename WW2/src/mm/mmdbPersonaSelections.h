#pragma once
// Created: Tue Feb 21 16:43:31 CST 2006
//
// Functions that handle saving of persona brigade selections
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiDBThreads.h"

class Player;

// The main thread function
extern void* mmdbPersonaSelectionsThread(void*);

// Function that saves the brigade selection of a persona
// NOTE: Save is not immediate and may not occur if the
// function is called for the same persona again before
// the save is committed to disk
extern void mmdbSavePersonaSelection(const Player* player);

struct PersonaSelectionThread : public DBThread::Thread
{
	virtual void dbrun() override;
};
