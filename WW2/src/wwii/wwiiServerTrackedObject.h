#pragma once
// Created: Tue Mar 21 14:55:05 CST 2006
//
//! @file wwii/wwiiServerTrackedObject.h
//! @brief Shared base class for describing server
//!        tracked objects.
//! @details
//! class ServerTrackedObject provides a common base for all
//! server tracked objects, providing a unique ID, type and
//! sub type identifiers, a position, and client-specific flags,
//! as well as an [optional] life-span management mechanism
//! and an attitude/orientation vector.
//!
//! Implementation classes should either derive directly from
//! ServerTrackedObject unless they need to share additional
//! data client<->server.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instTypes.h"

#define STO_SPAWN_THRESHOLD 512.0f

//////////////////////////////////////////////////////////////////////
//! @class WorldPosition_m
//! Network transportable double-precision world co-ordinates (x,y,z)
//! in meters.

class WorldPosition_m
{
public:
	//! Construct from x, y, z values.
	WorldPosition_m(const FLOAT64& withX, const FLOAT64& withY, const FLOAT64& withZ) : m_x(withX), m_y(withY), m_z(withZ) {}
	//! Construct from a vectors of doubles.
	WorldPosition_m(const FLOAT64VECTOR& vec) : m_x(vec.x), m_y(vec.y), m_z(vec.z) {}
	//! Network constructor.
	WorldPosition_m(ACTION_OBJECT* const a) { packer(FALSE, a); }
	//! Copy-constructor.
	WorldPosition_m(const WorldPosition_m& rhs) : m_x(rhs.m_x), m_y(rhs.m_y), m_z(rhs.m_z) {}
	//! Default constructor.
	WorldPosition_m() : m_x(0.0), m_y(0.0), m_z(0.0) {}

public:
	void packer(LOGICAL mode, ACTION_OBJECT* a, SINT32 = 0) { return (mode == TEUL_PACK) ? pack(a, 0) : unpack(a, 0); }
	void pack(ACTION_OBJECT* const a, SINT32 = 0) const;
	void unpack(ACTION_OBJECT* const a, SINT32 = 0);

	WorldPosition_m& operator = (const WorldPosition_m& rhs) { memcpy(this, &rhs, sizeof(*this)); return *this; }

	bool operator == ( const WorldPosition_m & rhs ) const { return (memcmp(this, &rhs, sizeof(*this)) == 0); }

public:
	const FLOAT64& x() const { return m_x; }
	const FLOAT64& y() const { return m_y; }
	const FLOAT64& z() const { return m_z; }

private:
	FLOAT64	m_x, m_y, m_z;
};

//////////////////////////////////////////////////////////////////////
//! @class AttitudeVector
//! Network transportable single-precision attitude vector (x,y,z).

class AttitudeVector
{
public:
	//! Construct from x, y, z values.
	AttitudeVector(FLOAT32 withX, FLOAT32 withY, FLOAT32 withZ) : m_x(withX), m_y(withY), m_z(withZ) {}
	//! Network constructor.
	AttitudeVector(ACTION_OBJECT* const a) { packer(FALSE, a); }
	//! Copy constructor.
	AttitudeVector(const AttitudeVector& rhs) : m_x(rhs.m_x), m_y(rhs.m_y), m_z(rhs.m_z) {}
	//! Default constructor.
	AttitudeVector() : m_x(0.0f), m_y(0.0f), m_z(0.0f) {}

public:
	void packer(LOGICAL mode, ACTION_OBJECT* a, SINT32 = 0) { return (mode == TEUL_PACK) ? pack(a, 0) : unpack(a, 0); }
	void pack(ACTION_OBJECT* const a, SINT32 = 0) const;
	void unpack(ACTION_OBJECT* const a, SINT32 = 0);

	AttitudeVector& operator = (const AttitudeVector& rhs) { memcpy(this, &rhs, sizeof(*this)); return *this; }

public:
	FLOAT32 x() const { return m_x; }
	FLOAT32 y() const { return m_y; }
	FLOAT32 z() const { return m_z; }

private:
	FLOAT32	m_x, m_y, m_z;
};

