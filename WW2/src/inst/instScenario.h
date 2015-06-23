#pragma once
// Created: Thu Oct 16 16:37:09 CDT 2008
//
// Structure for defining a common scenario
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Instance
{
	// This stuff is stored separately
	class Scenario
	{
	public:
		Scenario(const Descriptor& _descriptor
			, const std::string& _description
			, UINT16 _capacity, UINT16 _activationCapacity
			, UINT16 _minCapacity, UINT16 _maxCapacity
			, UINT16 _activationTime_s, UINT16 _runTime_m, UINT16 _offerTime_m
			, UINT16 _gameTime_m
			, UINT8 _minRank, UINT8 _maxRank
			, bool _autoCreate)
			: m_descriptor(_descriptor)
			, m_internalDescription(_description)
			, m_capacity(_capacity), m_activationCapacity(_activationCapacity)
			, m_minCapacity(_minCapacity), m_maxCapacity(_maxCapacity)
			, m_activationTime_s(_activationTime_s), m_runTime_m(_runTime_m), m_offerTime_m(_offerTime_m)
			, m_gameTime_m(_gameTime_m)
			, m_minRank(_minRank), m_maxRank(_maxRank)
			, m_autoCreate(_autoCreate)
			{}
		Scenario()
			: m_descriptor()
			, m_internalDescription("")
			, m_capacity(0), m_activationCapacity(0)
			, m_minCapacity(0), m_maxCapacity(0)
			, m_activationTime_s(0), m_runTime_m(0), m_offerTime_m(0)
			, m_gameTime_m(0)
			, m_minRank(0), m_maxRank(0)
			, m_autoCreate(false)
			{}

	protected:
		Descriptor		m_descriptor;				// Starts with only category+type populate

	public:
		std::string		m_internalDescription;		// Internal-use-only description of the scenario

		UINT16 			m_capacity;					// When created, capacity defaults to this
		UINT16			m_activationCapacity;		// If population reaches this, countdown starts
		UINT16			m_minCapacity;				// Minimum capacity an instantiation can have.
		UINT16			m_maxCapacity;				// Maximum capacity an instantiation can have.

		UINT16			m_activationTime_s;			// Time from exceeding minCapacity to countdown start
		UINT16			m_runTime_m;				// Minutes scenario can run for
		UINT16			m_offerTime_m;				// Minutes to continue offering for after activation

		UINT16 			m_gameTime_m;				// Lock game time to this time
		UINT8			m_minRank;					// Minimum rank to enter
		UINT8			m_maxRank;					// Max rank to enter

		bool			m_autoCreate : 1;			// true = create new instances when capacities are full
	};
}
