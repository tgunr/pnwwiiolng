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

class Property;			// Forward declaration
using PROPERTY_INDEX = std::map<UINT64, std::unique_ptr<Property>>;
extern PROPERTY_INDEX _properties;

// Parent class which defines a Property (ownable entity) within chat
class Property
{
public:
	// Take the top 48 bits of the fullobjid
	// this discards the class and type
	Property(const FULL_OBJ_ID id, WWII_COUNTRY owner, const char* name, UINT32 oid)
		: m_fid(id)
		, m_key(id.Key())
		, m_name(name)
		, m_owner(owner)
		, m_flags(0)
		, m_oid(oid)
	{}

	// Destructor
	virtual ~Property() noexcept {}

	FULL_OBJ_ID Fid() const noexcept { return m_fid; }
	UINT64 Key() const noexcept { return m_key; }
	const char* Name() const noexcept { return m_name.c_str(); }
	bool Matches(FULL_OBJ_ID rhs) const noexcept
			{ return (m_key == rhs.Key()); }
	bool Matches(const Property& rhs) const noexcept { return m_key == rhs.m_key; }
	virtual Property* Parent() const noexcept { return nullptr; }
	bool IsBridge() const noexcept { return m_fid.objClass == CLASS_CP && m_fid.objType == CP_BRIDGE; }
	virtual bool IsCP() const noexcept { return (m_fid.objClass == CLASS_CP); }
	virtual bool IsFacility() const noexcept { return (m_fid.objClass == CLASS_FACILITY); }
	UINT32 OID() const noexcept { return m_oid; }
	WWII_SIDE Side() const noexcept { return wwiiSide(m_owner); }

public:
	FULL_OBJ_ID			m_fid;
	UINT64				m_key;
	const std::string	m_name;
	WWII_COUNTRY		m_owner;
	UINT32				m_flags;
	UINT32				m_oid;
};

// Property Choke Point
class PropCP : public Property
{
public:
	PropCP(const FULL_OBJ_ID id, WWII_COUNTRY owner, const char* name, WWII_COUNTRY controller, UINT32 oid)
		: Property(id, owner, name, oid)
		, m_controller(controller)
			{}
	virtual ~PropCP() noexcept {}

	virtual bool IsCP() const noexcept override { return true; }
	virtual bool IsFacility() const noexcept override { return false; }

public:
	WWII_COUNTRY		m_controller;
};

// Property Facility
class PropFacility : public Property
{
public:
	PropFacility(const FULL_OBJ_ID id, WWII_COUNTRY owner, const char* name, const FULL_OBJ_ID cp, UINT32 oid);
	virtual ~PropFacility() noexcept {}

	virtual Property* Parent() const noexcept override { return (Property*)m_cp; }
	virtual bool IsCP() const noexcept override { return false; }
	virtual bool IsFacility() const noexcept override { return true; }

public:
	PropCP*				m_cp;				// Which CP I belong to
};

