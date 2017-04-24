#pragma once
// Created: Thu Dec 18 17:49:08 CST 2008
// Copyright (C) PlayNet, Inc 2008-2011 - All rights reserved.
// Not for redistribution.
//
// Data about a particular client connected to the auth host.
//
// Originating Programmer: Oliver "kfsone" Smith

struct SERVER;
struct APP_LAUNCH_REQ;

namespace Auth
{

struct ClientData
{
	ClientData(SINT32 _teulIndex, UINT32 _platform, UINT32 _address
				, UINT32 _oVersion, const char* _username, const char* _password)
		: server(nullptr)
		, teulIndex(_teulIndex)
		, platform(_platform)
		, address(_address)
		, oVersion(_oVersion)
		, steamID(0)
		, customerID(0)
		, arenaID(0)
		, sessionID(0)
		, appID(0)
		, appVersion(0)
		, m_username(_username), m_password(_password)
		, m_subscribedArenaIDs()
		, m_launchRequest(nullptr)
		, isPlaynet(false)
		, isPremium(false)
		, isQueued(false)
		{}

	ClientData(SINT32 _teulIndex, UINT32 _platform, UINT32 _address
				, UINT32 _oVersion, UINT32 _steamID)
		: server(nullptr)
		, teulIndex(_teulIndex)
		, platform(_platform)
		, address(_address)
		, oVersion(_oVersion)
		, steamID(_steamID)
		, customerID(0)
		, arenaID(0)
		, sessionID(0)
		, appID(0)
		, appVersion(0)
		, m_username(""), m_password("")
		, m_subscribedArenaIDs()
		, m_launchRequest(nullptr)
		, isPlaynet(false)
		, isPremium(false)
		, isQueued(false)
		{}

	~ClientData();

public:
	void AddToQueue(const APP_LAUNCH_REQ& request);
	void RemoveFromQueue(bool enteredGame);

public:
	SERVER*	server;
	SINT32	teulIndex;
	UINT32	platform;
	UINT32	address;			// address of the remote host
	UINT32	oVersion;
	SINT32	steamID;			// steam ID of client
	SINT32	customerID;			// customer ID of client
	SINT32	arenaID;
	UINT32	sessionID;
	SINT32	appID;				//Application ID
	UINT32	appVersion;			//Min Version of the app required
	std::string	m_username;
	std::string	m_password;
	std::vector<UCHAR> m_subscribedArenaIDs;	// Arenas this user has access to
	APP_LAUNCH_REQ* m_launchRequest;
	bool	isQueued : 1;		// Player is queued
	bool	isPlaynet;		// Staff account?
	bool	isPremium;		// Paying subscription?
};

}
