#pragma once
// Created: Fri Jun 24 11:21:54 CDT 2011
//
//! @brief Base class for host-specific instance implementations.
//! @author Oliver 'kfsone' Smith <osmith@playnet.com>
//
// Copyright (C) PlayNet, Inc 2011 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instHostInstance.h"

class Player ;

namespace Instance
{
	//! @class Instance::HostInstanceImpl
	//! @brief Base class for host-specific instance implementations.
	class HostInstanceImpl
	{
		//! @brief Constructor.
		//! @param[in] _inst The ID of this instance.
		public: HostInstanceImpl(HostInstance& _inst)
				: m_inst(_inst)
				{
				}

		// Members

		//! @brief Reference to the instance we belong to.
		private: HostInstance& m_inst ;
		//! @brief Returns a const-reference to the instance we belong to.
		public: const HostInstance& inst() const { return m_inst ; }
		//! @brief Returns a reference to the instance we belong to.
		public: HostInstance& inst() { return m_inst ; }
		//! @brief Destructor.
		public:	virtual ~HostInstanceImpl() {}

		//! @brief Post-construction handler.
		public:	virtual void PostConstructionHandler() {}

		//! @brief Pre-destruction handler.
		public:	virtual void PreDestructionHandler() {}

		//! @brief Player joining an instance.
		public: virtual void PlayerJoinHandler(wwiiPlayer* /*player*/) {}
		//! @brief Player leaving an instance.
		public: virtual void PlayerLeaveHandler(wwiiPlayer* /*player*/) {}

		//! Check a password.
		public: virtual bool CheckPassword(const std::string& /*utf8Password*/) { return true ; }
		//! Assign a password.
		public: virtual void SetPassword(const std::string& /*utf8Password*/) {}
		//! Retrieve the current password string.
		public: virtual const char* GetPassword() const { return "" ; }

		//! Update state.
		public: virtual void SetState(Instance::State state_) { }
	} ;

} // namespace Instance

