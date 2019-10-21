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
	//! @class Instance::Descriptor
	//! @brief Four-part identifier for instances.
	class Descriptor
	{
	public:
        Descriptor(UINT32 _category, UINT32 _type, UINT64 _ownerID=0LL, instanceid_t _id=0LL)
        : m_id(_id), um_ownerID(_ownerID), m_category(_category), m_type(_type) {}
        Descriptor() : m_id(0LL), um_ownerHandle(0LL), m_category(0), m_type(0) {}
        Descriptor(struct ACTION_OBJECT* a);

		bool isValid() const { return category() != 0; }

		// Perform a partial match between two descriptors seeing
		// if they describe the same /kind/ of instance.
		bool isInstanceOf(const Descriptor& rhs) const
		{
			return (category() == rhs.category() && ((type() == rhs.type()) | (category() == 0)));
		}

		bool operator==(const Descriptor& rhs) const
		{
			return isInstanceOf(rhs) && ownerID() == rhs.ownerID() && this->id() == rhs.id();
		}

		bool operator!=(const Descriptor& rhs) const
		{
			return !operator==(rhs);
		}

		bool operator==(instanceid_t rhs) const { return this->id() == rhs; }
		bool operator!=(instanceid_t rhs) const { return this->id() != rhs; }

		bool operator<(const Instance::Descriptor& rhs) const;

		bool isCampaign() const { return category() == 1; }

		UINT32 category() const { return m_category; }
		UINT32 type() const { return m_type; }
        ChatHandle handle() const { return um_ownerHandle; }
        const UINT64& ownerID() const { return um_ownerID; }
        instanceid_t id() const { return m_id; }

		SINT32  packer(LOGICAL mode, ACTION_OBJECT* a);

		//! Determine if this instance has strat.
		bool hasStrat() const { return isCampaign(); }

	private:
        instanceid_t	m_id;				//!< Host-supplied, campaign-specific, unique ID.

	public:
        union
        {
            // Every instance belongs to someone, interpretation
            // of this field is category/type dependent. The host
            // doesn't read it.
            UINT64		um_ownerID;
            ChatHandle	um_ownerHandle;
        };
		UINT16			m_category;		// See Categories
		UINT16			m_type;			// Client enumerated, per-category
	};

	// Global constants
	extern const Descriptor g_invalid;
	extern const Descriptor g_campaign;
	extern const Descriptor g_lobby;

	extern instanceid_t StrToInstanceID(const char* instStr);
}

