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

		Hit(const LhsType& victim_, const RhsType& shooter_
				, UINT32 components_, const UINT64& joules_
				)
			: EventType(Type, victim_, shooter_)
			, m_components(components_)
			, m_joules(joules_)
		{}

		public: UINT32 components() const { return m_components; }
		public: const UINT64& joules() const { return m_joules; }

		protected:	UINT32		m_components;		//!< Number of components hit.
		protected:	UINT64		m_joules;			//!< Total joules damage done.

	END_SCORING_EVENT( )
}
