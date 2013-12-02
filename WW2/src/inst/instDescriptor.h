#pragma once
// Created: Tue Oct  7 15:39:54 CDT 2008
//
// Describing an instance between the client/host
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//! @namespace Instance
//! @brief Components of the world-instancing system.

namespace Instance
{
#pragma pack(push, 1)
	//! @class Instance::Descriptor
	//! @brief Four-part identifier for instances.
	class Descriptor
	{
	public:
		Descriptor(const UINT16 category_, const UINT16 type_, const ChatHandle& handle_=0LL, const instanceid_t& id_=0ULL)
			: m_id(id_), m_handle(handle_), m_category(category_), m_type(type_) {}
		Descriptor() : m_id(0LL), m_handle(0LL), m_category(0), m_type(0) {}
		Descriptor(ACTION_OBJECT* a);

		bool isValid() const { return category() != 0; }

		// Perform a partial match between two descriptors seeing
		// if they describe the same /kind/ of instance.
		bool isInstanceOf(const Descriptor& rhs) const
		{
			return (category() == rhs.category() && ((type() == rhs.type()) | (category() == 0)));
		}

		bool operator==(const Descriptor& rhs) const
		{
			return isInstanceOf(rhs) && this->id() == rhs.id();
		}

		bool operator!=(const Descriptor& rhs) const
		{
			return !operator==(rhs);
		}

		bool operator==(const instanceid_t& rhs) const { return this->id() == rhs; }
		bool operator!=(const instanceid_t& rhs) const { return this->id() != rhs; }

		bool operator<(const Instance::Descriptor& rhs) const;

		bool isCampaign() const { return category() == 1; }

		UINT32 category() const { return m_category; }
		UINT32 type() const { return m_type; }
		const ChatHandle& handle() const { return m_handle; }
		const instanceid_t& id() const { return m_id; }

		int packer(unsigned int mode, ACTION_OBJECT* a);

		//! Determine if this instance has strat.
		bool hasStrat() const { return isCampaign(); }

	private:
		instanceid_t	m_id;			//!< Host-supplied, campaign-specific, unique ID.

	public:
		ChatHandle		m_handle;
		UINT16			m_category;		// See Categories
		UINT16			m_type;			// Client enumerated, per-category
	};
#pragma pack(pop)

	// Global constants
	extern const Descriptor g_invalid;
	extern const Descriptor g_campaign;
	extern const Descriptor g_lobby;

	extern instanceid_t StrToInstanceID(const char* instStr);
}

