#include <syscfg.h> // Path is project specific.
// Date: Wed Jun  8 11:30:09 CDT 2005
//
// Functions for handling area chat
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "chat/chatClientMessage.h"
#include "chat/chatGridTypes.h"
#include "chat/chatHandle.h"
#include "chat/chatHostMessage.h"
#include "chat/chatIcons.h"
#include "chat/chatMissionMember.h"
#include "chat/chatPlayer.h"
#include "chat/chatSay.h"
#include "chat/chatTypes.h"
#include "wwii/wwiiConfig.h"

// Definition of all the little word sounds we have for
// each country. Note: Pirate is MAX_COUNTRIES;)

static const char* g_fsNoCountry[] =
{	// NO_COUNTRY
	"wa", "ki", "chi", "doh", "dia", "ma",
	"kre", "hun", "lo", "hum", "mo", "pff",
	"thpo", "mak", "na", "kik", "chak", "zik",
	"mipt", "niha", "mah", "buh", "rahka",
	nullptr
};

static const char* g_fsEnglish[] =
{	// English
	"ay", "ing", "ee", "lee", "tee", "on", "oh",
	"abs", "o", "of", "or", "bli", "beg", "bug",
	"dam", "right", "ard", "day", "yes", "no",
	"oi", "and", "so", "not", "ter", "rot",
	"lad", "broke", "n", "but", "we", "err",
	"el", "off", "od", "ush", "bum", "you",
	"mum", "got", "last", "if", "well", "ham",
	"why", "lot", "lit", "nut", "daft", "yer",
	"wot", "stif", "ten", "har", "lot", "beer",
	"well", "gud", "wiv", "blud", "mo", "nade",
	"gun", "shot", "me", "elp", "my", "a", "cig",
	"vick", "bof", "tuf", "nar", "wor", "ers",
	"mar", "ek", "un", "ole", "oke", "ad",
	nullptr
};

static const char* g_fsAmerican[] =
{	// American
	nullptr
};

static const char* g_fsFrench[] =
{	// French
	"le", "la", "du", "su", "c'e", "sa", "va",
	"en", "fa", "eu", "bien", "mais", "et",
	"sur", "au", "les", "mes", "son", "vive",
	"cher", "oui", "mer", "de", "cour", "pier",
	"plus", "av", "ees", "cor", "ez", "tohn",
	"avec", "cha", "d'", "ou", "con", "sans",
    "mange", "jou", "dit", "chou", "peti", "gran",
	"vin", "pon", "shem", "vil", "non", "fais",
	"dieu", "nest", "pistol", "aide", "moi",
	"donner", "mon", "tir", "clope", "pinard",
	"jamais", "dans", "plus", "mama",
	nullptr
};

static const char* g_fsGerman[] =
{	// German
	"der", "das", "zi", "und", "er", "ist",
	"boten", "cig", "etten", "hans", "zer",
	"scho", "haf", "ein", "rras", "vur",
	"tot", "st", "mit", "frau", "en", "ja",
	"var", "dt", "zo", "ab", "lich", "oss",
	"mut", "gar", "nich", "ich", "bier", "vayg",
	"dus", "darf", "haupt", "kalt", "han",
	"de", "ho", "dein", "mein", "aber", "geb",
	"leid", "uber", "den", "lauf", "fewr", "ver",
	"hinter", "heiss", "frei", "hass", "lich",
	"ja", "nein", "rass", "mench", "ruk", "tee",
    "vas", "fan", "tas", "tich", "feer", "von",
	"reis", "hand", "hoch", "grenaten", "schuss",
	"hilf", "gevare", "rauch", "tsum", "maus",
	"luger", "mess", "sch", "viss", "haff", "ink",
	nullptr
};

static const char* g_fsItalian[] =
{	// Italian
	nullptr
};

static const char* g_fsChina[] =
{	// China
	nullptr
};

static const char* g_fsRussia[] =
{	// Russia
	nullptr
};

static const char* g_fsPirate[] =
{	// MAX_COUNTRIES -- PIRATE! -- AKA -- PiRAT;)
	"aye", "arrh", "matey", "grog", "ahoy", "oi",
	"land", "lubber", "argh", "avast", "piecesofeight",
	"ye", "der", "shiver", "timbers", "me",
	"plank", "scurvy", "dog", "rum", "chest", "off",
	"doubloons", "mighty", "yo", "ho", "hoho",
	"baytah", "teh", "doh", "twowiks", "gold", "thar",
	nullptr
};

static int g_foreignCounts[MAX_COUNTRIES+1];
static const char** g_foreignParts[MAX_COUNTRIES+1] = {
 g_fsNoCountry, g_fsEnglish, g_fsAmerican, g_fsFrench,
 g_fsGerman, g_fsItalian, g_fsChina, g_fsRussia,
 g_fsPirate
};

