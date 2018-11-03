#include <syscfg.h> // Path is project specific.
// Created: Sun Oct 12 16:28:46 CDT 2008
//
// Members of the standardised host player class.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>
#include "chat/chatLocalizedMessage.h"
#include "wwii/wwiiPlayer.h"
#include "wwii/wwiiExitCodes.h"
#include "wwii/wwiiPermissions.h"
#include "wwii/wwiiThreadTypes.h"
#include "wwii/wwiiTypes.h"				// For PEXIT_DEAD
#include "inst/instHost.h"
#include "str/stringLib.h"
#include "server/server.h"				// For serverReleasePlayer
#include "teul/teulBasic.h"				// For teulClose
#include "mem/mem.h"

// Allocate in blocks of 64 at a time.
static const UINT32 _CALLOC_RATE = 64;
// How fast to grow the pool indexes (to avoid allocations)
static const UINT32 _POOL_GROWTH_RATE = 1024;
// Rounding size (how big each allocation is rounded up to)
static const size_t _ROUNDING = 1023;
// Maximum number of pools to have (max size of a player)
static const UINT32 _MAX_POOLS = 3;

static std::mutex allocMutex;
using StorageHeader = UINT64;
using Pool = std::vector<UINT64*>;
static UINT32 _callocs = 0;
static UINT32 _reserved = 0;
static UINT32 _inUse = 0;
static UINT32 _news = 0;
static UINT32 _frees = 0;
// Pools are ONE BASED, i.e. pools[0] is actually the
// pool for objects with a non-zero size.
static Pool pools[_MAX_POOLS];

// Callback registration for sending localized messages.
namespace Chat
{
	TEUL_CALLBACK*	_SEND_LOCALIZED_MESSAGE;
}

//////////////////////////////////////////////////////////////////////
// Normalize the per-object size into a standardized object allocation
// size based on _ROUNDING. Sizes are rounded up.

static inline size_t
_normalizeObjectSize(size_t actualSize)
{
	assert( actualSize > 0 );
	return (actualSize + sizeof(StorageHeader) + _ROUNDING) & ~_ROUNDING;
}

//////////////////////////////////////////////////////////////////////
// Given an allocation size, return which pool to use.

static inline UINT32
_getPoolNo(size_t allocSize)
{
	assert( (allocSize & _ROUNDING) == 0 );

	UINT32 poolNo = (UINT32)(allocSize / (_ROUNDING + 1));
	// This will always be > 1. "pools" is 1 based so we
	// must subtract one to get the poolNo
	poolNo = poolNo - 1;

	assert( poolNo < _MAX_POOLS );

	return poolNo;
}

//////////////////////////////////////////////////////////////////////
// Custom allocator for wwiiPlayer derived objects.
// Sizes are rounded up into pages based on _ROUNDING, this helps
// avoid memory fragmentation but also tends to group different
// types into one memory space.
// Free objects are kept on our own stack to tend to re-use the
// same memory space and reduce system alloc calls, and are also
// allocated _CALLOC_RATE at a time to further reduce alloc
// calls.
// Allocations are made with calloc and re-used objects are auto-
// matically memset to ensure the constructor starts with an
// all zeros memory space.
// A mutex is used to make this thread safe.

