#pragma once
// Created: Wed Apr 20 15:29:33 CDT 2005
//
// Chat Channel-describing classes and values
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDefines.h"
#include "chat/chatServerTypes.h"
#include "chat/chatChannelTypes.h"
#include "chat/chatChannelID.h"

namespace Chat
{
	class Channel;
	class ChannelSubcription;
}

class Player;

enum
{
	AREA_CHN = MAX_CLIENT_CHANNELS,
// Insert new "secret" channels here
	MAX_HOST_CHANNELS
};

// Bit mask for the area chat channel
static const UINT32 AREA_CHN_BIT = (1 << AREA_CHN);

namespace Chat
{

//! @brief	Association between a player and a comms channel.
class ChannelSubscription
{
public:
	ChannelSubscription()
		: m_type(NO_CHANNEL)
		, m_channel(NULL)
		, m_subscriber(NULL)
		{}

	class Chat::Channel* channel() const { return m_channel; }
	CHANNEL_TYPE type() const { return m_type; }

	// Channel node can be used to reserve a particular radio slot to a particular radio type
	bool inUse() const { return (m_type > NO_CHANNEL); }
	bool isPlaceHolder() const { return (inUse() && m_channel == NULL); }
	bool isTuned() const { return (inUse() && m_channel != NULL); }

	SINT32 teulIndex() const;

	//! @brief Subscribe to a specific channel.
	public: void subscribe(CHANNEL_TYPE type_, Chat::Channel* channel_);
	//! @brief Unsubscribe from a channel.
	public: void unsubscribe();

	//! What type of channel this subscription is for.
	public:		CHANNEL_TYPE	m_type;				//! The type of channel:
	//! Pointer to the channel this subscription is tuned to.
	public:		Chat::Channel*	m_channel;
	//! Pointer to the player who owns this subscription.
	public:		Player*			m_subscriber;
};

class Channel
{
public:
	Channel(const ChannelID& channelID_, const char* const name_, const ChatHandle& handle_)
		: channelID(channelID_)
		, m_name(name_)
		, m_handle(handle_)
		, m_cpID()
		, m_missionID(0)
		, m_expireAt(0)
		, m_subscribers()
		{
			m_subscribers.clear();
		}

private:
	Channel();
	Channel(const Channel& rhs);

public:
	ChannelID		channelID;
	std::string		m_name;
	ChatHandle		m_handle;

	FULL_OBJ_ID		m_cpID;	//cpID, if associated with a CP
	UINT32			m_missionID;
	TeulTimeMS		m_expireAt;

	public:		typedef std::set<Chat::ChannelSubscription*> Subscribers;

	public:		const Subscribers& subscribers() const { return m_subscribers; }

	friend class Chat::ChannelSubscription;
	//! Add cnode to the subscribers list, does not modify cnode.
	private:	void addSubscriber(const Chat::ChannelSubscription& cnode);
	//! Remove cnode from the subscribers list, does not modify cnode.
	private:	void removeSubscriber(Chat::ChannelSubscription& cnode);

	public:		size_t numSubscribers() const { return m_subscribers.size(); }

	public:		CHANNEL_TYPE type() const { return channelID.Type(); }

	private:	Subscribers		m_subscribers;		// Players tuned to channel.
};

}

