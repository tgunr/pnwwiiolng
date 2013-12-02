#pragma once
// Created: Thu Jan 17 22:21:19 CST 2008
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// A return to base (RTB) is valid for any same side base of appropriate
// type (e.g. an English plane returning to an alternate French airbase
// would be a RTB).
//
// Originating Programmer: Oliver "kfsone" Smith

enum CODE_RTB
{
	RTB_SUCCESS = 0
,	RTB_BAIL_FRIENDLY
,	RTB_BAIL_ENEMY
,	RTB_DIED
};
