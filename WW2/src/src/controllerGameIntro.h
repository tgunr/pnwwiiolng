#pragma once

#pragma pack(push, 1)
//! @struct ProcessIntroduction
//! @brief Identification message from a managed process.
//! @detail Tells the controller who is attempting a connection.
struct ProcessIntroduction
{
	UINT64	m_processID;
	UINT32	m_processType;
};
#pragma pack(pop)
