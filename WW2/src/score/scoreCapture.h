#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"
#include "score/scoreActor.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"

namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class Scoring::Capture
	//! @brief Record a player being granted capture points.

	BEGIN_SCORING_EVENT( Capture, FULL_OBJ_ID /*building*/ )

		Capture(const LhsType& capper_, const RhsType& building_
				, WWII_SIDE forSide_
				)
			: EventType(Type, capper_, building_)
			, m_forSide(forSide_)
		{}

		public: WWII_SIDE forSide() const { return m_forSide ; }

		protected:	WWII_SIDE	m_forSide ;			//!< Which side they captured for.

		public: const LhsType& capper() const { return lhs() ; }
		public: const RhsType& building() const { return rhs() ; }

	END_SCORING_EVENT( )
}
