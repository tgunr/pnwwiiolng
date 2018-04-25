#pragma once
// Created: Sun Sep 25 02:35:11 CDT 2005
//
// Describes the structure for tracking reports
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"
#include "weap/weapSpec.h"

class ChatReport
{
public:
	// How long the text strings can be
	static const UINT32 MAX_TEXT_LENGTH = 400;

	// Categories
	enum { INVALID = 0 };
	enum { BUG = 1, GAMEPLAY, TERRAIN, IDEA, PLAYER, SPEED_HACK, CAPTURE_HACK, CLIENT_HACK };
	static const UINT32 MAX_CLIENT_CATEGORY = PLAYER;

	// Satuses ...
	// Submission = this is a new entry for the host, client should ignore
	// New: Posted and not viewed by a GM
	// Opened: A GM has viewed it and conversed with customer
	// Closed: GM has dealt with it
	// Escalating: GM has viewed it and needs a senior GM/staff to view it
	// Escalated: Opened -> Escalating -> Snr GM/Staff has taken it up
	// Attention_CRS: GMs have forwarded it for staff review
	enum { SUBMISSION = 0, NEW, OPENED, CLOSED, ESCALATING, ESCALATED, ATTENTION_CRS };

public:
	// Con-/de-structors
	ChatReport(UINT16 category=0, const std::string& report="", const ChatHandle offender=0LL);
	ChatReport(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }
	~ChatReport() {}

public:
	// Transport methods
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

public:
	// Unique ID assigned by the host
	UINT32			m_hostID;
	// Time stamp of report
	UINT32			m_time;
	// Category of the report
	UINT16			m_category;
	// Current status of the report
	UINT16			m_status;
	// Name of the poster and the report subject (if its a player)
	ChatHandle		m_poster;
	// If this is a vs-player complaint, who are they complaining about
	ChatHandle		m_offender;
	// 0 or the packed ID of the GM handling this
	ChatHandle		m_assigned;
	// What country they were
	UINT16			m_country;
	// What branch they were in
	UINT16			m_branch;
	// The HCUnit they were in
	HCUnitID		m_brigade;
	// ID of their mission, if any
	UINT32			m_mission;
	// If they were leading the mission
	bool			m_leader;
	// Vehicle they are or were in
	WEAPON_SPEC		m_weapon;
	// Sortie ID if any
	UINT32			m_sortie;
	// If they were spawned
	bool			m_spawned;
	// Current or last x/y/z co-ords
	SINT32			m_x, m_y, m_z;
	// Most importantly
	std::string		m_report;	// Upto 400 characters of report
	// Space for GMs to comment
	std::string		m_notes;
};

using CHAT_REPORT = teul_network_object<ChatReport>;
