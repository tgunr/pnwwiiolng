#pragma once
// Created: Mon Oct 22 13:19:34 CDT 2007
//
// Provides a background thread that writes to the Audit Database
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Setup the audit db thread
extern void wwiiAuditDBInitialize() ;

// Queue a statement for the Audit DB
extern void wwiiAuditDBStatement(const char* fmt, ...) ;

