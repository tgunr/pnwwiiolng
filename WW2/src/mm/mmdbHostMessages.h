#pragma once
// Created: Thu May 10 11:16:08 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

class BrigadeDetailsUpdate ;
class NewMission ;

extern SINT32 mmdbInstallHostCallbacks();
extern SINT32 mmdbSendNewMission(SINT32 teulIndex, NewMission& newMission) ;
extern SINT32 mmdbSendBrigadeDetails(SINT32 teulIndex, BrigadeDetailsUpdate& bdu) ;

