#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"
#include "wwii/wwiiFullObjID.h"

namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class	Scoring::WeaponReturn
	//! @brief	Indicates processing of a sortie's weapon completed.

	BEGIN_SCORING_EVENT( WeaponReturn, FULL_OBJ_ID /*where weapon was returned to*/ )

		public:	WeaponReturn(const LhsType& sortie_, const RhsType& destination_, const UINT32 rtbState_)
					: EventType(Type, sortie_, destination_)
					, m_rtbState(rtbState_)
				{}

		//! Get condition of the vehicle when returned. See wwii/wwiiRTBCodes.h.
		public: UINT32 rtbState() const { return m_rtbState; }

		protected: UINT32 m_rtbState;		//! See wwii/wwiiRTBCodes.h

	END_SCORING_EVENT( )

}



