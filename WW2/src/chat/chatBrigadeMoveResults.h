#pragma once
// Created: Wed Dec  7 14:23:09 CST 2005
//
// Enumerations for BrigadeMoves
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

enum BRIGADE_MOVE_RESULT
{
	BMR_APPROVED				// 0
	, BMR_FAILED
	, BMR_FAILED_TARGET
	, BMR_FAILED_OWNER
	, BMR_FAILED_PERMISSION
	, BMR_FAILED_TIME
	, BMR_FAILED_LINK
	, BMR_FAILED_CONTACT
	, BMR_FAILED_ABS
	, BMR_FAILED_AIR
	, BMR_FAILED_NAVAL			// 10
	, BMR_FAILED_FRONTLINE
	, BMR_FAILED_CONTROL
	, BMR_FAILED_NFL
	, BMR_FAILED_OFFMAP				// Tried to move non-ground bde
	, BMR_FAILED_OFFMAP_NP			// Tried to move offmap from a non-pocket
	, BMR_FAILED_PREM_SURDR			// Tried to surrender prematurely
	, BMR_FAILED_ENEMY_DEPOT		// Don't control both depots on link
	, BMR_FAILED_ENEMY_FB			// Don't control FB on link
} ;

