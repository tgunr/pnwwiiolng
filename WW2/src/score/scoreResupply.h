#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"
#include "score/scoreActor.h"

namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class Scoring::Resupply
	//! @brief Record a player being granted resupply points.

	BEGIN_SCORING_EVENT( Resupply, ChatHandle /*resupplied*/ )

		Resupply(const LhsType& resupplier_, const RhsType& resupplied_)
			: EventType(Type, resupplier_, resupplied_)
		{}

		public: const LhsType& resupplier() const { return lhs(); }
		public: const RhsType& resupplied() const { return rhs(); }

	END_SCORING_EVENT( )
}
