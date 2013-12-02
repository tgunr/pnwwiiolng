#pragma once
// Created: Thu Oct 13 20:34:02 CDT 2005
//
// MOTD handling
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

extern struct MOTD_REPLY& chatGetMotd(UINT32);
extern bool chatSetMotd(UINT32, const ChatHandle& poster, const std::string&);

