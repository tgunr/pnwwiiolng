#pragma once
// Created: Thu Oct 16 18:53:48 CDT 2008
//
// Instancing host prototypes
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class wwiiPlayer ;
namespace Instance { class Descriptor ; }
// Install the API callbacks
extern void instUpdatePlayerSide(wwiiPlayer* player, WWII_SIDE oldSide, WWII_SIDE toSide) ;

// Get the total population for the campaign
extern UINT32 instGetCampaignPopulation() ;
// Get the side count for a particualr side of the campaign.
extern UINT32 instGetCampaignSideCount(UINT8 forSide) ;

// For the bulk of the time, players move between instances, these are
// the entry and exit points into the system.
extern void instAddPlayerToInstancingSystem(wwiiPlayer* player) ;
extern void instRemovePlayerFromInstancingSystem(wwiiPlayer* player) ;

// Get the time inside a particular instance.
extern UINT32 instGetInstanceTime(const Instance::Descriptor& instance) ;

