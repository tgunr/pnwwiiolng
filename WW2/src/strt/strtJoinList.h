#pragma once
// Date: Wed Dec  7 21:54:38 CST 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class WeapJoinRequestNode ;
class FacilityStruct ;
struct POSTED_WEAPON ;

#include <vector>
typedef std::vector<WeapJoinRequestNode*> WeapJoinRequestList ;

struct JOIN_LIST
{
	enum _TYPE
	{
		_FREE,
		_FACILITY,
		_POST_JOIN_REQ,
		_POST_CREW
	} ;

	JOIN_LIST() : m_type(_FREE), m_facility(NULL), m_weapon(NULL), m_list() {}
	JOIN_LIST(_TYPE type, FacilityStruct* facility, POSTED_WEAPON* weapon)
		: m_type(type), m_facility(facility), m_weapon(weapon), m_list() { if ( type == _FACILITY ) m_list.reserve(16) ; }

	// Delete all of hte WeapJoinRequestLists.
	void clear(UINT32 exceptPlayerID, bool announceDenied) ;

	// Iterators
	WeapJoinRequestList::const_iterator begin() const { return m_list.begin() ; }
	WeapJoinRequestList::const_iterator end() const { return m_list.end() ; }
	const WeapJoinRequestNode* front() const { return empty() ? NULL : m_list.front() ; }
	const WeapJoinRequestNode* back() const { return empty() ? NULL : m_list.back() ; }
	WeapJoinRequestNode* front() { return empty() ? NULL : m_list.front() ; }
	WeapJoinRequestNode* back() { return empty() ? NULL : m_list.back() ; }
	bool empty() const { return m_list.empty() ; }

	void Add(WeapJoinRequestNode* node) ;
	void Extract(WeapJoinRequestNode* node) ; // Note: does note release the node itself.

public:
//	const WeapJoinRequestList& list() const { return m_list ; }
//	const WeapJoinRequestNode* list(UINT32 i) const { return m_list[i] ; }
//	WeapJoinRequestNode* list(UINT32 i) { return m_list[i] ; }

public:
	_TYPE				m_type;
	FacilityStruct*		m_facility;	//The facility to which its attached
	POSTED_WEAPON*		m_weapon ;	// Posted weapon to which we are attached
private: WeapJoinRequestList	m_list ;
} ;

