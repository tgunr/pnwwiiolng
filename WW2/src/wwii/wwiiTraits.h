#pragma once
// Copyright(C) PlayNet Inc 2010 -- All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
// Provides various interface/trait classes.

namespace Auth { class Dispatch; }
namespace Instance { class Descriptor; }

// Microsoft's Visual C++ compiler allows you to specifically
// tell it not to mess about with vtable construction for the
// pure virtual classes that form the basis of an "interface"
// type in C++, by using __declspec(novtable).
//
// GCC doesn't appear to provide anything similar, but may be
// figuring out for itself that they aren't needed.

#ifdef _MSC_VER
# define as_interface __declspec(novtable)
#else
# define as_interface
#endif

namespace WWII
{
	namespace Traits
	{
		//! Interface for objects which provide a teulIndex.
		struct as_interface	ITeulIndex			{ virtual SINT32 teulIndex() const = 0; };
		//! Interface for objects which provide a playerID.
		struct as_interface	IPlayerID			{ virtual playerid_t playerID() const = 0; };
		//! Interface for objects which provide a (displayable) ChatHandle.
		struct as_interface	IChatHandle			{ virtual ChatHandle chatHandle() const = 0; };
		//! Interface for objects which provide a lowercase ChatHandle for matching/indexes.
		struct as_interface	IMatchHandle		{ virtual ChatHandle matchHandle() const = 0; };
		//! Interface for objects which provide an Instance::Descriptor.
		struct as_interface	IInstanceDescriptor	{ virtual const Instance::Descriptor& instDescriptor() const = 0; };
		//! Interface for objects which provide a unique instanceID..
		struct as_interface	IInstanceID			{ virtual instanceid_t instID() const = 0; };
		//! Interface for objects which provide a missionID.
		struct as_interface	IMissionID			{ virtual UINT32 missionID() const = 0; };
		//! Interface for objects which provide a brigadeID.
		struct as_interface	IBrigadeID			{ virtual UINT32 brigadeID() const = 0; };
		//! Interface for objects which provide a sortieID.
		struct as_interface	ISortieID			{ virtual sortieid_t sortieID() const = 0; };
		//! Interface for objects which provide a WEAPON_SPEC.
		struct as_interface	IWeaponSpec			{ virtual WEAPON_SPEC weaponSpec() const = 0; };
		//! Interface for objects which provide a WWII_SIDE.
		struct as_interface	ISide				{ virtual WWII_SIDE side() const = 0; };
		//! Interface for objects which provide a WWII_COUNTRY.
		struct as_interface	ICountry			{ virtual WWII_COUNTRY country() const = 0; };
		//! Interface for objects which provide a const Auth::Dispatch.
		struct as_interface	IDispatch			{ virtual const Auth::Dispatch& dispatch() const = 0; };
		//! Interface for objects which provide a modifiable Auth::Dispatch.
		struct as_interface	IMutableDispatch	{ virtual Auth::Dispatch& dispatch() = 0; };
	} // namespace Traits
} // namespace WWII
