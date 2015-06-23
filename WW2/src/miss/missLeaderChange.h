#pragma once
// Created: Fri Dec  9 16:45:22 CST 2005
//
// Leadership change request/response
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#pragma pack(push, 1)
class LeaderChange
{
public:
	enum Status
	{
		_SUCCESS
	,	_FAIL_NO_MISSION
	,	_FAIL_IS_LEADER
	,	_FAIL_OTHER_MISSION
	,	_SET_BACKUP_LEADER
	};
	LeaderChange(UINT32 missionID=0, UINT32 leaderID=0, const ChatHandle& leaderHandle=0LL, const ChatHandle& requester=0LL, Status result=_SUCCESS)
		: m_missionID(missionID), m_leaderID(leaderID), m_leaderHandle(leaderHandle), m_requester(requester), m_result(_SUCCESS) {}
	~LeaderChange() {}

public:
	UINT32 process() const;

public:
	UINT32		m_missionID;
	UINT32		m_leaderID;
	ChatHandle	m_leaderHandle;
	ChatHandle	m_requester;
	UINT32		m_result;
};
#pragma pack(pop)

