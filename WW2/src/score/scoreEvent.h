#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.
//
// @file score/scoreEvent.h
// @brief Provides base class/definitions for scoring events.

#include "score/scoreActor.h"

//! @namespace Scoring
//! @brief Houses functionality related to processing of scoring events on the game servers.

#pragma pack(push, 1)
namespace Scoring
{
	namespace Events
	{
		//! Event types enumeration.
		enum Type
		{
			Invalid			=	0
		,	Assist
		,	Capture
		,	Death
		,	Despawn
		,	Guard
		,	Hit
		,	Kill
		,	PPO
		,	Resupply
		,	UMSKill
		,	WeaponReturn
		};
	}

	//! @class Scoring::EventBase
	//! @brief Un-attributed Event comprising event type and instance.
	class EventBase
	{
	public:	using LhsType = Scoring::Actor;
	public:	EventBase() : m_type(Events::Invalid), m_lhs() {}
	public: EventBase(const Events::Type type_, const LhsType& lhs_)
				: m_type(type_), m_lhs(lhs_)
				{}

			//! Event type.
			//! @see Scoring::Events::Type
	public: const Events::Type& type() const { return m_type; }
			//! Player this event is being attributed to.
	public: const LhsType& lhs() const { return m_lhs; }

	private:	Events::Type		m_type;		//!< What type of event.
	private:	LhsType				m_lhs;			//!< Player this is being attributed to.
	};

	//! @class Scoring::Event
	//! @brief Base class for describing scoring events.
	//! Events describe an action performed between two entities,
	//! lhs -> event -> rhs.
	template<typename RhsType_>
	class Event : public EventBase
	{
	public: using RhsType = RhsType_;
	public: using SelfType = Event<RhsType>;

	public:
		//! Default constructor.
		Event()
			: EventBase()
			, m_rhs()
		{}

	public:
		//! Populating constructor.
		Event(const Events::Type type_, const LhsType& lhs_, const RhsType& rhs_)
			: EventBase(type_, lhs_)
			, m_rhs(rhs_)
		{}

	public:
		//! Retrieve the right-hand-side entity.
		const RhsType& rhs() const { return m_rhs; }

	private:
		RhsType			m_rhs;			//!< Right-hand-side entity.
	};
}
#pragma pack(pop)

// Helper macro for constructing derived classes.

#define BEGIN_SCORING_EVENT( name_, RhsType_ )	\
	class name_ : public Scoring::Event<RhsType_> \
	{ \
	public:	static const Scoring::Events::Type Type = Events :: name_; \
	public: using LhsType = Scoring::Actor; \
	public: using RhsType = RhsType_; \
	public: using EventType = Scoring::Event<RhsType>; \
	public: bool process(Player* const player);

#define END_SCORING_EVENT( )	};


