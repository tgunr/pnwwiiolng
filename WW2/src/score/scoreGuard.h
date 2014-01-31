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
	//! @class Scoring::Guard
	//! @brief Record a player being granted guard points.

	BEGIN_SCORING_EVENT( Guard, FULL_OBJ_ID /*building*/ )

		Guard(const LhsType& guard_, const RhsType& building_, WWII_SIDE forSide_, UINT32 seconds_)
			: EventType(Type, guard_, building_)
			, m_forSide(forSide_)
			, m_seconds(seconds_)
		{}

		public: const WWII_SIDE& forSide() const { return m_forSide; }
		public: const UINT32& seconds() const { return m_seconds; }

		protected:	WWII_SIDE	m_forSide;			//!< Which side they guarded for.
		protected:	UINT32		m_seconds;			//!< How long they were guarding for.

		public: const LhsType& guard() const { return lhs(); }
		public: const RhsType& building() const { return rhs(); }

	END_SCORING_EVENT( )
}
