#pragma once
// Created: Mon Apr 25 15:11:31 CDT 2011
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Oliver "kfsone" Smith
//
// Base class for the persona/group xfer objects

namespace Server
{
	class Transfer
	{
		public: typedef unsigned long long xferid_t ;
		public: enum class XferReason : UINT32 { Login, Update, Relogin, Reassign, Mirror, Exit, MaxReasons } ;

		private: static xferid_t g_xferID ;

		//! Constructor.
		protected: Transfer(XferReason _reason, class ServerInfo* const _destination) ;

		public: virtual ~Transfer() {}

		public: Transfer(XferReason _reason=XferReason::Login)
			: m_xferID(0)
			, m_reason(_reason)
			, m_source(0)
			, m_dest(0)
			, m_timestamp(teulClock64)
			{}

		public: virtual void packer(LOGICAL mode, ACTION_OBJECT* const a)
		{
			a->packer(mode, m_xferID) ;
			a->packer(mode, (UINT32*)&m_reason, 8) ;
			a->packer(mode, m_source) ;
			a->packer(mode, m_dest) ;
		}

		//! Send handler for callbacks.
		public: static SINT32 SendHandler(ACTION_OBJECT* const a)
		{
			Transfer* const transfer = (Transfer*)(a->localRawData) ;
			transfer->packer(TEUL_PACK, a) ;
			return a->offset ;
		}

		//! Called on the receiving process when a transfer arrived.
		public: virtual SINT32 onReceive(SINT32 teulIndex) ;

		//! Called on the originating process when a transfer has
		//! been acknowledged (or declined) by the recipient.
		public: virtual SINT32 onComplete(SINT32 result) const = 0 ;

		protected:
			//! Unique ID for this transfer.
			xferid_t			m_xferID ;
			//! Reason for the xfer.
			XferReason			m_reason ;
			//! Source server id.
			UINT64				m_source ;
			//! Destination server id.
			UINT64				m_dest ;
			//! When the transfer started.
			TeulTimeMS			m_timestamp ;

		public:
			const xferid_t&		xferID() const { return m_xferID ; }
			XferReason			reason() const { return (XferReason)m_reason ; }
			const UINT64&		source() const { return m_source ; }
			const UINT64&		dest() const { return m_dest ; }
			const TeulTimeMS&	timestamp() const { return m_timestamp ; }
	} ;
}

