#pragma once
// Created: Mon Jul 30 15:54:55 CDT 2007
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Chat { struct LocalizedMessage; }

using HOSTLOC = std::map<int, std::string>;

bool chatReadHostLocFile();
const char* chatLString(int prefix, int message, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="");
const char* chatLString(const Chat::LocalizedMessage&);
