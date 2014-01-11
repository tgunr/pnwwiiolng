#pragma once
// Created: Mon 17 May 2004
//
// C++ data types for strat/etc within chat host
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "server/serverTypes.h"
#include "strt/strtDTL.h"

#include <map>
#include <string>
#include <math.h>

class Property ;			// Forward declaration
typedef std::map<UINT64, Property*> PROPERTY_INDEX ;
extern PROPERTY_INDEX _properties ;

// Parent class which defines a Property (ownable entity) within chat
class Property
{
public:
	// Take the top 48 bits of the fullobjid
	// this discards the class and type
	Property(const FULL_OBJ_ID& id, WWII_COUNTRY owner, const char* name, UINT32 oid)
		: m_fid(id)
		, m_key(id.Key())
		, m_name(name)
		, m_owner(owner)
		, m_flags(0)
		, m_oid(oid)
			{ _properties[m_key] = this ; }

	// Destructor
	virtual ~Property() { _properties.erase(m_key) ; }

	const FULL_OBJ_ID& Fid() const { return m_fid ; }
	UINT64 Key() const { return m_key ; }
	const char* Name() const { return m_name.c_str() ; }
	bool Matches(const FULL_OBJ_ID& rhs) const
			{ return (m_key == rhs.Key()) ; }
	bool Matches(const Property& rhs) const { return m_key == rhs.m_key ; }
	virtual Property* Parent() const { return nullptr ; }
	bool IsBridge() const { return m_fid.objClass == CLASS_CP && m_fid.objType == CP_BRIDGE ; }
	virtual bool IsCP() const { return (m_fid.objClass == CLASS_CP) ; }
	virtual bool IsFacility() const { return (m_fid.objClass == CLASS_FACILITY) ; }
	UINT32 OID() const { return m_oid ; }
	WWII_SIDE Side() const { return wwiiSide(m_owner) ; }

public:
	FULL_OBJ_ID			m_fid ;
	UINT64				m_key ;
	const std::string	m_name ;
	WWII_COUNTRY		m_owner ;
	UINT32				m_flags ;
	UINT32				m_oid ;
} ;

// Property Choke Point
class PropCP : public Property
{
public:
	PropCP(const FULL_OBJ_ID& id, WWII_COUNTRY owner, const char* name, WWII_COUNTRY controller, UINT32 oid)
		: Property(id, owner, name, oid)
		, m_controller(controller)
			{}

	virtual bool isCP() const { return true ; }
	virtual bool isFacility() const { return false ; }

public:
	WWII_COUNTRY		m_controller ;
} ;

// Property Facility
class PropFacility : public Property
{
public:
	PropFacility(const FULL_OBJ_ID& id, WWII_COUNTRY owner, const char* name, const FULL_OBJ_ID& cp, UINT32 oid) ;

	Property* parent() const { return (Property*)m_cp ; }
	virtual bool isCP() const { return false ; }
	virtual bool isFacility() const { return true ; }

public:
	PropCP*				m_cp ;				// Which CP I belong to
} ;

