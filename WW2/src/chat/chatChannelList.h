#pragma once
// Created: Thu Sep 21 16:54:24 2000
//
// List management stuff for channels
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

class ChannelID;

namespace Chat { class Channel; }

Chat::Channel* chatCreateChannel(const ChannelID& channelID, const char* name, ChatHandle chHandle);
Chat::Channel* chatFindChannel(const ChannelID& id);
bool chatDestroyChannel(const ChannelID id);

