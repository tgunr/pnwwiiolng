#pragma once
// Defines the ResupplyIssue structure received from the host.

#pragma pack(push, 1)
struct ResupplyIssue
{
	ResupplyIssue(ACTION_OBJECT* const a) ;

	void process() ;

	UINT32		m_fromHostIndex ;
	ChatHandle	m_fromHandle ;
} ;
#pragma pack(pop)
