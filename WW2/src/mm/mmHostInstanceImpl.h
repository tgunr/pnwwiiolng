#pragma once
// Created: Fri Jun 24 11:49:43 CDT 2011
//
// @brief Map-mission specific Host Instance Implementation overrides.
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instHostInstanceImpl.h"

namespace Instance
{
	class MMHostInstanceImpl : public HostInstanceImpl
	{
		//! @brief Allocate mmhost-specific impl instance.
	public: static HostInstanceImpl* Factory(Instance::HostInstance& _inst)
			{
				return new MMHostInstanceImpl(_inst);
			}

		//! @brief Constructor.
	public: MMHostInstanceImpl(HostInstance& inst_) : HostInstanceImpl(inst_) {}
		//! @brief Destructor.
	public:	virtual ~MMHostInstanceImpl() {}

		//! @brief Handle construction: create missions.
	public: virtual void PostConstructionHandler() OVERRIDE_FN;

		//! @brief Handle destruction:
	public: virtual void PreDestructionHandler() OVERRIDE_FN;
	};

} // namespace

