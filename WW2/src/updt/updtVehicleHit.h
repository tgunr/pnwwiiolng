#pragma once
/// @file	updt/updtVehicleHit.h
///	@author	Oliver 'kfs1' Smith
///	@brief	Provides the Vehicle::Hit and Vehicle::Hit::Component structure.

#include "updt/updtEnums.h"
#include "include/inplace_vector.h"

namespace Vehicle
{
	/// @struct	Vehicle::Hit
	///	@brief	Describes one vehicle hitting another.
	struct Hit
	{
		Hit() = default;
		Hit(const Hit& rhs) = default;
		Hit& operator = (const Hit& rhs) = default;

		Hit(UINT32 _target) : m_shooter(0), m_target(_target), m_components(), m_timestamp(teulClock64) {}
		Hit(ACTION_OBJECT* const a)
			: m_shooter(0), m_target(0), m_components()
			, m_timestamp(teulClock64)
			{ packer(TEUL_UNPACK, a); }

		SINT32 packer(PackMode mode, ACTION_OBJECT* a);

		UINT32				m_shooter;		///< Host index of the shooter.
		UINT32				m_target;		///< Host index of the victim.

		/// @struct Vehicle::Hits::Component
		/// @brief	Describes damage application to a given component during a hit.
		struct Component
		{
			Component() = default;
			Component(const Component& rhs) = default;
			Component& operator = (const Component& rhs) = default;

			Component(UCHAR _compID, bool _isImportant, UCHAR _numHits, UINT64 _damage)
				: compID(_compID), isImportant(_isImportant), NumHits(_numHits), m_damage(_damage)
				{}

			bool operator == (const Component& rhs) const
			{
				return compID == rhs.compID
						&& isImportant == rhs.isImportant
						&& NumHits == rhs.NumHits
						&& m_damage == rhs.m_damage;
			}

			UCHAR	compID;				///< ID of this component.
			bool	isImportant;			///< Flag indicating importance.
			UCHAR	NumHits;				///< How many hits.
			UINT64	m_damage;				///< Joules damage applied.
		};

		using Components = WWII::inplace_vector<Vehicle::Hit::Component, VEH_MAX_OBJ_HIT>;
		Components			m_components;	///< Tracking of strikes to components.

		// For tracking when we received hits.
		TeulTimeMS	m_timestamp;
	};
}