//////////////////////////////////////////////////////////////////////
//! @class ServerTrackedObject
//
//! Shared base class for all ServerTrackedObjects.
//! The "type" determines the shared understanding of the object,
//! while "subType" is used by the client to distinguish between
//! functionality-similar derivatives of a given type (e.g.
//! a different color of smoke grenade, or which type of vehicle).
//!
//! These determine whether the lifespan and/or attitude properties
//! come into play for this STO type.
//!
//! All STOs have unique server-side IDs.

class ServerTrackedObject
{
public:
	//! Shared object implementation type. The client and host
	//! both use this to determine the transport class used to
	//! pack/unpack the data and the local implementation details.
	enum
	{
		_REMOVAL								//!< "Removal" stub.
	,	_VEHICLE								//!< Object representing a vehicle.
	,	_ORDNANCE								//!< Fire ordnance.
	,	_HULK									//!< Dead vehicle.
	,	_EFFECT									//!< Graphical effect (e.g. smoke).
	,	_TERRAIN								//!< Dynamically placed terrain object.
	,	_PPO									//!< Player placed object; built on terrain sto.
	,	_UMS									//!< Unmanned mobile spawn point; built on ppo.
	,	_BAILOUT								//!< Player bailout from plane.
	};

	enum
	{
		// Update as pack sizes increase
		_MAX_PACKED_SIZE = 44					//!< Max packed size in bytes of a transport class.
	};

public:
	//! Host/Offline constructor specifying a client-generated UID, including optional lifeSpan
	explicit ServerTrackedObject(UINT32 withUid, UINT16 withType, UINT16 withSubType, const WorldPosition_m& withPosition, const AttitudeVector withAttitude=AttitudeVector(), UINT32 withFlags=0, UINT32 withLifeSpan=0)
		: m_instanceID(Instance::_CAMPAIGN)
		, m_position(withPosition), m_attitude(withAttitude)
		, m_created(teulClock64)
		, m_uid(withUid)
		, m_type(withType), m_subType(withSubType)
		, m_flags(withFlags)
		, m_lifespan(withLifeSpan)
		, m_owner(0)
		, m_activated(false)
		{}

	//! Copy constructor.
	ServerTrackedObject(const ServerTrackedObject& rhs)
		: m_instanceID(rhs.m_instanceID)
		, m_position(rhs.m_position), m_attitude(rhs.m_attitude)
		, m_created(rhs.m_created)
		, m_uid(rhs.m_uid)
		, m_type(rhs.m_type)
		, m_subType(rhs.m_subType)
		, m_flags(rhs.m_flags)
		, m_lifespan(rhs.m_lifespan)
		, m_owner(0)
		, m_activated(false)
		{}

	//! Network constructor.
	explicit ServerTrackedObject(ACTION_OBJECT* const a)
		: m_instanceID(Instance::_CAMPAIGN)
		, m_position(), m_attitude()
		, m_created(0)
		, m_uid(0)
		, m_type(0)
		, m_subType(0)
		, m_flags(0)
		, m_lifespan(0)
		, m_owner(0)
		, m_activated(false)
		{
			packer(FALSE, a);
		}

	//! Default constructor.
	ServerTrackedObject()
		: m_instanceID(Instance::_CAMPAIGN)
		, m_position(), m_attitude()
		, m_created(0)
		, m_uid(0)
		, m_type(0)
		, m_subType(0)
		, m_flags(0)
		, m_lifespan(0)
		, m_owner(0)
		, m_activated(false)
		{}

	virtual ~ServerTrackedObject(){}

public:
	//! Find an STO by it's unique ID.
	//! @param[in]	uid			Unique ID of the STO.
	//! @return nullptr if not found, otherwise pointer to the ServerTrackedObject base class
	//!         of the STO.
	static ServerTrackedObject* lookup(UINT32 uid);

	//! Add an STO to the internal index.
	static int addToIndex(ServerTrackedObject* sto);

	//! Result codes for addToIndex.
	enum
	{
		_ADD_FAIL = -1					//!< Unable to add.
	,	_ADD_SUCCESS = 0				//!< Added.
	,	_ADD_DUPLICATE = 1				//!< Duplicate UID.
	};

public:
	virtual SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

#ifdef FRONT_END
	//! Client function: Ask the host to create an instance of this STO.
	virtual void request(SINT32 chatIndex);
#endif

	//! Apply this many joules damage to this STO.
	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr);

