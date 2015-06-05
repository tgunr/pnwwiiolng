#pragma once

#pragma pack(push, 1)
//! @struct ProcessIntroduction
//! @brief Identification message from a managed process.
//! @detail Tells the controller who is attempting a connection.
struct ProcessIntroduction
{
	uint64_t	m_processID;
	uint32_t	m_processType;
	uint32_t	m_token;
};
#pragma pack(pop)