void*
wwiiPlayer::operator new(size_t actualSize)
{
	// Because wwiiPlayer is intended to be a base class, we
	// need to be aware that descendants may have different
	// sizes; by normalizing the sizes to significant
	// boundaries we can avoid pool fragmentation by sharing
	// the same blocks for different sized structs. So I use
	// multiple pools indexed by (normalized size / boundary)
	size_t allocSize = _normalizeObjectSize(actualSize);
	UINT32 poolNo = _getPoolNo(allocSize);

	Pool& pool = pools[poolNo];

	StorageHeader* ptr = nullptr;

	// We'll need a lock.
	{
		std::lock_guard<std::mutex> lock(allocMutex);

		// When the pool is empty, allocate a batch of objects.
		if (pool.empty())
		{
			if (pool.capacity() < _POOL_GROWTH_RATE)
				pool.reserve(_POOL_GROWTH_RATE);

			// calloc: so that we have pre-zero'd memory.
			void* block = memCalloc(_CALLOC_RATE, allocSize);
			_callocs++;
			if (block == nullptr)
				throw std::bad_alloc();

			_reserved += _CALLOC_RATE;

			// Now we need to split this block up into units
			// and feed them backwards onto the pool
			for (UINT32 i = 1; i <= _CALLOC_RATE; ++i)
			{
				StorageHeader* alloc = (StorageHeader*)((char*)block + (_CALLOC_RATE - i) * allocSize);
				pool.push_back(alloc);
			}
		}

		// Grab the last entry off the vector
		ptr = pool.back();
		pool.pop_back();

		// Adjust counters
		_reserved--;
		_inUse++;
		_news++;
	}

	assert( ptr != nullptr );

	if ( *ptr != 0 )
	{
		// Initialized, so it needs to be cleaned
		memset(ptr, 0, allocSize);
	}
	*ptr = allocSize;

	dbugLog(_FLD, "+player[%" PRSizeT " => pool #%u:%" PRSizeT "] => %p", actualSize, poolNo, allocSize, ptr);

	// Return the address of data after StorageHeader
	return ++ptr;
}

//////////////////////////////////////////////////////////////////////
// Custom delete operator puts the objects back into the apporipate

void
wwiiPlayer::operator delete(void* p)
{
	// Each player allocation actually carries a secret "size" value.
	StorageHeader* ptr = static_cast<StorageHeader*>(p) - 1;

	// Make sure storage header is good data
	UINT32 poolNo = _getPoolNo((size_t)*ptr);
	Pool& pool = pools[poolNo];

	dbugLog(_FLD, "-player[%" PRSizeT " => pool #%u:%" PRSizeT "] => %p", (size_t)*ptr, poolNo, (size_t)*ptr, ptr);

	size_t newSize = 0;

	{
		std::lock_guard<std::mutex> lock(allocMutex);

		// We're going to push this onto the top of the
		// pool, so check if this would push the pool
		// over it's capacity and grow it by a suitable
		// increment in advance.
		if ( pool.size() + 1 >= pool.capacity() )
		{
			newSize = pool.capacity() + _POOL_GROWTH_RATE;
			pool.reserve(newSize);
		}

		// Push it onto the back of the pool
		pool.push_back(ptr);

		// Adjust counters
		_reserved++;
		_inUse--;
		_frees++;
	}

	if ( newSize > 0 )
	{
		dbugLog(_FLD, "playerPool#%u grown to %" PRSizeT " spaces", poolNo, newSize);
	}
}

//////////////////////////////////////////////////////////////////////
// Log stats on the player allocation system

void
wwiiPlayer::logStats()
{
	std::lock_guard<std::mutex> lock(allocMutex);
	dbugLog(_FLN, "callocs:%u news:%u frees:%u reserved:%u inUse:%u"
					, _callocs
					, _news, _frees
					, _reserved, _inUse
					);
}

//////////////////////////////////////////////////////////////////////
// Copy operator.

wwiiPlayer&
wwiiPlayer::operator=(const wwiiPlayer* const rhs)
{
	m_hostInstance = nullptr;
	m_instance = Instance::g_invalid;

	m_teulIndex = -EINVAL;
	m_playerID = rhs->m_playerID;
	m_personaID = rhs->m_personaID;
	m_squadID = rhs->m_squadID;
	m_brigadeID = rhs->m_brigadeID;
	m_missionID = rhs->m_missionID;
	m_sortieID = rhs->m_sortieID;
	m_side = WWII_NO_SIDE;
	m_country = WWII_NO_COUNTRY;
	m_branch = rhs->m_branch;
	m_maxRank = rhs->m_maxRank;
	m_maxSquadRank = rhs->m_maxSquadRank;
	m_realRank = rhs->m_realRank;
	m_realSquadRank = rhs->m_realSquadRank;
	m_weaponSpecNo = rhs->m_weaponSpecNo;
	m_chatHandle = rhs->m_chatHandle;
	m_matchHandle = rhs->m_matchHandle;
	m_squadHandle = rhs->m_squadHandle;
	m_permissionMask = rhs->m_permissionMask;
	// Do not copy m_instance
	// Do not copy m_hostInstance
	m_flags = rhs->m_flags;
	m_dispatch = rhs->m_dispatch;

	SetCountry(rhs->m_country, rhs->m_side);
	SetInstance(rhs->m_instance);

	return *this;
}

