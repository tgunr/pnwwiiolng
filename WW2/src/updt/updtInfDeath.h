#pragma once
// Copyright (C) PlayNet Inc, 2012 -- All rights reserved.
// Originating Programmer: Oliver "kfs1" Smith

//! Death Types
ENUMCLASS DeathType
{
	Falling
,	Shot				// Being dropped by a bullet.
,	Explosion			// Toss me up in the air.
,	Collision			// Impact with vehicle
} ;

#pragma pack(push, 1)	// For wire format
class InfDeath
{
public:
	InfDeath(const FLOAT32 x_, FLOAT32 y_, FLOAT32 z_, const sortieid_t killedByIndex_, const ChatHandle killedByHandle_
			, const DeathType deathType_, const bool byEnvironment_, const UCHAR deathAngle_, const UCHAR deathForce_
			)
		: x(x_), y(y_), z(z_)
		, killedByIndex(killedByIndex_)
		, killedByHandle(killedByHandle_)
		, deathType(deathType_)
		, byEnvironment(byEnvironment_)
		, deathAngle(deathAngle_)
		, deathForce(deathForce_)
		, victim(0ULL)
	{}

public:
	static SINT32  Send(SINT32 teulIndex
						, const FLOAT32 x_, FLOAT32 y_, FLOAT32 z_, const sortieid_t killedByIndex_, const ChatHandle killedByHandle_
						, const DeathType deathType_, const bool byEnvironment_, const UCHAR deathAngle_, const UCHAR deathForce_
						) ;

	//// Members.
public:
	//! Position of the dying player.
	FLOAT32			x, y, z ;

	//! Who killed me. Set to ~0ULL for AI, please.
	sortieid_t		killedByIndex ;		// HostIndex of killer.
	ChatHandle		killedByHandle ;	// ChatHandle of killer.

	//! TROY: What do you want here? I'm not going to use any of this.
	DeathType		deathType : 7 ;		// Explosion, whatever.
	UCHAR			byEnvironment : 1 ;	// E.g. AI killed you.
	UCHAR			deathAngle : 4 ;	// Direction to die in.
	UCHAR			deathForce : 4 ;	// Hint as to how much force to use.

	ChatHandle		victim ;			// Player who died.
};
#pragma pack(pop)