// say function stuff
bool _enablePirate = false;

// Make sure that the foreign sound list is populated
static inline void
_setupForeignSounds()
{
	static bool initialized = false;
	if ( initialized )
		return;
	for ( int country = 0; country <= MAX_COUNTRIES; ++country )
	{
		int sound = 0;
		while ( g_foreignParts[country][sound] )
			++sound;
		g_foreignCounts[country] = sound;
	}
}

//////////////////////////////////////////////////////////////////////
// Clone the outgoing friendly message if we're able to talk
// to hostiles
static inline void
_populateEnemyMessage(Chat::HostMessage& enemyMsg, const Chat::HostMessage& friendlyMsg, WWII_COUNTRY country, char* phrase)
{
	memcpy(&enemyMsg, &friendlyMsg, sizeof(friendlyMsg));
	enemyMsg.icon = Chat::ENEMY_SAY_ICON;

#if !defined(TRAINING_SERVER) && !defined(FRES)
	static const size_t MAX_FOREIGN_STRING_LEN = 80;		// Lets not over-do them or allow spamming
	enemyMsg.msg = chatForeignizeMessage(country, strnlen(phrase, MAX_FOREIGN_STRING_LEN));
	enemyMsg.length = (UINT32)strnlen(enemyMsg.msg, MAX_MESSAGE_LENGTH);
	enemyMsg.handle = chatGetHandle("Enemy");
#endif
}

//////////////////////////////////////////////////////////////////////
// Detect whether the remote player is within the friendly hearing
// range of the sender
static inline bool
_inRange(const Player* observer, const Player* speaker, SINT64 range2)
{
	FLOAT64 dX = observer->currentX() - speaker->lastX();
	FLOAT64 dY = observer->currentY() - speaker->lastY();
	FLOAT64 dZ = (observer->currentZ() - speaker->lastZ()) * 2;

	SINT64 dist2 = (SINT64)(dX*dX + dY*dY + dZ*dZ);

	return (dist2 <= range2);
}

//////////////////////////////////////////////////////////////////////
// Populate the standard fields for a "say" message
static inline void
_populateSayMessage(Chat::HostMessage& outMsg
					, const Player* player
					, const Chat::ClientMessage* message
					, char* phrase
					, UINT32 icon
					)
{
	outMsg.handle = player->chatHandle();
	outMsg.id = message->m_id;
	outMsg.isDotMessage = false;
	outMsg.auxChanType = 0;
	outMsg.radioType[0] = 0;
	outMsg.radioType[1] = 0;
	outMsg.radioType[2] = 0;
	outMsg.radioType[3] = 0;
	outMsg.icon = icon;

	outMsg.msg = phrase;
	outMsg.length = (UINT32)strnlen(phrase, MAX_MESSAGE_LENGTH);
}

//////////////////////////////////////////////////////////////////////
// Convert an incoming string into outgoing jibberish;)
const char*
chatForeignizeMessage(WWII_COUNTRY country, size_t maxLen)
{
	static char text[MAX_MESSAGE_LENGTH+1];
	const char** parts = nullptr;
	int selection = 0;
	size_t destOff = 0;
	const char* selected;

	_setupForeignSounds();

	// If pirate mode is enabled, use MAX_COUNTRIES
	if ( _enablePirate )
		country = MAX_COUNTRIES;

	text[0] = 0;
	// If we have no words for your country, use NO_COUNTRY
	if ( g_foreignCounts[country] == 0 )
		country = WWII_NO_COUNTRY;

	parts = g_foreignParts[country];
	if ( parts == nullptr )
	{
		dbugLog(_FLI, "NULL POINTER for g_foreignParts[%d]", country);
		return text;
	}

	if ( maxLen > 4 && maxLen < 40 )
		maxLen += rand() % 8;

	text[destOff] = 0;
	char prev = 0;

	do
	{
		// Choose a random sylable to use
		selection = rand() % g_foreignCounts[country];
		selected = parts[selection];
		while ( *selected )
			prev = text[destOff++] = *(selected++);

		int symRate, spaceChance;
		switch (country)
		{
			case WWII_GERMANY: symRate = 6, spaceChance = 5; break;
			case WWII_FRANCE: symRate = 4, spaceChance = 6; break;
			case MAX_COUNTRIES: symRate = 1, spaceChance = 9; break;
			default: symRate = 3, spaceChance = 9; break;
		}

		// And maybe some dressing
		if ( prev != ' ' && prev != '\'' && rand() % (2+symRate) == 1 )
		{
			switch (rand() % (10+spaceChance))	// 10 being number of cased symbols below
			{
				case 0: case 1: text[destOff++] = '?'; break;
				case 2: text[destOff++] = ','; break;
				case 4: case 5: text[destOff++] = '.';; break;
				case 6: text[destOff++] = '.'; text[destOff++] = '.'; text[destOff++] = '.'; prev = ' '; break;
				case 7: text[destOff++] = '-'; prev = ' '; break;
				case 3: case 9: case 10: text[destOff++] = '!'; break;
				default: break;
			}
			if ( prev != ' ' )
				prev = text[destOff++] = ' ';
		}
	}
	while ( destOff < maxLen );

	text[destOff] = 0;
	return text;
}

