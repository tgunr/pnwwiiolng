#pragma once
// Created: Wed Mar  2 12:34:57 CST 2005
//
// Types used for exchanges between mmhost and dbd
//
// Copyright (C) 2005 PlayNet Inc, All rights reserved
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// List of selections the player has made on a particular persona,
// currently just what their last brigade was
class PersonaSelections
{
public:
	PersonaSelections(UINT32 persona, UINT32 brigade)
		: m_personaID(persona), m_brigadeID(brigade) { } ;
	PersonaSelections(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a) ; }

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) { return teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)) ; }

	UINT32		m_personaID ;		// Persona ID
	UINT32		m_brigadeID ;		// Brigade ID
} ;

