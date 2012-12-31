#pragma once
// Created: Wed Apr 20 14:51:52 CDT 2005
//
// Defines the persona update exchange between mmhost/chat
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmEnums.h"
#include "teul/teulPackers.h"
#include "weap/weapSpec.h"
#include "wwii/wwiiFullObjID.h"
#include "inst/instDescriptor.h"

//Update the mmhost sends the chat system when someone changes a persona
//	field.

namespace Chat
{

class PersonaUpdate
{
public:
	PersonaUpdate(
		const ChatHandle& handle, WWII_COUNTRY country, BRANCH_ENUM branch, RANK_NO rank
		, const Instance::Descriptor& instance, UINT32 sortieID
		, UINT32 brigade, UINT32 mission, const ChatHandle& leader
		, const FULL_OBJ_ID& origin, const FULL_OBJ_ID& target, const PermissionMask& mask, const WEAPON_SPEC& weapon
		, bool isSpawned, bool badChecksum
	) ;
	PersonaUpdate(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a) ; }
	~PersonaUpdate() {} ;

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) { return teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)) ; }

	SINT32 send() ;

	// Fields
public:
	Instance::Descriptor m_instance ;
	ChatHandle		m_handleID ;
	ChatHandle		m_leader ;			// Mission leader
	PermissionMask	m_permissionMask ;	// Current permissions mask
	FULL_OBJ_ID		m_origin ;			// The CP ID of the CP they spawned from
	FULL_OBJ_ID		m_target ;			// The CP ID of the destination of their mission
	WEAPON_SPEC		m_weapon ;			// Weapon spec of current weapon
	UINT32			m_missionID ;		// Mission the player has selected
	UINT32			m_brigadeID ;		// Brigade number
	UINT32			m_sortieID ;		// Sortie, if any.
	WWII_COUNTRY	m_country ;			// Player's country
	BRANCH_ENUM		m_branch ;
	RANK_NO			m_rank ;
//	bool			m_isLeader ;		// is this persona leader of their mission?
	bool			m_isSpawned ;		// True if the player is spawned
	bool			m_badChecksum ;		// Checksum bad?
} ;

}
