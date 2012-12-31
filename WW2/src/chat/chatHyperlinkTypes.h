#pragma once

// Enumeration of the types of hyperlinks.

namespace Chat
{
	enum HLType
	{
		_Player			// Details of a player.
	,	_Squad			// Details of a squad.
	,	_SquadInvite	// Inviation to join a squad.
	,	_Brigade		// Details of a squad
	,	_CP				// Details of a town.
	,	_Facility		// Details of a facility.
	,	_Mission		// Details of a mission.

	,	_HLTypes
	} ;

	extern const HLTypes[_HLTypes] ;
} // namespace

