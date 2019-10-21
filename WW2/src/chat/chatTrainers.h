#pragma once
// Created: Sun Aug 21 00:48:16 CDT 2005
//
// Functions for dealing with the trainer list
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

using TRAINERS = std::set<UINT32>;

void chatUpdateTrainerStatus(const playerid_t playerID, bool isTrainer);

// You can read but not write from outside
const TRAINERS& chatGetTrainers();
