#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"

#pragma pack(push, 1)
namespace Scoring
{
	BEGIN_SCORING_EVENT( Assist, Scoring::Actor /*victim*/ )

		public: Assist(const LhsType& killer_, const RhsType& victim_, const SINT64 value_)
					: EventType(Type, killer_, victim_)
					, m_value(value_)
				{}

		public: const SINT64& value() const { return m_value ; }

		protected: SINT64 m_value ;				//!< How much this type of kill is worth.

	END_SCORING_EVENT( )
}
#pragma pack(pop)

