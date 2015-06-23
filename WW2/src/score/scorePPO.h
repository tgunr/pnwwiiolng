#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"

namespace Scoring
{
	//! @class	Scoring::PPO
	//! @brief  Describes a PPO involved in an event.

	BEGIN_SCORING_EVENT( PPO, UINT32 /*ppoID*/ )

		public: PPO(const LhsType& shooter_, const RhsType ppoID_, const UINT32 ppoType_, const UINT32 ppoSubType_, const UINT32 ppoFlags_)
					: EventType(Type, shooter_, ppoID_)
					, m_type(ppoType_), m_subType(ppoSubType_), m_flags(ppoFlags_)
					{}

		public: UINT32 ppoID() const { return m_ppoID; }
		public: UINT32 type() const { return m_type; }
		public: UINT32 subType() const { return m_subType; }
		public: UINT32 flags() const { return m_flags; }

		protected:	UINT32	m_ppoID;			//!< PPO's ID.
		protected:	UINT32	m_type;				//!< STO-type field.
		protected:	UINT32	m_subType;			//!< STO-subType field.
		protected:	UINT32	m_flags;			//!< Flags teh PPO had.

	END_SCORING_EVENT( )
}