public:
	//! Get the unique ID.
	//! @return Per-campaign uniqueID for this sto.
	UINT32 uid() const { return m_uid; }
	//! Get the type.
	//! @return STO Type.
	UINT16 type() const { return m_type; }
	//! Get the sub-type.
	//! @return client-specific sub-type.
	UINT16 subType() const { return m_subType; }
	//! Get the position.
	//! @return Absolute world position in meters.
	const WorldPosition_m& position() const { return m_position; }
	//! Get the creation timestamp.
	//! @return millisecond timestamp of when the STO was created.
	TeulTimeMS created() const { return m_created; }
	//! Get the client specific flags.
	//! @return client-specific flags.
	UINT32 flags() const { return m_flags; }
	//! Returns the unique ID of the STO's instance.
	//! @return unique ID of the instance.
	instanceid_t instanceID() const { return m_instanceID; }
	//! Return the attitude vector (if any).
	//! @return Attitude/orientation vector.
	const AttitudeVector& attitude() const { return m_attitude; }
	//! Return the maximum lifespan of the object or 0.
	//! @return Lifespan (including elapsed) in milliseconds, or 0 if not applicable.
	UINT32 lifespan() const { return m_lifespan; }

	//! Get the time in milliseconds since the object was created.
	//! @return Time in milliseconds since the object was created.
	TeulTimeMS age() const { return m_created ? teulClock64 - m_created : 0LL; }

	//! Return the chat handle of the creator of this sto.
	//! @return ChatHandle or 0; currently only packed for bailout stos.
	ChatHandle owner() const { return m_owner; }

	//! Determine if this object has expired.
	//! @return true if the object has expired, otherwise false.
	virtual bool expired() const
	{
		return ( lifespan() && age() >= lifespan() );
	}

	//! Used to force an STO to expire.
	void setExpired()
	{
		m_created = 1ULL;				// A very long time ago.
		m_lifespan = 1;					// A very short time to live.
	}

	//! Set the chat handle of the player who created this sto
	void setOwner(ChatHandle o)
	{
		m_owner = o;
	}

	//! Get the remaining miliseconds lifespan.
	//! @return Number of miliseconds until object should expire,
	//!         or zero if object is already past due.
	UINT32 timeRemaining() const { return expired() ? 0 : (UINT32)lifespan() - (UINT32)age(); }

public:
	virtual bool activate();
	virtual void deactivate();

public:
	virtual bool activated() { return m_activated; }

public:
	instanceid_t	m_instanceID;				//!< Used by host to track instances.
protected:
	WorldPosition_m		m_position;				//!< World-relative position of object.
	AttitudeVector		m_attitude;				//!< Attitude/orientation.
	TeulTimeMS			m_created;				//!< Creation timestamp.
	UINT32				m_uid;					//!< Per-camapign unique ID.
	UINT16				m_type;					//!< Transport class.
	UINT16				m_subType;				//!< Client-specific.
	UINT32				m_flags;				//!< Client data (does not have to be state).
	UINT32				m_lifespan;				//!< Miliseconds time-to-live.
	ChatHandle			m_owner;				//!< ChatHandle of creating player or 0

private:
	bool m_activated;							//!< Set to true once activated.

	friend void wwiiRemoveSTO(UINT32 uid);
};

// Functions

//! Pack the type of an STO only.
extern SINT32 wwiiPackServerTrackedObjectType(LOGICAL mode, UCHAR* buffer, SINT32 offset, UINT16* type, SINT32 size);

//! Remove an STO by its UID.
extern void wwiiRemoveSTO(UINT32 uid);
extern bool wwiiValidSTO(UINT32 uid);
extern ServerTrackedObject* wwiiGetSTO(UINT32 uid);

//! Walk the STO lists for entries that have expired
extern void wwiiExpireSTOs();

typedef void (STO_ITERATOR) (ServerTrackedObject&);
typedef bool (STO_COOKIE_ITERATOR) (ServerTrackedObject& sto, void* cookie);

//! Patrol function.
extern void wwiiForEachSTO(STO_ITERATOR&);
extern bool wwiiForEachSTO(STO_COOKIE_ITERATOR&, void* cookie = nullptr);

// return a count of the specified type or of all types
extern UINT32 wwiiCountSTOs(UINT16 type = 0, UINT16 subtype = 0);
