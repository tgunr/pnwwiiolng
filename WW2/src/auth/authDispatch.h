#pragma once
// Created: Tue Jul 31 15:19:46 CDT 2007
// Copyright (C) PlayNet, Inc 2007-2011 - All rights reserved.
// Not for redistribution.

/// @class Auth::Dispatch
/// @brief Authentication token used between auth and game hosts to describe a player session.
/// This is basically the auth 'cookie' describing that a particular customer has been
/// granted a particular OTP on a particular IP address and which game version/platform
/// that was granted on.

#include "str/stringLib.h"

namespace Auth
{
	struct Dispatch
	{
		//! @brief Default constructor.
		Dispatch()
			: m_customerID(0), m_OTP(0), m_clientIP(0), m_appVersion(0), m_platform(0)
			{
				m_callsign[0] = 0;
			}
		//! @brief Initializing constructor.
		Dispatch(playerid_t _playerID, UINT32 _otp, UINT32 _ip, UINT32 _version, UCHAR _plat, const char* const _gamename)
			: m_customerID(_playerID), m_OTP(_otp), m_clientIP(_ip), m_appVersion(_version), m_platform(_plat)
			{
				strsafecpy(m_callsign, _gamename);
			}

		bool operator == (const Auth::Dispatch& rhs) const
		{
			return	( m_customerID == rhs.m_customerID
					&& m_OTP == rhs.m_OTP
					&& m_clientIP == rhs.m_clientIP
					&& m_appVersion == rhs.m_appVersion
					&& m_platform == rhs.m_platform
					&& strcmp(m_callsign, rhs.m_callsign) == 0
					);
		}

		UINT32	m_customerID;					//!< The ID of the customer coming in
		UINT32	m_OTP;							//!< The one time password for the customer
		UINT32	m_clientIP;						//!< The IP address from where they'll come
		UINT32	m_appVersion;					//!< Version of the app in use
		UCHAR	m_platform;					//!< Win/Mac
		char	m_callsign[MAX_HANDLE_SIZE];	//!< The game name of the customer coming in

		void packer(unsigned int mode, ACTION_OBJECT* a);
	};
}

