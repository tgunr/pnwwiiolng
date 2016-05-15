#pragma once
// Created: Tue Jan  5 11:31:27 CST 2010
//
// Player classes extended to handle management clients
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatPlayer.h"				// For Player

class ManagementPlayer : public Player
{
public:
	static void Initialize();

public:
	ManagementPlayer(SINT32 teulIndex, const char* withGameName, UINT16 withCountry);
	virtual ~ManagementPlayer();

	virtual bool isManager() const override { return true; }
	virtual bool isConnected() const override { return true; }

	// Describe to another GM
	virtual void describeTo(const Player& player, const FLOAT64& x, const FLOAT64& y, bool includePermissions) const override;
};
