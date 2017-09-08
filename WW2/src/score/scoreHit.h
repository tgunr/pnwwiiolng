#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"
#include "score/scoreActor.h"

namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class Scoring::Hit
	//! @brief Record of one player hitting another.
	//! These are accumulated until the victim sortie despawns,
	//! and then used to determine the killer of the sortie.

	BEGIN_SCORING_EVENT( Hit, Scoring::Actor /*shooter*/ )
		Hit(const LhsType& victim_, const RhsType& shooter_)
			: EventType(Type, victim_, shooter_)
		{}
	END_SCORING_EVENT( )
}
