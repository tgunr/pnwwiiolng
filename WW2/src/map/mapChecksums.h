#pragma once
// Created: Mon Feb 20 18:16:06 CST 2006
//
// Each client build is checksummed to allow us to
// monitor for players tampering with their .exe files
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Request the most recent checksums [background thread task]
extern void mapPollChecksums(bool force=false);

// Tell me if a particular app/checksum pair are valid.
// If the player is a producer, the checksum will be added to the db
extern bool mapCheckChecksum(UINT32 appVersion, const UINT64& checksum, bool isProducer);

