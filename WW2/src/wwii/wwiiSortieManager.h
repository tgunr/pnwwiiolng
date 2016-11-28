#pragma once
// Created: Wed Jan 16 15:11:09 CST 2008
//
// Functions for managing commits of sorties to the database
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class SortieManager
{
public:
	SortieManager(const char* set, const char* where) : m_set(set), m_where(where) {}

	void AddSortie(const sortieid_t sortieID) { m_pending.push_back(sortieID); }
	bool Commit();

private:
	const char*	m_set;
	const char*	m_where;

	using SORTIES = std::vector<sortieid_t>;
	SORTIES		m_pending;
};

