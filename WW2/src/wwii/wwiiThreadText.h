#pragma once
// Created: Mon Dec 12 16:02:07 CST 2005
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Thread-safe Shared text pool
//
// Originating Programmer: Oliver "kfsone" Smith

static const UINT32 TEXT_POOL_SIZE = 4095 ;
extern char* threadsAllocText() ;
extern char* threadsPrintf(const char* fmt, ...) ;
extern void threadsFreeText(char* text) ;
