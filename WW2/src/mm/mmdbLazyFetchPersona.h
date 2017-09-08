#pragma once
// Created: Sun Sep 7 06:24:00 CDT 2017
//
// Prototypes for mmdbLazyFetchPersona.cpp
//
// Copyright (C) PlayNet, Inc 1999-2017 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Ian "frisbone" Frisbie

#include "mm/mmEnums.h"
#include "mm/mmQueries.h"
#include "wwii/wwiiDBThreads.h"

typedef void (*PersonaCallbackFunction)(void *, bool);

/*
	This class allows a lazy selection of the persona when requested (not blocking main thread)
*/
class SelectPersonaQuery : public DBThreadQuery {
public:
	SelectPersonaQuery(PersonaSelection ps, Player* player, PersonaCallbackFunction callback=nullptr, void *callbackData=nullptr)
		: DBThreadQuery(DBThread::Default, "SelectPersonaQuery", DBThreadQuery::_CALLBACK
			, QRY_SELECT_PERSONA_DATA, player->playerID(), ps.m_countryID, ps.m_branchID)
		, m_ps(ps)
		, m_player(player)
		, m_callBack(callback) 
		, m_callbackData(callbackData)
		, m_newMinuteBalance(0)
		, m_changed(false)
		, m_rankNo(0)
		, m_rankPoints(0) { }

protected:
	PersonaSelection	m_ps;
	Player 	*m_player;
	PERSONA_LOAD_STATUS m_status;
        PersonaCallbackFunction m_callBack;
	void *m_callbackData;
	UINT32 m_newMinuteBalance;
	bool m_changed;
	RANK_NO m_rankNo;
	UINT32 m_rankPoints;
protected:
	virtual bool Process(const size_t numRows) override;

protected:
	virtual void Success() override;

	virtual void Failure() override;
};

/*
	This class allows a lazy creation of the persona when requested (not blocking main thread)
*/
class CreatePersonaQuery : public DBThreadQuery {
public:
	CreatePersonaQuery(PersonaSelection ps, Player* player, PersonaCallbackFunction callback=nullptr, void *callbackData=nullptr)
		: DBThreadQuery(DBThread::Default, "CreatePersonaQuery", DBThreadQuery::_CALLBACK
			, QRY_INSERT_NEW_PERSONA_RECORD, player->playerID(), ps.m_countryID, ps.m_branchID)
		, m_player(player)
		, m_ps(ps)
		, m_callBack(callback)
		, m_callbackData(callbackData) { }

protected:
	Player 	*m_player;
	PersonaSelection	m_ps;
        PersonaCallbackFunction m_callBack;
	void *m_callbackData;

protected:
	virtual bool Process(const size_t numRows) override;

protected:
	virtual void Success() override;
	virtual void Failure() override;
};
