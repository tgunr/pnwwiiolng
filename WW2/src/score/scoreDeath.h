#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"

#pragma pack(push, 1)
namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class Scoring::Death
	//! @brief Record of a player dieing.
	//! Used to generate a Kill event; however the killer is not
	//! guaranteed to have been determined at this point.
	//! In some cases, we always know the killer's sortie ID, e.g.
	//! death-by-building. In these cases, killerSortie will be
	//! non-zero.

	BEGIN_SCORING_EVENT( Death, Scoring::Actor /*killerSortieHint*/ )

		public:	Death(const LhsType& victim_, const RhsType& killerHint_
					, SINT32 deadX_, SINT32 deadY_, SINT32 deadZ_)
				: EventType(Type, victim_, killerHint_)
				, m_deadX(deadX_), m_deadY(deadY_), m_deadZ(deadZ_)
		{}

		protected:	SINT32	m_deadX ;
		protected:	SINT32	m_deadY ;
		protected:	SINT32	m_deadZ ;

		public: SINT32 deadX() const { return m_deadX ; }
		public: SINT32 deadY() const { return m_deadY ; }
		public: SINT32 deadZ() const { return m_deadZ ; }

	END_SCORING_EVENT( )
}
#pragma pack(pop)
