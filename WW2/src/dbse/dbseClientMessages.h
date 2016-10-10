#pragma once
// Created: Fri Sep  8 13:56:55 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

//install database client general callbacks
SINT32 dbseInstallClientCallbacks();

SINT32 dbseSendHandshake(SINT32 teulIndex, UINT32 arena);
