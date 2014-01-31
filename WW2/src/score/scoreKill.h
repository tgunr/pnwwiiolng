#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#pragma pack(push, 1)
namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class Scoring::Kill
	//! @brief Record of a player having been killed.
	//! If a Scoring::Death event can be matched to one or more
	//! Scoring::Hits to determine a killer, or if the Death
	//! message specifies a killer, that generates a Kill event
	//! with a fully qualified Actor for the creditTo()

	BEGIN_SCORING_EVENT( Kill, Scoring::Actor /*victim*/ )

		public: Kill(const LhsType& killer_, const RhsType& victim_, const SINT64 value_)
					: EventType(Type, killer_, victim_)
					, m_value(value_)
				{}

		public: const SINT64& value() const { return m_value; }

		protected: SINT64 m_value;				//!< How much this type of kill is worth.

	END_SCORING_EVENT( )
}
#pragma pack(pop)

