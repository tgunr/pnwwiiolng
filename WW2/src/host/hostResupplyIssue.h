#pragma once

#pragma pack(push, 1)
struct HostResupplyIssue
{
	HostResupplyIssue(const sortieid_t toSerial, const sortieid_t fromSerial, const ChatHandle fromHandle)
		: m_toSerial(toSerial), m_fromSerial(fromSerial), m_fromHandle(fromHandle) {}

	sortieid_t	m_toSerial;
	sortieid_t	m_fromSerial;
	ChatHandle	m_fromHandle;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct HostResupplyAccepted
{
	HostResupplyAccepted(const sortieid_t supplierID, const ChatHandle recipient)
		: m_supplierID(supplierID), m_recipient(recipient)
		{}

	sortieid_t	m_supplierID;			// ID of the vehicle that gave supply
	ChatHandle	m_recipient;			// Player that received it
};
#pragma pack(pop)

