#pragma once
// Copyright (C) PlayNet Inc, 2011 -- All rights reserved.
// Not for distribution except as part of a shared library.

#include "teul/teulPacket.h"
#include "score/scoreConstants.h"

#pragma pack(push, 1)
namespace Scoring
{
	extern TEUL_CALLBACK* xchgEventCallback;

	//! @class Scoring::Packet<EventTypeClass>
	//! @brief Creates a Teul::Packet encapsulation of an event message.
	template<class _EventClass>
	class Packet : public Teul::Packet
	{
	public: typedef Scoring::Packet<_EventClass> selftype;
	public: typedef _EventClass eventtype;

		//! Default constructor: Create a Teul::Packet with our
		//! callback and a payload size that carries our event.
	public: Packet() : Teul::Packet(Teul::PacketId(Scoring::CallbackX, Scoring::EVENT_Y)) { reserve(sizeof(eventtype));  }

		//! Access the Event component.
	public: const eventtype& data() const { return *(const eventtype*)payload(); }

		//! Assign the event component.
	public: void setData(const eventtype& data_)
		{
			append(data_);
		}
	};
}
#pragma pack(pop)

