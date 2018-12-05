#pragma once

struct TEUL_CALLBACK;

namespace Teul
{
	//! @class Teul::ConnectionInstruction
	//! @brief Host->Client instruction to open a new connection.
	class ConnectionInstruction
	{
	public:
		enum Role { Unknown, Chat, Cell };

	public:
		static TEUL_CALLBACK* s_callback;

		ConnectionInstruction(const Role role_, const UINT32 hostIP_, const UINT16 port_, const UINT32 otp_)
			: m_role(role_), m_hostIP(hostIP_), m_port(port_), m_otp(otp_)
			{}

		ConnectionInstruction(ACTION_OBJECT* const a);

		//! [host] Send to client.
		SINT32 send(SINT32 teulIndex);

		//! [client] Receive from host.
		SINT32 receive(SINT32 teulIndex);

		SINT32 packer(PackMode mode, ACTION_OBJECT* const a);

	public:
		Role	role() const { return m_role; }
		UINT32	hostIP() const { return m_hostIP; }
		UINT16	port() const { return m_port; }
		UINT32	otp() const { return m_otp; }

	private:
		Role	m_role;			//!< Purpose of the connection.
		UINT32	m_hostIP;			//!< The host's IP.
		UINT16	m_port;			//!< Port to connect on.
		UINT32	m_otp;				//!< One-Time-Password / session ID.
	};
}