//////////////////////////////////////////////////////////////////////

void
chatSay(Player* player, Chat::ClientMessage* message, char* phrase, AREA_CHAT_MODE mode, bool allowEchoToClient)
{
	if ( !player->isSpawned() || player->currentX() == INVALID_GRID_COORD )
	{
		player->message(LSTR_CAUTION, LSTR_AREA_ONLY_WHILE_SPAWNED);
		return;
	}

	UINT32 icon;
	SINT64 friendlyRange2, hostileRange2;
	bool isGMMessage = false;

	// Start by getting the unsquared value, which we'll need in a moment
	switch (mode)
	{
	case AREA_GM:
		isGMMessage = true;
		/* FALLTHROUGH */
	case AREA_YELL:
		friendlyRange2 = host_config.yellFriendlyRange;
		hostileRange2 = host_config.yellEnemyRange;
		icon = Chat::YELL_ICON;
	break;
	case AREA_WHISPER:
		friendlyRange2 = host_config.whisperFriendlyRange;
		hostileRange2 = host_config.whisperEnemyRange;
		icon = Chat::WHISPER_ICON;
	break;
	default:
		friendlyRange2 = host_config.sayFriendlyRange;
		hostileRange2 = host_config.sayEnemyRange;
		icon = Chat::SAY_ICON;
	break;
	}

	// Find which cells we need to transmit to for this guy
	static PLAYERS_ARRAY array;
	chatGridGetPlayersVector(player, std::max(friendlyRange2, hostileRange2), array);
	if ( array.empty() )
		return;

	// Square the values so we don't have to perform sqrt
	// every time we want to test distances
	friendlyRange2 *= friendlyRange2;
	hostileRange2 *= hostileRange2;

	static Chat::HostMessage outMsg;
	static Chat::HostMessage enemyMsg;

	_populateSayMessage(outMsg, player, message, phrase, icon);
	enemyMsg.msg = nullptr;

	Player* cur;

	for ( auto it = array.begin(), end = array.end(); it != end; ++it )
	{
		PLAYERS::iterator cit = (*it)->begin();
		while ( cit != (*it)->end() )
		{
			cur = static_cast<Player*>(*cit);
			++cit;

			if ( cur->playerID() <= 0 )
				continue;

			// Don't echo to myself unless the caller wants this behavior
			if ( allowEchoToClient == false && cur == player )
				continue;

			// In a grab conversation
			if ( cur->inGrab() == true )
				continue;

			// Not finished spawning (or maybe despawned)
			// or already seen this message (e.g. via another channel)
			if ( cur->currentX() == INVALID_GRID_COORD )
				continue;

			if ( cur->side() != player->side() && isGMMessage == false && cur->isBuzzard() == false )
			{
				// Target has turned off enemy chat receipt
				if ( cur->noEnemyChat() )
					continue;

				if ( _inRange(cur, player, hostileRange2) == false )
					continue;

				// If this is our first enemy, the message isn't populated yet.
				if ( enemyMsg.msg == nullptr )
					_populateEnemyMessage(enemyMsg, outMsg, player->country(), phrase);

				cur->send(enemyMsg);

				continue;
			}

			if ( _inRange(cur, player, friendlyRange2) == false )
				continue;

			cur->trySend(outMsg);
		}
	}
}

void
Player::Emit(const Chat::LocalizedMessage& lmsg)
{
	if ( !isSpawned() || currentX() == INVALID_GRID_COORD )
		return;

	SINT64 friendlyRange2 = host_config.sayFriendlyRange;
	friendlyRange2 = friendlyRange2 * friendlyRange2;

	static PLAYERS_ARRAY array;
	chatGridGetPlayersVector(this, friendlyRange2, array);

	Player* cur;
	for ( auto it = array.begin(), end = array.end(); it != end; ++it )
	{
		auto cit = (*it)->begin();
		while ( cit != (*it)->end() )
		{
			cur = static_cast<Player*>(*cit);
			++cit;

			if ( cur->playerID() <= 0 )
				continue;

			// In a grab conversation
			if ( cur->inGrab() == true )
				continue;

			// Not finished spawning (or maybe despawned)
			// or already seen this message (e.g. via another channel)
			if ( cur->currentX() == INVALID_GRID_COORD )
				continue;

			if ( cur->side() != side() )
				continue;

			if ( _inRange(cur, this, friendlyRange2) == false )
				continue;

			cur->message(lmsg);
		}
	}
}

