#pragma once
// Created: Tue Oct  7 15:40:06 CDT 2008
//
// Host's description of an instance.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instBasicStatus.h"

namespace Instance
{
	class ClientStatus : public BasicStatus
	{
	public:
		ClientStatus(const Descriptor& _descriptor
				, UINT16 _capacity, UINT16 _gameTime_m
				, UINT32 _createdAt, UINT32 _startedAt
				, UINT16 _squadMates
				)
			: BasicStatus(_descriptor, _capacity, _gameTime_m, _createdAt, _startedAt)
			, m_squadMates(_squadMates)
			{}
		ClientStatus(const BasicStatus& rhs_, const UINT16 squadMates_)
			: BasicStatus(rhs_)
			, m_squadMates(squadMates_)
			{}
		ClientStatus() : BasicStatus(), m_squadMates(0) {}
		ClientStatus(struct ACTION_OBJECT*);
		virtual ~ClientStatus() {}

		UINT16 squadMates() const { return m_squadMates; }

		virtual SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a) override;

	private:
		UINT16		m_squadMates;
	};
}
