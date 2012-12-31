#include <syscfg.h> // Path is project specific.
// Date: Thu Sep 21 16:54:55 2000
//
// List management for channels
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatDebugConditions.h"

#include <queue>

#include "chat/chatChannelList.h"
#include "chat/chatChannelID.h"
#include "chat/chatChannels.h"
#include "mem/mem.h"
#include "wwii/wwiiHash.h"
#include "chat/chatMissions.h"
#include "wwii/wwiiString.h"

#include "chat/chatPlayer.h"
#include "teul/teulTime.h"

struct ChannelHash
{
	UINT64 operator()(const ChannelID& channelID) const { return channelID.HashValue(); }
};

typedef std::unordered_map<ChannelID, Chat::Channel*, ChannelHash> CHANNEL_INDEX ;
CHANNEL_INDEX _channelIndex ;

static CHANNEL_INDEX _tentativeChannels ;

// Register a new chat channel
Chat::Channel*
chatCreateChannel(const ChannelID& channelID, const char* const name, ChatHandle chHandle)
{
	if ( channelID.IsPlaceHolder() )
	{
		dbugLog(_FLA, "attempting to create placeholder channel - id not properly populated") ;
		return NULL ;
	}

	// Check to see if channel already exists.
	if ( _channelIndex.find(channelID) != _channelIndex.end() )
	{
		LogAbort("ch:%llu.%u.%u.%llu attempting to re-create existing channel"
					, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()) ;
	}

	dbugLog(_FLD, "Creating ch:%llu.%u.%u.%llu %s: %s [%s]", channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID(), chanTypeName[channelID.Type()], name, chatGetCallsign(chHandle));
	Chat::Channel* const channel = new Chat::Channel(channelID, name, chHandle) ;

	_channelIndex[channelID] = channel ;

	switch ( channel->type() )
	{
	// Channels which don't get deleted.
	case BRIGADE_CHANNEL:
	case GCI_CHANNEL:
	case ADMIN_CHANNEL:
	case SIDE_GENERAL_CHANNEL:
	case SIDE_HELP_CHANNEL:
	case SIDE_OPS_CHANNEL:
	break ;
	default:
		_tentativeChannels[channelID] = channel ;
	break ;
	}

	dbugLog(_FLD, "ch:%llu.%u.%u.%llu channel created, numChannels now %"PRSizeT""
						, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
						, _channelIndex.size()) ;

	return channel;
}

//Function deletes a channel from the channel list and removes it
//	from the indexing
bool
chatDestroyChannel(const ChannelID channelID)
{
	//We don't want to delete channels in the PERMANENT section
	switch (channelID.Type())
	{
		// Transient channels
	case MISSION_CHANNEL:
		// Notify the mission tracking system that we have no takers here
		chatMissionChannelEmpty((UINT32)channelID.SubID()) ;
		/* PASSTHRU */
	case SQUAD_CHANNEL:
	case CP_CHANNEL:
	case PRIVATE_CHANNEL:
		break ;
	default:
		// Permanent channels
		return true ;
	}

	CHANNEL_INDEX::iterator it = _channelIndex.find(channelID) ;
	if ( it == _channelIndex.end() )
	{
		dbugLog(_FLI, "ch:%llu.%u.%u.%llu attempting to delete unlisted channel"
						, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()) ;
		return false ;
	}

	_tentativeChannels.erase(channelID) ;

	Chat::Channel* const channel = it->second ;
	_channelIndex.erase(it) ;
	delete channel ;

	dbugLog(_FLI, "ch:%llu.%u.%u.%llu freed channel, numChannels now %zu"
						, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID(), _channelIndex.size()) ;

	return true ;
}

Chat::Channel*
chatFindChannel(const ChannelID& id)
{
	CHANNEL_INDEX::iterator it = _channelIndex.find(id) ;
	return ( it != _channelIndex.end() ) ? it->second : NULL ;
}

//////////////////////////////////////////////////////////////////////
// Delete channels nobody is using.

