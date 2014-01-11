#pragma once
// Created: Fri Aug  6 17:14:44 CDT 2010
//
//! Chat Host implementation of Player Placed Object STO.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class PPOSTO : public ServerSTO
{
public:
	PPOSTO(ACTION_OBJECT* const a) ;

	virtual bool needsBroadcastRemoval() const { return true ; }
	virtual const char* typeName() const { return "ppo" ; }

public:
	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr) ;

	virtual void extractPlayer(const Player* player) ;

	virtual bool expired() const ;

public:
	const STODamage& damage() const { return m_damage ; }
	STODamage& damage() { return m_damage ; }

private:
	STODamage		m_damage ;

	std::vector<const Player*> m_occupants ;

	TeulTimeMS		m_maxLife ;
	TeulTimeMS		m_lastOccupied ;
} ;
