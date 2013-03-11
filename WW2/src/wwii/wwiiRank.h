#pragma once
// Created: Fri Apr  1 13:24:45 CST 2005
//
// Defines a information about ranks
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

typedef std::vector<const char*> RANK_NAMES ;
extern RANK_NAMES rankNames ;

class Rank
{
public:
	enum TYPE { NO_TYPE, ENLISTED, OFFICER, COMMAND, STAFF, MAX_TYPES } ;

// Static members
private:
	typedef std::vector<Rank> RANKS ;
	static RANKS s_ranks ;					// Index of ranks
	static UINT32 s_max ;				// Highest rank

public:
	static void add(const RANK_NO level, const TYPE type, const UINT32 points, const char* description, const PermissionMask& perm) ;
	static size_t count() { return ( s_ranks.empty() ? 0 : s_ranks.size() ) ; }
	static const Rank* lookup(RANK_NO level) { return ( valid(level) ? &s_ranks[level] : NULL ) ; }
	static UINT32 max() { return s_max ; }
	static void setSize(size_t num) ;
	static bool valid(RANK_NO level) { return (level <= s_max) ; }

public:
	Rank() : m_level(0), m_type(NO_TYPE), m_perm(0L), m_points(0) {}

public:
	bool commissioned() const { return (type() == OFFICER || type() == COMMAND) ; }
	bool kingOfTheHill() const { return type() > OFFICER ; }
	RANK_NO level() const { return m_level ; }
	Rank* next() const ;
	const PermissionMask& permissions() const { return m_perm ; }
	bool permissions(const PermissionMask& mask) const { return (m_perm & mask) != 0 ; }
	UINT32 points() const { return m_points ; }
	bool staff() const { return (type() == STAFF) ; }
	TYPE type() const { return (TYPE)m_type ; }

public:
	PermissionMask	m_perm ;					// 64 bit permission mask
	UINT32			m_points ;					// Points to complete this rank
	RANK_NO			m_level ;					// My level
	UINT8			m_type ;
} ;

extern void wwiiReadRanks(class DBHANDLE& dbh) ;
extern void wwiiFreeRanks() ;

