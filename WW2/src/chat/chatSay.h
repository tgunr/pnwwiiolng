#pragma once
// Copyright (C) PlayNet Inc 2004 - All Rights Reserved

#include "chat/chatPrototypes.h"

enum AREA_CHAT_MODE { AREA_SAY, AREA_YELL, AREA_WHISPER, AREA_GM };

const char* chatForeignizeMessage(WWII_COUNTRY country, size_t maxLen);

void chatSay(Player* player, Chat::ClientMessage* message, char* phrase, AREA_CHAT_MODE mode, bool allowEchoToClient);
