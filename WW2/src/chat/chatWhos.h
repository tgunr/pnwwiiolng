#pragma once
// Created: Wed Sep 21 13:56:44 CDT 2005
//
// Functions for manipulating chatWho objects
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Chat { class Who; }

extern void chatWhoPopulate(Chat::Who& who, const Player* player, const Player* target, bool forceExtendedInfo);
