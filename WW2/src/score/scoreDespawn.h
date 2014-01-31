#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "score/scoreEvent.h"
#include "wwii/wwiiFullObjID.h"
#include "include/xyz_tuple.h"

#pragma pack(push, 1)
namespace Scoring
{
	//////////////////////////////////////////////////////////////
	//! @class	Scoring::Despawn
	//! @brief	Denotes a sortie has despawned.

	BEGIN_SCORING_EVENT( Despawn, FULL_OBJ_ID /*facility*/ )

		public: Despawn(const LhsType& subject_, const RhsType& facility_, const UINT32 exitCode_, const UINT32 exitDest_, const SINT32 x_, const SINT32 y_, const SINT32 z_, const bool isVehicleDead_, const bool isResupply_)
					: EventType(Type, subject_, facility_)
					, m_exitCode(exitCode_)
					, m_exitDest(exitDest_)
					, m_position(x_, y_, z_)
					, m_isVehicleDead(isVehicleDead_)
					, m_isResupply(isResupply_)
					{}

					//! Return To Base code: see wwiiRTBCodes.h.
		public:		UINT32	exitCode() const { return m_exitCode; }
		public:		UINT32	exitDest() const { return m_exitDest; }
		public:		const XYZTuple<SINT32>& position() const { return m_position; }
		public:		bool isVehicleDead() const { return m_isVehicleDead; }
		public:		bool isResupply() const { return m_isResupply; }

		protected:	UINT32	m_exitCode;				//! The state the vehicle was in.
		protected:	UINT32	m_exitDest;				//! Whether they are despawning or exiting the game.
		protected:	XYZTuple<SINT32> m_position;	//! Coordinates at time of despawn.
		protected:	bool	m_isVehicleDead;		//! Vehicle death is separate to crew death.
		protected:	bool	m_isResupply;			//! Was the player on a mission to resupply.

	END_SCORING_EVENT( )
}
#pragma pack(pop)