//////////////////////////////////////////////////////////////////////
// Populate our dispatch from another

void
wwiiPlayer::SetDispatch(const Auth::Dispatch& rhs, UINT32 overridingCustomerID)
{
	m_dispatch.m_customerID = (overridingCustomerID) ? overridingCustomerID : rhs.m_customerID;
	m_dispatch.m_OTP = rhs.m_OTP;
	m_dispatch.m_clientIP = rhs.m_clientIP;
	m_dispatch.m_appVersion = rhs.m_appVersion;
	m_dispatch.m_platform = rhs.m_platform;
	strsafecpy(m_dispatch.m_callsign, rhs.m_callsign);
}

//////////////////////////////////////////////////////////////////////
// Log a message in the name and context of this player

void
wwiiPlayer::log(const char* msg) const
{
	dbugLog(_FLD, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " %s"
					, teulIndex(), playerID(), callsign()
					, instance().category(), instance().type(), instance().handle(), instance().id()
					, msg);
}

void
wwiiPlayer::log(const char* file, int lineno, unsigned int errType, const char* mask, ...) const
{
	static char buf[512];

	va_list args;
	va_start(args, mask);
	vsnprintf(buf, 511, mask, args);
	va_end(args);

	dbugLog(file, lineno, errType
				, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " %s"
				, teulIndex(), playerID(), callsign()
				, instance().category(), instance().type(), instance().handle(), instance().id()
				, buf);
}

//////////////////////////////////////////////////////////////////////
// Does this player effectively have GM privileges.

bool
wwiiPlayer::isEffectiveGM() const
{
#if defined(TRAINING_SERVER) || defined(BETA_RELEASE)
	return permissions(IS_PLAYNET | GAME_MANAGER | GAME_TRAINER);
#else
	return permissions(IS_PLAYNET | GAME_MANAGER);
#endif
}

//////////////////////////////////////////////////////////////////////
// Test if the player does not currently have a valid side.

bool
wwiiPlayer::noSide() const
{
	return false == isValidSide(side());
}

//////////////////////////////////////////////////////////////////////
// Set the player's callsign

void
wwiiPlayer::_setCallsign(const char* to, bool isPlayerPlaynet)
{
	m_matchHandle = chatGetLowercaseHandle(to);
	memset(m_dispatch.m_callsign, 0, sizeof(m_dispatch.m_callsign));
	chatGetCallsign(m_dispatch.m_callsign, m_matchHandle);
	if ( isPlayerPlaynet )
		strupr(m_dispatch.m_callsign);
	else
		strlwr(m_dispatch.m_callsign);
	m_chatHandle = chatGetHandle(m_dispatch.m_callsign);
}

//////////////////////////////////////////////////////////////////////
// Send a player to the exit server

void
wwiiPlayer::Release(SINT32 onTeulIndex, Server::ReleaseType releaseType)
{
	bool closeRequired = (onTeulIndex >= 0 && onTeulIndex == m_teulIndex);
	this->m_teulIndex = -1;
	serverReleasePlayer(onTeulIndex, this->playerID(), this->OTP(), PEXIT_DEAD, releaseType);

	if ( closeRequired )
		teulClose(onTeulIndex);
}

//////////////////////////////////////////////////////////////////////
// Return true if the persona is an acting high commander

bool
wwiiPlayer::isOfficer() const
{
	if ( side() == ALLIED )
		return permissions(ALLIED_COMMAND_STAFF);
	else if ( side() == AXIS )
		return permissions(AXIS_COMMAND_STAFF);
	return false;
}

