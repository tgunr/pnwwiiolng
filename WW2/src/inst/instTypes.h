#pragma once
// Created: Tue Oct  7 15:42:06 CDT 2008
//
// Instancing typedefs/enums etc.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Instance
{
	// Forward class declarations to save some includes.
	class Descriptor;
	class BasicStatus;

	// CATEGORIES:
	// 	These are categories shared between the client and host only.
	enum
	{
		_INVALID = 0				//!< Indicates "not an instance". Typically when someone is NOT in an instance yet.
	,	_CAMPAIGN = 1				//!< Campaign instance.
	,	_TUTORIAL = 2				//!< Shared tutorials.
	,	_TRAINING = 10				//!< Training tutorials.
	,	_SCENARIO = 20				//!< Scenarios (??).
		// CA2 instance types (index needs to match manual entry for this game type in the translation tool on the client)
	,	_DEATHMATCH = 30			// CA2 deathmatch instances
	,	_CTF = 40					// CA2 capture the flag instances
		// Instance::_AD_HOC is provided as a base for the client to
		// enumerate it's own instance types that do not need
		// host support. Enumerate them in your own file please
		// e.g.
		//    enum { _MY_INSTANCE_TYPE = Instance::_AD_HOC, _MY_TYPE2, _MY_TYPE3 ... };
	,	_AD_HOC = 100
	};

	// Codes returned by Listing, Create and Select
	enum ResultCodes
	{
		_BAD_DATA				//!< (All)      Uninitialized
	,	_SUCCESS				//!< (All)      Request was completed
	,	_ERROR					//!< (Both)     Unknown/internal error
	,	_SPECIAL_REQUESTED		//!< (List/Cre) Requested to list/create a campaign instance
	,	_BAD_DESCRIPTOR			//!< (All)      Garbage in the descriptor
	,	_ID_DOES_NOT_EXIST		//!< (Select)   Unique ID you specified does not exist
	,	_EXISTS					//!< (Create)   Match for your descriptor exists (where applicable)
	,	_ID_MISMATCH			//!< (Select)   ID does not match the category/type/owner you provided
	,	_FULL					//!< (Select)   Specified instance is full
	};

	// For teulKit
	enum { INST_X = 209 };
	enum
	{
		INST_XCHG_LISTING_Y = 1
	,	INST_XCHG_SELECT_Y
	,	INST_XCHG_CREATE_Y
	};
}

