#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"
#include "score/scoreActor.h"

namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class Scoring::UMSKill
	//! @brief Record a player being granted resupply points.

	BEGIN_SCORING_EVENT( UMSKill, UINT32 /*stoUID*/ )

		UMSKill(const LhsType& shooter_, const RhsType& stoUID_)
			: EventType(Type, shooter_, stoUID_)
		{}

		public: const LhsType& shooter() const { return lhs(); }
		public: const RhsType& stoUID() const { return rhs(); }

	END_SCORING_EVENT( )
}