void
chatDeleteUnusedChannels()
{
	for ( auto it = _tentativeChannels.begin() ; it != _tentativeChannels.end() ; ++it )
	{
		Chat::Channel* const channel = it->second ;
		if ( channel == NULL )
			LogAbort("dead channel in list") ;

		if ( channel->numSubscribers() > 0 )
		{
			if ( channel->m_expireAt > 0 )
				channel->m_expireAt = 0 ;
			continue ;
		}

		const ChannelID channelID = it->first ;

		if ( channel->m_expireAt == 0 )
		{
			static const TeulTimeMS FiveMinutes_ms = (5 * 60) * 1000 ;
			channel->m_expireAt = teulClock64 + FiveMinutes_ms ;
			dbugLog(_FLD, "ch:%llu.%u.%u.%llu marking for expiration"
							, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
							) ;
			continue ;
		}

		if ( channel->m_expireAt > teulClock64 )
		{
			continue ;
		}

		dbugLog(_FLM, "ch:%llu.%u.%u.%llu expired and empty, deleting"
						, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
						) ;
		chatDestroyChannel(channelID) ;

		break ;
	}
}

//////////////////////////////////////////////////////////////////////
// Add a channel node (subscriber) to a channel.

void
Chat::Channel::addSubscriber(const Chat::ChannelSubscription& cnode)
{
	// insert returns pair<interator, bool>.
	const bool inserted = m_subscribers.insert(const_cast<Chat::ChannelSubscription*>(&cnode)).second ;
	// The bool tells us whether the insert occurred or not.
	if ( inserted == true )
	{
		if ( g_debugRadios )
			dbugLog(_FLD, "ch:%llu.%u.%u.%llu added cnode %p to channel %p subscribers now: %zu"
							, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
							, &cnode, this
							, numSubscribers()
							) ;
	}
	else
	{
		dbugLog(_FLA, "ch:%llu.%u.%u.%llu UNABLE TO ADD CNODE %p to channel %p subscribers now: %zu"
						, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
						, &cnode, this
						, numSubscribers()
						) ;
	}
}


//////////////////////////////////////////////////////////////////////
// Remove a channel node (subscriber) from a channel.
// Only called from Chat::ChannelSubscription.unsubscribe().
// NOTE: Does NOT handle clearing out the subscription info from
// the channelnode object.

void
Chat::Channel::removeSubscriber(Chat::ChannelSubscription& cnode)
{
	if ( cnode.m_channel != this )
	{
		LogAbort("cnode %p attempting remove from non-subscribed channel %p!", &cnode, this) ;
	}

	const size_t numRemoved = m_subscribers.erase(&cnode) ;
	if ( g_debugRadios )
	{
		if ( numRemoved > 0 )
		{
			dbugLog(_FLD, "ch:%llu.%u.%u.%llu removed cnode %p from channel %p subscribers now: %zu"
							, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
							, &cnode, this
							, numSubscribers()
							) ;
		}
		else
		{
			dbugLog(_FLW, "ch:%llu.%u.%u.%llu UNABLE TO REMOVE CNODE %p from channel %p subscribers now: %zu"
							, channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID()
							, &cnode, this
							, numSubscribers()
							) ;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Clear out subscription info in a cnode (and detach from channel if
// we are attached).

void
Chat::ChannelSubscription::unsubscribe()
{
	if ( m_channel )
	{
		m_channel->removeSubscriber(*this) ;
	}

	m_type = NO_CHANNEL ;
	m_channel = NULL ;
}

//////////////////////////////////////////////////////////////////////
// Associate a cnode with a channel (i.e. subscribe to it), removing
// any pre-existing subscription information.

void
Chat::ChannelSubscription::subscribe(CHANNEL_TYPE type_, Chat::Channel* channel_)
{
	if ( m_channel != NULL )
		unsubscribe() ;

	this->m_channel = channel_ ;
	this->m_type = type_ ;

	// Channel will be null if this is a placeholder tune.
	if ( m_channel != NULL )
		m_channel->addSubscriber(*this) ;
}

//////////////////////////////////////////////////////////////////////
// Get the teulIndex for a Chat::ChannelSubscription.

SINT32
Chat::ChannelSubscription::teulIndex() const
{
	return m_subscriber->teulIndex() ;
}
